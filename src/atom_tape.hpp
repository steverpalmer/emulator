// atom_tape.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_TAPE_HPP_
#define ATOM_TAPE_HPP_

#include <fstream>

#include "common.hpp"
#include "memory.hpp"
#include "cpu.hpp"

namespace Atom
{

    class Filename:
        private NonCopyable
    {
    private:
        Filename();
        Glib::ustring path_result;
    public:
        char cstr[14];
        Filename(word addr, Memory &memory)
            {
                // string is located in memory at address addr,
                // No more that 14 characters long
                // terminated by a #0D
                int i(0);
                enum {More, EndOfFilename, EndOfBuffer} state(More);
                do
                {
                    cstr[i] = memory.get_byte(addr+i);
                    if (cstr[i] == '\x0D')
                        state = EndOfFilename;
                    else if (++i == 14)
                        state = EndOfBuffer;
                }
                while (state == More);
                switch (state)
                {
                case EndOfFilename:
                    cstr[i] = 0;
                    // FIXME: filename remapping for the OS
                    break;
                default:
                    cstr[0] = 0;
                }
            }
        const char *path(const Glib::ustring &directory)
            {
                path_result = directory;
                path_result += '/';
                path_result += cstr;
                return path_result.c_str();
            }
    };

    class Tape
        : public Device
    {
    private:
        class OSSAVE_Adaptor
            : public Hook
        {
            // Attributes
        private:
            Tape &tape;
            // Methods
            OSSAVE_Adaptor();
        public:
            explicit OSSAVE_Adaptor(Tape &p_tape)
                : Hook("OSSAVE")
                , tape(p_tape)
                {}
            void attach()
                {
                    tape.address_space->add_child(0xFAE5, *this);
                }
        private:
            virtual int get_byte_book(word, AccessType p_at)
                {
                    int result(-1);
                    if (p_at == AT_INSTRUCTION)
                    {
                        class Fail
                            : public std::exception
                        {
                            virtual const char *what() const throw()
                                { return "OSSAVE Fail"; }
                        } fail;
                        try
                        {
                            const word X(tape.mcs6502->m_register.X);
                            Filename filename(tape.address_space->get_word(X), *tape.address_space);
                            if (!filename.cstr[0]) throw fail;
                            std::ofstream stream(filename.path(tape.directory), std::ios::binary);
                            if (stream.fail()) throw fail;
                            for (int i(2); stream.good() && i < 6; i += 1)
                                stream.put(tape.address_space->get_word(X + i));
                            if (stream.fail()) throw fail;
                            const word start(tape.address_space->get_word(X + 6));
                            const word end(tape.address_space->get_word(X + 8));
                            for (int addr(start); addr < end; addr += 1)
                            {
                                stream.put(tape.address_space->get_byte(addr));
                                if (stream.fail()) throw fail;
                            }
                            stream.close();
                            if (stream.fail()) throw fail;
                            result = 0x60; // RTS
                        }
                        catch(Fail e)
                        {
                            result = 0x00; // BRK
                        }
                    }
                    return result;
                }
        };
                  
        class OSLOAD_Adaptor
            : public Hook
        {
            // Attributes
        private:
            Tape &tape;
            // Methods
            OSLOAD_Adaptor();
        public:
            explicit OSLOAD_Adaptor(Tape &p_tape)
                : Hook("OSLOAD")
                , tape(p_tape)
                {}
            void attach()
                {
                    tape.address_space->add_child(0xF96E, *this);
                }
        private:
            virtual int get_byte_book(word, AccessType p_at)
                {
                    int result(-1);
                    if (p_at == AT_INSTRUCTION)
                    {
                        class Fail
                            : public std::exception
                        {
                            virtual const char *what() const throw()
                                { return "OSLOAD Fail"; }
                        } fail;
                        try
                        {
                            const word X(tape.mcs6502->m_register.X);
                            Filename filename(tape.address_space->get_word(X), *tape.address_space);
                            if (!filename.cstr[0]) throw fail;
                            std::ifstream stream(filename.path(tape.directory), std::ios::binary);
                            if (stream.fail()) throw fail;
                            const byte reload_address_lsb(stream.get());
                            const byte reload_address_msb(stream.get());
                            // Although not described in the ATAP
                            // *RUN transfers control to the address at 0xD6
                            tape.address_space->set_byte(0xD6, stream.get());
                            tape.address_space->set_byte(0xD7, stream.get());
                            if (stream.fail()) throw fail;
                            const word reload_address = (
                                (tape.address_space->get_byte(X + 4) & 0x80) ?
                                tape.address_space->get_word(X + 2) :
                                word((reload_address_msb << 8) | reload_address_lsb)
                                );
                            for (int i(0); stream.good(); i++)
                                tape.address_space->set_byte(reload_address+i, stream.get());
                            if (stream.fail()) throw fail;
                            stream.close();
                            if (stream.fail()) throw fail;
                            result = 0x60; // RTS
                        }
                        catch(Fail e)
                        {
                            result = 0x00; // BRK
                        }
                    }
                    return result;
                }
        };
                  
    public:
        class Configurator
            : public virtual Device::Configurator
        {
        protected:
            Configurator() = default;
        public:
            virtual ~Configurator() = default;
            virtual const Glib::ustring &directory() const = 0;
            virtual const Device::Configurator *mcs6502() const = 0;
            virtual Device *device_factory() const
                { return new Tape(*this); }
        };
    private:
        const Glib::ustring directory;
        AddressSpace *address_space;
        MCS6502 *mcs6502;
        OSSAVE_Adaptor *OSSAVE;
        OSLOAD_Adaptor *OSLOAD;
        Tape();
    public:
        Tape(const Configurator &p_cfgr)
            : Device(p_cfgr)
            , directory(p_cfgr.directory())
            , mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
            , OSSAVE(new OSSAVE_Adaptor(*this))
            , OSLOAD(new OSLOAD_Adaptor(*this))
            {
                assert (mcs6502);
                address_space = mcs6502->address_space();
                assert (address_space);
                assert (OSSAVE);
                OSSAVE->attach();
                assert (OSLOAD);
                OSLOAD->attach();
            }
    };
    
}

#endif

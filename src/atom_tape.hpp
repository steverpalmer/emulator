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

    class Tape
        : public Device
    {
    private:
        class Filename:
            private NonCopyable
        {
        private:
            Filename();
            Glib::ustring path_result;
        public:
            char cstr[14];
            Filename(word addr, Memory &);
            const char *path(const Glib::ustring &directory);
        };

        class OSSAVE_Adaptor
            : public Hook
        {
            // Attributes
        private:
            Tape &tape;
            // Methods
            OSSAVE_Adaptor();
        public:
            explicit OSSAVE_Adaptor(Tape &);
            void attach();
        private:
            virtual int get_byte_hook(word, AccessType) override;
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
            explicit OSLOAD_Adaptor(Tape &);
            void attach();
        private:
            virtual int get_byte_hook(word, AccessType p_at) override;
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
            virtual Device *device_factory() const override
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
        explicit Tape(const Configurator &);
    };
    
}

#endif

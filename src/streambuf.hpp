// streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef STREAMBUF_HPP_
#define STREAMBUF_HPP_

#include <streambuf>

#include "memory.hpp"
#include "cpu.hpp"
#include "safe_queue.hpp"

class StreamBuf
    : public std::streambuf
    , private NonCopyable
{
    // Types
private:
    class OSRDCH_Adaptor
        : public Hook
    {
    public:
        class Configurator
            : private NonCopyable
        {
        public:
            virtual const MCS6502::Configurator *mcs6502() const = 0;
        };
        class HookParameters
            : public virtual Hook::Configurator
        {
        public:
            virtual const Part::id_type &id() const { return Part::anonymous_id; }
            virtual word size() const { return 1; }
        };
        // Attributes
    private:
        HookParameters      m_hook_parameters;
        MCS6502             *m_mcs6502;
        BlockingQueue<char> m_queue;  // syncronization interface
        // Methods
    public:
        explicit OSRDCH_Adaptor(const Configurator &p_cfgr)
            : Hook(m_hook_parameters)
            , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
            {}
    private:
        virtual int get_byte_hook(word, AccessType p_at)
            {
                int result(-1);
                if (p_at == AT_INSTRUCTION)
                {
                    result = 0x60 /* RTS */;
                    m_mcs6502->m_register.A = m_queue.pop();
                }
                return result;
            }
    public:
        void push(char p_char) { m_queue.push(p_char); }
    };

    class OSWRCH_Adaptor
        : public Hook
    {
    public:
        class Configurator
            : private NonCopyable
        {
        public:
            virtual const MCS6502::Configurator *mcs6502() const = 0;
        };
        class HookParameters
            : public virtual Hook::Configurator
        {
        public:
            virtual const Part::id_type &id() const { return Part::anonymous_id; }
            virtual word size() const { return 1; }
        };
        // Attributes
    private:
        HookParameters      m_hook_parameters;
        MCS6502             *m_mcs6502;
        BlockingQueue<char> m_queue;  // syncronization interface
        // Methods
    public:
        OSWRCH_Adaptor(const Configurator &p_cfgr)
            : Hook(m_hook_parameters)
            , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
            {}
    private:
        virtual int get_byte_hook(word, AccessType p_at)
            {
                if (p_at == AT_INSTRUCTION)
                {
                    m_queue.push(m_mcs6502->m_register.A);
                }
                return -1;
            }
    public:
        char pop() { return m_queue.pop(); }
    };

    class Configurator
        : public virtual OSRDCH_Adaptor::Configurator
        , public virtual OSWRCH_Adaptor::Configurator
    {
    public:
        virtual const AddressSpace::Configurator *address_space() const = 0;
    };

private:
    OSRDCH_Adaptor m_OSRDCH;
    OSWRCH_Adaptor m_OSWRCH;
    char get_buffer;

public:
    StreamBuf(const Configurator &p_cfgr)
        : m_OSRDCH(p_cfgr)
        , m_OSWRCH(p_cfgr)
        {
            AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
            assert (address_space);
            address_space->add_child(0xFE94, m_OSRDCH);
            address_space->add_child(0xFE52, m_OSWRCH);
            setg(&get_buffer+1, &get_buffer+1, &get_buffer+1);
        }

    std::iostream *iostream_factory()
        { return new std::iostream(this); }

private:
    virtual int_type overflow(int_type ch)
        {
            m_OSRDCH.push(ch);
            return traits_type::to_int_type(ch);
        }
    virtual int_type underflow()
        {
            get_buffer = m_OSWRCH.pop();
            setg(&get_buffer, &get_buffer, &get_buffer+1);
            return traits_type::to_int_type(get_buffer);
        }
};

#endif

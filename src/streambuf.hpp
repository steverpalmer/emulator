// streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef STREAMBUF_HPP_
#define STREAMBUF_HPP_

#include <streambuf>

#include "memory.hpp"
#include "cpu.hpp"
#include "synchronization_queue.hpp"

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
        // Attributes
    private:
        MCS6502                    *m_mcs6502;
        SynchronizationQueue<byte> m_queue;
        // Methods
    public:
        explicit OSRDCH_Adaptor(const Configurator &);
    private:
        virtual int get_byte_hook(word, AccessType);
    public:
        void nonblocking_push(byte);
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
        // Attributes
    private:
        MCS6502                    *m_mcs6502;
        SynchronizationQueue<byte> m_queue;  // syncronization interface
        // Methods
    public:
        explicit OSWRCH_Adaptor(const Configurator &);
    private:
        virtual int get_byte_hook(word, AccessType p_at);
    public:
        byte blocking_pull();
    };

    class Configurator
        : public virtual OSRDCH_Adaptor::Configurator
        , public virtual OSWRCH_Adaptor::Configurator
    {
    public:
        virtual const AddressSpace::Configurator *address_space() const = 0;
    };

private:
    OSRDCH_Adaptor   m_OSRDCH;
    OSWRCH_Adaptor   m_OSWRCH;
    enum { Nominal
         , OneBehind
         , CatchUp}  m_get_state;
    char             m_get_buffer[2];

public:
    explicit StreamBuf(const Configurator &);

    std::iostream *iostream_factory()
        { return new std::iostream(this); }

private:
    virtual int_type overflow(int_type);
    virtual int_type underflow();
};

#endif

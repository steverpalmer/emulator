// streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef STREAMBUF_HPP_
#define STREAMBUF_HPP_

#include <streambuf>

#include "device.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "synchronization_queue.hpp"

class StreamBuf
    : public std::streambuf
    , public Device
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
            virtual const Device::Configurator *mcs6502() const = 0;
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
            virtual bool pause_output() const = 0;
            virtual const Device::Configurator *mcs6502() const = 0;
        };
        // Attributes
    private:
        bool                       m_is_paused;
        MCS6502                    *m_mcs6502;
        SynchronizationQueue<byte> m_queue;  // syncronization interface
        // Methods
    public:
        explicit OSWRCH_Adaptor(const Configurator &);
    private:
        virtual int get_byte_hook(word, AccessType p_at);
    public:
        byte blocking_pull();
        virtual void reset()  { m_is_paused = true; m_queue.nonblocking_clear(); }
        virtual void pause()  { m_is_paused = true; }
        virtual void resume() { m_is_paused = false; }
        virtual bool is_paused() const { return m_is_paused; }
    };
public:
    class Configurator
        : public virtual Device::Configurator
        , public virtual OSRDCH_Adaptor::Configurator
        , public virtual OSWRCH_Adaptor::Configurator
    {
    public:
        virtual const Memory::Configurator *address_space() const = 0;
        virtual Device *device_factory() const
            { return new StreamBuf(*this); }
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

    virtual void reset()  { m_OSWRCH.reset(); }
    virtual void pause()  { m_OSWRCH.pause(); }
    virtual void resume() { m_OSWRCH.resume(); }
    virtual bool is_paused() const { return m_OSWRCH.is_paused(); }

    std::iostream *iostream_factory()
        { return new std::iostream(this); }

private:
    virtual int_type overflow(int_type);
    virtual int_type underflow();
};

#endif

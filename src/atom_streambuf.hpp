// atom_streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_STREAMBUF_HPP_
#define ATOM_STREAMBUF_HPP_

#include <streambuf>

#include "device.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "synchronization_queue.hpp"

class AtomStreamBuf
    : public std::streambuf
    , public Device
{
    // Types
private:
    class OSRDCH_Adaptor
        : public Hook
    {
        // Attributes
    private:
        AtomStreamBuf &m_atom_streambuf;
    public:
        SynchronizationQueue<byte> queue;
        // Methods
    public:
        explicit OSRDCH_Adaptor(AtomStreamBuf &);
    private:
        virtual int get_byte_hook(word, AccessType);
    };

    class OSWRCH_Adaptor
        : public Hook
    {
        // Attributes
    private:
        AtomStreamBuf &m_atom_streambuf;
    public:
        bool is_paused;
        SynchronizationQueue<byte> queue;  // syncronization interface
        // Methods
    public:
        OSWRCH_Adaptor(AtomStreamBuf &, bool);
    private:
        virtual int get_byte_hook(word, AccessType p_at);
    public:
    };
public:
    class Configurator
        : public virtual Device::Configurator
    {
    public:
        virtual const Device::Configurator *mcs6502() const = 0;
        virtual const Memory::Configurator *address_space() const = 0;
        virtual bool pause_output() const = 0;
        virtual Device *device_factory() const
            { return new AtomStreamBuf(*this); }
    };

private:
    MCS6502          *m_mcs6502;

    OSRDCH_Adaptor   m_OSRDCH;
    OSWRCH_Adaptor   m_OSWRCH;
    enum { Nominal
         , OneBehind
         , CatchUp}  m_get_state;
    char             m_get_buffer[2];

public:
    explicit AtomStreamBuf(const Configurator &);
    virtual ~AtomStreamBuf();

    virtual void reset()  { m_OSWRCH.is_paused = true; m_OSWRCH.queue.nonblocking_clear(); }
    virtual void pause()  { m_OSWRCH.is_paused = true; }
    virtual void resume() { m_OSWRCH.is_paused = false; }
    virtual bool is_paused() const { return m_OSWRCH.is_paused; }

    std::iostream *iostream_factory()
        { return new std::iostream(this); }

private:
    virtual int_type overflow(int_type);
    virtual int_type underflow();
};

#endif

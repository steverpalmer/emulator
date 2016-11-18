// atom_streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_STREAMBUF_HPP_
#define ATOM_STREAMBUF_HPP_

#include <streambuf>

#include "device.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "synchronization_queue.hpp"


class AtomStreamBufBase
    : public std::streambuf
{
    // Types
public:
    class OSRDCH_Adaptor
        : public Hook
    {
        // Attributes
    private:
        AtomStreamBufBase &m_streambuf;
    public:
        SynchronizationQueue<byte> queue;
        // Methods
    public:
        explicit OSRDCH_Adaptor(AtomStreamBufBase &);
        void attach(AddressSpace &);
    private:
        virtual int get_byte_hook(word, AccessType);
    };

    class OSWRCH_Adaptor
        : public Hook
    {
        // Attributes
    private:
        AtomStreamBufBase &m_streambuf;
    public:
        bool is_paused;
        SynchronizationQueue<byte> queue;  // syncronization interface
        // Methods
    public:
        OSWRCH_Adaptor(AtomStreamBufBase &, bool);
        void attach(AddressSpace &);
    private:
        virtual int get_byte_hook(word, AccessType p_at);
    public:
    };

public:
    MCS6502        &m_mcs6502;
    OSRDCH_Adaptor m_OSRDCH;
    OSWRCH_Adaptor m_OSWRCH;

public:
    AtomStreamBufBase(MCS6502 &, bool output_paused);
    virtual ~AtomStreamBufBase();

    virtual void reset()
        {
            m_OSWRCH.is_paused = true;
            m_OSWRCH.queue.nonblocking_clear();
        }
    virtual void pause()
        {
            m_OSWRCH.is_paused = true;
        }
    virtual void resume()
        {
            m_OSWRCH.is_paused = false;
        }
    virtual bool is_paused() const
        {
            return m_OSWRCH.is_paused;
        }
    void unblock();

    virtual int_type overflow(int_type p_ch);
    virtual int_type underflow();
    virtual int_type uflow();
};


class AtomInputStreamBuf
    : public std::streambuf
    , public Device
{
    // Types
public:
    class Configurator
        : public virtual Device::Configurator
    {
    public:
        virtual const Device::Configurator *mcs6502() const = 0;
        virtual const Memory::Configurator *address_space() const = 0;
        virtual Device *device_factory() const
            { return new AtomInputStreamBuf(*this); }
    };

private:
    AtomStreamBufBase m_streambuf_base;

public:
    explicit AtomInputStreamBuf(const Configurator &);
private:
    virtual ~AtomInputStreamBuf();
public:
    void unblock() { m_streambuf_base.unblock(); }

    std::istream *istream_factory()
        { return new std::istream(this); }

private:
    virtual int_type underflow() { return m_streambuf_base.underflow(); }
    virtual int_type uflow()     { return m_streambuf_base.uflow(); }
};


class AtomOutputStreamBuf
    : public std::streambuf
    , public Device
{
    // Types
public:
    class Configurator
        : public virtual Device::Configurator
    {
    public:
        virtual const Device::Configurator *mcs6502() const = 0;
        virtual bool pause_output() const = 0;
        virtual const Memory::Configurator *address_space() const = 0;
        virtual Device *device_factory() const
            { return new AtomOutputStreamBuf(*this); }
    };

private:
    AtomStreamBufBase m_streambuf_base;

public:
    explicit AtomOutputStreamBuf(const Configurator &);
private:
    virtual ~AtomOutputStreamBuf();
public:
    virtual void reset()  { m_streambuf_base.reset(); }
    virtual void pause()  { m_streambuf_base.pause(); }
    virtual void resume() { m_streambuf_base.resume(); }
    virtual bool is_paused() const { return m_streambuf_base.is_paused(); }
    void unblock()        { m_streambuf_base.unblock(); }

    std::ostream *ostream_factory()
        { return new std::ostream(this); }

private:
    virtual int_type overflow(int_type p_ch) { return m_streambuf_base.overflow(p_ch); }
};


class AtomStreamBuf
    : public std::streambuf
    , public Device
{
    // Types
public:
    class Configurator
        : public virtual Device::Configurator
    {
    public:
        virtual const Device::Configurator *mcs6502() const = 0;
        virtual bool pause_output() const = 0;
        virtual const Memory::Configurator *address_space() const = 0;
        virtual Device *device_factory() const
            { return new AtomStreamBuf(*this); }
    };

private:
    AtomStreamBufBase m_streambuf_base;

public:
    explicit AtomStreamBuf(const Configurator &);
private:
    virtual ~AtomStreamBuf();
public:
    virtual void reset()  { m_streambuf_base.reset(); }
    virtual void pause()  { m_streambuf_base.pause(); }
    virtual void resume() { m_streambuf_base.resume(); }
    virtual bool is_paused() const { return m_streambuf_base.is_paused(); }
    void unblock() { m_streambuf_base.unblock(); }

    std::iostream *iostream_factory()
        { return new std::iostream(this); }

private:
    virtual int_type underflow()             { return m_streambuf_base.underflow(); }
    virtual int_type overflow(int_type p_ch) { return m_streambuf_base.overflow(p_ch); }
};


#endif

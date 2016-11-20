// atom_streambuf.cpp
// Copyright 2016 Steve Palmer

#include "atom_streambuf.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".atom_streambuf.cpp"));
    return result;
}

// Atom::Streambuf::OSRDCH

Atom::Streambuf::OSRDCH_Adaptor::OSRDCH_Adaptor(const Configurator &, Atom::Streambuf &p_streambuf)
    : Hook()
    , streambuf(p_streambuf)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::OSRDCH_Adaptor(...)");
}

void Atom::Streambuf::OSRDCH_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::attach(...)");
    streambuf.address_space->add_child(0xFE94, *this);
}

int Atom::Streambuf::OSRDCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        int_type next_value = streambuf.put_queue.blocking_pull();
        if (traits_type::not_eof(next_value))
        {
            streambuf.mcs6502->m_register.A = next_value;
            result = 0x60 /* RTS */;
        }
    }
    return result;
}

// Atom::Streambuf::OSWRCH

Atom::Streambuf::OSWRCH_Adaptor::OSWRCH_Adaptor(const Configurator &p_cfgr, Atom::Streambuf &p_streambuf)
    : Hook()
    , streambuf(p_streambuf)
    , is_paused(p_cfgr.pause_output())
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::OSWRCH_Adaptor(...)");
}

void Atom::Streambuf::OSWRCH_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::attach(...)");
    streambuf.address_space->add_child(0xFE52, *this);
}

int Atom::Streambuf::OSWRCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    if (p_at == AT_INSTRUCTION /* && !is_paused */)
    {
        const int_type value(traits_type::to_int_type(streambuf.mcs6502->m_register.A));
        LOG4CXX_DEBUG(cpptrace_log(), "OSWRCH_Adaptor::get_byte_hook pushing an " << value);
        streambuf.get_queue.nonblocking_push(value);
    }
    return -1;
}

// Atom::Streambuf

Atom::Streambuf::Streambuf(const Configurator &p_cfgr)
    : mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::Streambuf(...)");
    assert (mcs6502);
    address_space = mcs6502->address_space();
    assert (address_space);
    OSRDCH = new OSRDCH_Adaptor(p_cfgr, *this);
    assert (OSRDCH);
    OSWRCH = new OSWRCH_Adaptor(p_cfgr, *this);
    assert (OSWRCH);
}

void Atom::Streambuf::terminating()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::terminating()");
    get_queue.unblock(traits_type::eof());
    put_queue.unblock(traits_type::eof());
}

Atom::Streambuf::~Streambuf()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::~Streambuf()");
}

Atom::Streambuf::int_type Atom::Streambuf::overflow(int_type p_ch)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::overflow(" << p_ch << ")");
    if (traits_type::not_eof(p_ch))
    {
        if (p_ch == '\n')
            p_ch = traits_type::to_int_type('\x0D');
        put_queue.nonblocking_push(p_ch);
    }
    return p_ch;
}

Atom::Streambuf::int_type Atom::Streambuf::underflow()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::underflow()");
    static enum { Nominal, OneBehind, CatchUp}  state = Nominal;
    static int_type         buffer[2];
    static std::mutex       mutex;
    std::lock_guard<std::mutex> lock(mutex);  // one at a time...
    int_type * current;
    switch (state)
    {
    case Nominal:
        buffer[0] = buffer[1];
        LOG4CXX_DEBUG(cpptrace_log(), "Atom::Streambuf::underflow trying to pulled");
        buffer[1] = get_queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "Atom::Streambuf::underflow pulled a " << buffer[1]);
        if (traits_type::not_eof(buffer[1]) && (buffer[1] == '\x0D'  || buffer[1] == '\x0A'))
        {
            state = OneBehind;
            // Don't break
        }
        else
        {
            current = &buffer[1];
            break;
        }
    case OneBehind:
        buffer[0] = buffer[1];
        LOG4CXX_DEBUG(cpptrace_log(), "Atom::Streambuf::underflow trying to pulled again");
        buffer[1] = get_queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "Atom::Streambuf::underflow pulled another " << int(buffer[1]));
        if (!traits_type::not_eof(buffer[1]))
        {
            current = &buffer[0];
            state = CatchUp;
        }
        else if (buffer[0] == '\x0D' && buffer[1] == '\x0A')
        {
            buffer[1] = '\n';
            current = &buffer[1];
            state = Nominal;
        }
        else if (buffer[0] == '\x0A' && buffer[1] == '\x0D')
        {
            buffer[1] = '\n';
            current = &buffer[1];
            state = Nominal;
        }
        else if (buffer[1] == '\x0D' || buffer[1] == '\x0A')
        {
            current = &buffer[0];
            // Stay OneBehind
        }
        else
        {
            current = &buffer[0];
            state = CatchUp;
        }
        break;
    case CatchUp:
        current = &buffer[1];
        state = Nominal;
        break;
    }
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::underflow() => " << *current);
    return *current;
}

Atom::Streambuf::int_type Atom::Streambuf::uflow()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::uflow()");
    return underflow();
}


// Atom::StreamDevice

Atom::StreamDevice::StreamDevice(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , streambuf(new Streambuf(p_cfgr))
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::StreamDevice::StreamDevice(" << p_cfgr << ")");
    assert (streambuf);
}

void Atom::StreamDevice::reset()
{
    streambuf->reset();
}

void Atom::StreamDevice::pause()
{
    streambuf->pause();
}

void Atom::StreamDevice::resume()
{
    streambuf->resume();
}

bool Atom::StreamDevice::is_paused()
{
    return streambuf->is_paused();
}

void Atom::StreamDevice::unblock()
{
    streambuf->put_queue.unblock(Atom::Streambuf::traits_type::eof());
    streambuf->get_queue.unblock(Atom::Streambuf::traits_type::eof());
}

void Atom::StreamDevice::terminating()
{
    streambuf->terminating();
}

Atom::StreamDevice::~StreamDevice()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::StreamDevice::~StreamDevice()");
    delete streambuf;
}

// Atom::IStream

Atom::IStream::IStream(const Configurator &p_cfgr)
    : StreamDevice(p_cfgr)
    , std::istream(streambuf)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IStream::IStream(" << p_cfgr << ")");
    streambuf->OSWRCH->attach();
}

Atom::IStream::~IStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IStream::~IStream()");
    (void) rdbuf(0);
}

// Atom::OStream

Atom::OStream::OStream(const Configurator &p_cfgr)
    : StreamDevice(p_cfgr)
    , std::ostream(streambuf)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::OStream::OStream(" << p_cfgr << ")");
    streambuf->OSRDCH->attach();
}

Atom::OStream::~OStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::OStream::~Atom::OStream()");
    (void) rdbuf(0);
}

// Atom::Stream

Atom::IOStream::IOStream(const Configurator &p_cfgr)
    : StreamDevice(p_cfgr)
    , std::iostream(streambuf)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IOStream::IOStream(" << p_cfgr << ")");
    streambuf->OSWRCH->attach();
    streambuf->OSRDCH->attach();
}

Atom::IOStream::~IOStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IOStream::~IOStream()");
    (void) rdbuf(0);
}

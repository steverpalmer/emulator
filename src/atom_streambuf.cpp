// atom_streambuf.cpp
// Copyright 2016 Steve Palmer

#include "atom_streambuf.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".atom_streambuf.cpp"));
    return result;
}

// Atom::Streambuf::OSRDCH

Atom::Streambuf::OSRDCH_Adaptor::OSRDCH_Adaptor(const Configurator &p_cfgr, Atom::Streambuf &p_streambuf)
    : Hook()
    , m_streambuf(p_streambuf)
    , m_mcs6502(*dynamic_cast<MCS6502 *>(PartsBin::instance()[p_cfgr.mcs6502()]))
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::OSRDCH_Adaptor([" << p_cfgr.mcs6502() << "])");
}

void Atom::Streambuf::OSRDCH_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::attach(...)");
    m_streambuf.m_address_space->add_child(0xFE94, *this);
}

int Atom::Streambuf::OSRDCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSRDCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        int_type next_value = m_streambuf.put_queue.blocking_pull();
        if (traits_type::not_eof(next_value))
        {
            result = 0x60 /* RTS */;
            m_mcs6502.m_register.A = next_value;
        }
    }
    return result;
}

// Atom::Streambuf::OSWRCH

Atom::Streambuf::OSWRCH_Adaptor::OSWRCH_Adaptor(const Configurator &p_cfgr, Atom::Streambuf &p_streambuf)
    : Hook()
    , m_streambuf(p_streambuf)
    , m_mcs6502(*dynamic_cast<MCS6502 *>(PartsBin::instance()[p_cfgr.mcs6502()]))
    , is_paused(p_cfgr.pause_output())
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::OSWRCH_Adaptor([" << p_cfgr.mcs6502() << "])");
}

void Atom::Streambuf::OSWRCH_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::attach(...)");
    m_streambuf.m_address_space->add_child(0xFE52, *this);
}

int Atom::Streambuf::OSWRCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::OSWRCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    if (p_at == AT_INSTRUCTION /* && !is_paused */)
    {
        LOG4CXX_DEBUG(cpptrace_log(), "OSWRCH_Adaptor::get_byte_hook pushing an " << m_mcs6502.m_register.A);
        m_streambuf.get_queue.nonblocking_push(traits_type::to_int_type(m_mcs6502.m_register.A));
    }
    return -1;
}

// Atom::Streambuf

Atom::Streambuf::Streambuf(const Configurator &p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Streambuf::Streambuf(...)");
    m_address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (m_address_space);
    m_OSRDCH = new OSRDCH_Adaptor(p_cfgr, *this);
    assert (m_OSRDCH);
    m_OSWRCH = new OSWRCH_Adaptor(p_cfgr, *this);
    assert (m_OSWRCH);
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
            p_ch = '\x0D';
        put_queue.nonblocking_push(p_ch);
    }
    return traits_type::to_int_type(p_ch);
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


// Atom::IStream

Atom::IStream::IStream(const Configurator &p_cfgr)
    : std::istream(new Streambuf(p_cfgr))
    , Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IStream::IStream(" << p_cfgr << ")");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->m_OSWRCH->attach();
}

void Atom::IStream::terminating()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IStream::terminating()");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->terminating();
}

Atom::IStream::~IStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IStream::~IStream()");
    delete rdbuf(0);
}

// Atom::OStream

Atom::OStream::OStream(const Configurator &p_cfgr)
    : std::ostream(new Atom::Streambuf(p_cfgr))
    , Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::OStream::OStream(" << p_cfgr << ")");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->m_OSRDCH->attach();
}

void Atom::OStream::terminating()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::OStream::terminating()");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->terminating();
}

Atom::OStream::~OStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::OStream::~Atom::OStream()");
    delete rdbuf(0);
}

// Atom::Stream

Atom::IOStream::IOStream(const Configurator &p_cfgr)
    : std::iostream(new Atom::Streambuf(p_cfgr))
    , Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IOStream::IOStream(" << p_cfgr << ")");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->m_OSWRCH->attach();
    dynamic_cast<Atom::Streambuf *>(rdbuf())->m_OSRDCH->attach();
}

void Atom::IOStream::terminating()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IOStream::terminating()");
    dynamic_cast<Atom::Streambuf *>(rdbuf())->terminating();
}

Atom::IOStream::~IOStream()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::IOStream::~IOStream()");
    delete rdbuf(0);
}

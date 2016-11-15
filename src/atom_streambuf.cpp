// atom_streambuf.cpp
// Copyright 2016 Steve Palmer

#include "atom_streambuf.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".atom_streambuf.cpp"));
    return result;
}

class HookParameters
    : public virtual Hook::Configurator
{
public:
    virtual const Part::id_type &id() const { return Part::anonymous_id; }
    virtual word size() const { return 1; }
};

static HookParameters hook_parameters;

// OSRDCH

AtomStreamBufBase::OSRDCH_Adaptor::OSRDCH_Adaptor(AtomStreamBufBase &p_streambuf)
    : Hook(hook_parameters)
    , m_streambuf(p_streambuf)
{
    LOG4CXX_INFO(cpptrace_log(), "OSRDCH_Adaptor::OSRDCH_Adaptor(...)");
}

void AtomStreamBufBase::OSRDCH_Adaptor::attach(AddressSpace &p_address_space)
{
    LOG4CXX_INFO(cpptrace_log(), "OSRDCH_Adaptor::attach(...)");
    p_address_space.add_child(0xFE94, *this);
}

int AtomStreamBufBase::OSRDCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "OSRDCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        result = 0x60 /* RTS */;
        m_streambuf.m_mcs6502.m_register.A = queue.blocking_pull();
    }
    return result;
}

// OSWRCH

AtomStreamBufBase::OSWRCH_Adaptor::OSWRCH_Adaptor(AtomStreamBufBase &p_streambuf,
                                                  bool p_is_paused)
    : Hook(hook_parameters)
    , m_streambuf(p_streambuf)
    , is_paused(p_is_paused)
{
    LOG4CXX_INFO(cpptrace_log(), "OSWRCH_Adaptor::OSWRCH_Adaptor(...)");
}

void AtomStreamBufBase::OSWRCH_Adaptor::attach(AddressSpace &p_address_space)
{
    LOG4CXX_INFO(cpptrace_log(), "OSWRCH_Adaptor::attach(...)");
    p_address_space.add_child(0xFE52, *this);
}

int AtomStreamBufBase::OSWRCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "OSWRCH_Adaptor::get_byte_hook(..., " << p_at << ")");
    if (p_at == AT_INSTRUCTION /* && !is_paused */)
    {
        LOG4CXX_DEBUG(cpptrace_log(), "OSWRCH_Adaptor::get_byte_hook pushing an " << int(m_streambuf.m_mcs6502.m_register.A));
        queue.nonblocking_push(m_streambuf.m_mcs6502.m_register.A);
    }
    return -1;
}

// AtomStreamBufBase

AtomStreamBufBase::AtomStreamBufBase(MCS6502 &p_mcs6502, bool output_paused)
    : m_mcs6502(p_mcs6502)
    , m_OSRDCH(*this)
    , m_OSWRCH(*this, output_paused)
    , m_get_state(Nominal)
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBufBase::AtomStreamBufBase([" << p_mcs6502.id() << "], " << output_paused << ")");
    setg(&m_get_buffer[2], &m_get_buffer[2], &m_get_buffer[2]);
}

AtomStreamBufBase::~AtomStreamBufBase()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBufBase::~AtomStreamBufBase()");
}

void AtomStreamBufBase::unblock()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBufBase::unblock()");
    m_OSRDCH.queue.unblock();
    m_OSWRCH.queue.unblock();
}

AtomStreamBufBase::int_type AtomStreamBufBase::overflow(int_type p_ch)
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBufBase::overflow(" << p_ch << ")");
    if (p_ch == traits_type::eof())
        assert (false);
    else
    {
        if (p_ch == '\n')
            p_ch = '\x0D';
        m_OSRDCH.queue.nonblocking_push(p_ch);
    }
    return traits_type::to_int_type(p_ch);
}

AtomStreamBufBase::int_type AtomStreamBufBase::underflow()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::underflow()");
    switch (m_get_state)
    {
    case Nominal:
        m_get_buffer[0] = m_get_buffer[1];
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow trying to pulled");
        m_get_buffer[1] = m_OSWRCH.queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow pulled a " << int(m_get_buffer[1]));
        if (m_get_buffer[1] == '\x0D')
        {
            m_get_state = OneBehind;
            // Don't break
        }
        else
        {
            setg(&m_get_buffer[1], &m_get_buffer[1], &m_get_buffer[2]);
            break;
        }
    case OneBehind:
        m_get_buffer[0] = m_get_buffer[1];
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow trying to pulled again");
        m_get_buffer[1] = m_OSWRCH.queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow pulled another " << int(m_get_buffer[1]));
        if (m_get_buffer[0] == '\x0D' && m_get_buffer[1] == '\x0A')
        {
            m_get_buffer[1] = '\n';
            setg(&m_get_buffer[1], &m_get_buffer[1], &m_get_buffer[2]);
            m_get_state = Nominal;
        }
        else if (m_get_buffer[1] == '\x0D')
        {
            setg(&m_get_buffer[0], &m_get_buffer[0], &m_get_buffer[1]);
            // Stay OneBehind
        }
        else
        {
            setg(&m_get_buffer[0], &m_get_buffer[0], &m_get_buffer[1]);
            m_get_state = CatchUp;
        }
        break;
    case CatchUp:
        setg(&m_get_buffer[1], &m_get_buffer[1], &m_get_buffer[2]);
        m_get_state = Nominal;
        break;
    }
    return traits_type::to_int_type(*gptr());
}



// AtomInputStreamBuf

AtomInputStreamBuf::AtomInputStreamBuf(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , m_streambuf_base(*dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()), true)
{
    LOG4CXX_INFO(cpptrace_log(), "AtomInputStreamBuf::AtomInputStreamBuf(" << p_cfgr << ")");
    AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (address_space);
    m_streambuf_base.m_OSWRCH.attach(*address_space);
}

AtomInputStreamBuf::~AtomInputStreamBuf()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomInputStreamBuf::~AtomInputStreamBuf()");
}

// AtomOutputStreamBuf

AtomOutputStreamBuf::AtomOutputStreamBuf(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , m_streambuf_base(*dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()), p_cfgr.pause_output())
{
    LOG4CXX_INFO(cpptrace_log(), "AtomOutputStreamBuf::AtomOutputStreamBuf(" << p_cfgr << ")");
    AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (address_space);
    m_streambuf_base.m_OSRDCH.attach(*address_space);
}

AtomOutputStreamBuf::~AtomOutputStreamBuf()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomOutputStreamBuf::~AtomOutputStreamBuf()");
}

// AtomStreamBuf

AtomStreamBuf::AtomStreamBuf(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , m_streambuf_base(*dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()), p_cfgr.pause_output())
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::AtomStreamBuf(" << p_cfgr << ")");
    AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (address_space);
    m_streambuf_base.m_OSWRCH.attach(*address_space);
    m_streambuf_base.m_OSRDCH.attach(*address_space);
}

AtomStreamBuf::~AtomStreamBuf()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::~AtomStreamBuf()");
}

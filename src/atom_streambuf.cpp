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
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBufBase::AtomStreamBufBase([" << p_mcs6502.id() << "], " << output_paused << ")");
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
    static enum { Nominal, OneBehind, CatchUp}  state = Nominal;
    static char             buffer[2];
    static std::mutex       mutex;
    std::lock_guard<std::mutex> lock(mutex);  // one at a time...
    char * current;
    switch (state)
    {
    case Nominal:
        buffer[0] = buffer[1];
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow trying to pulled");
        buffer[1] = m_OSWRCH.queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow pulled a " << int(buffer[1]));
        if (buffer[1] == '\x0D'  || buffer[1] == '\x0A')
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
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow trying to pulled again");
        buffer[1] = m_OSWRCH.queue.blocking_pull();
        LOG4CXX_DEBUG(cpptrace_log(), "AtomStreamBuf::underflow pulled another " << int(buffer[1]));
        if (buffer[0] == '\x0D' && buffer[1] == '\x0A')
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
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::underflow() returning ... ");
    const int_type result(traits_type::to_int_type(*current));
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::underflow() => " << result);
    return result;
}

AtomStreamBufBase::int_type AtomStreamBufBase::uflow()
{
    LOG4CXX_INFO(cpptrace_log(), "AtomStreamBuf::uflow()");
    return underflow();
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

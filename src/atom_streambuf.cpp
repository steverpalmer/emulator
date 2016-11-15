// atom_streambuf.cpp
// Copyright 2016 Steve Palmer

#include "atom_streambuf.hpp"

class HookParameters
    : public virtual Hook::Configurator
{
public:
    virtual const Part::id_type &id() const { return Part::anonymous_id; }
    virtual word size() const { return 1; }
};

static HookParameters hook_parameters;

// OSRDCH

AtomStreamBuf::OSRDCH_Adaptor::OSRDCH_Adaptor(AtomStreamBuf &p_atom_streambuf)
    : Hook(hook_parameters)
    , m_atom_streambuf(p_atom_streambuf)
{
}

int AtomStreamBuf::OSRDCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    int result(-1);
    if (p_at == AT_INSTRUCTION && m_atom_streambuf.m_mcs6502)
    {
        result = 0x60 /* RTS */;
        m_atom_streambuf.m_mcs6502->m_register.A = queue.blocking_pull();
    }
    return result;
}

// OSWRCH

AtomStreamBuf::OSWRCH_Adaptor::OSWRCH_Adaptor(AtomStreamBuf &p_atom_streambuf, bool p_is_paused)
    : Hook(hook_parameters)
    , m_atom_streambuf(p_atom_streambuf)
    , is_paused(p_is_paused)
{
}

int AtomStreamBuf::OSWRCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    if (p_at == AT_INSTRUCTION && !is_paused && m_atom_streambuf.m_mcs6502)
    {
        queue.nonblocking_push(m_atom_streambuf.m_mcs6502->m_register.A);
    }
    return -1;
}

// AtomStreamBuf

AtomStreamBuf::AtomStreamBuf(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
    , m_OSRDCH(*this)
    , m_OSWRCH(*this, p_cfgr.pause_output())
    , m_get_state(Nominal)
{
    if (m_mcs6502)
    {
        LOG4CXX_INFO(Part::log(), "making [" << m_mcs6502->id() << "] child of [" << id() << "]");
    }
    AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (address_space);
    address_space->add_child(0xFE94, m_OSRDCH);
    address_space->add_child(0xFE52, m_OSWRCH);
    setg(&m_get_buffer[2], &m_get_buffer[2], &m_get_buffer[2]);
}

AtomStreamBuf::~AtomStreamBuf()
{
    if (m_mcs6502)
    {
        LOG4CXX_INFO(Part::log(), "removing children of [" << id() << "]");
        m_mcs6502 = 0;
    }
}

AtomStreamBuf::int_type AtomStreamBuf::overflow(int_type p_ch)
{
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

AtomStreamBuf::int_type AtomStreamBuf::underflow()
{
    switch (m_get_state)
    {
    case Nominal:
        m_get_buffer[0] = m_get_buffer[1];
        m_get_buffer[1] = m_OSWRCH.queue.blocking_pull();
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
        m_get_buffer[1] = m_OSWRCH.queue.blocking_pull();
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

// streambuf.cpp
// Copyright 2016 Steve Palmer

#include "streambuf.hpp"

class HookParameters
    : public virtual Hook::Configurator
{
public:
    virtual const Part::id_type &id() const { return Part::anonymous_id; }
    virtual word size() const { return 1; }
};

static HookParameters hook_parameters;

// OSRDCH

StreamBuf::OSRDCH_Adaptor::OSRDCH_Adaptor(const Configurator &p_cfgr)
    : Hook(hook_parameters)
    , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
{
}

int StreamBuf::OSRDCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        result = 0x60 /* RTS */;
        m_mcs6502->m_register.A = m_queue.blocking_pull();
    }
    return result;
}

void StreamBuf::OSRDCH_Adaptor::nonblocking_push(byte p_byte)
{
    m_queue.nonblocking_push(p_byte);
}

// OSWRCH

StreamBuf::OSWRCH_Adaptor::OSWRCH_Adaptor(const Configurator &p_cfgr)
    : Hook(hook_parameters)
    , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
{
}

int StreamBuf::OSWRCH_Adaptor::get_byte_hook(word, AccessType p_at)
{
    if (p_at == AT_INSTRUCTION)
    {
        m_queue.nonblocking_push(m_mcs6502->m_register.A);
    }
    return -1;
}

byte StreamBuf::OSWRCH_Adaptor::blocking_pull()
{
    return m_queue.blocking_pull();
}

// StreamBuf

StreamBuf::StreamBuf(const Configurator &p_cfgr)
    : m_OSRDCH(p_cfgr)
    , m_OSWRCH(p_cfgr)
    , m_get_state(Nominal)
{
    AddressSpace *address_space = dynamic_cast<AddressSpace *>(p_cfgr.address_space()->memory_factory());
    assert (address_space);
    address_space->add_child(0xFE94, m_OSRDCH);
    address_space->add_child(0xFE52, m_OSWRCH);
    setg(&m_get_buffer[2], &m_get_buffer[2], &m_get_buffer[2]);
}

StreamBuf::int_type StreamBuf::overflow(int_type p_ch)
{
    if (p_ch == traits_type::eof())
        assert (false);
    else
    {
        if (p_ch == '\n')
            p_ch = '\x0D';
        m_OSRDCH.nonblocking_push(p_ch);
    }
    return traits_type::to_int_type(p_ch);
}

StreamBuf::int_type StreamBuf::underflow()
{
    switch (m_get_state)
    {
    case Nominal:
        m_get_buffer[0] = m_get_buffer[1];
        m_get_buffer[1] = m_OSWRCH.blocking_pull();
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
        m_get_buffer[1] = m_OSWRCH.blocking_pull();
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

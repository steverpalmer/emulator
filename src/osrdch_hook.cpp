// osrdch_hook.cpp
// Copyright 2016 Steve Palmer

#include "osrdch_hook.hpp"

OSRDCH_Hook::OSRDCH_Hook(const Configurator &p_cfgr)
    : Hook(p_cfgr)
    , m_mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.cpu()->device_factory()))
    , m_queue_head(0)
    , m_queue_tail(0)
{
}

int OSRDCH_Hook::get_byte_hook(word, AccessType p_at)
{
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        result = 0x60 /* RTS */;
        // At end:
        // * A = ASCII character
        // * X is unchanged
        // * Y is unchanged
        // * P.carry is undefined
        // * P.zero is undefined
        // * P.irqb is unchanged
        // * P.decimal is unchanged
        // * P.beak is unchanged
        // * P.unused is unchanged
        // * P.overflow is unchanged
        // * P.negative is undefined
        // * cycle count increased by a big number
        gunichar key = ~0;
        do
        {
            while (m_queue_tail == m_queue_head)  // wait while the queue is empty
                std::this_thread::yield();
            int new_head = m_queue_head + 1;
            if (new_head > int(m_key_queue.size()))
                new_head = 0;
            key = m_key_queue[m_queue_head];
            m_queue_head = new_head;
        }
        while (key & ~0x7F);
        m_mcs6502->m_register.A = key;
    }
    return result;
}

void OSRDCH_Hook::push_keypress(gunichar p_key)
{
    int new_tail = m_queue_tail + 1;
    if (new_tail >= int(m_key_queue.size()))
        new_tail = 0;
    if (new_tail != m_queue_head)
    {
        m_key_queue[new_tail] = p_key;
        m_queue_tail = new_tail;
    }
}

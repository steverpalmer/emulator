// text_input_hook.hpp
// Copyright 2016 Steve Palmer

#ifndef OSRDCH_HOOK_HPP_
#define OSRDCH_HOOK_HPP_

#include <array>
#include <atomic>
#include <thread>

#include "common.hpp"
#include "memory.hpp"
#include "cpu.hpp"

/// OSRDCH_Hook
///
/// This is a hook designed to hook at the start
/// of the Kernel Rom OSRDCH subroutine (#FE94).
/// It is intended to emulate the scanning of the keyboard
/// based on the values in a key queue
/// and then effect an RTS

class OSRDCH_Hook
    : public Hook
{
public:
    class Configurator
        : public virtual Hook::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual word size() const { return 1; }
        virtual MCS6502::Configurator *cpu() const;
        virtual Memory *memory_factory() const
            { return new OSRDCH_Hook(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    MCS6502                 *m_mcs6502;
    std::array<gunichar, 4> m_key_queue;
    std::atomic_int         m_queue_head;
    std::atomic_int         m_queue_tail;
    // Methods
public:
    explicit OSRDCH_Hook(const Configurator &);
private:
    virtual int get_byte_hook(word, AccessType);
public:
    virtual ~OSRDCH_Hook() = default;
    void push_keypress(gunichar p_key);
    void hook_to(AddressSpace &p_as)
        { p_as.add_child(0xFE94, *this); }
};

#endif

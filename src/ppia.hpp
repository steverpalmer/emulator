// ppia.hpp
// Copyright 2016 Steve Palmer

#ifndef PPIA_HPP_
#define PPIA_HPP_

#include <array>
#include <mutex>

#include "common.hpp"
#include "memory.hpp"
#include "atom_keyboard_interface.hpp"
#include "atom_monitor_interface.hpp"


class Ppia
    : public Memory
    , public virtual AtomKeyboardInterface
    , public AtomMonitorInterface
{
public:
    class Configurator
        : public virtual Memory::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        /// 1. Constructor Information - Name only
        /// 2. Factory Method
        virtual Memory *memory_factory() const
            { return new Ppia(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    std::array<byte, 4> m_register;
    struct
    {
        mutable std::recursive_mutex mutex;
        VDGMode  vdg_mode;
        VDGMode  notified_vdg_mode;
    } m_terminal;
    struct
    {
        mutable std::recursive_mutex mutex;
        std::array<byte, 12> row;
    } m_keyboard;
    typedef std::pair<int, byte> Scanpair;
    std::map<AtomKeyboardInterface::Key, Scanpair> key_mapping;
    // Methods
private:
    byte get_PortB(int p_row);
    byte get_PortC(byte p_previous);
    void set_PortA(byte p_byte);
public:
    explicit Ppia(const Configurator &);
    virtual ~Ppia();

    // Device
public:
    virtual word size() const { return 4; }
    virtual void reset();
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
protected:
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);

    // AtomKeyboardInterface
private:
    virtual void down(Key);
    virtual void up(Key);

    // TerminalInterface
public:
    virtual Part::id_type id() const { return Device::id(); }
    virtual VDGMode vdg_mode() const;

    virtual void serialize(std::ostream &) const;
};

#endif

// ppia.hpp
// Copyright 2016 Steve Palmer

#ifndef PPIA_HPP_
#define PPIA_HPP_

#include <array>
#include <chrono>
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
        virtual Memory *memory_factory() const override
            { return new Ppia(*this); }

        virtual void serialize(std::ostream &) const override;
    };
    // Attributes
private:
    std::array<byte, 4> m_register;
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    struct
    {
        mutable std::recursive_mutex mutex;
        VDGMode  vdg_mode;
    } m_monitor;
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
    Ppia();
public:
    explicit Ppia(const Configurator &);
private:
    virtual ~Ppia();

    // Device
public:
    virtual word size() const  override{ return 4; }
    virtual void reset() override;
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN) override;
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN) override;

    // AtomKeyboardInterface
private:
    virtual void down(Key) override;
    virtual void up(Key) override;

    // AtomMonitorInterface
public:
    virtual Part::id_type id() const  override{ return Device::id(); }
    virtual VDGMode vdg_mode() const override;

    virtual void serialize(std::ostream &) const override;
};

#endif

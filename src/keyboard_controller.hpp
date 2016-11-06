// keyboard_controller.hpp
// Copyright 2016 Steve Palmer

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

#include <SDL.h>

#include "common.hpp"
#include "device.hpp"
#include "terminal_interface.hpp"

class KeyboardController
    : protected TerminalInterface::Observer
{
    // Types
public:
    class Configurator
        : protected NonCopyable
    {
    protected:
        Configurator() = default;
	public:
        virtual ~Configurator() = default;

        virtual const Device::Configurator *reset_target() const = 0;

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };

    class KeyCommand;
public:
    TerminalInterface *m_terminal_interface;
    Device *m_reset_target;
    gunichar m_last_key_pressed;
    bool is_shift_pressed;
    bool is_control_pressed;
private:
    std::map<SDL_Keycode, const KeyCommand *> keys;
public:
    KeyboardController(TerminalInterface *, const Configurator &);
    virtual ~KeyboardController();

    void handle_event(SDL_KeyboardEvent &);

private:
    // TerminalInterface Observer implementation
    virtual void vdg_mode_update(TerminalInterface &, TerminalInterface::VDGMode) {}
    virtual void subject_loss(const TerminalInterface &);

public:
    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const KeyboardController &p_kc)
        { p_kc.serialize(p_s); return p_s; }
};

#endif

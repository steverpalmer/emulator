// terminal.hpp

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <SDL.h>

#include "part.hpp"
#include "screen_graphics_controller.hpp"
#include "keyboard_controller.hpp"


class Terminal : public Part {
public:
    class Configurator : public Part::Configurator
    {
    public:
        virtual MonitorView::Configurator        &monitor_view() const = 0;
        virtual KeyboardController::Configurator &keyboard_controller() const = 0;
        
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    MonitorView        m_monitor_view;
    KeyboardController m_keyboard_controller;
    // Methods
private:
    Terminal(const Terminal &);
    Terminal &operator=(const Terminal &);
public:
    Terminal(Device &p_device, TerminalInterface &p_terminal_interface, const Configurator &p_cfg);
    void update(SDL_Event *) { m_keyboard_controller.update(&event.key); }

    friend std::ostream &::operator <<(std::ostream &, const Terminal &);
};

#endif

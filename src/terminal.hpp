// terminal.hpp

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <SDL.h>

#include "part.hpp"
#include "monitor_view.hpp"
#include "keyboard_controller.hpp"


class Terminal
    : public virtual Part
{
public:
    class Configurator
        : public Part::Configurator
    {
    protected:
        Configurator();
    private:
        Configurator(const Configurator &);
        Configurator &operator=(const Configurator &);
	public:
        ~Configurator();
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
    Terminal(Memory &p_memory, TerminalInterface &p_terminal_interface, const Configurator &p_cfg);
    void update(SDL_Event *event) { m_keyboard_controller.update(&event->key); }

    friend std::ostream &::operator <<(std::ostream &, const Terminal &);
};

#endif

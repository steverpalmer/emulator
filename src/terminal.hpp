// terminal.hpp

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <ostream>

#include <SDL.h>

#include "part.hpp"
#include "memory.hpp"
#include "terminal_interface.hpp"
#include "monitor_view.hpp"
#include "keyboard_controller.hpp"


class Terminal
    : public Part
{
public:
    class Configurator
        : public virtual Part::Configurator
    {
    protected:
        Configurator() = default;
	public:
        virtual ~Configurator() = default;
    public:
        virtual const Part::id_type                    &memory_id() const = 0;
        virtual const Part::id_type                    &controller_id() const = 0;
        virtual const MonitorView::Configurator        &monitor_view() const = 0;
        virtual const KeyboardController::Configurator &keyboard_controller() const = 0;
        virtual Part *part_factory() const
            { return new Terminal(*this); }
        
        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    Memory             *m_memory;  // FIXME: should be const
    TerminalInterface  *m_terminal_interface;
    MonitorView        m_monitor_view;
    KeyboardController m_keyboard_controller;
    // Methods
public:
    explicit Terminal(const Configurator &);
    virtual ~Terminal() = default;
    void update(SDL_Event *event) { m_keyboard_controller.update(&event->key); }

    virtual void serialize(std::ostream &) const;
};

#endif

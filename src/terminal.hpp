// terminal.hpp
// Copyright 2016 Steve Palmer

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <SDL.h>

#include "common.hpp"
#include "part.hpp"
#include "memory.hpp"
#include "ppia.hpp"
#include "monitor_view.hpp"


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
        virtual const Memory::Configurator             *memory() const = 0;
        virtual const Memory::Configurator             *ppia() const = 0;
        virtual const MonitorView::Configurator        &monitor_view() const = 0;
        virtual Part *part_factory() const
            { return new Terminal(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    Memory             *m_memory;
    Ppia               *m_ppia;
    MonitorView        m_monitor_view;
    // Methods
public:
    explicit Terminal(const Configurator &);
    virtual ~Terminal();
    virtual void remove_child(Part &);
    bool handle_event(SDL_Event &);

    virtual void serialize(std::ostream &) const;
};

#endif

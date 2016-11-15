// monitor_view.hpp
// Copyright 2016 Steve Palmer

#ifndef MONITOR_VIEW_HPP_
#define MONITOR_VIEW_HPP_

#include <vector>

#include <SDL.h>

#include "common.hpp"
#include "memory.hpp"
#include "ppia.hpp"
#include "dispatcher.hpp"

class MonitorView
    : public virtual Device
{
    // Types
private:

    class Handler
        : public Dispatcher::Handler
    {
    protected:
        MonitorView &monitor_view;
        Handler(Uint32 p_event_type, MonitorView &);
    public:
        virtual ~Handler() = default;
        void prepare_event(SDL_Event &) const;
    };

    class ResizeHandler
        : public Handler
    {
    public:
        explicit ResizeHandler(MonitorView &);
        virtual ~ResizeHandler() = default;
        virtual void handle(const SDL_Event &);
    };

    class CharacterHandler
        : public Handler
    {
    public:
        explicit CharacterHandler(MonitorView &);
        virtual ~CharacterHandler() = default;
        void push(Memory &, word, byte, Memory::AccessType);
        virtual void handle(const SDL_Event &);
    };

    class ModeHandler
        : public Handler
    {
    public:
        explicit ModeHandler(MonitorView &);
        virtual ~ModeHandler() = default;
        void push(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);
        virtual void handle(const SDL_Event &);
    };

    class Observer
        : public virtual AtomMonitorInterface::Observer
        , public virtual Memory::Observer
    {
    private:
        CharacterHandler   m_character_handler;
        ModeHandler        m_mode_handler;
    public:
        Observer(CharacterHandler &, ModeHandler &);
        virtual ~Observer() = default;
    private:
        // Memory::Observer
        virtual void set_byte_update(Memory &, word, byte, Memory::AccessType);
        // AtomMonitorInterface::Observer
        virtual void vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);
    };

public:
    class Configurator
        : public virtual Device::Configurator
    {
    protected:
        Configurator() = default;
	public:
        virtual ~Configurator() = default;
        virtual const Glib::ustring        &fontfilename() const = 0;
        virtual const Glib::ustring        &window_title() const = 0;
        virtual const Memory::Configurator *memory() const = 0;
        virtual const Memory::Configurator *ppia() const = 0;
        virtual Device *device_factory() const
            { return new MonitorView(*this); }

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };

    class Mode;
    class Mode0;

private:
    Ppia                 *m_ppia;
    Memory               *m_memory;
    SDL_Window           *m_window;
    SDL_Renderer         *m_renderer;
    std::vector<int>     m_rendered;
    Mode                 *m_mode;
    Mode0                *m_mode0;

    ResizeHandler        m_resize_handler;
    CharacterHandler     m_character_handler;
    ModeHandler          m_mode_handler;

    Observer             m_observer;
public:
    void window_resize();
    void set_byte_update(Memory &, word, byte, Memory::AccessType);
    void vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);

public:
    explicit MonitorView(const Configurator &);
    virtual ~MonitorView();

    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const MonitorView &p_mv)
        { p_mv.serialize(p_s); return p_s; }
};


#endif

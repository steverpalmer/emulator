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

    class WindowHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
    public:
        explicit WindowHandler(MonitorView &);
        virtual ~WindowHandler() = default;
        virtual void handle(const SDL_Event &);
    };

    class SetByteHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
    public:
        explicit SetByteHandler(MonitorView &);
        virtual ~SetByteHandler() = default;
        void push(Memory &, word, byte, Memory::AccessType) const;
        virtual void handle(const SDL_Event &);
    };

    class VdgModeHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
    public:
        explicit VdgModeHandler(MonitorView &);
        virtual ~VdgModeHandler() = default;
        void push(AtomMonitorInterface &, AtomMonitorInterface::VDGMode) const;
        virtual void handle(const SDL_Event &);
    };

    class Observer
        : public virtual AtomMonitorInterface::Observer
        , public virtual Memory::Observer
    {
    private:
        const SetByteHandler &m_set_byte_handler;
        const VdgModeHandler &m_vdg_mode_handler;
    public:
        Observer(const SetByteHandler &, const VdgModeHandler &);
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
    Ppia             *m_ppia;
    Memory           *m_memory;
    SDL_Window       *m_window;
    SDL_Renderer     *m_renderer;
    std::vector<int> m_rendered;
    Mode             *m_mode;
    Mode0            *m_mode0;

    const WindowHandler  m_window_handler;
    const SetByteHandler m_set_byte_handler;
    const VdgModeHandler m_vdg_mode_handler;
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

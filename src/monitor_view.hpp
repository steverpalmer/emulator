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
    : public Device
{
    // Types
private:

    class WindowHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
        WindowHandler();
    public:
        explicit WindowHandler(MonitorView &);
        virtual ~WindowHandler() = default;
        virtual void handle(const SDL_Event &) override;
    };

    class SetByteHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
        SetByteHandler();
    public:
        explicit SetByteHandler(MonitorView &);
        virtual ~SetByteHandler() = default;
        void push(Memory &, word, byte) const;
        virtual void handle(const SDL_Event &) override;
    };

    class VdgModeHandler
        : public Dispatcher::StateHandler<MonitorView>
    {
        VdgModeHandler();
    public:
        explicit VdgModeHandler(MonitorView &);
        virtual ~VdgModeHandler() = default;
        void push(Atom::MonitorInterface &, Atom::MonitorInterface::VDGMode) const;
        virtual void handle(const SDL_Event &) override;
    };

    class Observer
        : public virtual Atom::MonitorInterface::Observer
        , public virtual Memory::Observer
    {
    private:
        const SetByteHandler &m_set_byte_handler;
        const VdgModeHandler &m_vdg_mode_handler;
        Observer();
    public:
        Observer(const SetByteHandler &, const VdgModeHandler &);
        virtual ~Observer() = default;
    private:
        // Memory::Observer
        virtual void set_byte_update(Memory &, word, byte, Memory::AccessType) override;
        // Atom::MonitorInterface::Observer
        virtual void vdg_mode_update(Atom::MonitorInterface &, Atom::MonitorInterface::VDGMode) override;
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
        virtual float                      initial_scale() const = 0;
        virtual int                        sdl_renderer() const = 0;
        virtual const Memory::Configurator *memory() const = 0;
        virtual const Memory::Configurator *ppia() const = 0;
        virtual Device *device_factory() const override
            { return new MonitorView(*this); }

        virtual void serialize(std::ostream &) const override;
    };

    class Mode;

private:
    Ppia             *m_ppia;
    Memory           *m_memory;
    SDL_Window       *m_window;
    SDL_Renderer     *m_renderer;
    std::vector<int> m_rendered;
    Mode             *m_mode;
    std::map<Atom::MonitorInterface::VDGMode, Mode *> m_mode_map;

    const WindowHandler  m_window_handler;
    const SetByteHandler m_set_byte_handler;
    const VdgModeHandler m_vdg_mode_handler;
    Observer             m_observer;
public:
    void window_resize();
    void set_byte_update(Memory &, word, byte);
    void vdg_mode_update(Atom::MonitorInterface &, Atom::MonitorInterface::VDGMode);

public:
    explicit MonitorView(const Configurator &);
private:
    MonitorView();
    virtual ~MonitorView();
public:
    virtual void serialize(std::ostream &) const override;
};


#endif

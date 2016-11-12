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
        Handler(Uint32 p_event_type, MonitorView &p_monitor_view)
            : Dispatcher::Handler(p_event_type)
            , monitor_view(p_monitor_view)
            {}
    public:
        virtual ~Handler() = default;
        void prepare_event(SDL_Event &p_event) const
            {
                SDL_memset(&p_event, 0, sizeof(p_event));
                p_event.type = event_type;
            }
    };
    
    class ResizeHandler
        : public Handler
    {
    public:
        ResizeHandler(MonitorView &p_monitor_view)
            : Handler(SDL_WINDOWEVENT, p_monitor_view)
            {}
        virtual ~ResizeHandler() = default;
        virtual void handle(SDL_Event & p_event)
            {
                if (p_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    monitor_view.window_resize();
            }
    };
    
    class CharacterHandler
        : public Handler
    {
    public:
        CharacterHandler(MonitorView &p_monitor_view)
            : Handler(SDL_RegisterEvents(1), p_monitor_view)
            {}
        virtual ~CharacterHandler() = default;
        void push(Memory &p_memory, word p_addr, byte p_byte, Memory::AccessType p_at)
            {
                SDL_Event event;
                prepare_event(event);
                event.user.code = p_addr | (p_byte << 16) | (p_at << 24);
                event.user.data1 = &p_memory;
                const int rv = SDL_PushEvent(&event);
                assert (rv);
            }
        virtual void handle(SDL_Event &p_event)
            {
                monitor_view.set_byte_update( *static_cast<Memory *>(p_event.user.data1)
                                            , word(p_event.user.code)
                                            , byte(p_event.user.code >> 16)
                                            , Memory::AccessType(p_event.user.code >> 24)
                                            );
            }
    };
    
    class ModeHandler
        : public Handler
    {
    public:
        ModeHandler(MonitorView &p_monitor_view)
            : Handler(SDL_RegisterEvents(1), p_monitor_view)
            {}
        virtual ~ModeHandler() = default;
        void push(AtomMonitorInterface &p_monitor, AtomMonitorInterface::VDGMode p_mode)
            {
                SDL_Event event;
                prepare_event(event);
                event.user.code = p_mode;
                event.user.data1 = &p_monitor;
                const int rv = SDL_PushEvent(&event);
                assert (rv);
            }
        virtual void handle(SDL_Event &p_event)
            {
                monitor_view.vdg_mode_update( *static_cast<AtomMonitorInterface *>(p_event.user.data1)
                                            , AtomMonitorInterface::VDGMode(p_event.user.code)
                                            );
            }
    };
    
    class SubjectLossHandler
        : public Handler
    {
    public:
        SubjectLossHandler(MonitorView &p_monitor_view)
            : Handler(SDL_RegisterEvents(1), p_monitor_view)
            {}
        virtual ~SubjectLossHandler() = default;
        void push(const void *p_subject)
            {
                SDL_Event event;
                prepare_event(event);
                event.user.data1 = &p_subject;
                const int rv = SDL_PushEvent(&event);
                assert (rv);
            }
        virtual void handle(SDL_Event &p_event)
            {
                monitor_view.remove_child(*static_cast<Part *>(p_event.user.data1));
            }
    };
    
    class Observer
        : public virtual AtomMonitorInterface::Observer
        , public virtual Memory::Observer
    {
    private:
        CharacterHandler   m_character_handler;
        ModeHandler        m_mode_handler;
        SubjectLossHandler m_subject_loss_handler;
    public:
        Observer(CharacterHandler   &p_character_handler,
                 ModeHandler        &p_mode_handler,
                 SubjectLossHandler &p_subject_loss_handler)
            : m_character_handler(p_character_handler)
            , m_mode_handler(p_mode_handler)
            , m_subject_loss_handler(p_subject_loss_handler)
            {}
        virtual ~Observer() = default;
    private:
        // Memory::Observer
        virtual void set_byte_update(Memory &p_memory, word p_addr, byte p_byte, Memory::AccessType p_at)
            { m_character_handler.push(p_memory, p_addr, p_byte, p_at); }
        virtual void subject_loss(const Memory &p_memory)
            { m_subject_loss_handler.push(static_cast<const void *>(&p_memory)); }
        // AtomMonitorInterface::Observer
        virtual void vdg_mode_update(AtomMonitorInterface & p_atom_monitor, AtomMonitorInterface::VDGMode p_mode)
            { m_mode_handler.push(p_atom_monitor, p_mode); }
        virtual void subject_loss(const AtomMonitorInterface & p_atom_monitor)
            { m_subject_loss_handler.push(static_cast<const void *>(&p_atom_monitor)); }
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
    SubjectLossHandler   m_subject_loss_handler;

    Observer             m_observer;
public:
    void window_resize();
    void set_byte_update(Memory &, word, byte, Memory::AccessType);
    void vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);
    virtual void remove_child(Part &);

public:
    explicit MonitorView(const Configurator &);
    virtual ~MonitorView();
        
    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const MonitorView &p_mv)
        { p_mv.serialize(p_s); return p_s; }
};


#endif

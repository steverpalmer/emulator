// monitor_view.hpp
// Copyright 2016 Steve Palmer

#ifndef MONITOR_VIEW_HPP_
#define MONITOR_VIEW_HPP_

#include <vector>

#include <SDL.h>

#include "common.hpp"
#include "atom_monitor_interface.hpp"
#include "memory.hpp"

class MonitorView
    : public virtual AtomMonitorInterface::Observer
    , public virtual Memory::Observer
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
        virtual const Glib::ustring &fontfilename() const = 0;
        virtual const Glib::ustring &window_title() const = 0;

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };

    class Mode;
    class Mode0;

private:
    AtomMonitorInterface *m_atom_monitor;
    Memory               *m_memory;
    SDL_Window           *m_window;
    SDL_Renderer         *m_renderer;
    std::vector<int>     m_rendered;
    Mode                 *m_mode;
    Mode0                *m_mode0;
    const Uint32         m_set_byte_update_event_type;
    const Uint32         m_vdg_mode_update_event_type;
private:
    // AtomMonitorInterface Observer implementation
    void real_vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);
    virtual void vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode);
    virtual void subject_loss(const AtomMonitorInterface &);
    // Memory Observer implementation
    void real_set_byte_update(Memory &, word, byte, Memory::AccessType);
    virtual void set_byte_update(Memory &, word, byte, Memory::AccessType);
    virtual void subject_loss(const Memory &);
public:
    MonitorView(AtomMonitorInterface *, Memory *, const Configurator &);
    virtual ~MonitorView();

    bool handle_event(SDL_Event &);

    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const MonitorView &p_mv)
        { p_mv.serialize(p_s); return p_s; }
};


#endif

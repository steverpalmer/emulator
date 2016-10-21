// monitor_view.hpp

#ifndef MONITOR_VIEW_HPP_
#define MONITOR_VIEW_HPP_

#include <ostream>
#include <array>

#include <SDL.h>
#include <glibmm/ustring.h>

#include "terminal_interface.hpp"
#include "memory.hpp"

class MonitorView
    : public TerminalInterface::Observer
    , public Memory::Observer
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
        virtual float scale() const = 0;
        virtual const Glib::ustring &fontfilename() const = 0;
        virtual const Glib::ustring &window_title() const = 0;
        virtual const Glib::ustring &icon_title() const = 0;

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };

    class Mode
        : protected NonCopyable
    {
    protected:
        MonitorView &m_state;
    protected:
        Mode(MonitorView &p_state)
            : m_state(p_state) {}
    public:
        virtual ~Mode() = default;
        virtual void render() = 0;
        virtual void set_byte_update(word p_addr, byte p_byte) = 0;
    };

    class Mode0
        : public Mode
    {
    private:
        std::array<SDL_Surface *, 256> m_glyph;
        std::array<int, 512>           m_rendered;
    public:
        Mode0(MonitorView &p_state, const MonitorView::Configurator &p_cfgr);
        virtual ~Mode0();
        virtual void render();
        virtual void set_byte_update(word p_addr, byte p_byte);
    };

private:
    TerminalInterface *m_terminal_interface;  // FIXME: should be const
    Memory             *m_memory;  // FIXME: should be const
    SDL_Surface        *m_screen;
    MonitorView::Mode  *m_mode;
    MonitorView::Mode0 m_mode0;
private:
    SDL_Surface *scale_and_convert_surface(SDL_Surface *src);
    inline void render() { if (m_mode) m_mode->render(); }
    virtual void set_byte_update(Memory *p_memory, word p_addr, byte p_byte, Memory::AccessType p_at)
        { if (m_mode) m_mode->set_byte_update(p_addr, p_byte); }
    virtual void vdg_mode_update(TerminalInterface *p_terminal, TerminalInterface::VDGMode p_mode);
public:
    MonitorView(TerminalInterface *, Memory *, const Configurator &);
    virtual ~MonitorView();

    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const MonitorView &p_mv)
        { p_mv.serialize(p_s); return p_s; }
};


#endif

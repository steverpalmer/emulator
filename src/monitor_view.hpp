// monitor_view.hpp

#ifndef MONITOR_VIEW_HPP_
#define MONITOR_VIEW_HPP_

#include <vector>

#include <SDL.h>

#include "common.hpp"
#include "terminal_interface.hpp"
#include "memory.hpp"

class MonitorView
    : public virtual TerminalInterface::Observer
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

    class Mode
        : protected NonCopyable
    {
    protected:
        MonitorView *m_state;
    protected:
        Mode(MonitorView *p_state);
    public:
        virtual ~Mode() = default;
        virtual void render() = 0;
        virtual void set_byte_update(word p_addr, byte p_byte) = 0;
    };

    class Mode0
        : public Mode
    {
    private:
        SDL_Texture * m_font;
    public:
        Mode0(MonitorView *p_state, const MonitorView::Configurator &p_cfgr);
        virtual ~Mode0();
        virtual void render();
        virtual void set_byte_update(word p_addr, byte p_byte);
    };

private:
    TerminalInterface  *m_terminal_interface;
    Memory             *m_memory;
    SDL_Window         *m_window;
    SDL_Renderer       *m_renderer;
    std::vector<int>   m_rendered;
    MonitorView::Mode  *m_mode;
    MonitorView::Mode0 *m_mode0;
private:
    // Memory Observer implementation
    virtual void set_byte_update(Memory &, word, byte, Memory::AccessType);
    // TerminalInterface Observer implementation
    virtual void vdg_mode_update(const TerminalInterface &, TerminalInterface::VDGMode);
public:
    MonitorView(TerminalInterface *, Memory *, const Configurator &);
    virtual ~MonitorView();

    void handle_event(SDL_WindowEvent &);
    
    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const MonitorView &p_mv)
        { p_mv.serialize(p_s); return p_s; }
};


#endif

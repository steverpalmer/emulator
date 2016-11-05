// monitor_view.cpp
// Copyright 2016 Steve Palmer

#include <cassert>
#include <cmath>
#include <string>

#include <iomanip>

#include "monitor_view.hpp"

#define FONT_FNAME "mc6847.bmp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".monitor_view.cpp"));
    return result;
}

class MonitorView::Mode
    : protected NonCopyable
{
protected:
    MonitorView *m_state;
protected:
    explicit Mode(MonitorView *p_state)
        : m_state(p_state)
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode::Mode(" << p_state << ")");
            assert (m_state);
        }
public:
    virtual ~Mode() = default;
    virtual void set_byte_update(word p_addr, byte p_byte) = 0;
    virtual void render() = 0;
};

class MonitorView::Mode0
    : public Mode
{
private:
    SDL_Texture * m_font;
public:
    Mode0(MonitorView *p_state, const MonitorView::Configurator &p_cfgr)
        : Mode(p_state)
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::Mode0(" << &p_state << ", " << p_cfgr << ")");
            SDL_Surface *fontfile(SDL_LoadBMP(p_cfgr.fontfilename().c_str()));
            assert (fontfile);
            assert (fontfile->w == 256);
            assert (fontfile->h == 96);
            m_font = SDL_CreateTextureFromSurface(m_state->m_renderer, fontfile);
            assert (m_font);
            SDL_FreeSurface(fontfile);
        }
    virtual ~Mode0()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::~Mode0()");
            SDL_DestroyTexture(m_font);
        }
    virtual void set_byte_update(word p_addr, byte p_byte)
        {
            LOG4CXX_INFO(cpptrace_log()
                         , "MonitorView::Mode0::set_byte_update("
                         << Hex(p_addr)
                         << ", "
                         << Hex(p_byte)
                         << ")");
            SDL_Rect texture;
            texture.w = 8;
            texture.h = 12;
            texture.x = (p_byte & ((1u << 5u) - 1u)) * texture.w;
            texture.y = (p_byte >> 5u) * texture.h;
            SDL_Rect window;
            window.w = 8;
            window.h = 12;
            window.x = (p_addr & ((1u << 5u) - 1u)) * window.w;
            window.y = (p_addr >> 5u) * window.h;
            const int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &texture, &window);
            assert (!rv);
            m_state->m_rendered[p_addr] = p_byte;
            SDL_RenderPresent(m_state->m_renderer);
        }
    virtual void render()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::render()");
            int rv = SDL_RenderClear(m_state->m_renderer);
            SDL_Rect texture;
            texture.w = 8;
            texture.h = 12;
            SDL_Rect window;
            window.w = 8;
            window.h = 12;
            assert (!rv);
            for (int addr = 0; addr < 512; addr++)
            {
                const byte ch = m_state->m_memory->get_byte(addr);
                texture.x = (ch & ((1u << 5u) - 1u)) * texture.w;
                texture.y = (ch >> 5u) * texture.h;
                window.x = (addr & ((1u << 5u) - 1u)) * window.w;
                window.y = (addr >> 5u) * window.h;
                int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &texture, &window);
                assert (!rv);
                m_state->m_rendered[addr] = ch;
            }
            SDL_RenderPresent(m_state->m_renderer);
        }
};

MonitorView::MonitorView(TerminalInterface *p_terminal_interface,
                         Memory *p_memory,
                         const Configurator &p_cfgr)
    : m_terminal_interface(p_terminal_interface)
    , m_memory(p_memory)
    , m_rendered(m_memory->size())
    , m_mode(0)
    , m_mode0(0)
{
    assert (p_terminal_interface);
    assert (p_memory);
    LOG4CXX_INFO(cpptrace_log(),
                 "MonitorView::MonitorView("
                 << "<controller name=\"" << p_terminal_interface->id() << "\"/>"
                 << "<memory name=\"" << p_memory->id() << "\"/>"
                 << p_cfgr << ")");
    m_window = SDL_CreateWindow(p_cfgr.window_title().c_str(),
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                512, 384,
                                SDL_WINDOW_RESIZABLE);
    assert (m_window);
    m_renderer = SDL_CreateRenderer(m_window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED);
    assert (m_renderer);
    const int rv = SDL_RenderSetLogicalSize(m_renderer, 256, 192);
    assert (!rv);
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
    m_mode0 = new Mode0(this, p_cfgr);
    m_terminal_interface->attach(*this);
    m_memory->attach(*this);
}

MonitorView::~MonitorView()
{
    LOG4CXX_INFO(cpptrace_log(), "~MonitorView::MonitorView()");
    m_memory->detach(*this);
    m_terminal_interface->detach(*this);
    m_mode = 0;
    if (m_mode0)
    {
        delete m_mode0;
        m_mode0 = 0;
    }
    if (m_renderer)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = 0;
    }
    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = 0;
    }
}

void MonitorView::handle_event(SDL_WindowEvent &p_window_event)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::handle_event(" << p_window_event.event << ")");
    switch (p_window_event.event)
    {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
        if (m_mode)
            m_mode->render();
        break;
    default:
        break;
    }
}

void MonitorView::set_byte_update(Memory &, word p_addr, byte p_byte, Memory::AccessType p_at)
{
    if (m_mode)
        if (p_byte != m_rendered[p_addr])
            m_mode->set_byte_update(p_addr, p_byte);
}

void MonitorView::vdg_mode_update(const TerminalInterface &, TerminalInterface::VDGMode p_mode)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::vdg_mode_update(" << p_mode << ")");
    switch (p_mode)
    {
    case TerminalInterface::VDG_MODE0:
        m_mode = m_mode0;
        break;
    default:
        LOG4CXX_ERROR(cpptrace_log(), "Can't render graphics mode " << p_mode);
        m_mode = 0;
    }
    if (m_mode)
        m_mode->render();
}

void MonitorView::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<fontfilename>" << fontfilename() << "</fontfilename>"
        << "<windowtitle>"  << window_title() << "</windowtitle>";
}

void MonitorView::serialize(std::ostream &p_s) const
{
    p_s << "MonitorView("
        << ")";
}

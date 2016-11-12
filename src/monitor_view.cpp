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
            LOG4CXX_INFO(SDL::log(), "SDL_LoadBMP(" << p_cfgr.fontfilename().c_str() << ")");
            SDL_Surface *fontfile(SDL_LoadBMP(p_cfgr.fontfilename().c_str()));
            assert (fontfile);
            assert (fontfile->w == 256);
            assert (fontfile->h == 96);
            LOG4CXX_INFO(SDL::log(), "SDL_CreateTextureFromSurface(...)");
            m_font = SDL_CreateTextureFromSurface(m_state->m_renderer, fontfile);
            assert (m_font);
            LOG4CXX_INFO(SDL::log(), "SDL_FreeSurface(...)");
            SDL_FreeSurface(fontfile);
        }
    virtual ~Mode0()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::~Mode0()");
            LOG4CXX_INFO(SDL::log(), "SDL_DestroyTexture(...)");
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
            LOG4CXX_INFO(SDL::log(), "SDL_RenderCopy(...)");
            const int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &texture, &window);
            assert (!rv);
            m_state->m_rendered[p_addr] = p_byte;
            LOG4CXX_INFO(SDL::log(), "SDL_PresentRender(...)");
            SDL_RenderPresent(m_state->m_renderer);
        }
    virtual void render()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::render()");
            if (m_state->m_memory) {
                LOG4CXX_INFO(SDL::log(), "SDL_RenderClear(...)");
                int rv = SDL_RenderClear(m_state->m_renderer);
                assert (!rv);
                SDL_Rect texture;
                texture.w = 8;
                texture.h = 12;
                SDL_Rect window;
                window.w = 8;
                window.h = 12;
                for (int addr = 0; addr < 512; addr++)
                {
                    const byte ch = m_state->m_memory->get_byte(addr);
                    texture.x = (ch & ((1u << 5u) - 1u)) * texture.w;
                    texture.y = (ch >> 5u) * texture.h;
                    window.x = (addr & ((1u << 5u) - 1u)) * window.w;
                    window.y = (addr >> 5u) * window.h;
                    LOG4CXX_INFO(SDL::log(), "SDL_RenderCopy(...)");
                    int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &texture, &window);
                    assert (!rv);
                    m_state->m_rendered[addr] = ch;
                }
                LOG4CXX_INFO(SDL::log(), "SDL_PresentRender(...)");
                SDL_RenderPresent(m_state->m_renderer);
            }
        }
};

MonitorView::MonitorView(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , m_ppia(dynamic_cast<Ppia *>(p_cfgr.ppia()->memory_factory()))
    , m_memory(p_cfgr.memory()->memory_factory())
    , m_rendered(m_memory->size())
    , m_mode(0)
    , m_mode0(0)
    , m_resize_handler(*this)
    , m_character_handler(*this)
    , m_mode_handler(*this)
    , m_subject_loss_handler(*this)
    , m_observer(m_character_handler, m_mode_handler, m_subject_loss_handler)
{
    assert (m_memory);
    LOG4CXX_INFO(Part::log(), "making [" << m_memory->id() << "] child of [" << id() << "]");
    m_memory->add_parent(*this);
    assert (m_ppia);
    LOG4CXX_INFO(Part::log(), "making [" << m_ppia->id() << "] child of [" << id() << "]");
    m_ppia->add_parent(*this);
    
    LOG4CXX_INFO(SDL::log(), "SDL_CreateWindow(...)");
    m_window = SDL_CreateWindow(p_cfgr.window_title().c_str(),
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                512, 384,
                                SDL_WINDOW_RESIZABLE);
    assert (m_window);
    LOG4CXX_INFO(SDL::log(), "SDL_CreateRenderer(...)");
    m_renderer = SDL_CreateRenderer(m_window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED);
    assert (m_renderer);
    LOG4CXX_INFO(SDL::log(), "SDL_RenderSetLogicalSize(..., 256, 192)");
    const int rv = SDL_RenderSetLogicalSize(m_renderer, 256, 192);
    assert (!rv);
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
    m_mode0 = new Mode0(this, p_cfgr);
    
    m_memory->attach(m_observer);
    m_ppia->AtomMonitorInterface::attach(m_observer);
}

MonitorView::~MonitorView()
{
    if (m_memory)
    {
        m_memory->detach(m_observer);
        remove_child(*m_memory);
    }
    if (m_ppia)
    {
        m_ppia->AtomMonitorInterface::detach(m_observer);
        remove_child(*m_ppia);
    }
    m_mode = 0;
    if (m_mode0)
    {
        delete m_mode0;
        m_mode0 = 0;
    }
    if (m_renderer)
    {
        LOG4CXX_INFO(SDL::log(), "SDL_DestroyRenderer(...)");
        SDL_DestroyRenderer(m_renderer);
        m_renderer = 0;
    }
    if (m_window)
    {
        LOG4CXX_INFO(SDL::log(), "SDL_DestroyWindow(...)");
        SDL_DestroyWindow(m_window);
        m_window = 0;
    }
}

void MonitorView::window_resize()
{
    if (m_mode)
        m_mode->render();
}

void MonitorView::set_byte_update(Memory &, word p_addr, byte p_byte, Memory::AccessType)
{
    if (m_mode)
        if (p_byte != m_rendered[p_addr])
            m_mode->set_byte_update(p_addr, p_byte);
}

void MonitorView::vdg_mode_update(AtomMonitorInterface &, AtomMonitorInterface::VDGMode p_mode)
{
    switch (p_mode)
    {
    case AtomMonitorInterface::VDG_MODE0:
        m_mode = m_mode0;
        break;
    default:
        m_mode = 0;
    }
    if (m_mode)
        m_mode->render();
}

void MonitorView::remove_child(Part &p_child)
{
    if (&p_child == m_memory)
    {
        LOG4CXX_INFO(Part::log(), "removing video memory child of [" << id() << "]");
        m_memory = 0;
    }
    if (&p_child == m_ppia)
    {
        LOG4CXX_INFO(Part::log(), "removing ppia child of [" << id() << "]");
        m_ppia = 0;
    }
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

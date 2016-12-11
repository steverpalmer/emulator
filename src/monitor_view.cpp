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

MonitorView::WindowHandler::WindowHandler(MonitorView &p_monitor_view)
    : Dispatcher::StateHandler<MonitorView>(SDL_WINDOWEVENT, p_monitor_view)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::WindowHandler::WindowHandler(...)");
}

void MonitorView::WindowHandler::handle(const SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::WindowHandler::handle(" << int(p_event.window.event) << ")");
    switch (p_event.window.event)
    {
    case SDL_WINDOWEVENT_SHOWN:
    case SDL_WINDOWEVENT_EXPOSED:
    case SDL_WINDOWEVENT_MOVED:
    case SDL_WINDOWEVENT_RESIZED:
    case SDL_WINDOWEVENT_SIZE_CHANGED:
    case SDL_WINDOWEVENT_MAXIMIZED:
    case SDL_WINDOWEVENT_RESTORED:
    default:
        state.window_resize();
        break;
    case SDL_WINDOWEVENT_MINIMIZED:
    case SDL_WINDOWEVENT_NONE:
    case SDL_WINDOWEVENT_HIDDEN:
    case SDL_WINDOWEVENT_ENTER:
    case SDL_WINDOWEVENT_LEAVE:
    case SDL_WINDOWEVENT_FOCUS_GAINED:
    case SDL_WINDOWEVENT_FOCUS_LOST:
    case SDL_WINDOWEVENT_CLOSE:
    	break;
    }
}

MonitorView::SetByteHandler::SetByteHandler(MonitorView &p_monitor_view)
    : Dispatcher::StateHandler<MonitorView>(p_monitor_view)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::SetByteHandler::SetByteHandler(...)");
}

void MonitorView::SetByteHandler::push(Memory &p_memory, word p_addr, byte p_byte) const
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::SetByteHandler::push(" << Hex(p_addr) << ", " << Hex(p_byte) << ")");
    SDL_Event event;
    prepare(event);
    event.user.code = p_addr | (p_byte << 16);
    event.user.data1 = &p_memory;
    Dispatcher::Handler::push(event);
}

void MonitorView::SetByteHandler::handle(const SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::CharacterHandler::handle(" << p_event.user.code << ")");
    state.set_byte_update( *static_cast<Memory *>(p_event.user.data1)
                         , word(p_event.user.code)
                         , byte(p_event.user.code >> 16)
                         );
}

MonitorView::VdgModeHandler::VdgModeHandler(MonitorView &p_monitor_view)
    : Dispatcher::StateHandler<MonitorView>(p_monitor_view)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::VdgModeHandler::VdgModeHandler(...)");
}

void MonitorView::VdgModeHandler::push(Atom::MonitorInterface &p_monitor, Atom::MonitorInterface::VDGMode p_mode) const
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::VdgModeHandler::push(" << p_mode << ")");
    SDL_Event event;
    prepare(event);
    event.user.code = static_cast<int>(p_mode);
    event.user.data1 = &p_monitor;
    Dispatcher::Handler::push(event);
}

void MonitorView::VdgModeHandler::handle(const SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::ModeHandler::handle(" << p_event.user.code << ")");
    state.vdg_mode_update( *static_cast<Atom::MonitorInterface *>(p_event.user.data1)
                         , Atom::MonitorInterface::VDGMode(p_event.user.code)
                         );
}

MonitorView::Observer::Observer(const SetByteHandler &p_set_byte_handler,
                                const VdgModeHandler &p_vdg_mode_handler)
    : m_set_byte_handler(p_set_byte_handler)
    , m_vdg_mode_handler(p_vdg_mode_handler)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Observer::Observer(...)");
}

void MonitorView::Observer::set_byte_update( Memory &p_memory
                                           , word p_addr
                                           , byte p_byte
                                           , Memory::AccessType)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Observer::set_byte_update(" << Hex(p_addr) << ", " << Hex(p_byte) << ")");
    m_set_byte_handler.push(p_memory, p_addr, p_byte);
}

void MonitorView::Observer::vdg_mode_update( Atom::MonitorInterface & p_monitor
                                           , Atom::MonitorInterface::VDGMode p_mode)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Observer::set_mode_update(" << p_mode << ")");
    m_vdg_mode_handler.push(p_monitor, p_mode);
}


class MonitorView::Mode
    : public Part
{
protected:
    MonitorView *m_state;
protected:
    explicit Mode(const Part::id_type p_id, MonitorView *p_state)
        : Part(p_id)
        , m_state(p_state)
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
        : Mode("Mode 0", p_state)
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
    virtual void set_byte_update(word p_addr, byte p_byte) override
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
    virtual void render() override
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

class MonitorView::Mode4
    : public Mode
{
private:
    SDL_Texture * m_font;
public:
    Mode4(MonitorView *p_state, const MonitorView::Configurator &p_cfgr)
        : Mode("Mode 4", p_state)
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode4::Mode4(" << &p_state << ", " << p_cfgr << ")");
            LOG4CXX_INFO(SDL::log(), "SDL_LoadBMP(...)");
            SDL_Surface *fontfile(SDL_LoadBMP("plot.bmp"));
            assert (fontfile);
            assert (fontfile->w == 8);
            assert (fontfile->h == 256);
            LOG4CXX_INFO(SDL::log(), "SDL_CreateTextureFromSurface(...)");
            m_font = SDL_CreateTextureFromSurface(m_state->m_renderer, fontfile);
            assert (m_font);
            LOG4CXX_INFO(SDL::log(), "SDL_FreeSurface(...)");
            SDL_FreeSurface(fontfile);
        }
    virtual ~Mode4()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode4::~Mode4()");
            LOG4CXX_INFO(SDL::log(), "SDL_DestroyTexture(...)");
            SDL_DestroyTexture(m_font);
        }
    virtual void set_byte_update(word p_addr, byte p_byte) override
        {
            LOG4CXX_INFO(cpptrace_log()
                         , "MonitorView::Mode4::set_byte_update("
                         << Hex(p_addr)
                         << ", "
                         << Hex(p_byte)
                         << ")");
            SDL_Rect texture;
            texture.w = 8;
            texture.h = 1;
            texture.x = 0;
            texture.y = p_byte;
            SDL_Rect window;
            window.w = 8;
            window.h = 1;
            window.x = (p_addr & ((1u << 5u) - 1u)) * window.w;
            window.y = (p_addr >> 5u);
            LOG4CXX_INFO(SDL::log(), "SDL_RenderCopy(...)");
            const int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &texture, &window);
            assert (!rv);
            m_state->m_rendered[p_addr] = p_byte;
            LOG4CXX_INFO(SDL::log(), "SDL_PresentRender(...)");
            SDL_RenderPresent(m_state->m_renderer);
        }
    virtual void render() override
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode4::render()");
            if (m_state->m_memory) {
                LOG4CXX_INFO(SDL::log(), "SDL_RenderClear(...)");
                int rv = SDL_RenderClear(m_state->m_renderer);
                assert (!rv);
                SDL_Rect texture;
                texture.w = 8;
                texture.h = 1;
                SDL_Rect window;
                window.w = 8;
                window.h = 1;
                for (int addr = 0; addr < 0x1800; addr++)
                {
                    const byte ch = m_state->m_memory->get_byte(addr);
                    texture.x = 0;
                    texture.y = ch;
                    window.x = (addr & ((1u << 5u) - 1u)) * window.w;
                    window.y = (addr >> 5u);
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
    , m_mode(nullptr)
    , m_mode0(nullptr)
	, m_mode4(nullptr)
    , m_window_handler(*this)
    , m_set_byte_handler(*this)
    , m_vdg_mode_handler(*this)
    , m_observer(m_set_byte_handler, m_vdg_mode_handler)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::MonitorView(" << p_cfgr << ")");
    assert (m_memory);
    LOG4CXX_INFO(Part::log(), "making [" << m_memory->id() << "] child of [" << id() << "]");
    assert (m_ppia);
    LOG4CXX_INFO(Part::log(), "making [" << m_ppia->id() << "] child of [" << id() << "]");

    LOG4CXX_INFO(SDL::log(), "SDL_CreateWindow(...)");
    m_window = SDL_CreateWindow(p_cfgr.window_title().c_str(),
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                int(256 * p_cfgr.initial_scale()), int(192 * p_cfgr.initial_scale()),
                                SDL_WINDOW_RESIZABLE);
    assert (m_window);
    LOG4CXX_INFO(SDL::log(), "SDL_CreateRenderer(...)");
    m_renderer = SDL_CreateRenderer(m_window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED);
    assert (m_renderer);
    LOG4CXX_INFO(SDL::log(), "SDL_RenderSetLogicalSize(..., 256, 192)");
    int rv = SDL_RenderSetLogicalSize(m_renderer, 256, 192);
    assert (!rv);
#if 0
    LOG4CXX_INFO(SDL::log(), "SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, \"0\")");
    rv = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    assert (rv);

    LOG4CXX_INFO(SDL::log(), "SDL_RenderSetIntegerScale(..., SDL_TRUE");
    rv = SDL_RenderSetIntegerScale(m_renderer, SDL_TRUE);
    assert (!rv);
#endif
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
    m_mode0 = new Mode0(this, p_cfgr);
    assert (m_mode0);
    m_mode4 = new Mode4(this, p_cfgr);
    assert (m_mode4);
    LOG4CXX_INFO(Part::log(), "making [" << m_mode0->id() << "] child of [" << id() << "]");

    m_memory->attach(m_observer);
    m_ppia->Atom::MonitorInterface::attach(m_observer);
}

MonitorView::~MonitorView()
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::~MonitorView()");
    LOG4CXX_INFO(Part::log(), "removing children of [" << id() << "]");
    m_memory = nullptr;
    m_ppia = nullptr;
    m_mode = nullptr;
    m_mode0 = nullptr;
    m_mode4 = nullptr;
    if (m_renderer)
    {
        LOG4CXX_INFO(SDL::log(), "SDL_DestroyRenderer(...)");
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window)
    {
        LOG4CXX_INFO(SDL::log(), "SDL_DestroyWindow(...)");
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void MonitorView::window_resize()
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::window_resize()");
    if (m_mode)
        m_mode->render();
}

void MonitorView::set_byte_update(Memory &, word p_addr, byte p_byte)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::set_byte_update(" << Hex(p_addr) << ", " << Hex(p_byte) << ")");
    if (m_mode)
        if (p_byte != m_rendered[p_addr])
            m_mode->set_byte_update(p_addr, p_byte);
}

void MonitorView::vdg_mode_update(Atom::MonitorInterface &, Atom::MonitorInterface::VDGMode p_mode)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::vdg_mode_update(" << p_mode << ")");
    switch (p_mode)
    {
    case Atom::MonitorInterface::VDGMode::MODE0:
        m_mode = m_mode0;
        break;
    case Atom::MonitorInterface::VDGMode::MODE4:
        m_mode = m_mode4;
        break;
    default:
        m_mode = nullptr;
    }
    if (m_mode)
        m_mode->render();
}


void MonitorView::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<monitor>"
        << "<controller>" << *ppia() << "</controller>"
        << "<video>" << *memory() << "</video>"
		<< "<scale>" << initial_scale() << "</scale>"
        << "<fontfilename>" << fontfilename() << "</fontfilename>"
        << "<windowtitle>"  << window_title() << "</windowtitle>"
        << "</monitor>";
}

void MonitorView::serialize(std::ostream &p_s) const
{
    p_s << "MonitorView(";
    p_s << "Mode(" << (m_mode?m_mode->id():"?") << ")";
    if (m_ppia) p_s << ", " << *m_ppia;
    if (m_memory) p_s << "," << *m_memory;
    p_s << ")";
}

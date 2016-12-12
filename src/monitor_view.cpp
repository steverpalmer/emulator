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
private:
    MonitorView *m_state;
    SDL_Texture *m_font;
    SDL_Rect     m_texture_rect;
    int          m_texture_cols;
    SDL_Rect     m_window_rect;
    int          m_window_cols;
    int          m_address_max;
public:
    Mode(const Part::id_type p_id, MonitorView *p_state, int p_x, int p_y, const Glib::ustring &p_fontfilename)
        : Part(p_id)
        , m_state(p_state)
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode::Mode(" << p_id << ", ..., " << p_x << ", " << p_y << ", " << p_fontfilename << ")");
            assert (m_state);
            SDL_Surface *fontfile(SDL_LoadBMP(p_fontfilename.c_str()));
            assert (fontfile);
            LOG4CXX_INFO(SDL::log(), "SDL_CreateTextureFromSurface(...)");
            m_font = SDL_CreateTextureFromSurface(m_state->m_renderer, fontfile);
            assert (m_font);
            m_texture_rect.w = 8;  // 8 pixels per character
            m_texture_cols = fontfile->w / m_texture_rect.w;
            assert (m_texture_cols * m_texture_rect.w == fontfile->w);  // BMP x-size must be a multiple of each 8 pixel character
            const int texture_rows(256 / m_texture_cols);
            assert (texture_rows * m_texture_cols == 256);  // BMP must divisible into 256 characters
            m_texture_rect.h = fontfile->h / texture_rows;
            assert (m_texture_rect.h * texture_rows == fontfile->h);  // BMP y-size must be a multiple of each characters
            assert (p_x == 128 || p_x == 256);
            m_window_cols = p_x / m_texture_rect.w;
            assert (m_window_cols * m_texture_rect.w == p_x);
            m_window_rect.w = 256 / m_window_cols;
            assert (m_window_rect.w * m_window_cols == 256);
            assert (p_y == 192 || p_y == 96 || p_y == 64);
            m_window_rect.h = m_texture_rect.h * 192 / p_y;
            m_address_max = p_x * p_y / (m_texture_rect.w * m_texture_rect.h);
            LOG4CXX_INFO(SDL::log(), "SDL_FreeSurface(...)");
            SDL_FreeSurface(fontfile);
        }
private:
    virtual ~Mode()
    {
        LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]MonitorView::Mode::~Mode()");
        LOG4CXX_INFO(SDL::log(), "SDL_DestroyTexture(...)");
        SDL_DestroyTexture(m_font);
    }
    void render_one(word p_addr, byte p_byte)
    {
        const int texture_y(p_byte / m_texture_cols);
        m_texture_rect.y = texture_y * m_texture_rect.h;
        m_texture_rect.x = (p_byte - (texture_y * m_texture_cols)) * m_texture_rect.w;
        const int window_y(p_addr / m_window_cols);
        m_window_rect.y = window_y * m_window_rect.h;
        m_window_rect.x = (p_addr - (window_y * m_window_cols)) * m_window_rect.w;
        LOG4CXX_INFO(SDL::log(), "SDL_RenderCopy(...)");
        const int rv = SDL_RenderCopy(m_state->m_renderer, m_font, &m_texture_rect, &m_window_rect);
        assert (!rv);
        m_state->m_rendered[p_addr] = p_byte;
    }
public:
    void set_byte_update(word p_addr, byte p_byte)
    {
        LOG4CXX_INFO(cpptrace_log()
                     , "MonitorView::Mode::set_byte_update("
                     << Hex(p_addr)
                     << ", "
                     << Hex(p_byte)
                     << ")");
        render_one(p_addr, p_byte);
        LOG4CXX_INFO(SDL::log(), "SDL_PresentRender(...)");
        SDL_RenderPresent(m_state->m_renderer);
    }
    void render()
        {
            LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode::render()");
            if (m_state->m_memory)
            {
                LOG4CXX_INFO(SDL::log(), "SDL_RenderClear(...)");
                int rv = SDL_RenderClear(m_state->m_renderer);
                assert (!rv);
                for (int addr = 0; addr < m_address_max; addr++)
                    render_one(addr, m_state->m_memory->get_byte(addr));
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

    LOG4CXX_INFO(SDL::log(), "SDL_GetNumRenderDrivers(...)");
    const int num_render_drivers(SDL_GetNumRenderDrivers());
    assert (num_render_drivers >= 1);
    for (int i(0); i < num_render_drivers; i += 1)
    {
    	SDL_RendererInfo renderer_info;
        LOG4CXX_INFO(SDL::log(), "SDL_GetRenderDriverInfo(" << i << ", ...)");
    	const int rc = SDL_GetRenderDriverInfo(i, &renderer_info);
    	assert (!rc);
    	LOG4CXX_DEBUG(SDL::log(), renderer_info.name);
    }

    LOG4CXX_INFO(SDL::log(), "SDL_CreateWindow(...)");
    m_window = SDL_CreateWindow(p_cfgr.window_title().c_str(),
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                int(256 * p_cfgr.initial_scale()), int(192 * p_cfgr.initial_scale()),
                                SDL_WINDOW_RESIZABLE);
    assert (m_window);
    LOG4CXX_INFO(SDL::log(), "SDL_CreateRenderer(...)");
    m_renderer = SDL_CreateRenderer(m_window,
    		                        p_cfgr.sdl_renderer(),
									0);
    assert (m_renderer);
    LOG4CXX_INFO(SDL::log(), "SDL_RenderSetLogicalSize(..., 256, 192)");
    int rv = SDL_RenderSetLogicalSize(m_renderer, 256, 192);
    assert (!rv);
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
    m_mode_map[Atom::MonitorInterface::VDGMode::MODE0] = new MonitorView::Mode("Mode 0", this, 256, 192, p_cfgr.fontfilename());
    m_mode_map[Atom::MonitorInterface::VDGMode::MODE1] = new MonitorView::Mode("Mode 1", this, 128,  64, "plot.bmp");
    m_mode_map[Atom::MonitorInterface::VDGMode::MODE2] = new MonitorView::Mode("Mode 2", this, 128,  96, "plot.bmp");
    m_mode_map[Atom::MonitorInterface::VDGMode::MODE3] = new MonitorView::Mode("Mode 3", this, 128, 192, "plot.bmp");
    m_mode_map[Atom::MonitorInterface::VDGMode::MODE4] = new MonitorView::Mode("Mode 4", this, 256, 192, "plot.bmp");

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
    m_mode_map.clear();
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
    try
    {
    	m_mode = m_mode_map.at(p_mode);
    }
    catch (std::out_of_range &e)
    {
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
		<< "<sdl_renderer>" << sdl_renderer() << "</sdl_renderer>"
        << "<fontfilename>" << fontfilename() << "</fontfilename>"
        << "<windowtitle>"  << window_title() << "</windowtitle>"
        << "</monitor>";
}

void MonitorView::serialize(std::ostream &p_s) const
{
    p_s << "MonitorView(";
    p_s << "Mode(" << (m_mode?m_mode->id():"?") << ")";
    SDL_RendererInfo renderer_info;
    if (!SDL_GetRendererInfo(m_renderer, &renderer_info))
    	p_s << ", " << renderer_info.name;
    if (m_ppia) p_s << ", " << *m_ppia;
    if (m_memory) p_s << "," << *m_memory;
    p_s << ")";
}

// monitor_view.cpp

#include <cassert>
#include <cmath>
#include <string>

#include <ostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "monitor_view.hpp"

#define FONT_FNAME "mc6847.bmp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".monitor_view.cpp"));
    return result;
}


/// SurfaceArray overlays a Surface with an square array
class SurfaceArray {
private:
    SDL_Surface *m_array;
    SDL_Rect     m_pos;
    int          m_x_range;
    int          m_key;
public:
    // SDL_Surface *data() const { return m_array; }
    inline int  key() const { return m_key; }
    inline void update() { SDL_UpdateRect(m_array, m_pos.x, m_pos.y, m_pos.w, m_pos.h); }

    SurfaceArray( SDL_Surface *p_surface, int x_range = 1, int y_range = 1, int p_key = 0 )
        : m_array(p_surface)
        , m_x_range(x_range)
        , m_key(p_key)
        {
            LOG4CXX_INFO(cpptrace_log(), "SurfaceArray::SurfaceArray(" << p_surface << ", " << x_range << ", " << y_range << ", " << p_key << ")");
            assert (p_surface);
            assert (p_surface->w % x_range == 0);
            assert (p_surface->h % y_range == 0);
            m_pos.w = p_surface->w / x_range;
            m_pos.h = p_surface->h / y_range;
            m_pos   = at(p_key);
        }
    // rely on the default copy constructor
    // SurfaceArray( SurfaceArray &other );
    // SurfaceArray( const SurfaceArray &other );
    // rely on the default assignment operator
    // SurfaceArray &operator=( SurfaceArray &other );
    // SurfaceArray &operator=( const SurfaceArray &other );
    // rely on the default destructor
    // ~SurfaceArray();
    SurfaceArray  &operator++()
        {
            LOG4CXX_INFO(cpptrace_log(), "operator++()");
            m_key++;
            m_pos.x += m_pos.w;
            if (m_pos.x >= m_array->w) {
                m_pos.x = 0;
                m_pos.y += m_pos.h;
            }
            return *this;
        }
#if 0
    SurfaceArray  &operator--()
        {
            LOG4CXX_INFO(cpptrace_log(), "operator--()");
            if (m_pos.x == 0)
            {
                m_pos.x = m_array->w;
                m_pos.y -= m_array->h;
            }
            m_pos.x -= m_pos.w;
            m_key--;
            return *this;
        }
#endif
    operator bool() const
        {
            LOG4CXX_INFO(cpptrace_log(), "operator bool()");
            return (0 <= m_pos.x && m_pos.x + m_pos.w <= m_array->w) &&
                (0 <= m_pos.y && m_pos.y + m_pos.h <= m_array->h);
        }
#if 0
    bool fill(Uint32 color)
        {
            LOG4CXX_INFO(cpptrace_log(), "fill(" << color << ")");
            const int rv = SDL_FillRect(m_array, &m_pos, color);
            return rv == 0;
        }
#endif
    SDL_Rect at(int p_key) const
        {
            LOG4CXX_INFO(cpptrace_log(), "at(" << p_key << ")");
            SDL_Rect result(m_pos);
            const int y(p_key / m_x_range);
            result.x = (p_key - y * m_x_range) * result.w;
            result.y = y * result.h;
            return result;
        }
    void set(const SurfaceArray &other, int p_key = -1)
        {
            LOG4CXX_INFO(cpptrace_log(), "set(" << p_key << ")");
            SDL_Rect tmp(other.m_pos); // Nasty Kludge because of SDL typing
            if (p_key >= 0)
                m_pos = at(p_key);
            const int rv = SDL_BlitSurface(other.m_array, &tmp, m_array, &m_pos);
            assert (!rv);
            update();
        }
    SDL_Surface *get(int p_key = -1) const
        {
            LOG4CXX_INFO(cpptrace_log(), "get(" << p_key << ")");
            int rv;
            SDL_Rect tmp( p_key < 0 ? m_pos : at(p_key));
            const SDL_PixelFormat *format = m_array->format;
            assert (format->BitsPerPixel == 8);
            SDL_Surface *result = SDL_CreateRGBSurface( m_array->flags,
                                                        m_pos.w,
                                                        m_pos.h,
                                                        format->BitsPerPixel,
                                                        format->Rmask,
                                                        format->Gmask,
                                                        format->Bmask,
                                                        format->Amask );
            if (result) {
                if (format->BitsPerPixel == 8) {
                    rv = SDL_SetPalette( result,
                                         SDL_LOGPAL,
                                         format->palette->colors,
                                         0,
                                         format->palette->ncolors );
                    assert (rv);
                }
                rv = SDL_BlitSurface(m_array, &tmp, result, 0);
                assert (!rv);
            }
            return result;
        }
    SDL_Surface *get(int new_w, int new_h) const
        {
            LOG4CXX_INFO(cpptrace_log(), "get(" << new_w << ", " << new_h << ")");
            int rv;
            assert (new_w >= m_pos.w);
            assert (new_h >= m_pos.h);
            const SDL_PixelFormat *format = m_array->format;
            assert (format->BitsPerPixel == 8);
            SDL_Surface *result = SDL_CreateRGBSurface( m_array->flags,
                                                        new_w,
                                                        new_h,
                                                        format->BitsPerPixel,
                                                        format->Rmask,
                                                        format->Gmask,
                                                        format->Bmask,
                                                        format->Amask);
            if (result) {
                if (format->BitsPerPixel == 8) {
                    rv = SDL_SetPalette( result,
                                         SDL_LOGPAL,
                                         format->palette->colors,
                                         0,
                                         format->palette->ncolors );
                    assert (rv);
                }
                rv = SDL_LockSurface(result);
                assert (!rv);
                byte *rbp=(byte *)result->pixels;
                for (int y = 0; y < new_h; y++) {
                    const int src_y((m_pos.h * y) / new_h + m_pos.y);
                    const byte *sbp(((byte *)m_array->pixels) + (src_y * m_array->pitch));
                    for (int x = 0; x < new_w; x++, rbp++) {
                        const int src_x((m_pos.w * x) / new_w + m_pos.x);
                        *rbp = sbp[src_x];
                    }
                }
                SDL_UnlockSurface(result);
            }
            return result;
        }
};

static int scale2character_width(float p_scale)
{
    return std::floor(8 * p_scale);
}

static int scale2character_height(float p_scale)
{
    return std::floor(12 * p_scale);
}

MonitorView::Mode::Mode(MonitorView *p_state)
    : m_state(p_state)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode::Mode(" << p_state << ")");
    assert (m_state);
}

MonitorView::Mode0::Mode0(MonitorView *p_state, const MonitorView::Configurator &p_cfgr)
    : Mode(p_state)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::Mode0(" << &p_state << ", " << p_cfgr << ")");
    const int character_w = scale2character_width(p_cfgr.scale());
    const int character_h = scale2character_height(p_cfgr.scale());
    SDL_Surface *fontfile(SDL_LoadBMP(p_cfgr.fontfilename().c_str()));
    for ( SurfaceArray mc6847(fontfile, 32, 8);
          mc6847 && mc6847.key() < 256;
          ++mc6847) {
        SDL_Surface *tmp(mc6847.get(character_w, character_h));
        m_glyph[mc6847.key()] = SDL_DisplayFormat(tmp);
        SDL_FreeSurface(tmp);
    }
    SDL_FreeSurface(fontfile);
}

MonitorView::Mode0::~Mode0()
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::~Mode0()");
    for (SDL_Surface *cg : m_glyph)
        if (cg)
            SDL_FreeSurface(cg);
}

void MonitorView::Mode0::set_byte_update(word p_addr, byte p_byte)
{
    LOG4CXX_INFO(cpptrace_log()
                 , "MonitorView::Mode0::set_byte_update("
                 << Hex(p_addr)
                 << ", "
                 << Hex(p_byte)
                 << ")");
    SurfaceArray(m_state->m_screen, 32, 16, p_addr).set(m_glyph[p_byte]);
    m_state->m_rendered[p_addr] = p_byte;
}

void MonitorView::Mode0::render()
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::Mode0::render()");
    assert (m_state);
    SurfaceArray si(m_state->m_screen, 32, 16);
    for (int addr = 0; addr < 512; addr++, ++si)
    {
        const byte ch = m_state->m_memory->get_byte(addr);
        SDL_Surface * const glyph(m_glyph[ch]);
        si.set(glyph);
        m_state->m_rendered[addr] = ch;
    }
}

MonitorView::MonitorView(TerminalInterface *p_terminal_interface,
                         Memory *p_memory,
                         const Configurator &p_cfgr)
    : m_terminal_interface(p_terminal_interface)
    , m_memory(p_memory)
    , m_rendered(m_memory->size())
    , m_mode(0)
    , m_mode0(this, p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "MonitorView::MonitorView("
                 << "<controller name=\"" << p_terminal_interface->id() << "\"/>"
                 << "<memory name=\"" << p_memory->id() << "\"/>"
                 << p_cfgr << ")");
    assert (p_terminal_interface);
    assert (p_memory);
    assert (p_cfgr.scale() >= 1.0);
    m_screen = SDL_SetVideoMode( scale2character_width(p_cfgr.scale()) * 32,
                                 scale2character_height(p_cfgr.scale()) * 16,
                                 0,
                                 SDL_SWSURFACE | SDL_ANYFORMAT );
    assert (m_screen);
    SDL_WM_SetCaption(p_cfgr.window_title().c_str(), p_cfgr.icon_title().c_str());
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
    m_terminal_interface->attach(*this);
    m_memory->attach(*this);
}

MonitorView::~MonitorView()
{
    LOG4CXX_INFO(cpptrace_log(), "~MonitorView::MonitorView()");
    m_memory->detach(*this);
    m_terminal_interface->detach(*this);
    m_mode = 0;
    SDL_FreeSurface(m_screen);
}

void MonitorView::set_byte_update(Memory *p_memory, word p_addr, byte p_byte, Memory::AccessType p_at)
{
    if (m_mode)
    {
        if (p_byte != m_rendered[p_addr])
            m_mode->set_byte_update(p_addr, p_byte);
    }
}

void MonitorView::vdg_mode_update(TerminalInterface *p_terminal, TerminalInterface::VDGMode p_mode)
{
    LOG4CXX_INFO(cpptrace_log(), "MonitorView::vdg_mode_update(" << p_mode << ")");
    assert (m_screen);
    switch (p_mode)
    {
    case TerminalInterface::VDG_MODE0:
        m_mode = &m_mode0;
        break;
    default:
        LOG4CXX_ERROR(cpptrace_log(), "Can't render graphics mode " << p_mode);
        m_mode = 0;
        // assert (false);  // TODO: render graphics modes
    }
    if (m_mode)
        m_mode->render();
}

void MonitorView::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<scale>"        << scale() << "</scale>"
        << "<fontfilename>" << fontfilename() << "</fontfilename>"
        << "<windowtitle>"  << window_title() << "</windowtitle>"
        << "<icontitle>"    << icon_title()   << "</icontitle>";
}

void MonitorView::serialize(std::ostream &p_s) const
{
    p_s << "MonitorView("
        << ")";
}

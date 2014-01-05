/*
 * screen_graphics_view.cpp
 *
 *  Created on: 9 May 2012
 *      Author: steve
 */

#include <cassert>
#include <cmath>
#include <string>
//#include <algorithm>
//#include <iterator>

#include <ostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "screen_graphics_view.hpp"

#define FONT_FNAME "mc6847.bmp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".screen_graphics_view.cpp"));
    return result;
}

class SurfaceArray : public Named {
private:
	SDL_Surface *m_array;
	SDL_Rect     m_pos;
	int          m_x_range;
	int          m_key;
public:
	SDL_Surface *data() const { return m_array; }
	int         key() const { return m_key; }
	void        update() { SDL_UpdateRect(m_array, m_pos.x, m_pos.y, m_pos.w, m_pos.h); }

	SurfaceArray( SDL_Surface *p_surface, std::string p_name = "", int x_range = 1, int y_range = 1, int p_key = 0 )
	: Named(p_name)
	, m_array(p_surface)
	, m_x_range(x_range)
	, m_key(p_key)
	{
	    LOG4CXX_INFO(cpptrace_log(), "SurfaceArray::SurfaceArray(" << p_surface << ", [" << p_name << "], " << x_range << ", " << y_range << ", " << p_key << ")");
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
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].operator++()");
		m_key++;
		m_pos.x += m_pos.w;
		if (m_pos.x >= m_array->w) {
			m_pos.x = 0;
			m_pos.y += m_pos.h;
		}
		return *this;
	}
	SurfaceArray  &operator--()
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].operator--()");
		if (m_pos.x == 0)
		{
			m_pos.x = m_array->w;
			m_pos.y -= m_array->h;
		}
		m_pos.x -= m_pos.w;
		m_key--;
		return *this;
	}
	operator bool() const
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].operator bool()");
		return (0 <= m_pos.x && m_pos.x + m_pos.w <= m_array->w) &&
			   (0 <= m_pos.y && m_pos.y + m_pos.h <= m_array->h);
	}
	bool fill(Uint32 color)
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].fill(" << color << ")");
		const int rv = SDL_FillRect(m_array, &m_pos, color);
		return rv == 0;
	}
	SDL_Rect at(int p_key) const
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].at(" << p_key << ")");
		SDL_Rect result(m_pos);
		const int y(p_key / m_x_range);
		result.x = (p_key - y * m_x_range) * result.w;
		result.y = y * result.h;
		return result;
	}
	void set(const SurfaceArray &other, int p_key = -1)
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set([" << other.name() << "], " << p_key << ")");
		SDL_Rect tmp(other.m_pos); // Nasty Kludge because of SDL typing
		if (p_key >= 0)
			m_pos = at(p_key);
		const int rv =SDL_BlitSurface(other.m_array, &tmp, m_array, &m_pos);
		assert (!rv);
		update();
	}
	SDL_Surface *get(int p_key = -1) const
	{
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get(" << p_key << ")");
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
	    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get(" << new_w << ", " << new_h << ")");
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

void ScreenGraphicsView::render_mode0()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].render_mode0()");
    SurfaceArray si(m_screen, "render_mode0", 32, 16);
    const std::vector<byte> &ram(m_atom.vdg_storage());
    for (int addr = 0; addr < 512; addr++, ++si) {
    	const byte ch = ram[addr];
    	if (ch != m_rendered[addr]) {
    		si.set(m_glyph[ch]);
    		m_rendered[addr] = ch;
    	}
    }
}

ScreenGraphicsView::ScreenGraphicsView(Atom &p_atom, const Configurator &p_cfg)
  : Named(p_cfg)
  , m_atom(p_atom)
  , m_rendered(std::min(int(p_atom.vdg_storage().size()), 0x1800))
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsView::ScreenGraphicsView([" << p_atom.name() << "], " << p_cfg << ")");
    assert (p_cfg.scale() >= 1.0);
    m_character_w = std::floor(8 * p_cfg.scale());
    m_character_h = std::floor(12 * p_cfg.scale());
    m_screen = SDL_SetVideoMode( m_character_w * 32,
                                 m_character_h * 16,
                                 0,
                                 SDL_SWSURFACE | SDL_ANYFORMAT );
    assert (m_screen);
    SDL_WM_SetCaption(p_cfg.window_title().data(), p_cfg.icon_title().data());
    SDL_Surface *fontfile(SDL_LoadBMP(p_cfg.fontfilename().data()));
    for ( SurfaceArray mc6847(fontfile, "mc6847", 32, 8);
    	  mc6847 && mc6847.key() < 256;
    	  ++mc6847) {
        SDL_Surface *tmp(mc6847.get(m_character_w, m_character_h));
        m_glyph[mc6847.key()] = SDL_DisplayFormat(tmp);
        SDL_FreeSurface(tmp);
    }
    SDL_FreeSurface(fontfile);
    std::fill(m_rendered.begin(), m_rendered.end(), -1); // (un)Initialise cache
}

ScreenGraphicsView::~ScreenGraphicsView()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].~ScreenGraphicsView()");
    for (SDL_Surface * cg : m_glyph)
        if (cg)
            SDL_FreeSurface(cg);
    SDL_FreeSurface(m_screen);
}

void ScreenGraphicsView::update()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].update()");
    switch(m_atom.vdg_mode()) {
    case VDG_MODE0 :                                                // Text Mode
    	render_mode0();
    	break;
	default:                                // TODO: render colour graphics (#6)
		assert (false);
    }
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsView::Configurator &p_cfg)
{
	p_s << static_cast<const Named::Configurator &>(p_cfg)
	    << ", scale=" << p_cfg.scale()
	    << ", fontfilename=\"" << p_cfg.fontfilename() << "\""
	    << ", windowtitle=\""  << p_cfg.window_title() << "\""
	    << ", icontitle=\""    << p_cfg.icon_title()   << "\"";
	return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsView &p_sgv)
{
    p_s << static_cast<const Named &>(p_sgv)
        << "atom:[" << p_sgv.m_atom.name() << "]"
        << "screen:" << p_sgv.m_screen
        << "character_w:" << p_sgv.m_character_w
        << "character_h:" << p_sgv.m_character_h;
    return p_s;
}


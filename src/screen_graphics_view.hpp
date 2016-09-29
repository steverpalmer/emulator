/*
 * screen_graphics_view.hpp
 *
 *  Created on: 9 May 2012
 *      Author: steve
 */

#ifndef SCREEN_GRAPHICS_VIEW_HPP_
#define SCREEN_GRAPHICS_VIEW_HPP_

#include <ostream>
#include <SDL.h>
#include <vector>
#include <array>

#include "common.hpp"
#include "terminal_interface.hpp"
#include "device.hpp"

class ScreenGraphicsView : public Part {
    // Types
public:
    class Configurator : public Part::Configurator
    {
    public:
        virtual float scale() const = 0;
        virtual const std::string &fontfilename() const = 0;
        virtual const std::string &window_title() const = 0;
        virtual const std::string &icon_title() const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
private:
    const TerminalInterface       &m_terminal;
    Device                        &m_memory;
    SDL_Surface                   *m_screen;
    std::vector<int>               m_rendered;
    // Mode 0 Stuff
    int                            m_character_w;
    int                            m_character_h;
    std::vector<SDL_Surface *>     m_glyph;
private:
    ScreenGraphicsView(const ScreenGraphicsView &);
    ScreenGraphicsView &operator=(const ScreenGraphicsView&);
    SDL_Surface *scale_and_convert_surface(SDL_Surface *src);
    void render_mode0();
public:
    ScreenGraphicsView(const TerminalInterface &, Device &, const Configurator &);
    virtual ~ScreenGraphicsView();
    void update();

    friend std::ostream &::operator<<(std::ostream&, const ScreenGraphicsView &);
};


#endif /* SCREEN_GRAPHICS_VIEW_HPP_ */

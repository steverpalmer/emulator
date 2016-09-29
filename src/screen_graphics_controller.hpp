/*
 * screen_graphics_controller.hpp
 *
 *  Created on: 9 May 2012
 *      Author: steve
 */

#ifndef SCREEN_GRAPHICS_CONTROLLER_HPP_
#define SCREEN_GRAPHICS_CONTROLLER_HPP_

#include <SDL.h>

#include <ostream>

#include "common.hpp"
#include "terminal_interface.hpp"
#include "screen_graphics_view.hpp"

class ScreenGraphicsController : public Part {
    // Types
public:
    class Configurator : public Named::Configurator
    {
    public:
        virtual const ScreenGraphicsView::Configurator &view() const = 0;
        virtual float RefreshRate_Hz() const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
private:
    TerminalInterface  &m_terminal;
    ScreenGraphicsView *m_view;
    SDL_TimerID        m_timer;
private:
    ScreenGraphicsController(const ScreenGraphicsController &);
    ScreenGraphicsController &operator=(const ScreenGraphicsController&);
public:
    ScreenGraphicsController(TerminalInterface &, const Configurator &);
    virtual ~ScreenGraphicsController();
    void update();

    friend std::ostream &::operator<<(std::ostream&, const ScreenGraphicsController &);
};

#endif /* SCREEN_GRAPHICS_CONTROLLER_HPP_ */

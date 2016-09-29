/*
 * screen_graphics_controller.cpp
 *
 *  Created on: 9 May 2012
 *      Author: steve
 */

#include <cassert>
#include <cmath>

#include <iostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "screen_graphics_controller.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".screen_graphics_controller.cpp"));
    return result;
}

static Uint32 callback(Uint32 interval, void *param)
{
    static SDL_UserEvent timer_event = {SDL_USEREVENT, 0, 0, 0 };
    const int rv = SDL_PushEvent((SDL_Event *)&timer_event);
    assert (!rv);
    return interval;
}

ScreenGraphicsController::ScreenGraphicsController(TerminalInterface &p_terminal, const Configurator &p_cfg)
    : Part(p_cfg)
    , m_terminal(p_terminal)
    , m_view(new ScreenGraphicsView(p_terminal, NULL, p_cfg.view()))
    , m_timer(SDL_AddTimer(std::ceil(1000.0 / p_cfg.RefreshRate_Hz()), callback, 0))
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsController::ScreenGraphicsController(" << p_cfg << ")");
    assert (m_view);
    assert (m_timer);
}

ScreenGraphicsController::~ScreenGraphicsController()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].~ScreenGraphicsController()");
    const SDL_bool rv = SDL_RemoveTimer(m_timer);
    assert(rv);
    delete m_view;
}

void ScreenGraphicsController::update()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].update()");
    m_terminal.set_vdg_refresh(true);
    m_view->update();
    m_terminal.set_vdg_refresh(false);
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsController::Configurator &p_cfg)
{
    p_s << static_cast<const Part::Configurator &>(p_cfg)
        << ", view=(" << p_cfg.view() << ")"
        << ", RefreshRate_Hz=" << p_cfg.RefreshRate_Hz();
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsController &p_sgc)
{
    p_s << "<ScreenGraphicsController>";
    p_s << static_cast<const Part &>(p_sgc);
    p_s << "<view>[" << p_sgc.m_view->id() << "]</view>";
    p_s << "</ScreenGraphicsController>";
    return p_s;
}

/*
 * screen_graphics_controller.cpp
 *
 *  Created on: 9 May 2012
 *      Author: steve
 */

#include <cassert>

#include <iostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "screen_graphics_controller.hpp"

const int ScrRefreshRate_ms = 100;

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

ScreenGraphicsController::ScreenGraphicsController(Atom &p_atom, const Configurator &p_cfg)
: Named(p_cfg)
, m_atom(p_atom)
, m_view(new ScreenGraphicsView(p_atom, p_cfg.view()))
, m_timer(SDL_AddTimer(p_cfg.RefreshRate_ms(), callback, 0))
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsController::ScreenGraphicsController([" << p_atom.name() << "], " << p_cfg << ")");
    assert (m_view);
    assert (m_timer);
}

ScreenGraphicsController::~ScreenGraphicsController()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].~ScreenGraphicsController()");
    const SDL_bool rv = SDL_RemoveTimer(m_timer);
    assert(rv);
    delete m_view;
}

void ScreenGraphicsController::update()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].update()");
    m_atom.set_vdg_refresh(true);
    m_view->update();
    m_atom.set_vdg_refresh(false);
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsController::Configurator &p_cfg)
{
	p_s << static_cast<const Named::Configurator &>(p_cfg)
	    << ", view=(" << p_cfg.view() << ")"
	    << ", RefreshRate_ms=" << p_cfg.RefreshRate_ms();
	return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsController &p_sgc)
{
    p_s << "<ScreenGraphicsController>";
    p_s << static_cast<const Named &>(p_sgc);
    p_s << "<atom>[" << p_sgc.m_atom.name() << "]</atom>";
    p_s << "<view>[" << p_sgc.m_view->name() << "]</view>";
    p_s << "</ScreenGraphicsController>";
    return p_s;
}

/******************************************************************************
 * $Author: steve $
 * $Date: 2004/04/11 09:59:52 $
 * $Id: main.c,v 1.2 2004/04/11 09:59:52 steve Exp $
 ******************************************************************************/

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include <SDL.h>

#include <log4cxx/logger.h>
#include "log4cxx/propertyconfigurator.h"

#include "config_xml.hpp"
#include "atom.hpp"
#include "keyboard_controller.hpp"
#include "screen_graphics_controller.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".main.cpp"));
    return result;
}

class Main {
    Configurator              m_cfg;
    Atom                     *m_atom;
    KeyboardController       *m_kc;
    ScreenGraphicsController *m_sgc;
    bool                      m_more;
private:
    Main();
    Main(const Main &);
    void operator=(const Main&);
public:
    Main(int agrc, char *argv[]);
    ~Main();
};


Main::Main(int argc, char *argv[])
    : m_cfg(argc, argv)
{
    LOG4CXX_INFO(cpptrace_log(), "Position 2 => " << static_cast<const Atom::Configurator &>(m_cfg.atom()));
    LOG4CXX_INFO(cpptrace_log(), "Main::Main(" << argc << ", " << argv << ")");
    LOG4CXX_INFO(cpptrace_log(), "SDL_Init");
    const int rv = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
    assert (!rv);
    LOG4CXX_INFO(cpptrace_log(), "Atom");
    m_atom = new Atom(m_cfg.atom());
    assert (m_atom);
    LOG4CXX_INFO(cpptrace_log(), "Keyboard");
    m_kc = new KeyboardController(*m_atom, m_cfg.keyboard());
    assert (m_kc);
    LOG4CXX_INFO(cpptrace_log(), "Screen");
    m_sgc = new ScreenGraphicsController(*m_atom, m_cfg.screen());
    assert (m_sgc);

    LOG4CXX_INFO(cpptrace_log(), "Atom is about to start ...");
    m_atom->resume();
    SDL_Event event;
    m_more = true;
    while( m_more && SDL_WaitEvent( &event ) )
        switch( event.type ){
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            m_kc->update(&event.key);
            break;
        case SDL_USEREVENT:
            m_sgc->update();
            break;
        case SDL_QUIT:
            m_more = false;
            break;
        default:
            break;
        }
    LOG4CXX_INFO(cpptrace_log(), "Atom is about to stop ...");
}

Main::~Main()
{
    LOG4CXX_INFO(cpptrace_log(), "Main::~Main()");
    SDL_Quit();
}

/******************************************************************************/
int main (int argc, char *argv[])
/******************************************************************************/
{
    log4cxx::PropertyConfigurator::configure("log4cxx.properties");
    LOG4CXX_INFO(cpptrace_log(), "main(" << argc << ", " << argv << ")");
    Main(argc, argv);
    return EXIT_SUCCESS;
}

// main.cpp

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include <iostream>

#include <SDL.h>

#include <log4cxx/logger.h>
#include "log4cxx/propertyconfigurator.h"

#include "common.hpp"
#include "config.hpp"
#include "config_xml.hpp"
#include "part.hpp"
#include "terminal.hpp"
#include "device.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".main.cpp"));
    return result;
}

class Main
    : public NonCopyable
{
private:
    Main();
public:
    Main(int argc, char *argv[])
        {
            LOG4CXX_INFO(cpptrace_log(), "Main::Main(" << argc << ", " << argv << ")");

            LOG4CXX_INFO(cpptrace_log(), "SDL_Init");
            const int rv = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
            assert (!rv);

            const Configurator *cfg = new Xml::Configurator(argc, argv);  // FIXME: remove Xml::
            assert (cfg);

            PartsBin::instance().build(*cfg);
            delete cfg;

            Terminal *terminal = dynamic_cast<Terminal *>(PartsBin::instance()["terminal"]);
            assert (terminal);

            Device *atom = dynamic_cast<Device *>(PartsBin::instance()["atom"]);
            assert (atom);

            LOG4CXX_INFO(cpptrace_log(), "Atom is about to start ...");
            atom->resume();
            SDL_Event event;
            bool more = true;
            while( more && SDL_WaitEvent( &event ) )
                switch( event.type ){
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    terminal->update(&event);
                    break;
                case SDL_QUIT:
                    more = false;
                    break;
                default:
                    break;
                }
            LOG4CXX_INFO(cpptrace_log(), "Atom is about to stop ...");
            atom->pause();

        }

    virtual ~Main()
        {
            LOG4CXX_INFO(cpptrace_log(), "Main::~Main()");
            PartsBin::instance().clear();
            SDL_Quit();
            LOG4CXX_INFO(cpptrace_log(), "Main done.");
        }
};


/******************************************************************************/
int main (int argc, char *argv[])
/******************************************************************************/
{
    log4cxx::PropertyConfigurator::configure("log4cxx.properties");
    LOG4CXX_INFO(cpptrace_log(), "main(" << argc << ", " << argv << ")");
    Main(argc, argv);
    return EXIT_SUCCESS;
}

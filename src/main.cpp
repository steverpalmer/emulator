// main.cpp

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include <SDL.h>

#include <log4cxx/logger.h>
#include "log4cxx/propertyconfigurator.h"

#inlcude "common.hpp"
#include "config.hpp"
#include "config_xml.hpp"
#include "part.hpp"
#include "terminal.hpp"
#include "cpu.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".main.cpp"));
    return result;
}

class Main
{
private:
    Main();
    Main(const Main &);
    Main &operator=(const Main&);
public:
    Main(int agrc, char *argv[])
        {
            LOG4CXX_INFO(cpptrace_log(), "Main::Main(" << argc << ", " << argv << ")");

            LOG4CXX_INFO(cpptrace_log(), "SDL_Init");
            const int rv = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
            assert (!rv);

            const Configurator *cfg = Xml::Configurator(argc, argv);
            LOG4CXX_INFO(cpptrace_log(), cfg);

            PartsBin::instance().build(cfg);

            Terminal *terminal = dynamic_cast<Terminal *>(PartsBin::instance()["/atom/terminal"]);
            assert (terminal);
            
            Cpu *atom = dynamic_cast<Cpu *>(PartsBin::instance()["/atom/mcs6502"]);
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
            SDL_Quit();
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

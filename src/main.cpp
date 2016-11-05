// main.cpp
// Copyright 2016 Steve Palmer

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include <thread>

#include <SDL.h>

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
    : protected NonCopyable
{
private:
    Main();
public:
    Main(int argc, char *argv[])
        {
            LOG4CXX_INFO(cpptrace_log(), "Main::Main(" << argc << ", " << argv << ")");

            LOG4CXX_INFO(cpptrace_log(), "SDL_Init");
            const int rv = SDL_Init( SDL_INIT_VIDEO );
            assert (!rv);

            const Configurator *cfg = new Xml::Configurator(argc, argv);  // FIXME: remove Xml::
            assert (cfg);

            PartsBin::instance().build(*cfg);
            delete cfg;

            Terminal *terminal = dynamic_cast<Terminal *>(PartsBin::instance()["terminal"]);
            assert (terminal);

            Device *computer = dynamic_cast<Device *>(PartsBin::instance()["computer"]);
            assert (computer);

            LOG4CXX_INFO(cpptrace_log(), "Computer is about to start ...");
            computer->reset();
            computer->resume();
            SDL_Event event;
            bool more = false;
            while( more && SDL_WaitEvent(&event) )
                switch( event.type ){
                case SDL_QUIT:
                    more = false;
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                case SDL_WINDOWEVENT:
                    (void) terminal->handle_event(event);
                    break;
                default:
                    LOG4CXX_DEBUG(cpptrace_log(), "Unhandled event:" << event.type);
                    break;
                }
            LOG4CXX_INFO(cpptrace_log(), "Computer is about to stop ...");
            computer->pause();
            while (not computer->is_paused())
                std::this_thread::yield();
        }

    virtual ~Main()
        {
            LOG4CXX_INFO(cpptrace_log(), "Main::~Main()");
            PartsBin::instance().clear();
            SDL_Quit();
            LOG4CXX_INFO(cpptrace_log(), "Main done.");
        }
};


void configure_logging(const char *command)
{
    // copy command since dirname may modify it!
    char * const command_copy = (char *)malloc(strlen(command)+1);
    (void) strcpy(command_copy, command);
    const char * const command_dir = dirname(command_copy);
    const int command_dir_length = strlen(command_dir);
    // build the path to the properties file
    const char * const filename = "log4cxx.properties";
    const int pathname_length(command_dir_length + 1 + strlen(filename) + 1);
    char * const pathname = (char *)malloc(pathname_length);
    (void) strcpy(pathname, command_dir);
    pathname[command_dir_length] = '/';
    (void) strcpy(&pathname[command_dir_length+1], filename);
    log4cxx::PropertyConfigurator::configure(pathname);
    // Tidy up
    free(pathname);
    free(command_copy);
}

int main (int argc, char *argv[])
{
    configure_logging(*argv);
    LOG4CXX_INFO(cpptrace_log(), "main(" << argc << ", " << argv << ")");
    Main(argc, argv);
    return EXIT_SUCCESS;
}

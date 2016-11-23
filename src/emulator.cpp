// emulator.cpp
// Copyright 2016 Steve Palmer

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include <thread>
#include <iostream>
#include <atomic>

#include <SDL.h>

#include "log4cxx/propertyconfigurator.h"

#include "common.hpp"
#include "config.hpp"
#include "config_xml.hpp"
#include "part.hpp"
#include "device.hpp"
#include "atom_streambuf.hpp"
#include "ppia.hpp"
#include "keyboard_adaptor.hpp"
#include "dispatcher.hpp"
#include "pump.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".emulator.cpp"));
    return result;
}

class Emulator
    : protected NonCopyable
{
private:
    Emulator();
    enum {Continue, QuitRequest, EventWaitError} loop_state;

    class QuitHandler
        : public Dispatcher::StateHandler<Emulator>
    {
    public:
        explicit QuitHandler(Emulator &p_emulator)
            : Dispatcher::StateHandler<Emulator>(SDL_QUIT, p_emulator)
            {
                LOG4CXX_INFO(cpptrace_log(), "QuitHandler::QuitHandler(...)");
            }
    private:
        void handle(const SDL_Event &)
            {
                LOG4CXX_INFO(cpptrace_log(), "QuitHandler::handle(...)");
                state.loop_state = QuitRequest;
            }
    } quit_handler;

public:
    Emulator(int argc, char *argv[])
        : quit_handler(*this)
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::Emulator(" << argc << ", " << argv << ")");

            LOG4CXX_INFO(SDL::log(), "SDL_Init( SDL_INIT_VIDEO )");
            const int rv = SDL_Init( SDL_INIT_VIDEO );
            assert (!rv);

            const Configurator *cfg = new Xml::Configurator(argc, argv);  // FIXME: remove Xml::
            assert (cfg);
            LOG4CXX_DEBUG(cpptrace_log(), *cfg);

            PartsBin::instance().build(*cfg);
            delete cfg;
            LOG4CXX_DEBUG(cpptrace_log(), PartsBin::instance());

            auto stream = dynamic_cast<Atom::IOStream *>(PartsBin::instance()["stream"]);
            Pump::Stdin stdin(stream);
            Pump::Stdout stdout(stream);

            Device *root = dynamic_cast<Device *>(PartsBin::instance()["root"]);
            assert (root);

            Ppia *ppia = dynamic_cast<Ppia *>(PartsBin::instance()["ppia"]);
            assert (ppia);
            KeyboardAdaptor *keyboard = new KeyboardAdaptor(ppia, root);

#if 0
            MCS6502 *mcs6502 = dynamic_cast<MCS6502 *>(PartsBin::instance()["mcs6502"]);
            assert (mcs6502);
            LOG4CXX_WARN(cpptrace_log(), "1 => " << PartsBin::instance());
            MCS6502::SetLogLevel(0xCEED, *mcs6502, log4cxx::Level::getInfo());
            LOG4CXX_WARN(cpptrace_log(), "2 => " << PartsBin::instance());
#endif

            LOG4CXX_INFO(cpptrace_log(), "Emulation is about to start ...");
            root->reset();
            root->resume();
            SDL_Event event;
            loop_state = Continue;
            while (loop_state == Continue)
            {
                LOG4CXX_INFO(SDL::log(), "SDL_WaitEvent(&event)");
                if (!SDL_WaitEvent(&event))
                    loop_state = EventWaitError;
                else
                    Dispatcher::instance().dispatch(event);
            }
            LOG4CXX_INFO(cpptrace_log(), "Emulation is about to stop ...");
            assert (loop_state == QuitRequest);

            stdin.stop();
            if (stream)
                stream->unblock();
            stdout.stop();
            root->pause();
            while (not root->is_paused())
                std::this_thread::yield();
            
            delete keyboard;
        }

    virtual ~Emulator()
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::~Emulator()");
            PartsBin::instance().clear();
            LOG4CXX_INFO(SDL::log(), "SDL_Quit");
            SDL_Quit();
            LOG4CXX_INFO(cpptrace_log(), "Emulator done.");
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
    Emulator(argc, argv);
    return EXIT_SUCCESS;
}

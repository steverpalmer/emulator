// emulator.cpp
// Copyright 2016 Steve Palmer

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <signal.h>

#include <thread>
#include <iostream>
#include <atomic>
#include <chrono>

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

static class Emulator *emulator(0);

class Emulator
    : protected NonCopyable
{
private:
    Emulator();
    Atom::IOStream *stream;
    Pump::Stdin *stdin;
    Pump::Stdout *stdout;
    Device *root;
    KeyboardAdaptor *keyboard;
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

    class ResetHandler
        : public Dispatcher::StateHandler<Emulator>
    {
    public:
        explicit ResetHandler(Emulator &p_emulator)
            : Dispatcher::StateHandler<Emulator>(p_emulator)
            {
                LOG4CXX_INFO(cpptrace_log(), "ResetHandler::ResetHandler(...)");
            }
    private:
        void handle(const SDL_Event &)
            {
                LOG4CXX_INFO(cpptrace_log(), "ResetHandler::handle(...)");
                state.reset();
            }
    } reset_handler;

public:
    Emulator(int argc, char *argv[])
        : stream(0)
        , stdin(0)
        , stdout(0)
        , root(0)
        , quit_handler(*this)
        , reset_handler(*this)
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::Emulator(" << argc << ", " << argv << ")");

            LOG4CXX_INFO(SDL::log(), "SDL_Init( SDL_INIT_VIDEO )");
            const int rv = SDL_Init( SDL_INIT_VIDEO );
            assert (!rv);

            // Remove the most prolific events that are of no interest
            (void)SDL_EventState(SDL_TEXTEDITING, SDL_IGNORE);
            (void)SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);
            (void)SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
            (void)SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
            (void)SDL_EventState(SDL_MOUSEWHEEL, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYDEVICEADDED, SDL_IGNORE);
            (void)SDL_EventState(SDL_JOYDEVICEREMOVED, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_IGNORE);
            (void)SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_IGNORE);
            (void)SDL_EventState(SDL_FINGERDOWN, SDL_IGNORE);
            (void)SDL_EventState(SDL_FINGERUP, SDL_IGNORE);
            (void)SDL_EventState(SDL_FINGERMOTION, SDL_IGNORE);
            (void)SDL_EventState(SDL_DOLLARGESTURE, SDL_IGNORE);
            (void)SDL_EventState(SDL_DOLLARRECORD, SDL_IGNORE);
            (void)SDL_EventState(SDL_MULTIGESTURE, SDL_IGNORE);

            const Configurator *cfg = new Xml::Configurator(argc, argv);  // FIXME: remove Xml::
            assert (cfg);
            LOG4CXX_DEBUG(cpptrace_log(), *cfg);

            PartsBin::instance().build(*cfg);
            delete cfg;
            LOG4CXX_DEBUG(cpptrace_log(), PartsBin::instance());

            stream = dynamic_cast<Atom::IOStream *>(PartsBin::instance()["stream"]);
            stdin = new Pump::Stdin(stream, quit_handler);
            stdout = new Pump::Stdout(stream);

            root = dynamic_cast<Device *>(PartsBin::instance()["root"]);
            assert (root);

            Ppia *ppia = dynamic_cast<Ppia *>(PartsBin::instance()["ppia"]);
            assert (ppia);
            keyboard = new KeyboardAdaptor(ppia, reset_handler);

#if 0
            MCS6502 *mcs6502 = dynamic_cast<MCS6502 *>(PartsBin::instance()["mcs6502"]);
            assert (mcs6502);
            LOG4CXX_WARN(cpptrace_log(), "1 => " << PartsBin::instance());
            MCS6502::SetLogLevel(0xCEED, *mcs6502, log4cxx::Level::getInfo());
            LOG4CXX_WARN(cpptrace_log(), "2 => " << PartsBin::instance());
#endif
        }

    void run()
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::run()");
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
            assert (loop_state == QuitRequest);

            LOG4CXX_INFO(cpptrace_log(), "Emulator::run() finishing ...");
            if (stdin)
                stdin->stop();
            if (stream)
                stream->unblock();
            if (stdout)
                stdout->stop();
            root->pause();
            while (not root->is_paused())
                std::this_thread::yield();
        }
    
    void reset()
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::reset()");
            if (root)
            {
                if (root->is_paused())
                {
                    LOG4CXX_INFO(cpptrace_log(), "Emulator::reset() when paused");
                    root->reset();
                }
                else
                {
                    LOG4CXX_INFO(cpptrace_log(), "Emulator::reset() pausing ...");
                    root->pause();
                    if (!root->is_paused())
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    LOG4CXX_INFO(cpptrace_log(), "Emulator::reset() reseting ...");
                    root->reset();
                    LOG4CXX_INFO(cpptrace_log(), "Emulator::reset() resuming ...");
                    root->resume();
                }
            }
            LOG4CXX_INFO(cpptrace_log(), "Emulator::reset() done");
        }

    void pause()
        {
            if (root)
                root->pause();
        }

    void resume()
        {
            if (root)
                root->resume();
        }

    bool is_paused() const
        {
            return !root || root->is_paused();
        }
    
    virtual ~Emulator()
        {
            LOG4CXX_INFO(cpptrace_log(), "Emulator::~Emulator()");
            delete stdout;
            delete stdin;
            delete keyboard;
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

void SIGUSR1_handler(int)
{
    LOG4CXX_INFO(cpptrace_log(), "SIGUSR1_handler(...)");
    if (emulator)
        emulator->reset();
}

int main (int argc, char *argv[])
{
    configure_logging(*argv);
    LOG4CXX_INFO(cpptrace_log(), "main(" << argc << ", " << argv << ")");
    emulator = new Emulator(argc, argv);
    assert (emulator);
    struct sigaction reset_action;
    reset_action.sa_handler = SIGUSR1_handler;
    sigemptyset (&reset_action.sa_mask);
    reset_action.sa_flags = 0;
    const int rc = sigaction(SIGUSR1, &reset_action, NULL);
    assert (rc==0);
    emulator->run();
    delete emulator;
    return EXIT_SUCCESS;
}

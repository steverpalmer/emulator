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

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".emulator.cpp"));
    return result;
}

class Pipe
    : public Part
{
private:
    std::istream     &m_source;
    std::ostream     &m_sink;
    std::atomic_bool m_more;
    std::thread      m_thread;
private:
    void thread_function()
        {
            LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::thread_function()");
            for (m_more=true; m_more; )
            {
                LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::thread_function reading");
                const int ch(m_source.get());
                LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::thread_function read " << ch);
                if (!m_more || ch == EOF)
                    break;
                LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::thread_function writing " << ch);
                m_sink.put(ch);
                LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::thread_function written");
            }
        }
public:
    Pipe(Part::id_type p_name, std::istream &p_source, std::ostream &p_sink)
        : Part(p_name)
        , m_source(p_source)
        , m_sink(p_sink)
        , m_thread(&Pipe::thread_function, this)
        {
            LOG4CXX_INFO(cpptrace_log(), "Pipe::Pipe(" << p_name << ", ...)");
        }
private:
    virtual void terminating()
        {
            LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::terminating()");
            m_more = false;
            LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::terminating() =>");
        }
    ~Pipe()
        {
            LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]Pipe::~Pipe()");
            m_thread.join();
        }
};

class Emulator
    : protected NonCopyable
{
private:
    Emulator();
    enum {Continue, QuitRequest, EventWaitError} loop_state;
    Device *root;

    class QuitHandler
        : public Dispatcher::StateHandler<Emulator>
    {
    public:
        explicit QuitHandler(Emulator &p_emulator)
            : Dispatcher::StateHandler<Emulator>(SDL_QUIT, p_emulator)
            {}
    private:
        void handle(const SDL_Event &)
            {
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

            auto *stream = dynamic_cast<Atom::IStream *>(PartsBin::instance()["stream"]);
            if (stream)
            {
                LOG4CXX_INFO(cpptrace_log(), "starting streaming");
                new Pipe("output stream", *stream, std::cout);
            }
            Device *root = dynamic_cast<Device *>(PartsBin::instance()["root"]);
            assert (root);

            Ppia *ppia = dynamic_cast<Ppia *>(PartsBin::instance()["ppia"]);
            assert (ppia);
            KeyboardAdaptor *keyboard = new KeyboardAdaptor(ppia, root);

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
            switch (loop_state)
            {
            case Continue:
            case EventWaitError:
                assert (false);
            case QuitRequest:
                root->pause();
                while (not root->is_paused())
                    std::this_thread::yield();

                delete keyboard;
            }
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

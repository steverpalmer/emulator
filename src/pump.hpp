// pump.hpp
// Copyright 2016 Steve Palmer

#ifndef PUMP_HPP_
#define PUMP_HPP_

#include <unistd.h>
#include <sys/select.h>
#include <iostream>
#include <thread>
#include <atomic>

#include "dispatcher.hpp"

namespace Pump
{

    template<class T>
    class Base
    {
    protected:
        T                *stream;
        struct timeval   timeout;
    private:
        std::atomic_bool more;
        std::thread      thread;
    protected:
        virtual void process_one_character()
            {
                std::this_thread::yield();
            };
        virtual void thread_function()
            {
                if (stream)
                    while (more)
                    {
                        // tv maybe altered, so must be reset each time
                        timeout.tv_sec = 0;
                        timeout.tv_usec = 100000;
                        process_one_character();
                    }
            }
        Base(T *p_stream)
            : stream(p_stream)
            , more(true)
            , thread(&Base::thread_function, this)
            {}
    private:
        Base();
        Base(const Base &) = delete;
        const Base &operator=(const Base &) = delete;
    public:
        void stop() { more = false; }
    protected:
        virtual ~Base() { thread.join(); }
    };

    class Stdin
        : public Base<std::ostream>
    {
        // Attributes
    private:
        const Dispatcher::Handler &quit_handler;
        // Methods
    private:
        void process_one_character()
            {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(STDIN_FILENO, &rfds);
                const int select_return = select(STDIN_FILENO+1, &rfds, NULL, NULL, &timeout);
                assert (select_return != -1);
                if (select_return)
                {
                    char ch;
                    // Should not block ...
                    const int read_return = read(STDIN_FILENO, &ch, 1);
                    if (read_return)
                        // May Block
                        stream->put(ch);
                    else
                        quit_handler.push();
                }
            }
        Stdin();
    public:
        Stdin(std::ostream *sink, const Dispatcher::Handler &p_quit_handler)
            : Base(sink)
            , quit_handler(p_quit_handler)
            {}
        virtual ~Stdin() = default;
    };

    class Stdout
        : public Base<std::istream>
    {
        // Methods
    private:
        void process_one_character()
            {
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(STDOUT_FILENO, &wfds);
                const int select_return = select(STDOUT_FILENO+1, NULL, &wfds, NULL, &timeout);
                assert (select_return != -1);
                if (select_return)
                {
                    // May Block
                    const int ch(stream->get());
                    if (std::istream::traits_type::not_eof(ch))
                    {
                        const char buf(ch);
                        // Should not block ...
                        const int write_return = write(STDOUT_FILENO, &buf, 1);
                        assert (write_return != -1);
                    }
                }
            }
        Stdout();
    public:
        Stdout(std::istream *source) : Base(source) {}
        virtual ~Stdout() = default;
    };

}

#endif

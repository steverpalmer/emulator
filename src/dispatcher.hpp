// dispatcher.hpp
// Copyright 2016 Steve Palmer

#ifndef DISPATCHER_HPP_
#define DISPATCHER_HPP_

#include <map>
#include <cassert>

#include <SDL.h>

#include "common.hpp"

class Dispatcher
    : public NonCopyable
{
public:
    class Handler
        : public NonCopyable
    {
    protected:
        const Uint32 event_type;
        explicit Handler(Uint32 p_event_type);
        Handler();
        void prepare(SDL_Event &p_event) const;
        void push(SDL_Event &) const;
    public:
        virtual ~Handler();
        virtual void handle(const SDL_Event &) = 0;
        void push() const;
    };

    template<class T>
    class StateHandler
        : public Handler
    {
    protected:
        T &state;
        explicit StateHandler(T &p_state)
            : Handler()
            , state(p_state)
            {}
        StateHandler(Uint32 p_event_type, T &p_state)
            : Handler(p_event_type)
            , state(p_state)
            {}
    public:
        virtual ~StateHandler() = default;
    };

private:
    std::map<Uint32, Handler *>m_map;
private:
    Dispatcher() = default;
public:
    virtual ~Dispatcher() = default;
    static Dispatcher &instance()
        {
            static Dispatcher *s_instance;
            if (!s_instance)
            {
                s_instance = new Dispatcher;
                assert (s_instance);
            }
            return *s_instance;
        }
    void attach(Uint32 p_event_type, Handler &);
    void detach(Uint32 p_event_type);
    void dispatch(SDL_Event &p_event);
};

#endif

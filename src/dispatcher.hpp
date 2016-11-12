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
    {
    protected:
        Uint32     &event_type;
        Handler(Uint32 p_event_type)
            : event_type(p_event_type)
            {
                Dispatcher::instance().map(event_type, *this);
            }
    public:
        virtual ~Handler()
            {
                Dispatcher::instance().demap(event_type);
            }
        virtual void handle(SDL_Event &) = 0;
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
    void map(Uint32 p_event_type, Handler &p_handler)
        {
            m_map[p_event_type] = &p_handler;
        }
    void demap(Uint32 p_event_type)
        {
            m_map[p_event_type] = 0;
        }
    void dispatch(SDL_Event &p_event)
        {
            try
            {
                Handler * handler(m_map.at(p_event.type));
                if (handler)
                    handler->handle(p_event);
            }
            catch (std::out_of_range e)
            {
            }
        }
};

#endif

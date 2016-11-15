// dispatcher.cpp
// Copyright 2016 Steve Palmer

#include "dispatcher.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".dispatcher.cpp"));
    return result;
}

Dispatcher::Handler::Handler(Uint32 p_event_type)
    : event_type(p_event_type)
{
    LOG4CXX_INFO(cpptrace_log(), "Dispatcher::Handler::Handler(" << p_event_type << ")");
    Dispatcher::instance().attach(event_type, *this);
}

Dispatcher::Handler::~Handler()
{
    LOG4CXX_INFO(cpptrace_log(), "Dispatcher::Handler::~Handler()");
    Dispatcher::instance().detach(event_type);
}

void Dispatcher::attach(Uint32 p_event_type, Handler &p_handler)
{
    LOG4CXX_INFO(cpptrace_log(), "Dispatcher::attach(" << p_event_type << ", ...)");
    m_map[p_event_type] = &p_handler;
}

void Dispatcher::detach(Uint32 p_event_type)
{
    LOG4CXX_INFO(cpptrace_log(), "Dispatcher::detach(" << p_event_type << ", ...)");
    m_map[p_event_type] = 0;
}

void Dispatcher::dispatch(SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "Dispatcher::dispatch(" << p_event.type << ")");
    try
    {
        Handler *handler(m_map.at(p_event.type));
        if (handler)
            handler->handle(p_event);
    }
    catch (std::out_of_range e)
    {
        LOG4CXX_INFO(cpptrace_log(), "Dispatcher::dispatch: Unknown event");
    }
}

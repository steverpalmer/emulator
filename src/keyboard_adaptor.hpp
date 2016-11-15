// keyboard_adaptor.hpp
// Copyright 2016 Steve Palmer

#ifndef KEYBOARD_ADAPTOR_HPP_
#define KEYBOARD_ADAPTOR_HPP_

#include <SDL.h>

#include "common.hpp"
#include "device.hpp"
#include "atom_keyboard_interface.hpp"
#include "dispatcher.hpp"

class KeyboardAdaptor
    : protected NonCopyable
{
    // Types
private:
    class Handler
        : public Dispatcher::Handler
    {
    protected:
        const KeyboardAdaptor &keyboard_adaptor;
        Handler(Uint32 p_event_type, KeyboardAdaptor &);
    public:
        virtual ~Handler() = default;
    };

    class DownHandler
        : public Handler
    {
    public:
        DownHandler(KeyboardAdaptor &);
        virtual ~DownHandler() = default;
        virtual void handle(const SDL_Event &);
    };

    class UpHandler
        : public Handler
    {
    public:
        UpHandler(KeyboardAdaptor &);
        virtual ~UpHandler() = default;
        virtual void handle(const SDL_Event &);
    };

public:
    AtomKeyboardInterface *m_atom_keyboard;
private:
    std::map<SDL_Scancode, AtomKeyboardInterface::Key> keys;
    DownHandler down_handler;
    UpHandler   up_handler;
public:
    KeyboardAdaptor(AtomKeyboardInterface *);
    virtual ~KeyboardAdaptor() = default;
};

#endif

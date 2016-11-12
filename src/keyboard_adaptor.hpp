// keyboard_handler.hpp
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
        KeyboardAdaptor &keyboard_adaptor;
        Handler(Uint32 p_event_type, KeyboardAdaptor &p_keyboard_adaptor)
            : Dispatcher::Handler(p_event_type)
            , keyboard_adaptor(p_keyboard_adaptor)
            {}
    public:
        virtual ~Handler() = default;
    };

    class KeyDownHandler
        : public Handler
    {
    public:
        KeyDownHandler(KeyboardAdaptor &p_keyboard_adaptor)
            : Handler(SDL_KEYDOWN, p_keyboard_adaptor)
            {}
        virtual ~KeyDownHandler() = default;
        virtual void handle(SDL_Event &p_event)
            {
                try
                {
                    if (keyboard_adaptor.m_atom_keyboard)
                        keyboard_adaptor.m_atom_keyboard->down(keyboard_adaptor.keys.at(p_event.key.keysym.scancode));
                }
                catch (std::out_of_range e)
                {
                }
            }
    };

    class KeyUpHandler
        : public Handler
    {
    public:
        KeyUpHandler(KeyboardAdaptor &p_keyboard_adaptor)
            : Handler(SDL_KEYUP, p_keyboard_adaptor)
            {}
        virtual ~KeyUpHandler() = default;
        virtual void handle(SDL_Event &p_event)
            {
                try
                {
                    if (keyboard_adaptor.m_atom_keyboard)
                        keyboard_adaptor.m_atom_keyboard->up(keyboard_adaptor.keys.at(p_event.key.keysym.scancode));
                }
                catch (std::out_of_range e)
                {
                }
            }
    };

public:
    AtomKeyboardInterface *m_atom_keyboard;
private:
    std::map<SDL_Scancode, AtomKeyboardInterface::Key> keys;
    KeyDownHandler keydown;
    KeyUpHandler   keyup;
public:
    KeyboardAdaptor(AtomKeyboardInterface *);
    virtual ~KeyboardAdaptor() = default;
};

#endif

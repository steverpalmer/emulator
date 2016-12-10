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
    class DownHandler
        : public Dispatcher::StateHandler<const KeyboardAdaptor>
    {
    public:
        DownHandler(const KeyboardAdaptor &);
        virtual ~DownHandler() = default;
        virtual void handle(const SDL_Event &) override;
    };

    class UpHandler
        : public Dispatcher::StateHandler<const KeyboardAdaptor>
    {
    public:
        UpHandler(const KeyboardAdaptor &);
        virtual ~UpHandler() = default;
        virtual void handle(const SDL_Event &) override;
    };

public:
    Atom::KeyboardInterface *m_atom_keyboard;
private:
    std::map<SDL_Scancode, Atom::KeyboardInterface::Key> keys;
    const DownHandler         down_handler;
    const UpHandler           up_handler;
    const Dispatcher::Handler &reset_handler;
    Atom::KeyboardInterface::Key atom_key(SDL_Scancode);
    KeyboardAdaptor();
public:
    KeyboardAdaptor(Atom::KeyboardInterface *, const Dispatcher::Handler &p_reset_handler);
    virtual ~KeyboardAdaptor() = default;
};

#endif

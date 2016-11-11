// keyboard_handler.hpp
// Copyright 2016 Steve Palmer

#ifndef KEYBOARD_ADAPTOR_HPP_
#define KEYBOARD_ADAPTOR_HPP_

#include <SDL.h>

#include "common.hpp"
#include "device.hpp"
#include "atom_keyboard_interface.hpp"

class KeyboardAdaptor
    : protected NonCopyable
{
    // Types
public:
    AtomKeyboardInterface *m_atom_keyboard;
private:
    std::map<SDL_Scancode, AtomKeyboardInterface::Key> keys;
public:
    KeyboardAdaptor(AtomKeyboardInterface *);
    virtual ~KeyboardAdaptor() = default;
    void handle(SDL_KeyboardEvent &);
};

#endif

// keyboard_controller.hpp

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

#include <ostream>
#include <SDL.h>

#include "common.hpp"
#include "terminal_interface.hpp"

class KeyboardController
{
    // Types
public:
    class Configurator
    {
    public:
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
private:
    TerminalInterface &m_terminal_interface;
private:
    KeyboardController(const KeyboardController &);
    KeyboardController &operator=(const KeyboardController&);
public:
    KeyboardController(TerminalInterface &, const Configurator &);
    void update(SDL_KeyboardEvent *);

    friend std::ostream &::operator<<(std::ostream&, const KeyboardController &);
};

#endif /* KEYBOARD_CONTROLLER_HPP_ */

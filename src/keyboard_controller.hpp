// keyboard_controller.hpp

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

#include <ostream>
#include <SDL.h>

#include "common.hpp"
#include "terminal_interface.hpp"

class KeyboardController
    : public NonCopyable
{
    // Types
public:
    class Configurator
        : public NonCopyable
    {
    protected:
        Configurator();
	public:
        ~Configurator();

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
private:
    TerminalInterface *m_terminal_interface;
public:
    KeyboardController(TerminalInterface *, const Configurator &);
    void update(SDL_KeyboardEvent *);

    friend std::ostream &::operator<<(std::ostream&, const KeyboardController &);
};

#endif /* KEYBOARD_CONTROLLER_HPP_ */

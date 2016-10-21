// keyboard_controller.hpp

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

#include <ostream>
#include <SDL.h>

#include "common.hpp"
#include "terminal_interface.hpp"

class KeyboardController
    : protected NonCopyable
{
    // Types
public:
    class Configurator
        : protected NonCopyable
    {
    protected:
        Configurator() = default;
	public:
        virtual ~Configurator() = default;

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };
private:
    TerminalInterface *m_terminal_interface;
public:
    KeyboardController(TerminalInterface *, const Configurator &);
    virtual ~KeyboardController() = default;
    void update(SDL_KeyboardEvent *);

    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const KeyboardController &p_kc)
        { p_kc.serialize(p_s); return p_s; }
};

#endif

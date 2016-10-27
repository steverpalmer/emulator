// keyboard_controller.hpp

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

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
    
    class KeyCommand
        : protected NonCopyable
    {
    protected:
        KeyboardController &m_state;
        explicit KeyCommand(KeyboardController &p_keyboard_controller)
            : m_state(p_keyboard_controller)
            {}
    public:
        virtual ~KeyCommand() = default;
        virtual void down() const = 0;
        virtual void up() const = 0;
    };

public:
    TerminalInterface *m_terminal_interface;
    gunichar m_last_key_pressed;
    bool is_shift_pressed;
    bool is_control_pressed;
private:
    std::map<SDL_Keycode, const KeyCommand *> keys;
public:
    KeyboardController(TerminalInterface *, const Configurator &);
    virtual ~KeyboardController() = default;

    void update(SDL_KeyboardEvent &);

    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const KeyboardController &p_kc)
        { p_kc.serialize(p_s); return p_s; }
};

#endif

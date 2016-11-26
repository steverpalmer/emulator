// keyboard_adaptor.cpp
// Copyright 2016 Steve Palmer

#include <cassert>

#include "keyboard_adaptor.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".keyboard_adaptor.cpp"));
    return result;
}

KeyboardAdaptor::DownHandler::DownHandler(const KeyboardAdaptor &p_keyboard_adaptor)
    : Dispatcher::StateHandler<const KeyboardAdaptor>(SDL_KEYDOWN, p_keyboard_adaptor)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::DownHandler::DownHandler()");
}

void KeyboardAdaptor::DownHandler::handle(const SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::DownHandler::handle(" << p_event.key.keysym.scancode << ")");
    try
    {
        const AtomKeyboardInterface::Key key = state.keys.at(p_event.key.keysym.scancode);
        switch (key)
        {
        case AtomKeyboardInterface::BREAK:
            state.reset_handler.push();
            break;
        default:
            if (state.m_atom_keyboard)
                state.m_atom_keyboard->down(key);
            break;
        }
    }
    catch (std::out_of_range e)
    {
        LOG4CXX_WARN(cpptrace_log(), "KeyboardAdaptor::DownHandler::handle: Unknown key " << p_event.key.keysym.scancode);
    }
}

KeyboardAdaptor::UpHandler::UpHandler(const KeyboardAdaptor &p_keyboard_adaptor)
    : Dispatcher::StateHandler<const KeyboardAdaptor>(SDL_KEYUP, p_keyboard_adaptor)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::UpHandler::UpHandler()");
}

void KeyboardAdaptor::UpHandler::handle(const SDL_Event &p_event)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::UpHandler::handle(" << p_event.key.keysym.scancode << ")");
    try
    {
        const AtomKeyboardInterface::Key key = state.keys.at(p_event.key.keysym.scancode);
        switch (key)
        {
        case AtomKeyboardInterface::BREAK:
            break;
        default:
            if (state.m_atom_keyboard)
                state.m_atom_keyboard->up(key);
            break;
        }
    }
    catch (std::out_of_range e)
    {
        LOG4CXX_WARN(cpptrace_log(), "KeyboardAdaptor::UpHandler::handle: Unknown key " << p_event.key.keysym.scancode);
    }
}

KeyboardAdaptor::KeyboardAdaptor(AtomKeyboardInterface *p_atom_keyboard, const Dispatcher::Handler &p_reset_handler)
    : m_atom_keyboard(p_atom_keyboard)
    , down_handler(*this)
    , up_handler(*this)
    , reset_handler(p_reset_handler)
{
    assert (m_atom_keyboard);
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::KeyboardAdaptor(" << p_atom_keyboard << ")");
    keys[SDL_SCANCODE_A] = AtomKeyboardInterface::A;
    keys[SDL_SCANCODE_B] = AtomKeyboardInterface::B;
    keys[SDL_SCANCODE_C] = AtomKeyboardInterface::C;
    keys[SDL_SCANCODE_D] = AtomKeyboardInterface::D;
    keys[SDL_SCANCODE_E] = AtomKeyboardInterface::E;
    keys[SDL_SCANCODE_F] = AtomKeyboardInterface::F;
    keys[SDL_SCANCODE_G] = AtomKeyboardInterface::G;
    keys[SDL_SCANCODE_H] = AtomKeyboardInterface::H;
    keys[SDL_SCANCODE_I] = AtomKeyboardInterface::I;
    keys[SDL_SCANCODE_J] = AtomKeyboardInterface::J;
    keys[SDL_SCANCODE_K] = AtomKeyboardInterface::K;
    keys[SDL_SCANCODE_L] = AtomKeyboardInterface::L;
    keys[SDL_SCANCODE_M] = AtomKeyboardInterface::M;
    keys[SDL_SCANCODE_N] = AtomKeyboardInterface::N;
    keys[SDL_SCANCODE_O] = AtomKeyboardInterface::O;
    keys[SDL_SCANCODE_P] = AtomKeyboardInterface::P;
    keys[SDL_SCANCODE_Q] = AtomKeyboardInterface::Q;
    keys[SDL_SCANCODE_R] = AtomKeyboardInterface::R;
    keys[SDL_SCANCODE_S] = AtomKeyboardInterface::S;
    keys[SDL_SCANCODE_T] = AtomKeyboardInterface::T;
    keys[SDL_SCANCODE_U] = AtomKeyboardInterface::U;
    keys[SDL_SCANCODE_V] = AtomKeyboardInterface::V;
    keys[SDL_SCANCODE_W] = AtomKeyboardInterface::W;
    keys[SDL_SCANCODE_X] = AtomKeyboardInterface::X;
    keys[SDL_SCANCODE_Y] = AtomKeyboardInterface::Y;
    keys[SDL_SCANCODE_Z] = AtomKeyboardInterface::Z;
    keys[SDL_SCANCODE_1] = AtomKeyboardInterface::_1_EXCLAMATION;
    keys[SDL_SCANCODE_2] = AtomKeyboardInterface::_2_DOUBLEQUOTE;
    keys[SDL_SCANCODE_3] = AtomKeyboardInterface::_3_NUMBER;
    keys[SDL_SCANCODE_4] = AtomKeyboardInterface::_4_DOLLAR;
    keys[SDL_SCANCODE_5] = AtomKeyboardInterface::_5_PERCENT;
    keys[SDL_SCANCODE_6] = AtomKeyboardInterface::_6_AMPERSAND;
    keys[SDL_SCANCODE_7] = AtomKeyboardInterface::_7_SINGLEQUOTE;
    keys[SDL_SCANCODE_8] = AtomKeyboardInterface::_8_LEFTPARENTHESIS;
    keys[SDL_SCANCODE_9] = AtomKeyboardInterface::_9_RIGHTPARENTHESIS;
    keys[SDL_SCANCODE_0] = AtomKeyboardInterface::_0;

    keys[SDL_SCANCODE_RETURN        ] = AtomKeyboardInterface::RETURN;
    keys[SDL_SCANCODE_BACKSPACE     ] = AtomKeyboardInterface::CIRCUMFLEX;
    keys[SDL_SCANCODE_TAB           ] = AtomKeyboardInterface::COPY;
    keys[SDL_SCANCODE_SPACE         ] = AtomKeyboardInterface::SPACE;
    keys[SDL_SCANCODE_MINUS         ] = AtomKeyboardInterface::MINUS_EQUALS;
    keys[SDL_SCANCODE_EQUALS        ] = AtomKeyboardInterface::COLON_ASTERISK;
    keys[SDL_SCANCODE_LEFTBRACKET   ] = AtomKeyboardInterface::AT;
    keys[SDL_SCANCODE_RIGHTBRACKET  ] = AtomKeyboardInterface::BACKSLASH;
    keys[SDL_SCANCODE_NONUSBACKSLASH] = AtomKeyboardInterface::LOCK;
    keys[SDL_SCANCODE_SEMICOLON     ] = AtomKeyboardInterface::SEMICOLON_PLUS;
    keys[SDL_SCANCODE_APOSTROPHE    ] = AtomKeyboardInterface::LEFTBRACKET;
    keys[SDL_SCANCODE_GRAVE         ] = AtomKeyboardInterface::ESCAPE;
    keys[SDL_SCANCODE_BACKSLASH     ] = AtomKeyboardInterface::RIGHTBRACKET;
    keys[SDL_SCANCODE_COMMA         ] = AtomKeyboardInterface::COMMA_LESSTHAN;
    keys[SDL_SCANCODE_PERIOD        ] = AtomKeyboardInterface::PERIOD_GREATERTHAN;
    keys[SDL_SCANCODE_SLASH         ] = AtomKeyboardInterface::SLASH_QUESTION;
    keys[SDL_SCANCODE_CAPSLOCK      ] = AtomKeyboardInterface::LOCK;
    keys[SDL_SCANCODE_DELETE        ] = AtomKeyboardInterface::DELETE;
    keys[SDL_SCANCODE_LEFT          ] = AtomKeyboardInterface::LEFT_RIGHT;
    keys[SDL_SCANCODE_RIGHT         ] = AtomKeyboardInterface::LEFT_RIGHT;
    keys[SDL_SCANCODE_UP            ] = AtomKeyboardInterface::UP_DOWN;
    keys[SDL_SCANCODE_DOWN          ] = AtomKeyboardInterface::UP_DOWN;

    keys[SDL_SCANCODE_LCTRL         ] = AtomKeyboardInterface::CTRL;
    keys[SDL_SCANCODE_LSHIFT        ] = AtomKeyboardInterface::SHIFT;
    keys[SDL_SCANCODE_LALT          ] = AtomKeyboardInterface::REPT;
    keys[SDL_SCANCODE_RCTRL         ] = AtomKeyboardInterface::CTRL;
    keys[SDL_SCANCODE_RSHIFT        ] = AtomKeyboardInterface::SHIFT;
    keys[SDL_SCANCODE_RALT          ] = AtomKeyboardInterface::REPT;

    keys[SDL_SCANCODE_PAUSE         ] = AtomKeyboardInterface::BREAK;

    keys[SDL_SCANCODE_F1            ] = AtomKeyboardInterface::SPARE1;
    keys[SDL_SCANCODE_F2            ] = AtomKeyboardInterface::SPARE2;
    keys[SDL_SCANCODE_F3            ] = AtomKeyboardInterface::SPARE3;
    keys[SDL_SCANCODE_F4            ] = AtomKeyboardInterface::SPARE4;
    keys[SDL_SCANCODE_F5            ] = AtomKeyboardInterface::SPARE5;
}

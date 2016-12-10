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
        const Atom::KeyboardInterface::Key key = state.keys.at(p_event.key.keysym.scancode);
        switch (key)
        {
        case Atom::KeyboardInterface::Key::BREAK:
            state.reset_handler.push();
            break;
        default:
            if (state.m_atom_keyboard)
                state.m_atom_keyboard->down(key);
            break;
        }
    }
    catch (std::out_of_range &e)
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
        const Atom::KeyboardInterface::Key key = state.keys.at(p_event.key.keysym.scancode);
        switch (key)
        {
        case Atom::KeyboardInterface::Key::BREAK:
            break;
        default:
            if (state.m_atom_keyboard)
                state.m_atom_keyboard->up(key);
            break;
        }
    }
    catch (std::out_of_range &e)
    {
        LOG4CXX_WARN(cpptrace_log(), "KeyboardAdaptor::UpHandler::handle: Unknown key " << p_event.key.keysym.scancode);
    }
}

KeyboardAdaptor::KeyboardAdaptor(Atom::KeyboardInterface *p_atom_keyboard, const Dispatcher::Handler &p_reset_handler)
    : m_atom_keyboard(p_atom_keyboard)
    , down_handler(*this)
    , up_handler(*this)
    , reset_handler(p_reset_handler)
{
    assert (m_atom_keyboard);
    LOG4CXX_INFO(cpptrace_log(), "KeyboardAdaptor::KeyboardAdaptor(" << p_atom_keyboard << ")");
    keys[SDL_SCANCODE_A] = Atom::KeyboardInterface::Key::A;
    keys[SDL_SCANCODE_B] = Atom::KeyboardInterface::Key::B;
    keys[SDL_SCANCODE_C] = Atom::KeyboardInterface::Key::C;
    keys[SDL_SCANCODE_D] = Atom::KeyboardInterface::Key::D;
    keys[SDL_SCANCODE_E] = Atom::KeyboardInterface::Key::E;
    keys[SDL_SCANCODE_F] = Atom::KeyboardInterface::Key::F;
    keys[SDL_SCANCODE_G] = Atom::KeyboardInterface::Key::G;
    keys[SDL_SCANCODE_H] = Atom::KeyboardInterface::Key::H;
    keys[SDL_SCANCODE_I] = Atom::KeyboardInterface::Key::I;
    keys[SDL_SCANCODE_J] = Atom::KeyboardInterface::Key::J;
    keys[SDL_SCANCODE_K] = Atom::KeyboardInterface::Key::K;
    keys[SDL_SCANCODE_L] = Atom::KeyboardInterface::Key::L;
    keys[SDL_SCANCODE_M] = Atom::KeyboardInterface::Key::M;
    keys[SDL_SCANCODE_N] = Atom::KeyboardInterface::Key::N;
    keys[SDL_SCANCODE_O] = Atom::KeyboardInterface::Key::O;
    keys[SDL_SCANCODE_P] = Atom::KeyboardInterface::Key::P;
    keys[SDL_SCANCODE_Q] = Atom::KeyboardInterface::Key::Q;
    keys[SDL_SCANCODE_R] = Atom::KeyboardInterface::Key::R;
    keys[SDL_SCANCODE_S] = Atom::KeyboardInterface::Key::S;
    keys[SDL_SCANCODE_T] = Atom::KeyboardInterface::Key::T;
    keys[SDL_SCANCODE_U] = Atom::KeyboardInterface::Key::U;
    keys[SDL_SCANCODE_V] = Atom::KeyboardInterface::Key::V;
    keys[SDL_SCANCODE_W] = Atom::KeyboardInterface::Key::W;
    keys[SDL_SCANCODE_X] = Atom::KeyboardInterface::Key::X;
    keys[SDL_SCANCODE_Y] = Atom::KeyboardInterface::Key::Y;
    keys[SDL_SCANCODE_Z] = Atom::KeyboardInterface::Key::Z;
    keys[SDL_SCANCODE_1] = Atom::KeyboardInterface::Key::_1_EXCLAMATION;
    keys[SDL_SCANCODE_2] = Atom::KeyboardInterface::Key::_2_DOUBLEQUOTE;
    keys[SDL_SCANCODE_3] = Atom::KeyboardInterface::Key::_3_NUMBER;
    keys[SDL_SCANCODE_4] = Atom::KeyboardInterface::Key::_4_DOLLAR;
    keys[SDL_SCANCODE_5] = Atom::KeyboardInterface::Key::_5_PERCENT;
    keys[SDL_SCANCODE_6] = Atom::KeyboardInterface::Key::_6_AMPERSAND;
    keys[SDL_SCANCODE_7] = Atom::KeyboardInterface::Key::_7_SINGLEQUOTE;
    keys[SDL_SCANCODE_8] = Atom::KeyboardInterface::Key::_8_LEFTPARENTHESIS;
    keys[SDL_SCANCODE_9] = Atom::KeyboardInterface::Key::_9_RIGHTPARENTHESIS;
    keys[SDL_SCANCODE_0] = Atom::KeyboardInterface::Key::_0;

    keys[SDL_SCANCODE_RETURN        ] = Atom::KeyboardInterface::Key::RETURN;
    keys[SDL_SCANCODE_BACKSPACE     ] = Atom::KeyboardInterface::Key::CIRCUMFLEX;
    keys[SDL_SCANCODE_TAB           ] = Atom::KeyboardInterface::Key::COPY;
    keys[SDL_SCANCODE_SPACE         ] = Atom::KeyboardInterface::Key::SPACE;
    keys[SDL_SCANCODE_MINUS         ] = Atom::KeyboardInterface::Key::MINUS_EQUALS;
    keys[SDL_SCANCODE_EQUALS        ] = Atom::KeyboardInterface::Key::COLON_ASTERISK;
    keys[SDL_SCANCODE_LEFTBRACKET   ] = Atom::KeyboardInterface::Key::AT;
    keys[SDL_SCANCODE_RIGHTBRACKET  ] = Atom::KeyboardInterface::Key::BACKSLASH;
    keys[SDL_SCANCODE_NONUSBACKSLASH] = Atom::KeyboardInterface::Key::LOCK;
    keys[SDL_SCANCODE_SEMICOLON     ] = Atom::KeyboardInterface::Key::SEMICOLON_PLUS;
    keys[SDL_SCANCODE_APOSTROPHE    ] = Atom::KeyboardInterface::Key::LEFTBRACKET;
    keys[SDL_SCANCODE_GRAVE         ] = Atom::KeyboardInterface::Key::ESCAPE;
    keys[SDL_SCANCODE_BACKSLASH     ] = Atom::KeyboardInterface::Key::RIGHTBRACKET;
    keys[SDL_SCANCODE_COMMA         ] = Atom::KeyboardInterface::Key::COMMA_LESSTHAN;
    keys[SDL_SCANCODE_PERIOD        ] = Atom::KeyboardInterface::Key::PERIOD_GREATERTHAN;
    keys[SDL_SCANCODE_SLASH         ] = Atom::KeyboardInterface::Key::SLASH_QUESTION;
    keys[SDL_SCANCODE_CAPSLOCK      ] = Atom::KeyboardInterface::Key::LOCK;
    keys[SDL_SCANCODE_DELETE        ] = Atom::KeyboardInterface::Key::DELETE;
    keys[SDL_SCANCODE_LEFT          ] = Atom::KeyboardInterface::Key::LEFT_RIGHT;
    keys[SDL_SCANCODE_RIGHT         ] = Atom::KeyboardInterface::Key::LEFT_RIGHT;
    keys[SDL_SCANCODE_UP            ] = Atom::KeyboardInterface::Key::UP_DOWN;
    keys[SDL_SCANCODE_DOWN          ] = Atom::KeyboardInterface::Key::UP_DOWN;

    keys[SDL_SCANCODE_LCTRL         ] = Atom::KeyboardInterface::Key::CTRL;
    keys[SDL_SCANCODE_LSHIFT        ] = Atom::KeyboardInterface::Key::SHIFT;
    keys[SDL_SCANCODE_LALT          ] = Atom::KeyboardInterface::Key::REPT;
    keys[SDL_SCANCODE_RCTRL         ] = Atom::KeyboardInterface::Key::CTRL;
    keys[SDL_SCANCODE_RSHIFT        ] = Atom::KeyboardInterface::Key::SHIFT;
    keys[SDL_SCANCODE_RALT          ] = Atom::KeyboardInterface::Key::REPT;

    keys[SDL_SCANCODE_PAUSE         ] = Atom::KeyboardInterface::Key::BREAK;

    keys[SDL_SCANCODE_F1            ] = Atom::KeyboardInterface::Key::SPARE1;
    keys[SDL_SCANCODE_F2            ] = Atom::KeyboardInterface::Key::SPARE2;
    keys[SDL_SCANCODE_F3            ] = Atom::KeyboardInterface::Key::SPARE3;
    keys[SDL_SCANCODE_F4            ] = Atom::KeyboardInterface::Key::SPARE4;
    keys[SDL_SCANCODE_F5            ] = Atom::KeyboardInterface::Key::SPARE5;
}

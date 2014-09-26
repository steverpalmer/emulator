/******************************************************************************
 * $Author: steve $
 * $Date: 2004/04/11 09:59:52 $
 * $Id: keyboard.c,v 1.2 2004/04/11 09:59:52 steve Exp $
 ******************************************************************************/
/**
 * The behaviour of the Keyboard mapping from the PC to the Atom is described in keyboard.h.
 * The implementation is described below.
 *
 * The implementation is based on SDL.
 *
 * Note that the SDL routines do not auto repeat, but can detect multiple key presses.
 * Therefore the Atom (which also does not auto repeat, but uses a REPT key instead) can be modelled.
 *
 * Note also that unlike other keys PC [Caps Lock] generates a Press event on first press, and then,
 * after a release, generates a Release event when next pressed. :-(  This may be a PC hardware artifact.
 *
 * KISS - The Atom model of the SHIFT and CTRL states is done based on the PC key being entered, which
 * may or may not involve actually pressing the corresponding PC keys.  On the other hand, the REPT key
 * is modelled based on pressing, say, the ALT (GR) key.
 *
 * The Main thread maps all PC keys to a Unicode value, and pass this to the PPIA module.
 * - This is a bit of a cheat, but should suffice for this application.
 * The PPIA module maps Unicode values to Row and Scan results.
 * - So only the PPIA module needs to know about the atom keyboard encoding.
 * Special keys are mapped as follows:
 *  The Atom Up arrow will be mapped to the Circumflex ^
 *  The Atom Cursor movement keys will be mapped to the Unicode Arrows:
 *   Left  => U2190
 *   Up    => U2191
 *   Right => U2192
 *   Down  => U2193
 *   Copy  => U2398 (Next Page)
 *   Rept  => Not mapped.
 *
 * The SDL_KeyboardEvent includes SDL's interpretation of the key as a unicode value.
 * For those keys which do not map to unicode, the unicode value appears to be given as 0.
 * (This value may also be returned for [CTRL]+[@], but this should not be a problem.)
 * So the processing goes something like:
 *
 * if key->keysym.unicode != 0
 *      result = key->keysym.unicode
 * else switch(key->keysym.sym)
 *      case SDLK_DOWN: result = 0x2193; break;
 *      case SDLK_LEFT: result = 0x2190; break;
 *      ...
 * Pass result to PPIA
 *
 ******************************************************************************/

#include <cassert>
#include <cctype>

#include <ostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "keyboard_controller.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".keyboard_controller.cpp"));
    return result;
}

KeyboardController::KeyboardController(Atom &p_atom, const Configurator &p_cfg)
  : Named(p_cfg)
  , m_atom(p_atom)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardController::KeyboardController([" << p_atom.name() << "], " << p_cfg << ")");
    (void)SDL_EnableUNICODE(1);
}

void KeyboardController::update(SDL_KeyboardEvent *p_key_event)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].update((" << Hex(p_key_event->type) << ", " << int(p_key_event->keysym.sym) << ", ...))");
    switch (p_key_event->type) {
    case SDL_KEYUP:                                                   // Release
        switch (p_key_event->keysym.sym) {
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            m_atom.set_is_shift_pressed(false);
            break;
        case SDLK_RCTRL:
        case SDLK_LCTRL:
            m_atom.set_is_ctrl_pressed(false);
            break;
        case SDLK_RALT:
        case SDLK_LALT:
            m_atom.set_is_rept_pressed(false);
            break;
        case SDLK_CAPSLOCK:
            m_atom.set_keypress(KBD_LOCK);
            break;
        case SDLK_NUMLOCK:
        case SDLK_SCROLLOCK:
        case SDLK_RMETA:
        case SDLK_LMETA:
        case SDLK_LSUPER:
        case SDLK_RSUPER:
        case SDLK_MODE:
        case SDLK_HELP:
        case SDLK_PRINT:
        case SDLK_SYSREQ:
        case SDLK_BREAK:
        case SDLK_MENU:
        case SDLK_POWER:
            // Do Nothing
            break;
        default:
            m_atom.set_keypress(KBD_NO_KEYPRESS);
            break;
        }
        break;
    case SDL_KEYDOWN:                                                   // Press
        if (p_key_event->keysym.unicode) {
            int key = p_key_event->keysym.unicode;
            if (key < 0x80) {     // Invert the Upper/Lower case'ness of the key
                if (std::islower(key))
                    key = std::toupper(key);
                else if (std::isupper(key))
                    key = std::tolower(key);
                else if (key == '\x08')           // Invert Backspace and Delete
                    key = '\x7F';
                else if (key == '\x7F')
                    key = '\x08';
            }
            m_atom.set_keypress(key);
        }
        else {
            switch (p_key_event->keysym.sym) {
            case SDLK_RSHIFT:
            case SDLK_LSHIFT:
                m_atom.set_is_shift_pressed(true);
                break;
            case SDLK_RCTRL:
            case SDLK_LCTRL:
                m_atom.set_is_ctrl_pressed(true);
                break;
            case SDLK_RALT:
            case SDLK_LALT:
                m_atom.set_is_rept_pressed(true);
                break;
            case SDLK_AT:
                m_atom.set_keypress(0);
                break;
            case SDLK_LEFT:
                m_atom.set_keypress(KBD_LEFT);
                break;
            case SDLK_UP:
                m_atom.set_keypress(KBD_UP);
                break;
            case SDLK_RIGHT:
                m_atom.set_keypress(KBD_RIGHT);
                break;
            case SDLK_DOWN:
                m_atom.set_keypress(KBD_DOWN);
                break;
            case SDLK_CAPSLOCK:
                m_atom.set_keypress(KBD_LOCK);
                break;
            case SDLK_F1:
                m_atom.set_keypress(KBD_COPY);
                break;
            case SDLK_F12:
                m_atom.reset();
                break;
            case SDLK_NUMLOCK:
            case SDLK_SCROLLOCK:
            case SDLK_RMETA:
            case SDLK_LMETA:
            case SDLK_LSUPER:
            case SDLK_RSUPER:
            case SDLK_MODE:
            case SDLK_HELP:
            case SDLK_PRINT:
            case SDLK_SYSREQ:
            case SDLK_BREAK:
            case SDLK_MENU:
            case SDLK_POWER:
                // Do Nothing
                break;
            default:
                m_atom.set_keypress(KBD_NO_KEYPRESS);
                break;
            }
        }
        break;
    default :
        assert(0);
    }
}

std::ostream &operator<<(std::ostream &p_s, const KeyboardController::Configurator &p_cfg)
{
    p_s << static_cast<const Named::Configurator &>(p_cfg);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const KeyboardController &p_kc)
{
    p_s << static_cast<const Named &>(p_kc)
        << "atom:" << p_kc.m_atom.name();
    return p_s;
}

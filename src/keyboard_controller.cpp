// keyboard_controller.cpp

/**
 * The behaviour of the Keyboard mapping from the PC to the Atom is described in keyboard.hpp.
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
 ******************************************************************************/

#include <cassert>
#include <cctype>

#include <iomanip>

#include "keyboard_controller.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".keyboard_controller.cpp"));
    return result;
}

class KeySimple
    : public KeyboardController::KeyCommand
{
private:
    gunichar m_unichar;
    gunichar m_shift;
    gunichar m_control;
public:
    KeySimple(KeyboardController &p_keyboard_controller,
              gunichar p_unichar,
              gunichar p_shift=TerminalInterface::KBD_NO_KEYPRESS,
              gunichar p_control=TerminalInterface::KBD_NO_KEYPRESS)
        : KeyCommand(p_keyboard_controller)
        , m_unichar(p_unichar)
        , m_shift(p_shift)
        , m_control(p_control)
        {
            assert (p_unichar != TerminalInterface::KBD_NO_KEYPRESS);
        }
    virtual ~KeySimple() = default;
    virtual void down() const
        {
            const gunichar key =
                (m_state.is_control_pressed && m_control!=TerminalInterface::KBD_NO_KEYPRESS) ? m_control
                : (m_state.is_shift_pressed && m_shift!=TerminalInterface::KBD_NO_KEYPRESS) ? m_shift
                : m_unichar;
            m_state.m_terminal_interface->set_keypress(key, m_state.m_last_key_pressed == key);
            m_state.m_last_key_pressed = key;
        }
    virtual void up() const
        {
            m_state.m_last_key_pressed = TerminalInterface::KBD_NO_KEYPRESS;
            m_state.m_terminal_interface->set_keypress(TerminalInterface::KBD_NO_KEYPRESS);
        }
};

class KeyModifier
    : public KeyboardController::KeyCommand
{
private:
    bool &m_flag;
public:
    KeyModifier(KeyboardController &p_keyboard_controller, bool &p_flag)
        : KeyCommand(p_keyboard_controller)
        , m_flag(p_flag)
        {}
    virtual ~KeyModifier() = default;
    virtual void down() const
        { m_flag = true; }
    virtual void up() const
        { m_flag = false; }
};


KeyboardController::KeyboardController(TerminalInterface *p_terminal_interface, const Configurator &p_cfgr)
    : m_terminal_interface(p_terminal_interface)
    , m_last_key_pressed(TerminalInterface::KBD_NO_KEYPRESS)
    , is_shift_pressed(false)
    , is_control_pressed(false)
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardController::KeyboardController("
                 << "<controller name=\"" << p_terminal_interface->id() << "\"/>"
                 << p_cfgr << ")");
    keys[SDLK_RETURN]       = new KeySimple(*this, '\r');
    keys[SDLK_ESCAPE]       = new KeySimple(*this, 0x1B);
    keys[SDLK_BACKSPACE]    = new KeySimple(*this, 0x7F);  // Swap Delete and Backspace
    keys[SDLK_TAB]          = new KeySimple(*this, '\t');
    keys[SDLK_SPACE]        = new KeySimple(*this, ' ');
    keys[SDLK_EXCLAIM]      = new KeySimple(*this, '!', '1');
    keys[SDLK_QUOTEDBL]     = new KeySimple(*this, '"', '2');
    keys[SDLK_HASH]         = new KeySimple(*this, '#', '~');
    keys[SDLK_PERCENT]      = new KeySimple(*this, '%', '5');
    keys[SDLK_DOLLAR]       = new KeySimple(*this, '$', '4');
    keys[SDLK_AMPERSAND]    = new KeySimple(*this, '&', '7');
    keys[SDLK_QUOTE]        = new KeySimple(*this, '\'', '@');
    keys[SDLK_LEFTPAREN]    = new KeySimple(*this, '(', '9');
    keys[SDLK_RIGHTPAREN]   = new KeySimple(*this, ')', '0');
    keys[SDLK_ASTERISK]     = new KeySimple(*this, '*', '8');
    keys[SDLK_PLUS]         = new KeySimple(*this, '+', '=');
    keys[SDLK_COMMA]        = new KeySimple(*this, ',', '<');
    keys[SDLK_MINUS]        = new KeySimple(*this, '-', '_');
    keys[SDLK_PERIOD]       = new KeySimple(*this, '.', '>');
    keys[SDLK_SLASH]        = new KeySimple(*this, '/', '?');
    keys[SDLK_0]            = new KeySimple(*this, '0', ')');
    keys[SDLK_1]            = new KeySimple(*this, '1', '!');
    keys[SDLK_2]            = new KeySimple(*this, '2', '"');
    keys[SDLK_3]            = new KeySimple(*this, '3');
    keys[SDLK_4]            = new KeySimple(*this, '4', '$');
    keys[SDLK_5]            = new KeySimple(*this, '5', '%');
    keys[SDLK_6]            = new KeySimple(*this, '6', '^');
    keys[SDLK_7]            = new KeySimple(*this, '7', '&');
    keys[SDLK_8]            = new KeySimple(*this, '8', '*');
    keys[SDLK_9]            = new KeySimple(*this, '9', '(');
    keys[SDLK_COLON]        = new KeySimple(*this, ':', ')');
    keys[SDLK_SEMICOLON]    = new KeySimple(*this, ';', ':');
    keys[SDLK_LESS]         = new KeySimple(*this, '<', ',');
    keys[SDLK_EQUALS]       = new KeySimple(*this, '=', '+');
    keys[SDLK_GREATER]      = new KeySimple(*this, '>', '.');
    keys[SDLK_QUESTION]     = new KeySimple(*this, '?', '/');
    keys[SDLK_AT]           = new KeySimple(*this, '@', '\'');
    keys[SDLK_LEFTBRACKET]  = new KeySimple(*this, '[', '[', 0x1B);
    keys[SDLK_BACKSLASH]    = new KeySimple(*this, '\\', '\\', 0x1C);
    keys[SDLK_RIGHTBRACKET] = new KeySimple(*this, ']', ']', 0x1D);
    keys[SDLK_CARET]        = new KeySimple(*this, '^', '6', 0x1E);
    keys[SDLK_UNDERSCORE]   = new KeySimple(*this, '_', '-', 0x1F);
    keys[SDLK_BACKQUOTE]    = new KeySimple(*this, '`');
    keys[SDLK_a]            = new KeySimple(*this, 'A', 'a', 0x01);  // swapping upper and lower case
    keys[SDLK_b]            = new KeySimple(*this, 'B', 'b', 0x02);  // swapping upper and lower case
    keys[SDLK_c]            = new KeySimple(*this, 'C', 'c', 0x03);  // swapping upper and lower case
    keys[SDLK_d]            = new KeySimple(*this, 'D', 'd', 0x04);  // swapping upper and lower case
    keys[SDLK_e]            = new KeySimple(*this, 'E', 'e', 0x05);  // swapping upper and lower case
    keys[SDLK_f]            = new KeySimple(*this, 'F', 'f', 0x06);  // swapping upper and lower case
    keys[SDLK_g]            = new KeySimple(*this, 'G', 'g', 0x07);  // swapping upper and lower case
    keys[SDLK_h]            = new KeySimple(*this, 'H', 'h', 0x08);  // swapping upper and lower case
    keys[SDLK_i]            = new KeySimple(*this, 'I', 'i', 0x09);  // swapping upper and lower case
    keys[SDLK_j]            = new KeySimple(*this, 'J', 'j', 0x0A);  // swapping upper and lower case
    keys[SDLK_k]            = new KeySimple(*this, 'K', 'k', 0x0B);  // swapping upper and lower case
    keys[SDLK_l]            = new KeySimple(*this, 'L', 'l', 0x0C);  // swapping upper and lower case
    keys[SDLK_m]            = new KeySimple(*this, 'M', 'm', 0x0D);  // swapping upper and lower case
    keys[SDLK_n]            = new KeySimple(*this, 'N', 'n', 0x0E);  // swapping upper and lower case
    keys[SDLK_o]            = new KeySimple(*this, 'O', 'o', 0x0F);  // swapping upper and lower case
    keys[SDLK_p]            = new KeySimple(*this, 'P', 'p', 0x10);  // swapping upper and lower case
    keys[SDLK_q]            = new KeySimple(*this, 'Q', 'q', 0x11);  // swapping upper and lower case
    keys[SDLK_r]            = new KeySimple(*this, 'R', 'r', 0x12);  // swapping upper and lower case
    keys[SDLK_s]            = new KeySimple(*this, 'S', 's', 0x13);  // swapping upper and lower case
    keys[SDLK_t]            = new KeySimple(*this, 'T', 't', 0x14);  // swapping upper and lower case
    keys[SDLK_u]            = new KeySimple(*this, 'U', 'u', 0x15);  // swapping upper and lower case
    keys[SDLK_v]            = new KeySimple(*this, 'V', 'v', 0x16);  // swapping upper and lower case
    keys[SDLK_w]            = new KeySimple(*this, 'W', 'w', 0x17);  // swapping upper and lower case
    keys[SDLK_x]            = new KeySimple(*this, 'X', 'x', 0x18);  // swapping upper and lower case
    keys[SDLK_y]            = new KeySimple(*this, 'Y', 'y', 0x19);  // swapping upper and lower case
    keys[SDLK_z]            = new KeySimple(*this, 'Z', 'z', 0x1A);  // swapping upper and lower case

    keys[SDLK_CAPSLOCK]     = new KeySimple(*this, TerminalInterface::KBD_LOCK);

    keys[SDLK_HOME]         = new KeySimple(*this, 0x1e);
    keys[SDLK_DELETE]       = new KeySimple(*this, 0x08);  // Swap Delete and Backspace
    keys[SDLK_RIGHT]        = new KeySimple(*this, TerminalInterface::KBD_RIGHT);
    keys[SDLK_LEFT]         = new KeySimple(*this, TerminalInterface::KBD_LEFT);
    keys[SDLK_DOWN]         = new KeySimple(*this, TerminalInterface::KBD_DOWN);
    keys[SDLK_UP]           = new KeySimple(*this, TerminalInterface::KBD_UP);

    keys[SDLK_KP_DIVIDE]    = new KeySimple(*this, '/');
    keys[SDLK_KP_MULTIPLY]  = new KeySimple(*this, '*');
    keys[SDLK_KP_MINUS]     = new KeySimple(*this, '-');
    keys[SDLK_KP_PLUS]      = new KeySimple(*this, '+');
    keys[SDLK_KP_ENTER]     = new KeySimple(*this, '\r');
    keys[SDLK_KP_1]         = new KeySimple(*this, '1');
    keys[SDLK_KP_2]         = new KeySimple(*this, '2');
    keys[SDLK_KP_3]         = new KeySimple(*this, '3');
    keys[SDLK_KP_4]         = new KeySimple(*this, '4');
    keys[SDLK_KP_5]         = new KeySimple(*this, '5');
    keys[SDLK_KP_6]         = new KeySimple(*this, '6');
    keys[SDLK_KP_7]         = new KeySimple(*this, '7');
    keys[SDLK_KP_8]         = new KeySimple(*this, '8');
    keys[SDLK_KP_9]         = new KeySimple(*this, '9');
    keys[SDLK_KP_0]         = new KeySimple(*this, '0');
    keys[SDLK_KP_PERIOD]    = new KeySimple(*this, '.');

    keys[SDLK_LCTRL]        = new KeyModifier(*this, is_control_pressed);
    keys[SDLK_LSHIFT]       = new KeyModifier(*this, is_shift_pressed);
    keys[SDLK_RCTRL]        = new KeyModifier(*this, is_control_pressed);
    keys[SDLK_RSHIFT]       = new KeyModifier(*this, is_shift_pressed);
}

void KeyboardController::update(SDL_KeyboardEvent &p_key_event)
{
    LOG4CXX_INFO(cpptrace_log(), "update((" << p_key_event.type << ", " << int(p_key_event.keysym.sym) << ", ...))");
    try
    {
        const KeyCommand *key_cmd = keys.at(p_key_event.keysym.sym);
        if (key_cmd)
            switch (p_key_event.type)
            {
            case SDL_KEYDOWN:  // Press
                key_cmd->down();
                break;
            case SDL_KEYUP:  // Release
                key_cmd->up();
                break;
            default :
                assert(0);
            }
    }
    catch (std::out_of_range &e)
    {
        LOG4CXX_WARN(cpptrace_log(), "Unmapped key:" << p_key_event.keysym.sym);
    }
}


void KeyboardController::Configurator::serialize(std::ostream &p_s) const
{
}

void KeyboardController::serialize(std::ostream &p_s) const
{
    p_s << "KeyboardController("
        << ")";
}

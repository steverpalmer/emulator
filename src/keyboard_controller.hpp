/******************************************************************************
 * $Author: steve $
 * $Date: 2004/04/11 09:59:52 $
 * $Id: keyboard.h,v 1.2 2004/04/11 09:59:52 steve Exp $
 ******************************************************************************/
/**
 * Keyboards can work in either of two ways:
 *  1. Layout mapping - keys in approximate positions map correctly
 *  2. Symbol mapping - keys with the same symbol map correctly
 *
 * Atom Keyboard Layout:
 *
 *                [ESC] [1!]  [2"] [3#] [4$] [5%] [6&] [7'] [8(] [9)] [0] [-=] [:*] [^] [BREAK]
 *   [Left or Right] [COPY] [Q]  [W]  [E]  [R]  [T]  [Y]  [U]  [I]  [O] [P]  [@]  [\] [DELETE]
 *       [Up or Down] [CTRL] [A]  [S]  [D]  [F]  [G]  [H]  [J]  [K]  [L] [;+] [[]  []] [RETURN]
 *             [LOCK] [SHIFT] [Z]  [X]  [C]  [V]  [B]  [N]  [M]  [,<] [.>] [/?] [SHIFT] [REPT]
 *                                                 [Space]
 *
 * There is a good match for the letters and numbers, but many punctuation and "control"
 * keys are misplaced.  At least, this is the case with my keyboard.  I can't say what
 * the layout match might be like on future PC keyboards.  Although the layout is similar,
 * I don't think that there are any standards.
 *
 * However, the user will be typing on a PC keyboard, and forcing him to press
 * [SHIFT]+[7&] to get a ['] seems very unfriendly.  Therefore, it is probably best
 * to use a symbol mapping.  When the user presses ['] on the PC, the simulator
 * maps this for a [SHIFT]+[7'] key.  This means that the SHIFT status (pressed/release)
 * may be determined by the PC key being pressed, not just the [SHIFT] key being pressed.
 * Also, ['] simulates the [SHIFT] key being pressed, but [:] (which does require a
 * PC [SHIFT] must lose the shift status in the mapping to [:*].
 *
 * On the other hand, some atom programs probed the keyboard directly looking for the
 * [SHIFT] and [CTRL] keys directly.  Perhaps if the [SHIFT] key is held for
 * a second, then it is directly mapped?
 *
 * Some keys simply do not appear on a modern PC keyboard (such as [COPY] and [REPT]).
 * The simple thing would be to map these to function keys, or [ALT] and [ALT GR] keys.
 *
 * Of course, some PC keys may need to the processed by the simulator (as opposed to the
 * simulated atom) - or maybe this are handled as GUI buttons?
 *
 * Then again, some keys are hotkeys intercepted by the OS and not passed to the applications.
 * For example, [PrntScr] and the "media" "keys" don't appear to reach the application.
 *
 ******************************************************************************/

#ifndef KEYBOARD_CONTROLLER_HPP_
#define KEYBOARD_CONTROLLER_HPP_

#include <ostream>
#include <SDL.h>

#include "common.hpp"
#include "atom.hpp"

class KeyboardController : public Named {
    // Types
public:
    class Configurator : public Named::Configurator
    {
    public:
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
private:
    Atom &m_atom;
private:
    KeyboardController(const KeyboardController &);
    KeyboardController &operator=(const KeyboardController&);
public:
    KeyboardController(Atom &, const Configurator &);
    void update(SDL_KeyboardEvent *);

    friend std::ostream &::operator<<(std::ostream&, const KeyboardController &);
};

#endif /* KEYBOARD_CONTROLLER_HPP_ */

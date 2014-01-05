/*
 * ppia.cpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#include <cassert>
#include <ostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "ppia.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".ppia.cpp"));
    return result;
}

enum Ports {
    PortA = 0,
    PortB = 1,
    PortC = 2,
    ControlPort = 3
};

#define SCANCODE(CLMN, SHFT_CTRL) (byte(~((1 << (CLMN)) | (SHFT_CTRL))))

#define SHIFT   byte(0x80)
#define CONTROL byte(0x40)


///****************************************************************************
/// PPIA
///
/// The PPIA module is a kind of composite/decorator on the individual ports
///****************************************************************************
/// Port A: Outputs
///
/// b3..b0 - Keyboard Row request
/// b7..b4 - Graphics Mode request
///
/// The Keyboard Row request is modelled by setting the shared m_keyboard_row.
/// The Graphics Mode request is modelled locally by calling scr_set_mode.
///****************************************************************************
/// Port B: Inputs (all active low)
///
/// b5..b0 - Keyboard Column response
/// b6     - Control Key Press return
/// b7     - Shift Key Press return
///
/// The keyboard response is modelled using the kbd module and the shared
/// m_keyboard_row value from Port A.
///****************************************************************************
/// Port C: Output and Input
///
/// b0 - Tape Output - Not Modelled
/// b1 - 2.4 kHz Enable - Not Modelled
/// b2 - Loud Speaker - Not Modelled
/// b3 - Not Used
/// b4 - 2.4 kHz Input - Not well Modelled
/// b5 - Tape Input - Not well Modelled
/// b6 - Repeat Key Press (active low)
/// b7 - 60 Hz Video Flyback Sync
///
/// The Repeat Key Press is modelled using the kdb module.
/// All other inputs are module simply as a 2 state flip-flop.
/// Outputs are ignored.
///
/// TODO: More complete model (#9)
///****************************************************************************
/// Control Port
///
/// Acorn Atom only uses code 0x8A which configures Ports A, B & C appropriately
///****************************************************************************

Ppia::Ppia(const Configurator &p_cfg)
  : Device(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::Ppia(" << p_cfg << ")");
    assert (p_cfg.size() == 4);
}

word Ppia::Configurator::size() const
{
	return word(4);
}

Ppia::~Ppia()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].~Ppia()");
}

byte Ppia::get_PortB(int p_row)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_PortB(" << p_row << ")");
    byte result(0xFF); // Active Low, so start with all high
    static const struct KeyData {
        byte m_row;
        byte m_scan;
    } key_mapping[128] = {
    		{ 7, SCANCODE(3, CONTROL) }, // 00 - <Ctrl> @ (nul)
    		{ 6, SCANCODE(3, CONTROL) }, // 01 - <Ctrl> A (soh)
    		{ 5, SCANCODE(3, CONTROL) }, // 02 - <Ctrl> B (stx) start printer
    		{ 4, SCANCODE(3, CONTROL) }, // 03 - <Ctrl> C (etx) end printer
    		{ 3, SCANCODE(3, CONTROL) }, // 04 - <Ctrl> D (eot)
    		{ 2, SCANCODE(3, CONTROL) }, // 05 - <Ctrl> E (enq)
    		{ 1, SCANCODE(3, CONTROL) }, // 06 - <Ctrl> F (ack) start screen
    		{ 0, SCANCODE(3, CONTROL) }, // 07 - <Ctrl> G (bel) bleep
    		{ 9, SCANCODE(4, CONTROL) }, // 08 - <Ctrl> H (bs)  backspace
    		{ 8, SCANCODE(4, CONTROL) }, // 09 - <Ctrl> I (tab) horizontal tab
    		{ 6, SCANCODE(1, CONTROL) }, // 0A - <Ctrl> J (lf)  linefeed
    		{ 6, SCANCODE(4, CONTROL) }, // 0B - <Ctrl> K (vt)  vertical tab
    		{ 5, SCANCODE(4, CONTROL) }, // 0C - <Ctrl> L (ff)  formfeed
    		{ 4, SCANCODE(4, CONTROL) }, // 0D - <Ctrl> M (cr)  return
    		{ 3, SCANCODE(4, CONTROL) }, // 0E - <Ctrl> N (so)  page mode on
    		{ 2, SCANCODE(4, CONTROL) }, // 0F - <Ctrl> O (si)  page mode off
    		{ 1, SCANCODE(4, CONTROL) }, // 10 - <Ctrl> P (dle)
    		{ 0, SCANCODE(4, CONTROL) }, // 11 - <Ctrl> Q (dc1)
    		{ 9, SCANCODE(5, CONTROL) }, // 12 - <Ctrl> R (dc2)
    		{ 8, SCANCODE(5, CONTROL) }, // 13 - <Ctrl> S (dc3)
    		{ 7, SCANCODE(5, CONTROL) }, // 14 - <Ctrl> T (dc4)
    		{ 6, SCANCODE(5, CONTROL) }, // 15 - <Ctrl> U (nak) end screen
    		{ 5, SCANCODE(5, CONTROL) }, // 16 - <Ctrl> V (syn)
    		{ 4, SCANCODE(5, CONTROL) }, // 17 - <Ctrl> W (etb)
    		{ 3, SCANCODE(5, CONTROL) }, // 18 - <Ctrl> X (can) cancel
    		{ 2, SCANCODE(5, CONTROL) }, // 19 - <Ctrl> Y (em)
    		{ 1, SCANCODE(5, CONTROL) }, // 1A - <Ctrl> Z (sub)
    		{ 8, SCANCODE(0, CONTROL) }, // 1B - <Ctrl> [ (esc) escape
    		{ 7, SCANCODE(0, CONTROL) }, // 1C - <Ctrl> \ (fs)
    		{ 6, SCANCODE(0, CONTROL) }, // 1D - <Ctrl> ] (gs)
    		{ 5, SCANCODE(0, CONTROL) }, // 1E - <Ctrl> <Up> (rs) home cursor
    		{ 4, SCANCODE(0, CONTROL) }, // 1F - <Ctrl> <Left> (us)
    		{ 9, SCANCODE(0, 0      ) }, // 20 - <Space>
    		{ 2, SCANCODE(1, SHIFT  ) }, // 21 - '!'
    		{ 1, SCANCODE(1, SHIFT  ) }, // 22 - '"'
    		{ 0, SCANCODE(1, SHIFT  ) }, // 23 - '#'
    		{ 9, SCANCODE(2, SHIFT  ) }, // 24 - '$'
    		{ 8, SCANCODE(2, SHIFT  ) }, // 25 - '%'
    		{ 7, SCANCODE(2, SHIFT  ) }, // 26 - '&'
    		{ 6, SCANCODE(2, SHIFT  ) }, // 27 - '''
    		{ 5, SCANCODE(2, SHIFT  ) }, // 28 - '('
    		{ 4, SCANCODE(2, SHIFT  ) }, // 29 - ')'
    		{ 3, SCANCODE(2, SHIFT  ) }, // 2A - '*'
    		{ 2, SCANCODE(2, SHIFT  ) }, // 2B - '+'
    		{ 1, SCANCODE(2, 0      ) }, // 2C - ','
    		{ 0, SCANCODE(2, 0      ) }, // 2D - '-'
    		{ 9, SCANCODE(3, 0      ) }, // 2E - '.'
    		{ 8, SCANCODE(3, 0      ) }, // 2F - '/'
    		{ 3, SCANCODE(1, 0      ) }, // 30 - '0'
    		{ 2, SCANCODE(1, 0      ) }, // 31 - '1'
    		{ 1, SCANCODE(1, 0      ) }, // 32 - '2'
    		{ 0, SCANCODE(1, 0      ) }, // 33 - '3'
    		{ 9, SCANCODE(2, 0      ) }, // 34 - '4'
    		{ 8, SCANCODE(2, 0      ) }, // 35 - '5'
    		{ 7, SCANCODE(2, 0      ) }, // 36 - '6'
    		{ 6, SCANCODE(2, 0      ) }, // 37 - '7'
    		{ 5, SCANCODE(2, 0      ) }, // 38 - '8'
    		{ 4, SCANCODE(2, 0      ) }, // 39 - '9'
    		{ 3, SCANCODE(2, 0      ) }, // 3A - ':'
    		{ 2, SCANCODE(2, 0      ) }, // 3B - ';'
    		{ 1, SCANCODE(2, SHIFT  ) }, // 3C - '<'
    		{ 0, SCANCODE(2, SHIFT  ) }, // 3D - '='
    		{ 9, SCANCODE(3, SHIFT  ) }, // 3E - '>'
    		{ 8, SCANCODE(3, SHIFT  ) }, // 3F - '?'
    		{ 7, SCANCODE(3, 0      ) }, // 40 - '@'
    		{ 6, SCANCODE(3, 0      ) }, // 41 - 'A'
    		{ 5, SCANCODE(3, 0      ) }, // 42 - 'B'
    		{ 4, SCANCODE(3, 0      ) }, // 43 - 'C'
    		{ 3, SCANCODE(3, 0      ) }, // 44 - 'D'
    		{ 2, SCANCODE(3, 0      ) }, // 45 - 'E'
    		{ 1, SCANCODE(3, 0      ) }, // 46 - 'F'
    		{ 0, SCANCODE(3, 0      ) }, // 47 - 'G'
    		{ 9, SCANCODE(4, 0      ) }, // 48 - 'H'
    		{ 8, SCANCODE(4, 0      ) }, // 49 - 'I'
    		{ 7, SCANCODE(4, 0      ) }, // 4A - 'J'
    		{ 6, SCANCODE(4, 0      ) }, // 4B - 'K'
    		{ 5, SCANCODE(4, 0      ) }, // 4C - 'L'
    		{ 4, SCANCODE(4, 0      ) }, // 4D - 'M,
    		{ 3, SCANCODE(4, 0      ) }, // 4E - 'N'
    		{ 2, SCANCODE(4, 0      ) }, // 4F - 'O'
    		{ 1, SCANCODE(4, 0      ) }, // 50 - 'P'
    		{ 0, SCANCODE(4, 0      ) }, // 51 - 'Q'
    		{ 9, SCANCODE(5, 0      ) }, // 52 - 'R'
    		{ 8, SCANCODE(5, 0      ) }, // 53 - 'S'
    		{ 7, SCANCODE(5, 0      ) }, // 54 - 'T'
    		{ 6, SCANCODE(5, 0      ) }, // 55 - 'U'
    		{ 5, SCANCODE(5, 0      ) }, // 56 - 'V'
    		{ 4, SCANCODE(5, 0      ) }, // 57 - 'W'
    		{ 3, SCANCODE(5, 0      ) }, // 58 - 'X'
    		{ 2, SCANCODE(5, 0      ) }, // 59 - 'Y'
    		{ 1, SCANCODE(5, 0      ) }, // 5A - 'Z'
    		{ 8, SCANCODE(0, 0      ) }, // 5B - '['
    		{ 7, SCANCODE(0, 0      ) }, // 5C - '\'
    		{ 6, SCANCODE(0, 0      ) }, // 5D - ']'
    		{ 5, SCANCODE(0, 0      ) }, // 5E - '^'
    		{ 4, SCANCODE(0, 0      ) }, // 5F - '_'
    		{ 0, SCANCODE(5, 0      ) }, // 60 - '`' mapped to <Esc>
    		{ 6, SCANCODE(3, SHIFT  ) }, // 61 - 'a'
    		{ 5, SCANCODE(3, SHIFT  ) }, // 62 - 'b'
    		{ 4, SCANCODE(3, SHIFT  ) }, // 63 - 'c'
    		{ 3, SCANCODE(3, SHIFT  ) }, // 64 - 'd'
    		{ 2, SCANCODE(3, SHIFT  ) }, // 65 - 'e'
    		{ 1, SCANCODE(3, SHIFT  ) }, // 66 - 'f'
    		{ 0, SCANCODE(3, SHIFT  ) }, // 67 - 'g'
    		{ 9, SCANCODE(4, SHIFT  ) }, // 68 - 'h'
    		{ 8, SCANCODE(4, SHIFT  ) }, // 69 - 'i'
    		{ 7, SCANCODE(4, SHIFT  ) }, // 6A - 'j'
    		{ 6, SCANCODE(4, SHIFT  ) }, // 6B - 'k'
    		{ 5, SCANCODE(4, SHIFT  ) }, // 6C - 'l'
    		{ 4, SCANCODE(4, SHIFT  ) }, // 6D - 'm'
    		{ 3, SCANCODE(4, SHIFT  ) }, // 6E - 'n'
    		{ 2, SCANCODE(4, SHIFT  ) }, // 6F - 'o'
    		{ 1, SCANCODE(4, SHIFT  ) }, // 70 - 'p'
    		{ 0, SCANCODE(4, SHIFT  ) }, // 71 - 'q'
    		{ 9, SCANCODE(5, SHIFT  ) }, // 72 - 'r'
    		{ 8, SCANCODE(5, SHIFT  ) }, // 73 - 's'
    		{ 7, SCANCODE(5, SHIFT  ) }, // 74 - 't'
    		{ 6, SCANCODE(5, SHIFT  ) }, // 75 - 'u'
    		{ 5, SCANCODE(5, SHIFT  ) }, // 76 - 'v'
    		{ 4, SCANCODE(5, SHIFT  ) }, // 77 - 'w'
    		{ 3, SCANCODE(5, SHIFT  ) }, // 78 - 'x'
    		{ 2, SCANCODE(5, SHIFT  ) }, // 79 - 'y'
    		{ 1, SCANCODE(5, SHIFT  ) }, // 7A - 'z'
    		{ 8, SCANCODE(0, SHIFT  ) }, // 7B - '{'
    		{ 7, SCANCODE(0, SHIFT  ) }, // 7C - '|'
    		{ 6, SCANCODE(0, SHIFT  ) }, // 7D - '}'
    		{ 5, SCANCODE(0, SHIFT  ) }, // 7E - '~'
    		{ 4, SCANCODE(1, 0      ) }  // 7F - <del>
    };
    if (m_io.m_pressed_key >= 0 && m_io.m_pressed_key < 128) { // ASCII 'normal' Key
        const KeyData &key_data(key_mapping[m_io.m_pressed_key]);
        result = key_data.m_scan;
        if (p_row != key_data.m_row)
            result |= ~(SHIFT | CONTROL);
    }
    else switch (m_io.m_pressed_key) {                           // Special keys
        default:                                       // Warn of unknown keypresses
            assert (false);                               // but otherwise ignore it
        case KBD_NO_KEYPRESS:
            // simply model the PC SHIFT and CTRL keys
            if (m_io.m_is_shift_pressed) result &= ~SHIFT;
            if (m_io.m_is_ctrl_pressed)  result &= ~CONTROL;
            break;
        case KBD_LEFT:
            if (p_row == 3)
                result = SCANCODE(0, SHIFT);
            else
                result = byte(~SHIFT);
            break;
        case KBD_UP:
            if (p_row == 2)
                result = SCANCODE(0, SHIFT);
            break;
        case KBD_RIGHT:
            if (p_row == 3)
                result = SCANCODE(0, 0);
            break;
        case KBD_DOWN:
            if (p_row == 2)
                result = SCANCODE(0, SHIFT);
            else
                result = byte(~SHIFT);
            break;
        case KBD_LOCK:
            if (p_row == 4)
                result = SCANCODE(0, 0);
            break;
        case KBD_COPY:
            if (p_row == 5)
                result = SCANCODE(1, 0);
            break;
        }
    //LOG4CXX_WARN(cpptrace_log(), "[" << name() << "].get_PortB(" << p_row << ") => " << Hex(result));
    return result;
}

byte Ppia::get_PortC(byte p_previous)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_PortC(" << Hex(p_previous) << ")");
    byte result(p_previous & 0xBF);          // clear REPT and FLYBACK signals
    if (!m_io.m_is_rept_pressed)
        result |= 0x40;
#if 0
    if (m_io.m_is_vdg_refresh)
        result |= 0x80;
#else
    result ^= 0x80;
#endif
    result ^= 0x30;                                      // Flip Tape input bits
    return result;
}

byte Ppia::get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    p_addr &= 3;                  // Only interested in least significant 2 bits
    byte result(m_byte[p_addr]);           // By default, set it to last value
    switch (p_addr)
        {
        case PortA :                                           // Nothing more to do
            break;
        case PortB :
            result = get_PortB(m_byte[PortA] & 0x0F);
            break;
        case PortC :
            result = get_PortC(result);
            break;
        case ControlPort :                                     // Nothing more to do
            break;
        }
    m_byte[p_addr] = result;
    return result;
}

void Ppia::set_PortA(byte p_byte)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_PortA(" << Hex(p_byte) << ")");
    static const VDGMode mode_mapping[16] =
        {
            VDG_MODE0,  // 0x0
            VDG_MODE1A, // 0x1
            VDG_MODE0,  // 0x2
            VDG_MODE1,  // 0x3
            VDG_MODE0,  // 0x4
            VDG_MODE2A, // 0x5
            VDG_MODE0,  // 0x6
            VDG_MODE2,  // 0x7
            VDG_MODE0,  // 0x8
            VDG_MODE3A, // 0x9
            VDG_MODE0,  // 0xA
            VDG_MODE3,  // 0xB
            VDG_MODE0,  // 0xC
            VDG_MODE4A, // 0xD
            VDG_MODE0,  // 0xE
            VDG_MODE4,  // 0xF
        };
    m_io.m_vdg_mode = mode_mapping[p_byte >> 4 & 0x0F];
}

void Ppia::set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    p_addr &= 3;                  // Only interested in least significant 2 bits
    switch (p_addr)
        {
        case PortA :
            set_PortA(p_byte);
            break;
        case PortB :                                               // Do Nothing
            break;
        case PortC :
            p_byte &= 0x0F;                              // Remember the outputs
            p_byte |= (m_byte[PortC] & 0xF0);            // Remember last inputs
            break;
        case ControlPort :
            switch (p_byte)
                {
                case 0x8A:                      // Normal condition - Do Nothing
                    break;
                case 0x04:
                case 0x05:                                         // Error bell
                    // TODO: beep (#7)
                    break;
                default:                                     // TODO: Unexpected
                    assert (false);
                }
            break;
        }
    m_byte[p_addr] = p_byte;                            // Remember byte written
}

void Ppia::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].reset()");
    // reset the IO Model Inputs first
    m_io.m_pressed_key      = KBD_NO_KEYPRESS;
    m_io.m_is_shift_pressed = false;
    m_io.m_is_ctrl_pressed  = false;
    m_io.m_is_rept_pressed  = false;

    // reset "Control" port first
    m_byte[ControlPort] = 0x8A;
    // reset output ports then input ports
    m_byte[PortA] = 0x00;
    m_byte[PortC] = get_PortC(0xFF);
    m_byte[PortB] = get_PortB(m_byte[PortA] & 0x0F);

    // Finally reset the IO Model outputs
    set_PortA(m_byte[PortA]);
}

std::ostream &operator<<(std::ostream &p_s, const Ppia::Configurator &p_cfg)
{
	p_s << static_cast<const Device::Configurator &>(p_cfg) << ", base=" << Hex(p_cfg.base()) << ", memory_size=" << Hex(p_cfg.memory_size());
	return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const VDGMode p_mode)
{
    if (p_mode >= 0 && p_mode < VDG_LAST) {
        static const char *name[VDG_LAST] = {
            "mode 0",
            "mode 1a",
            "mode 1",
            "mode 2a",
            "mode 2",
            "mode 3a",
            "mode 3",
            "mode 4a",
            "mode 4"
        };
        p_s << name[p_mode];
    }
    else
        p_s << int(p_mode);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Ppia &p_ppia)
{
    p_s << static_cast<const Device &>(p_ppia)
        <<   "PortA:"   << Hex(p_ppia.m_byte[PortA])
        << ", PortB:"   << Hex(p_ppia.m_byte[PortB])
        << ", PortC:"   << Hex(p_ppia.m_byte[PortC])
        << ", Control:" << Hex(p_ppia.m_byte[ControlPort])
        << ", Refresh:" << p_ppia.m_io.m_is_vdg_refresh
        << ", Key:"     << p_ppia.m_io.m_pressed_key
        << ", Shift:"   << p_ppia.m_io.m_is_shift_pressed
        << ", Ctrl:"    << p_ppia.m_io.m_is_ctrl_pressed
        << ", Alt:"     << p_ppia.m_io.m_is_rept_pressed;
    return p_s;
}


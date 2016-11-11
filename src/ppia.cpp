// ppia.cpp
// Copyright 2016 Steve Palmer

#include <cassert>
#include <iomanip>

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
///****************************************************************************
/// Port B: Inputs (all active low)
///
/// b5..b0 - Keyboard Column response
/// b6     - Control Key Press return
/// b7     - Shift Key Press return
///
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
/// All other inputs are module simply as a 2 state flip-flop.
/// Outputs are ignored.
///
/// TODO: More complete model (#9)
///****************************************************************************
/// Control Port
///
/// Acorn Atom only uses code 0x8A which configures Ports A, B & C appropriately
///****************************************************************************

Ppia::Ppia(const Configurator &p_cfgr)
    : Memory(p_cfgr)
    , m_register( { 0, 0, 0, 0 } )
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::Ppia(" << p_cfgr << ")");

    key_mapping[AtomKeyboardInterface::SPACE              ] = Scanpair(9, 1<<0);
    key_mapping[AtomKeyboardInterface::LEFTBRACKET        ] = Scanpair(8, 1<<0);
    key_mapping[AtomKeyboardInterface::BACKSLASH          ] = Scanpair(7, 1<<0);
    key_mapping[AtomKeyboardInterface::RIGHTBRACKET       ] = Scanpair(6, 1<<0);
    key_mapping[AtomKeyboardInterface::CIRCUMFLEX         ] = Scanpair(5, 1<<0);
    key_mapping[AtomKeyboardInterface::LOCK               ] = Scanpair(4, 1<<0);
    key_mapping[AtomKeyboardInterface::LEFT_RIGHT         ] = Scanpair(3, 1<<0);
    key_mapping[AtomKeyboardInterface::UP_DOWN            ] = Scanpair(2, 1<<0);
    key_mapping[AtomKeyboardInterface::SPARE1             ] = Scanpair(1, 1<<0);
    key_mapping[AtomKeyboardInterface::SPARE2             ] = Scanpair(0, 1<<0);
    key_mapping[AtomKeyboardInterface::SPARE3             ] = Scanpair(9, 1<<1);
    key_mapping[AtomKeyboardInterface::SPARE4             ] = Scanpair(8, 1<<1);
    key_mapping[AtomKeyboardInterface::SPARE5             ] = Scanpair(7, 1<<1);
    key_mapping[AtomKeyboardInterface::RETURN             ] = Scanpair(6, 1<<1);
    key_mapping[AtomKeyboardInterface::COPY               ] = Scanpair(5, 1<<1);
    key_mapping[AtomKeyboardInterface::DELETE             ] = Scanpair(4, 1<<1);
    key_mapping[AtomKeyboardInterface::_0                 ] = Scanpair(3, 1<<1);
    key_mapping[AtomKeyboardInterface::_1_EXCLAMATION     ] = Scanpair(2, 1<<1);
    key_mapping[AtomKeyboardInterface::_2_DOUBLEQUOTE     ] = Scanpair(1, 1<<1);
    key_mapping[AtomKeyboardInterface::_3_NUMBER          ] = Scanpair(0, 1<<1);
    key_mapping[AtomKeyboardInterface::_4_DOLLAR          ] = Scanpair(9, 1<<2);
    key_mapping[AtomKeyboardInterface::_5_PERCENT         ] = Scanpair(8, 1<<2);
    key_mapping[AtomKeyboardInterface::_6_AMPERSAND       ] = Scanpair(7, 1<<2);
    key_mapping[AtomKeyboardInterface::_7_SINGLEQUOTE     ] = Scanpair(6, 1<<2);
    key_mapping[AtomKeyboardInterface::_8_LEFTPARENTHESIS ] = Scanpair(5, 1<<2);
    key_mapping[AtomKeyboardInterface::_9_RIGHTPARENTHESIS] = Scanpair(4, 1<<2);
    key_mapping[AtomKeyboardInterface::COLON_ASTERISK     ] = Scanpair(3, 1<<2);
    key_mapping[AtomKeyboardInterface::SEMICOLON_PLUS     ] = Scanpair(2, 1<<2);
    key_mapping[AtomKeyboardInterface::COMMA_LESSTHAN     ] = Scanpair(1, 1<<2);
    key_mapping[AtomKeyboardInterface::MINUS_EQUALS       ] = Scanpair(0, 1<<2);
    key_mapping[AtomKeyboardInterface::PERIOD_GREATERTHAN ] = Scanpair(9, 1<<3);
    key_mapping[AtomKeyboardInterface::SLASH_QUESTION     ] = Scanpair(8, 1<<3);
    key_mapping[AtomKeyboardInterface::AT                 ] = Scanpair(7, 1<<3);
    key_mapping[AtomKeyboardInterface::A                  ] = Scanpair(6, 1<<3);
    key_mapping[AtomKeyboardInterface::B                  ] = Scanpair(5, 1<<3);
    key_mapping[AtomKeyboardInterface::C                  ] = Scanpair(4, 1<<3);
    key_mapping[AtomKeyboardInterface::D                  ] = Scanpair(3, 1<<3);
    key_mapping[AtomKeyboardInterface::E                  ] = Scanpair(2, 1<<3);
    key_mapping[AtomKeyboardInterface::F                  ] = Scanpair(1, 1<<3);
    key_mapping[AtomKeyboardInterface::G                  ] = Scanpair(0, 1<<3);
    key_mapping[AtomKeyboardInterface::H                  ] = Scanpair(9, 1<<4);
    key_mapping[AtomKeyboardInterface::I                  ] = Scanpair(8, 1<<4);
    key_mapping[AtomKeyboardInterface::J                  ] = Scanpair(7, 1<<4);
    key_mapping[AtomKeyboardInterface::K                  ] = Scanpair(6, 1<<4);
    key_mapping[AtomKeyboardInterface::L                  ] = Scanpair(5, 1<<4);
    key_mapping[AtomKeyboardInterface::M                  ] = Scanpair(4, 1<<4);
    key_mapping[AtomKeyboardInterface::N                  ] = Scanpair(3, 1<<4);
    key_mapping[AtomKeyboardInterface::O                  ] = Scanpair(2, 1<<4);
    key_mapping[AtomKeyboardInterface::P                  ] = Scanpair(1, 1<<4);
    key_mapping[AtomKeyboardInterface::Q                  ] = Scanpair(0, 1<<4);
    key_mapping[AtomKeyboardInterface::R                  ] = Scanpair(9, 1<<5);
    key_mapping[AtomKeyboardInterface::S                  ] = Scanpair(8, 1<<5);
    key_mapping[AtomKeyboardInterface::T                  ] = Scanpair(7, 1<<5);
    key_mapping[AtomKeyboardInterface::U                  ] = Scanpair(6, 1<<5);
    key_mapping[AtomKeyboardInterface::V                  ] = Scanpair(5, 1<<5);
    key_mapping[AtomKeyboardInterface::W                  ] = Scanpair(4, 1<<5);
    key_mapping[AtomKeyboardInterface::X                  ] = Scanpair(3, 1<<5);
    key_mapping[AtomKeyboardInterface::Y                  ] = Scanpair(2, 1<<5);
    key_mapping[AtomKeyboardInterface::Z                  ] = Scanpair(1, 1<<5);
    key_mapping[AtomKeyboardInterface::ESCAPE             ] = Scanpair(0, 1<<5);

    key_mapping[AtomKeyboardInterface::CTRL               ] = Scanpair(10, 1<<6);
    key_mapping[AtomKeyboardInterface::SHIFT              ] = Scanpair(10, 1<<7);
    key_mapping[AtomKeyboardInterface::REPT               ] = Scanpair(11, 1<<6);

    reset();
}

Ppia::~Ppia()
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::~Ppia([" << id() << "])");
    key_mapping.clear();
}

byte Ppia::get_PortB(int p_row)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::get_PortB(" << p_row << ")");
    m_keyboard.mutex.lock();
    const byte result(p_row < 10 ? m_keyboard.row[p_row] & m_keyboard.row[11] : m_keyboard.row[11]);
    m_keyboard.mutex.unlock();
    return result;
}

byte Ppia::get_PortC(byte p_previous)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::get_PortC(" << Hex(p_previous) << ")");
    byte result(p_previous & 0xBF);          // clear REPT and FLYBACK signals
    m_keyboard.mutex.lock();
    result &= m_keyboard.row[12];
    m_keyboard.mutex.unlock();
    result ^= 0x80;                                // Flip Terminal Refresh bit
    result ^= 0x30;                                      // Flip Tape input bits
    return result;
}

byte Ppia::get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::_get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    assert (p_addr < 4);
    byte result(m_register[p_addr]);             // By default, set it to last value
    switch (p_addr)
    {
    case PortA :                                       // Nothing more to do
        break;
    case PortB :
        result = get_PortB(m_register[PortA] & 0x0F);
        break;
    case PortC :
        result = get_PortC(result);
        break;
    case ControlPort :                                 // Nothing more to do
        break;
    }
    m_register[p_addr] = result;
    return result;
}

void Ppia::set_PortA(byte p_byte)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::set_PortA(" << Hex(p_byte) << ")");
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
    m_terminal.mutex.lock();
    m_terminal.vdg_mode = mode_mapping[p_byte >> 4 & 0x0F];
    if (m_terminal.vdg_mode != m_terminal.notified_vdg_mode)
    {
        vdg_mode_notify(m_terminal.vdg_mode);
        m_terminal.notified_vdg_mode = m_terminal.vdg_mode;
    }
    m_terminal.mutex.unlock();
}

void Ppia::_set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::_set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    assert (p_addr < 4);
    switch (p_addr)
    {
    case PortA :
        set_PortA(p_byte);
        break;
    case PortB :
        break;
    case PortC :
        p_byte &= 0x0F;
        p_byte |= (m_register[PortC] & 0xF0);
        break;
    case ControlPort :
        switch (p_byte)
        {
        case 0x8A:
            break;
        case 0x04:
        case 0x05:  // Error bell
            // TODO: beep (#7)
            break;
        default:  // Unexpected
            assert (false);
        }
        break;
    }
    m_register[p_addr] = p_byte;
}

void Ppia::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::reset()");
    // reset the IO Model Inputs first
    m_terminal.mutex.lock();
    m_terminal.notified_vdg_mode = VDG_LAST; // force a notification

    m_keyboard.mutex.lock();
    for (int i = 0; i <12; i++)
        m_keyboard.row[i] = ~0;  // Active Low
    m_keyboard.mutex.unlock();

    // reset "Control" port first
    m_register[ControlPort] = 0x8A;
    // reset output ports then input ports
    m_register[PortA] = 0x00;
    m_register[PortC] = get_PortC(0xFF);
    m_register[PortB] = get_PortB(m_register[PortA] & 0x0F);

    // Finally reset the IO Model outputs
    set_PortA(m_register[PortA]);
    m_terminal.mutex.unlock();
}

TerminalInterface::VDGMode Ppia::vdg_mode() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::vdg_mode()");
    m_terminal.mutex.lock();
    const VDGMode result(m_terminal.vdg_mode);
    m_terminal.mutex.unlock();
    return result;
}

void Ppia::down(Key p_key)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::down(" << p_key << ")");
    m_keyboard.mutex.lock();
    try
    {
        Scanpair &sp = key_mapping[p_key];
        m_keyboard.row[sp.first] &= ~sp.second;
    }
    catch (std::out_of_range e)
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "].Ppia::down: Unknown key: " << p_key);
    }
    m_keyboard.mutex.unlock();
}

void Ppia::up(Key p_key)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::up(" << p_key << ")");
    m_keyboard.mutex.lock();
    try
    {
        Scanpair &sp = key_mapping[p_key];
        m_keyboard.row[sp.first] |= sp.second;
    }
    catch (std::out_of_range e)
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "].Ppia::up: Unknown key: " << p_key);
    }
    m_keyboard.mutex.unlock();
}


void Ppia::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Memory::Configurator::serialize(p_s);
#else
    p_s << "<ppia ";
    Memory::Configurator::serialize(p_s);
    p_s << "/>";
#endif
}

void Ppia::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Memory::serialize(p_s);
#else
    p_s << "Ppia(";
    Memory::serialize(p_s);
    m_terminal.mutex.lock();
    p_s << ", PortA("   << Hex(m_register[PortA]) << ")"
        << ", PortB("   << Hex(m_register[PortB]) << ")"
        << ", PortC("   << Hex(m_register[PortC]) << ")"
        << ", Control(" << Hex(m_register[ControlPort]) << ")";
    m_terminal.mutex.unlock();
    p_s << ")";
#endif
}

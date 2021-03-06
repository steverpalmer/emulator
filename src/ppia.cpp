// ppia.cpp
// Copyright 2016 Steve Palmer

#include <cassert>
#include <iomanip>
#include <cmath>

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
    , m_register{ 0, 0, 0, 0 }
    , m_start( std::chrono::steady_clock::now())
    , key_mapping{
    	{Atom::KeyboardInterface::Key::SPACE,              Scanpair(9, 1<<0)},
		{Atom::KeyboardInterface::Key::LEFTBRACKET,        Scanpair(8, 1<<0)},
		{Atom::KeyboardInterface::Key::BACKSLASH,          Scanpair(7, 1<<0)},
		{Atom::KeyboardInterface::Key::RIGHTBRACKET,       Scanpair(6, 1<<0)},
		{Atom::KeyboardInterface::Key::CIRCUMFLEX,         Scanpair(5, 1<<0)},
		{Atom::KeyboardInterface::Key::LOCK,               Scanpair(4, 1<<0)},
		{Atom::KeyboardInterface::Key::LEFT_RIGHT,         Scanpair(3, 1<<0)},
		{Atom::KeyboardInterface::Key::UP_DOWN,            Scanpair(2, 1<<0)},
		{Atom::KeyboardInterface::Key::SPARE1,             Scanpair(1, 1<<0)},
		{Atom::KeyboardInterface::Key::SPARE2,             Scanpair(0, 1<<0)},
		{Atom::KeyboardInterface::Key::SPARE3,             Scanpair(9, 1<<1)},
		{Atom::KeyboardInterface::Key::SPARE4,             Scanpair(8, 1<<1)},
		{Atom::KeyboardInterface::Key::SPARE5,             Scanpair(7, 1<<1)},
		{Atom::KeyboardInterface::Key::RETURN,             Scanpair(6, 1<<1)},
		{Atom::KeyboardInterface::Key::COPY,               Scanpair(5, 1<<1)},
		{Atom::KeyboardInterface::Key::DELETE,             Scanpair(4, 1<<1)},
		{Atom::KeyboardInterface::Key::_0,                 Scanpair(3, 1<<1)},
		{Atom::KeyboardInterface::Key::_1_EXCLAMATION,     Scanpair(2, 1<<1)},
		{Atom::KeyboardInterface::Key::_2_DOUBLEQUOTE,     Scanpair(1, 1<<1)},
		{Atom::KeyboardInterface::Key::_3_NUMBER,          Scanpair(0, 1<<1)},
		{Atom::KeyboardInterface::Key::_4_DOLLAR,          Scanpair(9, 1<<2)},
		{Atom::KeyboardInterface::Key::_5_PERCENT,         Scanpair(8, 1<<2)},
		{Atom::KeyboardInterface::Key::_6_AMPERSAND,       Scanpair(7, 1<<2)},
		{Atom::KeyboardInterface::Key::_7_SINGLEQUOTE,     Scanpair(6, 1<<2)},
		{Atom::KeyboardInterface::Key::_8_LEFTPARENTHESIS, Scanpair(5, 1<<2)},
		{Atom::KeyboardInterface::Key::_9_RIGHTPARENTHESIS,Scanpair(4, 1<<2)},
		{Atom::KeyboardInterface::Key::COLON_ASTERISK,     Scanpair(3, 1<<2)},
		{Atom::KeyboardInterface::Key::SEMICOLON_PLUS,     Scanpair(2, 1<<2)},
		{Atom::KeyboardInterface::Key::COMMA_LESSTHAN,     Scanpair(1, 1<<2)},
		{Atom::KeyboardInterface::Key::MINUS_EQUALS,       Scanpair(0, 1<<2)},
		{Atom::KeyboardInterface::Key::PERIOD_GREATERTHAN, Scanpair(9, 1<<3)},
		{Atom::KeyboardInterface::Key::SLASH_QUESTION,     Scanpair(8, 1<<3)},
		{Atom::KeyboardInterface::Key::AT,                 Scanpair(7, 1<<3)},
		{Atom::KeyboardInterface::Key::A,                  Scanpair(6, 1<<3)},
		{Atom::KeyboardInterface::Key::B,                  Scanpair(5, 1<<3)},
		{Atom::KeyboardInterface::Key::C,                  Scanpair(4, 1<<3)},
		{Atom::KeyboardInterface::Key::D,                  Scanpair(3, 1<<3)},
		{Atom::KeyboardInterface::Key::E,                  Scanpair(2, 1<<3)},
		{Atom::KeyboardInterface::Key::F,                  Scanpair(1, 1<<3)},
		{Atom::KeyboardInterface::Key::G,                  Scanpair(0, 1<<3)},
		{Atom::KeyboardInterface::Key::H,                  Scanpair(9, 1<<4)},
		{Atom::KeyboardInterface::Key::I,                  Scanpair(8, 1<<4)},
		{Atom::KeyboardInterface::Key::J,                  Scanpair(7, 1<<4)},
		{Atom::KeyboardInterface::Key::K,                  Scanpair(6, 1<<4)},
		{Atom::KeyboardInterface::Key::L,                  Scanpair(5, 1<<4)},
		{Atom::KeyboardInterface::Key::M,                  Scanpair(4, 1<<4)},
		{Atom::KeyboardInterface::Key::N,                  Scanpair(3, 1<<4)},
		{Atom::KeyboardInterface::Key::O,                  Scanpair(2, 1<<4)},
		{Atom::KeyboardInterface::Key::P,                  Scanpair(1, 1<<4)},
		{Atom::KeyboardInterface::Key::Q,                  Scanpair(0, 1<<4)},
		{Atom::KeyboardInterface::Key::R,                  Scanpair(9, 1<<5)},
		{Atom::KeyboardInterface::Key::S,                  Scanpair(8, 1<<5)},
		{Atom::KeyboardInterface::Key::T,                  Scanpair(7, 1<<5)},
		{Atom::KeyboardInterface::Key::U,                  Scanpair(6, 1<<5)},
		{Atom::KeyboardInterface::Key::V,                  Scanpair(5, 1<<5)},
		{Atom::KeyboardInterface::Key::W,                  Scanpair(4, 1<<5)},
		{Atom::KeyboardInterface::Key::X,                  Scanpair(3, 1<<5)},
		{Atom::KeyboardInterface::Key::Y,                  Scanpair(2, 1<<5)},
		{Atom::KeyboardInterface::Key::Z,                  Scanpair(1, 1<<5)},
		{Atom::KeyboardInterface::Key::ESCAPE,             Scanpair(0, 1<<5)},

		{Atom::KeyboardInterface::Key::CTRL,               Scanpair(10, 1<<6)},
		{Atom::KeyboardInterface::Key::LEFT_SHIFT,         Scanpair(10, 1<<7)},
		{Atom::KeyboardInterface::Key::RIGHT_SHIFT,        Scanpair(10, 1<<7)},
		{Atom::KeyboardInterface::Key::REPT,               Scanpair(11, 1<<6)},
    }
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::Ppia(" << p_cfgr << ")");
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
    std::lock_guard<std::recursive_mutex> lock(m_keyboard.mutex);
    const byte result(p_row < 10 ? m_keyboard.row[p_row] & m_keyboard.row[10] : m_keyboard.row[10]);
    return result;
}

byte Ppia::get_PortC(byte p_previous)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::get_PortC(" << Hex(p_previous) << ")");
    byte result(p_previous | 0xD0);  // clear REPT, Flyback and 2.4KHz signal
    {
        std::lock_guard<std::recursive_mutex> lock(m_keyboard.mutex);
        result &= m_keyboard.row[11];
    }
    const std::chrono::duration<double> delay = std::chrono::steady_clock::now() - m_start;
    LOG4CXX_DEBUG(cpptrace_log(), "[" << id() << "].Ppia::get_PortC() delay: " << delay.count());
    {
        const double scaled_delay(delay.count() * 60.0);
        const double fractional_part(scaled_delay - std::floor(scaled_delay));
        if (fractional_part < 0.05)
            result &= ~0x80;
    }
    {
        const double scaled_delay(delay.count() * 2400.0);
        const double fractional_part(scaled_delay - std::floor(scaled_delay));
        if (fractional_part < 0.5)
            result &= ~0x10;
    }
    result ^= 0x20;
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
            VDGMode::MODE0,  // 0x0
			VDGMode::MODE1A, // 0x1
			VDGMode::MODE0,  // 0x2
			VDGMode::MODE1,  // 0x3
			VDGMode::MODE0,  // 0x4
			VDGMode::MODE2A, // 0x5
			VDGMode::MODE0,  // 0x6
			VDGMode::MODE2,  // 0x7
			VDGMode::MODE0,  // 0x8
			VDGMode::MODE3A, // 0x9
			VDGMode::MODE0,  // 0xA
			VDGMode::MODE3,  // 0xB
			VDGMode::MODE0,  // 0xC
			VDGMode::MODE4A, // 0xD
			VDGMode::MODE0,  // 0xE
			VDGMode::MODE4,  // 0xF
        };
    const VDGMode new_mode(mode_mapping[p_byte >> 4 & 0x0F]);
    bool need_to_notify;
    {
        std::lock_guard<std::recursive_mutex> lock(m_monitor.mutex);
        need_to_notify = (m_monitor.vdg_mode != new_mode);
        m_monitor.vdg_mode = new_mode;
    }
    if (need_to_notify)
        vdg_mode_notify(new_mode);
}

void Ppia::unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::unobserved_set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
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
    {
        std::lock_guard<std::recursive_mutex> lock(m_keyboard.mutex);
        for (int i = 0; i <12; i++)
            m_keyboard.row[i] = ~0;  // Active Low
    }
    {
        std::lock_guard<std::recursive_mutex> lock(m_monitor.mutex);
        m_monitor.vdg_mode = VDGMode::UNKNOWN; // force a notification
    }

    // reset "Control" port first
    m_register[ControlPort] = 0x8A;
    // reset output ports then input ports
    m_register[PortA] = 0x00;
    m_register[PortC] = get_PortC(0xFF);
    m_register[PortB] = get_PortB(m_register[PortA] & 0x0F);

    // Finally reset the IO Model outputs
    set_PortA(m_register[PortA]);
}

Atom::MonitorInterface::VDGMode Ppia::vdg_mode() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::vdg_mode()");
    std::lock_guard<std::recursive_mutex> lock(m_monitor.mutex);
    const VDGMode result(m_monitor.vdg_mode);
    return result;
}

void Ppia::down(Key p_key)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::down(...)");
    std::lock_guard<std::recursive_mutex> lock(m_keyboard.mutex);
    try
    {
        Scanpair &sp = key_mapping[p_key];
        m_keyboard.row[sp.first] &= ~sp.second;
    }
    catch (std::out_of_range &e)
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "].Ppia::down: Unknown key");
    }
}

void Ppia::up(Key p_key)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::up(...)");
    std::lock_guard<std::recursive_mutex> lock(m_keyboard.mutex);
    try
    {
        Scanpair &sp = key_mapping[p_key];
        m_keyboard.row[sp.first] |= sp.second;
    }
    catch (std::out_of_range &e)
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "].Ppia::up: Unknown key");
    }
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
    {
        std::lock_guard<std::recursive_mutex> lock(m_monitor.mutex);
        p_s << ", PortA("   << Hex(m_register[PortA]) << ")"
            << ", PortB("   << Hex(m_register[PortB]) << ")"
            << ", PortC("   << Hex(m_register[PortC]) << ")"
            << ", Control(" << Hex(m_register[ControlPort]) << ")";
    }
    p_s << ")";
#endif
}

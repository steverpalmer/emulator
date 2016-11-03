// ppia.cpp

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

#define SHIFT   byte(0x80)
#define CONTROL byte(0x40)
#define SCANCODE(CLMN, SHFT_CTRL) (byte(~((1 << (CLMN)) | (SHFT_CTRL))))

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

Ppia::Ppia(const Configurator &p_cfgr)
    : Memory(p_cfgr)
    , m_register( { 0, 0, 0, 0 } )
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::Ppia(" << p_cfgr << ")");
    // m_terminal.mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    key_mapping[0x00] = new Scanpair(7, SCANCODE(3, CONTROL)); // <Ctrl> @ (nul)
    key_mapping[0x01] = new Scanpair(6, SCANCODE(3, CONTROL)); // <Ctrl> A (soh)
    key_mapping[0x02] = new Scanpair(5, SCANCODE(3, CONTROL)); // <Ctrl> B (stx) start printer
    key_mapping[0x03] = new Scanpair(4, SCANCODE(3, CONTROL)); // <Ctrl> C (etx) end printer
    key_mapping[0x04] = new Scanpair(3, SCANCODE(3, CONTROL)); // <Ctrl> D (eot)
    key_mapping[0x05] = new Scanpair(2, SCANCODE(3, CONTROL)); // <Ctrl> E (enq)
    key_mapping[0x06] = new Scanpair(1, SCANCODE(3, CONTROL)); // <Ctrl> F (ack) start screen
    key_mapping[0x07] = new Scanpair(0, SCANCODE(3, CONTROL)); // <Ctrl> G (bel) bleep
    key_mapping[0x08] = new Scanpair(9, SCANCODE(4, CONTROL)); // <Ctrl> H (bs)  backspace
    key_mapping[0x09] = new Scanpair(8, SCANCODE(4, CONTROL)); // <Ctrl> I (tab) horizontal tab
    key_mapping[0x0A] = new Scanpair(7, SCANCODE(4, CONTROL)); // <Ctrl> J (lf)  linefeed
    key_mapping[0x0B] = new Scanpair(6, SCANCODE(4, CONTROL)); // <Ctrl> K (vt)  vertical tab
    key_mapping[0x0C] = new Scanpair(5, SCANCODE(4, CONTROL)); // <Ctrl> L (ff)  formfeed
    key_mapping[0x0D] = new Scanpair(4, SCANCODE(4, CONTROL)); // <Ctrl> M (cr)  return
    key_mapping[0x0E] = new Scanpair(3, SCANCODE(4, CONTROL)); // <Ctrl> N (so)  page mode on
    key_mapping[0x0F] = new Scanpair(2, SCANCODE(4, CONTROL)); // <Ctrl> O (si)  page mode off
    key_mapping[0x10] = new Scanpair(1, SCANCODE(4, CONTROL)); // <Ctrl> P (dle)
    key_mapping[0x11] = new Scanpair(0, SCANCODE(4, CONTROL)); // <Ctrl> Q (dc1)
    key_mapping[0x12] = new Scanpair(9, SCANCODE(5, CONTROL)); // <Ctrl> R (dc2)
    key_mapping[0x13] = new Scanpair(8, SCANCODE(5, CONTROL)); // <Ctrl> S (dc3)
    key_mapping[0x14] = new Scanpair(7, SCANCODE(5, CONTROL)); // <Ctrl> T (dc4)
    key_mapping[0x15] = new Scanpair(6, SCANCODE(5, CONTROL)); // <Ctrl> U (nak) end screen
    key_mapping[0x16] = new Scanpair(5, SCANCODE(5, CONTROL)); // <Ctrl> V (syn)
    key_mapping[0x17] = new Scanpair(4, SCANCODE(5, CONTROL)); // <Ctrl> W (etb)
    key_mapping[0x18] = new Scanpair(3, SCANCODE(5, CONTROL)); // <Ctrl> X (can) cancel
    key_mapping[0x19] = new Scanpair(2, SCANCODE(5, CONTROL)); // <Ctrl> Y (em)
    key_mapping[0x1A] = new Scanpair(1, SCANCODE(5, CONTROL)); // <Ctrl> Z (sub)
    key_mapping[0x1B] = new Scanpair(0, SCANCODE(5, CONTROL)); // <Ctrl> [ (esc) escape
    key_mapping[0x1C] = new Scanpair(7, SCANCODE(0, CONTROL)); // <Ctrl> \ (fs)
    key_mapping[0x1D] = new Scanpair(6, SCANCODE(0, CONTROL)); // <Ctrl> ] (gs)
    key_mapping[0x1E] = new Scanpair(5, SCANCODE(0, CONTROL)); // <Ctrl> ^ (rs) home cursor
    // 0x1F <Ctrl> _ (us) is not possible
    key_mapping[' ']  = new Scanpair(9, SCANCODE(0, 0));
    key_mapping['!']  = new Scanpair(2, SCANCODE(1, SHIFT));
    key_mapping['"']  = new Scanpair(1, SCANCODE(1, SHIFT));
    key_mapping['#']  = new Scanpair(0, SCANCODE(1, SHIFT));
    key_mapping['$']  = new Scanpair(9, SCANCODE(2, SHIFT));
    key_mapping['%']  = new Scanpair(8, SCANCODE(2, SHIFT));
    key_mapping['&']  = new Scanpair(7, SCANCODE(2, SHIFT));
    key_mapping['\''] = new Scanpair(6, SCANCODE(2, SHIFT));
    key_mapping['(']  = new Scanpair(5, SCANCODE(2, SHIFT));
    key_mapping[')']  = new Scanpair(4, SCANCODE(2, SHIFT));
    key_mapping['*']  = new Scanpair(3, SCANCODE(2, SHIFT));
    key_mapping['+']  = new Scanpair(2, SCANCODE(2, SHIFT));
    key_mapping[',']  = new Scanpair(1, SCANCODE(2, 0));
    key_mapping['-']  = new Scanpair(0, SCANCODE(2, 0));
    key_mapping['.']  = new Scanpair(9, SCANCODE(3, 0));
    key_mapping['/']  = new Scanpair(8, SCANCODE(3, 0));
    key_mapping['0']  = new Scanpair(3, SCANCODE(1, 0));
    key_mapping['1']  = new Scanpair(2, SCANCODE(1, 0));
    key_mapping['2']  = new Scanpair(1, SCANCODE(1, 0));
    key_mapping['3']  = new Scanpair(0, SCANCODE(1, 0));
    key_mapping['4']  = new Scanpair(9, SCANCODE(2, 0));
    key_mapping['5']  = new Scanpair(8, SCANCODE(2, 0));
    key_mapping['6']  = new Scanpair(7, SCANCODE(2, 0));
    key_mapping['7']  = new Scanpair(6, SCANCODE(2, 0));
    key_mapping['8']  = new Scanpair(5, SCANCODE(2, 0));
    key_mapping['9']  = new Scanpair(4, SCANCODE(2, 0));
    key_mapping[':']  = new Scanpair(3, SCANCODE(2, 0));
    key_mapping[';']  = new Scanpair(2, SCANCODE(2, 0));
    key_mapping['<']  = new Scanpair(1, SCANCODE(2, SHIFT));
    key_mapping['=']  = new Scanpair(0, SCANCODE(2, SHIFT));
    key_mapping['>']  = new Scanpair(9, SCANCODE(3, SHIFT));
    key_mapping['?']  = new Scanpair(8, SCANCODE(3, SHIFT));
    key_mapping['@']  = new Scanpair(7, SCANCODE(3, 0));
    key_mapping['A']  = new Scanpair(6, SCANCODE(3, 0));
    key_mapping['B']  = new Scanpair(5, SCANCODE(3, 0));
    key_mapping['C']  = new Scanpair(4, SCANCODE(3, 0));
    key_mapping['D']  = new Scanpair(3, SCANCODE(3, 0));
    key_mapping['E']  = new Scanpair(2, SCANCODE(3, 0));
    key_mapping['F']  = new Scanpair(1, SCANCODE(3, 0));
    key_mapping['G']  = new Scanpair(0, SCANCODE(3, 0));
    key_mapping['H']  = new Scanpair(9, SCANCODE(4, 0));
    key_mapping['I']  = new Scanpair(8, SCANCODE(4, 0));
    key_mapping['J']  = new Scanpair(7, SCANCODE(4, 0));
    key_mapping['K']  = new Scanpair(6, SCANCODE(4, 0));
    key_mapping['L']  = new Scanpair(5, SCANCODE(4, 0));
    key_mapping['M']  = new Scanpair(4, SCANCODE(4, 0));
    key_mapping['N']  = new Scanpair(3, SCANCODE(4, 0));
    key_mapping['O']  = new Scanpair(2, SCANCODE(4, 0));
    key_mapping['P']  = new Scanpair(1, SCANCODE(4, 0));
    key_mapping['Q']  = new Scanpair(0, SCANCODE(4, 0));
    key_mapping['R']  = new Scanpair(9, SCANCODE(5, 0));
    key_mapping['S']  = new Scanpair(8, SCANCODE(5, 0));
    key_mapping['T']  = new Scanpair(7, SCANCODE(5, 0));
    key_mapping['U']  = new Scanpair(6, SCANCODE(5, 0));
    key_mapping['V']  = new Scanpair(5, SCANCODE(5, 0));
    key_mapping['W']  = new Scanpair(4, SCANCODE(5, 0));
    key_mapping['X']  = new Scanpair(3, SCANCODE(5, 0));
    key_mapping['Y']  = new Scanpair(2, SCANCODE(5, 0));
    key_mapping['Z']  = new Scanpair(1, SCANCODE(5, 0));
    key_mapping['[']  = new Scanpair(8, SCANCODE(1, 0));
    key_mapping['\\'] = new Scanpair(7, SCANCODE(1, 0));
    key_mapping[']']  = new Scanpair(6, SCANCODE(1, 0));
    key_mapping['^']  = new Scanpair(5, SCANCODE(1, 0));
    // '_' is not possible
    // '`' is not possible
    key_mapping['a']  = new Scanpair(6, SCANCODE(3, SHIFT));
    key_mapping['b']  = new Scanpair(5, SCANCODE(3, SHIFT));
    key_mapping['c']  = new Scanpair(4, SCANCODE(3, SHIFT));
    key_mapping['d']  = new Scanpair(3, SCANCODE(3, SHIFT));
    key_mapping['e']  = new Scanpair(2, SCANCODE(3, SHIFT));
    key_mapping['f']  = new Scanpair(1, SCANCODE(3, SHIFT));
    key_mapping['g']  = new Scanpair(0, SCANCODE(3, SHIFT));
    key_mapping['h']  = new Scanpair(9, SCANCODE(4, SHIFT));
    key_mapping['i']  = new Scanpair(8, SCANCODE(4, SHIFT));
    key_mapping['j']  = new Scanpair(7, SCANCODE(4, SHIFT));
    key_mapping['k']  = new Scanpair(6, SCANCODE(4, SHIFT));
    key_mapping['l']  = new Scanpair(5, SCANCODE(4, SHIFT));
    key_mapping['m']  = new Scanpair(4, SCANCODE(4, SHIFT));
    key_mapping['n']  = new Scanpair(3, SCANCODE(4, SHIFT));
    key_mapping['o']  = new Scanpair(2, SCANCODE(4, SHIFT));
    key_mapping['p']  = new Scanpair(1, SCANCODE(4, SHIFT));
    key_mapping['q']  = new Scanpair(0, SCANCODE(4, SHIFT));
    key_mapping['r']  = new Scanpair(9, SCANCODE(5, SHIFT));
    key_mapping['s']  = new Scanpair(8, SCANCODE(5, SHIFT));
    key_mapping['t']  = new Scanpair(7, SCANCODE(5, SHIFT));
    key_mapping['u']  = new Scanpair(6, SCANCODE(5, SHIFT));
    key_mapping['v']  = new Scanpair(5, SCANCODE(5, SHIFT));
    key_mapping['w']  = new Scanpair(4, SCANCODE(5, SHIFT));
    key_mapping['x']  = new Scanpair(3, SCANCODE(5, SHIFT));
    key_mapping['y']  = new Scanpair(2, SCANCODE(5, SHIFT));
    key_mapping['z']  = new Scanpair(1, SCANCODE(5, SHIFT));
    // '{' '|' '}' '~' are not possible
    key_mapping[0x7F] = new Scanpair(4, SCANCODE(1, 0));

    key_mapping[KBD_LEFT]  = new Scanpair(3, SCANCODE(0, SHIFT));
    key_mapping[KBD_UP]    = new Scanpair(2, SCANCODE(0, 0));
    key_mapping[KBD_RIGHT] = new Scanpair(3, SCANCODE(0, 0));
    key_mapping[KBD_DOWN]  = new Scanpair(2, SCANCODE(0, SHIFT));
    key_mapping[KBD_LOCK]  = new Scanpair(4, SCANCODE(0, 0));
    key_mapping[KBD_COPY]  = new Scanpair(5, SCANCODE(1, 0));

    key_mapping[KBD_NO_KEYPRESS] = 0;
    reset();
}

Ppia::~Ppia()
{
    LOG4CXX_INFO(cpptrace_log(), "Ppia::~Ppia([" << id() << "])");
    for (auto &sp : key_mapping)
        delete sp.second;
    key_mapping.clear();
}

byte Ppia::get_PortB(int p_row)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::get_PortB(" << p_row << ")");
    byte result(0xFF); // Active Low, so start with all high
    m_terminal.mutex.lock();
    try
    {
        auto sp = key_mapping.at(m_terminal.pressed_key);
        if (sp)
        {
            result = sp->second;
            if (p_row != sp->first)
                result |= ~(SHIFT | CONTROL);
        }
    }
    catch (std::out_of_range &e)
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "] unexpected key: " << m_terminal.pressed_key);
    }
    m_terminal.mutex.unlock();
    return result;
}

byte Ppia::get_PortC(byte p_previous)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::get_PortC(" << Hex(p_previous) << ")");
    byte result(p_previous & 0xBF);          // clear REPT and FLYBACK signals
    m_terminal.mutex.lock();
    if (!m_terminal.repeat)
        result |= 0x40;
    result ^= 0x80;                                // Flip Terminal Refresh bit
    result ^= 0x30;                                      // Flip Tape input bits
    m_terminal.mutex.unlock();
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
    m_terminal.pressed_key       = KBD_NO_KEYPRESS;
    m_terminal.repeat            = false;

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

void Ppia::set_keypress(gunichar p_key, bool p_repeat)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Ppia::set_keypress(" << p_key << ")");
    if (key_mapping.find(p_key) == key_mapping.end())
    {
        LOG4CXX_WARN(cpptrace_log(), "[" << id() << "].Ppia::set_keypress: Unknown key: " << p_key);
    }
    else
    {
        m_terminal.mutex.lock();
        m_terminal.pressed_key = p_key;
        m_terminal.repeat      = p_repeat;
        m_terminal.mutex.unlock();
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
    m_terminal.mutex.lock();
    p_s << ", PortA("   << Hex(m_register[PortA]) << ")"
        << ", PortB("   << Hex(m_register[PortB]) << ")"
        << ", PortC("   << Hex(m_register[PortC]) << ")"
        << ", Control(" << Hex(m_register[ControlPort]) << ")"
        << ", Key("     << m_terminal.pressed_key << ")";
    m_terminal.mutex.unlock();
    p_s << ")";
#endif
}

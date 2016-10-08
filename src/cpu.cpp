// cpu.cpp

#include <cassert>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include <log4cxx/logger.h>
#include <glibmm/ustring.h>

#include "cpu.hpp"

#if 0
#define INST_CTRACE_LOG(...) CTRACE_LOG(m_6502.m_ctracelog, __VA_ARGS__)
#define JUMP_TRACE_LOGFORMAT "%s: %04X -> %04X"
#else
#define INST_CTRACE_LOG(...) {}
#define JUMP_TRACE_LOGFORMAT ""
#endif

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".cpu.cpp"));
    return result;
}

/// Core

#define INFINITE_STEPS_TO_GO static_cast<unsigned int>(-1)

Core::Core(const Configurator &p_cfg)
    : Part(p_cfg)
    , m_thread(pthread_self())
    , m_steps_to_go(0)
    , m_cycles(0)
{
    LOG4CXX_INFO(cpptrace_log(), "Core::Core(" << p_cfg << ")");
}

Core::~Core()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].~Core()");
    if (!pthread_equal(m_thread, pthread_self())) {
        pthread_cancel(m_thread);
        pthread_join(m_thread, 0); // Wait for thread to terminate
        m_thread = pthread_self();
    }
}

// Crude implementation of "core_loop" with a busy wait
void *loop(void *p)
{
    Core &c(*static_cast<Core *>(p));
    LOG4CXX_INFO(cpptrace_log(), "[" << c.id() << "].loop() started");
    for(;;) {
        if (c.m_steps_to_go) {
            c.single_step();
            if (c.m_steps_to_go != INFINITE_STEPS_TO_GO)
                c.m_steps_to_go--;
        }
    }
    return 0;
}

void Core::start()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].start()");
    if (pthread_equal(m_thread, pthread_self())) {
        const int rv = pthread_create(&m_thread, 0, loop, this);
        assert (!rv);
    }
}

void Core::step(int p_cnt)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].step(" << p_cnt << ")");
#if EXEC_TRACE
    dump(m_6502tracelog, 1);
#endif
    if (p_cnt < 1)
        p_cnt = 1;
    m_steps_to_go = p_cnt;
    start();
}

void Core::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].resume()");
    m_steps_to_go = INFINITE_STEPS_TO_GO;
    start();
}

void Core::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].pause()");
    m_steps_to_go = 0;
    start();
}

/// Absolute Memory Addresses

#define VECTOR_NMI       ( word(0xFFFA) )
#define VECTOR_RESET     ( word(0xFFFC) )
#define VECTOR_INTERRUPT ( word(0xFFFE) )
#define STACK_ADDRESS    ( word(0x0100) )

const byte CARRY   (1 << 0);
const byte ZERO    (1 << 1);
const byte IRQB    (1 << 2);
const byte DECIMAL (1 << 3);
const byte BREAK   (1 << 4);
const byte UNUSED1 (1 << 5);
const byte OVERFLOW(1 << 6);
const byte NEGATIVE(1 << 7);

static const byte BCD_to_BIN[256] =
{
    // 0x0/8  0x1/9     0x2/A     0x3/B     0x4/C     0x5/D     0x6/E     0x7/F
    byte( 0), byte( 1), byte( 2), byte( 3), byte( 4), byte( 5), byte( 6), byte( 7), // 0x00
    byte( 8), byte( 9), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x08
    byte(10), byte(11), byte(12), byte(13), byte(14), byte(15), byte(16), byte(17), // 0x10
    byte(18), byte(19), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x18
    byte(20), byte(21), byte(22), byte(23), byte(24), byte(25), byte(26), byte(27), // 0x20
    byte(28), byte(29), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x28
    byte(30), byte(31), byte(32), byte(33), byte(34), byte(35), byte(36), byte(37), // 0x30
    byte(38), byte(39), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x38
    byte(40), byte(41), byte(42), byte(43), byte(44), byte(45), byte(46), byte(47), // 0x40
    byte(48), byte(49), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x48
    byte(50), byte(51), byte(52), byte(53), byte(54), byte(55), byte(56), byte(57), // 0x50
    byte(58), byte(59), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x58
    byte(60), byte(61), byte(62), byte(63), byte(64), byte(65), byte(66), byte(67), // 0x60
    byte(68), byte(69), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x68
    byte(70), byte(71), byte(72), byte(73), byte(74), byte(75), byte(76), byte(77), // 0x70
    byte(78), byte(79), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x78
    byte(80), byte(81), byte(82), byte(83), byte(84), byte(85), byte(86), byte(87), // 0x80
    byte(88), byte(89), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x88
    byte(90), byte(91), byte(92), byte(93), byte(94), byte(95), byte(96), byte(97), // 0x90
    byte(98), byte(99), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0x98
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xA0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xA8
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xB0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xB8
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xC0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xC8
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xD0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xD8
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xE0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xE8
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xF0
    byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), byte(-1), // 0xF8
};

static const byte BIN_to_BCD[256] =
{
    // 0x0/8    0x1/9       0x2/A       0x3/B       0x4/C       0x5/D       0x6/E       0x7/F
    byte(0x00), byte(0x01), byte(0x02), byte(0x03), byte(0x04), byte(0x05), byte(0x06), byte(0x07), // 0x00
    byte(0x08), byte(0x09), byte(0x10), byte(0x11), byte(0x12), byte(0x13), byte(0x14), byte(0x15), // 0x08
    byte(0x16), byte(0x17), byte(0x18), byte(0x19), byte(0x20), byte(0x21), byte(0x22), byte(0x23), // 0x10
    byte(0x24), byte(0x25), byte(0x26), byte(0x27), byte(0x28), byte(0x29), byte(0x30), byte(0x31), // 0x18
    byte(0x32), byte(0x33), byte(0x34), byte(0x35), byte(0x36), byte(0x37), byte(0x38), byte(0x39), // 0x20
    byte(0x40), byte(0x41), byte(0x42), byte(0x43), byte(0x44), byte(0x45), byte(0x46), byte(0x47), // 0x28
    byte(0x48), byte(0x49), byte(0x50), byte(0x51), byte(0x52), byte(0x53), byte(0x54), byte(0x55), // 0x30
    byte(0x56), byte(0x57), byte(0x58), byte(0x59), byte(0x60), byte(0x61), byte(0x62), byte(0x63), // 0x38
    byte(0x64), byte(0x65), byte(0x66), byte(0x67), byte(0x68), byte(0x69), byte(0x70), byte(0x71), // 0x40
    byte(0x72), byte(0x73), byte(0x74), byte(0x75), byte(0x76), byte(0x77), byte(0x78), byte(0x79), // 0x48
    byte(0x80), byte(0x81), byte(0x82), byte(0x83), byte(0x84), byte(0x85), byte(0x86), byte(0x87), // 0x50
    byte(0x88), byte(0x89), byte(0x90), byte(0x91), byte(0x92), byte(0x93), byte(0x94), byte(0x95), // 0x58
    byte(0x96), byte(0x97), byte(0x98), byte(0x99), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x60
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x68
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x70
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x78
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x80
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x88
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x90
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0x98
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xA0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xA8
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xB0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xB8
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xC0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xC8
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xD0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xD8
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xE0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xE8
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xF0
    byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), byte(0xFF), // 0xF8
};

/// Cpu::Instruction Addressing Modes

/// Memory Locations

inline word EA_ZPAGE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(p_6502.m_register.PC++, AT_OPERAND); }

inline word EA_ZPAGE_X(MCS6502 &p_6502)
{ return EA_ZPAGE(p_6502) + p_6502.m_register.X; }

inline word EA_ZPAGE_Y(MCS6502 &p_6502)
{ return EA_ZPAGE(p_6502) + p_6502.m_register.Y; }

inline word EA_ABSOLUTE(MCS6502 &p_6502)
{
    const word result(p_6502.m_memory.get_word(p_6502.m_register.PC, AT_OPERAND));
    p_6502.m_register.PC += 2;
    return result;
}

inline word EA_ABSOLUTE_X(MCS6502 &p_6502)
{ return EA_ABSOLUTE(p_6502) + p_6502.m_register.X; }

inline word EA_ABSOLUTE_Y(MCS6502 &p_6502)
{ return EA_ABSOLUTE(p_6502) + p_6502.m_register.Y; }

inline word EA_INDIRECT_ZPAGE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ZPAGE(p_6502), AT_DATA); }

inline word EA_INDIRECT_ZPAGE_X(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ZPAGE_X(p_6502), AT_DATA); }

inline word EA_INDIRECT_ZPAGE_Y(MCS6502 &p_6502)
{ return EA_INDIRECT_ZPAGE(p_6502) + p_6502.m_register.Y; }

inline word EA_INDIRECT_ABSOLUTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ABSOLUTE(p_6502), AT_DATA); }

/// Bytes Values

inline byte IMMEDIATE_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(p_6502.m_register.PC++, AT_OPERAND); }

inline byte ZPAGE_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ZPAGE(p_6502), AT_DATA); }

inline byte ZPAGE_X_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ZPAGE_X(p_6502), AT_DATA); }

inline byte ZPAGE_Y_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ZPAGE_Y(p_6502), AT_DATA); }

inline byte ABSOLUTE_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ABSOLUTE(p_6502), AT_DATA); }

inline byte ABSOLUTE_X_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ABSOLUTE_X(p_6502), AT_DATA); }

inline byte ABSOLUTE_Y_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_ABSOLUTE_Y(p_6502), AT_DATA); }

inline byte INDIRECT_ZPAGE_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_INDIRECT_ZPAGE(p_6502), AT_DATA); }

inline byte INDIRECT_ZPAGE_X_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_INDIRECT_ZPAGE_X(p_6502), AT_DATA); }

inline byte INDIRECT_ZPAGE_Y_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_INDIRECT_ZPAGE_Y(p_6502), AT_DATA); }

inline byte INDIRECT_ABSOLUTE_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(EA_INDIRECT_ABSOLUTE(p_6502), AT_DATA); }

/// Word Values

inline word IMMEDIATE_WORD(MCS6502 &p_6502)
{
    const word result(p_6502.m_memory.get_word(p_6502.m_register.PC, AT_OPERAND));
    p_6502.m_register.PC += 2;
    return result;
}

inline word ZPAGE_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ZPAGE(p_6502), AT_DATA); }

inline word ZPAGE_X_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ZPAGE_X(p_6502), AT_DATA); }

inline word ZPAGE_Y_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ZPAGE_Y(p_6502), AT_DATA); }

inline word ABSOLUTE_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ABSOLUTE(p_6502), AT_DATA); }

inline word ABSOLUTE_WORD_X(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ABSOLUTE_X(p_6502), AT_DATA); }

inline word ABSOLUTE_WORD_Y(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_ABSOLUTE_Y(p_6502), AT_DATA); }

inline word INDIRECT_ZPAGE_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_INDIRECT_ZPAGE(p_6502), AT_DATA); }

inline word INDIRECT_ZPAGE_X_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_INDIRECT_ZPAGE_X(p_6502), AT_DATA); }

inline word INDIRECT_ZPAGE_Y_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_INDIRECT_ZPAGE_Y(p_6502), AT_DATA); }

inline word INDIRECT_ABSOLUTE_WORD(MCS6502 &p_6502)
{ return p_6502.m_memory.get_word(EA_INDIRECT_ABSOLUTE(p_6502), AT_DATA); }

/// Condition Flags

inline bool CS_COND(MCS6502 &p_6502)
{ return p_6502.m_register.P & CARRY; }

inline bool CC_COND(MCS6502 &p_6502)
{ return !CS_COND(p_6502); }

inline bool EQ_COND(MCS6502 &p_6502)
{ return p_6502.m_register.P & ZERO; }

inline bool NE_COND(MCS6502 &p_6502)
{ return !EQ_COND(p_6502); }

inline bool MI_COND(MCS6502 &p_6502)
{ return p_6502.m_register.P & NEGATIVE; }

inline bool PL_COND(MCS6502 &p_6502)
{ return !MI_COND(p_6502); }

inline bool VS_COND(MCS6502 &p_6502)
{ return p_6502.m_register.P & OVERFLOW; }

inline bool VC_COND(MCS6502 &p_6502)
{ return !VS_COND(p_6502); }

/// Stack Utilities

inline byte POP_BYTE(MCS6502 &p_6502)
{ return p_6502.m_memory.get_byte(STACK_ADDRESS + ++p_6502.m_register.S, AT_DATA); }

inline void PUSH_BYTE(MCS6502 &p_6502, byte p_byte)
{ p_6502.m_memory.set_byte(STACK_ADDRESS + p_6502.m_register.S--, p_byte, AT_DATA); }

inline word POP_WORD(MCS6502 &p_6502)
{
    const word lower_byte(POP_BYTE(p_6502));
    const word upper_byte(POP_BYTE(p_6502));
    return lower_byte | upper_byte << 8;
}

inline void PUSH_WORD(MCS6502 &p_6502, word p_word)
{
    PUSH_BYTE(p_6502, p_word >> 8);
    PUSH_BYTE(p_6502, p_word);
}

///****************************************************************************
/// Cpu::Instruction General Utilities
///****************************************************************************

class MCS6502::Instruction : public Part {
    // Attributes
private:
    const int         m_args;
protected:
    MCS6502           &m_6502;
public:
    const int         m_cycles;
    // Method
private:
    Instruction(const Instruction &);
    Instruction & operator= (const Instruction &);
protected:
    Instruction( MCS6502 &p_6502
               , int p_opcode
               , int p_cycles
               , const Glib::ustring p_prefix
               , int p_args = 0
               , const Glib::ustring p_suffix = "");
public:
    virtual void execute() = 0;

    friend std::ostream &::operator<<(std::ostream&, const Instruction&);
};

MCS6502::Instruction::Instruction( MCS6502 &p_6502
                                 , int p_opcode
                                 , int p_cycles
                                 , const Glib::ustring p_prefix
                                 , int p_args
                                 , const Glib::ustring p_suffix)
    : Part("MCS6502:Instruction:" + p_prefix + Glib::ustring(p_args < 0 ? 4 : p_args * 2, '_') + p_suffix)
    , m_args(p_args)
    , m_6502(p_6502)
    , m_cycles(p_cycles)
{
    LOG4CXX_INFO(cpptrace_log(), "Cpu::Instruction::Instruction("
                 << p_opcode
                 << ", "
                 << p_cycles
                 << ", \""
                 << p_prefix
                 << "\", "
                 << p_args
                 << ", \""
                 << p_suffix
                 << "\")");
    if (p_opcode >= 0)
        p_6502.m_opcode_mapping[p_opcode] = std::shared_ptr<MCS6502::Instruction>(this);
}


///****************************************************************************
/// ADC
/// Use an 'internal' 16bit accumulator to catch the carry
///****************************************************************************

inline void ADC(MCS6502 &p_6502, byte p_byte)
{
    //TODO: Not sure that Decimal Mode works correctly for flags
    word result(p_6502.m_register.A);
    if (p_6502.m_register.P & DECIMAL) {
        p_byte = BCD_to_BIN[p_byte];
        result = BCD_to_BIN[result];
    }
    result += p_byte;
    if (p_6502.m_register.P & CARRY)
        result++;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY | OVERFLOW);
    if (p_6502.m_register.P & DECIMAL) {
        if (result > 99)
            p_6502.m_register.P |= CARRY;
        result = BIN_to_BCD[result];
    }
    else {
        if (result & 0x0100)
            p_6502.m_register.P |= CARRY;
        result &= 0xFF;
    }
    if ((p_6502.m_register.A ^ result) & 0x80)
        p_6502.m_register.P |= OVERFLOW;
    p_6502.m_register.A = result;
    if (result & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (result == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_ADC_IMM : public MCS6502::Instruction {
public:
    explicit Instr_ADC_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x69, 2, "ADC @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_IMM::execute");
            ADC(m_6502, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_ADC_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_ADC_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x65, 3, "ADC ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_ZERO::execute");
            ADC(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_ADC_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_ADC_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x75, 4, "ADC ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_ZERO_X::execute");
            ADC(m_6502, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_ADC_ABS : public MCS6502::Instruction {
public:
    explicit Instr_ADC_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x6D, 4, "ADC ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_ABS::execute");
            ADC(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_ADC_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_ADC_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x7D, 4, "ADC ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_ABS_X::execute");
            ADC(m_6502, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_ADC_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_ADC_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x79, 4, "ADC ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_ABS_Y::execute");
            ADC(m_6502, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_ADC_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_ADC_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x61, 6, "ADC (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_PRE_INDEXED_INDIRECT::execute");
            ADC(m_6502, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_ADC_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_ADC_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x71, 5, "ADC (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ADC_POST_INDEXED_INDIRECT::execute");
            ADC(m_6502, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

///****************************************************************************
/// AND
///****************************************************************************

inline void AND(MCS6502 &p_6502, byte p_byte)
{
    p_6502.m_register.A &= p_byte;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO);
    if (p_6502.m_register.A & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_6502.m_register.A == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_AND_IMM : public MCS6502::Instruction {
public:
    explicit Instr_AND_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x29, 2, "AND @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_IMM::execute");
            AND(m_6502, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_AND_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_AND_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x25, 3, "AND ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_ZERO::execute");
            AND(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_AND_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_AND_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x35, 4, "AND ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_ZERO_X::execute");
            AND(m_6502, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_AND_ABS : public MCS6502::Instruction {
public:
    explicit Instr_AND_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x2D, 4, "AND ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_ABS::execute");
            AND(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_AND_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_AND_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x3D, 4, "AND ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_ABS_X::execute");
            AND(m_6502, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_AND_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_AND_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x39, 4, "AND ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_ABS_Y::execute");
            AND(m_6502, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_AND_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_AND_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x21, 6, "AND (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_PRE_INDEXED_INDIRECT::execute");
            AND(m_6502, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_AND_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_AND_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x31, 5, "AND (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_AND_POST_INDEXED_INDIRECT::execute");
            AND(m_6502, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

///****************************************************************************
/// ASL
///****************************************************************************

inline void ASL(MCS6502 &p_6502, byte &p_byte)
{
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY);
    if (p_byte & 0x80)
        p_6502.m_register.P |= CARRY;
    p_byte <<= 1;
    if (p_byte & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void ASL_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    ASL(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_ASL_A : public MCS6502::Instruction {
public:
    explicit Instr_ASL_A(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x0A, 2, "ASL A") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ASL_A::execute");
            ASL(m_6502, m_6502.m_register.A);
        };
};

class Instr_ASL_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_ASL_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x06, 5, "ASL ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ASL_ZERO::execute");
            ASL_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_ASL_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_ASL_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x16, 6, "ASL ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ASL_ZERO_X::execute");
            ASL_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_ASL_ABS : public MCS6502::Instruction {
public:
    explicit Instr_ASL_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x0E, 6, "ASL ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ASL_ABS::execute");
            ASL_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_ASL_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_ASL_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x1E, 7, "ASL ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ASL_ABS_X::execute");
            ASL_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

///****************************************************************************
/// BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS
/// offset is treated as a signed quantity
///****************************************************************************

inline void BRANCH(MCS6502 &p_6502, bool p_cond)
{
#if JUMP_TRACE
    const word from(p_6502.m_register.PC-1);
#endif
    if (p_cond) {
        p_6502.m_register.PC += signed_byte(p_6502.m_memory.get_byte(p_6502.m_register.PC, AT_OPERAND));
#if JUMP_TRACE
        log4c_category_debug(p_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                             "Bxx", from, p_6502.m_register.PC+1);
#endif
    }
    p_6502.m_register.PC++;
}

class Instr_BCC : public MCS6502::Instruction {
public:
    explicit Instr_BCC(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x90, 3, "BCC ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BCC::execute");
            BRANCH(m_6502, CC_COND(m_6502));
        };
};

class Instr_BCS : public MCS6502::Instruction {
public:
    explicit Instr_BCS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB0, 3, "BCS ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BCS::execute");
            BRANCH(m_6502, CS_COND(m_6502));
        };
};

class Instr_BEQ : public MCS6502::Instruction {
public:
    explicit Instr_BEQ(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF0, 3, "BEQ ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BEQ::execute");
            BRANCH(m_6502, EQ_COND(m_6502));
        };
};

class Instr_BMI : public MCS6502::Instruction {
public:
    explicit Instr_BMI(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x30, 3, "BMI ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BMI::execute");
            BRANCH(m_6502, MI_COND(m_6502));
        };
};

class Instr_BNE : public MCS6502::Instruction {
public:
    explicit Instr_BNE(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD0, 3, "BNE ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BNE::execute");
            BRANCH(m_6502, NE_COND(m_6502));
        };
};

class Instr_BPL : public MCS6502::Instruction {
public:
    explicit Instr_BPL(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x10, 3, "BPL ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BPL::execute");
            BRANCH(m_6502, PL_COND(m_6502));
        };
};

class Instr_BVC : public MCS6502::Instruction {
public:
    explicit Instr_BVC(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x50, 3, "BVC ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BVC::execute");
            BRANCH(m_6502, VC_COND(m_6502));
        };
};

class Instr_BVS : public MCS6502::Instruction {
public:
    explicit Instr_BVS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x70, 3, "BVS ", -1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BVS::execute");
            BRANCH(m_6502, VS_COND(m_6502));
        };
};

///****************************************************************************
/// BIT
///****************************************************************************

inline void BIT(MCS6502 &p_6502, byte p_byte)
{
    p_6502.m_register.P &= ~(NEGATIVE | OVERFLOW | ZERO); \
    p_6502.m_register.P |= (p_byte & (NEGATIVE | OVERFLOW)); \
    if ((p_byte & p_6502.m_register.A) == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_BIT_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_BIT_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x24, 3, "BIT ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BIT_ZERO::execute");
            BIT(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_BIT_ABS : public MCS6502::Instruction {
public:
    explicit Instr_BIT_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x2C, 4, "BIT ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BIT_ABS::execute");
            BIT(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

///****************************************************************************
/// BRK
///****************************************************************************

class Instr_BRK : public MCS6502::Instruction {
public:
    explicit Instr_BRK(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x00, 7, "BRK") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_BRK::execute");
            m_6502.m_InterruptSource |= BRK_INTERRUPT;
        };
};

///****************************************************************************
/// CLC, CLD, CLI, CLV
///****************************************************************************

class Instr_CLC : public MCS6502::Instruction {
public:
    explicit Instr_CLC(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x18, 2, "CLC") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CLC::execute");
            m_6502.m_register.P &= ~CARRY;
        };
};

class Instr_CLD : public MCS6502::Instruction {
public:
    explicit Instr_CLD(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD8, 2, "CLD") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CLD::execute");
            m_6502.m_register.P &= ~DECIMAL;
        };
};

class Instr_CLI : public MCS6502::Instruction {
public:
    explicit Instr_CLI(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x58, 2, "CLI") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CLI::execute");
            m_6502.m_register.P &= ~IRQB;
        };
};

class Instr_CLV : public MCS6502::Instruction {
public:
    explicit Instr_CLV(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB8, 2, "CLV") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CLV::execute");
            m_6502.m_register.P &= ~OVERFLOW;
        };
};

///****************************************************************************
/// CMP, CPX, CPY
/// Use an 'internal' 16bit accumulator to catch the carry
///****************************************************************************

inline void CMP_R(MCS6502 &p_6502, byte p_reg, byte p_byte)
{
    word result(p_reg);
    result -= p_byte;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY);
    if (!(result & 0x0100))
        p_6502.m_register.P |= CARRY;
    if (result & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (result == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_CMP_IMM : public MCS6502::Instruction {
public:
    explicit Instr_CMP_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC9, 2, "CMP @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_IMM::execute");
            CMP_R(m_6502, m_6502.m_register.A, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_CMP_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_CMP_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC5, 3, "CMP ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_ZERO::execute");
            CMP_R(m_6502, m_6502.m_register.A, ZPAGE_BYTE(m_6502));
        };
};

class Instr_CMP_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_CMP_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD5, 4, "CMP ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_ZERO_X::execute");
            CMP_R(m_6502, m_6502.m_register.A, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_CMP_ABS : public MCS6502::Instruction {
public:
    explicit Instr_CMP_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xCD, 4, "CMP ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_ABS::execute");
            CMP_R(m_6502, m_6502.m_register.A, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_CMP_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_CMP_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xDD, 4, "CMP ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_ABS_X::execute");
            CMP_R(m_6502, m_6502.m_register.A, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_CMP_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_CMP_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD9, 4, "CMP ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_ABS_Y::execute");
            CMP_R(m_6502, m_6502.m_register.A, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_CMP_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_CMP_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC1, 6, "CMP (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_PRE_INDEXED_INDIRECT::execute");
            CMP_R(m_6502, m_6502.m_register.A, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_CMP_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_CMP_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD1, 5, "CMP (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CMP_POST_INDEXED_INDIRECT::execute");
            CMP_R(m_6502, m_6502.m_register.A, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

class Instr_CPX_IMM : public MCS6502::Instruction {
public:
    explicit Instr_CPX_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE0, 2, "CPX @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPX_IMM::execute");
            CMP_R(m_6502, m_6502.m_register.X, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_CPX_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_CPX_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE4, 3, "CPX ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPX_ZERO::execute");
            CMP_R(m_6502, m_6502.m_register.X, ZPAGE_BYTE(m_6502));
        };
};

class Instr_CPX_ABS : public MCS6502::Instruction {
public:
    explicit Instr_CPX_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xEC, 4, "CPX ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPX_ABS::execute");
            CMP_R(m_6502, m_6502.m_register.X, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_CPY_IMM : public MCS6502::Instruction {
public:
    explicit Instr_CPY_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC0, 2, "CPY @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPY_IMM::execute");
            CMP_R(m_6502, m_6502.m_register.Y, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_CPY_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_CPY_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC4, 3, "CPY ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPY_ZERO::execute");
            CMP_R(m_6502, m_6502.m_register.Y, ZPAGE_BYTE(m_6502));
        };
};

class Instr_CPY_ABS : public MCS6502::Instruction {
public:
    explicit Instr_CPY_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xCC, 4, "CPY ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_CPY_ABS::execute");
            CMP_R(m_6502, m_6502.m_register.Y, ABSOLUTE_BYTE(m_6502));
        };
};

///****************************************************************************
/// DEC, DEX, DEY
///****************************************************************************

inline void DEC(MCS6502 &p_6502, byte &p_byte)
{
    p_byte -= 1;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO );
    if (p_byte & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void DEC_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    DEC(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_DEC_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_DEC_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC6, 5, "DEC ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEC_ZERO::execute");
            DEC_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_DEC_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_DEC_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xD6, 6, "DEC ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEC_ZERO_X::execute");
            DEC_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_DEC_ABS : public MCS6502::Instruction {
public:
    explicit Instr_DEC_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xCE, 6, "DEC ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEC_ABS::execute");
            DEC_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_DEC_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_DEC_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xDE, 7, "DEC ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEC_ABS_X::execute");
            DEC_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

class Instr_DEX : public MCS6502::Instruction {
public:
    explicit Instr_DEX(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xCA, 2, "DEX") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEX::execute");
            DEC(m_6502, m_6502.m_register.X);
        };
};

class Instr_DEY : public MCS6502::Instruction {
public:
    explicit Instr_DEY(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x88, 2, "DEY") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_DEY::execute");
            DEC(m_6502, m_6502.m_register.Y);
        };
};

///****************************************************************************
/// EOR
///****************************************************************************

inline void EOR(MCS6502 &p_6502, byte p_byte)
{
    p_6502.m_register.A ^= p_byte;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO);
    if (p_6502.m_register.A & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_6502.m_register.A == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_EOR_IMM : public MCS6502::Instruction {
public:
    explicit Instr_EOR_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x49, 2, "EOR @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_IMM::execute");
            EOR(m_6502, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_EOR_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_EOR_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x45, 3, "EOR ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_ZERO::execute");
            EOR(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_EOR_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_EOR_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x55, 4, "EOR ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_ZERO_X::execute");
            EOR(m_6502, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_EOR_ABS : public MCS6502::Instruction {
public:
    explicit Instr_EOR_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x4D, 4, "EOR ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_ABS::execute");
            EOR(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_EOR_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_EOR_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x5D, 4, "EOR ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_ABS_X::execute");
            EOR(m_6502, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_EOR_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_EOR_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x59, 4, "EOR ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_ABS_Y::execute");
            EOR(m_6502, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_EOR_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_EOR_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x41, 6, "EOR (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_PRE_INDEXED_INDIRECT::execute");
            EOR(m_6502, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_EOR_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_EOR_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x51, 5, "EOR (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_EOR_POST_INDEXED_INDIRECT::execute");
            EOR(m_6502, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

///****************************************************************************
/// INC, INX, INY
///****************************************************************************

inline void INC(MCS6502 &p_6502, byte &p_byte)
{
    p_byte += 1;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO );
    if (p_byte & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void INC_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    INC(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_INC_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_INC_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE6, 5, "INC ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INC_ZERO::execute");
            INC_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_INC_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_INC_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF6, 6, "INC ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INC_ZERO_X::execute");
            INC_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_INC_ABS : public MCS6502::Instruction {
public:
    explicit Instr_INC_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xEE, 6, "INC ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INC_ABS::execute");
            INC_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_INC_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_INC_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xFE, 7, "INC ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INC_ABS_X::execute");
            INC_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

class Instr_INX : public MCS6502::Instruction {
public:
    explicit Instr_INX(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE8, 2, "INX") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INX::execute");
            INC(m_6502, m_6502.m_register.X);
        };
};

class Instr_INY : public MCS6502::Instruction {
public:
    explicit Instr_INY(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xC8, 2, "INY") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_INY::execute");
            INC(m_6502, m_6502.m_register.Y);
        };
};

///****************************************************************************
/// JMP, JSR
///****************************************************************************

class Instr_JMP_DIRECT : public MCS6502::Instruction {
public:
    explicit Instr_JMP_DIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x4C, 3, "JMP ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_JMP_DIRECT::execute");
#if JUMP_TRACE
            const word from(m_6502.m_register.PC-1);
#endif
            m_6502.m_register.PC = IMMEDIATE_WORD(m_6502);
#if JUMP_TRACE
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "JMP", from, m_6502.m_register.PC);
#endif

        };
};

class Instr_JMP_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_JMP_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x6C, 5, "JMP (", 2, ")") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_JMP_INDIRECT::execute");
#if JUMP_TRACE
            const word from(m_6502.m_register.PC-1);
#endif
            m_6502.m_register.PC = ABSOLUTE_WORD(m_6502);
#if JUMP_TRACE
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "JMP", from, m_6502.m_register.PC);
#endif
        };
};

class Instr_JSR_DIRECT : public MCS6502::Instruction {
public:
    explicit Instr_JSR_DIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x20, 6, "JSR ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_JSR_DIRECT::execute");
#if JUMP_TRACE
            const word from(m_6502.m_register.PC-1);
#endif
            PUSH_WORD(m_6502, m_6502.m_register.PC+1); // Last byte of JSR
            m_6502.m_register.PC = IMMEDIATE_WORD(m_6502);
#if JUMP_TRACE
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "JSR", from, m_6502.m_register.PC);
#endif
        };
};

///****************************************************************************
/// LDA, LDX, LDY
///****************************************************************************

inline void LD_R(MCS6502 &p_6502, byte &p_reg, byte p_byte)
{
    p_reg = p_byte;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO);
    if (p_reg & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_reg == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_LDA_IMM : public MCS6502::Instruction {
public:
    explicit Instr_LDA_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA9, 2, "LDA @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_IMM::execute");
            LD_R(m_6502, m_6502.m_register.A, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_LDA_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_LDA_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA5, 3, "LDA ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_ZERO::execute");
            LD_R(m_6502, m_6502.m_register.A, ZPAGE_BYTE(m_6502));
        };
};

class Instr_LDA_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_LDA_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB5, 4, "LDA ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_ZERO_X::execute");
            LD_R(m_6502, m_6502.m_register.A, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_LDA_ABS : public MCS6502::Instruction {
public:
    explicit Instr_LDA_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xAD, 4, "LDA ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_ABS::execute");
            LD_R(m_6502, m_6502.m_register.A, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_LDA_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_LDA_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xBD, 4, "LDA ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_ABS_X::execute");
            LD_R(m_6502, m_6502.m_register.A, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_LDA_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_LDA_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB9, 4, "LDA ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_ABS_Y::execute");
            LD_R(m_6502, m_6502.m_register.A, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_LDA_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_LDA_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA1, 6, "LDA (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_PRE_INDEXED_INDIRECT::execute");
            LD_R(m_6502, m_6502.m_register.A, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_LDA_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_LDA_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB1, 5, "LDA (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDA_POST_INDEXED_INDIRECT::execute");
            LD_R(m_6502, m_6502.m_register.A, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

class Instr_LDX_IMM : public MCS6502::Instruction {
public:
    explicit Instr_LDX_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA2, 2, "LDX @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDX_IMM::execute");
            LD_R(m_6502, m_6502.m_register.X, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_LDX_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_LDX_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA6, 3, "LDX ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDX_ZERO::execute");
            LD_R(m_6502, m_6502.m_register.X, ZPAGE_BYTE(m_6502));
        };
};

class Instr_LDX_ZERO_Y : public MCS6502::Instruction {
public:
    explicit Instr_LDX_ZERO_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB6, 4, "LDX ", 1, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDX_ZERO_Y::execute");
            LD_R(m_6502, m_6502.m_register.X, ZPAGE_Y_BYTE(m_6502));
        };
};

class Instr_LDX_ABS : public MCS6502::Instruction {
public:
    explicit Instr_LDX_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xAE, 4, "LDX ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDX_ABS::execute");
            LD_R(m_6502, m_6502.m_register.X, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_LDX_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_LDX_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xBE, 4, "LDX ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDX_ABS_Y::execute");
            LD_R(m_6502, m_6502.m_register.X, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_LDY_IMM : public MCS6502::Instruction {
public:
    explicit Instr_LDY_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA0, 2, "LDY @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDY_IMM::execute");
            LD_R(m_6502, m_6502.m_register.Y, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_LDY_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_LDY_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA4, 3, "LDY ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDY_ZERO::execute");
            LD_R(m_6502, m_6502.m_register.Y, ZPAGE_BYTE(m_6502));
        };
};

class Instr_LDY_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_LDY_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xB4, 4, "LDY ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDY_ZERO_X::execute");
            LD_R(m_6502, m_6502.m_register.Y, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_LDY_ABS : public MCS6502::Instruction {
public:
    explicit Instr_LDY_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xAC, 4, "LDY ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDY_ABS::execute");
            LD_R(m_6502, m_6502.m_register.Y, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_LDY_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_LDY_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xBC, 4, "LDY ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LDY_ABS_X::execute");
            LD_R(m_6502, m_6502.m_register.Y, ABSOLUTE_X_BYTE(m_6502));
        };
};

///****************************************************************************
/// LSR
///****************************************************************************

inline void LSR(MCS6502 &p_6502, byte &p_byte)
{
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY);
    if (p_byte & 1)
        p_6502.m_register.P |= CARRY;
    p_byte = (p_byte >> 1) & 0x7F;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void LSR_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    LSR(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_LSR_A : public MCS6502::Instruction {
public:
    explicit Instr_LSR_A(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x4A, 2, "LSR A") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LSR_A::execute");
            LSR(m_6502, m_6502.m_register.A);
        };
};

class Instr_LSR_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_LSR_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x46, 5, "LSR ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LSR_ZERO::execute");
            LSR_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_LSR_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_LSR_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x56, 6, "LSR ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LSR_ZERO_X::execute");
            LSR_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_LSR_ABS : public MCS6502::Instruction {
public:
    explicit Instr_LSR_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x4E, 6, "LSR ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LSR_ABS::execute");
            LSR_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_LSR_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_LSR_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x5E, 7, "LSR ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_LSR_ABS_X::execute");
            LSR_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

///****************************************************************************
/// NOP
///****************************************************************************

class Instr_NOP : public MCS6502::Instruction {
public:
    explicit Instr_NOP(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xEA, 2, "NOP") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_NOP::execute");
            // Do Nothing
        };
};

///****************************************************************************
/// ORA
///****************************************************************************

inline void ORA(MCS6502 &p_6502, byte p_byte)
{
    p_6502.m_register.A |= p_byte;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO);
    if (p_6502.m_register.A & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_6502.m_register.A == 0)
        p_6502.m_register.P |= ZERO;
}

class Instr_ORA_IMM : public MCS6502::Instruction {
public:
    explicit Instr_ORA_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x09, 2, "ORA @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_IMM::execute");
            ORA(m_6502, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_ORA_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_ORA_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x05, 3, "ORA ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_ZERO::execute");
            ORA(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_ORA_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_ORA_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x15, 4, "ORA ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_ZERO_X::execute");
            ORA(m_6502, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_ORA_ABS : public MCS6502::Instruction {
public:
    explicit Instr_ORA_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x0D, 4, "ORA ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_ABS::execute");
            ORA(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_ORA_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_ORA_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x1D, 4, "ORA ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_ABS_X::execute");
            ORA(m_6502, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_ORA_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_ORA_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x19, 4, "ORA ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_ABS_Y::execute");
            ORA(m_6502, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_ORA_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_ORA_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x01, 6, "ORA (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_PRE_INDEXED_INDIRECT::execute");
            ORA(m_6502, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_ORA_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_ORA_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x11, 5, "ORA (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ORA_POST_INDEXED_INDIRECT::execute");
            ORA(m_6502, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

///****************************************************************************
/// PHA, PHP, PLA, PLP
///****************************************************************************

class Instr_PHA : public MCS6502::Instruction {
public:
    explicit Instr_PHA(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x48, 3, "PHA") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_PHA::execute");
            PUSH_BYTE(m_6502, m_6502.m_register.A);
        };
};

class Instr_PHP : public MCS6502::Instruction {
public:
    explicit Instr_PHP(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x08, 3, "PHP") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_PHP::execute");
            PUSH_BYTE(m_6502, m_6502.m_register.P);
        };
};

class Instr_PLA : public MCS6502::Instruction {
public:
    explicit Instr_PLA(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x68, 4, "PLA") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_PLA::execute");
            m_6502.m_register.A = POP_BYTE(m_6502);
            m_6502.m_register.P &= ~(NEGATIVE | ZERO);
            if (m_6502.m_register.A & NEGATIVE)
                m_6502.m_register.P |= NEGATIVE;
            if (m_6502.m_register.A == 0)
                m_6502.m_register.P |= ZERO;
        };
};

class Instr_PLP : public MCS6502::Instruction {
public:
    explicit Instr_PLP(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x28, 4, "PLP") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_PLP::execute");
            m_6502.m_register.P = POP_BYTE(m_6502);
        };
};

///****************************************************************************
/// ROL
///****************************************************************************

inline void ROL(MCS6502 &p_6502, byte &p_byte)
{
    const bool high_bit(p_byte & 0x80);
    p_byte <<= 1;
    if (p_6502.m_register.P & CARRY)
        p_byte |= 1;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY);
    if (high_bit)
        p_6502.m_register.P |= CARRY;
    if (p_byte & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void ROL_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    ROL(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_ROL_A : public MCS6502::Instruction {
public:
    explicit Instr_ROL_A(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x2A, 2, "ROL A") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROL_A::execute");
            ROL(m_6502, m_6502.m_register.A);
        };
};

class Instr_ROL_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_ROL_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x26, 5, "ROL ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROL_ZERO::execute");
            ROL_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_ROL_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_ROL_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x36, 6, "ROL ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROL_ZERO_X::execute");
            ROL_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_ROL_ABS : public MCS6502::Instruction {
public:
    explicit Instr_ROL_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x2E, 6, "ROL ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROL_ABS::execute");
            ROL_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_ROL_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_ROL_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x3E, 7, "ROL ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROL_ABS_X::execute");
            ROL_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

///****************************************************************************
/// ROR
///****************************************************************************

inline void ROR(MCS6502 &p_6502, byte &p_byte)
{
    const bool low_bit(p_byte & 0x01);
    p_byte >>= 1;
    if (p_6502.m_register.P & CARRY)
        p_byte |= 0x80;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY);
    if (low_bit)
        p_6502.m_register.P |= CARRY;
    if (p_byte & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (p_byte == 0)
        p_6502.m_register.P |= ZERO;
}

inline void ROR_EA(MCS6502 &p_6502, word p_addr)
{
    byte data(p_6502.m_memory.get_byte(p_addr, AT_DATA));
    ROR(p_6502, data);
    p_6502.m_memory.set_byte(p_addr, data, AT_DATA);
}

class Instr_ROR_A : public MCS6502::Instruction {
public:
    explicit Instr_ROR_A(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x6A, 2, "ROR A") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROR_A::execute");
            ROR(m_6502, m_6502.m_register.A);
        };
};

class Instr_ROR_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_ROR_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x66, 5, "ROR ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROR_ZERO::execute");
            ROR_EA(m_6502, EA_ZPAGE(m_6502));
        };
};

class Instr_ROR_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_ROR_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x76, 6, "ROR ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROR_ZERO_X::execute");
            ROR_EA(m_6502, EA_ZPAGE_X(m_6502));
        };
};

class Instr_ROR_ABS : public MCS6502::Instruction {
public:
    explicit Instr_ROR_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x6E, 6, "ROR ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROR_ABS::execute");
            ROR_EA(m_6502, EA_ABSOLUTE(m_6502));
        };
};

class Instr_ROR_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_ROR_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x7E, 7, "ROR ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_ROR_ABS_X::execute");
            ROR_EA(m_6502, EA_ABSOLUTE_X(m_6502));
        };
};

///****************************************************************************
/// RTI, RTS
///****************************************************************************

class Instr_RTI : public MCS6502::Instruction {
public:
    explicit Instr_RTI(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x40, 6, "RTI") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_RTI::execute");
#if JUMP_TRACE
            const word from(m_6502.m_register.PC-1);
#endif
            m_6502.m_register.P  = POP_BYTE(m_6502);
            m_6502.m_register.PC = POP_WORD(m_6502);
#if JUMP_TRACE
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "RTI", from, m_6502.m_register.PC);
#endif

        };
};

class Instr_RTS : public MCS6502::Instruction {
public:
    explicit Instr_RTS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x60, 6, "RTS") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_RTS::execute");
#if JUMP_TRACE
            const word from(m_6502.m_register.PC-1);
#endif
            m_6502.m_register.PC = POP_WORD(m_6502) + 1;
#if JUMP_TRACE
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "RTS", from, m_6502.m_register.PC);
#endif
        };
};

///****************************************************************************
/// SBC
/// Use an 'internal' 16bit accumulator to catch the carry
///****************************************************************************

inline void SBC(MCS6502 &p_6502, byte p_byte)
{
    word result(p_6502.m_register.A);
    if (p_6502.m_register.P & DECIMAL) {
        p_byte = BCD_to_BIN[p_byte];
        result = BCD_to_BIN[result];
    }
    result += ~p_byte;
    if (p_6502.m_register.P & CARRY)
        result++;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO | CARRY | OVERFLOW);
    if (p_6502.m_register.P & DECIMAL) {
        if (result <= 99)
            p_6502.m_register.P |= CARRY;
        result = BIN_to_BCD[result];
    }
    else {
        if (!(result & 0x0100))
            p_6502.m_register.P |= CARRY;
        result &= 0xFF;
    }
    if ((p_6502.m_register.A ^ result) & 0x80)
        p_6502.m_register.P |= OVERFLOW;
    p_6502.m_register.A = result;
    if (result & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (!result)
        p_6502.m_register.P |= ZERO;
}

class Instr_SBC_IMM : public MCS6502::Instruction {
public:
    explicit Instr_SBC_IMM(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE9, 2, "SBC @", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_IMM::execute");
            SBC(m_6502, IMMEDIATE_BYTE(m_6502));
        };
};

class Instr_SBC_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_SBC_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE5, 3, "SBC ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_ZERO::execute");
            SBC(m_6502, ZPAGE_BYTE(m_6502));
        };
};

class Instr_SBC_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_SBC_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF5, 4, "SBC ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_ZERO_X::execute");
            SBC(m_6502, ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_SBC_ABS : public MCS6502::Instruction {
public:
    explicit Instr_SBC_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xED, 4, "SBC ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_ABS::execute");
            SBC(m_6502, ABSOLUTE_BYTE(m_6502));
        };
};

class Instr_SBC_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_SBC_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xFD, 4, "SBC ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_ABS_X::execute");
            SBC(m_6502, ABSOLUTE_X_BYTE(m_6502));
        };
};

class Instr_SBC_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_SBC_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF9, 4, "SBC ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_ABS_Y::execute");
            SBC(m_6502, ABSOLUTE_Y_BYTE(m_6502));
        };
};

class Instr_SBC_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_SBC_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xE1, 6, "SBC (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_PRE_INDEXED_INDIRECT::execute");
            SBC(m_6502, INDIRECT_ZPAGE_X_BYTE(m_6502));
        };
};

class Instr_SBC_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_SBC_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF1, 5, "SBC (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SBC_POST_INDEXED_INDIRECT::execute");
            SBC(m_6502, INDIRECT_ZPAGE_Y_BYTE(m_6502));
        };
};

///****************************************************************************
/// SEC, SED, SEI
///****************************************************************************

class Instr_SEC : public MCS6502::Instruction {
public:
    explicit Instr_SEC(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x38, 2, "SEC") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SEC::execute");
            m_6502.m_register.P |= CARRY;
        };
};

class Instr_SED : public MCS6502::Instruction {
public:
    explicit Instr_SED(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xF8, 2, "SED") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SED::execute");
            m_6502.m_register.P |= DECIMAL;
        };
};

class Instr_SEI : public MCS6502::Instruction {
public:
    explicit Instr_SEI(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x78, 2, "SEI") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_SEI::execute");
            m_6502.m_register.P |= IRQB;
        };
};

///****************************************************************************
/// STA, STX, STY
///****************************************************************************

class Instr_STA_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_STA_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x85, 3, "STA ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_ZERO::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_STA_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x95, 4, "STA ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_ZERO_X::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE_X(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_ABS : public MCS6502::Instruction {
public:
    explicit Instr_STA_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x8D, 4, "STA ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_ABS::execute");
            m_6502.m_memory.set_byte(EA_ABSOLUTE(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_ABS_X : public MCS6502::Instruction {
public:
    explicit Instr_STA_ABS_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x9D, 5, "STA ", 2, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_ABS_X::execute");
            m_6502.m_memory.set_byte(EA_ABSOLUTE_X(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_ABS_Y : public MCS6502::Instruction {
public:
    explicit Instr_STA_ABS_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x99, 5, "STA ", 2, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_ABS_Y::execute");
            m_6502.m_memory.set_byte(EA_ABSOLUTE_Y(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_PRE_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_STA_PRE_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x81, 6, "STA (", 1, ",X)") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_PRE_INDEXED_INDIRECT::execute");
            m_6502.m_memory.set_byte(EA_INDIRECT_ZPAGE_X(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STA_POST_INDEXED_INDIRECT : public MCS6502::Instruction {
public:
    explicit Instr_STA_POST_INDEXED_INDIRECT(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x91, 6, "STA (", 1, "),Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STA_POST_INDEXED_INDIRECT::execute");
            m_6502.m_memory.set_byte(EA_INDIRECT_ZPAGE_Y(m_6502), m_6502.m_register.A, AT_DATA);
        };
};

class Instr_STX_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_STX_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x86, 3, "STX ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STX_ZERO::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE(m_6502), m_6502.m_register.X, AT_DATA);
        };
};

class Instr_STX_ZERO_Y : public MCS6502::Instruction {
public:
    explicit Instr_STX_ZERO_Y(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x96, 4, "STX ", 1, ",Y") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STX_ZERO_Y::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE_Y(m_6502), m_6502.m_register.X, AT_DATA);
        };
};

class Instr_STX_ABS : public MCS6502::Instruction {
public:
    explicit Instr_STX_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x8E, 4, "STX ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STX_ABS::execute");
            m_6502.m_memory.set_byte(EA_ABSOLUTE(m_6502), m_6502.m_register.X, AT_DATA);
        };
};

class Instr_STY_ZERO : public MCS6502::Instruction {
public:
    explicit Instr_STY_ZERO(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x84, 3, "STY ", 1) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STY_ZERO::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE(m_6502), m_6502.m_register.Y, AT_DATA);
        };
};

class Instr_STY_ZERO_X : public MCS6502::Instruction {
public:
    explicit Instr_STY_ZERO_X(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x94, 4, "STY ", 1, ",X") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STY_ZERO_X::execute");
            m_6502.m_memory.set_byte(EA_ZPAGE_X(m_6502), m_6502.m_register.Y, AT_DATA);
        };
};

class Instr_STY_ABS : public MCS6502::Instruction {
public:
    explicit Instr_STY_ABS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x8C, 4, "STY ", 2) {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_STY_ABS::execute");
            m_6502.m_memory.set_byte(EA_ABSOLUTE(m_6502), m_6502.m_register.Y, AT_DATA);
        };
};

///****************************************************************************
/// TAX, TAY, TSX, TXA, TXS, TYA
///****************************************************************************

inline void TR(MCS6502 &p_6502, byte p_source, byte &p_dest)
{
    p_dest = p_source;
    p_6502.m_register.P &= ~(NEGATIVE | ZERO);
    if (p_dest & NEGATIVE)
        p_6502.m_register.P |= NEGATIVE;
    if (!p_dest)
        p_6502.m_register.P |= ZERO;
}

class Instr_TAX : public MCS6502::Instruction {
public:
    explicit Instr_TAX(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xAA, 2, "TAX") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TAX::execute");
            TR(m_6502, m_6502.m_register.A, m_6502.m_register.X);
        };
};

class Instr_TAY : public MCS6502::Instruction {
public:
    explicit Instr_TAY(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xA8, 2, "TAY") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TAY::execute");
            TR(m_6502, m_6502.m_register.A, m_6502.m_register.Y);
        };
};

class Instr_TSX : public MCS6502::Instruction {
public:
    explicit Instr_TSX(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0xBA, 2, "TSX") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TSX::execute");
            TR(m_6502, m_6502.m_register.S, m_6502.m_register.X);
        };
};

class Instr_TXA : public MCS6502::Instruction {
public:
    explicit Instr_TXA(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x8A, 2, "TXA") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TXA::execute");
            TR(m_6502, m_6502.m_register.X, m_6502.m_register.A);
        };
};

class Instr_TXS : public MCS6502::Instruction {
public:
    explicit Instr_TXS(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x9A, 2, "TXS") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TXS::execute");
            TR(m_6502, m_6502.m_register.X, m_6502.m_register.S);
        };
};

class Instr_TYA : public MCS6502::Instruction {
public:
    explicit Instr_TYA(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, 0x98, 2, "TYA") {};
    virtual void execute()
        {
            INST_CTRACE_LOG("Instr_TYA::execute");
            TR(m_6502, m_6502.m_register.Y, m_6502.m_register.A);
        };
};

class Instr_Undefined : public MCS6502::Instruction {
public:
    explicit Instr_Undefined(MCS6502 &p_6502) : MCS6502::Instruction(p_6502, -1, 0, "Undefined Instruction") {};
    virtual void execute()
        {
            assert (false);
        };

    friend std::ostream &::operator<<(std::ostream&, const Instr_Undefined &);
};

///*****************************************************************************
///     00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
/// 00: BRK ORA  -   -   -  ORA ASL  -  PHP ORA ASL  -   -  ORA ASL  -
/// 10: BPL ORA  -   -   -  ORA ASL  -  CLC ORA  -   -   -  ORA ASL  -
/// 20: JSR AND  -   -  BIT AND ROL  -  PLP AND ROL  -  BIT AND ROL  -
/// 30: BMI AND  -   -   -  AND ROL  -  SEC AND  -   -   -  AND ROL  -
/// 40: RTI EOR  -   -   -  EOR LSR  -  PHA EOR LSR  -  JMP EOR LSR  -
/// 50: BVC EOR  -   -   -  EOR LSR  -  CLI EOR  -   -   -  EOR LSR  -
/// 60: RTS ADC  -   -   -  ADC ROR  -  PLA ADC ROR  -  JMP ADC ROR  -
/// 70: BVS ADC  -   -   -  ADC ROR  -  SEI ADC  -   -   -  ADC ROR  -
/// 80:  -  STA  -   -  STY STA STX  -  DEY  -  TXA  -  STY STA STX  -
/// 90: BCC STA  -   -  STY STA STX  -  TYA STA TXS  -   -  STA  -   -
/// A0: LDY LDA LDX  -  LDY LDA LDX  -  TAY LDA TAX  -  LDY LDA LDX  -
/// B0: BCS LDA  -   -  LDY LDA LDX  -  CLV LDA TSX  -  LDY LDA LDX  -
/// C0: CPY CMP  -   -  CPY CMP DEC  -  INY CMP DEX  -  CPY CMP DEC  -
/// D0: BNE CMP  -   -   -  CMP DEC  -  CLD CMP  -   -   -  CMP DEC  -
/// E0: CPX SBC  -   -  CPX SBC INC  -  INX SBC NOP  -  CPX SBC INC  -
/// F0: BEQ SBC  -   -   -  SBC INC  -  SED SBC  -   -   -  SBC INC  -
///*****************************************************************************
MCS6502::MCS6502(Device &p_memory, const Configurator &p_cfg)
    : Core(p_cfg)
    , m_memory(p_memory)
    , m_opcode_mapping(256, std::shared_ptr<MCS6502::Instruction>(new Instr_Undefined(*this)))
    , m_InterruptSource(NO_INTERRUPT)
{
    LOG4CXX_INFO(cpptrace_log(), "MCS6502::MCS6502([" << p_memory.id() << "], " << p_cfg << ")");
    assert (SIZE(m_memory.size()) == 65536);
    new Instr_BRK(*this);
    new Instr_ORA_PRE_INDEXED_INDIRECT(*this);
    new Instr_ORA_ZERO(*this);
    new Instr_ASL_ZERO(*this);
    new Instr_PHP(*this);
    new Instr_ORA_IMM(*this);
    new Instr_ASL_A(*this);
    new Instr_ORA_ABS(*this);
    new Instr_ASL_ABS(*this);
    new Instr_BPL(*this);
    new Instr_ORA_POST_INDEXED_INDIRECT(*this);
    new Instr_ORA_ZERO_X(*this);
    new Instr_ASL_ZERO_X(*this);
    new Instr_CLC(*this);
    new Instr_ORA_ABS_Y(*this);
    new Instr_ORA_ABS_X(*this);
    new Instr_ASL_ABS_X(*this);
    new Instr_JSR_DIRECT(*this);
    new Instr_AND_PRE_INDEXED_INDIRECT(*this);
    new Instr_BIT_ZERO(*this);
    new Instr_AND_ZERO(*this);
    new Instr_ROL_ZERO(*this);
    new Instr_PLP(*this);
    new Instr_AND_IMM(*this);
    new Instr_ROL_A(*this);
    new Instr_BIT_ABS(*this);
    new Instr_AND_ABS(*this);
    new Instr_ROL_ABS(*this);
    new Instr_BMI(*this);
    new Instr_AND_POST_INDEXED_INDIRECT(*this);
    new Instr_AND_ZERO_X(*this);
    new Instr_ROL_ZERO_X(*this);
    new Instr_SEC(*this);
    new Instr_AND_ABS_Y(*this);
    new Instr_AND_ABS_X(*this);
    new Instr_ROL_ABS_X(*this);
    new Instr_RTI(*this);
    new Instr_EOR_PRE_INDEXED_INDIRECT(*this);
    new Instr_EOR_ZERO(*this);
    new Instr_LSR_ZERO(*this);
    new Instr_PHA(*this);
    new Instr_EOR_IMM(*this);
    new Instr_LSR_A(*this);
    new Instr_JMP_DIRECT(*this);
    new Instr_EOR_ABS(*this);
    new Instr_LSR_ABS(*this);
    new Instr_BVC(*this);
    new Instr_EOR_POST_INDEXED_INDIRECT(*this);
    new Instr_EOR_ZERO_X(*this);
    new Instr_LSR_ZERO_X(*this);
    new Instr_CLI(*this);
    new Instr_EOR_ABS_Y(*this);
    new Instr_EOR_ABS_X(*this);
    new Instr_LSR_ABS_X(*this);
    new Instr_RTS(*this);
    new Instr_ADC_PRE_INDEXED_INDIRECT(*this);
    new Instr_ADC_ZERO(*this);
    new Instr_ROR_ZERO(*this);
    new Instr_PLA(*this);
    new Instr_ADC_IMM(*this);
    new Instr_ROR_A(*this);
    new Instr_JMP_INDIRECT(*this);
    new Instr_ADC_ABS(*this);
    new Instr_ROR_ABS(*this);
    new Instr_BVS(*this);
    new Instr_ADC_POST_INDEXED_INDIRECT(*this);
    new Instr_ADC_ZERO_X(*this);
    new Instr_ROR_ZERO_X(*this);
    new Instr_SEI(*this);
    new Instr_ADC_ABS_Y(*this);
    new Instr_ADC_ABS_X(*this);
    new Instr_ROR_ABS_X(*this);
    new Instr_STA_PRE_INDEXED_INDIRECT(*this);
    new Instr_STY_ZERO(*this);
    new Instr_STA_ZERO(*this);
    new Instr_STX_ZERO(*this);
    new Instr_DEY(*this);
    new Instr_TXA(*this);
    new Instr_STY_ABS(*this);
    new Instr_STA_ABS(*this);
    new Instr_STX_ABS(*this);
    new Instr_BCC(*this);
    new Instr_STA_POST_INDEXED_INDIRECT(*this);
    new Instr_STY_ZERO_X(*this);
    new Instr_STA_ZERO_X(*this);
    new Instr_STX_ZERO_Y(*this);
    new Instr_TYA(*this);
    new Instr_STA_ABS_Y(*this);
    new Instr_TXS(*this);
    new Instr_STA_ABS_X(*this);
    new Instr_LDY_IMM(*this);
    new Instr_LDA_PRE_INDEXED_INDIRECT(*this);
    new Instr_LDX_IMM(*this);
    new Instr_LDY_ZERO(*this);
    new Instr_LDA_ZERO(*this);
    new Instr_LDX_ZERO(*this);
    new Instr_TAY(*this);
    new Instr_LDA_IMM(*this);
    new Instr_TAX(*this);
    new Instr_LDY_ABS(*this);
    new Instr_LDA_ABS(*this);
    new Instr_LDX_ABS(*this);
    new Instr_BCS(*this);
    new Instr_LDA_POST_INDEXED_INDIRECT(*this);
    new Instr_LDY_ZERO_X(*this);
    new Instr_LDA_ZERO_X(*this);
    new Instr_LDX_ZERO_Y(*this);
    new Instr_CLV(*this);
    new Instr_LDA_ABS_Y(*this);
    new Instr_TSX(*this);
    new Instr_LDY_ABS_X(*this);
    new Instr_LDA_ABS_X(*this);
    new Instr_LDX_ABS_Y(*this);
    new Instr_CPY_IMM(*this);
    new Instr_CMP_PRE_INDEXED_INDIRECT(*this);
    new Instr_CPY_ZERO(*this);
    new Instr_CMP_ZERO(*this);
    new Instr_DEC_ZERO(*this);
    new Instr_INY(*this);
    new Instr_CMP_IMM(*this);
    new Instr_DEX(*this);
    new Instr_CPY_ABS(*this);
    new Instr_CMP_ABS(*this);
    new Instr_DEC_ABS(*this);
    new Instr_BNE(*this);
    new Instr_CMP_POST_INDEXED_INDIRECT(*this);
    new Instr_CMP_ZERO_X(*this);
    new Instr_DEC_ZERO_X(*this);
    new Instr_CLD(*this);
    new Instr_CMP_ABS_Y(*this);
    new Instr_CMP_ABS_X(*this);
    new Instr_DEC_ABS_X(*this);
    new Instr_CPX_IMM(*this);
    new Instr_SBC_PRE_INDEXED_INDIRECT(*this);
    new Instr_CPX_ZERO(*this);
    new Instr_SBC_ZERO(*this);
    new Instr_INC_ZERO(*this);
    new Instr_INX(*this);
    new Instr_SBC_IMM(*this);
    new Instr_NOP(*this);
    new Instr_CPX_ABS(*this);
    new Instr_SBC_ABS(*this);
    new Instr_INC_ABS(*this);
    new Instr_BEQ(*this);
    new Instr_SBC_POST_INDEXED_INDIRECT(*this);
    new Instr_SBC_ZERO_X(*this);
    new Instr_INC_ZERO_X(*this);
    new Instr_SED(*this);
    new Instr_SBC_ABS_Y(*this);
    new Instr_SBC_ABS_X(*this);
    new Instr_INC_ABS_X(*this);
    m_register.P = UNUSED1;
}

MCS6502::~MCS6502()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].~Cpu()");
}

void MCS6502::single_step()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].single_step()");
#if EXEC_TRACE
    dump(m_6502tracelog, 1);
#endif
    if (m_InterruptSource != NO_INTERRUPT) { // Interrupt Pending
#if JUMP_TRACE
        const word from(m_6502.m_register.PC);
#endif
        if (m_InterruptSource & RESET_INTERRUPT_ON) { // Process RESET
            if (m_InterruptSource & RESET_INTERRUPT_PULSE)
                m_InterruptSource &= ~(RESET_INTERRUPT_ON | RESET_INTERRUPT_PULSE);
#if 0
            // Datasheet says Reset prefixed by useless stack reads
            // though this causes a read of the uninitialised register S
            (void)m_memory.get_byte(STACK_ADDRESS + m_register.S--);
            (void)m_memory.get_byte(STACK_ADDRESS + m_register.S--);
            (void)m_memory.get_byte(STACK_ADDRESS + m_register.S--);
#endif
            m_register.P |= IRQB;
            m_register.PC = m_memory.get_word(VECTOR_RESET, AT_DATA);
            m_cycles += 7;
        }
        else if (m_InterruptSource & NMI_INTERRUPT_ON) { // Process NMI
            if (m_InterruptSource & NMI_INTERRUPT_PULSE)
                m_InterruptSource &= ~(NMI_INTERRUPT_ON | NMI_INTERRUPT_PULSE);
            PUSH_WORD(*this, m_register.PC);
            PUSH_BYTE(*this, m_register.P);
            m_register.P |= IRQB;
            m_register.PC = m_memory.get_word(VECTOR_NMI, AT_DATA);
            m_cycles += 7;
        }
        else if ((m_InterruptSource & IRQ_INTERRUPT_ON) && !(m_register.P & IRQB)) { // Process IRQ
            if (m_InterruptSource & IRQ_INTERRUPT_PULSE)
                m_InterruptSource &= ~(IRQ_INTERRUPT_ON | IRQ_INTERRUPT_PULSE);
            PUSH_WORD(*this, m_register.PC);
            PUSH_BYTE(*this, m_register.P);
            m_register.P |= IRQB;
            m_register.P &= ~BREAK;
            m_register.PC = m_memory.get_word(VECTOR_INTERRUPT, AT_DATA);
            m_cycles += 7;
        }
        else if (m_InterruptSource & BRK_INTERRUPT) { // Process BRK
            m_InterruptSource &= ~BRK_INTERRUPT;
            PUSH_WORD(*this, m_register.PC+1); // Obscure Point
            PUSH_BYTE(*this, m_register.P | BREAK);
            m_register.P |= IRQB;
            m_register.PC = m_memory.get_word(VECTOR_INTERRUPT, AT_DATA);
            m_cycles += 7;
        }
#if JUMP_TRACE
        if (from != m_6502.m_register.PC)
            log4c_category_info(m_6502.m_jumptracelog, JUMP_TRACE_LOGFORMAT,
                                "Interrupt!", from, m_6502.m_register.PC);
#endif
    }
    const byte opcode(m_memory.get_byte(m_register.PC++, AT_INSTRUCTION));
    std::shared_ptr<MCS6502::Instruction> instr(m_opcode_mapping[opcode]);
    assert (instr);
#if EXEC_TRACE
    instr->dump(m_6502tracelog);
#endif
    instr->execute();
    m_cycles += instr->m_cycles;
}

void MCS6502::reset(InterruptState p_is)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].reset(" << p_is << ")");
    switch (p_is) {
    case INTERRUPT_ON:
        m_InterruptSource |= RESET_INTERRUPT_ON;
        m_InterruptSource &= ~RESET_INTERRUPT_PULSE;
        break;
    case INTERRUPT_OFF:
        m_InterruptSource &= ~RESET_INTERRUPT_ON;
        m_InterruptSource &= ~RESET_INTERRUPT_PULSE;
        break;
    case INTERRUPT_PULSE:
        m_InterruptSource |= RESET_INTERRUPT_ON;
        m_InterruptSource |= RESET_INTERRUPT_PULSE;
        break;
    default:
        assert (false);
        break;
    }
}

void MCS6502::NMI(InterruptState p_is)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].NMI(" << p_is << ")");
    switch (p_is) {
    case INTERRUPT_ON:
        m_InterruptSource |= NMI_INTERRUPT_ON;
        m_InterruptSource &= ~NMI_INTERRUPT_PULSE;
        break;
    case INTERRUPT_OFF:
        m_InterruptSource &= ~NMI_INTERRUPT_ON;
        m_InterruptSource &= ~NMI_INTERRUPT_PULSE;
        break;
    case INTERRUPT_PULSE:
        m_InterruptSource |= NMI_INTERRUPT_ON;
        m_InterruptSource |= NMI_INTERRUPT_PULSE;
        break;
    default:
        assert (false);
        break;
    }
}

void MCS6502::IRQ(InterruptState p_is)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].IRQ(" << p_is << ")");
    switch (p_is) {
    case INTERRUPT_ON:
        m_InterruptSource |= IRQ_INTERRUPT_ON;
        m_InterruptSource &= ~IRQ_INTERRUPT_PULSE;
        break;
    case INTERRUPT_OFF:
        m_InterruptSource &= ~IRQ_INTERRUPT_ON;
        m_InterruptSource &= ~IRQ_INTERRUPT_PULSE;
        break;
    case INTERRUPT_PULSE:
        m_InterruptSource |= IRQ_INTERRUPT_ON;
        m_InterruptSource |= IRQ_INTERRUPT_PULSE;
        break;
    default:
        assert (false);
        break;
    }
}

#if 0
void MCS6502::trace_start()
{
    CPU_CTRACE_LOG("Cpu::trace_start(\"%s\")", id());
#if EXEC_TRACE
    const int previous_priority(log4c_category_set_priority(m_6502tracelog, LOG4C_PRIORITY_INFO));
    if (m_trace_start_count == 0) {
        assert (m_trace_finish_priority == LOG4C_PRIORITY_UNKNOWN);
        m_trace_finish_priority = previous_priority;
    }
    else
        assert (previous_priority == LOG4C_PRIORITY_INFO);
#endif
    m_trace_start_count++;
}

void MCS6502::trace_finish()
{
    CPU_CTRACE_LOG("Cpu::trace_finish(\"%s\")", id());
    if (m_trace_start_count > 0) {
        m_trace_start_count--;
#if EXEC_TRACE
        if (m_trace_start_count == 0) {
            const int old_priority(log4c_category_set_priority(m_6502tracelog, m_trace_finish_priority));
            assert (old_priority == LOG4C_PRIORITY_INFO);
            m_trace_finish_priority = LOG4C_PRIORITY_UNKNOWN;
        }
#endif
    }
    else
        assert (m_trace_finish_priority == LOG4C_PRIORITY_UNKNOWN);
}
#endif

std::ostream &operator<<(std::ostream &p_s, const Core::Configurator &p_cfg)
{
    return p_s << static_cast<const Part::Configurator &>(p_cfg);
}

std::ostream &operator<<(std::ostream &p_s, const MCS6502::Configurator &p_cfg)
{
    return p_s << "<MCS6502 " << static_cast<const Core::Configurator &>(p_cfg) << "/>";
}

std::ostream &operator<<(std::ostream &p_s, const InterruptState &p_is)
{
    if (p_is >= 0 && p_is < 3) {
        static const char *name[3] = {
            "on",
            "off",
            "pulse"
        };
        p_s << name[p_is];
    }
    else
        p_s << int(p_is);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Core &p_c)
{
    return p_s << static_cast<const Part &>(p_c)
               << ", Running("   << !pthread_equal(p_c.m_thread, pthread_self()) << ")"
               << ", StepsToGo(" << p_c.m_steps_to_go << ")";
}

std::ostream &operator<<(std::ostream &p_s, const InterruptSource &p_is)
{
    if (p_is & RESET_INTERRUPT_ON)    p_s << "reset_interrupt_on,";
    if (p_is & RESET_INTERRUPT_PULSE) p_s << "reset_interrupt_pulse,";
    if (p_is & NMI_INTERRUPT_ON)      p_s << "nmi_interrupt_on,";
    if (p_is & NMI_INTERRUPT_PULSE)   p_s << "nmi_interrupt_pulse,";
    if (p_is & IRQ_INTERRUPT_ON)      p_s << "irq_interrupt_on,";
    if (p_is & IRQ_INTERRUPT_PULSE)   p_s << "irq_interrupt_pulse,";
    if (p_is & BRK_INTERRUPT)         p_s << "brk_interrupt_pulse,";
    if (p_is & ~0x7F)
        p_s << int(p_is);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Instr_Undefined &p_i)
{
    const word PC(p_i.m_6502.m_register.PC-1);
    const byte opcode(p_i.m_6502.m_memory.get_byte(PC));
    return p_s << "Opcode:" << Hex(opcode);
}

std::ostream &operator<<(std::ostream &p_s, const MCS6502::Instruction &p_i)
{
    int val(0);
    p_s << Hex(p_i.m_6502.m_register.PC) << ": " << p_i.id();
    switch (p_i.m_args) {
    case -1: /* Relative */
        val = p_i.m_6502.m_register.PC;
        val += signed_byte(p_i.m_6502.m_memory.get_byte(val));
        val += 1;
        p_s << '#' << Hex(static_cast<word>(val));
        break;
    case 0: /* No Arg */
        break;
    case 1: /* 1 byte arg */
        val = p_i.m_6502.m_memory.get_byte(p_i.m_6502.m_register.PC);
        p_s << '#' << Hex(static_cast<byte>(val));
        break;
    case 2: /* 2 byte arg */
        val = p_i.m_6502.m_memory.get_word(p_i.m_6502.m_register.PC);
        p_s << '#' << Hex(static_cast<word>(val));
        break;
    }
    return p_s << std::endl;
}

std::ostream &operator<<(std::ostream &p_s, const MCS6502 &p_6502)
{
    return p_s << "MCS6502("
               << static_cast<const Core &>(p_6502)
               << ", Memory("  << p_6502.m_memory.id() << ")"
               << ", PC(" << Hex(p_6502.m_register.PC) << ")"
               << ", A("  << Hex(p_6502.m_register.A) << ")"
               << ", X("  << Hex(p_6502.m_register.X) << ")"
               << ", Y("  << Hex(p_6502.m_register.Y) << ")"
               << ", S("  << Hex(p_6502.m_register.S) << ")"
               << ", P("  << Hex(p_6502.m_register.P) << ")"
               << ", IntrSrc(" << p_6502.m_InterruptSource << ")";
}

// cpu.hpp

#ifndef CPU_HPP_
#define CPU_HPP_

#include <atomic>
#include <thread>
#include <array>

#include "common.hpp"
#include "part.hpp"
#include "memory.hpp"

enum InterruptState { INTERRUPT_ON,
                      INTERRUPT_OFF,
                      INTERRUPT_PULSE
};
extern std::ostream &operator<<(std::ostream&, const InterruptState&);

enum  InterruptSource { NO_INTERRUPT          = 0,
                        RESET_INTERRUPT_ON    = 1,
                        RESET_INTERRUPT_PULSE = 2,
                        NMI_INTERRUPT_ON      = 4,
                        NMI_INTERRUPT_PULSE   = 8,
                        IRQ_INTERRUPT_ON      = 16,
                        IRQ_INTERRUPT_PULSE   = 32,
                        BRK_INTERRUPT         = 64
};
extern std::ostream &operator<<(std::ostream&, const InterruptSource&);

class Cpu
    : public Device
{
    // Types
public:
    class Configurator
        : public virtual Device::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    std::atomic_uint m_steps_to_go;
protected:
    int              m_cycles;
private:
    std::atomic_bool m_thread_die;
    std::thread      m_thread;
    friend void      cpu_thread(Cpu *);
    // Methods
protected:
    explicit Cpu(const Configurator &);
public:
    virtual ~Cpu();
    virtual void resume();
    virtual void pause();
    virtual void step(int cnt = 1);
    virtual void single_step() = 0;
    virtual void reset(InterruptState) = 0;
    virtual void reset()
        { reset(INTERRUPT_PULSE); }
    virtual void NMI(InterruptState p_is = INTERRUPT_PULSE) = 0;
    virtual void IRQ(InterruptState p_is = INTERRUPT_PULSE) = 0;

    virtual void serialize(std::ostream &) const;
};

class MCS6502
    : public Cpu
{
public:
    class Configurator
        : public virtual Cpu::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual const Memory::Configurator *memory() const = 0;
        virtual Device *device_factory() const
            { return new MCS6502(*this); }
        virtual void serialize(std::ostream &) const;
    };
    class Instruction; // Forward declaration
    // Attributes
public:
    Memory *m_memory;
private:
    std::array<Instruction *, 256> m_opcode_mapping;
public:
    static log4cxx::LoggerPtr log()
        {
            static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(INSTRUCTION_PREFIX ".mcs6502"));
            return result;
        }
public:
    volatile int m_InterruptSource;
    struct {
        word PC;
        byte A;
        byte S;
        byte X;
        byte Y;
        byte P;
    } m_register;
    // Methods
public:
    explicit MCS6502(const Configurator &);
    virtual ~MCS6502();
    virtual void remove_child(Part &);

    virtual void reset(InterruptState p_is);
    virtual void NMI(InterruptState p_is = INTERRUPT_PULSE);
    virtual void IRQ(InterruptState p_is = INTERRUPT_PULSE);
    virtual void single_step();

    virtual void serialize(std::ostream &) const;
};

#endif

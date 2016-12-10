// cpu.hpp
// Copyright 2016 Steve Palmer

#ifndef CPU_HPP_
#define CPU_HPP_

#include <mutex>
#include <thread>
#include <array>

#include "common.hpp"
#include "part.hpp"
#include "memory.hpp"

enum class InterruptState { ON, OFF, PULSE };

extern std::ostream &operator<<(std::ostream&, const InterruptState&);

enum InterruptSource { NO_INTERRUPT          = 0,
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
        virtual void serialize(std::ostream &) const override;
    };
    // Attributes
private:
    static const unsigned int PAUSE;
    static const unsigned int THREAD_DIE;
    static const unsigned int FOREVER;
    mutable std::recursive_mutex m_mutex;
    unsigned int                 m_steps_to_go;
    bool                         m_is_stepping;
    std::thread                  m_thread;
    // Methods
private:
    void thread_function();
    Cpu();
protected:
    explicit Cpu(const Configurator &);
    virtual void terminating() override;
    virtual ~Cpu();
public:
    virtual void resume() override;
    virtual void pause() override;
    virtual bool is_paused() const override;
    virtual void step(int cnt = 1);
    virtual void single_step() = 0;
    virtual void reset(InterruptState) = 0;
    virtual void reset() override
        { reset(InterruptState::PULSE); }
    virtual void NMI(InterruptState p_is = InterruptState::PULSE) = 0;
    virtual void IRQ(InterruptState p_is = InterruptState::PULSE) = 0;

    virtual void serialize(std::ostream &) const override;
};

class MCS6502
    : public Cpu
{
public:
#if 0
    class SetLogLevel
        : public Hook
    {
    private:
        const log4cxx::LevelPtr level;
        SetLogLevel();
    public:
        SetLogLevel(word addr, const MCS6502 &mcs6502, log4cxx::LevelPtr p_level)
            : Hook()
            , level(p_level)
            {
                AddressSpace *as = mcs6502.address_space();
                assert (as);
                as->add_child(addr, *this);
            }
    private:
        virtual int get_byte_hook(word, AccessType p_at) override
            {
                if (p_at == AT_INSTRUCTION)
                {
                    MCS6502::log()->setLevel(level);
                }
                return -1;
            }
    };
#endif
    class Configurator
        : public virtual Cpu::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual const Memory::Configurator *memory() const = 0;
        virtual Device *device_factory() const override
            { return new MCS6502(*this); }
        virtual void serialize(std::ostream &) const override;
    };
    class Instruction;
    // Attributes
public:
    Memory                         *m_memory;
private:
    int                            m_cycles;
    class Instruction *            m_undefined_instruction;
    std::array<Instruction *, 256> m_opcode_mapping;
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
    static log4cxx::LoggerPtr log()
        {
            static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(INSTRUCTION_PREFIX ".mcs6502"));
            return result;
        }
    explicit MCS6502(const Configurator &);
private:
    MCS6502();
    virtual ~MCS6502();
public:
    AddressSpace *address_space() const { return dynamic_cast<AddressSpace *>(m_memory); }
    virtual void reset(InterruptState p_is) override;
    virtual void NMI(InterruptState p_is = InterruptState::PULSE) override;
    virtual void IRQ(InterruptState p_is = InterruptState::PULSE) override;
    virtual void single_step() override;

    virtual void serialize(std::ostream &) const override;
};

#endif

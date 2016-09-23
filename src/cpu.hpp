/*
 * cpu.hpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#ifndef CPU_HPP_
#define CPU_HPP_

#include <pthread.h>

#include <ostream>
#include <string>
#include <array>
#include <memory>
#include <atomic>

#include "common.hpp"
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

class Core : public Named {
    // Types
public:
    class Configurator : public Named::Configurator
    {
    public:
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    pthread_t        m_thread;
public:
    friend void      *loop(void *);
    Memory           &m_memory;
    std::atomic_uint m_steps_to_go;
    int              m_cycles;
    // Methods
private:
    void operator=(const Core&);
    Core(const Core &);
protected:
    Core(Memory &, const Configurator &);
    void start();
    void stop();
public:
    virtual ~Core();
    void resume();
    void pause();
    void step(int cnt = 1);
    virtual void single_step() = 0;
    virtual void reset(InterruptState p_is = INTERRUPT_PULSE) = 0;
    virtual void NMI(InterruptState p_is = INTERRUPT_PULSE) = 0;
    virtual void IRQ(InterruptState p_is = INTERRUPT_PULSE) = 0;

    friend std::ostream &::operator<<(std::ostream&, const Core&);
};

class MCS6502 : public Core {
public:
    class Configurator : public Core::Configurator
    {
    public:
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    class Instruction; // Forward declaration
    // Attributes
private:
    std::vector<std::shared_ptr<Instruction>> m_opcode_mapping;
public:
#if EXEC_TRACE
    log4c_category_t *m_6502tracelog;
    log4c_appender_t *m_6502traceapp;
    log4c_layout_t   *m_6502tracelay;
#endif
#if JUMP_TRACE
    log4c_category_t *m_jumptracelog;
private:
    int m_trace_finish_priority;
    unsigned int m_trace_start_count;
#endif
public:
    volatile int        m_InterruptSource;
    struct {
        word PC;
        byte A;
        byte S;
        byte X;
        byte Y;
        byte P;
    } m_register;
    // Methods
private:
    void interrupt(word p_addr);
    void construct();
public:
    explicit MCS6502(Memory &, const Configurator &);
    virtual ~MCS6502();
    virtual void reset(InterruptState p_is = INTERRUPT_PULSE);
    virtual void NMI  (InterruptState p_is = INTERRUPT_PULSE);
    virtual void IRQ  (InterruptState p_is = INTERRUPT_PULSE);
    virtual void single_step();
#if 0
    void trace_start();
    void trace_finish();
#endif

    friend std::ostream &::operator<<(std::ostream&, const MCS6502&);
};

#endif /* CPU_HPP_ */

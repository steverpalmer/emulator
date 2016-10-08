// atom.hpp

#ifndef ATOM_HPP_
#define ATOM_HPP_

#include <memory>
#include <ostream>
#include <vector>
#include <list>

#include "common.hpp"
#include "terminal_interface.hpp"
#include "device.hpp"
#include "ppia.hpp"
#include "cpu.hpp"

class Atom
    : public virtual Part
{
    // Types
public:
    class Configurator : public Part::Configurator
    {
    public:
        virtual const Memory::Configurator  &memory()          const = 0;
        virtual const MCS6502::Configurator &mcs6502()         const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::list<std::shared_ptr<Device>> m_devices;
    std::shared_ptr<Ppia> m_ppia;
    Memory                m_memory;
    MCS6502               m_6502;
#if 0
public:
    FILE         *m_OSWRCH_stream;
    FILE         *m_OSRDCH_stream;
    bool          m_OSRDCH_fallback;
#endif
    // Methods
public:
    explicit Atom(const Configurator &p_acfg);
    virtual ~Atom();

    // Wrappers on MCS6502
    int     cycles() const { return m_6502.m_cycles; }
    void    reset();
    void    NMI() { m_6502.NMI(); }
    void    IRQ() { m_6502.IRQ(); }
    void    step(int cnt=1) { m_6502.step(cnt); }
    void    resume() { m_6502.resume(); }
    void    pause() { m_6502.pause(); }

    friend std::ostream &::operator<<(std::ostream&, const Atom&);
};

#endif /* ATOM_HPP_ */

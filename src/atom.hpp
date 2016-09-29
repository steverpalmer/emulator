/*
 * atom.hpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

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

class Atom : public Part, public TerminalInterface {
    // Types
public:
    class Configurator : public Part::Configurator
    {
    public:
        virtual const Device::Configurator  *device(int i)     const = 0;
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
    int     cycles() const;
    void    reset();
    void    NMI();
    void    IRQ();
    void    step(int cnt=1);
    void    resume();
    void    pause();

    VDGMode vdg_mode() const
        { return m_ppia->vdg_mode(); }
    void set_vdg_refresh(bool p_flag)
        { m_ppia->set_vdg_refresh(p_flag); }
    void set_keypress(int p_key)
        { m_ppia->set_keypress(p_key); }
    void set_is_shift_pressed(bool p_flag)
        { m_ppia->set_is_shift_pressed(p_flag); }
    void set_is_ctrl_pressed(bool p_flag)
        { m_ppia->set_is_ctrl_pressed(p_flag); }
    void set_is_rept_pressed(bool p_flag)
        { m_ppia->set_is_rept_pressed(p_flag); }

    friend std::ostream &::operator<<(std::ostream&, const Atom&);
};

#endif /* ATOM_HPP_ */

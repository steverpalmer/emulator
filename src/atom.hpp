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
#include "memory.hpp"
#include "ppia.hpp"
#include "cpu.hpp"

class Atom : public Named {
    // Types
public:
    class Configurator : public Named::Configurator
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
    std::shared_ptr<Ram>  m_video_ram;
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

    VDGMode                 vdg_mode() const;
    const std::shared_ptr<Ram> &vdg_memory() const;
    void                    set_vdg_refresh(bool);
    void                    set_keypress(int key);
    void                    set_is_shift_pressed(bool);
    void                    set_is_ctrl_pressed(bool);
    void                    set_is_rept_pressed(bool);

    friend std::ostream &::operator<<(std::ostream&, const Atom&);
};

#endif /* ATOM_HPP_ */

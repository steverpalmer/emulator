/*
 * atom.cpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#include <cassert>
#include <csignal> // used to indicate the end of streaming input

#include <ostream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "atom.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".atom.cpp"));
    return result;
}

Atom::Atom(const Configurator &p_cfg)
    : Named    ("atom")
    , m_devices(p_cfg.devices().size())
    , m_memory ( p_cfg.memory() )
    , m_6502   ( m_memory, p_cfg.mcs6502() )
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Atom(" << p_cfg << ")");
    const std::vector<const Device::Configurator *> &d_cfg(p_cfg.devices());
    for (unsigned int i(0); i < d_cfg.size(); i++) {
        m_devices[i] = std::shared_ptr<Device>(d_cfg[i]->factory());
        m_memory.add_device(d_cfg[i]->base(), m_devices[i], d_cfg[i]->memory_size());
        if (m_devices[i]->name() == "video")
            m_video_storage = &dynamic_cast<Ram * >(m_devices[i].operator->())->m_storage;
        else if (m_devices[i]->name() == "ppia")
            m_ppia  = std::shared_ptr<Ppia>(dynamic_cast<Ppia *>(m_devices[i].operator->()));
    }
    reset();
}

Atom::~Atom()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].~Atom()");
}

int Atom::cycles() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].cycles()");
    return m_6502.m_cycles;
}

void Atom::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].reset()");
    for (std::shared_ptr<Device> d : m_devices)
        d->reset();
    m_memory.reset();
    m_6502.reset();
}

void Atom::NMI()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].NMI()");
    m_6502.NMI();
}

void Atom::IRQ()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].IRQ()");
    m_6502.IRQ();
}

void Atom::step(int p_cnt)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].step(" << p_cnt << ")");
    m_6502.step(p_cnt);
}

void Atom::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].resume()");
    m_6502.resume();
}

void Atom::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].pause()");
    m_6502.pause();
}

VDGMode Atom::vdg_mode() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].vdg_mode()");
    return m_ppia->m_io.m_vdg_mode;
}

const std::vector<byte> &Atom::vdg_storage() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].vdg_storage()");
    return *m_video_storage;
}

void Atom::set_vdg_refresh(bool p_is_refresh)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_vdg_refresh(" << p_is_refresh << ")");
    m_ppia->m_io.m_is_vdg_refresh = p_is_refresh;
}

void Atom::set_keypress(int p_key)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_keypress(" << p_key << ")");
    m_ppia->m_io.m_pressed_key = p_key;
}

void Atom::set_is_shift_pressed(bool p_is_shift_pressed)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_is_shift_pressed(" << p_is_shift_pressed << ")");
    m_ppia->m_io.m_is_shift_pressed = p_is_shift_pressed;
}

void Atom::set_is_ctrl_pressed(bool p_is_ctrl_pressed)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_is_ctrl_pressed(" << p_is_ctrl_pressed << ")");
    m_ppia->m_io.m_is_ctrl_pressed = p_is_ctrl_pressed;
}

void Atom::set_is_rept_pressed(bool p_is_rept_pressed)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_is_rept_pressed(" << p_is_rept_pressed << ")");
    m_ppia->m_io.m_is_rept_pressed = p_is_rept_pressed;
}

std::ostream &operator<<(std::ostream &p_s, const Atom::Configurator &p_cfg)
{
    for (const Device::Configurator * d : p_cfg.devices())
        p_s << "(" << *d << "), ";
    p_s << "(" << p_cfg.memory() << "), ";
    p_s << "(" << p_cfg.mcs6502() << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Atom &p_atom)
{
    p_s << p_atom.m_memory
        << p_atom.m_6502;
    return p_s;
}

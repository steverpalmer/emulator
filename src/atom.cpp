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
    : Part(p_cfg)
    , m_memory(p_cfg.memory())
    , m_6502(m_memory, p_cfg.mcs6502())
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Atom(" << p_cfg << ")");
#if 0
    const Device::Configurator *d_cfg;
    for (int i(0); (d_cfg = p_cfg.device(i)); i++)
    {
    	std::shared_ptr<Device> device(d_cfg->factory());
    	m_devices.push_back(device);
    	m_memory.add_device(d_cfg->base(), device, d_cfg->memory_size());
        if (device->id() == "video")
        	m_video_ram = std::shared_ptr<Ram>(dynamic_cast<Ram *>(device.operator->()));
        else if (device->id() == "ppia")
            m_ppia  = std::shared_ptr<Ppia>(dynamic_cast<Ppia *>(device.operator->()));
    }
#endif
    reset();
}

Atom::~Atom()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].~Atom()");
    m_memory.drop_devices();
}

int Atom::cycles() const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].cycles()");
    return m_6502.m_cycles;
}

void Atom::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].reset()");
    m_memory.reset();
    m_6502.reset();
}

void Atom::NMI()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].NMI()");
    m_6502.NMI();
}

void Atom::IRQ()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].IRQ()");
    m_6502.IRQ();
}

void Atom::step(int p_cnt)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].step(" << p_cnt << ")");
    m_6502.step(p_cnt);
}

void Atom::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].resume()");
    m_6502.resume();
}

void Atom::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].pause()");
    m_6502.pause();
}

std::ostream &operator<<(std::ostream &p_s, const Atom::Configurator &p_cfg)
{
    p_s << "Atom::Configurator([";
    const Device::Configurator *d_cfg;
    for (int i(0); (d_cfg = p_cfg.device(i)); i++)
    	p_s << "(" << i << ":" << *d_cfg << "), ";
    p_s << "], (" << p_cfg.memory() << "), ";
    p_s << "(" << p_cfg.mcs6502() << ")";
    p_s << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Atom &p_atom)
{
    p_s << p_atom.m_memory
        << p_atom.m_6502;
    return p_s;
}

// atom.cpp

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

void Atom::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].reset()");
    m_memory.reset();
    m_6502.reset();
}

std::ostream &operator<<(std::ostream &p_s, const Atom::Configurator &p_cfg)
{
    return p_s << "<atom " << static_cast<const Part::Configurator &>(p_cfg) << ">"
               << p_cfg.memory()
               << p_cfg.mcs6502();
}

std::ostream &operator<<(std::ostream &p_s, const Atom &p_atom)
{
    return p_s << "Atom("
               << "Memory(" << p_atom.m_memory << ")"
               << ", 6502(" << p_atom.m_6502 << ")";
}

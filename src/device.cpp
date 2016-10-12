// device.cpp

#include <cassert>
#include <ostream>
#include <fstream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "device.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".device.cpp"));
    return result;
}

// Device Methods

Device::Device(const Configurator &p_cfg)
    : Part(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Device::Device(" << p_cfg << ")");
}

Device::~Device()
{
    for (auto *p: m_parents)
        p->remove_child(this);
    m_parents.clear();
}

// Computer Methods

Computer::Computer(const Configurator &p_cfg)
    : Device(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Computer::Computer(" << p_cfg << ")");
    for (int i(0); const Device::Configurator *cfg = p_cfg.device(i); i++)
    {
        Device * const dev(cfg->device_factory());
        add_child(dev);
        dev->add_parent(this);
    }
}

void Computer::clear()
{
    for (auto *d: m_children)
        d->remove_parent(this);
    m_children.clear();
}

Computer::~Computer()
{
    clear();
}

void Computer::reset()
{
    for (auto &dev : m_children)
        dev->reset();
}

// Streaming Output

std::ostream &operator<<(std::ostream &p_s, const Device::Configurator &p_cfg)
{
    return p_s << static_cast<const Part::Configurator &>(p_cfg);
}

std::ostream &operator<<(std::ostream &p_s, const Computer::Configurator &p_cfg)
{
    p_s << "<computer " << static_cast<const Device::Configurator &>(p_cfg) << ">";
    for (int i(0); Device::Configurator *cfg = p_cfg.device(i); i++)
        p_s << *cfg;
    return p_s << "</computer>";
}

std::ostream &operator<<(std::ostream &p_s, const Device &p_dev)
{
    return p_s << static_cast<const Part &>(p_dev);
}

std::ostream &operator<<(std::ostream &p_s, const Computer &p_computer)
{
    p_s << "Computer(";
    for (const auto &dev : p_computer.m_children)
        p_s << dev;
    return p_s << ")";
}

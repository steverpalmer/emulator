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
    LOG4CXX_INFO(cpptrace_log(), "Device::~Device([" << id() << "])");
    for (auto *p: m_parents)
    {
        Computer *computer(dynamic_cast<Computer *>(p));
        if (computer)
            computer->remove_child(this);
    }
    m_parents.clear();
}

// Computer Methods

Computer::Computer(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Computer::Computer(" << p_cfgr << ")");
    for (int i(0); const Device::Configurator *cfgr = p_cfgr.device(i); i++)
    {
        Device * const device(cfgr->device_factory());
        add_child(device);
        device->add_parent(this);
    }
}

void Computer::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::clear()");
    for (auto *device: m_children)
        device->remove_parent(this);
    m_children.clear();
}

Computer::~Computer()
{
    LOG4CXX_INFO(cpptrace_log(), "Computer::~Computer([" << id() << "])");
    clear();
}

void Computer::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::reset()");
    for (auto &device : m_children)
        device->reset();
}

void Computer::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::pause()");
    for (auto &device : m_children)
        device->pause();
}

void Computer::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::resume()");
    for (auto &device : m_children)
        device->resume();
}


// Streaming Output

std::ostream &operator<<(std::ostream &p_s, const Device::Configurator &p_cfgr)
{
    return p_s << static_cast<const Part::Configurator &>(p_cfgr);
}

std::ostream &operator<<(std::ostream &p_s, const Computer::Configurator &p_cfgr)
{
    p_s << "<computer " << static_cast<const Device::Configurator &>(p_cfgr) << ">";
    for (int i(0); Device::Configurator *cfgr = p_cfgr.device(i); i++)
        p_s << *cfgr;
    return p_s << "</computer>";
}

std::ostream &operator<<(std::ostream &p_s, const Device &p_device)
{
    return p_s << static_cast<const Part &>(p_device);
}

std::ostream &operator<<(std::ostream &p_s, const Computer &p_computer)
{
    p_s << "Computer(";
    for (const auto &device : p_computer.m_children)
        p_s << device;
    return p_s << ")";
}

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
        LOG4CXX_DEBUG(cpptrace_log(), "[" << p->id() << "].remove_child([" << id() << "])");
        p->remove_child(this);
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
     }
}

void Computer::add_child(Device *p_device)
{
    (void) m_children.insert(p_device);
    p_device->add_parent(this);
}

void Computer::remove_child(Device *p_device)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::remove_child([" << p_device->id() << "])");
    (void) m_children.erase(p_device);
    p_device->remove_parent(this);
}

void Computer::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::clear()");
    for (auto *device: m_children)
        remove_child(device);
    assert (m_children.empty());
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

void Device::serialize(std::ostream &p_s) const
{
    Part::serialize(p_s);
    p_s << "Parents(";
    for (auto *d : m_parents)
        p_s << d->id() << ", ";
    p_s << ")";
}

void Computer::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<computer ";
    Device::Configurator::serialize(p_s);
    p_s << ">";
    for (int i(0); const Device::Configurator *cfgr = device(i); i++)
        cfgr->serialize(p_s);
    p_s << "</computer>";
}

void Computer::serialize(std::ostream &p_s) const
{
    p_s << "Computer(";
    Device::serialize(p_s);
    for (auto *device : m_children)
#if 0
        device->serialize(p_s);
#else
        p_s << "Device(" << device->id() << ")";
#endif
    p_s << ")";
}

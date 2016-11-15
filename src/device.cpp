// device.cpp
// Copyright 2016 Steve Palmer

#include <cassert>
#include <fstream>
#include <iomanip>

#include "device.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".device.cpp"));
    return result;
}

// Device Methods

Part *Device::ReferenceConfigurator::part_factory() const
{
    return Part::ReferenceConfigurator::part_factory();
}

Device *Device::ReferenceConfigurator::device_factory() const
{
    return dynamic_cast<Device *>(part_factory());
}

Device::Device(const Configurator &p_cfg)
    : Part(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Device::Device(" << p_cfg << ")");
}

Device::Device()
    : Part()
{
    LOG4CXX_INFO(cpptrace_log(), "Device::Device()");
}

Device::~Device()
{
    LOG4CXX_INFO(cpptrace_log(), "Device::~Device([" << id() << "])");
}

// Computer Methods

Computer::Computer(const Configurator &p_cfgr)
    : Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Computer::Computer(" << p_cfgr << ")");
    for (int i(0); auto cfgr = p_cfgr.device(i); i++)
    {
        auto device = cfgr->device_factory();
        assert (device);
        add_child(*device);
    }
}

void Computer::add_child(Device &p_device)
{
    LOG4CXX_INFO(Part::log(), "making [" << p_device.id() << "] child of [" << id() << "]");
    (void) m_children.insert(&p_device);
}

void Computer::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::clear()");
    assert (is_paused());
    LOG4CXX_INFO(Part::log(), "removing all children of [" << id() << "]");
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
    for (auto device : m_children)
        device->reset();
}

void Computer::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::pause()");
    for (auto device : m_children)
        device->pause();
}

void Computer::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Computer::resume()");
    for (auto device : m_children)
        device->resume();
}

bool Computer::is_paused() const
{
    bool result = true;
    LOG4CXX_DEBUG(cpptrace_log(), "[" << id() << "].Computer::is_paused()");
    for (auto device : m_children)
        result &= device->is_paused();
    return result;
}


// Streaming Output

void Device::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << id() << " [color=red];\n";
#else
    Part::Configurator::serialize(p_s);
#endif
}

void Device::ReferenceConfigurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << m_ref_id << " [color=red];\n";
#else
    p_s << "<device href=\"" << m_ref_id << "\"/>";
#endif
}

void Computer::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Device::Configurator::serialize(p_s);
    for (int i(0); auto cfgr = device(i); i++)
    {
        p_s << id() << " -> " << cfgr->id() << ";\n";
        cfgr->serialize(p_s);
    }
#else
    p_s << "<computer ";
    Device::Configurator::serialize(p_s);
    p_s << ">";
    for (int i(0); auto cfgr = device(i); i++)
        cfgr->serialize(p_s);
    p_s << "</computer>";
#endif
}

void Device::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << id() << " [color=red];\n";
#else
    Part::serialize(p_s);
#endif
}

void Computer::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Device::serialize(p_s);
    for (auto device : m_children)
        p_s << id() << " -> " << device->id() << ";\n";
#else
    p_s << "Computer(";
    Device::serialize(p_s);
    for (auto device : m_children)
        p_s << device->id() << ", ";
    p_s << ")";
#endif
}

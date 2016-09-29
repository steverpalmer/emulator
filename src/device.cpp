/*
 * device.cpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#include <cassert>
#include <ostream>
#include <fstream>
#include <iomanip>
//#include <algorithm>

#include <log4cxx/logger.h>

#include "memory.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".memory.cpp"));
    return result;
}

// Device helper functions

Device::Device(const Configurator &p_cfg)
    : Named(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Device::Device(" << p_cfg << ")");
}

word Device::get_word(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_word(" << Hex(p_addr) << ", " << p_at << ")");
    const word low_byte(get_byte(p_addr, p_at));
    const word high_byte(get_byte((p_addr+1)==m_storage.size()?0:(p_addr+1), p_at));
    return low_byte | high_byte << 8;
}

void Device::set_word(word p_addr, word p_word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_word(" << Hex(p_addr) << ", " << Hex(p_word) << ", " << p_at << ")");
    set_byte(p_addr, byte(p_word), p_at);
    set_byte((p_addr+1)==m_storage.size()?0:(p_addr+1), byte(p_word >> 8), p_at);
}

// Ram_Rom_Helper Methods

bool Ram_Rom_Helper::load(const std::string &p_filename)
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::in | fstream::binary);
        if (result = (file.rdstate()  & std::ifstream::goodbit) )
            for (byte &b : m_storage)
                b = file.get();
    }
    result result
}

bool Ram_Rom_Helper::save(const std::string &p_filename) const
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::out | fstream::binary | fstream::trunc);
        if (result = (file.rdstate()  & std::ifstream::goodbit) )
            for (const byte b : m_storage)
                file.put(b);
    }
    return result;
}

// Ram Methods

Ram::Ram(const Configurator &p_cfg)
    : Device(p_cfg)
    , m_memory(p_cfg.size())
    , m_filename(p_cfg.filename())
{
    LOG4CXX_INFO(cpptrace_log(), "Ram::Ram(" << p_cfg << ")");
    (void) m_memory.load(m_filename);
}

Ram::~Ram()
{
    (void) m_memory.save(m_filename);
}

// Rom Methods

Rom::Rom(const Configurator &p_cfg)
    : Device(p_cfg)
    , helper(p_cfg.size())
{
    LOG4CXX_INFO(cpptrace_log(), "Rom::Rom(" << p_cfg << ")");
    assert (load(p_cfg.filename()))
}

Rom::~Rom()
{
    // Do Nothing
}

#if 0
// Hook Methods

Hook::Hook(const Configurator &p_cfg)
    : Device(p_cfg)
    , m_base(0)
    , m_device(0)
{
    LOG4CXX_INFO(cpptrace_log(), "Hook::Hook(" << p_cfg << ")");
}

Hook::~Hook()
{
	m_device.reset();
}

byte Hook::get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    int result(execute(p_addr+m_base, -1, p_at));
    if (result < 0)
    {
        assert (m_device);
        result = m_device->get_byte(p_addr+m_base, p_at);
    }
    return byte(result);
}

void Hook::set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    if (execute(p_addr+m_base, p_byte, p_at))
    {
        assert m_device;
        m_device->set_byte(p_addr+m_base, p_byte, p_at);
    }
}
#endif

// Memory Methods

Memory::Memory(const Configurator &p_cfg)
    : Device(p_cfg)
    , m_device(0)
    , m_base(SIZE(p_cfg.size()), 0)
    , m_map(SIZE(p_cfg.size()), 0)
{
    LOG4CXX_INFO(cpptrace_log(), "Memory::Memory(" << p_cfg << ")");
    const Device::Configurator::mapping *mapping;
    for (int i(0); (mapping = p_cfg.device(i)); i++)
    {
    	std::shared_ptr<Device> device(mapping->device.factory());
    	m_devices.push_back(device);
    	m_memory.add_device(mapping->base, device, mapping->size);
    }
}

Memory::reset()
{
    for (std::shared_ptr<Device> d : m_devices)
        d->reset();
}

byte Memory::get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    return m_map[p_addr] ? m_map[p_addr]->get_byte(p_addr-m_base[p_addr], p_at) : 0;
}

void Memory::set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    if (m_map[p_addr])
    	m_map[p_addr]->set_byte(p_addr-m_base[p_addr], p_byte, p_at);
}

void Memory::add_device(word p_base, std::shared_ptr<Device> p_device, word p_size)
{
    assert (p_device);
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].add_device(" << Hex(p_base) << ", [" << p_device->name() << "], " << Hex(p_size) << ")");
    const int device_size(SIZE(p_device->size()));
    assert (device_size > 0);
    assert (!p_size || (p_size % device_size == 0));
    const int memory_size(p_size?p_size:device_size);
    for (int offset = 0; offset < memory_size; offset++)
    {
        m_base[p_base + offset] = p_base + (offset / device_size) * device_size;
        m_map[p_base + offset] = p_device;
    }
}

std::shared_ptr<Device> Memory::get_device(word p_addr) const
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].get_device(" << Hex(p_addr) << ")");
    return m_map[p_addr];
}

void Memory::drop_devices()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].drop_devices(" << ")");
    for (int i(0); i < 65536; i++)
    {
    	m_map[i].reset();
    	m_base[i] = 0;
    }
    m_devices.clear();
}

#if 0
void Memory::add_hook(word p_base, std::shared_ptr<Hook> p_hook, word p_size)
{
    assert (p_hook);
    LOG4CXX_INFO(cpptrace_log(), "[" << name() << "].add_hook(" << Hex(p_base) << ", [" << p_hook->name() << "], " << Hex(p_size) << ")");
    const int hook_size(SIZE(p_hook->size()));
    assert (!p_size || (p_size % hook_size == 0));
    const int memory_size(p_size?p_size:hook_size);
    p_hook->m_base = m_base[p_base];
    p_hook->m_device = m_map[p_base];
    m_map[p_base] = p_hook;
    for (int offset = 1; offset < p_size; offset++) {
    	assert (m_map[p_base + offset] == p_hook->m_device);
    	m_map[p_base + offset] = p_hook;
    }
}
#endif

std::ostream &operator<<(std::ostream &p_s, const Device::Configurator &p_cfg)
{
    p_s << "Device::Configurator("
        << static_cast<const Named::Configurator &>(p_cfg)
        << ", base=" << Hex(p_cfg.base())
        << ", memory_size=" << Hex(p_cfg.memory_size())
        << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Ram::Configurator &p_cfg)
{
    p_s << "Ram::Configurator("
        << static_cast<const Device::Configurator &>(p_cfg)
        << ", size=" << Hex(p_cfg.size())
        << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Rom::Configurator &p_cfg)
{
    p_s << "Rom::Configurator("
        << static_cast<const Ram::Configurator &>(p_cfg)
        << ", filename=\"" << p_cfg.filename()
        << "\")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Hook::Configurator &p_cfg)
{
    p_s << "Hook::Configurator("
        << static_cast<const Device::Configurator &>(p_cfg)
        << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Memory::Configurator &p_cfg)
{
    p_s << "Memory::Configurator("
        << static_cast<const Device::Configurator &>(p_cfg)
        << ")";
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const AccessType p_at)
{
    if (p_at >= 0 && p_at < AT_LAST) {
        static const char *name[AT_LAST] = {
            "unknown",
            "instruction",
            "operand",
            "data"
        };
        p_s << name[p_at];
    }
    else
        p_s << int(p_at);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Device &p_dev)
{
    p_s << static_cast<const Named &>(p_dev)
        << ", size:" << Hex(p_dev.size());
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Ram &p_ram)
{
    p_s << static_cast<const Device &>(p_ram)
        << ", data:" << std::endl;
    for (word addr(0); addr < p_ram.size();) {
        p_s << Hex(static_cast<word>(addr)) << ':';
        do {
            p_s << " " << Hex(p_ram.m_storage[addr]);
            addr++;
        }
        while (addr & 0x1F);
        p_s << std::endl;
    }
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Hook &p_hook)
{
    p_s << static_cast<const Device &>(p_hook)
        << p_hook.m_device;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Rom &p_rom)
{
    p_s << static_cast<const Ram &>(p_rom)
        << ", writeable:" << p_rom.m_is_writeable;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Memory &p_memory)
{
    p_s << static_cast<const Device &>(p_memory) << std::endl;
    std::shared_ptr<Device> previous_device;
    for (int addr=0; addr<0x10000; addr++) {
        std::shared_ptr<Device> cell(p_memory.m_map[addr]);
        if (cell != previous_device) {
            p_s << word(addr)
                << ": ***********************************************************************************************"
                << std::endl;
            previous_device = cell;
            if (previous_device)
                p_s << *previous_device;
        }
    }
    return p_s;
}

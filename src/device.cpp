// device.cpp

#include <cassert>
#include <ostream>
#include <fstream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "device.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".memory.cpp"));
    return result;
}

// Device helper functions

Device::Device(const Configurator &p_cfg)
    : Part(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Device::Device(" << p_cfg << ")");
}

word Device::get_word(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].get_word(" << Hex(p_addr) << ", " << p_at << ")");
    const word low_byte(get_byte(p_addr, p_at));
    const word high_byte(get_byte(((p_addr+1)==size())?0:(p_addr+1), p_at));
    return low_byte | high_byte << 8;
}

void Device::set_word(word p_addr, word p_word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].set_word(" << Hex(p_addr) << ", " << Hex(p_word) << ", " << p_at << ")");
    set_byte(p_addr, byte(p_word), p_at);
    set_byte(((p_addr+1)==size())?0:(p_addr+1), byte(p_word >> 8), p_at);
}

// SimpleMemory Methods

bool SimpleMemory::load(const Glib::ustring &p_filename)
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::in | fstream::binary);
        if ((result = (file.rdstate()  & std::ifstream::goodbit) ))
            for (byte &b : m_storage)
                b = file.get();
    }
    return result;
}

bool SimpleMemory::save(const Glib::ustring &p_filename) const
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::out | fstream::binary | fstream::trunc);
        if ((result = (file.rdstate()  & std::ifstream::goodbit) ))
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
    , m_memory(p_cfg.size())
{
    LOG4CXX_INFO(cpptrace_log(), "Rom::Rom(" << p_cfg << ")");
    assert (m_memory.load(p_cfg.filename()));
}

Rom::~Rom()
{
    // Do Nothing
}

// Memory Methods

Memory::Memory(const Configurator &p_cfg)
    : Device(p_cfg)
    , m_devices(0)
    , m_base(SIZE(p_cfg.size()), 0)
    , m_map(SIZE(p_cfg.size()), 0)
{
    LOG4CXX_INFO(cpptrace_log(), "Memory::Memory(" << p_cfg << ")");
    Memory::Configurator::mapping map;
    for (int i(0); (map = p_cfg.device(i), map.device); i++)
    {
        std::shared_ptr<Device> device(map.device->factory());
        m_devices.push_back(device);
        add_device(map.base, device, map.size);
    }
}

void Memory::_reset()
{
    for (std::shared_ptr<Device> d : m_devices)
        d->reset();
}

byte Memory::_get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]._get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    return m_map[p_addr] ? m_map[p_addr]->get_byte(p_addr-m_base[p_addr], p_at) : 0;
}

void Memory::_set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]._set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    if (m_map[p_addr])
    	m_map[p_addr]->set_byte(p_addr-m_base[p_addr], p_byte, p_at);
}

void Memory::add_device(word p_base, std::shared_ptr<Device> p_device, word p_size)
{
    assert (p_device);
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].add_device(" << Hex(p_base) << ", [" << p_device->id() << "], " << Hex(p_size) << ")");
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

void Memory::drop_devices()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].drop_devices(" << ")");
    for (int i(0); i < 65536; i++)
    {
    	m_map[i].reset();
    	m_base[i] = 0;
    }
    m_devices.clear();
}

// Streaming Output

std::ostream &operator<<(std::ostream &p_s, const Device::Configurator &p_cfg)
{
    return p_s << static_cast<const Part::Configurator &>(p_cfg);
}

std::ostream &operator<<(std::ostream &p_s, const Ram::Configurator &p_cfg)
{
    return p_s << "<Ram " << static_cast<const Device::Configurator &>(p_cfg) << ">"
               << "<size>" << Hex(p_cfg.size()) << "</size>"
               << "<filename>" << p_cfg.filename() << "</filename>"
               << "</Ram>";
}

std::ostream &operator<<(std::ostream &p_s, const Rom::Configurator &p_cfg)
{
    return p_s << "<Rom " << static_cast<const Device::Configurator &>(p_cfg) << ">"
               << "<filename>" << p_cfg.filename() << "</filename>"
               << "<size>" << Hex(p_cfg.size()) << "</size>"
               << "</Rom>";
}

std::ostream &operator<<(std::ostream &p_s, const Memory::Configurator &p_cfg)
{
    p_s << "<memory " << static_cast<const Device::Configurator &>(p_cfg) << ">"
        << "<size>" << Hex(p_cfg.size()) << "</size>";
    Memory::Configurator::mapping map;
    for (int i(0); (map = p_cfg.device(i), map.device); i++)
        p_s << "<map>"
            << "<base>" << Hex(map.base) << "</base>"
            << map.device
            << "<size>" << Hex(map.size) << "</size>"
            << "</map>";
    p_s << "</memory>";
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
    return p_s << static_cast<const Part &>(p_dev)
               << ", size(" << Hex(p_dev.size()) << ")";
}

std::ostream &operator<<(std::ostream &p_s, const SimpleMemory &p_simple)
{
    if (p_simple.size() >= 0x20)
        p_s << std::endl;
    for (word addr(0); addr < p_simple.size();) {
        p_s << Hex(static_cast<word>(addr)) << ':';
        do {
            p_s << " " << Hex(p_simple.m_storage[addr]);
            addr++;
        }
        while (addr & 0x1F);
        p_s << std::endl;
    }
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Ram &p_ram)
{
    p_s << "Ram("
        << static_cast<const Device &>(p_ram)
        << ", memory(" << p_ram.m_memory << ")";
    if (!p_ram.m_filename.empty())
        p_s << ", filename(" << p_ram.m_filename << ")";
    return p_s << ")";
}

std::ostream &operator<<(std::ostream &p_s, const Rom &p_rom)
{
    p_s << "Rom("
        << static_cast<const Device &>(p_rom)
        << ", memory(" << p_rom.m_memory << ")";
    return p_s << ")";
}

#if 0
std::ostream &operator<<(std::ostream &p_s, const Hook &p_hook)
{
    return p_s << static_cast<const Device &>(p_hook)
               << p_hook.m_device;
}
#endif

std::ostream &operator<<(std::ostream &p_s, const Memory &p_memory)
{
    p_s << "Memory("
        << static_cast<const Device &>(p_memory);
    std::shared_ptr<Device> previous_device;
    for (int addr=0; addr<0x10000; addr++) {
        std::shared_ptr<Device> cell(p_memory.m_map[addr]);
        if (cell != previous_device) {
            p_s << Hex(word(addr))
                << ": ***********************************************************************************************"
                << std::endl;
            previous_device = cell;
            if (previous_device)
                p_s << *previous_device;
        }
    }
    return p_s << ")";
}

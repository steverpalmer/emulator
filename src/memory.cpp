// memory.cpp

#include <cassert>
#include <ostream>
#include <fstream>
#include <iomanip>

#include <log4cxx/logger.h>

#include "memory.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".memory.cpp"));
    return result;
}

// Memory

Memory::Memory(const Configurator &p_cfg)
    : Device(p_cfg)
{
    LOG4CXX_INFO(cpptrace_log(), "Memory::Memory(" << p_cfg << ")");
}

Memory::~Memory()
{
    for (auto *p: m_parents)
    {
        AddressSpace *as(dynamic_cast<AddressSpace *>(p));
        Computer *comp(dynamic_cast<Computer *>(p));
        if (as)   as->remove_child(this);
        if (comp) comp->remove_child(this);
    }
    m_parents.clear();
    m_observers.clear();
}

word Memory::get_word(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].get_word(" << Hex(p_addr) << ", " << p_at << ")");
    const word low_byte(get_byte(p_addr, p_at));
    const word high_byte(get_byte(((p_addr+1)==size())?0:(p_addr+1), p_at));
    return low_byte | high_byte << 8;
}

void Memory::set_word(word p_addr, word p_word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].set_word(" << Hex(p_addr) << ", " << Hex(p_word) << ", " << p_at << ")");
    set_byte(p_addr, byte(p_word), p_at);
    set_byte(((p_addr+1)==size())?0:(p_addr+1), byte(p_word >> 8), p_at);
}

// Storage Methods

bool Storage::load(const Glib::ustring &p_filename)
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::in | fstream::binary);
        if ((result = (file.rdstate()  & ifstream::goodbit) ))
            for (byte &b : m_storage)
                b = file.get();
    }
    return result;
}

bool Storage::save(const Glib::ustring &p_filename) const
{
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        fstream file(p_filename, fstream::out | fstream::binary | fstream::trunc);
        if ((result = (file.rdstate()  & ifstream::goodbit) ))
            for (const byte b : m_storage)
                file.put(b);
    }
    return result;
}

// Ram Methods

Ram::Ram(const Configurator &p_cfg)
    : Device(p_cfg)
    , Memory(p_cfg)
    , m_storage(p_cfg.size())
    , m_filename(p_cfg.filename())
{
    LOG4CXX_INFO(cpptrace_log(), "Ram::Ram(" << p_cfg << ")");
    (void) m_storage.load(m_filename);
}

Ram::~Ram()
{
    (void) m_storage.save(m_filename);
}

// Rom Methods

Rom::Rom(const Configurator &p_cfg)
    : Device(p_cfg)
    , Memory(p_cfg)
    , m_storage(p_cfg.size())
{
    LOG4CXX_INFO(cpptrace_log(), "Rom::Rom(" << p_cfg << ")");
    assert (m_storage.load(p_cfg.filename()));
}

Rom::~Rom()
{
    // Do Nothing
}

// AddressSpace Methods

AddressSpace::AddressSpace(const Configurator &p_cfg)
    : Device(p_cfg)
    , Memory(p_cfg)
    , m_base(SIZE(p_cfg.size()), 0)
    , m_map(SIZE(p_cfg.size()), 0)
{
    LOG4CXX_INFO(cpptrace_log(), "AddressSpace::AddressSpace(" << p_cfg << ")");
    AddressSpace::Configurator::Mapping mapping;
    for (int i(0); (mapping = p_cfg.mapping(i), mapping.memory); i++)
    {
        Memory * const memory(mapping.memory->memory_factory());
        add_child(mapping.base, memory, mapping.size);
        memory->add_parent(this);
    }
}

void AddressSpace::reset()
{
    for (auto &mem : m_children)
        mem->reset();
}

byte AddressSpace::_get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]._get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    return m_map[p_addr] ? m_map[p_addr]->get_byte(p_addr-m_base[p_addr], p_at) : 0;
}

void AddressSpace::_set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "]._set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    if (m_map[p_addr])
    	m_map[p_addr]->set_byte(p_addr-m_base[p_addr], p_byte, p_at);
}

void AddressSpace::add_child(word p_base, Memory *p_memory, word p_size)
{
    assert (p_memory);
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].add(" << Hex(p_base) << ", [" << p_memory->id() << "], " << Hex(p_size) << ")");
    m_children.insert(p_memory);
    const int memory_size(SIZE(p_memory->size()));
    assert (memory_size > 0);
    assert (!p_size || (p_size % memory_size == 0));
    const int memory_space(p_size?p_size:memory_size);
    for (int offset = 0; offset < memory_space; offset++)
    {
        m_base[p_base + offset] = p_base + (offset / memory_size) * memory_size;
        m_map[p_base + offset] = p_memory;
    }
}

void AddressSpace::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].clear(" << ")");
    m_map.clear();
    m_base.clear();
    for (auto *m : m_children)
        m->remove_parent(this);
    m_children.clear();
}

AddressSpace::~AddressSpace()
{
    clear();
}

// Streaming Output

std::ostream &operator<<(std::ostream &p_s, const Memory::Configurator &p_cfg)
{
    return p_s << static_cast<const Device::Configurator &>(p_cfg);
}

std::ostream &operator<<(std::ostream &p_s, const Ram::Configurator &p_cfg)
{
    return p_s << "<ram " << static_cast<const Device::Configurator &>(p_cfg) << ">"
               << "<size>" << Hex(p_cfg.size()) << "</size>"
               << "<filename>" << p_cfg.filename() << "</filename>"
               << "</ram>";
}

std::ostream &operator<<(std::ostream &p_s, const Rom::Configurator &p_cfg)
{
    return p_s << "<rom " << static_cast<const Device::Configurator &>(p_cfg) << ">"
               << "<filename>" << p_cfg.filename() << "</filename>"
               << "<size>" << Hex(p_cfg.size()) << "</size>"
               << "</rom>";
}

std::ostream &operator<<(std::ostream &p_s, const AddressSpace::Configurator &p_cfg)
{
    p_s << "<address_space " << static_cast<const Memory::Configurator &>(p_cfg) << ">"
        << "<size>" << Hex(p_cfg.size()) << "</size>";
    AddressSpace::Configurator::Mapping mapping;
    for (int i(0); (mapping = p_cfg.mapping(i), mapping.memory); i++)
        p_s << "<map>"
            << "<base>" << Hex(mapping.base) << "</base>"
            << mapping.memory
            << "<size>" << Hex(mapping.size) << "</size>"
            << "</map>";
    p_s << "</address_space";
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

std::ostream &operator<<(std::ostream &p_s, const Memory &p_mem)
{
    return p_s << static_cast<const Device &>(p_mem)
               << ", size(" << Hex(p_mem.size()) << ")";
}

std::ostream &operator<<(std::ostream &p_s, const Storage &p_storage)
{
    if (p_storage.size() >= 0x20)
        p_s << std::endl;
    for (word addr(0); addr < p_storage.size();) {
        p_s << Hex(static_cast<word>(addr)) << ':';
        do {
            p_s << " " << Hex(p_storage.m_storage[addr]);
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
        << static_cast<const Memory &>(p_ram)
        << ", memory(" << p_ram.m_storage << ")";
    if (!p_ram.m_filename.empty())
        p_s << ", filename(" << p_ram.m_filename << ")";
    return p_s << ")";
}

std::ostream &operator<<(std::ostream &p_s, const Rom &p_rom)
{
    p_s << "Rom("
        << static_cast<const Memory &>(p_rom)
        << ", memory(" << p_rom.m_storage << ")";
    return p_s << ")";
}

std::ostream &operator<<(std::ostream &p_s, const AddressSpace &p_address_space)
{
    p_s << "AddressSpace("
        << static_cast<const Memory &>(p_address_space);
    Memory *previous_memory;
    for (int addr=0; addr<0x10000; addr++) {
        Memory *cell(p_address_space.m_map[addr]);
        if (cell != previous_memory) {
            p_s << Hex(word(addr))
                << ": ***********************************************************************************************"
                << std::endl;
            previous_memory = cell;
            if (previous_memory)
                p_s << *previous_memory;
        }
    }
    return p_s << ")";
}

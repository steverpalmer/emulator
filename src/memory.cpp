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

Memory::Memory(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , Device(p_cfgr)
{
    LOG4CXX_INFO(cpptrace_log(), "Memory::Memory(" << p_cfgr << ")");
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
    LOG4CXX_INFO(cpptrace_log(), "Storage::load(" << p_filename << ")");
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        ifstream file(p_filename, ifstream::in | ifstream::binary);
        if ((result = file.good()))
        {
            if (m_storage.size() == 0)
            {
                file.seekg(0, ios::beg);
                const std::streampos begin = file.tellg();
                file.seekg(0, ios::end);
                const std::streampos end = file.tellg();
                m_storage.resize(end-begin);
                file.seekg(0, ios::beg);
            }
            for (byte &b : m_storage)
                b = file.get();
        }
    }
    return result;
}

bool Storage::save(const Glib::ustring &p_filename) const
{
    LOG4CXX_INFO(cpptrace_log(), "Storage::save(" << p_filename << ")");
    bool result(true);
    if (!p_filename.empty()) {
        using namespace std;
        ofstream file(p_filename, ofstream::out | ofstream::binary | ofstream::trunc);
        if ((result = file.good()))
            for (const byte b : m_storage)
                file.put(b);
    }
    return result;
}

// Ram Methods

Ram::Ram(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , Device(p_cfgr)
    , Memory(p_cfgr)
    , m_storage(p_cfgr.size())
    , m_filename(p_cfgr.filename())
{
    LOG4CXX_INFO(cpptrace_log(), "Ram::Ram(" << p_cfgr << ")");
    (void) m_storage.load(m_filename);
}

Ram::~Ram()
{
    LOG4CXX_INFO(cpptrace_log(), "Ram::~Ram([" << id() << "])");
    (void) m_storage.save(m_filename);
}

// Rom Methods

Rom::Rom(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , Device(p_cfgr)
    , Memory(p_cfgr)
    , m_storage(p_cfgr.size())
{
    LOG4CXX_INFO(cpptrace_log(), "Rom::Rom(" << p_cfgr << ")");
    assert (m_storage.load(p_cfgr.filename()));
}

Rom::~Rom()
{
    LOG4CXX_INFO(cpptrace_log(), "Rom::~Rom([" << id() << "])");
    // Do Nothing
}

// AddressSpace Methods

AddressSpace::AddressSpace(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , Device(p_cfgr)
    , Memory(p_cfgr)
    , m_base(SIZE(p_cfgr.size()), 0)
    , m_map(SIZE(p_cfgr.size()), 0)
{
    LOG4CXX_INFO(cpptrace_log(), "AddressSpace::AddressSpace(" << p_cfgr << ")");
    AddressSpace::Configurator::Mapping mapping;
    for (int i(0); (mapping = p_cfgr.mapping(i), mapping.memory); i++)
    {
        Memory * const memory(mapping.memory->memory_factory());
        add_child(mapping.base, memory, mapping.size);
        memory->add_parent(this);
    }
}

byte AddressSpace::_get_byte(word p_addr, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::_get_byte(" << Hex(p_addr) << ", " << p_at << ")");
    return m_map[p_addr] ? m_map[p_addr]->get_byte(p_addr-m_base[p_addr], p_at) : 0;
}

void AddressSpace::_set_byte(word p_addr, byte p_byte, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::_set_byte(" << Hex(p_addr) << ", " << Hex(p_byte) << ", " << p_at << ")");
    if (m_map[p_addr])
    	m_map[p_addr]->set_byte(p_addr-m_base[p_addr], p_byte, p_at);
}

void AddressSpace::add_child(word p_base, Memory *p_memory, word p_size)
{
    assert (p_memory);
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::add_child(" << Hex(p_base) << ", [" << p_memory->id() << "], " << Hex(p_size) << ")");
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

void AddressSpace::remove_child(Memory *p_memory)
{
    assert (p_memory);
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::remove_child([" << p_memory->id() << "])");
    m_children.erase(p_memory);
}

void AddressSpace::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::clear()");
    m_map.clear();
    m_base.clear();
    for (auto *m : m_children)
        m->remove_parent(this);
    m_children.clear();
}

AddressSpace::~AddressSpace()
{
    LOG4CXX_INFO(cpptrace_log(), "AddressSpace::~AddressSpace([" << id() << "])");
    clear();
}

void AddressSpace::reset()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::reset()");
    for (auto &mem : m_children)
        mem->reset();
}

void AddressSpace::pause()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::pause()");
    for (auto &mem : m_children)
        mem->pause();
}

void AddressSpace::resume()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].AddressSpace::resume()");
    for (auto &mem : m_children)
        mem->resume();
}


// Streaming Output

void Ram::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<ram ";
    Memory::Configurator::serialize(p_s);
    p_s << ">"
        << "<size>" << Hex(size()) << "</size>"
        << "<filename>" << filename() << "</filename>"
        << "</ram>";
}

void Rom::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<rom ";
    Memory::Configurator::serialize(p_s);
    p_s << ">"
        << "<filename>" << filename() << "</filename>"
        << "<size>" << Hex(size()) << "</size>"
        << "</rom>";
}

void AddressSpace::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<address_space ";
    Memory::Configurator::serialize(p_s);
    p_s << ">"
        << "<size>" << Hex(size()) << "</size>";
    AddressSpace::Configurator::Mapping map;
    for (int i(0); (map = mapping(i), map.memory); i++)
    {
        p_s << "<map>"
            << "<base>" << Hex(map.base) << "</base>";
        map.memory->serialize(p_s);
        p_s << "<size>" << Hex(map.size) << "</size>"
            << "</map>";
    }
    p_s << "</address_space>";
}

std::ostream &operator<<(std::ostream &p_s, const Memory::AccessType p_at)
{
    if (p_at >= 0 && p_at < Memory::AT_LAST) {
        static const char *name[Memory::AT_LAST] = {
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

std::ostream &operator<<(std::ostream &p_s, const Storage &p_storage)
{
    p_s << "Storage(";
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
    return p_s << ")";
}

void Ram::serialize(std::ostream &p_s) const
{
    p_s << "Ram(";
    Memory::serialize(p_s);
    p_s << ", " << m_storage;
    if (!m_filename.empty())
        p_s << ", filename(\"" << m_filename << "\")";
    p_s << ")";
}

void Rom::serialize(std::ostream &p_s) const
{
    p_s << "Rom(";
    Memory::serialize(p_s);
    p_s << ", " << m_storage;
    p_s << ")";
}

void AddressSpace::serialize(std::ostream &p_s) const
{
    p_s << "AddressSpace(";
    Memory::serialize(p_s);
    Memory *previous_memory;
    for (int addr=0; addr < int(m_map.size()); addr++) {
        Memory *cell(m_map[addr]);
        if (cell != previous_memory) {
#if 0
            p_s << std::endl << Hex(word(addr))
                << ": ***********************************************************************************************"
                << std::endl;
            previous_memory = cell;
            if (previous_memory)
                previous_memory->serialize(p_s);
#else
            if (cell)
                p_s << Hex(word(addr)) << ":MemoryRef(" << cell->id() << ")";
            previous_memory = cell;
#endif
        }
    }
    p_s << ")";
}

// terminal.cpp

#include <ostream>

#include <log4cxx/logger.h>

#include "terminal.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".terminal.cpp"));
    return result;
}

Terminal::Terminal(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , m_memory(p_cfgr.memory()->memory_factory())
    , m_ppia(dynamic_cast<Ppia *>(p_cfgr.ppia()->memory_factory()))
    , m_monitor_view(m_ppia, m_memory, p_cfgr.monitor_view())
    , m_keyboard_controller(m_ppia, p_cfgr.keyboard_controller())
{
    LOG4CXX_INFO(cpptrace_log(), "Terminal::Terminal(" << p_cfgr << ")");
    assert (m_memory);
    m_memory->add_parent(this);
    assert (m_ppia);
    m_ppia->add_parent(this);
}

Terminal::~Terminal()
{
    LOG4CXX_INFO(cpptrace_log(), "[" << id() << "].Terminal::~Terminal()");
    if (m_memory)
        m_memory->remove_parent(this);
    if (m_ppia)
        m_ppia->remove_parent(this);
}

void Terminal::remove_child(Part *p_child)
{
    if (p_child == m_memory)
        m_memory = 0;
    if (p_child == m_ppia)
        m_ppia = 0;
}


void Terminal::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Part::Configurator::serialize(p_s);
    if (memory())
        p_s << id() << " -> " << memory()->id() << ";\n";
    if (ppia())
        p_s << id() << " -> " << ppia()->id() << ";\n";
#else
    p_s << "<terminal ";
    Part::Configurator::serialize(p_s);
    p_s << ">"
        << "<memory name=\"" << memory()->id() << "\"/>"
        << "<controller name=\"" << ppia()->id() << "\"/>"
        << monitor_view()
        << keyboard_controller()
        << "</terminal>";
#endif
}

void Terminal::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    Part::serialize(p_s);
    if (m_memory)
        p_s << id() << " -> " << m_memory->id() << ";\n";
    if (m_ppia)
        p_s << id() << " -> " << m_ppia->id() << ";\n";
#else
    p_s << "Terminal(";
    Part::serialize(p_s);
    p_s << "Memory(" << m_memory->id() << ")"
        << "Ppia(" << m_ppia->id() << ")"
        << m_monitor_view
        << m_keyboard_controller
        << ")";
#endif
}

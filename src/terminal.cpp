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
    , m_memory(dynamic_cast<Memory *>(PartsBin::instance()[p_cfgr.memory_id()]))
    , m_terminal_interface(dynamic_cast<TerminalInterface *>(PartsBin::instance()[p_cfgr.controller_id()]))
    , m_monitor_view(m_terminal_interface, m_memory, p_cfgr.monitor_view())
    , m_keyboard_controller(m_terminal_interface, p_cfgr.keyboard_controller())
{
    LOG4CXX_INFO(cpptrace_log(), "Terminal::Terminal(" << p_cfgr << ")");
}


void Terminal::Configurator::serialize(std::ostream &p_s) const
{
    p_s << "<terminal";
    Part::Configurator::serialize(p_s);
    p_s << "<memory name=\"" << memory_id() << "\"/>"
        << "<controller name=\"" << controller_id() << "\"/>"
        << monitor_view()
        << keyboard_controller()
        << "</terminal>";
}

void Terminal::serialize(std::ostream &p_s) const
{
    p_s << "Terminal(";
    Part::serialize(p_s);
    p_s << "Memory(" << m_memory->id() << ")"
        << "TerminalInterface(" << m_terminal_interface->id() << ")"
        << m_monitor_view
        << m_keyboard_controller
        << ")";
}

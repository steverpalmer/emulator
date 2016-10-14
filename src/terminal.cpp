// terminal.cpp

#include "part.hpp"
#include "terminal.hpp"

Terminal::Terminal(const Configurator &p_cfgr)
    : Part(p_cfgr)
    , m_memory(dynamic_cast<Memory *>(PartsBin::instance()[p_cfgr.memory_id()]))
    , m_terminal_interface(dynamic_cast<TerminalInterface *>(PartsBin::instance()[p_cfgr.controller_id()]))
    , m_monitor_view(m_terminal_interface, m_memory, p_cfgr.monitor_view())
    , m_keyboard_controller(m_terminal_interface, p_cfgr.keyboard_controller())
{
}


std::ostream &operator<<(std::ostream &p_s, const Terminal::Configurator &p_cfgr)
{
    return p_s << "<terminal " << static_cast<const Part::Configurator &>(p_cfgr) << ">"
               << "memory name=\"" << p_cfgr.memory_id() << "\"/>"
               << "controller name=\"" << p_cfgr.controller_id() << "\"/>"
               << p_cfgr.monitor_view()
               << p_cfgr.keyboard_controller()
               << "</terminal>";
}

std::ostream &operator<<(std::ostream &p_s, const Terminal &p_t)
{
    return p_s << "Terminal("
               << static_cast<const Part &>(p_t)
               << "Memory(" << p_t.m_memory->id() << ")"
               << "TerminalInterface(" << p_t.m_terminal_interface->id() << ")"
               << p_t.m_monitor_view
               << p_t.m_keyboard_controller
               << ")";
}

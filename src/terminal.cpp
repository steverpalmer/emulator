// terminal.cpp

#include "terminal.hpp"

Terminal::Terminal(Memory &p_memory, TerminalInterface &p_terminal_interface, const Configurator &p_cfg)
    : Part(p_cfg)
    , m_monitor_view(p_memory, p_terminal_interface, p_cfg.monitor_view())
    , m_keyboard_controller(p_terminal_interface, p_cfg.keyboard_controller())
{
}

friend std::ostream &::operator <<(std::ostream &, const Configurator &)
{
    return ps << "<terminal " << static_const<const ActivePart::Configurator &>(p_cfg) << ">"
              << p_cfg.screen_controller()
              << p_cfg.keyboard_controller()
              << "</terminal>";
}

friend std::ostream &::operator <<(std::ostream &, const Terminal &)
{
    return ps << "Terminal("
              << static_const<const Part &>(*this)
              << m_screen_controller
              << m_keyboard_controller
              << ")"
}

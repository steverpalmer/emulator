// config_xml.hpp

#ifndef CONFIG_XML_HPP_
#define CONFIG_XML_HPP_

#include <ostream>
#include <string>

#include <glibmm/ustring.h>
#include <libxml++/libxml++.h>

#include "common.hpp"

#include "atom.hpp"
#include "terminal.hpp"

class RamConfigurator : public Ram::Configurator
{
private:
    Glib::ustring m_id;
    word          m_size;
    Glib::ustring m_filename;
public:
    explicit RamConfigurator(const xmlpp::Node *p_node);
    const Glib::ustring &id()       const { return m_id; }
    word                size()      const  { return m_size; }
    const Glib::ustring &filename() const { return m_filename; }
};

class RomConfigurator : public Rom::Configurator
{
private:
    Glib::ustring m_id;
    word          m_size;
    Glib::ustring m_filename;
public:
    explicit RomConfigurator(const xmlpp::Node *p_node);
    const Glib::ustring &id()       const { return m_id; }
    const Glib::ustring &filename() const { return m_filename; }
    word                size()      const { return m_size; }
};

class PpiaConfigurator : public Ppia::Configurator
{
private:
    Glib::ustring m_id;
public:
    explicit PpiaConfigurator(const xmlpp::Node *p_node);
    const Glib::ustring &id() const { return m_id; }
};

class MemoryConfigurator : public Memory::Configurator
{
private:
    Glib::ustring m_id;
    word          m_size;
    struct Memory::Configurator::mapping m_last_device;
    std::vector<struct Memory::Configurator::mapping> m_device;
public:
    explicit MemoryConfigurator(const xmlpp::Node *p_node);
    const Glib::ustring &id()  const { return m_id; }
    word                size() const { return m_size; }
    const struct Memory::Configurator::mapping &device(int i) const
        { return (i < int(m_device.size())) ? m_device[i] : m_last_device; }
};

class MCS6502Configurator : public MCS6502::Configurator
{
private:
    Glib::ustring m_id;
public:
    explicit MCS6502Configurator(const xmlpp::Node *p_node = 0);
    const Glib::ustring &id() const { return m_id; }
};

class AtomConfigurator : public Atom::Configurator
{
private:
    Glib::ustring       m_id;
    MemoryConfigurator  *m_memory;
    MCS6502Configurator *m_mcs6502;
public:
    explicit AtomConfigurator(const xmlpp::Node *p_node);
    const Glib::ustring &id() const { return m_id; }
    const Memory::Configurator &memory()      const { return *m_memory; }
    const MCS6502::Configurator &mcs6502()    const { return *m_mcs6502; }
};

class KeyboardControllerConfigurator : public KeyboardController::Configurator
{
public:
    explicit KeyboardControllerConfigurator(const xmlpp::Node *p_node = 0);
};

class MonitorViewConfigurator : public MonitorView::Configurator
{
private:
    float         m_scale;
    Glib::ustring m_fontfilename;
    Glib::ustring m_window_title;
    Glib::ustring m_icon_title;
public:
    explicit MonitorViewConfigurator(const xmlpp::Node *p_node = 0);
    float               scale()         const { return m_scale; }
    const Glib::ustring &fontfilename() const { return m_fontfilename; }
    const Glib::ustring &window_title() const { return m_window_title; }
    const Glib::ustring &icon_title()   const { return m_icon_title; }
};

class TerminalConfigurator : public Terminal::Configurator
{
private:
    Glib::ustring m_id;
    KeyboardControllerConfigurator *m_keyboard;
    MonitorViewConfigurator *m_monitor;
public:
    explicit TerminalConfigurator(const xmlpp::Node *p_node = 0);
    const Glib::ustring              &id()                  const { return m_id; }
    KeyboardController::Configurator &keyboard_controller() const { return *m_keyboard; }
    MonitorView::Configurator        &monitor_view()        const { return *m_monitor; }
};

class Configurator
{
private:
    Glib::ustring                  m_XMLfilename;
public:
    AtomConfigurator               *m_atom;
    TerminalConfigurator           *m_terminal;
private:
    void process_command_line(int argc, char *argv[]);
    void process_XML();
    bool check_and_complete_params();
public:
    explicit Configurator(int argc, char *argv[]);
    virtual ~Configurator();
    const Atom::Configurator               &atom()     const { return *m_atom;     }
    const Terminal::Configurator           &terminal() const { return *m_terminal; }

    friend std::ostream &::operator<<(std::ostream&, const Configurator&);
};

#endif /* CONFIG_XML_HPP_ */

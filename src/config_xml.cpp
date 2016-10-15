// config_xml.cpp

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <cassert>
#include <log4cxx/logger.h>
#include <libxml++/libxml++.h>

#include <ostream>
#include <string>

#include <glibmm/ustring.h>
#include <libxml++/libxml++.h>

#include "common.hpp"
#include "config_xml.hpp"
#include "device.hpp"
#include "memory.hpp"
#include "ppia.hpp"
#include "cpu.hpp"
#include "terminal.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".config_xml.cpp"));
    return result;
}

namespace Xml
{

    class PartConfigurator
        : public virtual Part::Configurator
    {
    protected:
        Part::id_type m_id;
        explicit PartConfigurator(Part::id_type p_id) : m_id(p_id) {}
    public:
        virtual ~PartConfigurator();
        inline virtual const Part::id_type &id() const { return m_id; }
    };

    class MemoryRefConfigurator
        : public PartConfigurator
        , public Memory::Configurator
    {
    public:
        explicit MemoryRefConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator(p_node->eval_to_string("@name"))
            {}
        virtual ~MemoryRefConfigurator();
    };

    class RamConfigurator
        : public PartConfigurator
        , public Ram::Configurator
    {
    private:
        word          m_size;
        Glib::ustring m_filename;
    public:
        explicit RamConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator("ram")
            , m_filename("")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::RamConfigurator::RamConfigurator(" << p_node << ")");
                assert (p_node);
                try { m_id = p_node->eval_to_string("@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                m_size = p_node->eval_to_number("size");
                try { m_filename = p_node->eval_to_string("filename"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
            }
        virtual ~RamConfigurator();
        inline virtual word                size()      const { return m_size; }
        inline virtual const Glib::ustring &filename() const { return m_filename; }
    };

    class RomConfigurator
        : public PartConfigurator
        , public Rom::Configurator
    {
    private:
        word          m_size;
        Glib::ustring m_filename;
    public:
        explicit RomConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator("rom")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::RomConfigurator::RomConfigurator(" << p_node << ")");
                assert (p_node);
                try { m_id = p_node->eval_to_string("@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                m_filename = p_node->eval_to_string("filename");
                try { m_size = p_node->eval_to_number("size"); }
                catch(xmlpp::exception e)
                {
                    assert (0); // get the size of the file
                }
            }
        virtual ~RomConfigurator();
        inline virtual const Glib::ustring &filename() const { return m_filename; }
        inline virtual word                size()      const { return m_size; }
    };

    class PpiaConfigurator
        : public PartConfigurator
        , public Ppia::Configurator
    {
    public:
        explicit PpiaConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator("ppia")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::PpiaConfigurator::PpiaConfigurator(" << p_node << ")");
                assert (p_node);
                try { m_id = p_node->eval_to_string("@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
            }
        virtual ~PpiaConfigurator();
    };

    class AddressSpaceConfigurator
        : public PartConfigurator
        , public AddressSpace::Configurator
    {
    private:
        word m_size;
        const AddressSpace::Configurator::Mapping m_last_memory;
        std::vector<AddressSpace::Configurator::Mapping> m_memory;
    public:
        explicit AddressSpaceConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator("address_space")
            , m_size(0)
            , m_last_memory( { 0, 0, 0 } )
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MemoryConfigurator::MemoryConfigurator(" << p_node << ")");
                assert (p_node);
                try { m_id = p_node->eval_to_string("@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                try { m_size = p_node->eval_to_number("size"); }
                catch(xmlpp::exception e) { /* Do Nothing */ }
                for (auto *child: p_node->get_children())
                {
                    const xmlpp::Element *elm = dynamic_cast<const xmlpp::Element *>(child);
                    if (elm && elm->get_name() == "map")
                    {
                        AddressSpace::Configurator::Mapping *map = new AddressSpace::Configurator::Mapping(m_last_memory);
                        map->base = elm->eval_to_number("base");
                        try { map->size = elm->eval_to_number("size"); }
                        catch (xmlpp::exception e) { /* Do Nothing */ }
                        const xmlpp::NodeSet ns(elm->find("ram|rom|ppia|address_space|memory"));
                        assert (ns.size() == 1);
                        const xmlpp::Node *mem(ns[0]);
                        map->memory = (
                            (mem->get_name() == "ram")
                            ? static_cast<const Memory::Configurator *>(new RamConfigurator(mem))
                            : (mem->get_name() == "rom")
                            ? static_cast<const Memory::Configurator *>(new RomConfigurator(mem))
                            : (mem->get_name() == "ppia")
                            ? static_cast<const Memory::Configurator *>(new PpiaConfigurator(mem))
                            : (mem->get_name() == "address_space")
                            ? static_cast<const Memory::Configurator *>(new AddressSpaceConfigurator(mem))
                            : (mem->get_name() == "memory")
                            ? static_cast<const Memory::Configurator *>(new MemoryRefConfigurator(mem))
                            : 0 );
                        // FIXME: Should I do anything about size?
                        m_memory.push_back(*map);
                    }
                }
            }
        virtual ~AddressSpaceConfigurator()
            {
                for (auto &m : m_memory)
                    delete &m;
                m_memory.clear();
            }
        inline virtual word size() const { return m_size; }
        inline virtual const AddressSpace::Configurator::Mapping &mapping(int i) const
            { return (i < int(m_memory.size())) ? m_memory[i] : m_last_memory; }
    };

    class DeviceRefConfigurator
        : public PartConfigurator
        , public Device::Configurator
    {
    public:
        explicit DeviceRefConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator(p_node->eval_to_string("@name"))
            {}
        virtual ~DeviceRefConfigurator();
    };

    class MCS6502Configurator
        : public PartConfigurator
        , public MCS6502::Configurator
    {
    private:
        Memory::id_type m_memory_id;
    public:
        explicit MCS6502Configurator(const xmlpp::Node *p_node = 0)
            : PartConfigurator("mcs6502")
            , m_memory_id("address_space")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MCS6502Configurator::MCS6502Configurator(" << p_node << ")");
                if (p_node)
                {
                    try { m_id = p_node->eval_to_string("@name"); }
                    catch (xmlpp::exception e) { /* Do Nothing */ }
                    try { m_memory_id = p_node->eval_to_string("memory/@name"); }
                    catch (xmlpp::exception e) { /* Do Nothing */ }
                }
            }
        virtual ~MCS6502Configurator();
        virtual const Memory::id_type memory_id() const
            { return m_memory_id; }
    };

    class ComputerConfigurator
        : public PartConfigurator
        , public Computer::Configurator
    {
    private:
        std::vector<const Device::Configurator *>m_parts;
    public:
        explicit ComputerConfigurator(const xmlpp::Node *p_node)
            : PartConfigurator("computer")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::ComputerConfigurator::ComputerConfigurator(" << p_node << ")");
                assert(p_node);
                try { m_id = p_node->eval_to_string("@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                for (auto *child : p_node->get_children())
                {
                    const xmlpp::Element *child_elm(dynamic_cast<xmlpp::Element *>(child));
                    if (child_elm)
                    {
                        const Glib::ustring child_elm_name(child_elm->get_name());
                        if (child_elm_name == "device")
                            m_parts.push_back(new DeviceRefConfigurator(child_elm));
                        else if (child_elm_name == "computer")
                            m_parts.push_back(new ComputerConfigurator(child_elm));
                        else if (child_elm_name == "mcs6502")
                            m_parts.push_back(new MCS6502Configurator(child_elm));
                        else if (child_elm_name == "memory")
                            m_parts.push_back(new MemoryRefConfigurator(child_elm));
                        else if (child_elm_name == "address_space")
                            m_parts.push_back(new AddressSpaceConfigurator(child_elm));
                        else if (child_elm_name == "ram")
                            m_parts.push_back(new RamConfigurator(child_elm));
                        else if (child_elm_name == "rom")
                            m_parts.push_back(new RomConfigurator(child_elm));
                        else if (child_elm_name == "ppia")
                            m_parts.push_back(new PpiaConfigurator(child_elm));
                        else
                            ;  // Do Nothing
                    }
                }
            }
        virtual ~ComputerConfigurator();
        virtual const Device::Configurator *device(int i) const
            { return (i < int(m_parts.size())) ? m_parts[i] : 0; }

    };

    class KeyboardControllerConfigurator
        : public KeyboardController::Configurator
    {
    public:
        explicit KeyboardControllerConfigurator(const xmlpp::Node *p_node = 0)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::KeyboardControllerConfigurator::KeyboardControllerConfigurator(" << p_node << ")");
            }
        virtual ~KeyboardControllerConfigurator();
    };

    class MonitorViewConfigurator
        : public MonitorView::Configurator
    {
    private:
        float         m_scale;
        Glib::ustring m_fontfilename;
        Glib::ustring m_window_title;
        Glib::ustring m_icon_title;
    public:
        explicit MonitorViewConfigurator(const xmlpp::Node *p_node = 0)
            : m_scale(2.0)
            , m_fontfilename("mc6847.bmp")
            , m_window_title("Acorn Atom")
            , m_icon_title("Acorn Atom")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MonitorViewConfigurator::MonitorViewConfigurator(" << p_node << ")");
                if (p_node)
                {
                    try { m_scale = p_node->eval_to_number("scale"); }
                    catch (xmlpp::exception e) { /* Do Nothing */ }
                    try { m_fontfilename = p_node->eval_to_string("fontfilename"); }
                    catch (xmlpp::exception e) { /* Do Nothing */ }
                }
            }
        virtual ~MonitorViewConfigurator();
        float               scale()         const { return m_scale; }
        const Glib::ustring &fontfilename() const { return m_fontfilename; }
        const Glib::ustring &window_title() const { return m_window_title; }
        const Glib::ustring &icon_title()   const { return m_icon_title; }
    };

    class TerminalConfigurator
        : public PartConfigurator
        , public Terminal::Configurator
    {
    private:
        Part::id_type m_memory_id;
        Part::id_type m_controller_id;
        KeyboardControllerConfigurator *m_keyboard_controller;
        MonitorViewConfigurator *m_monitor_view;
    public:
        explicit TerminalConfigurator(const xmlpp::Node *p_node = 0)
            : PartConfigurator("terminal")
            , m_memory_id("video")
            , m_controller_id("ppia")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::TerminalConfigurator::TerminalConfigurator(" << p_node << ")");
                try { m_memory_id = p_node->eval_to_string("memory/@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                try { m_controller_id = p_node->eval_to_string("controller/@name"); }
                catch (xmlpp::exception e) { /* Do Nothing */ }
                m_keyboard_controller = new KeyboardControllerConfigurator(p_node);
                assert (m_keyboard_controller);
                m_monitor_view = new MonitorViewConfigurator(p_node);
                assert (m_monitor_view);
            }
        virtual ~TerminalConfigurator();
        const Part::id_type                      &memory_id()           const { return m_memory_id; }
        const Part::id_type                      &controller_id()       const { return m_controller_id; }
        const KeyboardController::Configurator   &keyboard_controller() const { return *m_keyboard_controller; }
        const MonitorView::Configurator          &monitor_view()        const { return *m_monitor_view; }
    };

    void Configurator::process_command_line(int argc, char *argv[])
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::Configurator::process_command_line(" << argc << ", " << argv << ")");
        opterr = 0;
        int c;
        while ((c = getopt(argc, argv, "f:")) != -1)
            switch (c) {
#if 0
            case 'i': /* Stream Input from ... */
                cfg->keyboard.filename = (strcmp(optarg, "-")?optarg:"");
                cfg->keyboard.viewscreen = true;
                break;
            case 'o': /* Stream Output to ... */
                cfg->screen.filename = (strcmp(optarg, "-")?optarg:"");
                cfg->keyboard.viewscreen = false;
                break;
#endif
            case 'f': /* Use Config File ... */
                m_XMLfilename = optarg;
                break;
            case '?': /* Unknown Option */
                LOG4CXX_WARN(cpptrace_log(), "Unknown Option '" << optopt << "'");
                break;
            default:
                LOG4CXX_WARN(cpptrace_log(), "Unexpected response from getopt() " << c);
                break;
            }
    }

    void Configurator::process_XML()
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::Configurator::process_XML()");
        try
        {
            xmlpp::DomParser parser;
            parser.parse_file(m_XMLfilename);
            if (!parser)
            {
                LOG4CXX_FATAL(cpptrace_log(), "Parse Failure");
                exit(1);
            }

            // TODO: Check version
            const xmlpp::Node *root(parser.get_document()->get_root_node());

            for (auto *child : root->get_children())
            {
                const xmlpp::Element *child_elm(dynamic_cast<xmlpp::Element *>(child));
                if (child_elm)
                {
                    const Glib::ustring child_elm_name(child_elm->get_name());
                    if (child_elm_name == "device")
                        m_parts.push_back(new DeviceRefConfigurator(child_elm));
                    else if (child_elm_name == "computer")
                        m_parts.push_back(new ComputerConfigurator(child_elm));
                    else if (child_elm_name == "mcs6502")
                        m_parts.push_back(new MCS6502Configurator(child_elm));
                    else if (child_elm_name == "memory")
                        m_parts.push_back(new MemoryRefConfigurator(child_elm));
                    else if (child_elm_name == "address_space")
                        m_parts.push_back(new AddressSpaceConfigurator(child_elm));
                    else if (child_elm_name == "ram")
                        m_parts.push_back(new RamConfigurator(child_elm));
                    else if (child_elm_name == "rom")
                        m_parts.push_back(new RomConfigurator(child_elm));
                    else if (child_elm_name == "ppia")
                        m_parts.push_back(new PpiaConfigurator(child_elm));
                    else if (child_elm_name == "terminal")
                        m_parts.push_back(new TerminalConfigurator(child_elm));
                    else
                        ; // Do Nothing
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOG4CXX_ERROR(cpptrace_log(), ex.what());
            exit(1);
        }
    }

    bool Configurator::check_and_complete_params()
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::Configurator::check_and_complete_params()");
        bool result = true;
#if 0
        if (m_terminal->monitor_view().scale() && (m_terminal->monitor_view().scale() < 1.0)) {
            LOG4CXX_ERROR(cpptrace_log(), "Bad Scale Factor " << m_terminal->monitor_view().scale());
            result = false;
        }
        struct HookNode **ptr = &cfg->atom.hooks;
        if (cfg->screen.filename) {
            *ptr = malloc(sizeof(**ptr));
            assert (*ptr);
            (*ptr)->action = HK_STREAM_OSWRCH;
            if (*cfg->screen.filename) {
                (*ptr)->object = fopen(cfg->screen.filename, "w");
                if (!(*ptr)->object) {
                    LOG_ERROR("Can't open output stream");
                    result = false;
                }
            }
            else
                (*ptr)->object = stdout;
            (*ptr)->next = 0;
            ptr = &((*ptr)->next);
        }
        if (cfg->keyboard.filename) {
            *ptr = malloc(sizeof(**ptr));
            assert (*ptr);
            (*ptr)->action = HK_STREAM_OSRDCH;
            if (*cfg->keyboard.filename) {
                (*ptr)->object = fopen(cfg->keyboard.filename, "r");
                if (!(*ptr)->object) {
                    LOG_ERROR("Can't open input stream");
                    result = false;
                }
                (*ptr)->size = cfg->keyboard.viewscreen;
            }
            else
                (*ptr)->object = stdin;
            (*ptr)->next = 0;
            ptr = &((*ptr)->next);
        }
        const int err = atom_config_error(m_atom);
        if (err) {
            LOG4CXX_ERROR(cpptrace_log(), "Invalid configuration" << err);
            atom_config_dump(cpptrace_log(), &atom);
            result = false;
        }
#endif
        LOG4CXX_DEBUG(cpptrace_log(), this);
        return result;
    }

    Configurator::Configurator(int argc, char *argv[])
        : ::Configurator(argc, argv)
        , m_XMLfilename("atomrc.xml")
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::Configurator::Configurator(" << argc << ", " << argv << ")");
        process_command_line(argc, argv);
        process_XML();
        if (!check_and_complete_params())
            exit(1);
        LOG4CXX_INFO(cpptrace_log(), "Position 1 => " << *this);
    }

}

// config_xml.cpp

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <unistd.h>

#include <cassert>
#include <exception>
#include <cstdlib>

#include <libxml++/libxml++.h>

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
    // Helper Stuff for processing the XML

    class XpathNotFound
        : public std::exception
    {
        virtual const char *what() const throw()
            { return "XPath not present"; }
    } xpath_not_present;

    class XpathAmbiguous
        : public std::exception
    {
        virtual const char *what() const throw()
            { return "XPath ambiguous"; }
    } xpath_ambiguous;

    class XpathNotValue
        : public std::exception
    {
        virtual const char *what() const throw()
            { return "XPath not a value"; }
    } xpath_not_a_value;

    class XpathNotFloat
        : public std::exception
    {
        virtual const char *what() const throw()
            { return "XPath did not return a float"; }
    } xpath_not_a_float;

    class XpathNotInt
        : public std::exception
    {
        virtual const char *what() const throw()
            { return "XPath did not return an integer"; }
    } xpath_not_an_integer;

    static xmlpp::Node::PrefixNsMap namespaces = { { "e", "http://www.srpalmer.me.uk/ns/emulator" } };

    Glib::ustring eval_to_string(const xmlpp::Node *p_node, const Glib::ustring &p_xpath)
    {
        Glib::ustring result("");
        auto ns(p_node->find(p_xpath, namespaces));
        switch (ns.size())
        {
        case 0:
        {
            throw xpath_not_present;
        }
        case 1:
        {
            const xmlpp::Attribute   *att = dynamic_cast<const xmlpp::Attribute *>(ns[0]);
            const xmlpp::ContentNode *cnt = dynamic_cast<const xmlpp::ContentNode *>(ns[0]);
            const xmlpp::Element     *elm = dynamic_cast<const xmlpp::Element *>(ns[0]);
            if (cnt)
                result = cnt->get_content();
            else if (att)
                result = att->get_value();
            else if (elm)
            {
                const xmlpp::TextNode *elm_txt = elm->get_child_text();
                if (elm_txt)
                    result = elm_txt->get_content();
                else
                    throw xpath_not_a_value;
            }
            else
                throw xpath_not_a_value;
            break;
        }
        default:
        {
            throw xpath_ambiguous;
        }
        }
        return result;
    }

    float eval_to_float(const xmlpp::Node *p_node, const Glib::ustring &p_xpath)
    {
        float result(atof(eval_to_string(p_node, p_xpath).c_str()));
        return result;
    }

    float eval_to_int(const xmlpp::Node *p_node, const Glib::ustring &p_xpath)
    {
        int result;
        const Glib::ustring str(eval_to_string(p_node, p_xpath));
        if (!str.empty())
        {
            if (str[0] == '#')
                result = std::stoi(str.c_str()+1, NULL, 16);
            else
                result = std::atoi(str.c_str());
        }
        return result;
    }

    // Concrete Configurators for the various types of objects
    // deriving their configuration from the XML file

    // Memory first

    class PartConfigurator
        : public virtual Part::Configurator
    {
    protected:
        Part::id_type m_id;
        explicit PartConfigurator(Part::id_type p_id, const xmlpp::Node *p_node=0)
            : m_id(p_id)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::PartConfigurator::PartConfigurator(" << p_id << ", " << p_node << ")");
                if (p_node)
                    try { m_id = eval_to_string(p_node, "@name"); }
                    catch (XpathNotFound e) {}
            }
    public:
        virtual ~PartConfigurator() = default;
        inline virtual const Part::id_type &id() const { return m_id; }
    };

    class DeviceConfigurator
        : public virtual Device::Configurator
        , protected PartConfigurator
    {
    protected:
        explicit DeviceConfigurator(Part::id_type p_id, const xmlpp::Node *p_node=0)
            : PartConfigurator(p_id, p_node)
            {}
    public:
        virtual ~DeviceConfigurator() = default;
    };

    class MemoryConfigurator
        : public virtual Memory::Configurator
        , protected DeviceConfigurator
    {
    protected:
        explicit MemoryConfigurator(Part::id_type p_id, const xmlpp::Node *p_node=0)
            : DeviceConfigurator(p_id, p_node)
            {}
    public:
        virtual ~MemoryConfigurator() = default;
    };

    Part::Configurator *part_configurator_factory(const xmlpp::Node *p_node);
    Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node);
    Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node);

    class MemoryRefConfigurator
        : public virtual Memory::Configurator
        , private MemoryConfigurator
    {
    public:
        explicit MemoryRefConfigurator(const xmlpp::Node *p_node)
            : MemoryConfigurator("", p_node)
            {}
        explicit MemoryRefConfigurator(Glib::ustring p_name)
            : MemoryConfigurator(p_name)
            {}
        virtual ~MemoryRefConfigurator() = default;
        virtual Memory *memory_factory() const
            { return dynamic_cast<Memory *>(PartsBin::instance()[m_id]); }
        static Memory::Configurator *_memory_configurator_factory(const xmlpp::Node *p_node)
            { return new MemoryRefConfigurator(p_node); }

        virtual void serialize(std::ostream &p_s) const
            {
                p_s << "<memory name=\"" << m_id << "\"/>";
            }
    };

    class RamConfigurator
        : public virtual Ram::Configurator
        , private MemoryConfigurator
    {
    private:
        word          m_size;
        Glib::ustring m_filename;
    public:
        explicit RamConfigurator(const xmlpp::Node *p_node)
            : MemoryConfigurator("", p_node)
            , m_filename("")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::RamConfigurator::RamConfigurator(" << p_node << ")");
                assert (p_node);
                m_size = eval_to_int(p_node, "e:size");
                try { m_filename = eval_to_string(p_node, "e:filename"); }
                catch (XpathNotFound e) {}
            }
        virtual ~RamConfigurator() = default;
        inline virtual word                size()      const { return m_size; }
        inline virtual const Glib::ustring &filename() const { return m_filename; }
        static Memory::Configurator *_memory_configurator_factory(const xmlpp::Node *p_node)
            { return new RamConfigurator(p_node); }
    };

    class RomConfigurator
        : public virtual Rom::Configurator
        , private MemoryConfigurator
    {
    private:
        word          m_size;
        Glib::ustring m_filename;
    public:
        explicit RomConfigurator(const xmlpp::Node *p_node)
            : MemoryConfigurator("", p_node)
            , m_size(0)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::RomConfigurator::RomConfigurator(" << p_node << ")");
                assert (p_node);
                m_filename = eval_to_string(p_node, "e:filename");
                try { m_size = eval_to_int(p_node, "e:size"); }
                catch(XpathNotFound::exception e) {}
            }
        virtual ~RomConfigurator() = default;
        inline virtual const Glib::ustring &filename() const { return m_filename; }
        inline virtual word                size()      const { return m_size; }
        static Memory::Configurator *_memory_configurator_factory(const xmlpp::Node *p_node)
            { return new RomConfigurator(p_node); }
    };

    class PpiaConfigurator
        : public virtual Ppia::Configurator
        , private MemoryConfigurator
    {
    public:
        explicit PpiaConfigurator(const xmlpp::Node *p_node)
            : MemoryConfigurator("ppia", p_node)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::PpiaConfigurator::PpiaConfigurator(" << p_node << ")");
                assert (p_node);
            }
        virtual ~PpiaConfigurator() = default;
        static Memory::Configurator *_memory_configurator_factory(const xmlpp::Node *p_node)
            { return new PpiaConfigurator(p_node); }
    };

    class AddressSpaceConfigurator
        : public virtual AddressSpace::Configurator
        , private MemoryConfigurator
    {
    private:
        word m_size;
        const AddressSpace::Configurator::Mapping m_last_memory;
        std::vector<AddressSpace::Configurator::Mapping> m_memory;
    public:
        explicit AddressSpaceConfigurator(const xmlpp::Node *p_node)
            : MemoryConfigurator("address_space", p_node)
            , m_size(0)
            , m_last_memory( { 0, 0, 0 } )
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::AddressSpaceConfigurator::AddressSpaceConfigurator(" << p_node << ")");
                assert (p_node);
                try { m_size = eval_to_int(p_node, "e:size"); }
                catch(XpathNotFound e) {}
                for (auto *child: p_node->get_children())
                {
                    const xmlpp::Element *child_elm = dynamic_cast<const xmlpp::Element *>(child);
                    if (child_elm && child_elm->get_name() == "map")
                    {
                        AddressSpace::Configurator::Mapping *map = new AddressSpace::Configurator::Mapping(m_last_memory);
                        map->base = eval_to_int(child_elm, "e:base");
                        try { map->size = eval_to_int(child_elm, "e:size"); }
                        catch (XpathNotFound e) { map->size = 0; }
                        const xmlpp::NodeSet ns(child_elm->find("e:ram|e:rom|e:ppia|e:address_space|e:memory", namespaces));
                        assert (ns.size() == 1);
                        const xmlpp::Node *memory_node(ns[0]);
                        map->memory = memory_configurator_factory(memory_node);
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
        static Memory::Configurator *_memory_configurator_factory(const xmlpp::Node *p_node)
            { return new AddressSpaceConfigurator(p_node); }
    };

    Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::memory_configurator_factory(" << p_node << ")");
        static std::map<std::string, Memory::Configurator * (*)(const xmlpp::Node *p_node)> factory_map;
        if (factory_map.size() == 0)
        {
            factory_map["memory"]        = MemoryRefConfigurator::_memory_configurator_factory;
            factory_map["ram"]           = RamConfigurator::_memory_configurator_factory;
            factory_map["rom"]           = RomConfigurator::_memory_configurator_factory;
            factory_map["ppia"]          = PpiaConfigurator::_memory_configurator_factory;
            factory_map["address_space"] = AddressSpaceConfigurator::_memory_configurator_factory;
        }
        Memory::Configurator *result(0);
        Memory::Configurator *(*factory)(const xmlpp::Node *p_node) = factory_map[p_node->get_name()];
        if (factory)
            result = factory(p_node);
        return result;
    }

    // Devices Second

    class DeviceRefConfigurator
        : public virtual Device::Configurator
        , private DeviceConfigurator
    {
    public:
        explicit DeviceRefConfigurator(const xmlpp::Node *p_node)
            : DeviceConfigurator("", p_node)
            {}
        explicit DeviceRefConfigurator(Glib::ustring p_name)
            : DeviceConfigurator(p_name)
            {}
        virtual ~DeviceRefConfigurator() = default;
        virtual Device *device_factory() const
            { return dynamic_cast<Device *>(PartsBin::instance()[m_id]); }
        static Device::Configurator *_device_configurator_factory(const xmlpp::Node *p_node)
            { return new DeviceRefConfigurator(p_node); }

        virtual void serialize(std::ostream &p_s) const
            {
                p_s << "<device name=\"" << m_id << "\"/>";
            }
    };

    class MCS6502Configurator
        : public virtual MCS6502::Configurator
        , private DeviceConfigurator
    {
    private:
        Memory::Configurator *m_memory;
    public:
        explicit MCS6502Configurator(const xmlpp::Node *p_node = 0)
            : DeviceConfigurator("", p_node)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MCS6502Configurator::MCS6502Configurator(" << p_node << ")");
                if (p_node)
                {
                    const xmlpp::NodeSet ns(p_node->find("e:ram|e:rom|e:ppia|e:address_space|e:memory", namespaces));
                    switch (ns.size())
                    {
                    case 0: { m_memory = new MemoryRefConfigurator("address_space"); break; }
                    case 1: { m_memory = memory_configurator_factory(ns[0]); break; }
                    default: { assert (false); }
                    }
                }
            }
        virtual ~MCS6502Configurator() = default;
        virtual const Memory::Configurator *memory() const
            { return m_memory; }
        static Device::Configurator *_device_configurator_factory(const xmlpp::Node *p_node)
            { return new MCS6502Configurator(p_node); }
    };

    class ComputerConfigurator
        : public virtual Computer::Configurator
        , private DeviceConfigurator
    {
    private:
        std::vector<const Device::Configurator *>m_devices;
    public:
        explicit ComputerConfigurator(const xmlpp::Node *p_node)
            : DeviceConfigurator("", p_node)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::ComputerConfigurator::ComputerConfigurator(" << p_node << ")");
                assert(p_node);
                for (auto *child : p_node->get_children())
                {
                    const xmlpp::Element *child_elm(dynamic_cast<xmlpp::Element *>(child));
                    if (child_elm)
                    {
                        Device::Configurator *child_cfgr = device_configurator_factory(child_elm);
                        if (child_cfgr)
                            m_devices.push_back(child_cfgr);
                    }
                }
            }
        virtual ~ComputerConfigurator() = default;  // FIXME
        virtual const Device::Configurator *device(int i) const
            { return (i < int(m_devices.size())) ? m_devices[i] : 0; }
        static Device::Configurator *_device_configurator_factory(const xmlpp::Node *p_node)
            { return new ComputerConfigurator(p_node); }

    };

    Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::device_configurator_factory(" << p_node << ")");
        static std::map<std::string, Device::Configurator * (*)(const xmlpp::Node *p_node)> factory_map;
        if (factory_map.size() == 0)
        {
            factory_map["device"]   = DeviceRefConfigurator::_device_configurator_factory;
            factory_map["mcs6502"]  = MCS6502Configurator::_device_configurator_factory;
            factory_map["computer"] = ComputerConfigurator::_device_configurator_factory;
        }
        Device::Configurator *result(0);
        Device::Configurator *(*factory)(const xmlpp::Node *p_node) = factory_map[p_node->get_name()];
        if (factory)
            result = factory(p_node);
        else
            result = memory_configurator_factory(p_node);
        return result;
    }

    // Other Parts last

    class KeyboardControllerConfigurator
        : public virtual KeyboardController::Configurator
    {
    public:
        explicit KeyboardControllerConfigurator(const xmlpp::Node *p_node = 0)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::KeyboardControllerConfigurator::KeyboardControllerConfigurator(" << p_node << ")");
            }
        virtual ~KeyboardControllerConfigurator() = default;
    };

    class MonitorViewConfigurator
        : public virtual MonitorView::Configurator
    {
    private:
        Glib::ustring m_fontfilename;
        Glib::ustring m_window_title;
    public:
        explicit MonitorViewConfigurator(const xmlpp::Node *p_node = 0)
            : m_fontfilename("mc6847.bmp")
            , m_window_title("Emulator")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MonitorViewConfigurator::MonitorViewConfigurator(" << p_node << ")");
                if (p_node)
                {
                    try { m_fontfilename = eval_to_string(p_node, "e:fontfilename"); }
                    catch (XpathNotFound e) {}
                }
            }
        virtual ~MonitorViewConfigurator() = default;
        const Glib::ustring &fontfilename() const { return m_fontfilename; }
        const Glib::ustring &window_title() const { return m_window_title; }
    };

    class TerminalConfigurator
        : public virtual Terminal::Configurator
        , private PartConfigurator
    {
    private:
        Memory::Configurator *m_memory;
        Memory::Configurator *m_ppia;
        KeyboardControllerConfigurator *m_keyboard_controller;
        MonitorViewConfigurator *m_monitor_view;
    public:
        explicit TerminalConfigurator(const xmlpp::Node *p_node = 0)
            : PartConfigurator("", p_node)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::TerminalConfigurator::TerminalConfigurator(" << p_node << ")");
                if (p_node)
                {
                    const xmlpp::NodeSet memory_ns(p_node->find("e:ram|e:rom|e:address_space|e:memory", namespaces));
                    switch (memory_ns.size())
                    {
                    case 0: { m_memory = new MemoryRefConfigurator("video"); break; }
                    case 1: { m_memory = memory_configurator_factory(memory_ns[0]); break; }
                    default: { assert (false); }
                    }
                    const xmlpp::NodeSet ppia_ns(p_node->find("e:ppia", namespaces));
                    switch (ppia_ns.size())
                    {
                    case 0: { m_ppia = new MemoryRefConfigurator("ppia"); break; }
                    case 1: { m_ppia = memory_configurator_factory(ppia_ns[0]); break; }
                    default: { assert (false); }
                    }
                }
                m_keyboard_controller = new KeyboardControllerConfigurator(p_node);
                assert (m_keyboard_controller);
                m_monitor_view = new MonitorViewConfigurator(p_node);
                assert (m_monitor_view);
            }
        virtual ~TerminalConfigurator() = default;
        const Memory::Configurator               *memory()              const { return m_memory; }
        const Memory::Configurator               *ppia()                const { return m_ppia; }
        const KeyboardController::Configurator   &keyboard_controller() const { return *m_keyboard_controller; }
        const MonitorView::Configurator          &monitor_view()        const { return *m_monitor_view; }
        static Part::Configurator *_part_configurator_factory(const xmlpp::Node *p_node)
            { return new TerminalConfigurator(p_node); }
    };

    Part::Configurator *part_configurator_factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::part_configurator_factory(" << p_node << ")");
        static std::map<std::string, Part::Configurator * (*)(const xmlpp::Node *p_node)> factory_map;
        if (factory_map.size() == 0)
        {
            factory_map["terminal"] = TerminalConfigurator::_part_configurator_factory;
        }
        Part::Configurator *result(0);
        Part::Configurator *(*factory)(const xmlpp::Node *p_node) = factory_map[p_node->get_name()];
        if (factory)
            result = factory(p_node);
        else
            result = device_configurator_factory(p_node);
        return result;
    }

    // Highest level functions

    void Configurator::process_command_line(int argc, char *argv[])
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::Configurator::process_command_line(" << argc << ", " << argv << ")");
        for (int i(0); i < argc; i++)
            LOG4CXX_INFO(cpptrace_log(), "Arg " << i << ":\"" << argv[i] << "\"");
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
                    Part::Configurator *child_cfgr = part_configurator_factory(child_elm);
                    if (child_cfgr)
                        m_parts.push_back(child_cfgr);
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
    }


    Configurator::~Configurator()
    {
        // FIXME: need a tidy clean up
    }
}

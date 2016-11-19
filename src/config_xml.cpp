// config_xml.cpp
// Copyright 2016 Steve Palmer

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <unistd.h>

#include <cassert>
#include <exception>
#include <cstdlib>

#include <libxml++/libxml++.h>
#include <libxml++/validators/relaxngvalidator.h>

#include "config_xml.hpp"

#include "device.hpp"
#include "memory.hpp"
#include "ppia.hpp"
#include "cpu.hpp"
#include "atom_streambuf.hpp"
#include "monitor_view.hpp"

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
        assert (p_node);
        auto ns(p_node->find(p_xpath, namespaces));
        switch (ns.size())
        {
        case 0:
            throw xpath_not_present;
        case 1:
        {
            auto att = dynamic_cast<const xmlpp::Attribute *>(ns[0]);
            auto cnt = dynamic_cast<const xmlpp::ContentNode *>(ns[0]);
            auto elm = dynamic_cast<const xmlpp::Element *>(ns[0]);
            if (cnt)
                result = cnt->get_content();
            else if (att)
                result = att->get_value();
            else if (elm)
            {
                auto elm_txt = elm->get_child_text();
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
            throw xpath_ambiguous;
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
        const Glib::ustring str(eval_to_string(p_node, p_xpath));
        const int result((!str.empty() && str[0] == '#')?std::stoi(str.c_str()+1, NULL, 16):std::atoi(str.c_str()));
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
        virtual const Part::id_type &id() const { return m_id; }
        static const Part::Configurator *factory(const xmlpp::Node *);
    };

    class PartReferenceConfigurator
        : public Part::ReferenceConfigurator
    {
    protected:
        explicit PartReferenceConfigurator(const xmlpp::Node *p_node)
            : Part::ReferenceConfigurator(eval_to_string(p_node, "@href"))
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::PartReferenceConfigurator::PartReferenceConfigurator(" << p_node << ")");
            }
    public:
        virtual ~PartReferenceConfigurator() = default;
    };

    class DeviceConfigurator
        : public virtual Device::Configurator
        , protected PartConfigurator
    {
    protected:
        explicit DeviceConfigurator(const Glib::ustring p_id, const xmlpp::Node *p_node=0)
            : PartConfigurator(p_id, p_node)
            {}
    public:
        virtual ~DeviceConfigurator() = default;
        static const Device::Configurator *factory(const xmlpp::Node *);
    };

    class DeviceRefConfigurator
        : public Device::ReferenceConfigurator
    {
    public:
        explicit DeviceRefConfigurator(const xmlpp::Node *p_node)
            : Device::ReferenceConfigurator(eval_to_string(p_node, "@href"))
            {}
        virtual ~DeviceRefConfigurator() = default;
        static const Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
            { return new DeviceRefConfigurator(p_node); }
    };

    class MemoryConfigurator
        : public virtual Memory::Configurator
        , protected DeviceConfigurator
    {
    protected:
        explicit MemoryConfigurator(const Glib::ustring p_id, const xmlpp::Node *p_node=0)
            : DeviceConfigurator(p_id, p_node)
            {}
    public:
        virtual ~MemoryConfigurator() = default;
        static const Memory::Configurator *factory(const xmlpp::Node *);
    };

    class MemoryRefConfigurator
        : public virtual Memory::ReferenceConfigurator
    {
    public:
        explicit MemoryRefConfigurator(const xmlpp::Node *p_node)
            : Memory::ReferenceConfigurator(eval_to_string(p_node, "@href"))
            {}
        virtual ~MemoryRefConfigurator() = default;
        static const Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
            { return new MemoryRefConfigurator(p_node); }
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
        static const Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
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
        static const Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
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
        static const Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
            { return new PpiaConfigurator(p_node); }
    };

    class AddressSpaceConfigurator
        : public virtual AddressSpace::Configurator
        , private MemoryConfigurator
    {
    private:
        word m_size;
        const AddressSpace::Configurator::Mapping m_last_memory;
        std::vector<AddressSpace::Configurator::Mapping *> m_memory;
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
                    const auto child_elm = dynamic_cast<const xmlpp::Element *>(child);
                    if (child_elm && child_elm->get_name() == "map")
                    {
                        AddressSpace::Configurator::Mapping *map = new AddressSpace::Configurator::Mapping(m_last_memory);
                        map->base = eval_to_int(child_elm, "e:base");
                        try { map->size = eval_to_int(child_elm, "e:size"); }
                        catch (XpathNotFound e) { map->size = 0; }
                        const auto &ns(child_elm->find("e:ram|e:rom|e:ppia|e:address_space|e:memory", namespaces));
                        assert (ns.size() == 1);
                        map->memory = MemoryConfigurator::factory(ns[0]);
                        m_memory.push_back(map);
                    }
                }
            }
        virtual ~AddressSpaceConfigurator()
            {
                for (auto it = m_memory.begin(); it != m_memory.end(); it = m_memory.erase(it))
                {
                    delete (*it)->memory;
                    delete *it;
                }
                assert (m_memory.empty());
            }
        inline virtual word size() const { return m_size; }
        inline virtual const AddressSpace::Configurator::Mapping &mapping(int i) const
            { return (i < int(m_memory.size())) ? *m_memory[i] : m_last_memory; }
        static const Memory::Configurator *memory_configurator_factory(const xmlpp::Node *p_node)
            { return new AddressSpaceConfigurator(p_node); }
    };

    const Memory::Configurator *MemoryConfigurator::factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::MemoryConfigurator::factory(" << p_node << ")");
        typedef const Memory::Configurator *(factory_function)(const xmlpp::Node *);
        static std::map<std::string, factory_function *> factory_map;
        if (factory_map.size() == 0)
        {
            factory_map["memory"]        = MemoryRefConfigurator::memory_configurator_factory;
            factory_map["ram"]           = RamConfigurator::memory_configurator_factory;
            factory_map["rom"]           = RomConfigurator::memory_configurator_factory;
            factory_map["ppia"]          = PpiaConfigurator::memory_configurator_factory;
            factory_map["address_space"] = AddressSpaceConfigurator::memory_configurator_factory;
        }
        factory_function *factory = factory_map[p_node->get_name()];
        const Memory::Configurator *result(factory ? factory(p_node) : 0);
        return result;
    }

    // Devices Second

    class MCS6502Configurator
        : public virtual MCS6502::Configurator
        , private DeviceConfigurator
    {
    private:
        const Memory::Configurator *m_memory;
    public:
        explicit MCS6502Configurator(const xmlpp::Node *p_node = 0)
            : DeviceConfigurator("", p_node)
            , m_memory(0)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MCS6502Configurator::MCS6502Configurator(" << p_node << ")");
                if (p_node)
                {
                    const xmlpp::NodeSet ns(p_node->find("e:ram|e:rom|e:ppia|e:address_space|e:memory", namespaces));
                    switch (ns.size())
                    {
                    case 0:
                        break;
                    case 1:
                        m_memory = MemoryConfigurator::factory(ns[0]);
                        break;
                    default:
                        assert (false);
                        break;
                    }
                }
                if (!m_memory)
                    m_memory = new Memory::ReferenceConfigurator("address_space");
            }
        virtual ~MCS6502Configurator()
            { delete m_memory; }
        virtual const Memory::Configurator *memory() const
            { return m_memory; }
        static const Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
            { return new MCS6502Configurator(p_node); }
    };

    class AtomStreamConfigurator
        : public virtual Atom::IOStream::Configurator
        , private DeviceConfigurator
    {
    private:
        const Part::id_type m_mcs6502;
        const Memory::Configurator *m_address_space;
        bool m_pause_output;
    public:
        explicit AtomStreamConfigurator(const xmlpp::Node *p_node = 0)
            : DeviceConfigurator("stream", p_node)
            , m_mcs6502("mcs6502")
            , m_address_space(0)
            , m_pause_output(false)
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::AtomStreamConfigurator::AtomStreamConfigurator(" << p_node << ")");
                if (!m_address_space)
                    m_address_space = new Memory::ReferenceConfigurator("address_space");
            }
        virtual ~AtomStreamConfigurator()
            {
                delete m_address_space;
            }
        virtual const Part::id_type &mcs6502() const { return m_mcs6502; }
        virtual const Memory::Configurator *address_space() const { return m_address_space; }
        virtual bool pause_output() const { return m_pause_output; }

        static const Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
            { return new AtomStreamConfigurator(p_node); }
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
                    auto child_elm(dynamic_cast<xmlpp::Element *>(child));
                    if (child_elm)
                    {
                        auto child_cfgr = DeviceConfigurator::factory(child_elm);
                        if (child_cfgr)
                            m_devices.push_back(child_cfgr);
                    }
                }
            }
        virtual ~ComputerConfigurator()
            {
                for (auto &device : m_devices)
                    delete device;
                m_devices.clear();
            }
        virtual const Device::Configurator *device(int i) const
            { return (i < int(m_devices.size())) ? m_devices[i] : 0; }
        static const Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
            { return new ComputerConfigurator(p_node); }

    };

    class MonitorViewConfigurator
        : public virtual MonitorView::Configurator
        , private DeviceConfigurator
    {
    private:
        Glib::ustring m_fontfilename;
        Glib::ustring m_window_title;
        const Memory::Configurator *m_memory;
        const Memory::Configurator *m_ppia;
    public:
        explicit MonitorViewConfigurator(const xmlpp::Node *p_node = 0)
            : DeviceConfigurator("", p_node)
            , m_fontfilename("")
            , m_window_title("")
            {
                LOG4CXX_INFO(cpptrace_log(), "Xml::MonitorViewConfigurator::MonitorViewConfigurator(" << p_node << ")");
                if (p_node)
                {
                    try { m_fontfilename = eval_to_string(p_node, "e:font/filename"); }
                    catch (XpathNotFound e) {}
                    try { m_window_title = eval_to_string(p_node, "e:window_title"); }
                    catch (XpathNotFound e) {}
                    const xmlpp::NodeSet video_memory_ns(p_node->find("e:video_memory", namespaces));
                    switch (video_memory_ns.size())
                    {
                    case 0:
                        break;
                    case 1:
                        for (auto child : video_memory_ns.front()->get_children())
                        {
                            m_memory = MemoryConfigurator::factory(child);
                            if (m_memory)
                                break;
                        }
                        break;
                    default:
                        assert (false);
                        break;
                    }
                    const xmlpp::NodeSet controller_ns(p_node->find("e:controller", namespaces));
                    switch (controller_ns.size())
                    {
                    case 0:
                        break;
                    case 1:
                        for (auto child : controller_ns.front()->get_children())
                        {
                            m_ppia = MemoryConfigurator::factory(child);
                            if (m_ppia)
                                break;
                        }
                        break;
                    default:
                        assert (false);
                        break;
                    }
                }
                if (m_fontfilename.empty())
                    m_fontfilename = "mc6847.bmp";
                if (m_window_title.empty())
                    m_window_title = "Emulator";
                if (!m_memory)
                    m_memory = new Memory::ReferenceConfigurator("video");
                if (!m_ppia)
                    m_ppia = new Memory::ReferenceConfigurator("ppia");
            }
        virtual ~MonitorViewConfigurator()
            {
                delete m_memory;
                delete m_ppia;
            }
        const Glib::ustring        &fontfilename() const { return m_fontfilename; }
        const Glib::ustring        &window_title() const { return m_window_title; }
        const Memory::Configurator *memory()       const { return m_memory; }
        const Memory::Configurator *ppia()         const { return m_ppia; }
        static const Device::Configurator *device_configurator_factory(const xmlpp::Node *p_node)
            { return new MonitorViewConfigurator(p_node); }
    };

    const Device::Configurator *DeviceConfigurator::factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::DeviceConfigurator::factory(" << p_node << ")");
        typedef const Device::Configurator *(factory_function)(const xmlpp::Node *);
        static std::map<Glib::ustring, factory_function *> factory_map;
        if (factory_map.size() == 0)
        {
            factory_map["device"]   = DeviceRefConfigurator::device_configurator_factory;
            factory_map["mcs6502"]  = MCS6502Configurator::device_configurator_factory;
            factory_map["stream"]   = AtomStreamConfigurator::device_configurator_factory;
            factory_map["computer"] = ComputerConfigurator::device_configurator_factory;
            factory_map["monitor"]  = MonitorViewConfigurator::device_configurator_factory;
        }
        factory_function *factory = factory_map[p_node->get_name()];
        const Device::Configurator *result(factory ? factory(p_node) : MemoryConfigurator::factory(p_node));
        return result;
    }

    // Other Parts last

    const Part::Configurator *PartConfigurator::factory(const xmlpp::Node *p_node)
    {
        LOG4CXX_INFO(cpptrace_log(), "Xml::PartConfigurator::factory(" << p_node << ")");
        typedef const Part::Configurator *(factory_function)(const xmlpp::Node *);
        static std::map<const Glib::ustring, factory_function *> factory_map;
        factory_function *factory = factory_map[p_node->get_name()];
        const Part::Configurator *result(factory ? factory(p_node) : DeviceConfigurator::factory(p_node));
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

            xmlpp::RelaxNGValidator validator("emulator_permissive.rng");
            const xmlpp::Document *document(parser.get_document());
            validator.validate(document);

            // TODO: Check version
            const xmlpp::Node *root(parser.get_document()->get_root_node());

            for (auto *child : root->get_children())
            {
                auto child_elm(dynamic_cast<xmlpp::Element *>(child));
                if (child_elm)
                {
                    auto child_cfgr = PartConfigurator::factory(child_elm);
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
        for (auto &part : m_parts)
            delete part;
        m_parts.clear();
    }
}

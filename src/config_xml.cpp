/*
 * config.cpp
 *
 *  Created on: 20 Apr 2012
 *      Author: steve
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <cassert>
#include <log4cxx/logger.h>
#include <libxml++/libxml++.h>

#include "config_xml.hpp"

#define CFG_FNAME "atomrc.xml"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".config_xml.cpp"));
    return result;
}

RamConfigurator::RamConfigurator(const xmlpp::Node *p_node)
    : m_name("ram")
{
    LOG4CXX_INFO(cpptrace_log(), "RamConfigurator::RamConfigurator(" << p_node << ")");
    assert (p_node);
    try { m_name = p_node->eval_to_string("@name"); }
    catch (xmlpp::exception e) { /* Do Nothing */ }
    m_base = p_node->eval_to_number("base");
    m_size = p_node->eval_to_number("size");
}

RomConfigurator::RomConfigurator(const xmlpp::Node *p_node)
    : m_name("rom")
{
    LOG4CXX_INFO(cpptrace_log(), "RomConfigurator::RomConfigurator(" << p_node << ")");
    assert (p_node);
    try { m_name = p_node->eval_to_string("@name"); }
    catch (xmlpp::exception e) { /* Do Nothing */ }
    m_base = p_node->eval_to_number("base");
    m_filename = p_node->eval_to_string("filename");
    try { m_size = p_node->eval_to_number("size"); }
    catch(xmlpp::exception e)
    {
        assert (0); // get the size of the file
    }
}

PpiaConfigurator::PpiaConfigurator(const xmlpp::Node *p_node)
    : m_name("ppia")
{
    LOG4CXX_INFO(cpptrace_log(), "PpiaConfigurator::PPiaConfigurator(" << p_node << ")");
    assert (p_node);
    try { m_name = p_node->eval_to_string("@name"); }
    catch (xmlpp::exception e) { /* Do Nothing */ }
    m_base = p_node->eval_to_number("base");
    m_memory_size = p_node->eval_to_number("size");
}

MemoryConfigurator::MemoryConfigurator(const xmlpp::Node *p_node)
    : m_name("memory")
{
    LOG4CXX_INFO(cpptrace_log(), "MemoryConfigurator::MemoryConfigurator(" << p_node << ")");
    assert (p_node);
    try { m_name = p_node->eval_to_string("@name"); }
    catch (xmlpp::exception e) { /* Do Nothing */ }
}

MCS6502Configurator::MCS6502Configurator(const xmlpp::Node *p_node)
    : m_name("mcs6502")
{
    LOG4CXX_INFO(cpptrace_log(), "MCS6502Configurator::MCS6502Configurator(" << p_node << ")");
    if (p_node)
    {
        try { m_name = p_node->eval_to_string("@name"); }
        catch (xmlpp::exception e) { /* Do Nothing */ }
    }
}

AtomConfigurator::AtomConfigurator(const xmlpp::Node *p_node)
    : m_name("atom")
    , m_devices(0)
{
    LOG4CXX_INFO(cpptrace_log(), "AtomConfigurator::AtomConfigurator(" << p_node << ")");
    assert(p_node);
    try { m_name = p_node->eval_to_string("@name"); }
    catch (xmlpp::exception e) { /* Do Nothing */ }
    xmlpp::NodeSet ns(p_node->find("memorymap/*"));
    for (const xmlpp::Node *child: p_node->get_children())
    {
        const xmlpp::Element *elm = dynamic_cast<const xmlpp::Element *>(child);
        assert (elm);
        if (elm->get_name() == "ram")
            m_devices.push_back(new RamConfigurator(elm));
        else if (elm->get_name() == "rom")
            m_devices.push_back(new RomConfigurator(elm));
        else if (elm->get_name() == "ppia")
            m_devices.push_back(new PpiaConfigurator(elm));
        else
            ;// Skip
    }
}

KeyboardControllerConfigurator::KeyboardControllerConfigurator(const xmlpp::Node *p_node)
    : m_name("KeyboardController")
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardControllerConfigurator::KeyboardControllerConfigurator(" << p_node << ")");
    // Do Nothing
}

ScreenGraphicsViewConfigurator::ScreenGraphicsViewConfigurator(const xmlpp::Node *p_node)
    : m_name("ScreenGraphicsView")
    , m_scale(2.0)
    , m_fontfilename("mc6847.bmp")
    , m_window_title("Acorn Atom")
    , m_icon_title("Acorn Atom")
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsViewConfigurator::ScreenGraphicsViewConfigurator(" << p_node << ")");
    if (p_node)
    {
        try { m_scale = p_node->eval_to_number("scale"); }
        catch (xmlpp::exception e) { /* Do Nothing */ }
        try { m_fontfilename = p_node->eval_to_string("fontfilename"); }
        catch (xmlpp::exception e) { /* Do Nothing */ }
    }
}

ScreenGraphicsControllerConfigurator::ScreenGraphicsControllerConfigurator(const xmlpp::Node *p_node)
    : m_name("ScreenGraphicsController")
    , m_RefreshRate_Hz(10)
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsControllerConfigurator::ScreenGraphicsControllerConfigurator(" << p_node << ")");
    if (p_node)
    {
        try { m_RefreshRate_Hz = p_node->eval_to_number("refreshrate"); }
        catch (xmlpp::exception e) { /* Do Nothing */ }
    }
}

void Configurator::process_command_line(int argc, char *argv[])
{
    LOG4CXX_INFO(cpptrace_log(), "Configurator::process_command_line(" << argc << ", " << argv << ")");
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
        default: /* Unexpected response from getopt() */
            LOG4CXX_WARN(cpptrace_log(), "Unexpected response from getopt() " << c);
            break;
        }
}

void Configurator::process_XML()
{
    LOG4CXX_INFO(cpptrace_log(), "Configurator::process_XML()");
    try
    {
        xmlpp::DomParser parser;
        parser.parse_file(m_XMLfilename);
        if (!parser)
        {
            LOG4CXX_FATAL(cpptrace_log(), "Parse Failure");
            exit(1);
        }

        const xmlpp::Node *root(parser.get_document()->get_root_node());

        m_atom = new AtomConfigurator(root);
        try
        {
            const xmlpp::NodeSet ns(root->find("io"));
            assert (ns.size() == 0 || ns.size() == 1);
            if (ns.size() == 1)
            {
                m_keyboard = new KeyboardControllerConfigurator(ns[0]);
                m_screen = new ScreenGraphicsControllerConfigurator(ns[0]);
            }
        }
        catch (xmlpp::exception e) { /* Do Nothing */ }

#if 0
        // Keyboard Stuff
        if (!cfg->keyboard.filename)
            switch (xpath_count(ctx, "/atom/keyboard/filename")) {
            case 0: /* No keyboard stream element */
                break;
            case 1: /* keyboard streaming requested */
                xpath_rd_txt(ctx, "/atom/keyboard/filename/text()", cfg->keyboard.filename);
                if (!cfg->keyboard.filename)
                    cfg->keyboard.filename = "";
                cfg->keyboard.viewscreen = ( xpath_count(ctx, "/atom/keyboard/viewscreen") > 0 );
                break;
            default: /* Error */
                LOG_ERROR("Incorrect keyboard stream entry in configuration file");
                assert (false);
                break;
            }
#endif

    }
    catch (const std::exception& ex)
    {
        LOG4CXX_ERROR(cpptrace_log(), ex.what());
        exit(1);
    }
}

bool Configurator::check_and_complete_params()
{
    LOG4CXX_INFO(cpptrace_log(), "Configurator::check_and_complete_params()");
    bool result = true;
    if (m_screen->view().scale() && (m_screen->view().scale() < 1.0)) {
        LOG4CXX_ERROR(cpptrace_log(), "Bad Scale Factor " << m_screen->view().scale());
        result = false;
    }
#if 0
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
#endif
#if 0
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
    : m_XMLfilename(CFG_FNAME)
{
    LOG4CXX_INFO(cpptrace_log(), "Configurator::Configurator(" << argc << ", " << argv << ")");
    process_command_line(argc, argv);
    process_XML();
    if (!check_and_complete_params())
        exit(1);
    LOG4CXX_INFO(cpptrace_log(), "Position 1 => " << static_cast<const Atom::Configurator &>(*m_atom));
}

Configurator::~Configurator()
{
}

std::ostream &operator<<(std::ostream &p_s, const AtomConfigurator &p_cfg)
{
    p_s << static_cast<const Named::Configurator &>(p_cfg)
        // :TODO: << p_cfg.m_devices
        << p_cfg.m_memory
        << p_cfg.m_mcs6502;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const KeyboardControllerConfigurator &p_cfg)
{
    p_s << static_cast<const Named::Configurator &>(p_cfg);
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsViewConfigurator &p_cfg)
{
    p_s << static_cast<const Named::Configurator &>(p_cfg)
        << ", scale=" << p_cfg.m_scale
        << ", fontfilename=" << p_cfg.m_fontfilename
        << ", window_title=" << p_cfg.m_window_title
        << ". icon_title=" << p_cfg.m_icon_title;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const ScreenGraphicsControllerConfigurator &p_cfg)
{
    p_s << static_cast<const Named::Configurator &>(p_cfg)
        << p_cfg.m_view
        << ", RefreshRate_Hz=" << p_cfg.m_RefreshRate_Hz;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Configurator &p_cfg)
{
    p_s << "Configurator::Configurator("
        << static_cast<const Atom::Configurator &>(*p_cfg.m_atom) << ", "
        << p_cfg.m_keyboard << ", "
        << p_cfg.m_screen << ")";
    return p_s;
}

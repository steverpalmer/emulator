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

RamConfigurator::RamConfigurator(const std::string &p_name, word p_base, word p_size)
    : m_name(p_name)
    , m_base(p_base)
    , m_size(p_size)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "RamConfigurator::RamConfigurator(\""
                 << p_name << "\", "
                 << p_base << ", "
                 << p_size << ")");
}

RomConfigurator::RomConfigurator(const std::string &p_name, word p_base, word p_size, std::string p_filename)
    : m_name(p_name)
    , m_base(p_base)
    , m_size(p_size)
    , m_filename(p_filename)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "RomConfigurator::RomConfigurator(\""
                 << p_name << "\", "
                 << p_base << ", "
                 << p_size << ", \""
                 << p_filename << "\")");
}

PpiaConfigurator::PpiaConfigurator(const std::string &p_name)
    : m_name(p_name)
    , m_base(0xB000)
    , m_memory_size(0x0400)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "PpiaConfigurator::PpiaConfigurator(\""
                 << p_name << "\")");
}

MemoryConfigurator::MemoryConfigurator(const std::string &p_name)
    : m_name(p_name)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "MemoryConfigurator::MemoryConfigurator(\""
                 << p_name << "\")");
}

MCS6502Configurator::MCS6502Configurator(const std::string &p_name)
    : m_name(p_name)
{
    LOG4CXX_INFO(cpptrace_log(),
                 "MCS6502Configurator::MCS6502Configurator(\""
                 << p_name << "\")");
}

AtomConfigurator::AtomConfigurator(const std::string &p_name)
    : m_name(p_name)
    , m_devices(0)
    , m_block0("block0", 0x0000, 0x0400)
    , m_lower( "lower",  0x2800, 0x1400)
    , m_video( "video",  0x8000, 0x1800)
    , m_basic( "basic",  0xC000, 0x1000, "basic.rom")
    , m_float( "float",  0xD000, 0x1000, "float.rom")
    , m_kernel("kernel", 0xF000, 0x1000, "kernel.rom")
{
    LOG4CXX_INFO(cpptrace_log(),
                 "AtomConfigurator::AtomConfigurator(\""
                 << p_name << "\")");
    m_devices.push_back(&m_block0);
    m_devices.push_back(&m_lower);
    m_devices.push_back(&m_video);
    m_devices.push_back(&m_ppia);
    m_devices.push_back(&m_basic);
    m_devices.push_back(&m_float);
    m_devices.push_back(&m_kernel);
}

KeyboardControllerConfigurator::KeyboardControllerConfigurator()
    : m_name("KeyboardController")
{
    LOG4CXX_INFO(cpptrace_log(), "KeyboardControllerConfigurator::KeyboardControllerConfigurator()");
}

ScreenGraphicsViewConfigurator::ScreenGraphicsViewConfigurator()
    : m_name("ScreenGraphicsView")
    , m_scale(2.0)
    , m_fontfilename("mc6847.bmp")
    , m_window_title("Acorn Atom")
    , m_icon_title("Acorn Atom")
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsViewConfigurator::ScreenGraphicsViewConfigurator()");
}

ScreenGraphicsControllerConfigurator::ScreenGraphicsControllerConfigurator()
    : m_name("ScreenGraphicsController")
    , m_RefreshRate_ms(100)
{
    LOG4CXX_INFO(cpptrace_log(), "ScreenGraphicsControllerConfigurator::ScreenGraphicsControllerConfigurator()");
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
 
static int xpath_count(xmlpp::Node &ctx, const std::string &path)
{
    LOG4CXX_INFO(cpptrace_log(), "xpath_count(_, " << path << ")");
    const int result = ctx.find(path).size();
    return result;
}

static void xpath_rd_txt(xmlpp::Node &ctx, const std::string &path, std::string &val)
{
    LOG4CXX_INFO(cpptrace_log(), "xpath_rd_txt(_, " << path << ", _)");
    xmlpp::NodeSet ns = ctx.find(path);
    if (ns.size() > 0)
    {
        assert (ns.size() == 1);
        const xmlpp::TextNode * n = dynamic_cast<xmlpp::TextNode *>(ns[0]);
        assert (n);
        val = n->get_content();
    }
}

static void xpath_rd_guint16(xmlpp::Node &ctx, const std::string &path, word &val)
{
    LOG4CXX_INFO(cpptrace_log(), "xpath_rd_guint16(_, " << path << ", _)");
    std::string txt;
    xpath_rd_txt(ctx, path, txt);
    std::stringstream convertor(txt);
    word result;
    if (convertor >> result)
        val = result;
}

static void xpath_rd_float(xmlpp::Node &ctx, const std::string &path, float &val)
{
    LOG4CXX_INFO(cpptrace_log(), "xpath_rd_float(_, " << path << ", _)");
    std::string txt;
    xpath_rd_txt(ctx, path, txt);
    std::stringstream convertor(txt);
    float result;
    if (convertor >> result)
        val = result;
}

#if 0
static void xml_callback_error(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LOG4CXX_ERROR(cpptrace_log(), msg);
    va_end(ap);
}

static void xml_callback_warn(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LOG4CXX_WARN(cpptrace_log(), msg);
    va_end(ap);
}
#endif

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

        const xmlpp::Node *pNode = parser.get_document()->get_root_node();
        m_atom.m_block0.m_base = strtol(pNode->eval_to_string("/atom/memorymap/ram[@name='block0']/base/text()").data(), 0, 0);

#if 0
    FILE *dummy = fopen(m_XMLfilename, "r");
    if (dummy && (getc(dummy) != EOF)) {
        fclose(dummy);
        xmlDocPtr doc = xmlParseFile(m_XMLfilename);
        if (!doc) {
            LOG4CXX_ERROR(cpptrace_log(), "Failed to parse configuration file \"" << m_XMLfilename << "\"");
            return;
        }
        xmlDocPtr atomRNGdoc = xmlParseFile("atom.rng");
        if (!atomRNGdoc) {
            LOG4CXX_ERROR(cpptrace_log(), "Failed to parse Relax NG schema file \"atom.rng\" - Stage 1");
        }
        else {
            xmlRelaxNGParserCtxtPtr atomRNGParserCtxt = xmlRelaxNGNewDocParserCtxt(atomRNGdoc);
            if (!atomRNGParserCtxt) {
                LOG4CXX_ERROR(cpptrace_log(), "Failed to parse Relax NG schema file \"atom.rng\" - Stage 2");
            }
            else {
                xmlRelaxNGSetParserErrors(atomRNGParserCtxt, xml_callback_error, xml_callback_warn, 0);
                xmlRelaxNGPtr atomRNGPtr = xmlRelaxNGParse(atomRNGParserCtxt);
                if (!atomRNGPtr) {
                    LOG4CXX_ERROR(cpptrace_log(), "Failed to parse Relax NG schema file \"atom.rng\" - Stage 3");
                }
                else {
                    xmlRelaxNGValidCtxtPtr atomRNGValidCtx = xmlRelaxNGNewValidCtxt(atomRNGPtr);
                    if (!atomRNGValidCtx) {
                        LOG4CXX_ERROR(cpptrace_log(), "Failed to parse Relax NG schema file \"atom.rng\" - Stage 4");
                    }
                    else {
                        xmlRelaxNGSetValidErrors(atomRNGValidCtx, xml_callback_error, xml_callback_warn, 0);
                        const int atomRNGValid = xmlRelaxNGValidateDoc(atomRNGValidCtx, doc);
                        if (atomRNGValid) {
                            LOG4CXX_ERROR(cpptrace_log(), "Failed to validate file \"" << m_XMLfilename << "\": " << atomRNGValid);
                        }
                        xmlRelaxNGFreeValidCtxt(atomRNGValidCtx);
                    }
                    xmlRelaxNGFree(atomRNGPtr);
                }
                xmlRelaxNGFreeParserCtxt(atomRNGParserCtxt);
            }
            xmlFreeDoc(atomRNGdoc);
        }
        xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
        if (!ctx) {
            LOG4CXX_ERROR(cpptrace_log(), "Failed to generate new XPath context");
            return;
        }

        // Atom Stuff
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='block0']/base/text()",     &m_atom.m_block0.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='block0']/size/text()",     &m_atom.m_block0.m_size);
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='lower']/base/text()",      &m_atom.m_lower.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='lower']/size/text()",      &m_atom.m_lower.m_size);
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='video']/base/text()",      &m_atom.m_video.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/ram[@name='video']/size/text()",      &m_atom.m_video.m_size);
        xpath_rd_guint16(ctx, "/atom/memorymap/ppia/base/text()",                    &m_atom.m_ppia.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/ppia/size/text()",                    &m_atom.m_ppia.m_memory_size);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='basic']/base/text()",      &m_atom.m_basic.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='basic']/size/text()",      &m_atom.m_basic.m_size);
        xpath_rd_txt(ctx, "/atom/memorymap/rom[@name='basic']/filename/text()",      m_atom.m_basic.m_filename);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='float']/base/text()",      &m_atom.m_float.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='float']/size/text()",      &m_atom.m_float.m_size);
        xpath_rd_txt(ctx, "/atom/memorymap/rom[@name='float']/filename/text()",      m_atom.m_float.m_filename);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='kernel']/base/text()",     &m_atom.m_kernel.m_base);
        xpath_rd_guint16(ctx, "/atom/memorymap/rom[@name='kernel']/size/text()",     &m_atom.m_kernel.m_size);
        xpath_rd_txt(ctx, "/atom/memorymap/rom[@name='kernel']/filename/text()",   m_atom.m_kernel.m_filename);

        // Screen Stuff
        float screen_scale;
        xpath_rd_float  (ctx, "/atom/io/scale/text()", &screen_scale);

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

        xmlXPathFreeContext(ctx);
        xmlFreeDoc(doc);
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
    if (m_screen.m_view.m_scale && (m_screen.m_view.m_scale < 1.0)) {
        LOG4CXX_ERROR(cpptrace_log(), "Bad Scale Factor " << m_screen.m_view.m_scale);
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
    LOG4CXX_INFO(cpptrace_log(), "Position 1 => " << static_cast<const Atom::Configurator &>(m_atom));
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
        << ", RefreshRate_ms=" << p_cfg.m_RefreshRate_ms;
    return p_s;
}

std::ostream &operator<<(std::ostream &p_s, const Configurator &p_cfg)
{
    p_s << "Configurator::Configurator("
        << static_cast<const Atom::Configurator &>(p_cfg.m_atom) << ", "
        << p_cfg.m_keyboard << ", "
        << p_cfg.m_screen << ")";
    return p_s;
}

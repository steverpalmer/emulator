/*
 * config.cpp
 *
 *  Created on: 20 Apr 2012
 *      Author: steve
 */

#if 0
Configurator::Configurator(int argc, char *argv[])
{

}
#endif

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/relaxng.h>
#include <libxml/xpath.h>

#include "config_xml.hpp"

#include "assert.h"

#define CFG_FNAME "atomrc"

static log4c_category_t *ctracelog;

#define CFG_CTRACE_LOG(...) CTRACE_LOG(ctracelog, __VA_ARGS__)

struct My_Cfg {
    const char *   progname;
    const char *   XMLname;
    struct {
        float      scale;
        char       *filename;
    }              screen;
    struct {
        bool   viewscreen;
        char       *filename;
    }              keyboard;
    struct AtomCfg atom;
    struct {
        Scr        scr;
        Kbd        kbd;
        Atom       *atom;
    }              build;
};


/******************************************************************************/
static void init_params(Cfg cfg)
/******************************************************************************/
{
    CFG_CTRACE_LOG("init_params()");
    assert (cfg);
    cfg->progname          = 0;
    cfg->XMLname           = CFG_FNAME;
    cfg->screen.scale      = 0.0;
    cfg->screen.filename   = 0;
    cfg->keyboard.viewscreen = true;
    cfg->keyboard.filename = 0;
    cfg->atom.block0_base  = 0x0000;
    cfg->atom.block0_size  = 0x0000;
    cfg->atom.lower_base   = 0x0000;
    cfg->atom.lower_size   = 0x0000;
    cfg->atom.video_base   = 0x0000;
    cfg->atom.video_size   = 0x0000;
    cfg->atom.ppia_base    = 0x0000;
    cfg->atom.ppia_size    = 0x0000;
    cfg->atom.basic_base   = 0x0000;
    cfg->atom.basic_size   = 0x0000;
    cfg->atom.basic_fname  = 0;
    cfg->atom.float_base   = 0x0000;
    cfg->atom.float_size   = 0x0000;
    cfg->atom.float_fname  = 0;
    cfg->atom.kernel_base  = 0x0000;
    cfg->atom.kernel_size  = 0x0000;
    cfg->atom.kernel_fname = 0;
    cfg->atom.hooks        = 0;
    cfg->build.scr  = NULL;
    cfg->build.kbd  = NULL;
    cfg->build.atom = NULL;
    cfg_dump(ctracelog, cfg);
}


/******************************************************************************/
static void process_command_line(Cfg cfg, int argc, char *argv[])
/******************************************************************************/
{
    CFG_CTRACE_LOG("process_command_line(%d, ...)", argc);
    cfg->progname = argv[0];
    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "i:o:f:")) != -1)
        switch (c) {
        case 'i': /* Stream Input from ... */
            cfg->keyboard.filename = (strcmp(optarg, "-")?optarg:"");
            cfg->keyboard.viewscreen = true;
            break;
        case 'o': /* Stream Output to ... */
            cfg->screen.filename = (strcmp(optarg, "-")?optarg:"");
            cfg->keyboard.viewscreen = false;
            break;
        case 'f': /* Use Config File ... */
            cfg->XMLname = optarg;
            break;
        case '?': /* Unknown Option */
            LOG_WARN("Unknown Option '%c'", optopt);
            break;
        default: /* Unexpected response from getopt() */
            LOG_WARN("Unexpected responce from getopt() %d", c);
            break;
        }
    cfg_dump(ctracelog, cfg);
}

/******************************************************************************/
static int xpath_count(xmlXPathContextPtr ctx, const char *path)
/******************************************************************************/
{
    CFG_CTRACE_LOG("xpath_count(%p, \"%s\")", ctx, path);
    int result = -1;
    /* FIXME: Dodgy Cast */
    xmlXPathObjectPtr n = xmlXPathEvalExpression((const xmlChar *)path, ctx);
    if (!n)
        LOG_ERROR("Failed to read \"%s\"", path);
    else {
        assert (n->type == XPATH_NODESET);
        result = xmlXPathNodeSetGetLength(n->nodesetval);
        xmlXPathFreeObject(n);
    }
    return result;
}


/******************************************************************************/
static void xpath_rd_txt(xmlXPathContextPtr ctx, const char *path, char **valptr)
/******************************************************************************/
{
    CFG_CTRACE_LOG("xpath_rd_txt(%p, \"%s\", %p)", ctx, path, valptr);
    assert (valptr);
    /* FIXME: Dodgy Cast */
    xmlXPathObjectPtr n = xmlXPathEvalExpression((const xmlChar *)path, ctx);
    if (!n) {
        LOG_ERROR("Failed to read \"%s\"", path);
        *valptr = 0;
    }
    else {
        assert (n->type == XPATH_NODESET);
        if (xmlXPathNodeSetGetLength(n->nodesetval) == 1) {
            assert (xmlNodeIsText(xmlXPathNodeSetItem(n->nodesetval, 0)));
            /* FIXME: Second Dodgy Cast */
            *valptr = (char *)xmlNodeGetContent(xmlXPathNodeSetItem(n->nodesetval, 0));
            assert (*valptr);
        }
        xmlXPathFreeObject(n);
    }
}


/******************************************************************************/
static void xpath_rd_guint16(xmlXPathContextPtr ctx, const char *path, word *valptr)
/******************************************************************************/
{
    CFG_CTRACE_LOG("xpath_rd_guint16(%p, \"%s\", %p)", ctx, path, valptr);
    char * txt = 0;
    xpath_rd_txt(ctx, path, &txt);
    if (txt)
        {
            const int rv = sscanf(txt, "%hx", valptr);
            assert(rv != EOF);
            xmlFree(txt);
        }
}


/******************************************************************************/
static void xpath_rd_float(xmlXPathContextPtr ctx, const char *path, float *valptr)
/******************************************************************************/
{
    CFG_CTRACE_LOG("xpath_rd_float(%p, \"%s\", %p)", ctx, path, valptr);
    char * txt = 0;
    xpath_rd_txt(ctx, path, &txt);
    if (txt)
        {
            const int rv = sscanf(txt, "%f", valptr);
            assert(rv != EOF);
            xmlFree(txt);
        }
}


static void xml_callback_error(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    log4c_category_vlog(rootlog, LOG4C_PRIORITY_ERROR, msg, ap);
    va_end(ap);
}

static void xml_callback_warn(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    log4c_category_vlog(rootlog, LOG4C_PRIORITY_WARN, msg, ap);
    va_end(ap);
}

/******************************************************************************/
static void process_XML( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("process_XML()");
    FILE *dummy = fopen(cfg->XMLname, "r");
    if (dummy && (getc(dummy) != EOF))
        {
            fclose(dummy);
            xmlDocPtr doc = xmlParseFile(cfg->XMLname);
            if (!doc) {
                LOG_ERROR("Failed to parse configuration file \"%s\"", cfg->XMLname);
                return;
            }
            xmlDocPtr atomRNGdoc = xmlParseFile("atom.rng");
            if (!atomRNGdoc) LOG_ERROR("Failed to parse Relax NG schema file \"atom.rng\" - Stage 1");
            else {
                xmlRelaxNGParserCtxtPtr atomRNGParserCtxt = xmlRelaxNGNewDocParserCtxt(atomRNGdoc);
                if (!atomRNGParserCtxt) LOG_ERROR("Failed to parse Relax NG schema file \"atom.rng\" - Stage 2");
                else {
                    xmlRelaxNGSetParserErrors(atomRNGParserCtxt, xml_callback_error, xml_callback_warn, 0);
                    xmlRelaxNGPtr atomRNGPtr = xmlRelaxNGParse(atomRNGParserCtxt);
                    if (!atomRNGPtr) LOG_ERROR("Failed to parse Relax NG schema file \"atom.rng\" - Stage 3");
                    else {
                        xmlRelaxNGValidCtxtPtr atomRNGValidCtx = xmlRelaxNGNewValidCtxt(atomRNGPtr);
                        if (!atomRNGValidCtx) LOG_ERROR("Failed to parse Relax NG schema file \"atom.rng\" - Stage 4");
                        else {
                            xmlRelaxNGSetValidErrors(atomRNGValidCtx, xml_callback_error, xml_callback_warn, 0);
                            const int atomRNGValid = xmlRelaxNGValidateDoc(atomRNGValidCtx, doc);
                            if (atomRNGValid) LOG_ERROR("Failed to validate file \"%s\": %d", cfg->XMLname, atomRNGValid);
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
                LOG_ERROR("Failed to generate new XPath context");
                return;
            }
            xpath_rd_float  (ctx, "/atom/screen/scale/text()", &cfg->screen.scale);
            if (!cfg->screen.filename)
                switch (xpath_count(ctx, "/atom/screen/filename")) {
                case 0: /* No screen filename element */
                    break;
                case 1: /* screen filename requested */
                    xpath_rd_txt    (ctx, "/atom/screen/filename/text()", &cfg->screen.filename);
                    if (!cfg->screen.filename)
                        cfg->screen.filename = "";
                    break;
                default: /* Error */
                    LOG_ERROR("Incorrect screen stream entry in configuration file");
                    assert (false);
                    break;
                }
            if (!cfg->keyboard.filename)
                switch (xpath_count(ctx, "/atom/keyboard/filename")) {
                case 0: /* No keyboard stream element */
                    break;
                case 1: /* keyboard streaming requested */
                    xpath_rd_txt    (ctx, "/atom/keyboard/filename/text()", &cfg->keyboard.filename);
                    if (!cfg->keyboard.filename)
                        cfg->keyboard.filename = "";
                    cfg->keyboard.viewscreen = ( xpath_count(ctx, "/atom/keyboard/viewscreen") > 0 );
                    break;
                default: /* Error */
                    LOG_ERROR("Incorrect keyboard stream entry in configuration file");
                    assert (false);
                    break;
                }
            xpath_rd_guint16(ctx, "/atom/ram[@name='block0']/base/text()",     &cfg->atom.block0_base);
            xpath_rd_guint16(ctx, "/atom/ram[@name='block0']/size/text()",     &cfg->atom.block0_size);
            xpath_rd_guint16(ctx, "/atom/ram[@name='lower']/base/text()",      &cfg->atom.lower_base);
            xpath_rd_guint16(ctx, "/atom/ram[@name='lower']/size/text()",      &cfg->atom.lower_size);
            xpath_rd_guint16(ctx, "/atom/ram[@name='video']/base/text()",      &cfg->atom.video_base);
            xpath_rd_guint16(ctx, "/atom/ram[@name='video']/size/text()",      &cfg->atom.video_size);
            xpath_rd_guint16(ctx, "/atom/ppia/base/text()",                    &cfg->atom.ppia_base);
            xpath_rd_guint16(ctx, "/atom/ppia/size/text()",                    &cfg->atom.ppia_size);
            xpath_rd_guint16(ctx, "/atom/rom[@name='basic']/base/text()",      &cfg->atom.basic_base);
            xpath_rd_guint16(ctx, "/atom/rom[@name='basic']/size/text()",      &cfg->atom.basic_size);
            xpath_rd_txt    (ctx, "/atom/rom[@name='basic']/filename/text()",  &cfg->atom.basic_fname);
            xpath_rd_guint16(ctx, "/atom/rom[@name='float']/base/text()",      &cfg->atom.float_base);
            xpath_rd_guint16(ctx, "/atom/rom[@name='float']/size/text()",      &cfg->atom.float_size);
            xpath_rd_txt    (ctx, "/atom/rom[@name='float']/filename/text()",  &cfg->atom.float_fname);
            xpath_rd_guint16(ctx, "/atom/rom[@name='kernel']/base/text()",     &cfg->atom.kernel_base);
            xpath_rd_guint16(ctx, "/atom/rom[@name='kernel']/size/text()",     &cfg->atom.kernel_size);
            xpath_rd_txt    (ctx, "/atom/rom[@name='kernel']/filename/text()", &cfg->atom.kernel_fname);
            xmlXPathFreeContext(ctx);
            xmlFreeDoc(doc);
            cfg_dump(ctracelog, cfg);
        }
}


/******************************************************************************/
static bool check_and_complete_params( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("check_and_complete_params()");
    bool result = true;
    struct HookNode **ptr = &cfg->atom.hooks;
    if (cfg->screen.scale && (cfg->screen.scale < 1.0)) {
        LOG_ERROR("Bad Scale Factor %f", cfg->screen.scale);
        result = false;
    }
    if (cfg->screen.filename)
        {
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
    if (cfg->keyboard.filename)
        {
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
    const int err = atom_config_error(&cfg->atom);
    if (err)
        {
            LOG_ERROR("Invalid configuration %d", err);
            atom_config_dump(ctracelog, &cfg->atom);
            result = false;
        }
    cfg_dump(ctracelog, cfg);
    return result;
}

/******************************************************************************/
void cfg_init(void)
/******************************************************************************/
{
    ctracelog = log4c_category_get(LOGNAME_CTRACE("config"));
    CFG_CTRACE_LOG("cfg_init()");
    xmlInitParser();
}


/******************************************************************************/
Cfg cfg_new ( int argc, char *argv[] )
/******************************************************************************/
/*
 * Returns NULL if there is a problem with the requested configuration.
 * In which case it should log some appropriate message.
 * It does not do the actual build!
 */
{
    CFG_CTRACE_LOG("cfg_new(%d, ...)", argc);
    Cfg result = 0;
    result = malloc(sizeof(*result));
    if (result) {
        init_params(result);
        process_command_line(result, argc, argv);
        process_XML(result);
        if (!check_and_complete_params(result))
            result = 0;
    }
    CFG_CTRACE_LOG("cfg_new => %p", result);
    return result;
}


/******************************************************************************/
bool cfg_del ( Cfg cfg )
/******************************************************************************/
/*
 * This can be called straight after the accessors to free up used memory.
 * In particular, it has no responsibility for freeing the built system.
 */
{
    CFG_CTRACE_LOG("cfg_del()");
    bool result = false;
    if (cfg) {
        struct HookNode *head = cfg->atom.hooks;
        while (head)
            {
                struct HookNode *tmp = head;
                head = head->next;
                free (tmp);
            }
        xmlCleanupParser();
        result = true;
    }
    return result;
}


/******************************************************************************/
void cfg_deinit(void)
/******************************************************************************/
{
    CFG_CTRACE_LOG("config_deinit()");
}


/******************************************************************************/
void cfg_dump ( log4c_category_t *log, Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("cfg_dump()");
    if (!log)
        log = log4c_category_get(LOGNAME_DUMP("Cfg"));
    if (cfg) {
        log4c_category_info(log, "Program Name:        \"%s\"", cfg->progname);
        log4c_category_info(log, "Config File:         \"%s\"", cfg->XMLname);
        log4c_category_info(log, "Screen Scale:        %f",     cfg->screen.scale);
        log4c_category_info(log, "Screen Stream:       \"%s\"", cfg->screen.filename);
        log4c_category_info(log, "Keyboard Stream:     \"%s\"", cfg->keyboard.filename);
        log4c_category_info(log, "Keyboard viewscreen: %d",     cfg->keyboard.viewscreen);
        atom_config_dump(log, &cfg->atom);
        if (cfg->build.scr)  scr_dump(log, cfg->build.scr);
        if (cfg->build.kbd)  kbd_dump(log, cfg->build.kbd);
        if (cfg->build.atom) atom_dump(log, cfg->build.atom);
    }
    else
        log4c_category_info(log, "NULL Cfg");
}


/******************************************************************************/
bool cfg_build ( Cfg cfg )
/******************************************************************************/
/*
 * Builds the execution system according to the configuration.
 * Returns false if it fails to build for some reason.
 * In which case it should log some appropriate message.
 */
{
    CFG_CTRACE_LOG("cfg_build()");
    cfg->build.scr = scr_new(cfg->screen.scale);
    if (!cfg->build.scr) {
        LOG_ERROR("Failed to build screen object");
        return false;
    }
    cfg->build.kbd = kbd_new();
    if (!cfg->build.kbd) {
        LOG_ERROR("Failed to build keyboard object");
        return false;
    }
    cfg->build.atom = atom_new(cfg->build.kbd, cfg->build.scr, &cfg->atom);
    if (!cfg->build.atom) {
        LOG_ERROR("Failed to build atom object");
        return false;
    }
    return true;
}


/******************************************************************************/
Scr cfg_get_scr ( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("cfg_get_scr()");
    return cfg->build.scr;
}


/******************************************************************************/
Kbd cfg_get_kbd ( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("cfg_get_kbd()");
    return cfg->build.kbd;
}


/******************************************************************************/
Atom *cfg_get_atom ( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("cfg_get_atom()");
    return cfg->build.atom;
}


/******************************************************************************/
bool cfg_get_viewscreen ( Cfg cfg )
/******************************************************************************/
{
    CFG_CTRACE_LOG("cfg_get_viewscreen()");
    return cfg->keyboard.viewscreen;
}

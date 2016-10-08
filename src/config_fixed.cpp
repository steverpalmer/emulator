// config_fixed.cpp

#if 0
#include <unistd.h>
#include <log4cxx/logger.h>
#endif

#include "config_fixed.hpp"

#if 0
volatile char *g_XMLfilename;

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".config_xml.cpp"));
    return result;
}

void process_command_line(int argc, char *argv[])
{
    LOG4CXX_INFO(cpptrace_log(), "process_command_line(" << argc << ", " << argv << ")");
    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "f:")) != -1)
        switch (c) {
        case 'f': /* Use Config File ... */
            g_XMLfilename = optarg;
            break;
        case '?': /* Unknown Option */
            LOG4CXX_WARN(cpptrace_log(), "Unknown Option '" << optopt << "'");
            break;
        default: /* Unexpected response from getopt() */
            LOG4CXX_WARN(cpptrace_log(), "Unexpected responce from getopt() " << c);
            break;
        }
}
#endif

Configurator::Configurator(int argc, char *argv[])
{
#if 0
	process_command_line(argc, argv);
#endif
}

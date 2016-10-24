// config.hpp

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <string.h>
#include <libgen.h>

#include <ostream>

#include "part.hpp"

class Configurator
    : public virtual PartsBin::Configurator
{
public:
    std::string command;
private:
    char * const command_copy;
public:
    std::string command_dir;
protected:
    Configurator(int argc, char *argv[])
        : command(*argv)
        , command_copy(new char[command.length()+1])
        {
            (void) strcpy(command_copy, command.c_str());
            command_dir = dirname(command_copy);
            delete command_copy;
        }
public:
    virtual ~Configurator() = default;
};

#endif

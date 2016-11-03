// config.hpp

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <string.h>
#include <libgen.h>
#include <stdlib.h>

#include <cassert>

#include "common.hpp"
#include "part.hpp"

class Configurator
    : public virtual PartsBin::Configurator
{
protected:
    const std::string command;
private:
    char * command_copy;
protected:
    std::string command_dir;
protected:
    Configurator(int argc, char *argv[])
        : command(*argv)
        , command_copy(0)
        {
            command_copy = (char *)malloc(command.length()+1);
            assert (command_copy);
            (void) strcpy(command_copy, command.c_str());
            command_dir = dirname(command_copy);
            free(command_copy);
        }
public:
    virtual ~Configurator() = default;
};

#endif

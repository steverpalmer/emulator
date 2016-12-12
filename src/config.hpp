// config.hpp
// Copyright 2016 Steve Palmer

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
        , command_copy(nullptr)
        {
            command_copy = (char *)malloc(command.length()+1);
            assert (command_copy);
            (void) strcpy(command_copy, command.c_str());
            command_dir = dirname(command_copy);
            free(command_copy);
        }
private:
    Configurator();
public:
    virtual ~Configurator() = default;
    virtual bool build_only() const = 0;
    virtual void serialize(std::ostream &p_s) const
    {
    	p_s << "<emulator>";
    	if (build_only())
    		p_s << "<build_only/>";
    	PartsBin::Configurator::serialize(p_s);
    	p_s << "</emulator>";
    }
    friend std::ostream &operator<<(std::ostream &p_s, const Configurator &p_cfgr)
        { p_cfgr.serialize(p_s); return p_s; }
};

#endif

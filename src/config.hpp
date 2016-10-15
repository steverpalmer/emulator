// config.hpp

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <ostream>

#include "part.hpp"

class Configurator
    : public PartsBin::Configurator
{
protected:
    Configurator(int argc, char *argv[]) {}
public:
    virtual ~Configurator() {}

    friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
        { return p_s; }
};

#endif

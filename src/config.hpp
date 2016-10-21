// config.hpp

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <ostream>

#include "part.hpp"

class Configurator
    : public virtual PartsBin::Configurator
{
protected:
    Configurator(int argc, char *argv[]) {}
public:
    virtual ~Configurator() = default;
};

#endif

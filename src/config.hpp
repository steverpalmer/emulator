// config.hpp

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include "part.hpp"

class Configurator
    : public PartsBin::Configurator
{
protected:
    Configurator(int argc, char *argv[]);
private:
    Configurator(const Configurator &);
    Configurator &operator=(const Configurator &);
public:
    virtual ~Configurator();
};

#endif

// config_fixed.hpp

#ifndef CONFIG_FIXED_HPP_
#define CONFIG_FIXED_HPP_

#include "config.hpp"

namespace Fixed
{

    class Configurator
        : public ::Configurator
    {
    private:
        std::vector<const Part::Configurator *>m_parts;
    public:
        explicit Configurator(int argc, char *argv[]);
        virtual ~Configurator();
        virtual const Part::Configurator *part(int i) const
            { return (i < int(m_parts.size())) ? m_parts[i] : 0; }
    };

}

#endif

// config_xml.hpp

#ifndef CONFIG_XML_HPP_
#define CONFIG_XML_HPP_

#include <vector>

#include "common.hpp"
#include "part.hpp"
#include "config.hpp"

namespace Xml
{

    class Configurator
        : public virtual ::Configurator
    {
    private:  // Attributes
        Glib::ustring m_XMLfilename;
        std::vector<const Part::Configurator *>m_parts;
    private:  // Methods
        void process_command_line(int argc, char *argv[]);
        void process_XML();
        bool check_and_complete_params();
    public:
        Configurator(int argc, char *argv[]);
        virtual ~Configurator();
        virtual const Part::Configurator *part(int i) const
            { return (i < int(m_parts.size())) ? m_parts[i] : 0; }
    };

}

#endif

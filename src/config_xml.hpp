// config_xml.hpp

#ifndef CONFIG_XML_HPP_
#define CONFIG_XML_HPP_

#include <ostream>

#include <string>

#include "config.hpp"

namespace Xml
{

    class Configurator
        : public ::Configurator
    {
    private:
        std::string m_XMLfilename;
    private:
        Configurator(const Configurator &);
        Configurator &operator=(const Configurator &);
        void process_command_line(int argc, char *argv[]);
        void process_XML();
        bool check_and_complete_params();
    public:
        explicit Configurator(int argc, char *argv[]);
        virtual ~Configurator();
        Part::Configurator *part(int i) const;
    };

}

#endif /* CONFIG_XML_HPP_ */

/*
 * config.hpp
 *
 *  Created on: 20 Apr 2012
 *      Author: steve
 */

#ifndef CONFIG_XML_HPP_
#define CONFIG_XML_HPP_

#include <ostream>
#include <string>

#include "common.hpp"

#include "atom.hpp"
#include "keyboard_controller.hpp"
#include "screen_graphics_controller.hpp"

class RamConfigurator : public Ram::Configurator {
private:
    const std::string m_name;
public:
    word m_base;
    word m_size;
    RamConfigurator(const std::string&, word, word);
    virtual const std::string &name() const { return m_name; }
    virtual word base() const  { return m_base; }
    virtual word size() const  { return m_size; }
};

class RomConfigurator : public Rom::Configurator {
private:
    const std::string m_name;
public:
    word        m_base;
    word        m_size;
    std::string m_filename;
    RomConfigurator(const std::string&, word, word, std::string);
    virtual const std::string &name()     const { return m_name; }
    virtual word              base()      const { return m_base; }
    virtual word              size()      const { return m_size; }
    virtual const std::string &filename() const { return m_filename; }
};

class PpiaConfigurator : public Ppia::Configurator {
private:
    const std::string m_name;
public:
    word m_base;
    word m_memory_size;
    explicit PpiaConfigurator(const std::string &p_name = "ppia");
    virtual const std::string &name()       const { return m_name; }
    virtual word              base()        const { return m_base; }
    virtual word              memory_size() const { return m_memory_size; }
};

class MemoryConfigurator : public Memory::Configurator {
private:
    const std::string m_name;
public:
    explicit MemoryConfigurator(const std::string &p_name = "memory");
    virtual const std::string &name() const { return m_name; }
};

class MCS6502Configurator : public MCS6502::Configurator {
private:
    const std::string m_name;
public:
    explicit MCS6502Configurator(const std::string &p_name = "mcs6502");
    virtual const std::string &name() const { return m_name; }
};

class AtomConfigurator : public Atom::Configurator {
private:
    const std::string m_name;
    std::vector<const Device::Configurator *> m_devices;
public:
    RamConfigurator     m_block0;
    RamConfigurator     m_lower;
    RamConfigurator     m_video;
    PpiaConfigurator    m_ppia;
    RomConfigurator     m_basic;
    RomConfigurator     m_float;
    RomConfigurator     m_kernel;
    MemoryConfigurator  m_memory;
    MCS6502Configurator m_mcs6502;
    explicit AtomConfigurator(const std::string &p_name = "atom");
    virtual const std::string &name() const { return m_name; }
    virtual const Device::Configurator  *device(int i) const { return i < int(m_devices.size()) ? m_devices[i] : 0; }
    virtual const Memory::Configurator  &memory()      const { return m_memory;     }
    virtual const MCS6502::Configurator &mcs6502()     const { return m_mcs6502;    }

    friend std::ostream &::operator<<(std::ostream&, const AtomConfigurator&);
};

class KeyboardControllerConfigurator : public KeyboardController::Configurator {
private:
    const std::string m_name;
public:
    KeyboardControllerConfigurator();
    virtual const std::string &name() const { return m_name; }

    friend std::ostream &::operator<<(std::ostream&, const KeyboardControllerConfigurator&);
};

class ScreenGraphicsViewConfigurator : public ScreenGraphicsView::Configurator {
private:
    const std::string m_name;
public:
    float       m_scale;
    std::string m_fontfilename;
    std::string m_window_title;
    std::string m_icon_title;
    ScreenGraphicsViewConfigurator();
    virtual const std::string &name()         const { return m_name; }
    virtual float              scale()        const { return m_scale; }
    virtual const std::string &fontfilename() const { return m_fontfilename; }
    virtual const std::string &window_title() const { return m_window_title; }
    virtual const std::string &icon_title()   const { return m_icon_title; }

    friend std::ostream &::operator<<(std::ostream&, const ScreenGraphicsViewConfigurator&);
};

class ScreenGraphicsControllerConfigurator : public ScreenGraphicsController::Configurator {
private:
    const std::string m_name;
public:
    ScreenGraphicsViewConfigurator m_view;
    float m_RefreshRate_Hz;
    ScreenGraphicsControllerConfigurator();
    virtual const std::string                    &name()           const { return m_name; }
    virtual const ScreenGraphicsViewConfigurator &view()           const { return m_view; }
    virtual float                                   RefreshRate_Hz() const { return m_RefreshRate_Hz; }

    friend std::ostream &::operator<<(std::ostream&, const ScreenGraphicsControllerConfigurator&);
};

class Configurator
{
private:
    std::string                          m_XMLfilename;
public:
    AtomConfigurator                     m_atom;
    KeyboardControllerConfigurator       m_keyboard;
    ScreenGraphicsControllerConfigurator m_screen;
private:
    void process_command_line(int argc, char *argv[]);
    void process_XML();
    bool check_and_complete_params();
public:
    Configurator(int argc, char *argv[]);
    virtual ~Configurator();
    const Atom::Configurator                     &atom()     const { return m_atom;     }
    const KeyboardController::Configurator       &keyboard() const { return m_keyboard; }
    const ScreenGraphicsController::Configurator &screen()   const { return m_screen;   }

    friend std::ostream &::operator<<(std::ostream&, const Configurator&);
};

#endif /* CONFIG_XML_HPP_ */

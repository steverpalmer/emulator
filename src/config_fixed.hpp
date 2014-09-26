/*
 * config.hpp
 *
 *  Created on: 20 Apr 2012
 *      Author: steve
 */

#ifndef CONFIG_FIXED_HPP_
#define CONFIG_FIXED_HPP_

#include <ostream>

#include "common.hpp"

#include "atom.hpp"
#include "keyboard_controller.hpp"
#include "screen_graphics_controller.hpp"

class RamConfigurator : public Ram::Configurator {
private:
    const std::string m_name;
    const word        m_base;
    const word        m_size;
public:
    RamConfigurator(const std::string &p_name, word p_base, word p_size)
    : m_name(p_name)
    , m_base(p_base)
    , m_size(p_size)
    {}
    virtual const std::string &name() const { return m_name; }
    virtual word base() const  { return m_base; }
    virtual word size() const  { return m_size; }
};

class RomConfigurator : public Rom::Configurator {
private:
    const std::string m_name;
    const word        m_base;
    const word        m_size;
    const std::string m_filename;
public:
    RomConfigurator(const std::string &p_name, word p_base, word p_size, const std::string &p_filename)
    : m_name(p_name)
    , m_base(p_base)
    , m_size(p_size)
    , m_filename(p_filename)
    {}
    virtual const std::string &name()     const { return m_name; }
    virtual word              base()      const { return m_base; }
    virtual word              size()      const { return m_size; }
    virtual const std::string &filename() const { return m_filename; }
};

class PpiaConfigurator : public Ppia::Configurator {
private:
    const std::string m_name;
    const word        m_base;
    const word        m_memory_size;
public:
    PpiaConfigurator(const std::string &p_name, word p_base, word p_memory_size)
    : m_name(p_name)
    , m_base(p_base)
    , m_memory_size(p_memory_size)
    {}
    virtual const std::string &name()       const { return m_name; }
    virtual word              base()        const { return m_base; }
    virtual word              memory_size() const { return m_memory_size; }
};

class MemoryConfigurator : public Memory::Configurator {
private:
    const std::string m_name;
public:
    MemoryConfigurator(const std::string &p_name) : m_name(p_name) {}
    virtual const std::string &name() const { return m_name; }
};

class MCS6502Configurator : public MCS6502::Configurator {
private:
    const std::string m_name;
public:
    MCS6502Configurator(const std::string &p_name) : m_name(p_name) {}
    virtual const std::string &name() const { return m_name; }
};

class AtomConfigurator : public Atom::Configurator {
private:
    const std::string m_name;
    std::vector<const Device::Configurator *> m_devices;
    const MemoryConfigurator  m_memory;
    const MCS6502Configurator m_mcs6502;
public:
    AtomConfigurator()
    : m_name("atom")
    , m_devices(0)
    , m_memory("memory")
    , m_mcs6502("6502")
    {
        m_devices.push_back(new RamConfigurator( "block0", 0x0000, 0x0400));
        m_devices.push_back(new RamConfigurator( "lower",  0x2800, 0x1400));
        m_devices.push_back(new RamConfigurator( "video",  0x8000, 0x1800));
        m_devices.push_back(new PpiaConfigurator("ppia",   0xB000, 0x0400));
        m_devices.push_back(new RomConfigurator( "basic",  0xC000, 0x1000, "basic.rom"));
        m_devices.push_back(new RomConfigurator( "float",  0xD000, 0x1000, "float.rom"));
        m_devices.push_back(new RomConfigurator( "kernel", 0xF000, 0x1000, "kernel.rom"));
    }
    virtual const std::string &name() const { return m_name; }
    virtual const std::vector<const Device::Configurator *> &devices() const { return m_devices; }
    virtual const Memory::Configurator  &memory()     const { return m_memory;     }
    virtual const MCS6502::Configurator &mcs6502()    const { return m_mcs6502;    }
};

class KeyboardControllerConfigurator : public KeyboardController::Configurator {
private:
    const std::string m_name;
public:
    KeyboardControllerConfigurator() : m_name("KeyboardController") {}
    virtual const std::string &name() const { return m_name; }
};

class ScreenGraphicsViewConfigurator : public ScreenGraphicsView::Configurator {
private:
    const std::string m_name;
    const float       m_scale;
    const std::string m_fontfilename;
    const std::string m_window_title;
    const std::string m_icon_title;
public:
    ScreenGraphicsViewConfigurator()
    : m_name("ScreenGraphicsView")
    , m_scale(2.0)
    , m_fontfilename("mc6847.bmp")
    , m_window_title("Acorn Atom")
    , m_icon_title("Acorn Atom")
    {}
    virtual const std::string &name()         const { return m_name; }
    virtual float              scale()        const { return m_scale; }
    virtual const std::string &fontfilename() const { return m_fontfilename; }
    virtual const std::string &window_title() const { return m_window_title; }
    virtual const std::string &icon_title()   const { return m_icon_title; }
};

class ScreenGraphicsControllerConfigurator : public ScreenGraphicsController::Configurator {
private:
    const std::string m_name;
    const ScreenGraphicsViewConfigurator m_view;
    const int m_RefreshRate_ms;
public:
    ScreenGraphicsControllerConfigurator()
    : m_name("ScreenGraphicsController")
    , m_RefreshRate_ms(100)
    {}
    virtual const std::string                    &name()           const { return m_name; }
    virtual const ScreenGraphicsViewConfigurator &view()           const { return m_view; }
    virtual int                                   RefreshRate_ms() const { return m_RefreshRate_ms; }
};

class Configurator
{
private:
    AtomConfigurator                     m_atom;
    KeyboardControllerConfigurator       m_keyboard;
    ScreenGraphicsControllerConfigurator m_screen;
public:
    Configurator(int argc, char *argv[]);
    const Atom::Configurator                     &atom()     { return m_atom;     }
    const KeyboardController::Configurator       &keyboard() { return m_keyboard; }
    const ScreenGraphicsController::Configurator &screen()   { return m_screen;   }

    friend std::ostream &::operator<<(std::ostream&, const Atom&);
};

#endif /* CONFIG_FIXED_HPP_ */

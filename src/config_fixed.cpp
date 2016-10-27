// config_xml.cpp

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <cassert>
#include <libxml++/libxml++.h>

#include <string>

#include "config_fixed.hpp"
#include "device.hpp"
#include "memory.hpp"
#include "ppia.hpp"
#include "cpu.hpp"
#include "terminal.hpp"

namespace Fixed
{

    class PartConfigurator
        : public virtual Part::Configurator
    {
    protected:
        Part::id_type m_id;
        explicit PartConfigurator(Part::id_type p_id) : m_id(p_id) {}
    public:
        virtual ~PartConfigurator() {}
        inline virtual const Part::id_type &id() const { return m_id; }
    };

    class RamConfigurator
        : public PartConfigurator
        , public Ram::Configurator
    {
    private:
        word          m_size;
        Glib::ustring m_filename;
    public:
        explicit RamConfigurator(Part::id_type p_id, word p_size, Glib::ustring="")
            : PartConfigurator(p_id)
            , m_size(p_size)
            , m_filename("")
            {}
        virtual ~RamConfigurator() {}
        inline virtual word                size()      const { return m_size; }
        inline virtual const Glib::ustring &filename() const { return m_filename; }
    };

    class RomConfigurator
        : public PartConfigurator
        , public Rom::Configurator
    {
    private:
        Glib::ustring m_filename;
        word          m_size;
    public:
        explicit RomConfigurator(Part::id_type p_id, Glib::ustring p_filename, word p_size=0)
            : PartConfigurator(p_id)
            , m_filename(p_filename)
            , m_size(p_size)
            {}
        virtual ~RomConfigurator() {}
        inline virtual const Glib::ustring &filename() const { return m_filename; }
        inline virtual word                size()      const { return m_size; }
    };

    class PpiaConfigurator
        : public PartConfigurator
        , public Ppia::Configurator
    {
    public:
        explicit PpiaConfigurator(Part::id_type p_id)
            : PartConfigurator(p_id)
            {}
        virtual ~PpiaConfigurator() {}
    };

    class AddressSpaceConfigurator
        : public PartConfigurator
        , public AddressSpace::Configurator
    {
    private:
        word m_size;
        const AddressSpace::Configurator::Mapping m_last_memory;
        std::vector<AddressSpace::Configurator::Mapping> m_memories;
    public:
        explicit AddressSpaceConfigurator(Part::id_type p_id, word p_size=0)
            : PartConfigurator(p_id)
            , m_size(p_size)
            , m_last_memory( { 0, 0, 0 } )
            {}
        void add(word p_base, const Memory::Configurator *p_memory, word p_size=0)
            { m_memories.push_back(new AddressSpace::Configurator::Mapping(p_base, p_memory, p_size)); }
        virtual ~AddressSpaceConfigurator()
            {
                for (auto &m : m_memories)
                    delete &m;
                m_memories.clear();
            }
        inline virtual word size() const { return m_size; }
        inline virtual const AddressSpace::Configurator::Mapping &mapping(int i) const
            { return (i < int(m_memories.size())) ? m_memories[i] : m_last_memory; }
    };

    class MCS6502Configurator
        : public PartConfigurator
        , public MCS6502::Configurator
    {
    private:
        Memory::id_type m_memory_id;
    public:
        explicit MCS6502Configurator(Part::id_type p_id, Part::id_type p_memory_id)
            : PartConfigurator(p_id)
            , m_memory_id(p_memory_id)
            {}
        virtual ~MCS6502Configurator() {}
        virtual const Memory::id_type memory_id() const
            { return m_memory_id; }
    };

    class ComputerConfigurator
        : public PartConfigurator
        , public Computer::Configurator
    {
    private:
        std::vector<const Device::Configurator *>m_devices;
    public:
        explicit ComputerConfigurator(Part::id_type p_id)
            : PartConfigurator(p_id)
            {}
        void add(const Device::Configurator *p_device)
            { m_devices.push_back(p_device); }
        virtual ~ComputerConfigurator() {}
        virtual const Device::Configurator *device(int i) const
            { return (i < int(m_devices.size())) ? m_devices[i] : 0; }

    };

    class KeyboardControllerConfigurator
        : public KeyboardController::Configurator
    {
    public:
        explicit KeyboardControllerConfigurator() {}
        virtual ~KeyboardControllerConfigurator() {}
    };

    class MonitorViewConfigurator
        : public MonitorView::Configurator
    {
    private:
        float         m_scale;
        Glib::ustring m_fontfilename;
        Glib::ustring m_window_title;
        Glib::ustring m_icon_title;
    public:
        explicit MonitorViewConfigurator(float p_scale=2.0
                                         , Glib::ustring p_fontfilename="mc6847.bmp"
                                         , Glib::ustring p_window_title="Acorn Atom"
                                         , Glib::ustring p_icon_title="Acorn Atom")
            : m_scale(p_scale)
            , m_fontfilename(p_fontfilename)
            , m_window_title(p_window_title)
            , m_icon_title(p_icon_title)
            {}
        virtual ~MonitorViewConfigurator() {}
        float               scale()         const { return m_scale; }
        const Glib::ustring &fontfilename() const { return m_fontfilename; }
        const Glib::ustring &window_title() const { return m_window_title; }
        const Glib::ustring &icon_title()   const { return m_icon_title; }
    };

    class TerminalConfigurator
        : public PartConfigurator
        , public Terminal::Configurator
    {
    private:
        Part::id_type m_memory_id;
        Part::id_type m_controller_id;
        KeyboardControllerConfigurator *m_keyboard_controller;
        MonitorViewConfigurator *m_monitor_view;
    public:
        explicit TerminalConfigurator(Part::id_type p_id
                                      , Part::id_type p_memory_id="video"
                                      , Part::id_type p_controller_id="ppia")
            : PartConfigurator(p_id)
            , m_memory_id(p_memory_id)
            , m_controller_id(p_controller_id)
            {
                m_keyboard_controller = new KeyboardControllerConfigurator;
                m_monitor_view = new MonitorViewConfigurator;
            }
        virtual ~TerminalConfigurator() {}
        const Part::id_type                      &memory_id()           const { return m_memory_id; }
        const Part::id_type                      &controller_id()       const { return m_controller_id; }
        const KeyboardController::Configurator   &keyboard_controller() const { return *m_keyboard_controller; }
        const MonitorView::Configurator          &monitor_view()        const { return *m_monitor_view; }
    };

    Configurator::Configurator(int argc, char *argv[])
        : ::Configurator(argc, argv)
    {
        auto address_space = new AddressSpaceConfigurator("address_space");
        address_space->add(0x0000, new RamConfigurator("block0", 0x0400));
        address_space->add(0x8000, new RamConfigurator("video",  0x0400));
        address_space->add(0xB000, new PpiaConfigurator("ppia"), 0x0400);
        address_space->add(0xC000, new RomConfigurator("basic",  "basic.rom",  0x4000));
        address_space->add(0xF000, new RomConfigurator("kernel", "kernel.rom", 0x4000));
        auto atom = new ComputerConfigurator("atom");
        atom->add(address_space);
        atom->add(new MCS6502Configurator("mcs6502", "address_space"));
        m_parts.push_back(atom);
        m_parts.push_back(new TerminalConfigurator("terminal"));
    }

}

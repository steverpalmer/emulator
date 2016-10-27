// device.hpp

#ifndef DEVICE_HPP_
#define DEVICE_HPP_

#include <set>  // Used by Computer

#include "common.hpp"
#include "part.hpp"

/// Model of all resetable devices.
///
/// This is the interface that must be supported by all resetable devices.

class Device
    : public Part
{
    // Types
public:
	/// The Device Configurator is an interface to two key items
	/// 1. the information needed by the device to construct an instance
	/// 2. a factory method that builds the instance of the device
    class Configurator
        : public virtual Part::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual Device *device_factory() const
            { return 0; }
        virtual Part *part_factory() const
            { return device_factory(); }

        virtual void serialize(std::ostream &) const;
    };
    // Methods
protected:
    explicit Device(const Configurator &);
public:
    virtual ~Device();
    virtual void reset() {};
    virtual void pause() {};
    virtual void resume() {};

    virtual void serialize(std::ostream &) const;
};


/// A Computer is a a collection of Devices
///
/// It is a composite pattern with the actual computer being made up
/// of other Devices.
class Computer
    : public Device
{
public:
    class Configurator
        : public virtual Device::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information
        virtual const Device::Configurator *device(int i) const = 0;
        /// 2. Factory Method
        virtual Device *device_factory() const
            { return new Computer(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    std::set<Device *> m_children;
    // Methods
public:
    explicit Computer(const Configurator &);
    virtual ~Computer();
public:
    void add_child(Device *);
    void remove_child(Device *, bool do_erase);
    virtual void remove_child(Device *p_device)
        { remove_child(p_device, true); }
    void clear();

    virtual void reset();
    virtual void pause();
    virtual void resume();

    virtual void serialize(std::ostream &) const;
};


#endif

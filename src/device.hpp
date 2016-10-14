// device.hpp

#ifndef DEVICE_HPP_
#define DEVICE_HPP_

#include <ostream>
#include <set>  // Used by Computer

#include "common.hpp"
#include "part.hpp"

/// Model of all resetable devices.
///
/// This is the interface that must be supported by all resetable devices.

class Device
    : public virtual Part
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
        Configurator();
    public:
        ~Configurator();
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual Device *device_factory() const {return 0; }
        virtual Part *part_factory() const { return device_factory(); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
protected:
    std::set<Device *> m_parents;
    // Methods
protected:
    explicit Device(const Configurator &);
public:
    void add_parent(Device *p_parent)
        { (void) m_parents.insert(p_parent); }
    void remove_parent(Device *p_parent)
        { (void) m_parents.erase(p_parent); }
    virtual ~Device();
    virtual void reset() {};
    virtual void pause() {};
    virtual void resume() {};

    friend std::ostream &::operator<<(std::ostream &, const Device &);
};


/// A Computer is a a collection of Devices
///
/// It is a composite pattern with the actual computer being made up
/// of other Devices.
class Computer
    : public virtual Device
{
public:
    class Configurator
        : public Device::Configurator
    {
    protected:
        Configurator();
    public:
        ~Configurator();
    	/// 1. Constructor Information
        virtual const Device::Configurator *device(int i) const = 0;
        /// 2. Factory Method
        virtual Device *device_factory() const
            { return new Computer(*this); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::set<Device *> m_children;
    // Methods
public:
    explicit Computer(const Configurator &);
    virtual ~Computer();
public:
    void add_child(Device *p_device) { (void) m_children.insert(p_device); }
    void remove_child(Device *p_device)  { (void) m_children.erase(p_device); }
    void clear();

    virtual void reset();
    virtual void pause();
    virtual void resume();

    friend std::ostream &::operator<<(std::ostream &, const Computer &);
};


#endif

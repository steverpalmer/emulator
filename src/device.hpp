// device.hpp
// Copyright 2016 Steve Palmer

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
        virtual Device *device_factory() const = 0;
        virtual Part *part_factory() const
            { return device_factory(); }

        virtual void serialize(std::ostream &) const;
    };

    class ReferenceConfigurator
        : public virtual Configurator
        , protected Part::ReferenceConfigurator
    {
    public:
        explicit ReferenceConfigurator(const id_type p_ref_id)
            : Part::ReferenceConfigurator(p_ref_id) {}
        virtual ~ReferenceConfigurator() = default;
        virtual Part *part_factory() const;
        virtual Device *device_factory() const;
        virtual void serialize(std::ostream &) const;
    };

    // Methods
protected:
    explicit Device(const Configurator &);
    explicit Device(const id_type &);
    Device();
    virtual ~Device();
public:
    virtual void reset() {};
    virtual void pause() {};
    virtual void resume() {};
    virtual bool is_paused() const { return true; }

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
        virtual const Device::Configurator *device(int) const = 0;
        /// 2. Factory Method
        virtual Device *device_factory() const
            { return new Computer(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    std::set<Device *> m_children;
    // Methods
private:
    Computer();
public:
    explicit Computer(const Configurator &);
private:
    virtual ~Computer();
public:
    void add_child(Device &);
    void clear();

    virtual void reset();
    virtual void pause();
    virtual void resume();
    virtual bool is_paused() const;

    virtual void serialize(std::ostream &) const;
};


#endif

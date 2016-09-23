/*
 * memory.hpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <ostream>
#include <vector> // Used by Ram and Rom
#include <array>  // User my Memory
#include <memory>

#include "common.hpp"

enum AccessType {AT_UNKNOWN, AT_INSTRUCTION, AT_OPERAND, AT_DATA, AT_LAST};
extern std::ostream &operator<<(std::ostream &, const AccessType);


/// This is the interface that must be supported by all Memory classes.
/// It is an Abstract Class with pure virtual get and put byte methods.
/// However, only function signatures are defined, no semantics.
///
/// Addresses supplied to get_ and put_byte are relative to the device.
class Device : public Named {
    // Types
public:
	/// The Device Configurator is an interface to three key items
	/// 1. the information needed by the device to construct an instance
	/// 2. a factory method that builds the instance of the device
	/// 3. the information needed to locate the device in memory
    class Configurator : public Named::Configurator
    {
    public:
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory()     const = 0;
        /// 3. Device Location
        virtual word                    base()        const = 0;
        virtual word                    memory_size() const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Methods
private:
    void operator=(const Device &);
    Device(const Device&);
protected:
    explicit Device(const Configurator &);
public:
    virtual word size() const = 0;
    virtual void reset() {}
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN) = 0;
    virtual void set_byte(word p_addr, byte p_byte, AccessType = AT_UNKNOWN) = 0;

    friend std::ostream &::operator<<(std::ostream &, const Device &);
};


/// Ram is a trivial implementation of Device as a simple RAM device.

/// In that sense, the class is little more than a "safe" array.
class Ram : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
    	virtual word                    size() const = 0;
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
        { return std::unique_ptr<Device>(new Ram(*this)); }
        /// 3. Device Location
        virtual word                    memory_size() const { return size(); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
public:
    std::vector<byte> m_storage;
    // Methods
private:
    void operator=(const Ram &);
    Ram(const Ram &);
public:
    explicit Ram(const Configurator &);
    virtual ~Ram() {}
    virtual word size() const { return m_storage.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    void         load(const std::string &p_filename);
    void         save(const std::string &p_filename) const;

    friend std::ostream &::operator<<(std::ostream &, const Ram &);

};

/// Rom is a type of Ram with a conditional put_byte.  In particular,
/// It is possible to write to a Rom if it is explicitly declared to
/// be writable.  Otherwise, put_bytes are silently ignored.
///
/// It can be constructed in a similar way to Ram, but with an additional
/// constructor which initialises the contents from a stream.
/// On construction, a ROM is not writeable.
class Rom : public Ram {
public:
    class Configurator : public Ram::Configurator
    {
    public:
    	/// 1. Constructor Information
        virtual const std::string       &filename() const = 0;
        /// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
        { return std::unique_ptr<Device>(new Rom(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    bool m_is_writeable;
    // Methods
private:
    void operator=(const Rom &);
    Rom(const Rom &);
public:
    explicit Rom(const Configurator &);
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    void         set_writeable(bool p_writeable);

    friend std::ostream &::operator<<(std::ostream &, const Rom &);

};

class Hook: public Device
{
public:
    class Configurator : public Device::Configurator
    {
    public:
        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
public:
    std::shared_ptr<Device> m_device;
    // Methods
protected:
    explicit Hook(const Configurator &);
    virtual ~Hook();
public:
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void set_byte(word p_addr, byte p_byte, AccessType = AT_UNKNOWN);
    virtual int  execute (word p_addr, int p_byte, AccessType p_at = AT_UNKNOWN) = 0;

    friend std::ostream &::operator<<(std::ostream &, const Hook &);
};

/// This is the Main Memory class whose primary client in the CPU.
/// In this class, Addresses are "absolute".
/// It is a composite pattern with the actual memory being made up
/// of other Devices located at specific places.
/// Optionally, the device can be replicated in the memory map by defining
/// it to occupy a larger part of the memory space.
/// It also provides a few helpers for getting and putting words as
/// combinations of getting and putting words (Low Endian).
class Memory : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
    	/// 2. Constructor Information
        virtual std::unique_ptr<Device> factory() const
		{ return std::unique_ptr<Device>(new Memory(*this)); }
        /// 3. Location Information
        virtual word                    base()        const { return 0; }
        virtual word                    memory_size() const { return 0; }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::vector<word>                    m_base;
    std::vector<std::shared_ptr<Device>> m_map;
    // Methods
private:
    void operator=(const Memory &);
    Memory(const Memory &);
public:
    explicit Memory(const Configurator &);
    virtual word            size() const { return 0; }
    virtual byte            get_byte  (word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void            set_byte  (word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    word                    get_word  (word p_addr, AccessType p_at = AT_UNKNOWN);
    void                    set_word  (word p_addr, word p_word, AccessType p_at = AT_UNKNOWN);
    void                    add_device(word p_base, std::shared_ptr<Device> p_device, word p_memory_size = 0);
    std::shared_ptr<Device> get_device(word p_addr) const;
    void                    drop_devices();
    void                    add_hook  (word p_addr, std::shared_ptr<Hook> p_hook, word p_size = 0);

    friend std::ostream &::operator<<(std::ostream &, const Memory &);
};


#endif /* MEMORY_HPP_ */

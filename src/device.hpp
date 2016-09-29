/*
 * device.hpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#ifndef DEVICE_HPP_
#define DEVICE_HPP_

#include <ostream>
#include <map>    // User by Devices
#include <vector> // Used by Ram and Rom
#include <array>  // User my Memory
#include <memory>

#include "common.hpp"

enum AccessType {AT_UNKNOWN, AT_INSTRUCTION, AT_OPERAND, AT_DATA, AT_LAST};
extern std::ostream &operator<<(std::ostream &, const AccessType);

/// Model of all memory mapped devices.
///
/// This is the interface that must be supported by all memory mapped devices.
/// It is an Abstract Class with pure virtual get and set byte methods.
/// However, only function signatures are defined, no semantics.
/// For example, the device might be and I/O chip,
/// so it is not true and a set followed by a get returns the value just set.
/// Similarly, the get methods may have side-effects, so are not const.
///
/// Addresses supplied to get_ and put_byte are relative to the device.
///
/// It also provides a few helpers for getting and putting words as
/// combinations of getting and putting words (Low Endian).
class Device : public Named {
    // Types
public:
	/// The Device Configurator is an interface to two key items
	/// 1. the information needed by the device to construct an instance
	/// 2. a factory method that builds the instance of the device
    class Configurator : public Named::Configurator
    {
    public:
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Methods
private:
    Device(const Device&);
    Device &operator=(const Device &);
protected:
    explicit Device(const Configurator &);
public:
    virtual word size() const = 0;
    virtual void reset() {}
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN) = 0;
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN) = 0;
    word         get_word(word p_addr, AccessType p_at = AT_UNKNOWN);
    void         set_word(word p_addr, word p_word, AccessType p_at = AT_UNKNOWN);

    friend std::ostream &::operator<<(std::ostream &, const Device &);
};

/// SimpleMemory is the basic model of memory
///
/// The difference bewteen Ram and Rom are:
///   set_byte does nothing on a Rom
///   On construction Ram may initialize the memory with the contents of a file if it exists
///   On construction Rom will initialize the memory with the contents of a file
///   On destruction Ram may save the memory to a file
/// These difference are handled in the Ram and Rom wrappers around Ram_Rom_Helper.
class SimpleMemory {
    std::vector<byte> m_storage;
public:
    SimpleMemory(int size) : m_storage(size, 0) {}
    inline word size()
        { return m_storage.size(); }
    inline byte get_byte(word p_addr, AccessType p_at)
        { return m_storage[p_addr]; }
    inline byte set_byte(word p_addr, byte p_byte, AccessType p_at)
        { m_storage[p_addr] = p_byte; }
    bool load(const std::string &p_filename);
    bool save(const std::string &p_filename) const;
};

/// Ram is a trivial implementation of Device as a simple RAM device.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// the storage may be persistent is it is configured with a filename.
class Ram : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
    	virtual word size() const = 0;
        virtual const std::string &filename() const = 0;
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Ram(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    SimpleMemory m_memory;
    std::string m_filename;
    // Methods
private:
    Ram(const Ram &);
    Ram &operator=(const Ram &);
public:
    explicit Ram(const Configurator &);
    virtual ~Ram();
    virtual word size() const
        { return m_memory.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_memory.get_byte(p_addr, p_at); }
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { m_memory.set_byte(p_addr, p_byte, p_at); }

    friend std::ostream &::operator<<(std::ostream &, const Ram &);

};

/// Rom is a trivial implementation of Device as a simple ROM device.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// set_byte does nothing and
/// the storage is initialised on construction.
class Rom : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
        virtual const std::string &filename() const = 0;
    	virtual word size() const = 0;
        /// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Rom(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    SimpleMemory m_memory;
    // Methods
private:
    Rom(const Rom &);
    Rom &operator=(const Rom &);
public:
    explicit Rom(const Configurator &);
    virtual ~Rom();
    virtual word size() const
        { return m_memory.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_memory.get_byte(p_addr, p_at); }
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { /* Do Nothing! */ }

    friend std::ostream &::operator<<(std::ostream &, const Rom &);

};

#if 0
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
    word m_base;
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
#endif

/// This is the Main Memory class whose primary client in the CPU.
///
/// It is a composite pattern with the actual memory being made up
/// of other Devices located at specific places.
/// Optionally, the device can be replicated in the memory map by defining
/// it to occupy a larger part of the memory space.
class Memory : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
        struct mapping {
            word base;
            Device::Configurator device;
            word size;
        };
        virtual const std::unique_ptr<struct mapping> device(int i) const = 0;
        /// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Memory(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::vector<std::shared_ptr<Device>> m_devices;
    std::vector<word>                    m_base;
    std::vector<std::shared_ptr<Device>> m_map;
    // Methods
private:
    Memory(const Memory &);
    Memory &operator=(const Memory &);
public:
    explicit Memory(const Configurator &);
    virtual void            reset();
    virtual word            size() const { return m_map.size(); }
    virtual byte            get_byte  (word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void            set_byte  (word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    void                    add_device(word p_base, std::shared_ptr<Device> p_device, word p_size = 0);
    std::shared_ptr<Device> get_device(word p_addr) const;
    void                    drop_devices();
#if 0
    void                    add_hook  (word p_addr, std::shared_ptr<Hook> p_hook, word p_size = 0);
#endif

    friend std::ostream &::operator<<(std::ostream &, const Memory &);
};


#endif /* DEVICE_HPP_ */

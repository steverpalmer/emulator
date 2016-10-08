// device.hpp

#ifndef DEVICE_HPP_
#define DEVICE_HPP_

#include <ostream>
#include <set>    // Used by Device observers
#include <vector> // Used by Ram and Rom storage
#include <memory> // Used be Memory

#include "common.hpp"
#include "part.hpp"

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
/// In addition, Devices are subjects (observerable), by other objects that
/// inherit from Device::Observer.  They will then be updated when the device is
/// reset, read or written.
///
/// It also provides a few helpers for getting and putting words as
/// combinations of getting and putting words (Low Endian).

class Device
    : public virtual Part
{
    // Types
public:
    /// The Device Observer is an interface to allow
    /// other classes to observe resets, get_bytes and set_bytes
    class Observer
    {
    private:
        Observer(const Observer &);
        Observer &operator=(const Observer &);
    protected:
        Observer();
    public:
        virtual void reset_update(Device *p_device) {};
        virtual void get_byte_update(Device *p_device, word p_addr, AccessType p_at, byte result) {};
        virtual void set_byte_update(Device *p_device, word p_addr, byte p_byte, AccessType p_at) {};
    };

	/// The Device Configurator is an interface to two key items
	/// 1. the information needed by the device to construct an instance
	/// 2. a factory method that builds the instance of the device
    class Configurator
        : public Part::Configurator
    {
    public:
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const = 0;

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
protected:
    std::set<Observer *> m_observers;
    // Methods
private:
    Device(const Device &);
    Device &operator=(const Device &);
protected:
    explicit Device(const Configurator &);
protected:
    virtual void _reset() {};
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
public:
    virtual word size() const = 0;

    inline void attach(Observer &p_observer) { m_observers.insert(&p_observer); }
    inline void detach(Observer &p_observer) { m_observers.erase(&p_observer); }

    virtual ~Device() { m_observers.clear(); }
    
    inline void reset()
        {
            _reset();
            for ( Observer *obs : m_observers )
                obs->reset_update(this);
        }
    inline byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        {
            const byte result(_get_byte(p_addr, p_at));
            for ( Observer *obs : m_observers )
                obs->get_byte_update(this, p_addr, p_at, result);
            return result;
        }
    inline void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        {
            _set_byte(p_addr, p_byte, p_at);
            for ( Observer *obs : m_observers )
                obs->set_byte_update(this, p_addr, p_byte, p_at);
        }
    word get_word(word p_addr, AccessType p_at = AT_UNKNOWN);
    void set_word(word p_addr, word p_word, AccessType p_at = AT_UNKNOWN);

    friend class Observer;
    
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
class SimpleMemory
{
    std::vector<byte> m_storage;
public:
    SimpleMemory(int size) : m_storage(size, 0) {}
    inline word size() const
        { return m_storage.size(); }
    inline byte _get_byte(word p_addr, AccessType p_at)
        { return m_storage[p_addr]; }
    inline void _set_byte(word p_addr, byte p_byte, AccessType p_at)
        { m_storage[p_addr] = p_byte; }
    bool load(const Glib::ustring &p_filename);
    bool save(const Glib::ustring &p_filename) const;

    friend std::ostream &::operator<<(std::ostream &, const SimpleMemory &);
};


/// Ram is a trivial implementation of Device as a simple RAM device.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// the storage may be persistent if it is configured with a filename.
class Ram
    : public Device
{
public:
    class Configurator
        : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
    	virtual word size() const = 0;
        virtual const Glib::ustring &filename() const = 0;
    	/// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Ram(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    SimpleMemory m_memory;
    Glib::ustring m_filename;
    // Methods
private:
    Ram(const Ram &);
    Ram &operator=(const Ram &);
public:
    explicit Ram(const Configurator &);
    virtual ~Ram();
    virtual word size() const
        { return m_memory.size(); }
protected:
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_memory._get_byte(p_addr, p_at); }
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { m_memory._set_byte(p_addr, p_byte, p_at); }

    friend std::ostream &::operator<<(std::ostream &, const Ram &);
};


/// Rom is a trivial implementation of Device as a simple ROM device.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// set_byte does nothing and
/// the storage is initialised on construction.
class Rom
    : public Device
{
public:
    class Configurator
        : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
        virtual const Glib::ustring &filename() const = 0;
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
protected:
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_memory._get_byte(p_addr, p_at); }
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { /* Do Nothing! */ }

    friend std::ostream &::operator<<(std::ostream &, const Rom &);
};


/// This is the Main Memory class whose primary client in the CPU.
///
/// It is a composite pattern with the actual memory being made up
/// of other Devices located at specific places.
/// Optionally, the device can be replicated in the memory map by defining
/// it to occupy a larger part of the memory space.
class Memory
    : public Device
{
public:
    class Configurator
        : public Device::Configurator
    {
    public:
    	/// 1. Constructor Information
        virtual word size() const { return 0; }
        struct mapping {
            word                 base;
            Device::Configurator *device;
            word                 size;
        };
        virtual const mapping &device(int i) const = 0;
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
    virtual word size() const { return m_map.size(); }
protected:
    virtual void _reset();
    virtual byte _get_byte  (word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void _set_byte  (word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
public:
    void add_device(word p_base, std::shared_ptr<Device> p_device, word p_size = 0);
    void drop_devices();

    friend std::ostream &::operator<<(std::ostream &, const Memory &);
};


#endif /* DEVICE_HPP_ */

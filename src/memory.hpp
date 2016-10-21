// memory.hpp

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <ostream>
#include <set>     // Used by Device observers and parents
#include <vector>  // Used by Ram and Rom storage
#include <memory>  // Used be Memory

#include "common.hpp"
#include "device.hpp"

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
/// In addition, Memorys are subjects (observerable), by other objects that
/// inherit from Memory::Observer.  They will then be updated when the memory is
/// read or written.
///
/// It also provides a few helpers for getting and putting words as
/// combinations of getting and putting words (Low Endian).

class Memory
    : public virtual Device
{
    // Types
public:
    enum AccessType {AT_UNKNOWN, AT_INSTRUCTION, AT_OPERAND, AT_DATA, AT_LAST};
    friend std::ostream &::operator<<(std::ostream &, const AccessType);

    /// The Device Observer is an interface to allow
    /// other classes to observe resets, get_bytes and set_bytes
    class Observer
        : NonCopyable
    {
    protected:
        Observer() {}
    public:
        virtual void get_byte_update(Device *p_device, word p_addr, AccessType p_at, byte result) {};
        virtual void set_byte_update(Device *p_device, word p_addr, byte p_byte, AccessType p_at) {};
    };

	/// The Device Configurator is an interface to two key items
	/// 1. the information needed by the device to construct an instance
	/// 2. a factory method that builds the instance of the device
    class Configurator
        : public Device::Configurator
    {
    protected:
        Configurator() {}
    public:
        virtual ~Configurator() {}
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual Memory *memory_factory() const { return 0; }
        virtual Device *device_factory() const
            { return memory_factory(); }

        virtual void serialize(std::ostream &p_s) const
            { Device::Configurator::serialize(p_s); }
    };
    // Attributes
protected:
    std::set<Observer *> m_observers;
    // Methods
protected:
    explicit Memory(const Configurator &);
protected:
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN) = 0;
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN) = 0;
public:
    virtual word size() const = 0;

    inline void attach(Observer &p_observer) { m_observers.insert(&p_observer); }
    inline void detach(Observer &p_observer) { m_observers.erase(&p_observer); }

    virtual ~Memory();
    
    inline byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        {
            const byte result(_get_byte(p_addr, p_at));
            for ( auto *obs : m_observers )
                obs->get_byte_update(this, p_addr, p_at, result);
            return result;
        }
    inline void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        {
            _set_byte(p_addr, p_byte, p_at);
            for ( auto *obs : m_observers )
                obs->set_byte_update(this, p_addr, p_byte, p_at);
        }
    word get_word(word p_addr, AccessType p_at = AT_UNKNOWN);
    void set_word(word p_addr, word p_word, AccessType p_at = AT_UNKNOWN);

    friend class Observer;

    virtual void serialize(std::ostream &p_s) const
        { Device::serialize(p_s); }
};


/// Storage is the basic model of memory
///
/// The difference bewteen Ram and Rom are:
///   set_byte does nothing on a Rom
///   On construction Ram may initialize the memory with the contents of a file if it exists
///   On construction Rom will initialize the memory with the contents of a file
///   On destruction Ram may save the memory to a file
/// These difference are handled in the Ram and Rom wrappers around Storage.
class Storage
{
    std::vector<byte> m_storage;
public:
    Storage(int size) : m_storage(size, 0) {}
    virtual ~Storage() {}
    inline word size() const
        { return m_storage.size(); }
    inline byte get_byte(word p_addr, Memory::AccessType p_at)
        { return m_storage[p_addr]; }
    inline void set_byte(word p_addr, byte p_byte, Memory::AccessType p_at)
        { m_storage[p_addr] = p_byte; }
    bool load(const Glib::ustring &p_filename);
    bool save(const Glib::ustring &p_filename) const;

    friend std::ostream &::operator<<(std::ostream &, const Storage &);
};


/// Ram is a trivial implementation of Memory as a simple RAM.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// the storage may be persistent if it is configured with a filename.
class Ram
    : public Memory
{
public:
    class Configurator
        : public Memory::Configurator
    {
    protected:
        Configurator() {}
    public:
        virtual ~Configurator() {}
    	/// 1. Constructor Information
    	virtual word size() const = 0;
        virtual const Glib::ustring &filename() const = 0;
    	/// 2. Factory Method
        virtual Memory *memory_factory() const
            { return new Ram(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    Storage m_storage;
    Glib::ustring m_filename;
    // Methods
public:
    explicit Ram(const Configurator &);
    virtual ~Ram();
    virtual word size() const
        { return m_storage.size(); }
protected:
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_storage.get_byte(p_addr, p_at); }
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { m_storage.set_byte(p_addr, p_byte, p_at); }

    virtual void serialize(std::ostream &) const;
};


/// Rom is a trivial implementation of Memory as a simple ROM.
///
/// It provides a wrapper around SimpleMemory, with addition that
/// set_byte does nothing and
/// the storage is initialised on construction.
class Rom
    : public Memory
{
public:
    class Configurator
        : public Memory::Configurator
    {
    protected:
        Configurator() {}
    public:
        virtual ~Configurator() {}
    	/// 1. Constructor Information
        virtual const Glib::ustring &filename() const = 0;
    	virtual word size() const = 0;
        /// 2. Factory Method
        virtual Memory *memory_factory() const
            { return new Rom(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    Storage m_storage;
    // Methods
public:
    explicit Rom(const Configurator &);
    virtual ~Rom();
    virtual word size() const
        { return m_storage.size(); }
protected:
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN)
        { return m_storage.get_byte(p_addr, p_at); }
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN)
        { /* Do Nothing! */ }

    virtual void serialize(std::ostream &) const;
};


/// This is the AddressSpace class whose primary client in the CPU.
///
/// It is a composite pattern with the actual memory being made up
/// of other Memorys located at specific places.
/// Optionally, the Memory can be replicated in the memory map by defining
/// it to occupy a larger part of the address space.
class AddressSpace
    : public Memory
{
public:
    class Configurator
        : public Memory::Configurator
    {
    protected:
        Configurator() {}
    public:
        virtual ~Configurator() {}
    	/// 1. Constructor Information
        virtual word size() const { return 0; }
        struct Mapping {
            word                       base;
            const Memory::Configurator *memory;
            word                       size;
        };
        virtual const Mapping &mapping(int i) const = 0;
        /// 2. Factory Method
        virtual Memory *memory_factory() const
            { return new AddressSpace(*this); }

        virtual void serialize(std::ostream &) const;
    };
    // Attributes
private:
    std::set<Memory *>    m_children;
    std::vector<word>     m_base;
    std::vector<Memory *> m_map;
    // Methods
public:
    explicit AddressSpace(const Configurator &);
    virtual word size() const { return m_map.size(); }
protected:
    virtual byte _get_byte  (word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void _set_byte  (word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
public:
    void add_child(word p_base, Memory *p_memory, word p_size = 0);
    virtual void remove_child(Memory *p_memory);
    void clear();
    virtual ~AddressSpace();
    virtual void reset();
    virtual void pause();
    virtual void resume();

    virtual void serialize(std::ostream &) const;
};


#endif

// memory.hpp
// Copyright 2016 Steve Palmer

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

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
    : public Device
{
    // Types
public:
    enum class AccessType {UNKNOWN, INSTRUCTION, OPERAND, DATA};
    friend std::ostream &::operator<<(std::ostream &, const AccessType);

    /// The Device Observer is an interface to allow
    /// other classes to observe resets, get_bytes and set_bytes
    class Observer
        : protected NonCopyable
    {
    protected:
        Observer() = default;
    public:
        virtual ~Observer() = default;
        virtual void set_byte_update(Memory &, word, byte, AccessType) = 0;
    };

	/// The Memory Configurator is an interface to two key items
	/// 1. the information needed by the memory to construct an instance
	/// 2. a factory method that builds the instance of the memory
    class Configurator
        : public virtual Device::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information - Name only at this level
    	/// 2. Factory Method
        virtual Memory *memory_factory() const = 0;
        virtual Device *device_factory() const override
            { return memory_factory(); }

        virtual void serialize(std::ostream &) const override;
    };

    class ReferenceConfigurator
        : public virtual Configurator
        , protected Device::ReferenceConfigurator
    {
    public:
        explicit ReferenceConfigurator(const id_type p_ref_id)
            : Device::ReferenceConfigurator(p_ref_id) {}
        virtual ~ReferenceConfigurator() = default;
        virtual Part *part_factory() const override;
        virtual Device *device_factory() const override;
        virtual Memory *memory_factory() const override;
        virtual void serialize(std::ostream &) const override;
    };

    // Attributes
private:
    std::set<Observer *> m_observers;
    // Methods
protected:
    explicit Memory(const Configurator &);
    explicit Memory(const id_type &);
    Memory();
    virtual ~Memory();
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN) = 0;
public:
    virtual word size() const = 0;

    void attach(Observer &);
    void detach(Observer &);

    virtual byte get_byte(word p_addr, AccessType p_at = AccessType::UNKNOWN) = 0;
    void set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN);
    word get_word(word p_addr, AccessType p_at = AccessType::UNKNOWN);
    void set_word(word p_addr, word p_word, AccessType p_at = AccessType::UNKNOWN);

    virtual void serialize(std::ostream &) const;
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
private:
    Storage();
public:
    Storage(int size) : m_storage(size) {}
    virtual ~Storage() = default;
    inline word size() const
        { return m_storage.size(); }
    inline byte get_byte(word p_addr, Memory::AccessType p_at)
        { return m_storage[p_addr]; }
    inline void set_byte(word p_addr, byte p_byte, Memory::AccessType p_at)
        { m_storage[p_addr] = p_byte; }
    void randomize(unsigned int seed = 0);
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
        : public virtual Memory::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information
    	virtual word size() const = 0;
    	virtual unsigned int seed() const = 0;
        virtual const Glib::ustring &filename() const = 0;
    	/// 2. Factory Method
        virtual Memory *memory_factory() const override
            { return new Ram(*this); }

        virtual void serialize(std::ostream &) const override;
    };
    // Attributes
private:
    Storage m_storage;
    Glib::ustring m_filename;
    // Methods
private:
    Ram();
public:
    explicit Ram(const Configurator &);
private:
    virtual ~Ram();
public:
    virtual word size() const override
        { return m_storage.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AccessType::UNKNOWN) override
        { return m_storage.get_byte(p_addr, p_at); }
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN) override
        { m_storage.set_byte(p_addr, p_byte, p_at); }

public:
    virtual void serialize(std::ostream &) const override;
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
        : public virtual Memory::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information
        virtual const Glib::ustring &filename() const = 0;
    	virtual word size() const = 0;
        /// 2. Factory Method
        virtual Memory *memory_factory() const override
            { return new Rom(*this); }

        virtual void serialize(std::ostream &) const override;
    };
    // Attributes
private:
    Storage m_storage;
    // Methods
private:
    Rom();
public:
    explicit Rom(const Configurator &);
private:
    virtual ~Rom();
public:
    virtual word size() const override
        { return m_storage.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AccessType::UNKNOWN) override
        { return m_storage.get_byte(p_addr, p_at); }
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN) override
        { /* Do Nothing! */ }

public:
    virtual void serialize(std::ostream &) const override;
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
        : public virtual Memory::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information
        virtual word size() const = 0;
        struct Mapping {
            word                       base;
            const Memory::Configurator *memory;
            word                       size;
        };
        virtual const Mapping &mapping(int i) const = 0;
        /// 2. Factory Method
        virtual Memory *memory_factory() const override
            { return new AddressSpace(*this); }

        virtual void serialize(std::ostream &) const override;
    };
    // Attributes
private:
    std::set<Memory *>    m_children;
    std::vector<word>     m_base;
    std::vector<Memory *> m_map;
    // Methods
public:
    explicit AddressSpace(const Configurator &);
    explicit AddressSpace(const id_type &, word p_size = 0);
    explicit AddressSpace(word p_size = 0);
    virtual word size() const  override{ return m_map.size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AccessType::UNKNOWN) override;
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN) override;
public:
    void add_child(word p_base, Memory &p_memory, word p_size = 0);
    void clear();
private:
    virtual ~AddressSpace();
public:
    virtual void reset() override;
    virtual void pause() override;
    virtual void resume() override;
    virtual bool is_paused() const override;

    virtual void serialize(std::ostream &) const override;

    friend class Hook;
};

class Hook
    : public Memory
{
    // Types
public:
    class Configurator
        : public virtual Memory::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
    	/// 1. Constructor Information
        virtual word size() const = 0;
        /// 2. Factory Method
        virtual Memory *memory_factory() const override
            { return new Hook(*this); }
    };
private:
    AddressSpace *m_address_space;
    // Methods
protected:
    explicit Hook(const Configurator &);
    explicit Hook(const id_type &, word p_size = 1);
    explicit Hook(word p_size = 1);
    virtual ~Hook() = default;
private:
    virtual int get_byte_hook(word p_addr, AccessType p_at) { return -1; }
    virtual int set_byte_hook(word p_addr, byte p_byte, AccessType p_at) { return -1; }
public:
    void fill(AddressSpace &p_as, word p_base);
    virtual word size() const override { return m_address_space->size(); }
    virtual byte get_byte(word p_addr, AccessType p_at = AccessType::UNKNOWN) override;
private:
    virtual void unobserved_set_byte(word p_addr, byte p_byte, AccessType p_at = AccessType::UNKNOWN) override;
};

#endif

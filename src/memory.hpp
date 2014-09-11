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
#include <memory> // Used by Memory

#include "common.hpp"

enum AccessType {AT_UNKNOWN, AT_INSTRUCTION, AT_OPERAND, AT_DATA, AT_LAST};
extern std::ostream &operator<<(std::ostream&, const AccessType);


/// This is the interface that must be supported by all Memory classes.
/// It is an Abstract Class with pure virtual get and put byte methods.
/// However, only function signatures are defined, no semantics.
/// Its only attribute is its size, which is declared on construction
/// and can never change.
class Device : public Named {
    // Types
public:
	class Configurator : public Named::Configurator
	{
	public:
		virtual Device *factory() const = 0;
		virtual word base() const = 0;
		virtual word size() const = 0;
		virtual word memory_size() const {return size(); }

		friend std::ostream &::operator <<(std::ostream &, const Configurator &);
	};
    // Attributes
public:
    const word m_size;
    // Methods
private:
    void operator=(const Device&);
    Device(const Device &);
protected:
    explicit Device(const Configurator &);
public:
    virtual void reset() {}
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN) = 0;
    virtual void set_byte(word p_addr, byte p_byte, AccessType = AT_UNKNOWN) = 0;

    friend std::ostream &::operator<<(std::ostream&, const Device &);
};


/// Ram is a trivial implementation of Device as a simple RAM device.


/// That is,
///    Ram R(0x0100);
///    R.put_byte(0x0080, 0xAA);
///    ...
///    assert (R.get_byte(0x0080) == 0xAA)
///
/// It can be constructed either with an externally defined array for storage,
/// or the storage can be created implicitly.  If the storage is allocated
/// on Ram construction, then it will be appropriately deallocated when
/// the Ram is destructed.
///
/// Addresses supplied to get_ and put_byte are relative to the start of the
/// device. That is, the address range is [0 .. m_size-1].
/// In that sense, the class is little more than a "safe" array.
class Ram : public Device {
public:
	class Configurator : public Device::Configurator
	{
	public:
		virtual Device *factory() const { return new Ram(*this); }
		virtual word base() const = 0;

		friend std::ostream &::operator <<(std::ostream &, const Configurator &);
	};
    // Attributes
private:
    const word m_base;
public:
    std::vector<byte> m_storage;
    // Methods
private:
    void operator=(const Ram&);
    Ram(const Ram &);
public:
    explicit Ram(const Configurator &);
    virtual ~Ram();
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    void         load(const std::string &p_filename);
    void         save(const std::string &p_filename) const;

    friend std::ostream &::operator<<(std::ostream&, const Ram &);

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
		virtual Device *factory() const { return new Rom(*this); }
		virtual const std::string &filename() const = 0;

		friend std::ostream &::operator <<(std::ostream &, const Configurator &);
	};
    // Attributes
private:
    bool m_is_writeable;
    // Methods
private:
    void operator=(const Rom&);
    Rom(const Rom &);
public:
    explicit Rom(const Configurator &);
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    void         set_writeable(bool p_writeable);

    friend std::ostream &::operator<<(std::ostream&, const Rom &);

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
	std::weak_ptr<Device> m_device;
    // Methods
protected:
    explicit Hook(const Configurator &);
public:
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void set_byte(word p_addr, byte p_byte, AccessType = AT_UNKNOWN);
    virtual int  execute (word p_addr, int p_byte, AccessType p_at = AT_UNKNOWN) = 0;

    friend std::ostream &::operator<<(std::ostream&, const Hook &);
};

/// This is the Main Memory class whose primary client in the CPU.
/// In this class, Addresses are "absolute".
/// It is a composite pattern with the actual memory being made up
/// of other iMemoryDevices located at specific places.
/// Optionally, the device can be replicated in the memory map by defining
/// it to occupy a larger part of the memory space.
/// It also provides a few helpers for getting and putting words as
/// combinations of getting and putting words (Low Endian).
class Memory : public Device {
public:
	class Configurator : public Device::Configurator
	{
	public:
		virtual Device *factory() const { return new Memory(*this); }
		virtual word size() const;
		virtual word base() const;

		friend std::ostream &::operator <<(std::ostream &, const Configurator &);
	};
    // Attributes
private:
	std::vector<std::weak_ptr<Device>> m_cell;
    // Methods
private:
    void operator=(const Memory&);
    Memory(const Memory &);
public:
    explicit Memory(const Configurator &);
    virtual byte          get_byte  (word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void          set_byte  (word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);
    word                  get_word  (word p_addr, AccessType p_at = AT_UNKNOWN);
    void                  set_word  (word p_addr, word p_word, AccessType p_at = AT_UNKNOWN);
    void                  add_device(word p_base, std::weak_ptr<Device> p_device, word p_size = 0);
    std::weak_ptr<Device> get_device(word p_addr) const;
    void                  add_hook  (word p_addr, std::weak_ptr<Hook> p_hook, word p_size = 0);

    friend std::ostream &::operator<<(std::ostream&, const Memory &);
};


#endif /* MEMORY_HPP_ */

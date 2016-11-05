// common.hpp
// Copyright 2016 Steve Palmer

#ifndef COMMON_HPP_
#define COMMON_HPP_

// Includes used everywhere

#include <ostream>  // part of the serialize pattern
#include <glibmm/ustring.h>  // Glib::ustring used everywhere
#include <log4cxx/logger.h>

// Logger Names
#define CTRACE_PREFIX      "ctrace"
#define PART_LOGGER        "part"
#define INSTRUCTION_PREFIX "instruction"

// Types used through out the emulator
#include <cstdint>
typedef uint8_t  byte;
typedef int8_t   signed_byte;
typedef uint16_t word;
typedef int16_t  signed_word;

// Utility macro
#define SIZE(n) (word((n)-word(1))+1)

// Utility classes
class NonCopyable
{
protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
private:
    NonCopyable(const NonCopyable &) = delete;
    const NonCopyable &operator=(const NonCopyable &) = delete;
};

// Streaming Utilitiy
#include <string>
#include <iomanip>
class Hex
{
	unsigned int m_v;
	int          m_w;
public:
	explicit Hex(byte p_v) : m_v(p_v), m_w(2) {}
	explicit Hex(word p_v) : m_v(p_v), m_w(4) {}
	friend std::ostream &operator<<(std::ostream &p_s, const Hex& p_n)
	{
		return p_s << std::hex << std::setw(p_n.m_w) << std::setfill('0') << std::uppercase << p_n.m_v;
	}
};

#endif

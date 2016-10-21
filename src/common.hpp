// common.hpp

#ifndef COMMON_HPP_
#define COMMON_HPP_

#define CTRACE_PREFIX "ctrace"

#include <cstdint>

typedef uint8_t  byte;
typedef int8_t   signed_byte;
typedef uint16_t word;
typedef int16_t  signed_word;

#include <string>
#include <ostream>
#include <iomanip>

#define SIZE(n) (word((n)-word(1))+1)

class NonCopyable
{
protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
private:
    NonCopyable(const NonCopyable &) = delete;
    const NonCopyable &operator=(const NonCopyable &) = delete;
};

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

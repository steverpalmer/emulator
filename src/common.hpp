/*
 * trivia.hpp
 *
 *  Created on: 7 May 2012
 *      Author: steve
 */

#ifndef TRIVIA_HPP_
#define TRIVIA_HPP_

#define CTRACE_PREFIX "ctrace"

#include <cstdint>

typedef uint8_t  byte;
typedef int8_t   signed_byte;
typedef uint16_t word;
typedef int16_t  signed_word;

#include <string>
#include <ostream>
#include <iomanip>


class Named {
public:
	class Configurator
	{
	public:
		virtual const std::string &name() const = 0;

		friend std::ostream &::operator <<(std::ostream &p_s, const Configurator &p_cfg)
		{
			p_s << "name=[" << p_cfg.name() << "]";
			return p_s;
		}
	};
	const std::string m_name;
public:
	const std::string &name() const { return m_name; }
protected:
	Named(const std::string &p_name = "") : m_name(p_name) {}
	Named(const Configurator &p_cfg) : m_name(p_cfg.name()) {}

	friend std::ostream &operator<<(std::ostream &p_s, const Named& p_n)
	{
		p_s << "name:[" << p_n.m_name << "]";
		return p_s;
	}
};

class Hex {
	unsigned int m_v;
	int          m_w;
public:
	explicit Hex(byte p_v) : m_v(p_v), m_w(2) {}
	explicit Hex(word p_v) : m_v(p_v), m_w(4) {}
	friend std::ostream &operator<<(std::ostream &p_s, const Hex& p_n)
	{
		p_s << std::hex << std::setw(p_n.m_w) << std::setfill('0') << std::uppercase << p_n.m_v;
		return p_s;
	}
};


#endif /* TRIVIA_HPP_ */

// part.hpp

#ifndef PART_HPP_
#define PART_HPP_

#include <ostream>

#include <glibmm/ustring.h>

class Part
{
public:
	class Configurator
	{
	public:
		virtual const Glib::ustring &id() const = 0;

		friend std::ostream &::operator <<(std::ostream &p_s, const Configurator &p_cfg)
		{
			return p_s << "id=\"" << p_cfg.id() << "\"";
		}
	};
private:
	const Glib::ustring m_id;
public:
	const Glib::ustring &id() const { return m_id; }
protected:
	explicit Part(const Glib::ustring &p_id = "") : m_id(p_id) {}
	explicit Part(const Configurator &p_cfg) : m_id(p_cfg.id()) {}
private:
	Part(const Part &);
	Part &operator=(const Part &);

	friend std::ostream &operator<<(std::ostream &p_s, const Part& p_p)
	{
		return p_s << "Part(" << p_p.m_id << ")";
	}
};

class ActivePart
    : public virtual Part
{
public:
    class Configurator
        : public Part::Configurator
    {
		friend std::ostream &::operator <<(std::ostream &p_s, const Configurator &p_cfg)
		{
			return p_s << static_cast<const Part::Configurator &>(p_cfg);
		}
    };
private:
    ActivePart(const ActivePart &);
    ActivePart &operator=(const ActivePart &);
protected:
    explicit ActivePart(const Configurator &p_cfg) : Part(p_cfg) {}
        
	friend std::ostream &operator<<(std::ostream &p_s, const ActivePart& p_ap)
	{
		return p_s << "ActivePart(" << static_cast<const Part &>(p_ap) << ")";
	}
};

class Computer
    : public ActivePart
{
};

#endif /* PART_HPP_ */

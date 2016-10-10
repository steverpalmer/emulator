// part.hpp

#ifndef PART_HPP_
#define PART_HPP_

#include <cassert>
#include <ostream>

#include <memory>
#include <string>
#include <unordered_map>
#include <glibmm/ustring.h>

#define PARTID_DELIMITER '/'
class Part;
typedef Glib::ustring PartId;

// PartsBin is a singleton that maps Glib::ustring to Part pointers
class PartsBin
{
public: // Types
    typedef PartId                                 key_type;
    typedef Part *                                 mapped_type;
private:
    typedef std::unordered_map<std::string, mapped_type> bin_type;
public:
    typedef bin_type::iterator                     iterator;
    typedef bin_type::const_iterator               const_iterator;
    typedef bin_type::size_type                    size_type;
    typedef std::pair<const key_type, mapped_type> value_type;
private:
    bin_type m_bin;
    PartsBin() : m_bin(100) {}
    PartsBin(const PartsBin &);
    PartsBin &operator=(const PartsBin &);
    static PartsBin *s_instance;
    int self_check() const;
public:
    static PartsBin &instance();
    static std::unique_ptr<Glib::ustring> canonical(const Glib::ustring &);
    // Wrapper boiler plate ...
    // Iterators
    inline iterator begin()
        {
            assert (self_check() == 0);
            iterator result(m_bin.begin());
            assert (self_check() == 0);
            return result;
        }
    inline iterator end()
        {
            assert (self_check() == 0);
            iterator result(m_bin.end());
            assert (self_check() == 0);
            return result;
        }
    inline const_iterator begin() const
        {
            assert (self_check() == 0);
            return m_bin.begin();
        }
    inline const_iterator end() const
        {
            assert (self_check() == 0);
            return m_bin.end();
        }
    // Capacity
    inline bool empty() const
        {
            assert (self_check() == 0);
            return m_bin.empty();
        }
    inline size_type size() const
        {
            assert (self_check() == 0);
            return m_bin.size();
        }
    inline size_type max_size() const
        {
            assert (self_check() == 0);
            return m_bin.max_size();
        }
    // Modifiers
    inline void clear()
        {
            assert (self_check() == 0);
            m_bin.clear();
            assert (self_check() == 0);
        }
    inline std::pair<iterator,bool> insert( const value_type& value )
        {
            assert (self_check() == 0);
            const value_type l_v(*PartsBin::canonical(value.first), value.second);
            std::pair<iterator,bool> result(m_bin.insert(l_v));
            assert (self_check() == 0);
            return result;
        }
    inline iterator insert( const_iterator hint, const value_type& value )
        {
            assert (self_check() == 0);
            const value_type l_v(*PartsBin::canonical(value.first), value.second);
            iterator result(m_bin.insert(hint, l_v));
            assert (self_check() == 0);
            return result;
        }
    template< class InputIt > inline void insert( InputIt first, InputIt last )
        {
            assert (self_check() == 0);
            m_bin.insert(first, last);
            assert (self_check() == 0);
        }
    inline iterator erase( const_iterator pos )
        {
            assert (self_check() == 0);
            iterator result(m_bin.erase(pos));
            assert (self_check() == 0);
            return result;
        }
    inline iterator erase( const_iterator first, const_iterator last )
        {
            assert (self_check() == 0);
            iterator result(m_bin.erase(first, last));
            assert (self_check() == 0);
            return result;
        }
    inline size_type erase( const key_type& key )
        {
            assert (self_check() == 0);
            size_type result(m_bin.erase(*PartsBin::canonical(key)));
            assert (self_check() == 0);
            return result;
        }
    // Lookup
    inline mapped_type& at( const key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin.at(*PartsBin::canonical(key)));
            assert (self_check() == 0);
            return result;
        }
    inline const mapped_type& at( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.at(*PartsBin::canonical(key));
        }
    inline mapped_type& operator[]( const key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin[*PartsBin::canonical(key)]);
            assert (self_check() == 0);
            return result;
        }
    inline mapped_type& operator[]( key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin[*PartsBin::canonical(key)]);
            assert (self_check() == 0);
            return result;
        }
    inline size_type count( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.count(*PartsBin::canonical(key));
        }
    inline iterator find( const key_type& key )
        {
            assert (self_check() == 0);
            iterator result(m_bin.find(*PartsBin::canonical(key)));
            assert (self_check() == 0);
            return result;
        }
    inline const_iterator find( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.find(*PartsBin::canonical(key));
        }
};

// Part is the base class for all key objects in the emulated computer.
class Part
{
public:
	class Configurator
	{
	public:
		virtual const PartId &id() const = 0;

		friend std::ostream &::operator <<(std::ostream &p_s, const Configurator &p_cfg);
	};
private:
	const PartId m_id; // Not necessarily Canonical!
public:
	const PartId &id() const { return m_id; }
protected:
	explicit Part(const PartId &p_id = "");
	explicit Part(const Configurator &p_cfg);
    virtual ~Part();
private:
	Part(const Part &);
	Part &operator=(const Part &);

	friend std::ostream &operator<<(std::ostream &p_s, const Part& p_p);
};

class ActivePart
    : public virtual Part
{
public:
    class Configurator
        : public Part::Configurator
    {
		friend std::ostream &::operator <<(std::ostream &p_s, const Configurator &p_cfg);
    };
private:
    ActivePart(const ActivePart &);
    ActivePart &operator=(const ActivePart &);
protected:
    explicit ActivePart(const Configurator &p_cfg) : Part(p_cfg) {}

	friend std::ostream &operator<<(std::ostream &p_s, const ActivePart& p_ap);
};

class Computer
    : public ActivePart
{
};

#endif /* PART_HPP_ */

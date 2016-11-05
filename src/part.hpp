// part.hpp
// Copyright 2016 Steve Palmer

#ifndef PART_HPP_
#define PART_HPP_

#include <cassert>

#include <set>  // Used by Part
#include <string>  // User by PartsBin
#include <map>  // Used by PartsBin

#include "common.hpp"

// Part is the base class for all key objects in the emulated computer.
class Part
    : protected NonCopyable
{
public:
    typedef Glib::ustring id_type;

    static const char    id_delimiter;
    static const id_type id_here;
    static const id_type id_up;

    static log4cxx::LoggerPtr log()
        {
            static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(PART_LOGGER));
            return result;
        }

	class Configurator
        : protected NonCopyable
	{
    protected:
        Configurator() = default;
	public:
        virtual ~Configurator() = default;
		virtual const id_type &id() const = 0;
        virtual Part *part_factory() const = 0;
        virtual void serialize(std::ostream &) const;
		friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
	};
private:
	id_type m_id; // Not necessarily Canonical!
protected:
    std::set<Part *> m_parents;
#if SERIALIZE_TO_DOT
    void serialize_parents(std::ostream &) const;
#endif
public:
	const id_type &id() const { return m_id; }
    static std::unique_ptr<id_type> canonical_id(const id_type &);
protected:
	explicit Part(const Configurator &);
public:
    virtual ~Part();

    void add_parent(Part &);
    void remove_parent(Part &);
    virtual void remove_child(Part &) {}

    virtual void serialize(std::ostream &) const;
	friend std::ostream &::operator<<(std::ostream &p_s, const Part &p_p)
        { p_p.serialize(p_s); return p_s; }
};


// PartsBin is a singleton map wrapper mapping Glib::ustring to Part pointers
class PartsBin
    : protected NonCopyable
{
public:
    class Configurator
        : protected NonCopyable
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual const Part::Configurator *part(int i) const = 0;

        virtual void serialize(std::ostream &) const;
        friend std::ostream &::operator<<(std::ostream &p_s, const Configurator &p_cfgr)
            { p_cfgr.serialize(p_s); return p_s; }
    };
public: // Types
    typedef Part::id_type                          key_type;
    typedef Part *                                 mapped_type;
private:
    typedef std::map<std::string, mapped_type>     bin_type;
public:
    typedef bin_type::iterator                     iterator;
    typedef bin_type::const_iterator               const_iterator;
    typedef bin_type::size_type                    size_type;
    typedef std::pair<const key_type, mapped_type> value_type;
private:
    bin_type m_bin;
    PartsBin() = default;
    int self_check() const;
public:
    virtual ~PartsBin() = default;
    static PartsBin &instance()
        {
            static PartsBin *s_instance;
            if (!s_instance)
            {
                s_instance = new PartsBin;
                assert (s_instance);
            }
            return *s_instance;
        }
    void build(const Configurator &p_cfg)
        {
            for (int i(0); const auto &p = p_cfg.part(i); i++)
                (void) p->part_factory();  // parts are self registering
        }
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
    void clear();
    inline std::pair<iterator,bool> insert( const value_type& value )
        {
            assert (self_check() == 0);
            const value_type l_v(*Part::canonical_id(value.first), value.second);
            std::pair<iterator,bool> result(m_bin.insert(l_v));
            assert (self_check() == 0);
            return result;
        }
    inline iterator insert( const_iterator hint, const value_type& value )
        {
            assert (self_check() == 0);
            const value_type l_v(*Part::canonical_id(value.first), value.second);
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
            size_type result(m_bin.erase(*Part::canonical_id(key)));
            assert (self_check() == 0);
            return result;
        }
    // Lookup
    inline mapped_type& at( const key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin.at(*Part::canonical_id(key)));
            assert (self_check() == 0);
            return result;
        }
    inline const mapped_type& at( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.at(*Part::canonical_id(key));
        }
    inline mapped_type& operator[]( const key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin[*Part::canonical_id(key)]);
            assert (self_check() == 0);
            return result;
        }
    inline mapped_type& operator[]( key_type& key )
        {
            assert (self_check() == 0);
            mapped_type &result(m_bin[*Part::canonical_id(key)]);
            assert (self_check() == 0);
            return result;
        }
    inline size_type count( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.count(*Part::canonical_id(key));
        }
    inline iterator find( const key_type& key )
        {
            assert (self_check() == 0);
            iterator result(m_bin.find(*Part::canonical_id(key)));
            assert (self_check() == 0);
            return result;
        }
    inline const_iterator find( const key_type& key ) const
        {
            assert (self_check() == 0);
            return m_bin.find(*Part::canonical_id(key));
        }
    virtual void serialize(std::ostream &) const;
    friend std::ostream &::operator<<(std::ostream &p_s, const PartsBin &p_pb)
        { p_pb.serialize(p_s); return p_s; }

};

#endif

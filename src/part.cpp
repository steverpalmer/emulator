// part.cpp

#include <string>
#include <list>

#include <iostream>

#include "part.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".part.cpp"));
    return result;
}

Part::Part(const Part::Configurator &p_cfgr)
    : m_id(p_cfgr.id())
{
    static int anonymous_id_counter = 1000;
    LOG4CXX_INFO(cpptrace_log(), "Part::Part(" << p_cfgr << ")");
    LOG4CXX_INFO(Part::log(), "created [" << m_id << "]");
    if (m_id.empty())
    {
        m_id = std::to_string(++anonymous_id_counter);
    }
    PartsBin::instance()[m_id] = this;
}

Part::~Part()
{
    LOG4CXX_INFO(cpptrace_log(), "Part::~Part([" << id() << "])");
    for (auto p: m_parents)
    {
        p->remove_child(*this);
        LOG4CXX_INFO(Part::log(), "removing [" << p->id() << "] as parent of [" << id() << "]");
    }
    m_parents.clear();
    LOG4CXX_INFO(Part::log(), "destructed [" << m_id << "]");
}

const char Part::id_delimiter     = '.';
const Part::id_type Part::id_here = ".";
const Part::id_type Part::id_up   = "..";

std::unique_ptr<Part::id_type> Part::canonical_id(const id_type &p_s)
{
#if 0
    LOG4CXX_INFO(cpptrace_log(), "Part::canonical_id(" << p_s << ")");
    std::list<Glib::ustring> l;
    const Glib::ustring s(p_s.normalize());
    // split on Delimiter
    Glib::ustring::size_type n;
    for (Glib::ustring::size_type i(0);
         (n = s.find(Part::id_delimiter, i)) != Glib::ustring::npos;
         i = n + 1 )
    {
        assert (s[n] == Part::id_delimiter);
        l.push_back(s.substr(i, n-1));
    }
    // Discard a trailing /
    if (!l.empty() && *l.rbegin() == "")
        l.pop_back();
    // Process // . and ..
    std::list<Glib::ustring> canonical_l;
    for (const auto &i : l)
    {
        if (i == "")
        {
            canonical_l.clear();
        }
        else if (i == Part::id_here)
        {
            /* Do Nothing */
        }
        else if (i == Part::id_up)
        {
            if (not canonical_l.empty())
                canonical_l.pop_back();
            else
                canonical_l.push_back(i);
        }
        else
        {
            canonical_l.push_back(i);
        }
    }
    // Rebuild string
    std::unique_ptr<Glib::ustring> result(new Glib::ustring);
    assert (result);
    for (const auto &t : canonical_l)
    {
        *result += Part::id_delimiter;
        *result += t;
    }
    if (*result == "")
        *result += Part::id_delimiter;
    return result;
#else
    return std::unique_ptr<Glib::ustring>(new Glib::ustring(p_s));
#endif
}

void Part::add_parent(Part &p_parent)
{
    LOG4CXX_INFO(Part::log(), "making [" << p_parent.id() << "] parent of [" << id() << "]");
    (void) m_parents.insert(&p_parent);
}

void Part::remove_parent(Part &p_parent)
{
    LOG4CXX_INFO(Part::log(), "removing [" << p_parent.id() << "] as parent of [" << id() << "]");
    (void) m_parents.erase(&p_parent);
}


int PartsBin::self_check() const
{
    LOG4CXX_DEBUG(cpptrace_log(), "PartsBin::self_check()");
#if 0
    for (auto &i : m_bin)
    {
        const std::unique_ptr<Part::id_type> s(Part::canonical_id(i.first));
        if (i.first != *s) return 1;
        if (!i.second) return 2;
    }
#endif
    return 0;
}

void PartsBin::clear()
{
    LOG4CXX_INFO(cpptrace_log(), "PartsBin::clear()");
    assert (self_check() == 0);
    for (auto it = m_bin.begin(); it != m_bin.end(); it = m_bin.erase(it))
    {
        LOG4CXX_DEBUG(Part::log(), *this);
        LOG4CXX_DEBUG(Part::log(), "// Deleting:" << it->first);
        auto p(it->second);
        it->second = 0;
        delete p;
    }
    LOG4CXX_DEBUG(Part::log(), *this);
    assert (m_bin.empty());
    assert (self_check() == 0);
}


void Part::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << id() << ";\n";
#else
    p_s << "name=\"" << id() << "\"";
#endif
}

void PartsBin::Configurator::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << "digraph partsbin_configurator {\n";
    for (int i(0); const Part::Configurator *p = part(i); i++)
        p->serialize(p_s);
    p_s << "}\n";
#else
    p_s << "<parts_bin>";
    for (int i(0); const Part::Configurator *p = part(i); i++)
        p->serialize(p_s);
    p_s << "</parts_bin>";
#endif
}

#if SERIALIZE_TO_DOT
void Part::serialize_parents(std::ostream &p_s) const
{
    for (auto device : m_parents)
        p_s << id() << " -> " << device->id() << " [style=dashed];\n";
}
#endif

void Part::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << m_id << ";\n";
    serialize_parents(p_s);
#else
    p_s << "id(\"" << m_id << "\")";
#endif
}

void PartsBin::serialize(std::ostream &p_s) const
{
#if SERIALIZE_TO_DOT
    p_s << "digraph partsbin {\n";
    p_s << "partsbin [color=blue];\n";
    for (auto &pair : m_bin )
    {
        p_s << "partsbin -> " << pair.first << ";\n";
        if (pair.second)
            p_s << *pair.second;
    }
    p_s << "}\n";
#else
    p_s << "PartsBin([";
    for (auto &pair : m_bin )
    {
        p_s << "(\"" << pair.first << "\":";
        if (pair.second)
            p_s << *pair.second;
        else
            p_s << "_";
        p_s << "), ";
    }
    p_s << "])";
#endif
}

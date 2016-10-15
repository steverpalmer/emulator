// part.cpp

#include <list>

#include <log4cxx/logger.h>

#include "part.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".part.cpp"));
    return result;
}

Part::Part(const Part::Configurator &p_cfgr)
    : m_id(p_cfgr.id())
{
    LOG4CXX_INFO(cpptrace_log(), "Part::Part(" << p_cfgr << ")");
    PartsBin::instance()[m_id] = this;
}

Part::~Part()
{
    LOG4CXX_INFO(cpptrace_log(), "Part::~Part(" << id() << ")");
    (void) PartsBin::instance().erase(m_id);
}

const char Part::id_delimiter     = '.';
const Part::id_type Part::id_here = ".";
const Part::id_type Part::id_up   = "..";
    
std::unique_ptr<Part::id_type> Part::canonical_id(const id_type &p_s)
{
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
}

int PartsBin::self_check() const
{
    LOG4CXX_INFO(cpptrace_log(), "PartsBin::self_check()");
    for (const auto &i : m_bin)
    {
        const std::unique_ptr<Part::id_type> s(Part::canonical_id(i.first));
        if (i.first != *s) return 1;
    }
    return 0;
}


std::ostream &operator<<(std::ostream &p_s, const Part::Configurator &p_cfgr)
{
    return p_s << "name=\"" << p_cfgr.id() << "\" ";
}

std::ostream &operator<<(std::ostream &p_s, const Part& p_p)
{
    return p_s << "Part(" << p_p.m_id << ")";
}

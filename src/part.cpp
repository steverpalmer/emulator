// part.cpp

#include <list>

#include "part.hpp"

std::unique_ptr<Glib::ustring> PartsBin::canonical(const Glib::ustring &p_s)
{
    std::list<Glib::ustring> l;
    const Glib::ustring s(p_s.normalize());
    // split on Delimiter
    Glib::ustring::size_type n;
    for (Glib::ustring::size_type i(0);
         (n = s.find(PARTID_DELIMITER, i)) != Glib::ustring::npos;
         i = n + 1 )
    {
        assert (s[n] == PARTID_DELIMITER);
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
        else if (i == ".")
        {
            /* Do Nothing */
        }
        else if (i == "..")
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
    std::unique_ptr<Glib::ustring> result(new Glib::ustring)
        ;
    assert (result);
    for (const auto &t : canonical_l)
    {
        *result += PARTID_DELIMITER;
        *result += t;
    }
    if (*result == "")
        *result += PARTID_DELIMITER;
    return result;
}

bool is_canonical(const Glib::ustring &p_s)
{
    const std::unique_ptr<Glib::ustring> s(PartsBin::canonical(p_s));
    const bool result(p_s == *s);
    return result;
}

int PartsBin::self_check() const
{
    for (const auto &i : m_bin)
    {
        if (is_canonical(i.first)) return 1;
    }
    return 0;
}

PartsBin &PartsBin::instance()
{
    if (!s_instance)
        s_instance = new PartsBin;
    return *s_instance;
}

Part::Part(const PartId &p_id)
    : m_id(p_id)
{
    PartsBin::instance()[m_id] = this;
}

Part::Part(const Part::Configurator &p_cfg)
    : m_id(p_cfg.id())
{
    PartsBin::instance()[m_id] = this;
}

Part::~Part()
{
    (void) PartsBin::instance().erase(m_id);
}

std::ostream &operator<<(std::ostream &p_s, const Part::Configurator &p_cfg)
{
    return p_s << "id=\"" << p_cfg.id() << "\" ";
}

std::ostream &operator<<(std::ostream &p_s, const ActivePart::Configurator &p_cfg)
{
    return p_s << static_cast<const Part::Configurator &>(p_cfg);
}

std::ostream &operator<<(std::ostream &p_s, const Part& p_p)
{
    return p_s << "Part(" << p_p.m_id << ")";
}

std::ostream &operator<<(std::ostream &p_s, const ActivePart& p_ap)
{
    return p_s << "ActivePart(" << static_cast<const Part &>(p_ap) << ")";
}

// terminal_interface.hpp

#ifndef TERMINAL_INTERFACE_HPP_
#define TERMINAL_INTERFACE_HPP_

#include <ostream>

#include <set>

#include "common.hpp"
#include "part.hpp"

enum VDGMode {
    VDG_MODE0,  /* 32x16 text & 64x48 block graphics in 0.5KB */
    VDG_MODE1A, /* 64x64 4 colour graphics           in 1KB */
    VDG_MODE1,  /* 128x64 black & white graphics     in 1KB */
    VDG_MODE2A, /* 128x64 4 colour graphics          in 2KB */
    VDG_MODE2,  /* 128x96 black & white graphics     in 1.5KB */
    VDG_MODE3A, /* 128x96 4 colour graphics          in 3KB */
    VDG_MODE3,  /* 128x192 black & white graphics    in 3KB */
    VDG_MODE4A, /* 128x192 4 colour graphics         in 6KB */
    VDG_MODE4,  /* 256x192 black & white graphics    in 6KB */
    VDG_LAST
};

std::ostream &operator<<(std::ostream &p_s, const VDGMode p_mode)
{
    if (p_mode >= 0 && p_mode < VDG_LAST) {
        static const char *name[VDG_LAST] = {
            "mode 0",
            "mode 1a",
            "mode 1",
            "mode 2a",
            "mode 2",
            "mode 3a",
            "mode 3",
            "mode 4a",
            "mode 4"
        };
        p_s << name[p_mode];
    }
    else
        p_s << int(p_mode);
    return p_s;
}

enum KBDSpecials {
    KBD_NO_KEYPRESS = 0xFFFF,
    KBD_LEFT        = 0x2190,
    KBD_UP          = 0x2191,
    KBD_RIGHT       = 0x2192,
    KBD_DOWN        = 0x2193,
    KBD_LOCK        = 0x21EC,
    KBD_COPY        = 0x2298,
};

class TerminalInterface
    : public virtual Part
{
public:
    class Observer
    {
    private:
        Observer(const Observer &);
        Observer &operator=(const Observer &);
    protected:
        Observer();
    public:
        virtual void vdg_mode_update(TerminalInterface *p_terminal, VDGMode p_mode) = 0;
    };

private:
    std::set<Observer *> m_observers;
protected:
    TerminalInterface();
private:
    TerminalInterface(const TerminalInterface &);
    TerminalInterface &operator=(const TerminalInterface &);
public:
    virtual VDGMode vdg_mode() const = 0;
    virtual void    set_keypress(int p_key) = 0;
    virtual void    set_is_shift_pressed(bool p_flag) = 0;
    virtual void    set_is_ctrl_pressed(bool p_flag) = 0;
    virtual void    set_is_rept_pressed(bool p_flag) = 0;
protected:
    inline void vdg_mode_notify(VDGMode p_mode)
        { for (Observer * obs : m_observers) obs->vdg_mode_update(this, p_mode); }
public:
    inline void attach(Observer &p_observer) { m_observers.insert(&p_observer); }
    inline void detach(Observer &p_observer) { m_observers.erase(&p_observer); }

    virtual ~TerminalInterface() { m_observers.clear(); }

    friend std::ostream &::operator<<(std::ostream &p_s, const TerminalInterface &p_ti)
        { return p_s << "TerminalInterface(" << p_ti.id() << ")"; }
};

#endif /* TERMINAL_INTERFACE_HPP_ */

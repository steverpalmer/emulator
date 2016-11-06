// terminal_interface.hpp
// Copyright 2016 Steve Palmer

#ifndef TERMINAL_INTERFACE_HPP_
#define TERMINAL_INTERFACE_HPP_

#include <set>

#include "common.hpp"
#include "part.hpp"

class TerminalInterface
    : protected NonCopyable
{
public:
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

    friend std::ostream &::operator<<(std::ostream &p_s, const VDGMode &p_mode)
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

    static const gunichar KBD_NO_KEYPRESS;
    static const gunichar KBD_LEFT;
    static const gunichar KBD_UP;
    static const gunichar KBD_RIGHT;
    static const gunichar KBD_DOWN;
    static const gunichar KBD_SHIFT;
    static const gunichar KBD_LOCK;
    static const gunichar KBD_CTRL;
    static const gunichar KBD_COPY;

    class Observer
        : protected NonCopyable
    {
    protected:
        Observer() = default;
    public:
        virtual ~Observer() = default;
        virtual void vdg_mode_update(TerminalInterface &, VDGMode) = 0;
        virtual void subject_loss(const TerminalInterface &) = 0;
    };

private:
    std::set<Observer *> m_observers;
protected:
    TerminalInterface() = default;
public:
    virtual Part::id_type id() const = 0;
    virtual VDGMode vdg_mode() const = 0;
    virtual void set_keypress(gunichar, bool p_repeat=false) = 0;
protected:
    void vdg_mode_notify(VDGMode);
public:
    void attach(Observer &);
    void detach(Observer &);

    virtual ~TerminalInterface();

};

#endif

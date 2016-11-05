// terminal_interface.cpp
// Copyright 2016 Steve Palmer

#include "terminal_interface.hpp"

const gunichar TerminalInterface::KBD_NO_KEYPRESS = -1;
const gunichar TerminalInterface::KBD_LEFT        = 0x2190;
const gunichar TerminalInterface::KBD_UP          = 0x2191;
const gunichar TerminalInterface::KBD_RIGHT       = 0x2192;
const gunichar TerminalInterface::KBD_DOWN        = 0x2193;
const gunichar TerminalInterface::KBD_SHIFT       = 0x21E7;
const gunichar TerminalInterface::KBD_LOCK        = 0x21EA;
const gunichar TerminalInterface::KBD_CTRL        = 0xE000;
const gunichar TerminalInterface::KBD_COPY        = 0xE001;

TerminalInterface::~TerminalInterface()
{
    m_observers.clear();
}

void TerminalInterface::attach(Observer &p_observer)
{
    m_observers.insert(&p_observer);
}

void TerminalInterface::detach(Observer &p_observer)
{
    m_observers.erase(&p_observer);
}


void TerminalInterface::vdg_mode_notify(VDGMode p_mode)
{
    for (auto obs : m_observers)
        obs->vdg_mode_update(*this, p_mode);
}

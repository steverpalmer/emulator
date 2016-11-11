// terminal_interface.cpp
// Copyright 2016 Steve Palmer

#include "terminal_interface.hpp"

TerminalInterface::~TerminalInterface()
{
    for (auto obs : m_observers)
        obs->subject_loss(*this);
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

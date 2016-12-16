// keyboard_adaptor.hpp
// Copyright 2016 Steve Palmer

#ifndef KEYBOARD_ADAPTOR_HPP_
#define KEYBOARD_ADAPTOR_HPP_

#include <SDL.h>

#include "common.hpp"
#include "device.hpp"
#include "ppia.hpp"
#include "memory.hpp"
#include "dispatcher.hpp"

class KeyboardAdaptor
    : public Device
{
    // Types
private:
    class DownHandler
        : public Dispatcher::StateHandler<const KeyboardAdaptor>
    {
    public:
        DownHandler(const KeyboardAdaptor &);
        virtual ~DownHandler() = default;
    private:
        virtual void handle(const SDL_Event &) override;
    };

    class UpHandler
        : public Dispatcher::StateHandler<const KeyboardAdaptor>
    {
    public:
        UpHandler(const KeyboardAdaptor &);
        virtual ~UpHandler() = default;
    private:
        virtual void handle(const SDL_Event &) override;
    };

public:
    class Configurator
        : public virtual Device::Configurator
    {
    protected:
        Configurator() = default;
    public:
        virtual ~Configurator() = default;
        virtual const Memory::Configurator *ppia() const = 0;
        virtual Device *device_factory() const override
            { return new KeyboardAdaptor(*this); }

        virtual void serialize(std::ostream &) const override;
    };

public:
    Atom::KeyboardInterface   *m_ppia;
private:
    std::map<SDL_Scancode, Atom::KeyboardInterface::Key> keys;
    const DownHandler         down_handler;
    const UpHandler           up_handler;
    KeyboardAdaptor();
public:
    explicit KeyboardAdaptor(const Configurator &);
private:
    virtual ~KeyboardAdaptor() = default;
public:
    virtual void serialize(std::ostream &) const override;
};

#endif

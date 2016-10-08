// ppia.hpp

#ifndef PPIA_HPP_
#define PPIA_HPP_

#include <pthread.h>

#include <ostream>
#include <array>

#include "common.hpp"
#include "device.hpp"
#include "terminal_interface.hpp"


class Ppia : public Device, public TerminalInterface {
public:
    class Configurator : public Device::Configurator
    {
    public:
        /// 1. Constructor Information - Name only
        /// 2. Factory Method
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Ppia(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::array<byte, 4> m_register;
    struct {
        mutable pthread_mutex_t mutex;
        VDGMode vdg_mode;
        VDGMode notified_vdg_mode;
        int     pressed_key;
        bool    is_shift_pressed;
        bool    is_ctrl_pressed;
        bool    is_rept_pressed;
    } m_terminal;
    // Methods
private:
    Ppia(const Ppia &);
    Ppia &operator=(const Ppia&);
    byte get_PortB(int p_row);
    byte get_PortC(byte p_previous);
    void set_PortA(byte p_byte);
public:
    explicit Ppia(const Configurator &);
    virtual ~Ppia();

    // Device
    
    virtual word size() const { return 4; }
    virtual void _reset();
    virtual byte _get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void _set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);

    // TerminalInterface
    
    virtual VDGMode vdg_mode() const;
    virtual void    set_keypress(int p_key);
    virtual void    set_is_shift_pressed(bool p_flag);
    virtual void    set_is_ctrl_pressed(bool p_flag);
    virtual void    set_is_rept_pressed(bool p_flag);
    
    friend std::ostream &::operator<<(std::ostream&, const Ppia &);
};

#endif /* PPIA_HPP_ */

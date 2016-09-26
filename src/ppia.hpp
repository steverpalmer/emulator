/*
 * ppia.hpp
 *
 *  Created on: 30 Mar 2012
 *      Author: steve
 */

#ifndef PPIA_HPP_
#define PPIA_HPP_

#include <ostream>
#include <array>

#include "common.hpp"
#include "memory.hpp"

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
extern std::ostream &operator<<(std::ostream&, const VDGMode);

enum KBDSpecials {
    KBD_NO_KEYPRESS = 0xFFFF,
    KBD_LEFT        = 0x2190,
    KBD_UP          = 0x2191,
    KBD_RIGHT       = 0x2192,
    KBD_DOWN        = 0x2193,
    KBD_LOCK        = 0x21EC,
    KBD_COPY        = 0x2298,
};

class Ppia : public Device {
public:
    class Configurator : public Device::Configurator
    {
    public:
        virtual std::unique_ptr<Device> factory() const
            { return std::unique_ptr<Device>(new Ppia(*this)); }

        friend std::ostream &::operator <<(std::ostream &, const Configurator &);
    };
    // Attributes
private:
    std::vector<byte> m_byte;
public:
    struct IO {
        VDGMode m_vdg_mode;
        bool    m_is_vdg_refresh;
        int     m_pressed_key;
        bool    m_is_shift_pressed;
        bool    m_is_ctrl_pressed;
        bool    m_is_rept_pressed;
    } m_io;
    // Methods
private:
    void operator=(const Ppia&);
    Ppia(const Ppia &);
    byte get_PortB(int p_row);
    byte get_PortC(byte p_previous);
    void set_PortA(byte p_byte);
public:
    explicit Ppia(const Configurator &);
    virtual ~Ppia();
    virtual word size() const { return 4; }
    virtual void reset();
    virtual byte get_byte(word p_addr, AccessType p_at = AT_UNKNOWN);
    virtual void set_byte(word p_addr, byte p_byte, AccessType p_at = AT_UNKNOWN);

    friend std::ostream &::operator<<(std::ostream&, const Ppia &);
};

#endif /* PPIA_HPP_ */

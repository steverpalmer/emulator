// atom_tape.cpp
// Copyright 2016 Steve Palmer

#include "atom_tape.hpp"

static log4cxx::LoggerPtr cpptrace_log()
{
    static log4cxx::LoggerPtr result(log4cxx::Logger::getLogger(CTRACE_PREFIX ".atom_tape.cpp"));
    return result;
}

class Fail
    : public ExceptionWithReason
{
public:
    Fail(const Glib::ustring &p_reason) : ExceptionWithReason(p_reason) {}
};

Atom::Tape::Filename::Filename(word addr, Memory &memory)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::Filename::Filename(...)");
    // string is located in memory at address addr,
    // No more that 14 characters long
    // terminated by a #0D
    int i(0);
    enum {More, EndOfFilename, EndOfBuffer} state(More);
    do
    {
        cstr[i] = memory.get_byte(addr+i);
        if (cstr[i] == '\x0D')
            state = EndOfFilename;
        else if (++i == 14)
            state = EndOfBuffer;
    }
    while (state == More);
    switch (state)
    {
    case EndOfFilename:
        cstr[i] = 0;
        // FIXME: filename remapping for the OS
        break;
    default:
        cstr[0] = 0;
    }
}

const char *Atom::Tape::Filename::path(const Glib::ustring &directory)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::Filename::path(...)");
    path_result = directory;
    path_result += '/';
    path_result += cstr;
    return path_result.c_str();
}

Atom::Tape::OSSAVE_Adaptor::OSSAVE_Adaptor(Tape &p_tape)
    : Hook("OSSAVE")
    , tape(p_tape)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSSAVE_Adaptor::OSSAVE_Adaptor(...)");
}

void Atom::Tape::OSSAVE_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSSAVE_Adaptor::attach()");
    tape.address_space->add_child(0xFAE5, *this);
}

int Atom::Tape::OSSAVE_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSSAVE_Adaptor::get_byte_hook(...)");
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        try
        {
            const word X(tape.mcs6502->m_register.X);
            Filename filename(tape.address_space->get_word(X), *tape.address_space);
            if (!filename.cstr[0]) throw Fail("OSSAVE Bad Filename");
            std::ofstream stream(filename.path(tape.directory), std::ios::binary);
            if (stream.fail()) throw Fail("OSSAVE Failed to Open file");
            for (int i(2); stream.good() && i < 6; i += 1)
                stream.put(tape.address_space->get_word(X + i));
            if (stream.fail()) throw Fail("OSSAVE Failed to write header");
            const word start(tape.address_space->get_word(X + 6));
            const word end(tape.address_space->get_word(X + 8));
            for (int addr(start); stream.good() && addr < end; addr += 1)
                stream.put(tape.address_space->get_byte(addr));
            if (stream.fail()) throw Fail("OSSAVE Failed to write data");
            stream.close();
            if (stream.fail()) throw Fail("OSSAVE Failed to close file");
            result = 0x60; // RTS
        }
        catch(Fail &e)
        {
            LOG4CXX_ERROR(cpptrace_log(), e.what());
            result = 0x00; // BRK
        }
    }
    return result;
}

Atom::Tape::OSLOAD_Adaptor::OSLOAD_Adaptor(Tape &p_tape)
    : Hook("OSLOAD")
    , tape(p_tape)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSLOAD_Adaptor::OSLOAD_Adaptor(...)");
}

void Atom::Tape::OSLOAD_Adaptor::attach()
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSLOAD_Adaptor::attach()");
    tape.address_space->add_child(0xF96E, *this);
}

int Atom::Tape::OSLOAD_Adaptor::get_byte_hook(word, AccessType p_at)
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::OSLOAD_Adaptor::get_byte_hook(...)");
    int result(-1);
    if (p_at == AT_INSTRUCTION)
    {
        try
        {
            const word X(tape.mcs6502->m_register.X);
            Filename filename(tape.address_space->get_word(X), *tape.address_space);
            if (!filename.cstr[0]) throw Fail("OSLOAD Bad Filename");
            const char *path(filename.path(tape.directory));
            LOG4CXX_DEBUG(cpptrace_log(), "Opening File " << path);
            std::ifstream stream(path, std::ios::binary);
            if (stream.fail()) throw Fail("OSLOAD Failed to Open file");
            const byte reload_address_lsb(stream.get());
            const byte reload_address_msb(stream.get());
            // Although not described in the ATAP
            // *RUN transfers control to the address at 0xD6
            tape.address_space->set_byte(0xD6, stream.get());
            tape.address_space->set_byte(0xD7, stream.get());
            if (stream.fail()) throw Fail("OSLOAD Failed to read header");
            const word reload_address = (
                (tape.address_space->get_byte(X + 4) & 0x80) ?
                tape.address_space->get_word(X + 2) :
                word((reload_address_msb << 8) | reload_address_lsb)
                );
            LOG4CXX_DEBUG(cpptrace_log(), "Loading to " << Hex(reload_address));
            for (int i(0); stream.good(); i++)
            {
                const auto ch(stream.get());
                if (std::ifstream::traits_type::not_eof(ch))
                    tape.address_space->set_byte(reload_address+i, ch);
            }
            if (!stream.eof()) throw Fail("OSLOAD Failed to read data");
            stream.close();
            result = 0x60; // RTS
        }
        catch(Fail &e)
        {
            LOG4CXX_ERROR(cpptrace_log(), e.what());
            result = 0x00; // BRK
        }
    }
    return result;
}

Atom::Tape::Tape(const Configurator &p_cfgr)
    : Device(p_cfgr)
    , directory(p_cfgr.directory())
    , mcs6502(dynamic_cast<MCS6502 *>(p_cfgr.mcs6502()->device_factory()))
    , OSSAVE(new OSSAVE_Adaptor(*this))
    , OSLOAD(new OSLOAD_Adaptor(*this))
{
    LOG4CXX_INFO(cpptrace_log(), "Atom::Tape::Tape(...)");
    assert (mcs6502);
    address_space = mcs6502->address_space();
    assert (address_space);
    assert (OSSAVE);
    OSSAVE->attach();
    assert (OSLOAD);
    OSLOAD->attach();
}

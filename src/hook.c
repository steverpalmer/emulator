static int hook_6502exectrace_start(void       *p_object,
                                    Memory     &p_mem,
                                    word    p_addr,
                                    int        p_byte,
                                    AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_trace_start");
    assert(p_object);
    ((MCS6502 *)p_object)->trace_start();
    return -1; // continue as normal
}

static int hook_6502exectrace_finish(void       *p_object,
                                     Memory     &p_mem,
                                     word    p_addr,
                                     int        p_byte,
                                     AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_trace_finish");
    assert(p_object);
    ((MCS6502 *)p_object)->trace_finish();
    return -1; // continue as normal
}

static int hook_mem_watch(void       *p_object,
                          Memory     &p_mem,
                          word    p_addr,
                          int        p_byte,
                          AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_mem_watch");
    assert (p_object);
    int result(p_mem.get_byte(p_addr, p_at));
    log4c_category_t *log((log4c_category_t *)p_object);
    if (p_byte < 0)
        log4c_category_notice(log, "%s read %04X => %02X",
                              AccessTypeName[p_at],
                              p_addr,
                              result);
    // result is the right value, so return it and finish get_byte
    else {
        log4c_category_notice(log, "%s write %02X to %04X currently %02X",
                              AccessTypeName[p_at],
                              p_byte,
                              p_addr,
                              result);
        result = -1; // continue with write attempt
    }
    return result;
}

/// This Hook fires when the program is about to execute the OSWRCH function.
/// It is also called from OSCRLF and OSECHO.
/// The accumulator holds the byte to be output.  On read of the first instruction
/// this hook sends the character into another output stream.
/// It returns the original instruction, so that output continues as normal.
/// From ATAP...
/// This subroutine sends the byte in the accumulator to the output channel.
/// Control characters are normally recognised as detailed in Section 18.1.3.
/// All registers are preserved.
static int hook_OSWRCH(void       *p_object,
                       Memory     &p_mem,
                       word    p_addr,
                       int        p_byte,
                       AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_OSWRCH");
    if (p_byte < 0 && p_at == AT_INSTRUCTION) {
        assert (p_object);
        Atom &atom(*((Atom *)p_object));
        (void)putc((int)atom.m_6502.m_register.A, atom.m_OSWRCH_stream);
    }
    return -1; // continue as normal
}

/// This Hook fires when the program is about to execute the OSRDCH function.
/// It is also called from OSECHO.
/// The hook function instead reads the input from a predefined stream and
/// returns an RTS instruction code.  In the process, it maps '\n' characters
/// from the C stream into the atom '\r' (#D) character.
/// At the EOF of the stream, the hook raises the SIGCHLD signal and stops the
/// emulation, or it returns -1 which prompts the original instructions to be
/// executed and the keyboard polled as if unhooked.
/// It increases the cycle time by 100000 to similate the time taken to read
/// character
/// From ATAP...
/// This subroutine reads a byte from the input channel and returns it in A.
/// The state of N, Z, and C is undefined; all other registers are preserved.
static int hook_OSRDCH(void       *p_object,
                       Memory     &p_mem,
                       word    p_addr,
                       int        p_byte,
                       AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_OSRDCH");
    int result(-1); // by default, continue as normal
    if (p_byte < 0 && p_at == AT_INSTRUCTION) {
        int rv;
        assert (p_object);
        Atom &atom(*((Atom *)p_object));
        const int ch(getc(atom.m_OSRDCH_stream));
        switch (ch) {
        case '\n' : // handle <return>
            atom.m_6502.m_register.A = (byte)'\r';
            break;
        case EOF : // handle end of input
            ATOM_CTRACE_LOG("hook_OSRDCH reached EOF");
            if (atom.m_OSRDCH_fallback)
                return -1; //TODO: Nasty hack; should unhook
            ATOM_CTRACE_LOG("hook_OSRDCH stopping");
            atom.m_6502.m_register.A = 0;
            atom.stop();
            ATOM_CTRACE_LOG("calling raise(%d) ...", SIGCHLD);
            rv = raise(SIGCHLD);
            assert (!rv);
            break;
        default :
            atom.m_6502.m_register.A = (byte)ch;
        }
        atom.m_6502.m_cycles += 100000; // Simulate 0.1s
        result = 0x60; // function complete, return an RTS instruction!
    }
    return result;
}

/// On entry X must point to the following data in zero page:
///   X+0 Address of string of characters, terminated by #0D, which is the file name.
bool extract_filename(Atom &atom, char * filename, int fnlength)
{
    ATOM_CTRACE_LOG("extract_filename");
    assert (filename);
    assert (fnlength > 0);
    fnlength -= 1; // allow for NULL terminator
    const word filename_addr(atom.m_memory.get_word(atom.m_6502.m_register.X));
    int i(0);
    enum {More, EOFilename, EOBuffer} state = More;
    do {
        filename[i] = atom.m_memory.get_byte(filename_addr+i);
        if (filename[i] == '\r')
            state = EOFilename;
        else {
            i++;
            if (i == fnlength)
                state = EOBuffer;
        }
    } while (state == More);
    filename[i] = 0; // add a NULL at the end
    //TODO:This should be further processed to ensure legality
    return (i != 0);
}

/// This Hook fires when the program is about to execute the OSSAVE function.
/// It writes the file using C file open and putc calls, and returns a RTS
/// instruction.
/// From ATAP...
/// This subroutine saves all of an area of memory to a specified file.
/// On entry X must point to the following data in zero page:
///   X+0 Address of string of characters, terminated by #0D, which is the file name.
///   X+2 Address for data to be reloaded to.
///   X+4 Execution address if data is to be executed
///   X+6 Start address of data in memory
///   X+8 End address + 1 of data in memory
/// The data is copied by the operating system without being altered.
/// All registers are used. In interrupt or DMA driven operating systems a wait
/// until completion should be performed if the carry flag was set on entry.
/// A break will occur if no storage space large enough can be found.
static int hook_OSSAVE(void       *p_object,
                       Memory     &p_mem,
                       word    p_addr,
                       int        p_byte,
                       AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_OSSAVE");
    int result(-1);
    if (p_byte < 0 && p_at == AT_INSTRUCTION) {
        bool success(false);
        assert (p_object);
        Atom &atom(*((Atom *)p_object));
        char filename[14];
        success = extract_filename(atom, filename, 14);
        if (success) {
            FILE *file = fopen(filename, "wb");
            if (file)
                {
                    success = (putc(atom.m_memory.get_byte(atom.m_6502.m_register.X + 2), file) != EOF) &&
                        (putc(atom.m_memory.get_byte(atom.m_6502.m_register.X + 3), file) != EOF) &&
                        (putc(atom.m_memory.get_byte(atom.m_6502.m_register.X + 4), file) != EOF) &&
                        (putc(atom.m_memory.get_byte(atom.m_6502.m_register.X + 5), file) != EOF);
                    const word start_addr(atom.m_memory.get_word(atom.m_6502.m_register.X + 6));
                    const word end_addr(atom.m_memory.get_word(atom.m_6502.m_register.X + 8));
                    for (int i = start_addr; success && i < end_addr; i++)
                        success = (putc(atom.m_memory.get_byte(i), file) != EOF);
                    success = (fclose(file) != EOF) && success;
                }
        }
        atom.m_6502.m_cycles += 1000000; // Simulate 1.0s
        if (success)
            result = 0x60; // RTS
        else
            result = 0x00; // BRK
    }
    return result;
}

/// This Hook fires when the program is about to execute the OSLOAD function.
/// It reads the file using C file open and getc calls, and returns a RTS
/// instruction.
/// From ATAP...
/// This subroutine loads a complete file into a specified area of memory.
/// On entry X must point to the following data in zero page:
///   X+0 address of string of characters, terminated by #0D, which is the file name.
///   X+2 Address in memory of the first byte of the destination.
///   X+4 Flag byte: if bit 7 = 0 use the file's start address.
/// All processor registers are used.
/// A break will occur if the file cannot be found.
/// In interrupt or DMA driven systems a wait until completion should be
/// performed if the carry flag was set on entry.
static int hook_OSLOAD(void       *p_object,
                       Memory     &p_mem,
                       word    p_addr,
                       int        p_byte,
                       AccessType p_at)
{
    ATOM_CTRACE_LOG("hook_OSLOAD");
    int result(-1);
    if (p_byte < 0 && p_at == AT_INSTRUCTION) {
        bool success(false);
        assert (p_object);
        Atom &atom(*((Atom *)p_object));
        char filename[14];
        success = extract_filename(atom, filename, 14);
        if (success) {
            FILE *file = fopen(filename, "rb");
            success = (file != 0);
            if (success)
                {
                    const int exec_addr_lsb(getc(file));
                    const int exec_addr_msb(getc(file));
                    const int load_addr_lsb(getc(file));
                    const int load_addr_msb(getc(file));
                    success = (exec_addr_lsb != EOF) && (exec_addr_msb != EOF) &&
                        (load_addr_lsb != EOF) && (load_addr_msb != EOF);
                    if (success)
                        {
                            atom.m_memory.put_byte(0xD4, load_addr_lsb);
                            atom.m_memory.put_byte(0xD3, load_addr_msb);
                            atom.m_memory.put_byte(0xD6, exec_addr_lsb);
                            atom.m_memory.put_byte(0xD7, exec_addr_msb);
                            word load_addr;
                            if (atom.m_memory.get_byte(atom.m_6502.m_register.X + 4) & 0x80)
                                load_addr = atom.m_memory.get_word(atom.m_6502.m_register.X + 2);
                            else
                                load_addr = load_addr_msb << 8 | load_addr_lsb;
                            for (int ch(getc(file)); ch != EOF; ch = getc(file))
                                atom.m_memory.put_byte(load_addr++, ch);
                        }
                    success = (fclose(file) != EOF) && success;
                }
        }
        atom.m_6502.m_cycles += 1000000; // Simulate 1.0s
        if (success) {
            result = 0x60; // RTS
            atom.m_memory.put_byte(0xD6, atom.m_memory.get_byte(0xDD) & 0x3F);
        }
        else {
            result = 0x00; // BRK
            atom.m_memory.put_byte(0xD6, atom.m_memory.get_byte(0xDD) | 0x80);
        }
    }
    return result;
}

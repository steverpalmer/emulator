#!/usr/bin/env python3
'''
Tests an atomrc.xml file for errors.
'''

import argparse
import sys
import pathlib
from lxml import etree
import array
import os


__version__ = '0.1'


def check(filename):
    if not isinstance(filename, str):
        raise TypeError

    path = pathlib.Path(filename)

    if not path.exists():
        print("Filename does not exist")
        sys.exit(1)

    if not path.is_file():
        print("Filename is not a file")
        sys.exit(1)

    try:
        inf = path.open()
    except:
        print("Can't open file")
        sys.exit(1)

    if 1:
        RNGparser = etree.XMLParser()
        RNGdoc = etree.parse('atom.rng', RNGparser)
        RNGrules = etree.RelaxNG(RNGdoc)
    else:
        print("Can't validate configuration XML")
        RNGrules = None

    XMLparser = etree.XMLParser()

    try:
        atomrc = etree.parse(inf, XMLparser)
    except:
        print("Can't parse file")
        print("\n".join(XMLparser.error_log))
        sys.exit(1)

    if RNGrules:
        if not RNGrules.validate(atomrc):
            print("Invalid configuration XML")
        
    devices = [None]
    memorymap = array.array('H', [0]*65536)

    for device in atomrc.xpath("/atom/memorymap/*"):
        devices.append(device)
        device_index = devices.index(device)
        device_base = int(device.xpath("base")[0].text, base=16)
        if device.tag == 'rom':
            device_filename = device.xpath("filename")[0].text
            if not os.path.exists(device_filename):
                print("ROM references a missing file:", device_filename)
            elif not os.path.isfile(device_filename):
                print("ROM references an inappropriate file:", device_filename)
            else:
                file_size = os.stat(device_filename).st_size
                device_size = device.xpath("size")
                device_size = file_size if len(device_size) == 0 else int(device_size[0].text, base=16)
                if file_size < device_size:
                    print("ROM references too small a file:", device_filename)
        else:
            device_size = int(device.xpath("size")[0].text, base=16)
        if device_base + device_size > 65536:
            print("Out of range Device address")
            device_size = 65536 - device_base
        else:
            for addr in range(device_base, device_base+device_size):
                if memorymap[addr]:
                    print("Overlapping Device address:", hex(addr))
                else:
                    memorymap[addr] = device_index

    # Check out the description of Block0 RAM

    for addr in range(0, 0x0400):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != 'ram':
            print("Missing RAM in block 0 space at address:", hex(addr))
    
    # Check out the description of the Video RAM

    for addr in range(0x8000, 0x8400):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != 'ram':
            print("Missing RAM in video space at address:", hex(addr))
    
    # Check out the description of the PPIA

    for addr in range(0xB000, 0xB004):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != 'ppia':
            print("Missing PPIA at address:", hex(addr))
    
    # Check out the description of the Kernel

    for addr in range(0xF000, 0x10000):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != 'rom':
            print("Missing ROM in kernel space at address:", hex(addr))

    # Check out the description of the Interpreter
            
    for addr in range(0xC000, 0xD000):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != 'rom':
            print("Missing ROM in Interpret space at address:", hex(addr))

def main():
    parser = argparse.ArgumentParser(description="Check the content of an atom XML configuration file")
    parser.add_argument('filename', help="name of file to check")
    parser.add_argument('-v', '--version', action='version', version='%(prog)s v' + __version__)
    cli_args = parser.parse_args()
    check(cli_args.filename)

if __name__ == "__main__":
    main()

__all__ = ['check']

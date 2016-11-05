#!/usr/bin/env python3
# check_atom_xml.py
# Copyright 2016 Steve Palmer
'''
Tests an atomrc.xml file for errors.
'''

import logging.config
import argparse
import sys
import pathlib
from lxml import etree
import array
import os


__version__ = '0.1'

logging.config.fileConfig(r'logging.conf')
LOG = logging.getLogger() if __name__ == '__main__' else logging.getLogger(__name__)

ATOM_NS = r'http://www.srpalmer.me.uk/ns/atom'

def tag_name(t):
    return '{' + ATOM_NS + '}' + t

def check(filename):
    LOG.debug("... argument checking")
    if not isinstance(filename, str):
        LOG.critcal("Invalid filename: %s", repr(filename))
        raise TypeError

    path = pathlib.Path(filename)

    LOG.info("Filename Checking")
    if not path.exists():
        LOG.critical("Filename %s does not exist", repr(filename))
        return False

    if not path.is_file():
        LOG.critical("Filename %s is not a file", repr(filename))
        sys.exit(1)

    try:
        inf = path.open()
    except:
        LOG.critical("Can't open file %s", repr(filename))
        return False

    LOG.debug("... reading RelaxNG schema")
    RNGparser = etree.XMLParser()
    try:
        RNG_DOM = etree.parse('atom.rng', RNGparser)
    except:
        LOG.debug("\n".join(RNGparser.error_log))
        RNG_DOM = None

    if RNG_DOM:
        try:
            RNGschema = etree.RelaxNG(RNG_DOM)
        except etree.RelaxNGParseError as exc:
            LOG.debug(repr(exc))
            RNGschema = None

    if not RNGschema:
        LOG.warning("Can't validate file %s", repr(filename))

    LOG.debug("... reading DOM file")
    XMLparser = etree.XMLParser()
    try:
        atomrc = etree.parse(inf, XMLparser)
    except:
        LOG.crtical("Can't parse file %s:\n%s", repr(filename), "\n".join(XMLparser.error_log))
        return False

    if RNGschema:
        LOG.info("Validating file %s against RelaxNG schema", repr(filename))
        if not RNGschema.validate(atomrc):
            LOG.error("Invalid configuration XML in file %s", repr(filename))

    LOG.debug("... building memory model")
    devices = [None]
    memorymap = array.array('H', [0]*65536)
    namespaces = {'a': ATOM_NS}
    ram_tag = tag_name('ram')
    rom_tag = tag_name('rom')
    ppia_tag = tag_name('ppia')

    for device in atomrc.xpath("/a:atom/a:memorymap/*", namespaces=namespaces):
        LOG.debug("... processing: %s" % etree.tostring(device))
        devices.append(device)
        device_index = devices.index(device)
        if device.tag in (ram_tag, ppia_tag):
            device_base = int(device.xpath("a:base", namespaces=namespaces)[0].text, base=16)
            device_size = int(device.xpath("a:size", namespaces=namespaces)[0].text, base=16)
        elif device.tag == rom_tag:
            device_base = int(device.xpath("a:base", namespaces=namespaces)[0].text, base=16)
            device_filename = device.xpath("a:filename", namespaces=namespaces)[0].text
            if not os.path.exists(device_filename):
                LOG.error("Device base %04X: ROM references a missing file: %s", device_base, repr(device_filename))
            elif not os.path.isfile(device_filename):
                LOG.error("Device base %04X: ROM references an inappropriate file: %s", device_base, repr(device_filename))
            else:
                file_size = os.stat(device_filename).st_size
                device_size = device.xpath("a:size", namespaces=namespaces)
                device_size = file_size if len(device_size) == 0 else int(device_size[0].text, base=16)
                if file_size < device_size:
                    LOG.error("Device base %04X: ROM references too small a file: %s", device_base, repr(device_filename))
        else:
            LOG.warning("Unknown type of memory mapped device: %s", repr(device.tag))
        if device_base + device_size > 65536:
            LOG.error("Device base %04X: Overflows memory space", device_base)
            device_size = 65536 - device_base
        else:
            LOG.debug("... registering device base at %04X", device_base)
            for addr in range(device_base, device_base+device_size):
                if memorymap[addr]:
                    LOG.error("Overlapping Devices at address %04X", addr)
                    break
                else:
                    memorymap[addr] = device_index

    LOG.info("Checking Block0 RAM")
    for addr in range(0, 0x0400):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != ram_tag:
            LOG.error("Missing RAM in block 0 space at address %04X", addr)
            break

    LOG.info("Checking Video RAM")
    for addr in range(0x8000, 0x8400):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != ram_tag:
            LOG.error("Missing RAM in video space at address %04X", addr)
            break

    LOG.info("Checking PPIA")
    for addr in range(0xB000, 0xB004):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != ppia_tag:
            LOG.error("Missing PPIA at address %04X", addr)
            break

    LOG.info("Checking Kernel ROM")
    for addr in range(0xF000, 0x10000):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != rom_tag:
            LOG.error("Missing kernel ROM at address %04X", addr)
            break

    LOG.info("Checking Interpreter ROM")
    for addr in range(0xC000, 0xD000):
        if memorymap[addr] == 0 or devices[memorymap[addr]].tag != rom_tag:
            LOG.error("Missing Interpreter ROM at address %04X", addr)
            break

    LOG.info("Checking Emulator Paramters")
    # The semantics for the emulator parameters are "overwrite"
    # That is, each parameter will default to something that is OK
    # but may be overwritten in the XML file.

    LOG.debug("... scale")
    scale = atomrc.xpath("/a:atom/a:io/a:scale", namespaces=namespaces)
    if (scale):
        if len(scale) != 1:
            LOG.error("scale parameter should only occur once")
        else:
            scale = float(scale[0].text)
            if scale < 1.0:
                LOG.error("scale must be at least 1.0 or larger")
            elif scale > 4.0:
                LOG.warning("scale value unexpectedlu large: %f", scale)

    LOG.debug("... fontfilename")
    fontfilename = atomrc.xpath("/a:atom/a:io/a:fontfilename", namespaces=namespaces)
    if (fontfilename):
        if len(fontfilename) != 1:
            LOG.error("fontfilename parameter should only occur once")
        else:
            fontfilename = pathlib.Path(fontfilename[0].text)
            if not fontfilename.exists():
                LOG.error("fontfilename %s does not exist", fontfilename)
            elif not fontfilename.is_file():
                LOG.error("fontfilename %s is not a file", fontfilename)
            elif not fontfilename.suffix == '.bmp':
                LOG.error("fontfilename %s does not end '.bmp'", fontfilename)

    LOG.debug("... refreshrate")
    refreshrate = atomrc.xpath("/a:atom/a:io/a:refreshrate", namespaces=namespaces)
    if (refreshrate):
        if len(refreshrate) != 1:
            LOG.error("refreshrate parameter should only occur once")
        else:
            refreshrate = float(refreshrate[0].text)
            if refreshrate > 100:
                LOG.error("refreshrate above 100Hz is excessive")
            elif refreshrate < 2:
                LOG.warning("refreshrate below 2Hz is a bit slow")

    return True

def main():
    parser = argparse.ArgumentParser(description="Check the content of an atom XML configuration file")
    parser.add_argument('filename', help="name of file to check")
    parser.add_argument('-v', '--version', action='version', version='%(prog)s v' + __version__)
    cli_args = parser.parse_args()
    check(cli_args.filename)

if __name__ == "__main__":
    main()

__all__ = ['check']

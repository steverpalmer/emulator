#!/usr/bin/env python3
# build_plot_bmp.py
# Copyright 2016 Steve Palmer
__version__ = "0.1"

import argparse

PARSER = argparse.ArgumentParser(description="build the plot image file(s) used by the emulator")
PARSER.add_argument('target', help="name of file to generate")
PARSER.add_argument('-v', '--version', action='version', version='%(prog)s ' + str(__version__))
ARGS = PARSER.parse_args()

from PIL import Image

PLOT_IMG = Image.new("1", (8,256))
for y in range(256):
    for x in range(8):
        if y & (128>>x):
#            PLOT_IMG.putpixel((7-x,y),1)
            PLOT_IMG.putpixel((x,y),1)

PLOT_IMG.save(ARGS.target)

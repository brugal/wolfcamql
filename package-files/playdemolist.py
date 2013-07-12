#!/usr/bin/env python
# www.python.org
############################
#
# playdemolist.py
#    play demos from wolfcam-ql/playdemos.txt
#
############################

from os import system

DEMO_LIST_FILE = "playdemos.txt"

def main():
    f = open(DEMO_LIST_FILE)
    lines = f.readlines()
    for fname in lines:
        fname = fname[:-1]  # strip newline
        system("wolfcamql.exe +demo \"%s\" +set quitdemo quit +set nextdemo vstr quitdemo" % fname)

if __name__ == "__main__":
    main()

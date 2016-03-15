#!/usr/bin/env python
# www.python.org  (Python 2 or 3)
############################
#
# playdemolist.py:  play demos from wolfcam-ql/playdemos.txt
#
# wolfcamql executable must be in your executable PATH list.
#
# This script must be run from the same directory as the wolfcamql executable
# (wolfcamql.exe, wolfcamql.i386, wolfcamqlmac) since it uses 'wolfcam-ql' as
# a relative directory.
#
# wolfcamql executable must also be in your executable PATH list.
#
# playdemos.txt format:  one demo file per line.  Ex:
#
# test-demo1.dm_91
# ctf-flag-run13.dm_91
# etc..
#
############################

from os import system

DEMO_LIST_FILE = "wolfcam-ql/playdemos.txt"

def main():
    f = open(DEMO_LIST_FILE)
    lines = f.readlines()
    for fname in lines:
        fname = fname.rstrip()

        # This makes use of quake3's 'nextdemo' cvar.  This isn't the name of
        # the next demo to play it is the command to execute after a demo has
        # completed.
        system("wolfcamql.exe +demo \"%s\" +set quitdemo quit +set nextdemo \"vstr quitdemo\"" % fname)

if __name__ == "__main__":
    main()

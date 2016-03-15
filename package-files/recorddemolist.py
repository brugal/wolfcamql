#!/usr/bin/env python
# www.python.org  (Python 2 or 3)
############################
#
# recorddemolist.py:  record portion of demos from wolfcam-ql/recorddemos.txt
#
# In wolfcam-ql/cgamepostinit.cfg add the following line to the end of the
# file:
#    exec commandsRecord
#
# This script must be run from the same directory as the wolfcamql executable
# (wolfcamql.exe, wolfcamql.i386, wolfcamqlmac) since it uses 'wolfcam-ql' as
# a relative directory.
#
# wolfcamql executable must also be in your executable PATH list.
#
# recorddemos.txt format:  one line each with start time, end time, and demo
# name.  Ex:
#
# 7:55 8:31 demo1.dm_91
# 10:02 12:16 demo2.dm_91
# etc..
#
############################

from os import system, remove
from os.path import basename, splitext

DEMO_LIST_FILE = "wolfcam-ql/recorddemos.txt"

def main():
    f = open(DEMO_LIST_FILE)
    lines = f.readlines()
    for line in lines:
        line = line.rstrip()
        words = line.split()
        startTime = words[0]
        endTime = words[1]
        fname = " ".join(words[2:])
        name = splitext(basename(fname))[0]

        commandsFile = "wolfcam-ql/commandsRecord.cfg"
        f = open(commandsFile, "w")
        f.write("seekclock %s;\n" % startTime)
        f.write("video name %s;\n" % name)
        f.write("at %s quit;\n" % endTime)
        f.close()

        system("wolfcamql.exe +demo %s" % name)
        remove(commandsFile)


if __name__ == "__main__":
    main()

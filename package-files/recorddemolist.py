#!/usr/bin/env python
# www.python.org
############################
#
# recorddemolist.py
#    record portion of demos from wolfcam-ql/recorddemos.txt
#
# in wolfcam-ql/cgamepostinit.cfg add the following line to the end of the
# file:
#    exec commandsRecord
#
############################

from os import system
from os.path import basename, splitext
from string import join

DEMO_LIST_FILE = "recorddemos.txt"

def main():
    f = open(DEMO_LIST_FILE)
    lines = f.readlines()
    for line in lines:
        line = line[:-1]  # strip newline
        words = line.split()
        startTime = words[0]
        endTime = words[1]
        fname = join(words[2:])
        name = splitext(basename(fname))[0]

        f = open("commandsRecord.cfg", "w")
        f.write("seekclock %s;\n" % startTime)
        f.write("video name %s;\n" % name)
        f.write("at %s quit;\n" % endTime)
        f.close()

        system("wolfcamql.exe +demo %s" % name)


if __name__ == "__main__":
    main()

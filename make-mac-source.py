#!/usr/bin/env python
import os, sys

shareDir = "/mac-share"
destDir = os.path.join(shareDir, "wolfcam-build")
buildLocal = False

if len(sys.argv) > 1  and  sys.argv[1] == "local":
    destDir = "build"
    buildLocal = True

if os.path.ismount(shareDir) == False  and  buildLocal != True:
    print "mac share isn't mounted"
    sys.exit(1)

os.system("tar --exclude='misc/msvc' --exclude='misc/msvc10' --exclude='misc/nsis' --exclude='code/libs/backtrace.dll' --exclude='code/libs/SDL.dll' --exclude='code/libs/win32' --exclude='code/libs/win64'  -zcvf %s/macsrc.tar.gz Makefile* *.sh *.py code/ ui/ misc/" % destDir)

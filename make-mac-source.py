#!/usr/bin/env python
import os, sys

shareDir = "/mac-old-share"
destDir = os.path.join(shareDir, "wolfcam-build")
buildLocal = False

if len(sys.argv) > 1  and  sys.argv[1] == "local":
    destDir = "build"
    buildLocal = True

if os.path.ismount(shareDir) == False  and  buildLocal != True:
    print "mac share isn't mounted"
    sys.exit(1)

os.system("tar -zcvf %s/macsrc.tar.gz Makefile* *.sh *.py code/ ui/ misc/" % destDir)

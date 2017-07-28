#!/usr/bin/env python
import os, shutil

mainDir = os.getcwd()

packageDir = "/share/wc/dev";

if not os.path.exists(packageDir):
    os.makedirs(packageDir)

os.system("cp -a package-files/* %s/" % packageDir)
os.system("cp COPYING.txt COPYING-backtrace.txt CREDITS-wolfcam.txt CREDITS-openarena.txt README-ioquake3.txt README-wolfcam.txt version.txt %s/" % packageDir)
os.system("cp code/libs/SDL.dll code/libs/backtrace.dll %s/" % packageDir)
os.system("cp build/release-mingw32-x86/ioquake3.x86.exe %s/wolfcamql.exe" % packageDir)
os.system("cp build/release-mingw32-x86/renderer_opengl1_x86.dll %s/" % packageDir)
os.system("cp build/release-mingw32-x86/baseq3/*.dll %s/wolfcam-ql/" % packageDir)
os.system("cp ui/wcmenudef.h %s/wolfcam-ql/ui/" % packageDir)


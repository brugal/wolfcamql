#!/usr/bin/env python
import os, shutil

mainDir = os.getcwd()

packageDir = "/share/wc/dev";

if not os.path.exists(packageDir):
    os.makedirs(packageDir)

# cp '-u' option not reliable in shared virtualbox directory
os.system("cp -a package-files/* %s/" % packageDir)
os.system("cp -a COPYING.txt COPYING-backtrace.txt CREDITS-wolfcam.txt CREDITS-openarena.txt README-ioquake3.txt README-wolfcam.txt opengl2-readme.md version.txt %s/" % packageDir)
os.system("cp -a code/libs/win32/SDL2.dll code/libs/win32/backtrace.dll %s/" % packageDir)
os.system("cp -a build/release-mingw32-x86/ioquake3.x86.exe %s/wolfcamql.exe" % packageDir)
os.system("cp -a build/release-mingw32-x86/renderer_opengl1_x86.dll %s/" % packageDir)
os.system("cp -a build/release-mingw32-x86/renderer_opengl2_x86.dll %s/" % packageDir)
os.system("cp -a build/release-mingw32-x86/baseq3/*.dll %s/wolfcam-ql/" % packageDir)
os.system("cp -a ui/wcmenudef.h %s/wolfcam-ql/ui/" % packageDir)


#!/usr/bin/env python
import os, shutil

mainDir = os.getcwd()
packageDir = "/share/wc/dev";

if not os.path.exists(packageDir):
    os.makedirs(packageDir)

# shutil.copytree() fails if destination dir exists
def copytree (src, dest):
    if not os.path.exists(dest):
        os.makedirs(dest)

    for f in os.listdir(src):
        s = os.path.join(src, f)
        d = os.path.join(dest, f)
        if os.path.isdir(s):
            copytree(s, d)
        else:
            shutil.copy2(s, d)

packageFilesDir = "package-files"

files = os.listdir(packageFilesDir)
for f in files:
    fpath = os.path.join(packageFilesDir, f)
    print "copying package file: " + fpath
    if os.path.isdir(fpath):
        #shutil.copytree(fpath, os.path.join(packageDir, f))
        copytree(fpath, os.path.join(packageDir, f))
    else:
        shutil.copy2(fpath, packageDir)

baseFiles = ["COPYING.txt", "COPYING-backtrace.txt", "CREDITS-wolfcam.txt", "CREDITS-openarena.txt", "README-ioquake3.txt", "README-wolfcam.txt", "opengl2-readme.md", "version.txt", "unifont-LICENSE.txt", "voip-readme.txt"]

for f in baseFiles:
    print "copying base file: " + f
    shutil.copy(f, packageDir)

# 64-bit

libDir = os.path.join("code", "libs", "win64")
buildDir = os.path.join("build", "release-mingw32-x86_64")
wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

print "copying 64-bit Windows binaries..."
try:
    shutil.copy2(os.path.join(libDir, "SDL264.dll"), packageDir)
    shutil.copy2(os.path.join(libDir, "backtrace64.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "ioquake3.x86_64.exe"), packageDir)
    shutil.move(os.path.join(packageDir, "ioquake3.x86_64.exe"), os.path.join(packageDir, "wolfcamql.exe"))
    shutil.copy2(os.path.join(buildDir, "renderer_opengl1_x86_64.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "renderer_opengl2_x86_64.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "cgamex86_64.dll"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "qagamex86_64.dll"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "uix86_64.dll"), wolfcamDir)
except IOError as err:
    print err

# 32-bit

libDir = os.path.join("code", "libs", "win32")
buildDir = os.path.join("build", "release-mingw32-x86")
wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

print "copying 32-bit Windows binaries..."
try:
    shutil.copy2(os.path.join(libDir, "SDL2.dll"), packageDir)
    shutil.copy2(os.path.join(libDir, "backtrace.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "ioquake3.x86.exe"), packageDir)
    shutil.move(os.path.join(packageDir, "ioquake3.x86.exe"), os.path.join(packageDir, "wolfcamql32.exe"))
    shutil.copy2(os.path.join(buildDir, "renderer_opengl1_x86.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "renderer_opengl2_x86.dll"), packageDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "cgamex86.dll"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "qagamex86.dll"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "uix86.dll"), wolfcamDir)
except IOError as err:
    print err

print "copying misc files..."
shutil.copy2(os.path.join("ui", "wcmenudef.h"), os.path.join(packageDir, "wolfcam-ql", "ui"))

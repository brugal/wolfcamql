#!/usr/bin/env python
import os, shutil, sys

build64 = True

for arg in sys.argv[1:]:
    if arg == "32":
        build64 = False

mainDir = os.getcwd()

if build64:
    packageDir = "/share/wc/dev"
else:
    packageDir = "/share/wc/dev32"

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
    print("copying package file: " + fpath)
    if os.path.isdir(fpath):
        #shutil.copytree(fpath, os.path.join(packageDir, f))
        copytree(fpath, os.path.join(packageDir, f))
    else:
        shutil.copy2(fpath, packageDir)

baseFiles = ["COPYING.txt", "COPYING-backtrace.txt", "CREDITS-wolfcam.txt", "CREDITS-openarena.txt", "README-ioquake3.txt", "README-wolfcam.txt", "version.txt"]

for f in baseFiles:
    print("copying base file: " + f)
    shutil.copy(f, packageDir)

print("copying docs")
copytree("docs", os.path.join(packageDir, "docs"))

if build64:  # 64-bit
    libDir = os.path.join("code", "thirdparty", "libs", "win64")
    buildDir = os.path.join("build", "release-mingw32-x86_64")
    wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

    if os.path.exists(os.path.join(buildDir, "ioquake3.exe")):
        print("copying 64-bit Windows binaries...")
        try:
            shutil.copy2(os.path.join(libDir, "SDL2.dll"), packageDir)
            shutil.copy2(os.path.join(libDir, "backtrace.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "ioquake3.exe"), packageDir)
            shutil.move(os.path.join(packageDir, "ioquake3.exe"), os.path.join(packageDir, "wolfcamql.exe"))
            shutil.copy2(os.path.join(buildDir, "renderer_opengl1.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "renderer_opengl2.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "cgame.dll"), wolfcamDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "qagame.dll"), wolfcamDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "ui.dll"), wolfcamDir)
        except IOError as err:
            print(err)
    else:
        print("skipping 64-bit Windows binaries")

else:  # 32-bit
    libDir = os.path.join("code", "thirdparty", "libs", "win32")
    buildDir = os.path.join("build", "release-mingw32-x86")
    wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

    if os.path.exists(os.path.join(buildDir, "ioquake3.exe")):
        print("copying 32-bit Windows binaries...")
        try:
            shutil.copy2(os.path.join(libDir, "SDL2.dll"), packageDir)
            shutil.copy2(os.path.join(libDir, "backtrace.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "ioquake3.exe"), packageDir)
            shutil.move(os.path.join(packageDir, "ioquake3.exe"), os.path.join(packageDir, "wolfcamql.exe"))
            shutil.copy2(os.path.join(buildDir, "renderer_opengl1.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "renderer_opengl2.dll"), packageDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "cgame.dll"), wolfcamDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "qagame.dll"), wolfcamDir)
            shutil.copy2(os.path.join(buildDir, "baseq3", "ui.dll"), wolfcamDir)
        except IOError as err:
            print(err)
    else:
        print("skipping 32-bit Windows binaries")

print("copying misc files...")
shutil.copy2(os.path.join("ui", "wcmenudef.h"), os.path.join(packageDir, "wolfcam-ql", "ui"))

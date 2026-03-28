#!/usr/bin/env python
import glob, logging, os, shutil, sys, tarfile

mainDir = os.getcwd()

VERSIONSTRING = ""

f = open("Makefile.local", "r")
while 1:
    line = f.readline()
    if not line:
        break;

    # VERSION=...
    if line.startswith("VERSION"):
        VERSIONSTRING = line.split('=')[1].strip()
        break;

f.close()

if VERSIONSTRING == "":
    print("couldn't get VERSIONSTRING")
    sys.exit(1)

packageDir = os.path.join("package-release", "wolfcamql%s" % VERSIONSTRING)

if os.path.exists(packageDir):
    shutil.rmtree(packageDir)


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
        shutil.copytree(fpath, os.path.join(packageDir, f))
    else:
        shutil.copy2(fpath, packageDir)

baseFiles = ["COPYING.txt", "COPYING-backtrace.txt", "CREDITS-wolfcam.txt", "CREDITS-openarena.txt", "README-ioquake3.txt", "README-wolfcam.txt", "version.txt"]

for f in baseFiles:
    print("copying base file: " + f)
    shutil.copy(f, packageDir)

print("copying docs")
copytree("docs", os.path.join(packageDir, "docs"))

libDir = os.path.join("code", "thirdparty", "libs", "win64")
buildDir = os.path.join("build", "release-mingw32-x86_64")
wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

print("copying Windows binaries...")
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

buildDir = os.path.join("build", "release-linux-x86_64")
wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

print("copying Linux binaries...")
try:
    shutil.copy2(os.path.join(buildDir, "ioquake3"), packageDir)
    shutil.move(os.path.join(packageDir, "ioquake3"), os.path.join(packageDir, "wolfcamql.bin"))
    shutil.copy2(os.path.join(buildDir, "renderer_opengl1.so"), packageDir)
    shutil.copy2(os.path.join(buildDir, "renderer_opengl2.so"), packageDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "cgame.so"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "qagame.so"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "baseq3", "ui.so"), wolfcamDir)
except IOError as err:
    print(err)

libDir = os.path.join("code", "thirdparty", "libs", "macosx")
buildDir = "mac-binaries"
wolfcamDir = os.path.join(packageDir, "wolfcam-ql")

print("copying Mac OS X binaries...")
try:
    shutil.copy2(os.path.join(buildDir, "wolfcamqlmac"), packageDir)
    shutil.copy2(os.path.join(buildDir, "renderer_opengl1.dylib"), packageDir)
    shutil.copy2(os.path.join(buildDir, "renderer_opengl2.dylib"), packageDir)
    shutil.copy2(os.path.join(buildDir, "cgame.dylib"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "qagame.dylib"), wolfcamDir)
    shutil.copy2(os.path.join(buildDir, "ui.dylib"), wolfcamDir)
    # last so that it's not copied if the above ones don't exist
    shutil.copy2(os.path.join(libDir, "libSDL2-2.0.0.dylib"), packageDir)
except IOError as err:
    print(err)

print("copying misc files...")
shutil.copy2(os.path.join("ui", "wcmenudef.h"), os.path.join(packageDir, "wolfcam-ql", "ui"))


print("building source...")
ignoreFiles = [ ".git", ".hg", ".hgignore", "build", "package-files", "package-release", os.path.join("code", "thirdparty", "libs"), "macwolfcambuild", "update-mac-binaries.sh", os.path.join("backtrace", "build"), "mac-binaries" ]

for f in glob.glob(os.path.join("backtrace", "binutils*gz")):
    ignoreFiles.append(f)

tar = tarfile.open(os.path.join(packageDir, "wolfcamql-src.tar.bz2"), "w:bz2")

def tarFilter(tarinfo):
    #print("tar file: " + tarinfo.name)
    if tarinfo.name in ignoreFiles:
        print("skipping " + tarinfo.name)
        return None
    if os.path.isdir(tarinfo.name):
        print("adding dir: " + tarinfo.name)
    return tarinfo

topFiles = os.listdir(".")
for f in topFiles:
    tar.add(f, filter=tarFilter)
tar.close()

print("creating package zip...")
log = logging.getLogger("make-package")
osh = logging.StreamHandler(sys.stdout)
osh.setFormatter(logging.Formatter("zip: %(message)s"))
osh.setLevel(logging.INFO)
log.addHandler(osh)
log.setLevel(logging.INFO)

shutil.make_archive(os.path.join("package-release", "wolfcamql-%s" % VERSIONSTRING), "zip", "package-release", "wolfcamql%s" % VERSIONSTRING, logger=log )

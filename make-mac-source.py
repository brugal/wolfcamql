#!/usr/bin/env python
import os, sys, tarfile

shareDir = "/mac-share"
destDir = os.path.join(shareDir, "wolfcam-build")
buildLocal = False

if len(sys.argv) > 1  and  sys.argv[1] == "local":
    destDir = "build"
    buildLocal = True

if os.path.ismount(shareDir) == False  and  buildLocal != True:
    print("mac share isn't mounted")
    sys.exit(1)

ignoreFiles = [ ".hg", ".hgignore", "backtrace", "build", "package-files",
                "package-release", os.path.join("code", "libs", "win32"),
                os.path.join("code", "libs", "win64"), "mac-binaries",
                os.path.join("misc", "msvc"), os.path.join("misc", "msvc10"),
                os.path.join("misc", "msvc11"),
                os.path.join("misc", "msvc12"),
                os.path.join("misc", "msvc142"),
                os.path.join("misc", "nsis")
]

tar = tarfile.open(os.path.join(destDir, "macsrc.tar.gz"), "w:gz")

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

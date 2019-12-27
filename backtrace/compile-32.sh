#!/bin/sh

BINUTILS_DIR=`pwd`/build/32/binutils-2.33.1

i686-w64-mingw32-windres -i resource.rc -o build/resource32.o

i686-w64-mingw32-gcc -O2 -static-libgcc -shared -Wall -o build/backtrace.dll backtrace.c build/resource32.o -I"$BINUTILS_DIR/bfd/" -I"$BINUTILS_DIR/include" -L"$BINUTILS_DIR/bfd" -L"$BINUTILS_DIR/libiberty" -L"$BINUTILS_DIR/zlib" -L"$BINUTILS_DIR/intl" -limagehlp -Wl,-Bstatic -lbfd -liberty -lz -lintl

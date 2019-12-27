#!/bin/sh

BINUTILS_DIR=`pwd`/build/64/binutils-2.33.1

x86_64-w64-mingw32-windres -i resource.rc -o build/resource64.o

x86_64-w64-mingw32-gcc -O2 -static-libgcc -shared -Wall -o build/backtrace64.dll backtrace.c build/resource64.o -I"$BINUTILS_DIR/bfd/" -I"$BINUTILS_DIR/include" -L"$BINUTILS_DIR/bfd" -L"$BINUTILS_DIR/libiberty" -L"$BINUTILS_DIR/zlib" -L"$BINUTILS_DIR/intl" -limagehlp -Wl,-Bstatic -lbfd -liberty -lz -lintl

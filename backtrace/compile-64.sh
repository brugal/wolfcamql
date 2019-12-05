#!/bin/sh

DIR=`pwd`/build/64/binutils-2.33.1

x86_64-w64-mingw32-gcc -O2 -static-libgcc -shared -Wall -o build/backtrace64.dll backtrace.c -I"$DIR/bfd/" -I"$DIR/include" -L"$DIR/bfd" -L"$DIR/libiberty" -L"$DIR/zlib" -L"$DIR/intl" -Wl,-Bstatic -lbfd -liberty -limagehlp -lz -lintl

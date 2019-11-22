#!/bin/sh

DIR=`pwd`/build/32/binutils-2.33.1

i686-w64-mingw32-gcc -O2 -static-libgcc -shared -Wall -o build/backtrace.dll backtrace.c -I$DIR/bfd/ -I$DIR/include -L$DIR/bfd -L$DIR/libiberty -L$DIR/intl -Wl,-Bstatic -lbfd -liberty -limagehlp -lz -lintl

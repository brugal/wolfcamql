
-- binutils --

# binutils-*.tar.gz

# mkdir build/32
# cd build/32

mkdir build/64
cd build/64

tar -zxvf ../../binutils-*.tar.gz
cd binutils*
# add --target ...  see: https://www.reactos.org/wiki/Building_MINGW-w64

# ./configure --host=i686-w64-mingw32 --target=i686-w64-mingw32

./configure --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32

make

-- backtrace.dll

edit backtrace.c and add <config.h> before <bfd.h>

# ./compile-32.sh

./compile-64.sh

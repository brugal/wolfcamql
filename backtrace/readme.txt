apt-get install libz-mingw-w64-dev

-- binutils --

cd ~/bin
ln -s /usr/bin/i686-w64-mingw32-ar mingw32-ar
export `pwd`:$PATH
cd -

# binutils-2.25.tar.gz
mkdir build/32
cd build/32

tar -zxvf ../../binutils-*.tar.gz
cd binutils*
CC=i686-w64-mingw32-gcc ./configure --host=mingw32
make

-- backtrace.dll

edit backtrace.c and add <config.h> before <bfd.h>

./build-32.sh

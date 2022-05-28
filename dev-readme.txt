#compile zlib win32 from linux

----

compile external ogg and vorbis mac os x:

  2014-09-11
    ogg and vorbis  ./configure CFLAGS="-arch i386"  --host=i386

compile freetype mac os x:

  # zlib has version information, try to use the same one

  CPPFLAGS="-I/Users/acano/share/zlib-1.2.12" CFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.7" ./configure --with-zlib=yes --with-bzip2=no --with-png=no --with-brotli=no
  make
  cp objs/.libs/libfreetype.a ...

----

compile freetype win32 (from linux):

  # zlib has version information, try to use the same one

  CPPFLAGS=-I/home/acano/work/hg/ioquakelive-demo-player/code/zlib-1.2.12 ./configure --host=i686-w64-mingw32 --with-zlib=yes --with-bzip2=no --with-png=no --with-brotli=no
  make
  cp objs/.libs/libfreetype.a ...

  for 64 bit:  --host=x86_64-w64-mingw32

----

6014:cc46e2efef98 ioquake3 patch:  2014-03-08-02 Update SDL2 to 2.0.2

Update to SDL2 2.0.2 crashes with i686-w64-mingw32-gcc 4.9.1 and '-O3'.  This is with Debian 8 oldstable (jessie).  Compiling with '-O2' or less avoids crash.  This also appears to be fixed with i686-w64-mingw32-gcc 6.3.0 2017516.  That's with Debian 9 stable (stretch).

----

convert -background none wolfcamql.svg -define icon:auto-resize=256,128,64,48,32,16 -border 0 quake3.ico

----

2020-11-27 updating to speex-1.2.0 from speex-1.2beta3 takes out needed structure: SpeexPreprocessState -- 2022-04-19 dsp functions were put in a separate library: speexdsp-1.2rc3.tar.gz
2022-04-21 for libspeex copy win32/config.h into main source directory -- not needed

for both libspeex and libspeexdsp:
  include/speex/speexdsp_config_types.h add '#ifdef _MSC_VER' wrapper

libspeexdsp-1.2rc3/fftwrap.c add '#define USE_KISS_FFT'

----

compile curl win32 (from linux):

# libcurl.a linked with -lcrypt32

CPPFLAGS=-I/home/acano/work/hg/ioquakelive-demo-player/code/zlib-1.2.12 ./configure --host=i686-w64-mingw32 --with-schannel

...

checking for sys/types.h... (cached) yes
checking for poll.h... (cached) no
checking for sys/poll.h... (cached) no
checking for in_addr_t... no
checking for in_addr_t equivalent... unknown
configure: error: Cannot find a type to use in place of in_addr_t


# 2022-05-25 current ioq3 version has progress meter enabled

compile curl win32 (from Windows mysys2)

pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-i686-toolchain
pacman -S mingw-w64-cross-binutils

open terminal with mingw32.exe / mingw64.exe

CPPFLAGS="-I/home/acano/zlib-1.2.12" ./configure --with-schannel --disable-shared --disable-ldap --disable-pthreads --without-zstd --enable-progress-meter

make



compile external ogg and vorbis mac os x:

  2014-09-11
    ogg and vorbis  ./configure CFLAGS="-arch i386"  --host=i386

compile freetype mac os x:

  ./configure --with-zlib=no --with-bzip2=no --with-png=no
  make
  cp objs/.libs/libfreetype.a ...

----

compile freetype win32 (from linux):

  ./configure --host=i686-w64-mingw32 --with-zlib=no --with-bzip2=no --with-png=no
  make
  cp objs/.libs/libfreetype.a ...

  for 64 bit:  --host=x86_64-w64-mingw32

----

6014:cc46e2efef98 ioquake3 patch:  2014-03-08-02 Update SDL2 to 2.0.2

Update to SDL2 2.0.2 crashes with i686-w64-mingw32-gcc 4.9.1 and '-O3'.  This is with Debian 8 oldstable (jessie).  Compiling with '-O2' or less avoids crash.  This also appears to be fixed with i686-w64-mingw32-gcc 6.3.0 2017516.  That's with Debian 9 stable (stretch).

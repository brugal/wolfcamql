
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


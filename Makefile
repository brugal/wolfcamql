#
# ioq3 Makefile
#
# GNU Make required
#
COMPILE_PLATFORM=$(shell uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e 's/i.86/x86/' | sed -e 's/^arm.*/arm/')

#arm64 hack!
ifeq ($(shell uname -m), arm64)
  COMPILE_ARCH=arm64
endif
ifeq ($(shell uname -m), aarch64)
  COMPILE_ARCH=arm64
endif

ifeq ($(COMPILE_PLATFORM),sunos)
  # Solaris uname and GNU uname differ
  COMPILE_ARCH=$(shell uname -p | sed -e 's/i.86/x86/')
endif

ifndef BUILD_STANDALONE
  BUILD_STANDALONE =
endif
ifndef BUILD_CLIENT
  BUILD_CLIENT     =
endif
ifndef BUILD_SERVER
  BUILD_SERVER     =
endif
ifndef BUILD_GAME_SO
  BUILD_GAME_SO    =
endif
ifndef BUILD_GAME_QVM
  BUILD_GAME_QVM   =
endif
ifndef BUILD_BASEGAME
  BUILD_BASEGAME =
endif
ifndef BUILD_MISSIONPACK
  BUILD_MISSIONPACK=
endif
ifndef BUILD_RENDERER_OPENGL1
  BUILD_RENDERER_OPENGL1=
endif
ifndef BUILD_RENDERER_OPENGL2
  BUILD_RENDERER_OPENGL2=
endif
ifndef BUILD_AUTOUPDATER  # DON'T build unless you mean to!
  BUILD_AUTOUPDATER=0
endif

#############################################################################
#
# If you require a different configuration from the defaults below, create a
# new file named "Makefile.local" in the same directory as this file and define
# your parameters there. This allows you to change configuration without
# causing problems with keeping up to date with the repository.
#
#############################################################################
-include Makefile.local

ifeq ($(COMPILE_PLATFORM),cygwin)
  PLATFORM=mingw32
endif

# detect "emmake make"
ifeq ($(findstring /emcc,$(CC)),/emcc)
  PLATFORM=emscripten
endif

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM

ifeq ($(PLATFORM),mingw32)
  MINGW=1
endif
ifeq ($(PLATFORM),mingw64)
  MINGW=1
endif

ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif

ifeq ($(COMPILE_ARCH),powerpc)
  COMPILE_ARCH=ppc
endif
ifeq ($(COMPILE_ARCH),powerpc64)
  COMPILE_ARCH=ppc64
endif

ifeq ($(COMPILE_ARCH),axp)
  COMPILE_ARCH=alpha
endif

ifndef ARCH
ARCH=$(COMPILE_ARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(COMPILE_PLATFORM))
  CROSS_COMPILING=1
else
  CROSS_COMPILING=0

  ifneq ($(ARCH),$(COMPILE_ARCH))
    CROSS_COMPILING=1
  endif
endif
export CROSS_COMPILING

ifndef VERSION
VERSION=1.36
endif

ifndef CLIENTBIN
CLIENTBIN=ioquake3
endif

ifndef SERVERBIN
SERVERBIN=ioq3ded
endif

ifndef BASEGAME
BASEGAME=baseq3
endif

ifndef BASEGAME_CFLAGS
BASEGAME_CFLAGS=
endif

ifndef MISSIONPACK
MISSIONPACK=missionpack
endif

ifndef MISSIONPACK_CFLAGS
MISSIONPACK_CFLAGS=-DMISSIONPACK
endif

ifndef COPYDIR
COPYDIR="/usr/local/games/quake3"
endif

ifndef COPYBINDIR
COPYBINDIR=$(COPYDIR)
endif

ifndef MOUNT_DIR
MOUNT_DIR=code
endif

ifndef BUILD_DIR
BUILD_DIR=build
endif

ifndef TEMPDIR
TEMPDIR=/tmp
endif

ifndef GENERATE_DEPENDENCIES
GENERATE_DEPENDENCIES=1
endif

ifndef USE_OPENAL
USE_OPENAL=1
endif

ifndef USE_OPENAL_DLOPEN
USE_OPENAL_DLOPEN=1
endif

ifndef USE_HTTP
USE_HTTP=1
endif

ifndef USE_CODEC_VORBIS
USE_CODEC_VORBIS=1
endif

ifndef USE_CODEC_OPUS
USE_CODEC_OPUS=1
endif

ifndef USE_MUMBLE
USE_MUMBLE=1
endif

ifndef USE_VOIP
USE_VOIP=1
endif

ifndef USE_FREETYPE
USE_FREETYPE=0
endif

ifndef USE_INTERNAL_LIBS
USE_INTERNAL_LIBS=1
endif

ifndef USE_INTERNAL_CURL_HEADERS
USE_INTERNAL_CURL_HEADERS=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_SPEEX
USE_INTERNAL_SPEEX=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_OGG
USE_INTERNAL_OGG=$(USE_INTERNAL_LIBS)
NEED_OGG=1
endif

ifndef USE_INTERNAL_OPENAL_HEADERS
USE_INTERNAL_OPENAL_HEADERS=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_VORBIS
USE_INTERNAL_VORBIS=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_OPUS
USE_INTERNAL_OPUS=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_SDL
USE_INTERNAL_SDL=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_ZLIB
USE_INTERNAL_ZLIB=$(USE_INTERNAL_LIBS)
endif

ifndef USE_INTERNAL_JPEG
USE_INTERNAL_JPEG=$(USE_INTERNAL_LIBS)
endif

ifndef USE_RENDERER_DLOPEN
USE_RENDERER_DLOPEN=1
endif

ifndef USE_ARCHLESS_FILENAMES
USE_ARCHLESS_FILENAMES=0
endif

ifndef USE_YACC
USE_YACC=0
endif

ifndef USE_AUTOUPDATER  # DON'T include unless you mean to!
USE_AUTOUPDATER=0
endif

ifndef DEBUG_CFLAGS
DEBUG_CFLAGS=-ggdb -O0
endif

ifdef CLANG
CFLAGS=-Qunused-arguments
CC=clang
CXX=clang++
endif

ifdef CGAME_HARD_LINKED
CGAME_HARD_LINKED = -DCGAME_HARD_LINKED
endif

#############################################################################

BD=$(BUILD_DIR)/debug-$(PLATFORM)-$(ARCH)
BR=$(BUILD_DIR)/release-$(PLATFORM)-$(ARCH)
SPLINESDIR=$(MOUNT_DIR)/splines
CDIR=$(MOUNT_DIR)/client
SDIR=$(MOUNT_DIR)/server
RCOMMONDIR=$(MOUNT_DIR)/renderercommon
RGL1DIR=$(MOUNT_DIR)/renderergl1
RGL2DIR=$(MOUNT_DIR)/renderergl2
CMDIR=$(MOUNT_DIR)/qcommon
SDLDIR=$(MOUNT_DIR)/sdl
ASMDIR=$(MOUNT_DIR)/asm
SYSDIR=$(MOUNT_DIR)/sys
GDIR=$(MOUNT_DIR)/game
CGDIR=$(MOUNT_DIR)/cgame
BLIBDIR=$(MOUNT_DIR)/botlib
NDIR=$(MOUNT_DIR)/null
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
JPDIR=$(MOUNT_DIR)/jpeg-9f
CURLDIR=$(MOUNT_DIR)/curl-8.15.0
FREETYPEDIR=$(MOUNT_DIR)/freetype-2.12.1
SPEEXDIR=$(MOUNT_DIR)/libspeex-1.2.0
SPEEXDSPDIR=$(MOUNT_DIR)/libspeexdsp-1.2rc3
OGGDIR=$(MOUNT_DIR)/libogg-1.3.6
VORBISDIR=$(MOUNT_DIR)/libvorbis-1.3.7
OPUSDIR=$(MOUNT_DIR)/opus-1.5.2
OPUSFILEDIR=$(MOUNT_DIR)/opusfile-0.12
OPENALDIR=${MOUNT_DIR}/openal-soft-1.24.3
ZDIR=$(MOUNT_DIR)/zlib-1.3.1
TOOLSDIR=$(MOUNT_DIR)/tools
Q3ASMDIR=$(MOUNT_DIR)/tools/asm
LBURGDIR=$(MOUNT_DIR)/tools/lcc/lburg
Q3CPPDIR=$(MOUNT_DIR)/tools/lcc/cpp
Q3LCCETCDIR=$(MOUNT_DIR)/tools/lcc/etc
Q3LCCSRCDIR=$(MOUNT_DIR)/tools/lcc/src
AUTOUPDATERSRCDIR=$(MOUNT_DIR)/autoupdater
LIBTOMCRYPTSRCDIR=$(AUTOUPDATERSRCDIR)/rsa_tools/libtomcrypt-1.17
TOMSFASTMATHSRCDIR=$(AUTOUPDATERSRCDIR)/rsa_tools/tomsfastmath-0.13.1
LOKISETUPDIR=misc/setup
NSISDIR=misc/nsis
WEBDIR=$(MOUNT_DIR)/web
SDLHDIR=$(MOUNT_DIR)/SDL2-2.32.8
LIBSDIR=$(MOUNT_DIR)/libs

bin_path=$(shell which $(1) 2> /dev/null)

# The autoupdater uses curl, so figure out its flags no matter what.
# We won't need this if we only build the server

# set PKG_CONFIG_PATH or PKG_CONFIG to influence this, e.g.
# PKG_CONFIG_PATH=/opt/cross/i386-mingw32msvc/lib/pkgconfig or
# PKG_CONFIG=arm-linux-gnueabihf-pkg-config
ifeq ($(CROSS_COMPILING),0)
  PKG_CONFIG ?= pkg-config
else
ifneq ($(PKG_CONFIG_PATH),)
  PKG_CONFIG ?= pkg-config
else
  # Don't use host pkg-config when cross-compiling.
  # (unknown-pkg-config is meant to be a non-existant command.)
  PKG_CONFIG ?= unknown-pkg-config
endif
endif

ifneq ($(call bin_path, $(PKG_CONFIG)),)
  CURL_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags libcurl)
  CURL_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs libcurl)
  OPENAL_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags openal)
  OPENAL_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs openal)
  SDL_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags sdl2|sed 's/-Dmain=SDL_main//')
  SDL_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs sdl2)
else
  # assume they're in the system default paths (no -I or -L needed)
  CURL_LIBS ?= -lcurl
  OPENAL_LIBS ?= -lopenal
endif

ifeq ($(USE_INTERNAL_CURL_HEADERS),1)
  CURL_CFLAGS+=-I$(CURLDIR)/include
endif

ifeq ($(USE_INTERNAL_OPENAL_HEADERS),1)
  OPENAL_CFLAGS+=-I${OPENALDIR}/include
endif

# Use sdl2-config if all else fails
ifeq ($(SDL_CFLAGS),)
  ifneq ($(call bin_path, sdl2-config),)
    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS = $(shell sdl2-config --libs)
  endif
endif

# Add git version info
USE_GIT=
ifeq ($(wildcard .git),.git)
  GIT_REV=$(shell git show -s --pretty=format:%h-%ad --date=short)
  ifneq ($(GIT_REV),)
    VERSION:=$(VERSION)_GIT_$(GIT_REV)
    USE_GIT=1
  endif
endif

CGAME_LIBS = -lpthread

EXTRACLIENTBINDEPS =

#############################################################################
# SETUP AND BUILD -- LINUX
#############################################################################

INSTALL=install
MKDIR=mkdir -p
EXTRA_FILES=
CLIENT_EXTRA_FILES=

ifneq (,$(findstring "$(COMPILE_PLATFORM)", "linux" "gnu_kfreebsd" "kfreebsd-gnu" "gnu"))
  TOOLS_CFLAGS += -DARCH_STRING=\"$(COMPILE_ARCH)\"
endif

ifneq (,$(findstring "$(PLATFORM)", "linux" "gnu_kfreebsd" "kfreebsd-gnu" "gnu"))

  ifeq ($(ARCH),x86_64)
    LDFLAGS += -m64
  endif

  ifndef CLANG
    CXX = g++
  endif

  ifdef CGAME_HARD_LINKED
    WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
    WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
    BASE_CFLAGS = -p -g -rdynamic -pipe -DUSE_ICON -msse $(CGAME_HARD_LINKED)
    SSE2_CFLAGS = -msse2
  else
    WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
    WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
    BASE_CFLAGS = -g -rdynamic -pipe -DUSE_ICON -msse
    SSE2_CFLAGS = -msse2
  endif

  CLIENT_CFLAGS += $(SDL_CFLAGS)

  ifeq ($(USE_FREETYPE),1)
    #FIXME linux version is always using system libfreetype

    # add extra freetype directories since they changed header locations
    FREETYPE_CFLAGS += -I/usr/include/freetype2 -I/usr/include/freetype2/freetype
    FREETYPE_LIBS = -lfreetype
  endif

  OPTIMIZEVM = -O3
  OPTIMIZE += $(OPTIMIZEVM) -ffast-math

  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM = -O3
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  else
  ifeq ($(ARCH),x86)
    OPTIMIZEVM = -O3 -march=i586
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED=true
  else
  ifeq ($(ARCH),ppc)
    ALTIVEC_CFLAGS = -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),ppc64)
    ALTIVEC_CFLAGS = -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),sparc)
    OPTIMIZE += -mtune=ultrasparc3 -mv8plus
    OPTIMIZEVM += -mtune=ultrasparc3 -mv8plus
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),armv7l)
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),alpha)
    # According to http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=410555
    # -ffast-math will cause the client to die with SIGFPE on Alpha
    OPTIMIZE = $(OPTIMIZEVM)
  endif
  endif
  endif

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC -fvisibility=hidden
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-ldl -lm
  AUTOUPDATER_LIBS += -ldl

  CLIENT_LIBS=$(SDL_LIBS) -lX11
  RENDERER_LIBS += $(SDL_LIBS)

  ifeq ($(ARCH),x86_64)
     #CLIENT_LIBS=-L/usr/lib/x86_64-linux-gnu -lSDL
  endif

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) $(OPENAL_LIBS)
    endif
  endif

  ifeq ($(USE_HTTP),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    CLIENT_LIBS += $(CURL_LIBS)
  endif

  ifeq ($(USE_MUMBLE),1)
    CLIENT_LIBS += -lrt
  endif

  ifeq ($(ARCH),x86)
    # linux32 make ...
    BASE_CFLAGS += -m32
  else
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -m64
  endif
  endif
else # ifeq Linux

#############################################################################
# SETUP AND BUILD -- MAC OS X
#############################################################################

GCC_IS_CLANG = $(shell gcc --version 2>/dev/null | grep clang)

ifeq ($(PLATFORM),darwin)

  ifneq ($(CXX),)
    CXX = g++
  endif

  HAVE_VM_COMPILED=true
  LIBS = -framework Cocoa
  CLIENT_LIBS=
  RENDERER_LIBS=
  OPTIMIZEVM = -O3

  BASE_CFLAGS = -Wall

  # Default minimum Mac OS X version
  ifeq ($(MACOSX_VERSION_MIN),)
    MACOSX_VERSION_MIN=10.9
    ifneq ($(findstring $(ARCH),ppc ppc64),)
      MACOSX_VERSION_MIN=10.5
    endif
    ifeq ($(ARCH),arm64)
      MACOSX_VERSION_MIN=11.0
    endif
  endif

  MACOSX_MAJOR=$(shell echo $(MACOSX_VERSION_MIN) | cut -d. -f1)
  MACOSX_MINOR=$(shell echo $(MACOSX_VERSION_MIN) | cut -d. -f2)
  ifeq ($(shell test $(MACOSX_MINOR) -gt 9; echo $$?),0)
    # Multiply and then remove decimal. 10.10 -> 101000.0 -> 101000
    MAC_OS_X_VERSION_MIN_REQUIRED=$(shell echo "$(MACOSX_MAJOR) * 10000 + $(MACOSX_MINOR) * 100" | bc | cut -d. -f1)
  else
    # Multiply by 100 and then remove decimal. 10.7 -> 1070.0 -> 1070
    MAC_OS_X_VERSION_MIN_REQUIRED=$(shell echo "$(MACOSX_VERSION_MIN) * 100" | bc | cut -d. -f1)
  endif

  LDFLAGS += -mmacosx-version-min=$(MACOSX_VERSION_MIN)
  BASE_CFLAGS += -mmacosx-version-min=$(MACOSX_VERSION_MIN) \
                 -DMAC_OS_X_VERSION_MIN_REQUIRED=$(MAC_OS_X_VERSION_MIN_REQUIRED)

  MACOSX_ARCH=$(ARCH)
  ifeq ($(ARCH),x86)
    MACOSX_ARCH=i386
  endif

  ifeq ($(ARCH),ppc)
    BASE_CFLAGS += -arch ppc
    ALTIVEC_CFLAGS = -faltivec
  endif
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -arch ppc64
    ALTIVEC_CFLAGS = -faltivec
  endif
  ifeq ($(ARCH),x86)
    OPTIMIZEVM += -march=prescott -mfpmath=sse
    # x86 vm will crash without -mstackrealign since MMX instructions will be
    # used no matter what and they corrupt the frame pointer in VM calls
    #FIXME also min version?
    BASE_CFLAGS += -arch i386 -m32 -mstackrealign
  endif
  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM += -mfpmath=sse
    BASE_CFLAGS += -arch x86_64
  endif
  ifeq ($(ARCH),arm64)
    # HAVE_VM_COMPILED=false # TODO: implement compiled vm
    BASE_CFLAGS += -arch arm64
  endif

  # When compiling on OSX for OSX, we're not cross compiling as far as the
  # Makefile is concerned, as target architecture is specified as a compiler
  # argument
  ifeq ($(COMPILE_PLATFORM),darwin)
    CROSS_COMPILING=0
  endif

  ifeq ($(CROSS_COMPILING),1)
    # If CC is already set to something generic, we probably want to use
    # something more specific
    ifneq ($(findstring $(strip $(CC)),cc gcc),)
      CC=
    endif

    ifndef CC
      ifndef DARWIN
        # macOS 10.9 SDK
        DARWIN=13
        ifneq ($(findstring $(ARCH),ppc ppc64),)
          # macOS 10.5 SDK, though as of writing osxcross doesn't support ppc/ppc64
          DARWIN=9
        endif
        ifeq ($(ARCH),arm64)
          # macOS 11.3 SDK
          DARWIN=20.4
        endif
      endif

      CC=$(MACOSX_ARCH)-apple-darwin$(DARWIN)-cc
      RANLIB=$(MACOSX_ARCH)-apple-darwin$(DARWIN)-ranlib
      LIPO=$(MACOSX_ARCH)-apple-darwin$(DARWIN)-lipo

      ifeq ($(call bin_path, $(CC)),)
        $(error Unable to find osxcross $(CC))
      endif
    endif
  else
    TOOLS_CFLAGS += -DMACOS_X
  endif

  ifndef LIPO
    LIPO=lipo
  endif

  BASE_CFLAGS += -fno-strict-aliasing -fno-common -pipe

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_INTERNAL_OPENAL_HEADERS),1)
      CLIENT_CFLAGS += -I/System/Library/Frameworks/OpenAL.framework/Headers
    endif
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += -framework OpenAL
    endif
  endif

  ifeq ($(USE_HTTP),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    CLIENT_LIBS += $(CURL_LIBS)
  endif

  ifeq ($(USE_FREETYPE),1)
      #FIXME using internal freetype headers
      #FREETYPE_CFLAGS = -I$(MOUNT_DIR)/freetype2/include -I$(MOUNT_DIR)/freetype2/include/freetype2/freetype -I$(MOUNT_DIR)/freetype2/include/freetype2
      FREETYPE_CFLAGS = -I$(FREETYPEDIR)/include
      FREETYPE_LIBS = $(LIBSDIR)/macosx/libfreetype.a
  endif

  ##ifeq ($(USE_RENDERER_DLOPEN),1)
  ##  CLIENT_CFLAGS += -DUSE_RENDERER_DLOPEN
  ##endif

  BASE_CFLAGS += -D_THREAD_SAFE=1

  CLIENT_LIBS += -framework IOKit
  RENDERER_LIBS += -framework OpenGL

  ifeq ($(USE_INTERNAL_SDL),1)
    ifeq ($(shell test $(MAC_OS_X_VERSION_MIN_REQUIRED) -ge 1090; echo $$?),0)
      # Universal Binary 2 - for running on macOS 10.9 or later
      # x86_64 (10.9 or later), arm64 (11.0 or later)
      MACLIBSDIR=$(LIBSDIR)/macosx-ub2
      BASE_CFLAGS += -I$(SDLHDIR)/include
    else
      # Universal Binary - for running on Mac OS X 10.5 or later
      # ppc (10.5/10.6), x86 (10.6 or later), x86_64 (10.6 or later)
      #
      # x86/x86_64 on 10.5 will run the ppc build.
      #
      # SDL 2.0.1,  last with Mac OS X PowerPC
      # SDL 2.0.4,  last with Mac OS X 10.5 (x86/x86_64)
      # SDL 2.0.22, last with Mac OS X 10.6 (x86/x86_64)
      #
      # code/libs/macosx-ub/libSDL2-2.0.0.dylib contents
      # - ppc build is SDL 2.0.1 with a header change so it compiles
      # - x86/x86_64 builds are SDL 2.0.22
      MACLIBSDIR=$(LIBSDIR)/macosx-ub
      ifneq ($(findstring $(ARCH),ppc ppc64),)
        SDLHDIR=$(MOUNT_DIR)/SDL2-2.0.1
        BASE_CFLAGS += -I$(SDLHDIR)/include
      else
        SDLHDIR=$(MOUNT_DIR)/SDL2-2.0.22
        BASE_CFLAGS += -I$(SDLHDIR)/include
      endif
    endif

    # We copy sdlmain before ranlib'ing it so that subversion doesn't think
    #  the file has been modified by each build.
    LIBSDLMAIN = $(B)/libSDL2main.a
    LIBSDLMAINSRC = $(MACLIBSDIR)/libSDL2main.a
    CLIENT_LIBS += $(MACLIBSDIR)/libSDL2-2.0.0.dylib
    RENDERER_LIBS += $(MACLIBSDIR)/libSDL2-2.0.0.dylib
    CLIENT_EXTRA_FILES += $(MACLIBSDIR)/libSDL2-2.0.0.dylib
  else
    BASE_CFLAGS += -I/Library/Frameworks/SDL2.framework/Headers
    CLIENT_LIBS += -framework SDL2
    RENDERER_LIBS += -framework SDL2
  endif

  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=dylib
  SHLIBCFLAGS=-fPIC -fno-common
  SHLIBLDFLAGS=-dynamiclib $(LDFLAGS) -Wl,-U,_com_altivec

  NOTSHLIBCFLAGS=-mdynamic-no-pic

  ifeq ($(MACOSX_INCLUDE_BINARY_PLIST),1)
    EXTRACLIENTBINDEPS += $(SYSDIR)/Info.plist
    CLIENT_LDFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$(SYSDIR)/Info.plist
  endif

else # ifeq darwin


#############################################################################
# SETUP AND BUILD -- MINGW32
#############################################################################

ifdef MINGW

  ifeq ($(CROSS_COMPILING),1)
    # If CC is already set to something generic, we probably want to use
    # something more specific
    ifneq ($(findstring $(strip $(CC)),cc gcc),)
      CC=
    endif

    # We need to figure out the correct gcc and windres
    ifeq ($(ARCH),x86_64)
      MINGW_PREFIXES=x86_64-w64-mingw32 amd64-mingw32msvc
    endif
    ifeq ($(ARCH),x86)
      MINGW_PREFIXES=i686-w64-mingw32 i586-mingw32msvc i686-pc-mingw32
    endif

    ifndef CC
      CC=$(firstword $(strip $(foreach MINGW_PREFIX, $(MINGW_PREFIXES), \
         $(call bin_path, $(MINGW_PREFIX)-gcc))))

      # ifndef CXX not working (Debian Stable) unlike ifndef CC
      CXX=$(firstword $(strip $(foreach MINGW_PREFIX, $(MINGW_PREFIXES), \
         $(call bin_path, $(MINGW_PREFIX)-g++))))
    endif

    ifndef WINDRES
      WINDRES=$(firstword $(strip $(foreach MINGW_PREFIX, $(MINGW_PREFIXES), \
         $(call bin_path, $(MINGW_PREFIX)-windres))))
    endif
  else
    # Some MinGW installations define CC to cc, but don't actually provide cc,
    # so check that CC points to a real binary and use gcc if it doesn't
    ifeq ($(call bin_path, $(CC)),)
      CC=gcc
    endif

  endif

  # using generic windres if specific one is not present
  ifndef WINDRES
    WINDRES=windres
  endif

  ifeq ($(CC),)
    $(error Cannot find a suitable cross compiler for $(PLATFORM))
  endif

  WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
  WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
  BASE_CFLAGS = -g -gdwarf-3 -DUSE_ICON -msse
  SSE2_CFLAGS = -msse2

  ifeq ($(ARCH),x86)
    LDFLAGS += -Xlinker --large-address-aware
  endif

  CLIENT_LIBS =
  # don't use pthreads with win32
  CGAME_LIBS =

  # In the absence of wspiapi.h, require Windows XP or later
  ifeq ($(shell test -e $(CMDIR)/wspiapi.h; echo $$?),1)
    BASE_CFLAGS += -DWINVER=0x501
  endif

  ifeq ($(USE_OPENAL),1)
    CLIENT_CFLAGS += $(OPENAL_CFLAGS)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LDFLAGS += $(OPENAL_LDFLAGS)
    endif
  endif

  ifeq ($(USE_FREETYPE),1)
    #FIXME using internal freetype headers
    #FREETYPE_CFLAGS += -Ifreetype2
    #FREETYPE_CFLAGS = -I$(MOUNT_DIR)/freetype2/include -I$(MOUNT_DIR)/freetype2/include/freetype2/freetype -I$(MOUNT_DIR)/freetype2/include/freetype2
    FREETYPE_CFLAGS = -I$(FREETYPEDIR)/include
    ifeq ($(ARCH),x86)
        FREETYPE_LIBS = $(LIBSDIR)/win32/libfreetype.a
    endif
    ifeq ($(ARCH),x86_64)
        FREETYPE_LIBS = $(LIBSDIR)/win64/libfreetype.a
    endif
  endif

  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM = -O3
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  endif
  ifeq ($(ARCH),x86)
    # need frame pointer for StackWalk() in backtrace.dll
    OPTIMIZEVM = -O3 -march=i586 -fno-omit-frame-pointer
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  endif

  SHLIBEXT=dll
  SHLIBCFLAGS=
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  BINEXT=.exe

  ifeq ($(CROSS_COMPILING),0)
    TOOLS_BINEXT=.exe
  endif

  ifeq ($(COMPILE_PLATFORM),cygwin)
    TOOLS_BINEXT=.exe

    # Under cygwin the default of using gcc for TOOLS_CC won't work, so
    # we need to figure out the appropriate compiler to use, based on the
    # host architecture that we're running under (as tools run on the host)
    ifeq ($(COMPILE_ARCH),x86_64)
      TOOLS_MINGW_PREFIXES=x86_64-w64-mingw32 amd64-mingw32msvc
    endif
    ifeq ($(COMPILE_ARCH),x86)
      TOOLS_MINGW_PREFIXES=i686-w64-mingw32 i586-mingw32msvc i686-pc-mingw32
    endif

    TOOLS_CC=$(firstword $(strip $(foreach TOOLS_MINGW_PREFIX, $(TOOLS_MINGW_PREFIXES), \
      $(call bin_path, $(TOOLS_MINGW_PREFIX)-gcc))))
  endif

  # 2021-02-26 cygwin g++ is automatically adding libwinpthread dependency
  #   checked cygwin 3.1.7 with both gcc 9.2.0 and 10.2.0
  ifeq ($(COMPILE_PLATFORM),cygwin)
    LIBS= -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,-Bdynamic,--no-whole-archive -lws2_32 -lwinmm -lpsapi -lshell32 -static-libgcc -static-libstdc++
  else
    LIBS= -lws2_32 -lwinmm -lpsapi -lshell32 -static-libgcc -static-libstdc++
    # 2022-04-29 i686-w64-mingw32-gcc (GCC) 10-win32 20210110 : cgamex86.dll
    # using libgcc_s_dw2-1.dll (needs symbols __divdi3 and __udivdi3)
    CGAME_LIBS += -static-libgcc
  endif

  AUTOUPDATER_LIBS += -lwininet

  # clang 3.4 doesn't support this
  ifneq ("$(CC)", $(findstring "$(CC)", "clang" "clang++"))
    CLIENT_LDFLAGS += -mwindows -gdwarf-3
  endif

  CLIENT_LIBS += -lgdi32 -lole32
  RENDERER_LIBS += -lgdi32 -lole32 -static-libgcc

  ifeq ($(USE_HTTP),1)
    CLIENT_LIBS += -lwininet
  endif

  ifeq ($(USE_RENDERER_DLOPEN),1)
    CLIENT_CFLAGS += -DUSE_RENDERER_DLOPEN
  endif

  ifeq ($(ARCH),x86)
    # build 32bit
    BASE_CFLAGS += -m32
  else
    BASE_CFLAGS += -m64
  endif

  # libmingw32 must be linked before libSDLmain
  CLIENT_LIBS += -lmingw32
  RENDERER_LIBS += -lmingw32

  ifeq ($(USE_INTERNAL_SDL),1)
    CLIENT_CFLAGS += -I$(SDLHDIR)/include
    ifeq ($(ARCH),x86)
      CLIENT_LIBS += $(LIBSDIR)/win32/libSDL2main.a \
                      $(LIBSDIR)/win32/libSDL2.dll.a
      RENDERER_LIBS += $(LIBSDIR)/win32/libSDL2main.a \
                      $(LIBSDIR)/win32/libSDL2.dll.a
      SDLDLL=SDL2.dll
      CLIENT_EXTRA_FILES += $(LIBSDIR)/win32/SDL2.dll
    else
      CLIENT_LIBS += $(LIBSDIR)/win64/libSDL264main.a \
                      $(LIBSDIR)/win64/libSDL264.dll.a
      RENDERER_LIBS += $(LIBSDIR)/win64/libSDL264main.a \
                      $(LIBSDIR)/win64/libSDL264.dll.a
      SDLDLL=SDL264.dll
      CLIENT_EXTRA_FILES += $(LIBSDIR)/win64/SDL264.dll
    endif
  else
    CLIENT_CFLAGS += $(SDL_CFLAGS)
    CLIENT_LIBS += $(SDL_LIBS)
    RENDERER_LIBS += $(SDL_LIBS)
    SDLDLL=SDL2.dll
  endif

else # ifdef MINGW

#############################################################################
# SETUP AND BUILD -- FREEBSD
#############################################################################

ifeq ($(PLATFORM),freebsd)
  # Use the default C compiler
  TOOLS_CC=cc

  # flags
  WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
  WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
  BASE_CFLAGS = -DUSE_ICON -DMAP_ANONYMOUS=MAP_ANON
  CLIENT_CFLAGS += $(SDL_CFLAGS)
  HAVE_VM_COMPILED = true

  OPTIMIZEVM = -O3
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  # don't need -ldl (FreeBSD)
  LIBS=-lm

  CLIENT_LIBS =

  CLIENT_LIBS += $(SDL_LIBS)
  RENDERER_LIBS += $(SDL_LIBS)

  # optional features/libraries
  ifeq ($(USE_OPENAL),1)
    ifeq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) $(OPENAL_LIBS)
    endif
  endif

  ifeq ($(USE_HTTP),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    CLIENT_LIBS += $(CURL_LIBS)
  endif

  # cross-compiling tweaks
  ifeq ($(ARCH),x86)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m32
    endif
  endif
  ifeq ($(ARCH),x86_64)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m64
    endif
  endif
else # ifeq freebsd

#############################################################################
# SETUP AND BUILD -- OPENBSD
#############################################################################

ifeq ($(PLATFORM),openbsd)

  WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
  WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
  BASE_CFLAGS = -pipe -DUSE_ICON -DMAP_ANONYMOUS=MAP_ANON
  CLIENT_CFLAGS += $(SDL_CFLAGS)

  OPTIMIZEVM = -O3
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM = -O3
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  else
  ifeq ($(ARCH),x86)
    OPTIMIZEVM = -O3 -march=i586
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED=true
  else
  ifeq ($(ARCH),ppc)
    ALTIVEC_CFLAGS = -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),ppc64)
    ALTIVEC_CFLAGS = -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),sparc64)
    OPTIMIZE += -mtune=ultrasparc3 -mv8plus
    OPTIMIZEVM += -mtune=ultrasparc3 -mv8plus
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),alpha)
    # According to http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=410555
    # -ffast-math will cause the client to die with SIGFPE on Alpha
    OPTIMIZE = $(OPTIMIZEVM)
  endif
  endif
  endif

  ifeq ($(USE_HTTP),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    CLIENT_LIBS += $(CURL_LIBS)
  endif

  # no shm_open on OpenBSD
  USE_MUMBLE=0

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-lm

  #FIXME this is fucked up, do like the Mac version
  #CLIENT_LIBS =

  CLIENT_LIBS += $(SDL_LIBS)
  RENDERER_LIBS += $(SDL_LIBS)

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) $(OPENAL_LIBS)
    endif
  endif
else # ifeq openbsd

#############################################################################
# SETUP AND BUILD -- NETBSD
#############################################################################

ifeq ($(PLATFORM),netbsd)

  LIBS=-lm
  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)
  THREAD_LIBS=-lpthread

  WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
  WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
  BASE_CFLAGS =

  ifeq ($(ARCH),x86)
    HAVE_VM_COMPILED=true
  endif

  BUILD_CLIENT = 0
else # ifeq netbsd

#############################################################################
# SETUP AND BUILD -- IRIX
#############################################################################

ifeq ($(PLATFORM),irix64)
  LIB=lib

  ARCH=mips

  CC = c99

  BASE_CFLAGS=-Dstricmp=strcasecmp -Xcpluscomm -woff 1185 \
    -I. -I$(ROOT)/usr/include
  CLIENT_CFLAGS += $(SDL_CFLAGS)
  OPTIMIZE = -O3

  SHLIBEXT=so
  SHLIBCFLAGS=
  SHLIBLDFLAGS=-shared

  LIBS=-ldl -lm -lgen
  AUTOUPDATER_LIBS += -ldl

  # FIXME: The X libraries probably aren't necessary?
  CLIENT_LIBS=-L/usr/X11/$(LIB) $(SDL_LIBS) \
    -lX11 -lXext -lm
  RENDERER_LIBS += $(SDL_LIBS)

else # ifeq IRIX

#############################################################################
# SETUP AND BUILD -- SunOS
#############################################################################

ifeq ($(PLATFORM),sunos)

  CC=gcc
  INSTALL=ginstall
  MKDIR=gmkdir -p
  COPYDIR="/usr/local/share/games/quake3"

  ifneq ($(ARCH),x86)
    ifneq ($(ARCH),sparc)
      $(error arch $(ARCH) is currently not supported)
    endif
  endif

  WARNINGS_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes
  WARNINGS_CXXFLAGS = -Wall -fno-strict-aliasing
  BASE_CFLAGS = -pipe -DUSE_ICON
  CLIENT_CFLAGS += $(SDL_CFLAGS)

  OPTIMIZEVM = -O3 -funroll-loops

  ifeq ($(ARCH),sparc)
    OPTIMIZEVM += -O3 \
      -fstrength-reduce -falign-functions=2 \
      -mtune=ultrasparc3 -mv8plus -mno-faster-structs
    HAVE_VM_COMPILED=true
  else
  ifeq ($(ARCH),x86)
    OPTIMIZEVM += -march=i586 -fomit-frame-pointer \
      -falign-functions=2 -fstrength-reduce
    HAVE_VM_COMPILED=true
    BASE_CFLAGS += -m32
    CLIENT_CFLAGS += -I/usr/X11/include/NVIDIA
    CLIENT_LDFLAGS += -L/usr/X11/lib/NVIDIA -R/usr/X11/lib/NVIDIA
  endif
  endif

  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-lsocket -lnsl -ldl -lm
  AUTOUPDATER_LIBS += -ldl

  BOTCFLAGS=-O0

  CLIENT_LIBS +=$(SDL_LIBS) -lX11 -lXext -liconv -lm
  RENDERER_LIBS += $(SDL_LIBS)

else # ifeq sunos

#############################################################################
# SETUP AND BUILD -- emscripten
#############################################################################

ifeq ($(PLATFORM),emscripten)

  ifneq ($(findstring /emcc,$(CC)),/emcc)
    CC=emcc
  endif
  ARCH=wasm32
  BINEXT=.js

  # dlopen(), opengl1, and networking are not functional
  USE_RENDERER_DLOPEN=0
  USE_OPENAL_DLOPEN=0
  BUILD_GAME_SO=0
  BUILD_RENDERER_OPENGL1=0
  BUILD_SERVER=0
  USE_HTTP=0

  CLIENT_CFLAGS+=-s USE_SDL=2

  CLIENT_LDFLAGS+=-s TOTAL_MEMORY=256MB
  CLIENT_LDFLAGS+=-s STACK_SIZE=5MB
  CLIENT_LDFLAGS+=-s MIN_WEBGL_VERSION=1 -s MAX_WEBGL_VERSION=2

  # The HTML file can use these functions to load extra files before the game starts.
  CLIENT_LDFLAGS+=-s EXPORTED_RUNTIME_METHODS=FS,addRunDependency,removeRunDependency
  CLIENT_LDFLAGS+=-s EXIT_RUNTIME=1
  CLIENT_LDFLAGS+=-s EXPORT_ES6
  CLIENT_LDFLAGS+=-s EXPORT_NAME=ioquake3

  # Game data files can be packaged by emcc into a .data file that lives next to the wasm bundle
  # and added to the virtual filesystem before the game starts. This requires the game data to be
  # present at build time and it can't be changed afterward.
  # For more flexibility, game data files can be loaded from a web server at runtime by listing
  # them in client-config.json. This way they don't have to be present at build time and can be
  # changed later.
  ifeq ($(EMSCRIPTEN_PRELOAD_FILE),1)
    ifeq ($(wildcard $(BASEGAME)/*),)
      $(error "No files in '$(BASEGAME)' directory for emscripten to preload.")
    endif
    CLIENT_LDFLAGS+=--preload-file $(BASEGAME)
  endif

  OPTIMIZEVM = -O3
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  # These allow a warning-free build.
  # Some of these warnings may actually be legit problems and should be fixed at some point.
  BASE_CFLAGS+=-Wno-deprecated-non-prototype -Wno-dangling-else -Wno-implicit-const-int-float-conversion -Wno-misleading-indentation -Wno-format-overflow -Wno-logical-not-parentheses -Wno-absolute-value

  DEBUG_CFLAGS=-g3 -O0 # -fsanitize=address -fsanitize=undefined
  # Emscripten needs debug compiler flags to be passed to the linker as well
  DEBUG_LDFLAGS=$(DEBUG_CFLAGS)

  SHLIBEXT=wasm
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-s SIDE_MODULE

else # ifeq emscripten

#############################################################################
# SETUP AND BUILD -- GENERIC
#############################################################################
  BASE_CFLAGS=
  OPTIMIZE = -O3

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared

endif #Linux
endif #darwin
endif #MINGW
endif #FreeBSD
endif #OpenBSD
endif #NetBSD
endif #IRIX
endif #SunOS
endif #emscripten

ifndef CC
  CC=gcc
endif

CC_VERSION=$(shell $(CC) --version | head -n1)
CXX_VERSION=$(shell $(CXX) --version | head -n1)

ifndef RANLIB
  RANLIB=ranlib
endif

ifneq ($(HAVE_VM_COMPILED),true)
  BASE_CFLAGS += -DNO_VM_COMPILED
endif

TARGETS =

ifndef FULLBINEXT
  ifeq ($(USE_ARCHLESS_FILENAMES),1)
    FULLBINEXT=$(BINEXT)
  else
    FULLBINEXT=.$(ARCH)$(BINEXT)
  endif
endif

ifndef SHLIBNAME
  ifeq ($(USE_ARCHLESS_FILENAMES),1)
    SHLIBNAME=.$(SHLIBEXT)
    RSHLIBNAME=$(SHLIBNAME)
  else
    SHLIBNAME=$(ARCH).$(SHLIBEXT)
    RSHLIBNAME=_$(SHLIBNAME)
  endif
endif

ifneq ($(BUILD_SERVER),0)
  TARGETS += $(B)/$(SERVERBIN)$(FULLBINEXT)
endif

ifneq ($(BUILD_CLIENT),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
    TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT)

    ifneq ($(BUILD_RENDERER_OPENGL1),0)
      TARGETS += $(B)/renderer_opengl1$(RSHLIBNAME)
    endif
    ifneq ($(BUILD_RENDERER_OPENGL2),0)
      TARGETS += $(B)/renderer_opengl2$(RSHLIBNAME)
    endif
  else
    ifneq ($(BUILD_RENDERER_OPENGL1),0)
      TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT)
    endif
    ifneq ($(BUILD_RENDERER_OPENGL2),0)
      TARGETS += $(B)/$(CLIENTBIN)_opengl2$(FULLBINEXT)
    endif
  endif
endif

ifneq ($(BUILD_GAME_SO),0)
  ifneq ($(BUILD_BASEGAME),0)
    TARGETS += \
      $(B)/$(BASEGAME)/cgame$(SHLIBNAME) \
      $(B)/$(BASEGAME)/qagame$(SHLIBNAME) \
      $(B)/$(BASEGAME)/ui$(SHLIBNAME)
  endif
  ifneq ($(BUILD_MISSIONPACK),0)
    TARGETS += \
      $(B)/$(MISSIONPACK)/cgame$(SHLIBNAME) \
      $(B)/$(MISSIONPACK)/qagame$(SHLIBNAME) \
      $(B)/$(MISSIONPACK)/ui$(SHLIBNAME)
  endif
endif

ifneq ($(BUILD_GAME_QVM),0)
  ifneq ($(BUILD_BASEGAME),0)
    TARGETS += \
      $(B)/$(BASEGAME)/vm/cgame.qvm \
      $(B)/$(BASEGAME)/vm/qagame.qvm \
      $(B)/$(BASEGAME)/vm/ui.qvm
  endif
  ifneq ($(BUILD_MISSIONPACK),0)
    TARGETS += \
      $(B)/$(MISSIONPACK)/vm/cgame.qvm \
      $(B)/$(MISSIONPACK)/vm/qagame.qvm \
      $(B)/$(MISSIONPACK)/vm/ui.qvm
  endif
endif

CLIENT_CXXFLAGS = -fwrapv

ifneq ($(BUILD_AUTOUPDATER),0)
  # PLEASE NOTE that if you run an exe on Windows Vista or later
  #  with "setup", "install", "update" or other related terms, it
  #  will unconditionally trigger a UAC prompt, and in the case of
  #  ioq3 calling CreateProcess() on it, it'll just fail immediately.
  #  So don't call this thing "autoupdater" here!
  AUTOUPDATER_BIN := autosyncerator$(FULLBINEXT)
  TARGETS += $(B)/$(AUTOUPDATER_BIN)
endif

ifeq ($(PLATFORM),emscripten)
  ifneq ($(BUILD_SERVER),0)
    GENERATEDTARGETS += $(B)/$(SERVERBIN).$(ARCH).wasm
    ifeq ($(EMSCRIPTEN_PRELOAD_FILE),1)
      GENERATEDTARGETS += $(B)/$(SERVERBIN).$(ARCH).data
    endif
  endif

  ifneq ($(BUILD_CLIENT),0)
    TARGETS += $(B)/$(CLIENTBIN).html
    ifneq ($(EMSCRIPTEN_PRELOAD_FILE),1)
      TARGETS += $(B)/$(CLIENTBIN)-config.json
    endif
    ifneq ($(USE_RENDERER_DLOPEN),0)
      GENERATEDTARGETS += $(B)/$(CLIENTBIN).$(ARCH).wasm
      ifeq ($(EMSCRIPTEN_PRELOAD_FILE),1)
        GENERATEDTARGETS += $(B)/$(CLIENTBIN).$(ARCH).data
      endif
    else
      ifneq ($(BUILD_RENDERER_OPENGL1),0)
        GENERATEDTARGETS += $(B)/$(CLIENTBIN).$(ARCH).wasm
        ifeq ($(EMSCRIPTEN_PRELOAD_FILE),1)
          GENERATEDTARGETS += $(B)/$(CLIENTBIN).$(ARCH).data
        endif
      endif
      ifneq ($(BUILD_RENDERER_OPENGL2),0)
        GENERATEDTARGETS += $(B)/$(CLIENTBIN)_opengl2.$(ARCH).wasm
        ifeq ($(EMSCRIPTEN_PRELOAD_FILE),1)
          GENERATEDTARGETS += $(B)/$(CLIENTBIN)_opengl2.$(ARCH).data
        endif
      endif
    endif
  endif
endif

ifeq ($(USE_OPENAL),1)
  CLIENT_CFLAGS += ${OPENAL_CFLAGS} -DUSE_OPENAL
  ifeq ($(USE_OPENAL_DLOPEN),1)
    CLIENT_CFLAGS += -DUSE_OPENAL_DLOPEN
  endif
endif

ifeq ($(USE_HTTP),1)
  CLIENT_CFLAGS += -DUSE_HTTP
endif

ifeq ($(USE_CODEC_VORBIS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_VORBIS
endif

ifeq ($(USE_RENDERER_DLOPEN),1)
  CLIENT_CFLAGS += -DUSE_RENDERER_DLOPEN
endif

ifeq ($(USE_MUMBLE),1)
  CLIENT_CFLAGS += -DUSE_MUMBLE
endif

ifeq ($(USE_VOIP),1)
  CLIENT_CFLAGS += -DUSE_VOIP
  SERVER_CFLAGS += -DUSE_VOIP
  NEED_OPUS=1
  ifeq ($(USE_INTERNAL_SPEEX),1)
    SPEEX_CFLAGS += -DFLOATING_POINT -DUSE_ALLOCA -I$(SPEEXDIR)/include -I$(SPEEXDSPDIR)/include
  else
    #FIXME 2022-04-21 speexdsp?
    SPEEX_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags speex speexdsp || true)
    SPEEX_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs speex speexdsp || echo -lspeex -lspeexdsp)
  endif
  CLIENT_CFLAGS += $(SPEEX_CFLAGS)
  CLIENT_LIBS += $(SPEEX_LIBS)
endif

ifeq ($(USE_CODEC_OPUS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_OPUS
  NEED_OPUS=1
endif

ifeq ($(NEED_OPUS),1)
  ifeq ($(USE_INTERNAL_OPUS),1)
    OPUS_CFLAGS = -DOPUS_BUILD -DHAVE_LRINTF -DFLOATING_POINT -DFLOAT_APPROX -DUSE_ALLOCA \
      -I$(OPUSDIR)/include -I$(OPUSDIR)/celt -I$(OPUSDIR)/silk \
      -I$(OPUSDIR)/silk/float -I$(OPUSFILEDIR)/include
  else
    OPUS_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags opusfile opus || true)
    OPUS_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs opusfile opus || echo -lopusfile -lopus)
  endif
  CLIENT_CFLAGS += $(OPUS_CFLAGS)
  CLIENT_LIBS += $(OPUS_LIBS)
  NEED_OGG=1
endif

ifeq ($(USE_CODEC_VORBIS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_VORBIS
  ifeq ($(USE_INTERNAL_VORBIS),1)
    CLIENT_CFLAGS += -I$(VORBISDIR)/include -I$(VORBISDIR)/lib
  else
    VORBIS_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags vorbisfile vorbis || true)
    VORBIS_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs vorbisfile vorbis || echo -lvorbisfile -lvorbis)
  endif
  CLIENT_CFLAGS += $(VORBIS_CFLAGS)
  CLIENT_LIBS += $(VORBIS_LIBS)
  NEED_OGG=1
endif

ifeq ($(NEED_OGG),1)
  ifeq ($(USE_INTERNAL_OGG),1)
    OGG_CFLAGS = -I$(OGGDIR)/include
  else
    OGG_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags ogg || true)
    OGG_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs ogg || echo -logg)
  endif
  CLIENT_CFLAGS += $(OGG_CFLAGS)
  CLIENT_LIBS += $(OGG_LIBS)
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
  ZLIB_CFLAGS = -DNO_GZIP -I$(ZDIR)
else
  ZLIB_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags zlib || true)
  ZLIB_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs zlib || echo -lz)
endif
BASE_CFLAGS += $(ZLIB_CFLAGS)
LIBS += $(ZLIB_LIBS)

ifeq ($(USE_INTERNAL_JPEG),1)
  BASE_CFLAGS += -DUSE_INTERNAL_JPEG
  BASE_CFLAGS += -I$(JPDIR)
else
  # IJG libjpeg doesn't have pkg-config, but libjpeg-turbo uses libjpeg.pc;
  # we fall back to hard-coded answers if libjpeg.pc is unavailable
  JPEG_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags libjpeg || true)
  JPEG_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs libjpeg || echo -ljpeg)
  BASE_CFLAGS += $(JPEG_CFLAGS)
  RENDERER_LIBS += $(JPEG_LIBS)
endif

ifeq ($(USE_FREETYPE),1)
  FREETYPE_CFLAGS ?= $(shell $(PKG_CONFIG) --silence-errors --cflags freetype2 || true)
  FREETYPE_LIBS ?= $(shell $(PKG_CONFIG) --silence-errors --libs freetype2 || echo -lfreetype)

  BASE_CFLAGS += -DBUILD_FREETYPE $(FREETYPE_CFLAGS)
  RENDERER_LIBS += $(FREETYPE_LIBS)
endif

ifeq ($(USE_AUTOUPDATER),1)
  CLIENT_CFLAGS += -DUSE_AUTOUPDATER -DAUTOUPDATER_BIN=\\\"$(AUTOUPDATER_BIN)\\\"
  SERVER_CFLAGS += -DUSE_AUTOUPDATER -DAUTOUPDATER_BIN=\\\"$(AUTOUPDATER_BIN)\\\"
endif

ifeq ($(BUILD_AUTOUPDATER),1)
  AUTOUPDATER_LIBS += $(LIBTOMCRYPTSRCDIR)/libtomcrypt.a $(TOMSFASTMATHSRCDIR)/libtfm.a
endif

ifeq ("$(CC)", $(findstring "$(CC)", "clang" "clang++"))
  BASE_CFLAGS += -Qunused-arguments
endif

ifdef DEFAULT_BASEDIR
  BASE_CFLAGS += -DDEFAULT_BASEDIR=\\\"$(DEFAULT_BASEDIR)\\\"
endif

ifeq ($(USE_INTERNAL_OPENAL_HEADERS),1)
  BASE_CFLAGS += -DUSE_INTERNAL_OPENAL_HEADERS
endif

ifeq ($(USE_INTERNAL_CURL_HEADERS),1)
  BASE_CFLAGS += -DUSE_INTERNAL_CURL_HEADERS
endif

ifeq ($(USE_INTERNAL_SDL),1)
  BASE_CFLAGS += -DUSE_INTERNAL_SDL_HEADERS
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
  BASE_CFLAGS += -DUSE_INTERNAL_ZLIB
endif

ifeq ($(BUILD_STANDALONE),1)
  BASE_CFLAGS += -DSTANDALONE
endif

ifeq ($(USE_ARCHLESS_FILENAMES),1)
  BASE_CFLAGS += -DUSE_ARCHLESS_FILENAMES
endif

ifeq ($(GENERATE_DEPENDENCIES),1)
  DEPEND_CFLAGS = -MMD
else
  DEPEND_CFLAGS =
endif

ifeq ($(NO_STRIP),1)
  STRIP_FLAG =
else
  STRIP_FLAG = -s
endif

# https://reproducible-builds.org/specs/source-date-epoch/
ifdef SOURCE_DATE_EPOCH
  BASE_CFLAGS += -DPRODUCT_DATE=\\\"$(shell date --date="@$$SOURCE_DATE_EPOCH" "+%b %_d %Y" | sed -e 's/ /\\\ /'g)\\\"
endif

BASE_CFLAGS += -DPRODUCT_VERSION=\\\"$(VERSION)\\\" -DWOLFCAM_VERSION=\\\"$(VERSION)\\\"

BASE_CFLAGS += -D_FILE_OFFSET_BITS=64

WARNINGS_CFLAGS += -Wformat=2 -Wno-format-zero-length -Wformat-security \
  -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute \
  -Wdisabled-optimization -Werror-implicit-function-declaration
WARNINGS_CXXFLAGS += -Wformat=2 -Wno-format-zero-length -Wformat-security \
  -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute \
  -Wdisabled-optimization
THIRDPARTY_CFLAGS += -Wno-strict-prototypes

ifeq ($(V),1)
echo_cmd=@:
Q=
else
echo_cmd=@echo
Q=@
endif

define DO_CC
$(echo_cmd) "CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) -o $@ -c $<
endef

define DO_THIRDPARTY_CC
$(echo_cmd) "THIRDPARTY_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(THIRDPARTY_CFLAGS) -o $@ -c $<
endef

# -fwrapv for $(SPLINES) gcc warning
define DO_CXX
$(echo_cmd) "CXX $<"
$(Q)$(CXX) $(CLIENT_CXXFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CXXFLAGS) -o $@ -c $<
endef

define DO_CC_ALTIVEC
$(echo_cmd) "CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) $(ALTIVEC_CFLAGS) -o $@ -c $<
endef

define DO_REF_CC
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) -o $@ -c $<
endef

define DO_THIRDPARTY_REF_CC
$(echo_cmd) "THIRDPARTY_REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(THIRDPARTY_CFLAGS) -o $@ -c $<
endef

define DO_REF_CC_ALTIVEC
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) $(ALTIVEC_CFLAGS) -o $@ -c $<
endef

define DO_REF_CC_SSE2
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) $(SSE2_CFLAGS) -o $@ -c $<
endef

define DO_REF_STR
$(echo_cmd) "REF_STR $<"
$(Q)rm -f $@
$(Q)$(STRINGIFY) $< $@
endef

define DO_BOT_CC
$(echo_cmd) "BOT_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(BOTCFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) -DBOTLIB -o $@ -c $<
endef

ifeq ($(GENERATE_DEPENDENCIES),1)
  DO_QVM_DEP=cat $(@:%.o=%.d) | sed -e 's/\.o/\.asm/g' >> $(@:%.o=%.d)
endif

define DO_SHLIB_CC
$(echo_cmd) "SHLIB_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC
$(echo_cmd) "GAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC
$(echo_cmd) "CGAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DCGAMESO -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC
$(echo_cmd) "UI_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DUI $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_SHLIB_CC_MISSIONPACK
$(echo_cmd) "SHLIB_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC_MISSIONPACK
$(echo_cmd) "GAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC_MISSIONPACK
$(echo_cmd) "CGAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC_MISSIONPACK
$(echo_cmd) "UI_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DUI $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) $(WARNINGS_CFLAGS) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_AS
$(echo_cmd) "AS $<"
$(Q)$(CC) $(CFLAGS) $(OPTIMIZE) -x assembler-with-cpp -o $@ -c $<
endef

define DO_DED_CC
$(echo_cmd) "DED_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) -DDEDICATED $(CFLAGS) $(SERVER_CFLAGS) $(OPTIMIZE) $(WARNINGS_CFLAGS) -o $@ -c $<
endef

define DO_THIRDPARTY_DED_CC
$(echo_cmd) "THIRDPARTY_DED_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) -DDEDICATED $(CFLAGS) $(SERVER_CFLAGS) $(OPTIMIZE) $(THIRDPARTY_CFLAGS) -o $@ -c $<
endef

define DO_WINDRES
$(echo_cmd) "WINDRES $<"
$(Q)$(WINDRES) -Imisc -i $< -o $@
endef


#############################################################################
# MAIN TARGETS
#############################################################################

default: release
all: debug release

debug:
	@$(MAKE) targets B=$(BD) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="$(DEBUG_CFLAGS)" OPTIMIZEVM="$(DEBUG_CFLAGS)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" CLIENT_CXXFLAGS="$(CLIENT_CXXFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V) \
	  LDFLAGS="$(LDFLAGS) $(DEBUG_LDFLAGS)"

release:
	@$(MAKE) targets B=$(BR) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="-DNQDEBUG $(OPTIMIZE)" OPTIMIZEVM="-DNQDEBUG $(OPTIMIZEVM)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" CLIENT_CXXFLAGS="$(CLIENT_CXXFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

ifneq ($(call bin_path, tput),)
  TERM_COLUMNS=$(shell if c=`tput cols`; then echo $$(($$c-4)); else echo 76; fi)
else
  TERM_COLUMNS=76
endif

define ADD_COPY_TARGET
TARGETS += $2
$2: $1
	$(echo_cmd) "CP $$<"
	@cp $1 $2
endef

# These functions allow us to generate rules for copying a list of files
# into the base directory of the build; this is useful for bundling libs,
# README files or whatever else
define GENERATE_COPY_TARGETS
$(foreach FILE,$1, \
  $(eval $(call ADD_COPY_TARGET, \
    $(FILE), \
    $(addprefix $(B)/,$(notdir $(FILE))))))
endef

$(call GENERATE_COPY_TARGETS,$(EXTRA_FILES))

ifneq ($(BUILD_CLIENT),0)
  $(call GENERATE_COPY_TARGETS,$(CLIENT_EXTRA_FILES))
endif

NAKED_TARGETS=$(shell echo $(TARGETS) | sed -e "s!$(B)/!!g")
NAKED_GENERATEDTARGETS=$(shell echo $(GENERATEDTARGETS) | sed -e "s!$(B)/!!g")

print_list=-@for i in $(1); \
     do \
             echo "    $$i"; \
     done

ifneq ($(call bin_path, fmt),)
  print_wrapped=@echo $(1) | fmt -w $(TERM_COLUMNS) | sed -e "s/^\(.*\)$$/    \1/"
else
  print_wrapped=$(print_list)
endif

# Create the build directories, check libraries and print out
# an informational message, then start building
targets: makedirs
	@echo ""
	@echo "Building in $(B):"
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  ARCH: $(ARCH)"
	@echo "  VERSION: $(VERSION)"
	@echo "  COMPILE_PLATFORM: $(COMPILE_PLATFORM)"
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  HAVE_VM_COMPILED: $(HAVE_VM_COMPILED)"
	@echo "  PKG_CONFIG: $(PKG_CONFIG)"
	@echo "  CC: $(CC)"
	@echo "  CC_VERSION: $(CC_VERSION)"
	@echo "  CXX: $(CXX)"
	@echo "  CXX_VERSION: $(CXX_VERSION)"
ifeq ($(PLATFORM),mingw32)
	@echo "  WINDRES: $(WINDRES)"
endif
	@echo ""
	@echo "  WARNINGS_CFLAGS:"
	$(call print_wrapped, $(WARNINGS_CFLAGS))
	@echo ""
	@echo "  WARNINGS_CXXFLAGS:"
	$(call print_wrapped, $(WARNINGS_CXXFLAGS))
	@echo ""
	@echo "  THIRDPARTY_CFLAGS:"
	$(call print_wrapped, $(THIRDPARTY_CFLAGS))
	@echo ""
	@echo "  CFLAGS:"
	$(call print_wrapped, $(CFLAGS) $(OPTIMIZE))
	@echo ""
	@echo "  CLIENT_CFLAGS:"
	$(call print_wrapped, $(CLIENT_CFLAGS))
	@echo ""
	@echo "  CLIENT_CXXFLAGS:"
	$(call print_wrapped, $(CLIENT_CXXFLAGS))
	@echo ""
	@echo "  SERVER_CFLAGS:"
	$(call print_wrapped, $(SERVER_CFLAGS))
	@echo ""
	@echo "  TOOLS_CFLAGS:"
	$(call print_wrapped, $(TOOLS_CFLAGS))
	@echo ""
	@echo "  LDFLAGS:"
	$(call print_wrapped, $(LDFLAGS))
	@echo ""
	@echo "  LIBS:"
	$(call print_wrapped, $(LIBS))
	@echo ""
	@echo "  CLIENT_LIBS:"
	$(call print_wrapped, $(CLIENT_LIBS))
	@echo ""
	@echo "  CLIENT_LDFLAGS:"
	$(call print_wrapped, $(CLIENT_LDFLAGS))
	@echo ""
	@echo "  AUTOUPDATER_LIBS:"
	$(call print_wrapped, $(AUTOUPDATER_LIBS))
	@echo ""
	@echo "  Output:"
	$(call print_list, $(NAKED_TARGETS))
	$(call print_list, $(NAKED_GENERATEDTARGETS))
	@echo ""
ifneq ($(TARGETS),)
  ifndef DEBUG_MAKEFILE
	@$(MAKE) $(TARGETS) $(B).zip V=$(V)
  endif
endif

$(B).zip: $(TARGETS)
ifeq ($(PLATFORM),darwin)
  ifdef ARCHIVE
	@("./make-macosx-app.sh" release $(ARCH); if [ "$$?" -eq 0 ] && [ -d "$(B)/ioquake3.app" ]; then rm -f $@; cd $(B) && zip --symlinks -r9 ../../$@ `find "ioquake3.app" -print | sed -e "s!$(B)/!!g"`; else rm -f $@; cd $(B) && zip -r9 ../../$@ $(NAKED_TARGETS); fi)
  endif
endif
ifneq ($(PLATFORM),darwin)
  ifdef ARCHIVE
	@rm -f $@
	@(cd $(B) && zip -r9 ../../$@ $(NAKED_TARGETS) $(NAKED_GENERATEDTARGETS))
  endif
endif
	@:

makedirs:
	@$(MKDIR) $(B)/autoupdater
	@$(MKDIR) $(B)/client/opus
	@$(MKDIR) $(B)/client/speex
	@$(MKDIR) $(B)/client/vorbis
	@$(MKDIR) $(B)/splines
	@$(MKDIR) $(B)/renderergl1
	@$(MKDIR) $(B)/renderergl2
	@$(MKDIR) $(B)/renderergl2/glsl
	@$(MKDIR) $(B)/ded
	@$(MKDIR) $(B)/$(BASEGAME)/cgame
	@$(MKDIR) $(B)/$(BASEGAME)/game
	@$(MKDIR) $(B)/$(BASEGAME)/ui
	@$(MKDIR) $(B)/$(BASEGAME)/qcommon
	@$(MKDIR) $(B)/$(BASEGAME)/vm
	@$(MKDIR) $(B)/$(MISSIONPACK)/cgame
	@$(MKDIR) $(B)/$(MISSIONPACK)/game
	@$(MKDIR) $(B)/$(MISSIONPACK)/ui
	@$(MKDIR) $(B)/$(MISSIONPACK)/qcommon
	@$(MKDIR) $(B)/$(MISSIONPACK)/vm
	@$(MKDIR) $(B)/tools/asm
	@$(MKDIR) $(B)/tools/etc
	@$(MKDIR) $(B)/tools/rcc
	@$(MKDIR) $(B)/tools/cpp
	@$(MKDIR) $(B)/tools/lburg

#############################################################################
# QVM BUILD TOOLS
#############################################################################

ifndef TOOLS_CC
  # A compiler which probably produces native binaries
  TOOLS_CC = gcc
endif

ifndef YACC
  YACC = yacc
endif

TOOLS_OPTIMIZE = -g -Wall -fno-strict-aliasing
TOOLS_CFLAGS += $(TOOLS_OPTIMIZE) \
                -DTEMPDIR=\"$(TEMPDIR)\" -DSYSTEM=\"\" \
                -I$(Q3LCCSRCDIR) \
                -I$(LBURGDIR)
TOOLS_LIBS =
TOOLS_LDFLAGS =

ifeq ($(GENERATE_DEPENDENCIES),1)
  TOOLS_CFLAGS += -MMD
endif

define DO_YACC
$(echo_cmd) "YACC $<"
$(Q)$(YACC) $<
$(Q)mv -f y.tab.c $@
endef

define DO_TOOLS_CC
$(echo_cmd) "TOOLS_CC $<"
$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) -o $@ -c $<
endef

define DO_TOOLS_CC_DAGCHECK
$(echo_cmd) "TOOLS_CC_DAGCHECK $<"
$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) -Wno-unused -o $@ -c $<
endef

LBURG       = $(B)/tools/lburg/lburg$(TOOLS_BINEXT)
DAGCHECK_C  = $(B)/tools/rcc/dagcheck.c
Q3RCC       = $(B)/tools/q3rcc$(TOOLS_BINEXT)
Q3CPP       = $(B)/tools/q3cpp$(TOOLS_BINEXT)
Q3LCC       = $(B)/tools/q3lcc$(TOOLS_BINEXT)
Q3ASM       = $(B)/tools/q3asm$(TOOLS_BINEXT)
STRINGIFY   = $(B)/tools/stringify$(TOOLS_BINEXT)

LBURGOBJ= \
  $(B)/tools/lburg/lburg.o \
  $(B)/tools/lburg/gram.o

# override GNU Make built-in rule for converting gram.y to gram.c
%.c: %.y
ifeq ($(USE_YACC),1)
	$(DO_YACC)
endif

$(B)/tools/lburg/%.o: $(LBURGDIR)/%.c
	$(DO_TOOLS_CC)

$(LBURG): $(LBURGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3RCCOBJ = \
  $(B)/tools/rcc/alloc.o \
  $(B)/tools/rcc/bind.o \
  $(B)/tools/rcc/bytecode.o \
  $(B)/tools/rcc/dag.o \
  $(B)/tools/rcc/dagcheck.o \
  $(B)/tools/rcc/decl.o \
  $(B)/tools/rcc/enode.o \
  $(B)/tools/rcc/error.o \
  $(B)/tools/rcc/event.o \
  $(B)/tools/rcc/expr.o \
  $(B)/tools/rcc/gen.o \
  $(B)/tools/rcc/init.o \
  $(B)/tools/rcc/inits.o \
  $(B)/tools/rcc/input.o \
  $(B)/tools/rcc/lex.o \
  $(B)/tools/rcc/list.o \
  $(B)/tools/rcc/main.o \
  $(B)/tools/rcc/null.o \
  $(B)/tools/rcc/output.o \
  $(B)/tools/rcc/prof.o \
  $(B)/tools/rcc/profio.o \
  $(B)/tools/rcc/simp.o \
  $(B)/tools/rcc/stmt.o \
  $(B)/tools/rcc/string.o \
  $(B)/tools/rcc/sym.o \
  $(B)/tools/rcc/symbolic.o \
  $(B)/tools/rcc/trace.o \
  $(B)/tools/rcc/tree.o \
  $(B)/tools/rcc/types.o

$(DAGCHECK_C): $(LBURG) $(Q3LCCSRCDIR)/dagcheck.md
	$(echo_cmd) "LBURG $(Q3LCCSRCDIR)/dagcheck.md"
	$(Q)$(LBURG) $(Q3LCCSRCDIR)/dagcheck.md $@

$(B)/tools/rcc/dagcheck.o: $(DAGCHECK_C)
	$(DO_TOOLS_CC_DAGCHECK)

$(B)/tools/rcc/%.o: $(Q3LCCSRCDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3RCC): $(Q3RCCOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3CPPOBJ = \
  $(B)/tools/cpp/cpp.o \
  $(B)/tools/cpp/lex.o \
  $(B)/tools/cpp/nlist.o \
  $(B)/tools/cpp/tokens.o \
  $(B)/tools/cpp/macro.o \
  $(B)/tools/cpp/eval.o \
  $(B)/tools/cpp/include.o \
  $(B)/tools/cpp/hideset.o \
  $(B)/tools/cpp/getopt.o \
  $(B)/tools/cpp/unix.o

$(B)/tools/cpp/%.o: $(Q3CPPDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3CPP): $(Q3CPPOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3LCCOBJ = \
	$(B)/tools/etc/lcc.o \
	$(B)/tools/etc/bytecode.o

$(B)/tools/etc/%.o: $(Q3LCCETCDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3LCC): $(Q3LCCOBJ) $(Q3RCC) $(Q3CPP)
	$(echo_cmd) "LD $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $(Q3LCCOBJ) $(TOOLS_LIBS)

$(STRINGIFY): $(TOOLSDIR)/stringify.c
	$(echo_cmd) "TOOLS_CC $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $(TOOLSDIR)/stringify.c $(TOOLS_LIBS)

QVM_CFLAGS = -DPRODUCT_VERSION=\"$(VERSION)\"  -DWOLFCAM_VERSION=\"$(VERSION)\"

define DO_Q3LCC
$(echo_cmd) "Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -o $@ $<
endef

define DO_CGAME_Q3LCC
$(echo_cmd) "CGAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DCGAME $(QVM_CFLAGS) -o $@ $<
endef

define DO_GAME_Q3LCC
$(echo_cmd) "GAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DQAGAME $(QVM_CFLAGS) -o $@ $<
endef

define DO_UI_Q3LCC
$(echo_cmd) "UI_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DUI $(QVM_CFLAGS) -o $@ $<
endef

define DO_Q3LCC_MISSIONPACK
$(echo_cmd) "Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK $(QVM_CFLAGS) -o $@ $<
endef

define DO_CGAME_Q3LCC_MISSIONPACK
$(echo_cmd) "CGAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DCGAME $(QVM_CFLAGS) -o $@ $<
endef

define DO_GAME_Q3LCC_MISSIONPACK
$(echo_cmd) "GAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DQAGAME $(QVM_CFLAGS) -o $@ $<
endef

define DO_UI_Q3LCC_MISSIONPACK
$(echo_cmd) "UI_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DMISSIONPACK -DUI $(QVM_CFLAGS) -o $@ $<
endef


Q3ASMOBJ = \
  $(B)/tools/asm/q3asm.o \
  $(B)/tools/asm/cmdlib.o

$(B)/tools/asm/%.o: $(Q3ASMDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3ASM): $(Q3ASMOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(TOOLS_CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)


#############################################################################
# AUTOUPDATER
#############################################################################

define DO_AUTOUPDATER_CC
$(echo_cmd) "AUTOUPDATER_CC $<"
$(Q)$(CC) $(CFLAGS) -I$(LIBTOMCRYPTSRCDIR)/src/headers -I$(TOMSFASTMATHSRCDIR)/src/headers $(CURL_CFLAGS) -o $@ -c $<
endef

Q3AUTOUPDATEROBJ = \
  $(B)/autoupdater/autoupdater.o

$(B)/autoupdater/%.o: $(AUTOUPDATERSRCDIR)/%.c
	$(DO_AUTOUPDATER_CC)

$(B)/$(AUTOUPDATER_BIN): $(Q3AUTOUPDATEROBJ)
	$(echo_cmd) "AUTOUPDATER_LD $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $(Q3AUTOUPDATEROBJ) $(AUTOUPDATER_LIBS)


#############################################################################
# CLIENT/SERVER
#############################################################################

Q3OBJ = \
  $(B)/client/cl_avi.o \
  $(B)/client/cl_camera.o \
  $(B)/client/cl_cgame.o \
  $(B)/client/cl_cin.o \
  $(B)/client/cl_console.o \
  $(B)/client/cl_input.o \
  $(B)/client/cl_huffyuv.o \
  $(B)/client/cl_keys.o \
  $(B)/client/cl_main.o \
  $(B)/client/cl_net_chan.o \
  $(B)/client/cl_parse.o \
  $(B)/client/cl_scrn.o \
  $(B)/client/cl_ui.o \
  \
  $(B)/client/cm_load.o \
  $(B)/client/cm_patch.o \
  $(B)/client/cm_polylib.o \
  $(B)/client/cm_test.o \
  $(B)/client/cm_trace.o \
  \
  $(B)/client/cmd.o \
  $(B)/client/common.o \
  $(B)/client/cvar.o \
  $(B)/client/files.o \
  $(B)/client/md4.o \
  $(B)/client/md5.o \
  $(B)/client/msg.o \
  $(B)/client/net_chan.o \
  $(B)/client/net_ip.o \
  $(B)/client/huffman.o \
  \
  $(B)/client/snd_altivec.o \
  $(B)/client/snd_adpcm.o \
  $(B)/client/snd_dma.o \
  $(B)/client/snd_mem.o \
  $(B)/client/snd_mix.o \
  $(B)/client/snd_wavelet.o \
  \
  $(B)/client/snd_main.o \
  $(B)/client/snd_codec.o \
  $(B)/client/snd_codec_wav.o \
  $(B)/client/snd_codec_ogg.o \
  $(B)/client/snd_codec_opus.o \
  \
  $(B)/client/qal.o \
  $(B)/client/snd_openal.o \
  \
  $(B)/client/sv_bot.o \
  $(B)/client/sv_ccmds.o \
  $(B)/client/sv_client.o \
  $(B)/client/sv_game.o \
  $(B)/client/sv_init.o \
  $(B)/client/sv_main.o \
  $(B)/client/sv_net_chan.o \
  $(B)/client/sv_snapshot.o \
  $(B)/client/sv_world.o \
  \
  $(B)/client/q_math.o \
  $(B)/client/q_shared.o \
  \
  $(B)/client/unzip.o \
  $(B)/client/ioapi.o \
  $(B)/client/vm.o \
  $(B)/client/vm_interpreted.o \
  \
  $(B)/client/be_aas_bspq3.o \
  $(B)/client/be_aas_cluster.o \
  $(B)/client/be_aas_debug.o \
  $(B)/client/be_aas_entity.o \
  $(B)/client/be_aas_file.o \
  $(B)/client/be_aas_main.o \
  $(B)/client/be_aas_move.o \
  $(B)/client/be_aas_optimize.o \
  $(B)/client/be_aas_reach.o \
  $(B)/client/be_aas_route.o \
  $(B)/client/be_aas_routealt.o \
  $(B)/client/be_aas_sample.o \
  $(B)/client/be_ai_char.o \
  $(B)/client/be_ai_chat.o \
  $(B)/client/be_ai_gen.o \
  $(B)/client/be_ai_goal.o \
  $(B)/client/be_ai_move.o \
  $(B)/client/be_ai_weap.o \
  $(B)/client/be_ai_weight.o \
  $(B)/client/be_ea.o \
  $(B)/client/be_interface.o \
  $(B)/client/l_crc.o \
  $(B)/client/l_libvar.o \
  $(B)/client/l_log.o \
  $(B)/client/l_memory.o \
  $(B)/client/l_precomp.o \
  $(B)/client/l_script.o \
  $(B)/client/l_struct.o \
  \
  $(B)/client/sdl_input.o \
  $(B)/client/sdl_snd.o \
  \
  $(B)/client/con_log.o \
  $(B)/client/sys_autoupdater.o \
  $(B)/client/sys_main.o \
  $(B)/client/bg_misc.o

ifdef MINGW
  Q3OBJ += \
    $(B)/client/cl_http_windows.o
else
  Q3OBJ += \
    $(B)/client/cl_http_curl.o
endif

ifdef MINGW
  Q3OBJ += \
    $(B)/client/con_passive.o
else
ifeq ($(PLATFORM),emscripten)
  Q3OBJ += \
    $(B)/client/con_passive.o
else
  Q3OBJ += \
    $(B)/client/con_tty.o
endif
endif

Q3R2OBJ = \
  $(B)/renderergl2/tr_animation.o \
  $(B)/renderergl2/tr_backend.o \
  $(B)/renderergl2/tr_bsp.o \
  $(B)/renderergl2/tr_cmds.o \
  $(B)/renderergl2/tr_curve.o \
  $(B)/renderergl2/tr_dsa.o \
  $(B)/renderergl2/tr_extramath.o \
  $(B)/renderergl2/tr_extensions.o \
  $(B)/renderergl2/tr_fbo.o \
  $(B)/renderergl2/tr_flares.o \
  $(B)/renderergl2/tr_font.o \
  $(B)/renderergl2/tr_glsl.o \
  $(B)/renderergl2/tr_image.o \
  $(B)/renderergl2/tr_image_bmp.o \
  $(B)/renderergl2/tr_image_jpg.o \
  $(B)/renderergl2/tr_image_pcx.o \
  $(B)/renderergl2/tr_image_png.o \
  $(B)/renderergl2/tr_image_tga.o \
  $(B)/renderergl2/tr_image_dds.o \
  $(B)/renderergl2/tr_init.o \
  $(B)/renderergl2/tr_light.o \
  $(B)/renderergl2/tr_main.o \
  $(B)/renderergl2/tr_marks.o \
  $(B)/renderergl2/tr_mesh.o \
  $(B)/renderergl2/tr_mme.o \
  $(B)/renderergl2/tr_mme_common.o \
  $(B)/renderergl2/tr_mme_sse2.o \
  $(B)/renderergl2/tr_model.o \
  $(B)/renderergl2/tr_model_iqm.o \
  $(B)/renderergl2/tr_noise.o \
  $(B)/renderergl2/tr_postprocess.o \
  $(B)/renderergl2/tr_scene.o \
  $(B)/renderergl2/tr_shade.o \
  $(B)/renderergl2/tr_shade_calc.o \
  $(B)/renderergl2/tr_shader.o \
  $(B)/renderergl2/tr_shadows.o \
  $(B)/renderergl2/tr_sky.o \
  $(B)/renderergl2/tr_surface.o \
  $(B)/renderergl2/tr_vbo.o \
  $(B)/renderergl2/tr_world.o \
  \
  $(B)/renderergl1/sdl_gamma.o \
  $(B)/renderergl1/sdl_glimp.o \
  \
  $(B)/renderergl2/puff.o

Q3R2STRINGOBJ = \
  $(B)/renderergl2/glsl/bokeh_fp.o \
  $(B)/renderergl2/glsl/bokeh_vp.o \
  $(B)/renderergl2/glsl/calclevels4x_fp.o \
  $(B)/renderergl2/glsl/calclevels4x_vp.o \
  $(B)/renderergl2/glsl/depthblur_fp.o \
  $(B)/renderergl2/glsl/depthblur_vp.o \
  $(B)/renderergl2/glsl/dlight_fp.o \
  $(B)/renderergl2/glsl/dlight_vp.o \
  $(B)/renderergl2/glsl/down4x_fp.o \
  $(B)/renderergl2/glsl/down4x_vp.o \
  $(B)/renderergl2/glsl/fogpass_fp.o \
  $(B)/renderergl2/glsl/fogpass_vp.o \
  $(B)/renderergl2/glsl/generic_fp.o \
  $(B)/renderergl2/glsl/generic_vp.o \
  $(B)/renderergl2/glsl/lightall_fp.o \
  $(B)/renderergl2/glsl/lightall_vp.o \
  $(B)/renderergl2/glsl/pshadow_fp.o \
  $(B)/renderergl2/glsl/pshadow_vp.o \
  $(B)/renderergl2/glsl/rectscreen_fp.o \
  $(B)/renderergl2/glsl/rectscreen_vp.o \
  $(B)/renderergl2/glsl/shadowfill_fp.o \
  $(B)/renderergl2/glsl/shadowfill_vp.o \
  $(B)/renderergl2/glsl/shadowmask_fp.o \
  $(B)/renderergl2/glsl/shadowmask_vp.o \
  $(B)/renderergl2/glsl/ssao_fp.o \
  $(B)/renderergl2/glsl/ssao_vp.o \
  $(B)/renderergl2/glsl/texturecolor_fp.o \
  $(B)/renderergl2/glsl/texturecolor_vp.o \
  $(B)/renderergl2/glsl/texturenocolor_fp.o \
  $(B)/renderergl2/glsl/texturenocolor_vp.o \
  $(B)/renderergl2/glsl/tonemap_fp.o \
  $(B)/renderergl2/glsl/tonemap_vp.o

Q3ROBJ = \
  $(B)/renderergl1/tr_altivec.o \
  $(B)/renderergl1/tr_animation.o \
  $(B)/renderergl1/tr_backend.o \
  $(B)/renderergl1/tr_bsp.o \
  $(B)/renderergl1/tr_cmds.o \
  $(B)/renderergl1/tr_curve.o \
  $(B)/renderergl1/tr_flares.o \
  $(B)/renderergl1/tr_font.o \
  $(B)/renderergl1/tr_image.o \
  $(B)/renderergl1/tr_image_bmp.o \
  $(B)/renderergl1/tr_image_jpg.o \
  $(B)/renderergl1/tr_image_pcx.o \
  $(B)/renderergl1/tr_image_png.o \
  $(B)/renderergl1/tr_image_tga.o \
  $(B)/renderergl1/tr_init.o \
  $(B)/renderergl1/tr_light.o \
  $(B)/renderergl1/tr_main.o \
  $(B)/renderergl1/tr_marks.o \
  $(B)/renderergl1/tr_mesh.o \
  $(B)/renderergl1/tr_mme.o \
  $(B)/renderergl1/tr_mme_common.o \
  $(B)/renderergl1/tr_mme_sse2.o \
  $(B)/renderergl1/tr_model.o \
  $(B)/renderergl1/tr_model_iqm.o \
  $(B)/renderergl1/tr_noise.o \
  $(B)/renderergl1/tr_scene.o \
  $(B)/renderergl1/tr_shade.o \
  $(B)/renderergl1/tr_shade_calc.o \
  $(B)/renderergl1/tr_shader.o \
  $(B)/renderergl1/tr_shadows.o \
  $(B)/renderergl1/tr_sky.o \
  $(B)/renderergl1/tr_surface.o \
  $(B)/renderergl1/tr_world.o \
  \
  $(B)/renderergl1/sdl_gamma.o \
  $(B)/renderergl1/sdl_glimp.o \
  \
  $(B)/renderergl1/puff.o

ifneq ($(USE_RENDERER_DLOPEN), 0)
  Q3ROBJ += \
    $(B)/renderergl1/q_shared.o \
    $(B)/renderergl1/q_math.o \
    $(B)/renderergl1/tr_subs.o

  Q3R2OBJ += \
    $(B)/renderergl2/q_shared.o \
    $(B)/renderergl2/q_math.o \
    $(B)/renderergl2/tr_subs.o
endif

ifneq ($(USE_INTERNAL_JPEG),0)
  JPGOBJ = \
    $(B)/renderergl1/jaricom.o \
    $(B)/renderergl1/jcapimin.o \
    $(B)/renderergl1/jcapistd.o \
    $(B)/renderergl1/jcarith.o \
    $(B)/renderergl1/jccoefct.o  \
    $(B)/renderergl1/jccolor.o \
    $(B)/renderergl1/jcdctmgr.o \
    $(B)/renderergl1/jchuff.o   \
    $(B)/renderergl1/jcinit.o \
    $(B)/renderergl1/jcmainct.o \
    $(B)/renderergl1/jcmarker.o \
    $(B)/renderergl1/jcmaster.o \
    $(B)/renderergl1/jcomapi.o \
    $(B)/renderergl1/jcparam.o \
    $(B)/renderergl1/jcprepct.o \
    $(B)/renderergl1/jcsample.o \
    $(B)/renderergl1/jctrans.o \
    $(B)/renderergl1/jdapimin.o \
    $(B)/renderergl1/jdapistd.o \
    $(B)/renderergl1/jdarith.o \
    $(B)/renderergl1/jdatadst.o \
    $(B)/renderergl1/jdatasrc.o \
    $(B)/renderergl1/jdcoefct.o \
    $(B)/renderergl1/jdcolor.o \
    $(B)/renderergl1/jddctmgr.o \
    $(B)/renderergl1/jdhuff.o \
    $(B)/renderergl1/jdinput.o \
    $(B)/renderergl1/jdmainct.o \
    $(B)/renderergl1/jdmarker.o \
    $(B)/renderergl1/jdmaster.o \
    $(B)/renderergl1/jdmerge.o \
    $(B)/renderergl1/jdpostct.o \
    $(B)/renderergl1/jdsample.o \
    $(B)/renderergl1/jdtrans.o \
    $(B)/renderergl1/jerror.o \
    $(B)/renderergl1/jfdctflt.o \
    $(B)/renderergl1/jfdctfst.o \
    $(B)/renderergl1/jfdctint.o \
    $(B)/renderergl1/jidctflt.o \
    $(B)/renderergl1/jidctfst.o \
    $(B)/renderergl1/jidctint.o \
    $(B)/renderergl1/jmemmgr.o \
    $(B)/renderergl1/jmemnobs.o \
    $(B)/renderergl1/jquant1.o \
    $(B)/renderergl1/jquant2.o \
    $(B)/renderergl1/jutils.o
endif

ifneq ($(USE_INTERNAL_ZLIB),0)
  Q3ROBJ += \
    $(B)/renderergl1/adler32.o \
    $(B)/renderergl1/crc32.o \
    $(B)/renderergl1/deflate.o \
    $(B)/renderergl1/infback.o \
    $(B)/renderergl1/inffast.o \
    $(B)/renderergl1/inflate.o \
    $(B)/renderergl1/inftrees.o \
    $(B)/renderergl1/trees.o \
    $(B)/renderergl1/zutil.o

  Q3R2OBJ += \
    $(B)/renderergl2/adler32.o \
    $(B)/renderergl2/crc32.o \
    $(B)/renderergl2/deflate.o \
    $(B)/renderergl2/infback.o \
    $(B)/renderergl2/inffast.o \
    $(B)/renderergl2/inflate.o \
    $(B)/renderergl2/inftrees.o \
    $(B)/renderergl2/trees.o \
    $(B)/renderergl2/zutil.o
endif

ifeq ($(ARCH),x86)
  Q3OBJ += \
    $(B)/client/snd_mixa.o \
    $(B)/client/matha.o \
    $(B)/client/snapvector.o \
    $(B)/client/ftola.o
endif
ifeq ($(ARCH),x86_64)
  Q3OBJ += \
    $(B)/client/snd_mixa.o \
    $(B)/client/ftola.o \
    $(B)/client/snapvector.o
endif

SPLINES += \
  $(B)/splines/math_angles.o \
  $(B)/splines/math_matrix.o \
  $(B)/splines/math_quaternion.o \
  $(B)/splines/math_vector.o \
  $(B)/splines/q_parse.o \
  $(B)/splines/splines.o \
  $(B)/splines/util_str.o

ifeq ($(USE_VOIP),1)
ifeq ($(USE_INTERNAL_SPEEX),1)
Q3OBJ += \
  $(B)/client/speex/bits.o \
  $(B)/client/speex/buffer.o \
  $(B)/client/speex/cb_search.o \
  $(B)/client/speex/exc_10_16_table.o \
  $(B)/client/speex/exc_10_32_table.o \
  $(B)/client/speex/exc_20_32_table.o \
  $(B)/client/speex/exc_5_256_table.o \
  $(B)/client/speex/exc_5_64_table.o \
  $(B)/client/speex/exc_8_128_table.o \
  $(B)/client/speex/fftwrap.o \
  $(B)/client/speex/filterbank.o \
  $(B)/client/speex/filters.o \
  $(B)/client/speex/gain_table.o \
  $(B)/client/speex/gain_table_lbr.o \
  $(B)/client/speex/hexc_10_32_table.o \
  $(B)/client/speex/hexc_table.o \
  $(B)/client/speex/high_lsp_tables.o \
  $(B)/client/speex/jitter.o \
  $(B)/client/speex/kiss_fft.o \
  $(B)/client/speex/kiss_fftr.o \
  $(B)/client/speex/lpc.o \
  $(B)/client/speex/lsp.o \
  $(B)/client/speex/lsp_tables_nb.o \
  $(B)/client/speex/ltp.o \
  $(B)/client/speex/mdf.o \
  $(B)/client/speex/modes.o \
  $(B)/client/speex/modes_wb.o \
  $(B)/client/speex/nb_celp.o \
  $(B)/client/speex/preprocess.o \
  $(B)/client/speex/quant_lsp.o \
  $(B)/client/speex/resample.o \
  $(B)/client/speex/sb_celp.o \
  $(B)/client/speex/scal.o \
  $(B)/client/speex/smallft.o \
  $(B)/client/speex/speex.o \
  $(B)/client/speex/speex_callbacks.o \
  $(B)/client/speex/speex_header.o \
  $(B)/client/speex/stereo.o \
  $(B)/client/speex/vbr.o \
  $(B)/client/speex/vorbis_psy.o \
  $(B)/client/speex/vq.o \
  $(B)/client/speex/window.o
endif
endif

ifeq ($(NEED_OPUS),1)
ifeq ($(USE_INTERNAL_OPUS),1)
Q3OBJ += \
  $(B)/client/opus/analysis.o \
  $(B)/client/opus/mlp.o \
  $(B)/client/opus/mlp_data.o \
  $(B)/client/opus/opus.o \
  $(B)/client/opus/opus_decoder.o \
  $(B)/client/opus/opus_encoder.o \
  $(B)/client/opus/opus_multistream.o \
  $(B)/client/opus/opus_multistream_encoder.o \
  $(B)/client/opus/opus_multistream_decoder.o \
  $(B)/client/opus/repacketizer.o \
  $(B)/client/opus/extensions.o \
  \
  $(B)/client/opus/bands.o \
  $(B)/client/opus/celt.o \
  $(B)/client/opus/cwrs.o \
  $(B)/client/opus/entcode.o \
  $(B)/client/opus/entdec.o \
  $(B)/client/opus/entenc.o \
  $(B)/client/opus/kiss_fft.o \
  $(B)/client/opus/laplace.o \
  $(B)/client/opus/mathops.o \
  $(B)/client/opus/mdct.o \
  $(B)/client/opus/modes.o \
  $(B)/client/opus/pitch.o \
  $(B)/client/opus/celt_encoder.o \
  $(B)/client/opus/celt_decoder.o \
  $(B)/client/opus/celt_lpc.o \
  $(B)/client/opus/quant_bands.o \
  $(B)/client/opus/rate.o \
  $(B)/client/opus/vq.o \
  \
  $(B)/client/opus/CNG.o \
  $(B)/client/opus/code_signs.o \
  $(B)/client/opus/init_decoder.o \
  $(B)/client/opus/decode_core.o \
  $(B)/client/opus/decode_frame.o \
  $(B)/client/opus/decode_parameters.o \
  $(B)/client/opus/decode_indices.o \
  $(B)/client/opus/decode_pulses.o \
  $(B)/client/opus/decoder_set_fs.o \
  $(B)/client/opus/dec_API.o \
  $(B)/client/opus/enc_API.o \
  $(B)/client/opus/encode_indices.o \
  $(B)/client/opus/encode_pulses.o \
  $(B)/client/opus/gain_quant.o \
  $(B)/client/opus/interpolate.o \
  $(B)/client/opus/LP_variable_cutoff.o \
  $(B)/client/opus/NLSF_decode.o \
  $(B)/client/opus/NSQ.o \
  $(B)/client/opus/NSQ_del_dec.o \
  $(B)/client/opus/PLC.o \
  $(B)/client/opus/shell_coder.o \
  $(B)/client/opus/tables_gain.o \
  $(B)/client/opus/tables_LTP.o \
  $(B)/client/opus/tables_NLSF_CB_NB_MB.o \
  $(B)/client/opus/tables_NLSF_CB_WB.o \
  $(B)/client/opus/tables_other.o \
  $(B)/client/opus/tables_pitch_lag.o \
  $(B)/client/opus/tables_pulses_per_block.o \
  $(B)/client/opus/VAD.o \
  $(B)/client/opus/control_audio_bandwidth.o \
  $(B)/client/opus/quant_LTP_gains.o \
  $(B)/client/opus/VQ_WMat_EC.o \
  $(B)/client/opus/HP_variable_cutoff.o \
  $(B)/client/opus/NLSF_encode.o \
  $(B)/client/opus/NLSF_VQ.o \
  $(B)/client/opus/NLSF_unpack.o \
  $(B)/client/opus/NLSF_del_dec_quant.o \
  $(B)/client/opus/process_NLSFs.o \
  $(B)/client/opus/stereo_LR_to_MS.o \
  $(B)/client/opus/stereo_MS_to_LR.o \
  $(B)/client/opus/check_control_input.o \
  $(B)/client/opus/control_SNR.o \
  $(B)/client/opus/init_encoder.o \
  $(B)/client/opus/control_codec.o \
  $(B)/client/opus/A2NLSF.o \
  $(B)/client/opus/ana_filt_bank_1.o \
  $(B)/client/opus/biquad_alt.o \
  $(B)/client/opus/bwexpander_32.o \
  $(B)/client/opus/bwexpander.o \
  $(B)/client/opus/debug.o \
  $(B)/client/opus/decode_pitch.o \
  $(B)/client/opus/inner_prod_aligned.o \
  $(B)/client/opus/lin2log.o \
  $(B)/client/opus/log2lin.o \
  $(B)/client/opus/LPC_analysis_filter.o \
  $(B)/client/opus/LPC_fit.o \
  $(B)/client/opus/LPC_inv_pred_gain.o \
  $(B)/client/opus/table_LSF_cos.o \
  $(B)/client/opus/NLSF2A.o \
  $(B)/client/opus/NLSF_stabilize.o \
  $(B)/client/opus/NLSF_VQ_weights_laroia.o \
  $(B)/client/opus/pitch_est_tables.o \
  $(B)/client/opus/resampler.o \
  $(B)/client/opus/resampler_down2_3.o \
  $(B)/client/opus/resampler_down2.o \
  $(B)/client/opus/resampler_private_AR2.o \
  $(B)/client/opus/resampler_private_down_FIR.o \
  $(B)/client/opus/resampler_private_IIR_FIR.o \
  $(B)/client/opus/resampler_private_up2_HQ.o \
  $(B)/client/opus/resampler_rom.o \
  $(B)/client/opus/sigm_Q15.o \
  $(B)/client/opus/sort.o \
  $(B)/client/opus/sum_sqr_shift.o \
  $(B)/client/opus/stereo_decode_pred.o \
  $(B)/client/opus/stereo_encode_pred.o \
  $(B)/client/opus/stereo_find_predictor.o \
  $(B)/client/opus/stereo_quant_pred.o \
  \
  $(B)/client/opus/apply_sine_window_FLP.o \
  $(B)/client/opus/corrMatrix_FLP.o \
  $(B)/client/opus/encode_frame_FLP.o \
  $(B)/client/opus/find_LPC_FLP.o \
  $(B)/client/opus/find_LTP_FLP.o \
  $(B)/client/opus/find_pitch_lags_FLP.o \
  $(B)/client/opus/find_pred_coefs_FLP.o \
  $(B)/client/opus/LPC_analysis_filter_FLP.o \
  $(B)/client/opus/LTP_analysis_filter_FLP.o \
  $(B)/client/opus/LTP_scale_ctrl_FLP.o \
  $(B)/client/opus/noise_shape_analysis_FLP.o \
  $(B)/client/opus/process_gains_FLP.o \
  $(B)/client/opus/regularize_correlations_FLP.o \
  $(B)/client/opus/residual_energy_FLP.o \
  $(B)/client/opus/warped_autocorrelation_FLP.o \
  $(B)/client/opus/wrappers_FLP.o \
  $(B)/client/opus/autocorrelation_FLP.o \
  $(B)/client/opus/burg_modified_FLP.o \
  $(B)/client/opus/bwexpander_FLP.o \
  $(B)/client/opus/energy_FLP.o \
  $(B)/client/opus/inner_product_FLP.o \
  $(B)/client/opus/k2a_FLP.o \
  $(B)/client/opus/LPC_inv_pred_gain_FLP.o \
  $(B)/client/opus/pitch_analysis_core_FLP.o \
  $(B)/client/opus/scale_copy_vector_FLP.o \
  $(B)/client/opus/scale_vector_FLP.o \
  $(B)/client/opus/schur_FLP.o \
  $(B)/client/opus/sort_FLP.o \
  \
  $(B)/client/http.o \
  $(B)/client/info.o \
  $(B)/client/internal.o \
  $(B)/client/opusfile.o \
  $(B)/client/stream.o \
  $(B)/client/wincerts.o
endif
endif

ifeq ($(NEED_OGG),1)
ifeq ($(USE_INTERNAL_OGG),1)
Q3OBJ += \
  $(B)/client/bitwise.o \
  $(B)/client/framing.o
endif
endif

ifeq ($(USE_CODEC_VORBIS),1)
ifeq ($(USE_INTERNAL_VORBIS),1)
Q3OBJ += \
  $(B)/client/vorbis/analysis.o \
  $(B)/client/vorbis/bitrate.o \
  $(B)/client/vorbis/block.o \
  $(B)/client/vorbis/codebook.o \
  $(B)/client/vorbis/envelope.o \
  $(B)/client/vorbis/floor0.o \
  $(B)/client/vorbis/floor1.o \
  $(B)/client/vorbis/info.o \
  $(B)/client/vorbis/lookup.o \
  $(B)/client/vorbis/lpc.o \
  $(B)/client/vorbis/lsp.o \
  $(B)/client/vorbis/mapping0.o \
  $(B)/client/vorbis/mdct.o \
  $(B)/client/vorbis/psy.o \
  $(B)/client/vorbis/registry.o \
  $(B)/client/vorbis/res0.o \
  $(B)/client/vorbis/smallft.o \
  $(B)/client/vorbis/sharedbook.o \
  $(B)/client/vorbis/synthesis.o \
  $(B)/client/vorbis/vorbisfile.o \
  $(B)/client/vorbis/window.o
endif
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
Q3OBJ += \
  $(B)/client/adler32.o \
  $(B)/client/crc32.o \
  $(B)/client/deflate.o \
  $(B)/client/infback.o \
  $(B)/client/inffast.o \
  $(B)/client/inflate.o \
  $(B)/client/inftrees.o \
  $(B)/client/trees.o \
  $(B)/client/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifneq ($(findstring $(ARCH),x86 x86_64),)
    Q3OBJ += \
      $(B)/client/vm_x86.o
  endif
  ifneq ($(findstring $(ARCH),ppc ppc64),)
    Q3OBJ += $(B)/client/vm_powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3OBJ += $(B)/client/vm_sparc.o
  endif
  ifeq ($(ARCH),armv7l)
    Q3OBJ += $(B)/client/vm_armv7l.o
  endif
endif

ifdef MINGW
  Q3OBJ += \
    $(B)/client/win_resource.o \
    $(B)/client/sys_win32.o
else
  Q3OBJ += \
    $(B)/client/sys_unix.o
endif

ifeq ($(PLATFORM),darwin)
  Q3OBJ += \
    $(B)/client/sys_osx.o
endif

ifeq ($(USE_MUMBLE),1)
  Q3OBJ += \
    $(B)/client/libmumblelink.o
endif

ifdef CGAME_HARD_LINKED

else

ifneq ($(USE_RENDERER_DLOPEN),0)
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(SPLINES) $(LIBSDLMAIN) $(EXTRACLIENTBINDEPS)
	$(echo_cmd) "LD $@"
	$(Q)$(CXX) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		$(NOTSHLIBLDFLAGS) -o $@ $(Q3OBJ) $(SPLINES) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(LIBS)

$(B)/renderer_opengl1$(RSHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_opengl2$(RSHLIBNAME): $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

else
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(Q3ROBJ) $(JPGOBJ) $(SPLINES) $(LIBSDLMAIN) $(EXTRACLIENTBINDEPS)
	$(echo_cmd) "LD $@"
	$(Q)$(CXX) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		$(NOTSHLIBLDFLAGS) -o $@ $(Q3OBJ) $(Q3ROBJ) $(JPGOBJ) $(SPLINES) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/$(CLIENTBIN)_opengl2$(FULLBINEXT): $(Q3OBJ) $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ) $(SPLINES) $(LIBSDLMAIN)
	$(echo_cmd) "LD $@"
	$(Q)$(CXX) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		$(NOTSHLIBLDFLAGS) -o $@ $(Q3OBJ) $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ) $(SPLINES) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)

endif
endif

ifneq ($(strip $(LIBSDLMAIN)),)
ifneq ($(strip $(LIBSDLMAINSRC)),)
$(LIBSDLMAIN) : $(LIBSDLMAINSRC)
ifeq ($(PLATFORM),darwin)
	$(LIPO) -extract $(MACOSX_ARCH) $< -o $@
else
	cp $< $@
endif
	$(RANLIB) $@
endif
endif



#############################################################################
# DEDICATED SERVER
#############################################################################

Q3DOBJ = \
  $(B)/ded/sv_bot.o \
  $(B)/ded/sv_client.o \
  $(B)/ded/sv_ccmds.o \
  $(B)/ded/sv_game.o \
  $(B)/ded/sv_init.o \
  $(B)/ded/sv_main.o \
  $(B)/ded/sv_net_chan.o \
  $(B)/ded/sv_snapshot.o \
  $(B)/ded/sv_world.o \
  \
  $(B)/ded/cm_load.o \
  $(B)/ded/cm_patch.o \
  $(B)/ded/cm_polylib.o \
  $(B)/ded/cm_test.o \
  $(B)/ded/cm_trace.o \
  $(B)/ded/cmd.o \
  $(B)/ded/common.o \
  $(B)/ded/cvar.o \
  $(B)/ded/files.o \
  $(B)/ded/md4.o \
  $(B)/ded/msg.o \
  $(B)/ded/net_chan.o \
  $(B)/ded/net_ip.o \
  $(B)/ded/huffman.o \
  \
  $(B)/ded/q_math.o \
  $(B)/ded/q_shared.o \
  \
  $(B)/ded/unzip.o \
  $(B)/ded/ioapi.o \
  $(B)/ded/vm.o \
  $(B)/ded/vm_interpreted.o \
  \
  $(B)/ded/be_aas_bspq3.o \
  $(B)/ded/be_aas_cluster.o \
  $(B)/ded/be_aas_debug.o \
  $(B)/ded/be_aas_entity.o \
  $(B)/ded/be_aas_file.o \
  $(B)/ded/be_aas_main.o \
  $(B)/ded/be_aas_move.o \
  $(B)/ded/be_aas_optimize.o \
  $(B)/ded/be_aas_reach.o \
  $(B)/ded/be_aas_route.o \
  $(B)/ded/be_aas_routealt.o \
  $(B)/ded/be_aas_sample.o \
  $(B)/ded/be_ai_char.o \
  $(B)/ded/be_ai_chat.o \
  $(B)/ded/be_ai_gen.o \
  $(B)/ded/be_ai_goal.o \
  $(B)/ded/be_ai_move.o \
  $(B)/ded/be_ai_weap.o \
  $(B)/ded/be_ai_weight.o \
  $(B)/ded/be_ea.o \
  $(B)/ded/be_interface.o \
  $(B)/ded/l_crc.o \
  $(B)/ded/l_libvar.o \
  $(B)/ded/l_log.o \
  $(B)/ded/l_memory.o \
  $(B)/ded/l_precomp.o \
  $(B)/ded/l_script.o \
  $(B)/ded/l_struct.o \
  \
  $(B)/ded/null_client.o \
  $(B)/ded/null_input.o \
  $(B)/ded/null_snddma.o \
  \
  $(B)/ded/con_log.o \
  $(B)/ded/sys_autoupdater.o \
  $(B)/ded/sys_main.o

ifeq ($(ARCH),x86)
  Q3DOBJ += \
      $(B)/ded/matha.o \
      $(B)/ded/snapvector.o \
      $(B)/ded/ftola.o
endif
ifeq ($(ARCH),x86_64)
  Q3DOBJ += \
      $(B)/ded/snapvector.o \
      $(B)/ded/ftola.o
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
Q3DOBJ += \
  $(B)/ded/adler32.o \
  $(B)/ded/crc32.o \
  $(B)/ded/deflate.o \
  $(B)/ded/infback.o \
  $(B)/ded/inffast.o \
  $(B)/ded/inflate.o \
  $(B)/ded/inftrees.o \
  $(B)/ded/trees.o \
  $(B)/ded/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifneq ($(findstring $(ARCH),x86 x86_64),)
    Q3DOBJ += \
      $(B)/ded/vm_x86.o
  endif
  ifneq ($(findstring $(ARCH),ppc ppc64),)
    Q3DOBJ += $(B)/ded/vm_powerpc.o $(B)/ded/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3DOBJ += $(B)/ded/vm_sparc.o
  endif
  ifeq ($(ARCH),armv7l)
    Q3DOBJ += $(B)/client/vm_armv7l.o
  endif
endif

ifdef MINGW
  Q3DOBJ += \
    $(B)/ded/win_resource.o \
    $(B)/ded/sys_win32.o \
    $(B)/ded/con_win32.o
else
  Q3DOBJ += \
    $(B)/ded/sys_unix.o \
    $(B)/ded/con_tty.o
endif

ifeq ($(PLATFORM),darwin)
  Q3DOBJ += \
    $(B)/ded/sys_osx.o
endif

$(B)/$(SERVERBIN)$(FULLBINEXT): $(Q3DOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(Q3DOBJ) $(LIBS)



#############################################################################
## BASEQ3 CGAME
#############################################################################

Q3CGOBJHARDLINKED_ = \
  $(B)/$(BASEGAME)/cgame/cg_main.o \
  $(B)/$(BASEGAME)/cgame/bg_misc.o \
  $(B)/$(BASEGAME)/cgame/bg_pmove.o \
  $(B)/$(BASEGAME)/cgame/bg_slidemove.o \
  $(B)/$(BASEGAME)/cgame/bg_lib.o \
  $(B)/$(BASEGAME)/cgame/bg_xmlparser.o \
  $(B)/$(BASEGAME)/cgame/cg_camera.o \
  $(B)/$(BASEGAME)/cgame/cg_consolecmds.o \
  $(B)/$(BASEGAME)/cgame/cg_draw.o \
  $(B)/$(BASEGAME)/cgame/cg_drawdc.o \
  $(B)/$(BASEGAME)/cgame/cg_drawtools.o \
  $(B)/$(BASEGAME)/cgame/cg_effects.o \
  $(B)/$(BASEGAME)/cgame/cg_ents.o \
  $(B)/$(BASEGAME)/cgame/cg_event.o \
  $(B)/$(BASEGAME)/cgame/cg_freeze.o \
  $(B)/$(BASEGAME)/cgame/cg_fx_scripts.o \
  $(B)/$(BASEGAME)/cgame/cg_info.o \
  $(B)/$(BASEGAME)/cgame/cg_localents.o \
  $(B)/$(BASEGAME)/cgame/cg_marks.o \
  $(B)/$(BASEGAME)/cgame/cg_mem.o \
  $(B)/$(BASEGAME)/cgame/cg_newdraw.o \
  $(B)/$(BASEGAME)/cgame/cg_particles.o \
  $(B)/$(BASEGAME)/cgame/cg_players.o \
  $(B)/$(BASEGAME)/cgame/cg_playerstate.o \
  $(B)/$(BASEGAME)/cgame/cg_predict.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_camera.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_capture.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_dof.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_math.o \
  $(B)/$(BASEGAME)/cgame/cg_scoreboard.o \
  $(B)/$(BASEGAME)/cgame/cg_servercmds.o \
  $(B)/$(BASEGAME)/cgame/cg_snapshot.o \
  $(B)/$(BASEGAME)/cgame/cg_sound.o \
  $(B)/$(BASEGAME)/cgame/cg_spawn.o \
  $(B)/$(BASEGAME)/cgame/cg_view.o \
  $(B)/$(BASEGAME)/cgame/cg_weapons.o \
  \
  $(B)/$(BASEGAME)/cgame/sc_misc.o \
  \
  $(B)/$(MISSIONPACK)/ui/ui_shared.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_consolecmds.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_ents.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_event.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_info.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_main.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_playerstate.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_predict.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_servercmds.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_snapshot.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_view.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_weapons.o

Q3CGOBJ_ = \
  $(B)/$(BASEGAME)/cgame/cg_main.o \
  $(B)/$(BASEGAME)/cgame/bg_misc.o \
  $(B)/$(BASEGAME)/cgame/bg_pmove.o \
  $(B)/$(BASEGAME)/cgame/bg_slidemove.o \
  $(B)/$(BASEGAME)/cgame/bg_xmlparser.o \
  $(B)/$(BASEGAME)/cgame/bg_lib.o \
  $(B)/$(BASEGAME)/cgame/cg_camera.o \
  $(B)/$(BASEGAME)/cgame/cg_consolecmds.o \
  $(B)/$(BASEGAME)/cgame/cg_draw.o \
  $(B)/$(BASEGAME)/cgame/cg_drawdc.o \
  $(B)/$(BASEGAME)/cgame/cg_drawtools.o \
  $(B)/$(BASEGAME)/cgame/cg_effects.o \
  $(B)/$(BASEGAME)/cgame/cg_ents.o \
  $(B)/$(BASEGAME)/cgame/cg_event.o \
  $(B)/$(BASEGAME)/cgame/cg_freeze.o \
  $(B)/$(BASEGAME)/cgame/cg_fx_scripts.o \
  $(B)/$(BASEGAME)/cgame/cg_info.o \
  $(B)/$(BASEGAME)/cgame/cg_localents.o \
  $(B)/$(BASEGAME)/cgame/cg_marks.o \
  $(B)/$(BASEGAME)/cgame/cg_mem.o \
  $(B)/$(BASEGAME)/cgame/cg_newdraw.o \
  $(B)/$(BASEGAME)/cgame/cg_particles.o \
  $(B)/$(BASEGAME)/cgame/cg_players.o \
  $(B)/$(BASEGAME)/cgame/cg_playerstate.o \
  $(B)/$(BASEGAME)/cgame/cg_predict.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_camera.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_capture.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_dof.o \
  $(B)/$(BASEGAME)/cgame/cg_q3mme_demos_math.o \
  $(B)/$(BASEGAME)/cgame/cg_scoreboard.o \
  $(B)/$(BASEGAME)/cgame/cg_servercmds.o \
  $(B)/$(BASEGAME)/cgame/cg_snapshot.o \
  $(B)/$(BASEGAME)/cgame/cg_sound.o \
  $(B)/$(BASEGAME)/cgame/cg_spawn.o \
  $(B)/$(BASEGAME)/cgame/cg_view.o \
  $(B)/$(BASEGAME)/cgame/cg_weapons.o \
  \
  $(B)/$(BASEGAME)/cgame/sc_misc.o \
  \
  $(B)/$(MISSIONPACK)/ui/ui_shared.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_consolecmds.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_ents.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_event.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_info.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_main.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_playerstate.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_predict.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_servercmds.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_snapshot.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_view.o \
  $(B)/$(BASEGAME)/cgame/wolfcam_weapons.o \
  \
  $(B)/$(BASEGAME)/qcommon/q_math.o \
  $(B)/$(BASEGAME)/qcommon/q_shared.o

Q3CGOBJ = $(Q3CGOBJ_) $(B)/$(BASEGAME)/cgame/cg_syscalls.o $(B)/$(BASEGAME)/cgame/cg_thread.o $(B)/$(BASEGAME)/cgame/cg_dll.o
Q3CGOBJHARDLINKED = $(Q3CGOBJHARDLINKED_) $(B)/$(BASEGAME)/cgame/cg_syscalls.o $(B)/$(BASEGAME)/cgame/cg_thread.o $(B)/$(BASEGAME)/cgame/cg_dll.o
Q3CGVMOBJ = $(Q3CGOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/cgame$(SHLIBNAME): $(Q3CGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3CGOBJ) $(CGAME_LIBS)

$(B)/$(BASEGAME)/vm/cgame.qvm: $(Q3CGVMOBJ) $(CGDIR)/cg_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3CGVMOBJ) $(CGDIR)/cg_syscalls.asm

ifdef CGAME_HARD_LINKED
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(SPLINES) $(Q3POBJ) $(LIBSDLMAIN) $(Q3CGOBJHARDLINKED)
	$(echo_cmd) "LD $@"
	$(Q)$(CXX) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3POBJ) $(SPLINES) $(Q3CGOBJHARDLINKED) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(LIBS)
endif

#############################################################################
## MISSIONPACK CGAME
#############################################################################

MPCGOBJ_ = \
  $(B)/$(MISSIONPACK)/cgame/cg_main.o \
  $(B)/$(MISSIONPACK)/cgame/bg_misc.o \
  $(B)/$(MISSIONPACK)/cgame/bg_pmove.o \
  $(B)/$(MISSIONPACK)/cgame/bg_slidemove.o \
  $(B)/$(MISSIONPACK)/cgame/bg_xmlparser.o \
  $(B)/$(MISSIONPACK)/cgame/bg_lib.o \
  $(B)/$(MISSIONPACK)/cgame/cg_consolecmds.o \
  $(B)/$(MISSIONPACK)/cgame/cg_newdraw.o \
  $(B)/$(MISSIONPACK)/cgame/cg_draw.o \
  $(B)/$(MISSIONPACK)/cgame/cg_drawdc.o \
  $(B)/$(MISSIONPACK)/cgame/cg_drawtools.o \
  $(B)/$(MISSIONPACK)/cgame/cg_effects.o \
  $(B)/$(MISSIONPACK)/cgame/cg_ents.o \
  $(B)/$(MISSIONPACK)/cgame/cg_event.o \
  $(B)/$(MISSIONPACK)/cgame/cg_info.o \
  $(B)/$(MISSIONPACK)/cgame/cg_localents.o \
  $(B)/$(MISSIONPACK)/cgame/cg_marks.o \
  $(B)/$(MISSIONPACK)/cgame/cg_particles.o \
  $(B)/$(MISSIONPACK)/cgame/cg_players.o \
  $(B)/$(MISSIONPACK)/cgame/cg_playerstate.o \
  $(B)/$(MISSIONPACK)/cgame/cg_predict.o \
  $(B)/$(MISSIONPACK)/cgame/cg_scoreboard.o \
  $(B)/$(MISSIONPACK)/cgame/cg_servercmds.o \
  $(B)/$(MISSIONPACK)/cgame/cg_snapshot.o \
  $(B)/$(MISSIONPACK)/cgame/cg_sound.o \
  $(B)/$(MISSIONPACK)/cgame/cg_view.o \
  $(B)/$(MISSIONPACK)/cgame/cg_weapons.o \
  $(B)/$(MISSIONPACK)/ui/ui_shared.o \
  \
  $(B)/$(MISSIONPACK)/qcommon/q_math.o \
  $(B)/$(MISSIONPACK)/qcommon/q_shared.o

MPCGOBJ = $(MPCGOBJ_) $(B)/$(MISSIONPACK)/cgame/cg_syscalls.o
MPCGVMOBJ = $(MPCGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/cgame$(SHLIBNAME): $(MPCGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPCGOBJ)

$(B)/$(MISSIONPACK)/vm/cgame.qvm: $(MPCGVMOBJ) $(CGDIR)/cg_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPCGVMOBJ) $(CGDIR)/cg_syscalls.asm



#############################################################################
## BASEQ3 GAME
#############################################################################

Q3GOBJ_ = \
  $(B)/$(BASEGAME)/game/g_main.o \
  $(B)/$(BASEGAME)/game/ai_chat.o \
  $(B)/$(BASEGAME)/game/ai_cmd.o \
  $(B)/$(BASEGAME)/game/ai_dmnet.o \
  $(B)/$(BASEGAME)/game/ai_dmq3.o \
  $(B)/$(BASEGAME)/game/ai_main.o \
  $(B)/$(BASEGAME)/game/ai_team.o \
  $(B)/$(BASEGAME)/game/ai_vcmd.o \
  $(B)/$(BASEGAME)/game/bg_misc.o \
  $(B)/$(BASEGAME)/game/bg_pmove.o \
  $(B)/$(BASEGAME)/game/bg_xmlparser.o \
  $(B)/$(BASEGAME)/game/bg_slidemove.o \
  $(B)/$(BASEGAME)/game/bg_lib.o \
  $(B)/$(BASEGAME)/game/g_active.o \
  $(B)/$(BASEGAME)/game/g_arenas.o \
  $(B)/$(BASEGAME)/game/g_bot.o \
  $(B)/$(BASEGAME)/game/g_client.o \
  $(B)/$(BASEGAME)/game/g_cmds.o \
  $(B)/$(BASEGAME)/game/g_combat.o \
  $(B)/$(BASEGAME)/game/g_items.o \
  $(B)/$(BASEGAME)/game/g_mem.o \
  $(B)/$(BASEGAME)/game/g_misc.o \
  $(B)/$(BASEGAME)/game/g_missile.o \
  $(B)/$(BASEGAME)/game/g_mover.o \
  $(B)/$(BASEGAME)/game/g_session.o \
  $(B)/$(BASEGAME)/game/g_spawn.o \
  $(B)/$(BASEGAME)/game/g_svcmds.o \
  $(B)/$(BASEGAME)/game/g_target.o \
  $(B)/$(BASEGAME)/game/g_team.o \
  $(B)/$(BASEGAME)/game/g_trigger.o \
  $(B)/$(BASEGAME)/game/g_utils.o \
  $(B)/$(BASEGAME)/game/g_weapon.o \
  \
  $(B)/$(BASEGAME)/qcommon/q_math.o \
  $(B)/$(BASEGAME)/qcommon/q_shared.o

Q3GOBJ = $(Q3GOBJ_) $(B)/$(BASEGAME)/game/g_syscalls.o
Q3GVMOBJ = $(Q3GOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/qagame$(SHLIBNAME): $(Q3GOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3GOBJ)

$(B)/$(BASEGAME)/vm/qagame.qvm: $(Q3GVMOBJ) $(GDIR)/g_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3GVMOBJ) $(GDIR)/g_syscalls.asm

#############################################################################
## MISSIONPACK GAME
#############################################################################

MPGOBJ_ = \
  $(B)/$(MISSIONPACK)/game/g_main.o \
  $(B)/$(MISSIONPACK)/game/ai_chat.o \
  $(B)/$(MISSIONPACK)/game/ai_cmd.o \
  $(B)/$(MISSIONPACK)/game/ai_dmnet.o \
  $(B)/$(MISSIONPACK)/game/ai_dmq3.o \
  $(B)/$(MISSIONPACK)/game/ai_main.o \
  $(B)/$(MISSIONPACK)/game/ai_team.o \
  $(B)/$(MISSIONPACK)/game/ai_vcmd.o \
  $(B)/$(MISSIONPACK)/game/bg_misc.o \
  $(B)/$(MISSIONPACK)/game/bg_pmove.o \
  $(B)/$(MISSIONPACK)/game/bg_xmlparser.o \
  $(B)/$(MISSIONPACK)/game/bg_slidemove.o \
  $(B)/$(MISSIONPACK)/game/bg_lib.o \
  $(B)/$(MISSIONPACK)/game/g_active.o \
  $(B)/$(MISSIONPACK)/game/g_arenas.o \
  $(B)/$(MISSIONPACK)/game/g_bot.o \
  $(B)/$(MISSIONPACK)/game/g_client.o \
  $(B)/$(MISSIONPACK)/game/g_cmds.o \
  $(B)/$(MISSIONPACK)/game/g_combat.o \
  $(B)/$(MISSIONPACK)/game/g_items.o \
  $(B)/$(MISSIONPACK)/game/g_mem.o \
  $(B)/$(MISSIONPACK)/game/g_misc.o \
  $(B)/$(MISSIONPACK)/game/g_missile.o \
  $(B)/$(MISSIONPACK)/game/g_mover.o \
  $(B)/$(MISSIONPACK)/game/g_session.o \
  $(B)/$(MISSIONPACK)/game/g_spawn.o \
  $(B)/$(MISSIONPACK)/game/g_svcmds.o \
  $(B)/$(MISSIONPACK)/game/g_target.o \
  $(B)/$(MISSIONPACK)/game/g_team.o \
  $(B)/$(MISSIONPACK)/game/g_trigger.o \
  $(B)/$(MISSIONPACK)/game/g_utils.o \
  $(B)/$(MISSIONPACK)/game/g_weapon.o \
  \
  $(B)/$(MISSIONPACK)/qcommon/q_math.o \
  $(B)/$(MISSIONPACK)/qcommon/q_shared.o

MPGOBJ = $(MPGOBJ_) $(B)/$(MISSIONPACK)/game/g_syscalls.o
MPGVMOBJ = $(MPGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/qagame$(SHLIBNAME): $(MPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPGOBJ)

$(B)/$(MISSIONPACK)/vm/qagame.qvm: $(MPGVMOBJ) $(GDIR)/g_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPGVMOBJ) $(GDIR)/g_syscalls.asm



#############################################################################
## BASEQ3 UI
#############################################################################

Q3UIOBJ_ = \
  $(B)/$(BASEGAME)/ui/ui_main.o \
  $(B)/$(BASEGAME)/ui/bg_misc.o \
  $(B)/$(BASEGAME)/ui/bg_lib.o \
  $(B)/$(BASEGAME)/ui/ui_addbots.o \
  $(B)/$(BASEGAME)/ui/ui_atoms.o \
  $(B)/$(BASEGAME)/ui/ui_cdkey.o \
  $(B)/$(BASEGAME)/ui/ui_cinematics.o \
  $(B)/$(BASEGAME)/ui/ui_confirm.o \
  $(B)/$(BASEGAME)/ui/ui_connect.o \
  $(B)/$(BASEGAME)/ui/ui_controls2.o \
  $(B)/$(BASEGAME)/ui/ui_credits.o \
  $(B)/$(BASEGAME)/ui/ui_demo2.o \
  $(B)/$(BASEGAME)/ui/ui_display.o \
  $(B)/$(BASEGAME)/ui/ui_gameinfo.o \
  $(B)/$(BASEGAME)/ui/ui_ingame.o \
  $(B)/$(BASEGAME)/ui/ui_loadconfig.o \
  $(B)/$(BASEGAME)/ui/ui_menu.o \
  $(B)/$(BASEGAME)/ui/ui_mfield.o \
  $(B)/$(BASEGAME)/ui/ui_mods.o \
  $(B)/$(BASEGAME)/ui/ui_network.o \
  $(B)/$(BASEGAME)/ui/ui_options.o \
  $(B)/$(BASEGAME)/ui/ui_playermodel.o \
  $(B)/$(BASEGAME)/ui/ui_players.o \
  $(B)/$(BASEGAME)/ui/ui_playersettings.o \
  $(B)/$(BASEGAME)/ui/ui_preferences.o \
  $(B)/$(BASEGAME)/ui/ui_qmenu.o \
  $(B)/$(BASEGAME)/ui/ui_removebots.o \
  $(B)/$(BASEGAME)/ui/ui_saveconfig.o \
  $(B)/$(BASEGAME)/ui/ui_serverinfo.o \
  $(B)/$(BASEGAME)/ui/ui_servers2.o \
  $(B)/$(BASEGAME)/ui/ui_setup.o \
  $(B)/$(BASEGAME)/ui/ui_sound.o \
  $(B)/$(BASEGAME)/ui/ui_sparena.o \
  $(B)/$(BASEGAME)/ui/ui_specifyserver.o \
  $(B)/$(BASEGAME)/ui/ui_splevel.o \
  $(B)/$(BASEGAME)/ui/ui_sppostgame.o \
  $(B)/$(BASEGAME)/ui/ui_spskill.o \
  $(B)/$(BASEGAME)/ui/ui_startserver.o \
  $(B)/$(BASEGAME)/ui/ui_team.o \
  $(B)/$(BASEGAME)/ui/ui_teamorders.o \
  $(B)/$(BASEGAME)/ui/ui_video.o \
  \
  $(B)/$(BASEGAME)/qcommon/q_math.o \
  $(B)/$(BASEGAME)/qcommon/q_shared.o

Q3UIOBJ = $(Q3UIOBJ_) $(B)/$(MISSIONPACK)/ui/ui_syscalls.o
Q3UIVMOBJ = $(Q3UIOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/ui$(SHLIBNAME): $(Q3UIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3UIOBJ)

$(B)/$(BASEGAME)/vm/ui.qvm: $(Q3UIVMOBJ) $(UIDIR)/ui_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3UIVMOBJ) $(UIDIR)/ui_syscalls.asm

#############################################################################
## MISSIONPACK UI
#############################################################################

MPUIOBJ_ = \
  $(B)/$(MISSIONPACK)/ui/ui_main.o \
  $(B)/$(MISSIONPACK)/ui/ui_atoms.o \
  $(B)/$(MISSIONPACK)/ui/ui_gameinfo.o \
  $(B)/$(MISSIONPACK)/ui/ui_players.o \
  $(B)/$(MISSIONPACK)/ui/ui_shared.o \
  \
  $(B)/$(MISSIONPACK)/ui/bg_misc.o \
  $(B)/$(MISSIONPACK)/ui/bg_lib.o \
  \
  $(B)/$(MISSIONPACK)/qcommon/q_math.o \
  $(B)/$(MISSIONPACK)/qcommon/q_shared.o

MPUIOBJ = $(MPUIOBJ_) $(B)/$(MISSIONPACK)/ui/ui_syscalls.o
MPUIVMOBJ = $(MPUIOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/ui$(SHLIBNAME): $(MPUIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPUIOBJ)

$(B)/$(MISSIONPACK)/vm/ui.qvm: $(MPUIVMOBJ) $(UIDIR)/ui_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPUIVMOBJ) $(UIDIR)/ui_syscalls.asm



#############################################################################
## CLIENT/SERVER RULES
#############################################################################

$(B)/splines/%.o: $(SPLINESDIR)/%.cpp
	$(DO_CXX)

$(B)/client/%.o: $(ASMDIR)/%.s
	$(DO_AS)

# k8 so inline assembler knows about SSE
$(B)/client/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8

$(B)/client/snd_altivec.o: $(CDIR)/snd_altivec.c
	$(DO_CC_ALTIVEC)

$(B)/client/%.o: $(CDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(SDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(CMDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)

$(B)/client/speex/%.o: $(SPEEXDIR)/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/speex/%.o: $(SPEEXDSPDIR)/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/%.o: $(OGGDIR)/src/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/%.o: $(GDIR)/%.c
	$(DO_CC)

$(B)/client/vorbis/%.o: $(VORBISDIR)/lib/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/src/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/celt/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/silk/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/silk/float/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/%.o: $(OPUSFILEDIR)/src/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/%.o: $(ZDIR)/%.c
	$(DO_THIRDPARTY_CC)

$(B)/client/ioapi.o: $(CMDIR)/ioapi.c
	$(DO_THIRDPARTY_CC)

$(B)/client/unzip.o: $(CMDIR)/unzip.c
	$(DO_THIRDPARTY_CC)

$(B)/client/%.o: $(SDLDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(SYSDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(SYSDIR)/%.m
	$(DO_CC)

$(B)/client/win_resource.o: $(SYSDIR)/win_resource.rc $(SYSDIR)/win_manifest.xml
	$(DO_WINDRES)


$(B)/renderergl1/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/%.o: $(SDLDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/%.o: $(JPDIR)/%.c
	$(DO_THIRDPARTY_REF_CC)

$(B)/renderergl1/%.o: $(ZDIR)/%.c
	$(DO_THIRDPARTY_REF_CC)

$(B)/renderergl1/%.o: $(RCOMMONDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/%.o: $(RGL1DIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/tr_altivec.o: $(RGL1DIR)/tr_altivec.c
	$(DO_REF_CC_ALTIVEC)

$(B)/renderergl1/tr_mme_sse2.o: $(RCOMMONDIR)/tr_mme_sse2.c
	$(DO_REF_CC_SSE2)

$(B)/renderergl2/tr_mme_sse2.o: $(RCOMMONDIR)/tr_mme_sse2.c
	$(DO_REF_CC_SSE2)

$(B)/renderergl2/glsl/%.c: $(RGL2DIR)/glsl/%.glsl $(STRINGIFY)
	$(DO_REF_STR)

$(B)/renderergl2/glsl/%.o: $(B)/renderergl2/glsl/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(ZDIR)/%.c
	$(DO_THIRDPARTY_REF_CC)

$(B)/renderergl2/%.o: $(RCOMMONDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(RGL2DIR)/%.c
	$(DO_REF_CC)


$(B)/ded/%.o: $(ASMDIR)/%.s
	$(DO_AS)

# k8 so inline assembler knows about SSE
$(B)/ded/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8

$(B)/ded/%.o: $(SDIR)/%.c
	$(DO_DED_CC)

$(B)/ded/%.o: $(CMDIR)/%.c
	$(DO_DED_CC)

$(B)/ded/%.o: $(ZDIR)/%.c
	$(DO_THIRDPARTY_DED_CC)

$(B)/ded/ioapi.o: $(CMDIR)/ioapi.c
	$(DO_THIRDPARTY_DED_CC)

$(B)/ded/unzip.o: $(CMDIR)/unzip.c
	$(DO_THIRDPARTY_DED_CC)

$(B)/ded/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)

$(B)/ded/%.o: $(SYSDIR)/%.c
	$(DO_DED_CC)

$(B)/ded/%.o: $(SYSDIR)/%.m
	$(DO_DED_CC)

$(B)/ded/win_resource.o: $(SYSDIR)/win_resource.rc $(SYSDIR)/win_manifest.xml
	$(DO_WINDRES)

$(B)/ded/%.o: $(NDIR)/%.c
	$(DO_DED_CC)

# Extra dependencies to ensure the git version is incorporated
ifeq ($(USE_GIT),1)
  $(B)/client/cl_console.o : .git
  $(B)/client/common.o : .git
  $(B)/ded/common.o : .git
endif


#############################################################################
## GAME MODULE RULES
#############################################################################

$(B)/$(BASEGAME)/cgame/bg_%.o: $(GDIR)/bg_%.c
	$(DO_CGAME_CC)

$(B)/$(BASEGAME)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC)

$(B)/$(BASEGAME)/cgame/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)

$(B)/$(BASEGAME)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)

$(B)/$(MISSIONPACK)/cgame/bg_%.o: $(GDIR)/bg_%.c
	$(DO_CGAME_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/cgame/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)

$(B)/$(MISSIONPACK)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)


$(B)/$(BASEGAME)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC)

$(B)/$(BASEGAME)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC)

$(B)/$(MISSIONPACK)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC_MISSIONPACK)


$(B)/$(BASEGAME)/ui/bg_%.o: $(GDIR)/bg_%.c
	$(DO_UI_CC)

$(B)/$(BASEGAME)/ui/%.o: $(Q3UIDIR)/%.c
	$(DO_UI_CC)

$(B)/$(BASEGAME)/ui/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_UI_Q3LCC)

$(B)/$(BASEGAME)/ui/%.asm: $(Q3UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)

$(B)/$(MISSIONPACK)/ui/bg_%.o: $(GDIR)/bg_%.c
	$(DO_UI_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/ui/%.o: $(UIDIR)/%.c
	$(DO_UI_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/ui/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)

$(B)/$(MISSIONPACK)/ui/%.asm: $(UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)


$(B)/$(BASEGAME)/qcommon/%.o: $(CMDIR)/%.c
	$(DO_SHLIB_CC)

$(B)/$(BASEGAME)/qcommon/%.asm: $(CMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC)

$(B)/$(MISSIONPACK)/qcommon/%.o: $(CMDIR)/%.c
	$(DO_SHLIB_CC_MISSIONPACK)

$(B)/$(MISSIONPACK)/qcommon/%.asm: $(CMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC_MISSIONPACK)


#############################################################################
# EMSCRIPTEN
#############################################################################

EMSCRIPTEN_PRELOAD_FILE_SWITCH := $(if $(filter 1,$(EMSCRIPTEN_PRELOAD_FILE)),ON,OFF)
$(B)/$(CLIENTBIN).html: $(WEBDIR)/client.html.in
	$(echo_cmd) "SED $@"
	$(Q)sed \
		-e 's/@CLIENT_NAME@/$(CLIENTBIN)/g;' \
		-e 's/@CLIENT_BINARY@/$(CLIENTBIN)_opengl2.$(ARCH)/g;' \
		-e 's/@BASEGAME@/$(BASEGAME)/g;' \
		-e 's/@EMSCRIPTEN_PRELOAD_FILE@/$(EMSCRIPTEN_PRELOAD_FILE_SWITCH)/g' \
		< $< > $@

$(B)/$(CLIENTBIN)-config.json: $(WEBDIR)/client-config.json
	$(echo_cmd) "CP $@"
	$(Q)cp $< $@


#############################################################################
# MISC
#############################################################################

OBJ = $(Q3OBJ) $(Q3ROBJ) $(Q3R2OBJ) $(Q3DOBJ) $(JPGOBJ) $(SPLINES) \
  $(MPGOBJ) $(Q3GOBJ) $(Q3CGOBJ) $(MPCGOBJ) $(Q3UIOBJ) $(MPUIOBJ) \
  $(MPGVMOBJ) $(Q3GVMOBJ) $(Q3CGVMOBJ) $(MPCGVMOBJ) $(Q3UIVMOBJ) $(MPUIVMOBJ)
TOOLSOBJ = $(LBURGOBJ) $(Q3CPPOBJ) $(Q3RCCOBJ) $(Q3LCCOBJ) $(Q3ASMOBJ)
STRINGOBJ = $(Q3R2STRINGOBJ)


copyfiles: release
	@if [ ! -d $(COPYDIR)/$(BASEGAME) ]; then echo "You need to set COPYDIR to where your Quake3 data is!"; fi
ifneq ($(BUILD_GAME_SO),0)
  ifneq ($(BUILD_BASEGAME),0)
	-$(MKDIR) -m 0755 $(COPYDIR)/$(BASEGAME)
  endif
  ifneq ($(BUILD_MISSIONPACK),0)
	-$(MKDIR) -m 0755 $(COPYDIR)/$(MISSIONPACK)
  endif
endif

ifneq ($(BUILD_CLIENT),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)$(FULLBINEXT)
    ifneq ($(BUILD_RENDERER_OPENGL1),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl1$(RSHLIBNAME) $(COPYBINDIR)/renderer_opengl1_$(SHLIBNAME)
    endif
    ifneq ($(BUILD_RENDERER_OPENGL2),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl2$(RSHLIBNAME) $(COPYBINDIR)/renderer_opengl2_$(SHLIBNAME)
    endif
  else
    ifneq ($(BUILD_RENDERER_OPENGL1),0)
       $(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)$(FULLBINEXT)
    endif
    ifneq ($(BUILD_RENDERER_OPENGL2),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)_opengl2$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)_opengl2$(FULLBINEXT)
    endif
  endif
endif

ifneq ($(BUILD_SERVER),0)
	@if [ -f $(BR)/$(SERVERBIN)$(FULLBINEXT) ]; then \
		$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(SERVERBIN)$(FULLBINEXT) $(COPYBINDIR)/$(SERVERBIN)$(FULLBINEXT); \
	fi
endif

ifneq ($(BUILD_GAME_SO),0)
  ifneq ($(BUILD_BASEGAME),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/cgame$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/qagame$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/ui$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
  endif
  ifneq ($(BUILD_MISSIONPACK),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/cgame$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/qagame$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/ui$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
  endif
endif

clean: clean-debug clean-release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) -C $(NSISDIR) clean
else
	@$(MAKE) -C $(LOKISETUPDIR) clean
endif

clean-debug:
	@$(MAKE) clean2 B=$(BD)

clean-release:
	@$(MAKE) clean2 B=$(BR)

clean2:
	@echo "CLEAN $(B)"
	@rm -f $(OBJ)
	@rm -f $(OBJ_D_FILES)
	@rm -f $(STRINGOBJ)
	@rm -f $(TARGETS)
	@rm -f $(GENERATEDTARGETS)

toolsclean: toolsclean-debug toolsclean-release

toolsclean-debug:
	@$(MAKE) toolsclean2 B=$(BD)

toolsclean-release:
	@$(MAKE) toolsclean2 B=$(BR)

toolsclean2:
	@echo "TOOLS_CLEAN $(B)"
	@rm -f $(TOOLSOBJ)
	@rm -f $(TOOLSOBJ_D_FILES)
	@rm -f $(LBURG) $(DAGCHECK_C) $(Q3RCC) $(Q3CPP) $(Q3LCC) $(Q3ASM) $(STRINGIFY)

distclean: clean toolsclean
	@rm -rf $(BUILD_DIR)
	@rm -rf mac-binaries/cgamex86.dylib mac-binaries/qagamex86.dylib mac-binaries/uix86.dylib mac-binaries/wolfcamqlmac mac-binaries/renderer_opengl1_x86.dylib

installer: release
ifdef MINGW
	@$(MAKE) VERSION=$(VERSION) -C $(NSISDIR) V=$(V) \
		SDLDLL=$(SDLDLL) \
		USE_RENDERER_DLOPEN=$(USE_RENDERER_DLOPEN) \
		USE_OPENAL_DLOPEN=$(USE_OPENAL_DLOPEN) \
		USE_INTERNAL_SPEEX=$(USE_INTERNAL_SPEEX) \
		USE_INTERNAL_OPUS=$(USE_INTERNAL_OPUS) \
		USE_INTERNAL_ZLIB=$(USE_INTERNAL_ZLIB) \
		USE_INTERNAL_JPEG=$(USE_INTERNAL_JPEG)
else
	@$(MAKE) VERSION=$(VERSION) -C $(LOKISETUPDIR) V=$(V)
endif

dist:
	git archive --format zip --output $(CLIENTBIN)-$(VERSION).zip HEAD

#############################################################################
# DEPENDENCIES
#############################################################################

# Rebuild every target if Makefile or Makefile.local changes
ifneq ($(DEPEND_MAKEFILE),0)
.EXTRA_PREREQS:= $(MAKEFILE_LIST)
endif

ifneq ($(B),)
  OBJ_D_FILES=$(filter %.d,$(OBJ:%.o=%.d))
  TOOLSOBJ_D_FILES=$(filter %.d,$(TOOLSOBJ:%.o=%.d))
  -include $(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)
endif

.PHONY: all clean clean2 clean-debug clean-release copyfiles \
	debug default dist distclean installer makedirs \
	release targets \
	toolsclean toolsclean2 toolsclean-debug toolsclean-release \
	$(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)

# If the target name contains "clean", don't do a parallel build
ifneq ($(findstring clean, $(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

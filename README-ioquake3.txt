![Build](https://github.com/ioquake/ioq3/workflows/Build/badge.svg)

                   ,---------------------------------------.
                   |   _                     _       ____  |
                   |  (_)___  __ _ _  _ __ _| |_____|__ /  |
                   |  | / _ \/ _` | || / _` | / / -_)|_ \  |
                   |  |_\___/\__, |\_,_\__,_|_\_\___|___/  |
                   |            |_|                        |
                   |                                       |
                   `--------- https://ioquake3.org --------'

The intent of this project is to provide a baseline Quake 3 which may be used
for further development and baseq3 fun.
Some of the major features currently implemented are:

  * SDL 2 backend
  * OpenAL sound API support (multiple speaker support and better sound
    quality)
  * Full x86_64 support on Linux
  * VoIP support, both in-game and external support through Mumble.
  * MinGW compilation support on Windows and cross compilation support on Linux
  * AVI video capture of demos
  * Much improved console autocompletion
  * Persistent console history
  * Colorized terminal output
  * Optional Ogg Vorbis support
  * Much improved QVM tools
  * Support for various esoteric operating systems
  * cl_guid support
  * HTTP/FTP download redirection (using cURL)
  * Multiuser support on Windows systems (user specific game data
    is stored in "%APPDATA%\Quake3")
  * PNG support
  * Web support via Emscripten
  * Many, many bug fixes

The map editor and associated compiling tools are not included. We suggest you
use a modern copy from http://icculus.org/gtkradiant/.

The original id software readme that accompanied the Q3 source release has been
renamed to id-readme.txt so as to prevent confusion. Please refer to the
website for updated status.

More documentation including a Player's Guide and Sysadmin Guide are on:
https://ioquake3.org/help/

If you've got issues that you aren't sure are worth filing as bugs, or just
want to chat, please visit our forums:
https://discourse.ioquake.org

# Thank You:

<p>
  <a href="https://www.digitalocean.com/">Digital Ocean<br/>
    <img src="https://opensource.nyc3.cdn.digitaloceanspaces.com/attribution/assets/PoweredByDO/DO_Powered_by_Badge_blue.svg" width="201px">
  </a>
</p>
---
<p>
<a href="https://www.discourse.org/">Discourse<br/>
<img src=
"https://user-images.githubusercontent.com/1681963/52239617-e2683480-289c-11e9-922b-5da55472e5b4.png"
 width="300px"></a>
</p>
---
<p>
<a href="https://icculus.org/">icculus dot org<br/>
<img src="http://icculus.org/icculus-org-now.png" width="300px"></a>
</p>
---
<p>
<a href="https://nuclearmonster.com/">Nuclear Monster<br/>
<img src="https://user-images.githubusercontent.com/903791/152968830-dd08737b-55c6-4ac6-9610-31121ea0e8c6.png" width="300px"></a>
</p>

# Compilation and installation

For *nix
  1. Change to the directory containing this readme.
  2. Run 'make'.

For Windows,
  1. Please refer to the excellent instructions here:
     https://ioquake3.org/help/building-ioquake3/

For macOS, building a Universal Binary (macOS 10.5 to 10.8, x86_64, x86, ppc)
  1. Install MacOSX SDK packages from XCode.  For maximum compatibility,
     install MacOSX10.5.sdk and MacOSX10.6.sdk.
  2. Change to the directory containing this README file.
  3. Run './make-macosx-ub.sh'
  4. Copy the resulting ioquake3.app in /build/release-darwin-universal
     to your /Applications/ioquake3 folder.

For macOS, building a Universal Binary 2 (macOS 10.9+, arm64, x86_64)
  1. Install MacOSX SDK packages from XCode.  Building for arm64 requires
     MacOSX11.sdk or later.
  2. Change to the directory containing this README file.
  3. Run './make-macosx-ub2.sh'
  4. Copy the resulting ioquake3.app in /build/release-darwin-universal2
     to your /Applications/ioquake3 folder.

For Web, building with Emscripten
  1. Follow the installation instructions for the Emscripten SDK including
     setting up the environment with emsdk_env.
  2. Run `emmake make debug` (or release).
  3. Copy or symlink your baseq3 pk3 files into the `build/debug-emscripten-wasm32/baseq3`
     directory so they can be loaded at run-time. Only game files listed in
     `client-config.json` will be loaded.
  4. Start a web server serving this directory. `python3 -m http.server`
     is an easy default that you may already have installed.
  5. Open `http://localhost:8000/build/debug-emscripten-wasm32/ioquake3.html`
     in a web browser. Open the developer console to see errors and warnings.
  6. Debugging the C code is possible using a Chrome extension. For details
     see https://developer.chrome.com/blog/wasm-debugging-2020

Installation, for *nix
  1. Set the COPYDIR variable in the shell to be where you installed Quake 3
     to. By default it will be /usr/local/games/quake3 if you haven't set it.
     This is the path as used by the original Linux Q3 installer and subsequent
     point releases.
  2. Run 'make copyfiles'.

It is also possible to cross compile for Windows under *nix using MinGW. Your
distribution may have mingw32 packages available. On debian/Ubuntu, you need to
install 'mingw-w64'. Thereafter cross compiling is simply a case running
'PLATFORM=mingw32 ARCH=x86 make' in place of 'make'. ARCH may also be set to
x86_64.

The following variables may be set, either on the command line or in
Makefile.local:

```
  DEPEND_MAKEFILE      - set to 0 to disable rebuilding all targets when
                         the Makefile or Makefile.local is changed
  CFLAGS               - use this for custom CFLAGS
  V                    - set to show cc command line when building
  DEFAULT_BASEDIR      - extra path to search for baseq3 and such
  BUILD_SERVER         - build the 'ioq3ded' server binary
  BUILD_CLIENT         - build the 'ioquake3' client binary
  BUILD_BASEGAME       - build the 'baseq3' binaries
  BUILD_MISSIONPACK    - build the 'missionpack' binaries
  BUILD_GAME_SO        - build the game shared libraries
  BUILD_GAME_QVM       - build the game qvms
  BUILD_STANDALONE     - build binaries suited for stand-alone games
  SERVERBIN            - rename 'ioq3ded' server binary
  CLIENTBIN            - rename 'ioquake3' client binary
  USE_RENDERER_DLOPEN  - build and use the renderer in a library
  BUILD_RENDERER_OPENGL1 build the opengl1 client / renderer library
  BUILD_RENDERER_OPENGL2 build the opengl2 client / renderer library
  USE_YACC             - use yacc to update code/tools/lcc/lburg/gram.c
  BASEGAME             - rename 'baseq3'
  BASEGAME_CFLAGS      - custom CFLAGS for basegame
  MISSIONPACK          - rename 'missionpack'
  MISSIONPACK_CFLAGS   - custom CFLAGS for missionpack (default '-DMISSIONPACK')
  USE_OPENAL           - use OpenAL where available
  USE_OPENAL_DLOPEN    - link with OpenAL at runtime
  USE_CURL             - use libcurl for http/ftp download support
  USE_CURL_DLOPEN      - link with libcurl at runtime
  USE_CODEC_VORBIS     - enable Ogg Vorbis support
  USE_CODEC_OPUS       - enable Ogg Opus support
  USE_MUMBLE           - enable Mumble support
  USE_VOIP             - enable built-in VoIP support
  USE_FREETYPE         - enable FreeType support for rendering fonts
  USE_INTERNAL_LIBS    - build internal libraries instead of dynamically
                         linking against system libraries; this just sets
                         the default for USE_INTERNAL_ZLIB etc.
                         and USE_LOCAL_HEADERS
  USE_INTERNAL_SPEEX   - build internal speex library instead of dynamically
                         linking against system libspeex
  USE_INTERNAL_ZLIB    - build and link against internal zlib
  USE_INTERNAL_JPEG    - build and link against internal JPEG library
  USE_INTERNAL_OGG     - build and link against internal ogg library
  USE_INTERNAL_OPUS    - build and link against internal opus/opusfile libraries
  USE_INTERNAL_VORBIS  - build and link against internal Vorbis library
  USE_LOCAL_HEADERS    - use headers local to ioq3 instead of system ones
  DEBUG_CFLAGS         - C compiler flags to use for building debug version
  COPYDIR              - the target installation directory
  TEMPDIR              - specify user defined directory for temp files
  EMSCRIPTEN_PRELOAD_FILE - set to 1 to package 'baseq3' (BASEGAME) directory
                            containing pk3s and loose files as a single
                            .data file that is loaded instead of listing
                            individual files in client-config.json
```

The defaults for these variables differ depending on the target platform.


# OpenGL ES support

The opengl2 renderer (the default) supports OpenGL ES 2+. Though there
are many missing features and the performance may not be sufficient for
embedded System-on-a-Chip and mobile platforms.

The opengl1 renderer does not have OpenGL ES support.

The opengl2 renderer will try both OpenGL and OpenGL ES APIs to find one that
works. The `r_preferOpenGLES` cvar controls which API to try first.
Set it to -1 for auto (default), 0 for OpenGL, and 1 for OpenGL ES. It should be set using command line arguments:

    ioquake3 +set cl_renderer opengl2 +set r_preferOpenGLES 1


# Console

## New cvars

```
  cl_autoRecordDemo                 - record a new demo on each map change
  cl_aviFrameRate                   - the framerate to use when capturing video
  cl_aviMotionJpeg                  - use the mjpeg codec when capturing video
  cl_guidServerUniq                 - makes cl_guid unique for each server
  cl_cURLLib                        - filename of cURL library to load
  cl_consoleKeys                    - space delimited list of key names or
                                      characters that toggle the console
  cl_mouseAccelStyle                - Set to 1 for QuakeLive mouse acceleration
                                      behaviour, 0 for standard q3
  cl_mouseAccelOffset               - Tuning the acceleration curve, see below

  con_autoclear			    - Set to 0 to disable clearing console
  				      input text when console is closed

  in_joystickUseAnalog              - Do not translate joystick axis events
                                      to keyboard commands

  j_forward                         - Joystick analogue to m_forward,
                                      for forward movement speed/direction.
  j_side                            - Joystick analogue to m_side,
                                      for side movement speed/direction.
  j_up                              - Joystick up movement speed/direction.
  j_pitch                           - Joystick analogue to m_pitch,
                                      for pitch rotation speed/direction.
  j_yaw                             - Joystick analogue to m_yaw,
                                      for yaw rotation speed/direction.
  j_forward_axis                    - Selects which joystick axis
                                      controls forward/back.
  j_side_axis                       - Selects which joystick axis
                                      controls left/right.
  j_up_axis                         - Selects which joystick axis
                                      controls up/down.
  j_pitch_axis                      - Selects which joystick axis
                                      controls pitch.
  j_yaw_axis                        - Selects which joystick axis
                                      controls yaw.

  s_useOpenAL                       - use the OpenAL sound backend if available
  s_alPrecache                      - cache OpenAL sounds before use
  s_alGain                          - the value of AL_GAIN for each source
  s_alSources                       - the total number of sources (memory) to
                                      allocate
  s_alDopplerFactor                 - the value passed to alDopplerFactor
  s_alDopplerSpeed                  - the value passed to alDopplerVelocity
  s_alMinDistance                   - the value of AL_REFERENCE_DISTANCE for
                                      each source
  s_alMaxDistance                   - the maximum distance before sounds start
                                      to become inaudible.
  s_alRolloff                       - the value of AL_ROLLOFF_FACTOR for each
                                      source
  s_alGraceDistance                 - after having passed MaxDistance, length
                                      until sounds are completely inaudible
  s_alDriver                        - which OpenAL library to use
  s_alDevice                        - which OpenAL device to use
  s_alAvailableDevices              - list of available OpenAL devices
  s_alInputDevice                   - which OpenAL input device to use
  s_alAvailableInputDevices         - list of available OpenAL input devices
  s_sdlBits                         - SDL bit resolution
  s_sdlSpeed                        - SDL sample rate
  s_sdlChannels                     - SDL number of channels
  s_sdlDevSamps                     - SDL DMA buffer size override
  s_sdlMixSamps                     - SDL mix buffer size override
  s_backend                         - read only, indicates the current sound
                                      backend
  s_muteWhenMinimized               - mute sound when minimized
  s_muteWhenUnfocused               - mute sound when window is unfocused
  sv_dlRate			    - bandwidth allotted to PK3 file downloads
  				      via UDP, in kbyte/s

  com_ansiColor                     - enable use of ANSI escape codes in the tty
  com_altivec                       - enable use of altivec on PowerPC systems
  com_standalone (read only)        - If set to 1, quake3 is running in
                                      standalone mode
  com_basegame                      - Use a different base than baseq3. If no
                                      original Quake3 or TeamArena pak files
                                      are found, this will enable running in
                                      standalone mode
  com_homepath                      - Specify name that is to be appended to the
                                      home path
  com_legacyprotocol		    - Specify protocol version number for
  				      legacy Quake3 1.32c protocol, see
				      "Network protocols" section below
				      (startup only)
  com_maxfpsUnfocused               - Maximum frames per second when unfocused
  com_maxfpsMinimized               - Maximum frames per second when minimized
  com_pipefile                      - Specify filename to create a named pipe
                                      through which other processes can control
                                      the server while it is running.
                                      Nonfunctional on Windows.
  com_gamename			    - Gamename sent to master server in
  				      getservers[Ext] query and infoResponse
                                      "gamename" infostring value. Also used
                                      for filtering local network games.

  in_joystickNo                     - select which joystick to use
  in_availableJoysticks             - list of available Joysticks
  in_keyboardDebug                  - print keyboard debug info

  sv_dlURL                          - the base of the HTTP or FTP site that
                                      holds custom pk3 files for your server
  sv_banFile                        - Name of the file that is used for storing
                                      the server bans

  net_ip6                           - IPv6 address to bind to
  net_port6                         - port to bind to using the ipv6 address
  net_enabled                       - enable networking, bitmask. Add up
                                      number for option to enable it:
                                      enable ipv4 networking:    1
                                      enable ipv6 networking:    2
                                      prioritise ipv6 over ipv4: 4
                                      disable multicast support: 8
  net_mcast6addr                    - multicast address to use for scanning for
                                      ipv6 servers on the local network
  net_mcastiface                    - outgoing interface to use for scan

  protocol                          - Allow changing protocol version (startup only)

  r_allowResize                     - make window resizable
  r_ext_texture_filter_anisotropic  - anisotropic texture filtering
  r_zProj                           - distance of observer camera to projection
                                      plane in quake3 standard units
  r_greyscale                       - desaturate textures, useful for anaglyph,
                                      supports values in the range of 0 to 1
  r_stereoEnabled                   - enable stereo rendering for techniques
                                      like shutter glasses (untested)
  r_anaglyphMode                    - Enable rendering of anaglyph images
                                      red-cyan glasses:    1
                                      red-blue:            2
                                      red-green:           3
				      green-magenta:	   4
                                      To swap the colors for left and right eye
                                      just add 4 to the value for the wanted
                                      color combination. For red-blue and
                                      red-green you probably want to enable
                                      r_greyscale
  r_stereoSeparation                - Control eye separation. Resulting
                                      separation is r_zProj divided by this
                                      value in quake3 standard units.
                                      See also
                                      http://wiki.ioquake3.org/Stereo_Rendering
                                      for more information
  r_marksOnTriangleMeshes           - Support impact marks on md3 models, MOD
                                      developers should increase the mark
                                      triangle limits in cg_marks.c if they
                                      intend to use this.
  r_sdlDriver                       - read only, indicates the SDL driver
                                      backend being used
  r_noborder                        - Remove window decoration from window
                                      managers, like borders and titlebar.
  r_mode -2			    - This new video mode automatically uses the
  	 			      desktop resolution.
```

## New commands

```
  video [filename]        - start video capture (use with demo command)
  stopvideo               - stop video capture
  stopmusic               - stop background music
  minimize                - Minimize the game and show desktop
  togglemenu		  - causes escape key event for opening/closing menu, or
  			    going to a previous menu. works in binds, even in UI

  print                   - print out the contents of a cvar
  unset                   - unset a user created cvar

  banaddr <range>         - ban an ip address range from joining a game on this
                            server, valid <range> is either playernum or CIDR
                            notation address range.
  exceptaddr <range>      - exempt an ip address range from a ban.
  bandel <range>          - delete ban (either range or ban number)
  exceptdel <range>       - delete exception (either range or exception number)
  listbans                - list all currently active bans and exceptions
  rehashbans              - reload the banlist from serverbans.dat
  flushbans               - delete all bans

  net_restart             - restart network subsystem to change latched settings
  game_restart <fs_game>  - Switch to another mod

  which <filename/path>	  - print out the path on disk to a loaded item

  execq <filename>        - quiet exec command, doesn't print "execing file.cfg"

  kicknum <client number> - kick a client by number, same as clientkick command
  kickall                 - kick all clients, similar to "kick all" (but kicks
                            everyone even if someone is named "all")
  kickbots                - kick all bots, similar to "kick allbots" (but kicks
                            all bots even if someone is named "allbots")

  tell <client num> <msg> - send message to a single client (new to server)

  cvar_modified [filter]  - list modified cvars, can filter results (such as "r*"
  		            for renderer cvars) like cvarlist which lists all cvars

  addbot random		  - the bot name "random" now selects a random bot
```


# README for Developers

## pk3dir

ioquake3 has a useful new feature for mappers. Paths in a game directory with
the extension ".pk3dir" are treated like pk3 files. This means you can keep
all files specific to your map in one directory tree and easily zip this
folder for distribution.

## 64bit mods

If you wish to compile external mods as shared libraries on a 64bit platform,
and the mod source is derived from the id Q3 SDK, you will need to modify the
interface code a little. Open the files ending in _syscalls.c and change
every instance of int to intptr_t in the declaration of the syscall function
pointer and the dllEntry function. Also find the vmMain function for each
module (usually in cg_main.c g_main.c etc.) and similarly replace the return
value in the prototype with intptr_t (arg0, arg1, ...stay int).

Add the following code snippet to q_shared.h:

```c
#ifdef Q3_VM
typedef int intptr_t;
#else
#include <stdint.h>
#endif
```

Note if you simply wish to run mods on a 64bit platform you do not need to
recompile anything since by default Q3 uses a virtual machine system.

## Creating mods compatible with Q3 1.32b

If you're using this package to create mods for the last official release of
Q3, it is necessary to pass the commandline option '-vq3' to your invocation
of q3asm. This is because by default q3asm outputs an updated qvm format that
is necessary to fix a bug involving the optimizing pass of the x86 vm JIT
compiler.

## Creating standalone games

Have you finished the daunting task of removing all dependencies on the Q3
game data? You probably now want to give your users the opportunity to play
the game without owning a copy of Q3, which consequently means removing cd-key
and authentication server checks. In addition to being a straightforward Q3
client, ioquake3 also purports to be a reliable and stable code base on which
to base your game project.

However, before you start compiling your own version of ioquake3, you have to
ask yourself: Have we changed or will we need to change anything of importance
in the engine?

If your answer to this question is "no", it probably makes no sense to build
your own binaries. Instead, you can just use the pre-built binaries on the
website. Just make sure the game is called with:

    +set com_basegame <yournewbase>

in any links/scripts you install for your users to start the game. The
binary must not detect any original quake3 game pak files. If this
condition is met, the game will set com_standalone to 1 and is then running
in stand alone mode.

If you want the engine to use a different directory in your homepath than
e.g. "Quake3" on Windows or ".q3a" on Linux, then set a new name at startup
by adding

    +set com_homepath <homedirname>

to the command line. You can also control which game name to use when talking
to the master server:

    +set com_gamename <gamename>

So clients requesting a server list will only receive servers that have a
matching game name.

Example line:

    +set com_basegame basefoo +set com_homepath .foo
    +set com_gamename foo

If you really changed parts that would make vanilla ioquake3 incompatible with
your mod, we have included another way to conveniently build a stand-alone
binary. Just run make with the option BUILD_STANDALONE=1. Don't forget to edit
the PRODUCT_NAME and subsequent #defines in qcommon/q_shared.h with
information appropriate for your project.

## Standalone game licensing

While a lot of work has been put into ioquake3 that you can benefit from free
of charge, it does not mean that you have no obligations to fulfill. Please be
aware that as soon as you start distributing your game with an engine based on
our sources we expect you to fully comply with the requirements as stated in
the GPL. That includes making sources and modifications you made to the
ioquake3 engine as well as the game-code used to compile the .qvm files for
the game logic freely available to everyone. Furthermore, note that the "QIIIA
Game Source License" prohibits distribution of mods that are intended to
operate on a version of Q3 not sanctioned by id software:

    "with this Agreement, ID grants to you the non-exclusive and limited right
    to distribute copies of the Software ... for operation only with the full
    version of the software game QUAKE III ARENA"

This means that if you're creating a standalone game, you cannot use said
license on any portion of the product. As the only other license this code has
been released under is the GPL, this is the only option.

This does NOT mean that you cannot market this game commercially. The GPL does
not prohibit commercial exploitation and all assets (e.g. textures, sounds,
maps) created by yourself are your property and can be sold like every other
game you find in stores.


## PNG support

ioquake3 supports the use of PNG (Portable Network Graphic) images as
textures. It should be noted that the use of such images in a map will
result in missing placeholder textures where the map is used with the id
Quake 3 client or earlier versions of ioquake3.

Recent versions of GtkRadiant and q3map2 support PNG images without
modification. However GtkRadiant is not aware that PNG textures are supported
by ioquake3. To change this behaviour open the file 'q3.game' in the 'games'
directory of the GtkRadiant base directory with an editor and change the
line:

    texturetypes="tga jpg"

to

    texturetypes="tga jpg png"

Restart GtkRadiant and PNG textures are now available.

## Building with MinGW for pre Windows XP

IPv6 support requires a header named "wspiapi.h" to abstract away from
differences in earlier versions of Windows' IPv6 stack. There is no MinGW
equivalent of this header and the Microsoft version is obviously not
redistributable, so in its absence we're forced to require Windows XP.
However if this header is acquired separately and placed in the qcommon/
directory, this restriction is lifted.


# Contributing

Please submit patches through GitHub pull requests.

The focus for ioq3 is to develop a stable base suitable for further development
and provide players with the same Quake 3 experience they've had for years.

We do have graphical improvements with the new renderer, but they are off by default.
See opengl2-readme.md for more information.

# Credits

Maintainers

  * James Canete <use.less01@gmail.com>
  * Ludwig Nussel <ludwig.nussel@suse.de>
  * Thilo Schulz <arny@ats.s.bawue.de>
  * Tim Angus <tim@ngus.net>
  * Tony J. White <tjw@tjw.org>
  * Jack Slater <jack@ioquake.org>
  * Zack Middleton <zturtleman@gmail.com>

Significant contributions from

  * Ryan C. Gordon <icculus@icculus.org>
  * Andreas Kohn <andreas@syndrom23.de>
  * Joerg Dietrich <Dietrich_Joerg@t-online.de>
  * Stuart Dalton <badcdev@gmail.com>
  * Vincent S. Cojot <vincent at cojot dot name>
  * optical <alex@rigbo.se>
  * Aaron Gyes <floam@aaron.gy>



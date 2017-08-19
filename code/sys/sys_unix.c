/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#define __USE_GNU
#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif


#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>
#include <execinfo.h>

//#define _GNU_SOURCE
//#ifndef _GNU_SOURCE
//#define _GNU_SOURCE
//#endif

#include <dlfcn.h>

//#ifdef __linux__
#include <sys/utsname.h>
//#endif

#ifdef __APPLE__
//#include <CoreServices/CoreServices.h>
#endif

qboolean stdinIsATTY;

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };
static char QuakeLivePath[MAX_OSPATH] = { 0 };

// Used to store the Steam Quake 3 installation path
//static char steamPath[ MAX_OSPATH ] = { 0 };

// Used to store the GOG Quake 3 installation path
//static char gogPath[ MAX_OSPATH ] = { 0 };

/*
==================
Sys_DefaultHomePath
==================
*/
char *Sys_DefaultHomePath(void)
{
	char *p;

	if( !*homePath && com_homepath != NULL )
	{
		if( ( p = getenv( "HOME" ) ) != NULL )
		{
			Com_sprintf(homePath, sizeof(homePath), "%s%c", p, PATH_SEP);
#ifdef __APPLE__
			Q_strcat(homePath, sizeof(homePath),
					"Library/Application Support/");

			if(com_homepath->string[0])
				Q_strcat(homePath, sizeof(homePath), com_homepath->string);
			else
				Q_strcat(homePath, sizeof(homePath), HOMEPATH_NAME_MACOSX);
#else
			if(com_homepath->string[0])
				Q_strcat(homePath, sizeof(homePath), com_homepath->string);
			else
				Q_strcat(homePath, sizeof(homePath), HOMEPATH_NAME_UNIX);
#endif
		}
	}

	return homePath;
}

char *Sys_QuakeLiveDir (void)
{
	const char *p;
	const char *override = NULL;
	const char *user = getenv("USER");
	char searchPath[MAX_OSPATH];
	char subPath[MAX_OSPATH];
	DIR *d;
	struct dirent *dir;

	if (!*QuakeLivePath) {
		override = Cvar_VariableString("fs_quakelivedir");
		if (override  &&  *override) {
			Q_strncpyz(QuakeLivePath, override, sizeof(QuakeLivePath));
			//FS_ReplaceSeparators(QuakeLivePath);
			return QuakeLivePath;
		}

		if ((p = getenv("HOME")) != NULL) {
			if (user  &&  *user) {
				struct stat st;
				int count;

				// check for quake live steam
				count = 0;
				while (count < 2) {
					//const char *tag;

					if (count == 0) {
						Com_sprintf(searchPath, sizeof(searchPath), "%s/.wine/drive_c/Program Files (x86)/Steam/steamapps/common/Quake Live", p);
						//tag = "64-bit";
					} else {
						Com_sprintf(searchPath, sizeof(searchPath), "%s/.wine/drive_c/Program Files/Steam/steamapps/common/Quake Live", p);
						//tag = "32-bit";
					}

					d = opendir(searchPath);
					if (d) {
						while ((dir = readdir(d)) != NULL) {
							//printf("%s:  '%s'\n", tag, dir->d_name);
							if (dir->d_name[0] == '.') {
								continue;
							}
							Com_sprintf(subPath, sizeof(subPath), "%s/%s/baseq3", searchPath, dir->d_name);
							if (stat(subPath, &st) == 0) {
								// got it
								Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/%s", searchPath, dir->d_name);
								goto done;
							}
						}
					}

					count++;
				}  // while (count < 2)

				// didn't find wine steam quake live, try wine stand alone
				Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/.wine/drive_c/users/%s/Application Data/id Software/quakelive/home", p, user);
			} else {
				// try old quakelive native linux/mac support
#ifdef __APPLE__
				//FIXME not sure
				// /Library/Application\ Support/Quakelive/
				Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/Library/Application Support/Quakelive/home/", p);
#else
				Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/.quakelive/quakelive/home", p);
#endif
			}
		}
	}

done:

	return QuakeLivePath;
}

/*
================
Sys_SteamPath
================
*/
char *Sys_SteamPath( void )
{
	// Disabled since Steam doesn't let you install Quake 3 on Mac/Linux
#if 0 //#ifdef STEAMPATH_NAME
	char *p;

	if( ( p = getenv( "HOME" ) ) != NULL )
    {
#ifdef __APPLE__
		char *steamPathEnd = "/Library/Application Support/Steam/SteamApps/common/" STEAMPATH_NAME;
#else
		char *steamPathEnd = "/.steam/steam/SteamApps/common/" STEAMPATH_NAME;
#endif
		Com_sprintf(steamPath, sizeof(steamPath), "%s%s", p, steamPathEnd);
	}
#endif

	//return steamPath;

	return "";
}

/*
================
Sys_GogPath
================
*/
char *Sys_GogPath( void )
{
#if 0  // disabled
	// GOG also doesn't let you install Quake 3 on Mac/Linux
	return gogPath;
#endif

	return "";
}

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038 */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
     (which would affect the wrap period) */
int curtime;
int Sys_Milliseconds (void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	FILE *fp;

	fp = fopen( "/dev/urandom", "r" );
	if( !fp )
		return qfalse;

	setvbuf( fp, NULL, _IONBF, 0 ); // don't buffer reads from /dev/urandom

	if( fread( string, sizeof( byte ), len, fp ) != len )
	{
		fclose( fp );
		return qfalse;
	}

	fclose( fp );
	return qtrue;
}

/*
==================
Sys_GetCurrentUser
==================
*/
char *Sys_GetCurrentUser( void )
{
	struct passwd *p;

	if ( (p = getpwuid( getuid() )) == NULL ) {
		return "player";
	}
	return p->pw_name;
}

//////////
#if 0
/*
==================
Sys_GetClipboardData
==================
*/
#if defined(DEDICATED)
char *Sys_GetClipboardData (void)
{
	return NULL;
}
#elif defined(__APPLE__)

extern char *Cocoa_GetClipboardData (void);

char *Sys_GetClipboardData(void)
{
	const char *s;
	char *data;
	int len;

	s = Cocoa_GetClipboardData();
	len = strlen(s);
	data = Z_Malloc(len + 1);
	if (data == NULL) {
		Com_Printf("^1Sys_GetClipboardData:  couldn't allocate memory for data\n");
		return NULL;
	}

	Q_strncpyz(data, s, len);

	// like win32 clipboard handler
	strtok(data, "\n\r\b");

	return data;
}

#else  // default unix client

#include "SDL_syswm.h"
#include "SDL.h"
//#include <X11/Xlib.h>
#include <X11/Xatom.h>

//static qboolean clipBoardInit = qfalse;

//static void clipBoardInit (void)
//{
//
//}

char *Sys_GetClipboardData (void)
{
	//FIXME sdl2
	return NULL;

#if 0
	SDL_SysWMinfo info;
	int r;
	Atom XA_CLIPBOARD;
	Atom ourAtom;
	Atom UTF8_STRING;
	Window selectionWindow;
	Window ourWindow;
	Display *dpy;
	SDL_Event event;
	int selectionFormat;
	Atom selectionType;
	unsigned long nbytes;
	unsigned char *data;
	unsigned long nbytesAfter;
	char *retData;
	int startTime;

	//FIXME move from here
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	// need to initialize before getting window information
	SDL_VERSION(&info.version);
	r = SDL_GetWMInfo(&info);

	if (r != 1) {
		Com_Printf("^3Sys_GetClipboardData: SDL_GetWMInfo failed %d  '%s'\n", r, SDL_GetError());
		return NULL;
	}

	dpy = info.info.x11.display;
	ourWindow = info.info.x11.window;

	XA_CLIPBOARD = XInternAtom(dpy, "CLIPBOARD", True);
	if (XA_CLIPBOARD == None) {
		Com_Printf("^1Sys_GetClipboardData:  couldn't create clipboard atom\n");
		return NULL;
	}

	UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", True);
	if (UTF8_STRING == None) {
		Com_Printf("^1Sys_GetClipboardData:  couldn't create utf8 atom\n");
		return NULL;
	}

	// create if necessary
	ourAtom = XInternAtom(dpy, "WOLFCAMQL_SELECTION", False);
	if (ourAtom == None) {
		Com_Printf("^1Sys_GetClipboardData:  couldn't create our atom\n");
		return NULL;
	}

	selectionWindow = XGetSelectionOwner(dpy, XA_CLIPBOARD);
	if (selectionWindow == None) {
		Com_Printf("no selection window\n");
		return NULL;
	}
	if (selectionWindow == ourWindow) {
		//FIXME
		Com_Printf("we already own selection\n");
		return NULL;
	}

	// ask other window to copy to our atom
	//XConvertSelection(dpy, XA_PRIMARY, UTF8_STRING, ourAtom, ourWindow, CurrentTime);
	XConvertSelection(dpy, XA_CLIPBOARD, UTF8_STRING, ourAtom, ourWindow, CurrentTime);
	//XFlush(dpy);


	// wait for other window to copy the data
	startTime = Sys_Milliseconds();
	while (1) {
		int r;
		//int num;
		//SDL_Event events[20];
		//int i;

		//SDL_WaitEvent(&event);
		//SDL_WaitEventTimeout(&event, 1000);
		//FIXME should use sdl_peepevent
		//r = SDL_PollEvent(&event);
		SDL_PumpEvents();
		//r = SDL_PeepEvents(events, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_SYSWMEVENT));
		//r = SDL_PeepEvents(events, 1, SDL_GETEVENT, SDL_ALLEVENTS);
		//r = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_ALLEVENTS);
		//r = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_SYSWMEVENT);
		//num = SDL_PeepEvents(events, 20, SDL_GETEVENT, SDL_EVENTMASK(SDL_SYSWMEVENT));
		r = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_SYSWMEVENT));

#if 0
		Com_Printf("got wm events: %d\n", num);
		for (i = 0;  i < num;  i++) {
			if (events[i].type == SDL_SYSWMEVENT) {

			}
		}
#endif
		//Com_Printf("ev: %d\n", event.type);

#if 0
		if (r == 0) {
			Com_Printf("no event yet...\n");
		} else {
			Com_Printf("xxxxxxxx got event r:%d\n", r);
		}
#endif

		if (r == 1  &&  event.type == SDL_SYSWMEVENT) {
			//Com_Printf("got SDL_SYSWMEVENT\n");
			XEvent xevent = event.syswm.msg->event.xevent;

			if (xevent.type == SelectionNotify  &&  xevent.xselection.requestor == ourWindow) {
				// got it
				break;
			} else {
				//FIXME pop this back?
			}
		}

		if ((Sys_Milliseconds() - startTime > 3000)) {
			Com_Printf("^3Sys_GetClipboardData:  timed out\n");
			return NULL;
		}

		//Com_Printf("sleeping ...\n");
		usleep(1000 * 50);
	}

	XGetWindowProperty(dpy, ourWindow, ourAtom, 0, INT_MAX/4, False, AnyPropertyType, &selectionType, &selectionFormat, &nbytes, &nbytesAfter, &data);
	if (nbytesAfter != 0) {
		Com_Printf("^3Sys_GetClipboardData: %ld bytes after\n", nbytesAfter);
	}
	//Com_Printf("got selection data... '%s'\n", data);

	retData = Z_Malloc(nbytes + 1);
	if (retData == NULL) {
		Com_Printf("^1Sys_GetClipboardData:  couldn't allocate memory for clipboard buffer\n");
		XFree(data);
		return NULL;
	}

	Q_strncpyz(retData, (char *)data, nbytes + 1);

	// like win32 clipboard handler
	strtok(retData, "\n\r\b");

	XFree(data);
	XDeleteProperty(dpy, ourWindow, ourAtom);

	return retData;
#endif
}

#endif

#endif
////////////

#define MEM_THRESHOLD 96*1024*1024

/*
==================
Sys_LowPhysicalMemory

TODO
==================
*/
qboolean Sys_LowPhysicalMemory( void )
{
	return qfalse;
}

/*
==================
Sys_Basename
==================
*/
const char *Sys_Basename( char *path )
{
	return basename( path );
}

/*
==================
Sys_Dirname
==================
*/
const char *Sys_Dirname( char *path )
{
	return dirname( path );
}

/*
==============
Sys_FOpen
==============
*/
FILE *Sys_FOpen( const char *ospath, const char *mode ) {
	struct stat buf;

	// check if path exists and is a directory
	if ( !stat( ospath, &buf ) && S_ISDIR( buf.st_mode ) )
		return NULL;

	return fopen( ospath, mode );
}

/*
==================
Sys_Mkdir
==================
*/
qboolean Sys_Mkdir( const char *path )
{
	int result = mkdir( path, 0750 );

	if( result != 0 )
		return errno == EEXIST;

	return qtrue;
}

/*
==================
Sys_Mkfifo
==================
*/
FILE *Sys_Mkfifo( const char *ospath )
{
	FILE	*fifo;
	int		result;
	int		fn;
	struct stat buf;

	// if file already exists AND is a pipefile, remove it
	if( !stat( ospath, &buf ) && S_ISFIFO( buf.st_mode ) )
		FS_Remove( ospath );

	result = mkfifo( ospath, 0600 );
	if( result != 0 )
		return NULL;

	fifo = fopen( ospath, "w+" );
	if( fifo )
	{
		fn = fileno( fifo );
		fcntl( fn, F_SETFL, O_NONBLOCK );
	}

	return fifo;
}

/*
==================
Sys_Cwd
==================
*/
char *Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

	char *result = getcwd( cwd, sizeof( cwd ) - 1 );
	if( result != cwd )
		return NULL;

	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

/*
==================
Sys_ListFilteredFiles
==================
*/
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char          search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char          filename[MAX_OSPATH];
	DIR           *fdir;
	struct dirent *d;
	struct stat   st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s/%s", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s", basedir );
	}

	if ((fdir = opendir(search)) == NULL) {
		return;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
			continue;

		if (st.st_mode & S_IFDIR) {
			if (Q_stricmp(d->d_name, ".") && Q_stricmp(d->d_name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s/%s", subdirs, d->d_name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	}

	closedir(fdir);
}

/*
==================
Sys_ListFiles
==================
*/
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	struct dirent *d;
	DIR           *fdir;
	qboolean      dironly = wantsubs;
	char          search[MAX_OSPATH];
	int           nfiles;
	char          **listCopy;
	char          *list[MAX_FOUND_FILES];
	int           i;
	struct stat   st;

	int           extLen;
	qboolean wantDirs;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = NULL;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension)
		extension = "";

	if (!Q_stricmpn(extension, "*wantDirs", sizeof("*wantDirs"))) {
		wantDirs = qtrue;
		extension = "";
	} else {
		wantDirs = qfalse;
	}

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	}

	extLen = strlen( extension );

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL) {
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
		if (stat(search, &st) == -1)
			continue;

#if 0
		if ((dironly && !(st.st_mode & S_IFDIR)) ||
			(!dironly && (st.st_mode & S_IFDIR)))
			continue;
#endif

		if (dironly  &&  !(st.st_mode & S_IFDIR)) {
			continue;
		}

		if (!dironly  &&  (st.st_mode & S_IFDIR)  &&  !wantDirs) {
			continue;
		}

		if (*extension) {
			if ( strlen( d->d_name ) < extLen ||
				Q_stricmp(
					d->d_name + strlen( d->d_name ) - extLen,
					extension ) ) {
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 )
			break;

		if (wantDirs  &&  (st.st_mode & S_IFDIR)) {
			if (d->d_name[0] == '.'  &&  d->d_name[1] == '\0') {
				continue;
			}
			list[nfiles] = CopyString(va("%s/", d->d_name));
		} else {
			list[ nfiles ] = CopyString( d->d_name );
		}

		nfiles++;
	}

	list[ nfiles ] = NULL;

	closedir(fdir);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

/*
==================
Sys_FreeFileList
==================
*/
void Sys_FreeFileList( char **list )
{
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}

/*
==================
Sys_Sleep

Block execution for msec or until input is recieved.
==================
*/
void Sys_Sleep( int msec )
{
	if( msec == 0 )
		return;

	if( stdinIsATTY )
	{
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);
		if( msec < 0 )
		{
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, NULL);
		}
		else
		{
			struct timeval timeout;

			timeout.tv_sec = msec/1000;
			timeout.tv_usec = (msec%1000)*1000;
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout);
		}
	}
	else
	{
		// With nothing to select() on, we can't wait indefinitely
		if( msec < 0 )
			msec = 10;

		usleep( msec * 1000 );
	}
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog( const char *error )
{
	char buffer[ 1024 ];
	unsigned int size;
	int f = -1;
	const char *homepath = Cvar_VariableString( "fs_homepath" );
	const char *gamedir = Cvar_VariableString( "fs_game" );
	const char *fileName = "crashlog.txt";
	char *dirpath = FS_BuildOSPath( homepath, gamedir, "");
	char *ospath = FS_BuildOSPath( homepath, gamedir, fileName );

	Sys_Print( va( "%s\n", error ) );

#ifndef DEDICATED
	Sys_Dialog( DT_ERROR, va( "%s. See \"%s\" for details.", error, ospath ), "Error" );
#endif

	// Make sure the write path for the crashlog exists...

	if(!Sys_Mkdir(homepath))
	{
		Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", homepath);
		return;
	}

	if(!Sys_Mkdir(dirpath))
	{
		Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", dirpath);
		return;
	}

	// We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	// which will come through here, so we don't want to recurse forever by
	// calling FS_FOpenFileWrite()...use the Unix system APIs instead.
	f = open( ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640 );
	if( f == -1 )
	{
		Com_Printf( "ERROR: couldn't open %s\n", fileName );
		return;
	}

	// We're crashing, so we don't care much if write() or close() fails.
	while( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 ) {
		if( write( f, buffer, size ) != size ) {
			Com_Printf( "ERROR: couldn't fully write to %s\n", fileName );
			break;
		}
	}

	close( f );
}

#ifndef __APPLE__
static char execBuffer[ 1024 ];
static char *execBufferPointer;
static char *execArgv[ 16 ];
static int execArgc;

/*
==============
Sys_ClearExecBuffer
==============
*/
static void Sys_ClearExecBuffer( void )
{
	execBufferPointer = execBuffer;
	Com_Memset( execArgv, 0, sizeof( execArgv ) );
	execArgc = 0;
}

/*
==============
Sys_AppendToExecBuffer
==============
*/
static void Sys_AppendToExecBuffer( const char *text )
{
	size_t size = sizeof( execBuffer ) - ( execBufferPointer - execBuffer );
	int length = strlen( text ) + 1;

	if( length > size || execArgc >= ARRAY_LEN( execArgv ) )
		return;

	Q_strncpyz( execBufferPointer, text, size );
	execArgv[ execArgc++ ] = execBufferPointer;

	execBufferPointer += length;
}

/*
==============
Sys_Exec
==============
*/
static int Sys_Exec( void )
{
	pid_t pid = fork( );

	if( pid < 0 )
		return -1;

	if( pid )
	{
		// Parent
		int exitCode;

		wait( &exitCode );

		return WEXITSTATUS( exitCode );
	}
	else
	{
		// Child
		execvp( execArgv[ 0 ], execArgv );

		// Failed to execute
		exit( -1 );

		return -1;
	}
}

/*
==============
Sys_ZenityCommand
==============
*/
static void Sys_ZenityCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "zenity" );

	switch( type )
	{
		default:
	    case DT_INFO:      Sys_AppendToExecBuffer( "--info" ); break;
		case DT_WARNING:   Sys_AppendToExecBuffer( "--warning" ); break;
		case DT_ERROR:     Sys_AppendToExecBuffer( "--error" ); break;
		case DT_YES_NO:
			Sys_AppendToExecBuffer( "--question" );
			Sys_AppendToExecBuffer( "--ok-label=Yes" );
			Sys_AppendToExecBuffer( "--cancel-label=No" );
			break;

		case DT_OK_CANCEL:
			Sys_AppendToExecBuffer( "--question" );
			Sys_AppendToExecBuffer( "--ok-label=OK" );
			Sys_AppendToExecBuffer( "--cancel-label=Cancel" );
			break;
	}

	Sys_AppendToExecBuffer( va( "--text=%s", message ) );
	Sys_AppendToExecBuffer( va( "--title=%s", title ) );
}

/*
==============
Sys_KdialogCommand
==============
*/
static void Sys_KdialogCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "kdialog" );

	switch( type )
	{
		default:
		case DT_INFO:      Sys_AppendToExecBuffer( "--msgbox" ); break;
		case DT_WARNING:   Sys_AppendToExecBuffer( "--sorry" ); break;
		case DT_ERROR:     Sys_AppendToExecBuffer( "--error" ); break;
		case DT_YES_NO:    Sys_AppendToExecBuffer( "--warningyesno" ); break;
		case DT_OK_CANCEL: Sys_AppendToExecBuffer( "--warningcontinuecancel" ); break;
	}

	Sys_AppendToExecBuffer( message );
	Sys_AppendToExecBuffer( va( "--title=%s", title ) );
}

/*
==============
Sys_XmessageCommand
==============
*/
static void Sys_XmessageCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "xmessage" );
	Sys_AppendToExecBuffer( "-buttons" );

	switch( type )
	{
		default:           Sys_AppendToExecBuffer( "OK:0" ); break;
		case DT_YES_NO:    Sys_AppendToExecBuffer( "Yes:0,No:1" ); break;
		case DT_OK_CANCEL: Sys_AppendToExecBuffer( "OK:0,Cancel:1" ); break;
	}

	Sys_AppendToExecBuffer( "-center" );
	Sys_AppendToExecBuffer( message );
}

/*
==============
Sys_Dialog

Display a *nix dialog box
==============
*/
dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title )
{
	typedef enum
	{
		NONE = 0,
		ZENITY,
		KDIALOG,
		XMESSAGE,
		NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef void (*dialogCommandBuilder_t)( dialogType_t, const char *, const char * );

	const char              *session = getenv( "DESKTOP_SESSION" );
	qboolean                tried[ NUM_DIALOG_PROGRAMS ] = { qfalse };
	dialogCommandBuilder_t  commands[ NUM_DIALOG_PROGRAMS ] = { NULL };
	dialogCommandType_t     preferredCommandType = NONE;
	int						i;

	commands[ ZENITY ] = &Sys_ZenityCommand;
	commands[ KDIALOG ] = &Sys_KdialogCommand;
	commands[ XMESSAGE ] = &Sys_XmessageCommand;

	// This may not be the best way
	if( !Q_stricmp( session, "gnome" ) )
		preferredCommandType = ZENITY;
	else if( !Q_stricmp( session, "kde" ) )
		preferredCommandType = KDIALOG;

	for( i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++ )
	{
		if( preferredCommandType != NONE && preferredCommandType != i )
			continue;

		if( !tried[ i ] )
		{
			int exitCode;

			commands[ i ]( type, message, title );
			exitCode = Sys_Exec( );

			if( exitCode >= 0 )
			{
				switch( type )
				{
				case DT_YES_NO:    return exitCode ? DR_NO : DR_YES;
				case DT_OK_CANCEL: return exitCode ? DR_CANCEL : DR_OK;
				default:           return DR_OK;
				}
			}

			tried[ i ] = qtrue;

			// The preference failed, so start again in order
			if( preferredCommandType != NONE )
			{
				preferredCommandType = NONE;
				i = NONE + 1;
			}
		}
	}

	Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to show a dialog\n" );
	return DR_OK;
}
#endif

/*
==============
Sys_GLimpSafeInit

Unix specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit( void )
{
	// NOP
}

/*
==============
Sys_GLimpInit

Unix specific GL implementation initialisation
==============
*/
void Sys_GLimpInit( void )
{
	// NOP
}

void Sys_SetFloatEnv(void)
{
	// rounding towards nearest
	fesetround(FE_TONEAREST);
}

/*
==============
Sys_PlatformInit

Unix specific initialisation
==============
*/

#if defined(__APPLE__)  ||  DEDICATED

#include <execinfo.h>

// from backtrace() man page
static void print_dl_backtrace (void)
{
	void *callstack[128];
	int i;
	int frames;
	char **strs;

	frames = backtrace(callstack, 128);
	strs = backtrace_symbols(callstack, frames);
	for (i = 0;  i < frames;  i++) {
		fprintf(stderr, "%s\n", strs[i]);
	}
}

static int print_gdb_trace (void)
{
	//FIXME test gdb?

	//FIXME testing
	//print_dl_backtrace();

	return 0;
}

static void signal_crash (int signum, siginfo_t *info, void *ptr)
{
	fprintf(stderr, "crash with signal %d\n", signum);
	print_dl_backtrace();
	exit(-1);
}

#else

/*
 *
 * This source xxx is used to print out a stack-trace when your program
 * segfaults. It is relatively reliable and spot-on accurate.
 *
 * This code is in the public domain. Use it as you see fit, some credit
 * would be appreciated, but is not a prerequisite for usage. Feedback
 * on it's use would encourage further development and maintenance.
 *
 * Due to a bug in gcc-4.x.x you currently have to compile as C++ if you want
 * demangling to work.
 *
 * Please note that it's been ported into my ULS library, thus the check for
 * HAS_ULSLIB and the use of the sigsegv_outp macro based on that define.
 *
 * Author: Jaco Kroon <jaco@kroon.co.za>
 *
 * Copyright (C) 2005 - 2010 Jaco Kroon
 *
 *

 * the following is based upon the above
 *FIXME dladdr() sucks since it can't get symbols for static functions
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
#ifndef __USE_GNU
#define __USE_GNU
#endif
*/

/* Bug in gcc prevents from using CPP_DEMANGLE in pure "C" */
#if !defined(__cplusplus) && !defined(NO_CPP_DEMANGLE)
#define NO_CPP_DEMANGLE
#endif

#include <ucontext.h>

//#include <dlfcn.h>
#ifndef NO_CPP_DEMANGLE
#include <cxxabi.h>
#ifdef __cplusplus
using __cxxabiv1::__cxa_demangle;
#endif
#endif

#ifdef HAS_ULSLIB
#include "uls/logger.h"
#define sigsegv_outp(x)         sigsegv_outp(,gx)
#else
#define sigsegv_outp(x, ...)    fprintf(stderr, x "\n", ##__VA_ARGS__)
#endif

#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
//#error lskdjflskdjflksdjf
# define REGFORMAT "%016llx"
#elif defined(REG_EIP)
//#error ssss
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
#error slkdjf
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

//FIXME linux only :  /proc/self/exe
static int print_gdb_trace (void)
{
    char pid_buf[30];
    char name_buf[512];
	int child_pid;

    sprintf(pid_buf, "%d", getpid());
    name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
    child_pid = fork();
	if (child_pid < 0) {
		return -1;
	}

    if (child_pid == 0) {
        dup2(2, 1);   // redirect output to stderr
        fprintf(stdout, "stack trace for %s pid=%s\n", name_buf, pid_buf);
        execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt full", name_buf, pid_buf, NULL);
		exit(1);  // should only get here if execlp() failed
    } else {
		int status;
        waitpid(child_pid, &status, 0);

		if (WEXITSTATUS(status) == 0) {
			return 0;
		} else {
			printf("couldn't execute gdb\n");
			return -1;
		}
    }

	return 0;
}

static void signal_crash (int signum, siginfo_t *info, void *ptr)
{
	static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};

	int i, f = 0;
	ucontext_t *ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;
	void **bp = 0;
	void *ip = 0;

	//printf("here we go...\n");
	if (print_gdb_trace() != -1) {
		//return;
		exit(1);
	}

	printf("fallback trace...\n");

	sigsegv_outp("signal string: %s\n", strsignal(signum));
	sigsegv_outp("info.si_signo = %d", signum);
	sigsegv_outp("info.si_errno = %d", info->si_errno);
	sigsegv_outp("info.si_code  = %d (%s)", info->si_code, si_codes[info->si_code]);
	sigsegv_outp("info.si_addr  = %p", info->si_addr);
	for(i = 0; i < NGREG; i++)
		sigsegv_outp("reg[%02d]       = 0x" REGFORMAT, i, ucontext->uc_mcontext.gregs[i]);

#ifndef SIGSEGV_NOSTACK
#if defined(SIGSEGV_STACK_IA64) || defined(SIGSEGV_STACK_X86)
#if defined(SIGSEGV_STACK_IA64)
	ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
	bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
#elif defined(SIGSEGV_STACK_X86)
	ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
	bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
#endif

#ifdef SIGSEGV_STACK_X86
	sigsegv_outp("Stack trace: eip %p  ip %p  bp %p", (void *)ucontext->uc_mcontext.gregs[REG_EIP], ip, bp);

	fprintf (stderr, "    gs: 0x%08x   fs: 0x%08x   es: 0x%08x   ds: 0x%08x\n"
               "   edi: 0x%08x  esi: 0x%08x  ebp: 0x%08x  esp: 0x%08x\n"
               "   ebx: 0x%08x  edx: 0x%08x  ecx: 0x%08x  eax: 0x%08x\n"
               "  trap:   %8u  err: 0x%08x  eip: 0x%08x   cs: 0x%08x\n"
			 "  flag: 0x%08x   sp: 0x%08x   ss: 0x%08x  cr2: 0x%08lx\n",
			 ucontext->uc_mcontext.gregs [REG_GS],     ucontext->uc_mcontext.gregs [REG_FS],   ucontext->uc_mcontext.gregs [REG_ES],  ucontext->uc_mcontext.gregs [REG_DS],
			 ucontext->uc_mcontext.gregs [REG_EDI],    ucontext->uc_mcontext.gregs [REG_ESI],  ucontext->uc_mcontext.gregs [REG_EBP], ucontext->uc_mcontext.gregs [REG_ESP],
			 ucontext->uc_mcontext.gregs [REG_EBX],    ucontext->uc_mcontext.gregs [REG_EDX],  ucontext->uc_mcontext.gregs [REG_ECX], ucontext->uc_mcontext.gregs [REG_EAX],
			 ucontext->uc_mcontext.gregs [REG_TRAPNO], ucontext->uc_mcontext.gregs [REG_ERR],  ucontext->uc_mcontext.gregs [REG_EIP], ucontext->uc_mcontext.gregs [REG_CS],
			 ucontext->uc_mcontext.gregs [REG_EFL],    ucontext->uc_mcontext.gregs [REG_UESP], ucontext->uc_mcontext.gregs [REG_SS],  ucontext->uc_mcontext.cr2
			 );
#endif

#if 0
	if (ip) {
		//if (dladdr(ip - 21, &dlinfo)) {
		if (dladdr(0x80bb12c, &dlinfo)) {
			const char *symname = dlinfo.dli_sname;
			sigsegv_outp("%s\n", symname);
		}
	}
#endif

	while(bp && ip) {
		if(!dladdr(ip, &dlinfo))
			break;

		const char *symname = dlinfo.dli_sname;

#ifndef NO_CPP_DEMANGLE
		int status;
		char * tmp = __cxa_demangle(symname, NULL, 0, &status);

		if (status == 0 && tmp)
			symname = tmp;
#endif

		sigsegv_outp("% 2d: %p <%s+%lx> (%s)",
					 ++f,
					 ip,
					 symname,
					 (unsigned long)ip - (unsigned long)dlinfo.dli_saddr,
					 dlinfo.dli_fname);

#ifndef NO_CPP_DEMANGLE
		if (tmp)
			free(tmp);
#endif

		if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
			break;

		ip = bp[1];
		bp = (void**)bp[0];
	}
#else
	sigsegv_outp("Stack trace (non-dedicated):");
	//FIXME sz and strings not defined
	{
		int sz;
		char **strings;

	sz = backtrace(bt, 20);
	strings = backtrace_symbols(bt, sz);
	for(i = 0; i < sz; ++i)
		sigsegv_outp("%s", strings[i]);
	}

#endif
	sigsegv_outp("End of stack trace.");
#else
	sigsegv_outp("Not printing stack strace.");
#endif
	_exit (-1);
}

#endif

void Sys_PlatformInit (qboolean useBacktrace, qboolean useConsoleOutput, qboolean useDpiAware)
{
	struct sigaction action;
	const char *term = getenv("TERM");
	struct utsname un;
	int err;

	if (useBacktrace) {
		memset(&action, 0, sizeof(action));
		action.sa_sigaction = signal_crash;
		action.sa_flags = SA_SIGINFO;
		if(sigaction(SIGSEGV, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGSEGV handler\n");
		}
		if(sigaction(SIGHUP, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGHUP handler\n");
		}
		if(sigaction(SIGTRAP, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGTRAP handler\n");
		}
		if(sigaction(SIGABRT, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGABRT handler\n");
		}
		if(sigaction(SIGBUS, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGBUS handler\n");
		}
		if(sigaction(SIGILL, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGILL handler\n");
		}
		if(sigaction(SIGFPE, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGFPE handler\n");
		}
		if(sigaction(SIGTERM, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGTERM handler\n");
		}
		if(sigaction(SIGINT, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGINT handler\n");
		}
	} else {
		signal(SIGHUP, Sys_SigHandler);
		signal(SIGQUIT, Sys_SigHandler);
		signal(SIGTRAP, Sys_SigHandler);
		signal(SIGABRT, Sys_SigHandler);
		signal(SIGBUS, Sys_SigHandler);
	}

	Sys_SetFloatEnv();

	stdinIsATTY = isatty( STDIN_FILENO ) &&
		!( term && ( !strcmp( term, "raw" ) || !strcmp( term, "dumb" ) ) );

	err = uname(&un);
	if (err == -1) {
		Com_Printf("^1couldn't get uname() info\n");
	} else {
		Com_Printf("%s, %s, %s, %s\n", un.sysname, un.release, un.version, un.machine);
	}

#if defined(__linux__)   ||  defined(__APPLE__)
	{
		FILE *f;
		char buf[32];
		int n;
#if defined(__linux__)
		const char *fname = "/etc/os-release";
#elif defined(__APPLE__)
		const char *fname = "/System/Library/CoreServices/SystemVersion.plist";
#else
		const char *fname = NULL;
#endif

		if (fname) {
			f = fopen(fname, "r");
			if (f) {
				while (1) {
					n = fread(buf, 1, sizeof(buf) - 1, f);
					if (n < 1) {
						break;
					}
					if (ferror(f)) {
						Com_Printf("^1couldn't read %s\n", fname);
						break;
					}
					buf[n] = '\0';
					Com_Printf("%s", buf);
				}
				Com_Printf("\n");
				fclose(f);
			}
		}
	}
#elif 0  //defined(__APPLE__)
	{
		SInt32 majorVersion, minorVersion, bugFixVersion;

		Gestalt(gestaltSystemVersionMajor, &majorVersion);
		Gestalt(gestaltSystemVersionMinor, &minorVersion);
		Gestalt(gestaltSystemVersionBugFix, &bugFixVersion);

		//FIXME SInt32 on 64 bit
		Com_Printf("Mac OS X %d.%d.%d\n", (int)majorVersion, (int)minorVersion, (int)bugFixVersion);
	}
#endif
}

/*
==============
Sys_PlatformExit

Unix specific deinitialisation
==============
*/
void Sys_PlatformExit( void )
{
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/

void Sys_SetEnv(const char *name, const char *value)
{
	if(value && *value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}

/*
==============
Sys_PID
==============
*/
int Sys_PID( void )
{
	return getpid( );
}

/*
==============
Sys_PIDIsRunning
==============
*/
qboolean Sys_PIDIsRunning( int pid )
{
	return kill( pid, 0 ) == 0;
}

/*
=================
Sys_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
void Sys_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAX_PRINT_MSG ];
	int         length = 0;
	static int  q3ToAnsi[ 8 ] =
	{
		30, // COLOR_BLACK
		31, // COLOR_RED
		32, // COLOR_GREEN
		33, // COLOR_YELLOW
		34, // COLOR_BLUE
		36, // COLOR_CYAN
		35, // COLOR_MAGENTA
		0   // COLOR_WHITE
	};

	while( *msg )
	{
		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if( *msg == '\n' )
			{
				// Issue a reset and then the newline
				fputs( "\033[0m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				Com_sprintf( buffer, sizeof( buffer ), "\033[%dm",
						q3ToAnsi[ ColorIndex( *( msg + 1 ) ) ] );
				fputs( buffer, stderr );
				msg += 2;
			}
		}
		else
		{
			if( length >= MAX_PRINT_MSG - 1 )
				break;

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

void Sys_Backtrace_f (void)
{
	print_gdb_trace();
}

void Sys_OpenQuakeLiveDirectory (void)
{
	char *path;

	path = Cvar_VariableString("fs_quakelivedir");
	if (!*path) {
		path = Sys_QuakeLiveDir();
	}

#ifdef __APPLE__
	system(va("open \"%s\"&", path));
#else
	system(va("xdg-open \"%s\"&", path));
#endif
}

void Sys_OpenWolfcamDirectory (void)
{
	char *path;

	path = Cvar_VariableString("fs_homepath");
	if (!*path) {
		path = Sys_DefaultHomePath();
	}

#ifdef __APPLE__
	system(va("open \"%s\"&", path));
#else
	system(va("xdg-open \"%s\"&", path));
#endif
}

int Sys_DirnameCmp (const char *pathName1, const char *pathName2)
{
#ifdef __APPLE__
	return Q_stricmp(pathName1, pathName2);
#else
	return strcmp(pathName1, pathName2);
#endif
}

// popen

typedef struct {
	FILE *fp;
	int lastErrno;
} popenDataUnix_t;

// need to free() returned data
popenData_t *Sys_PopenAsync (const char *command)
{
	popenData_t *p;
	popenDataUnix_t *pu;

	int fd;
	int flags;

	p = (popenData_t *)malloc(sizeof(popenData_t));
	if (p == NULL) {
		Com_Printf("^1%s couldn't allocate memory\n", __FUNCTION__);
		return NULL;
	}
	memset(p, 0, sizeof(popenData_t));

	pu = (popenDataUnix_t *)malloc(sizeof(popenDataUnix_t));
	if (pu == NULL) {
		Com_Printf("^1%s couldn't get unix data\n", __FUNCTION__);
		free(p);
		return NULL;
	}
	memset(pu, 0, sizeof(popenDataUnix_t));
	p->data = pu;

	pu->fp = popen(command, "r");
	if (pu->fp == NULL) {
		Com_Printf("^1%s couldn't launch process '%s'\n", __FUNCTION__, command);
		free(pu);
		free(p);
		return NULL;
	}

	// set non-blocking
	fd = fileno(pu->fp);
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	pu->lastErrno = EWOULDBLOCK;

	return p;
}

void Sys_PopenClose (popenData_t *p)
{
	popenDataUnix_t *pu;
	int err;
	int ret;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return;
	}

	pu = p->data;

	if (pu->fp == NULL) {
		Com_Printf("^1%s invalid file\n", __FUNCTION__);
		return;
	}

	ret = pclose(pu->fp);
	err = errno;

	Com_Printf("popen done: %d  %d\n", ret, err);
	if (ret != 0) {
		Com_Printf("  %d: %s\n", err, strerror(err));
	}

	free(p->data);
	//pu->fp = NULL;
}

char *Sys_PopenGetLine (popenData_t *p, char *buffer, int size)
{
	popenDataUnix_t *pu;
	char *retp;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return NULL;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return NULL;
	}

	pu = p->data;

	if (pu->fp == NULL) {
		Com_Printf("^1%s invalid file\n", __FUNCTION__);
		return NULL;
	}

	if (buffer == NULL) {
		Com_Printf("^1%s buffer == NULL\n", __FUNCTION__);
		return NULL;
	}

	if (size <= 0) {
		Com_Printf("^1%s invalid size %d\n", __FUNCTION__, size);
		return NULL;
	}

	retp = fgets(buffer, size, pu->fp);
	pu->lastErrno = errno;

	return retp;
}

qboolean Sys_PopenIsDone (popenData_t *p)
{
	popenDataUnix_t *pu;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return qfalse;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return qfalse;
	}

	pu = p->data;

	if (pu->fp == NULL) {
		Com_Printf("^1%s invalid file\n", __FUNCTION__);
		return qfalse;
	}

	if (pu->lastErrno != EWOULDBLOCK) {
		return qtrue;
	}

	if (feof(pu->fp)) {
		return qtrue;
	}

	return qfalse;
}

const char *Sys_GetSteamCmd (void)
{
	const char *s;

	s = Cvar_VariableString("fs_steamcmd");

	if (s  &&  *s) {
		return s;
	} else {
		return "steamcmd.sh";
	}
}

/*
=================
Sys_DllExtension

Check if filename should be allowed to be loaded as a DLL.
=================
*/
qboolean Sys_DllExtension( const char *name ) {
	const char *p;
	char c = 0;

	if ( COM_CompareExtension( name, DLL_EXT ) ) {
		return qtrue;
	}

	// Check for format of filename.so.1.2.3
	p = strstr( name, DLL_EXT "." );

	if ( p ) {
		p += strlen( DLL_EXT );

		// Check if .so is only followed for periods and numbers.
		while ( *p ) {
			c = *p;

			if ( !isdigit( c ) && c != '.' ) {
				return qfalse;
			}

			p++;
		}

		// Don't allow filename to end in a period. file.so., file.so.0., etc
		if ( c != '.' ) {
			return qtrue;
		}
	}

	return qfalse;
}

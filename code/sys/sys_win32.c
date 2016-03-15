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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <process.h>


// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };
static char QuakeLivePath[MAX_OSPATH] = { 0 };

#ifndef DEDICATED
static UINT timerResolution = 0;
#endif

/*
================
Sys_DefaultHomePath
================
*/
char *Sys_DefaultHomePath( void )
{
	TCHAR szPath[MAX_PATH];
	FARPROC qSHGetFolderPath;
	HMODULE shfolder;

	if( !*homePath )
	{
		shfolder = LoadLibrary("shfolder.dll");

		if(shfolder == NULL)
		{
			Com_Printf("Unable to load SHFolder.dll\n");
			return NULL;
		}

		// SHGetFolderPath() is deprecated since Vista
		qSHGetFolderPath = GetProcAddress(shfolder, "SHGetFolderPathA");
		if(qSHGetFolderPath == NULL)
		{
			Com_Printf("Unable to find SHGetFolderPath in SHFolder.dll\n");
			FreeLibrary(shfolder);
			return NULL;
		}

		if( !SUCCEEDED( qSHGetFolderPath( NULL, CSIDL_APPDATA,
						NULL, 0, szPath ) ) )
		{
			Com_Printf("Unable to detect CSIDL_APPDATA\n");
			FreeLibrary(shfolder);
			return NULL;
		}
		Q_strncpyz( homePath, szPath, sizeof( homePath ) );
		Q_strcat( homePath, sizeof( homePath ), "\\wolfcamql" );
		FreeLibrary(shfolder);
	}

	return homePath;
}

char *Sys_QuakeLiveDir (void)
{
	TCHAR szPath[MAX_PATH];
	FARPROC qSHGetFolderPath;
	HMODULE shfolder;
	const char *override = NULL;
	// "LocalLow\\id Software\\quakelive\\home\\baseq3";
	// "id Software\\quakelive\\home\\baseq3";
	const char *win7Vista = "LocalLow\\id Software\\quakelive\\home";
	const char *xp = "id Software\\quakelive\\home";
	const char *steamPath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Quake Live";
	const char *steamPath32bit = "C:\\Program Files\\Steam\\steamapps\\common\\Quake Live";
	HANDLE hFind;
	WIN32_FIND_DATA FindData;
	char searchString[MAX_OSPATH];
	int count;

	if (!*QuakeLivePath) {
		override = Cvar_VariableString("fs_quakelivedir");
		if (override  &&  *override) {
			Q_strncpyz(QuakeLivePath, override, sizeof(QuakeLivePath));
			//FS_ReplaceSeparators(QuakeLivePath);
			return QuakeLivePath;
		}

		// check steam first
	    // C:\\Program Files (x86)\\Steam\\steamapps\\common\\Quake Live\\12345678901234567\\baseq3

		count = 0;
		while (count < 2) {
			const char *spath = steamPath;

			if (count == 1) {
				spath = steamPath32bit;
			}

			Com_sprintf(searchString, sizeof(searchString), "%s\\*", spath);

			//Com_Printf("searching for '%s'\n", searchString);
			hFind = FindFirstFile(searchString, &FindData);
			if (hFind != INVALID_HANDLE_VALUE) {

				do {
					if (FindData.cFileName[0] == '.') {  // skip .  and ..
						continue;
					}
					Com_sprintf(searchString, sizeof(searchString), "%s\\%s\\baseq3", spath, FindData.cFileName);
					//Com_Printf("checking '%s'\n", searchString);
					if (Sys_FileIsDirectory(searchString)) {
						// got it
						//Com_Printf("found steam directory '%s'\n", FindData.cFileName);
						Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s\\%s", spath, FindData.cFileName);
						FindClose(hFind);
						return QuakeLivePath;
					} else {
						//Com_Printf("found steam file '%s' but it is not a directory\n", FindData.cFileName);
					}

				} while(FindNextFile(hFind, &FindData));
			}
			FindClose(hFind);
			//Com_Printf("(%s) didn't find steam directory...\n", count == 0 ? "64-bit" : "32-bit");
			count++;
		}  // while (count < 2)

		// try old quake live stand alone directory
		shfolder = LoadLibrary("shfolder.dll");

		if(shfolder == NULL)
		{
			Com_Printf("Unable to load SHFolder.dll\n");
			return NULL;
		}

		qSHGetFolderPath = GetProcAddress(shfolder, "SHGetFolderPathA");
		if(qSHGetFolderPath == NULL)
		{
			Com_Printf("Unable to find SHGetFolderPath in SHFolder.dll\n");
			FreeLibrary(shfolder);
			return NULL;
		}

		if( !SUCCEEDED( qSHGetFolderPath( NULL, CSIDL_APPDATA,
						NULL, 0, szPath ) ) )
		{
			Com_Printf("Unable to detect CSIDL_APPDATA\n");
			FreeLibrary(shfolder);
			return NULL;
		}
		/*

		  Vista/win7 ->
		  %AppData%\LocalLow\id Software\quakelive\home\baseq3

		  XP ->
		  %appdata%\id Software\quakelive\home\baseq3

		*/
		Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s\\..\\%s", szPath, win7Vista);
		if (_access(QuakeLivePath, 0) == -1) {
			Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s\\%s", szPath, xp);
			if (_access(QuakeLivePath, 0) == -1) {
				// oh well
				Com_Printf("^1couldn't find quake live directory\n");
			}
		}
		FS_ReplaceSeparators(QuakeLivePath);
		FreeLibrary(shfolder);
	}

	//Com_Printf("qpath '%s'\n", QuakeLivePath);
	return QuakeLivePath;
}

/*
================
Sys_Milliseconds
================
*/
int sys_timeBase;
int Sys_Milliseconds (void)
{
	int             sys_curtime;
	static qboolean initialized = qfalse;

	if (!initialized) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	HCRYPTPROV  prov;

	if( !CryptAcquireContext( &prov, NULL, NULL,
		PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )  {

		return qfalse;
	}

	if( !CryptGenRandom( prov, len, (BYTE *)string ) )  {
		CryptReleaseContext( prov, 0 );
		return qfalse;
	}
	CryptReleaseContext( prov, 0 );
	return qtrue;
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser( void )
{
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );

	if( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	//char *cliptext;
	WCHAR *wcliptext;
	int nchars;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

#if 0
		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				if (data == NULL) {
					Com_Printf("^1Sys_GetClipboardData:  couldn't allocate memory for data\n");
					GlobalUnlock(hClipboardData);
					CloseClipboard();
					return NULL;
				}
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}
#endif

		if ( ( hClipboardData = GetClipboardData( CF_UNICODETEXT ) ) != 0 ) {
			if ( ( wcliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				nchars = WideCharToMultiByte(
											 CP_UTF8,
											 0,
											 wcliptext,
											 GlobalSize(hClipboardData),
											 NULL,
											 0,
											 NULL,
											 NULL);
				if (nchars <= 0) {
					GlobalUnlock(hClipboardData);
					return NULL;
				}

				data = Z_Malloc(nchars + 1);
				if (data == NULL) {
					Com_Printf("^1Sys_GetClipboardData:  couldn't allocate memory for data\n");
					GlobalUnlock(hClipboardData);
					CloseClipboard();
					return NULL;
				}
				Com_Memset(data, 0, nchars + 1);

				WideCharToMultiByte(
									CP_UTF8,
									0,
									wcliptext,
									GlobalSize(hClipboardData),
									data,
									nchars,
									NULL,
									NULL);
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

#define MEM_THRESHOLD 96*1024*1024

/*
==================
Sys_LowPhysicalMemory
==================
*/
qboolean Sys_LowPhysicalMemory( void )
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus (&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==============
Sys_Basename
==============
*/
const char *Sys_Basename( char *path )
{
	static char base[ MAX_OSPATH ] = { 0 };
	int length;

	length = strlen( path ) - 1;

	// Skip trailing slashes
	while( length > 0 && path[ length ] == '\\' )
		length--;

	while( length > 0 && path[ length - 1 ] != '\\' )
		length--;

	Q_strncpyz( base, &path[ length ], sizeof( base ) );

	length = strlen( base ) - 1;

	// Strip trailing slashes
	while( length > 0 && base[ length ] == '\\' )
    base[ length-- ] = '\0';

	return base;
}

/*
==============
Sys_Dirname
==============
*/
const char *Sys_Dirname( char *path )
{
	static char dir[ MAX_OSPATH ] = { 0 };
	int length;

	Q_strncpyz( dir, path, sizeof( dir ) );
	length = strlen( dir ) - 1;

	while( length > 0 && dir[ length ] != '\\' )
		length--;

	dir[ length ] = '\0';

	return dir;
}

/*
==============
Sys_FOpen
==============
*/
FILE *Sys_FOpen( const char *ospath, const char *mode ) {
	return fopen( ospath, mode );
}

/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir( const char *path )
{
	if( !CreateDirectory( path, NULL ) )
	{
		if( GetLastError( ) != ERROR_ALREADY_EXISTS )
			return qfalse;
	}

	return qtrue;
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
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
==============
Sys_ListFilteredFiles
==============
*/
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	intptr_t			findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}

/*
==============
strgtr
==============
*/
static qboolean strgtr(const char *s0, const char *s1)
{
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

/*
==============
Sys_ListFiles
==============
*/
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t			findhandle;
	int			flag;
	int			i;
	qboolean wantDirs;
	int extLen;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
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

	if ( !extension) {
		extension = "";
	}

	if (!Q_stricmpn(extension, "*wantDirs", sizeof("*wantDirs"))) {
		wantDirs = qtrue;
		extension = "";
	} else {
		wantDirs = qfalse;
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	extLen = strlen( extension );

	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR)  ||  wantDirs ) {
			if (*extension) {
				if ( strlen( findinfo.name ) < extLen ||
					 Q_stricmp(
							   findinfo.name + strlen( findinfo.name ) - extLen,
							   extension ) ) {
					continue; // didn't match
				}
			}
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			if (wantDirs  &&  findinfo.attrib & _A_SUBDIR) {
				if (findinfo.name[0] == '.'  &&  findinfo.name[1] == '\0') {
					continue;
				}
				list[nfiles] = CopyString(va("%s/", findinfo.name));
			} else {
				list[ nfiles ] = CopyString( findinfo.name );
			}
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

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

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

/*
==============
Sys_FreeFileList
==============
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
==============
Sys_Sleep

Block execution for msec or until input is received.
==============
*/
void Sys_Sleep( int msec )
{
	if( msec == 0 )
		return;

#ifdef DEDICATED
	if( msec < 0 )
		WaitForSingleObject( GetStdHandle( STD_INPUT_HANDLE ), INFINITE );
	else
		WaitForSingleObject( GetStdHandle( STD_INPUT_HANDLE ), msec );
#else
	// Client Sys_Sleep doesn't support waiting on stdin
	if( msec < 0 )
		return;

	Sleep( msec );
#endif
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog( const char *error )
{
	if( MessageBox( NULL, va( "%s\nCopy console log to clipboard?", error ),
			NULL, MB_YESNO|MB_ICONERROR ) == IDYES )
	{
		HGLOBAL memoryHandle;
		char *clipMemory;

		memoryHandle = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, CON_LogSize( ) + 1 );
		clipMemory = (char *)GlobalLock( memoryHandle );

		if( clipMemory )
		{
			char *p = clipMemory;
			char buffer[ 1024 ];
			unsigned int size;

			while( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 )
			{
				Com_Memcpy( p, buffer, size );
				p += size;
			}

			*p = '\0';

			if( OpenClipboard( NULL ) && EmptyClipboard( ) )
				SetClipboardData( CF_TEXT, memoryHandle );

			GlobalUnlock( clipMemory );
			CloseClipboard( );
		}
	}
}

#ifndef DEDICATED
static qboolean SDL_VIDEODRIVER_externallySet = qfalse;
#endif

/*
==============
Sys_GLimpSafeInit

Windows specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit( void )
{
#ifndef DEDICATED
	if( !SDL_VIDEODRIVER_externallySet )
	{
		// Here, we want to let SDL decide what do to unless
		// explicitly requested otherwise
		_putenv( "SDL_VIDEODRIVER=" );
	}
#endif
}

/*
==============
Sys_GLimpInit

Windows specific GL implementation initialisation
==============
*/
void Sys_GLimpInit( void )
{
#ifndef DEDICATED
	if( !SDL_VIDEODRIVER_externallySet )
	{
		// It's a little bit weird having in_mouse control the
		// video driver, but from ioq3's point of view they're
		// virtually the same except for the mouse input anyway
		if( Cvar_VariableIntegerValue( "in_mouse" ) == -1 )
		{
			// Use the windib SDL backend, which is closest to
			// the behaviour of idq3 with in_mouse set to -1
			_putenv( "SDL_VIDEODRIVER=windib" );
		}
		else
		{
			// Use the DirectX SDL backend
			_putenv( "SDL_VIDEODRIVER=directx" );
		}
	}
#endif
}

static HANDLE HStdout = INVALID_HANDLE_VALUE;
static CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
static qboolean GotHandle = qfalse;

typedef BOOL (*SetProcessDPIAwareFunc)(void);
typedef HRESULT (*SetProcessDpiAwarenessFunc)(int);

/*
==============
Sys_PlatformInit

Windows specific initialisation
==============
*/
void Sys_PlatformInit (qboolean useBacktrace, qboolean useConsoleOutput, qboolean useDpiAware)
{
	HMODULE bt;
	OSVERSIONINFO vi;
	const char *osname;
	int ret;
	const char *arch;  // = "x86";
#ifndef DEDICATED
	TIMECAPS ptc;
	const char *SDL_VIDEODRIVER;
#endif
	HMODULE shcore = NULL;
	HMODULE user32 = NULL;
	HRESULT hr;
	SetProcessDpiAwarenessFunc fpSetProcessDpiAwareness;
	SetProcessDPIAwareFunc fpSetProcessDPIAware;
	qboolean setDpiAware = qfalse;

	// set dpi awareness

	//FIXME as option?
	// Not using manifest since it might not run with Windows XP.  Using a
	// manifest would allow reporting of the correct OSVERSIONINFO.
	// It is suggested to call *DpiAware* functions as early as possible.
	if (useDpiAware) {
		shcore = LoadLibrary("shcore.dll");
		if (shcore == NULL) {
			//FIXME printing
			Com_Printf("shcore.dll not available\n");
		} else {
			fpSetProcessDpiAwareness = (SetProcessDpiAwarenessFunc)GetProcAddress(shcore, "SetProcessDpiAwareness");
			if (fpSetProcessDpiAwareness == NULL) {
				//FIXME printing
				Com_Printf("couldn't get function SetProcessDpiAwareness\n");
			} else {
				hr = fpSetProcessDpiAwareness(2 /*PROCESS_PER_MONITOR_DPI_AWARE*/);
				if (hr != S_OK) {
					//FIXME  printing
					// this can happen if the application property has already been set with a dpi awareness value
					Com_Printf("couldn't use SetProcessDpiAwareness (%ld) : %lu\n", hr, GetLastError());
				} else {
					Com_Printf("SetProcessDpiAwareness\n");
					setDpiAware = qtrue;
				}
			}

			FreeLibrary(shcore);
		}

		if (!setDpiAware) {
			// try SetProcessDPIAware()
			user32 = LoadLibrary("user32.dll");
			if (user32 == NULL) {
				//FIXME printing
				Com_Printf("user32.dll not available\n");
			} else {
				fpSetProcessDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(user32, "SetProcessDPIAware");
				if (fpSetProcessDPIAware == NULL) {
					//FIXME printing
					Com_Printf("couldn't get function SetProcessDPIAware\n");
				} else {
					BOOL r;

					r = fpSetProcessDPIAware();
					if (r == 0) {
						//FIXME printing
						Com_Printf("SetProcessDPIAware failed (%d) : %ld\n", r, GetLastError());
					} else {
						Com_Printf("SetProcessDPIAware\n");
						setDpiAware = qtrue;
					}
				}
				FreeLibrary(user32);
			}
		}
	}

	// sdl redirects output, steal it back
	if (useConsoleOutput) {
		BOOL b = AttachConsole(ATTACH_PARENT_PROCESS);
		if (b) {
			freopen("conin$", "r", stdin);
			freopen("conout$", "w", stdout);
			freopen("conout$", "w", stderr);

			HStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			if (HStdout != INVALID_HANDLE_VALUE) {

				if (GetConsoleScreenBufferInfo(HStdout, &ScreenBufferInfo) != 0) {
					SetConsoleTextAttribute(HStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					fputs("\r\n", stderr);
					Com_Printf("win32:  using console output\n");
					GotHandle = qtrue;
				} else {
					Com_Printf("win32:  couldn't get screen buffer information for console output, error %ld\n", GetLastError());
					HStdout = INVALID_HANDLE_VALUE;
					GotHandle = qfalse;
				}
			} else {
				Com_Printf("win32:  couldn't get output handle for console output, error %ld\n", GetLastError());
				GotHandle = qfalse;
			}
		} else {
			Com_Printf("win32:  couldn't get parent console for console output, error %ld\n", GetLastError());
		}
	}

	if (useBacktrace) {
		bt = LoadLibraryA("backtrace.dll");
		Com_Printf("backtrace: %d\n", (int)bt);
	}

#ifndef DEDICATED
	SDL_VIDEODRIVER = getenv( "SDL_VIDEODRIVER" );

	if( SDL_VIDEODRIVER )
	{
		Com_Printf( "SDL_VIDEODRIVER is externally set to \"%s\", "
				"in_mouse -1 will have no effect\n", SDL_VIDEODRIVER );
		SDL_VIDEODRIVER_externallySet = qtrue;
	}
	else
		SDL_VIDEODRIVER_externallySet = qfalse;

	if(timeGetDevCaps(&ptc, sizeof(ptc)) == MMSYSERR_NOERROR)
	{
		timerResolution = ptc.wPeriodMin;

		if(timerResolution > 1)
		{
			Com_Printf("Warning: Minimum supported timer resolution is %ums "
					   "on this system, recommended resolution 1ms\n", timerResolution);
		}

		timeBeginPeriod(timerResolution);
	}
	else
		timerResolution = 0;
#endif

	memset(&vi, 0, sizeof(vi));
	vi.dwOSVersionInfoSize = sizeof(vi);
	ret = GetVersionEx(&vi);
	if (!ret) {
		Com_Printf("^1couldn't get Windows OS version info, error code: %lu\n", GetLastError());
		return;
	}

	switch (vi.dwPlatformId) {
	case 0:
		osname = "Windows 3.1";
		arch = "x86";
		break;
	case 1:
		switch (vi.dwMinorVersion) {
		case 0:
			if (vi.szCSDVersion[1] == 'C'  ||  vi.szCSDVersion[1] == 'B') {
				osname = "Windows 95 (Release 2)";
			} else {
				osname = "Windows 95";
			}
			break;
		case 10:
			if (vi.szCSDVersion[1] == 'A') {
				osname = "Windows 98 SE";
			} else {
				osname = "Windows 98";
			}
			break;
		case 90:
			osname = "Windows Millennium";
			break;
		default:
			osname = "Windows (unknown non nt)";
			break;
		}
		arch = "x86";
		break;
	case 2: {
		OSVERSIONINFOEX viex;
		SYSTEM_INFO si;
		qboolean ntWorkStation;

		//FIXME GetVersionEx() is deprecated since Windows 8 and will show as Windows 8 even for Windows 10 if there isn't a manifest showing win10 support
		memset(&viex, 0, sizeof(viex));
		memset(&si, 0, sizeof(si));

		viex.dwOSVersionInfoSize = sizeof(viex);
		ret = GetVersionEx((OSVERSIONINFO *)&viex);
		if (!ret) {
			Com_Printf("^1couldn't get extendend Windows OS information, error code: %lu\n", GetLastError());
			osname = "Windows (unknown version after 98)";
			arch = "???";
			break;
		}

		// sanity check
		if (vi.dwMajorVersion != viex.dwMajorVersion  ||  vi.dwMinorVersion != viex.dwMinorVersion  ||  vi.dwPlatformId != viex.dwPlatformId) {
			Com_Printf("^1version info mismatch: major %lu %lu, minor %lu %lu, platform %lu %lu\n", vi.dwMajorVersion, viex.dwMajorVersion, vi.dwMinorVersion, viex.dwMinorVersion, vi.dwPlatformId, viex.dwPlatformId);
			//osname = "Windows (unknown version after 98)";
			//break;
		}

		if (viex.wProductType == VER_NT_WORKSTATION) {
			ntWorkStation = qtrue;
		} else {
			ntWorkStation = qfalse;
		}

		//FIXME home, enteprise, basic, etc...

		// GetNativeSystemInfo() to ignore compatability mode?
		//GetSystemInfo(&si);
		GetNativeSystemInfo(&si);

		switch (si.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_AMD64:
			arch = "x64";
			break;
		case PROCESSOR_ARCHITECTURE_ARM:
			arch = "ARM";
			break;
		case PROCESSOR_ARCHITECTURE_IA64:
			arch = "Intel Itanium-based";
			break;
		case PROCESSOR_ARCHITECTURE_INTEL:
			arch = "x86";
			break;
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			arch = "unknown architecture";
			break;
		default:
			arch = "???";
			break;
		}

		switch (vi.dwMajorVersion) {
		case 6:  // vista +
			switch (vi.dwMinorVersion) {
			case 0:
				if (ntWorkStation) {
					osname = "Windows Vista";
				} else {
					osname = "Windows Server 2008";
				}
				break;
			case 1:
				if (ntWorkStation) {
					osname = "Windows 7";
				} else {
					osname = "Windows Server 2008 R2";
				}
				break;
			case 2:
				if (ntWorkStation) {
					osname = "Windows 8";
				} else {
					osname = "Windows Server 2012";
				}
				break;
			default:
				osname = "Windows (unknown version after xp)";
				break;
			}
			break;
		case 5:  // xp and 2000
			switch (vi.dwMinorVersion) {
			case 0:
				osname = "Windows 2000";
				break;
			case 1:
				osname = "Windows XP";
				break;
			case 2:
				if (GetSystemMetrics(SM_SERVERR2)) {
					osname = "Windows Server 2003 R2";
				} else {
					osname = "Windows Server 2003";
				}
				break;
			default:
				osname = "Windows (unknown version before vista)";
				break;
			}
			break;
		default:
			osname = "Windows (unknown major version)";
		}
		break;
	}
	default:
		osname = "Windows (unknown)";
		arch = "???";
		break;
	}

	Com_Printf("%s %s version %lu.%lu (%lu) Platform %lu  %s\n", osname, arch, vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber, vi.dwPlatformId, vi.szCSDVersion);
}

/*
==============
Sys_PlatformExit

Windows specific initialisation
==============
*/


void Sys_PlatformExit (void)
{
#ifndef DEDICATED
	if (timerResolution)
		timeEndPeriod(timerResolution);
#endif

	if (GotHandle  &&  HStdout != INVALID_HANDLE_VALUE) {
		SetConsoleTextAttribute(HStdout, ScreenBufferInfo.wAttributes);
		fputs("\r\n", stderr);
	}
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/

void Sys_SetEnv(const char *name, const char *value)
{
	if(value)
		_putenv(va("%s=%s", name, value));
	else
		_putenv(va("%s=", name));
}

void Sys_Backtrace_f (void)
{
	int pid;
	char command[MAX_STRING_CHARS];

	pid = _getpid();
	Com_Printf("Sys_Backtrace_f() pid: %d\n", pid);
	//execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt full", name_buf, pid_buf, NULL);
	//system("./gdb --batch -n -ex thread -ex bt full ");
	//snprintf(command, sizeof(command), "./gdb.exe --batch -n -ex thread -ex bt full wolfcamql.exe %d > bt.txt", pid);
	snprintf(command, sizeof(command), "gdb.exe --batch -n -ex \"thread apply all bt full\" wolfcamql.exe %d > bt.txt", pid);
	system(command);
}

void Sys_OpenQuakeLiveDirectory (void)
{
	PROCESS_INFORMATION pi;
    STARTUPINFO si;
	char *path;

	path = Cvar_VariableString("fs_quakelivedir");
	Com_Printf("%s\n", path);
	if (!*path) {
		path = Sys_QuakeLiveDir();
	}

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	CreateProcessA("c:\\windows\\explorer.exe", va("c:\\windows\\explorer.exe \"%s\"", path), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void Sys_OpenWolfcamDirectory (void)
{
	PROCESS_INFORMATION pi;
    STARTUPINFO si;
	char *path;

	path = Cvar_VariableString("fs_homepath");
	Com_Printf("%s\n", path);
	if (!*path) {
		path = Sys_DefaultHomePath();
	}

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	CreateProcessA("c:\\windows\\explorer.exe", va("c:\\windows\\explorer.exe \"%s\"", path), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int Sys_DirnameCmp (const char *pathName1, const char *pathName2)
{
	return Q_stricmp(pathName1, pathName2);
}


void Sys_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAX_PRINT_MSG ];
	int         length = 0;
	static int  q3ToAnsi[ 8 ] =
	{
		FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,  // COLOR_BLACK, using bright white so it's visible
		FOREGROUND_INTENSITY | FOREGROUND_RED,  // COLOR_RED
		FOREGROUND_INTENSITY | FOREGROUND_GREEN,  // COLOR_GREEN
		FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,  // COLOR_YELLOW
		FOREGROUND_INTENSITY | FOREGROUND_BLUE,  // COLOR_BLUE
		FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,  // COLOR_CYAN
		FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,  // COLOR_MAGENTA
		//FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,  // COLOR_WHITE
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,  // COLOR_WHITE, using default gray foreground color is ms-dos prompt
	};

	if (!GotHandle  ||  HStdout == INVALID_HANDLE_VALUE) {
		fputs(msg, stderr);
		return;
	}

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
				SetConsoleTextAttribute(HStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
				fputs("\r\n", stderr);
				msg++;
			}
			else
			{
				// Print the color code
				SetConsoleTextAttribute(HStdout, q3ToAnsi[ ColorIndex( *( msg + 1 ) ) ]);
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

// popen

#define POPEN_DATA_EMPTY 0
#define POPEN_DATA_AVAILABLE 1
#define POPEN_DONE 2
#define POPEN_DONE_THREAD_ERROR 3

typedef struct {
	HANDLE hChildStdOutR;
	HANDLE hChildStdOutW;
	HANDLE hChildStdInR;
	HANDLE hChildStdInW;

	HANDLE readMutex;
	HANDLE threadHandle;
	unsigned int threadId;
	int threadStatus;
	BOOL threadCancel;
	PROCESS_INFORMATION piProcInfo;
	char szCmdline[4096];

	char buffer[4096];
} popenDataWin_t;

static unsigned int __stdcall Proc_ReadThreadFunc (void *param)
{
	popenDataWin_t *pw;
	BOOL keepRunning = TRUE;
	DWORD r;
	BOOL bResult;
	BOOL bReadResult;
	DWORD dwRead;
	DWORD readLastError;

	pw = param;

	if (pw == NULL) {
		Com_Printf("^1ERROR:  %s pw == NULL\n", __FUNCTION__);
		keepRunning = FALSE;
	}

	while (keepRunning) {
		BOOL waiting = FALSE;

		r = WaitForSingleObject(pw->readMutex, INFINITE);
		if (r) {
			Com_Printf("^1ERROR: %s couldn't get mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
			keepRunning = FALSE;
			pw->threadStatus = POPEN_DONE_THREAD_ERROR;
		}

		if (pw->threadCancel) {
			keepRunning = FALSE;
		} else {
			if (pw->threadStatus == POPEN_DATA_AVAILABLE) {
				// still waiting for main thread to get our data
				waiting = TRUE;
			} else if (pw->threadStatus == POPEN_DATA_EMPTY) {
				// try to get new data

				bResult = ReleaseMutex(pw->readMutex);
				if (bResult == FALSE) {
					Com_Printf("^1ERROR:  %s  start read release mutex error: %ld\n", __FUNCTION__, GetLastError());
					keepRunning = FALSE;
					pw->threadStatus = POPEN_DONE_THREAD_ERROR;
				}

				bReadResult = ReadFile(pw->hChildStdOutR, pw->buffer, sizeof(pw->buffer) - 1, &dwRead, NULL);
				readLastError = GetLastError();

				r = WaitForSingleObject(pw->readMutex, INFINITE);
				if (r) {
					Com_Printf("^1ERROR: %s read done couldn't get mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
					keepRunning = FALSE;
					pw->threadStatus = POPEN_DONE_THREAD_ERROR;
				}

				if (bReadResult == FALSE  ||  dwRead < 0) {
					keepRunning = FALSE;
					pw->threadStatus = POPEN_DONE;

					if (bReadResult == FALSE  &&  readLastError != ERROR_BROKEN_PIPE) {
						Com_Printf("^1ERROR: %s read error:  %ld\n", __FUNCTION__, readLastError);
						pw->threadStatus = POPEN_DONE_THREAD_ERROR;
					}
				} else if (dwRead == 0) {
					// all done
					keepRunning = FALSE;
					pw->threadStatus = POPEN_DONE;
				} else {
					// got data
					//Com_Printf("got data %ld\n", dwRead);
					pw->buffer[dwRead] = '\0';
					pw->threadStatus = POPEN_DATA_AVAILABLE;
				}
			} else {
				Com_Printf("^1ERROR: %s invalid thread status value %d\n", __FUNCTION__, pw->threadStatus);
				keepRunning = FALSE;
			}
		}

		bResult = ReleaseMutex(pw->readMutex);
		if (bResult == FALSE) {
			Com_Printf("^1ERROR:  %s  couldn't release mutex error: %ld\n", __FUNCTION__, GetLastError());
			keepRunning = FALSE;
			pw->threadStatus = POPEN_DONE_THREAD_ERROR;
		}

		if (waiting) {
			Sys_Sleep(100);
		}
	}

	return 0;
}

// need to free() returned data
popenData_t *Sys_PopenAsync (const char *command)
{
	popenData_t *p;
	popenDataWin_t *pw;
	SECURITY_ATTRIBUTES saAttr;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	p = (popenData_t *)malloc(sizeof(popenData_t));
	if (p == NULL) {
		Com_Printf("^1%s couldn't allocate memory\n", __FUNCTION__);
		return NULL;
	}
	memset(p, 0, sizeof(popenData_t));

	pw = (popenDataWin_t *)malloc(sizeof(popenDataWin_t));
	if (pw == NULL) {
		Com_Printf("^1%s couldn't get Windows data %ld\n", __FUNCTION__, GetLastError());
		free(p);
		return NULL;
	}
	memset(pw, 0, sizeof(popenDataWin_t));

	p->data = pw;
	pw->threadStatus = 0;
	pw->threadCancel = FALSE;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&pw->hChildStdOutR, &pw->hChildStdOutW, &saAttr, 0)) {
		Com_Printf("^1%s couldn't create stdout pipes %ld\n", __FUNCTION__, GetLastError());
		free(pw);
		free(p);
		return NULL;
	}

	// make sure parent stdout read handle isn't inherited
	if (!SetHandleInformation(pw->hChildStdOutR, HANDLE_FLAG_INHERIT, 0)) {
		Com_Printf("^1%s couldn't change inherit status of stdout read pipe %ld\n", __FUNCTION__, GetLastError());
		CloseHandle(pw->hChildStdOutR);
		CloseHandle(pw->hChildStdOutW);
		free(pw);
		free(p);
		return NULL;
	}

	if (!CreatePipe(&pw->hChildStdInR, &pw->hChildStdInW, &saAttr, 0)) {
		Com_Printf("^1%s couldn't create stdin pipes %ld\n", __FUNCTION__, GetLastError());
		CloseHandle(pw->hChildStdOutR);
		CloseHandle(pw->hChildStdOutW);
		free(pw);
		free(p);
		return NULL;
	}

	// make sure parent stdin write handle isn't inherited
	if (!SetHandleInformation(pw->hChildStdInW, HANDLE_FLAG_INHERIT, 0)) {
		Com_Printf("^1%s couldn't change inherit status of stdin write pipe %ld\n", __FUNCTION__, GetLastError());
		CloseHandle(pw->hChildStdInR);
		CloseHandle(pw->hChildStdInW);
		CloseHandle(pw->hChildStdOutR);
		CloseHandle(pw->hChildStdOutW);
		free(pw);
		free(p);
		return NULL;
	}



	
	// create child process

	// testing
	//Q_strncpyz(pw->szCmdline, "ping google.com", sizeof(pw->szCmdline));

	Q_strncpyz(pw->szCmdline, command, sizeof(pw->szCmdline));
	memset(&pw->piProcInfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&siStartInfo, 0, sizeof(STARTUPINFO));

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = pw->hChildStdOutW;
	siStartInfo.hStdOutput = pw->hChildStdOutW;
	siStartInfo.hStdInput = pw->hChildStdInR;  //NULL;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	bSuccess = CreateProcess(NULL,
							 pw->szCmdline,
							 NULL,  // process security attributes
							 NULL,  // primary thread security attributes
							 TRUE,  // inherit handles


							 // creation flags

							 //CREATE_NEW_CONSOLE,
							 CREATE_NO_WINDOW,

							 NULL,  // use parent environment
							 NULL,  // use parent current directory
							 &siStartInfo,
							 &pw->piProcInfo);
	if (!bSuccess) {
		Com_Printf("^1%s couldn't create process '%s'  error: %ld\n", __FUNCTION__, command, GetLastError());
		CloseHandle(pw->hChildStdInR);
		CloseHandle(pw->hChildStdInW);
		CloseHandle(pw->hChildStdOutR);
		CloseHandle(pw->hChildStdOutW);
		free(pw);
		free(p);
		return NULL;
	}

	//CloseHandle(pw->piProcInfo.hProcess);
	//CloseHandle(pw->piProcInfo.hThread);
	
	//Sys_Sleep(15000);
	CloseHandle(pw->hChildStdOutW);
	CloseHandle(pw->hChildStdInW);
	
	pw->readMutex = CreateMutex(NULL, FALSE, NULL);
	if (pw->readMutex == NULL) {
		Com_Printf("^1%s couldn't create mutex %ld\n", __FUNCTION__, GetLastError());
		CloseHandle(pw->piProcInfo.hProcess);
		CloseHandle(pw->piProcInfo.hThread);
		CloseHandle(pw->hChildStdInR);
		CloseHandle(pw->hChildStdOutR);
		free(pw);
		free(p);
		return NULL;
	}

	pw->threadHandle = (HANDLE) _beginthreadex(NULL,
											   0,  // stack size
											   Proc_ReadThreadFunc,
											   pw,  // param pointer
											   0,  // initflag
											   &pw->threadId);
	if (pw->threadHandle == NULL) {
		Com_Printf("^1%s couldn't create thread %ld\n", __FUNCTION__, GetLastError());
		CloseHandle(pw->readMutex);
		CloseHandle(pw->piProcInfo.hProcess);
		CloseHandle(pw->piProcInfo.hThread);
		CloseHandle(pw->hChildStdInR);
		CloseHandle(pw->hChildStdOutR);
		free(pw);
		free(p);
		return NULL;
	}

	return p;
}

void Sys_PopenClose (popenData_t *p)
{
	popenDataWin_t *pw;
	DWORD r;
	BOOL bResult;
	DWORD exitCode;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return;
	}

	pw = p->data;

	r = WaitForSingleObject(pw->readMutex, INFINITE);
	if (r) {
		Com_Printf("^1%s failed to get mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
	}

	pw->threadCancel = TRUE;

	r = ReleaseMutex(pw->readMutex);
	if (!r) {
		Com_Printf("^1%s failed to release mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());

	}

	// join thread
	r = WaitForSingleObject(pw->threadHandle, INFINITE);
	if (r) {
		Com_Printf("^1%s couldn't wait for thread (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
	}

	CloseHandle(pw->threadHandle);
	CloseHandle(pw->readMutex);

	// close process
	r = WaitForSingleObject(pw->piProcInfo.hProcess, INFINITE);
	if (r) {
		Com_Printf("^1%s couldn't wait for process to finish (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
	}
	bResult = GetExitCodeProcess(pw->piProcInfo.hProcess, &exitCode);
	if (bResult == FALSE) {
		Com_Printf("^1%s couldn't get process exit code %ld\n", __FUNCTION__, GetLastError());
	}

	Com_Printf("popen done: %ld\n", exitCode);

	CloseHandle(pw->piProcInfo.hProcess);
	CloseHandle(pw->piProcInfo.hThread);
	CloseHandle(pw->hChildStdInR);
	CloseHandle(pw->hChildStdOutR);

	free(p->data);
}

char *Sys_PopenGetLine (popenData_t *p, char *buffer, int size)
{
	popenDataWin_t *pw;
	char *retp = NULL;
	DWORD r;
	BOOL bResult;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return NULL;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return NULL;
	}

	pw = p->data;

	//Com_Printf("%p\n", pw);  // silence gcc

	r = WaitForSingleObject(pw->readMutex, INFINITE);
	if (r) {
		Com_Printf("^1%s couldn't get mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
		pw->threadCancel = TRUE;
		return NULL;
	}

	if (pw->threadStatus == POPEN_DATA_EMPTY  ||  pw->threadStatus == POPEN_DONE) {
		// pass
		retp = NULL;
	} else if (pw->threadStatus == POPEN_DATA_AVAILABLE) {
		char *p;
		char *s;
		int minSize;
		int i;
		int j;

		// return data

		minSize = size;
		if (minSize > sizeof(pw->buffer)) {
			minSize = sizeof(pw->buffer);
		}

		s = buffer;
		p = pw->buffer;

		//Com_Printf("^5 buffer orig: '%s'\n", pw->buffer);

		for (i = 0;  i < minSize - 1;  i++) {
			if (*p == '\0') {
				*s = *p;
				break;
			} else if (*p == '\r') {
				// skip
				s--;
			} else if (*p == '\n') {
				*s = *p;
				break;
			} else {
				*s = *p;
			}

			s++;
			p++;
		}

		//Com_Printf("^3 buffer:  '%s'\n", buffer);

		if (*s == '\0') {
			pw->threadStatus = POPEN_DATA_EMPTY;
			pw->buffer[0] = '\0';
		} else {   // data still available, clear what was sent
			int n;

			s++;
			*s = '\0';

			p = pw->buffer;
			for (j = i + 1, n = 0;  j < sizeof(pw->buffer) - 1;  j++, n++) {
				p[n] = p[j];
				if (p[n] == '\0') {
					break;
				}
			}

			//Com_Printf("^2 buffer new: '%s'\n", pw->buffer);
		}

		retp = buffer;
	} else {
		Com_Printf("^1%s invalid thread status %d\n", __FUNCTION__, pw->threadStatus);
		retp = NULL;
	}

	bResult = ReleaseMutex(pw->readMutex);
	if (bResult == FALSE) {
		Com_Printf("^1%s  couldn't release mutex error: %ld\n", __FUNCTION__, GetLastError());
		pw->threadCancel = TRUE;
		return NULL;
	}

	return retp;
}

qboolean Sys_PopenIsDone (popenData_t *p)
{
	popenDataWin_t *pw;
	DWORD r;
	BOOL bResult;
	int status;

	if (p == NULL) {
		Com_Printf("^1%s p == NULL\n", __FUNCTION__);
		return qtrue;
	}

	if (p->data == NULL) {
		Com_Printf("^1%s p->data == NULL\n", __FUNCTION__);
		return qtrue;
	}

	pw = p->data;

	r = WaitForSingleObject(pw->readMutex, INFINITE);
	if (r) {
		Com_Printf("^1%s couldn't get mutex (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
		pw->threadCancel = TRUE;
		return qtrue;
	}

	status = pw->threadStatus;

	bResult = ReleaseMutex(pw->readMutex);
	if (bResult == FALSE) {
		Com_Printf("^1%s  couldn't release mutex error: %ld\n", __FUNCTION__, GetLastError());
		pw->threadCancel = TRUE;
		return qtrue;
	}

	if (status != POPEN_DATA_EMPTY  &&  status != POPEN_DATA_AVAILABLE) {
		//Com_Printf("^5 status %d\n", status);
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
		return "steamcmd.exe";
	}
}

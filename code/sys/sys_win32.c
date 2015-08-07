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
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
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

/*
==============
Sys_PlatformInit

Windows specific initialisation
==============
*/
void Sys_PlatformInit (qboolean useBacktrace, qboolean useConsoleOutput)
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
	if (timerResolution)
		timeEndPeriod(timerResolution);

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

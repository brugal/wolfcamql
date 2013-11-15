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
#include <execinfo.h>
//#define _GNU_SOURCE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

/*
==================
Sys_DefaultHomePath
==================
*/
char *Sys_DefaultHomePath(void)
{
	char *p;

	if( !*homePath )
	{
		if( ( p = getenv( "HOME" ) ) != NULL )
		{
			Q_strncpyz( homePath, p, sizeof( homePath ) );
#ifdef MACOS_X
			Q_strcat( homePath, sizeof( homePath ),
					"/Library/Application Support/Wolfcamql" );
#else
			Q_strcat( homePath, sizeof( homePath ), "/.wolfcamql" );
#endif
		}
	}

	return homePath;
}

char *Sys_QuakeLiveDir (void)
{
	char *p;
	const char *override = NULL;

	if (!*QuakeLivePath) {
		override = Cvar_VariableString("fs_quakelivedir");
		if (override  &&  *override) {
			Q_strncpyz(QuakeLivePath, override, sizeof(QuakeLivePath));
			//FS_ReplaceSeparators(QuakeLivePath);
			return QuakeLivePath;
		}
		if ((p = getenv("HOME")) != NULL) {
#ifdef MACOS_X
			//FIXME not sure
			// /Library/Application\ Support/Quakelive/
			Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/Library/Application Support/Quakelive/", p);
#else
			Com_sprintf(QuakeLivePath, sizeof(QuakeLivePath), "%s/.quakelive/quakelive/", p);
#endif
		}
	}

	return QuakeLivePath;
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

/*
==================
Sys_GetClipboardData
==================
*/
char *Sys_GetClipboardData(void)
{
	return NULL;
}

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
	//int           extLen;
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

	//extLen = strlen( extension );

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
			if ( strlen( d->d_name ) < strlen( extension ) ||
				Q_stricmp(
					d->d_name + strlen( d->d_name ) - strlen( extension ),
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

#ifdef MACOS_X
/*
=================
Sys_StripAppBundle

Discovers if passed dir is suffixed with the directory structure of a Mac OS X
.app bundle. If it is, the .app directory structure is stripped off the end and
the result is returned. If not, dir is returned untouched.
=================
*/
char *Sys_StripAppBundle( char *dir )
{
	static char cwd[MAX_OSPATH];

	Q_strncpyz(cwd, dir, sizeof(cwd));
	if(strcmp(Sys_Basename(cwd), "MacOS"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	if(strcmp(Sys_Basename(cwd), "Contents"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	if(!strstr(Sys_Basename(cwd), ".app"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	return cwd;
}
#endif // MACOS_X


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

#if defined(MACOS_X) && !DEDICATED
	/* This function has to be in a separate file, compiled as Objective-C. */
	extern void Cocoa_MsgBox( const char *text );
	if (!com_dedicated || !com_dedicated->integer)
		Cocoa_MsgBox(error);
#endif

	/* make sure the write path for the crashlog exists... */
	if(!Sys_Mkdir(homepath))
	{
		Com_Printf( "ERROR: couldn't create path '%s' for crash log.\n", homepath);
		return;
	}

	if(!Sys_Mkdir(dirpath))
	{
		Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", dirpath);
		return;
	}

	/* we might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	   which will come through here, so we don't want to recurse forever by
	   calling FS_FOpenFileWrite()...use the Unix system APIs instead. */
	f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);
	if( f == -1 )
	{
		Com_Printf( "ERROR: couldn't open %s\n", fileName );
		return;
	}

	/* We're crashing, so we don't care much if write() or close() fails. */
	while( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 ) {
		if (write( f, buffer, size ) != size) {
			Com_Printf( "ERROR: couldn't fully write to %s\n", fileName );
			break;
		}
	}

	close(f);
}

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

/*
==============
Sys_PlatformInit

Unix specific initialisation
==============
*/

#if defined(MACOS_X)  ||  DEDICATED

static int print_gdb_trace (void)
{
	return 0;
}

static void signal_crash (int signum, siginfo_t *info, void *ptr)
{
	fprintf(stderr, "crash with signal %d\n", signum);
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
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

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

    if (!child_pid) {
        dup2(2, 1);   // redirect output to stderr
        fprintf(stdout, "stack trace for %s pid=%s\n", name_buf, pid_buf);
        execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt full", name_buf, pid_buf, NULL);
        //abort(); /* If gdb failed to start */
		//exit(1);
    } else {
        waitpid(child_pid, NULL, 0);
		//exit(1);
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

	if (print_gdb_trace() != -1) {
		//return;
		exit(1);
	}

	sigsegv_outp("%s\n", strsignal(signum));
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
	sz = backtrace(bt, 20);
	strings = backtrace_symbols(bt, sz);
	for(i = 0; i < sz; ++i)
		sigsegv_outp("%s", strings[i]);
#endif
	sigsegv_outp("End of stack trace.");
#else
	sigsegv_outp("Not printing stack strace.");
#endif
	_exit (-1);
}

#endif

void Sys_PlatformInit (qboolean useBacktrace)
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
		if(sigaction(SIGIOT, &action, NULL) < 0) {
			Com_Printf("^1ERROR setting SIGIOT handler\n");
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
		signal(SIGIOT, Sys_SigHandler);
		signal(SIGBUS, Sys_SigHandler);
	}

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

void Sys_PlatformExit (void)
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

#ifdef MACOS_X
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

#ifdef MACOS_X
	system(va("open \"%s\"&", path));
#else
	system(va("xdg-open \"%s\"&", path));
#endif
}

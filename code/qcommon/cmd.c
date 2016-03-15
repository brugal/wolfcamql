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
// cmd.c -- Quake script command processing module

#include "q_shared.h"
#include "qcommon.h"

#define	MAX_CMD_BUFFER	(16384 * 10)
#define	MAX_CMD_LINE	(1024 * 10)

typedef struct {
	byte	*data;
	int		maxsize;
	int		cursize;
} cmd_t;

int			cmd_wait;
cmd_t		cmd_text;
byte		cmd_text_buf[MAX_CMD_BUFFER];


#define MAX_ALIASES 256
static alias_t cmd_aliases[MAX_ALIASES];
static int numAliases = 0;


//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void Cmd_Wait_f( void ) {
	if ( Cmd_Argc() == 2 ) {
		cmd_wait = atoi( Cmd_Argv( 1 ) );
		if ( cmd_wait < 0 )
			cmd_wait = 1; // ignore the argument
	} else {
		cmd_wait = 1;
	}
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void Cbuf_AddText( const char *text ) {
	int		l;
	
	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText( const char *text ) {
	int		len;
	int		i;

	len = strlen( text ) + 1;
	if ( len + cmd_text.cursize > cmd_text.maxsize ) {
		Com_Printf( "Cbuf_InsertText overflowed\n" );
		return;
	}

	// move the existing command text
	for ( i = cmd_text.cursize - 1 ; i >= 0 ; i-- ) {
		cmd_text.data[ i + len ] = cmd_text.data[ i ];
	}

	// copy the new text in
	Com_Memcpy( cmd_text.data, text, len - 1 );

	// add a \n
	cmd_text.data[ len - 1 ] = '\n';

	cmd_text.cursize += len;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, const char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		if (text && strlen(text) > 0) {
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", text);
			//Com_Printf("^1execing now '%s'\n", text);
			Cmd_ExecuteString (text);
		} else {
			Cbuf_Execute();
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", cmd_text.data);
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[MAX_CMD_LINE];
	int		quotes;
	qboolean cComment;
	qboolean slashComment;

	while (cmd_text.cursize)
	{
		if ( cmd_wait > 0 ) {
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)cmd_text.data;

		cComment = qfalse;
		slashComment = qfalse;
		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (cComment) {
				if (text[i - 1] == '*'  &&  text[i] == '/') {
					break;
				} else {
					continue;
				}
			}

			if (slashComment) {
				if (text[i] == '\0'  ||  text[i] == '\n'  ||  text[i] == '\r') {
					break;
				} else {
					continue;
				}
			}

			if (i > 0) {
				if (text[i - 1] == '/'  &&  text[i] == '*') {
					cComment = qtrue;
					continue;
				}
				if (text[i - 1] == '/'  &&  text[i] == '/') {
					slashComment = qtrue;
					continue;
				}
			}

			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';') {
				//Com_Printf("breaking...\n");
				break;	// don't break if inside a quoted string
			}
			if (text[i] == '\n' || text[i] == '\r' )
				break;
		}

		if( i >= (MAX_CMD_LINE - 1)) {
			i = MAX_CMD_LINE - 1;
		}
				
		Com_Memcpy (line, text, i);
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line

		Cmd_ExecuteString (line);
	}
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
//static qboolean ExecNowDamnIt = qfalse;

void Cmd_Exec_f( void ) {
	qboolean quiet;
	union {
		char	*c;
		void	*v;
	} f;
	//int		len;
	char	filename[MAX_QPATH];

	quiet = !Q_stricmp(Cmd_Argv(0), "execq");

	if (Cmd_Argc () != 2) {
		Com_Printf ("exec%s <filename> : execute a script file%s\n", quiet ? "q" : "", quiet ? " without notification" : "");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	//len = FS_ReadFile( filename, &f.v);
	FS_ReadFile( filename, &f.v);
	if (!f.c) {
		if ((!com_cl_running  ||  !com_sv_running)  ||  (com_execVerbose  &&  com_execVerbose->integer)) {
			if (!quiet) {
				Com_Printf("couldn't exec %s\n", filename);
			}
		}
		return;
	}

	if ((!com_cl_running  ||  !com_sv_running)  ||  (com_execVerbose  &&  com_execVerbose->integer)) {
		if (!quiet) {
			Com_Printf ("execing %s\n", filename);
		}
	}

	if (0) {  //(ExecNowDamnIt) {
		Cbuf_ExecuteText(EXEC_NOW, f.c);
	} else {
		Cbuf_InsertText (f.c);
	}

	FS_FreeFile (f.v);
}

#if 0
void Cmd_ExecNow_f (void)
{
	ExecNowDamnIt = qtrue;
	Cmd_Exec_f();
	ExecNowDamnIt = qfalse;
}
#endif


/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
void Cmd_Vstr_f( void ) {
	char	*v;

	if (Cmd_Argc () != 2) {
		Com_Printf ("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = Cvar_VariableString( Cmd_Argv( 1 ) );
	Cbuf_InsertText( va("%s\n", v ) );
}

void Cmd_Ifeq_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifeq <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval == v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifneq_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifneq <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval != v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifgt_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifgt <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval > v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifgte_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifgte <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval >= v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Iflt_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  iflt <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval < v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Iflte_f (void)
{
	int cval;
	int v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  iflte <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableIntegerValue(Cmd_Argv(1));
	v = atoi(Cmd_Argv(2));
	if (cval <= v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifeqf_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifeqf <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval == v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifneqf_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifneqf <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval != v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifgtf_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifgtf <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval > v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifgtef_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifgtef <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval >= v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifltf_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifltf <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval < v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

void Cmd_Ifltef_f (void)
{
	float cval;
	float v;
	const char *s;

	if (Cmd_Argc() < 4) {
		Com_Printf("usage:  ifltef <cvar> <value> <vstr to execute if true> [optional vstr to execute if false]\n");
		return;
	}

	cval = Cvar_VariableValue(Cmd_Argv(1));
	v = atof(Cmd_Argv(2));
	if (cval <= v) {
		s = Cvar_VariableString(Cmd_Argv(3));
		Cbuf_InsertText(va("%s\n", s));
	} else {
		if (Cmd_Argc() >= 5) {
			s = Cvar_VariableString(Cmd_Argv(4));
			Cbuf_InsertText(va("%s\n", s));
		}
	}
}

static void Cmd_AliasExec_f (void)
{
	int i;
	const char *arg;

	arg = Cmd_Argv(0);

	for (i = 0;  i < numAliases;  i++) {
		if (!Q_stricmp(arg, cmd_aliases[i].name)) {
			Cbuf_InsertText(va("%s\n", cmd_aliases[i].command));
			break;
		}
	}
}

static void Cmd_Alias_f (void)
{
	int i;

	if (Cmd_Argc() == 1) {
		Com_Printf("Usage: alias \"command\"\n");
		Com_Printf("Alias List:\n");

		for (i = 0;  i < numAliases;  i++) {
			Com_Printf("  %s \"%s\"\n", cmd_aliases[i].name, cmd_aliases[i].command);
		}

		return;
	}

	if (Cmd_Argc() == 2) {
		return;
	}

	//FIXME hash and CopyString()

	for (i = 0;  i < numAliases;  i++) {
		if (!Q_stricmp(Cmd_Argv(1), cmd_aliases[i].name)) {
			Q_strncpyz(cmd_aliases[i].command, Cmd_Argv(2), sizeof(cmd_aliases[i].command));
			return;
		}
	}

	if (numAliases >= MAX_ALIASES) {
		Com_Printf("^1maximum number of aliases %d\n", MAX_ALIASES);
		return;
	}

	Q_strncpyz(cmd_aliases[numAliases].name, Cmd_Argv(1), sizeof(cmd_aliases[numAliases].name));
	Q_strncpyz(cmd_aliases[numAliases].command, Cmd_Argv(2), sizeof(cmd_aliases[numAliases].command));
	numAliases++;

	Cmd_AddCommand(Cmd_Argv(1), Cmd_AliasExec_f);
}

static void Cmd_Unalias_f (void)
{
	int i, j;
	alias_t *a;

	if (Cmd_Argc() == 1) {
		Com_Printf("Usage: unalias \"command\"\n");
		Com_Printf("Alias List:\n");

		for (i = 0;  i < numAliases;  i++) {
			Com_Printf("  %s \"%s\"\n", cmd_aliases[i].name, cmd_aliases[i].command);
		}

		return;
	}

	for (i = 0;  i < numAliases;  i++) {
		a = &cmd_aliases[i];

		if (!Q_stricmp(Cmd_Argv(1), a->name)) {
			Cmd_RemoveCommand(a->name);
			for (j = i + 1;  j < numAliases;  j++) {
				memcpy(&cmd_aliases[j - 1], &cmd_aliases[j], sizeof(cmd_aliases[j - 1]));
			}

			numAliases--;
			return;
		}
	}
}

static void Cmd_Unaliasall_f (void)
{
	int i;

	for (i = 0;  i < numAliases;  i++) {
		Cmd_RemoveCommand(cmd_aliases[i].name);
	}

	numAliases = 0;
}

static void Cmd_CompleteAliasName (char *args, int argNum)
{
	const char *name;
	int i;

	name = args + 7;

	//FIXME only one match doesn't print in quake live
	if (argNum == 2) {
		for (i = 0;  i < numAliases;  i++) {
			if (!Q_stricmpn(name, cmd_aliases[i].name, strlen(name))) {
				Com_Printf("     %s\n", cmd_aliases[i].name);
			}
		}
	}
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	Com_Printf ("%s\n", Cmd_Args());
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	char					*name;
	xcommand_t				function;
	completionFunc_t	complete;
} cmd_function_t;


static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
static	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted
static	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)

static	cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc( void ) {
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= cmd_argc ) {
		return "";
	}
	return cmd_argv[arg];
}

/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_Argv( arg ), bufferLength );
}


/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char	*Cmd_Args( void ) {
	static	char		cmd_args[MAX_STRING_CHARS];
	int		i;

	cmd_args[0] = 0;
	for ( i = 1 ; i < cmd_argc ; i++ ) {
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char *Cmd_ArgsFrom( int arg ) {
	static	char		cmd_args[BIG_INFO_STRING];
	int		i;

	cmd_args[0] = 0;
	if (arg < 0)
		arg = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgsBuffer( char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_Args(), bufferLength );
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
============
*/
char *Cmd_Cmd(void)
{
	return cmd_cmd;
}

/*
   Replace command separators with space to prevent interpretation
   This is a hack to protect buggy qvms
   https://bugzilla.icculus.org/show_bug.cgi?id=3593
   https://bugzilla.icculus.org/show_bug.cgi?id=4769
*/

void Cmd_Args_Sanitize( void )
{
	int i;

	for ( i = 1 ; i < cmd_argc ; i++ ) {
		char* c = cmd_argv[i];

		if(strlen(c) > MAX_CVAR_VALUE_STRING - 1)
			c[MAX_CVAR_VALUE_STRING - 1] = '\0';

		while ((c = strpbrk(c, "\n\r;"))) {
			*c = ' ';
			++c;
		}
	}
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
static void Cmd_TokenizeString2( const char *text_in, qboolean ignoreQuotes ) {
	const unsigned char *text;
	unsigned char *textOut;

#ifdef TKN_DBG
  // FIXME TTimo blunt hook to try to find the tokenization of userinfo
  Com_DPrintf("Cmd_TokenizeString: %s\n", text_in);
#endif

  //Com_Printf("string:  '%s'\n", text_in);

	// clear previous args
	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}
	
	Q_strncpyz( cmd_cmd, text_in, sizeof(cmd_cmd) );

	text = (unsigned char *)text_in;
	textOut = (unsigned char *)cmd_tokenized;

	while ( 1 ) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return;			// all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				while (*text) {
					if (text[0] == '\n'  ||  text[0] == '\r') {
						break;
					}
					text++;
				}
				if (!*text) {
					return;
				}
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return;		// all tokens parsed
				}
				text += 2;
			} else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings
    // NOTE TTimo this doesn't handle \" escaping
		if ( !ignoreQuotes && *text == '"' ) {
			cmd_argv[cmd_argc] = (char *)textOut;
			cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = (char *)textOut;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *text > ' ' ) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return;		// all tokens parsed
		}
	}
	
}

/*
============
Cmd_TokenizeString
============
*/
void Cmd_TokenizeString( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qfalse );
}

/*
============
Cmd_TokenizeStringIgnoreQuotes
============
*/
void Cmd_TokenizeStringIgnoreQuotes( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qtrue );
}

/*
============
Cmd_FindCommand
============
*/
cmd_function_t *Cmd_FindCommand( const char *cmd_name )
{
	cmd_function_t *cmd;
	for( cmd = cmd_functions; cmd; cmd = cmd->next )
		if( !Q_stricmp( cmd_name, cmd->name ) )
			return cmd;
	return NULL;
}

/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function ) {
	cmd_function_t	*cmd;
	
	// fail if the command already exists
	if( Cmd_FindCommand( cmd_name ) )
	{
		// allow completion-only commands to be silently doubled
		if ( function != NULL ) {
			Com_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
		}
		return;
	}

	// use a small malloc to avoid zone fragmentation
	cmd = S_Malloc (sizeof(cmd_function_t));
	cmd->name = CopyString( cmd_name );
	cmd->function = function;
	cmd->complete = NULL;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

/*
============
Cmd_SetCommandCompletionFunc
============
*/
void Cmd_SetCommandCompletionFunc( const char *command, completionFunc_t complete ) {
	cmd_function_t	*cmd;

	for( cmd = cmd_functions; cmd; cmd = cmd->next ) {
		if( !Q_stricmp( command, cmd->name ) ) {
			cmd->complete = complete;
			return;
		}
	}
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand( const char *cmd_name ) {
	cmd_function_t	*cmd, **back;

	back = &cmd_functions;
	while( 1 ) {
		cmd = *back;
		if ( !cmd ) {
			// command wasn't active
			return;
		}
		if ( !strcmp( cmd_name, cmd->name ) ) {
			*back = cmd->next;
			if (cmd->name) {
				Z_Free(cmd->name);
			}
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}

/*
============
Cmd_RemoveCommandSafe

Only remove commands with no associated function
============
*/
void Cmd_RemoveCommandSafe( const char *cmd_name )
{
	cmd_function_t *cmd = Cmd_FindCommand( cmd_name );

	if( !cmd )
		return;
	if( cmd->function )
	{
		Com_Error( ERR_DROP, "Restricted source tried to remove "
				   "system command \"%s\"", cmd_name );
		return;
	}

	Cmd_RemoveCommand( cmd_name );
}

/*
============
Cmd_CommandCompletion
============
*/
void	Cmd_CommandCompletion( void(*callback)(const char *s) ) {
	cmd_function_t	*cmd;
	
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		callback( cmd->name );
	}
}

/*
============
Cmd_CompleteArgument
============
*/
void Cmd_CompleteArgument( const char *command, char *args, int argNum ) {
	cmd_function_t	*cmd;

	for( cmd = cmd_functions; cmd; cmd = cmd->next ) {
		if( !Q_stricmp( command, cmd->name ) ) {
			if ( cmd->complete ) {
				cmd->complete( args, argNum );
			}
		}
	}
}


/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void	Cmd_ExecuteString( const char *text ) {	
	cmd_function_t	*cmd, **prev;

	// execute the command line
	Cmd_TokenizeString( text );		
	if ( !Cmd_Argc() ) {
		return;		// no tokens
	}

	// check registered command functions	
	for ( prev = &cmd_functions ; *prev ; prev = &cmd->next ) {
		cmd = *prev;
		if ( !Q_stricmp( cmd_argv[0],cmd->name ) ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = cmd_functions;
			cmd_functions = cmd;

			// perform the action
			if ( !cmd->function ) {
				// let the cgame or game handle it
				break;
			} else {
				cmd->function ();
			}
			//Com_Printf("^1did function '%s'\n", cmd->name);
			return;
		}
	}
	
	// check cvars
	if ( Cvar_Command() ) {
		return;
	}

	//Com_Printf("^1yes\n");
	// check client game commands
	if ( com_cl_running && com_cl_running->integer && CL_GameCommand() ) {
		return;
	}

	// check server game commands
	if ( com_sv_running && com_sv_running->integer && SV_GameCommand() ) {
		return;
	}

	// check ui commands
	if ( com_cl_running && com_cl_running->integer && UI_GameCommand() ) {
		return;
	}

	// send it as a server command if we are connected
	// this will usually result in a chat message
	CL_ForwardCommandToServer ( text );
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int				i;
	char			*match;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	} else {
		match = NULL;
	}

	i = 0;
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		if (match && !Com_Filter(match, cmd->name, qfalse)) continue;

		Com_Printf ("%s\n", cmd->name);
		i++;
	}
	Com_Printf ("%i commands\n", i);
}

/*
==================
Cmd_CompleteCfgName
==================
*/
void Cmd_CompleteCfgName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "", "cfg", qfalse, qtrue, NULL );
	}
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void) {
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_SetCommandCompletionFunc( "exec", Cmd_CompleteCfgName );
	Cmd_AddCommand ("execq",Cmd_Exec_f);
	Cmd_SetCommandCompletionFunc( "execq", Cmd_CompleteCfgName );
	Cmd_AddCommand ("vstr",Cmd_Vstr_f);
	Cmd_SetCommandCompletionFunc( "vstr", Cvar_CompleteCvarName );
	Cmd_AddCommand("alias", Cmd_Alias_f);
	Cmd_SetCommandCompletionFunc("alias", Cmd_CompleteAliasName);
	Cmd_AddCommand("unalias", Cmd_Unalias_f);
	Cmd_AddCommand("unaliasall", Cmd_Unaliasall_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
	Cmd_AddCommand("ifeq", Cmd_Ifeq_f);
	Cmd_AddCommand("ifneq", Cmd_Ifneq_f);
	Cmd_AddCommand("ifgt", Cmd_Ifgt_f);
	Cmd_AddCommand("ifgte", Cmd_Ifgte_f);
	Cmd_AddCommand("iflt", Cmd_Iflt_f);
	Cmd_AddCommand("iflte", Cmd_Iflte_f);

	Cmd_AddCommand("ifeqf", Cmd_Ifeqf_f);
	Cmd_AddCommand("ifneqf", Cmd_Ifneqf_f);
	Cmd_AddCommand("ifgtf", Cmd_Ifgtf_f);
	Cmd_AddCommand("ifgtef", Cmd_Ifgtef_f);
	Cmd_AddCommand("ifltf", Cmd_Ifltf_f);
	Cmd_AddCommand("ifltef", Cmd_Ifltef_f);
}


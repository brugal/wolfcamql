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
// console.c

#include "client.h"
#include "cl_console.h"
#include "keys.h"

int g_console_field_width = 78;

typedef struct {
	char color;
	int codePoint;
	char utf8Bytes[4];
	int numUtf8Bytes;
} consoleChar_t;


#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE	(32768 * 10)
typedef struct {
	qboolean	initialized;

	consoleChar_t text[CON_TEXTSIZE];

	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at con_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines
	int firstLine;

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

static console_t	con;

static cvar_t		*con_conspeed;
static cvar_t		*con_notifytime;
static cvar_t		*con_transparency;
static cvar_t *con_fracSize;
static cvar_t *con_rgb;
static cvar_t *con_scale;
static cvar_t *con_lineWidth;

#define	DEFAULT_CONSOLE_WIDTH	78
#define MIN_CON_SCALE 0.001f

static void Con_CalcFirstLine (void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// Can't toggle the console when it's the only thing available
	if ( cls.state == CA_DISCONNECTED && Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_CONSOLE );
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i].color = (ColorIndex(COLOR_WHITE));
		con.text[i].codePoint = ' ';
		con.text[i].utf8Bytes[0] = ' ';
		con.text[i].utf8Bytes[1] = '\0';
		con.text[i].utf8Bytes[2] = '\0';
		con.text[i].utf8Bytes[3] = '\0';
		con.text[i].numUtf8Bytes = 1;
	}

	Con_CalcFirstLine();
	Con_Bottom();		// go to end
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x, i;
	const consoleChar_t *line;
	fileHandle_t	f;
	int		bufferlen;
	char	*buffer;
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if (line[x].codePoint != ' ') {
				break;
			}
		if (x != con.linewidth)
			break;
	}

#ifdef _WIN32
	bufferlen = (con.linewidth * 4  + 3) * sizeof ( char );
#else
	bufferlen = (con.linewidth * 4  + 2) * sizeof ( char );
#endif

	buffer = Hunk_AllocateTempMemory( bufferlen );
	if (!buffer) {
		Com_Printf("^3%s() couldn't allocate memory for line buffer\n", __FUNCTION__);
		FS_FCloseFile(f);
		return;
	}

	// write the remaining lines
	buffer[bufferlen-1] = 0;
	for ( ; l <= con.current ; l++)
	{
		int bi;  // buffer index

		line = con.text + (l%con.totallines)*con.linewidth;
		for (i = 0, bi = 0;  i < con.linewidth;  i++) {
			int n;
			for (n = 0;  n < line[i].numUtf8Bytes;  n++) {
				buffer[bi] = line[i].utf8Bytes[n];
				bi++;
			}
		}
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory( buffer );
	FS_FCloseFile( f );

	Com_Printf ("Dumped console text to %s.\n", filename );
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;

	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}


static void Con_CalcFirstLine (void)
{
	int i;
	int x;
	const consoleChar_t *line;

	for (i = con.current - con.totallines + 1;  i <= con.current;  i++) {
		line = con.text + (i % con.totallines) * con.linewidth;
		for (x = 0;  x < con.linewidth;  x++) {
			if (line[x].codePoint != ' ') {
				break;
			}
		}
		if (x != con.linewidth) {
			break;
		}
	}

	con.firstLine = i - 1;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
static consoleChar_t tbuf[CON_TEXTSIZE];

static void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;

	if (con_lineWidth) {
		if (*con_lineWidth->string) {
			width = con_lineWidth->integer;  //(SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
		} else {
			width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;
		}

		if (width <= 0) {
			width = 1;
		}
	} else {
		width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	}

	if (width == con.linewidth)
		return;

	//printf("resizing console...\n");

	//FIXME this can't happen
	if (width < 1)			// video hasn't been initialized yet
	{
		//width = DEFAULT_CONSOLE_WIDTH;
		width = con_lineWidth->integer;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++) {
			con.text[i].color = ColorIndex(COLOR_WHITE);
			con.text[i].codePoint = ' ';
			con.text[i].utf8Bytes[0] = ' ';
			con.text[i].utf8Bytes[1] = '\0';
			con.text[i].utf8Bytes[2] = '\0';
			con.text[i].utf8Bytes[3] = '\0';
			con.text[i].numUtf8Bytes = 1;
		}
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;

		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(consoleChar_t));
		for(i=0; i<CON_TEXTSIZE; i++) {
			con.text[i].color = ColorIndex(COLOR_WHITE);
			con.text[i].codePoint = ' ';
			con.text[i].utf8Bytes[0] = ' ';
			con.text[i].utf8Bytes[1] = '\0';
			con.text[i].utf8Bytes[2] = '\0';
			con.text[i].utf8Bytes[3] = '\0';
			con.text[i].numUtf8Bytes = 1;
		}


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				//FIXME widen lines if width is greater
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;

	Con_CalcFirstLine();
}

/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "", "txt", qfalse, qtrue, NULL );
	}
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", CVAR_ARCHIVE);
	con_conspeed = Cvar_Get ("con_conspeed", "3", CVAR_ARCHIVE);
	con_transparency = Cvar_Get ("con_transparency", "0.04", CVAR_ARCHIVE);
	con_fracSize = Cvar_Get ("con_fracSize", "0", CVAR_ARCHIVE);
	con_rgb = Cvar_Get("con_rgb", "", CVAR_ARCHIVE);
	con_scale = Cvar_Get("con_scale", "1.0", CVAR_ARCHIVE);
	con_lineWidth = Cvar_Get("con_lineWidth", "", CVAR_ARCHIVE);

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);
	Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );
}


/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed (qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
    if (skipnotify)
		  con.times[con.current % NUM_CON_TIMES] = 0;
    else
		  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for (i = 0;  i < con.linewidth;  i++) {
		consoleChar_t *c;

		c = &con.text[(con.current%con.totallines)*con.linewidth+i];
		c->color = ColorIndex(COLOR_WHITE);
		c->codePoint = ' ';
		c->utf8Bytes[0] = ' ';
		c->utf8Bytes[1] = '\0';
		c->utf8Bytes[2] = '\0';
		c->utf8Bytes[3] = '\0';
		c->numUtf8Bytes = 1;
	}
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint (const char *txt)
{
	int             y, l;
	unsigned char   c;
	unsigned short  color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF
	const char *sp;

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	//if ( !force  &&  cl_noprint && cl_noprint->integer > 1) {
	//	return;
	//}

	//return;
	if (!con.initialized) {
		con.color[0] =
		con.color[1] =
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *((unsigned char *) txt)) != 0 ) {
		int codePoint;
		int numUtf8Bytes;
		qboolean error;
		char bytes[4];
		int bc;

		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}
#if 1
		codePoint = Q_GetCpFromUtf8((const char *)txt, &numUtf8Bytes, &error);
		bytes[0] = bytes[1] = bytes[2] = bytes[3] = '\0';
		for (bc = 0;  bc < numUtf8Bytes;  bc++) {
			bytes[bc] = txt[bc];
		}
#endif
		//codePoint = txt[0];
		//bytes[0] = txt[0];
		//numUtf8Bytes = 1;
		
		// count word length
		l = 0;
#if 1
		sp = txt;
		for (l=0 ; l< con.linewidth ; l++) {
			int cp;
			int nbytes;
			qboolean err;

			// don't read past buffer
			if (sp[0] == '\0') {
				break;
			}

			nbytes = 0;
			err = qfalse;
			cp = Q_GetCpFromUtf8((const char *)sp, &nbytes, &err);
			sp += nbytes;

#if 0
			if (err) {
				Com_Printf("error with string '%s'\n", txt);
				//Com_Printf("l: %d  sp: '%s'\n", l, sp - numUtf8Bytes);
				exit(1);
			}
#endif
			//if ( txt[l] <= ' ') {
			if (cp == ' ') {
				break;
			}

		}
#endif
		
		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(skipnotify);

		}

		//txt++;
		txt += numUtf8Bytes;

		switch (codePoint)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default: {	// display character and advance
			consoleChar_t *cc;
			y = con.current % con.totallines;
			//con.text[y*con.linewidth+con.x] = (color << 8) | c;
			cc = &con.text[y*con.linewidth+con.x];
			cc->color = color;
			cc->codePoint = codePoint;
			cc->utf8Bytes[0] = bytes[0];
			cc->utf8Bytes[1] = bytes[1];
			cc->utf8Bytes[2] = bytes[2];
			cc->utf8Bytes[3] = bytes[3];
			cc->numUtf8Bytes = numUtf8Bytes;

			// testing
			//cc->codePoint = 'x';
			//cc->utf8Bytes[0] = 'x';
			//cc->numUtf8Bytes = 1;
			// numUtf8bytes = 1

			// testing
#if 0
			if (numUtf8Bytes > 1) {
				char tmpBuffer[5];

				tmpBuffer[0] = cc->utf8Bytes[0];
				tmpBuffer[1] = cc->utf8Bytes[1];
				tmpBuffer[2] = cc->utf8Bytes[2];
				tmpBuffer[3] = cc->utf8Bytes[3];
				tmpBuffer[4] = '\0';
				printf("got codepoint %d 0x%x '%s'\n", codePoint, codePoint, tmpBuffer);
			}
#endif
			con.x++;
			if (con.x >= con.linewidth) {
				Con_Linefeed(skipnotify);
				con.x = 0;
			}
			break;
		}

		}
	}


	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
static void Con_DrawInput (void) {
	float y;
	float cwidth;
	float cheight;
	float conScale;

	if ( cls.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}

	cheight = SMALLCHAR_HEIGHT;
	cwidth = SMALLCHAR_WIDTH;

	conScale = con_scale->value;
	if (conScale < MIN_CON_SCALE) {
		conScale = MIN_CON_SCALE;
	}
	cheight *= conScale;
	cwidth *= conScale;

	y = con.vislines - ( cheight * 2.0 );

	if (g_consoleField.widthInChars <= g_consoleField.cursor) {
		//FIXME what if con.color is red
		re.SetColor(g_color_table[ColorIndex(COLOR_RED)]);
	} else {
		re.SetColor(con.color);
	}

	SCR_DrawSmallCharExt( (float)con.xadjust + 1.0 * cwidth, y, cwidth, cheight, ']' );

	re.SetColor(con.color);

	Field_VariableSizeDraw( &g_consoleField, con.xadjust + 2 * cwidth, y,
							cwidth - 3 * cwidth, SMALLCHAR_WIDTH, cwidth, cheight, qtrue, qtrue );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	consoleChar_t *text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;

	if (cl_noprint  &&  cl_noprint->integer > 0) {
		return;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		//if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI) ) {
			continue;
		}

		for (x = 0 ; x < con.linewidth ; x++) {
			if (text[x].codePoint == ' ') {
				continue;
			}
			if (ColorIndexForNumber(text[x].color) != currentColor) {
				currentColor = ColorIndexForNumber(text[x].color);
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH, v + SMALLCHAR_HEIGHT, text[x].codePoint);
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor( NULL );

	//if (Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
	if (Key_GetCatcher( ) & (KEYCATCH_UI) ) {
		return;
	}

	// draw the chat line
	if ( Key_GetCatcher( ) & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			SCR_DrawBigString (8, v, "say_team:", 1.0f, qfalse );
			skip = 10;
		}
		else
		{
			SCR_DrawBigString (8, v, "say:", 1.0f, qfalse );
			skip = 5;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
			SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue, qtrue );
	}
}

void Con_DrawConsoleLinesOver (int xpos, int ypos, int numLines)
{
	int		x;
	float xposf;
	float v;
	consoleChar_t *text;
	int		i;
	int		currentColor;

	currentColor = 7;
	re.SetColor(g_color_table[currentColor]);

	//FIXME duplicate code with Con_DrawNotify()

	xposf = xpos;
	v = ypos;
	SCR_AdjustFrom640(&xposf, &v, NULL, NULL);

	for (i = con.current - numLines; i <= con.current;  i++)
	{
		if (i < 0)
			continue;
		text = con.text + ((i % con.totallines) * con.linewidth);

		for (x = 0 ; x < con.linewidth ; x++) {
			if (text[x].codePoint == ' ') {
				continue;
			}
			if (ColorIndexForNumber(text[x].color) != currentColor) {
				currentColor = ColorIndexForNumber(text[x].color);
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(xposf + con.xadjust + (x+1)*SMALLCHAR_WIDTH, v + SMALLCHAR_HEIGHT, text[x].codePoint);
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor(NULL);
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
static void Con_DrawSolidConsole( float frac ) {
	int i;
	//float x;
	int x;
	float y;
	int				rows;
	//short			*text;
	consoleChar_t *text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	vec4_t			color;
	float cwidth;
	float cheight;
	float conScale;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT;
	if ( y < 1 ) {
		y = 0;
	} else {
		if (!cls.consoleShader) {
			color[0] = color[1] = color[2] = 0;
			color[3] = 1;
			SCR_FillRect(0, 0, SCREEN_WIDTH, y, color);
		} else if (!*con_rgb->string) {
			Vector4Set(color, 1, 1, 1, 1.0 - con_transparency->value);
			re.SetColor(color);
			SCR_DrawPic(0, 0, SCREEN_WIDTH, y, cls.consoleShader);
			re.SetColor(NULL);
		} else {
			int v;

			v = con_rgb->integer;
			color[0] = (v & 0xff0000) / 0x010000 / 255.0;
			color[1] = (v & 0x00ff00) / 0x000100 / 255.0;
			color[2] = (v & 0x0000ff) / 0x000001 / 255.0;
			color[3] = 1.0 - con_transparency->value;

			SCR_FillRect(0, 0, SCREEN_WIDTH, y, color);
		}
	}

	color[0] = 1;
	color[1] = 0;
	color[2] = 0;
	color[3] = 1;
	SCR_FillRect( 0, y, SCREEN_WIDTH, 2, color );


	// draw the version number

	re.SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );

	i = strlen( Q3_VERSION );

	for (x=0 ; x<i ; x++) {
		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x + 1 ) * SMALLCHAR_WIDTH,
						   lines - SMALLCHAR_HEIGHT, Q3_VERSION[x] );
	}

	re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );

	// draw the text
	cwidth = SMALLCHAR_WIDTH;
	cheight = SMALLCHAR_HEIGHT;

	conScale = con_scale->value;
	if (conScale < MIN_CON_SCALE) {
		conScale = MIN_CON_SCALE;
	}
	cwidth *= conScale;
	cheight *= conScale;

	con.vislines = lines;
	rows = (lines - cwidth) / cwidth;		// rows of text to draw

	y = lines - (cheight * 3);

	// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		if ((con.display - con.firstLine) <= ((con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 5)) {
			re.SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
		} else {

			re.SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		}
		for (x=0 ; x<con.linewidth ; x+=4) {
			SCR_DrawSmallCharExt( con.xadjust + (x+1)*cwidth, y, cwidth, cheight, '^' );
		}
		y -= cheight;
		rows--;
	}

	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= cheight, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		for (x=0 ; x<con.linewidth ; x++) {
			//if ( ( text[x] & 0xff ) == ' ' ) {
			if (text[x].codePoint == ' ') {
				continue;
			}

			//if ( ColorIndexForNumber( text[x]>>8 ) != currentColor ) {
			if (ColorIndexForNumber(text[x].color) != currentColor) {
				//currentColor = ColorIndexForNumber( text[x]>>8 );
				currentColor = ColorIndexForNumber(text[x].color);
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallCharExt(  con.xadjust + (x+1)*cwidth, y, cwidth, cheight, text[x].codePoint );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	float frac;

	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		if (con_fracSize->value > 0.0) {
			frac = con_fracSize->value;
		} else {
			frac = con.displayFrac;
		}
		Con_DrawSolidConsole( frac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			if (cl_noprint  &&  cl_noprint->integer < 2) {
				// pass
			} else {
				Con_DrawNotify ();
			}
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	//float ts;
	static int lastTime = 0;
	int currentTime;

	currentTime = Sys_Milliseconds();

#if 0
	ts = com_timescale->value;
	if (ts <= 0.0) {
		ts = 0.01;
	}
#endif

	// decide on the destination height of the console
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5;		// half screen
	else
		con.finalFrac = 0;				// none visible

	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		//con.displayFrac -= con_conspeed->value*cls.realFrametime*0.001 / ts;
		con.displayFrac -= con_conspeed->value * (currentTime - lastTime) * 0.001;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		//con.displayFrac += con_conspeed->value*cls.realFrametime*0.001 / ts;
		con.displayFrac += con_conspeed->value * (currentTime - lastTime) * 0.001;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

#if 0
	if (cl_freezeDemo->integer  &&  con.finalFrac == 0.0f) {
		//con.displayFrac = 0;
	}
#endif

	lastTime = currentTime;
}


void Con_PageUp (void)
{
	float conScale;

	conScale = con_scale->value;
	if (conScale < MIN_CON_SCALE) {
		conScale = MIN_CON_SCALE;
	}
	if ((con.display - con.firstLine) < ((con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 5)) {
		//printf("returning:  con.display %d, con.firstLine %d, lines %d\n", con.display, con.firstLine, (con.vislines / SMALLCHAR_HEIGHT) - 5);
		//printf("con.current: %d\n", con.current);
		return;
	}

	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}

	// don't scroll past beginning of buffer
	if ((con.display - con.firstLine) < ((con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 5)) {
		//printf("no scroll:  con.display %d, con.firstLine %d, lines %d\n", con.display, con.firstLine, (con.vislines / SMALLCHAR_HEIGHT) - 5);
		con.display = con.firstLine + (con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 5;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top (void)
{
	float conScale;

	conScale = con_scale->value;
	if (conScale < MIN_CON_SCALE) {
		conScale = MIN_CON_SCALE;
	}

	if ((con.display - con.firstLine) < ((con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 3)) {
		return;
	}

	con.display = con.firstLine + (con.vislines / (SMALLCHAR_HEIGHT * conScale)) - 5;
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}

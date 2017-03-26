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
//
// string allocation/managment

#include "ui_shared.h"

//FIXME argh...  hack
static char *ScriptBuffer = NULL;
static qboolean UseScriptBuffer = qfalse;


typedef struct menuVar_s {
	char name[MAX_MENU_VAR_NAME];
	float value;
} menuVar_t;

static menuVar_t menuVars[MAX_MENU_VARS];

//FIXME hack for team game lists
static listBoxDef_t *LastListSelected = NULL;

#define SCROLL_TIME_START					500
#define SCROLL_TIME_ADJUST				150
#define SCROLL_TIME_ADJUSTOFFSET	40
#define SCROLL_TIME_FLOOR					20

typedef struct scrollInfo_s {
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	itemDef_t *item;
	qboolean scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static void (*captureFunc) (void *p) = 0;
static void *captureData = NULL;

static itemDef_t *itemCapture = NULL;   // item that has the mouse captured ( if any )

displayContextDef_t *DC = NULL;

static qboolean g_waitingForKey = qfalse;
static qboolean g_editingField = qfalse;

static itemDef_t *g_bindItem = NULL;
static itemDef_t *g_editItem = NULL;

//FIXME hack
static itemDef_t *LastFocusItem = NULL;

menuDef_t Menus[MAX_MENUS];      // defined menus
int menuCount = 0;               // how many

menuDef_t *menuStack[MAX_OPEN_MENUS];
int openMenuCount = 0;

static qboolean debugMode = qfalse;

#define DOUBLE_CLICK_DELAY 300
static int lastListBoxClickTime = 0;

static void Item_RunScript(itemDef_t *item, const char *s);
static void Item_RunFrameScript (itemDef_t *item, const char *script);
static void Item_SetupKeywordHash(void);
static void Menu_SetupKeywordHash(void);
static int BindingIDFromName(const char *name);
static qboolean Item_Bind_HandleKey(itemDef_t *item, int key, qboolean down);
static itemDef_t *Menu_SetPrevCursorItem(menuDef_t *menu);
static itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu);
static qboolean Menu_OverActiveItem(menuDef_t *menu, float x, float y);
static qboolean Item_Slider_HandleKey (itemDef_t *item, int key, qboolean down);

//char *Q_MathScript (char *script, float *val, int *error);

static qboolean ItemParse_if (itemDef_t *item, int handle, char *token, qboolean forceSkip);

#ifdef CGAME
#define MEM_POOL_SIZE  (1024 * 1024 * 2)  //128 * 1024
#else
#define MEM_POOL_SIZE  (1024 * 1024 * 2)  //1024 * 1024
#endif

static char		memoryPool[MEM_POOL_SIZE];
static int		allocPoint, outOfMemory;


/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size ) {
	char	*p;

	if ( allocPoint + size > MEM_POOL_SIZE ) {
		outOfMemory = qtrue;
		if (DC->Print) {
			DC->Print("UI_Alloc: Failure. Out of memory!  %d + %d > %d\n", allocPoint, size, MEM_POOL_SIZE);
		}
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = qfalse;
}

qboolean UI_OutOfMemory( void ) {
	return outOfMemory;
}



#define HASH_TABLE_SIZE 2048
/*
================
return a hash value for the string
================
*/
static unsigned hashForString(const char *str) {
	int		i;
	unsigned	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (str[i] != '\0') {
		letter = tolower(str[i]);
		hash+=(unsigned)(letter)*(i+119);
		i++;
	}
	hash &= (HASH_TABLE_SIZE-1);
	return hash;
}

typedef struct stringDef_s {
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

static int strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

static int strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];


const char *String_Alloc(const char *p) {
	int len;
	unsigned hash;
	stringDef_t *str, *last;
	static const char *staticNULL = "";

	if (p == NULL) {
		Com_Printf("^1String_Alloc() p == NULL\n");
		return NULL;
	}

	if (*p == 0) {
		return staticNULL;
	}

	hash = hashForString(p);

	str = strHandle[hash];
	while (str) {
		if (strcmp(p, str->str) == 0) {
			return str->str;
		}
		str = str->next;
	}

#if 0
	Com_Printf("%0x  new string '%s'  len %d\n", hash, p, strlen(p));
	str = strHandle[hash];
	while (str) {
		Com_Printf("^5%0x '%s'\n", hash, str->str);
		if (strcmp(p, str->str) == 0) {
			//Com_Printf("String_Alloc() got it: '%s'\n", str->str);
			return str->str;
		}
		str = str->next;
	}
#endif

	len = strlen(p);
	if (len + strPoolIndex + 1 < STRING_POOL_SIZE) {
		int ph = strPoolIndex;
		strcpy(&strPool[strPoolIndex], p);
		strPoolIndex += len + 1;

		last = strHandle[hash];
		if (last) {
			while (last->next) {
				last = last->next;
			}
		}

		str  = UI_Alloc(sizeof(stringDef_t));
		str->next = NULL;
		str->str = &strPool[ph];
		if (last) {
			last->next = str;
		} else {
			strHandle[hash] = str;
		}

		//Com_Printf("^5String_Alloc() space left: %fmb of %fmb\n", (STRING_POOL_SIZE - strPoolIndex) / (1024.0 * 1024.0), STRING_POOL_SIZE / (1024.0 * 1024.0));

		return &strPool[ph];
	}

	Com_Printf("^1String_Alloc() out of memory\n");
	String_Report();

	return NULL;
}

void String_Report(void) {
	float f;
	Com_Printf("Memory/String Pool Info\n");
	Com_Printf("----------------\n");
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf("String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE);
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf("Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE);
}

/*
=================
String_Init
=================
*/
void String_Init(void) {
	int i;
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		strHandle[i] = NULL;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	menuCount = 0;
	openMenuCount = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();
	if (DC && DC->getBindingBuf) {
		Controls_GetConfig();
	}
}

#if 1  // unused
/*
=================
PC_SourceWarning
=================
*/
static NO_WARNING_UNUSED_FUNCTION __attribute__((format(printf, 2, 3))) void PC_SourceWarning (int handle, char *format, ...)
{
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string);
}
#endif

/*
=================
PC_SourceError
=================
*/
static __attribute__((format(printf, 2, 3))) void PC_SourceError (int handle, char *format, ...)
{
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);
}

qboolean MenuVar_Set (const char *varName, float f)
{
	int i;

	for (i = 0;  i < MAX_MENU_VARS;  i++) {
		menuVar_t *mv;

		mv = &menuVars[i];
		if (mv->name[0] == '\0'  ||  !Q_stricmp(varName, mv->name)) {
			if (mv->name[0] == '\0') {
				Q_strncpyz(mv->name, varName, sizeof(mv->name));
				if (strlen(varName) >= MAX_MENU_VAR_NAME) {
					Com_Printf("^1ERROR: truncated menu name, max menu var name length is %d: '%s'\n", MAX_MENU_VAR_NAME, varName);
				}
			}
			mv->value = f;

			return qtrue;
		}
	}

	Com_Printf("^3MenuSetVar() couldn't set '%s', MAX_MENU_VARS (%d) reached\n", varName, MAX_MENU_VARS);

	return qfalse;
}

float MenuVar_Get (const char *varName)
{
	int i;
	float f;

	f = 0;
	for (i = 0;  i < MAX_MENU_VARS;  i++) {
		menuVar_t *mv;

		mv = &menuVars[i];
		if (mv->name[0] == '\0') {
			Com_Printf("^3MenuVar_Get() no such menu variable '%s'\n", varName);
			break;
		}

		if (!Q_stricmp(varName, mv->name)) {
			f = mv->value;
			break;
		}
	}

	return f;
}

/*
=================
LerpColor
=================
*/
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

/*
=================
Float_Parse
=================
*/
qboolean Float_Parse (char **p, float *f) {
	char	*token;
	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0) {
		if (token[0] != '@') {
			if (token[0] == '0'  &&  (token[1] == 'x'  ||  token[1] == 'X')) {
				*f = Com_HexStrToInt(token);
			} else {
				if (!Q_isdigit(token[0])  &&  (token[0] != '+'  &&  token[0] != '-'  &&  token[0] != '.')) {
					Com_Printf("^1Float_Parse() invalid float token: '%s'\n", token);
				}
				*f = atof(token);
			}
		} else {
			*f = MenuVar_Get(token);
		}
		return qtrue;
	} else {
		Com_Printf("^1Float_Parse() couldn't get token\n");
		return qfalse;
	}
}

/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse(int handle, float *f) {
	pc_token_t token;
	char *p;
	char buf[MAX_TOKENLENGTH];

	if (UseScriptBuffer) {
		return Float_Parse(&ScriptBuffer, f);
	}

	p = buf;
	p[0] = '\0';

	if (!trap_PC_ReadToken(handle, &token)) {
		Com_Printf("^1PC_Float_Parse() couldn't get token1\n");
		return qfalse;
	}

	if (token.string[0] == '-') {
		p[0] = '-';
		p[1] = '\0';
		p++;
		if (!trap_PC_ReadToken(handle, &token)) {
			Com_Printf("^1PC_Float_Parse() couldn't get token2\n");
			return qfalse;
		}
	}

	Q_strncpyz(p, token.string, sizeof(buf) - 1);

	p = buf;
	Float_Parse(&p, f);

	return qtrue;
}

/*
=================
Color_Parse
=================
*/
qboolean Color_Parse(char **p, vec4_t *c) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!Float_Parse(p, &f)) {
			Com_Printf("^1Color_Parse() couldn't get token %d\n", i);
			//Com_Printf("^3'%s'\n", *p);
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse(int handle, vec4_t *c) {
	int i;
	float f;

	if (UseScriptBuffer) {
		return Color_Parse(&ScriptBuffer, c);
	}

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			Com_Printf("^1PC_Color_Parse() couldn't get token %d\n", i);
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/*
=================
Int_Parse
=================
*/
qboolean Int_Parse(char **p, int *i) {
	char	*token;
	token = COM_ParseExt(p, qfalse);

	if (token && token[0] != 0) {
		if (token[0] != '@') {
			if (token[0] == '0'  &&  (token[1] == 'x'  ||  token[1] == 'X')) {
                *i = Com_HexStrToInt(token);
			} else {
				if (!Q_isdigit(token[0])  &&  (token[0] != '+'  &&  token[0] != '-')) {
					Com_Printf("^1Int_Parse() invalid integer token: '%s'\n", token);
				}
				*i = atoi(token);
			}
		} else {
			*i = round(MenuVar_Get(token));
		}
		return qtrue;
	} else {
		Com_Printf("^1Int_Parse() couldn't get token\n");
		return qfalse;
	}
}

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse (int handle, int *i)
{
	pc_token_t token;
	char *p;
	char buf[MAX_TOKENLENGTH];

	if (!i) {
		Com_Printf("^1ERROR PC_Int_Parse i == null\n");
		return qfalse;
	}

	if (UseScriptBuffer) {
		return Int_Parse(&ScriptBuffer, i);
	}

	p = buf;
	p[0] = '\0';

	if (!trap_PC_ReadToken(handle, &token)) {
		Com_Printf("^1PC_Int_Parse() couldn't get token1\n");
		return qfalse;
	}

	if (token.string[0] == '-') {
		p[0] = '-';
		p[1] = '\0';
		p++;
		if (!trap_PC_ReadToken(handle, &token)) {
			Com_Printf("^1PC_Int_Parse() couldn't get token2\n");
			return qfalse;
		}
	}

	Q_strncpyz(p, token.string, sizeof(buf) - 1);

	p = buf;
	Int_Parse(&p, i);

	return qtrue;
}

/*
=================
Rect_Parse
=================
*/
qboolean Rect_Parse(char **p, rectDef_t *r) {
	if (Float_Parse(p, &r->x)) {
		if (Float_Parse(p, &r->y)) {
			if (Float_Parse(p, &r->w)) {
				if (Float_Parse(p, &r->h)) {
					return qtrue;
				}
			}
		}
	}

	Com_Printf("^1Rect_Parse() couldn't parse rectangle\n");
	return qfalse;
}

/*
=================
PC_Rect_Parse
=================
*/
qboolean PC_Rect_Parse(int handle, rectDef_t *r) {
	if (UseScriptBuffer) {
		return Rect_Parse(&ScriptBuffer, r);
	}

	if (PC_Float_Parse(handle, &r->x)) {
		if (PC_Float_Parse(handle, &r->y)) {
			if (PC_Float_Parse(handle, &r->w)) {
				if (PC_Float_Parse(handle, &r->h)) {
					return qtrue;
				}
			}
		}
	}

	Com_Printf("^1PC_Rect_Parse() couldn't parse rectangle\n");
	return qfalse;
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse(char **p, const char **out) {
	char *token;

	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0) {
		*(out) = String_Alloc(token);
		if (!*(out)) {
			Com_Printf("^1String_Parse() couldn't allocate string\n");
			return qfalse;
		}

		return qtrue;
	}

	//Com_Printf("^1String_Parse() couldn't parse string\n");
	return qfalse;
}

/*
=================
PC_String_Parse
=================
*/
qboolean PC_String_Parse(int handle, const char **out) {
	pc_token_t token;

	if (UseScriptBuffer) {
		return String_Parse(&ScriptBuffer, out);
	}

	if (!trap_PC_ReadToken(handle, &token)) {
		//Com_Printf("^1PC_String_Parse() couldn't read token\n");
		return qfalse;
	}

	*(out) = String_Alloc(token.string);

	if (!*(out)) {
		Com_Printf("^1PC_String_Parse() couldn't allocate string\n");
		return qfalse;
	}

    return qtrue;
}

static qboolean Script_ParseExt (char **p, const char **out, const char **lastToken)
{
	char script[1024];
	const char *token;
	int braceCount;

	memset(script, 0, sizeof(script));
	if (lastToken) {
		*lastToken = NULL;
	}

	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if (!String_Parse(p, &token)) {
		Com_Printf("^1Script_Parse() couldn't get first token\n");
		return qfalse;
	}
	if (Q_stricmp(token, "{") != 0) {
		if (lastToken == NULL) {
			Com_Printf("^1Script_Parse() first token != '{'  '%s'\n", token);
			return qfalse;
		} else {
			*lastToken = String_Alloc(token);
			return qfalse;
		}
	}

	braceCount = 1;
	while ( 1 ) {
		if (!String_Parse(p, &token)) {
			Com_Printf("^1Script_Parse() couldn't get token in loop\n");
			return qfalse;
		}
		if (!token) {
			Com_Printf("^1Script_Parse() token == NULL\n");
			Com_Printf("p == '%s'\n", *p);
			return qfalse;
		}

		if (Q_stricmp(token, "}") == 0) {
			braceCount--;
			if (braceCount < 0) {
				Com_Printf("^1Script_Parse() braceCount < 0 (%d)\n", braceCount);
				return qfalse;
			} else if (braceCount == 0) {
				*out = String_Alloc(script);
				if (!*(out)) {
					Com_Printf("^1Script_Parse() couldn't allocate string\n");
					return qfalse;
				}
				return qtrue;
			}
		} else if (Q_stricmp(token, "{") == 0) {
			braceCount++;
		}

		if (token[1] != '\0') {
			Q_strcat(script, 1024, va("\"%s\"", token));
		} else {
			Q_strcat(script, 1024, token);
		}
		Q_strcat(script, 1024, " ");
	}

	// shouldn't get here
	return qfalse;
}

/*
=================
Script_Parse
=================
*/
qboolean Script_Parse (char **p, const char **out)
{
	return Script_ParseExt(p, out, NULL);
}

qboolean PC_Script_ParseExt (int handle, const char **out, const char **lastToken)
{
	char script[1024];
	pc_token_t token;
	int braceCount;

	if (UseScriptBuffer) {
		return Script_ParseExt(&ScriptBuffer, out, lastToken);
	}

	memset(script, 0, sizeof(script));
	if (lastToken) {
		*lastToken = NULL;
	}

	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if (!trap_PC_ReadToken(handle, &token)) {
		Com_Printf("^1PC_Script_Parse() couldn't get first token\n");
		return qfalse;
	}
	if (Q_stricmp(token.string, "{") != 0) {
		if (lastToken == NULL) {
			Com_Printf("^1PC_Script_Parse() first token != '{'  '%s'\n", token.string);
			return qfalse;
		} else {
			*lastToken = String_Alloc(token.string);
			return qfalse;
		}
	}

	braceCount = 1;
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token)) {
			Com_Printf("^1PC_Script_Parse() couldn't get token in loop\n");
			return qfalse;
		}
		if (Q_stricmp(token.string, "}") == 0) {
			braceCount--;
			if (braceCount < 0) {
				Com_Printf("^1PC_Script_Parse() braceCount < 0 (%d)\n", braceCount);
				return qfalse;
			} else if (braceCount == 0) {
				*out = String_Alloc(script);
				if (!*out) {
					Com_Printf("^1PC_Script_Parse() couldn't allocate string\n");
					return qfalse;
				}
				return qtrue;
			}
		} else if (Q_stricmp(token.string, "{") == 0) {
			braceCount++;
		}

		if (token.string[1] != '\0') {
			Q_strcat(script, 1024, va("\"%s\"", token.string));
		} else {
			Q_strcat(script, 1024, token.string);
		}
		Q_strcat(script, 1024, " ");
	}

	// shouldn't get here
	return qfalse;
}

/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse (int handle, const char **out)
{
	return PC_Script_ParseExt(handle, out, NULL);
}

/*
=================
Parenthesis_Parse
=================
*/
qboolean Parenthesis_Parse(char **p, const char **out) {
	char script[1024];
	const char *token;
	int braceCount;

	memset(script, 0, sizeof(script));
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if (!String_Parse(p, &token)) {
		Com_Printf("^1Parenthesis_Parse() couldn't get first token\n");
		return qfalse;
	}
	if (Q_stricmp(token, "(") != 0) {
		Com_Printf("^1Parenthesis_Parse() first token != '('  '%s'\n", token);
	    return qfalse;
	}

	braceCount = 1;
	while ( 1 ) {
		if (!String_Parse(p, &token)) {
			Com_Printf("^1Parenthesis_Parse() couldn't get token in loop\n");
			return qfalse;
		}
		if (Q_stricmp(token, ")") == 0) {
			braceCount--;
			if (braceCount < 0) {
				Com_Printf("^1Parenthesis_Parse() braceCount < 0 (%d)\n", braceCount);
				return qfalse;
			} else if (braceCount == 0) {
				*out = String_Alloc(script);
				if (!*out) {
					Com_Printf("^1Parenthesis_Parse() couldn't allocate string\n");
					return qfalse;
				}
				return qtrue;
			}
		} else if (Q_stricmp(token, "(") == 0) {
			braceCount++;
		}

		if (token[1] != '\0') {
			Q_strcat(script, 1024, va("\"%s\"", token));
		} else {
			Q_strcat(script, 1024, token);
		}
		Q_strcat(script, 1024, " ");
	}

	// shouldn't get here
	return qfalse;
}

/*
=================
PC_Parenthesis_Parse
=================
*/
qboolean PC_Parenthesis_Parse(int handle, const char **out) {
	char script[1024];
	pc_token_t token;
	int braceCount;

	if (UseScriptBuffer) {
		return Parenthesis_Parse(&ScriptBuffer, out);
	}

	memset(script, 0, sizeof(script));
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if (!trap_PC_ReadToken(handle, &token)) {
		Com_Printf("^1PC_Parenthesis_Parse() couldn't get first token\n");
		return qfalse;
	}
	if (Q_stricmp(token.string, "(") != 0) {
		Com_Printf("^1PC_Parenthesis_Parse() first token != '('  '%s'\n", token.string);
	    return qfalse;
	}

	braceCount = 1;
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token)) {
			Com_Printf("^1PC_Parenthesis_Parse() couldn't get token in loop\n");
			return qfalse;
		}
		if (Q_stricmp(token.string, ")") == 0) {
			braceCount--;
			if (braceCount < 0) {
				Com_Printf("^1PC_Parenthesis_Parse() braceCount < 0 (%d)\n", braceCount);
				return qfalse;
			} else if (braceCount == 0) {
				*out = String_Alloc(script);
				if (!*out) {
					Com_Printf("^1PC_Parenthesis_Parse() couldn't allocate string\n");
					return qfalse;
				}
				return qtrue;
			}
		} else if (Q_stricmp(token.string, "(") == 0) {
			braceCount++;
		}

		if (token.string[1] != '\0') {
			Q_strcat(script, 1024, va("\"%s\"", token.string));
		} else {
			Q_strcat(script, 1024, token.string);
		}
		Q_strcat(script, 1024, " ");
	}

	// shouldn't get here
	return qfalse;
}

// display, window, menu, item code
//

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void Init_Display(displayContextDef_t *dc) {
	DC = dc;
}



// type and style painting 

static void GradientBar_Paint (rectDef_t *rect, vec4_t color, int widescreen, rectDef_t menuRect) {
	// gradient bar takes two paints
	DC->setColor( color );
	DC->drawHandlePic(rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar, widescreen, menuRect);
	DC->setColor( NULL );
}


/*
==================
Window_Init

Initializes a window structure ( windowDef_t ) with defaults
 
==================
*/
static void Window_Init(Window *w) {
	memset(w, 0, sizeof(windowDef_t));
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

static void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount) {
  if (*flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN)) {
    if (DC->realTime > *nextTime) {
      *nextTime = DC->realTime + offsetTime;
      if (*flags & WINDOW_FADINGOUT) {
        *f -= fadeAmount;
        if (bFlags && *f <= 0.0) {
          *flags &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE);
        }
      } else {
        *f += fadeAmount;
        if (*f >= clamp) {
          *f = clamp;
          if (bFlags) {
            *flags &= ~WINDOW_FADINGIN;
          }
        }
      }
    }
  }
}



static void Window_Paint(Window *w, float fadeAmount, float fadeClamp, float fadeCycle, int widescreen, rectDef_t menuRect) {
  //float bordersize = 0;
  vec4_t color;
  rectDef_t fillRect;

  if ( w == NULL ) {
	  return;
  }

  //Com_Printf("window paint\n");

  if (debugMode) {
    color[0] = color[1] = color[2] = color[3] = 1;
    DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color, widescreen, menuRect);
  }

  if (w->style == 0 && w->border == 0) {
    return;
  }

  fillRect = w->rect;

  if (w->border != 0) {
    fillRect.x += w->borderSize;
    fillRect.y += w->borderSize;
    fillRect.w -= w->borderSize + 1;
    fillRect.h -= w->borderSize + 1;
  }

  if (w->style == WINDOW_STYLE_FILLED) {
    // box, but possible a shader that needs filled
		if (w->background) {
		  Fade(&w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, qtrue, fadeAmount);
      DC->setColor(w->backColor);
	  DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background, widescreen, menuRect);
		  DC->setColor(NULL);
		} else {
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor, widescreen, menuRect);
		}
  } else if (w->style == WINDOW_STYLE_GRADIENT) {
	  GradientBar_Paint(&fillRect, w->backColor, widescreen, menuRect);
    // gradient bar
  } else if (w->style == WINDOW_STYLE_SHADER) {
    if (w->flags & WINDOW_FORECOLORSET) {
      DC->setColor(w->foreColor);
    }
    DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background, widescreen, menuRect);
    DC->setColor(NULL);
  } else if (w->style == WINDOW_STYLE_TEAMCOLOR) {
    if (DC->getTeamColor) {
      DC->getTeamColor(&color);
      DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, color, widescreen, menuRect);
    }
  } else if (w->style == WINDOW_STYLE_CINEMATIC) {
		if (w->cinematic == -1) {
			w->cinematic = DC->playCinematic(w->cinematicName, fillRect.x, fillRect.y, fillRect.w, fillRect.h, widescreen, menuRect);
			if (w->cinematic == -1) {
				w->cinematic = -2;
			}
		} 
		if (w->cinematic >= 0) {
			//FIXME why two calls?
	    DC->runCinematicFrame(w->cinematic);
		DC->drawCinematic(w->cinematic, fillRect.x, fillRect.y, fillRect.w, fillRect.h, widescreen, menuRect);
		}
  }

  if (w->border == WINDOW_BORDER_FULL) {
    // full
    // HACK HACK HACK
    if (w->style == WINDOW_STYLE_TEAMCOLOR) {
      if (color[0] > 0) { 
        // red
        color[0] = 1;
        color[1] = color[2] = .5;

      } else {
        color[2] = 1;
        color[0] = color[1] = .5;
      }
      color[3] = 1;
      DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color, widescreen, menuRect);
    } else {
		DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor, widescreen, menuRect);
    }
  } else if (w->border == WINDOW_BORDER_HORZ) {
    // top/bottom
    DC->setColor(w->borderColor);
    DC->drawTopBottom(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, widescreen, menuRect);
  	DC->setColor( NULL );
  } else if (w->border == WINDOW_BORDER_VERT) {
    // left right
    DC->setColor(w->borderColor);
    DC->drawSides(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, widescreen, menuRect);
  	DC->setColor( NULL );
  } else if (w->border == WINDOW_BORDER_KCGRADIENT) {
    // this is just two gradient bars along each horz edge
    rectDef_t r = w->rect;
    r.h = w->borderSize;
    GradientBar_Paint(&r, w->borderColor, widescreen, menuRect);
    r.y = w->rect.y + w->rect.h - 1;
    GradientBar_Paint(&r, w->borderColor, widescreen, menuRect);
  }

}


static void Item_SetScreenCoords(itemDef_t *item, float x, float y) {
  
  if (item == NULL) {
    return;
  }

  if (item->window.border != 0) {
    x += item->window.borderSize;
    y += item->window.borderSize;
  }

  item->window.rect.x = x + item->window.rectClient.x;
  item->window.rect.y = y + item->window.rectClient.y;
  item->window.rect.w = item->window.rectClient.w;
  item->window.rect.h = item->window.rectClient.h;

  // force the text rects to recompute
  item->textRect.w = 0;
  item->textRect.h = 0;
}

// FIXME: consolidate this with nearby stuff
static void Item_UpdatePosition(itemDef_t *item) {
  float x, y;
  menuDef_t *menu;
  
  if (item == NULL || item->parent == NULL) {
    return;
  }

  menu = item->parent;

  x = menu->window.rect.x;
  y = menu->window.rect.y;
  
  if (menu->window.border != 0) {
    x += menu->window.borderSize;
    y += menu->window.borderSize;
  }

  Item_SetScreenCoords(item, x, y);

}

// menus
static void Menu_UpdatePosition(menuDef_t *menu) {
  int i;
  float x, y;

  if (menu == NULL) {
    return;
  }
  
  x = menu->window.rect.x;
  y = menu->window.rect.y;
  if (menu->window.border != 0) {
    x += menu->window.borderSize;
    y += menu->window.borderSize;
  }

  for (i = 0; i < menu->itemCount; i++) {
    Item_SetScreenCoords(menu->items[i], x, y);
  }
}

static void Menu_PostParse(menuDef_t *menu) {
	if (menu == NULL) {
		return;
	}
	if (menu->fullScreen) {
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition(menu);
}

static itemDef_t *Menu_ClearFocus(menuDef_t *menu) {
  int i;
  itemDef_t *ret = NULL;

  if (menu == NULL) {
    return NULL;
  }

  for (i = 0; i < menu->itemCount; i++) {
    if (menu->items[i]->window.flags & WINDOW_HASFOCUS) {
      ret = menu->items[i];
    } 
    menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;
    if (menu->items[i]->leaveFocus) {
      Item_RunScript(menu->items[i], menu->items[i]->leaveFocus);
    }
  }
 
  return ret;
}

static qboolean IsVisible(int flags) {
  return (flags & WINDOW_VISIBLE && !(flags & WINDOW_FADINGOUT));
}

static qboolean Rect_ContainsPoint (const rectDef_t *rect, float x, float y)
{
  if (rect) {
    if (x > rect->x && x < rect->x + rect->w && y > rect->y && y < rect->y + rect->h) {
      return qtrue;
    }
  }
  return qfalse;
}

#if 0
static qboolean Item_ContainsPoint (const itemDef_t *item, float x, float y)
{
	rectDef_t rect;
	float aspect;
	float width43;
	float diff;
	float diff640;
	float newXScale;

	if (!item) {
		Com_Printf("^1Item_ContainsPoint item == NULL\n");
		return qfalse;
	}

	memcpy(&rect, &item->window.rect, sizeof(rectDef_t));
	//FIXME get cg_wideScreen value

	//FIXME store calculations
	aspect = (float)DC->glconfig.vidWidth / (float)DC->glconfig.vidHeight;

	width43 = 4.0f * (DC->glconfig.vidHeight / 3.0);
	diff = (float)DC->glconfig.vidWidth - width43;

	diff640 = 640.0f * diff / (float)DC->glconfig.vidWidth;
	newXScale = width43 / (float)DC->glconfig.vidWidth;

	// already scaled to 640x480, reverse and apply new

	if (item->widescreen == WIDESCREEN_NONE  ||  aspect <= 1.25f) {
		//use regular scaling, don't alter rect values
	} else if (item->widescreen == WIDESCREEN_LEFT) {
		rect.w *= newXScale;
	} else if (item->widescreen == WIDESCREEN_CENTER) {
		rect.w *= newXScale;
		rect.x *= newXScale;
		rect.x += diff640 / 2;
	} else if (item->widescreen == WIDESCREEN_RIGHT) {
		rect.w *= newXScale;
		rect.x += diff640;
	}

	return Rect_ContainsPoint(&rect, x, y);
}
#endif

static qboolean Rect_ContainsWidescreenPoint (const rectDef_t *rectIn, float x, float y, int widescreen)
{
	rectDef_t rect;
	float aspect;
	float width43;
	float diff;
	float diff640;
	float newXScale;

	if (!rectIn) {
		Com_Printf("^1Rect_ContainsWidescreenPoint rectIn == NULL\n");
		return qfalse;
	}

	memcpy(&rect, rectIn, sizeof(rectDef_t));

	//FIXME store calculations
	aspect = (float)DC->glconfig.vidWidth / (float)DC->glconfig.vidHeight;

	width43 = 4.0f * (DC->glconfig.vidHeight / 3.0);
	diff = (float)DC->glconfig.vidWidth - width43;

	diff640 = 640.0f * diff / (float)DC->glconfig.vidWidth;
	newXScale = width43 / (float)DC->glconfig.vidWidth;

	// already scaled to 640x480, reverse and apply new

	if (widescreen == WIDESCREEN_NONE  ||  aspect <= 1.25f  ||  DC->widescreen != 5) {
		//use regular scaling, don't alter rect values
	} else if (widescreen == WIDESCREEN_LEFT) {
		rect.w *= newXScale;
	} else if (widescreen == WIDESCREEN_CENTER) {
		rect.w *= newXScale;
		rect.x *= newXScale;
		rect.x += diff640 / 2;
	} else if (widescreen == WIDESCREEN_RIGHT) {
		rect.w *= newXScale;
		rect.x += diff640;
	}

	return Rect_ContainsPoint(&rect, x, y);
}

// assumes cursor is being drawn fullscreen (widescreen == WIDESCREEN_NONE),
// converts to the relative 640x480 coordinates of the widescreen hud
static float CursorX_Widescreen (int widescreen)
{
	float aspect;
	float width43;
	float diff;
	float realXScale;
	float newXScale;
	float x;

	x = DC->cursorx;

	//FIXME store calculations
	aspect = (float)DC->glconfig.vidWidth / (float)DC->glconfig.vidHeight;
	width43 = 4.0 * (DC->glconfig.vidHeight / 3.0f);
	diff = (float)DC->glconfig.vidWidth - width43;

	realXScale = (float)DC->glconfig.vidWidth / 640.0f;
	newXScale = 640.0f / width43;

	if (widescreen == WIDESCREEN_NONE  ||  aspect <= 1.25f  ||  DC->widescreen != 5) {
		//use regular scaling, don't chage x
	} else if (widescreen == WIDESCREEN_LEFT) {
		x *= realXScale;
		x *= newXScale;
	} else if (widescreen == WIDESCREEN_CENTER) {
		x *= realXScale;  // where we really are
		x -= diff / 2;  // relative to widescreen box
		x *= newXScale;
	} else if (widescreen == WIDESCREEN_RIGHT) {
		x *= realXScale;
		x -= diff;
		x *= newXScale;
	}

	if (x < 0.0f) {
		x = 0.0f;
	} else if (x > 640.0f) {
		x = 640.0f;
	}

	return x;
}

static int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name) {
  int i;
  int count = 0;
  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0)) {
      count++;
    }
  }
  return count;
}

static itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name) {
  int i;
  int count = 0;
  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0)) {
      if (count == index) {
        return menu->items[i];
      }
      count++;
    } 
  }
  return NULL;
}



static void Script_SetColor(itemDef_t *item, char **args) {
  const char *name;
  int i;
  float f;
  vec4_t *out;
  // expecting type of color to set and 4 args for the color
  if (String_Parse(args, &name)) {
      out = NULL;
      if (Q_stricmp(name, "backcolor") == 0) {
        out = &item->window.backColor;
        item->window.flags |= WINDOW_BACKCOLORSET;
      } else if (Q_stricmp(name, "forecolor") == 0) {
        out = &item->window.foreColor;
        item->window.flags |= WINDOW_FORECOLORSET;
      } else if (Q_stricmp(name, "bordercolor") == 0) {
        out = &item->window.borderColor;
      }

      if (out) {
        for (i = 0; i < 4; i++) {
          if (!Float_Parse(args, &f)) {
            return;
          }
          (*out)[i] = f;
        }
      }
  }
}

static void Script_SetAsset(itemDef_t *item, char **args) {
  const char *name;
  // expecting name to set asset to
  if (String_Parse(args, &name)) {
    // check for a model 
    if (item->type == ITEM_TYPE_MODEL) {
    }
  }
}

static void Script_SetBackground(itemDef_t *item, char **args) {
  const char *name;

  // expecting name to set asset to
  if (String_Parse(args, &name)) {
	  if (Q_stricmp(name, item->window.backgroundName)) {
		  item->window.background = DC->registerShaderNoMip(name);
		  item->window.backgroundName = String_Alloc(name);
	  }
  }
}


static itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p) {
  int i;
  if (menu == NULL || p == NULL) {
    return NULL;
  }

  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(p, menu->items[i]->window.name) == 0) {
      return menu->items[i];
    }
  }

  return NULL;
}

static void Script_SetTeamColor(itemDef_t *item, char **args) {
  if (DC->getTeamColor) {
    int i;
    vec4_t color;
    DC->getTeamColor(&color);
    for (i = 0; i < 4; i++) {
      item->window.backColor[i] = color[i];
    }
  }
}

static void Script_SetItemColor(itemDef_t *item, char **args) {
  const char *itemname;
  const char *name;
  vec4_t color;
  int i;
  vec4_t *out;
  // expecting type of color to set and 4 args for the color
  if (String_Parse(args, &itemname) && String_Parse(args, &name)) {
    itemDef_t *item2;
    int j;
    int count = Menu_ItemsMatchingGroup(item->parent, itemname);

    if (!Color_Parse(args, &color)) {
      return;
    }

    for (j = 0; j < count; j++) {
      item2 = Menu_GetMatchingItemByNumber(item->parent, j, itemname);
      if (item2 != NULL) {
        out = NULL;
        if (Q_stricmp(name, "backcolor") == 0) {
          out = &item2->window.backColor;
        } else if (Q_stricmp(name, "forecolor") == 0) {
          out = &item2->window.foreColor;
          item2->window.flags |= WINDOW_FORECOLORSET;
        } else if (Q_stricmp(name, "bordercolor") == 0) {
          out = &item2->window.borderColor;
        }

        if (out) {
          for (i = 0; i < 4; i++) {
            (*out)[i] = color[i];
          }
        }
      }
    }
  }
}


static void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup(menu, p);
	for (i = 0; i < count; i++) {
		item = Menu_GetMatchingItemByNumber(menu, i, p);
		if (item != NULL) {
			if (bShow) {
				item->window.flags |= WINDOW_VISIBLE;
			} else {
				item->window.flags &= ~WINDOW_VISIBLE;
				// stop cinematics playing in the window
				if (item->window.cinematic >= 0) {
					DC->stopCinematic(item->window.cinematic);
					item->window.cinematic = -1;
				}
			}
		}
	}
}

static void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut) {
  itemDef_t *item;
  int i;
  int count = Menu_ItemsMatchingGroup(menu, p);
  for (i = 0; i < count; i++) {
    item = Menu_GetMatchingItemByNumber(menu, i, p);
    if (item != NULL) {
      if (fadeOut) {
        item->window.flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
        item->window.flags &= ~WINDOW_FADINGIN;
      } else {
        item->window.flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
        item->window.flags &= ~WINDOW_FADINGOUT;
      }
    }
  }
}

menuDef_t *Menus_FindByName(const char *p) {
  int i;
  for (i = 0; i < menuCount; i++) {
    if (Q_stricmp(Menus[i].window.name, p) == 0) {
      return &Menus[i];
    } 
  }
  return NULL;
}

void Menus_ShowByName(const char *p) {
	menuDef_t *menu = Menus_FindByName(p);
	if (menu) {
		Menus_Activate(menu);
	}
}

void Menus_OpenByName(const char *p) {
  Menus_ActivateByName(p);
}

static void Menu_RunCloseScript(menuDef_t *menu) {
	if (menu && menu->window.flags & WINDOW_VISIBLE && menu->onClose) {
		itemDef_t item;
    item.parent = menu;
    Item_RunScript(&item, menu->onClose);
	}
}

void Menus_CloseByName(const char *p) {
  menuDef_t *menu = Menus_FindByName(p);
  if (menu != NULL) {
		Menu_RunCloseScript(menu);
		menu->window.flags &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS);
  }
}

void Menus_CloseAll(void) {
  int i;
  for (i = 0; i < menuCount; i++) {
		Menu_RunCloseScript(&Menus[i]);
		Menus[i].window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
  }
}


static void Script_Show(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_ShowItemByName(item->parent, name, qtrue);
  }
}

static void Script_Hide(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_ShowItemByName(item->parent, name, qfalse);
  }
}

static void Script_FadeIn(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_FadeItemByName(item->parent, name, qfalse);
  }
}

static void Script_FadeOut(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_FadeItemByName(item->parent, name, qtrue);
  }
}



static void Script_Open(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menus_OpenByName(name);
  }
}

static void Script_ConditionalOpen(itemDef_t *item, char **args) {
	const char *cvar;
	const char *name1;
	const char *name2;
	float           val;

	if ( String_Parse(args, &cvar) && String_Parse(args, &name1) && String_Parse(args, &name2) ) {
		val = DC->getCVarValue( cvar );
		if ( val == 0.f ) {
			Menus_OpenByName(name2);
		} else {
			Menus_OpenByName(name1);
		}
	}
}

static void Script_Close(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menus_CloseByName(name);
  }
}

static void Menu_TransitionItemByName(menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt) {
  itemDef_t *item;
  int i;
  int count = Menu_ItemsMatchingGroup(menu, p);
  for (i = 0; i < count; i++) {
    item = Menu_GetMatchingItemByNumber(menu, i, p);
    if (item != NULL) {
      item->window.flags |= (WINDOW_INTRANSITION | WINDOW_VISIBLE);
      item->window.offsetTime = time;
			memcpy(&item->window.rectClient, &rectFrom, sizeof(rectDef_t));
			memcpy(&item->window.rectEffects, &rectTo, sizeof(rectDef_t));
			item->window.rectEffects2.x = abs(rectTo.x - rectFrom.x) / amt;
			item->window.rectEffects2.y = abs(rectTo.y - rectFrom.y) / amt;
			item->window.rectEffects2.w = abs(rectTo.w - rectFrom.w) / amt;
			item->window.rectEffects2.h = abs(rectTo.h - rectFrom.h) / amt;
      Item_UpdatePosition(item);
    }
  }
}


static void Script_Transition(itemDef_t *item, char **args) {
  const char *name;
	rectDef_t rectFrom, rectTo;
  int time;
	float amt;

  if (String_Parse(args, &name)) {
    if ( Rect_Parse(args, &rectFrom) && Rect_Parse(args, &rectTo) && Int_Parse(args, &time) && Float_Parse(args, &amt)) {
      Menu_TransitionItemByName(item->parent, name, rectFrom, rectTo, time, amt);
    }
  }
}


static void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time) {
  itemDef_t *item;
  int i;
  int count = Menu_ItemsMatchingGroup(menu, p);
  for (i = 0; i < count; i++) {
    item = Menu_GetMatchingItemByNumber(menu, i, p);
    if (item != NULL) {
      item->window.flags |= (WINDOW_ORBITING | WINDOW_VISIBLE);
      item->window.offsetTime = time;
      item->window.rectEffects.x = cx;
      item->window.rectEffects.y = cy;
      item->window.rectClient.x = x;
      item->window.rectClient.y = y;
      Item_UpdatePosition(item);
    }
  }
}


static void Script_Orbit(itemDef_t *item, char **args) {
  const char *name;
  float cx, cy, x, y;
  int time;

  if (String_Parse(args, &name)) {
    if ( Float_Parse(args, &x) && Float_Parse(args, &y) && Float_Parse(args, &cx) && Float_Parse(args, &cy) && Int_Parse(args, &time) ) {
      Menu_OrbitItemByName(item->parent, name, x, y, cx, cy, time);
    }
  }
}



static void Script_SetFocus(itemDef_t *item, char **args) {
  const char *name;
  itemDef_t *focusItem;

  if (String_Parse(args, &name)) {
    focusItem = Menu_FindItemByName(item->parent, name);
    if (focusItem && !(focusItem->window.flags & WINDOW_DECORATION) && !(focusItem->window.flags & WINDOW_HASFOCUS)) {
      Menu_ClearFocus(item->parent);
      focusItem->window.flags |= WINDOW_HASFOCUS;
      if (focusItem->onFocus) {
        Item_RunScript(focusItem, focusItem->onFocus);
      }
      if (DC->Assets.itemFocusSound) {
		  //Com_Printf("focus sound\n");
        DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
      }
    }
  }
}

static void Script_SetPlayerModel(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    DC->setCVar("team_model", name);
  }
}

static void Script_SetPlayerHead(itemDef_t *item, char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    DC->setCVar("team_headmodel", name);
  }
}

static void Script_SetCvar(itemDef_t *item, char **args) {
	const char *cvar, *val;
	if (String_Parse(args, &cvar) && String_Parse(args, &val)) {
		DC->setCVar(cvar, val);
	}
}

static void Script_SetVar (itemDef_t *item, char **args)
{
	const char *var;
	const char *mathScript;
	float f;
	int err;

	if (!String_Parse(args, &var)) {
		Com_Printf("^1Script_SetVar() couldn't get variable name\n");
		return;
	}

	if (!Parenthesis_Parse(args, &mathScript)) {
		Com_Printf("^1Script_SetVar() couldn't get math block\n");
		return;
	}

	Q_MathScript((char *)mathScript, &f, &err);
	MenuVar_Set(var, f);
}

static void Script_Exec(itemDef_t *item, char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		//Com_Printf("^3exec '%s'\n", val);
		if (DC->executeText) {
			DC->executeText(EXEC_APPEND, va("%s ; ", val));
		}
	}
}

static void Script_Play(itemDef_t *item, char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		if (Q_stricmp(val, item->playSoundName)) {
			item->playSound = DC->registerSound(val, qfalse);
			item->playSoundName = String_Alloc(val);
		}
		DC->startLocalSound(item->playSound, CHAN_LOCAL_SOUND);
	}
}

static void Script_playLooped(itemDef_t *item, char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		DC->stopBackgroundTrack();
		DC->startBackgroundTrack(val, val);
	}
}


commandDef_t commandList[] =
{
  {"fadein", &Script_FadeIn},                   // group/name
  {"fadeout", &Script_FadeOut},                 // group/name
  {"show", &Script_Show},                       // group/name
  {"hide", &Script_Hide},                       // group/name
  {"setcolor", &Script_SetColor},               // works on this
  {"open", &Script_Open},                       // menu
	{"conditionalopen", &Script_ConditionalOpen},	// menu
  {"close", &Script_Close},                     // menu
  {"setasset", &Script_SetAsset},               // works on this
  {"setbackground", &Script_SetBackground},     // works on this
  {"setitemcolor", &Script_SetItemColor},       // group/name
  {"setteamcolor", &Script_SetTeamColor},       // sets this background color to team color
  {"setfocus", &Script_SetFocus},               // sets this background color to team color
  {"setplayermodel", &Script_SetPlayerModel},   // sets this background color to team color
  {"setplayerhead", &Script_SetPlayerHead},     // sets this background color to team color
  {"transition", &Script_Transition},           // group/name
  {"setcvar", &Script_SetCvar},           // group/name
  {"setvar", &Script_SetVar},
  {"exec", &Script_Exec},           // group/name
  {"play", &Script_Play},           // group/name
  {"playlooped", &Script_playLooped},           // group/name
  {"orbit", &Script_Orbit}                      // group/name
};

static int scriptCommandCount = ARRAY_LEN(commandList);


static void Item_RunScript(itemDef_t *item, const char *s) {
  char script[1024], *p;
  int i;
  qboolean bRan;
  memset(script, 0, sizeof(script));
  if (item && s && s[0]) {
    Q_strcat(script, 1024, s);
    p = script;
    while (1) {
      const char *command;
      // expect command then arguments, ; ends command, NULL ends script
      if (!String_Parse(&p, &command)) {
        return;
      }

      if (command[0] == ';' && command[1] == '\0') {
        continue;
      }

      bRan = qfalse;
      for (i = 0; i < scriptCommandCount; i++) {
        if (Q_stricmp(command, commandList[i].name) == 0) {
          (commandList[i].handler(item, &p));
          bRan = qtrue;
          break;
        }
      }
      // not in our auto list, pass to handler
      if (!bRan) {
        DC->runScript(&p);
      }
    }
  }
}


static qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag) {
  char script[1024], *p;
  memset(script, 0, sizeof(script));
  if (item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest) {
		char buff[1024];

		DC->getCVarString(item->cvarTest, buff, sizeof(buff));

    Q_strcat(script, 1024, item->enableCvar);
    p = script;
    while (1) {
      const char *val;
      // expect value then ; or NULL, NULL ends list
      if (!String_Parse(&p, &val)) {
				return (item->cvarFlags & flag) ? qfalse : qtrue;
      }

      if (val[0] == ';' && val[1] == '\0') {
        continue;
      }

			// enable it if any of the values are true
			if (item->cvarFlags & flag) {
        if (Q_stricmp(buff, val) == 0) {
					return qtrue;
				}
			} else {
				// disable it if any of the values are true
        if (Q_stricmp(buff, val) == 0) {
					return qfalse;
				}
			}

    }
		return (item->cvarFlags & flag) ? qfalse : qtrue;
  }
	return qtrue;
}


// will optionaly set focus to this item
static qboolean Item_SetFocus(itemDef_t *item, float x, float y) {
	int i;
	itemDef_t *oldFocus;
	sfxHandle_t *sfx = &DC->Assets.itemFocusSound;
	qboolean playSound = qfalse;
	menuDef_t *parent;

	//Com_Printf("Item_SetFocus %p\n", item);

	// sanity check, non-null, not a decoration and does not already have the focus
	if (item == NULL) {
		return qfalse;
	}

	if (item->window.flags & WINDOW_HASFOCUS) {
		//Com_Printf("already has focus\n");
		return qtrue;
	}

	if (item == NULL || item->window.flags & WINDOW_DECORATION || item->window.flags & WINDOW_HASFOCUS || !(item->window.flags & WINDOW_VISIBLE)) {
		return qfalse;
	}

	// this can be NULL
	parent = (menuDef_t*)item->parent; 
      
	// items can be enabled and disabled based on cvars
	if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) {
		return qfalse;
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW)) {
		return qfalse;
	}

	oldFocus = Menu_ClearFocus(item->parent);

	if (item->type == ITEM_TYPE_TEXT) {
		rectDef_t r;
		r = item->textRect;
		r.y -= r.h;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			item->window.flags |= WINDOW_HASFOCUS;
			if (item->focusSound) {
				sfx = &item->focusSound;
			}
			playSound = qtrue;
		} else {
			if (oldFocus) {
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if (oldFocus->onFocus) {
					Item_RunScript(oldFocus, oldFocus->onFocus);
				}
			}
		}
	} else {
	    item->window.flags |= WINDOW_HASFOCUS;
		if (item->onFocus) {
			Item_RunScript(item, item->onFocus);
		}
		if (item->focusSound) {
			sfx = &item->focusSound;
		}
		playSound = qtrue;
	}

	if (playSound && sfx) {
		DC->startLocalSound( *sfx, CHAN_LOCAL_SOUND );
	}

	for (i = 0; i < parent->itemCount; i++) {
		if (parent->items[i] == item) {
			parent->cursorItem = i;
			break;
		}
	}

	//FIXME hack
	if (item  &&  item->window.flags & WINDOW_HASFOCUS) {
		LastFocusItem = item;
	}

	return qtrue;
}

static int Item_ListBox_MaxScroll(itemDef_t *item) {
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount(item->special);
	int max;

	if (item->window.flags & WINDOW_HORIZONTAL) {
		max = count - (item->window.rect.w / listPtr->elementWidth) + 1;
	}
	else {
		max = count - (item->window.rect.h / listPtr->elementHeight) + 1;
	}
	if (max < 0) {
		return 0;
	}
	return max;
}

static int Item_ListBox_ThumbPosition(itemDef_t *item) {
	float max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	max = Item_ListBox_MaxScroll(item);
	if (item->window.flags & WINDOW_HORIZONTAL) {
		size = item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0) {
			pos = (size-SCROLLBAR_SIZE) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	}
	else {
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0) {
			pos = (size-SCROLLBAR_SIZE) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

static int Item_ListBox_ThumbDrawPosition(itemDef_t *item) {
	int min, max;

	if (itemCapture == item) {
		if (item->window.flags & WINDOW_HORIZONTAL) {
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2*SCROLLBAR_SIZE - 1;
			//Com_Printf("checking cursor x\n");
			//if (DC->cursorx >= min + SCROLLBAR_SIZE/2 && DC->cursorx <= max + SCROLLBAR_SIZE/2) {
			if (CursorX_Widescreen(item->widescreen) >= min + SCROLLBAR_SIZE/2 && CursorX_Widescreen(item->widescreen) <= max + SCROLLBAR_SIZE/2) {
				return CursorX_Widescreen(item->widescreen) - SCROLLBAR_SIZE/2;
			}
			else {
				return Item_ListBox_ThumbPosition(item);
			}
		}
		else {
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2*SCROLLBAR_SIZE - 1;
			if (DC->cursory >= min + SCROLLBAR_SIZE/2 && DC->cursory <= max + SCROLLBAR_SIZE/2) {
				return DC->cursory - SCROLLBAR_SIZE/2;
			}
			else {
				return Item_ListBox_ThumbPosition(item);
			}
		}
	}
	else {
		return Item_ListBox_ThumbPosition(item);
	}
}

static float Item_Slider_ThumbPosition(itemDef_t *item) {
	float value, range, x;
	editFieldDef_t *editDef = item->typeData;

	if (item->text) {
		x = item->textRect.x + item->textRect.w + 8;
	} else {
		x = item->window.rect.x;
	}

	if (!editDef || !item->cvar) {
		return x;
	}

	value = DC->getCVarValue(item->cvar);

	if (value < editDef->minVal) {
		value = editDef->minVal;
	} else if (value > editDef->maxVal) {
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

static int Item_Slider_OverSlider(itemDef_t *item, float x, float y) {
	rectDef_t r;

	//Com_Printf("slider over slider\n");

	if (!item) {
		Com_Printf("^1Item_SlideOverSlide item == NULL\n");
		return 0;
	}

	r.x = Item_Slider_ThumbPosition(item) - (SLIDER_THUMB_WIDTH / 2);
	r.y = item->window.rect.y - 2;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
		return WINDOW_LB_THUMB;
	}
	return 0;
}

static int Item_ListBox_OverLB(itemDef_t *item, float x, float y) {
	rectDef_t r;
	//listBoxDef_t *listPtr;
	int thumbstart;
	//int count;

	if (!item) {
		Com_Printf("^1Item_ListBox_OverLB item == NULL\n");
	}

	//count = DC->feederCount(item->special);
	//listPtr = (listBoxDef_t*)item->typeData;
	if (item->window.flags & WINDOW_HORIZONTAL) {
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		thumbstart = Item_ListBox_ThumbPosition(item);
		r.x = thumbstart;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_THUMB;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_PGDN;
		}
	} else {
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_LEFTARROW;
		}
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_RIGHTARROW;
		}
		thumbstart = Item_ListBox_ThumbPosition(item);
		r.y = thumbstart;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_THUMB;
		}
		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}


static void Item_ListBox_MouseEnter(itemDef_t *item, float x, float y) 
{
	rectDef_t r;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	//Com_Printf("enter list box\n");

	item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN);
	item->window.flags |= Item_ListBox_OverLB(item, x, y);

	if (item->window.flags & WINDOW_HORIZONTAL) {
		if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN))) {
			// check for selection hit as we have exausted buttons and thumb
			if (listPtr->elementStyle == LISTBOX_IMAGE) {
				r.x = item->window.rect.x;
				r.y = item->window.rect.y;
				r.h = item->window.rect.h - SCROLLBAR_SIZE;
				r.w = item->window.rect.w - listPtr->drawPadding;
				if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
					listPtr->cursorPos =  (int)((x - r.x) / listPtr->elementWidth)  + listPtr->startPos;
					if (listPtr->cursorPos >= listPtr->endPos) {
						listPtr->cursorPos = listPtr->endPos;
					}
				}
			} else {
				// text hit.. 
			}
		}
	} else if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN))) {
		r.x = item->window.rect.x;
		r.y = item->window.rect.y;
		r.w = item->window.rect.w - SCROLLBAR_SIZE;
		r.h = item->window.rect.h - listPtr->drawPadding;
		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			listPtr->cursorPos =  (int)((y - 2 - r.y) / listPtr->elementHeight)  + listPtr->startPos;
			if (listPtr->cursorPos > listPtr->endPos) {
				listPtr->cursorPos = listPtr->endPos;
			}
		}
	}
}

static void Item_MouseEnter(itemDef_t *item, float x, float y) {
	rectDef_t r;
	if (item) {
		r = item->textRect;
		r.y -= r.h;
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) {
			return;
		}

		if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW)) {
			return;
		}

		if (Rect_ContainsWidescreenPoint(&r, x, y, item->widescreen)) {
			if (!(item->window.flags & WINDOW_MOUSEOVERTEXT)) {
				Item_RunScript(item, item->mouseEnterText);
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}
			if (!(item->window.flags & WINDOW_MOUSEOVER)) {
				Item_RunScript(item, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

		} else {
			// not in the text rect
			if (item->window.flags & WINDOW_MOUSEOVERTEXT) {
				// if we were
				Item_RunScript(item, item->mouseExitText);
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}
			if (!(item->window.flags & WINDOW_MOUSEOVER)) {
				Item_RunScript(item, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if (item->type == ITEM_TYPE_LISTBOX) {
				Item_ListBox_MouseEnter(item, x, y);
			}
		}
	}
}

static void Item_MouseLeave(itemDef_t *item) {
  if (item) {
    if (item->window.flags & WINDOW_MOUSEOVERTEXT) {
      Item_RunScript(item, item->mouseExitText);
      item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
    }
    Item_RunScript(item, item->mouseExit);
    item->window.flags &= ~(WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW);
  }
}

// unused
#if 0
static itemDef_t *Menu_HitTest(menuDef_t *menu, float x, float y) {
  int i;
  for (i = 0; i < menu->itemCount; i++) {
	  if (Rect_ContainsWidescreenPoint(&menu->items[i]->window.rect, x, y, menu->items[i]->widescreen)) {
		  return menu->items[i];
	  }
  }
  return NULL;
}
#endif

static void Item_SetMouseOver(itemDef_t *item, qboolean focus) {
  if (item) {
    if (focus) {
      item->window.flags |= WINDOW_MOUSEOVER;
    } else {
      item->window.flags &= ~WINDOW_MOUSEOVER;
    }
  }
}


static qboolean Item_OwnerDraw_HandleKey(itemDef_t *item, int key) {
  if (item && DC->ownerDrawHandleKey) {
	  return DC->ownerDrawHandleKey(item->window.ownerDraw, item->window.ownerDrawFlags, item->window.ownerDrawFlags2, &item->special, key);
  }
  return qfalse;
}

static qboolean Item_ListBox_HandleKey(itemDef_t *item, int key, qboolean down, qboolean force) {
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount(item->special);
	int max, viewmax;

	if (force || (Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen) && item->window.flags & WINDOW_HASFOCUS)) {
		max = Item_ListBox_MaxScroll(item);
		if (item->window.flags & WINDOW_HORIZONTAL) {
			viewmax = (item->window.rect.w / listPtr->elementWidth);
			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) 
			{
				if (!listPtr->notselectable) {
					listPtr->cursorPos--;
					if (listPtr->cursorPos < 0) {
						listPtr->cursorPos = 0;
					}
					if (listPtr->cursorPos < listPtr->startPos) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else {
					listPtr->startPos--;
					if (listPtr->startPos < 0)
						listPtr->startPos = 0;
				}
				return qtrue;
			}
			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) 
			{
				if (!listPtr->notselectable) {
					listPtr->cursorPos++;
					if (listPtr->cursorPos < listPtr->startPos) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= count) {
						listPtr->cursorPos = count-1;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else {
					listPtr->startPos++;
					if (listPtr->startPos >= count)
						listPtr->startPos = count-1;
				}
				return qtrue;
			}
		}
		else {
			viewmax = (item->window.rect.h / listPtr->elementHeight);
			if ( key == K_UPARROW || key == K_KP_UPARROW ) 
			{
				if (!listPtr->notselectable) {
					listPtr->cursorPos--;
					if (listPtr->cursorPos < 0) {
						listPtr->cursorPos = 0;
					}
					if (listPtr->cursorPos < listPtr->startPos) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else {
					listPtr->startPos--;
					if (listPtr->startPos < 0)
						listPtr->startPos = 0;
				}
				return qtrue;
			}
			if ( key == K_DOWNARROW || key == K_KP_DOWNARROW ) 
			{
				if (!listPtr->notselectable) {
					listPtr->cursorPos++;
					if (listPtr->cursorPos < listPtr->startPos) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if (listPtr->cursorPos >= count) {
						listPtr->cursorPos = count-1;
					}
					if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else {
					listPtr->startPos++;
					if (listPtr->startPos > max)
						listPtr->startPos = max;
				}
				return qtrue;
			}
		}
		// mouse hit
		if (key == K_MOUSE1 || key == K_MOUSE2) {
			if (item->window.flags & WINDOW_LB_LEFTARROW) {
				listPtr->startPos--;
				if (listPtr->startPos < 0) {
					listPtr->startPos = 0;
				}
			} else if (item->window.flags & WINDOW_LB_RIGHTARROW) {
				// one down
				listPtr->startPos++;
				if (listPtr->startPos > max) {
					listPtr->startPos = max;
				}
			} else if (item->window.flags & WINDOW_LB_PGUP) {
				// page up
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0) {
					listPtr->startPos = 0;
				}
			} else if (item->window.flags & WINDOW_LB_PGDN) {
				// page down
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max) {
					listPtr->startPos = max;
				}
			} else if (item->window.flags & WINDOW_LB_THUMB) {
				// Display_SetCaptureItem(item);
			} else {
				// select an item
				if (DC->realTime < lastListBoxClickTime && listPtr->doubleClick) {
					Item_RunScript(item, listPtr->doubleClick);
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				if (item->cursorPos != listPtr->cursorPos) {
					item->cursorPos = listPtr->cursorPos;
					listPtr->selectedCursorPos = listPtr->cursorPos;
					LastListSelected = listPtr;
					DC->feederSelection(item->special, item->cursorPos);
				}
				//Com_Printf("selected %p\n", item);
			}
			return qtrue;
		}
		if ( key == K_HOME || key == K_KP_HOME) {
			// home
			listPtr->startPos = 0;
			return qtrue;
		}
		if ( key == K_END || key == K_KP_END) {
			// end
			listPtr->startPos = max;
			return qtrue;
		}
		if (key == K_PGUP || key == K_KP_PGUP ) {
			// page up
			if (!listPtr->notselectable) {
				listPtr->cursorPos -= viewmax;
				if (listPtr->cursorPos < 0) {
					listPtr->cursorPos = 0;
				}
				if (listPtr->cursorPos < listPtr->startPos) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else {
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0) {
					listPtr->startPos = 0;
				}
			}
			return qtrue;
		}
		if ( key == K_PGDN || key == K_KP_PGDN ) {
			// page down
			if (!listPtr->notselectable) {
				listPtr->cursorPos += viewmax;
				if (listPtr->cursorPos < listPtr->startPos) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if (listPtr->cursorPos >= count) {
					listPtr->cursorPos = count-1;
				}
				if (listPtr->cursorPos >= listPtr->startPos + viewmax) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else {
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max) {
					listPtr->startPos = max;
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean Item_YesNo_HandleKey(itemDef_t *item, int key) {
	if (item->cvar) {
		qboolean action = qfalse;

		if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3) {
			if (Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen) && item->window.flags & WINDOW_HASFOCUS) {
				action = qtrue;
			}
		} else if (UI_SelectForKey(key) != 0) {
			action = qtrue;
		}

		if (action) {
			DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
			return qtrue;
		}
	}

	return qfalse;
}

static int Item_Multi_CountSettings(itemDef_t *item) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr == NULL) {
		return 0;
	}
	return multiPtr->count;
}

static int Item_Multi_FindCvarByValue(itemDef_t *item) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr) {
		if (multiPtr->strDef) {
	    DC->getCVarString(item->cvar, buff, sizeof(buff));
		} else {
			value = DC->getCVarValue(item->cvar);
		}
		for (i = 0; i < multiPtr->count; i++) {
			if (multiPtr->strDef) {
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0) {
					return i;
				}
			} else {
 				if (multiPtr->cvarValue[i] == value) {
 					return i;
 				}
 			}
 		}
	}
	return 0;
}

static const char *Item_Multi_Setting(itemDef_t *item) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr) {
		if (multiPtr->strDef) {
	    DC->getCVarString(item->cvar, buff, sizeof(buff));
		} else {
			value = DC->getCVarValue(item->cvar);
		}
		for (i = 0; i < multiPtr->count; i++) {
			if (multiPtr->strDef) {
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0) {
					return multiPtr->cvarList[i];
				}
			} else {
 				if (multiPtr->cvarValue[i] == value) {
					return multiPtr->cvarList[i];
 				}
 			}
 		}
	}
	return "";
}

static qboolean Item_Multi_HandleKey(itemDef_t *item, int key) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr) {
		if (item->cvar) {
			int select = 0;

			if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3) {
				if (Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen) && item->window.flags & WINDOW_HASFOCUS) {
					select = (key == K_MOUSE2) ? -1 : 1;
				}
			} else {
				select = UI_SelectForKey(key);
			}

			if (select != 0) {
				int current = Item_Multi_FindCvarByValue(item) + 1;
				int max = Item_Multi_CountSettings(item);
				if ( current < 0 ) {
					current = max-1;
				} else if ( current >= max ) {
					current = 0;
				}
				if (multiPtr->strDef) {
					DC->setCVar(item->cvar, multiPtr->cvarStr[current]);
				} else {
					float value = multiPtr->cvarValue[current];
					if (((float)((int) value)) == value) {
						DC->setCVar(item->cvar, va("%i", (int) value ));
					}
					else {
						DC->setCVar(item->cvar, va("%f", value ));
					}
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

//FIXME utf8
static qboolean Item_TextField_HandleKey(itemDef_t *item, int key) {
	char buff[1024];
	int len;
	itemDef_t *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	//Com_Printf("item key:  %d '%c'\n", key, key);
	if (item->cvar) {

		memset(buff, 0, sizeof(buff));
		DC->getCVarString(item->cvar, buff, sizeof(buff));
		len = strlen(buff);
		if (editPtr->maxChars && len > editPtr->maxChars) {
			len = editPtr->maxChars;
		}
		if ( key & K_CHAR_FLAG ) {
			key &= ~K_CHAR_FLAG;


			if (key == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
				if ( item->cursorPos > 0 ) {
					memmove( &buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos);
					item->cursorPos--;
					if (item->cursorPos < editPtr->paintOffset) {
						editPtr->paintOffset--;
					}
				}
				DC->setCVar(item->cvar, buff);
	    		return qtrue;
			}


			//
			// ignore any non printable chars
			//
			if ( key < 32 || !item->cvar) {
			    return qtrue;
		    }

			if (item->type == ITEM_TYPE_NUMERICFIELD) {
				if (key < '0' || key > '9') {
					return qfalse;
				}
			}

			if (!DC->getOverstrikeMode()) {
				if (( len == MAX_EDITFIELD - 1 ) || (editPtr->maxChars && len >= editPtr->maxChars)) {
					return qtrue;
				}
				memmove( &buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
			} else {
				if (editPtr->maxChars && item->cursorPos >= editPtr->maxChars) {
					return qtrue;
				}
			}

			buff[item->cursorPos] = key;

			DC->setCVar(item->cvar, buff);

			if (item->cursorPos < len + 1) {
				item->cursorPos++;
				if (editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars) {
					editPtr->paintOffset++;
				}
			}

		} else {

			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					DC->setCVar(item->cvar, buff);
				}
				return qtrue;
			}

			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) 
			{
				if (editPtr->maxPaintChars && item->cursorPos >= editPtr->maxPaintChars && item->cursorPos < len) {
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if (item->cursorPos < len) {
					item->cursorPos++;
				} 
				return qtrue;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) 
			{
				if ( item->cursorPos > 0 ) {
					item->cursorPos--;
				}
				if (item->cursorPos < editPtr->paintOffset) {
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == K_HOME || key == K_KP_HOME) {// || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == K_END || key == K_KP_END)  {// ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if(item->cursorPos > editPtr->maxPaintChars) {
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if ( key == K_INS || key == K_KP_INS ) {
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				return qtrue;
			}
		}

		if (key == K_TAB || key == K_DOWNARROW || key == K_KP_DOWNARROW) {
			newItem = Menu_SetNextCursorItem(item->parent);
			if (newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD)) {
				g_editItem = newItem;
			}
		}

		if (key == K_UPARROW || key == K_KP_UPARROW) {
			newItem = Menu_SetPrevCursorItem(item->parent);
			if (newItem && (newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD)) {
				g_editItem = newItem;
			}
		}

		if ( key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE)  {
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;

}

static void Scroll_ListBox_AutoFunc(void *p) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	if (DC->realTime > si->nextScrollTime) { 
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue; 
	}

	if (DC->realTime > si->nextAdjustTime) {
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR) {
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_ListBox_ThumbFunc(void *p) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	rectDef_t r;
	int pos, max;

	//Com_Printf("listbox thumbfunc...\n");

	listBoxDef_t *listPtr = (listBoxDef_t*)si->item->typeData;
	if (si->item->window.flags & WINDOW_HORIZONTAL) {
		if (CursorX_Widescreen(si->item->widescreen) == si->xStart) {
			return;
		}
		r.x = si->item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = si->item->window.rect.y + si->item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = si->item->window.rect.w - (SCROLLBAR_SIZE*2) - 2;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (CursorX_Widescreen(si->item->widescreen) - r.x - SCROLLBAR_SIZE/2) * max / (r.w - SCROLLBAR_SIZE);
		if (pos < 0) {
			pos = 0;
		}
		else if (pos > max) {
			pos = max;
		}
		listPtr->startPos = pos;
		si->xStart = CursorX_Widescreen(si->item->widescreen);
	}
	else if (DC->cursory != si->yStart) {

		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - (SCROLLBAR_SIZE*2) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursory - r.y - SCROLLBAR_SIZE/2) * max / (r.h - SCROLLBAR_SIZE);
		if (pos < 0) {
			pos = 0;
		}
		else if (pos > max) {
			pos = max;
		}
		listPtr->startPos = pos;
		si->yStart = DC->cursory;
	}

	if (DC->realTime > si->nextScrollTime) { 
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue; 
	}

	if (DC->realTime > si->nextAdjustTime) {
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR) {
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_Slider_ThumbFunc(void *p) {
	float x, value, cursorx;
	scrollInfo_t *si = (scrollInfo_t*)p;
	editFieldDef_t *editDef = si->item->typeData;

	//Com_Printf("slider thumb func\n");

	if (si->item->text) {
		x = si->item->textRect.x + si->item->textRect.w + 8;
	} else {
		x = si->item->window.rect.x;
	}

	cursorx = CursorX_Widescreen(si->item->widescreen);  //DC->cursorx;
	//cursorx = DC->cursorx;

	if (cursorx < x) {
		cursorx = x;
	} else if (cursorx > x + SLIDER_WIDTH) {
		cursorx = x + SLIDER_WIDTH;
	}
	value = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= (editDef->maxVal - editDef->minVal);
	value += editDef->minVal;

	//Com_Printf("^5slider thumbfunc %f\n", value);

	DC->setCVar(si->item->cvar, va("%f", value));
}

static void Item_StartCapture(itemDef_t *item, int key) {
	int flags;
	switch (item->type) {
    case ITEM_TYPE_EDITFIELD:
    case ITEM_TYPE_NUMERICFIELD:

		case ITEM_TYPE_LISTBOX:
		{
			flags = Item_ListBox_OverLB(item, DC->cursorx, DC->cursory);
			if (flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW)) {
				scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
				scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
				scrollInfo.adjustValue = SCROLL_TIME_START;
				scrollInfo.scrollKey = key;
				scrollInfo.scrollDir = (flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_AutoFunc;
				itemCapture = item;
				//Com_Printf("got listbox autofunc %p\n", captureFunc);
			} else if (flags & WINDOW_LB_THUMB) {
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = CursorX_Widescreen(scrollInfo.item->widescreen);  //DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_ThumbFunc;
				itemCapture = item;
				//Com_Printf("got listbox thumbfunc %p\n", captureFunc);
			}
			break;
		}
	    case ITEM_TYPE_SLIDER_COLOR:
			Com_Printf("FIXME Item_StartCapture() ITEM_TYPE_SLIDER_COLOR\n");
		case ITEM_TYPE_SLIDER:
		{
			//Com_Printf("slider start capture\n");
			flags = Item_Slider_OverSlider(item, DC->cursorx, DC->cursory);
			//Com_Printf("flags: %d\n", flags);
			if (flags & WINDOW_LB_THUMB) {
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = CursorX_Widescreen(scrollInfo.item->widescreen);  //DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_Slider_ThumbFunc;
				itemCapture = item;
				//Com_Printf("got slider thumb %p\n", captureFunc);
			}
			break;
		}
	}
}

static void Item_StopCapture(itemDef_t *item) {

}

static qboolean Item_Slider_HandleKey(itemDef_t *item, int key, qboolean down) {
	float x, value, width, work;

	//DC->Print("slider handle key\n");
	if (item->cvar) {
		if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3) {
			editFieldDef_t *editDef = item->typeData;
			if (editDef  &&  Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen)  &&  item->window.flags & WINDOW_HASFOCUS) {
				rectDef_t testRect;
				width = SLIDER_WIDTH;
				if (item->text) {
					x = item->textRect.x + item->textRect.w + 8;
				} else {
					x = item->window.rect.x;
				}

				testRect = item->window.rect;
				testRect.x = x;
				value = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				//DC->Print("slider x: %f\n", testRect.x);
				testRect.w = (SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2);
				//DC->Print("slider w: %f\n", testRect.w);
				if (Rect_ContainsWidescreenPoint(&testRect, DC->cursorx, DC->cursory, item->widescreen)) {
					work = CursorX_Widescreen(item->widescreen) - x;
					//Com_Printf("x: %d  wsx: %f\n", DC->cursorx, CursorX_Widescreen(item->widescreen));
					//Com_Printf("rescaled:  %f\n", CursorX_Widescreen(item->widescreen) / (DC->glconfig.vidWidth / 640.0));
					value = work / width;
					value *= (editDef->maxVal - editDef->minVal);
					// vm fuckage
					// value = (((float)(DC->cursorx - x)/ SLIDER_WIDTH) * (editDef->maxVal - editDef->minVal));
					value += editDef->minVal;
					//Com_Printf("slider setting value %f\n", value);
					DC->setCVar(item->cvar, va("%f", value));
					return qtrue;
				}
			}
		} else {
			int select = UI_SelectForKey(key);
			if (select != 0) {
				editFieldDef_t *editDef = item->typeData;
				if (editDef) {
					// 20 is number of steps
					value = DC->getCVarValue(item->cvar) + (((editDef->maxVal - editDef->minVal)/20) * select);
					if (value < editDef->minVal)
						value = editDef->minVal;
					else if (value > editDef->maxVal)
						value = editDef->maxVal;

					DC->setCVar(item->cvar, va("%f", value));
					return qtrue;
				}
			}
		}
	}

	//DC->Print("slider handle key exit\n");
	return qfalse;
}


static qboolean Item_HandleKey(itemDef_t *item, int key, qboolean down) {

	if (itemCapture) {
		Item_StopCapture(itemCapture);
		itemCapture = NULL;
		captureFunc = 0;
		captureData = NULL;
		//Com_Printf("^2stop capture\n");
	} else {
		if ( down && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			Item_StartCapture(item, key);
		}
	}

	if (!down) {
		return qfalse;
	}

  switch (item->type) {
    case ITEM_TYPE_BUTTON:
      return qfalse;
      break;
    case ITEM_TYPE_RADIOBUTTON:
      return qfalse;
      break;
    case ITEM_TYPE_CHECKBOX:
      return qfalse;
      break;
    case ITEM_TYPE_EDITFIELD:
    case ITEM_TYPE_NUMERICFIELD:
      //return Item_TextField_HandleKey(item, key);
      return qfalse;
      break;
    case ITEM_TYPE_COMBO:
      return qfalse;
      break;
    case ITEM_TYPE_LISTBOX:
      return Item_ListBox_HandleKey(item, key, down, qfalse);
      break;
    case ITEM_TYPE_YESNO:
      return Item_YesNo_HandleKey(item, key);
      break;
    case ITEM_TYPE_MULTI:
      return Item_Multi_HandleKey(item, key);
      break;
    case ITEM_TYPE_OWNERDRAW:
      return Item_OwnerDraw_HandleKey(item, key);
      break;
    case ITEM_TYPE_BIND:
			return Item_Bind_HandleKey(item, key, down);
      break;
    case ITEM_TYPE_SLIDER_COLOR:
		Com_Printf("FIXME Item_HandleKey()  ITEM_TYPE_SLIDER_COLOR\n");
    case ITEM_TYPE_SLIDER:
      return Item_Slider_HandleKey(item, key, down);
      break;
    //case ITEM_TYPE_IMAGE:
    //  Item_Image_Paint(item);
    //  break;
    default:
      return qfalse;
      break;
  }

  //return qfalse;
}

static void Item_Action(itemDef_t *item) {
  if (item) {
    Item_RunScript(item, item->action);
  }
}

static itemDef_t *Menu_SetPrevCursorItem(menuDef_t *menu) {
  qboolean wrapped = qfalse;
	int oldCursor = menu->cursorItem;
  
  if (menu->cursorItem < 0) {
    menu->cursorItem = menu->itemCount-1;
    wrapped = qtrue;
  } 

  while (menu->cursorItem > -1) {
    
    menu->cursorItem--;
    if (menu->cursorItem < 0 && !wrapped) {
      wrapped = qtrue;
      menu->cursorItem = menu->itemCount -1;
    }

		if (Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory)) {
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1);
      return menu->items[menu->cursorItem];
    }
  }
	menu->cursorItem = oldCursor;
	return NULL;

}

static itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu) {

  qboolean wrapped = qfalse;
	int oldCursor = menu->cursorItem;


  if (menu->cursorItem == -1) {
    menu->cursorItem = 0;
    wrapped = qtrue;
  }

  while (menu->cursorItem < menu->itemCount) {

    menu->cursorItem++;
    if (menu->cursorItem >= menu->itemCount && !wrapped) {
      wrapped = qtrue;
      menu->cursorItem = 0;
    }
		if (Item_SetFocus(menu->items[menu->cursorItem], DC->cursorx, DC->cursory)) {
			Menu_HandleMouseMove(menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1);
      return menu->items[menu->cursorItem];
    }
    
  }

	menu->cursorItem = oldCursor;
	return NULL;
}

static void Window_CloseCinematic(windowDef_t *window) {
	if (window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0) {
		DC->stopCinematic(window->cinematic);
		window->cinematic = -1;
	}
}

static void Menu_CloseCinematics(menuDef_t *menu) {
	if (menu) {
		int i;
		Window_CloseCinematic(&menu->window);
	  for (i = 0; i < menu->itemCount; i++) {
		  Window_CloseCinematic(&menu->items[i]->window);
			if (menu->items[i]->type == ITEM_TYPE_OWNERDRAW) {
				DC->stopCinematic(0-menu->items[i]->window.ownerDraw);
			}
	  }
	}
}

static void Display_CloseCinematics( void ) {
	int i;
	for (i = 0; i < menuCount; i++) {
		Menu_CloseCinematics(&Menus[i]);
	}
}

void  Menus_Activate(menuDef_t *menu) {
	menu->window.flags |= (WINDOW_HASFOCUS | WINDOW_VISIBLE);
	if (menu->onOpen) {
		itemDef_t item;
		item.parent = menu;
		//Com_Printf("^5onOpen\n");
		Item_RunScript(&item, menu->onOpen);
	}

	if (menu->soundName && *menu->soundName) {
//		DC->stopBackgroundTrack();					// you don't want to do this since it will reset s_rawend
		DC->startBackgroundTrack(menu->soundName, menu->soundName);
	}

	Display_CloseCinematics();

}

static int Display_VisibleMenuCount( void ) {
	int i, count;
	count = 0;
	for (i = 0; i < menuCount; i++) {
		if (Menus[i].window.flags & (WINDOW_FORCED | WINDOW_VISIBLE)) {
			count++;
		}
	}
	return count;
}

static void Menus_HandleOOBClick(menuDef_t *menu, int key, qboolean down) {
	if (menu) {
		int i;
		// basically the behaviour we are looking for is if there are windows in the stack.. see if 
		// the cursor is within any of them.. if not close them otherwise activate them and pass the 
		// key on.. force a mouse move to activate focus and script stuff 
		if (down && menu->window.flags & WINDOW_OOB_CLICK) {
			Menu_RunCloseScript(menu);
			menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
		}

		for (i = 0; i < menuCount; i++) {
			if (Menu_OverActiveItem(&Menus[i], DC->cursorx, DC->cursory)) {
				Menu_RunCloseScript(menu);
				menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
				Menus_Activate(&Menus[i]);
				Menu_HandleMouseMove(&Menus[i], DC->cursorx, DC->cursory);
				Menu_HandleKey(&Menus[i], key, down);
			}
		}

		if (Display_VisibleMenuCount() == 0) {
			if (DC->Pause) {
				DC->Pause(qfalse);
			}
		}
		Display_CloseCinematics();
	}
}

static rectDef_t *Item_CorrectedTextRect(itemDef_t *item) {
	static rectDef_t rect;
	memset(&rect, 0, sizeof(rectDef_t));
	if (item) {
		rect = item->textRect;
		if (rect.w) {
			rect.y -= rect.h;
		}
	}
	return &rect;
}

// menu item key horizontal action: -1 = previous value, 1 = next value, 0 = no change
int UI_SelectForKey(int key)
{
	switch (key) {
	case K_MOUSE1:
	case K_MOUSE3:
	case K_ENTER:
	case K_KP_ENTER:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
		return 1; // next
	case K_MOUSE2:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		return -1; // previous
	}

	// no change
	return 0;
}


//FIXME utf8
void Menu_HandleKey(menuDef_t *menu, int key, qboolean down) {
	//int i;
	itemDef_t *item = NULL;
	static qboolean inHandler = qfalse;

#if 0
	if (down) {
		Com_Printf("Menu_HandleKey() %p 0x%x %d\n", menu, key, down);
	}
#endif

	if (inHandler) {
		return;
	}

	inHandler = qtrue;
	if (g_waitingForKey && down) {
		Item_Bind_HandleKey(g_bindItem, key, down);
		inHandler = qfalse;
		return;
	}

	if (g_editingField && down) {
		if (!Item_TextField_HandleKey(g_editItem, key)) {
			g_editingField = qfalse;
			g_editItem = NULL;
			inHandler = qfalse;
			return;
		} else if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3) {
			g_editingField = qfalse;
			g_editItem = NULL;
			Display_MouseMove(NULL, DC->cursorx, DC->cursory);
		} else if (key == K_TAB || key == K_UPARROW || key == K_DOWNARROW) {
			return;
		}
	}

	if (menu == NULL) {
		inHandler = qfalse;
		return;
	}

	// see if the mouse is within the window bounds and if so is this a mouse click
	if (down && !(menu->window.flags & WINDOW_POPUP) && !Rect_ContainsWidescreenPoint(&menu->window.rect, DC->cursorx, DC->cursory, menu->widescreen)) {
		static qboolean inHandleKey = qfalse;
		if (!inHandleKey && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			inHandleKey = qtrue;
			Menus_HandleOOBClick(menu, key, down);
			inHandleKey = qfalse;
			inHandler = qfalse;
			return;
		}
	}

#if 0
	// get the item with focus
	for (i = 0; i < menu->itemCount; i++) {
		if (menu->items[i]->window.flags & WINDOW_HASFOCUS) {
			item = menu->items[i];
		}
	}
#endif

	//FIXME hack
	if (LastFocusItem  &&  LastFocusItem->window.flags & WINDOW_HASFOCUS) {
		item = LastFocusItem;
	}

	//Com_Printf("item: %p  count == %d\n", item, menu->itemCount);

	if (item != NULL) {
		if (Item_HandleKey(item, key, down)) {
			Item_Action(item);
			inHandler = qfalse;
			return;
		}
	}

	if (!down) {
		inHandler = qfalse;
		return;
	}

	//Com_Printf("default handler item: %p\n", item);

	// default handling

	//FIXME
	//inHandler = qfalse;
	//return;

	switch ( key ) {
#if 0  //FIXME
		case K_F11:
			if (DC->getCVarValue("developer")) {
				debugMode ^= 1;
			}
			break;

		case K_F12:
			if (DC->getCVarValue("developer")) {
				DC->executeText(EXEC_APPEND, "screenshot\n");
			}
			break;
		case K_KP_UPARROW:
		case K_UPARROW:
			Menu_SetPrevCursorItem(menu);
			break;

		case K_ESCAPE:
			if (!g_waitingForKey && menu->onESC) {
				itemDef_t it;
		    it.parent = menu;
		    Item_RunScript(&it, menu->onESC);
			}
			break;
		case K_TAB:
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			Menu_SetNextCursorItem(menu);
			break;
#endif
		case K_MOUSE1:
		case K_MOUSE2:
			if (item) {
				if (item->type == ITEM_TYPE_TEXT) {
					if (Rect_ContainsWidescreenPoint(Item_CorrectedTextRect(item), DC->cursorx, DC->cursory, item->widescreen)) {
						Item_Action(item);
					}
				} else if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD) {
					if (Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen)) {
						item->cursorPos = 0;
						g_editingField = qtrue;
						g_editItem = item;
						DC->setOverstrikeMode(qtrue);
					}
				} else {
					if (Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen)) {
						Item_Action(item);
					}
				}
			}
			break;
#if 0  //FIXME
		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX12:
		case K_AUX13:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
		case K_KP_ENTER:
		case K_ENTER:
			if (item) {
				if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD) {
					item->cursorPos = 0;
					g_editingField = qtrue;
					g_editItem = item;
					DC->setOverstrikeMode(qtrue);
				} else {
						Item_Action(item);
				}
			}
			break;
#endif
	}
	inHandler = qfalse;
}

static void ToWindowCoords(float *x, float *y, windowDef_t *window) {
	if (window->border != 0) {
		*x += window->borderSize;
		*y += window->borderSize;
	} 
	*x += window->rect.x;
	*y += window->rect.y;
}

// unused
#if 0
static void Rect_ToWindowCoords(rectDef_t *rect, windowDef_t *window) {
	ToWindowCoords(&rect->x, &rect->y, window);
}
#endif

static void Item_SetTextExtents(itemDef_t *item, float *width, float *height, const char *text) {
	const char *textPtr = (text) ? text : item->text;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}

	if (textPtr == NULL ) {
		return;
	}

	*width = item->textRect.w;
	*height = item->textRect.h;

	// keeps us from computing the widths and heights more than once
	if (*width == 0 || (item->type == ITEM_TYPE_OWNERDRAW && item->textalignment == ITEM_ALIGN_CENTER)) {
		//FIXME widescreen?
		float originalWidth = DC->textWidth(item->text, item->textscale, 0, item->fontIndex, item->widescreen, menuRect);

		if (item->type == ITEM_TYPE_OWNERDRAW && (item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_RIGHT)) {
			originalWidth += DC->ownerDrawWidth(item->window.ownerDraw, item->textscale, item->fontIndex, item->widescreen, menuRect);
		} else if (item->type == ITEM_TYPE_EDITFIELD && item->textalignment == ITEM_ALIGN_CENTER && item->cvar) {
			char buff[256];
			DC->getCVarString(item->cvar, buff, 256);
			originalWidth += DC->textWidth(buff, item->textscale, 0, item->fontIndex, item->widescreen, menuRect);
		}

		*width = DC->textWidth(textPtr, item->textscale, 0, item->fontIndex, item->widescreen, menuRect);
		*height = DC->textHeight(textPtr, item->textscale, 0, item->fontIndex, item->widescreen, menuRect);
		item->textRect.w = *width;
		item->textRect.h = *height;
		item->textRect.x = item->textalignx;
		item->textRect.y = item->textaligny;
		if (item->textalignment == ITEM_ALIGN_RIGHT) {
			item->textRect.x = item->textalignx - originalWidth;
		} else if (item->textalignment == ITEM_ALIGN_CENTER) {
			item->textRect.x = item->textalignx - originalWidth / 2;
		}

		ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
	}
}

static void Item_TextColor(itemDef_t *item, vec4_t *newColor) {
	vec4_t lowLight;
	menuDef_t *parent = (menuDef_t*)item->parent;

	Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);

	if (item->window.flags & WINDOW_HASFOCUS) {
		lowLight[0] = 0.8 * parent->focusColor[0]; 
		lowLight[1] = 0.8 * parent->focusColor[1]; 
		lowLight[2] = 0.8 * parent->focusColor[2]; 
		lowLight[3] = 0.8 * parent->focusColor[3]; 
		LerpColor(parent->focusColor,lowLight,*newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime/BLINK_DIVISOR) & 1)) {
		lowLight[0] = 0.8 * item->window.foreColor[0]; 
		lowLight[1] = 0.8 * item->window.foreColor[1]; 
		lowLight[2] = 0.8 * item->window.foreColor[2]; 
		lowLight[3] = 0.8 * item->window.foreColor[3]; 
		LerpColor(item->window.foreColor,lowLight,*newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(newColor, &item->window.foreColor, sizeof(vec4_t));
		// items can be enabled and disabled based on cvars
	}

	if (item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest) {
		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) {
			memcpy(newColor, &parent->disableColor, sizeof(vec4_t));
		}
	}
}

static void Item_Text_AutoWrapped_Paint (itemDef_t *item)
{
	char text[1024];
	const char *p, *textPtr, *newLinePtr;
	char buff[1024];
	int len, newLine;
	float width, height, newLineWidth;
	float textWidth;
	float y;
	vec4_t color;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}

	textWidth = 0;
	newLinePtr = NULL;

	if (item->text == NULL) {
		if (item->cvar == NULL) {
			return;
		}
		else {
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else {
		textPtr = item->text;
	}
	if (*textPtr == '\0') {
		return;
	}
	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	y = item->textaligny;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;
	while (p) {
		int numUtf8Bytes;
		qboolean error;
		int n;

		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0') {
			newLine = len;
			newLinePtr = p+1;
			newLineWidth = textWidth;
		}
		textWidth = DC->textWidth(buff, item->textscale, 0, item->fontIndex, item->widescreen, menuRect);
		if ( (newLine && textWidth > item->window.rect.w) || *p == '\n' || *p == '\0') {
			if (len) {
				if (item->textalignment == ITEM_ALIGN_LEFT) {
					item->textRect.x = item->textalignx;
				} else if (item->textalignment == ITEM_ALIGN_RIGHT) {
					item->textRect.x = item->textalignx - newLineWidth;
				} else if (item->textalignment == ITEM_ALIGN_CENTER) {
					item->textRect.x = item->textalignx - newLineWidth / 2;
				}
				item->textRect.y = y;
				ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
				//
				buff[newLine] = '\0';
				DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, buff, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
			}
			if (*p == '\0') {
				break;
			}
			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		Q_GetCpFromUtf8(p, &numUtf8Bytes, &error);
		for (n = 0;  n < numUtf8Bytes;  n++) {
			buff[len + n] = p[n];
		}
		len += numUtf8Bytes;
		p += numUtf8Bytes;
		buff[len] = '\0';
	}
}

static void Item_Text_Wrapped_Paint(itemDef_t *item) {
	char text[1024];
	const char *p, *start, *textPtr;
	char buff[1024];
	float width, height;
	float x, y;
	vec4_t color;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	// now paint the text and/or any optional images
	// default to left

	if (item->text == NULL) {
		if (item->cvar == NULL) {
			return;
		}
		else {
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else {
		textPtr = item->text;
	}
	if (*textPtr == '\0') {
		return;
	}

	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	x = item->textRect.x;
	y = item->textRect.y;
	start = textPtr;
	p = strchr(textPtr, '\r');
	while (p && *p) {
		strncpy(buff, start, p-start+1);
		buff[p-start] = '\0';
		DC->drawText(x, y, item->textscale, color, buff, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
		y += height + 5;
		start += p - start + 1;
		p = strchr(p+1, '\r');
	}
	DC->drawText(x, y, item->textscale, color, start, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
}

static void Item_Text_Paint(itemDef_t *item) {
	char text[1024];
	const char *textPtr;
	float height, width;
	vec4_t color;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	if (item->window.flags & WINDOW_WRAPPED) {
		Item_Text_Wrapped_Paint(item);
		return;
	}
	if (item->window.flags & WINDOW_AUTOWRAPPED) {
		Item_Text_AutoWrapped_Paint(item);
		return;
	}

	if (item->text == NULL) {
		if (item->cvar == NULL) {
			return;
		}
		else {
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else {
		textPtr = item->text;
	}

	// this needs to go here as it sets extents for cvar types as well
	Item_SetTextExtents(item, &width, &height, textPtr);

	if (*textPtr == '\0') {
		return;
	}


	Item_TextColor(item, &color);

	//FIXME: this is a fucking mess
/*
	adjust = 0;
	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		adjust = 0.5;
	}

	if (item->textStyle == ITEM_TEXTSTYLE_SHADOWED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		Fade(&item->window.flags, &DC->Assets.shadowColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
		DC->drawText(item->textRect.x + DC->Assets.shadowX, item->textRect.y + DC->Assets.shadowY, item->textscale, DC->Assets.shadowColor, textPtr, adjust, item->fontIndex, item->widescreen);
	}
*/


//	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
//		Fade(&item->window.flags, &item->window.outlineColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
//		/*
//		Text_Paint(item->textRect.x-1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x-1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x-1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		*/
//		DC->drawText(item->textRect.x - 1, item->textRect.y + 1, item->textscale * 1.02, item->window.outlineColor, textPtr, adjust, item->fontIndex);
//	}

	DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, textPtr, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
}



//float			trap_Cvar_VariableValue( const char *var_name );
//void			trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

static void Item_TextField_Paint(itemDef_t *item) {
	char buff[1024];
	vec4_t newColor, lowLight;
	int offset;
	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	Item_Text_Paint(item);

	buff[0] = '\0';

	if (item->cvar) {
		DC->getCVarString(item->cvar, buff, sizeof(buff));
	} 

	parent = (menuDef_t*)item->parent;

	if (item->window.flags & WINDOW_HASFOCUS) {
		lowLight[0] = 0.8 * parent->focusColor[0]; 
		lowLight[1] = 0.8 * parent->focusColor[1]; 
		lowLight[2] = 0.8 * parent->focusColor[2]; 
		lowLight[3] = 0.8 * parent->focusColor[3]; 
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	offset = (item->text && *item->text) ? 8 : 0;
	if (item->window.flags & WINDOW_HASFOCUS && g_editingField) {
		char cursor = DC->getOverstrikeMode() ? '_' : '|';
		DC->drawTextWithCursor(item->textRect.x + item->textRect.w + offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset, item->cursorPos - editPtr->paintOffset , cursor, editPtr->maxPaintChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	} else {
		DC->drawText(item->textRect.x + item->textRect.w + offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset, 0, editPtr->maxPaintChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	}

}

static void Item_YesNo_Paint(itemDef_t *item) {
	vec4_t newColor, lowLight;
	float value;
	menuDef_t *parent = (menuDef_t*)item->parent;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if (item->window.flags & WINDOW_HASFOCUS) {
		lowLight[0] = 0.8 * parent->focusColor[0]; 
		lowLight[1] = 0.8 * parent->focusColor[1]; 
		lowLight[2] = 0.8 * parent->focusColor[2]; 
		lowLight[3] = 0.8 * parent->focusColor[3]; 
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if (item->text) {
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, (value != 0) ? "Yes" : "No", 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	} else {
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, (value != 0) ? "Yes" : "No", 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	}
}

static void Item_Multi_Paint(itemDef_t *item) {
	vec4_t newColor, lowLight;
	const char *text = "";
	menuDef_t *parent = (menuDef_t*)item->parent;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	if (item->window.flags & WINDOW_HASFOCUS) {
		lowLight[0] = 0.8 * parent->focusColor[0]; 
		lowLight[1] = 0.8 * parent->focusColor[1]; 
		lowLight[2] = 0.8 * parent->focusColor[2]; 
		lowLight[3] = 0.8 * parent->focusColor[3]; 
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	text = Item_Multi_Setting(item);

	if (item->text) {
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	} else {
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	}
}


typedef struct {
	char	*command;
	int		defaultbind1;
	int		defaultbind2;
	int		bind1;
	int		bind2;
} bind_t;

typedef struct
{
	char*	name;
	float	defaultvalue;
	float	value;	
} configcvar_t;


static bind_t g_bindings[] = 
{
	{"+scores",			 K_TAB,				-1,		-1, -1},
	{"+button2",		 K_ENTER,			-1,		-1, -1},
	{"+speed", 			 K_SHIFT,			-1,		-1,	-1},
	{"+forward", 		 K_UPARROW,		-1,		-1, -1},
	{"+back", 			 K_DOWNARROW,	-1,		-1, -1},
	{"+moveleft", 	 ',',					-1,		-1, -1},
	{"+moveright", 	 '.',					-1,		-1, -1},
	{"+moveup",			 K_SPACE,			-1,		-1, -1},
	{"+movedown",		 'c',					-1,		-1, -1},
	{"+left", 			 K_LEFTARROW,	-1,		-1, -1},
	{"+right", 			 K_RIGHTARROW,	-1,		-1, -1},
	{"+strafe", 		 K_ALT,				-1,		-1, -1},
	{"+lookup", 		 K_PGDN,				-1,		-1, -1},
	{"+lookdown", 	 K_DEL,				-1,		-1, -1},
	{"+mlook", 			 '/',					-1,		-1, -1},
	{"centerview", 	 K_END,				-1,		-1, -1},
	{"+zoom", 			 -1,						-1,		-1, -1},
	{"weapon 1",		 '1',					-1,		-1, -1},
	{"weapon 2",		 '2',					-1,		-1, -1},
	{"weapon 3",		 '3',					-1,		-1, -1},
	{"weapon 4",		 '4',					-1,		-1, -1},
	{"weapon 5",		 '5',					-1,		-1, -1},
	{"weapon 6",		 '6',					-1,		-1, -1},
	{"weapon 7",		 '7',					-1,		-1, -1},
	{"weapon 8",		 '8',					-1,		-1, -1},
	{"weapon 9",		 '9',					-1,		-1, -1},
	{"weapon 10",		 '0',					-1,		-1, -1},
	{"weapon 11",		 -1,					-1,		-1, -1},
	{"weapon 12",		 -1,					-1,		-1, -1},
	{"weapon 13",		 -1,					-1,		-1, -1},
	{"+attack", 		 K_CTRL,				-1,		-1, -1},
	{"weapprev",		 '[',					-1,		-1, -1},
	{"weapnext", 		 ']',					-1,		-1, -1},
	{"+button3", 		 K_MOUSE3,			-1,		-1, -1},
	{"+button4", 		 K_MOUSE4,			-1,		-1, -1},
	{"prevTeamMember", 'w',					-1,		-1, -1},
	{"nextTeamMember", 'r',					-1,		-1, -1},
	{"nextOrder", 't',					-1,		-1, -1},
	{"confirmOrder", 'y',					-1,		-1, -1},
	{"denyOrder", 'n',					-1,		-1, -1},
	{"taskOffense", 'o',					-1,		-1, -1},
	{"taskDefense", 'd',					-1,		-1, -1},
	{"taskPatrol", 'p',					-1,		-1, -1},
	{"taskCamp", 'c',					-1,		-1, -1},
	{"taskFollow", 'f',					-1,		-1, -1},
	{"taskRetrieve", 'v',					-1,		-1, -1},
	{"taskEscort", 'e',					-1,		-1, -1},
	{"taskOwnFlag", 'i',					-1,		-1, -1},
	{"taskSuicide", 'k',					-1,		-1, -1},
	{"tauntKillInsult", K_F1,			-1,		-1, -1},
	{"tauntPraise", K_F2,			-1,		-1, -1},
	{"tauntTaunt", K_F3,			-1,		-1, -1},
	{"tauntDeathInsult", K_F4,			-1,		-1, -1},
	{"tauntGauntlet", K_F5,			-1,		-1, -1},
	{"scoresUp", K_KP_PGUP,			-1,		-1, -1},
	{"scoresDown", K_KP_PGDN,			-1,		-1, -1},
	{"messagemode",  -1,					-1,		-1, -1},
	{"messagemode2", -1,						-1,		-1, -1},
	{"messagemode3", -1,						-1,		-1, -1},
	{"messagemode4", -1,						-1,		-1, -1},
#if 0  // doesn't work, uses q3default.cfg
	{ "pause", K_PAUSE, -1, -1, -1 },
	{ "freecam", K_KP_ENTER, -1, -1, -1 },
	{ "rewind 10", K_KP_LEFTARROW, -1, -1, -1 },
	{ "fastforward 10", K_KP_RIGHTARROW, -1, -1, -1 },
	{ "fastforward 60", K_KP_UPARROW, -1, -1, -1 },
	{ "rewind 60", K_KP_DOWNARROW, -1, -1, -1 },
#endif
};


static const int g_bindCount = ARRAY_LEN(g_bindings);

#ifndef MISSIONPACK
static configcvar_t g_configcvars[] =
{
	{"cl_run",			0,					0},
	{"m_pitch",			0,					0},
	{"cg_autoswitch",	0,					0},
	{"sensitivity",		0,					0},
	{"in_joystick",		0,					0},
	{"joy_threshold",	0,					0},
	{"m_filter",		0,					0},
	{"cl_freelook",		0,					0},
	{NULL,				0,					0}
};
#endif

/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment (char *command, int *twokeys)
{
	int		count;
	int		j;
	char	b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		DC->getBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if (count == 2) {
				break;
			}
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
void Controls_GetConfig( void )
{
	int		i;
	int		twokeys[2];

	// iterate each command, get its numeric binding
	for (i=0; i < g_bindCount; i++)
	{

		Controls_GetKeyAssignment(g_bindings[i].command, twokeys);

		g_bindings[i].bind1 = twokeys[0];
		g_bindings[i].bind2 = twokeys[1];
	}

	//s_controls.invertmouse.curvalue  = DC->getCVarValue( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	//s_controls.alwaysrun.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_run" ) );
	//s_controls.autoswitch.curvalue   = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cg_autoswitch" ) );
	//s_controls.sensitivity.curvalue  = UI_ClampCvar( 2, 30, Controls_GetCvarValue( "sensitivity" ) );
	//s_controls.joyenable.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "in_joystick" ) );
	//s_controls.joythreshold.curvalue = UI_ClampCvar( 0.05, 0.75, Controls_GetCvarValue( "joy_threshold" ) );
	//s_controls.freelook.curvalue     = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_freelook" ) );
}

/*
=================
Controls_SetConfig
=================
*/
void Controls_SetConfig(qboolean restart)
{
	int		i;

	// iterate each command, get its numeric binding
	for (i=0; i < g_bindCount; i++)
	{

		if (g_bindings[i].bind1 != -1)
		{	
			DC->setBinding( g_bindings[i].bind1, g_bindings[i].command );

			if (g_bindings[i].bind2 != -1)
				DC->setBinding( g_bindings[i].bind2, g_bindings[i].command );
		}
	}

	//if ( s_controls.invertmouse.curvalue )
	//	DC->setCVar("m_pitch", va("%f),-fabs( DC->getCVarValue( "m_pitch" ) ) );
	//else
	//	trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );

	//trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	//trap_Cvar_SetValue( "cl_run", s_controls.alwaysrun.curvalue );
	//trap_Cvar_SetValue( "cg_autoswitch", s_controls.autoswitch.curvalue );
	//trap_Cvar_SetValue( "sensitivity", s_controls.sensitivity.curvalue );
	//trap_Cvar_SetValue( "in_joystick", s_controls.joyenable.curvalue );
	//trap_Cvar_SetValue( "joy_threshold", s_controls.joythreshold.curvalue );
	//trap_Cvar_SetValue( "cl_freelook", s_controls.freelook.curvalue );
	DC->executeText(EXEC_APPEND, "in_restart\n");
	//trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
void Controls_SetDefaults( void )
{
	int	i;

	// iterate each command, set its default binding
  for (i=0; i < g_bindCount; i++)
	{
		g_bindings[i].bind1 = g_bindings[i].defaultbind1;
		g_bindings[i].bind2 = g_bindings[i].defaultbind2;
	}

	//s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	//s_controls.alwaysrun.curvalue    = Controls_GetCvarDefault( "cl_run" );
	//s_controls.autoswitch.curvalue   = Controls_GetCvarDefault( "cg_autoswitch" );
	//s_controls.sensitivity.curvalue  = Controls_GetCvarDefault( "sensitivity" );
	//s_controls.joyenable.curvalue    = Controls_GetCvarDefault( "in_joystick" );
	//s_controls.joythreshold.curvalue = Controls_GetCvarDefault( "joy_threshold" );
	//s_controls.freelook.curvalue     = Controls_GetCvarDefault( "cl_freelook" );
}

static int BindingIDFromName(const char *name) {
	int i;
  for (i=0; i < g_bindCount; i++)
	{
		if (Q_stricmp(name, g_bindings[i].command) == 0) {
			return i;
		}
	}
	return -1;
}

char g_nameBind1[32];
char g_nameBind2[32];

static void BindingFromName(const char *cvar) {
	int	i, b1, b2;

	// iterate each command, set its default binding
	for (i=0; i < g_bindCount; i++)
	{
		if (Q_stricmp(cvar, g_bindings[i].command) == 0) {
			b1 = g_bindings[i].bind1;
			if (b1 == -1) {
				break;
			}
				DC->keynumToStringBuf( b1, g_nameBind1, 32 );
				Q_strupr(g_nameBind1);

				b2 = g_bindings[i].bind2;
				if (b2 != -1)
				{
					DC->keynumToStringBuf( b2, g_nameBind2, 32 );
					Q_strupr(g_nameBind2);
					strcat( g_nameBind1, " or " );
					strcat( g_nameBind1, g_nameBind2 );
				}
			return;
		}
	}
	strcpy(g_nameBind1, "???");
}

static void Item_Slider_Paint(itemDef_t *item) {
	vec4_t newColor, lowLight;
	float x, y;  //, value;
	menuDef_t *parent = (menuDef_t*)item->parent;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	//value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if (item->window.flags & WINDOW_HASFOCUS) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	y = item->window.rect.y;
	if (item->text) {
		Item_Text_Paint(item);
		x = item->textRect.x + item->textRect.w + 8;
	} else {
		x = item->window.rect.x;
	}
	DC->setColor(newColor);
	DC->drawHandlePic(x, y, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar, item->widescreen, menuRect);

	x = Item_Slider_ThumbPosition(item);
	DC->drawHandlePic(x - (SLIDER_THUMB_WIDTH / 2), y - 2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb, item->widescreen, menuRect);

}

static void Item_Bind_Paint(itemDef_t *item) {
	vec4_t newColor, lowLight;
	float value;
	int maxChars = 0;
	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;
	rectDef_t menuRect;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	if (editPtr) {
		maxChars = editPtr->maxPaintChars;
	}

	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if (item->window.flags & WINDOW_HASFOCUS) {
		if (g_bindItem == item) {
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		} else {
			lowLight[0] = 0.8f * parent->focusColor[0]; 
			lowLight[1] = 0.8f * parent->focusColor[1]; 
			lowLight[2] = 0.8f * parent->focusColor[2]; 
			lowLight[3] = 0.8f * parent->focusColor[3]; 
		}
		LerpColor(parent->focusColor,lowLight,newColor,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
	} else {
		memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if (item->text) {
		Item_Text_Paint(item);
		BindingFromName(item->cvar);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, g_nameBind1, 0, maxChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	} else {
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, (value != 0) ? "FIXME" : "FIXME", 0, maxChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
	}
}

qboolean Display_KeyBindPending(void) {
	return g_waitingForKey;
}

static qboolean Item_Bind_HandleKey(itemDef_t *item, int key, qboolean down) {
	int			id;
	int			i;

	if (!g_waitingForKey)
	{
		if (down &&  ((key == K_MOUSE1 &&  Rect_ContainsWidescreenPoint(&item->window.rect, DC->cursorx, DC->cursory, item->widescreen))
					  || key == K_ENTER || key == K_KP_ENTER || key == K_JOY1 || key == K_JOY2 || key == K_JOY3 || key == K_JOY4)) {
			g_waitingForKey = qtrue;
			g_bindItem = item;
		}
		return qtrue;
	}
	else
	{
		if (g_bindItem == NULL) {
			return qtrue;
		}

		if (key & K_CHAR_FLAG) {
			return qtrue;
		}

		switch (key)
		{
			case K_ESCAPE:
				g_waitingForKey = qfalse;
				return qtrue;
	
			case K_BACKSPACE:
				id = BindingIDFromName(item->cvar);
				if (id != -1) {
					g_bindings[id].bind1 = -1;
					g_bindings[id].bind2 = -1;
				}
				Controls_SetConfig(qtrue);
				g_waitingForKey = qfalse;
				g_bindItem = NULL;
				return qtrue;

			case '`':
				return qtrue;
		}
	}

	if (key != -1)
	{

		for (i=0; i < g_bindCount; i++)
		{

			if (g_bindings[i].bind2 == key) {
				g_bindings[i].bind2 = -1;
			}

			if (g_bindings[i].bind1 == key)
			{
				g_bindings[i].bind1 = g_bindings[i].bind2;
				g_bindings[i].bind2 = -1;
			}
		}
	}


	id = BindingIDFromName(item->cvar);

	if (id != -1) {
		if (key == -1) {
			if( g_bindings[id].bind1 != -1 ) {
				DC->setBinding( g_bindings[id].bind1, "" );
				g_bindings[id].bind1 = -1;
			}
			if( g_bindings[id].bind2 != -1 ) {
				DC->setBinding( g_bindings[id].bind2, "" );
				g_bindings[id].bind2 = -1;
			}
		}
		else if (g_bindings[id].bind1 == -1) {
			g_bindings[id].bind1 = key;
		}
		else if (g_bindings[id].bind1 != key && g_bindings[id].bind2 == -1) {
			g_bindings[id].bind2 = key;
		}
		else {
			DC->setBinding( g_bindings[id].bind1, "" );
			DC->setBinding( g_bindings[id].bind2, "" );
			g_bindings[id].bind1 = key;
			g_bindings[id].bind2 = -1;
		}						
	}

	Controls_SetConfig(qtrue);	
	g_waitingForKey = qfalse;

	return qtrue;
}



static void AdjustFrom640(float *x, float *y, float *w, float *h) {
	//FIXME widescreen
	
	//*x = *x * DC->scale + DC->bias;
	*x *= DC->xscale;
	*y *= DC->yscale;
	*w *= DC->xscale;
	*h *= DC->yscale;
}

//FIXME widescreen
static void Item_Model_Paint(itemDef_t *item) {
	float x, y, w, h;
	refdef_t refdef;
	refEntity_t		ent;
	vec3_t			mins, maxs, origin;
	vec3_t			angles;
	modelDef_t *modelPtr = (modelDef_t*)item->typeData;

	if (modelPtr == NULL) {
		return;
	}

	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	x = item->window.rect.x+1;
	y = item->window.rect.y+1;
	w = item->window.rect.w-2;
	h = item->window.rect.h-2;

	AdjustFrom640( &x, &y, &w, &h );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	DC->modelBounds( item->asset, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the model nearly fills the box
	if (qtrue) {
		float len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )
		//origin[0] = len / tan(w/2);
	} else {
		origin[0] = item->textscale;
	}
	refdef.fov_x = (modelPtr->fov_x) ? modelPtr->fov_x : w;
	refdef.fov_y = (modelPtr->fov_y) ? modelPtr->fov_y : h;

	//refdef.fov_x = (int)((float)refdef.width / 640.0f * 90.0f);
	//xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	//refdef.fov_y = atan2( refdef.height, xx );
	//refdef.fov_y *= ( 360 / M_PI );

	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof(ent) );

	//adjust = 5.0 * sin( (float)uis.realtime / 500 );
	//adjust = 360 % (int)((float)uis.realtime / 1000);
	//VectorSet( angles, 0, 0, 1 );

	// use item storage to track
	if (modelPtr->rotationSpeed) {
		if (DC->realTime > item->window.nextTime) {
			item->window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle = (int)(modelPtr->angle + 1) % 360;
		}
	}
	VectorSet( angles, 0, modelPtr->angle, 0 );
	AnglesToAxis( angles, ent.axis );

	ent.hModel = item->asset;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );

}

// unused
#if 0
static void Item_Image_Paint(itemDef_t *item) {
	rectDef_t menuRect;

	if (item == NULL) {
		return;
	}

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}
	DC->drawHandlePic(item->window.rect.x+1, item->window.rect.y+1, item->window.rect.w-2, item->window.rect.h-2, item->asset, item->widescreen, menuRect);
	//Com_Printf("item image paint\n");
}
#endif

void Item_ListBox_Paint(itemDef_t *item) {
	float x, y, size, count, i, thumb;
	qhandle_t image;
	qhandle_t optionalImage;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	rectDef_t menuRect;
	float elementHeight;

	if (item->parent) {
		menuRect = ((menuDef_t *)item->parent)->window.rect;
	}

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount(item->special);

	// hack to dynamically change scoreboard line height
	elementHeight = listPtr->elementHeight;

	if (item->special == FEEDER_REDTEAM_LIST  ||  item->special == FEEDER_BLUETEAM_LIST) {
		float forceLineHeight;
		int defaultCount;

		forceLineHeight = DC->getCVarValue("cg_scoreBoardForceLineHeightTeam");
		defaultCount = DC->getCVarValue("cg_scoreBoardForceLineHeightTeamDefault");
		if (defaultCount < 1) {
			defaultCount = 1;
		}

		// default height of 16 fits 8 players
		if (forceLineHeight > 0.001f) {
			elementHeight = forceLineHeight;
		} else if (forceLineHeight < 0.0f) {
			if (count > defaultCount) {
				elementHeight *= (float)defaultCount / (count + 1);  // +1 since the last one can be cut off
			}
		} else {  // 0 disables and uses height set in menu
			// pass
		}
	} else if (item->special == FEEDER_SCOREBOARD) {
		float forceLineHeight;
		int defaultCount;

		forceLineHeight = DC->getCVarValue("cg_scoreBoardForceLineHeight");
		defaultCount = DC->getCVarValue("cg_scoreBoardForceLineHeightDefault");
		if (defaultCount < 1) {
			defaultCount = 1;
		}

		// default height of 18 fits 9 players
		if (forceLineHeight > 0.001f) {
			elementHeight = forceLineHeight;
		} else if (forceLineHeight < 0.0f) {
			if (count > defaultCount) {
				elementHeight *= (float)defaultCount / (count + 1);
			}
		} else {  // 0 disables and uses height set in menu
			// pass
		}
	}

	// default is vertical if horizontal flag is not here
	if (item->window.flags & WINDOW_HORIZONTAL) {
		// draw scrollbar in bottom of the window
		// bar
		x = item->window.rect.x + 1;
		y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft, item->widescreen, menuRect);
		x += SCROLLBAR_SIZE - 1;
		size = item->window.rect.w - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, size+1, SCROLLBAR_SIZE, DC->Assets.scrollBar, item->widescreen, menuRect);
		x += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight, item->widescreen, menuRect);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);//Item_ListBox_ThumbPosition(item);
		if (thumb > x - SCROLLBAR_SIZE - 1) {
			thumb = x - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic(thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb, item->widescreen, menuRect);
		//
		listPtr->endPos = listPtr->startPos;
		size = item->window.rect.w - 2;
		// items
		// size contains max available space
		if (listPtr->elementStyle == LISTBOX_IMAGE) {
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for (i = listPtr->startPos; i < count; i++) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image) {
					DC->drawHandlePic(x+1, y+1, listPtr->elementWidth - 2, elementHeight - 2, image, item->widescreen, menuRect);
				}

				if (i == item->cursorPos) {
					DC->drawRect(x, y, listPtr->elementWidth-1, elementHeight-1, item->window.borderSize, item->window.borderColor, item->widescreen, menuRect);
				}

				size -= listPtr->elementWidth;
				if (size < listPtr->elementWidth) {
					listPtr->drawPadding = size; //listPtr->elementWidth - size;
					break;
				}
				x += listPtr->elementWidth;
				listPtr->endPos++;
				// fit++;
			}
		} else {
			//
		}
	} else {
		// draw scrollbar to right side of the window
		x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE - 1;
		y = item->window.rect.y + 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp, item->widescreen, menuRect);
		y += SCROLLBAR_SIZE - 1;

		listPtr->endPos = listPtr->startPos;
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, size+1, DC->Assets.scrollBar, item->widescreen, menuRect);
		y += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown, item->widescreen, menuRect);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);//Item_ListBox_ThumbPosition(item);
		if (thumb > y - SCROLLBAR_SIZE - 1) {
			thumb = y - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic(x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb, item->widescreen, menuRect);

		// adjust size for item painting
		size = item->window.rect.h - 2;
		if (listPtr->elementStyle == LISTBOX_IMAGE) {
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for (i = listPtr->startPos; i < count; i++) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image) {
					DC->drawHandlePic(x+1, y+1, listPtr->elementWidth - 2, elementHeight - 2, image, item->widescreen, menuRect);
				}

				if (i == item->cursorPos) {
					DC->drawRect(x, y, listPtr->elementWidth - 1, elementHeight - 1, item->window.borderSize, item->window.borderColor, item->widescreen, menuRect);
				}

				listPtr->endPos++;
				size -= listPtr->elementWidth;
				if (size < elementHeight) {
					listPtr->drawPadding = elementHeight - size;
					break;
				}
				y += elementHeight;
				// fit++;
			}
		} else {
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for (i = listPtr->startPos; i < count; i++) {
				const char *text;
				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if (LastListSelected == listPtr  &&  i == listPtr->selectedCursorPos) {
#if 0
					vec4_t color;

					Vector4Copy(listPtr->selectedColor, color);
					color[3] = 0.5;
#endif

				    DC->fillRect(x + 2, y + 2 + 4, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, item->window.outlineColor, item->widescreen, menuRect);
				} else if ((int)i % 2 == 1) {
				    //DC->fillRect(x + 2, y + 2, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, item->window.outlineColor, item->widescreen, menuRect);
				    DC->fillRect(x + 2, y + 2 + 4, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, listPtr->altRowColor, item->widescreen, menuRect);
				}

				if (listPtr->numColumns > 0) {
					int j;
					for (j = 0; j < listPtr->numColumns; j++) {
						text = DC->feederItemText(item->special, i, j, &optionalImage);
						if (optionalImage >= 0) {
							DC->drawHandlePic(x + 4 + listPtr->columnInfo[j].pos, y - 1 + elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage, item->widescreen, menuRect);
						} else if (text) {
							//DC->drawText(x + 4 + listPtr->columnInfo[j].pos, y + elementHeight, item->textscale, item->window.foreColor, text, 0, listPtr->columnInfo[j].maxChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
							// scoreboard text
							DC->drawText(x + 4 + listPtr->columnInfo[j].pos, y + elementHeight, item->textscale, listPtr->elementColor, text, 0, listPtr->columnInfo[j].maxChars, item->textStyle, item->fontIndex, item->widescreen, menuRect);
							//Com_Printf("feeder: text scale %f\n", item->textscale);
							// 0.16
							//Com_Printf("max: %d\n", listPtr->columnInfo[j].maxChars);
						}
					}
				} else {
					text = DC->feederItemText(item->special, i, 0, &optionalImage);
					if (optionalImage >= 0) {
						//DC->drawHandlePic(x + 4 + elementHeight, y, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
					} else if (text) {
						//DC->drawText(x + 4, y + elementHeight, item->textscale, item->window.foreColor, text, 0, 0, item->textStyle, item->fontIndex);
						DC->drawText(x + 4, y + elementHeight, item->textscale, listPtr->elementColor, text, 0, 0, item->textStyle, item->fontIndex, item->widescreen, menuRect);
					}
				}

				//FIXME covers names in scoreboard
#if 0
				if (i == item->cursorPos) {
					DC->fillRect(x + 2, y + 2, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, item->window.outlineColor, item->widescreen, menuRect);
				}
#endif
#if 0
				if ((int)i % 2 == 1) {
				    //DC->fillRect(x + 2, y + 2, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, item->window.outlineColor, item->widescreen, menuRect);
				    DC->fillRect(x + 2, y + 2 + 4, item->window.rect.w - SCROLLBAR_SIZE - 4, elementHeight, listPtr->altRowColor, item->widescreen, menuRect);
				}
#endif
				size -= elementHeight;
				if (size < elementHeight) {
					listPtr->drawPadding = elementHeight - size;
					break;
				}
				listPtr->endPos++;
				y += elementHeight;
				// fit++;
			}
		}
	}
}


static void Item_OwnerDraw_Paint(itemDef_t *item) {
	//menuDef_t *parent;
	int menuWidescreen = 0;

	if (item == NULL) {
		return;
	}

	//parent = (menuDef_t*)item->parent;

	if (DC->ownerDrawItem) {
		vec4_t color, lowLight;
		menuDef_t *parent = (menuDef_t*)item->parent;
		rectDef_t menuRect;

		if (parent == NULL) {
			Com_Printf("^3FIXME check ItemOwnerDraw_Paint parent is null\n");
		} else {
			menuWidescreen = parent->widescreen;
			menuRect = parent->window.rect;
		}

		Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);
		memcpy(&color, &item->window.foreColor, sizeof(color));
		if (item->numColors > 0 && DC->getValue) {
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int i;
			float f = DC->getValue(item->window.ownerDraw);
			for (i = 0; i < item->numColors; i++) {
				if (f >= item->colorRanges[i].low && f <= item->colorRanges[i].high) {
					memcpy(&color, &item->colorRanges[i].color, sizeof(color));
					break;
				}
			}
		}

		if (item->window.flags & WINDOW_HASFOCUS) {
			lowLight[0] = 0.8 * parent->focusColor[0];
			lowLight[1] = 0.8 * parent->focusColor[1];
			lowLight[2] = 0.8 * parent->focusColor[2];
			lowLight[3] = 0.8 * parent->focusColor[3];
			LerpColor(parent->focusColor,lowLight,color,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
		} else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime/BLINK_DIVISOR) & 1)) {
			lowLight[0] = 0.8 * item->window.foreColor[0];
			lowLight[1] = 0.8 * item->window.foreColor[1];
			lowLight[2] = 0.8 * item->window.foreColor[2];
			lowLight[3] = 0.8 * item->window.foreColor[3];
			LerpColor(item->window.foreColor,lowLight,color,0.5+0.5*sin(DC->realTime / PULSE_DIVISOR));
		}

		if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) {
		  Com_Memcpy(color, parent->disableColor, sizeof(vec4_t));
		}

		//FIXME ((menuDef_t*)(item->parent))->widescreen check if parent is null

		if (item->text) {
			//FIXME check widescreen here
			Item_Text_Paint(item);
				if (item->text[0]) {
					// +8 is an offset kludge to properly align owner draw items that have text combined with them
					DC->ownerDrawItem(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->window.ownerDrawFlags2, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle, item->fontIndex, menuWidescreen, item->widescreen, menuRect);
				} else {
					DC->ownerDrawItem(item->textRect.x + item->textRect.w, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->window.ownerDrawFlags2, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle, item->fontIndex, menuWidescreen, item->widescreen, menuRect);
				}
			} else {
			DC->ownerDrawItem(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, item->textalignx, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->window.ownerDrawFlags2, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle, item->fontIndex, menuWidescreen, item->widescreen, menuRect);
		}
	}
}

static void Item_Paint(itemDef_t *item) {
  vec4_t red;
  menuDef_t *parent;
  rectDef_t menuRect;

  red[0] = red[3] = 1;
  red[1] = red[2] = 0;

  if (item == NULL) {
    return;
  }

  if (item->parent) {
	  menuRect = ((menuDef_t *)item->parent)->window.rect;
  }
  if (item->run) {
	  //UseScriptBuffer = qtrue;
	  //ScriptBuffer = (char *)item->run;
	  Item_RunFrameScript(item, item->run);
	  Item_UpdatePosition(item);
	  //Com_Printf("name %s\n", item->window.name);
  }

  parent = (menuDef_t *)item->parent;

  if (item->window.flags & WINDOW_ORBITING) {
    if (DC->realTime > item->window.nextTime) {
      float rx, ry, a, c, s, w, h;

      item->window.nextTime = DC->realTime + item->window.offsetTime;
      // translate
      w = item->window.rectClient.w / 2;
      h = item->window.rectClient.h / 2;
      rx = item->window.rectClient.x + w - item->window.rectEffects.x;
      ry = item->window.rectClient.y + h - item->window.rectEffects.y;
      a = 3 * M_PI / 180;
  	  c = cos(a);
      s = sin(a);
      item->window.rectClient.x = (rx * c - ry * s) + item->window.rectEffects.x - w;
      item->window.rectClient.y = (rx * s + ry * c) + item->window.rectEffects.y - h;
      Item_UpdatePosition(item);

    }
  }


  if (item->window.flags & WINDOW_INTRANSITION) {
    if (DC->realTime > item->window.nextTime) {
      int done = 0;
      item->window.nextTime = DC->realTime + item->window.offsetTime;
			// transition the x,y

			if (item->window.rectClient.x == item->window.rectEffects.x) {
				done++;
			} else {
				if (item->window.rectClient.x < item->window.rectEffects.x) {
					item->window.rectClient.x += item->window.rectEffects2.x;
					if (item->window.rectClient.x > item->window.rectEffects.x) {
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				} else {
					item->window.rectClient.x -= item->window.rectEffects2.x;
					if (item->window.rectClient.x < item->window.rectEffects.x) {
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
			}
			if (item->window.rectClient.y == item->window.rectEffects.y) {
				done++;
			} else {
				if (item->window.rectClient.y < item->window.rectEffects.y) {
					item->window.rectClient.y += item->window.rectEffects2.y;
					if (item->window.rectClient.y > item->window.rectEffects.y) {
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				} else {
					item->window.rectClient.y -= item->window.rectEffects2.y;
					if (item->window.rectClient.y < item->window.rectEffects.y) {
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
			}
			if (item->window.rectClient.w == item->window.rectEffects.w) {
				done++;
			} else {
				if (item->window.rectClient.w < item->window.rectEffects.w) {
					item->window.rectClient.w += item->window.rectEffects2.w;
					if (item->window.rectClient.w > item->window.rectEffects.w) {
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				} else {
					item->window.rectClient.w -= item->window.rectEffects2.w;
					if (item->window.rectClient.w < item->window.rectEffects.w) {
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
			}
			if (item->window.rectClient.h == item->window.rectEffects.h) {
				done++;
			} else {
				if (item->window.rectClient.h < item->window.rectEffects.h) {
					item->window.rectClient.h += item->window.rectEffects2.h;
					if (item->window.rectClient.h > item->window.rectEffects.h) {
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				} else {
					item->window.rectClient.h -= item->window.rectEffects2.h;
					if (item->window.rectClient.h < item->window.rectEffects.h) {
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
			}

      Item_UpdatePosition(item);

      if (done == 4) {
        item->window.flags &= ~WINDOW_INTRANSITION;
      }

    }
  }

  if ((item->window.ownerDrawFlags  ||  item->window.ownerDrawFlags2)  &&  DC->ownerDrawVisible) {
	  if (!DC->ownerDrawVisible(item->window.ownerDrawFlags, item->window.ownerDrawFlags2)) {
			item->window.flags &= ~WINDOW_VISIBLE;
		} else {
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)) {
		if (!Item_EnableShowViaCvar(item, CVAR_SHOW)) {
			return;
		}
	}

  if (item->window.flags & WINDOW_TIMEDVISIBLE) {

	}

  if (!(item->window.flags & WINDOW_VISIBLE)) {
    return;
  }

  // paint the rect first.. 
  Window_Paint(&item->window, parent->fadeAmount , parent->fadeClamp, parent->fadeCycle, item->widescreen, menuRect);

  if (debugMode) {
		vec4_t color;
		rectDef_t *r = Item_CorrectedTextRect(item);
    color[1] = color[3] = 1;
    color[0] = color[2] = 0;
    DC->drawRect(r->x, r->y, r->w, r->h, 1, color, item->widescreen, menuRect);
  }

  //DC->drawRect(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, 1, red, item->widescreen);

  switch (item->type) {
    case ITEM_TYPE_OWNERDRAW:
	  Item_OwnerDraw_Paint(item);
      break;
    case ITEM_TYPE_TEXT:
    case ITEM_TYPE_BUTTON:
      Item_Text_Paint(item);
      break;
    case ITEM_TYPE_RADIOBUTTON:
      break;
    case ITEM_TYPE_CHECKBOX:
      break;
    case ITEM_TYPE_EDITFIELD:
    case ITEM_TYPE_NUMERICFIELD:
      Item_TextField_Paint(item);
      break;
    case ITEM_TYPE_COMBO:
      break;
    case ITEM_TYPE_LISTBOX:
      Item_ListBox_Paint(item);
      break;
    //case ITEM_TYPE_IMAGE:
    //  Item_Image_Paint(item);
    //  break;
    case ITEM_TYPE_MODEL:
      Item_Model_Paint(item);
      break;
    case ITEM_TYPE_YESNO:
      Item_YesNo_Paint(item);
      break;
    case ITEM_TYPE_MULTI:
      Item_Multi_Paint(item);
      break;
    case ITEM_TYPE_BIND:
      Item_Bind_Paint(item);
      break;
    case ITEM_TYPE_SLIDER_COLOR:
		Com_Printf("FIXME item paint()  ITEM_TYPE_SLIDER_COLOR\n");
    case ITEM_TYPE_SLIDER:
      Item_Slider_Paint(item);
      break;
    default:
      break;
  }

}

void Menu_Init(menuDef_t *menu) {
	memset(menu, 0, sizeof(menuDef_t));
	menu->cursorItem = -1;
	menu->fadeAmount = DC->Assets.fadeAmount;
	menu->fadeClamp = DC->Assets.fadeClamp;
	menu->fadeCycle = DC->Assets.fadeCycle;
	// hack for ql, end game scoreboards don't define wideescreen so 2 appears
	// to be the default
	menu->widescreen = 2;
	Window_Init(&menu->window);
}

// unused
#if 0
static itemDef_t *Menu_GetFocusedItem(menuDef_t *menu) {
  int i;
  if (menu) {
    for (i = 0; i < menu->itemCount; i++) {
      if (menu->items[i]->window.flags & WINDOW_HASFOCUS) {
        return menu->items[i];
      }
    }
  }
  return NULL;
}
#endif

menuDef_t *Menu_GetFocused(void) {
  int i;

  for (i = 0; i < menuCount; i++) {
	  //Com_Printf("%d focus %d  visible %d   '%s'\n", i, Menus[i].window.flags & WINDOW_HASFOCUS, Menus[i].window.flags & WINDOW_VISIBLE, Menus[i].window.name);
	  
	  if (Menus[i].window.flags & WINDOW_HASFOCUS && Menus[i].window.flags & WINDOW_VISIBLE) {
		  return &Menus[i];
    }
  }
  return NULL;
}

void Menu_ScrollFeeder(menuDef_t *menu, int feeder, qboolean down) {
	if (menu) {
		int i;
    for (i = 0; i < menu->itemCount; i++) {
			if (menu->items[i]->special == feeder) {
				Item_ListBox_HandleKey(menu->items[i], (down) ? K_DOWNARROW : K_UPARROW, qtrue, qtrue);
				return;
			}
		}
	}
}



void Menu_SetFeederSelection(menuDef_t *menu, int feeder, int index, const char *name) {
	if (menu == NULL) {
		if (name == NULL) {
			menu = Menu_GetFocused();
		} else {
			menu = Menus_FindByName(name);
		}
	}

	if (menu) {
		int i;
    for (i = 0; i < menu->itemCount; i++) {
			if (menu->items[i]->special == feeder) {
				if (index == 0) {
					listBoxDef_t *listPtr = (listBoxDef_t*)menu->items[i]->typeData;
					listPtr->cursorPos = 0;
					listPtr->startPos = 0;
				}
				menu->items[i]->cursorPos = index;
				DC->feederSelection(menu->items[i]->special, menu->items[i]->cursorPos);
				return;
			}
		}
	}
}

qboolean Menus_AnyFullScreenVisible(void) {
  int i;
  for (i = 0; i < menuCount; i++) {
    if (Menus[i].window.flags & WINDOW_VISIBLE && Menus[i].fullScreen) {
			return qtrue;
    }
  }
  return qfalse;
}

menuDef_t *Menus_ActivateByName(const char *p) {
  int i;
  menuDef_t *m = NULL;
  //menuDef_t *focus = Menu_GetFocused();
  menuDef_t *focus = NULL;

  focus = Menu_GetFocused();

  for (i = 0; i < menuCount; i++) {
    if (Q_stricmp(Menus[i].window.name, p) == 0) {
	    m = &Menus[i];
		Menus_Activate(m);
		//focus = Menu_GetFocused();
		if (openMenuCount < MAX_OPEN_MENUS && focus != NULL) {
			menuStack[openMenuCount++] = focus;
			//Com_Printf("^6activated '%s'\n", p);
		} else {
			if (focus == NULL) {
				Com_Printf("^1ERROR: %s focus == NULL\n", __func__);
			} else {
				Com_Printf("^1ERROR:  %s MAX_OPEN_MENUS (%d)\n", __func__, MAX_OPEN_MENUS);
			}
		}
    } else {
      Menus[i].window.flags &= ~WINDOW_HASFOCUS;
    }
  }

  Display_CloseCinematics();
  return m;
}


void Item_Init(itemDef_t *item) {
	if (item == NULL) {
		Com_Printf("^1ERROR Item_Init item == null\n");
		return;
	}

	memset(item, 0, sizeof(itemDef_t));
	item->textscale = 0.22f;  // 0.55f
	Window_Init(&item->window);
}

void Menu_HandleMouseMove(menuDef_t *menu, float x, float y) {
  int i, pass;
  qboolean focusSet = qfalse;

  itemDef_t *overItem;
  if (menu == NULL) {
    return;
  }

  if (!(menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED))) {
    return;
  }

	if (itemCapture) {
		//Item_MouseMove(itemCapture, x, y);
		return;
	}

	if (g_waitingForKey || g_editingField) {
		return;
	}

  // FIXME: this is the whole issue of focus vs. mouse over.. 
  // need a better overall solution as i don't like going through everything twice
  for (pass = 0; pass < 2; pass++) {
    for (i = 0; i < menu->itemCount; i++) {
      // turn off focus each item
      // menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

      if (!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED))) {
        continue;
      }

			// items can be enabled and disabled based on cvars
			if (menu->items[i]->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_ENABLE)) {
				continue;
			}

			if (menu->items[i]->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(menu->items[i], CVAR_SHOW)) {
				continue;
			}



			//if (Rect_ContainsPoint(&menu->items[i]->window.rect, x, y)) {
			//if (Item_ContainsPoint(menu->items[i], x, y)) {
			if (Rect_ContainsWidescreenPoint(&menu->items[i]->window.rect, x, y, menu->items[i]->widescreen)) {
				if (pass == 1) {
					overItem = menu->items[i];
					if (overItem->type == ITEM_TYPE_TEXT && overItem->text) {
						if (!Rect_ContainsWidescreenPoint(Item_CorrectedTextRect(overItem), x, y, overItem->widescreen)) {
							continue;
						}
					}
					// if we are over an item
					if (IsVisible(overItem->window.flags)) {
						//Com_Printf("overitem %p\n", overItem);
						// different one
						Item_MouseEnter(overItem, x, y);
						// Item_SetMouseOver(overItem, qtrue);

						// if item is not a decoration see if it can take focus
						if (!focusSet) {
							focusSet = Item_SetFocus(overItem, x, y);
						}
					}
				}
      } else if (menu->items[i]->window.flags & WINDOW_MOUSEOVER) {
          Item_MouseLeave(menu->items[i]);
          Item_SetMouseOver(menu->items[i], qfalse);
      }
    }
  }

}

void Menu_Paint(menuDef_t *menu, qboolean forcePaint) {
	int i;

	if (menu == NULL) {
		return;
	}

	//FIXME menu->run script

	if (!(menu->window.flags & WINDOW_VISIBLE) &&  !forcePaint) {
		return;
	}

	if ((menu->window.ownerDrawFlags  ||  menu->window.ownerDrawFlags2)  &&  DC->ownerDrawVisible  &&  !DC->ownerDrawVisible(menu->window.ownerDrawFlags, menu->window.ownerDrawFlags2)) {
		return;
	}
	
	if (forcePaint) {
		menu->window.flags |= WINDOW_FORCED;
	}

	// draw the background if necessary
	if (menu->fullScreen) {
		// implies a background shader
		// FIXME: make sure we have a default shader if fullscreen is set with no background
		DC->drawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background, menu->widescreen, menu->window.rect);
	} else if (menu->window.background) {
		// this allows a background shader without being full screen
		//UI_DrawHandlePic(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, menu->backgroundShader);
	}

	// paint the background and or border
	Window_Paint(&menu->window, menu->fadeAmount, menu->fadeClamp, menu->fadeCycle, menu->widescreen, menu->window.rect);

	for (i = 0; i < menu->itemCount; i++) {
		Item_Paint(menu->items[i]);
	}

	if (debugMode) {
		vec4_t color;
		color[0] = color[2] = color[3] = 1;
		color[1] = 0;
		DC->drawRect(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, 1, color, menu->widescreen, menu->window.rect);
	}
}

/*
===============
Item_ValidateTypeData
===============
*/
static void Item_ValidateTypeData(itemDef_t *item) {
	if (item->typeData) {
		return;
	}

	if (item->type == ITEM_TYPE_LISTBOX) {
		item->typeData = UI_Alloc(sizeof(listBoxDef_t));
		memset(item->typeData, 0, sizeof(listBoxDef_t));
	} else if (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_YESNO || item->type == ITEM_TYPE_BIND || item->type == ITEM_TYPE_SLIDER   ||  item->type == ITEM_TYPE_SLIDER_COLOR || item->type == ITEM_TYPE_TEXT) {
		item->typeData = UI_Alloc(sizeof(editFieldDef_t));
		memset(item->typeData, 0, sizeof(editFieldDef_t));
		if (item->type == ITEM_TYPE_EDITFIELD) {
			if (!((editFieldDef_t *) item->typeData)->maxPaintChars) {
				((editFieldDef_t *) item->typeData)->maxPaintChars = MAX_EDITFIELD;
			}
		}
	} else if (item->type == ITEM_TYPE_MULTI) {
		item->typeData = UI_Alloc(sizeof(multiDef_t));
	} else if (item->type == ITEM_TYPE_MODEL) {
		item->typeData = UI_Alloc(sizeof(modelDef_t));
	}
}

/*
===============
Keyword Hash
===============
*/

#define KEYWORDHASH_SIZE	512

typedef struct keywordHash_s
{
	char *keyword;
	qboolean (*func)(itemDef_t *item, int handle);
	struct keywordHash_s *next;
} keywordHash_t;

int KeywordHash_Key(char *keyword) {
	int hash, i;

	hash = 0;
	for (i = 0; keyword[i] != '\0'; i++) {
		if (keyword[i] >= 'A' && keyword[i] <= 'Z')
			hash += (keyword[i] + ('a' - 'A')) * (119 + i);
		else
			hash += keyword[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE-1);
	return hash;
}

static void KeywordHash_Add(keywordHash_t *table[], keywordHash_t *key) {
	int hash;

	hash = KeywordHash_Key(key->keyword);
/*
	if (table[hash]) {
		int collision = qtrue;
	}
*/
	key->next = table[hash];
	table[hash] = key;
}

static keywordHash_t *KeywordHash_Find(keywordHash_t *table[], char *keyword)
{
	keywordHash_t *key;
	int hash;

	hash = KeywordHash_Key(keyword);
	for (key = table[hash]; key; key = key->next) {
		if (!Q_stricmp(key->keyword, keyword))
			return key;
	}
	return NULL;
}

/*
===============
Item Keyword Parse functions
===============
*/

// name <string>
static qboolean ItemParse_name( itemDef_t *item, int handle ) {
	if (!PC_String_Parse(handle, &item->window.name)) {
		return qfalse;
	}
	return qtrue;
}

// name <string>
static qboolean ItemParse_focusSound( itemDef_t *item, int handle ) {
	const char *name;
	if (!PC_String_Parse(handle, &name)) {
		return qfalse;
	}
	if (Q_stricmp(name, item->focusSoundName)) {
		item->focusSound = DC->registerSound(name, qfalse);
		item->focusSoundName = String_Alloc(name);
	}
	return qtrue;
}


// text <string>
static qboolean ItemParse_text( itemDef_t *item, int handle ) {
	if (!PC_String_Parse(handle, &item->text)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_textExt( itemDef_t *item, int handle ) {
	const char *inb;
	char outb[MAX_STRING_CHARS];
	const char *ip;
	char *op;
	char token[MAX_STRING_CHARS];
	char formatString[MAX_STRING_CHARS];
	char cvarStringBuffer[MAX_STRING_CHARS];
	char *tp;
	int length;
	float f;
	char *s;

	if (!PC_String_Parse(handle, &inb)) {
		return qfalse;
	}

	outb[0] = '\0';
	ip = inb;
	op = outb;

	length = 0;
	while (ip[0] != '\0') {
		if (length >= (MAX_STRING_CHARS - 1)) {
			Com_Printf("^1ItemParse_textExt() output string overflow\n");
			return qfalse;
		}

		if (ip[0] == '%') {
			if (ip[1] == '\0') {
				Com_Printf("^1ItemParse_textExt() invalid string '%s'\n", inb);
				return qfalse;
			} else if (ip[1] == '%') {
				ip++;
			} else {  // check for script variable
				int slength;
				int tlength;
				qboolean convertToInt;
				qboolean convertToString;

				//ip++;  // skip '%'

				// get formatString
				formatString[0] = '\0';
				s = formatString;
				slength = 0;

				convertToInt = qfalse;
				convertToString = qfalse;
				while (1) {
					if (slength >= (MAX_STRING_CHARS - 1)) {
						Com_Printf("^1ItemParse_textExt() format string too long\n");
						return qfalse;
					}
					if (ip[0] == '\0') {
						Com_Printf("^1ItemParse_textExt() missing format string\n");
						return qfalse;
					}

					if (ip[0] == '{') {
						s[0] = '\0';
						ip++;
						break;
					}

					if (!(ip[0] == ' '  ||  isdigit(ip[0])  ||  ip[0] == '.'  ||  ip[0] == 'f'  ||  ip[0] == 'd'  ||  ip[0] == 'x'  ||  ip[0] == '%'  ||  ip[0] == 's') ) {
						Com_Printf("^1ItemParse_textExt() invalid conversion char '%c'\n", ip[0]);
						return qfalse;
					}

					if (ip[0] == 'd'  ||  ip[0] == 'x') {
						convertToInt = qtrue;
					}

					if (ip[0] == 's') {
						convertToString = qtrue;
					}

					s[0] = ip[0];
					s++;
					ip++;
					slength++;
				}

				if (strlen(formatString) < 2) {
					Com_Printf("^1ItemParse_textExt() invalid format string '%s'\n", formatString);
					return qfalse;
				}

				token[0] = '\0';
				tp = token;
				tlength = 0;

				while (1) {
					if (tlength >= (MAX_STRING_CHARS - 1)) {
						Com_Printf("^1ItemParse_textExt() token string too long\n");
						return qfalse;
					}
					if (ip[0] == '\0') {
						Com_Printf("^1ItemParse_textExt() missing closing '}'\n");
						return qfalse;
					}
					if (ip[0] == '}') {
						tp[0] = '\0';
						ip++;
						break;
					}

					tp[0] = ip[0];
					tp++;
					ip++;
					tlength++;
				}

				if (convertToString) {
					cvarStringBuffer[0] = '\0';
					DC->getCVarString(token, cvarStringBuffer, sizeof(cvarStringBuffer));
					s = cvarStringBuffer;
					//Com_Printf("cvar '%s'\n", s);
				} else {
					f = MenuVar_Get(token);
					if (convertToInt) {
						s = va(formatString, (int)round(f));
					} else {
						s = va(formatString, f);
					}
				}

				while (*s) {
					if (length >= (MAX_STRING_CHARS - 1)) {
						Com_Printf("^1ItemParse_textExt() output string too long\n");
						return qfalse;
					}
					op[0] = s[0];
					op++;
					s++;
					length++;
				}
			}
		}

		op[0] = ip[0];

		ip++;
		op++;
		length++;
	}

	op[0] = '\0';

	item->text = String_Alloc(outb);

	return qtrue;
}

// group <string>
static qboolean ItemParse_group( itemDef_t *item, int handle ) {
	if (!PC_String_Parse(handle, &item->window.group)) {
		return qfalse;
	}
	return qtrue;
}

// asset_model <string>
static qboolean ItemParse_asset_model( itemDef_t *item, int handle ) {
	const char *temp;
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_String_Parse(handle, &temp)) {
		return qfalse;
	}
	if (Q_stricmp(temp, item->assetName)) {
		item->asset = DC->registerModel(temp);
		item->assetName = String_Alloc(temp);
		modelPtr->angle = rand() % 360;
	}
	return qtrue;
}

// asset_shader <string>
static qboolean ItemParse_asset_shader( itemDef_t *item, int handle ) {
	const char *temp;

	if (!PC_String_Parse(handle, &temp)) {
		return qfalse;
	}
	if (Q_stricmp(temp, item->assetName)) {
		item->asset = DC->registerShaderNoMip(temp);
		item->assetName = String_Alloc(temp);
	}
	return qtrue;
}

// model_origin <number> <number> <number>
static qboolean ItemParse_model_origin( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_Float_Parse(handle, &modelPtr->origin[0])) {
		if (PC_Float_Parse(handle, &modelPtr->origin[1])) {
			if (PC_Float_Parse(handle, &modelPtr->origin[2])) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

// model_fovx <number>
static qboolean ItemParse_model_fovx( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_x)) {
		return qfalse;
	}
	return qtrue;
}

// model_fovy <number>
static qboolean ItemParse_model_fovy( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_y)) {
		return qfalse;
	}
	return qtrue;
}

// model_rotation <integer>
static qboolean ItemParse_model_rotation( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->rotationSpeed)) {
		return qfalse;
	}
	return qtrue;
}

// model_angle <integer>
static qboolean ItemParse_model_angle( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->angle)) {
		return qfalse;
	}
	return qtrue;
}

// rect <rectangle>
static qboolean ItemParse_rect( itemDef_t *item, int handle ) {
	if (!PC_Rect_Parse(handle, &item->window.rectClient)) {
		return qfalse;
	}
	//Com_Printf("%f %f %f %f\n", item->window.rectClient.x, item->window.rectClient.y, item->window.rectClient.w, item->window.rectClient.h);
	return qtrue;
}

// style <integer>
static qboolean ItemParse_style( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->window.style)) {
		return qfalse;
	}
	return qtrue;
}

// decoration
static qboolean ItemParse_decoration( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_DECORATION;
	return qtrue;
}

// notselectable
static qboolean ItemParse_notselectable( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;
	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (item->type == ITEM_TYPE_LISTBOX && listPtr) {
		listPtr->notselectable = qtrue;
	}
	return qtrue;
}

// manually wrapped
static qboolean ItemParse_wrapped( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_WRAPPED;
	return qtrue;
}

// auto wrapped
static qboolean ItemParse_autowrapped( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_AUTOWRAPPED;
	return qtrue;
}


// horizontalscroll
static qboolean ItemParse_horizontalscroll( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_HORIZONTAL;
	return qtrue;
}

// type <integer>
static qboolean ItemParse_type( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->type)) {
		return qfalse;
	}
	Item_ValidateTypeData(item);
	return qtrue;
}

// elementwidth, used for listbox image elements
// uses textalignx for storage
static qboolean ItemParse_elementwidth( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Float_Parse(handle, &listPtr->elementWidth)) {
		return qfalse;
	}
	return qtrue;
}

// elementheight, used for listbox image elements
// uses textaligny for storage
static qboolean ItemParse_elementheight( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Float_Parse(handle, &listPtr->elementHeight)) {
		return qfalse;
	}

	return qtrue;
}

static qboolean ItemParse_elementColor( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Color_Parse(handle, &listPtr->elementColor)) {
		return qfalse;
	}

	return qtrue;
}

static qboolean ItemParse_selectedColor( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	//Com_Printf("FIXME ui_shared.c selectedColor\n");
	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Color_Parse(handle, &listPtr->selectedColor)) {
		return qfalse;
	}
	//listPtr->selectedColor[3] = 0.0;
	return qtrue;
}

static qboolean ItemParse_altRowColor( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Color_Parse(handle, &listPtr->altRowColor)) {
		return qfalse;
	}
	return qtrue;
}

// feeder <float>
static qboolean ItemParse_feeder( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->special)) {
		return qfalse;
	}
	return qtrue;
}

// elementtype, used to specify what type of elements a listbox contains
// uses textstyle for storage
static qboolean ItemParse_elementtype( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	listPtr = (listBoxDef_t*)item->typeData;
	if (!PC_Int_Parse(handle, &listPtr->elementStyle)) {
		return qfalse;
	}
	return qtrue;
}

// columns sets a number of columns and an x pos and width per.. 
static qboolean ItemParse_columns( itemDef_t *item, int handle ) {
	int num, i;
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	listPtr = (listBoxDef_t*)item->typeData;
	if (PC_Int_Parse(handle, &num)) {
		if (num > MAX_LB_COLUMNS) {
			Com_Printf("^1ERROR:  ItemParse_columns() %d > MAX_LB_COLUMNS(%d)\n", num, MAX_LB_COLUMNS);
			num = MAX_LB_COLUMNS;
		}
		listPtr->numColumns = num;
		for (i = 0; i < num; i++) {
			int pos, width, maxChars;

			if (PC_Int_Parse(handle, &pos) && PC_Int_Parse(handle, &width) && PC_Int_Parse(handle, &maxChars)) {
				listPtr->columnInfo[i].pos = pos;
				listPtr->columnInfo[i].width = width;
				listPtr->columnInfo[i].maxChars = maxChars;
				//Com_Printf("%d %d %d\n", pos, width, maxChars);
			} else {
				return qfalse;
			}
		}
	} else {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_border( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->window.border)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_bordersize( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->window.borderSize)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_visible( itemDef_t *item, int handle ) {
	int i;

	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	if (i) {
		item->window.flags |= WINDOW_VISIBLE;
	} else {
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	return qtrue;
}

static qboolean ItemParse_ownerdraw( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->window.ownerDraw)) {
		return qfalse;
	}
	item->type = ITEM_TYPE_OWNERDRAW;
	return qtrue;
}

static qboolean ItemParse_align( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->alignment)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_textalign( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->textalignment)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_textalignx( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->textalignx)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_textaligny( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->textaligny)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_textscale( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->textscale)) {
		return qfalse;
	}
	//Com_Printf("item %p '%s'  scale %f\n", item, item->window.name, item->textscale);
	return qtrue;
}

static qboolean ItemParse_textstyle( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->textStyle)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_backcolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		item->window.backColor[i]  = f;
	}
	return qtrue;
}

static qboolean ItemParse_forecolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		item->window.foreColor[i]  = f;
		item->window.flags |= WINDOW_FORECOLORSET;
	}
	return qtrue;
}

static qboolean ItemParse_bordercolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		item->window.borderColor[i]  = f;
	}
	return qtrue;
}

static qboolean ItemParse_outlinecolor( itemDef_t *item, int handle ) {
	if (!PC_Color_Parse(handle, &item->window.outlineColor)){
		return qfalse;
	}
	//FIXME ac hack
	//item->window.outlineColor[3] = 0.2;
	return qtrue;
}

static qboolean ItemParse_background( itemDef_t *item, int handle ) {
	const char *name;

	if (!PC_String_Parse(handle, &name)) {
		return qfalse;
	}
	if (Q_stricmp(name, item->window.backgroundName)) {
		item->window.background = DC->registerShaderNoMip(name);
		item->window.backgroundName = String_Alloc(name);
	}

	return qtrue;
}

static qboolean ItemParse_cinematic( itemDef_t *item, int handle ) {
	if (!PC_String_Parse(handle, &item->window.cinematicName)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_doubleClick( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData) {
		return qfalse;
	}

	listPtr = (listBoxDef_t*)item->typeData;

	if (!PC_Script_Parse(handle, &listPtr->doubleClick)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_onFocus( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->onFocus)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_leaveFocus( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->leaveFocus)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_mouseEnter( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->mouseEnter)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_mouseExit( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->mouseExit)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_mouseEnterText( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->mouseEnterText)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_mouseExitText( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->mouseExitText)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_action( itemDef_t *item, int handle ) {
	if (!PC_Script_Parse(handle, &item->action)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_special( itemDef_t *item, int handle ) {
	if (!PC_Float_Parse(handle, &item->special)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_cvarTest( itemDef_t *item, int handle ) {
	if (!PC_String_Parse(handle, &item->cvarTest)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemParse_cvar( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!PC_String_Parse(handle, &item->cvar)) {
		return qfalse;
	}
	if (item->typeData) {
		editPtr = (editFieldDef_t*)item->typeData;
		editPtr->minVal = -1;
		editPtr->maxVal = -1;
		editPtr->defVal = -1;
	}
	return qtrue;
}

static qboolean ItemParse_cvarSet (itemDef_t *item, int handle)
{
	const char *cvarName;
	const char *cvarString;

	if (!PC_String_Parse(handle, &cvarName)) {
		return qfalse;
	}
	if (!PC_String_Parse(handle, &cvarString)) {
		return qfalse;
	}

	DC->setCVar(cvarName, cvarString);

	return qtrue;
}

static qboolean ItemParse_maxChars( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;

	if (!PC_Int_Parse(handle, &maxChars)) {
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxChars = maxChars;
	return qtrue;
}

static qboolean ItemParse_maxPaintChars( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;

	if (!PC_Int_Parse(handle, &maxChars)) {
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxPaintChars = maxChars;
	return qtrue;
}



static qboolean ItemParse_cvarFloat( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	editPtr = (editFieldDef_t*)item->typeData;
	if (PC_String_Parse(handle, &item->cvar) &&
		PC_Float_Parse(handle, &editPtr->defVal) &&
		PC_Float_Parse(handle, &editPtr->minVal) &&
		PC_Float_Parse(handle, &editPtr->maxVal)) {
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_cvarInt( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData) {
		Com_Printf("^5parse int invalid type data\n");
		return qfalse;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	//FIXME PC_Int_Parse() ?
	if (PC_String_Parse(handle, &item->cvar) &&
		PC_Float_Parse(handle, &editPtr->defVal) &&
		PC_Float_Parse(handle, &editPtr->minVal) &&
		PC_Float_Parse(handle, &editPtr->maxVal)) {
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_cvarStrList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;
	int pass;
	
	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qtrue;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	pass = 0;
	while ( 1 ) {
		///////////////// here
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu item");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';') {
			continue;
		}

		if (pass == 0) {
			multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
			pass = 1;
		} else {
			multiPtr->cvarStr[multiPtr->count] = String_Alloc(token.string);
			pass = 0;
			multiPtr->count++;
			if (multiPtr->count >= MAX_MULTI_CVARS) {
				Com_Printf("^1ERROR:  ItemParse_cvarStrList() MAX_MULTI_CVARS(%d)\n", MAX_MULTI_CVARS);
				return qfalse;
			}
		}

	}
	return qfalse;
}

static qboolean ItemParse_cvarFloatList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;
	
	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	while ( 1 ) {
		/////////////// here
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu item");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';') {
			continue;
		}

		multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
		if (!PC_Float_Parse(handle, &multiPtr->cvarValue[multiPtr->count])) {
			return qfalse;
		}

		multiPtr->count++;
		if (multiPtr->count >= MAX_MULTI_CVARS) {
			Com_Printf("^3%s MAX_MULTI_CVARS\n", __func__);
			return qfalse;
		}

	}
	return qfalse;
}



static qboolean ItemParse_addColorRange( itemDef_t *item, int handle ) {
	colorRangeDef_t color;

	if (PC_Float_Parse(handle, &color.low) &&
		PC_Float_Parse(handle, &color.high) &&
		PC_Color_Parse(handle, &color.color) ) {
		if (item->numColors < MAX_COLOR_RANGES) {
			memcpy(&item->colorRanges[item->numColors], &color, sizeof(color));
			item->numColors++;
		} else {
			Com_Printf("^1ERROR: MAX_COLOR_RANGES exceeded: %d >= %d\n", item->numColors, MAX_COLOR_RANGES);
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	item->window.ownerDrawFlags |= i;
	return qtrue;
}

static qboolean ItemParse_ownerdrawFlag2( itemDef_t *item, int handle ) {
	int i;
	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	item->window.ownerDrawFlags2 |= i;
	return qtrue;
}

static qboolean ItemParse_enableCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_ENABLE;
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_disableCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_DISABLE;
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_showCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_hideCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_HIDE;
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_defaultContent( itemDef_t *item, int handle ) {
	const char *temp;

	if (!PC_String_Parse(handle, &temp)) {
		return qfalse;
	}
	if (Q_stricmp(temp, item->window.backgroundName)) {
		item->window.background = DC->registerShaderNoMip(temp);
		item->window.backgroundName = String_Alloc(temp);
	}
	return qtrue;
}

static qboolean ItemParse_cellId( itemDef_t *item, int handle ) {
	int i;
	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	//item->window.ownerDrawFlags |= i;
	//FIXME
	return qtrue;
}

static qboolean ItemParse_font (itemDef_t *item, int handle) {
	const char *fontName;
	int pointSize;
	char checkName[MAX_QPATH];
	char baseName[MAX_QPATH];
	int i;

	if (!PC_String_Parse(handle, &fontName)) {
		return qfalse;
	}

	//Com_Printf("^4 fontName: '%s'\n", fontName);

	// 2015-10-26 ql now added itemdef 'font' keyword which is an integer
	if (strlen(fontName) == 1  &&  (fontName[0] == '0'  ||  fontName[0] == '1'  ||  fontName[0] == '2')) {
		int fontId;

		//PC_SourceWarning(handle, "font %s\n", fontName);
		//FIXME pointSize
		pointSize = 48;
		fontId = atoi(fontName);
		switch (fontId) {
		case FONT_SANS:
			fontName = DEFAULT_SANS_FONT;
			break;
		case FONT_MONO:
			fontName = DEFAULT_MONO_FONT;
			break;
		default:
		case FONT_DEFAULT:
			fontName = DEFAULT_FONT;
			break;
		}
	} else {  // wolfcamql font keyword (fontname : pointsize)
		if (!PC_Int_Parse(handle, &pointSize)) {
			return qfalse;
		}
	}

	if (!Q_stricmp(fontName, "qlfont")) {
		item->fontIndex = 0;
		return qtrue;
	} else if (!Q_stricmp(fontName, "q3tiny")  ||  !Q_stricmp(fontName, "q3small")  ||  !Q_stricmp(fontName, "q3big")  ||  !Q_stricmp(fontName, "q3giant")) {
		Q_strncpyz(checkName, fontName, sizeof(checkName));
	} else {
		Q_strncpyz(checkName, fontName, sizeof(checkName));
		COM_StripExtension(COM_SkipPath(checkName), baseName, sizeof(baseName));
		Q_strncpyz(checkName, va("fonts2/%s_%i.dat", baseName, pointSize), sizeof(checkName));
	}

	for (i = 0;  i <= DC->Assets.numFonts;  i++) {
		//Com_Printf("^5%d '%s'\n", i, DC->Assets.extraFonts[i].name);
		if (!Q_stricmp(checkName, DC->Assets.extraFonts[i].name)) {
			//Com_Printf("font '%s' already loaded\n", fontName);
			item->fontIndex = i;
			return qtrue;
		}
	}

	if (DC->Assets.numFonts + 1 >= MAX_FONTS + 1) {
		Com_Printf("^3ItemParse_font() MAX_FONTS (%d) reached, couldn't load '%s'\n", MAX_FONTS, fontName);
		item->fontIndex = 0;
		return qtrue;
	}

	DC->registerFont(fontName, pointSize, &DC->Assets.extraFonts[DC->Assets.numFonts + 1]);
	item->fontIndex = DC->Assets.numFonts + 1;
	DC->Assets.numFonts++;

	return qtrue;
}

static qboolean ItemParse_precision( itemDef_t *item, int handle ) {
	if (!PC_Int_Parse(handle, &item->precision)) {
		return qfalse;
	}
	return qtrue;
}

//FIXME utf8  --  2016-02-22 ok used with predefined math tokens
static char *Q_GetToken (char *inputString, char *token, qboolean isFilename, qboolean *newLine)
{
    char *p;
    char c;
    qboolean gotFirstToken;

	if (!inputString) {
		Com_Printf("Q_GetToken inputString == NULL\n");
		return NULL;
	}

    *newLine = qfalse;
    p = inputString;
    token[0] = '\0';

    if (p[0] == '\0') {  //  ||  (p[0] != '\t'  &&  (p[0] < ' '  ||  p[0] > '~'))) {
        //Com_Printf("xx\n");
        *newLine = qtrue;
        //Com_Printf("CG_GetToken() '%s'\n", tokenOrig);
        return p;
    }

#if 0
    if (p[0] == '\0'  ||  (p[0] != '\t'  &&  (p[0] < ' '  ||  p[0] > '~'))) {
        Com_Printf("xx\n");
        *newLine = qtrue;
        return p;
    }
#endif

    gotFirstToken = qfalse;

    while (1) {
        c = p[0];
        if (c == '\0') {
            //Com_Printf("end\n");
            *newLine = qtrue;
            break;
        }
		//FIXME utf8  --  2016-02-22 ok used with predefined math tokens
        if (c != '\t'  &&  (c < ' '  ||  c > '~')) {
            *newLine = qtrue;
            if (gotFirstToken) {
                p--;
            }
            break;
        }

        if (c == ' '  ||  c == '\t'  ||  c == '\''  ||  c == '"') {
            if (!gotFirstToken) {
                p++;
                continue;
            } else {
                break;
            }
        }

        if (!isFilename  &&  (c == '/'  ||  c == '-'  ||  c == '+'  ||  c == '*'  ||  c  == '('  ||  c  == ')' ||  c == ',')) {
            if (gotFirstToken) {
                p--;
                break;
            } else {
                token[0] = c;
                token++;
                break;
            }
        }

		//FIXME utf8  --  2016-02-22 ok used with predefined math tokens
        if (c < '!'  ||  c > '~') {
            break;
        }

        if (c == '{'  ||  c == '}') {
            if (gotFirstToken) {
                p--;
                break;
            }
        }

        token[0] = c;
        token++;
        p++;
        gotFirstToken = qtrue;
    }

    p++;
    token[0] = '\0';

#if 0
    if (isFilename) {
        Com_Printf("^6filename token: '%s'\n", start);
    } else {
        Com_Printf("^5token: '%s'\n", start);
    }
#endif

    //Com_Printf("CG_GetToken() '%s'\n", tokenOrig);

    return p;
}

//FIXME
#define OP_NOP 0
#define OP_PLUS 1
#define OP_MINUS 2
#define OP_MULT 3
#define OP_DIV 4
#define OP_LESS 5
#define OP_GREATER 6
#define OP_NOT 7
#define OP_EQUAL 8
#define OP_AND 9
#define OP_OR 10
#define OP_VAL 11
#define OP_MOD 12

//#define OP_CVAR 12  //FIXME probably not
//#define OP_RAND 13

#define OP_FSQRT 14
#define OP_FCEIL 15
#define OP_FFLOOR 16
#define OP_FSIN 17
#define OP_FCOS 18
#define OP_FWAVE 19
#define OP_FCLIP 20
#define OP_FACOS 21
#define OP_FASIN 22
#define OP_FATAN 23
#define OP_FATAN2 24
#define OP_FTAN 25
#define OP_FPOW 26
#define OP_FOWNERDRAWVALUE 27
#define OP_FINWATER 28

#define OP_FUNCFIRST OP_FSQRT
#define OP_FUNCLAST OP_FINWATER

#define USE_SWITCH 0

char *Q_MathScript (char *script, float *val, int *error)
{
    char token[MAX_QPATH];
    char buffer[256];  //FIXME
    char *p;
    float ops[256];
    int numOps;
    int i;
    int j;
    int err;
    static int recursiveCount = 0;
    int uniqueId;
    qboolean newLine;
    int verbose;

    recursiveCount++;
    uniqueId = rand();
    verbose = qfalse;  //SC_Cvar_Get_Int("debug_math");
    //verbose = SC_Cvar_Get_Int("debug_math");

    //Com_Printf("math(%d %d): '%s'\n", recursiveCount, uniqueId, script);

	newLine = qfalse;
    numOps = 0;
    while (1) {
        script = Q_GetToken(script, token, qfalse, &newLine);
        if (verbose) {
            //Com_Printf("token: '%s'  script[0] '%c' %d  newLine:%d\n", token, script[0], script[0], newLine);
            Com_Printf("qmath token: '%s'  script[0] %d  newLine:%d\n", token, script[0], newLine);
        }
        if (token[0] == '\0'  ||  token[0] == '{') {  //  ||  newLine)  {  // ||  end) {
        //if (token[0] == '\0'  ||  token[0] == '}') {  //  ||  newLine)  {  // ||  end) {
            break;
        }

        if (token[0] == '(') {
            int parenCount;

            parenCount = 1;
            p = script;
            while (1) {
                float tmpVal;

                if (p[0] == '(') {
                    parenCount++;
                } else if (p[0] == ')') {
                    parenCount--;
                }

                if (p[0] == '\t') {  // the only non printable char allowed
                    p++;
                    continue;
                }

                if (parenCount == 0  ||  p[0] == '\0'  ||  p[0] == '\n'  ||  (p[0] < ' '  ||  p[0] > '~')) {
                    memcpy(buffer, script, p - script);
                    buffer[p - script] = '\0';
                    //Com_Printf("paren(%d): %d '%s'\n", recursiveCount + 1, p - script, buffer);
                    err = 0;
                    Q_MathScript(buffer, &tmpVal, &err);
                    if (err) {
                        *error = err;
                        recursiveCount--;
                        return script;
                    }
                    ops[numOps] = OP_VAL;
                    numOps++;
                    ops[numOps] = tmpVal;
                    numOps++;
                    script = p + 1;
                    break;
                }
                p++;
            }
        }

		if (0) {

        } else if (!USE_SWITCH  &&  token[0] == '+') {
            ops[numOps] = OP_PLUS;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '-') {
            ops[numOps] = OP_MINUS;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '*') {
            ops[numOps] = OP_MULT;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '/') {
            ops[numOps] = OP_DIV;
            numOps += 2;
			goto tokenHandled;
		} else if (!USE_SWITCH  &&  token[0] == '%') {
            ops[numOps] = OP_MOD;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '<') {
            ops[numOps] = OP_LESS;
            numOps += 2;
			goto tokenHandled;
        } else if (token[0] == '>') {
            ops[numOps] = OP_GREATER;
            numOps += 2;
        } else if (token[0] == '!') {
            ops[numOps] = OP_NOT;
            numOps += 2;
        } else if (token[0] == '=') {
            ops[numOps] = OP_EQUAL;
            numOps += 2;
        } else if (token[0] == '&') {
            ops[numOps] = OP_AND;
            numOps += 2;
        } else if (token[0] == '|') {
            ops[numOps] = OP_OR;
            numOps += 2;
        } else if (!USE_SWITCH  &&  (token[0] == '.'  ||  (token[0] >= '0'  &&  token[0] <= '9'))) {  //((token[0] >= '0'  &&  token[0] <= '9')  ||  (token[0] == '-'  &&  (token[1] >= '0'  &&  token[1] <= '9'))) {
			//opnumbertoken:
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = atof(token);
            numOps++;
			goto tokenHandled;
#if 0
        } else if (!Q_stricmpt(token, "time")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = (cg.ftime - (float)cgs.levelStartTime) / 1000.0;
            //ops[numOps + 1] = (cg.time - (float)cgs.levelStartTime) / 1000.0;
            numOps += 2;
#endif
        // functions //

        } else if (!Q_stricmp(token, "sqrt")) {
            ops[numOps] = OP_FSQRT;
            numOps += 2;
        } else if (!Q_stricmp(token, "ceil")) {
            ops[numOps] = OP_FCEIL;
            numOps += 2;
        } else if (!Q_stricmp(token, "floor")) {
            ops[numOps] = OP_FFLOOR;
            numOps += 2;
        } else if (!Q_stricmp(token, "sin")) {
            ops[numOps] = OP_FSIN;
            numOps += 2;
        } else if (!Q_stricmp(token, "cos")) {
            ops[numOps] = OP_FCOS;
            numOps += 2;
        } else if (!Q_stricmp(token, "wave")) {
            ops[numOps] = OP_FWAVE;
            numOps += 2;
        } else if (!Q_stricmp(token, "clip")) {
            ops[numOps] = OP_FCLIP;
            numOps += 2;
        } else if (!Q_stricmp(token, "acos")) {
            ops[numOps] = OP_FACOS;
            numOps += 2;
        } else if (!Q_stricmp(token, "asin")) {
            ops[numOps] = OP_FASIN;
            numOps += 2;
        } else if (!Q_stricmp(token, "atan")) {
            ops[numOps] = OP_FATAN;
            numOps += 2;
		} else if (!Q_stricmp(token, "atan2")) {
			ops[numOps] = OP_FATAN2;
			numOps += 2;
        } else if (!Q_stricmp(token, "tan")) {
            ops[numOps] = OP_FTAN;
            numOps += 2;
		} else if (!Q_stricmp(token, "pow")) {
			ops[numOps] = OP_FPOW;
			numOps += 2;
		} else if (!Q_stricmp(token, "pi")) {
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = M_PI;
            numOps++;
        } else if (!Q_stricmp(token, "rand")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = random();
            numOps += 2;
        } else if (!Q_stricmp(token, "crand")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = crandom();
            numOps += 2;
		} else if (!Q_stricmp(token, "realtime")) {
			float val;

			val = DC->realTime;
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = val;
            numOps += 2;
		} else if (!Q_stricmp(token, "gametime")) {
			float val;

			val = DC->cgTime;
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = val;
            numOps += 2;
		} else if (!Q_stricmp(token, "ownerdrawvalue")) {
			ops[numOps] = OP_FOWNERDRAWVALUE;
			numOps += 2;
		} else if (token[0] == '@') {
			float f;

			f = MenuVar_Get(token);

			ops[numOps] = OP_VAL;
			ops[numOps + 1] = f;
			numOps += 2;
		} else if (token[0] == '(') {
			// pass
        } else if (DC->cvarExists(token)) {
			//FIXME check @vars
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = DC->getCVarValue(token);  //trap_Cvar_VariableValue(token)  //SC_Cvar_Get_Float(token);
            numOps += 2;
            if (verbose) {
                //Com_Printf("^3q3math unknown token: 's'\n", token);
                //Com_Printf("cvar: '%s' %f\n", token, SC_Cvar_Get_Float(token));
            }
        } else {
			Com_Printf("^3qmath unknown token/cvar '%s'\n", token);
			recursiveCount--;
			return script;
		}

	tokenHandled:
        if (newLine) {
            break;
        }
    }

    // sanity check
    if (ops[0] >= OP_PLUS  &&  ops[0] <= OP_OR  &&  ops[0] != OP_MINUS) {
        *error = 1;
        Com_Printf("^3qmath invalid first token(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
        recursiveCount--;
        return script;
    }

    if (numOps > 0  &&  (ops[numOps - 2] >= OP_PLUS  &&  ops[numOps - 2] <= OP_OR)) {
        *error = 2;
        Com_Printf("^3qmath invalid last token(%d %d): %f\n", recursiveCount, uniqueId, ops[numOps - 2]);
        recursiveCount--;
        return script;
    }

    // functions
    for (i = numOps - 2;  i >= 0;  i -= 2) {
        float val, val2;

        if (ops[i] < OP_FUNCFIRST  ||  ops[i] > OP_FUNCLAST) {
            continue;
        }

        val = 0;
        if ((i + 2) < (numOps - 1)) {
            if (ops[i + 2] != OP_VAL) {
                *error = 6;
                Com_Printf("^3qmath invalid function value type (%d %d): %f\n", recursiveCount, uniqueId, ops[i + 2]);
                recursiveCount--;
                return script;
            }
            val = ops[i + 3];
        }

        if (ops[i] == OP_FSQRT) {
            val = sqrt(val);
        } else if (ops[i] == OP_FCEIL) {
            val = ceil(val);
        } else if (ops[i] == OP_FFLOOR) {
            val = floor(val);
        } else if (ops[i] == OP_FSIN) {
            val = sin(DEG2RAD(val));
        } else if (ops[i] == OP_FCOS) {
            val = cos(DEG2RAD(val));
        } else if (ops[i] == OP_FWAVE) {
            //val = sin(val / M_PI);
			// fucking q3mme -- this is what they have
			val = sin(val * 2 * M_PI);
        } else if (ops[i] == OP_FCLIP) {
            if (val < 0.0) {
                val = 0;
            } else if (val > 1.0) {
                val = 1;
            }
        } else if (ops[i] == OP_FACOS) {
            val = RAD2DEG(acos(val));
        } else if (ops[i] == OP_FASIN) {
            val = RAD2DEG(M_PI / 2.0 - acos(val));
        } else if (ops[i] == OP_FATAN) {
            val = RAD2DEG(M_PI / 2.0 - acos(val / (sqrt(val * val + 1))));
		} else if (ops[i] == OP_FATAN2) {
            val2 = ops[i + 5];
			val = RAD2DEG(atan2(val, val2));
			numOps -= 2;
        } else if (ops[i] == OP_FTAN) {
			val = tan(DEG2RAD(val));
		} else if (ops[i] == OP_FPOW) {
            val2 = ops[i + 5];
			val = powf(val, val2);
			numOps -= 2;
		} else if (ops[i] == OP_FOWNERDRAWVALUE) {
			val = DC->getValue(val);
        } else {
            Com_Printf("^3qmath unknown function %f\n", ops[i]);
        }

        ops[i] = OP_VAL;
        ops[i + 1] = val;

        for (j = i + 4;  j < numOps;  j++) {
            ops[j - 2] = ops[j];
        }
        numOps -= 2;
    }

    // checking
    for (i = 2;  i < numOps;  i += 2) {
        if (ops[i] >= OP_PLUS  &&  ops[i] <= OP_OR) {
            if (ops[i + 2] >= OP_PLUS  &&  ops[i + 2] <= OP_OR) {
                if (ops[i + 2] != OP_MINUS) {
                    *error = 3;
                    Com_Printf("^3qmath two operands following each other(%d %d)\n", recursiveCount, uniqueId);
                    recursiveCount--;
                    return script;
                } else {
                    i += 2;
                }
            }
        }
    }

    // - as multiplier
    for (i = 0;  i < numOps;  i += 2) {
        if (ops[i] == OP_NOP) {
            continue;
        }
        if (ops[i] == OP_MINUS) {
            if (i == 0  ||  (i >= 2  &&  (ops[i - 2] >= OP_PLUS  &&  ops[i - 2] <= OP_OR))) {
                ops[i] = OP_VAL;
                ops[i + 1] = -(ops[i + 3]);
                for (j = i + 4;  j < numOps;  j++) {
                    ops[j - 2] = ops[j];
                }
                numOps -= 2;
            }
        }
    }

    // * /

    for (i = 0;  i < numOps;  i += 2) {
        if (ops[i] == OP_NOP) {
            continue;
        }

        if (ops[i] == OP_MULT) {
            //Com_Printf("(%d %d)  %f * %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] *= ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_DIV) {
            //Com_Printf("(%d %d)  %f / %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
#if 0
			if (ops[i + 3] == 0.0) {
				Com_Printf("^3qmath divide by zero\n");
			}
#endif
            ops[i - 1] /= ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
		} else if (ops[i] == OP_MOD) {
            //Com_Printf("(%d %d)  %f % %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = (int)round(ops[i - 1]) % (int)round(ops[i + 3]);
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        }
    }

    // + -

    for (i = 0;  i < numOps;  i += 2) {
        if (ops[i] == OP_NOP) {
            continue;
        }

        if (ops[i] == OP_PLUS) {
            //Com_Printf("(%d %d)  %f + %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] += ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_MINUS) {
            //Com_Printf("(%d %d)  %f - %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] -= ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        }
    }

    // < > ! =    (= is ==  and  ! is !=, equal and not equal)

    for (i = 0;  i < numOps;  i += 2) {
        if (ops[i] == OP_NOP) {
            continue;
        }

        if (ops[i] == OP_LESS) {
            //Com_Printf("(%d %d)  %f < %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] < ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_GREATER) {
            //Com_Printf("(%d %d)  %f > %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] > ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_NOT) {
            //Com_Printf("(%d %d)  %f != %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] != ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_EQUAL) {
            //Com_Printf("(%d %d)  %f == %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] == ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        }
    }

	// & |
    for (i = 0;  i < numOps;  i += 2) {
        if (ops[i] == OP_NOP) {
            continue;
        }

        if (ops[i] == OP_AND) {
            //Com_Printf("(%d %d)  %f && %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] && ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        } else if (ops[i] == OP_OR) {
            //Com_Printf("(%d %d)  %f || %f\n", recursiveCount, uniqueId, ops[i - 1], ops[i + 3]);
            ops[i - 1] = ops[i - 1] || ops[i + 3];
            for (j = i + 4;  j < numOps;  j++) {
                ops[j - 4] = ops[j];
            }
            numOps -= 4;
            i -= 2;
            continue;
        }
	}

    // sanity check
    if (ops[0] != OP_VAL) {
        *error = 5;
        Com_Printf("^3qmath invalid final value op(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
        recursiveCount--;
        return script;
    }

    *val = ops[1];
    if (verbose) {
        Com_Printf("val(%d %d)(numOps:%d): %f\n", recursiveCount, uniqueId, numOps, *val);
    }

    recursiveCount--;
    return script;
}

static qboolean ItemParse_setVar (itemDef_t *item, int handle)
{
	const char *var;
	char *mathScript;
	int err;
	float f;

	if (!PC_String_Parse(handle, &var)) {
		return qfalse;
	}

	if (!PC_Parenthesis_Parse(handle, (const char **)&mathScript)) {
		return qfalse;
	}
	Q_MathScript(mathScript, &f, &err);
	if (!MenuVar_Set(var, f)) {
		return qfalse;
	}

	return qtrue;
}

static qboolean ItemParse_run (itemDef_t *item, int handle)
{
	if (PC_Script_Parse(handle, &item->run)) {
		return qtrue;
	}
	return qfalse;
}

static qboolean ItemParse_printVal (itemDef_t *item, int handle)
{
	char *mathScript;
	int err;
	float f;

	if (!PC_Parenthesis_Parse(handle, (const char **)&mathScript)) {
		return qfalse;
	}
	Q_MathScript(mathScript, &f, &err);
	Com_Printf("%f\n", f);

	return qtrue;
}

static qboolean ItemParse_widescreen (itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->widescreen)) {
		return qfalse;
	}

	//Com_Printf("^3FIXME item parse widescreen %d\n", item->widescreen);
	//PC_SourceWarning(handle, "FIXME item parse widescreen %d", item->widescreen);

	if (item->widescreen < 0  ||  item->widescreen > 3) {
		PC_SourceError(handle, "invalid widescreen value %d", item->widescreen);
		item->widescreen = 0;
	}

	return qtrue;
}

keywordHash_t itemParseKeywords[] = {
	{"name", ItemParse_name, NULL},
	{"text", ItemParse_text, NULL},
	{"textext", ItemParse_textExt, NULL},
	{"group", ItemParse_group, NULL},
	{"asset_model", ItemParse_asset_model, NULL},
	{"asset_shader", ItemParse_asset_shader, NULL},
	{"model_origin", ItemParse_model_origin, NULL},
	{"model_fovx", ItemParse_model_fovx, NULL},
	{"model_fovy", ItemParse_model_fovy, NULL},
	{"model_rotation", ItemParse_model_rotation, NULL},
	{"model_angle", ItemParse_model_angle, NULL},
	{"rect", ItemParse_rect, NULL},
	{"style", ItemParse_style, NULL},
	{"decoration", ItemParse_decoration, NULL},
	{"notselectable", ItemParse_notselectable, NULL},
	{"wrapped", ItemParse_wrapped, NULL},
	{"autowrapped", ItemParse_autowrapped, NULL},
	{"horizontalscroll", ItemParse_horizontalscroll, NULL},
	{"type", ItemParse_type, NULL},
	{"elementwidth", ItemParse_elementwidth, NULL},
	{"elementheight", ItemParse_elementheight, NULL},
	{"elementcolor", ItemParse_elementColor, NULL},
	{"selectedcolor", ItemParse_selectedColor, NULL},
	{"altrowcolor", ItemParse_altRowColor, NULL},
	{"feeder", ItemParse_feeder, NULL},
	{"elementtype", ItemParse_elementtype, NULL},
	{"columns", ItemParse_columns, NULL},
	{"border", ItemParse_border, NULL},
	{"bordersize", ItemParse_bordersize, NULL},
	{"visible", ItemParse_visible, NULL},
	{"ownerdraw", ItemParse_ownerdraw, NULL},
	{"align", ItemParse_align, NULL},
	{"textalign", ItemParse_textalign, NULL},
	{"textalignx", ItemParse_textalignx, NULL},
	{"textaligny", ItemParse_textaligny, NULL},
	{"textscale", ItemParse_textscale, NULL},
	{"textstyle", ItemParse_textstyle, NULL},
	{"backcolor", ItemParse_backcolor, NULL},
	{"forecolor", ItemParse_forecolor, NULL},
	{"bordercolor", ItemParse_bordercolor, NULL},
	{"outlinecolor", ItemParse_outlinecolor, NULL},
	{"background", ItemParse_background, NULL},
	{"onFocus", ItemParse_onFocus, NULL},
	{"leaveFocus", ItemParse_leaveFocus, NULL},
	{"mouseEnter", ItemParse_mouseEnter, NULL},
	{"mouseExit", ItemParse_mouseExit, NULL},
	{"mouseEnterText", ItemParse_mouseEnterText, NULL},
	{"mouseExitText", ItemParse_mouseExitText, NULL},
	{"action", ItemParse_action, NULL},
	{"special", ItemParse_special, NULL},
	{"cvar", ItemParse_cvar, NULL},
	{"cvarset", ItemParse_cvarSet, NULL},
	{"maxChars", ItemParse_maxChars, NULL},
	{"maxPaintChars", ItemParse_maxPaintChars, NULL},
	{"focusSound", ItemParse_focusSound, NULL},
	{"cvarFloat", ItemParse_cvarFloat, NULL},
	{"cvarInt", ItemParse_cvarInt, NULL},
	{"cvarStrList", ItemParse_cvarStrList, NULL},
	{"cvarFloatList", ItemParse_cvarFloatList, NULL},
	{"addColorRange", ItemParse_addColorRange, NULL},
	{"ownerdrawFlag", ItemParse_ownerdrawFlag, NULL},
	{"ownerdrawFlag2", ItemParse_ownerdrawFlag2, NULL},
	{"enableCvar", ItemParse_enableCvar, NULL},
	{"cvarTest", ItemParse_cvarTest, NULL},
	{"disableCvar", ItemParse_disableCvar, NULL},
	{"showCvar", ItemParse_showCvar, NULL},
	{"hideCvar", ItemParse_hideCvar, NULL},
	{"cinematic", ItemParse_cinematic, NULL},
	{"doubleclick", ItemParse_doubleClick, NULL},
	{"defaultContent", ItemParse_defaultContent, NULL},
	{"cellId", ItemParse_cellId, NULL},
	{"font", ItemParse_font, NULL},
	{"precision", ItemParse_precision, NULL},
	{"setvar", ItemParse_setVar, NULL},
	{"run", ItemParse_run, NULL},
	{"printval", ItemParse_printVal, NULL},
	{ "widescreen", ItemParse_widescreen, NULL },
	{NULL, 0, NULL}
};

keywordHash_t *itemParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Item_SetupKeywordHash
===============
*/
static void Item_SetupKeywordHash(void) {
	int i;

	memset(itemParseKeywordHash, 0, sizeof(itemParseKeywordHash));
	for (i = 0; itemParseKeywords[i].keyword; i++) {
		KeywordHash_Add(itemParseKeywordHash, &itemParseKeywords[i]);
	}
}

static void Item_RunFrameScript (itemDef_t *item, const char *script)
{
	keywordHash_t *key;
	const char *token;
	char *oldScriptBuffer;
	qboolean oldUseScriptBuffer;

	if (item == NULL) {
		Com_Printf("^1Item_RunFrameScript() item == NULL\n");
		return;
	}

	if (script == NULL) {
		Com_Printf("^1Item_RunFrameScript() script == NULL\n");
		return;
	}

	oldScriptBuffer = ScriptBuffer;
	oldUseScriptBuffer = UseScriptBuffer;

	UseScriptBuffer = qtrue;
	ScriptBuffer = (char *)script;
	while ( 1 ) {
	loopStart:
		if (!String_Parse(&ScriptBuffer, &token)) {
			// all done
			break;
		}

		while (!Q_stricmp(token, "if")) {
			char s[MAX_TOKEN_CHARS];

			if (!ItemParse_if(item, -1, s, qfalse)) {
				ScriptBuffer = oldScriptBuffer;
				UseScriptBuffer = oldUseScriptBuffer;
				return;
			}
			if (s[0] == '\0') {
				goto loopStart;
			}
			token = String_Alloc(s);
		}

		key = KeywordHash_Find(itemParseKeywordHash, (char *)token);
		if (!key) {
			Com_Printf("^1Item_RunFrameScript()  unknown menu item keyword '%s'\n", token);
			continue;
		}
		if ( !key->func(item, -1) ) {
			Com_Printf("^1Item_RunFrameScript()  couldn't parse menu item keyword '%s'\n", token);
			break;
		}
	}

	ScriptBuffer = oldScriptBuffer;
	UseScriptBuffer = oldUseScriptBuffer;
}

static qboolean ItemParse_if (itemDef_t *item, int handle, char *lastToken, qboolean forceSkip)
{
	char *mathScript;
	const char *script;
	float f;
	int err;
	int oldUseScriptBuffer;
	char *oldScriptBuffer;
	const char *token;
	qboolean runElse;

	if (!PC_Parenthesis_Parse(handle, (const char **)&mathScript)) {
		return qfalse;
	}

	if (!forceSkip) {
		Q_MathScript(mathScript, &f, &err);
	} else {
		f = 0;
	}

	if (!PC_Script_Parse(handle, &script)) {
		return qfalse;
	}

	runElse = qfalse;
	if (f == 0.0) {
		runElse = qtrue;
	}

	// handle 'if' block
	if (!forceSkip  &&  !runElse) {
		//FIXME :(
 		oldUseScriptBuffer = UseScriptBuffer;
		oldScriptBuffer = ScriptBuffer;

		Item_RunFrameScript(item, script);

		ScriptBuffer = oldScriptBuffer;
		UseScriptBuffer = oldUseScriptBuffer;
	}

	// handle 'else' and 'elif' blocks

	//while (1) {
	if (!PC_String_Parse(handle, &token)) {
		// end of script
		lastToken[0] = '\0';
		return qtrue;
	}

	Q_strncpyz(lastToken, token, MAX_TOKEN_CHARS);

	if (token[0] == '}') {
		return qtrue;
	} else if (!Q_stricmp(token, "else")) {
		if (!PC_Script_ParseExt(handle, &script, &token)) {
			if (!Q_stricmp(token, "if")) {
				if (!forceSkip  &&  runElse) {
					return ItemParse_if(item, handle, lastToken, qfalse);
				} else {
					return ItemParse_if(item, handle, lastToken, qtrue);
				}
			} else {
				return qfalse;
			}
		}

		if (!forceSkip  &&  runElse) {
			oldUseScriptBuffer = UseScriptBuffer;
			oldScriptBuffer = ScriptBuffer;

			Item_RunFrameScript(item, script);

			ScriptBuffer = oldScriptBuffer;
			UseScriptBuffer = oldUseScriptBuffer;
		}
		lastToken[0] = '\0';
		return qtrue;
	} else {
		return qtrue;
	}

	//}
}

/*
===============
Item_ApplyHacks

Hacks to fix issues with Team Arena menu scripts
===============
*/
static void Item_ApplyHacks( itemDef_t *item ) {

	// Fix length of favorite address in createfavorite.menu
	if ( item->type == ITEM_TYPE_EDITFIELD && item->cvar && !Q_stricmp( item->cvar, "ui_favoriteAddress" ) ) {
		editFieldDef_t *editField = (editFieldDef_t *)item->typeData;

		// enough to hold an IPv6 address plus null
		if ( editField->maxChars < 48 ) {
			Com_Printf( "Extended create favorite address edit field length to hold an IPv6 address\n" );
			editField->maxChars = 48;
		}
	}

	if ( item->type == ITEM_TYPE_EDITFIELD && item->cvar && ( !Q_stricmp( item->cvar, "ui_Name" ) || !Q_stricmp( item->cvar, "ui_findplayer" ) ) ) {
		editFieldDef_t *editField = (editFieldDef_t *)item->typeData;

		// enough to hold a full player name
		if ( editField->maxChars < MAX_NAME_LENGTH ) {
			if ( editField->maxPaintChars > editField->maxChars ) {
				editField->maxPaintChars = editField->maxChars;
			}

			Com_Printf( "Extended player name field using cvar %s to %d characters\n", item->cvar, MAX_NAME_LENGTH );
			editField->maxChars = MAX_NAME_LENGTH;
		}
	}

}

/*
===============
Item_Parse
===============
*/
static qboolean Item_Parse(int handle, itemDef_t *item) {
	pc_token_t token;
	keywordHash_t *key;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	UseScriptBuffer = qfalse;
	while ( 1 ) {
	loopStart:
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "Item_Parse() end of file inside menu item");
			return qfalse;
		}

		// hack for reading extra token in parse_if()
		// returns qfalse if last token was '}'
		while (!Q_stricmp(token.string, "if")) {
			char s[MAX_TOKEN_CHARS];

			if (!ItemParse_if(item, handle, s, qfalse)) {
				return qfalse;
			}
			if (s[0] == '\0') {
				goto loopStart;
			}
			Q_strncpyz(token.string, s, sizeof(token.string));
		}

		if (*token.string == '}') {
			Item_ApplyHacks( item );
			return qtrue;
		}

		key = KeywordHash_Find(itemParseKeywordHash, token.string);
		if (!key) {
			PC_SourceError(handle, "Item_Parse() unknown menu item keyword %s", token.string);
			continue;
		}

		if ( !key->func(item, handle) ) {
			PC_SourceError(handle, "Item_Parse() couldn't parse menu item keyword %s", token.string);
			return qfalse;
		}
	}
	return qfalse;
}


// Item_InitControls
// init's special control types
static void Item_InitControls(itemDef_t *item) {
	if (item == NULL) {
		return;
	}
	if (item->type == ITEM_TYPE_LISTBOX) {
		listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
		item->cursorPos = 0;
		if (listPtr) {
			listPtr->cursorPos = 0;
			listPtr->startPos = 0;
			listPtr->endPos = 0;
			listPtr->cursorPos = 0;
		}
	}
}

/*
===============
Menu Keyword Parse functions
===============
*/

static qboolean MenuParse_font( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_String_Parse(handle, &menu->font)) {
		return qfalse;
	}
	if (!DC->Assets.fontRegistered) {
		DC->registerFont(menu->font, 48, &DC->Assets.textFont);
		DC->Assets.fontRegistered = qtrue;
	}
	return qtrue;
}

static qboolean MenuParse_name( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_String_Parse(handle, &menu->window.name)) {
		return qfalse;
	}
	if (Q_stricmp(menu->window.name, "main") == 0) {
		// default main as having focus
		//menu->window.flags |= WINDOW_HASFOCUS;
	}
	return qtrue;
}

static qboolean MenuParse_fullscreen( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	union
	{
		qboolean b;
		int i;
	} fullScreen;

	if (!PC_Int_Parse(handle, &fullScreen.i)) {
		return qfalse;
	}
	menu->fullScreen = fullScreen.b;
	return qtrue;
}

static qboolean MenuParse_rect( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Rect_Parse(handle, &menu->window.rect)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_style( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Int_Parse(handle, &menu->window.style)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_visible( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	if (i) {
		menu->window.flags |= WINDOW_VISIBLE;
	} else {
		menu->window.flags &= ~WINDOW_VISIBLE;
	}

	return qtrue;
}

static qboolean MenuParse_onOpen( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Script_Parse(handle, &menu->onOpen)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_onClose( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Script_Parse(handle, &menu->onClose)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_onESC( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Script_Parse(handle, &menu->onESC)) {
		return qfalse;
	}
	return qtrue;
}



static qboolean MenuParse_border( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Int_Parse(handle, &menu->window.border)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_borderSize( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Float_Parse(handle, &menu->window.borderSize)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_backcolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		menu->window.backColor[i]  = f;
	}
	return qtrue;
}

static qboolean MenuParse_forecolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		menu->window.foreColor[i]  = f;
		menu->window.flags |= WINDOW_FORECOLORSET;
		//Com_Printf("color[%d] %f\n", i, f);
	}
	return qtrue;
}

static qboolean MenuParse_bordercolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		menu->window.borderColor[i]  = f;
	}
	return qtrue;
}

static qboolean MenuParse_focuscolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		menu->focusColor[i]  = f;
	}
	return qtrue;
}

static qboolean MenuParse_disablecolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;
	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		menu->disableColor[i]  = f;
	}
	return qtrue;
}


static qboolean MenuParse_outlinecolor( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (!PC_Color_Parse(handle, &menu->window.outlineColor)){
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_background( itemDef_t *item, int handle ) {
	const char *buff;
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_String_Parse(handle, &buff)) {
		return qfalse;
	}
	if (Q_stricmp(buff, menu->window.backgroundName)) {
		menu->window.background = DC->registerShaderNoMip(buff);
		menu->window.backgroundName = String_Alloc(buff);
	}
	return qtrue;
}

static qboolean MenuParse_cinematic( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_String_Parse(handle, &menu->window.cinematicName)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	menu->window.ownerDrawFlags |= i;
	return qtrue;
}

static qboolean MenuParse_ownerdrawFlag2( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	menu->window.ownerDrawFlags2 |= i;
	return qtrue;
}

static qboolean MenuParse_ownerdraw( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &menu->window.ownerDraw)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_widescreen( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &menu->widescreen)) {
		return qfalse;
	}

	//PC_SourceWarning(handle, "FIXME menu widescreen %d", menu->widescreen);

	//Com_Printf("^1............\n");

	if (menu->widescreen < 0  ||  menu->widescreen > 3) {
		//Com_Printf("^1MenuParse invalid widescreen value: %d", menu->widescreen);
		PC_SourceError(handle, "menu parse invalid widescreen value: %d\n", menu->widescreen);
		menu->widescreen = 0;
	}

	return qtrue;
}


// decoration
static qboolean MenuParse_popup( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	menu->window.flags |= WINDOW_POPUP;
	return qtrue;
}


static qboolean MenuParse_outOfBounds( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	menu->window.flags |= WINDOW_OOB_CLICK;
	return qtrue;
}

static qboolean MenuParse_soundLoop( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_String_Parse(handle, &menu->soundName)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_fadeClamp( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Float_Parse(handle, &menu->fadeClamp)) {
		return qfalse;
	}
	return qtrue;
}

static qboolean MenuParse_fadeAmount( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Float_Parse(handle, &menu->fadeAmount)) {
		return qfalse;
	}
	return qtrue;
}


static qboolean MenuParse_fadeCycle( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &menu->fadeCycle)) {
		return qfalse;
	}
	return qtrue;
}


static qboolean MenuParse_itemDef( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if (menu->itemCount < MAX_MENUITEMS) {
		menu->items[menu->itemCount] = UI_Alloc(sizeof(itemDef_t));
		if (!menu->items[menu->itemCount]) {
			Com_Printf("^1ERROR:  UI_Alloc() failed for MenuParse_itemDef\n");
			return qfalse;
		}
		Item_Init(menu->items[menu->itemCount]);
		//FIXME widescreen hack
		menu->items[menu->itemCount]->widescreen = menu->widescreen;
		if (!Item_Parse(handle, menu->items[menu->itemCount])) {
			return qfalse;
		}
		/*
		if (menu->widescreen != menu->items[menu->itemCount]->widescreen) {
			Com_Printf("^4new widescreen value: %d from %d\n", menu->items[menu->itemCount]->widescreen, menu->widescreen);
		}
		*/
		Item_InitControls(menu->items[menu->itemCount]);
		menu->items[menu->itemCount++]->parent = menu;
	} else {
		Com_Printf("^1MenuParse_itemDef() menu->itemCount >= MAX_MENUITEMS (%d)\n", MAX_MENUITEMS);
	}
	return qtrue;
}

keywordHash_t menuParseKeywords[] = {
	{"font", MenuParse_font, NULL},
	{"name", MenuParse_name, NULL},
	{"fullscreen", MenuParse_fullscreen, NULL},
	{"rect", MenuParse_rect, NULL},
	{"style", MenuParse_style, NULL},
	{"visible", MenuParse_visible, NULL},
	{"onOpen", MenuParse_onOpen, NULL},
	{"onClose", MenuParse_onClose, NULL},
	{"onESC", MenuParse_onESC, NULL},
	{"border", MenuParse_border, NULL},
	{"borderSize", MenuParse_borderSize, NULL},
	{"backcolor", MenuParse_backcolor, NULL},
	{"forecolor", MenuParse_forecolor, NULL},
	{"bordercolor", MenuParse_bordercolor, NULL},
	{"focuscolor", MenuParse_focuscolor, NULL},
	{"disablecolor", MenuParse_disablecolor, NULL},
	{"outlinecolor", MenuParse_outlinecolor, NULL},
	{"background", MenuParse_background, NULL},
	{"ownerdraw", MenuParse_ownerdraw, NULL},
	{"ownerdrawFlag", MenuParse_ownerdrawFlag, NULL},
	{"ownerdrawFlag2", MenuParse_ownerdrawFlag2, NULL},
	{"outOfBoundsClick", MenuParse_outOfBounds, NULL},
	{"soundLoop", MenuParse_soundLoop, NULL},
	{"itemDef", MenuParse_itemDef, NULL},
	{"cinematic", MenuParse_cinematic, NULL},
	{"popup", MenuParse_popup, NULL},
	{"fadeClamp", MenuParse_fadeClamp, NULL},
	{"fadeCycle", MenuParse_fadeCycle, NULL},
	{"fadeAmount", MenuParse_fadeAmount, NULL},
	{"setvar", ItemParse_setVar, NULL},
	{"widescreen", MenuParse_widescreen, NULL},
	{NULL, 0, NULL}
};

keywordHash_t *menuParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Menu_SetupKeywordHash
===============
*/
static void Menu_SetupKeywordHash(void) {
	int i;

	memset(menuParseKeywordHash, 0, sizeof(menuParseKeywordHash));
	for (i = 0; menuParseKeywords[i].keyword; i++) {
		KeywordHash_Add(menuParseKeywordHash, &menuParseKeywords[i]);
	}
}

#if 0
static qboolean MenuParse_if (itemDef_t *item, int handle, char *lastToken);

static void Menu_RunFrameScript (itemDef_t *item, const char *script)
{
	keywordHash_t *key;
	const char *token;
	char *oldScriptBuffer;
	qboolean oldUseScriptBuffer;

	if (item == NULL) {
		Com_Printf("^1Menu_RunFrameScript() item == NULL\n");
		return;
	}

	if (script == NULL) {
		Com_Printf("^1Menu_RunFrameScript() script == NULL\n");
		return;
	}

	oldScriptBuffer = ScriptBuffer;
	oldUseScriptBuffer = UseScriptBuffer;

	UseScriptBuffer = qtrue;
	ScriptBuffer = (char *)script;
	while ( 1 ) {
	loopStart:
		if (!String_Parse(&ScriptBuffer, &token)) {
			// all done
			break;
		}

		while (!Q_stricmp(token, "if")) {
			char s[MAX_TOKEN_CHARS];

			if (!MenuParse_if(item, -1, s)) {
				ScriptBuffer = oldScriptBuffer;
				UseScriptBuffer = oldUseScriptBuffer;
				return;
			}
			if (s[0] == '\0') {
				goto loopStart;
			}
			token = String_Alloc(s);
		}

		key = KeywordHash_Find(menuParseKeywordHash, (char *)token);
		if (!key) {
			Com_Printf("^1Menu_RunFrameScript()  unknown menu item keyword '%s'\n", token);
			continue;
		}
		if ( !key->func(item, -1) ) {
			Com_Printf("^1Menu_RunFrameScript()  couldn't parse menu item keyword '%s'\n", token);
			break;
		}
	}

	ScriptBuffer = oldScriptBuffer;
	UseScriptBuffer = oldUseScriptBuffer;
}

static qboolean MenuParse_if (itemDef_t *item, int handle, char *lastToken)
{
	char *mathScript;
	const char *script;
	float f;
	int err;
	int oldUseScriptBuffer;
	char *oldScriptBuffer;
	const char *token;
	qboolean runElse;

	if (!PC_Parenthesis_Parse(handle, (const char **)&mathScript)) {
		return qfalse;
	}

	Q_MathScript(mathScript, &f, &err);
	if (!PC_Script_Parse(handle, &script)) {
		return qfalse;
	}

	runElse = qfalse;
	if (f == 0.0) {
		runElse = qtrue;
	}

	// handle 'if' block
	if (!runElse) {
		//FIXME :(
 		oldUseScriptBuffer = UseScriptBuffer;
		oldScriptBuffer = ScriptBuffer;

		Menu_RunFrameScript(item, script);

		ScriptBuffer = oldScriptBuffer;
		UseScriptBuffer = oldUseScriptBuffer;
	}

	// handle 'else' block

	if (!PC_String_Parse(handle, &token)) {
		// end of script
		lastToken[0] = '\0';
		return qtrue;
	}

	Q_strncpyz(lastToken, token, MAX_TOKEN_CHARS);

	if (token[0] == '}') {
		return qtrue;
	} else	if (!Q_stricmp(token, "else")) {
		if (!PC_Script_Parse(handle, &script)) {
			return qfalse;
		}
		if (runElse) {
			oldUseScriptBuffer = UseScriptBuffer;
			oldScriptBuffer = ScriptBuffer;

			Menu_RunFrameScript(item, script);

			ScriptBuffer = oldScriptBuffer;
			UseScriptBuffer = oldUseScriptBuffer;
		}
		lastToken[0] = '\0';
		return qtrue;
	} else {
		return qtrue;
	}
}
#endif

/*
===============
Menu_Parse
===============
*/
static qboolean Menu_Parse(int handle, menuDef_t *menu) {
	pc_token_t token;
	keywordHash_t *key;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	while ( 1 ) {

		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu");
			return qfalse;
		}

#if 0
		// hack for reading extra token in parse_if()
		// returns qfalse if last token was '}'
		if (!Q_stricmp(token.string, "if")) {
			char s[MAX_TOKEN_CHARS];

			Com_Printf("menuparse if\n");
			if (!MenuParse_if((itemDef_t *)menu, handle, s)) {
				return qfalse;
			}
			if (s[0] == '\0') {
				continue;
			}
			Q_strncpyz(token.string, s, sizeof(token.string));
		}
#endif

		if (*token.string == '}') {
			//PC_SourceError(handle, "close %s", token.string);
			return qtrue;
		}

		key = KeywordHash_Find(menuParseKeywordHash, token.string);
		if (!key) {
			PC_SourceError(handle, "unknown menu keyword %s", token.string);
			continue;
		}
		if ( !key->func((itemDef_t*)menu, handle) ) {
			PC_SourceError(handle, "couldn't parse menu keyword %s", token.string);
			return qfalse;
		}
	}
	return qfalse;
}

/*
===============
Menu_New
===============
*/
void Menu_New(int handle) {
	menuDef_t *menu = &Menus[menuCount];

	if (menuCount < MAX_MENUS) {
		Menu_Init(menu);
		if (Menu_Parse(handle, menu)) {
			Menu_PostParse(menu);
			menuCount++;
		}
	} else {
		Com_Printf("^1ERROR: Menu_New() already at MAX_MENUS  %d\n", MAX_MENUS);
	}
}

int Menu_Count(void) {
	return menuCount;
}

void Menu_HandleCapture (void)
{
	if (captureFunc) {
		//Com_Printf("^6calling capture func %p\n", captureFunc);
		captureFunc(captureData);
	}
}

void Menu_PaintAll(void) {
	int i;

	Menu_HandleCapture();

	for (i = 0; i < Menu_Count(); i++) {
		Menu_Paint(&Menus[i], qfalse);
		//Com_Printf("Menu_PaintAll %d  '%s'\n", i, Menus[i].window.name);
	}

	if (debugMode) {
		vec4_t v = {1, 1, 1, 1};
		rectDef_t r;
		DC->drawText(5, 25, .5, v, va("fps: %f", DC->FPS), 0, 0, 0, 0, 0, r);
	}
}

void Menu_Reset(void) {
	Com_Printf("Menu_Reset()\n");
	menuCount = 0;
}

displayContextDef_t *Display_GetContext(void) {
	return DC;
}
 
#ifndef MISSIONPACK
static float captureX;
static float captureY;
#endif

void *Display_CaptureItem(int x, int y) {
	int i;

	for (i = 0; i < menuCount; i++) {
		// turn off focus each item
		// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;
		if (Rect_ContainsWidescreenPoint(&Menus[i].window.rect, x, y, Menus[i].widescreen)) {
			return &Menus[i];
		}
	}
	return NULL;
}


// FIXME: 
qboolean Display_MouseMove(void *p, int x, int y) {
	int i;
	menuDef_t *menu = p;

	if (menu == NULL) {
		menu = Menu_GetFocused();
		if (menu) {
			if (menu->window.flags & WINDOW_POPUP) {
				Menu_HandleMouseMove(menu, x, y);
				return qtrue;
			}
		}
		for (i = 0; i < menuCount; i++) {
			Menu_HandleMouseMove(&Menus[i], x, y);
		}
	} else {
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		Menu_UpdatePosition(menu);
	}
 	return qtrue;

}

int Display_CursorType(int x, int y) {
	int i;
	for (i = 0; i < menuCount; i++) {
		rectDef_t r2;
		r2.x = Menus[i].window.rect.x - 3;
		r2.y = Menus[i].window.rect.y - 3;
		r2.w = r2.h = 7;
		if (Rect_ContainsWidescreenPoint(&r2, x, y, Menus[i].widescreen)) {
			return CURSOR_SIZER;
		}
	}
	return CURSOR_ARROW;
}


void Display_HandleKey(int key, qboolean down, int x, int y) {
	menuDef_t *menu = Display_CaptureItem(x, y);
	if (menu == NULL) {  
		menu = Menu_GetFocused();
	}
	if (menu) {
		Menu_HandleKey(menu, key, down );
	}
}

static void Window_CacheContents(windowDef_t *window) {
	if (window) {
		if (window->cinematicName) {
			//FIXME widescreen
			int cin = DC->playCinematic(window->cinematicName, 0, 0, 0, 0, /*widescreen*/ 0, window->rect);
			DC->stopCinematic(cin);
		}
	}
}


static void Item_CacheContents(itemDef_t *item) {
	if (item) {
		Window_CacheContents(&item->window);
	}

}

static void Menu_CacheContents(menuDef_t *menu) {
	if (menu) {
		int i;
		Window_CacheContents(&menu->window);
		for (i = 0; i < menu->itemCount; i++) {
			Item_CacheContents(menu->items[i]);
		}

		if (menu->soundName && *menu->soundName) {
			DC->registerSound(menu->soundName, qfalse);
		}
	}

}

void Display_CacheAll(void) {
	int i;
	for (i = 0; i < menuCount; i++) {
		Menu_CacheContents(&Menus[i]);
	}
}


static qboolean Menu_OverActiveItem(menuDef_t *menu, float x, float y) {
 	if (menu && menu->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED)) {
		if (Rect_ContainsWidescreenPoint(&menu->window.rect, x, y, menu->widescreen)) {
			int i;
			for (i = 0; i < menu->itemCount; i++) {
				// turn off focus each item
				// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

				if (!(menu->items[i]->window.flags & (WINDOW_VISIBLE | WINDOW_FORCED))) {
					continue;
				}

				if (menu->items[i]->window.flags & WINDOW_DECORATION) {
					continue;
				}

				if (Rect_ContainsWidescreenPoint(&menu->items[i]->window.rect, x, y, menu->items[i]->widescreen)) {
					itemDef_t *overItem = menu->items[i];
					if (overItem->type == ITEM_TYPE_TEXT && overItem->text) {
						if (Rect_ContainsWidescreenPoint(Item_CorrectedTextRect(overItem), x, y, overItem->widescreen)) {
							return qtrue;
						} else {
							continue;
						}
					} else {
						return qtrue;
					}
				}
			}

		}
	}
	return qfalse;
}


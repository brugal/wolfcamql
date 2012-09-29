
#include "cg_local.h"
#include "wolfcam_local.h"

void SC_Lowercase (char *p)
{
    while (*p) {
        int c = *p;
        *p = (char)tolower(c);
        p++;
    }
}

// ex:  '255 125 100'
int SC_ParseColorFromStr (const char *s, int *r, int *g, int *b)
{
    int i;
    int sl;
    //int wp;
    int c;
    qboolean inWhiteSpace;

    if (!s)
        return -1;

    sl = strlen(s);

    if (sl < 1)
        return -1;

    inWhiteSpace = qtrue;
    c = 0;
    for (/*wp = 0,*/ i = 0;  i < sl;  i++) {

        if (s[i] != ' '  &&  (s[i] < '0'  ||  s[i] > '9')) {
            CG_Printf("invalid color string\n");
            return -1;
        }
        if (inWhiteSpace) {
            if (s[i] != ' ') {
                int colorValue;

                inWhiteSpace = qfalse;
                colorValue = atoi(s + i);
                switch (c) {
                case 0:
                    *r = colorValue;
                    break;
                case 1:
                    *g = colorValue;
                    break;
                case 2:
                    *b = colorValue;
                    break;
                default:
                    break;
                }

                c++;
            }
        } else {
            if (s[i] == ' ') {
                inWhiteSpace = qtrue;
            }
        }
    }

    return 0;
}

int SC_ParseEnemyColorsFromStr (const char *s, byte colors[3][4] /* [3][4] */)
{
    int i;
    int j;
    int sl;
#define CSIZE (4 * 3)
    //int wsm[CSIZE];  // end of white space markers
    //int wp;
    int r, c;
    qboolean inWhiteSpace;

    for (i = 0;  i < 3;  i++) {
        for (j = 0;  j < 4;  j++) {
            colors[i][j] = 255;
        }
    }

    if (!s)
        return -1;

    sl = strlen(s);

    inWhiteSpace = qtrue;
    r = c = 0;
    for (/*wp = 0,*/ i = 0;  i < sl;  i++) {

        if (s[i] != ' '  &&  (s[i] < '0'  ||  s[i] > '9')) {
            CG_Printf("invalid enemy colors string\n");
            return -1;
        }
        if (inWhiteSpace) {
            if (s[i] != ' ') {
                //wsm[wp] = i;
                //wp++;
                //if (wp >= CSIZE)
                //    goto done;
                inWhiteSpace = qfalse;
                //CG_Printf("%d:  %d\n", i, atoi(s + i));
                colors[r][c] = atoi(s + i);
                //CG_Printf("[%d][%d]:  %d\n", r, c, colors[r][c]);
                c++;
                if (c == 3) {
                    c = 0;
                    r++;
                }
            }
        } else {
            if (s[i] == ' ') {
                inWhiteSpace = qtrue;
            }
        }
    }

// done:

    return 0;
#undef CSIZE
}

int SC_ParseVec3FromStr (const char *s, vec3_t v)
{
    int i;
    //int j;
    int sl;
#define CSIZE 3
    //int wp;
    int c;
    qboolean inWhiteSpace;

    if (!s)
        return -1;

	// sscanf()  doesn't work with integer strings :(  ex:  "100 -99 20"

    sl = strlen(s);

    inWhiteSpace = qtrue;
    c = 0;
    for (/*wp = 0,*/ i = 0;  i < sl;  i++) {

        if (s[i] != ' '  &&  ((s[i] < '0'  ||  s[i] > '9')  &&  s[i] != '-'  &&  s[i] != '.')) {
            CG_Printf("invalid vec3 string '%s'\n", s);
            return -1;
        }
        if (inWhiteSpace) {
            if (s[i] != ' ') {
                inWhiteSpace = qfalse;
                //colors[r][c] = atoi(s + i);
                //v[c] = (float)atoi(s + i);
				v[c] = atof(s + i);
                //CG_Printf("[%d][%d]:  %d\n", r, c, colors[r][c]);
                c++;
                if (c == 3) {
                    break;
                }
            }
        } else {
            if (s[i] == ' ') {
                inWhiteSpace = qtrue;
            }
        }
    }

// done:

	//CG_Printf("SC_ParseVec3FromStr()  '%s'  %f %f %f\n", s, v[0], v[1], v[2]);

    return 0;
#undef CSIZE
}

int SC_RedFromCvar (vmCvar_t cvar)
{
    int val;

    //val = cvar.value;
    val = cvar.integer;

    if (val <= 0xffffff) {
        val &= 0xff0000;
        val /= 0x010000;
    } else {  // hex with alpha
        val &= 0xff000000;
        val /= 0x01000000;
    }

    return val;
}

int SC_GreenFromCvar (vmCvar_t cvar)
{
    int val;

    //val = cvar.value;
    val = cvar.integer;

    if (val <= 0xffffff) {
        val &= 0x00ff00;
        val /= 0x000100;
    } else {  // hex with alpha
        val &= 0x00ff0000;
        val /= 0x00010000;
    }

    return val;
}

int SC_BlueFromCvar (vmCvar_t cvar)
{
    int val;

    //val = cvar.value;
    val = cvar.integer;

    if (val <= 0xffffff) {
        val &= 0x0000ff;
        val /= 0x000001;
    } else {  // hex with alpha
        val &= 0x0000ff00;
        val /= 0x00000100;
    }

    return val;
}

int SC_AlphaFromCvar (vmCvar_t cvar)
{
    int val;

    //val = cvar.value;
    val = cvar.integer;

    if (val <= 0xffffff) {
        val = 0;
    } else {  // hex with alpha
        val &= 0x000000ff;
        val /= 0x00000001;
    }

    return val;
}

// doesn't work
#if 0
void SC_Vec4ColorFromCvar (vec4_t *v, vmCvar_t cvar)
{
#if 0
    v[0] = (float)SC_RedFromCvar(cvar);
    v[1] = (float)SC_GreenFromCvar(cvar);
    v[2] = (float)SC_BlueFromCvar(cvar);
    v[3] = (float)SC_AlphaFromCvar(cvar);
#endif
}
#endif

// for shader colors
void SC_ByteVec3ColorFromCvar (byte *b, vmCvar_t cvar)
{
    b[0] = SC_RedFromCvar(cvar);
    b[1] = SC_GreenFromCvar(cvar);
    b[2] = SC_BlueFromCvar(cvar);
    //b[3] = SC_AlphaFromCvar(cvar);
    // vec4 doesn't work, incorrect values
}

void SC_Vec4ColorFromCvars (vec4_t color, vmCvar_t colorCvar, vmCvar_t alphaCvar)
{
    color[0] = (float)SC_RedFromCvar(colorCvar) / 255.f;
    color[1] = (float)SC_GreenFromCvar(colorCvar) / 255.f;
    color[2] = (float)SC_BlueFromCvar(colorCvar) / 255.f;
    color[3] = (float)alphaCvar.integer / 255.f;
}

void SC_Vec3ColorFromCvar (vec3_t color, vmCvar_t colorCvar)
{
    color[0] = (float)SC_RedFromCvar(colorCvar) / 255.f;
    color[1] = (float)SC_GreenFromCvar(colorCvar) / 255.f;
    color[2] = (float)SC_BlueFromCvar(colorCvar) / 255.f;
    //color[3] = (float)alphaCvar.integer / 255.f;
}

void SC_ByteVecColorFromInt (byte *b, int color)
{
	b[0] = (color & 0xff0000) / 0x010000;
	b[1] = (color & 0x00ff00) / 0x000100;
	b[2] = (color & 0x0000ff) / 0x000001;
}

int SC_Cvar_Get_Int (const char *cvar)
{
    char buff[MAX_STRING_CHARS];

    //memset(buff, 0, sizeof(buff));  //FIXME why are you doing this again??
    buff[0] = '\0';
    trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));

    return atoi(buff);
}

float SC_Cvar_Get_Float (const char *cvar)
{
    char buff[MAX_STRING_CHARS];

    buff[0] = '\0';
    trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));

    return atof(buff);
}

const char *SC_Cvar_Get_String (const char *cvar)
{
    static char buff[MAX_STRING_CHARS];

    buff[0] = '\0';
    trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));

    return (const char *)buff;
}

qboolean CG_IsEnemy (clientInfo_t *ci)
{
    int ourClientNum;
    clientInfo_t *us;

    if (wolfcam_following) {
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    us = &cgs.clientinfo[ourClientNum];

    if (us == ci) {
        return qfalse;
    }

    if (cgs.gametype >= GT_TEAM  &&  ci->team != us->team) {
        //Com_Printf("us %s %d    them %s %d\n", us->name, us->team, ci->name, ci->team);
        return qtrue;
    } else if (cgs.gametype < GT_TEAM) {
        return qtrue;
    }

    return qfalse;
}

qboolean CG_IsTeammate (clientInfo_t *ci)
{
    int ourClientNum;
    clientInfo_t *us;

    if (cgs.gametype < GT_TEAM) {
        return qfalse;
    }

    if (wolfcam_following) {
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    us = &cgs.clientinfo[ourClientNum];

    if (ci->team != us->team) {
        return qfalse;
    }

    return qtrue;
}

qboolean CG_IsUs (clientInfo_t *ci)
{
    int ourClientNum;

    if (wolfcam_following) {
        //Com_Printf("wc %d  %s  %s\n", wcg.clientNum, cgs.clientinfo[wcg.clientNum].name, ci->name);
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    if (ci == &cgs.clientinfo[ourClientNum]) {
        return qtrue;
    }

    return qfalse;
}

qboolean CG_IsTeamGame (int gametype)
{

	//FIXME red rover?
	if (gametype == GT_TEAM  ||  gametype == GT_CA  ||  gametype == GT_CTF  ||  gametype == GT_1FCTF  ||  gametype == GT_OBELISK  ||  gametype == GT_HARVESTER  ||  gametype == GT_FREEZETAG  ||  gametype == GT_DOMINATION  ||  gametype == GT_CTFS  ||  gametype == GT_RED_ROVER  ||  gametype == GT_NTF  ||  gametype == GT_2V2) {
		return qtrue;
	}

	return qfalse;
}

char *CG_FS_ReadLine (qhandle_t f, int *len)
{
    static char buffer[1024];
    int i;
    //int r;

    //trap_FS_Read( void *buffer, int len, fileHandle_t f );
    i = 0;
    *len = 0;
    buffer[0] = '\0';
    while (1) {
        if (i == 1022) {
            buffer[i + 1] = '\0';
            Com_Printf("CG_FS_ReadLine() buffer filled\n");
            return buffer;
        }
        trap_FS_Read((void *)&buffer[i], 1, f);
        //Com_Printf("%d %c", (int)buffer[i], buffer[i]);
        //Com_Printf("\n");
        if (buffer[i] == '\0') {
            //Com_Printf("...\n");
            return buffer;
        }
        if (buffer[i] == '\n'  ||  buffer[i] == '\f'  ||  buffer[i] == '\r') {
            buffer[i] = '\n';
            buffer[i + 1] = '\0';
            (*len)++;
            return buffer;
        }

        (*len)++;
        i++;
    }

    *len = 0;
    return NULL;
}

char *CG_GetToken (char *inputString, char *token, qboolean isFilename, qboolean *newLine)
{
    //int i;
    char *p;
    char c;
    qboolean gotFirstToken;
    //char *start;
    //char *tokenOrig;

    //tokenOrig = token;

    //start = token;

    if (1) {  //(isFilename) {
        //Com_Printf("inputstring '%s'\n", inputString);
    }

	if (!inputString) {
		Com_Printf("CG_GetToken inputString == NULL\n");
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

        if (!isFilename  &&  (c == '/'  ||  c == '-'  ||  c == '+'  ||  c == '*'  ||  c  == '('  ||  c  == ')')) {
            if (gotFirstToken) {
                p--;
                break;
            } else {
                token[0] = c;
                token++;
                break;
            }
        }

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

char *CG_GetTokenGameType (char *inputString, char *token, qboolean isFilename, qboolean *newLine)
{
    //int i;
    char *p;
    char c;
    qboolean gotFirstToken;
    //char *start;
    //char *tokenOrig;

    //tokenOrig = token;

    //start = token;

    if (1) {  //(isFilename) {
        //Com_Printf("inputstring '%s'\n", inputString);
    }

	if (!inputString) {
		Com_Printf("CG_GetToken inputString == NULL\n");
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
        if (c != '\t'  &&  (c < ' '  ||  c > '~')) {
            *newLine = qtrue;
            if (gotFirstToken) {
                p--;
            }
            break;
        }

        if (c == ' '  ||  c == '\t'  ||  c == '\''  ||  c == '"'  ||  c == ',') {
            if (!gotFirstToken) {
                p++;
                continue;
            } else {
                break;
            }
        }

        if (!isFilename  &&  (c == '/'  ||  c == '-'  ||  c == '+'  ||  c == '*'  ||  c  == '('  ||  c  == ')')) {
            if (gotFirstToken) {
                p--;
                break;
            } else {
                token[0] = c;
                token++;
                break;
            }
        }

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

void CG_LStrip (char **cp)
{
    char c;

    while (1) {
        c = *cp[0];
        if (c == '\0') {
            return;
        }
        if (c >= '!'  &&  c <= '~') {
            return;
        }

        (*cp)++;
    }
}

void CG_StripSlashComments (char *s)
{
    char *start;
    int len;
    char c;

    start = s;

    while (1) {
        if (s[0] == '\0') {
            break;
        } else if (s[1] == '\0') {
            break;
        } else if (s[0] == '/'  &&  s[1] == '/') {
            s[0] = '\n';
            s[1] = '\0';
            //return;
            break;
        }

        s++;
    }

    len = strlen(start);
    if (len < 2) {
        return;
    }

    s--;
    while (s > start) {
        c = s[0];

        if (c >= '!'  &&  c <= '~') {
            break;
        }

        s[1] = '\0';
        s[0] = '\n';
        s--;
    }

    //Com_Printf("^3new line %d '%s'\n", strlen(start), start);
}

void CG_CleanString (char *s)
{
    while (1) {
        if (s[0] == '\0') {
            break;
        } else if (s[0] == '\t') {
            s[0] = ' ';
        } else if (s[0] == '\f'  ||  s[0] == '\r'  ||  s[0] == '\n') {
            s[0] = '\n';
        } else if (s[0] < ' '  ||  s[0] > '~') {
            s[0] = ' ';
        }
        s++;
    }
}

void CG_Abort (void)
{
    int *p = NULL;

    CG_Printf("crashing deliberately ...\n");
    *p = 666;
}

void CG_ScaleModel (refEntity_t *re, float size)
{
    re->radius = size;
    VectorScale(re->axis[0], re->radius, re->axis[0]);
    VectorScale(re->axis[1], re->radius, re->axis[1]);
    VectorScale(re->axis[2], re->radius, re->axis[2]);
    re->nonNormalizedAxes = qtrue;
}

qboolean CG_IsFirstPersonView (int clientNum)
{
    if (cg.freecam) {
        return qfalse;
    }
    if (cg.cameraMode) {
        return qfalse;
    }
    if (cg.renderingThirdPerson) {
        return qfalse;
    }
    if (wolfcam_following  &&  wcg.clientNum != clientNum) {
        return qfalse;
    }

    if (clientNum != cg.snap->ps.clientNum) {
        return qfalse;
    }

#if 0
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		qfalse;
	}
#endif

    //FIXME free float spectator?

    return qtrue;
}

const char *CG_GetLocalTimeString (void)
{
    qtime_t now;
    static char nowString[128];
    int tm;
    int offset;
    qboolean timeAm = qfalse;

    offset = cg.time - cgs.levelStartTime;
	if (0) {  //(cgs.cpma) {
		tm = atoi(Info_ValueForKey(CG_ConfigStringNoConvert(672), "i"));
	} else {
		tm = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_levelStartTime"));
	}

    if (tm  &&  cg.demoPlayback  &&  cg_localTime.integer == 0) {
        if (offset > 0) {
            tm += (offset / 1000);
        }
        trap_RealTime(&now, qfalse, tm);
    } else {
        trap_RealTime(&now, qtrue, 0);
    }
    if (cg_localTimeStyle.integer == 0) {
        Com_sprintf(nowString, sizeof(nowString), "%02d:%02d (%s %d, %d)", now.tm_hour, now.tm_min, MonthAbbrev[now.tm_mon], now.tm_mday, 1900 + now.tm_year);
    } else {
		  if (now.tm_hour < 12) {
			  timeAm = qtrue;
			  if (now.tm_hour < 1) {
				  now.tm_hour = 12;
			  }
		  } else if (now.tm_hour >= 12) {
			  timeAm = qfalse;
			  if (now.tm_hour >= 13) {
				  now.tm_hour -= 12;
			  }
		  }
          Com_sprintf(nowString, sizeof(nowString), "%d:%02d%s (%s %d, %d)", now.tm_hour, now.tm_min, timeAm ? "am" : "pm", MonthAbbrev[now.tm_mon], now.tm_mday, 1900 + now.tm_year);
	  }

    return nowString;
}

// check version is at least this, or possibly not quakelive client
qboolean CG_CheckQlVersion (int n0, int n1, int n2, int n3)
{
	// ex:  0.1.0.432

	if (cgs.qlversion[0] > n0) {
		return qtrue;
	} else if (cgs.qlversion[0] < n0) {
		return qfalse;
	}

	// n0 == cgs.qlversion[0]
	if (cgs.qlversion[1] > n1) {
		return qtrue;
	} else if (cgs.qlversion[1] < n1) {
		return qfalse;
	}

	// n1 == cgs.qlversion[1]
	if (cgs.qlversion[2] > n2) {
		return qtrue;
	} else if (cgs.qlversion[2] < n2) {
		return qfalse;
	}

	// n2 == cgs.qlversion[2]
	if (cgs.qlversion[3] > n3) {
		return qtrue;
	} else if (cgs.qlversion[3] < n3) {
		return qfalse;
	}

	// n3 == cgs.qlversion[3]
	return qtrue;
}

// not freezetag
qboolean CG_EntityFrozen (centity_t *cent)
{
	qboolean entFreeze;

	entFreeze = qfalse;
	if (cent == &cg.predictedPlayerEntity) {
		if (cg.freezeEntity[cg.snap->ps.clientNum]) {
			entFreeze = qtrue;
		}
	} else {
		if (cg.freezeEntity[cent - cg_entities]) {
			entFreeze = qtrue;
		}
	}

	return entFreeze;
}

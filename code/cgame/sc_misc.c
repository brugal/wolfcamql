
#include "cg_local.h"

#include "sc.h"
#include "cg_main.h"
//#include "cg_q3mme_scripts.h"
//#include "cg_localents.h"
#include "cg_syscalls.h"
#include "wolfcam_predict.h"

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
				//Com_Printf("%f '%s' xx\n", v[c], s + i);
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

int SC_RedFromCvar (const vmCvar_t *cvar)
{
    int val;

    val = cvar->integer;

    if (val <= 0xffffff) {
        val &= 0xff0000;
        val /= 0x010000;
    } else {  // hex with alpha
        val &= 0xff000000;
        val /= 0x01000000;
    }

    return val;
}

int SC_GreenFromCvar (const vmCvar_t *cvar)
{
    int val;

    val = cvar->integer;

    if (val <= 0xffffff) {
        val &= 0x00ff00;
        val /= 0x000100;
    } else {  // hex with alpha
        val &= 0x00ff0000;
        val /= 0x00010000;
    }

    return val;
}

int SC_BlueFromCvar (const vmCvar_t *cvar)
{
    int val;

    val = cvar->integer;

    if (val <= 0xffffff) {
        val &= 0x0000ff;
        val /= 0x000001;
    } else {  // hex with alpha
        val &= 0x0000ff00;
        val /= 0x00000100;
    }

    return val;
}

int SC_AlphaFromCvar (const vmCvar_t *cvar)
{
    int val;

    val = cvar->integer;

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
void SC_Vec4ColorFromCvar (vec4_t *v, const vmCvar_t *cvar)
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
void SC_ByteVec3ColorFromCvar (byte *b, const vmCvar_t *cvar)
{
    b[0] = SC_RedFromCvar(cvar);
    b[1] = SC_GreenFromCvar(cvar);
    b[2] = SC_BlueFromCvar(cvar);
    //b[3] = SC_AlphaFromCvar(cvar);
    // vec4 doesn't work, incorrect values
}

void SC_Vec4ColorFromCvars (vec4_t color, const vmCvar_t *colorCvar, const vmCvar_t *alphaCvar)
{
    color[0] = (float)SC_RedFromCvar(colorCvar) / 255.f;
    color[1] = (float)SC_GreenFromCvar(colorCvar) / 255.f;
    color[2] = (float)SC_BlueFromCvar(colorCvar) / 255.f;
    color[3] = (float)alphaCvar->integer / 255.f;
}

void SC_Vec3ColorFromCvar (vec3_t color, const vmCvar_t *colorCvar)
{
    color[0] = (float)SC_RedFromCvar(colorCvar) / 255.f;
    color[1] = (float)SC_GreenFromCvar(colorCvar) / 255.f;
    color[2] = (float)SC_BlueFromCvar(colorCvar) / 255.f;
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

qboolean CG_IsEnemy (const clientInfo_t *ci)
{
    int ourClientNum;
    const clientInfo_t *us;

    if (wolfcam_following) {
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    us = &cgs.clientinfo[ourClientNum];

    if (us == ci) {
        return qfalse;
    }

	if (CG_IsTeamGame(cgs.gametype)  &&  ci->team != us->team) {
        return qtrue;
	} else if (!CG_IsTeamGame(cgs.gametype)) {
        return qtrue;
    }

    return qfalse;
}

qboolean CG_IsTeammate (const clientInfo_t *ci)
{
    int ourClientNum;
    const clientInfo_t *us;

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

qboolean CG_IsEnemyTeam (int team)
{
    int ourClientNum;
    const clientInfo_t *us;

    if (wolfcam_following) {
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    us = &cgs.clientinfo[ourClientNum];

    //if (us == ci) {
    //    return qfalse;
    //}

	if (CG_IsTeamGame(cgs.gametype)  &&  team != us->team) {
        return qtrue;
	} else if (!CG_IsTeamGame(cgs.gametype)) {
        return qtrue;
    }

    return qfalse;
}

qboolean CG_IsOurTeam (int team)
{
    int ourClientNum;
    const clientInfo_t *us;

    if (cgs.gametype < GT_TEAM) {
        return qfalse;
    }

    if (wolfcam_following) {
        ourClientNum = wcg.clientNum;
    } else {
        ourClientNum = cg.snap->ps.clientNum;
    }

    us = &cgs.clientinfo[ourClientNum];

    if (team != us->team) {
        return qfalse;
    }

    return qtrue;
}

qboolean CG_IsUs (const clientInfo_t *ci)
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

const char *CG_GetToken (const char *inputString, char *token, qboolean isFilename, qboolean *newLine)
{
    //int i;
    const char *p;
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


	//if (inputString >= EffectScripts.weapons[0]
    *newLine = qfalse;
    p = inputString;
    token[0] = '\0';

    if (p[0] == '\0') {  //  ||  (p[0] != '\t'  &&  (p[0] < ' '  ||  p[0] > '~'))) {
        //Com_Printf("xx\n");
        *newLine = qtrue;

#if 0
		if (jt  &&  !jt->valid) {
			jt->valid = qtrue;
			jt->end = p;
			Q_strncpyz(jt->token, tokenStart, sizeof(jt->token));
			jt->hitNewLine = *newLine;
		}
#endif
        //Com_Printf("CG_GetToken() '%s'\n", tokenOrig);
        return p;
    }

#if 0
    if (p[0] == '\0'  ||  (p[0] != '\t'  &&  (p[0] < ' '  ||  p[0] > '~'))) {
        Com_Printf("xx\n");
        *newLine = qtrue;

#if 0
		if (jt  &&  !jt->valid) {
			jt->valid = qtrue;
			jt->end = p;
			Q_strncpyz(jt->token, tokenStart, sizeof(jt->token));
			jt->hitNewLine = *newLine;
		}
#endif

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

        if (!isFilename  &&  (c == '/'  ||  c == '-'  ||  c == '+'  ||  c == '*'  ||  c  == '('  ||  c  == ')'  ||  c == ',')) {
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

const char *CG_GetTokenGameType (const char *inputString, char *token, qboolean isFilename, qboolean *newLine)
{
    //int i;
    const char *p;
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

const char *CG_GetTimeString (int ms)
{
	static char timeString[128];
	int minutes;
	int	seconds;
	int frac;

	if (cgs.gametype == GT_RACE) {
		if (ms < 0  ||  ms >= MAX_RACE_SCORE) {
			timeString[0] = '-';
			timeString[1] = '\0';
			return timeString;
		}
	}

	frac = ms % 1000;
	seconds = (ms / 1000) % 60;
	minutes = (ms / 1000 / 60);

	Com_sprintf(timeString, sizeof(timeString), "%d:%02d.%03d", minutes, seconds, frac);

	return timeString;
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
qboolean CG_EntityFrozen (const centity_t *cent)
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

qboolean CG_GameTimeout (void)
{
	qboolean inTimeout = qfalse;

	if (cgs.protocol == PROTOCOL_QL) {
		if (cgs.timeoutBeginTime) {
			if (!cgs.timeoutEndTime) {  // ref pause
				inTimeout = qtrue;
			} else if (cg.time < cgs.timeoutEndTime) {
				inTimeout = qtrue;
			}
		}
	} else if (cgs.cpma) {
		if (cg.time < cgs.timeoutEndTime) {
			inTimeout = qtrue;
		}
	}

	//FIXME osp

	return inTimeout;
}

//FIXME check for falling off ledge
//FIXME land and move
static void predict_player_position (const centity_t *cent, vec3_t result, float t)
{
	vec3_t velocity;
	float gravity = 800.0f;
	trace_t tr;

    //FIXME check that abs(velocity) > 20 ?
    //VectorCopy(ent->s.pos.trBase, result);
    //VectorCopy(cent->lerpOrigin, result);
	VectorCopy(cent->currentState.pos.trBase, result);
	// lerp
	t += (cg.ftime - cg.snap->serverTime) / 1000.0f;
	VectorCopy(cent->currentState.pos.trDelta, velocity);

    result[0] = result[0] + velocity[0] * t;
    result[1] = result[1] + velocity[1] * t;
	result[2] = result[2] + velocity[2] * t;
    if (cent->currentState.groundEntityNum == ENTITYNUM_NONE) {
        result[2] = result[2] + (0.5 * -gravity * t * t);
	}

	memset(&tr, 0, sizeof(tr));
	//CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, view, wcg.clientNum, MASK_SOLID );

    //FIXME check traces to see if player wont be stopped

	//FIXME bg_playerMins
	Wolfcam_WeaponTrace(&tr, cent->lerpOrigin, NULL, NULL, result, cent->currentState.number, CONTENTS_SOLID);  //FIXME CONTENTS_BODY for other players

	VectorCopy(tr.endpos, result);
}

#define MINVAL 0.000001f

static float rocket_flight_time (const vec3_t origin, const vec3_t end, float rocketSpeed)
{
	vec3_t dir;
	float dist;

	if (rocketSpeed <= 0.0) {
	    CG_Printf("^1rocket_flight_time() rocket speed (%f) <= 0.0\n", rocketSpeed);
		return -1;
    }

	VectorSubtract(end, origin, dir);
    dist = VectorLength (dir);
    dist -= 14.0;  // CalcMuzz
    dist -= 45.0;  // MISSILE_PRESTEP_TIME  //FIXME why isn't this 50?
    if (dist <= MINVAL) {
        dist = 0.0;
    }

	return (dist / rocketSpeed);
}
#undef MINVAL

//FIXME check that player velocity isn't higher than rocket

void CG_Rocket_Aim (const centity_t *enemy, vec3_t epos)
{
    float flightTime;
    int i;
    vec3_t ourPosEstimate, enemyPosEstimate;

	float rocketSpeed;
	int iterations;
	float maxError;
	float tError;
	qboolean debug;

	debug = qfalse;

	rocketSpeed = cgs.rocketSpeed;

	if (wolfcam_following) {
		//VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, ourPosEstimate);
		VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trBase, ourPosEstimate);
	} else {
		VectorCopy(cg.snap->ps.origin, ourPosEstimate);
	}

	VectorCopy(enemy->lerpOrigin, enemyPosEstimate);

	flightTime = rocket_flight_time(ourPosEstimate, enemyPosEstimate, rocketSpeed);
    predict_player_position(enemy, enemyPosEstimate, flightTime);

	//VectorCopy(enemyPosEstimate, epos);

	//VectorCopy(enemy->currentState.pos.trBase, enemyPosEstimate);
	//ftimeLast = 0;
	iterations = 40;  //40;  //200;  //40;
	maxError = 32.0f / rocketSpeed / 2.0f;  // 32 game units, about the player hit box
	for (i = 0;  i < iterations;  i ++) {
		float ftime;
		float ftimeNew;

		ftime = rocket_flight_time(ourPosEstimate, enemyPosEstimate, rocketSpeed);
		predict_player_position(enemy, enemyPosEstimate, ftime);

		ftimeNew = rocket_flight_time(ourPosEstimate, enemyPosEstimate, rocketSpeed);
		tError = ftimeNew - ftime;
		if (debug) {
			CG_Printf("%f  error %f\n", cg.ftime, tError);
		}

		if (tError < maxError  &&  tError > -maxError) {
			if (debug) {
				CG_Printf("^5got it.... %d  %f ***************\n", i, ftimeNew - ftime);
			}
			VectorCopy(enemyPosEstimate, epos);
			return;
		}

		if (ftimeNew > ftime) {
			// moving away from us
			if (debug) {
				Com_Printf("%f  moving away...\n", cg.ftime);
			}
			//flightTime *= (1.0 + (0.1 * (iterations - i)/iterations));
			ftime *= (1.0 + (0.1 * (iterations - i)/iterations));
		} else if (ftimeNew < ftime) {
			// moving towards us
			if (debug) {
				Com_Printf("%f  moving towards us\n", cg.ftime);
			}
			//flightTime *= (1.0 - (0.1 * (iterations - i)/iterations));
			ftime *= (1.0 - (0.1 * (iterations - i)/iterations));
		}
		//ftime = (ftime + ftimeNew) / 2.0f;

		predict_player_position(enemy, enemyPosEstimate, ftime);
		VectorCopy(enemyPosEstimate, epos);
	}


	if (debug) {
		CG_Printf("^1couldn't get accurate rocket aim cent %d  error %f  time %f\n", enemy->currentState.number, tError, flightTime);
		//trap_Cvar_Set("cl_freezeDemo", "1");
	}
	return;
}

// also checks if client is valid
qboolean CG_ClientInSnapshot (int clientNum)
{
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return qfalse;
	}

	if (!cg.snap) {
		return qfalse;
	}

	if (clientNum == cg.snap->ps.clientNum) {
		return qtrue;
	}

	if (cg_entities[clientNum].currentValid) {
		return qtrue;
	}

	return qfalse;
}

int CG_NumPlayers (void)
{
	int n;
	int i;
	const clientInfo_t *ci;

	n = 0;
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid) {
			continue;
		}
		if (ci->team == TEAM_SPECTATOR) {
			continue;
		}
		n++;
	}

	return n;
}

qboolean CG_ScoresEqual (int score1, int score2)
{
	if (cgs.gametype == GT_RACE) {
		if (score1 >= 0x7fffffff) {
			score1 = -1;
		}
		if (score2 >= 0x7fffffff) {
			score2 = -1;
		}
	}

	if (score1 == score2) {
		return qtrue;
	}

	return qfalse;
}

qboolean CG_CpmaIsRoundWarmup (void)
{
	int roundTime;
	const char *str;

	//FIXME ctfs
	if (cgs.gametype == GT_CA) {
		str = CG_ConfigStringNoConvert(CSCPMA_SCORES);
		roundTime = atoi(Info_ValueForKey(str, "tw"));
		//Com_Printf("^3round time %d\n", roundTime);
		if (roundTime > 0  &&  roundTime > cg.time) {
			//Com_Printf("^5round warmup\n");
			return qtrue;
		} else {
			// check game warmup
			str = CG_ConfigStringNoConvert(CSCPMA_GAMESTATE);
			roundTime = atoi(Info_ValueForKey(str, "tw"));
			if (roundTime != 0) {
				//Com_Printf("^2game warmup\n");
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
  mvd

  weapon info in ps.ammo[clientNum]
  health/armor in ps.powerups[clientNum]

  ----

  ps.powerups:

  0000 0000 0000 0000 0000 0000 0000 0000
  XXXX XXXX .... .... .... .... .... ....  health
  .... .... XXXX XXXX .... .... .... ....  armor
  .... .... .... .... XXXX XXXX XXXX XXXX  damage done

  ----

  ps.ammo:

  0000 0000 0000 0000 0000 0000 0000 0000
  .... .... .... .... .... .... XXXX XXXX  ammo, 255 represents infinite?
  .... .... XXXX XXXX XXXX XXXX .... ....  weapons held in duel (others like
                                           ffa?)
  .... .... .... .... ..xx xxxx .... ....  location in team games

 */
qboolean CG_IsCpmaMvd (void)
{
	if (cgs.protocol != PROTOCOL_Q3  ||  !cgs.cpma) {
		return qfalse;
	}

	if (!cg.snap) {
		return qfalse;
	}

	if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
		return qfalse;
	}

	if (cg.snap->ps.pm_type == PM_FREEZE) {
		return qtrue;
	}

	return qfalse;
}

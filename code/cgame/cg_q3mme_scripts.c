#include "cg_local.h"

#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_mem.h"
#include "cg_predict.h"
#include "cg_q3mme_scripts.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "sc.h"

//FIXME epsilon for float comaparisons
//FIXME special float values
//FIXME shaderlist

// can't use pc token fx scripts don't all have filenames quoted

#define MAX_COUNT 1000  //FIXME take out

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
#define OP_FINWATER 27

#define OP_FUNCFIRST OP_FSQRT
#define OP_FUNCLAST OP_FINWATER


typedef enum {
	FX_MODEL_DIR,
	FX_MODEL_ANGLES,
	FX_MODEL_AXIS,
} fxModelType_t;

ScriptVars_t ScriptVars;

static int parsingEmitterScriptLevel = 0;
qboolean EmitterScript = qfalse;
qboolean PlainScript = qfalse;  //FIXME hack
qboolean DistanceScript = qfalse;

static qboolean ScriptJustParsing = qfalse;  //FIXME more hack
effectScripts_t EffectScripts;

static int EmittedEntities = 0;

//static qboolean WithinComment = qfalse;

enum {
	// runscript()

    TOKEN_NOID = 0,
    TOKEN_RED,
    TOKEN_GREEN,
    TOKEN_BLUE,
    TOKEN_COLOR,
    TOKEN_ALPHA,
    TOKEN_SHADER,
    TOKEN_MODEL,
    TOKEN_SIZE,
    TOKEN_WIDTH,
    TOKEN_ANGLE,
    TOKEN_T0,
    TOKEN_T1,
    TOKEN_T2,
    TOKEN_T3,
    TOKEN_T4,
    TOKEN_T5,
    TOKEN_T6,
    TOKEN_T7,
    TOKEN_T8,
    TOKEN_T9,
    TOKEN_VELOCITY,
    TOKEN_VELOCITY0,
    TOKEN_VELOCITY1,
    TOKEN_VELOCITY2,
    TOKEN_ORIGIN,
    TOKEN_ORIGIN0,
    TOKEN_ORIGIN1,
    TOKEN_ORIGIN2,
    TOKEN_END,
    TOKEN_END0,
    TOKEN_END1,
    TOKEN_END2,
    TOKEN_DIR,
    TOKEN_DIR0,
    TOKEN_DIR1,
    TOKEN_DIR2,
    TOKEN_ANGLES,
    TOKEN_ANGLES0,
    TOKEN_ANGLES1,
    TOKEN_ANGLES2,
    TOKEN_V0,
    TOKEN_V00,
    TOKEN_V01,
    TOKEN_V02,
    TOKEN_V1,
    TOKEN_V10,
    TOKEN_V11,
    TOKEN_V12,
    TOKEN_V2,
    TOKEN_V20,
    TOKEN_V21,
    TOKEN_V22,
    TOKEN_V3,
    TOKEN_V30,
    TOKEN_V31,
    TOKEN_V32,
    TOKEN_V4,
    TOKEN_V40,
    TOKEN_V41,
    TOKEN_V42,
    TOKEN_V5,
    TOKEN_V50,
    TOKEN_V51,
    TOKEN_V52,
    TOKEN_V6,
    TOKEN_V60,
    TOKEN_V61,
    TOKEN_V62,
    TOKEN_V7,
    TOKEN_V70,
    TOKEN_V71,
    TOKEN_V72,
    TOKEN_V8,
    TOKEN_V80,
    TOKEN_V81,
    TOKEN_V82,
    TOKEN_V9,
    TOKEN_V90,
    TOKEN_V91,
    TOKEN_V92,
    TOKEN_SCALE,
    TOKEN_ADD,
    TOKEN_ADDSCALE,
    TOKEN_SUB,
    TOKEN_SUBSCALE,
    TOKEN_COPY,
    TOKEN_CLEAR,
    TOKEN_WOBBLE,
    TOKEN_RANDOM,
    TOKEN_NORMALIZE,
    TOKEN_PERPENDICULAR,
    TOKEN_CROSS,
    TOKEN_SPRITE,
    TOKEN_SPARK,
    TOKEN_QUAD,
    TOKEN_BEAM,
    TOKEN_LIGHT,
    TOKEN_DISTANCE,
    TOKEN_EMITTER,
    TOKEN_EMITTERF,
    TOKEN_ALPHAFADE,
	TOKEN_CULLDISTANCEVALUE,
    TOKEN_SHADERTIME,
    TOKEN_DIRMODEL,
    TOKEN_LOOPSOUND,
    TOKEN_ROTATE,
    TOKEN_VIBRATE,
    TOKEN_EMITTERID,
    TOKEN_DECAL,
    TOKEN_SOUND,
    TOKEN_SOUNDLOCAL,
    TOKEN_SOUNDWEAPON,
    TOKEN_ANGLESMODEL,
    TOKEN_AXISMODEL,
    TOKEN_INTERVAL,
    TOKEN_COLORFADE,
    TOKEN_PUSHPARENT,
    TOKEN_POP,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_REPEAT,
    TOKEN_ROTATEAROUND,
    TOKEN_PARENTORIGIN,
    TOKEN_MOVEGRAVITY,
    TOKEN_SOUNDLIST,
    TOKEN_SOUNDLISTLOCAL,
    TOKEN_SOUNDLISTWEAPON,
    TOKEN_SHADERCLEAR,
    TOKEN_EXTRASHADER,
    TOKEN_EXTRASHADERCLEAR,
	TOKEN_EXTRASHADERENDTIME,
    TOKEN_TRACE,
    TOKEN_DECALTEMP,
    TOKEN_MODELLIST,
    TOKEN_SHADERLIST,
    TOKEN_YAW,
    TOKEN_PITCH,
    TOKEN_ROLL,
    TOKEN_MOVEBOUNCE,
    TOKEN_SINK,
    TOKEN_IMPACT,
    TOKEN_PARENTVELOCITY,
    TOKEN_INVERSE,
    TOKEN_RINGS,
    TOKEN_ECHO,
    TOKEN_RETURN,
    TOKEN_CONTINUE,
    TOKEN_COMMAND,
	TOKEN_ANIMFRAME,

    ////////////

	// math
	TOKEN_NUMBER,


	//TOKEN_PARENTORIGIN,
	TOKEN_PARENTORIGIN0,

	TOKEN_PARENTORIGIN1,
	TOKEN_PARENTORIGIN2,
	//TOKEN_PARENTVELOCITY,
	TOKEN_PARENTVELOCITY0,
	TOKEN_PARENTVELOCITY1,
	TOKEN_PARENTVELOCITY2,
	TOKEN_PARENTANGLES,
	TOKEN_PARENTANGLES0,
	TOKEN_PARENTANGLES1,
	TOKEN_PARENTANGLES2,
	TOKEN_PARENTANGLE,
	TOKEN_PARENTYAW,
	TOKEN_PARENTPITCH,
	TOKEN_PARENTROLL,
	TOKEN_PARENTSIZE,
	TOKEN_PARENTDIR,
	TOKEN_PARENTDIR0,
	TOKEN_PARENTDIR1,
	TOKEN_PARENTDIR2,
	TOKEN_PARENTEND,
	TOKEN_PARENTEND0,
	TOKEN_PARENTEND1,
	TOKEN_PARENTEND2,

	TOKEN_TIME,
	TOKEN_CGTIME,
	TOKEN_LOOP,
	TOKEN_LOOPCOUNT,
	TOKEN_PI,
	TOKEN_RAND,
	TOKEN_CRAND,
	TOKEN_LERP,
	TOKEN_LIFE,
	TOKEN_CLIENTNUM,
	TOKEN_ENEMY,
	TOKEN_TEAMMATE,
	TOKEN_INEYES,
	TOKEN_TEAM,
	TOKEN_SURFACETYPE,
	TOKEN_GAMETYPE,
	TOKEN_INWATER,

	TOKEN_MATH_MULT,
	TOKEN_MATH_DIV,
	TOKEN_MATH_MINUS,
	TOKEN_MATH_PLUS,
	TOKEN_MATH_MOD,
	TOKEN_MATH_LESS,
	TOKEN_MATH_GREATER,
	TOKEN_MATH_NOT,
	TOKEN_MATH_EQUAL,
	TOKEN_MATH_AND,
	TOKEN_MATH_OR,

	TOKEN_MATH_SQRT,
	TOKEN_MATH_CEIL,
	TOKEN_MATH_FLOOR,
	TOKEN_MATH_SIN,
	TOKEN_MATH_COS,
	TOKEN_MATH_TAN,
	TOKEN_MATH_WAVE,
	TOKEN_MATH_CLIP,
	TOKEN_MATH_ASIN,
	TOKEN_MATH_ACOS,
	TOKEN_MATH_ATAN,
	TOKEN_MATH_ATAN2,
	TOKEN_MATH_POW,

	//

	// vectors
	//FIXME

    TOKEN_FIXME,
};

typedef struct {  // fxToken_s {
	const char *name;
	int id;
} fxToken_t;


static fxToken_t fxTokens[] = {
    { NULL, TOKEN_NOID },

    { "red", TOKEN_RED },
    { "green", TOKEN_GREEN },
    { "blue", TOKEN_BLUE },
    { "color", TOKEN_COLOR },
    { "alpha", TOKEN_ALPHA },
    { "shader", TOKEN_SHADER },
    { "model", TOKEN_MODEL },
    { "size", TOKEN_SIZE },
    { "width", TOKEN_WIDTH },
    { "angle", TOKEN_ANGLE },
    { "t0", TOKEN_T0 },
    { "t1", TOKEN_T1 },
    { "t2", TOKEN_T2 },
    { "t3", TOKEN_T3 },
    { "t4", TOKEN_T4 },
    { "t5", TOKEN_T5 },
    { "t6", TOKEN_T6 },
    { "t7", TOKEN_T7 },
    { "t8", TOKEN_T8 },
    { "t9", TOKEN_T9 },
    { "velocity", TOKEN_VELOCITY },
    { "velocity0", TOKEN_VELOCITY0 },
    { "velocity1", TOKEN_VELOCITY1 },
    { "velocity2", TOKEN_VELOCITY2 },
    { "origin", TOKEN_ORIGIN },
    { "origin0", TOKEN_ORIGIN0 },
    { "origin1", TOKEN_ORIGIN1 },
    { "origin2", TOKEN_ORIGIN2 },
    { "end", TOKEN_END },
    { "end0", TOKEN_END0 },
    { "end1", TOKEN_END1 },
    { "end2", TOKEN_END2 },
    { "dir", TOKEN_DIR },
    { "dir0", TOKEN_DIR0 },
    { "dir1", TOKEN_DIR1 },
    { "dir2", TOKEN_DIR2 },
    { "angles", TOKEN_ANGLES },
    { "angles0", TOKEN_ANGLES0 },
    { "angles1", TOKEN_ANGLES1 },
    { "angles2", TOKEN_ANGLES2 },
    { "v0", TOKEN_V0 },
    { "v00", TOKEN_V00 },
    { "v01", TOKEN_V01 },
    { "v02", TOKEN_V02 },
    { "v1", TOKEN_V1 },
    { "v10", TOKEN_V10 },
    { "v11", TOKEN_V11 },
    { "v12", TOKEN_V12 },
    { "v2", TOKEN_V2 },
    { "v20", TOKEN_V20 },
    { "v21", TOKEN_V21 },
    { "v22", TOKEN_V22 },
    { "v3", TOKEN_V3 },
    { "v30", TOKEN_V30 },
    { "v31", TOKEN_V31 },
    { "v32", TOKEN_V32 },
    { "v4", TOKEN_V4 },
    { "v40", TOKEN_V40 },
    { "v41", TOKEN_V41 },
    { "v42", TOKEN_V42 },
    { "v5", TOKEN_V5 },
    { "v50", TOKEN_V50 },
    { "v51", TOKEN_V51 },
    { "v52", TOKEN_V52 },
    { "v6", TOKEN_V6 },
    { "v60", TOKEN_V60 },
    { "v61", TOKEN_V61 },
    { "v62", TOKEN_V62 },
    { "v7", TOKEN_V7 },
    { "v70", TOKEN_V70 },
    { "v71", TOKEN_V71 },
    { "v72", TOKEN_V72 },
    { "v8", TOKEN_V8 },
    { "v80", TOKEN_V80 },
    { "v81", TOKEN_V81 },
    { "v82", TOKEN_V82 },
    { "v9", TOKEN_V9 },
    { "v90", TOKEN_V90 },
    { "v91", TOKEN_V91 },
    { "v92", TOKEN_V92 },
    { "scale", TOKEN_SCALE },
    { "add", TOKEN_ADD },
    { "addscale", TOKEN_ADDSCALE },
    { "sub", TOKEN_SUB },
    { "subscale", TOKEN_SUBSCALE },
    { "copy", TOKEN_COPY },
    { "clear", TOKEN_CLEAR },
    { "wobble", TOKEN_WOBBLE },
    { "random", TOKEN_RANDOM },
    { "normalize", TOKEN_NORMALIZE },
    { "perpendicular", TOKEN_PERPENDICULAR },
    { "cross", TOKEN_CROSS },
    { "sprite", TOKEN_SPRITE },
    { "spark", TOKEN_SPARK },
    { "quad", TOKEN_QUAD },
    { "beam", TOKEN_BEAM },
    { "light", TOKEN_LIGHT },
    { "distance", TOKEN_DISTANCE },
    { "emitter", TOKEN_EMITTER },
    { "emitterf", TOKEN_EMITTERF },
    { "alphafade", TOKEN_ALPHAFADE },
	{ "culldistancevalue", TOKEN_CULLDISTANCEVALUE },
    { "shadertime", TOKEN_SHADERTIME },
    { "dirmodel", TOKEN_DIRMODEL },
    { "loopsound", TOKEN_LOOPSOUND },
    { "rotate", TOKEN_ROTATE },
    { "vibrate", TOKEN_VIBRATE },
    { "emitterid", TOKEN_EMITTERID },
    { "decal", TOKEN_DECAL },
    { "sound", TOKEN_SOUND },
    { "soundlocal", TOKEN_SOUNDLOCAL },
    { "soundweapon", TOKEN_SOUNDWEAPON },
    { "anglesmodel", TOKEN_ANGLESMODEL },
    { "axismodel", TOKEN_AXISMODEL },
    { "interval", TOKEN_INTERVAL },
    { "colorfade", TOKEN_COLORFADE },
    { "pushparent", TOKEN_PUSHPARENT },
    { "pop", TOKEN_POP },
    { "if", TOKEN_IF },
    { "else", TOKEN_ELSE },
    { "elif", TOKEN_ELIF },
    { "repeat", TOKEN_REPEAT },
    { "rotatearound", TOKEN_ROTATEAROUND },
    { "parentorigin", TOKEN_PARENTORIGIN },
    { "movegravity", TOKEN_MOVEGRAVITY },
    { "soundlist", TOKEN_SOUNDLIST },
    { "soundlistlocal", TOKEN_SOUNDLISTLOCAL },
    { "soundlistweapon", TOKEN_SOUNDLISTWEAPON },
    { "shaderclear", TOKEN_SHADERCLEAR },
    { "extrashader", TOKEN_EXTRASHADER },
    { "extrashaderendtime", TOKEN_EXTRASHADERENDTIME },
    { "extrashaderclear", TOKEN_EXTRASHADERCLEAR },
    { "trace", TOKEN_TRACE },
    { "decaltemp", TOKEN_DECALTEMP },
    { "modellist", TOKEN_MODELLIST },
    { "shaderlist", TOKEN_SHADERLIST },
    { "yaw", TOKEN_YAW },
    { "pitch", TOKEN_PITCH },
    { "roll", TOKEN_ROLL },
    { "movebounce", TOKEN_MOVEBOUNCE },
    { "sink", TOKEN_SINK },
    { "impact", TOKEN_IMPACT },
    { "parentvelocity", TOKEN_PARENTVELOCITY },
    { "inverse", TOKEN_INVERSE },
    { "rings", TOKEN_RINGS },
    { "echo", TOKEN_ECHO },
    { "return", TOKEN_RETURN },
    { "continue", TOKEN_CONTINUE },
    { "command", TOKEN_COMMAND },
	{ "animframe", TOKEN_ANIMFRAME },

	// math

	{ "parentOrigin", TOKEN_PARENTORIGIN },
	{ "parentOrigin0", TOKEN_PARENTORIGIN0 },
	{ "parentOrigin1", TOKEN_PARENTORIGIN1 },
	{ "parentOrigin2", TOKEN_PARENTORIGIN2 },
	{ "parentVelocity", TOKEN_PARENTVELOCITY },
	{ "parentVelocity0", TOKEN_PARENTVELOCITY0 },
	{ "parentVelocity1", TOKEN_PARENTVELOCITY1 },
	{ "parentVelocity2", TOKEN_PARENTVELOCITY2 },
	{ "parentAngles", TOKEN_PARENTANGLES },
	{ "parentAngles0", TOKEN_PARENTANGLES0 },
	{ "parentAngles1", TOKEN_PARENTANGLES1 },
	{ "parentAngles2", TOKEN_PARENTANGLES2 },
	{ "parentangle", TOKEN_PARENTANGLE },
	{ "parentyaw", TOKEN_PARENTYAW },
	{ "parentpitch", TOKEN_PARENTPITCH },
	{ "parentroll", TOKEN_PARENTROLL },
	{ "parentsize", TOKEN_PARENTSIZE },
	{ "parentDir", TOKEN_PARENTDIR },
	{ "parentDir0", TOKEN_PARENTDIR0 },
	{ "parentDir1", TOKEN_PARENTDIR1 },
	{ "parentDir2", TOKEN_PARENTDIR2 },
	{ "parentEnd", TOKEN_PARENTEND },
	{ "parentEnd0", TOKEN_PARENTEND0 },
	{ "parentEnd1", TOKEN_PARENTEND1 },
	{ "parentEnd2", TOKEN_PARENTEND2 },

	{ "time", TOKEN_TIME },
	{ "cgtime", TOKEN_CGTIME },
	{ "loop", TOKEN_LOOP },
	{ "loopcount", TOKEN_LOOPCOUNT },
	{ "pi", TOKEN_PI },
	{ "rand", TOKEN_RAND },
	{ "crand", TOKEN_CRAND },
	{ "lerp", TOKEN_LERP },
	{ "life", TOKEN_LIFE },
	{ "clientnum", TOKEN_CLIENTNUM },
	{ "enemy", TOKEN_ENEMY },
	{ "teammate", TOKEN_TEAMMATE },
	{ "ineyes", TOKEN_INEYES },
	{ "surfacetype", TOKEN_SURFACETYPE },
	{ "gametype", TOKEN_GAMETYPE },
	{ "inwater", TOKEN_INWATER },

	{ "*", TOKEN_MATH_MULT },
	{ "/", TOKEN_MATH_DIV },
	{ "-", TOKEN_MATH_MINUS },
	{ "+", TOKEN_MATH_PLUS },
	{ "%", TOKEN_MATH_MOD },
	{ "<", TOKEN_MATH_LESS },
	{ ">", TOKEN_MATH_GREATER },
	{ "!", TOKEN_MATH_NOT },
	{ "=", TOKEN_MATH_EQUAL },
	{ "&", TOKEN_MATH_AND },
	{ "|", TOKEN_MATH_OR },

	{ "sqrt", TOKEN_MATH_SQRT },
	{ "ceil", TOKEN_MATH_CEIL },
	{ "floor", TOKEN_MATH_FLOOR },
	{ "sin", TOKEN_MATH_SIN },
	{ "cos", TOKEN_MATH_COS },
	{ "wave", TOKEN_MATH_WAVE },
	{ "clip", TOKEN_MATH_CLIP },
	{ "acos", TOKEN_MATH_ACOS },
	{ "asin", TOKEN_MATH_ASIN },
	{ "atan", TOKEN_MATH_ATAN },
	{ "atan2", TOKEN_MATH_ATAN2 },
	{ "pow", TOKEN_MATH_POW },

};

static const char *PrintShort (const char *s, int len)
{
	static char buffer[1024];
	int n;

	if (len >= sizeof(buffer)) {
		n = sizeof(buffer);
	} else {
		n = len;
	}

	Q_strncpyz(buffer, s, n);

	return buffer;
}

static const char *CG_GetFxTokenExt (const char *inputString, char *token, qboolean isFilename, qboolean *newLine, int *tokenId, float *tokenValue)
{
	int jitIndex;
	jitFxToken_t *jt;
	const char *ret;
	qboolean canCompile;

	if (!inputString) {
		Com_Printf("CG_GetFxTokenExt() inputString == NULL\n");
		return NULL;
	}

	canCompile = qfalse;
	jitIndex = (int)(inputString - EffectScripts.weapons[0].fireScript);
	if (cg_fxCompiled.integer  &&  jitIndex >= 0  &&  inputString < (const char *)&EffectScripts.names[0])  {  // hack
		jt = EffectScripts.jitToken[jitIndex];
		canCompile = qtrue;
	} else {
		jt = NULL;
	}

	//Com_Printf(" -- CG_GetFxTokenExt() '%s'\n", PrintShort(inputString, 8));

	if (jt  &&  jt->valid) {
		if (1) {  //(!jt->tokenId) {
			//Q_strncpyz(token, jt->token, sizeof(jt->token));  // argh...
			//Q_strcpyz(token, jt->token);
			strcpy(token, jt->token);
		}
		//Com_Printf("^5jit '%s'\n", token);
		*newLine = jt->hitNewLine;
		*tokenId = jt->tokenId;
		*tokenValue = jt->value;
		return jt->end;
	}

	ret = CG_GetToken(inputString, token, isFilename, newLine);
	*tokenId = 0;

	if (cg_fxCompiled.integer > 1) {
		Com_Printf("^%cCG_GetFxTokenExt() token '%s' %s %d in range %d\n", inputString < (const char *)&EffectScripts.names[0] ? '6' : '3', token, inputString < (const char *)&EffectScripts.names[0] ? "compiled" : "", jitIndex, inputString < (const char *)&EffectScripts.names[0]);
	}

	//if (jt  &&  !jt->valid) {
	if (canCompile) {
		int i;

		EffectScripts.jitToken[jitIndex] = CG_CallocMem(sizeof(jitFxToken_t));
		jt = EffectScripts.jitToken[jitIndex];
		if (!jt) {
			Com_Printf("^1couldn't allocate memory for jitFxToken_t\n");
			return ret;
		}
		jt->valid = qtrue;
		jt->end = ret;
		//Q_strncpyz(jt->token, token, sizeof(jt->token));
		jt->token = String_Alloc(token);
		//Com_Printf("^3jttoken '%s'\n", jt->token);
		jt->hitNewLine = *newLine;

		if (token[0] == '.'  ||  (token[0] >= '0'  &&  token[0] <= '9')) {
			jt->tokenId = TOKEN_NUMBER;
			jt->value = atof(token);
		} else {
			for (i = 0;  i < ARRAY_LEN(fxTokens);  i++) {
				if (fxTokens[i].name == NULL) {
					continue;
				}
				if (!Q_stricmp(token, fxTokens[i].name)) {
					jt->tokenId = fxTokens[i].id;
					break;
				}
			}
		}
	}

	return ret;
}

void CG_FreeFxJitTokens (void)
{
	int i;
	size_t s;
	jitFxToken_t *j;

	s = 0;

	for (i = 0;  i < ARRAY_LEN(EffectScripts.jitToken);  i++) {
		j = EffectScripts.jitToken[i];
		if (j == NULL) {
			continue;
		}

		s += sizeof(jitFxToken_t);
		CG_FreeMem(j);
		EffectScripts.jitToken[i] = NULL;
	}

	Com_Printf("freed %f mb for jit fx tokens\n", s / (1024.0 * 1024.0));
}

static const char *CG_GetFxToken (const char *inputString, char *token, qboolean isFilename, qboolean *newLine, int *tokenId)
{
	float tokenValue;

	return CG_GetFxTokenExt(inputString, token, isFilename, newLine, tokenId, &tokenValue);
}

static localEntity_t *CG_AllocFxEntity (float id)
{
	localEntity_t *le;

	le = CG_AllocLocalEntity();
	if (id < 0.0) {
		CG_MakeLowPriorityEntity(le);
	}

	return le;
}

#if 0  //def CGAMESO
#include <sys/time.h>

int64_t QstrcmpCount = 0;

static int Q_stricmpt (const char *s1, const char *s2)
{
	struct timeval start, end;
	int r;
	int64_t startf, endf;

	//QstrcmpCount++;

#if 0
	if (EmitterScript) {
		Com_Printf("stricmp: '%s'  '%s'\n", s1, s2);
	}
#endif

	//return Q_stricmp(s1, s2);

	gettimeofday(&start, NULL);
	//1000000
	startf = start.tv_sec * 1000000.0 + start.tv_usec;
	r = Q_stricmp(s1, s2);

	gettimeofday(&end, NULL);
	endf = end.tv_sec * 1000000.0 + end.tv_usec;
	//Com_Printf("tt %lld\n", endf - startf);
	QstrcmpCount += endf - startf;

	return r;
}

#else

int QstrcmpCount = 0;
static int Q_stricmpt (const char *s1, const char *s2)
{
	QstrcmpCount++;

	return Q_stricmp(s1, s2);
	//return strcmp(s1, s2);
}

#endif

#define MAX_OPS 256
static const char *CG_Q3mmeMathExt (const char *script, const char *end, float *val, int *error, qboolean checkCompiled)
{
    char token[MAX_QPATH];
    //char buffer[256];  //FIXME
    const char *p;
    //double ops[MAX_OPS];
	float ops[MAX_OPS];
    int numOps;
    int i;
    int j;
    int err;
    static int recursiveCount = 0;
    static int uniqueId = 0;
    qboolean newLine;
    int verbose;
	int tokenId = 0;
	float tokenValue;
	//const char *oo;

	//oo = script;

    recursiveCount++;
    uniqueId++;  // = rand();
    verbose = qfalse;  //SC_Cvar_Get_Int("debug_math");
    //verbose = SC_Cvar_Get_Int("debug_math");

    //Com_Printf("math(%d %d): '%s'\n", recursiveCount, uniqueId, script);

    numOps = 0;
    while (1) {
		const char *scriptStart;

		if (end  &&  script >= end) {
			break;
		}

		scriptStart = script;

		if (checkCompiled) {
			script = CG_GetFxTokenExt(script, token, qfalse, &newLine, &tokenId, &tokenValue);
		} else {
			script = CG_GetToken(script, token, qfalse, &newLine);
			tokenId = 0;
		}

		if (end  &&  script > end) {
			//Com_Printf("went past end: %p  here: %p\n", end, script);
			script = scriptStart;
			break;
		}

        if (verbose) {
            //Com_Printf("token: '%s'  script[0] '%c' %d  newLine:%d\n", token, script[0], script[0], newLine);
            Com_Printf("math token: '%s'  script[0] %d  newLine:%d\n", token, script[0], newLine);
        }

        if (token[0] == '\0'  ||  token[0] == '{') {  //  ||  newLine)  {  // ||  end) {
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

                if (parenCount == 0  ||  p[0] == '\0'  ||  p[0] == '\n'  ||  (p[0] < ' '  ||  p[0] > '~')) {  //  ||  (end  &&  p >= end)) {
					//int sn;

					//FIXME just testing old scripts, take out
#if 0
					sn = (int)(p - script);
					if (sn >= sizeof(buffer)) {
						Com_Printf("^3CG_Q3mmeMathExt() open paren temp buffer exceeded:  %d >= %d\n", sn, sizeof(buffer));
					}
#endif


#if 0  // old way
					sn = (int)(p - script);
					if (sn >= sizeof(buffer)) {
						CG_Printf("^1CG_Q3mmeMathExt() open paren temp buffer exceeded:  %d >= %d\n", sn, sizeof(buffer));
						*error = err;
						recursiveCount--;
						return script;
					}
                    memcpy(buffer, script, p - script);
                    buffer[p - script] = '\0';
                    //Com_Printf("paren(%d): %d '%s'\n", recursiveCount + 1, p - script, buffer);
                    err = 0;
                    CG_Q3mmeMath(buffer, &tmpVal, &err);
#endif

#if 1
					err = 0;
					CG_Q3mmeMathExt(script, p, &tmpVal, &err, checkCompiled);
					//CG_Q3mmeMathExt(script, p, &tmpVal, &err, checkCompiled);
#endif
                    if (err) {
                        *error = err;
                        recursiveCount--;
                        return script;
                    }

					if (numOps + 2 >= MAX_OPS) {
						CG_Printf("^3CG_Q3mmeMathExt() max ops (%d) in parenthesis parse\n", MAX_OPS);
						script = p + 1;
						break;
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


		if (tokenId) {
			switch (tokenId) {
			case TOKEN_NUMBER:
				ops[numOps] = OP_VAL;
				numOps++;
				ops[numOps] = tokenValue;
				numOps++;
				goto handledToken;
				break;

			case TOKEN_PARENTORIGIN:
				goto parentorigintoken;
				break;
			case TOKEN_PARENTORIGIN0:
				goto parentorigin0token;
				break;
			case TOKEN_PARENTORIGIN1:
				goto parentorigin1token;
				break;
			case TOKEN_PARENTORIGIN2:
				goto parentorigin2token;
				break;
			case TOKEN_PARENTVELOCITY:
				goto parentvelocitytoken;
				break;
			case TOKEN_PARENTVELOCITY0:
				goto parentvelocity0token;
				break;
			case TOKEN_PARENTVELOCITY1:
				goto parentvelocity1token;
				break;
			case TOKEN_PARENTVELOCITY2:
				goto parentvelocity2token;
				break;
			case TOKEN_PARENTANGLES:
				goto parentanglestoken;
				break;
			case TOKEN_PARENTANGLES0:
				goto parentangles0token;
				break;
			case TOKEN_PARENTANGLES1:
				goto parentangles1token;
				break;
			case TOKEN_PARENTANGLES2:
				goto parentangles2token;
				break;
			case TOKEN_PARENTANGLE:
				goto parentangletoken;
				break;
			case TOKEN_PARENTYAW:
				goto parentyawtoken;
				break;
			case TOKEN_PARENTPITCH:
				goto parentpitchtoken;
				break;
			case TOKEN_PARENTROLL:
				goto parentrolltoken;
				break;
			case TOKEN_PARENTSIZE:
				goto parentsizetoken;
				break;
			case TOKEN_PARENTDIR:
				goto parentdirtoken;
				break;
			case TOKEN_PARENTDIR0:
				goto parentdir0token;
				break;
			case TOKEN_PARENTDIR1:
				goto parentdir1token;
				break;
			case TOKEN_PARENTDIR2:
				goto parentdir2token;
				break;
			case TOKEN_PARENTEND:
				goto parentendtoken;
				break;
			case TOKEN_PARENTEND0:
				goto parentend0token;
				break;
			case TOKEN_PARENTEND1:
				goto parentend1token;
				break;
			case TOKEN_PARENTEND2:
				goto parentend2token;
				break;


			/////////////////////////

			case TOKEN_T0:
				goto t0token;
				break;
			case TOKEN_T1:
				goto t1token;
				break;
			case TOKEN_T2:
				goto t2token;
				break;
			case TOKEN_T3:
				goto t3token;
				break;
			case TOKEN_T4:
				goto t4token;
				break;
			case TOKEN_T5:
				goto t5token;
				break;
			case TOKEN_T6:
				goto t6token;
				break;
			case TOKEN_T7:
				goto t7token;
				break;
			case TOKEN_T8:
				goto t8token;
				break;
			case TOKEN_T9:
				goto t9token;
				break;
			case TOKEN_V0:
				goto v0token;
				break;
			case TOKEN_V00:
				goto v00token;
				break;
			case TOKEN_V01:
				goto v01token;
				break;
			case TOKEN_V02:
				goto v02token;
				break;
			case TOKEN_V1:
				goto v1token;
				break;
			case TOKEN_V10:
				goto v10token;
				break;
			case TOKEN_V11:
				goto v11token;
				break;
			case TOKEN_V12:
				goto v12token;
				break;
			case TOKEN_V2:
				goto v2token;
				break;
			case TOKEN_V20:
				goto v20token;
				break;
			case TOKEN_V21:
				goto v21token;
				break;
			case TOKEN_V22:
				goto v22token;
				break;
			case TOKEN_V3:
				goto v3token;
				break;
			case TOKEN_V30:
				goto v30token;
				break;
			case TOKEN_V31:
				goto v31token;
				break;
			case TOKEN_V32:
				goto v32token;
				break;
			case TOKEN_V4:
				goto v4token;
				break;
			case TOKEN_V40:
				goto v40token;
				break;
			case TOKEN_V41:
				goto v41token;
				break;
			case TOKEN_V42:
				goto v42token;
				break;
			case TOKEN_V5:
				goto v5token;
				break;
			case TOKEN_V50:
				goto v50token;
				break;
			case TOKEN_V51:
				goto v51token;
				break;
			case TOKEN_V52:
				goto v52token;
				break;
			case TOKEN_V6:
				goto v6token;
				break;
			case TOKEN_V60:
				goto v60token;
				break;
			case TOKEN_V61:
				goto v61token;
				break;
			case TOKEN_V62:
				goto v62token;
				break;
			case TOKEN_V7:
				goto v7token;
				break;
			case TOKEN_V70:
				goto v70token;
				break;
			case TOKEN_V71:
				goto v71token;
				break;
			case TOKEN_V72:
				goto v72token;
				break;
			case TOKEN_V8:
				goto v8token;
				break;
			case TOKEN_V80:
				goto v80token;
				break;
			case TOKEN_V81:
				goto v81token;
				break;
			case TOKEN_V82:
				goto v82token;
				break;
			case TOKEN_V9:
				goto v9token;
				break;
			case TOKEN_V90:
				goto v90token;
				break;
			case TOKEN_V91:
				goto v91token;
				break;
			case TOKEN_V92:
				goto v92token;
				break;

			case TOKEN_END:
				goto endtoken;
				break;
			case TOKEN_END0:
				goto end0token;
				break;
			case TOKEN_END1:
				goto end1token;
				break;
			case TOKEN_END2:
				goto end2token;
				break;

			case TOKEN_ANGLES:
				goto anglestoken;
				break;
			case TOKEN_ANGLES0:
				goto angles0token;
				break;
			case TOKEN_ANGLES1:
				goto angles1token;
				break;
			case TOKEN_ANGLES2:
				goto angles2token;
				break;

			case TOKEN_DIR:
				goto dirtoken;
				break;
			case TOKEN_DIR0:
				goto dir0token;
				break;
			case TOKEN_DIR1:
				goto dir1token;
				break;
			case TOKEN_DIR2:
				goto dir2token;
				break;

			case TOKEN_VELOCITY:
				goto velocitytoken;
				break;
			case TOKEN_VELOCITY0:
				goto velocity0token;
				break;
			case TOKEN_VELOCITY1:
				goto velocity1token;
				break;
			case TOKEN_VELOCITY2:
				goto velocity2token;
				break;

			case TOKEN_ORIGIN:
				goto origintoken;
				break;
			case TOKEN_ORIGIN0:
				goto origin0token;
				break;
			case TOKEN_ORIGIN1:
				goto origin1token;
				break;
			case TOKEN_ORIGIN2:
				goto origin2token;
				break;

			///////////////////////

			case TOKEN_MOVEGRAVITY:
				goto movegravitytoken;
				break;
			case TOKEN_COLORFADE:
				goto colorfadetoken;
				break;
			case TOKEN_SHADERTIME:
				goto shadertimetoken;
				break;
			case TOKEN_ALPHAFADE:
				goto alphafadetoken;
				break;
			case TOKEN_CULLDISTANCEVALUE:
				goto culldistancetokenvalue;
				break;
			case TOKEN_EMITTERID:
				goto emitteridtoken;
				break;
			case TOKEN_VIBRATE:
				goto vibratetoken;
				break;
			case TOKEN_ROTATE:
				goto rotatetoken;
				break;

			case TOKEN_ALPHA:
				goto alphatoken;
				break;
			case TOKEN_BLUE:
				goto bluetoken;
				break;
			case TOKEN_GREEN:
				goto greentoken;
				break;
			case TOKEN_RED:
				goto redtoken;
				break;
			case TOKEN_CGTIME:
				goto cgtimetoken;
				break;
			case TOKEN_ROLL:
				goto rolltoken;
				break;
			case TOKEN_PITCH:
				goto pitchtoken;
				break;
			case TOKEN_YAW:
				goto yawtoken;
				break;
			case TOKEN_ANGLE:
				goto angletoken;
				break;
			case TOKEN_WIDTH:
				goto widthtoken;
				break;
			case TOKEN_SIZE:
				goto sizetoken;
				break;
			case TOKEN_ANIMFRAME:
				goto animframetoken;
				break;

			////////////////////

			case TOKEN_TIME:
				goto timetoken;
				break;
			case TOKEN_LOOP:
				goto looptoken;
				break;
			case TOKEN_LOOPCOUNT:
				goto loopcounttoken;
				break;
			case TOKEN_PI:
				goto pitoken;
				break;
			case TOKEN_RAND:
				goto randtoken;
				break;
			case TOKEN_CRAND:
				goto crandtoken;
				break;
			case TOKEN_LERP:
				goto lerptoken;
				break;
			case TOKEN_LIFE:
				goto lifetoken;
				break;
			case TOKEN_CLIENTNUM:
				goto clientnumtoken;
				break;
			case TOKEN_ENEMY:
				goto enemytoken;
				break;
			case TOKEN_TEAMMATE:
				goto teammatetoken;
				break;
			case TOKEN_INEYES:
				goto ineyestoken;
				break;
			case TOKEN_TEAM:
				goto teamtoken;
				break;
			case TOKEN_SURFACETYPE:
				goto surfacetypetoken;
				break;
			case TOKEN_GAMETYPE:
				goto gametypetoken;
				break;
			case TOKEN_INWATER:
				goto inwatertoken;
				break;

			case TOKEN_MATH_MULT:
				goto math_multtoken;
				break;
			case TOKEN_MATH_DIV:
				goto math_divtoken;
				break;
			case TOKEN_MATH_MINUS:
				goto math_minustoken;
				break;
			case TOKEN_MATH_PLUS:
				goto math_plustoken;
				break;
			case TOKEN_MATH_MOD:
				goto math_modtoken;
				break;
			case TOKEN_MATH_LESS:
				goto math_lesstoken;
				break;
			case TOKEN_MATH_GREATER:
				goto math_greatertoken;
				break;
			case TOKEN_MATH_NOT:
				goto math_nottoken;
				break;
			case TOKEN_MATH_EQUAL:
				goto math_equaltoken;
				break;
			case TOKEN_MATH_AND:
				goto math_andtoken;
				break;
			case TOKEN_MATH_OR:
				goto math_ortoken;
				break;

			case TOKEN_MATH_SQRT:
				goto math_sqrttoken;
				break;
			case TOKEN_MATH_CEIL:
				goto math_ceiltoken;
				break;
			case TOKEN_MATH_FLOOR:
				goto math_floortoken;
				break;
			case TOKEN_MATH_SIN:
				goto math_sintoken;
				break;
			case TOKEN_MATH_COS:
				goto math_costoken;
				break;
			case TOKEN_MATH_TAN:
				goto math_tantoken;
				break;
			case TOKEN_MATH_WAVE:
				goto math_wavetoken;
				break;
			case TOKEN_MATH_CLIP:
				goto math_cliptoken;
				break;
			case TOKEN_MATH_ASIN:
				goto math_asintoken;
				break;
			case TOKEN_MATH_ACOS:
				goto math_acostoken;
				break;
			case TOKEN_MATH_ATAN:
				goto math_atantoken;
				break;
			case TOKEN_MATH_ATAN2:
				goto math_atan2token;
				break;
			case TOKEN_MATH_POW:
				goto math_powtoken;
				break;
			default:
				Com_Printf("^1CG_Q3mmeMathExt() unhandled token %d '%s'\n", tokenId, token);
				break;
			}
		}

		if (0) {

        } else if (token[0] == '+') {
		math_plustoken:
            ops[numOps] = OP_PLUS;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '-') {
		math_minustoken:
            ops[numOps] = OP_MINUS;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '*') {
		math_multtoken:
            ops[numOps] = OP_MULT;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '/') {
		math_divtoken:
            ops[numOps] = OP_DIV;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '%') {
		math_modtoken:
            ops[numOps] = OP_MOD;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '<') {
		math_lesstoken:
            ops[numOps] = OP_LESS;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '>') {
		math_greatertoken:
            ops[numOps] = OP_GREATER;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '!') {
		math_nottoken:
            ops[numOps] = OP_NOT;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '=') {
		math_equaltoken:
            ops[numOps] = OP_EQUAL;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '&') {
		math_andtoken:
            ops[numOps] = OP_AND;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '|') {
		math_ortoken:
            ops[numOps] = OP_OR;
            numOps += 2;
			goto handledToken;
        } else if (token[0] == '.'  ||  (token[0] >= '0'  &&  token[0] <= '9')) {  //((token[0] >= '0'  &&  token[0] <= '9')  ||  (token[0] == '-'  &&  (token[1] >= '0'  &&  token[1] <= '9'))) {
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = atof(token);
            numOps++;
			goto handledToken;
        } else if (!Q_stricmpt(token, "size")) {
		sizetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.size;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "width")) {
		widthtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.width;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "angle")) {
		angletoken:
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.angle;
            ops[numOps + 1] = ScriptVars.rotate;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t0")) {
		t0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t0;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t1")) {
		t1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t1;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t2")) {
		t2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t2;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t3")) {
		t3token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t3;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t4")) {
		t4token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t4;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t5")) {
		t5token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t5;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t6")) {
		t6token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t6;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t7")) {
		t7token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t7;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t8")) {
		t8token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t8;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "t9")) {
		t9token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t9;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin")) {
		origintoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.origin);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin0")) {
		origin0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin1")) {
		origin1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin2")) {
		origin2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity")) {
		velocitytoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.velocity);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity0")) {
		velocity0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity1")) {
		velocity1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity2")) {
		velocity2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir")) {
		dirtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.dir);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir0")) {
		dir0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir1")) {
		dir1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir2")) {
		dir2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles")) {
		anglestoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.angles);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles0")) {
		angles0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles1")) {
		angles1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles2")) {
		angles2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "yaw")) {
		yawtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[YAW];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "pitch")) {
		pitchtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[PITCH];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "roll")) {
		rolltoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[ROLL];
            numOps += 2;
			goto handledToken;
#if 0  // -- definately not used in q3mme (color0, color1, color2)
        } else if (!Q_stricmpt(token, "color")) {
		colortoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.color);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "color0")) {
		color0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "color1")) {
		color1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "color2")) {
		color2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "color3")) {  // alpha
		color3token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[3];
            numOps += 2;
			goto handledToken;
#endif
        } else if (!Q_stricmpt(token, "parentOrigin")) {
		parentorigintoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentOrigin);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentOrigin0")) {
		parentorigin0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentOrigin1")) {
		parentorigin1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentOrigin2")) {
		parentorigin2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentvelocity")) {
		parentvelocitytoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentVelocity);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentVelocity0")) {
		parentvelocity0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentVelocity1")) {
		parentvelocity1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentVelocity2")) {
		parentvelocity2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentAngles")) {
		parentanglestoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentAngles);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentAngles0")) {
		parentangles0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentAngles1")) {
		parentangles1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentAngles2")) {
		parentangles2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentangle")) {
		parentangletoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngle;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentyaw")) {
		parentyawtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[YAW];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentpitch")) {
		parentpitchtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[PITCH];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentroll")) {
		parentrolltoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[ROLL];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentsize")) {
		parentsizetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentSize;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentDir")) {
		parentdirtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentDir);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentDir0")) {
		parentdir0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentDir1")) {
		parentdir1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentDir2")) {
		parentdir2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentEnd")) {
		parentendtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentEnd);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentEnd0")) {
		parentend0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentEnd1")) {
		parentend1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentEnd2")) {
		parentend2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "end")) {
		endtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.end);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "end0")) {
		end0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "end1")) {
		end1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "end2")) {
		end2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "time")) {
		timetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = (float)((cg.ftime - (float)cgs.levelStartTime) / 1000.0);
            //ops[numOps + 1] = (cg.time - (float)cgs.levelStartTime) / 1000.0;
            numOps += 2;
			goto handledToken;
		} else if (!Q_stricmpt(token, "cgtime")) {
		cgtimetoken:
			ops[numOps] = OP_VAL;
			ops[numOps + 1] = (float)cg.ftime;
			numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "loop")) {
		looptoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.loop;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "loopcount")) {
		loopcounttoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.loopCount;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "red")) {
		redtoken:
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.red;
            ops[numOps + 1] = ScriptVars.color[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "green")) {
		greentoken:
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.green;
            ops[numOps + 1] = ScriptVars.color[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "blue")) {
		bluetoken:
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.blue;
            ops[numOps + 1] = ScriptVars.color[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "alpha")) {
		alphatoken:
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.alpha;
            ops[numOps + 1] = ScriptVars.color[3];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "pi")) {
		pitoken:
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = M_PI;
            numOps++;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rand")) {
		randtoken:
            //FIXME how is rand set, it's supposed to be a fixed value ??
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = random();  //ScriptVars.rand;
			//Com_Printf("rand() %f\n", ops[numOps + 1]);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "crand")) {
		crandtoken:
            //FIXME how is crand set, it's supposed to be a fixed value ??
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = crandom();  //ScriptVars.crand;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "lerp")) {
		lerptoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.lerp;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "life")) {
		lifetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.life;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v0")) {
		v0token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v0);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v1")) {
		v1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v1);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v2")) {
		v2token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v2);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v3")) {
		v3token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v3);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v4")) {
		v4token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v4);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v5")) {
		v5token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v5);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v6")) {
		v6token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v6);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v7")) {
		v7token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v7);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v8")) {
		v8token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v8);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v9")) {
		v9token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v9);
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v00")) {
		v00token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v01")) {
		v01token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v02")) {
		v02token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v10")) {
		v10token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v11")) {
		v11token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v12")) {
		v12token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v20")) {
		v20token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v21")) {
		v21token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v22")) {
		v22token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v30")) {
		v30token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v31")) {
		v31token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v32")) {
		v32token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v40")) {
		v40token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v41")) {
		v41token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v42")) {
		v42token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v50")) {
		v50token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v51")) {
		v51token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v52")) {
		v52token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v60")) {
		v60token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v61")) {
		v61token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v62")) {
		v62token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v70")) {
		v70token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v71")) {
		v71token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v72")) {
		v72token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v80")) {
		v80token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v81")) {
		v81token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v82")) {
		v82token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v90")) {
		v90token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[0];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v91")) {
		v91token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[1];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "v92")) {
		v92token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[2];
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rotate")) {
		rotatetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rotate;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "vibrate")) {
		vibratetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.vibrate;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "emitterid")) {
		emitteridtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.emitterId;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "alphafade")) {
		alphafadetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.alphaFade;
            numOps += 2;
			goto handledToken;
		} else if (!Q_stricmpt(token, "culldistancevalue")) {
		culldistancetokenvalue:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.cullDistanceValue;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "shadertime")) {
		shadertimetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.shaderTime;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "colorfade")) {
		colorfadetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.colorFade;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "movegravity")) {
		movegravitytoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.moveGravity;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "team")) {
		teamtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.team;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "clientnum")) {
		clientnumtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.clientNum;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "enemy")) {
		enemytoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.enemy;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "teammate")) {
		teammatetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.teamMate;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "ineyes")) {
		ineyestoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.inEyes;
            numOps += 2;
			goto handledToken;
#if 0
        } else if (!Q_stricmpt(token, "rewardImpressive")) {
		rewardImpressivetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardImpressive;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rewardExcellent")) {
		rewardExcellenttoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardExcellent;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rewardHumiliation")) {
		rewardHumiliationtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardHumiliation;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rewardDefend")) {
		rewardDefendtoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardDefend;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "rewardAssist")) {
		rewardAssisttoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardAssist;
            numOps += 2;
			goto handledToken;
#endif
        } else if (!Q_stricmpt(token, "surfacetype")) {
		surfacetypetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.surfaceType;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "gametype")) {
		gametypetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = cgs.gametype;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "animframe")) {
		animframetoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.animFrame;
            numOps += 2;
			goto handledToken;

        // functions //

        } else if (!Q_stricmpt(token, "sqrt")) {
		math_sqrttoken:
            ops[numOps] = OP_FSQRT;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "ceil")) {
		math_ceiltoken:
            ops[numOps] = OP_FCEIL;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "floor")) {
		math_floortoken:
            ops[numOps] = OP_FFLOOR;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "sin")) {
		math_sintoken:
            ops[numOps] = OP_FSIN;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "cos")) {
		math_costoken:
            ops[numOps] = OP_FCOS;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "wave")) {
		math_wavetoken:
            ops[numOps] = OP_FWAVE;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "clip")) {
		math_cliptoken:
            ops[numOps] = OP_FCLIP;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "acos")) {
		math_acostoken:
            ops[numOps] = OP_FACOS;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "asin")) {
		math_asintoken:
            ops[numOps] = OP_FASIN;
            numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "atan")) {
		math_atantoken:
            ops[numOps] = OP_FATAN;
            numOps += 2;
			goto handledToken;
		} else if (!Q_stricmpt(token, "atan2")) {
		math_atan2token:
			ops[numOps] = OP_FATAN2;
			numOps += 2;
			goto handledToken;
        } else if (!Q_stricmpt(token, "tan")) {
		math_tantoken:
            ops[numOps] = OP_FTAN;
            numOps += 2;
			goto handledToken;
		} else if (!Q_stricmpt(token, "pow")) {
		math_powtoken:
			ops[numOps] = OP_FPOW;
			numOps += 2;
			goto handledToken;
		} else if (!Q_stricmpt(token, "inwater")) {
		inwatertoken:
			ops[numOps] = OP_VAL;
			ops[numOps + 1] = CG_PointContents(ScriptVars.origin, -1) & CONTENTS_WATER;
			numOps += 2;
			goto handledToken;
		} else if (token[0] == '(') {
			// pass
        } else if (trap_Cvar_Exists(token)) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = SC_Cvar_Get_Float(token);
            numOps += 2;
            if (verbose) {
                //Com_Printf("^3q3mme math unknown token: 's'\n", token);
                Com_Printf("cvar: '%s' %f\n", token, SC_Cvar_Get_Float(token));
            }
        } else {
			Com_Printf("^3q3mme math unknown token/cvar '%s'\n", token);
			Com_Printf("next: '%s'\n", PrintShort(script, 16));
			//CG_Abort();
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = 0;
            numOps += 2;
			//recursiveCount--;
			//return script;
		}

	handledToken:
        if (newLine) {
			//Com_Printf("^2math done rest: '%s'\n", PrintShort(script, 10));
            break;
        }
    }

    // sanity check
    if (ops[0] >= OP_PLUS  &&  ops[0] <= OP_OR  &&  ops[0] != OP_MINUS) {
        *error = 1;
        Com_Printf("^3q3mme math invalid first token(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
        recursiveCount--;
        return script;
    }

    if (numOps > 0  &&  (ops[numOps - 2] >= OP_PLUS  &&  ops[numOps - 2] <= OP_OR)) {
        *error = 2;
        Com_Printf("^3q3mme math invalid last token(%d %d): %f\n", recursiveCount, uniqueId, ops[numOps - 2]);
        recursiveCount--;
		//CG_Abort();
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
                Com_Printf("^3q3mme math invalid function value type (%d %d): %f\n", recursiveCount, uniqueId, ops[i + 2]);
                recursiveCount--;
                return script;
            }
            val = ops[i + 3];
        }

        if (ops[i] == OP_FSQRT) {
            val = sqrt(val);
        } else if (ops[i] == OP_FCEIL) {
            //val = syscall(CG_CEIL, val);
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
        } else {
            Com_Printf("^3q3mme math unknown function %f\n", ops[i]);
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
                    Com_Printf("^3q3mme math two operands following each other(%d %d)\n", recursiveCount, uniqueId);
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

    // * / %

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
				CG_Printf("^3q3mme math divide by zero\n");
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
        Com_Printf("^3q3mme math invalid final value op(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
        recursiveCount--;
		//Com_Printf("orig '%s'\n", PrintShort(oo, 16));
		//CG_Abort();
        return script;
    }

    *val = ops[1];
    if (verbose) {
        Com_Printf("val(%d %d)(numOps:%d): %f\n", recursiveCount, uniqueId, numOps, *val);
    }

    recursiveCount--;
    return script;
}
#undef MAX_OPS

const char *CG_Q3mmeMath (const char *script, float *val, int *error)
{
	return CG_Q3mmeMathExt(script, NULL, val, error, qtrue);
}

static vec3_t *CG_GetScriptVector (const char *name, int tokenId)
{
	// tks vector
    if (tokenId) {
		switch (tokenId) {
		case TOKEN_ORIGIN:
			goto origintoken;
			break;

		case TOKEN_V0:
			goto v0token;
			break;
		case TOKEN_V1:
			goto v1token;
			break;
		case TOKEN_V2:
			goto v2token;
			break;
		case TOKEN_V3:
			goto v3token;
			break;
		case TOKEN_V4:
			goto v4token;
			break;
		case TOKEN_V5:
			goto v5token;
			break;
		case TOKEN_V6:
			goto v6token;
			break;
		case TOKEN_V7:
			goto v7token;
			break;
		case TOKEN_V8:
			goto v8token;
			break;
		case TOKEN_V9:
			goto v9token;
			break;

		case TOKEN_VELOCITY:
			goto velocitytoken;
			break;
		case TOKEN_DIR:
			goto dirtoken;
			break;
		case TOKEN_ANGLES:
			goto anglestoken;
			break;
		case TOKEN_COLOR:
			goto colortoken;
			break;
		case TOKEN_PARENTORIGIN:
			goto parentorigintoken;
			break;
		case TOKEN_PARENTVELOCITY:
			goto parentvelocitytoken;
			break;
		case TOKEN_PARENTANGLES:
			goto parentanglestoken;
			break;
		case TOKEN_PARENTDIR:
			goto parentdirtoken;
			break;
		case TOKEN_PARENTEND:
			goto parentendtoken;
			break;

		case TOKEN_END:
			goto endtoken;
			break;

		default:
			Com_Printf("^3CG_GetScriptVector() unhandled token %d '%s'\n", tokenId, name);
			break;
		}
	}

    if (!Q_stricmpt(name, "v0")) {
	v0token:
        return &ScriptVars.v0;
    } else if (!Q_stricmpt(name, "v1")) {
	v1token:
        return &ScriptVars.v1;
    } else if (!Q_stricmpt(name, "v2")) {
	v2token:
        return &ScriptVars.v2;
    } else if (!Q_stricmpt(name, "v3")) {
	v3token:
        return &ScriptVars.v3;
    } else if (!Q_stricmpt(name, "v4")) {
	v4token:
        return &ScriptVars.v4;
    } else if (!Q_stricmpt(name, "v5")) {
	v5token:
        return &ScriptVars.v5;
    } else if (!Q_stricmpt(name, "v6")) {
	v6token:
        return &ScriptVars.v6;
    } else if (!Q_stricmpt(name, "v7")) {
	v7token:
        return &ScriptVars.v7;
    } else if (!Q_stricmpt(name, "v8")) {
	v8token:
        return &ScriptVars.v8;
    } else if (!Q_stricmpt(name, "v9")) {
	v9token:
        return &ScriptVars.v9;
    } else if (!Q_stricmpt(name, "origin")) {
	origintoken:
        return &ScriptVars.origin;
    } else if (!Q_stricmpt(name, "velocity")) {
	velocitytoken:
        return &ScriptVars.velocity;
    } else if (!Q_stricmpt(name, "dir")) {
	dirtoken:
        return &ScriptVars.dir;
    } else if (!Q_stricmpt(name, "angles")) {
	anglestoken:
        return &ScriptVars.angles;
    } else if (!Q_stricmpt(name, "color")) {
	colortoken:
        return (vec3_t *)&ScriptVars.color;
    } else if (!Q_stricmpt(name, "parentOrigin")) {
	parentorigintoken:
        return &ScriptVars.parentOrigin;
    } else if (!Q_stricmpt(name, "parentvelocity")) {
	parentvelocitytoken:
        return &ScriptVars.parentVelocity;
    } else if (!Q_stricmpt(name, "parentAngles")) {
	parentanglestoken:
        return &ScriptVars.parentAngles;
    } else if (!Q_stricmpt(name, "parentDir")) {
	parentdirtoken:
        return &ScriptVars.parentDir;
	} else if (!Q_stricmpt(name, "parentEnd")) {
	parentendtoken:
		return &ScriptVars.parentEnd;
    } else if (!Q_stricmpt(name, "end")) {
	endtoken:
		return &ScriptVars.end;
	}

    Com_Printf("^3unknown script vector '%s'\n", name);

    return &ScriptVars.tmpVector;  // just avoiding problems and for testing
}

static const char *CG_SkipOpeningBrace (const char *script)
{
    const char *p;

    p = script;
    while (1) {
        if (p[0] == '\0') {
            Com_Printf("CG_SkipOpeningBrace() eof\n");
            return NULL;
        } else if (p[0] == '{') {
            p++;
            break;
        }
        p++;
    }

    return p;
}

// assumed opening brace not included
static const char *CG_SkipBlock (const char *script)
{
    int block;
    const char *p;

	if (!script) {
		Com_Printf("CG_SkipBlock() script == NULL\n");
		return NULL;
	}

    block = 1;
    p = script;
    while (1) {
        if (p[0] == '\0') {
            Com_Printf("CG_SkipBlock() eof\n");
            return NULL;
        } else if (p[0] == '{') {
            block++;
        } else if (p[0] == '}') {
            block--;
        }

        if (block == 0) {
            p++;
            break;
        }

        p++;
    }

    return p;
}

#if 0
// assumed opening brace not included, returns error
static int CG_CopyBlock (const char *src, char *dest)
{
    const char *end;

    end = CG_SkipBlock(src);
	if (end - src >= (1024 * 32)) {  //FIXME tracking bug
		Com_Printf("^1fuck\n");
		dest[0] = '\0';
		return 1;
	}
    if (!end) {
        Com_Printf("^1ERROR CG_CopyBlock()\n");
		//Com_Printf("dest: '%s'\n", dest);
        dest[0] = '\0';
        return 1;
    }
    memcpy(dest, src, end - src);
    dest[end - src] = '\0';
    return 0;
}
#endif

static int CG_NumIfBlocks (const char *script, const char **endOfBlocks)
{
    const char *p;
    const char *tp;
    int n;
    char token[MAX_QPATH];
    qboolean newLine;
	int tokenId;

    //p = script;
    p = CG_SkipOpeningBrace(script);
    if (!p) {
        Com_Printf("CG_NumIfBlocks() eof opening brace\n");
        return 0;
    }
    n = 0;
    p = CG_SkipBlock(p);
    if (!p) {
        Com_Printf("CG_NumIfBlocks() eof main 'if' block\n");
        return 0;
    }
    n++;

    while (1) {
        tp = p;
        while (1) {
            tp = CG_GetFxToken(tp, token, qfalse, &newLine, &tokenId);
            if (tp[0] == '\0') {
                return 0;
            }
            // allow braces and keywords on new lines
            if (token[0] == '\0') {
                continue;
            }
            break;
        }
        if (!Q_stricmpt(token, "else")  ||  !Q_stricmpt(token, "elif")) {
            n++;
            p = CG_SkipOpeningBrace(p);
            if (!p) {
                Com_Printf("CG_NumIfBlocks() eof opening brace %d\n", n);
                return 0;
            }
            p = CG_SkipBlock(p);
            if (!p) {
                Com_Printf("CG_NumIfBlocks() eof block %d\n", n);
                return 0;
            }
        } else {
            break;
        }
    }

    if (endOfBlocks) {
        *endOfBlocks = p;  //(char *)p;  //FIXME const
    }

    return n;
}

static void CG_FX_SetRenderfx (int *renderfx)
{
    //FIXME render effects option
#if 0
	if (ScriptVars.shadow) {
		// pass
	} else {
		re->renderfx |= RF_NOSHADOW;
	}
#endif
	if (ScriptVars.depthHack) {
		*renderfx |= RF_DEPTHHACK;
	}
	if (ScriptVars.thirdPerson) {
		*renderfx |= RF_THIRD_PERSON;
	}
	if (ScriptVars.firstPerson) {
		*renderfx |= RF_FIRST_PERSON;
	}

}

//FIXME don't pass anything in
static void CG_FX_Wobble (const vec3_t dir, vec3_t newDir, const vec3_t origin, float degrees)
{
    vec3_t right, up;
    double newLen;
    vec3_t endPoint;

    MakeNormalVectors(dir, right, up);
    VectorCopy(origin, endPoint);
    VectorMA(endPoint, crandom() * 60, right, endPoint);
    VectorMA(endPoint, crandom() * 60, up, endPoint);
    VectorSubtract(endPoint, origin, newDir);
    newLen = tan(DEG2RAD(degrees));
    VectorNormalize(newDir);
    VectorMA(origin, newLen, newDir, endPoint);
    VectorMA(endPoint, 1, dir, endPoint);
    VectorSubtract(endPoint, origin, newDir);
    VectorNormalize(newDir);
}

static void CG_FX_Vibrate (void)
{
	//float oldLen;
	float newLen;
	float fac;
	float maxDist;
	float newValue;

	if (EmitterScript  &&  !PlainScript) {
		return;
	}

	if (ScriptJustParsing) {
		return;
	}

	newLen = Distance(cg.refdef.vieworg, ScriptVars.origin);
	maxDist = cg_vibrateMaxDistance.value;  //800.0;
	//Com_Printf("dist: %f\n", newLen);

	if (newLen > maxDist) {
		return;
	}
	fac = 1.0 - newLen / maxDist;
	newValue = ScriptVars.vibrate * fac;
	if (newValue < cg.vibrateCameraValue) {
		return;
	}
	cg.vibrateCameraValue = newValue;
	if (cg.vibrateCameraPhase == 0) {
		cg.vibrateCameraPhase = crandom() * M_PI;
	}
	cg.vibrateCameraTime = cg.ftime;
}

static void CG_CopyStoredScriptVarsToLE (localEntity_t *le)
{
#if 0
	//FIXME maybe more stuff?
	VectorCopy(ScriptVars.v0, le->v0);
	VectorCopy(ScriptVars.v1, le->v1);
	VectorCopy(ScriptVars.v2, le->v2);
	VectorCopy(ScriptVars.v3, le->v3);
	VectorCopy(ScriptVars.origin, le->scriptOrigin);
	VectorCopy(ScriptVars.dir, le->scriptDir);
	VectorCopy(ScriptVars.velocity, le->scriptVelocity);

	Vector4Copy(ScriptVars.color, le->scriptColor);

	le->scriptSize = ScriptVars.size;
	le->scriptRotate = ScriptVars.rotate;

	le->scriptHasMoveBounce = ScriptVars.hasMoveBounce;
	le->scriptMoveBounce1 = ScriptVars.moveBounce1;
	le->scriptMoveBounce2 = ScriptVars.moveBounce2;
	le->scriptHasMoveGravity = ScriptVars.hasMoveGravity;
	le->scriptMoveGravity = ScriptVars.moveGravity;
	le->scriptHasSink = ScriptVars.hasSink;
	le->scriptSink1 = ScriptVars.sink1;
	le->scriptSink2 = ScriptVars.sink2;

	Q_strncpyz(le->scriptShader, ScriptVars.shader, MAX_QPATH);

	le->t0 = ScriptVars.t0;
	le->t1 = ScriptVars.t1;
	le->t2 = ScriptVars.t2;
	le->t3 = ScriptVars.t3;
#endif

	memcpy(&le->sv, &ScriptVars, sizeof(ScriptVars_t));
}

void CG_GetStoredScriptVarsFromLE (const localEntity_t *le)
{
#if 0
	VectorCopy(le->v0, ScriptVars.v0);
	VectorCopy(le->v1, ScriptVars.v1);
	VectorCopy(le->v2, ScriptVars.v2);
	VectorCopy(le->v3, ScriptVars.v3);
	VectorCopy(le->scriptOrigin, ScriptVars.origin);
	VectorCopy(le->scriptDir, ScriptVars.dir);
	VectorCopy(le->scriptVelocity, ScriptVars.velocity);

	Vector4Copy(le->scriptColor, ScriptVars.color);

	ScriptVars.size = le->scriptSize;
	ScriptVars.rotate = le->scriptRotate;

	ScriptVars.hasMoveBounce = le->scriptHasMoveBounce;
	ScriptVars.moveBounce1 = le->scriptMoveBounce1;
	ScriptVars.moveBounce2 = le->scriptMoveBounce2;
	ScriptVars.hasMoveGravity = le->scriptHasMoveGravity;
	ScriptVars.moveGravity = le->scriptMoveGravity;
	ScriptVars.hasSink = le->scriptHasSink;
	ScriptVars.sink1 = le->scriptSink1;
	ScriptVars.sink2 = le->scriptSink2;

	Q_strncpyz(ScriptVars.shader, le->scriptShader, MAX_QPATH);

	ScriptVars.t0 = le->t0;
	ScriptVars.t1 = le->t1;
	ScriptVars.t2 = le->t2;
	ScriptVars.t3 = le->t3;
#endif

	memcpy(&ScriptVars, &le->sv, sizeof(ScriptVars_t));
}

static void CG_FX_Sprite (qboolean viewAligned, qboolean isSpark)
{
    localEntity_t *le;
	//refEntity_t ent;
    refEntity_t *re;
	refEntity_t ent;
    //vec3_t move;
	//vec3_t angles;

    //Com_Printf("sprite: %f\n", ScriptVars.emitterTime);

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif

	//Com_Printf("sprite\n");

    //VectorCopy(ScriptVars.origin, move);

    if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
#if 0
		if (isSpark) {
			//FIXME dir
		} else {
			ScriptVars.width = 0;
		}
#endif
		CG_CopyStoredScriptVarsToLE(le);
		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;
		le->fxType = LEFX_EMIT;
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		re = &le->refEntity;
		le->color[0] = ScriptVars.color[0];
		le->color[1] = ScriptVars.color[1];
		le->color[2] = ScriptVars.color[2];
		le->color[3] = ScriptVars.color[3];

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;
		VectorCopy(ScriptVars.origin, le->pos.trBase);
		le->pos.trDelta[0] = ScriptVars.velocity[0];
		le->pos.trDelta[1] = ScriptVars.velocity[1];
		le->pos.trDelta[2] = ScriptVars.velocity[2];
		re->shaderTime = cg.ftime / 1000.0f;
		//Com_Printf("yes\n");
		//Com_Printf("^1sprite size: %f '%s'\n", ScriptVars.size, ScriptVars.shader);
    } else {
		//Com_Printf("no\n");
		//le->fxType = LEFX_NONE;
		memset(&ent, 0, sizeof(ent));
		re = &ent;
    }

	if (ScriptVars.hasShaderTime) {
		re->shaderTime = ScriptVars.shaderTime;
	}

	if (isSpark) {
		re->reType = RT_SPARK;
		VectorCopy(ScriptVars.velocity, re->oldorigin);
		re->width = ScriptVars.width;
	} else if (viewAligned) {
		re->reType = RT_SPRITE;
		re->width = 0;
	} else {
		re->reType = RT_SPRITE_FIXED;
		//vectoangles(ScriptVars.dir, angles);
		//AnglesToAxis(angles, re->axis);
		VectorCopy(ScriptVars.dir, re->oldorigin);
		re->width = 0;
	}

	CG_FX_SetRenderfx(&re->renderfx);

    re->rotation = ScriptVars.rotate;
    re->radius = ScriptVars.size;
#if 0
	if (isSpark) {
		re->width = ScriptVars.width;
		//FIXME dir
	} else {
		re->width = 0;
	}
#endif
	if (*ScriptVars.shader) {
		re->customShader = trap_R_RegisterShader(ScriptVars.shader);
	} else {
		//re->customShader = 0;  //FIXME you're masking a bug
	}
    re->shaderRGBA[0] = 255 * ScriptVars.color[0];
    re->shaderRGBA[1] = 255 * ScriptVars.color[1];
    re->shaderRGBA[2] = 255 * ScriptVars.color[2];
    re->shaderRGBA[3] = 255 * ScriptVars.color[3];

	VectorCopy(ScriptVars.origin, re->origin);
	//VectorCopy(ScriptVars.origin, re->oldorigin);
	//FIXME axis angles

	if (parsingEmitterScriptLevel == 0) {
		CG_AddRefEntityFX(re);
	}
}

static void CG_FX_Spark (void)
{
    //FIXME not seen in scripts
    //Com_Printf("FIXME fx spark\n");

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif

	//CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
	// just guessing
	if (!*ScriptVars.shader) {
		Q_strncpyz(ScriptVars.shader, "gfx/misc/tracer", sizeof(ScriptVars.shader));
	}
	//CG_FX_Sprite(qfalse, qtrue);
	CG_FX_Sprite(qtrue, qtrue);
}

static void CG_FX_Beam (void)
{
	localEntity_t *le;
	refEntity_t *re;
	refEntity_t ent;

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif



    if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		CG_CopyStoredScriptVarsToLE(le);

		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;

		le->fxType = LEFX_EMIT;

		le->leFlags = LEF_PUFF_DONT_SCALE;
		//le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		re = &le->refEntity;

		le->color[0] = ScriptVars.color[0];
		le->color[1] = ScriptVars.color[1];
		le->color[2] = ScriptVars.color[2];
		le->color[3] = ScriptVars.color[3];

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;

		re->shaderTime = cg.time / 1000.0f;  //FIXME check, was 0
    } else {
        //le->emitterScript[0] = '\0';
		//le->fxType = LEFX_NONE;  // so that the script isn't run
		memset(&ent, 0, sizeof(ent));
		re = &ent;
    }

	if (ScriptVars.hasShaderTime) {
		re->shaderTime = ScriptVars.shaderTime;
	}

    re->reType = RT_BEAM_Q3MME;
    re->rotation = ScriptVars.rotate;
    re->radius = ScriptVars.size;
	//re->angle = ScriptVars.angle;
	if (*ScriptVars.shader) {
		re->customShader = trap_R_RegisterShader(ScriptVars.shader);
	} else {
		//re->customShader = 0;  //FIXME you're masking a bug
	}
    re->shaderRGBA[0] = 255 * ScriptVars.color[0];
    re->shaderRGBA[1] = 255 * ScriptVars.color[1];
    re->shaderRGBA[2] = 255 * ScriptVars.color[2];
    re->shaderRGBA[3] = 255 * ScriptVars.color[3];

    //VectorCopy( move, le->pos.trBase );
    //VectorCopy(le->pos.trBase, re->origin);
    //le->pos.trDelta[0] = ScriptVars.velocity[0];
    //le->pos.trDelta[1] = ScriptVars.velocity[1];
    //le->pos.trDelta[2] = ScriptVars.velocity[2];

	CG_FX_SetRenderfx(&re->renderfx);

	VectorCopy(ScriptVars.origin, re->origin);
	//VectorCopy(ScriptVars.end, re->oldorigin);
	VectorCopy(ScriptVars.dir, re->oldorigin);

	if (parsingEmitterScriptLevel == 0) {
		CG_AddRefEntityFX(re);
	}
}

static void CG_FX_Light (void)
{
	localEntity_t *le;

	if (EmitterScript) {  //  &&  !PlainScript) {
		return;
	}

#if 0
	if (ScriptJustParsing) {
		return;
	}
#endif

	if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		CG_CopyStoredScriptVarsToLE(le);
		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;
		le->fxType = LEFX_EMIT_LIGHT;
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		//re = &le->refEntity;
		le->color[0] = ScriptVars.color[0];
		le->color[1] = ScriptVars.color[1];
		le->color[2] = ScriptVars.color[2];
		le->color[3] = ScriptVars.color[3];

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;
		VectorCopy(ScriptVars.origin, le->pos.trBase);
		le->pos.trDelta[0] = ScriptVars.velocity[0];
		le->pos.trDelta[1] = ScriptVars.velocity[1];
		le->pos.trDelta[2] = ScriptVars.velocity[2];
	} else {
		trap_R_AddLightToScene(ScriptVars.origin, ScriptVars.size, ScriptVars.color[0], ScriptVars.color[1], ScriptVars.color[2]);
		return;
	}
}

static void CG_FX_Model (fxModelType_t ftype)
{
	refEntity_t ent;
	refEntity_t *re;
	localEntity_t *le;
    //entityState_t *s1;
    vec3_t ang;

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif

    //FIXME maybe no entityState
    //s1 = &ScriptVars.currentState;

    if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
#if 0
        if (CG_CopyBlock(ScriptVars.emitterScriptStart, le->emitterScript)) {
            Com_Printf("couldn't copy emitter script\n");
            return;
        }
		le->emitterScript[ScriptVars.emitterScriptEnd - ScriptVars.emitterScriptStart] = '\0';
#endif
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		re = &le->refEntity;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		VectorCopy(ScriptVars.origin, le->pos.trBase);
		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;
		CG_CopyStoredScriptVarsToLE(le);

		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;

		le->fxType = LEFX_EMIT;

		re->shaderTime = cg.ftime / 1000.0f;
    } else {
        //le->emitterScript[0] = '\0';
		memset(&ent, 0, sizeof(ent));
		re = &ent;
		//Com_Printf("using local refEnt: '%s'\n", ScriptVars.model);
    }

	if (ScriptVars.hasShaderTime) {
		re->shaderTime = ScriptVars.shaderTime;
	}

	//re->reType = RT_MODEL;
	switch (ftype) {
	case FX_MODEL_DIR:
		re->reType = RT_MODEL_FX_DIR;
		break;
	case FX_MODEL_ANGLES:
		re->reType = RT_MODEL_FX_ANGLES;
		break;
	case FX_MODEL_AXIS:
		re->reType = RT_MODEL_FX_AXIS;
		break;
	default:
		CG_Printf("^1CG_FX_Model() invalid model type %d\n", ftype);
		re->reType = RT_MODEL;
		break;
	}

	VectorCopy(ScriptVars.origin, re->origin);
	VectorCopy(ScriptVars.origin, re->oldorigin);

	re->skinNum = cg.clientFrame & 1;
	re->hModel = trap_R_RegisterModel(ScriptVars.model);  //weapon->missileModel;
	//Com_Printf("model: '%s'\n", ScriptVars.model);
	//ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;
	CG_FX_SetRenderfx(&re->renderfx);

	//re->renderfx = RF_SHADOW_PLANE;

    re->radius = ScriptVars.size;
    re->rotation = ScriptVars.rotate;
    if (*ScriptVars.shader) {
        re->customShader = trap_R_RegisterShader(ScriptVars.shader);
    } else {
		//re->customShader = 0;  //FIXME you're masking a bug
	}
    re->shaderRGBA[0] = 255 * ScriptVars.color[0];
    re->shaderRGBA[1] = 255 * ScriptVars.color[1];
    re->shaderRGBA[2] = 255 * ScriptVars.color[2];
    re->shaderRGBA[3] = 255 * ScriptVars.color[3];

    //FIXME this should be set before
	//if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
    if (VectorNormalize2(ScriptVars.velocity, re->axis[0]) == 0) {
		re->axis[0][2] = 1;
	}

#if 0
    if (0) {  //(ScriptVars.spin) {
	// spin as it moves
	if (s1->pos.trType != TR_STATIONARY) {
		RotateAroundDirection(re->axis, cg.time / 4);
        //Com_Printf("rotating\n");
	} else {
#if 1  //def MPACK
		if (s1->weapon == WP_PROX_LAUNCHER) {
			AnglesToAxis(ScriptVars.angles, re->axis);
		}
		else
#endif
		{
			RotateAroundDirection(re->axis, s1->time);
		}
	}
    }
#endif

	if (ftype == FX_MODEL_DIR  ||  ftype == FX_MODEL_ANGLES) {
		if (VectorLength(ScriptVars.velocity)) {
			if (ftype == FX_MODEL_DIR) {
				vectoangles(ScriptVars.dir, ang);
				AnglesToAxis(ang, re->axis);
			} else if (ftype == FX_MODEL_ANGLES) {
				AnglesToAxis(ScriptVars.angles, re->axis);
				//Com_Printf("angles to axis\n");
			}
		} else {  // hack to match behavior of dirModel and anglesModel
			if (!VectorLength(ScriptVars.angles)) {
					vectoangles(ScriptVars.dir, ang);
					AnglesToAxis(ang, re->axis);
			} else {
				// hack for lg impact beam
				AnglesToAxis(ScriptVars.angles, re->axis);
			}
		}
	} else if (ftype == FX_MODEL_AXIS) {
		VectorCopy(ScriptVars.axis[0], re->axis[0]);
		VectorCopy(ScriptVars.axis[1], re->axis[1]);
		VectorCopy(ScriptVars.axis[2], re->axis[2]);
	}

	//Com_Printf("rotation %f\n", re->rotation);

	//RotateAroundDirection(re->axis, cg.time / 4);

	if (re->rotation) {
		//FIXME stationary check
		//Com_
		RotateAroundDirection(re->axis, re->rotation);
	}

	if (re->radius != 1) {
		VectorScale(re->axis[0], re->radius, re->axis[0]);
		VectorScale(re->axis[1], re->radius, re->axis[1]);
		VectorScale(re->axis[2], re->radius, re->axis[2]);
		re->nonNormalizedAxes = qtrue;
	}

	re->frame = ScriptVars.animFrame;

	// add to refresh list, possibly with quad glow
	//CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE );  //FIXME missile?
    //FIXME powerups option
    if (parsingEmitterScriptLevel == 0) {
		CG_AddRefEntityFX(re);
	}
}


static void CG_FX_Decal (void)
{
	qhandle_t shader;

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif

	shader = trap_R_RegisterShader(ScriptVars.shader);
	if (!shader) {
		return;
	}

	//FIXME shader time?

    CG_ImpactMark(shader, ScriptVars.origin, ScriptVars.dir, ScriptVars.rotate, ScriptVars.color[0], ScriptVars.color[1], ScriptVars.color[2], ScriptVars.color[3], ScriptVars.decalAlpha, ScriptVars.size, qfalse, ScriptVars.decalEnergy, qtrue);  //FIXME sure about 'debug' as qtrue??
	//Com_Printf("impact mark color %f %f %f %f\n", ScriptVars.color[0], ScriptVars.color[1], ScriptVars.color[2], ScriptVars.color[3]);
}

static void CG_FX_DecalTemp (void)
{
	qhandle_t shader;

    if (EmitterScript) {
        return;
    }

#if 0
	if (PlainScript) {
		return;
	}
#endif

	shader = trap_R_RegisterShader(ScriptVars.shader);
    CG_ImpactMark(shader, ScriptVars.origin, ScriptVars.dir, ScriptVars.rotate, ScriptVars.color[0], ScriptVars.color[1], ScriptVars.color[2], ScriptVars.color[3], ScriptVars.decalAlpha, ScriptVars.size, qtrue, ScriptVars.decalEnergy, qtrue);  //FIXME sure about 'debug' as qtrue??
}

static void CG_FX_Rings (void)
{
	localEntity_t *le;
	refEntity_t *re;

    if (EmitterScript) {
        return;
    }

	if (PlainScript) {
		return;
	}

	//Com_Printf("rings\n");

	le = CG_AllocFxEntity(ScriptVars.emitterId);

	re = &le->refEntity;

    if (parsingEmitterScriptLevel > 0) {
#if 0
        if (CG_CopyBlock(ScriptVars.emitterScriptStart, le->emitterScript)) {
            Com_Printf("couldn't copy emitter script\n");
            return;
        }
		le->emitterScript[ScriptVars.emitterScriptEnd - ScriptVars.emitterScriptStart] = '\0';
#endif
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		CG_CopyStoredScriptVarsToLE(le);

		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;

		le->fxType = LEFX_EMIT;

		re->shaderTime = cg.ftime / 1000.0f;
    } else {
        //le->emitterScript[0] = '\0';
		le->fxType = LEFX_NONE;
    }

	if (ScriptVars.hasShaderTime) {
		re->shaderTime = ScriptVars.shaderTime;
	}

	VectorCopy(ScriptVars.origin, re->origin);
	//VectorCopy(ScriptVars.end, re->oldorigin);
	VectorCopy(ScriptVars.dir, re->oldorigin);
	re->radius = ScriptVars.size;
	re->rotation = ScriptVars.width;
	//ScriptVars.rotate = ScriptVars.width;
	//Com_Printf("width %f\n", re->rotation);
	//Com_Printf("start %f %f %f    end %f %f %f\n", ScriptVars.origin[0], ScriptVars.origin[1], ScriptVars.origin[2], ScriptVars.end[0], ScriptVars.end[1], ScriptVars.end[2]);

	le->leType = LE_FADE_RGB;
	le->startTime = cg.ftime;
	le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
	le->pos.trTime = cg.ftime;
	le->trTimef = cg.ftime;

	re->reType = RT_RAIL_RINGS_Q3MME;
	re->customShader = cgs.media.railRingsShader;

	re->shaderRGBA[0] = ScriptVars.color[0] * 255;
	re->shaderRGBA[1] = ScriptVars.color[1] * 255;
	re->shaderRGBA[2] = ScriptVars.color[2] * 255;
	re->shaderRGBA[3] = ScriptVars.color[3] * 255;

	//Com_Printf("color %d %d %d %d\n", re->shaderRGBA[0], re->shaderRGBA[1], re->shaderRGBA[2], re->shaderRGBA[3]);

	le->color[0] = ScriptVars.color[0];
	le->color[1] = ScriptVars.color[1];
	le->color[2] = ScriptVars.color[2];
	le->color[3] = ScriptVars.color[3];
}

static void CG_FX_Sound (int channel)
{
	localEntity_t *le;
	sfxHandle_t sfx;

	if (EmitterScript) {  //  &&  !PlainScript) {
		return;
	}

#if 0
	if (ScriptJustParsing) {
		return;
	}
#endif

	if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		CG_CopyStoredScriptVarsToLE(le);
		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;
		le->fxType = LEFX_EMIT_SOUND;
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		//re = &le->refEntity;
		le->color[0] = ScriptVars.color[0];
		le->color[1] = ScriptVars.color[1];
		le->color[2] = ScriptVars.color[2];
		le->color[3] = ScriptVars.color[3];

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;
		VectorCopy(ScriptVars.origin, le->pos.trBase);
		le->pos.trDelta[0] = ScriptVars.velocity[0];
		le->pos.trDelta[1] = ScriptVars.velocity[1];
		le->pos.trDelta[2] = ScriptVars.velocity[2];
	} else {
		if (*ScriptVars.sound) {
			sfx = trap_S_RegisterSound(ScriptVars.sound, qfalse);
			if (channel == CHAN_WEAPON) {
				CG_StartSound(NULL, ScriptVars.clientNum, CHAN_WEAPON, sfx);
			} else if (channel == CHAN_LOCAL) {
				CG_StartLocalSound(sfx, CHAN_LOCAL_SOUND);
			} else {
				CG_StartSound(ScriptVars.origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
			}
		}
	}
}

static void CG_FX_LoopSound (void)
{
	localEntity_t *le;
	qhandle_t lsound;

	if (EmitterScript) {  //  &&  !PlainScript) {
		return;
	}

#if 0
	if (ScriptJustParsing) {
		return;
	}
#endif

	if (parsingEmitterScriptLevel > 0) {
		le = CG_AllocFxEntity(ScriptVars.emitterId);
		EmittedEntities++;
		le->emitterScript = ScriptVars.emitterScriptStart;
		le->emitterScriptEnd = ScriptVars.emitterScriptEnd;
		CG_CopyStoredScriptVarsToLE(le);
		VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
		VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
		le->sv.lastDistanceTime = cg.ftime;
		le->sv.lastIntervalTime = cg.ftime;
		le->fxType = LEFX_EMIT_LOOPSOUND;
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		//re = &le->refEntity;
		le->color[0] = ScriptVars.color[0];
		le->color[1] = ScriptVars.color[1];
		le->color[2] = ScriptVars.color[2];
		le->color[3] = ScriptVars.color[3];

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.ftime;
		le->trTimef = cg.ftime;
		VectorCopy(ScriptVars.origin, le->pos.trBase);
		le->pos.trDelta[0] = ScriptVars.velocity[0];
		le->pos.trDelta[1] = ScriptVars.velocity[1];
		le->pos.trDelta[2] = ScriptVars.velocity[2];
		//FIXME entNumber for looping sound
	} else {
		lsound = trap_S_RegisterSound(ScriptVars.loopSound, qfalse);
		if (lsound) {
			if (0) {  //(ScriptVars.parentCent) {
				CG_AddLoopingSound(ScriptVars.parentCent->currentState.number, ScriptVars.origin, ScriptVars.velocity, lsound);
			} else {
				if (FxLoopSounds >= MAX_LOOP_SOUNDS) {
					CG_Printf("^3fx:  max loop sounds added\n");
				} else {
					CG_AddLoopingSound(FxLoopSounds, ScriptVars.origin, ScriptVars.velocity, lsound);
					FxLoopSounds++;
				}
			}
		}
	}
}

static const char *CG_ParseRenderTokens (const char *script)
{
    char token[MAX_QPATH];
    qboolean newLine;
	const char *s;
	int tokenId;

    ScriptVars.firstPerson = qfalse;
    ScriptVars.thirdPerson = qfalse;
    ScriptVars.shadow = qfalse;
    ScriptVars.cullNear = qfalse;  // overlapping sprites, draw the closer one
    ScriptVars.cullRadius = qfalse;
	ScriptVars.cullDistance = qfalse;
    ScriptVars.depthHack = qfalse;
    ScriptVars.stencil = qfalse;
    newLine = qfalse;

    while (!newLine) {
        s = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
        if (token[0] == '\0') {
            break;
        } else if (!Q_stricmpt(token, "firstperson")) {
            ScriptVars.firstPerson = qtrue;
        } else if (!Q_stricmpt(token, "thirdperson")) {
            ScriptVars.thirdPerson = qtrue;
        } else if (!Q_stricmpt(token, "shadow")) {
            ScriptVars.shadow = qtrue;
        } else if (!Q_stricmpt(token, "cullnear")) {
            ScriptVars.cullNear = qtrue;
        } else if (!Q_stricmpt(token, "cullradius")) {
            ScriptVars.cullRadius = qtrue;
		} else if (!Q_stricmpt(token, "culldistance")) {
			ScriptVars.cullDistance = qtrue;
        } else if (!Q_stricmpt(token, "depthhack")) {
            ScriptVars.depthHack = qtrue;
        } else if (!Q_stricmpt(token, "stencil")) {
            ScriptVars.stencil = qtrue;
        } else {
			if (!newLine) {
				Com_Printf("unknown render option '%s'\n", token);
				return NULL;
			} else {
				break;
			}
        }
		script = s;
        //Com_Printf("render option: '%s'\n", token);
    }

    return script;
}



qboolean CG_RunQ3mmeScript (const char *script, const char *emitterEnd)
{
    int braceCount;
    char token[MAX_QPATH];
    int err;
    qboolean newLine;
    float f;
    vec3_t *vsrc1, *vsrc2, *vdest;
    vec3_t tvec;
    const char *p;
    int i;
	int tokenId;
	qboolean fullLerp;
	//int startTime;
#if 0  //def CGAMESO
	struct timeval start, end;
	int64_t startf, endf;

#endif


#if 0  //def CGAMESO
	gettimeofday(&start, NULL);
	startf = start.tv_sec * 1000000.0 + start.tv_usec;
#endif


	//startTime = trap_Milliseconds();

    // moveGravity moveBounce moveImpact sink impact death

    //FIXME alot more from strings

    braceCount = 1;

    //Com_Printf("script: %s\n", script);

    while (1) {
        script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
		if (script == NULL) {
			Com_Printf("CG_RunQ3mmeScript() script == NULL\n");
			return qtrue;
		}
        if (script[0] == '\0') {  //(token[0] == '\0') {
			//Com_Printf("^3done\n");
            break;
			//Com_Printf("breaking\n");
        }

		if (SC_Cvar_Get_Int("debug_fxscript")) {
			if (!EmitterScript) {
				Com_Printf("script token: '%s'\n", token);
			}
		}

		//Com_Printf("token: %s\n", token);

		if (!tokenId) {
			if (token[0] == '\0') {
				continue;
			}

			if (token[0] == '{') {
				braceCount++;
			} else if (token[0] == '}') {
				braceCount--;
			}
		}

        if (braceCount == 0  ||  (emitterEnd  &&  script >= emitterEnd)) {

#if 0  //def CGAMESO
			gettimeofday(&end, NULL);
			endf = end.tv_sec * 1000000.0 + end.tv_usec;
			Com_Printf("%lld  ->  %lld\n", endf - startf, QstrcmpCount);
			QstrcmpCount = 0;
#endif
			//Com_Printf("^2emitter end\n");

            return qfalse;
        }

#if 0  //def CGAMESO
	gettimeofday(&start, NULL);
	startf = start.tv_sec * 1000000.0 + start.tv_usec;
#endif

	// tks CG_RunQ3mmeScript
	if (tokenId) {
		//Com_Printf("runq3mmescript tokenid %d\n", tokenId);
		switch (tokenId) {
		case TOKEN_RED:
			goto redtoken;
			break;
		case TOKEN_GREEN:
			goto greentoken;
			break;
		case TOKEN_BLUE:
			goto bluetoken;
			break;
		case TOKEN_COLOR:
			goto colortoken;
			break;
		case TOKEN_ALPHA:
			goto alphatoken;
			break;
		case TOKEN_SHADER:
			goto shadertoken;
			break;
		case TOKEN_MODEL:
			goto modeltoken;
			break;
		case TOKEN_SIZE:
			goto sizetoken;
			break;
		case TOKEN_WIDTH:
			goto widthtoken;
			break;
		case TOKEN_ANGLE:
			goto angletoken;
			break;
		case TOKEN_T0:
			goto t0token;
			break;
		case TOKEN_T1:
			goto t1token;
			break;
		case TOKEN_T2:
			goto t2token;
			break;
		case TOKEN_T3:
			goto t3token;
			break;
		case TOKEN_T4:
			goto t4token;
			break;
		case TOKEN_T5:
			goto t5token;
			break;
		case TOKEN_T6:
			goto t6token;
			break;
		case TOKEN_T7:
			goto t7token;
			break;
		case TOKEN_T8:
			goto t8token;
			break;
		case TOKEN_T9:
			goto t9token;
			break;
		case TOKEN_VELOCITY0:
			goto velocity0token;
			break;
		case TOKEN_VELOCITY1:
			goto velocity1token;
			break;
		case TOKEN_VELOCITY2:
			goto velocity2token;
			break;
		case TOKEN_ORIGIN0:
			goto origin0token;
			break;
		case TOKEN_ORIGIN1:
			goto origin1token;
			break;
		case TOKEN_ORIGIN2:
			goto origin2token;
			break;
		case TOKEN_END0:
			goto end0token;
			break;
		case TOKEN_END1:
			goto end1token;
			break;
		case TOKEN_END2:
			goto end2token;
			break;
		case TOKEN_DIR0:
			goto dir0token;
			break;
		case TOKEN_DIR1:
			goto dir1token;
			break;
		case TOKEN_DIR2:
			goto dir2token;
			break;
		case TOKEN_ANGLES0:
			goto angles0token;
			break;
		case TOKEN_ANGLES1:
			goto angles1token;
			break;
		case TOKEN_ANGLES2:
			goto angles2token;
			break;
		case TOKEN_V00:
			goto v00token;
			break;
		case TOKEN_V01:
			goto v01token;
			break;
		case TOKEN_V02:
			goto v02token;
			break;
		case TOKEN_V10:
			goto v10token;
			break;
		case TOKEN_V11:
			goto v11token;
			break;
		case TOKEN_V12:
			goto v12token;
			break;
		case TOKEN_V20:
			goto v20token;
			break;
		case TOKEN_V21:
			goto v21token;
			break;
		case TOKEN_V22:
			goto v22token;
			break;
		case TOKEN_V30:
			goto v30token;
			break;
		case TOKEN_V31:
			goto v31token;
			break;
		case TOKEN_V32:
			goto v32token;
			break;
		case TOKEN_V40:
			goto v40token;
			break;
		case TOKEN_V41:
			goto v41token;
			break;
		case TOKEN_V42:
			goto v42token;
			break;
		case TOKEN_V50:
			goto v50token;
			break;
		case TOKEN_V51:
			goto v51token;
			break;
		case TOKEN_V52:
			goto v52token;
			break;
		case TOKEN_V60:
			goto v60token;
			break;
		case TOKEN_V61:
			goto v61token;
			break;
		case TOKEN_V62:
			goto v62token;
			break;
		case TOKEN_V70:
			goto v70token;
			break;
		case TOKEN_V71:
			goto v71token;
			break;
		case TOKEN_V72:
			goto v72token;
			break;
		case TOKEN_V80:
			goto v80token;
			break;
		case TOKEN_V81:
			goto v81token;
			break;
		case TOKEN_V82:
			goto v82token;
			break;
		case TOKEN_V90:
			goto v90token;
			break;
		case TOKEN_V91:
			goto v91token;
			break;
		case TOKEN_V92:
			goto v92token;
			break;
		case TOKEN_SCALE:
			goto scaletoken;
			break;
		case TOKEN_ADD:
			goto addtoken;
			break;
		case TOKEN_ADDSCALE:
			goto addscaletoken;
			break;
		case TOKEN_SUB:
			goto subtoken;
			break;
		case TOKEN_SUBSCALE:
			goto subscaletoken;
			break;
		case TOKEN_COPY:
			goto copytoken;
			break;
		case TOKEN_CLEAR:
			goto cleartoken;
			break;
		case TOKEN_WOBBLE:
			goto wobbletoken;
			break;
		case TOKEN_RANDOM:
			goto randomtoken;
			break;
		case TOKEN_NORMALIZE:
			goto normalizetoken;
			break;
		case TOKEN_PERPENDICULAR:
			goto perpendiculartoken;
			break;
		case TOKEN_CROSS:
			goto crosstoken;
			break;
		case TOKEN_SPRITE:
			goto spritetoken;
			break;
		case TOKEN_SPARK:
			goto sparktoken;
			break;
		case TOKEN_QUAD:
			goto quadtoken;
			break;
		case TOKEN_BEAM:
			goto beamtoken;
			break;
		case TOKEN_LIGHT:
			goto lighttoken;
			break;
		case TOKEN_DISTANCE:
			goto distancetoken;
			break;
		case TOKEN_EMITTER:
			goto emittertoken;
			break;
		case TOKEN_EMITTERF:
			goto emitterftoken;
			break;
		case TOKEN_ALPHAFADE:
			goto alphafadetoken;
			break;
		case TOKEN_CULLDISTANCEVALUE:
			goto culldistancevaluetoken;
			break;
		case TOKEN_SHADERTIME:
			goto shadertimetoken;
			break;
		case TOKEN_DIRMODEL:
			goto dirmodeltoken;
			break;
		case TOKEN_LOOPSOUND:
			goto loopsoundtoken;
			break;
		case TOKEN_ROTATE:
			goto rotatetoken;
			break;
		case TOKEN_VIBRATE:
			goto vibratetoken;
			break;
		case TOKEN_EMITTERID:
			goto emitteridtoken;
			break;
		case TOKEN_DECAL:
			goto decaltoken;
			break;
		case TOKEN_SOUND:
			goto soundtoken;
			break;
		case TOKEN_SOUNDLOCAL:
			goto soundlocaltoken;
			break;
		case TOKEN_SOUNDWEAPON:
			goto soundweapontoken;
			break;
		case TOKEN_ANGLESMODEL:
			goto anglesmodeltoken;
			break;
		case TOKEN_AXISMODEL:
			goto axismodeltoken;
			break;
		case TOKEN_INTERVAL:
			goto intervaltoken;
			break;
		case TOKEN_COLORFADE:
			goto colorfadetoken;
			break;
		case TOKEN_PUSHPARENT:
			goto pushparenttoken;
			break;
		case TOKEN_POP:
			goto poptoken;
			break;
		case TOKEN_IF:
			goto iftoken;
			break;
#if 0  //FIXME
		case TOKEN_ELSE:
			goto elsetoken;
			break;
		case TOKEN_ELIF:
			goto eliftoken;
			break;
#endif
		case TOKEN_REPEAT:
			goto repeattoken;
			break;
		case TOKEN_ROTATEAROUND:
			goto rotatearoundtoken;
			break;
		case TOKEN_PARENTORIGIN:
			goto parentorigintoken;
			break;
		case TOKEN_MOVEGRAVITY:
			goto movegravitytoken;
			break;
		case TOKEN_SOUNDLIST:
			goto soundlisttoken;
			break;
		case TOKEN_SOUNDLISTLOCAL:
			goto soundlistlocaltoken;
			break;
		case TOKEN_SOUNDLISTWEAPON:
			goto soundlistweapontoken;
			break;
		case TOKEN_SHADERCLEAR:
			goto shadercleartoken;
			break;
		case TOKEN_EXTRASHADER:
			goto extrashadertoken;
			break;
		case TOKEN_EXTRASHADERCLEAR:
			goto extrashadercleartoken;
			break;
		case TOKEN_EXTRASHADERENDTIME:
			goto extrashaderendtimetoken;
			break;
		case TOKEN_TRACE:
			goto tracetoken;
			break;
		case TOKEN_DECALTEMP:
			goto decaltemptoken;
			break;
		case TOKEN_MODELLIST:
			goto modellisttoken;
			break;
		case TOKEN_SHADERLIST:
			goto shaderlisttoken;
			break;
		case TOKEN_YAW:
			goto yawtoken;
			break;
		case TOKEN_PITCH:
			goto pitchtoken;
			break;
		case TOKEN_ROLL:
			goto rolltoken;
			break;
		case TOKEN_MOVEBOUNCE:
			goto movebouncetoken;
			break;
		case TOKEN_SINK:
			goto sinktoken;
			break;
		case TOKEN_IMPACT:
			goto impacttoken;
			break;
		case TOKEN_PARENTVELOCITY:
			goto parentvelocitytoken;
			break;
		case TOKEN_INVERSE:
			goto inversetoken;
			break;
		case TOKEN_RINGS:
			goto ringstoken;
			break;
		case TOKEN_ECHO:
			goto echotoken;
			break;
		case TOKEN_RETURN:
			goto returntoken;
			break;
		case TOKEN_CONTINUE:
			goto continuetoken;
			break;
		case TOKEN_COMMAND:
			goto commandtoken;
			break;
		case TOKEN_ANIMFRAME:
			goto animframetoken;
			break;
		default:
			Com_Printf("^1CG_RunQ3mmeScript() unhandled token %d '%s'\n", tokenId, token);
			break;
		}
	}
		if (0) {

		} else if (!Q_stricmpt(token, "red")) {
		redtoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[0], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;
		} else if (!Q_stricmpt(token, "green")) {
		greentoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[1], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;
		} else if (!Q_stricmpt(token, "blue")) {
		bluetoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[2], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;
		} else if (!Q_stricmpt(token, "color")) {
		colortoken:
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			ScriptVars.color[0] = atof(token);
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			ScriptVars.color[1] = atof(token);
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			ScriptVars.color[2] = atof(token);
			goto handledToken;
        } else if (!Q_stricmpt(token, "alpha")) {
		alphatoken:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.color[3], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "shader")) {
		shadertoken:
			//Com_Printf("about to get shader\n");
            script = CG_GetFxToken(script, ScriptVars.shader, qtrue, &newLine, &tokenId);
			goto handledToken;
        } else if (!Q_stricmpt(token, "model")) {
		modeltoken:
            script = CG_GetFxToken(script, ScriptVars.model, qtrue, &newLine, &tokenId);
			goto handledToken;
        } else if (!Q_stricmpt(token, "size")) {
		sizetoken:
            //Com_Printf("script size\n");
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.size, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "width")) {
		widthtoken:
            //Com_Printf("script width\n");
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.width, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "angle")) {
		angletoken:
            //Com_Printf("script angle\n");
            err = 0;
            //script = CG_Q3mmeMath(script, &ScriptVars.angle, &err);
            script = CG_Q3mmeMath(script, &ScriptVars.rotate, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t0")) {
		t0token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t0, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t1")) {
		t1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t1, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t2")) {
		t2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t2, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t3")) {
		t3token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t3, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t4")) {
		t4token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t4, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t5")) {
		t5token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t5, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t6")) {
		t6token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t6, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t7")) {
		t7token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t7, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t8")) {
		t8token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t8, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "t9")) {
		t9token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t9, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity0")) {
		velocity0token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity1")) {
		velocity1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "velocity2")) {
		velocity2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin0")) {
		origin0token:
			//Com_Printf("^5origin0 script xxx rest '%s'\n", PrintShort(script, 10));
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin1")) {
		origin1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "origin2")) {
		origin2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "end0")) {
		end0token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.end[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "end1")) {
		end1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.end[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "end2")) {
		end2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.end[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir0")) {
		dir0token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir1")) {
		dir1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "dir2")) {
		dir2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles0")) {
		angles0token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles1")) {
		angles1token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "angles2")) {
		angles2token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v00")) {
		v00token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v01")) {
		v01token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v02")) {
		v02token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v10")) {
		v10token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v11")) {
		v11token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v12")) {
		v12token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v20")) {
		v20token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v21")) {
		v21token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v22")) {
		v22token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v30")) {
		v30token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v31")) {
		v31token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v32")) {
		v32token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v40")) {
		v40token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v41")) {
		v41token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v42")) {
		v42token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v50")) {
		v50token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v51")) {
		v51token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v52")) {
		v52token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v60")) {
		v60token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v61")) {
		v61token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v62")) {
		v62token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v70")) {
		v70token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v71")) {
		v71token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v72")) {
		v72token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v80")) {
		v80token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v81")) {
		v81token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v82")) {
		v82token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v90")) {
		v90token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v91")) {
		v91token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "v92")) {
		v92token:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "scale")) {
		scaletoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
#if 0
            (*vdest)[0] = (*vsrc1)[0] * f;
            (*vdest)[1] = (*vsrc1)[1] * f;
            (*vdest)[2] = (*vsrc1)[2] * f;
#endif
			VectorScale(*vsrc1, f, *vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "add")) {
		addtoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorAdd(*vsrc1, *vsrc2, *vdest);
			goto handledToken;

        } else if (!Q_stricmpt(token, "addscale")) {
		addscaletoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
			//Com_Printf("src2 '%s'\n", token);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            tvec[0] = (*vsrc2)[0] * f;
            tvec[1] = (*vsrc2)[1] * f;
            tvec[2] = (*vsrc2)[2] * f;
            //VectorAdd(*vsrc1, tvec, *vdest);
			//VectorMA(start, fraction, dir, intersection);

			//VectorCopy(*vsrc2, tvec);
			//VectorNormalize(tvec);
			VectorMA(*vsrc1, f, *vsrc2, *vdest);
			//VectorMA(*vsrc1, f, tvec, *vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "sub")) {
		subtoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorSubtract(*vsrc1, *vsrc2, *vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "subscale")) {
		subscaletoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            tvec[0] = (*vsrc2)[0] * f;
            tvec[1] = (*vsrc2)[1] * f;
            tvec[2] = (*vsrc2)[2] * f;
            VectorSubtract(*vsrc1, tvec, *vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "copy")) {
		copytoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorCopy(*vsrc1, *vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "clear")) {
		cleartoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorClear(*vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "wobble")) {
		wobbletoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            CG_FX_Wobble(*vsrc1, *vdest, ScriptVars.origin, f);
			goto handledToken;
        } else if (!Q_stricmpt(token, "random")) {
		randomtoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
#if 0
            (*vdest)[0] = 1000.0 * crandom();
            (*vdest)[1] = 1000.0 * crandom();
            (*vdest)[2] = 1000.0 * crandom();
#endif
            (*vdest)[0] = crandom();
            (*vdest)[1] = crandom();
            (*vdest)[2] = crandom();
            VectorNormalize(*vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "normalize")) {
		normalizetoken:
            // note: can be 1 or 2 args
            //
            // normalize <src> <dst> Normalize a vector so make it length 1
            //
            // but in base_weapons.fx:
            //
            // t0 dir
            // normalize dir
            //Com_Printf("normalize\n");
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            if (token[0] == '\0') {
                //Com_Printf("only 1 arg for normalize\n");
                vdest = vsrc1;
            } else {
                vdest = CG_GetScriptVector(token, tokenId);
            }
            VectorCopy(*vsrc1, *vdest);
            VectorNormalize(*vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "perpendicular")) {
		perpendiculartoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            PerpendicularVector(*vdest, *vsrc1);
			goto handledToken;
        } else if (!Q_stricmpt(token, "cross")) {
		crosstoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            CrossProduct(*vsrc1, *vsrc2, *vdest);
			goto handledToken;

        } else if (!Q_stricmpt(token, "sprite")) {
		spritetoken:
            // sprite <options> -> view aligned image -> origin, shader, color, size, angle
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Sprite(qtrue, qfalse);
			goto handledToken;
        } else if (!Q_stricmpt(token, "spark")) {
		sparktoken:
            // spark <options> -> view aligned spark -> origin, velocity, shader, color, size, width
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Spark();
			goto handledToken;
        } else if (!Q_stricmpt(token, "quad")) {
		quadtoken:
            // quad -> fixed aligned sprite -> origin, dir, shader, color, size, angle
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Sprite(qfalse, qfalse);
			goto handledToken;
        } else if (!Q_stricmpt(token, "beam")) {
		beamtoken:
            // beam -> image stretched between 2 points, origin, shader
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Beam();
			goto handledToken;
        } else if (!Q_stricmpt(token, "light")) {
		lighttoken:
			ScriptVars.emitterScriptEnd = script;
            CG_FX_Light();
			goto handledToken;
        } else if (!Q_stricmpt(token, "distance")) {
		distancetoken:
			{

            qboolean runBlock;
			vec3_t dir;
			vec3_t origOrigin;
			int count;

			//Com_Printf("running distance script:  distance %f\n", Distance(ScriptVars.lastDistancePosition, ScriptVars.origin));
			//Com_Printf("%f %f %f\n", ScriptVars.lastDistancePosition[0], ScriptVars.lastDistancePosition[1], ScriptVars.lastDistancePosition[2]);
			//Com_Printf("%f %f %f\n", ScriptVars.origin[0], ScriptVars.origin[1], ScriptVars.origin[2]);

            runBlock = qfalse;
            err = 0;
            CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
			if (parsingEmitterScriptLevel == 0) {
			if (DistanceScript) {
				if (ScriptVars.distance >= f  &&  f > 0.0) {
					runBlock = qtrue;
					//Com_Printf("running distance %f\n", ScriptVars.distance);
					ScriptVars.distance = 0;
				} else {
					//Com_Printf("not running distance %f\n", ScriptVars.distance);
				}
			} else {
				if (Distance(ScriptVars.lastDistancePosition, ScriptVars.origin) >= f  &&  f > 0.0) {
					//ScriptVars.lastDistanceTime = cg.ftime;
					runBlock = qtrue;
				}
			}
			if (runBlock  &&  ScriptVars.lastDistanceTime == -1) {
				runBlock = qfalse;
				VectorCopy(ScriptVars.origin, ScriptVars.lastDistancePosition);
				ScriptVars.lastDistanceTime = cg.ftime;
			}
            if (runBlock) {
				vec3_t newOrigin;
				float oldDistance;
				//ScriptVars_t svOrig;

				if (f < cg_fxScriptMinDistance.value) {
					f = cg_fxScriptMinDistance.value;
				}

				if (parsingEmitterScriptLevel) {
					//Com_Printf("dist %d\n", parsingEmitterScriptLevel);
				}

				//FIXME linear motion -- gravity needs to be taken into account

				VectorSubtract(ScriptVars.origin, ScriptVars.lastDistancePosition, dir);
				VectorNormalize(dir);
				VectorCopy(ScriptVars.origin, origOrigin);
				VectorCopy(ScriptVars.origin, newOrigin);

				count = 0;
				oldDistance = 0;  // silence gcc warning

				//memcpy(&svOrig, &ScriptVars, sizeof(svOrig));

				while (1) {  //(Distance(ScriptVars.lastDistancePosition, origOrigin) >= f) {
					vec_t d;

					d = Distance(ScriptVars.lastDistancePosition, origOrigin);
					if (d < f) {
						break;
					}

					// check if it overshoots
					if (count == 0) {
						// pass and just set oldDistance
					} else {
						if (d >= oldDistance) {
							//Com_Printf("^5overshooting:  old %f  new %f  count %d\n", oldDistance, d, count);
							break;
						}
					}
					oldDistance = d;
					count++;

					VectorMA(ScriptVars.lastDistancePosition, f, dir, newOrigin);
					VectorCopy(newOrigin, ScriptVars.origin);

					if (CG_RunQ3mmeScript(script, NULL)) {
						return qtrue;
					}
					VectorCopy(newOrigin, ScriptVars.lastDistancePosition);

					//Com_Printf("dist %f  step %f  count %d\n", Distance(ScriptVars.lastDistancePosition, origOrigin), f, count);
				}
				VectorCopy(origOrigin, ScriptVars.origin);
				VectorCopy(origOrigin, ScriptVars.lastDistancePosition);
				ScriptVars.lastDistanceTime = cg.ftime;
            }
			}
            script = CG_SkipBlock(script);
            if (!script) {
                return qtrue;
            }
			}
			goto handledToken;
		} else if (!Q_stricmpt(token, "emitter")) {
		emittertoken:

			fullLerp = qfalse;
			goto emitterblock;

        } else if (!Q_stricmpt(token, "emitterf")) {
		emitterftoken:

			fullLerp = qtrue;

		emitterblock:
			{
			localEntity_t *le;
			//char *debugstart;
			qboolean lastEmitterScriptValue;
			int lastEmittedEntities;
			int numEmittedEntities;

			if (cg.paused) {
				script = CG_SkipOpeningBrace(script);
				script = CG_SkipBlock(script);
				goto handledToken;
			}

			//debugstart = script;

            //Com_Printf("FIXME emitter\n");
            err = 0;
            CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
				//parsingEmitterScriptLevel--;
                return qtrue;
            }

			if (f <= 0.0) {
				script = CG_SkipOpeningBrace(script);
				script = CG_SkipBlock(script);
				goto handledToken;
			}

            parsingEmitterScriptLevel++;
			//Com_Printf("^3parsingEmitterScriptLevel %d\n", parsingEmitterScriptLevel);

			if (f < cg_fxScriptMinEmitter.value) {
				f = cg_fxScriptMinEmitter.value;
			}

            ScriptVars.emitterTime = f;
#if 0
			if (!Q_stricmpt(token, "emitterf")) {
				ScriptVars.emitterFullLerp = qtrue;
			} else {
				ScriptVars.emitterFullLerp = qfalse;
			}
#endif
			ScriptVars.emitterFullLerp = fullLerp;
			ScriptVars.emitterKill = qfalse;

            //Com_Printf("emitter %f\n", f);
            script = CG_SkipOpeningBrace(script);
            if (!script) {
				//parsingEmitterScriptLevel--;
                return qtrue;
            }
            ScriptVars.emitterScriptStart = script;
#if 0
			// hack to do things like light in emitter
			le = CG_AllocFxEntity(ScriptVars.emitterId);

			le->emitterScript = script;
			CG_CopyStoredScriptVarsToLE(le);
			le->fxType = LEFX_SCRIPT;
			le->startTime = cg.ftime;
			le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
			VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
			le->sv.lastDistanceTime = cg.ftime;
			le->sv.lastIntervalTime = cg.ftime;

#endif
			// just parsing and setting up for things to be run later
			ScriptJustParsing = qtrue;
			lastEmitterScriptValue = EmitterScript;  //FIXME hack to let emitter run from emitter
			EmitterScript = qfalse;
			lastEmittedEntities = EmittedEntities;
			EmittedEntities = 0;
            if (CG_RunQ3mmeScript(script, NULL)) {  //FIXME don't do things like light, have them check scriptlevel
				return qtrue;
			}
			EmitterScript = lastEmitterScriptValue;
			numEmittedEntities = EmittedEntities;
			EmittedEntities = lastEmittedEntities;
			ScriptJustParsing = qfalse;

			if (!numEmittedEntities) {
				//Com_Printf("non emitted\n");
				le = CG_AllocFxEntity(ScriptVars.emitterId);

				le->emitterScript = script;
				CG_CopyStoredScriptVarsToLE(le);
				le->fxType = LEFX_SCRIPT;
				le->startTime = cg.ftime;
				le->endTime = cg.ftime + ScriptVars.emitterTime * 1000.0;
				le->lifeRate = 1.0 / (le->endTime - le->startTime);

				VectorCopy(ScriptVars.origin, le->sv.lastDistancePosition);
				VectorCopy(ScriptVars.origin, le->sv.lastIntervalPosition);
				le->sv.lastDistanceTime = cg.ftime;
				le->sv.lastIntervalTime = cg.ftime;
			}

            script = CG_SkipBlock(script);
            if (!script) {
				//parsingEmitterScriptLevel--;
                return qtrue;
            }
            parsingEmitterScriptLevel--;
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "alphaFade")) {
		alphafadetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.alphaFade = f;
            ScriptVars.hasAlphaFade = qtrue;
			goto handledToken;
		} else if (!Q_stricmpt(token, "cullDistancevalue")) {
		culldistancevaluetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.cullDistanceValue = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "shaderTime")) {
		shadertimetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.shaderTime = f;
            ScriptVars.hasShaderTime = qtrue;
			goto handledToken;
        } else if (!Q_stricmpt(token, "dirmodel")) {
		dirmodeltoken:
			ScriptVars.emitterScriptEnd = script;
            //CG_FX_DirModel();
			CG_FX_Model(FX_MODEL_DIR);
			goto handledToken;
        } else if (!Q_stricmpt(token, "loopsound")) {
		loopsoundtoken:
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetFxToken(script, ScriptVars.loopSound, qtrue, &newLine, &tokenId);
			CG_FX_LoopSound();
			goto handledToken;
        } else if (!Q_stricmpt(token, "rotate")) {
		rotatetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.rotate = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "vibrate")) {
		vibratetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.vibrate = f;
			CG_FX_Vibrate();
			goto handledToken;
        } else if (!Q_stricmpt(token, "emitterId")) {
		emitteridtoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.emitterId = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "decal")) {
		decaltoken:
            // options:  energy alpha
			ScriptVars.emitterScriptEnd = script;
            ScriptVars.decalEnergy = qfalse;
            ScriptVars.decalAlpha = qfalse;
            while (!newLine) {
                script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
                if (token[0] == '\0') {
                    break;
                } else if (!Q_stricmpt(token, "energy")) {
                    ScriptVars.decalEnergy = qtrue;
                } else if (!Q_stricmpt(token, "alpha")) {
                    ScriptVars.decalAlpha = qtrue;
                } else {
                    Com_Printf("unknown render option '%s'\n", token);
                    return qtrue;
                }
                //Com_Printf("decal option: '%s'\n", token);
            }
            CG_FX_Decal();
			goto handledToken;
        } else if (!Q_stricmpt(token, "sound")) {
		soundtoken:
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetFxToken(script, ScriptVars.sound, qtrue, &newLine, &tokenId);
			CG_FX_Sound(CHAN_AUTO);
			goto handledToken;
        } else if (!Q_stricmpt(token, "soundlocal")) {
		soundlocaltoken:
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetFxToken(script, ScriptVars.sound, qtrue, &newLine, &tokenId);
			CG_FX_Sound(CHAN_LOCAL);
			goto handledToken;
        } else if (!Q_stricmpt(token, "soundweapon")) {
		soundweapontoken:
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetFxToken(script, ScriptVars.sound, qtrue, &newLine, &tokenId);
			CG_FX_Sound(CHAN_WEAPON);
			goto handledToken;
        } else if (!Q_stricmpt(token, "anglesmodel")) {
		anglesmodeltoken:
			ScriptVars.emitterScriptEnd = script;
            //CG_FX_AnglesModel();
			CG_FX_Model(FX_MODEL_ANGLES);
			goto handledToken;
		} else if (!Q_stricmpt(token, "axismodel")) {
		axismodeltoken:
			ScriptVars.emitterScriptEnd = script;
			CG_FX_Model(FX_MODEL_AXIS);
			goto handledToken;
        } else if (!Q_stricmpt(token, "interval")) {
		intervaltoken:
			{
            qboolean runBlock;
			double cgftimeOrig;
			double cgtimeOrig;

            runBlock = qfalse;
            err = 0;
            CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }

            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
			if (parsingEmitterScriptLevel == 0) {
			if (cg.ftime - ScriptVars.lastIntervalTime >= (f * 1000.0)) {
				//Com_Printf("^3interval %f\n", cg.ftime - ScriptVars.lastIntervalTime);
                runBlock = qtrue;
            }
			if (runBlock  &&  ScriptVars.lastIntervalTime == -1) {
				runBlock = qfalse;
				VectorCopy(ScriptVars.origin, ScriptVars.lastIntervalPosition);
				ScriptVars.lastIntervalTime = cg.ftime;
			}
            if (runBlock  &&  f > 0.0) {
				int count;

				if (f < cg_fxScriptMinInterval.value) {
					f = cg_fxScriptMinInterval.value;
				}

				cgftimeOrig = cg.ftime;
				cgtimeOrig = cg.time;

				cg.ftime = ScriptVars.lastIntervalTime;
				cg.time = cg.ftime;

				count = 0;
				//while (cgftimeOrig - ScriptVars.lastIntervalTime >= (f * 1000.0)) {
				//if (1) {  //while (cg.ftime < cgftimeOrig) {
				while (cg.ftime < cgftimeOrig) {
                    VectorCopy(ScriptVars.origin, ScriptVars.lastIntervalPosition);
                    //ScriptVars.lastIntervalTime = cg.ftime;

					cg.ftime += (f * 1000.0);
					cg.time = cg.ftime;

					if (CG_RunQ3mmeScript(script, NULL)) {
						return qtrue;
					}
                    ScriptVars.lastIntervalTime = cg.ftime;
					count++;
					if (count > MAX_COUNT) {
						//Com_Printf("^1interval max count\n");
						//break;
					}
					//Com_Printf("count: %d  %f < %f  f:%f\n", count, cg.ftime, cgftimeOrig, f);
				}

				cg.ftime = cgftimeOrig;
				cg.time = cgtimeOrig;
            }
			}
            script = CG_SkipBlock(script);
            if (!script) {
                return qtrue;
            }
			}
			goto handledToken;

		} else if (!Q_stricmpt(token, "colorfade")) {
		colorfadetoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.colorFade = f;
            ScriptVars.hasColorFade = qtrue;
			goto handledToken;
        } else if (!Q_stricmpt(token, "pushparent")) {
		pushparenttoken:
            // see color2 in weapon/rail/trail
            // color2 is stored in the parent input structure, retrieve it like this
            // pushparent color2
            // pop color
            if (ScriptVars.vecStackCount >= MAX_SCRIPTED_VARS_VECTOR_STACK) {
                Com_Printf("vecStack full:  %d >= MAX_SCRIPTED_VARS_VECTOR_STACK\n", ScriptVars.vecStackCount);
                return qtrue;
            }
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            if (!Q_stricmpt(token, "color1")) {
                VectorCopy(ScriptVars.color1, ScriptVars.vecStack[ScriptVars.vecStackCount]);
                ScriptVars.vecStackCount++;
            } else if (!Q_stricmpt(token, "color2")) {
                VectorCopy(ScriptVars.color2, ScriptVars.vecStack[ScriptVars.vecStackCount]);
                ScriptVars.vecStackCount++;
            } else {
                Com_Printf("pushparent: unknown token '%s'\n", token);
                return qtrue;
            }
			goto handledToken;
        } else if (!Q_stricmpt(token, "pop")) {
		poptoken:
            // see 'pushparent'
            if (ScriptVars.vecStackCount <= 0) {
                Com_Printf("pop error, vecStackCount %d\n", ScriptVars.vecStackCount);
                return qtrue;
            }
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorCopy(ScriptVars.vecStack[ScriptVars.vecStackCount - 1], *vdest);
            ScriptVars.vecStackCount--;
			goto handledToken;
        } else if (!Q_stricmpt(token, "if")) {
		iftoken:
			{
            int numIfBlocks;
            const char *endOfAllBlocks;

            //Com_Printf("script: 'if'\n");
            numIfBlocks = CG_NumIfBlocks(script, &endOfAllBlocks);
            //Com_Printf("num if blocks %d\n", numIfBlocks);
            if (numIfBlocks <= 0) {
                Com_Printf("error number of 'if' blocks %d\n", numIfBlocks);
                return qtrue;
            }

            for (i = 0;  i < numIfBlocks;  i++) {
                if (i > 0) {
                    while (1) {
                        script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
                        if (script[0] == '\0') {
                            //Com_Printf("if block %d eof\n", i);
                            return qtrue;
                        }
                        if (token[0] == '\0') {
                            continue;
                        }
                        break;
                    }
                }
                //Com_Printf("ifblocks token '%s'\n", token);
                if (i > 0) {
                    if (!Q_stricmpt(token, "else")) {
                        // check for 'else if'
                        p = script;
                        while (1) {
                            p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                            if (p[0] == '\0') {
                                //Com_Printf("parsing else block eof\n");
                                return qtrue;
                            }
                            if (token[0] == '\0') {
                                continue;
                            }
                            break;
                        }
                        //Com_Printf("else token: '%s'\n", token);
                        if (!Q_stricmpt(token, "if")) {
                            // pass, go on to test condition
                            script = p;
                            //Com_Printf("iiii '%s'\n", script);
                        } else {
                            // just plain old 'else'
                            //Com_Printf("else: '%s'\n", script);
                            script = CG_SkipOpeningBrace(script);
                            if (!script) {
                                return qtrue;
                            }
                            //Com_Printf("running else block\n");
                            if (CG_RunQ3mmeScript(script, NULL)) {
								return qtrue;
							}
                            break;
                        }
                    } else if (!Q_stricmpt(token, "elif")) {
                        // pass, go on to test condition
                    }
                }

                err = 0;
                //script = CG_Q3mmeMath(script, &f, &err);
                // function snarfs trailing brace
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }

                //Com_Printf("test %f\n", f);
                //Com_Printf("test '%s'\n", script);
                script = CG_SkipOpeningBrace(script);
                if (!script) {
                    return qtrue;
                }
                if (f != 0.0) {  //FIXME eps
                    //Com_Printf("running block %d\n", i + 1);
                    if (CG_RunQ3mmeScript(script, NULL)) {
						return qtrue;
					}
                    break;
                }
                script = CG_SkipBlock(script);
                if (!script) {
                    return qtrue;
                }
            }

            script = endOfAllBlocks;
            //Com_Printf("end 'if' token '%c'\n", script[0]);
            //Com_Printf("end 'if' '%s'\n", script);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "repeat")) {
		repeattoken:
			{
            //qboolean runBlock;
			float lastLoopVal;
			int lastLoopCount;

			lastLoopVal = ScriptVars.loop;
			lastLoopCount = ScriptVars.loopCount;

            //runBlock = qfalse;
            err = 0;
            CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
			if (f != 0) {
				for (i = 0;  i < f;  i++) {
					ScriptVars.loopCount = i;
					//Com_Printf("loop %d\n", i);
					ScriptVars.loop = (float)i / f;
					if (CG_RunQ3mmeScript(script, NULL)) {
						return qtrue;
					}
				}
			}

			ScriptVars.loop = lastLoopVal;
			ScriptVars.loopCount = lastLoopCount;
            script = CG_SkipBlock(script);
            if (!script) {
                return qtrue;
            }
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "rotatearound")) {
		rotatearoundtoken:
			{
			vec3_t tmp;

            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc1 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vsrc2 = CG_GetScriptVector(token, tokenId);
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            //FIXME double check if this is the right function
			//PerpendicularVector(tmp, *vsrc2);
			//RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
			VectorCopy(*vsrc1, tmp);
            //RotatePointAroundVector(*vdest, *vsrc2, *vsrc1, f);
            RotatePointAroundVector(*vdest, *vsrc2, tmp, f);
            //RotatePointAroundVector(*vdest, tmp, *vsrc1, f);
			//Com_Printf("rotatearound: distance %f\n", Distance(*vdest, *vsrc2));
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentorigin")) {
		parentorigintoken:
            Com_Printf("^3CG_RunQ3mmeScript() can't set parentOrigin\n");
            return qtrue;
			goto handledToken;
        } else if (!Q_stricmpt(token, "movegravity")) {
		movegravitytoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.moveGravity = f;
			ScriptVars.hasMoveGravity = qtrue;
			goto handledToken;
        } else if (!Q_stricmpt(token, "soundlist")) {
		soundlisttoken:
			{
            qboolean hasMathToken;

			ScriptVars.emitterScriptEnd = script;
            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                if (p[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '{') {
                    break;
                } else {
                    hasMathToken = qtrue;
                    break;
                }
            }
            if (hasMathToken) {
                err = 0;
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }
                //ScriptVars.selectedSoundList = f;
            } else {
                //ScriptVars.selectedSoundList = rand();  // modulo later
				f = random();
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
            ScriptVars.numSoundList = 0;
            while (1) {
                if (ScriptVars.numSoundList >= MAX_FX_SOUND_LIST) {
                    Com_Printf("ScriptVars.numSoundList >= MAX_FX_SOUND_LIST\n");
                    break;
                }
                script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
                if (script[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '}') {
                    break;
                }
                Q_strncpyz(ScriptVars.soundList[ScriptVars.numSoundList], token, MAX_QPATH);
                ScriptVars.numSoundList++;
            }
            if (ScriptVars.numSoundList <= 0) {
                Com_Printf("^1Error numSoundList <= 0\n");
                return qtrue;
            }

			ScriptVars.selectedSoundList = f * (float)(ScriptVars.numSoundList - 1);

#if 0
            if (!hasMathToken) {
                ScriptVars.selectedSoundList = ScriptVars.selectedSoundList % ScriptVars.numSoundList;
            } else {
				ScriptVars.selectedSoundList *= (ScriptVars.numSoundList - 1);
				Com_Printf("sound list selected: %d\n", ScriptVars.selectedSoundList);
			}
#endif

            if (ScriptVars.selectedSoundList >= ScriptVars.numSoundList) {
                Com_Printf("^1Error ScriptVars.selectedSoundList >= ScriptVars.numSoundList\n");
                return qtrue;
            }
			Q_strncpyz(ScriptVars.sound, ScriptVars.soundList[ScriptVars.selectedSoundList], MAX_QPATH);
			CG_FX_Sound(CHAN_AUTO);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "soundlistlocal")) {
		soundlistlocaltoken:
			{
            qboolean hasMathToken;

			ScriptVars.emitterScriptEnd = script;
            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                if (p[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '{') {
                    break;
                } else {
                    hasMathToken = qtrue;
                    break;
                }
            }
            if (hasMathToken) {
                err = 0;
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }
                //ScriptVars.selectedSoundList = f;
            } else {
                //ScriptVars.selectedSoundList = rand();  // modulo later
				f = random();
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
            ScriptVars.numSoundList = 0;
            while (1) {
                if (ScriptVars.numSoundList >= MAX_FX_SOUND_LIST) {
                    Com_Printf("ScriptVars.numSoundList >= MAX_FX_SOUND_LIST\n");
                    break;
                }
                script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
                if (script[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '}') {
                    break;
                }
                Q_strncpyz(ScriptVars.soundList[ScriptVars.numSoundList], token, MAX_QPATH);
                ScriptVars.numSoundList++;
            }
            if (ScriptVars.numSoundList <= 0) {
                Com_Printf("^1Error numSoundList <= 0\n");
                return qtrue;
            }

			ScriptVars.selectedSoundList = f * (float)(ScriptVars.numSoundList - 1);

#if 0
            if (!hasMathToken) {
                ScriptVars.selectedSoundList = ScriptVars.selectedSoundList % ScriptVars.numSoundList;
            } else {
				ScriptVars.selectedSoundList *= (ScriptVars.numSoundList - 1);
				Com_Printf("sound list selected: %d\n", ScriptVars.selectedSoundList);
			}
#endif

            if (ScriptVars.selectedSoundList >= ScriptVars.numSoundList) {
                Com_Printf("^1Error ScriptVars.selectedSoundList >= ScriptVars.numSoundList\n");
                return qtrue;
            }
			Q_strncpyz(ScriptVars.sound, ScriptVars.soundList[ScriptVars.selectedSoundList], MAX_QPATH);
			CG_FX_Sound(CHAN_LOCAL);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "soundlistweapon")) {
		soundlistweapontoken:
			{
            qboolean hasMathToken;

			ScriptVars.emitterScriptEnd = script;
            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                if (p[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '{') {
                    break;
                } else {
                    hasMathToken = qtrue;
                    break;
                }
            }
            if (hasMathToken) {
                err = 0;
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }
                //ScriptVars.selectedSoundList = f;
            } else {
                //ScriptVars.selectedSoundList = rand();  // modulo later
				f = random();
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
            ScriptVars.numSoundList = 0;
            while (1) {
                if (ScriptVars.numSoundList >= MAX_FX_SOUND_LIST) {
                    Com_Printf("ScriptVars.numSoundList >= MAX_FX_SOUND_LIST\n");
                    break;
                }
                script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
                if (script[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '}') {
                    break;
                }
                Q_strncpyz(ScriptVars.soundList[ScriptVars.numSoundList], token, MAX_QPATH);
                ScriptVars.numSoundList++;
            }
            if (ScriptVars.numSoundList <= 0) {
                Com_Printf("^1Error numSoundList <= 0\n");
                return qtrue;
            }

			ScriptVars.selectedSoundList = f * (float)(ScriptVars.numSoundList - 1);

#if 0
            if (!hasMathToken) {
                ScriptVars.selectedSoundList = ScriptVars.selectedSoundList % ScriptVars.numSoundList;
            } else {
				ScriptVars.selectedSoundList *= (ScriptVars.numSoundList - 1);
				Com_Printf("sound list selected: %d\n", ScriptVars.selectedSoundList);
			}
#endif

            if (ScriptVars.selectedSoundList >= ScriptVars.numSoundList) {
                Com_Printf("^1Error ScriptVars.selectedSoundList >= ScriptVars.numSoundList\n");
                return qtrue;
            }
			Q_strncpyz(ScriptVars.sound, ScriptVars.soundList[ScriptVars.selectedSoundList], MAX_QPATH);
			CG_FX_Sound(CHAN_WEAPON);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "shaderclear")) {
		shadercleartoken:
            // from base_weapons.fx:
            // Clear the previous shader so it uses the one in the model
			ScriptVars.shader[0] = '\0';
			goto handledToken;
		} else if (!Q_stricmpt(token, "extrashader")) {
		extrashadertoken:
			script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
			if (ScriptVars.parentCent  &&  *token) {
				//FIXME nomip?
				ScriptVars.parentCent->extraShader = trap_R_RegisterShader(token);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "extrashaderclear")) {
		extrashadercleartoken:
			if (ScriptVars.parentCent) {
				ScriptVars.parentCent->extraShader = 0;
			}
			goto handledToken;
		} else if (!Q_stricmpt(token, "extrashaderendtime")) {
		extrashaderendtimetoken:
			{
			float f;
			err = 0;
			script = CG_Q3mmeMath(script, &f, &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			if (ScriptVars.parentCent) {
				ScriptVars.parentCent->extraShaderEndTime = f;
			}
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "trace")) {
		tracetoken:
			{
			vec3_t endPoint;
			trace_t trace;

			VectorMA(ScriptVars.origin, 8192 * 16, ScriptVars.dir, endPoint);
			CG_Trace(&trace, ScriptVars.origin, NULL, NULL, endPoint, -1, CONTENTS_SOLID);
			VectorCopy(trace.endpos, ScriptVars.origin);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "decaltemp")) {
		decaltemptoken:
			ScriptVars.emitterScriptEnd = script;
            ScriptVars.decalEnergy = qfalse;
            ScriptVars.decalAlpha = qfalse;
            while (!newLine) {
                script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
                if (token[0] == '\0') {
                    break;
                } else if (!Q_stricmpt(token, "energy")) {
                    ScriptVars.decalEnergy = qtrue;
                } else if (!Q_stricmpt(token, "alpha")) {
                    ScriptVars.decalAlpha = qtrue;
                } else {
                    Com_Printf("unknown render option '%s'\n", token);
                    return qtrue;
                }
                //Com_Printf("decal option: '%s'\n", token);
            }
            CG_FX_DecalTemp();
			goto handledToken;
        } else if (!Q_stricmpt(token, "modellist")) {
		modellisttoken:
			{
            qboolean hasMathToken;

            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                if (p[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '{') {
                    break;
                } else {
                    hasMathToken = qtrue;
                    break;
                }
            }
            if (hasMathToken) {
                err = 0;
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }
                //ScriptVars.selectedModelList = f;  // fix later
				//Com_Printf("modellist math %f\n", f);
            } else {
                //ScriptVars.selectedModelList = rand();  // modulo later
				f = random();
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
            ScriptVars.numModelList = 0;
            while (1) {
                if (ScriptVars.numModelList >= MAX_FX_MODEL_LIST) {
                    Com_Printf("ScriptVars.numModelList >= MAX_FX_MODEL_LIST\n");
                    break;
                }
                script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
                if (script[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '}') {
                    break;
                }
                Q_strncpyz(ScriptVars.modelList[ScriptVars.numModelList], token, MAX_QPATH);
                ScriptVars.numModelList++;
            }
            if (ScriptVars.numModelList <= 0) {
                Com_Printf("^1Error numModelList <= 0\n");
                return qtrue;
            }

			ScriptVars.selectedModelList = f * (float)(ScriptVars.numModelList - 1);
			//Com_Printf("selected: %d  %f\n", ScriptVars.selectedModelList, f);

#if 0
            if (!hasMathToken) {
                //ScriptVars.selectedModelList = ScriptVars.selectedModelList % ScriptVars.numModelList;
				ScriptVars.selectedModelList = f * (float)(ScriptVars.numModelList - 1);
            } else {
				//ScriptVars.selectedModelList *= (float)(ScriptVars.numModelList - 1);
				Com_Printf("selected: %d\n", ScriptVars.selectedModelList);
			}
#endif
            if (ScriptVars.selectedModelList >= ScriptVars.numModelList) {
                Com_Printf("^1Error ScriptVars.selectedModelList >= ScriptVars.numModelList\n");
                return qtrue;
            }
			Q_strncpyz(ScriptVars.model, ScriptVars.modelList[ScriptVars.selectedModelList], MAX_QPATH);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "shaderlist")) {
		shaderlisttoken:
			{
            qboolean hasMathToken;

            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetFxToken(p, token, qfalse, &newLine, &tokenId);
                if (p[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '{') {
                    break;
                } else {
                    hasMathToken = qtrue;
                    break;
                }
            }
            if (hasMathToken) {
                err = 0;
                CG_Q3mmeMath(script, &f, &err);
                if (err) {
                    Com_Printf("^1math error\n");
                    return qtrue;
                }
            } else {
				f = random();
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
            ScriptVars.numShaderList = 0;
            while (1) {
                if (ScriptVars.numShaderList >= MAX_FX_SHADER_LIST) {
                    Com_Printf("ScriptVars.numShaderList >= MAX_FX_SHADER_LIST\n");
                    break;
                }
                script = CG_GetFxToken(script, token, qtrue, &newLine, &tokenId);
                if (script[0] == '\0') {
                    return qtrue;
                }
                if (token[0] == '\0') {
                    continue;
                }
                if (token[0] == '}') {
                    break;
                }
                Q_strncpyz(ScriptVars.shaderList[ScriptVars.numShaderList], token, MAX_QPATH);
                ScriptVars.numShaderList++;
            }
            if (ScriptVars.numShaderList <= 0) {
                Com_Printf("^1Error numShaderList <= 0\n");
                return qtrue;
            }

			ScriptVars.selectedShaderList = f * (float)(ScriptVars.numShaderList - 1);
            if (ScriptVars.selectedShaderList >= ScriptVars.numShaderList) {
                Com_Printf("^1Error ScriptVars.selectedShaderList >= ScriptVars.numShaderList\n");
                return qtrue;
            }
			Q_strncpyz(ScriptVars.shader, ScriptVars.shaderList[ScriptVars.selectedShaderList], MAX_QPATH);
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "yaw")) {
		yawtoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[YAW] = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "pitch")) {
		pitchtoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[PITCH] = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "roll")) {
		rolltoken:
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[ROLL] = f;
			goto handledToken;
        } else if (!Q_stricmpt(token, "movebounce")) {
		movebouncetoken:
			{
			const char *scriptOrig;

#if 0
            //FIXME double check two math args
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.moveBounce1 = f;
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.moveBounce2 = f;
			ScriptVars.hasMoveBounce = qtrue;
			Com_Printf("*movebounce\n");
#endif
			scriptOrig = script;
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			err = 0;
		    //Com_Printf("1 %p %p '%s'\n", scriptOrig, script, PrintShort(scriptOrig, 10));
			//Com_Printf("new '%s'\n", PrintShort(script, 10));
			//CG_Q3mmeMath(token, &f, &err);
			CG_Q3mmeMathExt(scriptOrig, script, &f, &err, qtrue);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.moveBounce1 = f;

			scriptOrig = script;
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			err = 0;
		    //Com_Printf("2 '%s'\n", PrintShort(scriptOrig, 10));
			//CG_Q3mmeMath(token, &f, &err);
			CG_Q3mmeMathExt(scriptOrig, script, &f, &err, qtrue);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.moveBounce2 = f;
			ScriptVars.hasMoveBounce = qtrue;
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "sink")) {
		sinktoken:
			{
			const char *scriptOrig;

            //FIXME double check two math args
#if 0
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.sink1 = f;
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.sink2 = f;
			ScriptVars.hasSink = qtrue;
			//Com_Printf("*sink\n");
			CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			Com_Printf("sink next token: '%s'\n", token);
#endif
			scriptOrig = script;
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			err = 0;
			//CG_Q3mmeMath(token, &f, &err);
			CG_Q3mmeMathExt(scriptOrig, script, &f, &err, qtrue);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.sink1 = f;

			scriptOrig = script;
			script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
			err = 0;
			//CG_Q3mmeMath(token, &f, &err);
			CG_Q3mmeMathExt(scriptOrig, script, &f, &err, qtrue);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.sink2 = f;
			ScriptVars.hasSink = qtrue;
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "impact")) {
		impacttoken:
			{
            qboolean runBlock;

            runBlock = qfalse;
            err = 0;
            CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            script = CG_SkipOpeningBrace(script);
            if (!script) {
                return qtrue;
            }
			if (ScriptVars.impacted  &&  VectorLength(ScriptVars.velocity) > f) {
				runBlock = qtrue;
			} else {
				ScriptVars.impacted = qfalse;
			}
            //FIXME test for block run
            if (runBlock) {
				vec3_t oldOrigin;
				qboolean escript;
				vec3_t oldDir;

				//Com_Printf("yes\n");
				VectorCopy(ScriptVars.origin, oldOrigin);
				VectorCopy(ScriptVars.impactOrigin, ScriptVars.origin);
				VectorCopy(ScriptVars.dir, oldDir);
				VectorCopy(ScriptVars.impactDir, ScriptVars.dir);

				escript = EmitterScript;
				EmitterScript = qfalse;
                if (CG_RunQ3mmeScript(script, NULL)) {
					return qtrue;
				}
				EmitterScript = escript;

				VectorCopy(oldOrigin, ScriptVars.origin);
				VectorCopy(oldDir, ScriptVars.dir);
				ScriptVars.impacted = qfalse;
            }
            script = CG_SkipBlock(script);
            if (!script) {
                return qtrue;
            }
			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "parentvelocity")) {
		parentvelocitytoken:
            Com_Printf("^3CG_RunQ3mmeScript() can't set parentVelocity\n");
            return qtrue;
			goto handledToken;
        } else if (!Q_stricmpt(token, "inverse")) {
		inversetoken:
            script = CG_GetFxToken(script, token, qfalse, &newLine, &tokenId);
            vdest = CG_GetScriptVector(token, tokenId);
            VectorInverse(*vdest);
			goto handledToken;
        } else if (!Q_stricmpt(token, "rings")) {
		ringstoken:
            //FIXME contenders weapon/rail/trail --  huh?
			ScriptVars.emitterScriptEnd = script;
            CG_FX_Rings();
			goto handledToken;
		} else if (!Q_stricmpt(token, "echo")) {
		echotoken:
			err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			Com_Printf("%f\n", f);
			//Com_Printf("%s\n", script);
			goto handledToken;
		} else if (!Q_stricmpt(token, "return")) {  // for testing
		returntoken:
			//Com_Printf("returning...\n");
			return qtrue;
			goto handledToken;
		} else if (!Q_stricmpt(token, "continue")) {  // for testing
		continuetoken:
			script = CG_SkipBlock(script);
			goto handledToken;
		} else if (!Q_stricmpt(token, "command")) {
		commandtoken:
			{
			char c;
			int count;
			char newToken[MAX_QPATH];
			char outputToken[MAX_STRING_CHARS];

			count = 0;
			memset(outputToken, 0, sizeof(outputToken));

			trap_autoWriteConfig(qfalse);
			cg.configWriteDisabled = qtrue;

			while (1) {
				c = script[0];
				//Com_Printf("^1script[0] '%c'\n", c);
				if (c == '\n'  ||  c == '\f'  ||  c == '\r'  ||  c == '\0') {
					if (count < (MAX_STRING_CHARS - 2)) {
						outputToken[count] = '\n';
						count++;
						outputToken[count] = '\0';
						//Com_Printf("^3'%s'\n", outputToken);
						trap_SendConsoleCommandNow(outputToken);
					} else {
						Com_Printf("^1ERROR: 'command' string too long\n");
					}
					break;
				}

				if (c == '%') {
					if (script[1] == '%') {
						script++;
					} else {
						i = 0;
						while (1) {
							script++;
							c = script[0];
							if (c < '0'  ||  c > 'z') {
								newToken[i] = '\0';
								//CG_Q3mmeMath(newToken, &f, &err);
								CG_Q3mmeMathExt(newToken, NULL, &f, &err, qfalse);
								//Com_Printf("last token = '%c'\n", c);
								//Com_Printf("'%s'  %f\n", newToken, f);
								//script--;
								break;
							}
							newToken[i] = c;
							i++;
						}

						//Com_Printf("*** old token '%s'\n", outputToken);
						Com_sprintf(&outputToken[count], (MAX_STRING_CHARS - 1) - count, "%f", f);
						//Com_Printf("*** new token '%s'\n", outputToken);
						count = strlen(outputToken);
						continue;
					}
				}

				outputToken[count] = c;
				count++;
				script++;

				//Com_Printf("xxxxx tk '%s'\n", outputToken);
			}

			// done

			}
			goto handledToken;
        } else if (!Q_stricmpt(token, "animframe")) {
		animframetoken:
            //Com_Printf("script size\n");
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.animFrame, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;

        } else if (token[0] != '\0'  &&  token[0] != '}') {
            Com_Printf("q3mme script unknown token: '%s'  tokenId %d  rest: '%s'\n", token, tokenId, script);
			//Com_Printf("%d\n", trap_Milliseconds() - startTime);
#if 0  //def CGAMESO
			gettimeofday(&end, NULL);
			endf = end.tv_sec * 1000000.0 + end.tv_usec;
			Com_Printf("%lld\n", endf - startf);
#endif
			return qtrue;
        }

handledToken:
		; // pass

    }

#if 0  //def CGAMESO
	gettimeofday(&end, NULL);
	endf = end.tv_sec * 1000000.0 + end.tv_usec;
	Com_Printf("%lld  ->  %lld\n", endf - startf, QstrcmpCount);
	QstrcmpCount = 0;
#endif

	return qfalse;

}

static void CG_SetQ3mmeFXScript (const char *name, char *dest, int destSize, fileHandle_t handle, char *lastLine)
{
    int len;
    int braceCount;
    char *line = NULL;
    char *p;
	qboolean skipLine = qfalse;
	qboolean skippedOpeningBrace = qfalse;

	if (EffectScripts.numEffects >= MAX_FX) {
		Com_Printf("CG_SetQ3mmeFXScripts() couldn't set >= MAX_FX\n");
		return;
	}

	Q_strncpyz(EffectScripts.names[EffectScripts.numEffects], name, sizeof(EffectScripts.names[EffectScripts.numEffects]));
	EffectScripts.ptr[EffectScripts.numEffects] = dest;
	EffectScripts.numEffects++;

    //braceCount = 1;
	braceCount = 0;

	// skip opening brace
	while (1) {
		if (lastLine[0] == '{') {
			braceCount++;
			line = lastLine + 1;
			skippedOpeningBrace = qtrue;
			skipLine = qtrue;
			break;
		}
		if (lastLine[0] == '\n'  ||  lastLine[0] == '\0') {
			break;
		}
		lastLine++;
		//Com_Printf("'%c'\n", lastLine[0]);
	}

	//Com_Printf("skipLine  %d '%s'\n", skipLine, line ? line : "nil");

    while (1) {
		if (!skipLine) {
			line = CG_FS_ReadLine(handle, &len);
			//Com_Printf("new linesssss '%s'\n", line);
			if (!len) {
				break;
			}
		}
		skipLine = qfalse;

		CG_CleanString(line);
		CG_LStrip(&line);
		CG_StripSlashComments(line);
		SC_Lowercase(line);

		//Com_Printf("here line: %d '%s'\n", (int)line[0], line);

        if (line[0] == '\n') {
            continue;
        }

        if (line[0] == '\0') {
            continue;
        }

		//Com_Printf("wtf\n");

		if (skippedOpeningBrace) {
			Q_strcat(dest, destSize, line);
		}

        p = line;
        while (1) {
            if (p[0] == '{') {
				if (!skippedOpeningBrace) {
					Q_strcat(dest, destSize, p + 1);
					skippedOpeningBrace = qtrue;
				}
                braceCount++;
            } else if (p[0] == '}') {
                braceCount--;
            } else if (p[0] == '\0') {
                break;
            }

            if (braceCount == 0) {
				if (skippedOpeningBrace) {
					goto alldone;
				}
            }
            p++;
        }
    }
 alldone:

    //Com_Printf("%s script:\n%s\n\n", name, dest);

    return;
}

static void CG_ParseQ3mmeEffect (const char *name, char *lastLine, fileHandle_t handle)
{
    char token[MAX_QPATH];
    char *line;
    int len;
    int braceCount;  //FIXME
	weaponEffects_t *wi;
    qboolean newLine;

	if (0) {

	} else if (!Q_stricmpt(name, "player/talk")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerTalk, sizeof(EffectScripts.playerTalk), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/impressive")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerImpressive, sizeof(EffectScripts.playerImpressive), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/excellent")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerExcellent, sizeof(EffectScripts.playerExcellent), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/holyshit")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerHolyshit, sizeof(EffectScripts.playerHolyshit), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/accuracy")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerAccuracy, sizeof(EffectScripts.playerAccuracy), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/gauntlet")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerGauntlet, sizeof(EffectScripts.playerGauntlet), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalComboKill")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalComboKill, sizeof(EffectScripts.playerMedalComboKill), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalMidAir")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalMidAir, sizeof(EffectScripts.playerMedalMidAir), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalRevenge")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalRevenge, sizeof(EffectScripts.playerMedalRevenge), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalFirstFrag")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalFirstFrag, sizeof(EffectScripts.playerMedalFirstFrag), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalRampage")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalRampage, sizeof(EffectScripts.playerMedalRampage), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalPerforated")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalPerforated, sizeof(EffectScripts.playerMedalPerforated), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalAccuracy")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalAccuracy, sizeof(EffectScripts.playerMedalAccuracy), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalHeadshot")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalHeadshot, sizeof(EffectScripts.playerMedalHeadshot), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalPerfect")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalPerfect, sizeof(EffectScripts.playerMedalPerfect), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/MedalQuadGod")) {  // new ql awards
		CG_SetQ3mmeFXScript(name, EffectScripts.playerMedalQuadGod, sizeof(EffectScripts.playerMedalQuadGod), handle, lastLine);
		return;

	} else if (!Q_stricmpt(name, "player/connection")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerConnection, sizeof(EffectScripts.playerConnection), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/defend")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerDefend, sizeof(EffectScripts.playerDefend), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/assist")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerAssist, sizeof(EffectScripts.playerAssist), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/capture")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerCapture, sizeof(EffectScripts.playerCapture), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/haste")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerHaste, sizeof(EffectScripts.playerHaste), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/flight")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerFlight, sizeof(EffectScripts.playerFlight), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/head/trail")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerHeadTrail, sizeof(EffectScripts.playerHeadTrail), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/torso/trail")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerTorsoTrail, sizeof(EffectScripts.playerTorsoTrail), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/legs/trail")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerLegsTrail, sizeof(EffectScripts.playerLegsTrail), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/quad")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerQuad, sizeof(EffectScripts.playerQuad), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/friend")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerFriend, sizeof(EffectScripts.playerFriend), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/teleportIn")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerTeleportIn, sizeof(EffectScripts.playerTeleportIn), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/teleportOut")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.playerTeleportOut, sizeof(EffectScripts.playerTeleportOut), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "common/jumpPad")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.jumpPad, sizeof(EffectScripts.jumpPad), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "common/headShot")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.headShot, sizeof(EffectScripts.headShot), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/gibbed")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.gibbed, sizeof(EffectScripts.gibbed), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "player/thawed")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.thawed, sizeof(EffectScripts.thawed), handle, lastLine);
	} else if (!Q_stricmpt(name, "weapon/common/bubbles")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.bubbles, sizeof(EffectScripts.bubbles), handle, lastLine);
		return;
	} else if (!Q_stricmpt(name, "weapon/common/impactFlesh")) {
		CG_SetQ3mmeFXScript(name, EffectScripts.impactFlesh, sizeof(EffectScripts.impactFlesh), handle, lastLine);
		return;


		////////////////////////////////



    } else if (!Q_stricmpt(name, "weapon/gauntlet/fire")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/gauntlet/flash")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/gauntlet/projectile")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/gauntlet/trail")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/gauntlet/impact")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/gauntlet/impactflesh")) {
        wi = &EffectScripts.weapons[WP_GAUNTLET];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/machinegun/fire")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/machinegun/flash")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/machinegun/projectile")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/machinegun/trail")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/machinegun/impact")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/machinegun/impactflesh")) {
        wi = &EffectScripts.weapons[WP_MACHINEGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/shotgun/fire")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/shotgun/flash")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/shotgun/projectile")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/shotgun/trail")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/shotgun/impact")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/shotgun/impactflesh")) {
        wi = &EffectScripts.weapons[WP_SHOTGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/grenade/fire")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grenade/flash")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grenade/projectile")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grenade/trail")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grenade/impact")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/grenade/impactflesh")) {
        wi = &EffectScripts.weapons[WP_GRENADE_LAUNCHER];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/rocket/fire")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rocket/flash")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rocket/projectile")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rocket/trail")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rocket/impact")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/rocket/impactflesh")) {
        wi = &EffectScripts.weapons[WP_ROCKET_LAUNCHER];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/lightning/fire")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/lightning/flash")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/lightning/projectile")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/lightning/trail")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/lightning/impact")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/lightning/impactflesh")) {
        wi = &EffectScripts.weapons[WP_LIGHTNING];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/rail/fire")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rail/flash")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rail/projectile")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rail/trail")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/rail/impact")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/rail/impactflesh")) {
        wi = &EffectScripts.weapons[WP_RAILGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/plasma/fire")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/plasma/flash")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/plasma/projectile")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/plasma/trail")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/plasma/impact")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/plasma/impactflesh")) {
        wi = &EffectScripts.weapons[WP_PLASMAGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/bfg/fire")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/bfg/flash")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/bfg/projectile")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/bfg/trail")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/bfg/impact")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/bfg/impactflesh")) {
        wi = &EffectScripts.weapons[WP_BFG];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/nailgun/fire")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/nailgun/flash")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/nailgun/projectile")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/nailgun/trail")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/nailgun/impact")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/nailgun/impactflesh")) {
        wi = &EffectScripts.weapons[WP_NAILGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/prox/fire")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/prox/flash")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/prox/projectile")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/prox/trail")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/prox/impact")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/prox/impactflesh")) {
        wi = &EffectScripts.weapons[WP_PROX_LAUNCHER];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/chaingun/fire")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/chaingun/flash")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/chaingun/projectile")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/chaingun/trail")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/chaingun/impact")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/chaingun/impactflesh")) {
        wi = &EffectScripts.weapons[WP_CHAINGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;


    } else if (!Q_stricmpt(name, "weapon/grapple/fire")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grapple/flash")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grapple/projectile")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grapple/trail")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/grapple/impact")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/grapple/impactflesh")) {
        wi = &EffectScripts.weapons[WP_GRAPPLING_HOOK];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/hmg/fire")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasFireScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->fireScript, sizeof(wi->fireScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/hmg/flash")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasFlashScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->flashScript, sizeof(wi->flashScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/hmg/projectile")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasProjectileScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->projectileScript, sizeof(wi->projectileScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/hmg/trail")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasTrailScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->trailScript, sizeof(wi->trailScript), handle, lastLine);
        return;
    } else if (!Q_stricmpt(name, "weapon/hmg/impact")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasImpactScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactScript, sizeof(wi->impactScript), handle, lastLine);
        return;
	} else if (!Q_stricmpt(name, "weapon/hmg/impactflesh")) {
        wi = &EffectScripts.weapons[WP_HEAVY_MACHINEGUN];
        wi->hasImpactFleshScript = qtrue;
        CG_SetQ3mmeFXScript(name, wi->impactFleshScript, sizeof(wi->impactFleshScript), handle, lastLine);
        return;



    } else if (EffectScripts.numExtra < MAX_FX_EXTRA) {
		CG_SetQ3mmeFXScript(name, EffectScripts.extra[EffectScripts.numExtra], MAX_FX_SCRIPT_SIZE, handle, lastLine);
		//Q_strncpyz(EffectScripts.extraNames[EffectScripts.numExtra], name, MAX_QPATH);
		//Com_Printf("^6extra effect: '%s'\n", EffectScripts.extraNames[EffectScripts.numExtra]);
		Com_Printf("^6extra effect:  '%s'\n", name);
		EffectScripts.numExtra++;
		return;
	} else {
		Com_Printf("^3Warning MAX_FX_EXTRA (%d) skipping '%s'\n", MAX_FX_EXTRA, name);
	}

    braceCount = 1;

    while (1) {
        line = CG_FS_ReadLine(handle, &len);
        if (!len) {
            break;
        }

		CG_CleanString(line);
        CG_LStrip(&line);
        CG_StripSlashComments(line);
		SC_Lowercase(line);

        if (line[0] == '\n') {
            continue;
        }

        while (1) {
			//FIXME const
			// don't use CG_GetFxToken() uses wrong newline status
            //line = (char *)CG_GetFxToken(line, token, qfalse, &newLine, &tokenId);
            line = (char *)CG_GetToken(line, token, qfalse, &newLine);
            if (token[0] == '\0') {
                break;
            }

            //Com_Printf("token: '%s'\n", token);
            if (token[0] == '{') {
                braceCount++;
                continue;
            } else if (token[0] == '}') {
                braceCount--;
            }

            if (braceCount == 0) {
                goto alldone;
            }
        }
    }
 alldone:

    return;
}

static void CG_ParseQ3mmeScripts (const char *fileName)
{
    fileHandle_t handle;
    char token[MAX_QPATH];
    //qboolean haveName;
    char *line;
    int len;
    qboolean newLine;

	if (!fileName) {
		return;
	}
	if (!*fileName) {
		return;
	}

    trap_FS_FOpenFile(fileName, &handle, FS_READ);
    if (!handle) {
        Com_Printf("couldn't open fx file: '%s'\n", fileName);
        return;
    }

    //haveName = qfalse;

    while (1) {
        line = CG_FS_ReadLine(handle, &len);
		//Com_Printf("line: '%s'\n", line);
        if (!len) {
			//Com_Printf("!len\n");
            break;
        }

		CG_CleanString(line);
        CG_LStrip(&line);
        CG_StripSlashComments(line);
		SC_Lowercase(line);

		//Com_Printf("line:  '%s'\n", line);

        if (line[0] == '\n') {
            continue;
        }

		//Com_Printf("line:  '%s'\n", line);
#if 0
        //FIXME testing
        //line[len - 2] = '\0';  // strip newline
        if (line[0] == '/'  &&  line[1] == '/') {
            continue;
        }
#endif
        //Com_Printf("line: '%s'\n", line);
		//FIXME const
		// not CG_GetFxToken() compiled with wrong newline status
        //line = (char *)CG_GetFxToken(line, token, qtrue, &newLine, &tokenId);
        line = (char *)CG_GetToken(line, token, qtrue, &newLine);
        if (token[0] == '\0') {
            continue;
        }
         Com_Printf("effect: '%s'\n", token);
         CG_ParseQ3mmeEffect(token, line, handle);
    }

    trap_FS_FCloseFile(handle);
}

static void CG_ResetFXScripting (void)
{
    parsingEmitterScriptLevel = 0;
	EmitterScript = qfalse;
	PlainScript = qfalse;
	ScriptJustParsing = qfalse;
	DistanceScript = qfalse;
	EmittedEntities = 0;
}

void CG_ResetFXIntervalAndDistance (centity_t *cent)
{
	VectorCopy(cent->lerpOrigin, cent->lastFlashDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastModelDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastTrailDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastImpactDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastLegsDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastTorsoDistancePosition);
	VectorCopy(cent->lerpOrigin, cent->lastHeadDistancePosition);

	VectorCopy(cent->lerpOrigin, cent->lastFlashIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastModelIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastTrailIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastImpactIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastLegsIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastTorsoIntervalPosition);
	VectorCopy(cent->lerpOrigin, cent->lastHeadIntervalPosition);

	cent->trailTime = cg.ftime;  //cg.snap->serverTime;

	cent->lastFlashIntervalTime = cg.ftime;
	cent->lastFlashDistanceTime = cg.ftime;

	cent->lastModelIntervalTime = cg.ftime;
	cent->lastModelDistanceTime = cg.ftime;

	cent->lastTrailIntervalTime = cg.ftime;
	cent->lastTrailDistanceTime = cg.ftime;

	cent->lastImpactIntervalTime = cg.ftime;
	cent->lastImpactDistanceTime = cg.ftime;

	cent->lastLegsIntervalTime = cg.ftime;
	cent->lastLegsDistanceTime = cg.ftime;

	cent->lastTorsoIntervalTime = cg.ftime;
	cent->lastTorsoDistanceTime = cg.ftime;

	cent->lastHeadIntervalTime = cg.ftime;
	cent->lastHeadDistanceTime = cg.ftime;
}

void CG_ReloadQ3mmeScripts (const char *fileName)
{
	int i;
	centity_t *cent;

	CG_RemoveFXLocalEntities(qtrue, 0);
	CG_FreeFxJitTokens();
	memset(&EffectScripts, 0, sizeof(EffectScripts));
	CG_ResetFXScripting();
	CG_ParseQ3mmeScripts(fileName);

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cent = &cg_entities[i];

		if (!cent->currentValid) {
			continue;
		}

		CG_ResetFXIntervalAndDistance(cent);
	}
}

void CG_ResetScriptVars (void)
{
	memset(&ScriptVars, 0, sizeof(ScriptVars));
	Vector4Set(ScriptVars.color, 1, 1, 1, 1);
	ScriptVars.size = 1;
	ScriptVars.width = 1;
	CG_ResetFXScripting();
}

void CG_CopyPlayerDataToScriptData (centity_t *cent)
{
	const clientInfo_t *ci;
	int clientNum;

	//Com_Printf("CG_CopyPlayerDataToScriptData() cent %d  es.num %d cnum %d\n", cent - cg_entities, cent->currentState.number, cent->currentState.clientNum);

	clientNum = cent->currentState.clientNum;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("CG_CopyPlayerDataToScriptData() invalid clientNum %d  %p  %ld\n", clientNum, cent, (long int)(cent - cg_entities));
		clientNum = 0;
	}

	ci = &cgs.clientinfo[clientNum];

	VectorCopy(cent->lerpOrigin, ScriptVars.origin);
	VectorCopy(cent->lerpAngles, ScriptVars.angles);
	VectorCopy(cent->currentState.pos.trDelta, ScriptVars.velocity);
	VectorCopy(cent->currentState.pos.trDelta, ScriptVars.dir);
	VectorNormalize(ScriptVars.dir);
	ScriptVars.team = ci->team;
	ScriptVars.clientNum = clientNum;
	ScriptVars.enemy = CG_IsEnemy(ci);
	ScriptVars.teamMate = CG_IsTeammate(ci);
	if (cg.freecam  ||  cg.renderingThirdPerson) {
		ScriptVars.inEyes = qfalse;
	} else {
		ScriptVars.inEyes = CG_IsUs(ci);
	}
	VectorCopy(ci->color1, ScriptVars.color1);
	VectorCopy(ci->color2, ScriptVars.color2);

	if (clientNum == cg.snap->ps.clientNum) {
		ScriptVars.parentCent =  &cg.predictedPlayerEntity;
	} else {
		ScriptVars.parentCent = cent;
	}

#if 0
	if (cent->currentState.clientNum == cg.snap->ps.clientNum) {
		ScriptVars.rewardImpressive = cg.rewardImpressive;
		ScriptVars.rewardExcellent = cg.rewardExcellent;
		ScriptVars.rewardHumiliation = cg.rewardHumiliation;
		ScriptVars.rewardDefend = cg.rewardDefend;
		ScriptVars.rewardAssist = cg.rewardAssist;
	} else {
		ScriptVars.rewardImpressive = qfalse;
		ScriptVars.rewardExcellent = qfalse;
		ScriptVars.rewardHumiliation = qfalse;
		ScriptVars.rewardDefend = qfalse;
		ScriptVars.rewardAssist = qfalse;
	}
#endif
}


/////

void CG_CopyPositionDataToCent (positionData_t *pd)
{
	VectorCopy(ScriptVars.lastIntervalPosition, pd->intervalPosition);
	pd->intervalTime = ScriptVars.lastIntervalTime;
	VectorCopy(ScriptVars.lastDistancePosition, pd->distancePosition);
	pd->distanceTime = ScriptVars.lastDistanceTime;
}

void CG_CopyPositionDataToScript (const positionData_t *pd)
{
	VectorCopy(pd->intervalPosition, ScriptVars.lastIntervalPosition);
	ScriptVars.lastIntervalTime = pd->intervalTime;
	VectorCopy(pd->distancePosition, ScriptVars.lastDistancePosition);
	ScriptVars.lastDistanceTime = pd->distanceTime;
}

void CG_UpdatePositionData (const centity_t *cent, positionData_t *pd)
{
	VectorCopy(cent->lerpOrigin, pd->intervalPosition);
	pd->intervalTime = cg.ftime;
	VectorCopy(cent->lerpOrigin, pd->distancePosition);
	pd->distanceTime = cg.ftime;
}

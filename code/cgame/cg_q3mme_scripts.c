#include "cg_local.h"

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
#define OP_CVAR 12  //FIXME probably not
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

#define USE_SWITCH 0

typedef enum {
	FX_MODEL_DIR,
	FX_MODEL_ANGLES,
	FX_MODEL_AXIS,
} fxModelType_t;

ScriptVars_t ScriptVars;
ScriptVars_t EmitterScriptVars;

static int parsingEmitterScriptLevel = 0;
qboolean EmitterScript = qfalse;
qboolean PlainScript = qfalse;  //FIXME hack
qboolean DistanceScript = qfalse;

static qboolean ScriptJustParsing = qfalse;  //FIXME more hack
effectScripts_t EffectScripts;

static int EmittedEntities = 0;

//static qboolean WithinComment = qfalse;

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

char *CG_Q3mmeMath (char *script, float *val, int *error)
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

    numOps = 0;
    while (1) {
        script = CG_GetToken(script, token, qfalse, &newLine);
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

                if (parenCount == 0  ||  p[0] == '\0'  ||  p[0] == '\n'  ||  (p[0] < ' '  ||  p[0] > '~')) {
                    memcpy(buffer, script, p - script);
                    buffer[p - script] = '\0';
                    //Com_Printf("paren(%d): %d '%s'\n", recursiveCount + 1, p - script, buffer);
                    err = 0;
                    CG_Q3mmeMath(buffer, &tmpVal, &err);
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

	if (USE_SWITCH) {
		switch(token[0]) {
		case '+':
			goto opplustoken;
		case '-':
			goto opnegativetoken;
		case '*':
			goto opmultiplytoken;
		case '/':
			goto opdividetoken;
		case '<':
			goto oplessthantoken;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			goto opnumbertoken;
		case 'l':  /**/
			switch(token[1]) {  // l
			case 'e':
				switch(token[2]) {  // le
				case 'r':
					switch(token[3]) {  // ler
					case 'p':
						switch(token[4]) {  // lerp
						case '\0':
							goto lerptoken;
						}
					}
				}
			}
		case 'o':  /**/
			switch(token[1]) {  // o
			case 'r':
				switch(token[2]) {  // or
				case 'i':
					switch(token[3]) {  // ori
					case 'g':
						switch(token[4]) {  // orig
						case 'i':
							switch(token[5]) {  // origi
							case 'n':
								switch(token[6]) {  // origin
								case '\0':
									goto origintoken;
								}
							}
						}
					}
				}
			}
		case 'v':  /**/
			switch(token[1]) {  // v
			case '1':
				switch(token[2]) {  // v1
				case '\0':
					goto v1token;
				}
			}
		}
	}
		if (0) {

        } else if (!USE_SWITCH  &&  token[0] == '+') {
		opplustoken:
            ops[numOps] = OP_PLUS;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '-') {
		opnegativetoken:
            ops[numOps] = OP_MINUS;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '*') {
		opmultiplytoken:
            ops[numOps] = OP_MULT;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '/') {
		opdividetoken:
            ops[numOps] = OP_DIV;
            numOps += 2;
			goto tokenHandled;
        } else if (!USE_SWITCH  &&  token[0] == '<') {
		oplessthantoken:
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
        } else if (!USE_SWITCH  &&  token[0] >= '0'  &&  token[0] <= '9') {  //((token[0] >= '0'  &&  token[0] <= '9')  ||  (token[0] == '-'  &&  (token[1] >= '0'  &&  token[1] <= '9'))) {
		opnumbertoken:
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = atof(token);
            numOps++;
			goto tokenHandled;

        } else if (!Q_stricmpt(token, "size")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.size;
            numOps += 2;
        } else if (!Q_stricmpt(token, "width")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.width;
            numOps += 2;
        } else if (!Q_stricmpt(token, "angle")) {
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.angle;
            ops[numOps + 1] = ScriptVars.rotate;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t0;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t1;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t2;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t3")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t3;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t4")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t4;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t5")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t5;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t6")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t6;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t7")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t7;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t8")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t8;
            numOps += 2;
        } else if (!Q_stricmpt(token, "t9")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.t9;
            numOps += 2;
        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "origin")) {
		origintoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.origin);
            numOps += 2;
			goto tokenHandled;
        } else if (!Q_stricmpt(token, "origin0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "origin1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "origin2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.origin[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "velocity")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.velocity);
            numOps += 2;
        } else if (!Q_stricmpt(token, "velocity0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "velocity1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "velocity2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.velocity[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "dir")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.dir);
            numOps += 2;
        } else if (!Q_stricmpt(token, "dir0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "dir1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "dir2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.dir[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "angles")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.angles);
            numOps += 2;
        } else if (!Q_stricmpt(token, "angles0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "angles1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "angles2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "yaw")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[YAW];
            numOps += 2;
        } else if (!Q_stricmpt(token, "pitch")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[PITCH];
            numOps += 2;
        } else if (!Q_stricmpt(token, "roll")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.angles[ROLL];
            numOps += 2;
#if 0  // stupid
        } else if (!Q_stricmpt(token, "color")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.color);
            numOps += 2;
        } else if (!Q_stricmpt(token, "color0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "color1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "color2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "color3")) {  // alpha
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.color[3];
            numOps += 2;
#endif
        } else if (!Q_stricmpt(token, "parentOrigin")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentOrigin);
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentOrigin0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentOrigin1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentOrigin2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentOrigin[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentvelocity")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentVelocity);
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentVelocity0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentVelocity1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentVelocity2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentVelocity[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentAngles")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentAngles);
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentAngles0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentAngles1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentAngles2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentangle")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngle;
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentyaw")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[YAW];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentpitch")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[PITCH];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentroll")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentAngles[ROLL];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentsize")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentSize;
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentDir")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentDir);
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentDir0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentDir1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentDir2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentDir[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentEnd")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.parentEnd);
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentEnd0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentEnd1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "parentEnd2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.parentEnd[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "end")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.end);
            numOps += 2;
        } else if (!Q_stricmpt(token, "end0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "end1")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "end2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.end[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "time")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = (cg.ftime - (float)cgs.levelStartTime) / 1000.0;
            //ops[numOps + 1] = (cg.time - (float)cgs.levelStartTime) / 1000.0;
            numOps += 2;
		} else if (!Q_stricmpt(token, "cgtime")) {
			ops[numOps] = OP_VAL;
			ops[numOps + 1] = cg.ftime;
			numOps += 2;
        } else if (!Q_stricmpt(token, "loop")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.loop;
            numOps += 2;
        } else if (!Q_stricmpt(token, "loopcount")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.loopCount;
            numOps += 2;
        } else if (!Q_stricmpt(token, "red")) {
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.red;
            ops[numOps + 1] = ScriptVars.color[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "green")) {
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.green;
            ops[numOps + 1] = ScriptVars.color[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "blue")) {
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.blue;
            ops[numOps + 1] = ScriptVars.color[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "alpha")) {
            ops[numOps] = OP_VAL;
            //ops[numOps + 1] = ScriptVars.alpha;
            ops[numOps + 1] = ScriptVars.color[3];
            numOps += 2;
        } else if (!Q_stricmpt(token, "pi")) {
            ops[numOps] = OP_VAL;
            numOps++;
            ops[numOps] = M_PI;
            numOps++;
        } else if (!Q_stricmpt(token, "rand")) {
            //FIXME how is rand set, it's supposed to be a fixed value ??
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = random();  //ScriptVars.rand;
			//Com_Printf("rand() %f\n", ops[numOps + 1]);
            numOps += 2;
        } else if (!Q_stricmpt(token, "crand")) {
            //FIXME how is crand set, it's supposed to be a fixed value ??
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = crandom();  //ScriptVars.crand;
            numOps += 2;
        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "lerp")) {
		lerptoken:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.lerp;
            numOps += 2;
			goto tokenHandled;
        } else if (!Q_stricmpt(token, "life")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.life;
            numOps += 2;
        } else if (!Q_stricmpt(token, "v0")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v0);
            numOps += 2;
        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "v1")) {
		v1token:
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v1);
            numOps += 2;
			goto tokenHandled;
        } else if (!Q_stricmpt(token, "v2")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v2);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v3")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v3);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v4")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v4);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v5")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v5);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v6")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v6);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v7")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v7);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v8")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v8);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v9")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = VectorLength(ScriptVars.v9);
            numOps += 2;
        } else if (!Q_stricmpt(token, "v00")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v01")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v02")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v0[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v10")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v11")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v12")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v1[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v20")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v21")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v22")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v2[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v30")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v31")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v32")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v3[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v40")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v41")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v42")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v4[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v50")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v51")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v52")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v5[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v60")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v61")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v62")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v6[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v70")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v71")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v72")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v7[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v80")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v81")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v82")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v8[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v90")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[0];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v91")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[1];
            numOps += 2;
        } else if (!Q_stricmpt(token, "v92")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.v9[2];
            numOps += 2;
        } else if (!Q_stricmpt(token, "rotate")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rotate;
            numOps += 2;
        } else if (!Q_stricmpt(token, "vibrate")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.vibrate;
            numOps += 2;
        } else if (!Q_stricmpt(token, "emitterid")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.emitterId;
            numOps += 2;
        } else if (!Q_stricmpt(token, "alphafade")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.alphaFade;
            numOps += 2;
        } else if (!Q_stricmpt(token, "colorfade")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.colorFade;
            numOps += 2;
        } else if (!Q_stricmpt(token, "movegravity")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.moveGravity;
            numOps += 2;
        } else if (!Q_stricmpt(token, "team")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.team;
            numOps += 2;
        } else if (!Q_stricmpt(token, "clientnum")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.clientNum;
            numOps += 2;
        } else if (!Q_stricmpt(token, "enemy")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.enemy;
            numOps += 2;
        } else if (!Q_stricmpt(token, "teammate")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.teamMate;
            numOps += 2;
        } else if (!Q_stricmpt(token, "ineyes")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.inEyes;
            numOps += 2;
#if 0
        } else if (!Q_stricmpt(token, "rewardImpressive")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardImpressive;
            numOps += 2;
        } else if (!Q_stricmpt(token, "rewardExcellent")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardExcellent;
            numOps += 2;
        } else if (!Q_stricmpt(token, "rewardHumiliation")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardHumiliation;
            numOps += 2;
        } else if (!Q_stricmpt(token, "rewardDefend")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardDefend;
            numOps += 2;
        } else if (!Q_stricmpt(token, "rewardAssist")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.rewardAssist;
            numOps += 2;
#endif
        } else if (!Q_stricmpt(token, "surfacetype")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = ScriptVars.surfaceType;
            numOps += 2;
        } else if (!Q_stricmpt(token, "gametype")) {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = cgs.gametype;
            numOps += 2;

        // functions //

        } else if (!Q_stricmpt(token, "sqrt")) {
            ops[numOps] = OP_FSQRT;
            numOps += 2;
        } else if (!Q_stricmpt(token, "ceil")) {
            ops[numOps] = OP_FCEIL;
            numOps += 2;
        } else if (!Q_stricmpt(token, "floor")) {
            ops[numOps] = OP_FFLOOR;
            numOps += 2;
        } else if (!Q_stricmpt(token, "sin")) {
            ops[numOps] = OP_FSIN;
            numOps += 2;
        } else if (!Q_stricmpt(token, "cos")) {
            ops[numOps] = OP_FCOS;
            numOps += 2;
        } else if (!Q_stricmpt(token, "wave")) {
            ops[numOps] = OP_FWAVE;
            numOps += 2;
        } else if (!Q_stricmpt(token, "clip")) {
            ops[numOps] = OP_FCLIP;
            numOps += 2;
        } else if (!Q_stricmpt(token, "acos")) {
            ops[numOps] = OP_FACOS;
            numOps += 2;
        } else if (!Q_stricmpt(token, "asin")) {
            ops[numOps] = OP_FASIN;
            numOps += 2;
        } else if (!Q_stricmpt(token, "atan")) {
            ops[numOps] = OP_FATAN;
            numOps += 2;
		} else if (!Q_stricmpt(token, "atan2")) {
			ops[numOps] = OP_FATAN2;
			numOps += 2;
        } else if (!Q_stricmpt(token, "tan")) {
            ops[numOps] = OP_FTAN;
            numOps += 2;
		} else if (!Q_stricmpt(token, "pow")) {
			ops[numOps] = OP_FPOW;
			numOps += 2;
		} else if (!Q_stricmpt(token, "inwater")) {
			ops[numOps] = OP_VAL;
			ops[numOps + 1] = CG_PointContents(ScriptVars.origin, -1) & CONTENTS_WATER;
			numOps += 2;
        } else if (token[0] != '(') {
            ops[numOps] = OP_VAL;
            ops[numOps + 1] = SC_Cvar_Get_Float(token);
            numOps += 2;
            if (verbose) {
                //Com_Printf("^3q3mme math unknown token: 's'\n", token);
                Com_Printf("cvar: '%s' %f\n", token, SC_Cvar_Get_Float(token));
            }
        }

	tokenHandled:
        if (newLine) {
            break;
        }
    }

    // sanity check
    if (ops[0] >= OP_PLUS  &&  ops[0] <= OP_OR  &&  ops[0] != OP_MINUS) {
        *error = 1;
        Com_Printf("q3mme math invalid first token(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
        recursiveCount--;
        return script;
    }

    if (numOps > 0  &&  (ops[numOps - 2] >= OP_PLUS  &&  ops[numOps - 2] <= OP_OR)) {
        *error = 2;
        Com_Printf("q3mme math invalid last token(%d %d): %f\n", recursiveCount, uniqueId, ops[numOps - 2]);
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
                Com_Printf("q3mme math invalid function value type (%d %d): %f\n", recursiveCount, uniqueId, ops[i + 2]);
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
            Com_Printf("q3mme math unknown function %f\n", ops[i]);
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
                    Com_Printf("q3mme math two operands following each other(%d %d)\n", recursiveCount, uniqueId);
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
				CG_Printf("^3fxmath divide by zero\n");
			}
#endif
            ops[i - 1] /= ops[i + 3];
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

    // < > ! = & |

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
        } else if (ops[i] == OP_AND) {
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
        Com_Printf("q3mme math invalid final value op(%d %d): %f\n", recursiveCount, uniqueId, ops[0]);
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

static vec3_t *CG_GetScriptVector (char *name)
{
	char *token;

	token = name;

 if (USE_SWITCH) {
	switch(token[0]) {
	case 'o':  /**/
		switch(token[1]) {  // o
		case 'r':
			switch(token[2]) {  // or
			case 'i':
				switch(token[3]) {  // ori
				case 'g':
					switch(token[4]) {  // orig
					case 'i':
						switch(token[5]) {  // origi
						case 'n':
							switch(token[6]) {  // origin
							case '\0':
								goto origintoken;
							}
						}
					}
				}
			}
		}
	case 'v':  /**/
		switch(token[1]) {  // v
		case '1':
			switch(token[2]) {  // v1
			case '\0':
				goto v1token;
			}
		}
	}
 }

    if (!Q_stricmpt(name, "v0")) {
        return &ScriptVars.v0;
    } else if (!USE_SWITCH  &&  !Q_stricmpt(name, "v1")) {
	v1token:
        return &ScriptVars.v1;
    } else if (!Q_stricmpt(name, "v2")) {
        return &ScriptVars.v2;
    } else if (!Q_stricmpt(name, "v3")) {
        return &ScriptVars.v3;
    } else if (!Q_stricmpt(name, "v4")) {
        return &ScriptVars.v4;
    } else if (!Q_stricmpt(name, "v5")) {
        return &ScriptVars.v5;
    } else if (!Q_stricmpt(name, "v6")) {
        return &ScriptVars.v6;
    } else if (!Q_stricmpt(name, "v7")) {
        return &ScriptVars.v7;
    } else if (!Q_stricmpt(name, "v8")) {
        return &ScriptVars.v8;
    } else if (!Q_stricmpt(name, "v9")) {
        return &ScriptVars.v9;
    } else if (!USE_SWITCH  &&  !Q_stricmpt(name, "origin")) {
	origintoken:
        return &ScriptVars.origin;
    } else if (!Q_stricmpt(name, "velocity")) {
        return &ScriptVars.velocity;
    } else if (!Q_stricmpt(name, "dir")) {
        return &ScriptVars.dir;
    } else if (!Q_stricmpt(name, "angles")) {
        return &ScriptVars.angles;
    } else if (!Q_stricmpt(name, "color")) {
        return (vec3_t *)&ScriptVars.color;
    } else if (!Q_stricmpt(name, "parentOrigin")) {
        return &ScriptVars.parentOrigin;
    } else if (!Q_stricmpt(name, "parentvelocity")) {
        return &ScriptVars.parentVelocity;
    } else if (!Q_stricmpt(name, "parentAngles")) {
        return &ScriptVars.parentAngles;
    } else if (!Q_stricmpt(name, "parentDir")) {
        return &ScriptVars.parentDir;
	} else if (!Q_stricmpt(name, "parentEnd")) {
		return &ScriptVars.parentEnd;
    } else if (!Q_stricmpt(name, "end")) {
		return &ScriptVars.end;
	}

    Com_Printf("^3unknown script vector '%s'\n", name);

    return &ScriptVars.tmpVector;  // just avoiding problems and for testing
}

static char *CG_SkipOpeningBrace (char *script)
{
    char *p;

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
static char *CG_SkipBlock (char *script)
{
    int block;
    char *p;

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
static int CG_CopyBlock (char *src, char *dest)
{
    char *end;

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

static int CG_NumIfBlocks (char *script, char **endOfBlocks)
{
    char *p;
    char *tp;
    int n;
    char token[MAX_QPATH];
    qboolean newLine;

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
            tp = CG_GetToken(tp, token, qfalse, &newLine);
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
        *endOfBlocks = p;
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
static void CG_FX_Wobble (vec3_t dir, vec3_t newDir, vec3_t origin, float degrees)
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

void CG_GetStoredScriptVarsFromLE (localEntity_t *le)
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
		le = CG_AllocLocalEntity();
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
		//Com_Printf("yes\n");
    } else {
		//Com_Printf("no\n");
		//le->fxType = LEFX_NONE;
		memset(&ent, 0, sizeof(ent));
		re = &ent;
    }

    re->shaderTime = cg.ftime / 1000.0f;

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
		trap_R_AddRefEntityToScene(re);
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
		le = CG_AllocLocalEntity();
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

    } else {
        //le->emitterScript[0] = '\0';
		//le->fxType = LEFX_NONE;  // so that the script isn't run
		memset(&ent, 0, sizeof(ent));
		re = &ent;
    }

    re->shaderTime = 0;  //cg.time / 1000.0f;

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
		trap_R_AddRefEntityToScene(re);
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
		le = CG_AllocLocalEntity();
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
		le = CG_AllocLocalEntity();
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
    } else {
        //le->emitterScript[0] = '\0';
		memset(&ent, 0, sizeof(ent));
		re = &ent;
		//Com_Printf("using local refEnt: '%s'\n", ScriptVars.model);
    }

	re->shaderTime = cg.ftime / 1000.0f;

	re->reType = RT_MODEL;
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

	// add to refresh list, possibly with quad glow
	//CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE );
    //FIXME powerups option
    if (parsingEmitterScriptLevel == 0) {
		trap_R_AddRefEntityToScene(re);
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

	le = CG_AllocLocalEntity();

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
    } else {
        //le->emitterScript[0] = '\0';
		le->fxType = LEFX_NONE;
    }

	re = &le->refEntity;

	VectorCopy(ScriptVars.origin, re->origin);
	VectorCopy(ScriptVars.end, re->oldorigin);
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

	re->shaderTime = cg.ftime / 1000.0f;
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

static void CG_FX_Sound (void)
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
		le = CG_AllocLocalEntity();
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
			trap_S_StartSound(ScriptVars.origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
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
		le = CG_AllocLocalEntity();
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
			//trap_S_AddLoopingSound(ScriptVars.entNumber, ScriptVars.origin, ScriptVars.velocity, lsound);
			//FIXME 1021 hack
			trap_S_AddLoopingSound(1021 /**/, ScriptVars.origin, ScriptVars.velocity, lsound);
		}
	}
}

static char *CG_ParseRenderTokens (char *script)
{
    char token[MAX_QPATH];
    qboolean newLine;
	char *s;

    ScriptVars.firstPerson = qfalse;
    ScriptVars.thirdPerson = qfalse;
    ScriptVars.shadow = qfalse;
    ScriptVars.cullNear = qfalse;  // overlapping sprites, draw the closer one
    ScriptVars.cullRadius = qfalse;
    ScriptVars.depthHack = qfalse;
    ScriptVars.stencil = qfalse;
    newLine = qfalse;

    while (!newLine) {
        s = CG_GetToken(script, token, qfalse, &newLine);
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



qboolean CG_RunQ3mmeScript (char *script, char *emitterEnd)
{
    int braceCount;
    char token[MAX_QPATH];
    int err;
    qboolean newLine;
    float f;
    vec3_t *vsrc1, *vsrc2, *vdest;
    vec3_t tvec;
    char *p;
    int i;
	qboolean copiedEmitterScriptVars;
	//int startTime;
#if 0  //def CGAMESO
	struct timeval start, end;
	int64_t startf, endf;

#endif

#if 0
	if (SC_Cvar_Get_Int("cl_freezeDemo")) {
		Com_Printf("script while paused\n");
		return qtrue;
	}
#endif

#if 0  //def CGAMESO
	gettimeofday(&start, NULL);
	startf = start.tv_sec * 1000000.0 + start.tv_usec;
#endif

#if 0  //FIXME remove switch test
	//FIXME switch test
	if (EmitterScript) {
	i = 666;
	switch (i) {
	case 0: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 1: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 2: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 3: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 4: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 5: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 6: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 7: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 8: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 9: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 10: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 11: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 12: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 13: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 14: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 15: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 16: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 17: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 18: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 19: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 20: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 21: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 22: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 23: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 24: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 25: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 26: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 27: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 28: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 29: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	case 30: {
		Com_Printf("%d\n", i - 1);
		break;
	}
	default: {
#if 0  //def CGAMESO
		//gettimeofday(&end, NULL);
		//endf = end.tv_sec * 1000000.0 + end.tv_usec;
		//Com_Printf("%lld\n", endf - startf);

#endif
	}
	}
	return qfalse;
	}
#endif

	//startTime = trap_Milliseconds();
   
    // moveGravity moveBounce moveImpact sink impact death

    //FIXME alot more from strings

    braceCount = 1;
	copiedEmitterScriptVars = qfalse;

    //Com_Printf("script: %s\n", script);

    while (1) {
        script = CG_GetToken(script, token, qfalse, &newLine);
		if (script == NULL) {
			Com_Printf("CG_RunQ3mmeScript() script == NULL\n");
			return qtrue;
		}
        if (script[0] == '\0') {  //(token[0] == '\0') {
			if (EmitterScript  &&  !copiedEmitterScriptVars) {
				//Com_Printf("'%s'  copying emitter script vars\n", token);
				memcpy(&EmitterScriptVars, &ScriptVars, sizeof(ScriptVars_t));
				copiedEmitterScriptVars = qtrue;
			}
            break;
			//Com_Printf("breaking\n");
        }

		if (SC_Cvar_Get_Int("debug_fxscript")) {
			if (!EmitterScript) {
				Com_Printf("script token: '%s'\n", token);
			}
		}

		if (EmitterScript  &&  !copiedEmitterScriptVars) {
			if (emitterEnd  &&  script >= emitterEnd) {
				//Com_Printf("'%s'  copying emitter script vars\n", token);
				//Com_Printf("emitter shader: '%s'\n", ScriptVars.shader);
				memcpy(&EmitterScriptVars, &ScriptVars, sizeof(ScriptVars_t));
				copiedEmitterScriptVars = qtrue;
			}
		}

        if (token[0] == '\0') {
            continue;
        }

        if (token[0] == '{') {
            braceCount++;
        } else if (token[0] == '}') {
            braceCount--;
        }

        if (braceCount == 0) {
			if (EmitterScript  &&  !copiedEmitterScriptVars) {
				//Com_Printf("'%s'  copying emitter script vars\n", token);
				memcpy(&EmitterScriptVars, &ScriptVars, sizeof(ScriptVars_t));
				copiedEmitterScriptVars = qtrue;
			}

#if 0  //def CGAMESO
			gettimeofday(&end, NULL);
			endf = end.tv_sec * 1000000.0 + end.tv_usec;
			Com_Printf("%lld  ->  %lld\n", endf - startf, QstrcmpCount);
			QstrcmpCount = 0;
#endif

            return qfalse;
        }

#if 0  //def CGAMESO
	gettimeofday(&start, NULL);
	startf = start.tv_sec * 1000000.0 + start.tv_usec;
#endif

	if (USE_SWITCH) {
	    switch(token[0]) {
		case 'a':  /**/
			switch(token[1]) {  // a
			case 'd':
				switch (token[2]) {  // ad
				case 'd':
					switch(token[3]) {  // add
					case 's':
						switch(token[4]) {  // adds
						case 'c':
							switch(token[5]) {  // addsc
							case 'a':
								switch(token[6]) {  // addsca
								case 'l':
									switch(token[7]) {  // addscal
									case 'e':
										switch(token[8]) { // addscale
										case '\0':
											goto addscaletoken;
										}
									}
								}
							}
						}
					}
				}
				break;
		    case 'l':
				switch(token[2]) {  // al
				case 'p':
					switch(token[3]) {  // alp
					case 'h':
						switch(token[4]) {  // alph
						case 'a':
							switch(token[5]) {  // alpha
							case '\0':
								goto alphatoken;
							}
						}
					}
				}
			}
			break;
		case 'b':  /**/
			switch(token[1]) {  // b
			case 'l':
				switch(token[2]) {  // bl
				case 'u':
					switch(token[3]) {  // blu
					case 'e':
						switch(token[4]) {  // blue
						case '\0':
							goto bluetoken;
						}
					}
				}
			}
			break;
		case 'c':  /**/
			//Com_Printf("c\n");
			switch(token[1]) {  // c
			case 'o':
				switch(token[2]) {  // co
				case 'l':
					switch(token[3]) {  // col
					case 'o':
						switch(token[4]) {  // colo
						case 'r':
							switch(token[5]) {  // color
							case '\0':
								goto colortoken;
							case 'f':
								switch(token[6]) {  // colorf
								case 'a':
									switch(token[7]) {  // colorfa
									case 'd':
										switch(token[8]) {  // colorfad
										case 'e':
											switch(token[9]) {  // colorfade
											case '\0':
												goto colorfadetoken;
											}
										}
									}
								}
								break;
								//}
							}
							break;
						}
						break;
					}
					break;
				}
				break;
			}
			break;
	    case 'd':  /**/
			break;
		case 'e': /**/
			break;
		case 'f': /**/
			break;
		case 'g': /**/
			switch(token[1]) {  // g
			case 'r':
				switch(token[2]) {  // gr
				case 'e':
					switch(token[3]) {  // gre
					case 'e':
						switch(token[4]) {  // gree
						case 'n':
							switch(token[5]) {  // green
							case '\0':
								goto greentoken;
							}
						}
					}
				}
			}
			break;
		case 'h':  /**/
			break;
		case 'i':  /**/
			break;
		case 'j':  /**/
			break;
		case 'k':  /**/
			break;
		case 'l':  /**/
			break;
		case 'm':  /**/
			break;
		case 'n':  /**/
			break;
		case 'o':  /**/
			break;
		case 'p':  /**/
			break;
		case 'q':  /**/
			break;
		case 'r':  /**/
			switch(token[1]) {  // r
			case 'e':
				switch(token[2]) {  // re
				case 'd':
					switch(token[3]) {  // red
					case '\0':
						goto redtoken;
					}
				}
			}
			break;
		case 's':  /**/
			switch(token[1]) {  // s
			case 'h':
				switch(token[2]) {  // sh
				case 'a':
					switch(token[3]) {  // sha
					case 'd':
						switch(token[4]) {  // shad
						case 'e':
							switch(token[5]) {  // shade
							case 'r':
								switch(token[6]) {  // shader
								case '\0':
									goto shadertoken;
								}
							}
						}
					}
				}
			case 'p':
				switch(token[2]) {  // sp
				case 'r':
					switch(token[3]) {  // spr
					case 'i':
						switch(token[4]) {  // spri
						case 't':
							switch(token[5]) {  // sprit
							case 'e':
								switch(token[6]) {  // sprite
								case '\0':
									goto spritetoken;
								}
							}
						}
					}
				}
			}
			break;
		case 't':  /**/
			break;
		case 'u':  /**/
			break;
		case 'v':  /**/
			break;
		case 'w':  /**/
			break;
		case 'x':  /**/
			break;
		case 'y':  /**/
			break;
		case 'z':  /**/
			break;
		}
	} // USE_SWITCH


		if (0) {

		} else if (!USE_SWITCH  &&  !Q_stricmpt(token, "red")) {
		redtoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[0], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;

		} else if (!USE_SWITCH  &&  !Q_stricmpt(token, "green")) {
		greentoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[1], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;

		} else if (!USE_SWITCH  &&  !Q_stricmpt(token, "blue")) {
		bluetoken:
			err = 0;
			script = CG_Q3mmeMath(script, &ScriptVars.color[2], &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			goto handledToken;

		} else if (!USE_SWITCH  &&  !Q_stricmpt(token, "color")) {
		colortoken:
			script = CG_GetToken(script, token, qfalse, &newLine);
			ScriptVars.color[0] = atof(token);
			script = CG_GetToken(script, token, qfalse, &newLine);
			ScriptVars.color[1] = atof(token);
			script = CG_GetToken(script, token, qfalse, &newLine);
			ScriptVars.color[2] = atof(token);
			goto handledToken;

        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "alpha")) {
		alphatoken:
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.color[3], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			goto handledToken;
        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "shader")) {
		shadertoken:
			//Com_Printf("about to get shader\n");
            script = CG_GetToken(script, ScriptVars.shader, qtrue, &newLine);
			goto handledToken;

        } else if (!Q_stricmpt(token, "model")) {
            script = CG_GetToken(script, ScriptVars.model, qtrue, &newLine);
        } else if (!Q_stricmpt(token, "size")) {
            //Com_Printf("script size\n");
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.size, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "width")) {
            //Com_Printf("script width\n");
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.width, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "angle")) {
            //Com_Printf("script angle\n");
            err = 0;
            //script = CG_Q3mmeMath(script, &ScriptVars.angle, &err);
            script = CG_Q3mmeMath(script, &ScriptVars.rotate, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t0")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t0, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t1")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t1, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t2")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t2, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t3")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t3, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t4")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t4, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t5")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t5, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t6")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t6, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t7")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t7, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t8")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t8, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "t9")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.t9, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "velocity0")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "velocity1")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "velocity2")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.velocity[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "origin0")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "origin1")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "origin2")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.origin[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "dir0")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "dir1")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "dir2")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.dir[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "angles0")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "angles1")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "angles2")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.angles[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v00")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v01")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v02")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v0[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v10")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v11")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v12")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v1[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v20")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v21")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v22")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v2[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v30")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v31")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v32")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v3[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v40")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v41")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v42")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v4[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v50")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v51")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v52")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v5[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v60")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v61")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v62")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v6[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v70")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v71")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v72")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v7[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v80")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v81")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v82")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v8[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v90")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[0], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v91")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[1], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "v92")) {
            err = 0;
            script = CG_Q3mmeMath(script, &ScriptVars.v9[2], &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "scale")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
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
        } else if (!Q_stricmpt(token, "add")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorAdd(*vsrc1, *vsrc2, *vdest);
			goto handledToken;

        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "addscale")) {
		addscaletoken:
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
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
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorSubtract(*vsrc1, *vsrc2, *vdest);
        } else if (!Q_stricmpt(token, "subscale")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
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
        } else if (!Q_stricmpt(token, "copy")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorCopy(*vsrc1, *vdest);
        } else if (!Q_stricmpt(token, "clear")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorClear(*vdest);
        } else if (!Q_stricmpt(token, "wobble")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            CG_FX_Wobble(*vsrc1, *vdest, ScriptVars.origin, f);
        } else if (!Q_stricmpt(token, "random")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
#if 0
            (*vdest)[0] = 1000.0 * crandom();
            (*vdest)[1] = 1000.0 * crandom();
            (*vdest)[2] = 1000.0 * crandom();
#endif
            (*vdest)[0] = crandom();
            (*vdest)[1] = crandom();
            (*vdest)[2] = crandom();
            VectorNormalize(*vdest);
        } else if (!Q_stricmpt(token, "normalize")) {
            // note: can be 1 or 2 args
            //
            // normalize <src> <dst> Normalize a vector so make it length 1
            //
            // but in base_weapons.fx:
            //
            // t0 dir
            // normalize dir
            //Com_Printf("normalize\n");
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            if (token[0] == '\0') {
                //Com_Printf("only 1 arg for normalize\n");
                vdest = vsrc1;
            } else {
                vdest = CG_GetScriptVector(token);
            }
            VectorCopy(*vsrc1, *vdest);
            VectorNormalize(*vdest);
        } else if (!Q_stricmpt(token, "perpendicular")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            PerpendicularVector(*vdest, *vsrc1);
        } else if (!Q_stricmpt(token, "cross")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            CrossProduct(*vsrc1, *vsrc2, *vdest);
			goto handledToken;

        } else if (!USE_SWITCH  &&  !Q_stricmpt(token, "sprite")) {
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
            // spark <options> -> view aligned spark -> origin, velocity, shader, color, size, width
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Spark();
        } else if (!Q_stricmpt(token, "quad")) {
            // quad -> fixed aligned sprite -> origin, dir, shader, color, size, angle
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Sprite(qfalse, qfalse);
        } else if (!Q_stricmpt(token, "beam")) {
            // beam -> image stretched between 2 points, origin, shader
			ScriptVars.emitterScriptEnd = script;
            script = CG_ParseRenderTokens(script);
            if (!script) {
                return qfalse;
            }
            CG_FX_Beam();
        } else if (!Q_stricmpt(token, "light")) {
			ScriptVars.emitterScriptEnd = script;
            CG_FX_Light();
        } else if (!Q_stricmpt(token, "distance")) {
            qboolean runBlock;
			vec3_t dir;
			vec3_t origOrigin;
			int count;

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
					Com_Printf("running distance %f\n", ScriptVars.distance);
					ScriptVars.distance = 0;
					EmitterScriptVars.distance = 0;  //FIXME hack
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

				if (f < cg_fxScriptMinDistance.value) {
					f = cg_fxScriptMinDistance.value;
				}

				if (parsingEmitterScriptLevel) {
					//Com_Printf("dist %d\n", parsingEmitterScriptLevel);
				}

				//FIXME linear motion -- gravity needs to be taken into account

                //if (1) {  //(!DistanceScript) {  //(ScriptVars.cent) {  //FIXME idiot
				VectorSubtract(ScriptVars.origin, ScriptVars.lastDistancePosition, dir);
				//VectorSubtract(ScriptVars.lastDistancePosition, ScriptVars.origin, dir);
				VectorNormalize(dir);
				VectorCopy(ScriptVars.origin, origOrigin);
				VectorCopy(ScriptVars.origin, newOrigin);
				count = 0;

				while (Distance(ScriptVars.lastDistancePosition, origOrigin) >= f) {
					count++;

					if (!EmitterScript) {
						//Com_Printf("running distance script:  emitter %d\n", EmitterScript);
					}
					//VectorMA(ScriptVars.lastDistancePosition, f, dir, ScriptVars.origin);
					VectorMA(ScriptVars.lastDistancePosition, f, dir, newOrigin);
					VectorCopy(newOrigin, ScriptVars.origin);

					//Com_Printf("distance script: %f\n", ScriptVars.distance);
					if (CG_RunQ3mmeScript(script, NULL)) {
						return qtrue;
					}
					VectorCopy(newOrigin, ScriptVars.lastDistancePosition);

#if 0
					if (count > cg_fxScriptMaxCount.integer) {
						Com_Printf("^1distance max count\n");
						break;
					}
#endif
					//Com_Printf("dist: %f  count %d\n", Distance(ScriptVars.lastDistancePosition, origOrigin), count);
				}
				//Com_Printf("dist count %d\n", count);
				VectorCopy(origOrigin, ScriptVars.origin);
				VectorCopy(origOrigin, ScriptVars.lastDistancePosition);
				ScriptVars.lastDistanceTime = cg.ftime;
            }
			}
            script = CG_SkipBlock(script);
            if (!script) {
                return qtrue;
            }
        } else if (!Q_stricmpt(token, "emitter")  ||  (!Q_stricmpt(token, "emitterf"))) {
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

			if (f < cg_fxScriptMinEmitter.value) {
				f = cg_fxScriptMinEmitter.value;
			}

            ScriptVars.emitterTime = f;
			if (!Q_stricmpt(token, "emitterf")) {
				ScriptVars.emitterFullLerp = qtrue;
			} else {
				ScriptVars.emitterFullLerp = qfalse;
			}
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
			le = CG_AllocLocalEntity();

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
				le = CG_AllocLocalEntity();

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
        } else if (!Q_stricmpt(token, "alphaFade")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.alphaFade = f;
            ScriptVars.hasAlphaFade = qtrue;
        } else if (!Q_stricmpt(token, "dirmodel")) {
			ScriptVars.emitterScriptEnd = script;
            //CG_FX_DirModel();
			CG_FX_Model(FX_MODEL_DIR);
        } else if (!Q_stricmpt(token, "loopsound")) {
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetToken(script, ScriptVars.loopSound, qtrue, &newLine);
			CG_FX_LoopSound();
        } else if (!Q_stricmpt(token, "rotate")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.rotate = f;
        } else if (!Q_stricmpt(token, "vibrate")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.vibrate = f;
			CG_FX_Vibrate();
        } else if (!Q_stricmpt(token, "emitterId")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.emitterId = f;
        } else if (!Q_stricmpt(token, "decal")) {
            // options:  energy alpha
			ScriptVars.emitterScriptEnd = script;
            ScriptVars.decalEnergy = qfalse;
            ScriptVars.decalAlpha = qfalse;
            while (!newLine) {
                script = CG_GetToken(script, token, qfalse, &newLine);
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
        } else if (!Q_stricmpt(token, "sound")) {
			ScriptVars.emitterScriptEnd = script;
            script = CG_GetToken(script, ScriptVars.sound, qtrue, &newLine);
			//Com_Printf("got sound: '%s'\n", ScriptVars.sound);
			CG_FX_Sound();
        } else if (!Q_stricmpt(token, "anglesmodel")) {
			ScriptVars.emitterScriptEnd = script;
            //CG_FX_AnglesModel();
			CG_FX_Model(FX_MODEL_ANGLES);
		} else if (!Q_stricmpt(token, "axismodel")) {
			ScriptVars.emitterScriptEnd = script;
			CG_FX_Model(FX_MODEL_AXIS);
        } else if (!Q_stricmpt(token, "interval")) {
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
			goto handledToken;

		} else if (!USE_SWITCH  &&  !Q_stricmpt(token, "colorfade")) {
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
            // see color2 in weapon/rail/trail
            // color2 is stored in the parent input structure, retrieve it like this
            // pushparent color2
            // pop color
            if (ScriptVars.vecStackCount >= MAX_SCRIPTED_VARS_VECTOR_STACK) {
                Com_Printf("vecStack full:  %d >= MAX_SCRIPTED_VARS_VECTOR_STACK\n", ScriptVars.vecStackCount);
                return qtrue;
            }
            script = CG_GetToken(script, token, qfalse, &newLine);
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
        } else if (!Q_stricmpt(token, "pop")) {
            // see 'pushparent'
            if (ScriptVars.vecStackCount <= 0) {
                Com_Printf("pop error, vecStackCount %d\n", ScriptVars.vecStackCount);
                return qtrue;
            }
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorCopy(ScriptVars.vecStack[ScriptVars.vecStackCount - 1], *vdest);
            ScriptVars.vecStackCount--;
        } else if (!Q_stricmpt(token, "if")) {
            int numIfBlocks;
            char *endOfAllBlocks;

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
                        script = CG_GetToken(script, token, qfalse, &newLine);
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
                            p = CG_GetToken(p, token, qfalse, &newLine);
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
        } else if (!Q_stricmpt(token, "repeat")) {
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
        } else if (!Q_stricmpt(token, "rotatearound")) {
			vec3_t tmp;

            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc1 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vsrc2 = CG_GetScriptVector(token);
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
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
        } else if (!Q_stricmpt(token, "parentorigin")) {
            Com_Printf("can't set parentOrigin\n");
            return qtrue;
        } else if (!Q_stricmpt(token, "movegravity")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.moveGravity = f;
			ScriptVars.hasMoveGravity = qtrue;
        } else if (!Q_stricmpt(token, "soundlist")) {
            qboolean hasMathToken;

			ScriptVars.emitterScriptEnd = script;
            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetToken(p, token, qfalse, &newLine);
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
                script = CG_GetToken(script, token, qtrue, &newLine);
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
			CG_FX_Sound();
        } else if (!Q_stricmpt(token, "shaderclear")) {
            // from base_weapons.fx:
            // Clear the previous shader so it uses the one in the model
			ScriptVars.shader[0] = '\0';
        } else if (!Q_stricmpt(token, "trace")) {
			vec3_t endPoint;
			trace_t trace;

			VectorMA(ScriptVars.origin, 8192 * 16, ScriptVars.dir, endPoint);
			CG_Trace(&trace, ScriptVars.origin, NULL, NULL, endPoint, -1, CONTENTS_SOLID);
			VectorCopy(trace.endpos, ScriptVars.origin);
        } else if (!Q_stricmpt(token, "decaltemp")) {
			ScriptVars.emitterScriptEnd = script;
            ScriptVars.decalEnergy = qfalse;
            ScriptVars.decalAlpha = qfalse;
            while (!newLine) {
                script = CG_GetToken(script, token, qfalse, &newLine);
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
        } else if (!Q_stricmpt(token, "modellist")) {
            qboolean hasMathToken;

            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetToken(p, token, qfalse, &newLine);
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
                script = CG_GetToken(script, token, qtrue, &newLine);
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
        } else if (!Q_stricmpt(token, "shaderlist")) {
            qboolean hasMathToken;

            // check for math token
            hasMathToken = qfalse;
            p = script;
            while (1) {
                p = CG_GetToken(p, token, qfalse, &newLine);
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
                script = CG_GetToken(script, token, qtrue, &newLine);
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
        } else if (!Q_stricmpt(token, "yaw")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[YAW] = f;
        } else if (!Q_stricmpt(token, "pitch")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[PITCH] = f;
        } else if (!Q_stricmpt(token, "roll")) {
            err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
            ScriptVars.angles[ROLL] = f;
        } else if (!Q_stricmpt(token, "movebounce")) {
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
			script = CG_GetToken(script, token, qfalse, &newLine);
			err = 0;
			CG_Q3mmeMath(token, &f, &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.moveBounce1 = f;
			script = CG_GetToken(script, token, qfalse, &newLine);
			err = 0;
			CG_Q3mmeMath(token, &f, &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.moveBounce2 = f;
			ScriptVars.hasMoveBounce = qtrue;
        } else if (!Q_stricmpt(token, "sink")) {
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
			CG_GetToken(script, token, qfalse, &newLine);
			Com_Printf("sink next token: '%s'\n", token);
#endif
			script = CG_GetToken(script, token, qfalse, &newLine);
			err = 0;
			CG_Q3mmeMath(token, &f, &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.sink1 = f;
			script = CG_GetToken(script, token, qfalse, &newLine);
			err = 0;
			CG_Q3mmeMath(token, &f, &err);
			if (err) {
				Com_Printf("^1math error\n");
				return qtrue;
			}
			ScriptVars.sink2 = f;
			ScriptVars.hasSink = qtrue;
        } else if (!Q_stricmpt(token, "impact")) {
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
        } else if (!Q_stricmpt(token, "parentvelocity")) {
            Com_Printf("can't set parentVelocity\n");
            return qtrue;
        } else if (!Q_stricmpt(token, "inverse")) {
            script = CG_GetToken(script, token, qfalse, &newLine);
            vdest = CG_GetScriptVector(token);
            VectorInverse(*vdest);
        } else if (!Q_stricmpt(token, "rings")) {
            //FIXME contenders weapon/rail/trail --  huh?
			ScriptVars.emitterScriptEnd = script;
            CG_FX_Rings();
		} else if (!Q_stricmpt(token, "echo")) {
			err = 0;
            script = CG_Q3mmeMath(script, &f, &err);
            if (err) {
                Com_Printf("^1math error\n");
                return qtrue;
            }
			Com_Printf("%f\n", f);
			//Com_Printf("%s\n", script);
		} else if (!Q_stricmpt(token, "return")) {  // for testing
			//Com_Printf("returning...\n");
			return qtrue;
		} else if (!Q_stricmpt(token, "continue")) {  // for testing
			script = CG_SkipBlock(script);
		} else if (!Q_stricmpt(token, "command")) {
			char c;
			int count;
			char newToken[MAX_QPATH];

			count = 0;
			memset(token, 0, sizeof(token));

			trap_autoWriteConfig(qfalse);
			cg.configWriteDisabled = qtrue;

			while (1) {
				c = script[0];
				if (c == '\n'  ||  c == '\f'  ||  c == '\r'  ||  c == '\0') {
					if (count < (MAX_STRING_CHARS - 2)) {
						token[count] = '\n';
						count++;
						token[count] = '\0';
						//Com_Printf("^3'%s'\n", token);
						trap_SendConsoleCommandNow(token);
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
								CG_Q3mmeMath(newToken, &f, &err);
								//Com_Printf("'%s'  %f\n", newToken, f);
								script--;
								break;
							}
							newToken[i] = c;
							i++;
						}

						Com_sprintf(&token[count], (MAX_STRING_CHARS - 1) - count, "%f", f);
						count = strlen(token);
						continue;
					}
				}
				token[count] = c;
				count++;
				script++;
			}
        } else if (token[0] != '\0'  &&  token[0] != '}') {
            Com_Printf("q3mme script unknown token: '%s'\n", token);
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

static void CG_SetQ3mmeFXScript (char *name, char *dest, int destSize, fileHandle_t handle, char *lastLine)
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

static void CG_ParseQ3mmeEffect (char *name, char *lastLine, fileHandle_t handle)
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
            line = CG_GetToken(line, token, qfalse, &newLine);
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
        line = CG_GetToken(line, token, qtrue, &newLine);
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

void CG_ReloadQ3mmeScripts (const char *fileName)
{
	int i;
	centity_t *cent;

	CG_RemoveFXLocalEntities(qtrue, 0);
	memset(&EffectScripts, 0, sizeof(EffectScripts));
	CG_ResetFXScripting();
	CG_ParseQ3mmeScripts(fileName);

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cent = &cg_entities[i];

		if (!cent->currentValid) {
			continue;
		}

		VectorCopy(cent->lerpOrigin, cent->lastFlashDistancePosition);
		VectorCopy(cent->lerpOrigin, cent->lastModelDistancePosition);
		VectorCopy(cent->lerpOrigin, cent->lastTrailDistancePosition);
		VectorCopy(cent->lerpOrigin, cent->lastImpactDistancePosition);

		VectorCopy(cent->lerpOrigin, cent->lastFlashIntervalPosition);
		VectorCopy(cent->lerpOrigin, cent->lastModelIntervalPosition);
		VectorCopy(cent->lerpOrigin, cent->lastTrailIntervalPosition);
		VectorCopy(cent->lerpOrigin, cent->lastImpactIntervalPosition);

		cent->trailTime = cg.ftime;  //cg.snap->serverTime;

		cent->lastFlashIntervalTime = cg.ftime;
		cent->lastFlashDistanceTime = cg.ftime;

		cent->lastModelIntervalTime = cg.ftime;
		cent->lastModelDistanceTime = cg.ftime;

		cent->lastTrailIntervalTime = cg.ftime;
		cent->lastTrailDistanceTime = cg.ftime;

		cent->lastImpactIntervalTime = cg.ftime;
		cent->lastImpactDistanceTime = cg.ftime;

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
	clientInfo_t *ci;
	int clientNum;

	//Com_Printf("CG_CopyPlayerDataToScriptData() cent %d  es.num %d cnum %d\n", cent - cg_entities, cent->currentState.number, cent->currentState.clientNum);

	clientNum = cent->currentState.clientNum;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("CG_CopyPlayerDataToScriptData() invalid clientNum %d  %p  %d\n", clientNum, cent, cent - cg_entities);
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

void CG_CopyStaticScriptDataToCent (centity_t *cent)
{
	VectorCopy(ScriptVars.lastIntervalPosition, cent->lastModelIntervalPosition);
	cent->lastModelIntervalTime = ScriptVars.lastIntervalTime;
	VectorCopy(ScriptVars.lastDistancePosition, cent->lastModelDistancePosition);
	cent->lastModelDistanceTime = ScriptVars.lastDistanceTime;
}

void CG_CopyStaticCentDataToScript (centity_t *cent)
{
	VectorCopy(cent->lastModelIntervalPosition, ScriptVars.lastIntervalPosition);
	ScriptVars.lastIntervalTime = cent->lastModelIntervalTime;
	VectorCopy(cent->lastModelDistancePosition, ScriptVars.lastDistancePosition);
	ScriptVars.lastDistanceTime = cent->lastModelDistanceTime;
}

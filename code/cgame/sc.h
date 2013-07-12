#ifndef sc_h_included
#define sc_h_included

//#include "cg_local.h"  //FIXME broken

void SC_Lowercase (char *p);
int SC_ParseColorFromStr (const char *s, int *r, int *g, int *b);
int SC_ParseEnemyColorsFromStr (const char *s, byte colors[3][4]);
int SC_ParseVec3FromStr (const char *s, vec3_t v);

int SC_RedFromCvar (const vmCvar_t *cvar);
int SC_GreenFromCvar (const vmCvar_t *cvar);
int SC_BlueFromCvar (const vmCvar_t *cvar);
int SC_AlphaFromCvar (const vmCvar_t *cvar);
//void SC_Vec4ColorFromCvar (vec4_t *v, const vmCvar_t *cvar);
void SC_ByteVec3ColorFromCvar (byte *b, const vmCvar_t *cvar);
void SC_Vec4ColorFromCvars (vec4_t color, const vmCvar_t *colorCvar, const vmCvar_t *alphaCvar);
void SC_Vec3ColorFromCvar (vec3_t color, const vmCvar_t *cvar);
void SC_ByteVecColorFromInt (byte *b, int color);

int SC_Cvar_Get_Int (const char *cvar);
float SC_Cvar_Get_Float (const char *cvar);
const char *SC_Cvar_Get_String (const char *cvar);

qboolean CG_IsEnemy (const clientInfo_t *ci);
qboolean CG_IsTeammate (const clientInfo_t *ci);
qboolean CG_IsUs (const clientInfo_t *ci);
qboolean CG_IsFirstPersonView (int clientNum);
qboolean CG_IsTeamGame (int gametype);

char *CG_FS_ReadLine (qhandle_t f, int *len);
const char *CG_GetToken (const char *inputString, char *token, qboolean isFilename, qboolean *newLine);
const char *CG_GetTokenGameType (const char *inputString, char *token, qboolean isFilename, qboolean *newLine);

void CG_LStrip (char **cp);
void CG_StripSlashComments (char *s);
void CG_CleanString (char *s);

void CG_Abort (void);

void CG_ScaleModel (refEntity_t *re, float size);

const char *CG_GetLocalTimeString (void);

qboolean CG_CheckQlVersion (int n0, int n1, int n2, int n3);

qboolean CG_EntityFrozen (const centity_t *cent);

qboolean CG_GameTimeout (void);

void CG_Rocket_Aim (const centity_t *enemy, vec3_t epos);

#endif  // sc_h_included

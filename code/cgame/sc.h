#ifndef sc_h_included
#define sc_h_included

//#include "cg_local.h"  //FIXME broken

//extern vmCvar_t sc_drawBBox;

void SC_Lowercase (char *p);
//void SC_AddBoundingBox (centity_t *cent);
int SC_ParseColorFromStr (const char *s, int *r, int *g, int *b);
int SC_ParseEnemyColorsFromStr (const char *s, byte colors[3][4]);
int SC_ParseVec3FromStr (const char *s, vec3_t v);
int SC_Cvar_Get_Int (const char *cvar);
float SC_Cvar_Get_Float (const char *cvar);
const char *SC_Cvar_Get_String (const char *cvar);
//void SC_DrawPredictedRail (void);
int SC_RedFromCvar (vmCvar_t cvar);
int SC_GreenFromCvar (vmCvar_t cvar);
int SC_BlueFromCvar (vmCvar_t cvar);
int SC_AlphaFromCvar (vmCvar_t cvar);
//void SC_Vec4ColorFromCvar (vec4_t *v, vmCvar_t cvar);
void SC_ByteVec3ColorFromCvar (byte *b, vmCvar_t cvar);
void SC_Vec3ColorFromCvar (vec3_t color, vmCvar_t cvar);
void SC_Vec4ColorFromCvars (vec4_t color, vmCvar_t colorCvar, vmCvar_t alphaCvar);
void SC_ByteVecColorFromInt (byte *b, int color);
char *CG_FS_ReadLine (qhandle_t f, int *len);
void CG_LStrip (char **cp);
char *CG_GetToken (char *inputString, char *token, qboolean isFilename, qboolean *newLine);
char *CG_GetTokenGameType (char *inputString, char *token, qboolean isFilename, qboolean *newLine);
void CG_CleanString (char *s);
void CG_ScaleModel (refEntity_t *re, float size);
qboolean CG_IsFirstPersonView (int clientNum);
const char *CG_GetLocalTimeString (void);

#endif  // sc_h_included

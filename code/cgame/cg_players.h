#ifndef cg_players_h_included
#define cg_players_h_included

#include "../qcommon/q_shared.h"

extern char    			*cg_customSoundNames[MAX_CUSTOM_SOUNDS];
//extern vec3_t g_color_table_ql_0_1_0_303[];


sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );
qboolean CG_FileExists (const char *filename);
qhandle_t CG_RegisterSkinVertexLight (const char *name);
qboolean CG_FindClientHeadFile (char *filename, int length, const clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext, qboolean dontForceTeamSkin);
qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName, qboolean dontForceTeamSkin );

void CG_Q3ColorFromString (const char *v, vec3_t color);
void CG_CpmaColorFromString (const char *v, vec3_t color);
void CG_OspColorFromIntString (const int *v, vec3_t color);
void CG_OspColorFromString (const char *v, vec3_t color);

void CG_LoadClientInfo (clientInfo_t *ci, int clientNum, qboolean dontForceTeamSkin);
void CG_CopyClientInfoModel( const clientInfo_t *from, clientInfo_t *to );
void CG_NewClientInfo( int clientNum );
void CG_LoadDeferredPlayers( void );

void CG_RunLerpFrame (const clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale, int time);
void CG_ClearLerpFrame (const clientInfo_t *ci, lerpFrame_t *lf, int animationNumber , int time);

void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						 int *torsoOld, int *torso, float *torsoBackLerp );
void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] );

void CG_FloatSprite (qhandle_t shader, const vec3_t origin, int renderfx, const byte *color, int radius);
void CG_FloatNumber (int n, const vec3_t origin, int renderfx, const byte *color, float scale);
void CG_FloatNumberExt (int n, const vec3_t origin, int renderfx, const byte *color, int time);
void CG_FloatEntNumbers (void);

void CG_AddRefEntityWithPowerups (refEntity_t *ent, const entityState_t *state, int team);

void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
qboolean CG_DuelPlayerScoreValid (int clientNum);
qboolean CG_DuelPlayerInfoValid (int clientNum);

#endif  // cg_players_h_included

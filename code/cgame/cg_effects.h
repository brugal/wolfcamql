#ifndef cg_effects_h_included
#define cg_effects_h_included

#include "../qcommon/q_shared.h"

#include "cg_localents.h"

void CG_BubbleTrail (const vec3_t start, const vec3_t end, float spacing );

localEntity_t *CG_SmokePuff( const vec3_t p,
				   const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );

void CG_SpawnEffect (const vec3_t org);

#if 1  //def MPACK
void CG_LightningBoltBeam( const vec3_t start, const vec3_t end );
void CG_KamikazeEffect( const vec3_t org );
void CG_ObeliskExplode( const vec3_t org, int entityNum );
void CG_ObeliskPain( const vec3_t org );
void CG_InvulnerabilityImpact( const vec3_t org, const vec3_t angles );
void CG_InvulnerabilityJuiced( const vec3_t org );
#endif

void CG_ScorePlum( int client, const vec3_t org, int score );
void CG_HeadShotPlum (const vec3_t org);

localEntity_t *CG_MakeExplosion( const vec3_t origin, const vec3_t dir,
								qhandle_t hModel, qhandle_t shader, int msec,
								qboolean isSprite );

void CG_Bleed( const vec3_t origin, int entityNum );
void CG_ImpactSparks( int weapon, const vec3_t origin, const vec3_t dir, int entityNum );

void CG_FX_GibPlayer (centity_t *cent);
void CG_FX_ThawPlayer (centity_t *cent);

void CG_GibPlayer (const centity_t *cent);
void CG_ThawPlayer (const centity_t *cent);

#endif  //  cg_effects_h_included

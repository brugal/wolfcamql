#ifndef cg_ents_h_included
#define cg_ents_h_included

#include "../qcommon/q_shared.h"

void CG_PrintEntityState (int n);
void CG_PrintEntityStatep (const entityState_t *ent);

qboolean CG_AllowedAmbientSound (qhandle_t h);

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName );
void CG_SetEntitySoundPosition( const centity_t *cent );
void CG_Beam( const centity_t *cent );
void CG_AdjustPositionForMover (const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, float subTime, const vec3_t angles_in, vec3_t angles_out);
void CG_AddPacketEntities( void );

#endif // cg_ents_h_included

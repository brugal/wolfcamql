#ifndef cg_event_h_included
#define cg_event_h_included

const char *CG_PlaceString( int rank );
void CG_TimedItemPickup (int index, const vec3_t origin, int clientNum, int time, qboolean spec);
void CG_PainEvent( centity_t *cent, int health );
int CG_CheckClientEventCpma (int clientNum, const entityState_t *es);
void CG_EntityEvent( centity_t *cent, const vec3_t position );
void CG_CheckEvents( centity_t *cent );

#endif  // cg_event_h_included

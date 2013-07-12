#ifndef cg_weapons_h_included
#define cg_weapons_h_included

int CG_ModToWeapon (int mod);
void CG_SimpleRailTrail (const vec3_t start, const vec3_t end, float railTime, const byte color[4]);
void CG_RailTrail( const clientInfo_t *ci, const vec3_t start, const vec3_t end );
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );
void CG_LightningBolt( centity_t *cent, vec3_t origin );
void CG_AddPlayerWeapon( const refEntity_t *parent, const playerState_t *ps, centity_t *cent, int team );
void CG_AddViewWeapon (const playerState_t *ps);
void CG_DrawWeaponSelect( void );

void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );
void CG_OutOfAmmoChange( void );	// should this be in pmove?
void CG_FireWeapon( centity_t *cent );

void CG_MissileHitWall( int weapon, int clientNum, const vec3_t origin, const vec3_t dir, impactSound_t soundType, qboolean knowClientNum );
void CG_MissileHitPlayer( int weapon, const vec3_t origin, const vec3_t dir, int entityNum, int shooterClientNum );

void CG_ShotgunFire( const entityState_t *es );

void CG_GetWeaponFlashOrigin (int clientNum, vec3_t startPoint);
void CG_Bullet( const vec3_t origin, int sourceEntityNum, const vec3_t normal, qboolean flesh, int fleshEntityNum );


#endif  // cg_weapons_h_included

// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

#include "cg_effects.h"
#include "cg_freeze.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_sound.h"
#include "cg_syscalls.h"  // trap_R_RegisterShader
#include "sc.h"

#include "wolfcam_local.h"

/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail (const vec3_t start, const vec3_t end, float spacing ) {
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA( move, i, vec, move );

	VectorScale (vec, spacing, vec);

	for ( ; i < len; i += spacing ) {
		localEntity_t	*le;
		refEntity_t		*re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + 1000.0 + random() * 250.0;
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy( move, le->pos.trBase );
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		VectorAdd (move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *CG_SmokePuff( const vec3_t p, const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0 / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	}
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	} else {
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

static void CG_DeathEffect( const vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SPRITE_EXPLOSION;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->rotation = 0;
	re->radius = 100;

	re->shaderTime = cg.time / 1000.0f;

	re->customShader = cgs.media.deathEffectShader;

	AxisClear( re->axis );

	VectorCopy( org, re->origin );
#if 1  //def MPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif

	//VectorCopy(re->origin, le->pos.trBase);
}

/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect (const vec3_t org)
{
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

#if  0  //ndef MPACK
	re->customShader = cgs.media.teleportEffectShader;
#endif
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear( re->axis );

	VectorCopy( org, re->origin );
#if 1  //def MPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif
}


#if 1  //def MPACK
/*
===============
CG_LightningBoltBeam
===============
*/
void CG_LightningBoltBeam( const vec3_t start, const vec3_t end ) {
	localEntity_t	*le;
	refEntity_t		*beam;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SHOWREFENTITY;
	le->startTime = cg.time;
	le->endTime = cg.time + 50;

	beam = &le->refEntity;

	VectorCopy( start, beam->origin );
	// this is the end point
	VectorCopy( end, beam->oldorigin );

	beam->reType = RT_LIGHTNING;
	beam->customShader = cgs.media.lightningShader;
	beam->radius = 8;
}

/*
==================
CG_KamikazeEffect
==================
*/
void CG_KamikazeEffect( const vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_KAMIKAZE;
	le->startTime = cg.time;
	le->endTime = cg.time + 3000;//2250;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	VectorClear(le->angles.trBase);

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.kamikazeEffectModel;

	VectorCopy( org, re->origin );

}

/*
==================
CG_ObeliskExplode
==================
*/
void CG_ObeliskExplode( const vec3_t org, int entityNum ) {
	localEntity_t	*le;
	vec3_t origin;

	// create an explosion
	VectorCopy( org, origin );
	origin[2] += 64;
	le = CG_MakeExplosion( origin, vec3_origin,
						   cgs.media.dishFlashModel,
						   cgs.media.rocketExplosionShader,
						   600, qtrue );
	le->light = 300;
	le->lightColor[0] = 1;
	le->lightColor[1] = 0.75;
	le->lightColor[2] = 0.0;
}

/*
==================
CG_ObeliskPain
==================
*/
void CG_ObeliskPain( const vec3_t org ) {
	float r;
	sfxHandle_t sfx;

	// hit sound
	r = rand() & 3;
	if ( r < 2 ) {
		sfx = cgs.media.obeliskHitSound1;
	} else if ( r == 2 ) {
		sfx = cgs.media.obeliskHitSound2;
	} else {
		sfx = cgs.media.obeliskHitSound3;
	}
	CG_StartSound ( org, ENTITYNUM_NONE, CHAN_BODY, sfx );
}


/*
==================
CG_InvulnerabilityImpact
==================
*/
void CG_InvulnerabilityImpact( const vec3_t org, const vec3_t angles ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int				r;
	sfxHandle_t		sfx;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_INVULIMPACT;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityImpactModel;

	VectorCopy( org, re->origin );
	AnglesToAxis( angles, re->axis );

	r = rand() & 3;
	if ( r < 2 ) {
		sfx = cgs.media.invulnerabilityImpactSound1;
	} else if ( r == 2 ) {
		sfx = cgs.media.invulnerabilityImpactSound2;
	} else {
		sfx = cgs.media.invulnerabilityImpactSound3;
	}
	CG_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, sfx );
}

/*
==================
CG_InvulnerabilityJuiced
==================
*/
void CG_InvulnerabilityJuiced( const vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_INVULJUICED;
	le->startTime = cg.time;
	le->endTime = cg.time + 10000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityJuicedModel;

	VectorCopy( org, re->origin );
	VectorClear(angles);
	AnglesToAxis( angles, re->axis );

	CG_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, cgs.media.invulnerabilityJuicedSound );
}

#endif  // if 1  MPACK

/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum( int client, const vec3_t org, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlums.integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );


	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	VectorCopy( org, le->pos.trBase );
	if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20) {
		le->pos.trBase[2] -= 20;
	}

	//CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, lastPos);


	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}

void CG_DamagePlum (int client, const vec3_t org, int score, int weapon)
{
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;

	le = CG_AllocLocalEntity();
	le->leFlags = client;
	le->leType = LE_DAMAGEPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_damagePlumTime.integer;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);


	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	// don't use floats to store int
	//le->radius = score;
	//le->bounceFactor = weapon;
	le->angles.trTime = score;
	le->angles.trDuration = weapon;


	VectorCopy(org, le->pos.trBase);

	le->pos.trDelta[0] += crandom() * cg_damagePlumRandomVelocity.integer;
	le->pos.trDelta[1] += crandom() * cg_damagePlumRandomVelocity.integer;
	
	//Com_Printf("^2dir: %d\n", dir);
	//VectorScale(le->pos.trDelta, -100, le->pos.trDelta);
	
	//le->pos.trDelta[2] = 300;
	//le->pos.trDelta[2] += rand() % 200;


	//le->pos.trDelta[2] = 150;  //FIXME maybe random?
	//le->pos.gravity = 400;

	le->pos.trDelta[2] = cg_damagePlumBounce.integer;  //120;
	le->pos.gravity = cg_damagePlumGravity.integer;  //250;
	
	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	//CG_Printf( "DamagePlum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	//VectorCopy(org, lastPos);


	re = &le->refEntity;

	re->reType = RT_SPRITE;
	//re->radius = 16;

	VectorClear(angles);
	AnglesToAxis(angles, re->axis);
}

/*
==================
CG_HeadShotPlum
==================
*/
void CG_HeadShotPlum (const vec3_t org)
{
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	//if (client != cg.predictedPlayerState.clientNum || cg_scorePlums.integer == 0) {
	if (0) {  //(client != cg.snap->ps.clientNum || cg_scorePlums.integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_HEADSHOTPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 2000;  //4000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	VectorCopy( org, le->pos.trBase );
	if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20) {
		le->pos.trBase[2] -= 20;
	}

	VectorCopy(org, lastPos);

	re = &le->refEntity;
	re->reType = RT_SPRITE;
	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}

/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion( const vec3_t origin, const vec3_t dir,
								qhandle_t hModel, qhandle_t shader,
								int msec, qboolean isSprite ) {
	float			ang;
	localEntity_t	*ex;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CG_MakeExplosion: msec = %i", msec );
	}

	//Com_Printf("explosion\n");

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale( dir, 16, tmpVec );
		VectorAdd( tmpVec, origin, newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, newOrigin );

		// set axis with random rotate
		if ( !dir ) {
			AxisClear( ex->refEntity.axis );
		} else {
			ang = rand() % 360;
			VectorCopy( dir, ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( newOrigin, ex->refEntity.origin );
	VectorCopy( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed( const vec3_t origin, int entityNum ) {
	localEntity_t	*ex;

	if ( !cg_blood.integer ) {
		return;
	}

	// don't show own blood in view
	if (wolfcam_following  &&  wcg.clientNum == entityNum) {
		return;
	}

	//if (!cgs.media.bloodExplosionShader) {
	//}

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + cg_bleedTime.integer;

	VectorCopy ( origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = rand() % 360;
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if ( entityNum == cg.snap->ps.clientNum ) {
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
	}
	//CG_Printf("bleed %d\n", entityNum);
}

void CG_ImpactSparks( int weapon, const vec3_t origin, const vec3_t dir, int entityNum)
{
	localEntity_t *le;
	qhandle_t shader;
	float radius;
	int ourClientNum;
	vec4_t color;

	//Com_Printf("imp %d\n", entityNum);

	//FIXME hack for 2010-08-08 new ql spawn invuln
	if (cg_entities[entityNum].currentState.powerups & PWEX_SPAWNPROTECTION) {
		//Com_Printf("invul %d  %s\n", entityNum, cgs.clientinfo[entityNum].name);
		//CG_PlayerFloatSprite(&cg_entities[entityNum], cgs.media.connectionShader);
		if (entityNum >= 0  &&  entityNum < MAX_CLIENTS) {
			cg_entities[entityNum].spawnArmorTime = cg.time;
		}
	}

	if (cg_impactSparks.integer == 0) {
		return;
	}

	if (cg_shotgunImpactSparks.integer == 0  &&  weapon == WP_SHOTGUN) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if (entityNum == ourClientNum  &&  !cg.freecam) {
		return;
	}

	if (cgs.gametype == GT_FREEZETAG  &&  !cg.freecam) {
		if (cgs.clientinfo[entityNum].team == cgs.clientinfo[ourClientNum].team) {
			return;
		}
		if (CG_FreezeTagFrozen(entityNum)) {
			return;
		}
	}

	if (!*cg_impactSparksColor.string) {
		shader = trap_R_RegisterShader("gfx/misc/tracer");
		Vector4Set(color, 1, 1, 1, 1);
	} else {
		if (cg_impactSparksHighlight.integer) {
			shader = trap_R_RegisterShader("wc/tracerHighlight");
		} else {
			shader = trap_R_RegisterShader("wc/tracer");
		}

		SC_Vec3ColorFromCvar(color, &cg_impactSparksColor);
		color[3] = 1.0;
	}
	radius = cg_impactSparksSize.value;

	le = CG_SmokePuff_ql(origin, vec3_origin, radius,
						 color[0], color[1], color[2], color[3],
						 cg_impactSparksLifetime.integer,
						 cg.time,  // start time
						 0,  // fade in time
						 0,  // flags
						 shader);
	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_PUFF_DONT_SCALE_NOT_KIDDING_MAN | LEF_PUFF_DONT_SCALE;
	le->pos.trDelta[2] = cg_impactSparksVelocity.value;
	le->refEntity.reType = RT_SPRITE;
	VectorCopy(origin, le->refEntity.origin);
	VectorCopy(origin, le->refEntity.oldorigin);
	AxisCopy(axisDefault, le->refEntity.axis);
	le->refEntity.rotation = 0;
}

static void CG_LaunchGib_ql (const vec3_t origin, const vec3_t velocity, qhandle_t hModel)
{
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_gibTime.integer;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	if (*cg_gibColor.string) {
		re->customShader = trap_R_RegisterShader("wc/tracer");
		SC_ByteVecColorFromInt(re->shaderRGBA, cg_gibColor.integer);
		re->shaderRGBA[3] = 255;
	}

	if (!cg_gibGravity.integer) {
		le->pos.trType = TR_LINEAR;
	} else {
		le->pos.trType = TR_GRAVITY;
	}
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;
	le->pos.gravity = cg_gibGravity.integer;
	le->bounceFactor = cg_gibBounceFactor.value;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

#if 0  // unused
static void CG_LaunchGib_ql_floating1 (const vec3_t origin, const vec3_t velocity, qhandle_t hModel)
{
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_gibTime.integer;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;
	re->customSkin = trap_R_RegisterShader("xgibs");
	//FIXME just do blood trail ql

	le->pos.trType = TR_LINEAR;
	VectorCopy( origin, le->pos.trBase );
	//FIXME they start and a random off set of origin
	VectorCopy( velocity, le->pos.trDelta );

	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = 0;  //LEBS_BLOOD;
	le->leMarkType = 0;  //LEMT_BLOOD;
}
#endif

static void CG_LaunchGib_ql_floating (const vec3_t origin, const vec3_t velocity, qhandle_t hModel)
{
	localEntity_t	*le;
	qhandle_t shader;
	float radius;
	vec4_t color;

	if (!*cg_gibSparksColor.string) {
		shader = trap_R_RegisterShader("gfx/misc/tracer");
		Vector4Set(color, 1, 1, 1, 1);
	} else {
		if (cg_gibSparksHighlight.integer) {
			shader = trap_R_RegisterShader("wc/tracerHighlight");
		} else {
			shader = trap_R_RegisterShader("wc/tracer");
		}
		SC_Vec3ColorFromCvar(color, &cg_gibSparksColor);
		color[3] = 1.0;
	}

	radius = cg_gibSparksSize.value;

	le = CG_SmokePuff_ql(origin, vec3_origin, radius,
						 color[0], color[1], color[2], color[3],
						 cg_gibTime.integer,
						 cg.time,  // start time
						 0,  // fade in time
						 0,  // flags
						 shader);
	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_PUFF_DONT_SCALE_NOT_KIDDING_MAN | LEF_PUFF_DONT_SCALE;
	VectorCopy(velocity, le->pos.trDelta);

	le->refEntity.reType = RT_SPRITE;
	VectorCopy(origin, le->refEntity.origin);
	VectorCopy(origin, le->refEntity.oldorigin);
	AxisCopy(axisDefault, le->refEntity.axis);
	le->refEntity.rotation = 0;

	return;
}

/*
==================
CG_LaunchGib
==================
*/
static void CG_LaunchGib( const vec3_t origin, const vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	//FIXME testing
	//re->reType = RT_MODEL;
	//re->customSkin = trap_R_RegisterShader("models/gibs/qgibs.tga");
	//re->customSkin = trap_R_RegisterShader("models/gibs/abdomen02.png");
	//re->customSkin = trap_R_RegisterShader("xgibs");
	//re->customSkin = trap_R_RegisterShader("adboxblack");

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;

#if 0
	re->shaderRGBA[0] = 0;
	re->shaderRGBA[1] = 255;
	re->shaderRGBA[2] = 0;
	re->shaderRGBA[3] = 255;
	//trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b )
	//trap_R_AddLightToScene(origin, 1000.0, 0.0, 1.0, 0.0);
#endif
}

static void CG_LaunchIce (const vec3_t origin, const vec3_t velocity, qhandle_t hModel)
{
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	//FIXME testing
	//re->reType = RT_MODEL;
	//re->customSkin = trap_R_RegisterShader("models/gibs/qgibs.tga");
	//re->customSkin = trap_R_RegisterShader("models/gibs/abdomen02.png");
	//re->customSkin = trap_R_RegisterShader("xgibs");
	//re->customSkin = trap_R_RegisterShader("adboxblack");

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_ICE;
	le->leMarkType = LEMT_ICE;

#if 0
	re->shaderRGBA[0] = 0;
	re->shaderRGBA[1] = 255;
	re->shaderRGBA[2] = 0;
	re->shaderRGBA[3] = 255;
	//trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b )
	//trap_R_AddLightToScene(origin, 1000.0, 0.0, 1.0, 0.0);
#endif
}

#define	GIB_VELOCITY	250
#define	GIB_JUMP		250

static void CG_GibPlayer_ql (const centity_t *cent)
{
	vec3_t	origin, velocity;
	int i;
	float gibVelocity;
	int gibJump;
	vec3_t playerOrigin;
	vec3_t newVelocity;
	vec3_t randomVelocity;
	float mix;

	//if ( !cg_blood.integer ) {
	//	return;
	//}

	// allow gibs to be turned off for speed
	if (!cg_gibs.integer) {
		return;
	}

	VectorCopy(cent->currentState.pos.trBase, playerOrigin);

	CG_DeathEffect(playerOrigin);

	gibJump = cg_gibJump.value;

	if (rand() % 1) {
		gibJump = -gibJump;
	}

	mix = cg_gibDirScale.value;
	if (mix > 1.0) {
		mix = 1.0;
	}
	if (mix < 0.0) {
		mix = 0.0;
	}

	for (i = 0;  i < cg_gibs.integer;  i++) {
		ByteToDir(cent->currentState.eventParm, velocity);
		gibVelocity = cg_gibVelocity.value;

		VectorScale(velocity, gibVelocity + crandom() * cg_gibVelocityRandomness.value, velocity);

		/////
		VectorCopy(playerOrigin, origin);
		VectorCopy(velocity, newVelocity);

		randomVelocity[0] = crandom()*gibVelocity;
		randomVelocity[1] = crandom()*gibVelocity;
		randomVelocity[2] = gibJump + crandom()*gibVelocity;

		newVelocity[0] += crandom() * cg_gibRandomness.value;
		newVelocity[1] += crandom() * cg_gibRandomness.value;
		newVelocity[2] += crandom() * cg_gibRandomnessZ.value + gibJump;

		newVelocity[0] = newVelocity[0] * mix + randomVelocity[0] * (1.0 - mix);
		newVelocity[1] = newVelocity[1] * mix + randomVelocity[1] * (1.0 - mix);
		newVelocity[2] = newVelocity[2] * mix + randomVelocity[2] * (1.0 - mix);

		origin[0] += crandom() * cg_gibOriginOffset.value;
		origin[1] += crandom() * cg_gibOriginOffset.value;
		origin[2] += crandom() * cg_gibOriginOffsetZ.value;

		CG_LaunchGib_ql(origin, newVelocity, cgs.media.gibSphere);
	}

	for (i = 0;  i < cg_gibs.integer;  i++) {
		VectorCopy(playerOrigin, origin);
		velocity[0] = 0;
		velocity[1] = 0;
		velocity[2] = cg_gibFloatingVelocity.value + crandom() * cg_gibFloatingRandomness.value;

		origin[0] += crandom() * cg_gibFloatingOriginOffset.value;
		origin[1] += crandom() * cg_gibFloatingOriginOffset.value;
		origin[2] += crandom() * cg_gibFloatingOriginOffsetZ.value;

		CG_LaunchGib_ql_floating(origin, velocity, cgs.media.gibSphere);
	}

	return;
}

void CG_FX_GibPlayer (centity_t *cent)
{
	//vec3_t velocity;

	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_ResetScriptVars();
	//VectorCopy(cent->lerpOrigin, ScriptVars.origin);
	CG_CopyPlayerDataToScriptData(cent);
	VectorCopy(cent->lerpOrigin, ScriptVars.lastIntervalPosition);
	ScriptVars.lastIntervalTime = cg.ftime;


	VectorCopy(cent->currentState.pos.trDelta, ScriptVars.parentVelocity);
	Vector4Set(ScriptVars.color, 1, 1, 1, 1);

	CG_RunQ3mmeScript(EffectScripts.gibbed, NULL);
}

void CG_FX_ThawPlayer (centity_t *cent)
{
	//vec3_t velocity;

	//FIXME maybe find client, possibly in last snapshot

	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_ResetScriptVars();
	//VectorCopy(cent->lerpOrigin, ScriptVars.origin);
	//CG_CopyPlayerDataToScriptData(cent);

	VectorCopy(cent->currentState.pos.trBase, ScriptVars.origin);
	VectorCopy(ScriptVars.origin, ScriptVars.lastIntervalPosition);
	ScriptVars.lastIntervalTime = cg.ftime;


	VectorCopy(cent->currentState.pos.trDelta, ScriptVars.parentVelocity);
	Vector4Set(ScriptVars.color, 1, 1, 1, 1);

	CG_RunQ3mmeScript(EffectScripts.thawed, NULL);
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/

void CG_GibPlayer (const centity_t *cent)
{
	vec3_t	origin, velocity;
	vec3_t playerOrigin;

	// allow gibs to be turned off for speed
	if ( !cg_gibs.integer ) {
		return;
	}

	VectorCopy(cent->currentState.pos.trBase, playerOrigin);

//	if (*EffectScripts.gibbed) {
//		CG_FX_GibPlayer(playerOrigin);
//		return;
//	}

	if (!SC_Cvar_Get_Int("cl_useq3gibs")) {
		CG_GibPlayer_ql(cent);
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	if ( rand() & 1 ) {
		CG_LaunchGib( origin, velocity, cgs.media.gibSkull );
	} else {
		CG_LaunchGib( origin, velocity, cgs.media.gibBrain );
	}


	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibAbdomen );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibArm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibChest );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFist );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFoot );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibForearm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibIntestine );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );
}

void CG_ThawPlayer (const centity_t *cent)
{
	vec3_t	origin, velocity;
	vec3_t playerOrigin;
	int i;

	// allow gibs to be turned off for speed
	if ( !cg_gibs.integer ) {
		return;
	}

	VectorCopy(cent->currentState.pos.trBase, playerOrigin);

//	if (*EffectScripts.gibbed) {
//		CG_FX_GibPlayer(playerOrigin);
//		return;
//	}


	for (i = 0;  i < 5;  i++) {
		VectorCopy( playerOrigin, origin );
		velocity[0] = crandom()*GIB_VELOCITY;
		velocity[1] = crandom()*GIB_VELOCITY;
		velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
		CG_LaunchIce( origin, velocity, cgs.media.iceAbdomen);

		VectorCopy( playerOrigin, origin );
		velocity[0] = crandom()*GIB_VELOCITY;
		velocity[1] = crandom()*GIB_VELOCITY;
		velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
		CG_LaunchIce( origin, velocity, cgs.media.iceBrain);
	}
}

#if 0  // unused

static void CG_LaunchExplode( const vec3_t origin, const vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 10000 + random() * 6000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;  //LEBS_BLOOD;  //LEBS_BRASS;
	le->leMarkType = LEMT_NONE;  //LEMT_BLOOD;  //LEMT_NONE;
}
#endif


#if 0  // unused

#define	EXP_VELOCITY	100
#define	EXP_JUMP		150

static void CG_BigExplode( const vec3_t playerOrigin ) {
	vec3_t	origin, velocity;
	qhandle_t model;

	//if ( !cg_blood.integer ) {
	//	return;
	//}

	model = cgs.media.smoke2;
	//model = cgs.media.gibSphere;

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, model );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, model );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*1.5;
	velocity[1] = crandom()*EXP_VELOCITY*1.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, model );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.0;
	velocity[1] = crandom()*EXP_VELOCITY*2.0;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, model );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.5;
	velocity[1] = crandom()*EXP_VELOCITY*2.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, model );
}

#endif

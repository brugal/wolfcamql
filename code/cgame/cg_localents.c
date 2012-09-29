// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define	MAX_LOCAL_ENTITIES	8192  //4096  //1024  //1024  //512
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

static localEntity_t tmpLocalEntity;

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
//void CG_InitLocalEntities (qboolean firstCall)
void CG_InitLocalEntities (void)
{
	int		i;

#if 0
	if (!firstCall) {
		CG_ClearLocalEntitiesTimeChange();
		return;
	}
#endif

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}

#if 0
void CG_ClearLocalEntitiesTimeChange (void)
{
	localEntity_t	*le, *next;

	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if (le->fxType  &&  le->sv.dontClear) {
			//CG_FreeLocalEntity(le);
			continue;
		}
		CG_FreeLocalEntity(le);
	}
}
#endif

/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if (le == &tmpLocalEntity) {
		return;
	}

	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	//le->emitterScript[0] = '\0';

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
//static localEntity_t testEntity;

localEntity_t	*CG_AllocLocalEntity( void ) {
	localEntity_t	*le;

	if (SC_Cvar_Get_Int("cl_freezeDemo")) {
		//Com_Printf("script while paused\n");
		memset(&tmpLocalEntity, 0, sizeof(tmpLocalEntity));
		return &tmpLocalEntity;
	}

	// testing
	//memset(&testEntity, 0, sizeof(testEntity));
	//return &testEntity;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		//CG_Printf("^1max local entities\n");
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;



	memset( le, 0, sizeof( *le ) );

	//Com_Printf("%d sizeof le %d\n", trap_Milliseconds(), sizeof(*le));

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
CG_BloodTrail

Leave expanding blood puffs behind gibs
================
*/

localEntity_t *CG_SmokePuff_ql( const vec3_t p, const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader )
{
	//static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	//re->rotation = Q_random( &seed ) * 360;
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

void CG_BloodTrail_ql( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;
	qhandle_t	shader;
	float radius;
	int i;
	//vec3_t normal;
	vec4_t color;

	//shader = cgs.media.plasmaBallShader;
	//shader = trap_R_RegisterShader("models/gibs/gibs");
	//shader = trap_R_RegisterShaderNoMip("models/gibs/gibs");
	//shader = trap_R_RegisterShader("gibspark");

	shader = trap_R_RegisterShader("xgibs");
	//shader = trap_R_RegisterShader("flareShader");
	//shader = trap_R_RegisterShader("bloodExplosion");
	//shader = trap_R_RegisterShader("textures/sfx/specular5.jpg");
	//shader = trap_R_RegisterShader("viewBloodBlend");
	//shader = trap_R_RegisterShader("gfx/damage/blood_screen.png");
	//shader = trap_R_RegisterShader("gfx/misc/flare.tga");
	//shader = trap_R_RegisterShader("gfx/misc/tracer2.jpg");
	if (!*cg_gibSparksColor.string) {
		shader = trap_R_RegisterShader("gfx/misc/tracer");
		Vector4Set(color, 1, 1, 1, 1);
	} else {
		if (cg_gibSparksHighlight.integer) {
			shader = trap_R_RegisterShader("wc/tracerHighlight");
		} else {
			shader = trap_R_RegisterShader("wc/tracer");
		}
		SC_Vec3ColorFromCvar(color, cg_gibSparksColor);
		color[3] = 1.0;
	}


	//radius = 2;  //20; //0.25;  //0.5;  //100;  //2;
	radius = cg_gibSparksSize.value;

	step = cg_gibStepTime.integer;  //150;  //100;  //25;  //500;  //150;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	//Com_Printf("t: %d  t2: %d   step: %d  cg.frametime %d\n", t, t2, step, cg.frametime);
	//Com_Printf("cg.time: %d   cg.frametime: %d\n", cg.time, cg.frametime);

	for ( ; t <= t2; t += step ) {
		for (i = 0;  i < 1;  i++) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		//Com_Printf("trail t %d\n", t);
#if 1
		blood = CG_SmokePuff_ql( newOrigin, vec3_origin,
					  radius,		// radius
					  //0, 1, 0, 0.01,	// color
								 //1, 1, 1, 1, //0.21,	// color
								 color[0], color[1], color[2], color[3],
								 //2000,		// trailTime
					 cg_gibTime.integer / 3,
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  shader );
		// use the optimized version
		//blood->leType = LE_FRAGMENT;  //LE_FALL_SCALE_FADE;
		blood->leType = LE_MOVE_SCALE_FADE;  //LE_FALL_SCALE_FADE;
		blood->leFlags = LEF_PUFF_DONT_SCALE_NOT_KIDDING_MAN | LEF_PUFF_DONT_SCALE;  //LEF_PUFF_DONT_SCALE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
		//blood->refEntity.reType = RT_MODEL;
		blood->refEntity.reType = RT_SPRITE;
		//blood->refEntity.reType = RT_RAIL_CORE;
		//blood->refEntity.hModel = cgs.media.gibSphere;

		VectorCopy( newOrigin, blood->refEntity.origin );
		VectorCopy( newOrigin, blood->refEntity.oldorigin );
		AxisCopy( axisDefault, blood->refEntity.axis );
		//trap_R_AddLightToScene(newOrigin, 1000.0, 0.0, 1.0, 0.0);
		blood->refEntity.rotation = 0;
#if 0
		blood->refEntity.radius = radius;
		blood->refEntity.shaderRGBA[0] = 0;
		blood->refEntity.shaderRGBA[1] = 255;
		blood->refEntity.shaderRGBA[2] = 0;
		blood->refEntity.shaderRGBA[3] = 255;
#endif
#endif
		//AxisCopy( axisDefault, axis );
		//FIXME
		//VectorSet(normal, 0, 0, 1);
		//CG_ImpactMark(shader, newOrigin, normal, random()*360, 1, 1, 1, 1, qfalse, radius, qfalse, qfalse, qfalse);
		}
	}
}

void CG_BloodTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;
	qhandle_t	shader;
	int radius;

	if (!SC_Cvar_Get_Int("cl_useq3gibs")) {
		CG_BloodTrail_ql(le);
		return;
	}

	//shader = cgs.media.bloodTrailShader;
	shader = cgs.media.q3bloodTrailShader;

	radius = 20;

	step = 150;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		blood = CG_SmokePuff( newOrigin, vec3_origin,
					  radius,		// radius
							  1, 1, 1, 1,	// color
							  //(float)0x70 / (float)0xff, 0, 0, (float)0xc8 / (float)0xff,
					  2000,		// trailTime
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  shader );
		// use the optimized version
		blood->leType = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
	}
}

void CG_IceTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t *ice;
	qhandle_t	shader;
	int radius;

	shader = cgs.media.iceTrailShader;

	radius = 20;

	step = 150;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		ice = CG_SmokePuff( newOrigin, vec3_origin,
					  radius,		// radius
							  1, 1, 1, 1,	// color
							  //(float)0x70 / (float)0xff, 0, 0, (float)0xc8 / (float)0xff,
					  2000,		// trailTime
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  shader );
		// use the optimized version
		ice->leType = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		ice->pos.trDelta[2] = 40;
	}
}


/*
================
CG_FragmentBounceMark
================
*/
void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
	int			radius;
	qhandle_t	shader;

	if ((le->leMarkType == LEMT_BLOOD  &&  cg_blood.integer)  ||  le->leMarkType == LEMT_ICE) {

		radius = 16 + (rand()&31);
		if (le->leMarkType == LEMT_BLOOD  &&  SC_Cvar_Get_Int("cl_useq3gibs")) {
			shader = cgs.media.q3bloodMarkShader;
		} else if (le->leMarkType == LEMT_ICE) {
			shader = cgs.media.iceMarkShader;
			return;
		} else {
			//shader = cgs.media.bloodMarkShader;
			//FIXME  ql gibs do they always leave marks?
			if (cgs.gametype != GT_FREEZETAG) {
				//return;
			}
			shader = cgs.media.burnMarkShader;
			radius = 5;
		}

		CG_ImpactMark( shader, trace->endpos, trace->plane.normal, random()*360,
					   1,1,1,1, qtrue, radius, qfalse, qfalse, qfalse );
	} else if ( le->leMarkType == LEMT_BURN ) {

		radius = 8 + (rand()&15);
		CG_ImpactMark( cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360,
					   1,1,1,1, qtrue, radius, qfalse, qfalse, qfalse );
	}


	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}

/*
================
CG_FragmentBounceSound
================
*/
void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace ) {
	if ( le->leBounceSoundType == LEBS_BLOOD ) {
		// half the gibs will make splat sounds
		if ( rand() & 1 ) {
			int r = rand()&3;
			sfxHandle_t	s = 0;

			if (!SC_Cvar_Get_Int("cl_useq3gibs")) {
				r = rand() % 4;
				if (r == 0) {
					s = cgs.media.electroGibBounceSound1;
				} else if (r == 1) {
					s = cgs.media.electroGibBounceSound2;
				} else if (r == 2) {
					s = cgs.media.electroGibBounceSound3;
				} else if (r == 3) {
					s = cgs.media.electroGibBounceSound4;
				}
			} else {
				// q3
				if ( r == 0 ) {
					s = cgs.media.gibBounce1Sound;
				} else if ( r == 1 ) {
					s = cgs.media.gibBounce2Sound;
				} else {
					s = cgs.media.gibBounce3Sound;
				}
			}
			trap_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}
	} else if ( le->leBounceSoundType == LEBS_BRASS ) {

	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->leBounceSoundType = LEBS_NONE;
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;


	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid ||
		( trace->plane.normal[2] > 0 &&
		( le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) ) {
		le->pos.trType = TR_STATIONARY;
	} else {

	}
}

/*
================
CG_AddFragment
================
*/
void CG_AddFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;
	refEntity_t *re;

	re = &le->refEntity;

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	oldZ;
		float newZ;
		vec3_t mins, maxs;

		t = le->endTime - cg.time;
		if ( t < SINK_TIME ) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy( le->refEntity.origin, le->refEntity.lightingOrigin );
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * ( 1.0 - (float)t / SINK_TIME );
			newZ = le->refEntity.origin[2];
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.origin[2] = oldZ;
		} else {
			oldZ = le->refEntity.origin[2];
			newZ = le->refEntity.origin[2];
			trap_R_AddRefEntityToScene( &le->refEntity );
		}

		switch (re->reType) {
		case RT_SPARK:
		case RT_SPRITE:
		case RT_SPRITE_FIXED:
			if (fabs(newZ - oldZ) > re->radius) {
				CG_FreeLocalEntity(le);
			}
			break;
		case RT_MODEL:
			trap_R_ModelBounds(re->hModel, mins, maxs);
			if (fabs(newZ - oldZ) > ((float)maxs[2] * re->radius)) {
				CG_FreeLocalEntity(le);
			}
			break;
		default:
			break;
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectoryf( &le->pos, cg.time, newOrigin, cg.foverf );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	//Com_Printf("trace.fraction %f\n", trace.fraction);
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectoryf( &le->angles, cg.time, angles, cg.foverf );
			AnglesToAxis( angles, le->refEntity.axis );
		}

		trap_R_AddRefEntityToScene( &le->refEntity );

		//Com_Printf("bounce type %d  (%d)  mark type %d  (%d)\n", le->leBounceSoundType, le->leBounceSoundType == LEBS_BLOOD, le->leMarkType, le->leMarkType == LEMT_BLOOD);
		// add a blood trail
		if ((le->leBounceSoundType == LEBS_BLOOD  &&  cg_blood.integer)  ||  (le->leMarkType == LEMT_BLOOD  &&  !SC_Cvar_Get_Int("cl_useq3gibs"))) {
		//if (le->leBounceSoundType == LEBS_BLOOD  &&  cg_blood.integer) {
			CG_BloodTrail( le );
		} else if (le->leMarkType == LEMT_ICE) {
			CG_IceTrail(le);
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( CG_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	if (Q_fabs(DotProduct(trace.plane.normal, trace.plane.normal)) == 0.0f) {
		//byte color[4] = { 255, 255, 0, 255 };

		//Com_Printf("CG_AddFragment skipping, trace.plane.normal all zero\n");
		//CG_FloatNumber(666, trace.endpos, RF_DEPTHHACK, color);
#ifndef Q3_VM
		//__asm__("int3");
#endif
		CG_FreeLocalEntity(le);
		//trap_R_AddRefEntityToScene(&le->refEntity);
		//trap_Cvar_Set("cl_freezedemo", "1");
		return;
	}

	// leave a mark
	CG_FragmentBounceMark( le, &trace );

	// do a bouncy sound
	CG_FragmentBounceSound( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );

	trap_R_AddRefEntityToScene( &le->refEntity );
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;
	double ourTime;

	if (le->leFlags & LEF_REAL_TIME) {
		ourTime = cg.realTime;
	} else {
		ourTime = cg.ftime;
	}

	re = &le->refEntity;

	c = ( le->endTime - ourTime ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene( re );
}

/*
==================
CG_AddMoveScaleFade
==================
*/
static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	if ( le->fadeInTime > le->startTime && cg.ftime < le->fadeInTime ) {
		// fade / grow time
		c = 1.0 - (float) ( le->fadeInTime - cg.ftime ) / ( le->fadeInTime - le->startTime );
	}
	else {
		// fade / grow time
		c = ( le->endTime - cg.ftime ) * le->lifeRate;
	}

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectoryf( &le->pos, cg.time, re->origin, cg.foverf );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius  &&  !cg_allowLargeSprites.integer) {
		//Com_Printf("view in sprite\n");
		if (!cg_allowSpritePassThrough.integer) {
			CG_FreeLocalEntity( le );
		}
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.ftime ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * ( 1.0 - c ) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius  &&  !cg_allowLargeSprites.integer) {
		if (!cg_allowSpritePassThrough.integer) {
			CG_FreeLocalEntity( le );
		}
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade time
	c = ( le->endTime - cg.ftime ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - ( 1.0 - c ) * le->pos.trDelta[2];

	if (!(le->leFlags & LEF_PUFF_DONT_SCALE_NOT_KIDDING_MAN)) {
		re->radius = le->radius * ( 1.0 - c ) + 16;
	} else {
		re->radius = le->radius;
	}

	//Com_Printf("radius %f\n", re->radius);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius  &&  !cg_allowLargeSprites.integer) {
		if (!cg_allowSpritePassThrough.integer) {
			CG_FreeLocalEntity( le );
		}
		return;
	}

	trap_R_AddRefEntityToScene( re );
}



/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t	*ent;

	ent = &ex->refEntity;

	// add the entity
	trap_R_AddRefEntityToScene(ent);

	// add the dlight
	if ( ex->light ) {
		float		light;

		light = (float)( cg.ftime - ex->startTime ) / ( ex->endTime - ex->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2] );
	}
}

/*
================
CG_AddSpriteExplosion
================
*/
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float c;

	re = le->refEntity;

	c = ( le->endTime - cg.ftime ) / ( float ) ( le->endTime - le->startTime );
	if ( c > 1 ) {
		c = 1.0;	// can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * ( 1.0 - c ) + 30;

	trap_R_AddRefEntityToScene( &re );

	// add the dlight
	if ( le->light ) {
		float		light;

		light = (float)( cg.ftime - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2] );
	}
}


#if 1  //def MPACK
/*
====================
CG_AddKamikaze
====================
*/
void CG_AddKamikaze( localEntity_t *le ) {
	refEntity_t	*re;
	refEntity_t shockwave;
	float		c;
	vec3_t		test, axis[3];
	//int			t;
	double tf;

	re = &le->refEntity;

	//t = cg.time - le->startTime;
	tf = cg.ftime - le->startTime;

	VectorClear( test );
	AnglesToAxis( test, axis );

	if (tf > (double)KAMI_SHOCKWAVE_STARTTIME && tf < (double)KAMI_SHOCKWAVE_ENDTIME) {

		if (!(le->leFlags & LEF_SOUND1)) {
//			trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeExplodeSound );
			trap_S_StartLocalSound(cgs.media.kamikazeExplodeSound, CHAN_AUTO);
			le->leFlags |= LEF_SOUND1;
		}
		// 1st kamikaze shockwave
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.shaderTime = re->shaderTime;
		VectorCopy(re->origin, shockwave.origin);

		c = (float)(tf - KAMI_SHOCKWAVE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME);
		VectorScale( axis[0], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0] );
		VectorScale( axis[1], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1] );
		VectorScale( axis[2], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2] );
		shockwave.nonNormalizedAxes = qtrue;

		if (tf > (double)KAMI_SHOCKWAVEFADE_STARTTIME) {
			c = (float)(tf - KAMI_SHOCKWAVEFADE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVEFADE_STARTTIME);
		}
		else {
			c = 0;
		}
		c *= 0xff;
		shockwave.shaderRGBA[0] = 0xff - c;
		shockwave.shaderRGBA[1] = 0xff - c;
		shockwave.shaderRGBA[2] = 0xff - c;
		shockwave.shaderRGBA[3] = 0xff - c;

		trap_R_AddRefEntityToScene( &shockwave );
	}

	if (tf > (double)KAMI_EXPLODE_STARTTIME && tf < (double)KAMI_IMPLODE_ENDTIME) {
		// explosion and implosion
		c = ( le->endTime - cg.ftime ) * le->lifeRate;
		c *= 0xff;
		re->shaderRGBA[0] = le->color[0] * c;
		re->shaderRGBA[1] = le->color[1] * c;
		re->shaderRGBA[2] = le->color[2] * c;
		re->shaderRGBA[3] = le->color[3] * c;

		if( tf < (double)KAMI_IMPLODE_STARTTIME ) {
			c = (float)(tf - KAMI_EXPLODE_STARTTIME) / (float)(KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME);
		}
		else {
			if (!(le->leFlags & LEF_SOUND2)) {
//				trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeImplodeSound );
				trap_S_StartLocalSound(cgs.media.kamikazeImplodeSound, CHAN_AUTO);
				le->leFlags |= LEF_SOUND2;
			}
			c = (float)(KAMI_IMPLODE_ENDTIME - tf) / (float) (KAMI_IMPLODE_ENDTIME - KAMI_IMPLODE_STARTTIME);
		}
		VectorScale( axis[0], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[0] );
		VectorScale( axis[1], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[1] );
		VectorScale( axis[2], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[2] );
		re->nonNormalizedAxes = qtrue;

		trap_R_AddRefEntityToScene( re );
		// add the dlight
		trap_R_AddLightToScene( re->origin, c * 1000.0, 1.0, 1.0, c );
	}

	if (tf > (double)KAMI_SHOCKWAVE2_STARTTIME && tf < (double)KAMI_SHOCKWAVE2_ENDTIME) {
		// 2nd kamikaze shockwave
		if (le->angles.trBase[0] == 0 &&
			le->angles.trBase[1] == 0 &&
			le->angles.trBase[2] == 0) {
			le->angles.trBase[0] = random() * 360;
			le->angles.trBase[1] = random() * 360;
			le->angles.trBase[2] = random() * 360;
		}
		else {
			c = 0;
		}
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.shaderTime = re->shaderTime;
		VectorCopy(re->origin, shockwave.origin);

		test[0] = le->angles.trBase[0];
		test[1] = le->angles.trBase[1];
		test[2] = le->angles.trBase[2];
		AnglesToAxis( test, axis );

		c = (float)(tf - KAMI_SHOCKWAVE2_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2_STARTTIME);
		VectorScale( axis[0], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0] );
		VectorScale( axis[1], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1] );
		VectorScale( axis[2], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2] );
		shockwave.nonNormalizedAxes = qtrue;

		if (tf > (double)KAMI_SHOCKWAVE2FADE_STARTTIME) {
			c = (float)(tf - KAMI_SHOCKWAVE2FADE_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2FADE_STARTTIME);
		}
		else {
			c = 0;
		}
		c *= 0xff;
		shockwave.shaderRGBA[0] = 0xff - c;
		shockwave.shaderRGBA[1] = 0xff - c;
		shockwave.shaderRGBA[2] = 0xff - c;
		shockwave.shaderRGBA[3] = 0xff - c;

		trap_R_AddRefEntityToScene( &shockwave );
	}
}

/*
===================
CG_AddInvulnerabilityImpact
===================
*/
void CG_AddInvulnerabilityImpact( localEntity_t *le ) {
	trap_R_AddRefEntityToScene( &le->refEntity );
}

/*
===================
CG_AddInvulnerabilityJuiced
===================
*/
void CG_AddInvulnerabilityJuiced( localEntity_t *le ) {
	//int t;
	double tf;
	centity_t cent;

	//t = cg.time - le->startTime;
	tf = cg.ftime - le->startTime;

	if ( tf > 3000.0 ) {
		le->refEntity.axis[0][0] = (float) 1.0 + 0.3 * (tf - 3000.0) / 2000.0;
		le->refEntity.axis[1][1] = (float) 1.0 + 0.3 * (tf - 3000.0) / 2000.0;
		le->refEntity.axis[2][2] = (float) 0.7 + 0.3 * (2000.0 - (tf - 3000.0)) / 2000.0;
	}
	if ( tf > 5000.0 ) {
		le->endTime = 0;
		//FIXME hack
		memset(&cent, 0, sizeof(cent));
		//VectorCopy(le->refEntity.origin, cent.lerpOrigin);
		VectorCopy(le->refEntity.origin, cent.currentState.pos.trBase);
		if (*EffectScripts.gibbed) {
			CG_FX_GibPlayer(&cent);
		} else {
			CG_GibPlayer(&cent);
		}
	}
	else {
		trap_R_AddRefEntityToScene( &le->refEntity );
	}
}

/*
===================
CG_AddRefEntity
===================
*/
void CG_AddRefEntity( localEntity_t *le ) {
	double ourTime;

	if (le->leFlags & LEF_REAL_TIME) {
		ourTime = cg.realTime;
	} else {
		ourTime = cg.ftime;
	}

	//FIXME wc  huh????
	//if (le->endTime < ourTime) {
	if (le->endTime > ourTime) {
		CG_FreeLocalEntity( le );
		return;
	}
	trap_R_AddRefEntityToScene( &le->refEntity );
}

#endif
/*
===================
CG_AddScorePlum
===================
*/
#define NUMBER_SIZE		8

void CG_AddScorePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len;
	int			i, score, digits[10], numdigits, negative;

	re = &le->refEntity;

	c = ( le->endTime - cg.ftime ) * le->lifeRate;

	score = le->radius;
	if (score < 0) {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}
	else {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if (score >= 50) {
			re->shaderRGBA[1] = 0;
		} else if (score >= 20) {
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		} else if (score >= 10) {
			re->shaderRGBA[2] = 0;
		} else if (score >= 2) {
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;
		}

	}
	if (c < 0.25)
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	VectorCopy(le->pos.trBase, origin);
	origin[2] += 110 - c * 100;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);

	VectorMA(origin, -10 + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < 20  &&  !cg_allowLargeSprites.integer) {
		if (!cg_allowSpritePassThrough.integer) {
			CG_FreeLocalEntity( le );
		}
		return;
	}

	negative = qfalse;
	if (score < 0) {
		negative = qtrue;
		score = -score;
	}

	for (numdigits = 0; !(numdigits && !score); numdigits++) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if (negative) {
		digits[numdigits] = 10;
		numdigits++;
	}

	for (i = 0; i < numdigits; i++) {
		VectorMA(origin, (float) (((float) numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene( re );
	}
}


static void CG_Add_FX_Emitted (localEntity_t *le)
{
	refEntity_t *re;
	float totalFade;
	float lf;
	double deltaTime;
	vec3_t newOrigin;
	float gravityValue;
	trace_t trace;
	vec3_t delta;
	float len;
	vec3_t lastOrigin;
	//float lastDeltaTime;
	float dot;
	//int hitTime;
	float ratio;

#if 0
	if (1) {  //(rand() % 100 != 0) {
		trap_R_AddRefEntityToScene(&le->refEntity);
		return;
	}
#endif

#if 0
	re = &le->refEntity;
	//len = Distance(cg.refdef.vieworg, ScriptVars.origin);
	len = Distance(cg.refdef.vieworg, re->origin);
	if (len > 500) {
		//return;
	}

	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		ratio = re->radius / len;
		if (ratio < cg_fxratio.value) {  //0.002) {
			Com_Printf("1 returning\n");
			trap_R_AddRefEntityToScene(re);
			return;
		}
	}
#endif

	//Com_Printf("%f fx %p\n", cg.ftime, le);

	CG_GetStoredScriptVarsFromLE(le);
	//Com_Printf("%f fx %p gravity %d\n", cg.ftime, le, ScriptVars.hasMoveGravity);

#if 1
	if (cg.renderingThirdPerson  ||  cg.freecam) {
		ScriptVars.inEyes = qfalse;
	} else {
		ScriptVars.inEyes = CG_IsUs(&cgs.clientinfo[ScriptVars.clientNum]);
	}
#endif

	ScriptVars.lerp = (cg.ftime - le->startTime) / (le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	re = &le->refEntity;

	//FIXME before or after running script?  If before can do distance and impact
	//FIXME other move, check for velocity?

#if 0
	if (le->pos.trType == TR_STATIONARY) {
		re->customShader = cgs.media.connectionShader;
	}
#endif

	if (ScriptVars.hasSink  &&  le->pos.trType == TR_STATIONARY) {
		//int t;
		double tf;
		float oldZ;
		float newZ;
		float sinkTime;
		vec3_t mins, maxs;

		// assuming sink1 is removal time as percent of total time
		// assuming sink2 is sink velocity
		sinkTime = (1.0 - ScriptVars.sink1) * (le->endTime - le->startTime);
		//t = le->endTime - cg.ftime;
		tf = le->endTime - cg.ftime;

		if (tf < sinkTime  &&  sinkTime != 0.0) {
			VectorCopy(re->origin, re->lightingOrigin);
			re->renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = re->origin[2];
			re->origin[2] -= ScriptVars.sink2 * (1.0 - (float)tf / sinkTime);
			newZ = re->origin[2];
			trap_R_AddRefEntityToScene(re);
			re->origin[2] = oldZ;
		} else {
			oldZ = re->origin[2];
			newZ = re->origin[2];
			trap_R_AddRefEntityToScene(re);
		}

		switch (re->reType) {
		case RT_SPARK:
		case RT_SPRITE:
		case RT_SPRITE_FIXED:
			if (fabs(newZ - oldZ) > re->radius) {
				CG_FreeLocalEntity(le);
			}
			break;
		case RT_MODEL:
			trap_R_ModelBounds(re->hModel, mins, maxs);
			if (fabs(newZ - oldZ) > ((float)maxs[2] * re->radius)) {
				//Com_Printf("free model\n");
				CG_FreeLocalEntity(le);
			}
			break;
		default:
			break;
		}

		return;  //FIXME run script?

	} else if (ScriptVars.hasMoveBounce  ||  ScriptVars.hasMoveGravity) {
		vec3_t oldVelocity;

		//deltaTime = (cg.ftime - le->startTime) * 0.001;
		deltaTime = (cg.ftime - le->trTimef) * 0.001;
		//lastDeltaTime = (le->lastRunTime - le->startTime) * 0.001;
		VectorMA(le->pos.trBase, deltaTime, ScriptVars.velocity, newOrigin);  //ScriptVars.origin);
		//VectorMA(le->pos.trBase, lastDeltaTime, ScriptVars.velocity /*fuck me*/, lastOrigin);
		VectorCopy(re->origin, lastOrigin);
		gravityValue = 0;
		if (ScriptVars.hasMoveGravity) {
			gravityValue = ScriptVars.moveGravity;
		} else if (ScriptVars.hasMoveBounce) {
			gravityValue = ScriptVars.moveBounce1;
		}
		VectorCopy(ScriptVars.velocity, oldVelocity);
		//FIXME deltaTime maybe not the best check
		if (gravityValue  &&  Q_fabs(deltaTime) > 0.0f) {
			//newOrigin[2] -= 0.5 * gravityValue * deltaTime * deltaTime;
			//newOrigin[2] -= gravityValue * deltaTime * deltaTime;
			newOrigin[2] -= gravityValue * deltaTime * deltaTime;
			VectorSubtract(newOrigin, lastOrigin, ScriptVars.velocity);
			//Com_Printf("%f\n", deltaTime);
			VectorScale(ScriptVars.velocity, 1.0 / deltaTime, ScriptVars.velocity);
			//ScriptVars.velocity[2] -= 800;
			//VectorCopy(ScriptVars.velocity, le->sv.velocity);
			//Com_P
		}

		//Com_Printf("vl %f distance %f time %f distance from orig %f\n", VectorLength(ScriptVars.velocity), Distance(ScriptVars.origin, newOrigin), deltaTime, Distance(le->pos.trBase, newOrigin));
		CG_Trace(&trace, re->origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);
		if (trace.fraction != 1.0) {
			if (CG_PointContents(trace.endpos, 0) & CONTENTS_NODROP) {
				//Com_Printf("freeing entity: nodrop\n");
				CG_FreeLocalEntity(le);
				return;
			}
			if (trace.surfaceFlags & SURF_NOIMPACT) {
				//Com_Printf("freeing entity: noimpact\n");
				CG_FreeLocalEntity(le);
				return;
			}
			VectorCopy(trace.endpos, newOrigin);
			//FIXME check for bounce, mark as stationary and do sink
			if (ScriptVars.hasMoveBounce) {
				//FIXME mark impact for impact script
				// reflect velocity
				//re->customShader = cgs.media.connectionShader;
				//FIXME frametime
				//hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
				//BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
				VectorCopy(oldVelocity, ScriptVars.velocity);
				//Com_Printf("bounce velocity old %f %f %f\n", oldVelocity[0], oldVelocity[1], oldVelocity[2]);
				dot = DotProduct(ScriptVars.velocity, trace.plane.normal);
				VectorMA(ScriptVars.velocity, -2 * dot, trace.plane.normal, ScriptVars.velocity);
				VectorScale(ScriptVars.velocity, ScriptVars.moveBounce2, ScriptVars.velocity);
				VectorCopy(ScriptVars.velocity, le->sv.velocity);
				VectorCopy(trace.endpos, le->pos.trBase);
				le->trTimef = cg.ftime;
				VectorCopy(trace.endpos, ScriptVars.impactOrigin);
				VectorCopy(trace.endpos, le->sv.impactOrigin);
				VectorCopy(trace.plane.normal, ScriptVars.impactDir);
				VectorCopy(trace.plane.normal, le->sv.impactDir);
				ScriptVars.impacted = qtrue;
				le->sv.impacted = qtrue;
				// check for stop
#if 0
				if (trace.allsolid  ||  (trace.plane.normal[2] > 0  &&  (ScriptVars.velocity[2] < 40  ||  ScriptVars.velocity[2] < -cg.frametime * ScriptVars.velocity[2]))) {
					le->pos.trType = TR_STATIONARY;
					//re->customShader = cgs.media.connectionShader;
				}
#endif
				//if (trace.allsolid  ||  (trace.plane.normal[2] > 0  &&  ScriptVars.velocity[2] < 40)) {
				if (VectorLength(ScriptVars.velocity) < 20) {
					le->pos.trType = TR_STATIONARY;
					//re->customShader = cgs.media.connectionShader;
				}
			} else {
				//Com_Printf("freeing local ent\n");
				CG_FreeLocalEntity(le);
				return;
			}
		} else {
			//Com_Printf("trace == 1.0\n");
		}
		//FIXME tumble

		//ScriptVars.distance += Distance(ScriptVars.origin, newOrigin);
		//le->sv.distance += Distance(ScriptVars.origin, newOrigin);
		le->sv.distance += Distance(lastOrigin, newOrigin);
		ScriptVars.distance = le->sv.distance;
		//ScriptVars.distance = 0;  //FIXME testing
		VectorCopy(newOrigin, ScriptVars.origin);

		//Com_Printf("moving\n");
		//FIXME trace, stop, gravity, bounce, distance
	}

#if 0
	{
		char buffer[4192];
		memcpy(buffer, le->emitterScript, le->emitterScriptEnd - le->emitterScript);
		buffer[le->emitterScriptEnd - le->emitterScript] = '\0';
		Com_Printf("emitter script: '%s'\n", buffer);
	}
#endif

	len = Distance(cg.refdef.vieworg, ScriptVars.origin);
	if (len > 500) {
		//return;
	}

	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		if (re->reType == RT_SPARK) {
			//VectorCopy(ScriptVars.velocity, re->oldorigin);
			//re->width = ScriptVars.width;
		} else if (re->reType == RT_SPRITE_FIXED) {  // quad
			//VectorCopy(ScriptVars.dir, re->oldorigin);
			re->width = 0;
		}
		ratio = re->radius / len;
		if (ratio < cg_fxratio.value) {  //0.002) {
			//Com_Printf("returning\n");
			trap_R_AddRefEntityToScene(re);
			return;
		}
	}

	if (cg.ftime - le->lastRunTime > cg_fxinterval.value) {  //  &&  cg.frametime < 9) {  //25) {  //(cg.time != le->lastRunTime) {  //(cg.time - le->lastRunTime > 100) {
	//if (rand() % 100 == 0) {
		DistanceScript = qfalse;  //qtrue;  //FIXME hack
		EmitterScript = qtrue;
		//Com_Printf("script: %s\n", le->emitterScript);
		CG_RunQ3mmeScript(le->emitterScript, le->emitterScriptEnd);
		EmitterScript = qfalse;
		DistanceScript = qfalse;
		le->lastRunTime = cg.ftime;
		//memcpy(&ScriptVars, &EmitterScriptVars, sizeof(ScriptVars_t));  //FIXME testing -- take out ScriptVars.* from below
		VectorCopy(EmitterScriptVars.origin, re->origin);
		//FIXME size?
		//VectorCopy(ScriptVars.origin, le->pos.trBase);
	} else {
		if (ScriptVars.hasMoveBounce  ||  ScriptVars.hasMoveGravity) {
			//VectorCopy(ScriptVars.origin, re->origin);
			VectorCopy(newOrigin, re->origin);
		} else {
			// use re->origin
		}
		//Com_Printf("width %f\n", re->width);
		trap_R_AddRefEntityToScene(re);
		return;
	}


	//Com_Printf("shader: '%s'\n", ScriptVars.shader);
	//FIXME check shaderTime
	//FIXME *scale *fade and move properties

	// kill sprite if view inside
	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		VectorSubtract(re->origin, cg.refdef.vieworg, delta);
		len = VectorLength(delta);
		if (len < EmitterScriptVars.size) {
			// don't free just skip
			//CG_FreeLocalEntity(le);
			//return;
		}
	}

	if (ScriptVars.hasAlphaFade) {
		totalFade = 1.0 - EmitterScriptVars.alphaFade;
		lf = 1 - totalFade * EmitterScriptVars.lerp;

		EmitterScriptVars.color[3] *= lf;
	}
	if (EmitterScriptVars.hasColorFade) {
		totalFade = 1.0 - EmitterScriptVars.colorFade;
		lf = 1 - totalFade * EmitterScriptVars.lerp;

		EmitterScriptVars.color[0] *= lf;
		EmitterScriptVars.color[1] *= lf;
		EmitterScriptVars.color[2] *= lf;
	}
	re->shaderRGBA[0] = 255 * EmitterScriptVars.color[0];
	re->shaderRGBA[1] = 255 * EmitterScriptVars.color[1];
	re->shaderRGBA[2] = 255 * EmitterScriptVars.color[2];
	re->shaderRGBA[3] = 255 * EmitterScriptVars.color[3];
	//Com_Printf("shader: '%d'\n", re->customShader);
	if (*EmitterScriptVars.shader) {
		//Com_Printf("loading shader '%s'\n", ScriptVars.shader);
		//re->customShader = trap_R_RegisterShader(ScriptVars.shader);
	} else {
		//re->customShader = 0;  //FIXME masking a bug
	}
	//re->customShader = 0;

	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		if (re->reType == RT_SPARK) {  //(EmitterScriptVars.width) {
			VectorCopy(EmitterScriptVars.velocity, re->oldorigin);
			re->width = EmitterScriptVars.width;
		} else if (re->reType == RT_SPRITE_FIXED) {
			VectorCopy(EmitterScriptVars.dir, re->oldorigin);
			re->width = 0;
		}
	}

	re->radius = EmitterScriptVars.size;
	//Com_Printf("size %f\n", re->radius);
	//FIXME hack
	if (re->reType == RT_RAIL_RINGS_Q3MME) {
		re->rotation = EmitterScriptVars.width;
	} else {
		re->rotation = EmitterScriptVars.rotate;
		//FIXME check
		//re->rotation = EmitterScriptVars.angle;
	}
	//Com_Printf("rotation %f\n", re->rotation);
	//FIXME add velocity or not?
	//VectorCopy(ScriptVars.velocity, le->pos.trDelta);

	// use ScriptVars incase it was reset by script system
	le->sv.distance = ScriptVars.distance;
	le->sv.impacted = ScriptVars.impacted;
	//VectorCopy(EmitterScriptVars.origin, le->sv.origin);
	VectorCopy(ScriptVars.lastIntervalPosition, le->sv.lastIntervalPosition);
	le->sv.lastIntervalTime = ScriptVars.lastIntervalTime;
	VectorCopy(ScriptVars.lastDistancePosition, le->sv.lastDistancePosition);
	le->sv.lastDistanceTime = ScriptVars.lastDistanceTime;
	//FIXME distance and radius culling

	trap_R_AddRefEntityToScene(re);
}

static void CG_Run_FX_Emitted_Script (localEntity_t *le)
{

	//return;
	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_GetStoredScriptVarsFromLE(le);
	ScriptVars.lerp = (float)(cg.ftime - le->startTime) / (float)(le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	//return;

	if (cg.ftime != le->lastRunTime) {
		EmitterScript = qtrue;
		PlainScript = qtrue;  //FIXME shit
		if (ScriptVars.lerp >= 1.0) {
			//Com_Printf("last time...  %p\n", le);
		}
		CG_RunQ3mmeScript(le->emitterScript, NULL);
		EmitterScript = qfalse;
		PlainScript = qfalse;
		le->lastRunTime = cg.ftime;
	}

}

static void CG_Add_FX_Emitted_Light (localEntity_t *le)
{

	//return;
	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_GetStoredScriptVarsFromLE(le);
	ScriptVars.lerp = (float)(cg.ftime - le->startTime) / (float)(le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	//return;

	if (cg.ftime != le->lastRunTime) {
		EmitterScript = qtrue;
		//PlainScript = qtrue;  //FIXME shit
		CG_RunQ3mmeScript(le->emitterScript, NULL);
		EmitterScript = qfalse;
		//PlainScript = qfalse;
		le->lastRunTime = cg.ftime;
	}

	trap_R_AddLightToScene(ScriptVars.origin, ScriptVars.size, ScriptVars.color[0], ScriptVars.color[1], ScriptVars.color[2]);
}

static void CG_Add_FX_Emitted_Sound (localEntity_t *le)
{
	sfxHandle_t sfx;

	//return;
	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_GetStoredScriptVarsFromLE(le);
	ScriptVars.lerp = (float)(cg.ftime - le->startTime) / (float)(le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	//return;

	if (cg.ftime != le->lastRunTime) {
		EmitterScript = qtrue;
		//PlainScript = qtrue;  //FIXME shit
		CG_RunQ3mmeScript(le->emitterScript, NULL);
		EmitterScript = qfalse;
		//PlainScript = qfalse;
		le->lastRunTime = cg.ftime;
	}
	if (*ScriptVars.sound) {
		sfx = trap_S_RegisterSound(ScriptVars.sound, qfalse);
		trap_S_StartSound(ScriptVars.origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
	}
}

static void CG_Add_FX_Emitted_LoopSound (localEntity_t *le)
{
	qhandle_t lsound;

	//return;
	//memset(&ScriptVars, 0, sizeof(ScriptVars));
	CG_GetStoredScriptVarsFromLE(le);
	ScriptVars.lerp = (float)(cg.ftime - le->startTime) / (float)(le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	//return;

	if (cg.ftime != le->lastRunTime) {
		EmitterScript = qtrue;
		//PlainScript = qtrue;  //FIXME shit
		CG_RunQ3mmeScript(le->emitterScript, NULL);
		EmitterScript = qfalse;
		//PlainScript = qfalse;
		le->lastRunTime = cg.ftime;
	}
	lsound = trap_S_RegisterSound(ScriptVars.loopSound, qfalse);
	if (lsound) {
		//trap_S_AddLoopingSound(ScriptVars.entNumber, ScriptVars.origin, ScriptVars.velocity, lsound);
		//FIXME 1021 hack
		trap_S_AddLoopingSound(1021 /**/, ScriptVars.origin, ScriptVars.velocity, lsound);
	}
}

//==============================================================================

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t	*le, *next;
	int count;
	double ourTime;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	count = 0;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if (le->leFlags & LEF_REAL_TIME) {
			ourTime = cg.realTime;
		} else {
			ourTime = cg.ftime;
		}

		if (!le->fxType) {
			if (ourTime >= le->endTime) {
				CG_FreeLocalEntity(le);
				continue;
			}
		} else {
			if (le->sv.emitterFullLerp  &&  le->sv.emitterKill) {
				//Com_Printf("killing emitter full %p\n", le);
				CG_FreeLocalEntity(le);
				continue;
			} else if (le->sv.emitterFullLerp  &&  ourTime >= le->endTime) {
				//Com_Printf("going to kill emitter %p\n", le);
				le->sv.emitterKill = qtrue;
			} else if (ourTime >= le->endTime) {
				CG_FreeLocalEntity(le);
				continue;
			}
		}

#if 0  // testing
		switch (le->fxType) {
		case LEFX_EMIT: {
			refEntity_t *re;
			float dist;
			float ratio;

			re = &le->refEntity;
			dist = Distance(cg.refdef.vieworg, re->origin);
			ratio = re->radius / dist;
			//Com_Printf("%d  %f\n", count, ratio);
			//if (ratio > 0.0005) {
			if (ratio > 0.0020) {
				trap_R_AddRefEntityToScene(&le->refEntity);
			}
			break;
		}
		default:
			break;
		}
		continue;
#endif

		count++;

#if 0  // testing
		if (count > 200) {
			continue;
		}
#endif

		switch (le->fxType) {
		case LEFX_EMIT:
			CG_Add_FX_Emitted(le);
			continue;
		case LEFX_SCRIPT:
			CG_Run_FX_Emitted_Script(le);
			continue;
		case LEFX_EMIT_LIGHT:
			CG_Add_FX_Emitted_Light(le);
			continue;
		case LEFX_EMIT_SOUND:
			CG_Add_FX_Emitted_Sound(le);
			continue;
		case LEFX_EMIT_LOOPSOUND:
			CG_Add_FX_Emitted_LoopSound(le);
			continue;
		default:
			break;
		}

		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FRAGMENT:			// gibs and brass
			CG_AddFragment( le );
			break;

		case LE_MOVE_SCALE_FADE:		// water bubbles
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:				// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE: // gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:		// rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum( le );
			break;

#if 1  //def MPACK
		case LE_KAMIKAZE:
			CG_AddKamikaze( le );
			break;
		case LE_INVULIMPACT:
			CG_AddInvulnerabilityImpact( le );
			break;
		case LE_INVULJUICED:
			CG_AddInvulnerabilityJuiced( le );
			break;
		case LE_SHOWREFENTITY:
			CG_AddRefEntity( le );
			break;
#endif
		}
	}
}

void CG_RemoveFXLocalEntities (qboolean all, float emitterId)
{
	localEntity_t	*le, *next;

	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		//if (le->fxType == LEFX_EMIT  ||  le->fxType == LEFX_SCRIPT  ||  le->fxType == LEFX_EMIT_LIGHT  ||  le->fxType == LEFX_EMIT_SOUND  ||  le->fxType == LEFX_EMIT_LOOPSOUND) {
		if (le->fxType  &&  (all  ||  le->sv.emitterId == emitterId)) {
			CG_FreeLocalEntity(le);
			continue;
		}
	}
}



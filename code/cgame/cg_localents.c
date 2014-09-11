// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#include "cg_effects.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_predict.h"
#include "cg_syscalls.h"
#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)
  #include "cg_thread.h"
#endif
#include "sc.h"

//#define THREAD_DEBUG

#define MAX_LOCAL_ENTITIES (MAX_REFENTITIES - 3)

static localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
static localEntity_t	cg_activeLocalEntities;		// double linked list
static localEntity_t	*cg_freeLocalEntities;		// single linked list

static localEntity_t tmpLocalEntity;

int FxLoopSounds = FX_LOOP_SOUNDS_BASE;

//FIXME force too many?
//#define MAX_FX_EXTERNAL_FORCES 128
//FIXME force cg_local.h

//FIXME force linked list
//static fxExternalForce_t FxExternalForces[MAX_FX_EXTERNAL_FORCES];
//static int numFxExternalForces = 0;

#define FRAME_COUNT_THREAD_SHUTDOWN -1000
#define FRAME_COUNT_INITIAL 1
#define MAX_FRAME_COUNT 32

static int FrameCount = FRAME_COUNT_INITIAL;

#define MAX_THREADS 7

// list iteration can change held pointer
// thread index 0 means no threads, if using threads index begins at 1 (main process) and the rest are MAX_THREADS
static localEntity_t *Next[MAX_THREADS + 2];


#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

static qboolean ThreadInit = qfalse;
static thread_t ThreadIds[MAX_THREADS];

static thread_mutex_t OurGlobalLock;
static thread_mutex_t EntListLock;
static thread_mutex_t NextLock;
static thread_mutex_t CountLock;
static thread_mutex_t EntHandledLock;
static thread_mutex_t SysCallLock;

static semaphore_t RunSem[MAX_THREADS];
static semaphore_t StopSem[MAX_THREADS];


#if 1

#define Lock_Global() thread_mutex_lock(&OurGlobalLock)
#define Unlock_Global() thread_mutex_unlock(&OurGlobalLock)

#else

#define Lock_Global()
#define Unlock_Global()

#endif

#define Lock_Next() thread_mutex_lock(&NextLock)
#define Unlock_Next() thread_mutex_unlock(&NextLock)

#define Lock_EntList() thread_mutex_lock(&EntListLock)
#define Unlock_EntList() thread_mutex_unlock(&EntListLock)

#define Lock_Count() thread_mutex_lock(&CountLock)
#define Unlock_Count() thread_mutex_unlock(&CountLock)

#define Lock_EntHandled() thread_mutex_lock(&EntHandledLock)
#define Unlock_EntHandled() thread_mutex_unlock(&EntHandledLock)

#define Lock_SysCall() thread_mutex_lock(&SysCallLock)
#define Unlock_SysCall() thread_mutex_unlock(&SysCallLock)


#else  // if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

#define Lock_Global()
#define Unlock_Global()

#define Lock_EntList()
#define Unlock_EntList()

#define Lock_Next()
#define Unlock_Next()

#define Lock_Count()
#define Unlock_Count()

#define Lock_EntHandled()
#define Unlock_EntHandled()

#define Lock_SysCall()
#define Unlock_SysCall()

#endif  // if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)


static void Increment_FrameCount (void)
{
	FrameCount++;

	// negative FrameCount used as signal
	if (FrameCount > MAX_FRAME_COUNT) {
		FrameCount = FRAME_COUNT_INITIAL;
	}
}

#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

static void CG_AddLocalEntitiesExt (int threadNum);

static void *runThread (void *data)
{
	int number;
	int newCount;

	number = (int)data;

	while (1) {
		qboolean done;

		semaphore_wait(&RunSem[number]);

		done = qfalse;

		Lock_Count();
		newCount = FrameCount;
		Unlock_Count();

		// doit

		if (newCount == FRAME_COUNT_THREAD_SHUTDOWN) {
			//Com_Printf("exit thread %d %f\n", number, cg.ftime);
			done = qtrue;
		} else {
			CG_AddLocalEntitiesExt(number + 2);
			//Com_Printf("thread %d: %d\n", number, count);
		}

		//Com_Printf("*thread(%d) complete %d %f\n", number, count, cg.ftime);

		// tell main thread we are done
		semaphore_post(&StopSem[number]);

		if (done) {
			break;
		}

	}

	//Com_Printf("thread %d dead %f\n", number, cg.ftime);
    thread_exit(NULL);
}
#endif

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void CG_InitLocalEntities (void)
{
	int		i;

#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

	if (!ThreadInit) {
		thread_mutex_init(&OurGlobalLock, NULL);
		thread_mutex_init(&EntListLock, NULL);
		thread_mutex_init(&NextLock, NULL);
		thread_mutex_init(&CountLock, NULL);
		thread_mutex_init(&EntHandledLock, NULL);
		thread_mutex_init(&SysCallLock, NULL);

		for (i = 0;  i < ARRAY_LEN(RunSem);  i++) {
			if (semaphore_init(&RunSem[i], 0, 0) != 0) {
				Com_Printf("run semaphore %d init failed\n", i);
			}
		}

		for (i = 0;  i < ARRAY_LEN(StopSem);  i++) {
			if (semaphore_init(&StopSem[i], 0, 0) != 0) {
				Com_Printf("stop semaphore %d init failed\n", i);
			}
		}
	}

#endif  // if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

	Lock_EntList();

	memset(cg_localEntities, 0, sizeof(cg_localEntities));

	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	cg_activeLocalEntities.lowNext = &cg_activeLocalEntities;
	cg_activeLocalEntities.lowPrev = &cg_activeLocalEntities;

	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}

	Lock_Next();
	for (i = 0;  i < ARRAY_LEN(Next);  i++) {
		Next[i] = NULL;
	}
	Unlock_Next();

	cg.numLocalEntities = 0;

	Unlock_EntList();

#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

	if (!ThreadInit) {
		int r;

		for (i = 0;  i < ARRAY_LEN(ThreadIds);  i++) {
			r = thread_create(&ThreadIds[i], NULL, runThread, (void *)i);
			if (r) {
				CG_Printf("^1%s couldn't create thread %d\n", __FUNCTION__, i + 1);
			} else {
				CG_Printf("^6thread %d started\n", i + 1);
			}
		}

		ThreadInit = qtrue;
	}
#endif

	//Com_Printf("jitToken %f\n", sizeof(EffectScripts.jitToken) / (1024.0 * 1024.0));
}

void CG_ShutdownLocalEnts (qboolean destructor)
{
	if (!destructor) {
		CG_Printf("^5local ents shutdown\n");
	}

#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

	if (ThreadInit) {
		int i;

		Lock_Count();
		FrameCount = FRAME_COUNT_THREAD_SHUTDOWN;
		Unlock_Count();

		// wake threads
		for (i = 0;  i < ARRAY_LEN(RunSem);  i++) {
			semaphore_post(&RunSem[i]);
		}

		// wait until threads are done
		for (i = 0;  i < ARRAY_LEN(StopSem);  i++) {
			semaphore_wait(&StopSem[i]);
		}

		thread_join(ThreadIds[1], NULL);

		//CG_Printf("threads done...\n");

		thread_mutex_destroy(&OurGlobalLock);
		thread_mutex_destroy(&EntListLock);
		thread_mutex_destroy(&NextLock);
		thread_mutex_destroy(&CountLock);
		thread_mutex_destroy(&EntHandledLock);
		thread_mutex_destroy(&SysCallLock);

		for (i = 0;  i < ARRAY_LEN(RunSem);  i++) {
			semaphore_destroy(&RunSem[i]);
		}

		for (i = 0;  i < ARRAY_LEN(StopSem);  i++) {
			semaphore_destroy(&StopSem[i]);
		}

		ThreadInit = qfalse;
		if (!destructor) {
			CG_Printf("threads shutdown\n");
		}
	}
#endif  // if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)
}

/*
==================
CG_FreeLocalEntity
==================
*/
static void CG_FreeLocalEntity( localEntity_t *le ) {
	int i;

	if (le == &tmpLocalEntity) {
		return;
	}

	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	Lock_Next();

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	if (le->lowPriority) {
		le->lowPrev->lowNext = le->lowNext;
		le->lowNext->lowPrev = le->lowPrev;
	}


	for (i = 0;  i < ARRAY_LEN(Next);  i++) {
		if (Next[i] == le  &&  le) {
			Next[i] = le->prev;
			//Com_Printf("shifting %p\n", le);
		}
	}


	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
	cg.numLocalEntities--;

	Unlock_Next();

}

static void CG_DropLocalEntity (void)
{
	localEntity_t *le;
	int count;

	//Lock_EntList();

	if (cg_fxDebugEntities.integer > 2) {
		Com_Printf("start---\n");
	}

	//FIXME testing
	//CG_FreeLocalEntity(cg_activeLocalEntities.prev);
	//Unlock_EntList();
	//return;

	le = cg_activeLocalEntities.lowPrev;
	if (le != &cg_activeLocalEntities) {
		if (cg_fxDebugEntities.integer > 1) {
			Com_Printf("^5%f  max local ents, freeing low priority entity %p  emitterId %f  startTime %f\n", cg.ftime, le, le->sv.emitterId, le->startTime);
		}
		CG_FreeLocalEntity(le);
	} else {
		le = cg_activeLocalEntities.prev;
		count = 0;
		while (1) {
			if (le == &cg_activeLocalEntities) {
				if (cg_fxDebugEntities.integer > 1) {
					Com_Printf("^1CG_DropLocalEntity() couldn't find nonfx entity to free up using %p\n", le);
				}
				le = cg_activeLocalEntities.prev;
				break;
			}

			count++;

			if (le->leFlags & LEF_ALREADY_ADDED) {
				// skip
				if (cg_fxDebugEntities.integer > 3) {
					Com_Printf("^3%f  skipping already added nonfx %p (id %d)  %d  startTime %f\n", cg.ftime, le, le->leMarkType, count, le->startTime);
				}
				le = le->prev;
				continue;
			}

			// found it
			break;
		}
		if (cg_fxDebugEntities.integer > 0) {
			Com_Printf("^3%f  max local ents, freeing entity %p startTime %f\n", cg.ftime, le, le->startTime);
		}
		CG_FreeLocalEntity(le);
	}

	//Unlock_EntList();
}


static localEntity_t *CG_AllocLocalEntityExt (qboolean checkPause)
{
	localEntity_t	*le;

	Lock_EntList();

	if (checkPause  &&  SC_Cvar_Get_Int("cl_freezeDemo")) {
		//Com_Printf("script while paused\n");
		memset(&tmpLocalEntity, 0, sizeof(tmpLocalEntity));
		Unlock_EntList();
		return &tmpLocalEntity;
	}

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_DropLocalEntity();
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;

	cg.numLocalEntities++;

    Unlock_EntList();

	return le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/

localEntity_t *CG_AllocLocalEntity (void)
{
	return CG_AllocLocalEntityExt(qtrue);
}

// entities that work in real time:  camera path lines and freecam rail
localEntity_t *CG_AllocLocalEntityRealTime (void)
{
	return CG_AllocLocalEntityExt(qfalse);
}

void CG_MakeLowPriorityEntity (localEntity_t *le)
{
	Lock_EntList();

	le->lowNext = cg_activeLocalEntities.lowNext;
	le->lowPrev = &cg_activeLocalEntities;
	cg_activeLocalEntities.lowNext->lowPrev = le;
	cg_activeLocalEntities.lowNext = le;
	le->lowPriority = qtrue;

	Unlock_EntList();
}

static void R_AddRefEntityPtrToScene (refEntity_t *ent)
{
	if (cg_fxq3mmeCompatibility.integer) {
		trap_R_AddRefEntityToScene(ent);
	} else {
		trap_R_AddRefEntityPtrToScene(ent);
	}
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
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
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

static void CG_BloodTrail_ql( const localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;
	qhandle_t	shader;
	float radius;
	int i;
	vec4_t color;

	shader = trap_R_RegisterShader("xgibs");

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

	step = cg_gibStepTime.integer;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	//Com_Printf("t: %d  t2: %d   step: %d  cg.frametime %d\n", t, t2, step, cg.frametime);
	//Com_Printf("cg.time: %d   cg.frametime: %d\n", cg.time, cg.frametime);

	for ( ; t <= t2; t += step ) {
		for (i = 0;  i < 1;  i++) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		//Com_Printf("trail t %d\n", t);
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
		blood->refEntity.rotation = 0;
		}
	}
}

static void CG_BloodTrail( const localEntity_t *le ) {
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

static void CG_IceTrail( const localEntity_t *le ) {
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
static void CG_FragmentBounceMark( localEntity_t *le, const trace_t *trace ) {
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
static void CG_FragmentBounceSound( localEntity_t *le, const trace_t *trace ) {
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
static void CG_ReflectVelocity( localEntity_t *le, const trace_t *trace ) {
	vec3_t	velocity;
	//float	dot;
	int		hitTime;

	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );

	// reflect the velocity on the trace plane

	VectorReflect(velocity, trace->plane.normal, le->pos.trDelta);

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
static void CG_AddFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;
	const refEntity_t *re;

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
				//Lock_EntList();
				//CG_FreeLocalEntity(le);
				//Unlock_EntList();
				// already called R_AddRefEntityPtrToScene()
				le->endTime = 0;
				//le->startTime = -1;
				le->fxType = 0;
				//le->leFlags = LEF_ALREADY_ADDED_FX;
			}
			break;
		case RT_MODEL:
			trap_R_ModelBounds(re->hModel, mins, maxs);
			if (fabs(newZ - oldZ) > ((float)maxs[2] * re->radius)) {
				//Lock_EntList();
				//CG_FreeLocalEntity(le);
				//Unlock_EntList();
				// already called R_AddRefEntityPtrToScene()
				le->endTime = 0;
				//le->startTime = -2;
				le->fxType = 0;
				//le->leFlags = LEF_ALREADY_ADDED_FX;
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
		Lock_EntList();
		CG_FreeLocalEntity( le );
		Unlock_EntList();
		return;
	}

	if (Q_fabs(DotProduct(trace.plane.normal, trace.plane.normal)) == 0.0f) {
		//byte color[4] = { 255, 255, 0, 255 };

		//Com_Printf("CG_AddFragment skipping, trace.plane.normal all zero\n");
		//CG_FloatNumber(666, trace.endpos, RF_DEPTHHACK, color);
#ifndef Q3_VM
		//__asm__("int3");
#endif
		Lock_EntList();
		CG_FreeLocalEntity(le);
		Unlock_EntList();
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
static void CG_AddFadeRGB( localEntity_t *le ) {
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
			Lock_EntList();
			CG_FreeLocalEntity( le );
			Unlock_EntList();
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
			Lock_EntList();
			CG_FreeLocalEntity( le );
			Unlock_EntList();
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
			Lock_EntList();
			CG_FreeLocalEntity( le );
			Unlock_EntList();
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
static void CG_AddExplosion( const localEntity_t *ex ) {
	const refEntity_t	*ent;

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
	refEntity_t	re;  //FIXME why a copy?
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
static void CG_AddKamikaze( localEntity_t *le ) {
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

		CG_AddRefEntity(&shockwave);
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

		CG_AddRefEntity(re);
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

		CG_AddRefEntity(&shockwave);
	}
}

/*
===================
CG_AddInvulnerabilityImpact
===================
*/
static void CG_AddInvulnerabilityImpact( const localEntity_t *le ) {
	trap_R_AddRefEntityToScene( &le->refEntity );
}

/*
===================
CG_AddInvulnerabilityJuiced
===================
*/
static void CG_AddInvulnerabilityJuiced( localEntity_t *le ) {
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
CG_AddLocalRefEntity
===================
*/
static void CG_AddLocalRefEntity( localEntity_t *le ) {
	double ourTime;

	if (le->leFlags & LEF_REAL_TIME) {
		ourTime = cg.realTime;
	} else {
		ourTime = cg.ftime;
	}

	//FIXME wc  huh????
	//if (le->endTime < ourTime) {
	if (le->endTime > ourTime) {
		Lock_EntList();
		CG_FreeLocalEntity( le );
		Unlock_EntList();
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

static void CG_AddScorePlum( localEntity_t *le ) {
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
			Lock_EntList();
			CG_FreeLocalEntity( le );
			Unlock_EntList();
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
		CG_AddRefEntity(re);
	}
}

/*
===================
CG_AddHeadShotPlum
===================
*/
static void CG_AddHeadShotPlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len;
	float r;

	re = &le->refEntity;

	c = ( le->endTime - cg.ftime ) * le->lifeRate;

	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0xff;

	r = 32.0;
	re->radius = r - (r * c);

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
			Lock_EntList();
			CG_FreeLocalEntity( le );
			Unlock_EntList();
		}
		return;
	}

	re->customShader = cgs.media.headShotIcon;
	VectorCopy(origin, re->origin);

	trap_R_AddRefEntityToScene(re);
}

void CG_RunFxAll (const char *name)
{
	localEntity_t *le, *next;
	int i;
	const char *fx;

	fx = NULL;
	for (i = 0;  i < EffectScripts.numEffects;  i++) {
		if (!Q_stricmp(name, EffectScripts.names[i])) {
			//CG_RunQ3mmeScript(EffectScripts.ptr[i], NULL);
			fx = EffectScripts.ptr[i];
			break;
		}
	}

	if (!fx) {
		Com_Printf("couldn't find fx '%s'\n", name);
		return;
	}

	Lock_EntList();

	le = cg_activeLocalEntities.prev;
	for ( ;  le != &cg_activeLocalEntities;  le = next) {
		next = le->prev;

		if (!le->fxType) {
			continue;
		}
		CG_GetStoredScriptVarsFromLE(le);

		CG_RunQ3mmeScript(fx, NULL);

		//CG_CopyStoredScriptVarsToLE(le);
		memcpy(&le->sv, &ScriptVars, sizeof(ScriptVars_t));
		//VectorCopy(ScriptVars.origin, re->origin);
		//VectorCopy(ScriptVars.origin, le->pos.trBase);
	}

	Unlock_EntList();
}

static qboolean IsFxModel (int reType)
{
	switch (reType) {
	case RT_MODEL_FX_DIR:
	case RT_MODEL_FX_ANGLES:
	case RT_MODEL_FX_AXIS:
		return qtrue;
		break;
	default:
		return qfalse;
		break;
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


	CG_GetStoredScriptVarsFromLE(le);

	// testing
	//R_AddRefEntityPtrToScene(&le->refEntity);
	//return;

	if (cg.renderingThirdPerson  ||  cg.freecam) {
		ScriptVars.inEyes = qfalse;
	} else {
		ScriptVars.inEyes = CG_IsUs(&cgs.clientinfo[ScriptVars.clientNum]);
	}

	ScriptVars.lerp = (cg.ftime - le->startTime) / (le->endTime - le->startTime);
	if (ScriptVars.lerp >= 1.0) {
		ScriptVars.lerp = 1.01;
	}

	re = &le->refEntity;

#if 0  //FIXME maybe?
	if (IsFxModel(re->reType)  &&  VectorNormalize2(ScriptVars.velocity, re->axis[0]) == 0) {
		//Com_Printf("^3yes... model\n");
		re->axis[0][2] = 1;
	}
#endif

	if (IsFxModel(re->reType)) {
		re->skinNum = cg.clientFrame & 1;  //FIXME changeable from scripts?
	}

	//FIXME before or after running script?  If before can do distance and impact
	//FIXME other move, check for velocity?

	if (ScriptVars.hasSink  &&  le->pos.trType == TR_STATIONARY) {
		double tf;
		float oldZ;
		float newZ;
		float sinkTime;
		vec3_t mins, maxs;
		int i;

#if 0  // force
		// add external forces
		//FIXME force
		//for (i = 0;  i < numFxExternalForces;  i++) {
		for (i = 0;  i < MAX_FX_EXTERNAL_FORCES;  i++) {
			const fxExternalForce_t *force;
			vec3_t dir;

			force = &FxExternalForces[i];
			if (!force->valid) {
				continue;
			}

			if (force->radial) {
				float dist;
				float f;
				float maxDist;
				float power;
				vec3_t forceOrigin;

				VectorCopy(force->origin, forceOrigin);
				forceOrigin[2] += 25 + 5 * random();

				//VectorSubtract(ScriptVars.origin, force->origin, dir);
				//VectorSubtract(force->origin, ScriptVars.origin, dir);
				//dist = Distance(force->origin, ScriptVars.origin);

				VectorSubtract(le->pos.trBase, forceOrigin, dir);
				dist = Distance(le->pos.trBase, forceOrigin);

				//dist = VectorNormalize(dir);
				//VectorScale(dir, 20, dir);
				//VectorScale(dir, force->power, dir);
				//VectorScale(dir, 10, dir);
				//f = 10.0f;
				//f = 1.0f;
				//fac = 1.0 - newLen / maxDist;
				//newValue = ScriptVars.vibrate * fac;

				power = 0.15f;
				maxDist = 500.0f;
				f = 1.0 - dist / maxDist;
				if (f < 0.0) {
					continue;
				}

				Com_Printf("dir: %f %f %f", dir[0], dir[1], dir[2]);

				VectorScale(dir, power * f, dir);
				le->trTimef = cg.ftime;

				if (le->pos.trType == TR_STATIONARY) {
					le->pos.trType = TR_LINEAR;
					VectorCopy(dir, ScriptVars.velocity);
				} else {

					VectorAdd(dir, ScriptVars.velocity, ScriptVars.velocity);
				}

				VectorCopy(ScriptVars.velocity, le->sv.velocity);

				continue;
			}

			//FIXME nonradial

		}
#endif

		// assuming sink1 is removal time as percent of total time
		// assuming sink2 is sink velocity
		sinkTime = (1.0 - ScriptVars.sink1) * (le->endTime - le->startTime);
		tf = le->endTime - cg.ftime;

		if (tf < sinkTime  &&  sinkTime != 0.0) {
			VectorCopy(re->origin, re->lightingOrigin);
			re->renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = re->origin[2];
			re->origin[2] -= ScriptVars.sink2 * (1.0 - (float)tf / sinkTime);
			newZ = re->origin[2];
			R_AddRefEntityPtrToScene(re);
			re->origin[2] = oldZ;
		} else {
			oldZ = re->origin[2];
			newZ = re->origin[2];
			R_AddRefEntityPtrToScene(re);
		}

		switch (re->reType) {
		case RT_SPARK:
		case RT_SPRITE:
		case RT_SPRITE_FIXED:
			if (fabs(newZ - oldZ) > re->radius) {
				//Lock_EntList();
				//CG_FreeLocalEntity(le);
				//Unlock_EntList();
				// already called R_AddRefEntityPtrToScene()
				le->endTime = 0;
				//le->startTime = -3;
				le->fxType = 0;
				//le->leFlags = LEF_ALREADY_ADDED_FX;
			}
			break;
		case RT_MODEL:
			trap_R_ModelBounds(re->hModel, mins, maxs);
			if (fabs(newZ - oldZ) > ((float)maxs[2] * re->radius)) {
				//Com_Printf("free model\n");
				//Lock_EntList();
				//CG_FreeLocalEntity(le);
				//Unlock_EntList();
				// already called R_AddRefEntityPtrToScene()
				le->endTime = 0;
				//le->startTime = -4;
				le->fxType = 0;
				//le->leFlags = LEF_ALREADY_ADDED_FX;
			}
			break;
		default:
			break;
		}

		return;  //FIXME run script?

	} else if (ScriptVars.hasMoveBounce  ||  ScriptVars.hasMoveGravity) {
		vec3_t oldVelocity;
		int i;

#if 0  // force
		// add external forces
		//FIXME force
		//for (i = 0;  i < numFxExternalForces;  i++) {
		for (i = 0;  i < MAX_FX_EXTERNAL_FORCES;  i++) {
			const fxExternalForce_t *force;
			vec3_t dir;

			force = &FxExternalForces[i];
			if (!force->valid) {
				continue;
			}

			if (force->radial) {
				float dist;
				float f;
				float maxDist;
				float power;
				vec3_t forceOrigin;

				VectorCopy(force->origin, forceOrigin);
				forceOrigin[2] += 25 + 5 * random();

				//VectorSubtract(ScriptVars.origin, force->origin, dir);
				//VectorSubtract(force->origin, ScriptVars.origin, dir);
				//dist = Distance(force->origin, ScriptVars.origin);

				VectorSubtract(le->pos.trBase, forceOrigin, dir);
				dist = Distance(le->pos.trBase, forceOrigin);

				//dist = VectorNormalize(dir);
				//VectorScale(dir, 20, dir);
				//VectorScale(dir, force->power, dir);
				//VectorScale(dir, 10, dir);
				//f = 10.0f;
				//f = 1.0f;
				//fac = 1.0 - newLen / maxDist;
				//newValue = ScriptVars.vibrate * fac;

				power = 0.15f;
				maxDist = 500.0f;
				f = 1.0 - dist / maxDist;
				if (f < 0.0) {
					continue;
				}

				Com_Printf("dir: %f %f %f", dir[0], dir[1], dir[2]);

				VectorScale(dir, power * f, dir);
				le->trTimef = cg.ftime;

				if (le->pos.trType == TR_STATIONARY) {
					le->pos.trType = TR_LINEAR;
					VectorCopy(dir, ScriptVars.velocity);
				} else {

					VectorAdd(dir, ScriptVars.velocity, ScriptVars.velocity);
				}

				VectorCopy(ScriptVars.velocity, le->sv.velocity);

				continue;
			}

			//FIXME nonradial

		}
#endif

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

#if 0  // no..  fast moving emitter can go through something and both start and end in non solid
		//Com_Printf("distance %f\n", Distance(re->origin, newOrigin));

		if (CG_PointContents(newOrigin, 0) & CONTENTS_SOLID) {

			CG_Trace(&trace, re->origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);
		} else {
			VectorCopy(newOrigin, trace.endpos);
			trace.fraction = 1.0;
		}

#else

		CG_Trace(&trace, re->origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);

#endif

		if (trace.fraction != 1.0) {
			if (CG_PointContents(trace.endpos, 0) & CONTENTS_NODROP) {
				//Com_Printf("freeing entity: nodrop\n");
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
				return;
			}
			if (trace.surfaceFlags & SURF_NOIMPACT) {
				//Com_Printf("freeing entity: noimpact\n");
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
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

				//if (trace.allsolid  ||  (trace.plane.normal[2] > 0  &&  ScriptVars.velocity[2] < 40)) {
				if (VectorLength(ScriptVars.velocity) < 20) {
					le->pos.trType = TR_STATIONARY;
					//re->customShader = cgs.media.connectionShader;
				}
			} else {
				//Com_Printf("freeing local ent\n");
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
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

		//Com_Printf("moving %f %f %f\n", ScriptVars.origin[0], ScriptVars.origin[1], ScriptVars.origin[2]);
		//FIXME trace, stop, gravity, bounce, distance
	}

	//R_AddRefEntityPtrToScene(&le->refEntity);

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
			R_AddRefEntityPtrToScene(re);
			return;
		}
	}

	if (cg.ftime - le->lastRunTime > cg_fxinterval.value) {  //  &&  cg.frametime < 9) {  //25) {  //(cg.time != le->lastRunTime) {  //(cg.time - le->lastRunTime > 100) {
	//if (rand() % 100 == 0) {
		DistanceScript = qfalse;  //qtrue;  //FIXME hack
		EmitterScript = qtrue;
		//Com_Printf("script: %s\n", le->emitterScript);
#if 0
		if (1) {  //(re->reType == RT_SPRITE) {
				//Com_Printf("script: %s\n", le->emitterScript);
			Com_Printf("^4script: (%f) '", ScriptVars.size);
			Q_PrintSubString(le->emitterScript, le->emitterScriptEnd);
			Com_Printf("'\n");
		}
#endif
        //Com_Printf("^1radius: %f\n", re->radius);

		if (cg_fxq3mmeCompatibility.integer) {
			CG_RunQ3mmeScript(le->emitterScript, NULL);
		} else {
			CG_RunQ3mmeScript(le->emitterScript, le->emitterScriptEnd);
		}

		EmitterScript = qfalse;
		DistanceScript = qfalse;
		le->lastRunTime = cg.ftime;

		//FIXME this is fucked up since emitter scripts can change origin
		//FIXME move* has to be a command...  fix
		if (ScriptVars.hasMoveBounce  ||  ScriptVars.hasMoveGravity) {
			VectorCopy(newOrigin, re->origin);
		} else {
			//no.... use re->origin
			// jujy
			VectorCopy(ScriptVars.origin, re->origin);
		}
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
		R_AddRefEntityPtrToScene(re);
		return;
	}


	//Com_Printf("shader: '%s'\n", ScriptVars.shader);
	if (ScriptVars.hasShaderTime) {
		re->shaderTime = ScriptVars.shaderTime;
	}

	//FIXME *scale *fade and move properties

	// kill sprite if view inside
	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		VectorSubtract(re->origin, cg.refdef.vieworg, delta);
		len = VectorLength(delta);
		if (len < ScriptVars.size) {
			// don't free just skip
			//Lock_EntList();
			//CG_FreeLocalEntity(le);
			//Unlock_EntList();
			//return;
		}
	}

	if (ScriptVars.hasAlphaFade) {
		totalFade = 1.0 - ScriptVars.alphaFade;
		lf = 1 - totalFade * ScriptVars.lerp;

		ScriptVars.color[3] *= lf;
	}

	if (ScriptVars.hasColorFade) {
		totalFade = 1.0 - ScriptVars.colorFade;
		lf = 1.0 - totalFade * ScriptVars.lerp;

		//Com_Printf("lf %f\n", lf);
		ScriptVars.color[0] *= lf;
		ScriptVars.color[1] *= lf;
		ScriptVars.color[2] *= lf;
	}
	re->shaderRGBA[0] = 255 * ScriptVars.color[0];
	re->shaderRGBA[1] = 255 * ScriptVars.color[1];
	re->shaderRGBA[2] = 255 * ScriptVars.color[2];
	re->shaderRGBA[3] = 255 * ScriptVars.color[3];
	//Com_Printf("shader: '%d'\n", re->customShader);
	if (*ScriptVars.shader) {
		//Com_Printf("loading shader '%s'\n", ScriptVars.shader);
		//re->customShader = trap_R_RegisterShader(ScriptVars.shader);
	} else {
		//re->customShader = 0;  //FIXME masking a bug
	}
	//re->customShader = 0;

	if (re->reType == RT_SPRITE  ||  re->reType == RT_SPRITE_FIXED  ||  re->reType == RT_SPARK) {
		if (re->reType == RT_SPARK) {  //(ScriptVars.width) {
			VectorCopy(ScriptVars.velocity, re->oldorigin);
			re->width = ScriptVars.width;
		} else if (re->reType == RT_SPRITE_FIXED) {
			VectorCopy(ScriptVars.dir, re->oldorigin);
			re->width = 0;
		}
	} else if (re->reType == RT_BEAM_Q3MME) {  //  ||  re->reType == RT_RAIL_RINGS_Q3MME) {
		VectorCopy(ScriptVars.dir, re->oldorigin);
		//VectorCopy(ScriptVars.dir, re->oldorigin);
	} else if (re->reType == RT_RAIL_RINGS_Q3MME) {
		//VectorCopy(ScriptVars.end, re->oldorigin);
		VectorCopy(ScriptVars.dir, re->oldorigin);
	}

	re->radius = ScriptVars.size;
	//Com_Printf("size %f\n", re->radius);
	//FIXME hack
	if (re->reType == RT_RAIL_RINGS_Q3MME) {
		re->rotation = ScriptVars.width;
	} else {
		re->rotation = ScriptVars.rotate;
		//FIXME check
		//re->rotation = ScriptVars.angle;
	}
	//Com_Printf("rotation %f\n", re->rotation);
	//FIXME add velocity or not?
	//VectorCopy(ScriptVars.velocity, le->pos.trDelta);

	//FIXME for models:

#if 0  //FIXME maybe?
	if (IsFxModel(re->reType)  &&  VectorNormalize2(ScriptVars.velocity, re->axis[0]) == 0) {
		Com_Printf("^3yes2... model\n");
		re->axis[0][2] = 1;
	}
#endif

#if 0
	//FIXME duplicate code... no, sort of
	if (re->reType == RT_MODEL_FX_DIR  ||  re->reType == RT_MODEL_FX_ANGLES) {
		vec3_t ang;

		//if (VectorLength(ScriptVars.velocity)) {
		if (VectorLength(le->sv.velocity)) {
			if (re->reType == RT_MODEL_FX_DIR) {
				vectoangles(ScriptVars.dir, ang);
				AnglesToAxis(ang, re->axis);
			} else if (re->reType == RT_MODEL_FX_ANGLES) {
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
	} else if (re->reType == RT_MODEL_AXIS) {
		VectorCopy(ScriptVars.axis[0], re->axis[0]);
		VectorCopy(ScriptVars.axis[1], re->axis[1]);
		VectorCopy(ScriptVars.axis[2], re->axis[2]);
	}
#endif

#if 0
	if (IsFxModel(re->reType)) {
		if (re->rotation) {
			//FIXME duplicate code
			//FIXME stationary check
			Com_Printf("rotation: %f\n", re->rotation);
			RotateAroundDirection(re->axis, re->rotation);
		}

#if 0
		if (1) {  //(re->radius != 1) {
			VectorScale(re->axis[0], re->radius, re->axis[0]);
			VectorScale(re->axis[1], re->radius, re->axis[1]);
			VectorScale(re->axis[2], re->radius, re->axis[2]);
			re->nonNormalizedAxes = qtrue;
        } else {
			//re->nonNormalizedAxes = qfalse;
		}
#endif
	}
#endif

	//FIXME save origin here for non-move types?

	// use ScriptVars incase it was reset by script system
	le->sv.distance = ScriptVars.distance;
	le->sv.impacted = ScriptVars.impacted;
	//VectorCopy(ScriptVars.origin, le->sv.origin);
	VectorCopy(ScriptVars.lastIntervalPosition, le->sv.lastIntervalPosition);
	le->sv.lastIntervalTime = ScriptVars.lastIntervalTime;
	VectorCopy(ScriptVars.lastDistancePosition, le->sv.lastDistancePosition);
	le->sv.lastDistanceTime = ScriptVars.lastDistanceTime;
	//FIXME distance and radius culling

	//Com_Printf("origin: %f %f %f\n", re->origin[0], re->origin[1], re->origin[2]);
    //Com_Printf("adding sprite SV %f  radius %f\n", ScriptVars.size, re->radius);

	R_AddRefEntityPtrToScene(re);
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

#if 0
	Com_Printf("^3light script: (%f) '", ScriptVars.size);
	Q_PrintSubString(le->emitterScript, le->emitterScriptEnd);
	Com_Printf("'\n");
#endif

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
		//FIXME duplicate code
		if (0) {  //(ScriptVars.parentCent) {
			trap_S_AddLoopingSound(ScriptVars.parentCent->currentState.number, ScriptVars.origin, ScriptVars.velocity, lsound);
		} else {
			if (FxLoopSounds >= MAX_LOOP_SOUNDS) {
				CG_Printf("^3fx:  (emitted) max loop sounds added\n");
			} else {
				trap_S_AddLoopingSound(FxLoopSounds, ScriptVars.origin, ScriptVars.velocity, lsound);
				FxLoopSounds++;
			}
		}

	}
}

//==============================================================================


static void CG_AddLocalEntitiesExt (int threadNum)
{
	localEntity_t	*le;  // *next;
	int count;
	double ourTime;
	int handled;

	//Lock_Global();

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	//le = cg_activeLocalEntities.prev;
	Lock_Next();
	le = Next[threadNum];
	Unlock_Next();

	handled = 0;
	count = 0;
	//for ( ; le != &cg_activeLocalEntities ; le = next ) {
	//for ( ; le != &cg_activeLocalEntities ; le = Next[threadNum], count++ ) {
	for ( ;  le != &cg_activeLocalEntities;  count++ ) {
	//while (le != &cg_activeLocalEntities) {
	//count++;

		// grab next now, so if the local entity is freed we
		// still have it

		if (count > 1) {
			Lock_Next();
			le = Next[threadNum];
			Unlock_Next();
			if (le == &cg_activeLocalEntities) {
				break;
			}
		}

		Lock_Next();
		if (le) {
			Next[threadNum] = le->prev;
		} else {
			Next[threadNum] = NULL;
			Unlock_Next();
			break;
		}
		Unlock_Next();

		Lock_EntHandled();

		if (1) {  //(threadNum != 0) {
			// threads
			if (le->frameCountHandled == FrameCount) {
				// already handled
				//Com_Printf("handled %p\n", le);
				Unlock_EntHandled();
				continue;
			}
		}

		le->frameCountHandled = FrameCount;
		Unlock_EntHandled();

		handled++;

		//Com_Printf("add ent thread %d  %p\n", threadNum, le);


		if (le->leFlags & LEF_ALREADY_ADDED) {
			//Lock_EntList();
			//CG_FreeLocalEntity(le);
			//Unlock_EntList();
			//R_AddRefEntityPtrToScene(&le->refEntity);
			continue;
		} else if (le->leFlags & LEF_ALREADY_ADDED_FX) {
			Lock_SysCall();
			R_AddRefEntityPtrToScene(&le->refEntity);
			Unlock_SysCall();
			continue;
		}

		if (le->leFlags & LEF_REAL_TIME) {
			ourTime = cg.realTime;
		} else {
			ourTime = cg.ftime;
		}


		if (!le->fxType) {
			if (ourTime >= le->endTime) {
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
				//Unlock_Global();
				continue;
			}
		} else {
			if (le->sv.emitterFullLerp  &&  le->sv.emitterKill) {
				//Com_Printf("killing emitter full %p\n", le);
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
				//Unlock_Global();
				continue;
			} else if (le->sv.emitterFullLerp  &&  ourTime >= le->endTime) {
				//Com_Printf("going to kill emitter %p\n", le);
				le->sv.emitterKill = qtrue;
			} else if (ourTime >= le->endTime) {
				Lock_EntList();
				CG_FreeLocalEntity(le);
				Unlock_EntList();
				//Unlock_Global();
				continue;
			}
		}

		//FIXME check thread saftey
		switch (le->fxType) {
		case LEFX_EMIT:
			Lock_Global();
			CG_Add_FX_Emitted(le);
			Unlock_Global();
			continue;
		case LEFX_SCRIPT:
			Lock_Global();
			CG_Run_FX_Emitted_Script(le);
			Unlock_Global();
			continue;
		case LEFX_EMIT_LIGHT:
			Lock_Global();
			CG_Add_FX_Emitted_Light(le);
			Unlock_Global();
			continue;
		case LEFX_EMIT_SOUND:
			Lock_Global();
			CG_Add_FX_Emitted_Sound(le);
			Unlock_Global();
			continue;
		case LEFX_EMIT_LOOPSOUND:
			Lock_Global();
			CG_Add_FX_Emitted_LoopSound(le);
			Unlock_Global();
			continue;
		default:
			break;
		}

		Lock_Global();

		switch ( le->leType ) {
		default:
			Unlock_Global();
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

		case LE_HEADSHOTPLUM:
			CG_AddHeadShotPlum(le);
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
			CG_AddLocalRefEntity( le );
			break;
#endif
		}

		Unlock_Global();
	}

#ifdef THREAD_DEBUG
	Lock_SysCall();
	Com_Printf("thread %d handled %d\n", threadNum, handled);
	Unlock_SysCall();
#endif

	//Unlock_Global();
}

/*
===================
CG_AddLocalEntities

===================
*/

#if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

void CG_AddLocalEntities (void)
{
	int i;

	for (i = 0;  i < ARRAY_LEN(Next);  i++) {
		Next[i] = cg_activeLocalEntities.prev;
	}

	if (cg_fxThreads.integer > 0) {
		int numThreads;

		numThreads = cg_fxThreads.integer;
		if (numThreads > MAX_THREADS) {
			numThreads = MAX_THREADS;
		}
		if (numThreads > ARRAY_LEN(RunSem)) {
			numThreads = ARRAY_LEN(RunSem);
		}
		if (numThreads > ARRAY_LEN(StopSem)) {
			numThreads = ARRAY_LEN(StopSem);
		}

		Lock_Count();
		Increment_FrameCount();
		Unlock_Count();

		//Com_Printf("about to wake threads %d\n", FrameCount);

		// wake threads
		for (i = 0;  i < numThreads;  i++) {
			semaphore_post(&RunSem[i]);
		}

		//Com_Printf("main thread %d %f\n", count, cg.ftime);
		CG_AddLocalEntitiesExt(1);
		//Com_Printf("main thread %d done %f\n", count, cg.ftime);

		// wait for threads to finish
		for (i = 0;  i < numThreads;  i++) {
			semaphore_wait(&StopSem[i]);
		}

#ifdef THREAD_DEBUG
		Com_Printf("thread done %d %f\n", FrameCount, cg.ftime);
#endif
	} else {
		Increment_FrameCount();
		CG_AddLocalEntitiesExt(0);
	}
}

#else  // if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

void CG_AddLocalEntities (void)
{
	Next[0] = cg_activeLocalEntities.prev;
	Increment_FrameCount();
	CG_AddLocalEntitiesExt(0);
}

#endif  // #if !defined(Q3_VM)  &&  defined(ENABLE_THREADS)

void CG_ClearLocalFrameEntities (void)
{
	localEntity_t *le;
	localEntity_t *next;
	int count;

	//FIXME don't go through this list twice
	le = cg_activeLocalEntities.prev;
	if (!le) {  // local entities not initialized yet
		return;
	}

	count = 0;
	Lock_EntList();

	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if (le->leFlags & LEF_ALREADY_ADDED  ||  le->leFlags & LEF_ALREADY_ADDED_FX) {
			count++;
#if 0
			if (count > 300) {
				Com_Printf("^1count: %d\n", count);
			}
#endif
			//Lock_EntList();
			CG_FreeLocalEntity(le);
			//Unlock_EntList();
			continue;
		}

		//Com_Printf("next %p  active %p\n", next, &cg_activeLocalEntities);
	}

	Unlock_EntList();
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
			Lock_EntList();
			CG_FreeLocalEntity(le);
			Unlock_EntList();
			continue;
		}
	}
}

void CG_ListLocalEntities (void)
{
	const localEntity_t *le;
	const localEntity_t *next;
	int count;
	int scripts;
	int lights;
	int sounds;
	int loopSounds;
	int fxEmitted;

	count = 0;
	scripts = 0;
	fxEmitted = 0;
	lights = 0;
	sounds = 0;
	loopSounds = 0;

	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		count++;
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if (le->fxType == LEFX_EMIT) {
			fxEmitted++;
			if (le->refEntity.reType == RT_SPRITE) {
				Com_Printf("^3%5d ^7emitterId %f fxsprite timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.shader);
			} else if (le->refEntity.reType == RT_SPRITE_FIXED) {
				Com_Printf("^3%5d ^7emitterId %f fxsprite fixed timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.shader);
			} else if (le->refEntity.reType == RT_MODEL) {
				Com_Printf("^3%5d ^7emitterId %f fxmodel timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.model);
			} else if (le->refEntity.reType == RT_SPARK) {
				Com_Printf("^3%5d ^7emitterId %f fxspark timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.shader);
			} else if (le->refEntity.reType == RT_BEAM_Q3MME) {
				Com_Printf("^3%5d ^7emitterId %f fxbeamq3mme timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.shader);
			} else if (le->refEntity.reType == RT_RAIL_RINGS_Q3MME) {
				Com_Printf("^3%5d ^7emitterId %f fxrailringsq3mme timeleft: %f (%f) '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->sv.shader);
			} else {
				Com_Printf("^3%5d ^7emitterId %f fxunknown timeleft: %f (%f) '%d reType' shader '%s'\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime, le->refEntity.reType, le->sv.shader);
			}
		} else if (le->fxType == LEFX_SCRIPT) {
			Com_Printf("^3%5d ^7emitterId %f fxscript timeleft: %f (%f)\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime);
			scripts++;
		} else if (le->fxType == LEFX_EMIT_LIGHT) {
			lights++;
			Com_Printf("^3%5d ^7emitterId %f fxlight timeleft: %f (%f)\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime);
		} else if (le->fxType == LEFX_EMIT_SOUND) {
			sounds++;
			Com_Printf("^3%5d ^7emitterId %f fxsound timeleft: %f (%f)\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime);
		} else if (le->fxType == LEFX_EMIT_LOOPSOUND) {
			loopSounds++;
			Com_Printf("^3%5d ^7emitterId %f fxloopsound timeleft: %f (%f)\n", count, le->sv.emitterId, le->endTime - cg.ftime, le->endTime - le->startTime);
		} else if (le->leFlags == LEF_ALREADY_ADDED) {
			Com_Printf("^3%5d ^7cgame direct frame render\n", count);
		} else if (le->leFlags == LEF_ALREADY_ADDED_FX) {
			Com_Printf("^3%5d ^7fx direct frame render\n", count);
		}
	}

	Com_Printf("^5total %d   scripts %d   fxEmitted %d  lights %d  sounds %d  loopsounds %d\n", count, scripts, fxEmitted, lights, sounds, loopSounds);
}

void CG_AddRefEntity (const refEntity_t *re)
{
	localEntity_t *le;

	// Need to add it to local entity list in case the limit is reached
	// and the renderer drops it.

	le = CG_AllocLocalEntityExt(qfalse);
	le->endTime = 0;
	le->leFlags = LEF_ALREADY_ADDED;

	// testing
	// careful with this and cgame added entities since they need
	// to be rendered first come first serve, while addlocalentities()
	// renders newest to oldest
	//memcpy(&le->refEntity, re, sizeof(le->refEntity));

	trap_R_AddRefEntityToScene(re);
}

void CG_AddRefEntityFX (const refEntity_t *re)
{
	localEntity_t *le;

	// Need to add it to local entity list in case the limit is reached
	// and the renderer drops it.

	//le = CG_AllocLocalEntity();
	le = CG_AllocLocalEntityExt(qfalse);
	le->endTime = 0;
	le->leFlags = LEF_ALREADY_ADDED_FX;

	// can still be removed if needed
	memcpy(&le->refEntity, re, sizeof(le->refEntity));

	//trap_R_AddRefEntityToScene(re);
}

#if 0  //FIXME force
void CG_UpdateFxExternalForces (void)
{
	int i;

	for (i = 0;  i < MAX_FX_EXTERNAL_FORCES;  i++) {
		fxExternalForce_t *f;

		f = &FxExternalForces[i];
		if (!f->valid) {
			continue;
		}

		if (f->startTime - cg.time >= f->duration) {
			f->valid = qfalse;
			continue;
		}
	}
}

//FIXME force
void CG_AddFxExternalForce (fxExternalForce_t *force)
{
	int i;

	for (i = 0;  i < MAX_FX_EXTERNAL_FORCES;  i++) {
		fxExternalForce_t *f;

		f = &FxExternalForces[i];
		if (f->valid) {
			continue;
		}

		memcpy(f, force, sizeof(fxExternalForce_t));
	}
}

//FIXME force
void CG_ClearFxExternalForces (void)
{
	int i;

	for (i = 0;  i < MAX_FX_EXTERNAL_FORCES;  i++) {
		FxExternalForces[i].valid = qfalse;
	}
}
#endif

#ifndef cg_localents_h_included
#define cg_localents_h_included

#include "cg_fx_scripts.h"

//#define ENABLE_THREADS 1

// local entities are created as a result of events or predicted actions,
// and live independently from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	qboolean energy;
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
	LE_DAMAGEPLUM,
	LE_HEADSHOTPLUM,
#if 1  //def MPACK
	LE_KAMIKAZE,
	LE_INVULIMPACT,
	LE_INVULJUICED,
	LE_SHOWREFENTITY,
#endif
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008,			// sound 2 for kamikaze
	LEF_PUFF_DONT_SCALE_NOT_KIDDING_MAN = 0x0010, // do not scale size over time, and this time I mean it
	LEF_REAL_TIME = 0x0020,  // don't use game time for end/start/life time
	LEF_ALREADY_ADDED = 0x0040,  // trap_R_AddRefEntityToScene() already called for this so it can't be dropped, called by non fx code
	LEF_ALREADY_ADDED_FX = 0x0080,
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD,
	LEMT_ICE,
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS,
	LEBS_ICE,
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef enum {
	LEFX_NONE,  // don't use q3mme scripting
	LEFX_EMIT,  // has something to render
	LEFX_EMIT_LIGHT,
	LEFX_EMIT_SOUND,
	LEFX_EMIT_LOOPSOUND,
	LEFX_SCRIPT,  // just a script to run
} leFXType_t;

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	struct localEntity_s *lowPrev, *lowNext;
	leType_t		leType;
	int				leFlags;

	//int				startTime;
	//int				endTime;
	double startTime;
	double endTime;
	double				fadeInTime;

	double			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;
	double trTimef;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	refEntity_t		refEntity;

	leFXType_t fxType;
	//char emitterScript[1024 * 32];  //FIXME use pointer, this adds 16mb
	const char *emitterScript;
	const char *emitterScriptEnd;
	double lastRunTime;

	ScriptVars_t sv;
	qboolean lowPriority;

	// for threads
	int frameCountHandled;

} localEntity_t;

typedef struct fxForce_s {
	int startTime;
	int duration;
	vec3_t origin;
	vec3_t dir;
	qboolean radial;
	float power;
	qboolean valid;
} fxExternalForce_t;


//FIXME hack, loop sounds independent of an actual entity
#define FX_LOOP_SOUNDS_BASE (MAX_LOOP_SOUNDS / 2  + 1)

extern int FxLoopSounds;

void CG_InitLocalEntities( void );
localEntity_t *CG_AllocLocalEntity( void );
localEntity_t *CG_AllocLocalEntityRealTime( void );
void CG_MakeLowPriorityEntity (localEntity_t *le);
void CG_AddLocalEntities( void );
void CG_ClearLocalFrameEntities (void);
void CG_RemoveFXLocalEntities (qboolean all, float emitterId);
void CG_ListLocalEntities (void);
void CG_AddRefEntity (const refEntity_t *re);
void CG_AddRefEntityFX (const refEntity_t *re);

localEntity_t *CG_SmokePuff_ql( const vec3_t p, const vec3_t vel,
								float radius,
								float r, float g, float b, float a,
								float duration,
								int startTime,
								int fadeInTime,
								int leFlags,
								qhandle_t hShader );

void CG_RunFxAll (const char *name);

///////////////////////////////////////////////////
//FIXME forward declarations

void CG_GetStoredScriptVarsFromLE (const localEntity_t *le);
void CG_ShutdownLocalEnts (qboolean destructor);
void CG_UpdateFxExternalForces (void);
void CG_AddFxExternalForce (fxExternalForce_t *force);
void CG_ClearFxExternalForces (void);
void CG_HideDamagePlums (int client, int weapon);

#endif  // cg_localents_h_included

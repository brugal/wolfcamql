// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
#include "sc.h"
#include "../qcommon/qfiles.h"  // MAX_MAP_ADVERTISEMENTS
#include "../ui/ui_shared.h"
#include "cg_camera.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define DEFAULT_FOV 110

#define MAX_RAWCAMERAPOINTS 32768  //4192
#define MAX_TIMED_ITEMS 16   //FIXME
#define MAX_CHAT_LINES  100
#define MAX_CHAT_LINES_MASK (MAX_CHAT_LINES - 1)
#define MAX_SPAWN_POINTS 128  //FIXME from g_client.c
#define MAX_MIRROR_SURFACES MAX_GENTITIES
#define DEFAULT_RECORD_PATHNAME "path"

//FIXME from game/g_local.h
#define       MAX_SPAWN_VARS                  64
#define       MAX_SPAWN_VARS_CHARS    4096

#if 1  //def MPACK
#define CG_FONT_THRESHOLD 0.1
#endif

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
//#define	STEP_TIME			100  //200  // cg_stepSmoothTime
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
//#define	ZOOM_TIME			150
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
//#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

//#define	MAX_STEP_CHANGE		32  // cg_stepSmoothMaxChange

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

//#define	NUM_CROSSHAIRS		20  in ui_shared.h

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define	DEFAULT_MODEL			"sarge"
#ifdef MISSIONPACK
#define	DEFAULT_TEAM_MODEL		"james"
#define	DEFAULT_TEAM_HEAD		"*james"
#else
#define	DEFAULT_TEAM_MODEL		"sarge"
#define	DEFAULT_TEAM_HEAD		"sarge"
#endif

#define MAX_EVENT_FILTER 1024
#define ICON_SCALE_DISTANCE 200.0

//FIXME from menudef.h

#define ITEM_TEXTSTYLE_NORMAL 0
#define ITEM_TEXTSTYLE_BLINK 1
#define ITEM_TEXTSTYLE_PULSE 2
#define ITEM_TEXTSTYLE_SHADOWED 3
#define ITEM_TEXTSTYLE_OUTLINED 4
#define ITEM_TEXTSTYLE_OUTLINESHADOWED 5
#define ITEM_TEXTSTYLE_SHADOWEDMORE 6

#define LAG_SAMPLES		128

typedef struct {
	int             frameSamples[LAG_SAMPLES];
	int             frameCount;
	int             snapshotFlags[LAG_SAMPLES];
	int             snapshotSamples[LAG_SAMPLES];
	int             snapshotCount;
} lagometer_t;

extern lagometer_t  lagometer;

extern char *gametypeConfigs[];

extern vec3_t g_color_table_ql_0_1_0_303[];

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_WOOD,
	FOOTSTEP_SNOW,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;


typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

	// railgun trail spawning
	vec3_t			railgunImpact;
	qboolean		railgunFlash;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;
	int 			lgSoundTime;
	int				muzzleFlashTime;
} playerEntity_t;

//=================================================



// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity
	qboolean inCurrentSnapshot;
	qboolean inNextSnapshot;

	//int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets

	// q3mme fx scripting
	vec3_t lastFlashIntervalPosition;  // also player/legs
	double lastFlashIntervalTime;
	vec3_t lastFlashDistancePosition;
	double lastFlashDistanceTime;

	vec3_t lastModelIntervalPosition;  // also powerups
	double lastModelIntervalTime;
	vec3_t lastModelDistancePosition;
	double lastModelDistanceTime;

	vec3_t lastTrailIntervalPosition;  // also player/head
	double lastTrailIntervalTime;
	vec3_t lastTrailDistancePosition;
	double lastTrailDistanceTime;

	vec3_t lastImpactIntervalPosition;  // also player/torso
	double lastImpactIntervalTime;
	vec3_t lastImpactDistancePosition;
	double lastImpactDistanceTime;

	int				dustTrailTime;
	int				miscTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;

	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
	entityState_t	oldState;
	int				serverTimeOffset;  // when rolling back positions
	int spawnArmorTime;
	int cgtime;
	qboolean wasReset;

	int lastLgFireFrameCount;
	int lastLgImpactFrameCount;
	double lastLgImpactTime;
	vec3_t lastLgImpactPosition;
} centity_t;

#define MAX_SCRIPTED_VARS_VECTOR_STACK 64
#define MAX_FX_SOUND_LIST 32
#define MAX_FX_MODEL_LIST 32
#define MAX_FX_SHADER_LIST 32

typedef struct ScriptVars_s {
    float size;
    float width;
    float angle;
    float t0;
    float t1;
    float t2;
    float t3;

	// extra
	float t4;
	float t5;
	float t6;
	float t7;
	float t8;
	float t9;

	float rotate;
	float vibrate;

    vec3_t origin;
	vec3_t end;
    vec3_t velocity;
    vec3_t dir;
    vec3_t angles;
	vec3_t axis[3];

    float time;
    float loop;
	int loopCount;
    //float red;
    //float green;
    //float blue;
    //float alpha;
    float rand;
    float crand;
    float lerp;
    float life;

    vec3_t v0, v1, v2, v3;

	// extra
	vec3_t v4, v5, v6, v7, v8, v9;

	vec3_t tmpVector;

	vec4_t color;

	vec3_t color1;
	vec3_t color2;

	int team;  // 0 free, 1 red, 2 blue, 3 spec
	int clientNum;
	qboolean enemy;
	qboolean teamMate;
	qboolean inEyes;
	int surfaceType;  // 0 not specified, 1 metal, 2 wood
#if 0
	qboolean rewardImpressive;
	qboolean rewardExcellent;
	qboolean rewardHumiliation;
	qboolean rewardDefend;
	qboolean rewardAssist;
#endif

	//qhandle_t shader;
	char shader[MAX_QPATH];
	//qhandle_t model;
	char model[MAX_QPATH];
	char loopSound[MAX_QPATH];
	char sound[MAX_QPATH];

	vec3_t parentOrigin;
	vec3_t parentVelocity;
	vec3_t parentAngles;
	vec3_t parentDir;
	vec3_t parentEnd;
	int parentSize;
	float parentAngle;

	vec3_t vecStack[MAX_SCRIPTED_VARS_VECTOR_STACK];
	int vecStackCount;

	qboolean hasAlphaFade;
	float alphaFade;
	qboolean hasColorFade;
	float colorFade;
	qboolean hasMoveGravity;
	float moveGravity;

	// render options
	qboolean firstPerson;
	qboolean thirdPerson;
	qboolean shadow;
	qboolean cullNear;
	qboolean cullRadius;
	qboolean depthHack;
	qboolean stencil;  //FIXME check

	qboolean decalEnergy;
	qboolean decalAlpha;

	char soundList[MAX_FX_SOUND_LIST][MAX_QPATH];
	int numSoundList;
	int selectedSoundList;

	char modelList[MAX_FX_MODEL_LIST][MAX_QPATH];
	int numModelList;
	int selectedModelList;

	char shaderList[MAX_FX_SHADER_LIST][MAX_QPATH];
	int numShaderList;
	int selectedShaderList;

	qboolean hasMoveBounce;
	float moveBounce1;
	float moveBounce2;
	qboolean hasSink;
	float sink1;
	float sink2;
	float emitterTime;
	qboolean emitterFullLerp;  // lerp goes from 0.0 to 1.0 and includes 1.0 as last pass
	qboolean emitterKill;

	float distance;

	//int entNumber;
	//entityState_t currentState;
	qboolean spin;
	vec3_t lastIntervalPosition;
	double lastIntervalTime;
	vec3_t lastDistancePosition;
	double lastDistanceTime;
	//centity_t *cent;  // idiot
	char *emitterScriptStart;
	char *emitterScriptEnd;

	vec3_t impactOrigin;
	vec3_t impactDir;
	qboolean impacted;

	float emitterId;

} ScriptVars_t;

extern ScriptVars_t ScriptVars;
extern ScriptVars_t EmitterScriptVars;
extern qboolean EmitterScript;
extern qboolean DistanceScript;
extern qboolean PlainScript;  //FIXME hack

#define MAX_FX_SCRIPT_SIZE  (1024 * 32)
#define MAX_FX_EXTRA 128
#define MAX_FX 256

typedef struct weaponEffects_s {
	qboolean hasFireScript;
	qboolean hasFlashScript;
	qboolean hasProjectileScript;
	qboolean hasTrailScript;
	qboolean hasImpactScript;
	qboolean hasImpactFleshScript;

	char fireScript[MAX_FX_SCRIPT_SIZE];
	char flashScript[MAX_FX_SCRIPT_SIZE];
	char projectileScript[MAX_FX_SCRIPT_SIZE];
	char trailScript[MAX_FX_SCRIPT_SIZE];
	char impactScript[MAX_FX_SCRIPT_SIZE];
	char impactFleshScript[MAX_FX_SCRIPT_SIZE];
} weaponEffects_t;


typedef struct effectScripts_s {
	weaponEffects_t weapons[WP_NUM_WEAPONS];  //FIXME oops -- devmap with weapon not included

	char playerTalk[MAX_FX_SCRIPT_SIZE];
	char playerConnection[MAX_FX_SCRIPT_SIZE];
	char playerImpressive[MAX_FX_SCRIPT_SIZE];
	char playerExcellent[MAX_FX_SCRIPT_SIZE];
	char playerHolyshit[MAX_FX_SCRIPT_SIZE];
	char playerAccuracy[MAX_FX_SCRIPT_SIZE];
	char playerGauntlet[MAX_FX_SCRIPT_SIZE];
	char playerHaste[MAX_FX_SCRIPT_SIZE];
	char playerDefend[MAX_FX_SCRIPT_SIZE];
	char playerAssist[MAX_FX_SCRIPT_SIZE];
	char playerCapture[MAX_FX_SCRIPT_SIZE];
	char playerQuad[MAX_FX_SCRIPT_SIZE];
	char playerFlight[MAX_FX_SCRIPT_SIZE];
	char playerFriend[MAX_FX_SCRIPT_SIZE];
	char playerHeadTrail[MAX_FX_SCRIPT_SIZE];
	char playerTorsoTrail[MAX_FX_SCRIPT_SIZE];
	char playerLegsTrail[MAX_FX_SCRIPT_SIZE];

	char playerTeleportIn[MAX_FX_SCRIPT_SIZE];
	char playerTeleportOut[MAX_FX_SCRIPT_SIZE];

	char jumpPad[MAX_FX_SCRIPT_SIZE];

	char bubbles[MAX_FX_SCRIPT_SIZE];  //FIXME pointer
	char gibbed[MAX_FX_SCRIPT_SIZE];
	char thawed[MAX_FX_SCRIPT_SIZE];
	char impactFlesh[MAX_FX_SCRIPT_SIZE];

	//FIXME ugly
	char extra[MAX_FX_SCRIPT_SIZE][MAX_FX_EXTRA];
	//char extraNames[MAX_QPATH][MAX_FX_EXTRA];
	int numExtra;

	char names[MAX_QPATH][MAX_FX];
	char *ptr[MAX_FX];
	int numEffects;

} effectScripts_t;

extern effectScripts_t EffectScripts;

//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

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
	char *emitterScript;
	char *emitterScriptEnd;
	double lastRunTime;

	ScriptVars_t sv;
} localEntity_t;

//======================================================================

typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerups;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	int 		perfect;
	int				team;

	qboolean		alive;
	int				frags;
	int				deaths;
	int				bestWeapon;

	int net;

	//qboolean		fake;  // filled in for demo playback
} score_t;

typedef struct {
	int hits;
	int atts;
	int accuracy;
	int damage;
	int kills;
} duelWeaponStats_t;

typedef struct {

	int ping;  // 1
	int kills;  // 2
	int deaths;  // 3
	int accuracy;  // 4
	int damage;  // 5

	int redArmorPickups;  // 11
	float redArmorTime;  // 12
	int yellowArmorPickups;  // 13
	float yellowArmorTime;  // 14
	int greenArmorPickups;  // 15
	float greenArmorTime;  // 16
	int megaHealthPickups;  // 17
	float megaHealthTime;  // 18

	int awardExcellent;
	int awardImpressive;
	int awardHumiliation;

	// 24
	duelWeaponStats_t weaponStats[WP_NUM_WEAPONS];
} duelScore_t;

typedef struct {
	qboolean valid;

	int clientNum;
	int team;
	int subscriber;
	int tks;
	int tkd;
	int damageDone;

	int accuracy;

} tdmPlayerScore_t;

typedef struct {
	qboolean valid;
	int version;

	int rra;  // ra pickups
	int rya;  // ya
	int rga;  // green
	int rmh;  // mega
	int rquad;  // quad
	int rbs;  // battle suit
	int rregen;
	int rhaste;
	int rinvis;

	int rquadTime;  // quad possesion time
	int rbsTime;
	int rregenTime;
	int rhasteTime;
	int rinvisTime;

	int bra;  // ra
	int bya;  // yellow
	int bga;  // green
	int bmh;  // mega
	int bquad;  // quad
	int bbs;  // battle suit
	int bregen;
	int bhaste;
	int binvis;

	int bquadTime;  // quad possesion time
	int bbsTime;  // battle suit possesion time
	int bregenTime;
	int bhasteTime;
	int binvisTime;

	tdmPlayerScore_t playerScore[MAX_CLIENTS];

} tdmScore_t;

typedef struct {
	qboolean valid;

	int clientNum;
	int damageDone;
	int damageReceived;

	int gauntKills;
	int mgAccuracy;
	int mgKills;
	int sgAccuracy;
	int sgKills;
	int glAccuracy;
	int glKills;
	int rlAccuracy;
	int rlKills;
	int lgAccuracy;
	int lgKills;
	int rgAccuracy;
	int rgKills;
	int pgAccuracy;
	int pgKills;
} caStats_t;

typedef struct {
	qboolean valid;

	int clientNum;
	int selfKill;
	int tks;
	int tkd;
	int damageDone;
	int damageReceived;
	int ra;
	int ya;
	int ga;
	int mh;
	int quad;
	int bs;

	// per weapon stats?
	int u14;
	// ...
	int u82;
} tdmStats_t;

typedef struct {
	qboolean valid;

	int clientNum;
	int team;
	int subscriber;
} ctfPlayerScore_t;

typedef struct {
	qboolean valid;
	int version;

	int rra;  // ra pickups
	int rya;  // ya
	int rga;  // green
	int rmh;  // mega
	int rquad;  // quad
	int rbs;  // battle suit
	int rregen;
	int rhaste;
	int rinvis;
	int rflag;
	int rmedkit;

	int rquadTime;  // quad possesion time
	int rbsTime;
	int rregenTime;
	int rhasteTime;
	int rinvisTime;
	int rflagTime;

	// 18
	int bra;  // ra
	int bya;  // yellow
	int bga;  // green
	int bmh;  // mega
	int bquad;  // quad
	int bbs;  // battle suit
	int bregen;
	int bhaste;
	int binvis;
	int bflag;
	int bmedkit;

	int bquadTime;  // quad possesion time
	int bbsTime;  // battle suit
	int bregenTime;
	int bhasteTime;
	int binvisTime;
	int bflagTime;

	// 35
	ctfPlayerScore_t playerScore[MAX_CLIENTS];
} ctfScore_t;

typedef struct {
	qboolean valid;

	int clientNum;  // 1
	int selfKill;
	int damageDone; // 3
	int damageReceived; // 4
	int ra;  // 5
	int ya;  // 6
	int ga;  // 7
	int mh;  // 8
	int quad;  // 9
	int bs;  // 10
	int regen;  // 11
	int haste;  // 12
	int invis;  // 13
} ctfStats_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	32

typedef struct {
	qboolean		infoValid;

	char			name[MAX_QPATH * 2];
	char			whiteName[MAX_QPATH];  // no color codes
	team_t			team;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	vec3_t			color1;
	vec3_t			color2;

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;
	int				invulnerabilityStartTime;
	int				invulnerabilityStopTime;

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
	char			headModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
	//FIXME hack, take out, just use torsoSkin and legsSkin
	char torsoSkinName[MAX_QPATH];
	char legsSkinName[MAX_QPATH];
	char			redTeam[MAX_TEAMNAME];
	char			blueTeam[MAX_TEAMNAME];
	qboolean		deferred;

	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model

	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	qhandle_t		legsSkinAlt;
	qhandle_t		legsEnemySkinAlt;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	qhandle_t		torsoSkinAlt;
	qhandle_t		torsoEnemySkinAlt;

	qhandle_t		headModel;
	qhandle_t		headSkin;
	qhandle_t		headSkinAlt;
	qhandle_t		headEnemySkinAlt;

	qhandle_t headModelFallback;
	qhandle_t headSkinFallback;

	qhandle_t		modelIcon;

	animation_t		animations[MAX_TOTALANIMATIONS];

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	qboolean knowSkillRating;
	int	skillRating;

	// client override
	qboolean override;

	qboolean hasHeadColor;
	byte headColor[4];
	qboolean hasTorsoColor;
	byte torsoColor[4];
	qboolean hasLegsColor;
	byte legsColor[4];

	qboolean hasHeadSkin;
	qboolean hasTorsoSkin;
	qboolean hasLegsSkin;

	qboolean hasModelScale;
	qboolean hasLegsModelScale;
	qboolean hasTorsoModelScale;
	qboolean hasHeadModelScale;
	qboolean hasHeadOffset;
	qboolean hasModelAutoScale;

	float modelScale;
	float legsModelScale;
	float torsoModelScale;
	float headModelScale;
	float oheadOffset;
	float modelAutoScale;

	// end override

	qhandle_t countryFlag;
	char clanTag[MAX_QPATH * 2];
	char whiteClanTag[MAX_QPATH];

	qboolean premiumSubscriber;
	float playerModelHeight;

} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	void			(*ejectBrassFunc)( centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
	qboolean		loopFireSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		10

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;

typedef struct {
	vec3_t origin;
	//byte color[4];
	vec3_t angles;
} rawCameraPathKeyPoint_t;

typedef struct {
	vec3_t origin;
	int respawnLength;
	int pickupTime;
	int specPickupTime;
	int respawnTime;
	int clientNum;

	// cpm mega health
	int countDownTrigger;
} timedItem_t;

typedef struct {
	vec3_t origin;
	int angle;
	int spawnflags;
	qboolean redSpawn;
	qboolean blueSpawn;
	qboolean initial;
	qboolean deathmatch;
} spawnPoint_t;

typedef struct {
	char killer[MAX_STRING_CHARS];
	char killerWhiteName[MAX_STRING_CHARS];
	int killerClientNum;
	int killerTeam;
	char victim[MAX_STRING_CHARS];
	char victimWhiteName[MAX_STRING_CHARS];
	int victimClientNum;
	int victimTeam;
	int weapon;
	qhandle_t icon;
	int time;
	char q3obitString[MAX_STRING_CHARS];
} obituary_t;

typedef struct {
	qboolean general;
	qboolean player;
	qboolean item;  //FIXME types
	qboolean missile;
	qboolean mover;
	qboolean beam;
	qboolean portal;
	qboolean speaker;
	qboolean pushTrigger;
	qboolean teleportTrigger;
	qboolean invisible;
	qboolean grapple;
	qboolean team;
	qboolean events;
} filterTypes_t;

#define MAX_JUMPS_INFO 256
#define MAX_JUMPS_INFO_MASK (MAX_JUMPS_INFO - 1)

typedef struct {
	int speed;
	int time;
} jumpInfo_t;

#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

#define MAX_CVAR_INTERP 32

typedef struct {
	qboolean valid;
	qboolean realTime;  // real time, or game time (clock)
	double startTime;
	double endTime;
	float startValue;
	float endValue;
	char cvar[MAX_STRING_CHARS];
} cvarInterp_t;

#define MAX_AT_COMMANDS 128

typedef struct {
	qboolean valid;
	double ftime;
	char command[MAX_STRING_CHARS];

	double lastCheckedTime;
} atCommand_t;

typedef struct {
	trajectory_t pos;
	vec3_t angles;  // hack for prox mines
	int weapon;
	int clientNum;
	int numUses;
} projectile_t;

#define MAX_FLOAT_NUMBERS 1024

typedef struct {
	int number;
	vec3_t origin;
	int renderfx;
	byte color[4];
	int time;
	int startTime;
} floatNumber_t;

#define MAX_POI_PICS 128

typedef struct {
	vec3_t origin;
	int startTime;
	int length;
	int team;
	qhandle_t shader;
} poiPic_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_CTFS_ROUND_SCORES 64

#define MAX_PREDICTED_EVENTS	16

typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;

	qboolean	paused;
	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*prevSnap;
	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t *nextNextSnap;
	snapshot_t nnSnap;
	qboolean nextNextSnapValid;
	qboolean noMove;

	snapshot_t	activeSnapshots[2];

	double		frameInterpolation;	// (cg.ftime - (double)cg.snap->serverTime ) / (double)(cg.nextSnap->serverTime - cg.snap->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	double ftime;  // cg.time + (cg.foverf / 1000.0)
	int			oldTime;		// time at last frame, used for missile trails and prediction checking
	int oldSnapNum;

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time
	int			realTime;
	int			snapshotOffset;

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	// zoom key
	qboolean	zoomed;
	double		zoomTime;
	double zoomRealTime;
	double zoomBrokenTime;
	double zoomBrokenRealTime;
	float		zoomSensitivity;

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	int avgRedPing;
	int avgBluePing;
	score_t		scores[MAX_CLIENTS];
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	menuDef_t *menuScoreboard;
	char		killerName[MAX_NAME_LENGTH];
	qhandle_t killerWeaponIcon;
	int killerClientNum;
	vec3_t killerOrigin;
	vec3_t deadAngles;
	char			spectatorList[MAX_STRING_CHARS];		// list of names
	int				spectatorLen;												// length of list
	float			spectatorWidth;											// width in device units
	int				spectatorTime;											// next time to offset
	int				spectatorPaintX;										// current paint x
	int				spectatorPaintX2;										// current paint x
	int				spectatorOffset;										// current offset from start
	int				spectatorOffsetWidth;
	int				spectatorPaintLen; 									// current offset from start
	char			spectatorListCurrentColor;

	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];

	// centerprinting
	int			centerPrintTime;
	int			centerPrintCharWidth;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;
	qboolean	centerPrintIsFragMessage;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

#ifdef MISSIONPACK
	// for voice chat buffer
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;
#endif

	// warmup countdown
	int			warmup;
	int			warmupCount;

	int			matchRestartServerTime;

	//==========================

	int			itemPickup;
	int			itemPickupTime;
	int 		itemPickupClockTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed separately
	int itemPickupCount;

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int     nextOrbitTime;

	qboolean cameraMode;		// if rendering from a loaded camera


	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	clientInfo_t	enemyModel;
	clientInfo_t	teamModel;
	clientInfo_t	teamModelRed;
	clientInfo_t	teamModelBlue;
	clientInfo_t	ourModel;
	clientInfo_t serverModel;
	qboolean		useTeamSkins;

	//byte			enemyColors[4][3];
	byte			teamColors[4][4];

	//byte			deadBodyColor[4];
	//byte			grenadeColor[3];
	//vec4_t			crosshairColor;

	int				avgPing;
	int				pingCalculateTime;
	qboolean		showDemoScores;

	qboolean		spawning;  // CG_Spawn*() functions are valid
	int				numSpawnVars;
	char			*spawnVars[MAX_SPAWN_VARS][2];  // key /value pair
	int				numSpawnVarChars;
	char			spawnVarChars[MAX_SPAWN_VARS_CHARS];

	//#define MAX_TIMED_ITEMS 16   //FIXME
	int				numRedArmors;
	timedItem_t		redArmors[MAX_TIMED_ITEMS];
	int				numYellowArmors;
	timedItem_t		yellowArmors[MAX_TIMED_ITEMS];
	int	numGreenArmors;
	timedItem_t greenArmors[MAX_TIMED_ITEMS];
	int				numMegaHealths;
	timedItem_t		megaHealths[MAX_TIMED_ITEMS];
	int				numQuads;
	timedItem_t		quads[MAX_TIMED_ITEMS];
	int				numBattleSuits;
	timedItem_t		battleSuits[MAX_TIMED_ITEMS];
	int maxItemRespawnTime;

	int				numSpawnPoints;
	spawnPoint_t	spawnPoints[MAX_SPAWN_POINTS];

	int numMirrorSurfaces;
	vec3_t mirrorSurfaces[MAX_MIRROR_SURFACES];

	int				lastChatBeepTime;
	int				lastTeamChatBeepTime;

	obituary_t		lastObituary;

	// chat area has additional info besides just chat:  'player connected', etc..
	char			chatAreaStrings[MAX_CHAT_LINES][MAX_STRING_CHARS];
	int				chatAreaStringsTime[MAX_CHAT_LINES];
	int	chatAreaStringsIndex;

	char			lastFragString[MAX_STRING_CHARS];
	int				lastFragTime;
	int				lastFragVictim;
	int				lastFragKiller;
	char			lastFragVictimName[MAX_QPATH * 2];
	char			lastFragVictimWhiteName[MAX_QPATH * 2];
	int				lastFragVictimTeam;
	int				lastFragWeapon;
	int				lastFragMod;
	char			lastFragq3obitString[MAX_STRING_CHARS];

	int				shotgunHits;
	int				powerupRespawnSoundIndex;  // from config string
	int				kamikazeRespawnSoundIndex;
	int proxTickSoundIndex;

	// freecam
	qboolean		freecam;
	qboolean freecamAnglesSet;
	vec3_t			fpos;
	vec3_t			fang;
	vec3_t			fvelocity;
	qboolean freecamSet;  // at least one switch to freecam so fpos an fang valid
	int				mousex;
	int				mousey;
	int				mouseButton2;
	int				mouseButton1;
	int				keyu;
	int				keyd;
	int				keyf;
	int				keyb;
	int				keyr;
	int				keyl;
	int				keya;
	int				keyrollright;
	int keyrollleft;
	int keyrollstopzero;

	int				fMoveTime;
	playerState_t	freecamPlayerState;
	double positionSetTime;

	int				freecamFireTime;
	qboolean		recordPath;
	qhandle_t		recordPathFile;
	int	recordPathLastServerTime;
	qboolean		playPath;
	//int				playPathSkipNum;
	//qboolean		playPathStarted;
	//int	playPathCommandStartServerTime;
	int pathServerTimeStart;
	int pathRealTimeStart;
	float pathTimescale;
	int pathCGTimeStart;
	double pathCurrentTime;
	vec3_t pathCurrentOrigin;
	vec3_t pathCurrentAngles;
	double pathNextTime;
	vec3_t pathNextOrigin;
	vec3_t pathNextAngles;
	int playPathCount;

	qboolean	drawAccStats;
	int serverAccuracyStatsTime;
	int serverAccuracyStatsClientNum;
	int	serverAccuracyStats[MAX_WEAPONS];

	int echoPopupStartTime;
	int echoPopupTime;
	char echoPopupString[MAX_STRING_CHARS];
	int echoPopupX;
	int echoPopupY;
	float echoPopupScale;

	int errorPopupStartTime;
	int errorPopupTime;
	char errorPopupString[MAX_STRING_CHARS];

	int viewEnt;
	qboolean chase;
	int chaseEnt;
	vec3_t viewEntAngles;
	float viewEntOffsetX;
	float viewEntOffsetY;
	float viewEntOffsetZ;
	float chaseEntOffsetX;
	float chaseEntOffsetY;
	float chaseEntOffsetZ;
	qboolean viewUnlockYaw;
	qboolean viewUnlockPitch;

	qboolean useViewPointMark;
	vec3_t viewPointMarkOrigin;
	qboolean viewPointMarkSet;

	int loopStartTime;
	int loopEndTime;
	qboolean looping;
	qboolean fragForwarding;
	int fragForwardPreKillTime;
	int fragForwardDeathHoverTime;
	int fragForwardFragCount;

	int damageDoneTime;
	int damageDone;
	int hitSound;

	float crosshairBrightness;
	int crosshairAlphaAdjust;

	int serverFrameTime;
	qboolean demoSeeking;

	rawCameraPathKeyPoint_t rawCameraPoints[MAX_RAWCAMERAPOINTS];
	int numRawCameraPoints;

	//cameraPathKeyPoint_t cameraPoints[MAX_CAMERAPOINTS];
	cameraPoint_t cameraPoints[MAX_CAMERAPOINTS];
	vec3_t splinePoints[MAX_SPLINEPOINTS];
	int	splinePointsCameraPoints[MAX_SPLINEPOINTS];
	int numSplinePoints;

	int numCameraPoints;
	int selectedCameraPointMin;
	int selectedCameraPointMax;

	int selectedCameraPointField;

	qboolean cameraPlaying;
	qboolean cameraPlayedLastFrame;
	int currentCameraPoint;
	qboolean cameraWaitToSync;
	qboolean cameraJustStarted;
	qboolean playCameraCommandIssued;
	double cameraPointCommandTime;
	vec3_t cameraLastOrigin;
	vec3_t cameraVelocity;

	int numNoMoveClients;

	qboolean offlineDemoSkipEvents;
	int lastWeapon;
	vec3_t color1;  // our rail
	vec3_t color2;  // our rail

	double vibrateCameraTime;
	double vibrateCameraValue;
	double vibrateCameraPhase;
	vec3_t vibrateCameraOrigin;

	qboolean videoRecording;
	// sub millisecond overflow:  foverf = (double)ioverf / SUBTIME_RESOLUTION
	// foverf result is fraction of millisecond, so true millisecond time is:
	// (double)cg.time + cg.foverf  : note that is stored in cg.ftime
	int ioverf;
	double foverf;
	qboolean timerGoesUp;
	qboolean draw;
	menuDef_t *testMenu;
	duelScore_t duelScores[2];
	int duelPlayer1;
	int duelPlayer2;

	caStats_t caStats[MAX_CLIENTS];

	tdmScore_t tdmScore;
	tdmStats_t tdmStats[MAX_CLIENTS];

	ctfScore_t ctfScore;
	ctfStats_t ctfStats[MAX_CLIENTS];

	qboolean mapEnableBreath;
	qboolean mapEnableDust;
	int numChatLinesVisible;
	qboolean demoHasWarmup;
	int warmupTimeStart;
	qboolean drawInfo;

	float shotgunPattern[20][2];

	qboolean filterEntity[MAX_GENTITIES];
	filterTypes_t filter;

	qboolean freezeEntity[MAX_GENTITIES];
	centity_t freezeCent[MAX_GENTITIES];  // lazy
	playerState_t freezePs;
	int freezeCgtime;
	float freezeXyspeed;
	float freezeLandChange;
	int freezeLandTime;

	jumpInfo_t jumps[MAX_JUMPS_INFO];
	int numJumps;
	int jumpsFirstTime;
	qboolean jumpsNeedClearing;

	vec3_t victimOrigin;
	vec3_t victimAngles;

	vec3_t wcKillerOrigin;
	vec3_t wcKillerAngles;
	int wcKillerClientNum;

	qboolean mouseSeeking;
	int lastMouseSeekTime;
	qboolean forceDrawChat;

	qboolean useFakeArgs;
	int fakeArgc;
	char *fakeArgs[64];

	cvarInterp_t cvarInterp[MAX_CVAR_INTERP];
	atCommand_t atCommands[MAX_AT_COMMANDS];
	projectile_t projectiles[MAX_GENTITIES];
	int numProjectiles;

	qboolean configWriteDisabled;
	qboolean configStringOverride[MAX_CONFIGSTRINGS];
	char configStringOurs[MAX_CONFIGSTRINGS][MAX_STRING_CHARS];

	int proxTime;
	int proxCounter;
	int proxTick;

	qboolean dumpEntities;
	qboolean dumpFreecam;
	qboolean dumpValid[MAX_GENTITIES];
	int dumpLastServerTime;

	qhandle_t dumpFile;

#if 0
	// did this rewards get earned this snapshot -- passed on to fx system
	qboolean rewardImpressive;
	qboolean rewardExcellent;
	qboolean rewardHumiliation;
	qboolean rewardDefend;
	qboolean rewardAssist;
#endif

	qboolean qlColors;
	qboolean hasStartTimer;
	qboolean hasStopTimer;
	vec3_t startTimerOrigin;
	vec3_t stopTimerOrigin;

	floatNumber_t floatNumbers[MAX_FLOAT_NUMBERS];
	int numFloatNumbers;

	int eventFilter[MAX_EVENT_FILTER];
	int numEventFilter;

	poiPic_t poiPics[MAX_POI_PICS];
	int numPoiPics;

	int ctfsRedRoundScores[MAX_CTFS_ROUND_SCORES];
	int ctfsBlueRoundScores[MAX_CTFS_ROUND_SCORES];
	int ctfsRedScore;
	int ctfsBlueScore;
	qboolean ctfsRoundScoreValid;

	vec3_t lastImpactOrigin;
} cg_t;


// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	charsetProp;
	qhandle_t	charsetPropGlow;
	qhandle_t	charsetPropB;
	qhandle_t	whiteShader;

#if 1  //def MPACK
	qhandle_t	redCubeModel;
	qhandle_t	blueCubeModel;
	qhandle_t	redCubeIcon;
	qhandle_t	blueCubeIcon;
#endif
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagModel2;
	qhandle_t	blueFlagModel2;
	qhandle_t	neutralFlagModel2;
	qhandle_t   neutralFlagModel3;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

#if 1  //def MISSIONPACK
	qhandle_t	overloadBaseModel;
	qhandle_t	overloadTargetModel;
	qhandle_t	overloadLightsModel;
	qhandle_t	overloadEnergyModel;

	qhandle_t	harvesterModel;
	qhandle_t	harvesterRedSkin;
	qhandle_t	harvesterBlueSkin;
	qhandle_t	harvesterNeutralModel;
#endif

	qhandle_t dominationModel;
	qhandle_t dominationRedSkin;
	qhandle_t dominationBlueSkin;
	qhandle_t dominationNeutralSkin;
	qhandle_t dominationFloorMark;
	qhandle_t dominationCapA;
	qhandle_t dominationCapB;
	qhandle_t dominationCapC;
	qhandle_t dominationCapD;
	qhandle_t dominationCapE;
	qhandle_t dominationCapADist;
	qhandle_t dominationCapBDist;
	qhandle_t dominationCapCDist;
	qhandle_t dominationCapDDist;
	qhandle_t dominationCapEDist;
	qhandle_t dominationDefendA;
	qhandle_t dominationDefendB;
	qhandle_t dominationDefendC;
	qhandle_t dominationDefendD;
	qhandle_t dominationDefendE;
	qhandle_t dominationDefendADist;
	qhandle_t dominationDefendBDist;
	qhandle_t dominationDefendCDist;
	qhandle_t dominationDefendDDist;
	qhandle_t dominationDefendEDist;

	qhandle_t harvesterCapture;
	qhandle_t adAttack;
	qhandle_t adCapture;
	qhandle_t adDefend;

	qhandle_t	armorModel;
	qhandle_t	armorIcon;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;
	qhandle_t	gibSphere;

	qhandle_t iceAbdomen;
	qhandle_t iceBrain;

	qhandle_t	smoke2;

	qhandle_t	machinegunBrassModel;
	qhandle_t	shotgunBrassModel;

	qhandle_t	railRingsShader;
	qhandle_t	railCoreShader;

	qhandle_t	lightningShader;

	qhandle_t	friendShader;
	qhandle_t	frozenShader;
	qhandle_t foeShader;
	qhandle_t selfShader;

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	selectShader;
	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	ubyte   crosshairOrigImage[NUM_CROSSHAIRS][64 * 64 * 4];
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];

	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	smokePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	shotgunSmokePuffShader;
	qhandle_t	plasmaBallShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;
	qhandle_t	q3bloodTrailShader;
	qhandle_t	iceTrailShader;
#if 1  //def MPACK
	qhandle_t	nailPuffShader;
	qhandle_t	blueProxMine;
#endif

	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[5];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	q3bloodMarkShader;
	qhandle_t	iceMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	spawnArmorShader;
	qhandle_t	spawnArmor2Shader;
	qhandle_t	hastePuffShader;
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;
	qhandle_t	iceShader[3];

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
	qhandle_t	bloodExplosionShader;

	// special effects models
	qhandle_t	teleportEffectModel;
	qhandle_t	teleportEffectShader;

	qhandle_t	deathEffectModel;
	qhandle_t	deathEffectShader;

#if 1  //def MPACK
	qhandle_t	kamikazeEffectModel;
	qhandle_t	kamikazeShockWave;
	qhandle_t	kamikazeHeadModel;
	qhandle_t	kamikazeHeadTrail;
	qhandle_t	guardPowerupModel;
	qhandle_t	scoutPowerupModel;
	qhandle_t	doublerPowerupModel;
	qhandle_t	armorRegenPowerupModel;
	qhandle_t	invulnerabilityImpactModel;
	qhandle_t	invulnerabilityJuicedModel;
	qhandle_t	medkitUsageModel;
	qhandle_t	dustPuffShader;
	qhandle_t	heartShader;
#endif
	qhandle_t	invulnerabilityPowerupModel;

	qhandle_t 	caScoreRed;
	qhandle_t	caScoreBlue;

	// scoreboard headers
	qhandle_t	scoreboardName;
	qhandle_t	scoreboardPing;
	qhandle_t	scoreboardScore;
	qhandle_t	scoreboardTime;

	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;

	qhandle_t	adbox1x1;
	qhandle_t	adbox2x1;
	qhandle_t	adbox2x1_trans;
	qhandle_t	adbox4x1;
	qhandle_t	adbox8x1;
	qhandle_t	adboxblack;

	qhandle_t thawIcon;

	// pickup armor, health, powerups icons
	qhandle_t pickup_iconra;
	qhandle_t pickup_iconya;
	qhandle_t pickup_iconga;
	qhandle_t pickup_iconmh;
	qhandle_t pickup_iconmedkit;
	qhandle_t pickup_iconredflag;
	qhandle_t pickup_iconblueflag;
	qhandle_t pickup_iconneutralflag;
	qhandle_t pickup_iconquad;
	qhandle_t pickup_iconbs;
	qhandle_t pickup_iconregen;
	qhandle_t pickup_iconhaste;
	qhandle_t pickup_iconinvis;

	// sounds
	sfxHandle_t klaxon1;
	sfxHandle_t klaxon2;
	sfxHandle_t buzzer;
	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	sfxHandle_t	sfx_railg;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;
#if 1  //def MPACK
	sfxHandle_t	sfx_proxexp;
	sfxHandle_t	sfx_nghit;
	sfxHandle_t	sfx_nghitflesh;
	sfxHandle_t	sfx_nghitmetal;
	sfxHandle_t	sfx_chghit;
	sfxHandle_t	sfx_chghitflesh;
	sfxHandle_t	sfx_chghitmetal;
	sfxHandle_t kamikazeExplodeSound;
	sfxHandle_t kamikazeImplodeSound;
	sfxHandle_t kamikazeFarSound;
	sfxHandle_t useInvulnerabilitySound;
	sfxHandle_t invulnerabilityImpactSound1;
	sfxHandle_t invulnerabilityImpactSound2;
	sfxHandle_t invulnerabilityImpactSound3;
	sfxHandle_t invulnerabilityJuicedSound;
	sfxHandle_t obeliskHitSound1;
	sfxHandle_t obeliskHitSound2;
	sfxHandle_t obeliskHitSound3;
	sfxHandle_t	obeliskRespawnSound;
	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;
	sfxHandle_t	youSuckSound;
#endif
	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
	sfxHandle_t	electroGibSound1;
	sfxHandle_t	electroGibSound2;
	sfxHandle_t	electroGibSound3;
	sfxHandle_t	electroGibSound4;
	sfxHandle_t	electroGibBounceSound1;
	sfxHandle_t	electroGibBounceSound2;
	sfxHandle_t	electroGibBounceSound3;
	sfxHandle_t	electroGibBounceSound4;

	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t jumpPadSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
	sfxHandle_t	hitSound0;
	sfxHandle_t hitSound1;
	sfxHandle_t hitSound2;
	sfxHandle_t hitSound3;
	sfxHandle_t hitSoundHighArmor;
	sfxHandle_t hitSoundLowArmor;
	sfxHandle_t hitTeamSound;
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
	sfxHandle_t assistSound;
	sfxHandle_t defendSound;
	sfxHandle_t firstImpressiveSound;
	sfxHandle_t firstExcellentSound;
	sfxHandle_t firstHumiliationSound;

	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;

	sfxHandle_t voteNow;
	sfxHandle_t votePassed;
	sfxHandle_t voteFailed;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t flightSound;
	sfxHandle_t medkitSound;

	sfxHandle_t weaponHoverSound;

	// teamplay sounds
	sfxHandle_t captureAwardSound;
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;
	sfxHandle_t redWinsSound;
	sfxHandle_t blueWinsSound;
	sfxHandle_t redWinsRoundSound;
	sfxHandle_t blueWinsRoundSound;
	sfxHandle_t roundBeginsInSound;
	sfxHandle_t roundDrawSound;
	sfxHandle_t thirtySecondWarningSound;

	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;
	sfxHandle_t	takenYourTeamSound;
	sfxHandle_t	takenOpponentSound;

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
	sfxHandle_t neutralFlagReturnedSound;
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t	enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t	youHaveFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;
	sfxHandle_t holyShitSound;
	sfxHandle_t lastStandingSound;
	sfxHandle_t attackFlagSound;
	sfxHandle_t defendFlagSound;
	sfxHandle_t perfectSound;
	sfxHandle_t roundOverSound;
	sfxHandle_t aLostSound;
	sfxHandle_t bLostSound;
	sfxHandle_t cLostSound;
	sfxHandle_t dLostSound;
	sfxHandle_t eLostSound;
	sfxHandle_t aCapturedSound;
	sfxHandle_t bCapturedSound;
	sfxHandle_t cCapturedSound;
	sfxHandle_t dCapturedSound;
	sfxHandle_t eCapturedSound;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

#if 1  //def MPACK
	// new stuff
	qhandle_t patrolShader;
	qhandle_t assaultShader;
	qhandle_t campShader;
	qhandle_t followShader;
	qhandle_t defendShader;
	qhandle_t teamLeaderShader;
	qhandle_t retrieveShader;
	qhandle_t escortShader;
	qhandle_t flagShaders[3];
	sfxHandle_t	countPrepareTeamSound;

	sfxHandle_t armorRegenSound;
	sfxHandle_t armorRegenSoundRegen;
	sfxHandle_t doublerSound;
	sfxHandle_t guardSound;
	sfxHandle_t scoutSound;
#endif
	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
	sfxHandle_t	n_healthSound;
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;
	sfxHandle_t killBeep[8];
	sfxHandle_t thawTick;

	// don't actually load these
	fontInfo_t	giantchar;
	fontInfo_t	bigchar;
	fontInfo_t	smallchar;
	fontInfo_t	tinychar;

	fontInfo_t	qlfont12;
	fontInfo_t	qlfont16;
	fontInfo_t	qlfont20;
	fontInfo_t	qlfont24;
	fontInfo_t	qlfont48;

	//fontInfo_t q3font16;

	fontInfo_t	centerPrintFont;
	int centerPrintFontModificationCount;
	fontInfo_t	fragMessageFont;
	int fragMessageFontModificationCount;
	fontInfo_t rewardsFont;
	int rewardsFontModificationCount;
	fontInfo_t fpsFont;
	int fpsFontModificationCount;
	fontInfo_t clientItemTimerFont;
	int clientItemTimerFontModificationCount;
	fontInfo_t ammoWarningFont;
	int ammoWarningFontModificationCount;

	// such crap
	fontInfo_t playerNamesFont;
	int playerNamesFontModificationCount;
	int playerNamesStyleModificationCount;
	int playerNamesPointSizeModificationCount;
	int playerNamesColorModificationCount;
	int playerNamesAlphaModificationCount;

	fontInfo_t snapshotFont;
	int snapshotFontModificationCount;
	fontInfo_t crosshairNamesFont;
	int crosshairNamesFontModificationCount;
	fontInfo_t crosshairTeammateHealthFont;
	int crosshairTeammateHealthFontModificationCount;
	fontInfo_t warmupStringFont;
	int warmupStringFontModificationCount;
	fontInfo_t waitingForPlayersFont;
	int waitingForPlayersFontModificationCount;
	fontInfo_t voteFont;
	int voteFontModificationCount;
	fontInfo_t teamVoteFont;
	int teamVoteFontModificationCount;
	fontInfo_t followingFont;
	int followingFontModificationCount;
	fontInfo_t weaponBarFont;
	int weaponBarFontModificationCount;
	fontInfo_t itemPickupsFont;
	int itemPickupsFontModificationCount;
	fontInfo_t originFont;
	int originFontModificationCount;
	fontInfo_t speedFont;
	int speedFontModificationCount;
	fontInfo_t lagometerFont;
	int lagometerFontModificationCount;
	fontInfo_t attackerFont;
	int attackerFontModificationCount;
	fontInfo_t teamOverlayFont;
	int teamOverlayFontModificationCount;
	fontInfo_t cameraPointInfoFont;
	int cameraPointInfoFontModificationCount;
	fontInfo_t jumpSpeedsFont;
	int jumpSpeedsFontModificationCount;
	fontInfo_t jumpSpeedsTimeFont;
	int jumpSpeedsTimeFontModificationCount;
	fontInfo_t proxWarningFont;
	int proxWarningFontModificationCount;
	fontInfo_t dominationPointStatusFont;
	int dominationPointStatusFontModificationCount;

	qhandle_t	gametypeIcon[GT_MAX_GAME_TYPE];
	qhandle_t singleShader;

	qhandle_t infiniteAmmo;
	qhandle_t premiumIcon;

	qhandle_t defragItemShader;
	qhandle_t weaplit;
	qhandle_t flagCarrier;
	qhandle_t flagCarrierNeutral;

} cgMedia_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;

	int firstServerMessageNum;
	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	int		gametype;
	int				dmflags;
	int				teamflags;
	int scorelimit;
	int				fraglimit;
	int				roundlimit;
	int				roundtimelimit;
	int				capturelimit;
	int				timelimit;
	int				realTimelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];
	char			redTeamName[MAX_QPATH];
	char			blueTeamName[MAX_QPATH];
	char redTeamClanTag[MAX_QPATH];
	char blueTeamClanTag[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;
	int				redPlayersLeft;
	int				bluePlayersLeft;
	int				roundBeginTime;
	int				roundNum;
	int roundTurn;
	qboolean		roundStarted;
	int				countDownSoundPlayed;
	qboolean		thirtySecondWarningPlayed;
	char firstPlace[MAX_STRING_CHARS];
	char secondPlace[MAX_STRING_CHARS];

	int				timeoutBeginCgTime;
	int				timeoutBeginTime;
	int				timeoutEndTime;
	qboolean		timeoutCountingDown;
	int				redTeamTimeoutsLeft;
	int				blueTeamTimeoutsLeft;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];
	clientInfo_t	clientinfoOrig[MAX_CLIENTS];
	//qboolean		newConnectedClient[MAX_CLIENTS];  // for demo playback so they can be shown in the scoreboard, cleared with 'scores' server command
	qhandle_t		clientNameImage[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

	int cursorX;
	int cursorY;
	//qboolean eventHandling;
	int eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// orders
	int currentOrder;
	qboolean orderPending;
	int orderTime;
	int currentVoiceClient;
	int acceptOrderTime;
	int acceptTask;
	int acceptLeader;
	char acceptVoice[MAX_NAME_LENGTH];

	// camera script
	float	scrFadeAlpha, scrFadeAlphaCurrent;
	int		scrFadeStartTime;
	int		scrFadeDuration;
	// end camera script

	// media
	cgMedia_t		media;

    qboolean fKeyPressed[256];  // Key status to get around console issues
	int fKeyPressedLastTime[256];
	qboolean fKeyRepeat[256];  // ++vstr

	qboolean		adsLoaded;
	int 			numAds;
	qboolean		transAds[MAX_MAP_ADVERTISEMENTS];  // transparent add
	float 			adverts[MAX_MAP_ADVERTISEMENTS * 16];  // fu
	char adShaders[MAX_MAP_ADVERTISEMENTS][MAX_QPATH];

	qboolean isQuakeLiveDemo;
	int				qlversion[4];

	qboolean gotFirstSnap;
	int firstSnapServerTime;
	int lastConnectedDisconnectedPlayer;
	char lastConnectedDisconnectedPlayerName[MAX_QPATH];
	clientInfo_t *lastConnectedDisconnectedPlayerClientInfo;
	int skillRating;
	qboolean needToCheckSkillRating;
	int sv_fps;  //FIXME sometimes not reported

	qboolean instaGib;
	float serverModelScale;
	float serverHeadModelScale;
	qboolean serverHaveHeadScaleOffset;
	float serverHeadModelScaleOffset;
	char serverModelOverride[MAX_QPATH];
	char serverHeadModelOverride[MAX_QPATH];
	qboolean serverHaveCustomModelString;
	qboolean serverAllowCustomHead;
	int protocol;
	qboolean cpma;
	qboolean cpm;
	qboolean osp;
	qboolean ospEncrypt;
	qboolean defrag;
	int cpmaTimeoutTime;

	int dominationControlPointModel;
	int dominationRedPoints;
	int dominationBluePoints;
	centity_t *dominationControlPointEnts[5];
} cgs_t;


//==============================================================================

extern displayContextDef_t cgDC;
extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES + 1];  //FIXME hack for viewEnt
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

extern clientInfo_t		EM_ModelInfo;
extern int				EM_Loaded;
extern byte				EC_Colors[3][4];
extern int				EC_Loaded;
extern char    			*cg_customSoundNames[MAX_CUSTOM_SOUNDS];

//extern	vmCvar_t		cg_centertime;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
extern	vmCvar_t		cg_swingSpeed;
extern	vmCvar_t		cg_shadows;
extern	vmCvar_t		cg_gibs;
extern vmCvar_t cg_gibColor;
extern	vmCvar_t		cg_gibJump;
extern	vmCvar_t		cg_gibVelocity;
extern vmCvar_t cg_gibVelocityRandomness;
extern vmCvar_t cg_gibDirScale;
extern vmCvar_t cg_gibOriginOffset;
extern vmCvar_t cg_gibOriginOffsetZ;
extern vmCvar_t cg_gibRandomness;
extern vmCvar_t cg_gibRandomnessZ;
extern	vmCvar_t		cg_gibTime;
extern	vmCvar_t		cg_gibStepTime;
extern vmCvar_t cg_gibBounceFactor;
extern vmCvar_t cg_gibGravity;
extern	vmCvar_t		cg_gibSparksSize;
extern vmCvar_t cg_gibSparksColor;
extern vmCvar_t cg_gibSparksHighlight;
extern vmCvar_t cg_gibFloatingVelocity;
extern vmCvar_t cg_gibFloatingRandomness;
extern vmCvar_t cg_gibFloatingOriginOffset;
extern vmCvar_t cg_gibFloatingOriginOffsetZ;
extern	vmCvar_t		cg_impactSparks;
extern	vmCvar_t		cg_impactSparksLifetime;
extern	vmCvar_t		cg_impactSparksSize;
extern	vmCvar_t		cg_impactSparksVelocity;
extern vmCvar_t cg_impactSparksColor;
extern vmCvar_t cg_impactSparksHighlight;
extern	vmCvar_t		cg_shotgunImpactSparks;
extern	vmCvar_t		cg_shotgunMarks;
extern vmCvar_t cg_shotgunStyle;
extern vmCvar_t cg_shotgunRandomness;
extern	vmCvar_t		cg_drawTimer;

extern  vmCvar_t		cg_drawClientItemTimer;
extern	vmCvar_t		cg_drawClientItemTimerX;
extern	vmCvar_t		cg_drawClientItemTimerY;
extern	vmCvar_t		cg_drawClientItemTimerScale;
extern	vmCvar_t		cg_drawClientItemTimerFont;
extern	vmCvar_t		cg_drawClientItemTimerPointSize;
extern vmCvar_t cg_drawClientItemTimerAlpha;
extern	vmCvar_t		cg_drawClientItemTimerStyle;
extern	vmCvar_t		cg_drawClientItemTimerAlign;
extern vmCvar_t			cg_drawClientItemTimerSpacing;
extern vmCvar_t cg_itemSpawnPrint;

extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawFPSNoText;
extern vmCvar_t cg_drawFPSX;
extern vmCvar_t cg_drawFPSY;
extern vmCvar_t cg_drawFPSAlign;
extern vmCvar_t cg_drawFPSStyle;
extern vmCvar_t cg_drawFPSFont;
extern vmCvar_t cg_drawFPSPointSize;
extern vmCvar_t cg_drawFPSScale;
//extern vmCvar_t cg_drawFPSTime;
extern vmCvar_t cg_drawFPSColor;
extern vmCvar_t cg_drawFPSAlpha;
//extern vmCvar_t cg_drawFPSFade;
//extern vmCvar_t cg_drawFPSFadeTime;

extern	vmCvar_t		cg_drawSnapshot;
extern	vmCvar_t		cg_drawSnapshotX;
extern	vmCvar_t		cg_drawSnapshotY;
extern vmCvar_t cg_drawSnapshotAlign;
extern vmCvar_t cg_drawSnapshotStyle;
extern vmCvar_t cg_drawSnapshotFont;
extern vmCvar_t cg_drawSnapshotPointSize;
extern vmCvar_t cg_drawSnapshotScale;
extern vmCvar_t cg_drawSnapshotColor;
extern vmCvar_t cg_drawSnapshotAlpha;

extern	vmCvar_t		cg_draw3dIcons;
extern	vmCvar_t		cg_drawIcons;

extern	vmCvar_t		cg_drawAmmoWarning;
extern	vmCvar_t		cg_drawAmmoWarningX;
extern	vmCvar_t		cg_drawAmmoWarningY;
extern vmCvar_t cg_drawAmmoWarningAlign;
extern vmCvar_t cg_drawAmmoWarningStyle;
extern vmCvar_t cg_drawAmmoWarningFont;
extern vmCvar_t cg_drawAmmoWarningPointSize;
extern vmCvar_t cg_drawAmmoWarningScale;
//extern vmCvar_t cg_drawAmmoWarningTime;
extern vmCvar_t cg_drawAmmoWarningColor;
extern vmCvar_t cg_drawAmmoWarningAlpha;
//extern vmCvar_t cg_drawAmmoWarningFade;
//extern vmCvar_t cg_drawAmmoWarningFadeTime;

extern vmCvar_t cg_drawCrosshair;

extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairNamesX;
extern vmCvar_t cg_drawCrosshairNamesY;
extern vmCvar_t cg_drawCrosshairNamesAlign;
extern vmCvar_t cg_drawCrosshairNamesStyle;
extern vmCvar_t cg_drawCrosshairNamesFont;
extern vmCvar_t cg_drawCrosshairNamesPointSize;
extern vmCvar_t cg_drawCrosshairNamesScale;
extern vmCvar_t cg_drawCrosshairNamesTime;
extern vmCvar_t cg_drawCrosshairNamesColor;
extern vmCvar_t cg_drawCrosshairNamesAlpha;
extern vmCvar_t cg_drawCrosshairNamesFade;
extern vmCvar_t cg_drawCrosshairNamesFadeTime;

extern vmCvar_t cg_drawCrosshairTeammateHealth;
extern vmCvar_t cg_drawCrosshairTeammateHealthX;
extern vmCvar_t cg_drawCrosshairTeammateHealthY;
extern vmCvar_t cg_drawCrosshairTeammateHealthAlign;
extern vmCvar_t cg_drawCrosshairTeammateHealthStyle;
extern vmCvar_t cg_drawCrosshairTeammateHealthFont;
extern vmCvar_t cg_drawCrosshairTeammateHealthPointSize;
extern vmCvar_t cg_drawCrosshairTeammateHealthScale;
extern vmCvar_t cg_drawCrosshairTeammateHealthTime;
extern vmCvar_t cg_drawCrosshairTeammateHealthColor;
extern vmCvar_t cg_drawCrosshairTeammateHealthAlpha;
extern vmCvar_t cg_drawCrosshairTeammateHealthFade;
extern vmCvar_t cg_drawCrosshairTeammateHealthFadeTime;

extern vmCvar_t cg_drawRewards;
extern vmCvar_t cg_drawRewardsMax;
extern vmCvar_t cg_drawRewardsX;
extern vmCvar_t cg_drawRewardsY;
extern vmCvar_t cg_drawRewardsAlign;
extern vmCvar_t cg_drawRewardsStyle;
extern vmCvar_t cg_drawRewardsFont;
extern vmCvar_t cg_drawRewardsPointSize;
extern vmCvar_t cg_drawRewardsScale;
extern vmCvar_t cg_drawRewardsImageScale;
extern vmCvar_t cg_drawRewardsTime;
extern vmCvar_t cg_drawRewardsColor;
extern vmCvar_t cg_drawRewardsAlpha;
extern vmCvar_t cg_drawRewardsFade;
extern vmCvar_t cg_drawRewardsFadeTime;

extern	vmCvar_t		cg_drawTeamOverlay;
extern	vmCvar_t		cg_drawTeamOverlayX;
extern	vmCvar_t		cg_drawTeamOverlayY;
extern	vmCvar_t		cg_teamOverlayUserinfo;
extern vmCvar_t cg_drawTeamOverlayFont;
extern vmCvar_t cg_drawTeamOverlayPointSize;
extern vmCvar_t cg_drawTeamOverlayAlign;
//extern vmCvar_t cg_drawTeamOverlayAlignY;
extern vmCvar_t cg_drawTeamOverlayScale;
//extern vmCvar_t cg_drawTeamOverlayAlpha;

extern vmCvar_t cg_drawJumpSpeeds;
extern vmCvar_t cg_drawJumpSpeedsNoText;
extern vmCvar_t cg_drawJumpSpeedsMax;
extern vmCvar_t cg_drawJumpSpeedsX;
extern vmCvar_t cg_drawJumpSpeedsY;
extern vmCvar_t cg_drawJumpSpeedsAlign;
extern vmCvar_t cg_drawJumpSpeedsStyle;
extern vmCvar_t cg_drawJumpSpeedsFont;
extern vmCvar_t cg_drawJumpSpeedsPointSize;
extern vmCvar_t cg_drawJumpSpeedsScale;
extern vmCvar_t cg_drawJumpSpeedsColor;
extern vmCvar_t cg_drawJumpSpeedsAlpha;

extern vmCvar_t cg_drawJumpSpeedsTime;
extern vmCvar_t cg_drawJumpSpeedsTimeNoText;
extern vmCvar_t cg_drawJumpSpeedsTimeX;
extern vmCvar_t cg_drawJumpSpeedsTimeY;
extern vmCvar_t cg_drawJumpSpeedsTimeAlign;
extern vmCvar_t cg_drawJumpSpeedsTimeStyle;
extern vmCvar_t cg_drawJumpSpeedsTimeFont;
extern vmCvar_t cg_drawJumpSpeedsTimePointSize;
extern vmCvar_t cg_drawJumpSpeedsTimeScale;
extern vmCvar_t cg_drawJumpSpeedsTimeColor;
extern vmCvar_t cg_drawJumpSpeedsTimeAlpha;

extern	vmCvar_t		cg_crosshairX;
extern	vmCvar_t		cg_crosshairY;
extern	vmCvar_t		cg_crosshairSize;
extern	vmCvar_t		cg_crosshairHealth;
extern vmCvar_t cg_crosshairPulse;
extern vmCvar_t cg_crosshairHitStyle;
extern vmCvar_t cg_crosshairHitColor;
extern vmCvar_t cg_crosshairHitTime;
extern vmCvar_t cg_crosshairBrightness;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairAlphaAdjust;

extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_drawTeamBackground;
extern	vmCvar_t		cg_draw2D;
extern	vmCvar_t		cg_animSpeed;
extern	vmCvar_t		cg_debugAnim;
extern	vmCvar_t		cg_debugPosition;
extern	vmCvar_t		cg_debugEvents;
extern	vmCvar_t		cg_railTrailTime;
extern vmCvar_t cg_railQL;
//extern vmCvar_t cg_railQLWhiteShift;
extern vmCvar_t cg_railQLRailRingWhiteValue;
extern vmCvar_t cg_railNudge;
extern vmCvar_t cg_railRings;
extern vmCvar_t cg_railRadius;
extern vmCvar_t cg_railRotation;
extern vmCvar_t cg_railSpacing;
extern vmCvar_t cg_railItemColor;
extern vmCvar_t cg_railUseOwnColors;
extern vmCvar_t cg_railFromMuzzle;
extern	vmCvar_t		cg_errorDecay;
extern	vmCvar_t		cg_nopredict;
extern	vmCvar_t		cg_noPlayerAnims;
extern	vmCvar_t		cg_showmiss;
extern	vmCvar_t		cg_footsteps;
extern	vmCvar_t		cg_marks;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_markFadeTime;
extern vmCvar_t cg_debugImpactOrigin;
extern	vmCvar_t		cg_brassTime;
extern	vmCvar_t		cg_gun_frame;
extern	vmCvar_t		cg_gun_x;
extern	vmCvar_t		cg_gun_y;
extern	vmCvar_t		cg_gun_z;
extern vmCvar_t cg_gunSize;
extern vmCvar_t cg_gunSizeThirdPerson;
extern	vmCvar_t		cg_drawGun;
extern	vmCvar_t		cg_viewsize;
extern	vmCvar_t		cg_tracerChance;
extern	vmCvar_t		cg_tracerWidth;
extern	vmCvar_t		cg_tracerLength;
extern	vmCvar_t		cg_autoswitch;
extern	vmCvar_t		cg_ignore;
extern	vmCvar_t		cg_simpleItems;
extern vmCvar_t cg_simpleItemsScale;
extern vmCvar_t cg_itemsWh;
extern vmCvar_t cg_itemFx;
extern vmCvar_t cg_itemSize;
extern	vmCvar_t		cg_fov;
extern vmCvar_t cg_fovy;
extern	vmCvar_t		cg_zoomFov;
extern vmCvar_t cg_zoomTime;
extern vmCvar_t cg_zoomIgnoreTimescale;
extern vmCvar_t cg_zoomBroken;
extern	vmCvar_t		cg_thirdPersonRange;
extern	vmCvar_t		cg_thirdPersonAngle;
extern	vmCvar_t		cg_thirdPerson;
extern	vmCvar_t		cg_stereoSeparation;

extern	vmCvar_t		cg_lagometer;
extern	vmCvar_t		cg_lagometerX;
extern	vmCvar_t		cg_lagometerY;
extern 	vmCvar_t 		cg_lagometerFlash;
extern	vmCvar_t		cg_lagometerFlashValue;
extern vmCvar_t cg_lagometerAlign;
extern vmCvar_t cg_lagometerFontAlign;
extern vmCvar_t cg_lagometerFontStyle;
extern vmCvar_t cg_lagometerFont;
extern vmCvar_t cg_lagometerFontPointSize;
extern vmCvar_t cg_lagometerFontScale;
extern vmCvar_t cg_lagometerScale;
extern vmCvar_t cg_lagometerFontColor;
extern vmCvar_t cg_lagometerAlpha;
extern vmCvar_t cg_lagometerFontAlpha;
extern vmCvar_t cg_lagometerAveragePing;
extern vmCvar_t cg_lagometerSnapshotPing;

extern	vmCvar_t		cg_drawAttacker;
extern	vmCvar_t		cg_drawAttackerX;
extern	vmCvar_t		cg_drawAttackerY;
extern vmCvar_t cg_drawAttackerAlign;
extern vmCvar_t cg_drawAttackerStyle;
extern vmCvar_t cg_drawAttackerFont;
extern vmCvar_t cg_drawAttackerPointSize;
extern vmCvar_t cg_drawAttackerScale;
extern vmCvar_t cg_drawAttackerImageScale;
extern vmCvar_t cg_drawAttackerTime;
extern vmCvar_t cg_drawAttackerColor;
extern vmCvar_t cg_drawAttackerAlpha;
extern vmCvar_t cg_drawAttackerFade;
extern vmCvar_t cg_drawAttackerFadeTime;

extern	vmCvar_t		cg_synchronousClients;
extern	vmCvar_t		cg_teamChatTime;
extern	vmCvar_t		cg_teamChatHeight;
extern	vmCvar_t		cg_stats;
extern	vmCvar_t 		cg_forceModel;
extern vmCvar_t cg_forcePovModel;
extern	vmCvar_t 		cg_buildScript;
extern	vmCvar_t		cg_paused;
extern	vmCvar_t		cg_blood;
extern	vmCvar_t		cg_predictItems;
extern	vmCvar_t		cg_deferPlayers;
extern	vmCvar_t		cg_drawFriend;
extern vmCvar_t cg_drawFriendMinWidth;
extern vmCvar_t cg_drawFriendMaxWidth;
extern vmCvar_t cg_drawFoe;
extern vmCvar_t cg_drawFoeMinWidth;
extern vmCvar_t cg_drawFoeMaxWidth;
extern vmCvar_t cg_drawSelf;
extern vmCvar_t cg_drawSelfMinWidth;
extern vmCvar_t cg_drawSelfMaxWidth;
extern	vmCvar_t		cg_teamChatsOnly;
#ifdef MISSIONPACK
extern	vmCvar_t		cg_noVoiceChats;
extern	vmCvar_t		cg_noVoiceText;
#endif
extern  vmCvar_t		cg_scorePlums;

extern	vmCvar_t		cg_smoothClients;
extern	vmCvar_t		pmove_fixed;
extern	vmCvar_t		pmove_msec;
//extern	vmCvar_t		cg_pmove_fixed;
//extern vmCvar_t g_weapon_plasma_rate;

extern	vmCvar_t		cg_cameraOrbit;
extern	vmCvar_t		cg_cameraOrbitDelay;
extern	vmCvar_t		cg_timescaleFadeEnd;
extern	vmCvar_t		cg_timescaleFadeSpeed;
extern	vmCvar_t		cg_timescale;
extern	vmCvar_t		cg_cameraMode;
extern  vmCvar_t		cg_smallFont;
extern  vmCvar_t		cg_bigFont;
extern	vmCvar_t		cg_noTaunt;
extern	vmCvar_t		cg_noProjectileTrail;
extern vmCvar_t cg_smokeRadius_SG;
extern vmCvar_t cg_smokeRadius_GL;
extern vmCvar_t cg_smokeRadius_NG;
extern vmCvar_t cg_smokeRadius_RL;
extern vmCvar_t cg_smokeRadius_PL;
extern vmCvar_t cg_smokeRadius_breath;
extern vmCvar_t cg_enableBreath;
extern vmCvar_t cg_smokeRadius_dust;
extern vmCvar_t cg_enableDust;
extern vmCvar_t cg_smokeRadius_flight;
extern vmCvar_t cg_smokeRadius_haste;
//extern	vmCvar_t		cg_oldRail;
extern	vmCvar_t		cg_oldRocket;
//extern	vmCvar_t		cg_oldPlasma;
extern vmCvar_t cg_plasmaStyle;
extern	vmCvar_t		cg_trueLightning;
#if 1  //def MISSIONPACK
extern	vmCvar_t		cg_redTeamName;
extern	vmCvar_t		cg_blueTeamName;
extern	vmCvar_t		cg_currentSelectedPlayer;
extern	vmCvar_t		cg_currentSelectedPlayerName;
extern	vmCvar_t		cg_singlePlayer;
extern	vmCvar_t		cg_serverEnableDust;
extern	vmCvar_t		cg_serverEnableBreath;
extern	vmCvar_t		cg_singlePlayerActive;
extern  vmCvar_t		cg_recordSPDemo;
extern  vmCvar_t		cg_recordSPDemoName;
extern	vmCvar_t		cg_obeliskRespawnDelay;
#endif

extern vmCvar_t cg_drawSpeed;
extern vmCvar_t cg_drawSpeedX;
extern vmCvar_t cg_drawSpeedY;
extern vmCvar_t cg_drawSpeedNoText;
extern vmCvar_t cg_drawSpeedAlign;
extern vmCvar_t cg_drawSpeedStyle;
extern vmCvar_t cg_drawSpeedFont;
extern vmCvar_t cg_drawSpeedPointSize;
extern vmCvar_t cg_drawSpeedScale;
extern vmCvar_t cg_drawSpeedColor;
extern vmCvar_t cg_drawSpeedAlpha;

extern vmCvar_t cg_drawOrigin;
extern vmCvar_t cg_drawOriginX;
extern vmCvar_t cg_drawOriginY;
extern vmCvar_t cg_drawOriginAlign;
extern vmCvar_t cg_drawOriginStyle;
extern vmCvar_t cg_drawOriginFont;
extern vmCvar_t cg_drawOriginPointSize;
extern vmCvar_t cg_drawOriginScale;
extern vmCvar_t cg_drawOriginColor;
extern vmCvar_t cg_drawOriginAlpha;

extern vmCvar_t	cg_drawScores;
extern vmCvar_t cg_drawPlayersLeft;
extern vmCvar_t cg_drawPowerups;

extern vmCvar_t cg_drawItemPickups;
extern vmCvar_t cg_drawItemPickupsX;
extern vmCvar_t cg_drawItemPickupsY;
extern vmCvar_t cg_drawItemPickupsImageScale;
extern vmCvar_t cg_drawItemPickupsAlign;
extern vmCvar_t cg_drawItemPickupsStyle;
extern vmCvar_t cg_drawItemPickupsFont;
extern vmCvar_t cg_drawItemPickupsPointSize;
extern vmCvar_t cg_drawItemPickupsScale;
extern vmCvar_t cg_drawItemPickupsTime;
extern vmCvar_t cg_drawItemPickupsColor;
extern vmCvar_t cg_drawItemPickupsAlpha;
extern vmCvar_t cg_drawItemPickupsFade;
extern vmCvar_t cg_drawItemPickupsFadeTime;
extern vmCvar_t cg_drawItemPickupsCount;

extern vmCvar_t cg_drawFollowing;
extern vmCvar_t cg_drawFollowingX;
extern vmCvar_t cg_drawFollowingY;
extern vmCvar_t cg_drawFollowingAlign;
extern vmCvar_t cg_drawFollowingStyle;
extern vmCvar_t cg_drawFollowingFont;
extern vmCvar_t cg_drawFollowingPointSize;
extern vmCvar_t cg_drawFollowingScale;
extern vmCvar_t cg_drawFollowingColor;
extern vmCvar_t cg_drawFollowingAlpha;

extern vmCvar_t cg_testQlFont;
//extern vmCvar_t cg_deathSparkRadius;
extern vmCvar_t cg_qlhud;
extern vmCvar_t cg_qlFontScaling;

extern vmCvar_t cg_weaponBar;
extern vmCvar_t cg_weaponBarX;
extern vmCvar_t cg_weaponBarY;
extern vmCvar_t cg_weaponBarFont;
extern vmCvar_t cg_weaponBarPointSize;

extern vmCvar_t cg_drawFullWeaponBar;

extern vmCvar_t cg_scoreBoardStyle;
extern vmCvar_t cg_scoreBoardSpectatorScroll;
extern vmCvar_t cg_scoreBoardWhenDead;
extern vmCvar_t cg_scoreBoardAtIntermission;
extern vmCvar_t cg_scoreBoardWarmup;
extern vmCvar_t cg_scoreBoardOld;

extern vmCvar_t cg_hitBeep;

extern vmCvar_t cg_drawSpawns;
extern vmCvar_t cg_drawSpawnsInitial;
extern vmCvar_t cg_drawSpawnsInitialZ;
extern vmCvar_t cg_drawSpawnsRespawns;
extern vmCvar_t cg_drawSpawnsRespawnsZ;
extern vmCvar_t cg_drawSpawnsShared;
extern vmCvar_t cg_drawSpawnsSharedZ;

extern vmCvar_t cg_freecam_noclip;
extern vmCvar_t cg_freecam_sensitivity;
extern vmCvar_t cg_freecam_yaw;
extern vmCvar_t cg_freecam_pitch;
extern vmCvar_t cg_freecam_speed;
extern vmCvar_t cg_freecam_crosshair;
extern vmCvar_t cg_freecam_useTeamSettings;
extern vmCvar_t cg_freecam_rollValue;
extern vmCvar_t cg_freecam_useServerView;
extern vmCvar_t cg_freecam_unlockPitch;

extern vmCvar_t	cg_chatTime;
extern vmCvar_t cg_chatLines;
extern vmCvar_t cg_chatHistoryLength;

extern vmCvar_t cg_drawCenterPrint;
extern vmCvar_t cg_drawCenterPrintX;
extern vmCvar_t cg_drawCenterPrintY;
extern vmCvar_t cg_drawCenterPrintAlign;
extern vmCvar_t cg_drawCenterPrintStyle;
extern vmCvar_t cg_drawCenterPrintFont;
extern vmCvar_t cg_drawCenterPrintPointSize;
extern vmCvar_t cg_drawCenterPrintScale;
extern vmCvar_t cg_drawCenterPrintTime;
extern vmCvar_t cg_drawCenterPrintColor;
extern vmCvar_t cg_drawCenterPrintAlpha;
extern vmCvar_t cg_drawCenterPrintFade;
extern vmCvar_t cg_drawCenterPrintFadeTime;

extern vmCvar_t cg_drawVote;
extern vmCvar_t cg_drawVoteX;
extern vmCvar_t cg_drawVoteY;
extern vmCvar_t cg_drawVoteAlign;
extern vmCvar_t cg_drawVoteStyle;
extern vmCvar_t cg_drawVoteFont;
extern vmCvar_t cg_drawVotePointSize;
extern vmCvar_t cg_drawVoteScale;
extern vmCvar_t cg_drawVoteColor;
extern vmCvar_t cg_drawVoteAlpha;

extern vmCvar_t cg_drawTeamVote;
extern vmCvar_t cg_drawTeamVoteX;
extern vmCvar_t cg_drawTeamVoteY;
extern vmCvar_t cg_drawTeamVoteAlign;
extern vmCvar_t cg_drawTeamVoteStyle;
extern vmCvar_t cg_drawTeamVoteFont;
extern vmCvar_t cg_drawTeamVotePointSize;
extern vmCvar_t cg_drawTeamVoteScale;
extern vmCvar_t cg_drawTeamVoteColor;
extern vmCvar_t cg_drawTeamVoteAlpha;

extern vmCvar_t cg_drawWaitingForPlayers;
extern vmCvar_t cg_drawWaitingForPlayersX;
extern vmCvar_t cg_drawWaitingForPlayersY;
extern vmCvar_t cg_drawWaitingForPlayersAlign;
extern vmCvar_t cg_drawWaitingForPlayersStyle;
extern vmCvar_t cg_drawWaitingForPlayersFont;
extern vmCvar_t cg_drawWaitingForPlayersPointSize;
extern vmCvar_t cg_drawWaitingForPlayersScale;
extern vmCvar_t cg_drawWaitingForPlayersColor;
extern vmCvar_t cg_drawWaitingForPlayersAlpha;

extern vmCvar_t cg_drawWarmupString;
extern vmCvar_t cg_drawWarmupStringX;
extern vmCvar_t cg_drawWarmupStringY;
extern vmCvar_t cg_drawWarmupStringAlign;
extern vmCvar_t cg_drawWarmupStringStyle;
extern vmCvar_t cg_drawWarmupStringFont;
extern vmCvar_t cg_drawWarmupStringPointSize;
extern vmCvar_t cg_drawWarmupStringScale;
extern vmCvar_t cg_drawWarmupStringColor;
extern vmCvar_t cg_drawWarmupStringAlpha;

extern vmCvar_t cg_ambientSounds;
extern vmCvar_t cg_weather;

extern vmCvar_t cg_interpolateMissiles;
extern vmCvar_t wolfcam_hoverTime;
extern vmCvar_t wolfcam_switchMode;
//extern vmCvar_t cg_fragForwardStyle;
extern vmCvar_t wolfcam_firstPersonSwitchSoundStyle;

extern vmCvar_t cg_weaponRedTeamColor;
extern vmCvar_t cg_weaponBlueTeamColor;
//extern vmCvar_t cg_noTeamColor;

extern vmCvar_t cg_hudRedTeamColor;
extern vmCvar_t cg_hudBlueTeamColor;
extern vmCvar_t cg_hudNoTeamColor;
extern vmCvar_t cg_hudForceRedTeamClanTag;
extern vmCvar_t cg_hudForceBlueTeamClanTag;

extern vmCvar_t cg_enemyModel;
extern vmCvar_t cg_enemyHeadSkin;
extern vmCvar_t cg_enemyTorsoSkin;
extern vmCvar_t cg_enemyLegsSkin;
extern vmCvar_t cg_enemyHeadColor;
extern vmCvar_t cg_enemyTorsoColor;
extern vmCvar_t cg_enemyLegsColor;
extern vmCvar_t cg_enemyRailColor1;
extern vmCvar_t cg_enemyRailColor2;
extern vmCvar_t cg_enemyRailColor1Team;
extern vmCvar_t cg_enemyRailColor2Team;
extern vmCvar_t cg_enemyRailItemColor;
extern vmCvar_t cg_enemyRailItemColorTeam;
//extern vmCvar_t cg_enemyOldRail;
extern vmCvar_t cg_enemyRailRings;
extern vmCvar_t cg_enemyRailNudge;

extern vmCvar_t cg_useDefaultTeamSkins;
extern vmCvar_t cg_teamModel;
extern vmCvar_t cg_teamHeadSkin;
extern vmCvar_t cg_teamTorsoSkin;
extern vmCvar_t cg_teamLegsSkin;
extern vmCvar_t cg_teamHeadColor;
extern vmCvar_t cg_teamTorsoColor;
extern vmCvar_t cg_teamLegsColor;
extern vmCvar_t cg_teamRailColor1;
extern vmCvar_t cg_teamRailColor2;
extern vmCvar_t cg_teamRailColor1Team;
extern vmCvar_t cg_teamRailColor2Team;
extern vmCvar_t cg_teamRailItemColor;
extern vmCvar_t cg_teamRailItemColorTeam;
//extern vmCvar_t cg_teamOldRail;
extern vmCvar_t cg_teamRailRings;
extern vmCvar_t cg_teamRailNudge;

extern vmCvar_t cg_ourModel;

extern vmCvar_t cg_deadBodyColor;

extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_chatBeep;
extern vmCvar_t cg_chatBeepMaxTime;
extern vmCvar_t cg_teamChatBeep;
extern vmCvar_t cg_teamChatBeepMaxTime;
extern vmCvar_t cg_audioAnnouncer;
extern vmCvar_t cg_audioAnnouncerRewards;
extern vmCvar_t cg_audioAnnouncerRound;
extern vmCvar_t cg_audioAnnouncerWarmup;
extern vmCvar_t cg_audioAnnouncerVote;
extern vmCvar_t cg_audioAnnouncerTeamVote;
extern vmCvar_t cg_audioAnnouncerFlagStatus;
extern vmCvar_t cg_audioAnnouncerLead;
extern vmCvar_t cg_audioAnnouncerTimeLimit;
extern vmCvar_t cg_audioAnnouncerFragLimit;
extern vmCvar_t cg_audioAnnouncerWin;
extern vmCvar_t cg_audioAnnouncerScore;
extern vmCvar_t cg_audioAnnouncerLastStanding;
extern vmCvar_t cg_audioAnnouncerDominationPoint;

extern vmCvar_t wolfcam_drawFollowing;
extern vmCvar_t wolfcam_drawFollowingOnlyName;

extern vmCvar_t cg_printTimeStamps;
extern vmCvar_t cg_screenDamageAlpha_Team;
extern vmCvar_t cg_screenDamage_Team;
extern vmCvar_t cg_screenDamageAlpha_Self;
extern vmCvar_t cg_screenDamage_Self;
extern vmCvar_t cg_screenDamageAlpha;
extern vmCvar_t cg_screenDamage;
extern vmCvar_t cg_echoPopupTime;
extern vmCvar_t cg_echoPopupX;
extern vmCvar_t cg_echoPopupY;
extern vmCvar_t cg_echoPopupScale;
extern vmCvar_t cg_accX;
extern vmCvar_t cg_accY;
extern vmCvar_t cg_loadDefaultMenus;
extern vmCvar_t cg_grenadeColor;
extern vmCvar_t cg_grenadeColorAlpha;
extern vmCvar_t cg_grenadeTeamColor;
extern vmCvar_t cg_grenadeTeamColorAlpha;
extern vmCvar_t cg_grenadeEnemyColor;
extern vmCvar_t cg_grenadeEnemyColorAlpha;

extern vmCvar_t cg_fragMessageStyle;
extern vmCvar_t cg_drawFragMessageSeparate;
extern vmCvar_t cg_drawFragMessageX;
extern vmCvar_t cg_drawFragMessageY;
extern vmCvar_t cg_drawFragMessageAlign;
extern vmCvar_t cg_drawFragMessageStyle;
extern vmCvar_t cg_drawFragMessageFont;
extern vmCvar_t cg_drawFragMessagePointSize;
extern vmCvar_t cg_drawFragMessageScale;
extern vmCvar_t cg_drawFragMessageTime;
extern vmCvar_t cg_drawFragMessageColor;
extern vmCvar_t cg_drawFragMessageAlpha;
extern vmCvar_t cg_drawFragMessageFade;
extern vmCvar_t cg_drawFragMessageFadeTime;
extern vmCvar_t cg_drawFragMessageTokens;
extern vmCvar_t cg_drawFragMessageTeamTokens;
extern vmCvar_t cg_drawFragMessageThawTokens;
extern vmCvar_t cg_drawFragMessageFreezeTokens;
extern vmCvar_t cg_drawFragMessageFreezeTeamTokens;
extern vmCvar_t cg_drawFragMessageIconScale;

extern vmCvar_t cg_obituaryTokens;
extern vmCvar_t cg_obituaryIconScale;
extern vmCvar_t cg_obituaryRedTeamColor;
extern vmCvar_t cg_obituaryBlueTeamColor;
extern vmCvar_t cg_obituaryTime;
extern vmCvar_t cg_obituaryFadeTime;

extern vmCvar_t cg_fragTokenAccuracyStyle;

extern vmCvar_t cg_drawPlayerNames;
//extern vmCvar_t cg_drawPlayerNamesX;
extern vmCvar_t cg_drawPlayerNamesStyle;
extern vmCvar_t cg_drawPlayerNamesY;
extern vmCvar_t cg_drawPlayerNamesFont;
extern vmCvar_t cg_drawPlayerNamesPointSize;
extern vmCvar_t cg_drawPlayerNamesScale;
extern vmCvar_t cg_drawPlayerNamesColor;
extern vmCvar_t cg_drawPlayerNamesAlpha;

extern vmCvar_t cg_perKillStatsExcludePostKillSpam;
extern vmCvar_t cg_perKillStatsClearNotFiringTime;
extern vmCvar_t cg_perKillStatsClearNotFiringExcludeSingleClickWeapons;

extern vmCvar_t cg_printSkillRating;

extern vmCvar_t cg_lightningImpact;
extern vmCvar_t cg_lightningImpactSize;
extern vmCvar_t cg_lightningImpactOthersSize;
extern vmCvar_t cg_lightningImpactCap;
extern vmCvar_t cg_lightningImpactCapMin;
extern vmCvar_t cg_lightningImpactProject;
extern vmCvar_t cg_lightningStyle;
extern vmCvar_t cg_lightningRenderStyle;
extern vmCvar_t cg_lightningAngleOriginStyle;
extern vmCvar_t cg_debugLightningImpactDistance;
extern vmCvar_t cg_lightningSize;

extern vmCvar_t cg_drawEntNumbers;
extern vmCvar_t cg_drawEventNumbers;
extern vmCvar_t cg_demoSmoothing;
extern vmCvar_t cg_drawCameraPath;

extern vmCvar_t cg_drawCameraPointInfo;
extern vmCvar_t cg_drawCameraPointInfoX;
extern vmCvar_t cg_drawCameraPointInfoY;
extern vmCvar_t cg_drawCameraPointInfoAlign;
extern vmCvar_t cg_drawCameraPointInfoStyle;
extern vmCvar_t cg_drawCameraPointInfoFont;
extern vmCvar_t cg_drawCameraPointInfoPointSize;
extern vmCvar_t cg_drawCameraPointInfoScale;
extern vmCvar_t cg_drawCameraPointInfoColor;
extern vmCvar_t cg_drawCameraPointInfoSelectedColor;
extern vmCvar_t cg_drawCameraPointInfoAlpha;

extern vmCvar_t cg_drawViewPointMark;

extern vmCvar_t cg_levelTimerDirection;
//extern vmCvar_t cg_levelTimerStyle;
extern vmCvar_t cg_levelTimerDefaultTimeLimit;
extern vmCvar_t cg_checkForOfflineDemo;

extern vmCvar_t cg_muzzleFlash;

extern vmCvar_t cg_weaponDefault;
extern vmCvar_t cg_weaponNone;
extern vmCvar_t cg_weaponGauntlet;
extern vmCvar_t cg_weaponMachineGun;
extern vmCvar_t cg_weaponShotgun;
extern vmCvar_t cg_weaponGrenadeLauncher;
extern vmCvar_t cg_weaponRocketLauncher;
extern vmCvar_t cg_weaponLightningGun;
extern vmCvar_t cg_weaponRailGun;
extern vmCvar_t cg_weaponPlasmaGun;
extern vmCvar_t cg_weaponBFG;
extern vmCvar_t cg_weaponGrapplingHook;
extern vmCvar_t cg_weaponNailGun;
extern vmCvar_t cg_weaponProximityLauncher;
extern vmCvar_t cg_weaponChainGun;

extern vmCvar_t cg_spawnArmorTime;
extern vmCvar_t cg_fxfile;
extern vmCvar_t cg_fxinterval;
extern vmCvar_t cg_fxratio;
extern vmCvar_t cg_fxScriptMinEmitter;
extern vmCvar_t cg_fxScriptMinDistance;
extern vmCvar_t cg_fxScriptMinInterval;
extern vmCvar_t cg_fxLightningGunImpactFps;
extern vmCvar_t cg_vibrate;
extern vmCvar_t cg_vibrateTime;
extern vmCvar_t cg_vibrateMaxDistance;
extern vmCvar_t cg_vibrateForce;

extern vmCvar_t cg_animationsIgnoreTimescale;
extern vmCvar_t cg_animationsRate;

extern vmCvar_t cg_quadFireSound;
extern vmCvar_t cg_kickScale;

// referenced in menu files with cvarTest
extern vmCvar_t cg_gameType;
extern vmCvar_t cg_compMode;
extern vmCvar_t cg_drawSpecMessages;
extern vmCvar_t g_training;

extern vmCvar_t cg_waterWarp;
extern vmCvar_t cg_allowLargeSprites;
extern vmCvar_t cg_allowSpritePassThrough;
extern vmCvar_t cg_drawSprites;
extern vmCvar_t cg_drawSpriteSelf;
extern vmCvar_t cg_playerLeanScale;
extern vmCvar_t cg_cameraRewindTime;
extern vmCvar_t cg_cameraQue;
extern vmCvar_t cg_cameraAddUsePreviousValues;
extern vmCvar_t cg_cameraUpdateFreeCam;
extern vmCvar_t cg_cameraDefaultOriginType;
extern vmCvar_t cg_cameraDebugPath;
extern vmCvar_t cg_cameraSmoothFactor;

extern vmCvar_t cg_flightTrail;
extern vmCvar_t cg_hasteTrail;

extern vmCvar_t cg_noItemUseMessage;
extern vmCvar_t cg_noItemUseSound;
extern vmCvar_t cg_itemUseMessage;
extern vmCvar_t cg_itemUseSound;

extern vmCvar_t cg_localTime;
extern vmCvar_t cg_localTimeStyle;
extern vmCvar_t cg_warmupTime;

extern vmCvar_t cg_clientOverrideIgnoreTeamSettings;
extern vmCvar_t cg_killBeep;
extern vmCvar_t cg_deathStyle;
extern vmCvar_t cg_deathShowOwnCorpse;
extern vmCvar_t cg_mouseSeekScale;
extern vmCvar_t cg_mouseSeekPollInterval;
extern vmCvar_t cg_mouseSeekUseTimescale;

extern vmCvar_t cg_teamKillWarning;
extern vmCvar_t cg_inheritPowerupShader;
extern vmCvar_t cg_fadeColor;
extern vmCvar_t cg_fadeAlpha;
extern vmCvar_t cg_fadeStyle;
extern vmCvar_t cg_enableAtCommands;
extern vmCvar_t cg_quadKillCounter;
extern vmCvar_t cg_battleSuitKillCounter;
extern vmCvar_t cg_wideScreen;
extern vmCvar_t cg_wideScreenScoreBoardHack;

extern vmCvar_t cg_wh;
extern vmCvar_t cg_whShader;

extern vmCvar_t cg_playerShader;
extern vmCvar_t cg_adShaderOverride;

extern vmCvar_t cg_firstPersonSwitchSound;
extern vmCvar_t cg_proxMineTick;

extern vmCvar_t cg_drawProxWarning;
extern vmCvar_t cg_drawProxWarningX;
extern vmCvar_t cg_drawProxWarningY;
extern vmCvar_t cg_drawProxWarningAlign;
extern vmCvar_t cg_drawProxWarningStyle;
extern vmCvar_t cg_drawProxWarningFont;
extern vmCvar_t cg_drawProxWarningPointSize;
extern vmCvar_t cg_drawProxWarningScale;
extern vmCvar_t cg_drawProxWarningColor;
extern vmCvar_t cg_drawProxWarningAlpha;

extern vmCvar_t cg_customMirrorSurfaces;
extern vmCvar_t cg_demoStepSmoothing;
extern vmCvar_t cg_stepSmoothTime;
extern vmCvar_t cg_stepSmoothMaxChange;
extern vmCvar_t cg_pathRewindTime;
extern vmCvar_t cg_pathSkipNum;
extern vmCvar_t cg_dumpEntsUseServerTime;

extern vmCvar_t cg_playerModelForceScale;
extern vmCvar_t cg_playerModelForceLegsScale;
extern vmCvar_t cg_playerModelForceTorsoScale;
extern vmCvar_t cg_playerModelForceHeadScale;
extern vmCvar_t cg_playerModelForceHeadOffset;
extern vmCvar_t cg_playerModelAutoScaleHeight;
extern vmCvar_t cg_playerModelAllowServerScale;
extern vmCvar_t cg_playerModelAllowServerScaleDefault;
extern vmCvar_t cg_playerModelAllowServerOverride;

extern vmCvar_t cg_powerupLight;
extern vmCvar_t cg_buzzerSound;
extern vmCvar_t cg_flagStyle;
extern vmCvar_t cg_helpIconStyle;
extern vmCvar_t cg_helpIconMinWidth;
extern vmCvar_t cg_helpIconMaxWidth;
//extern vmCvar_t cg_helpIconAlpha;
extern vmCvar_t cg_dominationPointTeamColor;
extern vmCvar_t cg_dominationPointTeamAlpha;
extern vmCvar_t cg_dominationPointEnemyColor;
extern vmCvar_t cg_dominationPointEnemyAlpha;
extern vmCvar_t cg_dominationPointNeutralColor;
extern vmCvar_t cg_dominationPointNeutralAlpha;
extern vmCvar_t cg_attackDefendVoiceStyle;

extern vmCvar_t cg_drawDominationPointStatus;
extern vmCvar_t cg_drawDominationPointStatusX;
extern vmCvar_t cg_drawDominationPointStatusY;
extern vmCvar_t cg_drawDominationPointStatusFont;
extern vmCvar_t cg_drawDominationPointStatusPointSize;
extern vmCvar_t cg_drawDominationPointStatusScale;
extern vmCvar_t cg_drawDominationPointStatusEnemyColor;
extern vmCvar_t cg_drawDominationPointStatusTeamColor;
extern vmCvar_t cg_drawDominationPointStatusBackgroundColor;
extern vmCvar_t cg_drawDominationPointStatusAlpha;
extern vmCvar_t cg_drawDominationPointStatusTextColor;
extern vmCvar_t cg_drawDominationPointStatusTextAlpha;
extern vmCvar_t cg_drawDominationPointStatusTextStyle;

extern vmCvar_t cg_roundScoreBoard;

// end cvar_t

//
// cg_main.c
//
qboolean CG_ConfigStringIndexToQ3 (int *index);
qboolean CG_ConfigStringIndexFromQ3 (int *index);
const char *CG_ConfigString( int index );
const char *CG_ConfigStringNoConvert (int index);
const char *CG_Argv( int arg );
int CG_Argc (void);
const char *CG_ArgsFrom (int arg);

void QDECL CG_PrintToScreen( const char *msg, ... );
void QDECL CG_Printf( const char *msg, ... );
void QDECL CG_Error( const char *msg, ... );

void CG_StartMusic( void );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_LoadMenus(const char *menuFile);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
score_t *CG_GetSelectedScore(void);
void CG_BuildSpectatorString(void);


//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
void CG_AddBufferedSound( sfxHandle_t sfx);

void CG_DrawActiveFrame (int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, qboolean videoRecording, int ioverf, qboolean draw);


//
// cg_drawtools.c
//
void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawStretchPic (float x, float y, float width, float height, float s1, float t1, float s2, float t2, qhandle_t hShader);
void CG_DrawString( float x, float y, const char *string,
				   float charWidth, float charHeight, const float *modulate );


void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
					   qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars, fontInfo_t *font );
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str, fontInfo_t *font );

float	*CG_FadeColor( int startMsec, int totalMsec );
void  CG_FadeColorVec4 (vec4_t color, int startMsec, int totalMsec, int fadeTimeMsec);
float *CG_FadeColorRealTime (int startMsec, int totalMsec);
float *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);


//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_LagometerMarkNoMove (void);
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_CenterPrintFragMessage (const char *str, int y, int charWidth);
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int ownerDrawFlags2, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle, int fontIndex);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font);
void CG_Text_Pic_Paint (float x, float y, float scale, vec4_t color, int *text, float adjust, int limit, int style, fontInfo_t *fontOrig, int textHeight, float iconScale);
void CG_CreateNameSprites (void);
void CG_CreateNameSprite (float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *fontOrig, qhandle_t h, int imageWidth, int imageHeight);
void CG_Text_Paint_Bottom (float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font);
void CG_Text_Paint_old(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int fontIndex);
void CG_Text_Paint_Align (rectDef_t *rect, float scale, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *fontOrig, int align);
int CG_Text_Width(const char *text, float scale, int limit, fontInfo_t *font);
int CG_Text_Width_old(const char *text, float scale, int limit, int fontIndex);
int CG_Text_Height(const char *text, float scale, int limit, fontInfo_t *font);
int CG_Text_Height_old(const char *text, float scale, int limit, int fontIndex);
void CG_SelectPrevPlayer(void);
void CG_SelectNextPlayer(void);
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags, int flags2);
void CG_RunMenuScript(char **args);
void CG_ShowResponseHead(void);
void CG_SetPrintString(int type, const char *p);
void CG_InitTeamChat(void);
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText (vec4_t color);
const char *CG_GetKillerText(void);
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles );
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_Text_PaintCharScale (float x, float y, float width, float height, float xscale, float yscale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_CheckOrderPending(void);
const char *CG_GameTypeString(void);
qboolean CG_YourTeamHasFlag(void);
qboolean CG_OtherTeamHasFlag(void);
qhandle_t CG_StatusHandle(int task);



//
// cg_player.c
//
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team );
void CG_NewClientInfo( int clientNum );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover (const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, float subTime);

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );



//
// cg_weapons.c
//
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );

void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent );
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean knowClientNum );
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum, int shooterClientNum );
void CG_ImpactSparks( int weapon, vec3_t origin, vec3_t dir, int entityNum );
void CG_ShotgunFire( entityState_t *es );
void CG_Bullet( vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum );

void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_SimpleRailTrail (vec3_t start, vec3_t end, byte color[4]);
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team );
void CG_DrawWeaponSelect( void );
void CG_DrawWeaponBar (void);

void CG_OutOfAmmoChange( void );	// should this be in pmove?

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float r, float g, float b, float a,
					qboolean alphaFade,
					   float radius, qboolean temporary, qboolean energy, qboolean debug );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff_ql( const vec3_t p, const vec3_t vel,
								float radius,
								float r, float g, float b, float a,
								float duration,
								int startTime,
								int fadeInTime,
								int leFlags,
								qhandle_t hShader );
localEntity_t *CG_SmokePuff( const vec3_t p,
				   const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing );
void CG_SpawnEffect( vec3_t org );
#if 1  //def MPACK
void CG_KamikazeEffect( vec3_t org );
void CG_ObeliskExplode( vec3_t org, int entityNum );
void CG_ObeliskPain( vec3_t org );
void CG_InvulnerabilityImpact( vec3_t org, vec3_t angles );
void CG_InvulnerabilityJuiced( vec3_t org );
void CG_LightningBoltBeam( vec3_t start, vec3_t end );
#endif
void CG_ScorePlum( int client, vec3_t org, int score );

void CG_GibPlayer(centity_t *cent);
void CG_ThawPlayer (centity_t *cent);
void CG_BigExplode( vec3_t playerOrigin );

void CG_Bleed( vec3_t origin, int entityNum );

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, qhandle_t shader, int msec,
								qboolean isSprite );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation (qboolean loading);

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo (qboolean firstCall);
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);
#ifdef MISSIONPACK
void CG_LoadVoiceChats( void );
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );
#endif

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );


//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *fmt );

// abort the game
void		trap_Error( const char *fmt );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );
void trap_SendConsoleCommandNow(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points,
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void	trap_S_StopBackgroundTrack( void );
int trap_RealTime (qtime_t *qtime, qboolean now, int convertTime);

void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderLightMap( const char *name, int lightmap );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int lightmap );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
					   float frac, const char *tagName );
void trap_R_RemapShader (const char *oldShader, const char *newShader, const char *timeOffset, qboolean keepLightmap, qboolean userSet);
void trap_R_ClearRemappedShader (const char *shaderName);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned separately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean	trap_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale );

// aids for VM testing
void		testPrintInt( char *string, int i );
void		testPrintFloat( char *string, float f );

int			trap_MemoryRemaining( void );
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );
void		trap_Key_GetBinding(int keynum, char *buffer);
void		trap_Get_Advertisements(int *num, float *verts, char shaders[][MAX_QPATH]);
void		trap_R_DrawConsoleLines (void);
int			trap_GetLastExecutedServerCommand (void);
qboolean trap_GetNextKiller (int us, int serverTime, int *killer, int *foundServerTime, qboolean onlyOtherClient);
qboolean trap_GetNextVictim (int us, int serverTime, int *victim, int *foundServerTime, qboolean onlyOtherClient);
void trap_ReplaceShaderImage (qhandle_t h, const ubyte *data, int width, int height);
qhandle_t trap_RegisterShaderFromData (const char *name, const ubyte *data, int width, int height, qboolean mipmap, qboolean allowPicmip, int wrapClampMode, int lightmapIndex);
void trap_GetShaderImageDimensions (qhandle_t h, int *width, int *height);
void trap_GetShaderImageData (qhandle_t h, ubyte *data);
void trap_CalcSpline (int step, float tension, float *out);
qhandle_t trap_R_GetSingleShader (void);

typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t; // bk001201 - warning: useless keyword or type name in empty declaration


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

void trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles, float *fov);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

int trap_PC_LoadSource( const char *filename );
int trap_PC_FreeSource( int handle );
int trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int trap_PC_AddGlobalDefine( char *define );
void trap_SetPathLines (int *numCameraPoints, cameraPoint_t *cameraPoints, int *numSplinePoints, vec3_t *splinePoints, vec4_t color);
int trap_GetGameStartTime (void);
int trap_GetGameEndTime (void);
int trap_GetFirstServerTime (void);
void trap_AddAt (int serverTime, const char *clockTime, const char *command);
int trap_GetLegsAnimStartTime (int entityNum);
int trap_GetTorsoAnimStartTime (int entityNum);
void trap_autoWriteConfig (qboolean write);
int trap_GetItemPickupNumber (int pickupTime);
int trap_GetItemPickup (int pickupNumber, itemPickup_t *ip);

void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
extern qboolean		initparticles;
int CG_NewParticleArea ( int num );
qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName, qboolean dontForceTeamSkin );
void CG_ResetTimedItemPickupTimes (void);

qboolean CG_ParseSpawnVars( void );
void CG_LoadCustomHud (void);
void CG_ParseMenu (const char *menuFile);
void CG_LightningBolt( centity_t *cent, vec3_t origin );
void CG_LoadDefaultMenus (void);
float CG_Cvar_Get (const char *cvar);
void CG_ResetEntity( centity_t *cent );

void CG_StartCamera(const char *name, qboolean startBlack);
void CG_Fade( int a, int time, int duration );
void CG_DrawFlashFade( void );
void CG_ParseWarmup (void);
void CG_PrintEntityState (int n);
void CG_PrintEntityStatep (entityState_t *ent);
void CG_CheckFontUpdates (void);
void CG_CreateScoresFromClientInfo (void);
int CG_ModToWeapon (int mod);
int *CG_CreateFragString (qboolean lastFrag);
void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to );
qboolean CG_IsEnemy (clientInfo_t *ci);
qboolean CG_IsTeammate (clientInfo_t *ci);
qboolean CG_IsUs (clientInfo_t *ci);
qboolean CG_IsTeamGame (int gametype);
void CG_GetModelName (char *s, char *model);
void CG_GetModelAndSkinName (char *s, char *model, char *skin);
void CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit, fontInfo_t *font);
void CG_Text_Paint_Limit_Bottom(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit, fontInfo_t *font);
int CG_Text_Length (const char *s);
qhandle_t CG_RegisterSkinVertexLight (const char *name);
void CG_ResetLagometer (void);
void CG_FloatEntNumbers (void);
void CG_FloatNumber (int n, vec3_t origin, int renderfx, byte *color, float scale);
void CG_FloatNumberExt (int n, vec3_t origin, int renderfx, byte *color, int time);
void CG_FloatSprite (qhandle_t shader, vec3_t origin, int renderfx, byte *color, int radius);
void CG_AdjustOriginToAvoidSolid (vec3_t origin, centity_t *cent);
float CG_CalcSpline (int step, float tension);
//void CG_CreateSplinePoints (void);
void CG_UpdateCameraInfo (void);
void CG_TimeChange (int serverTime, int ioverf);
void CG_CleanUpFieldNumber (void);
void CG_EchoPopup (const char *s, int x, int y);
void CG_ErrorPopup (const char *s);
int CG_GetCurrentTimeWithDirection (void);
const char *CG_GetLevelTimerString (void);
double CG_GetServerTimeFromClockString (const char *timeString);
int CG_NumOverTimes (void);
void CG_Q3ColorFromString (const char *v, vec3_t color);
void CG_CpmaColorFromString (const char *v, vec3_t color);
void CG_OspColorFromString (const char *v, vec3_t color);
void CG_OspColorFromIntString (int *v, vec3_t color);
//void CG_ParseQ3mmeScripts (const char *fileName);
qboolean Q_Isfreeze (int clientNum);
void CG_PlayerFloatSprite (centity_t *cent, qhandle_t shader);
//void CG_RunQ3mmeFlashScript(weaponInfo_t *weapon, float dlight[3], byte shaderRGBA[4], float *flashSize);
qboolean CG_FileExists (const char *filename);
void CG_StripSlashComments (char *s);
char *CG_Q3mmeMath (char *script, float *val, int *error);
// returns whether to stop the rest of the script
qboolean CG_RunQ3mmeScript (char *script, char *emitterEnd);
void CG_ReloadQ3mmeScripts (const char *fileName);
void CG_GetStoredScriptVarsFromLE (localEntity_t *le);
void CG_FX_GibPlayer (centity_t *cent);
void CG_FX_ThawPlayer (centity_t *cent);
void CG_RemoveFXLocalEntities (qboolean all, float emitterId);
void CG_ResetScriptVars (void);
void CG_CopyPlayerDataToScriptData (centity_t *cent);
void CG_CopyStaticScriptDataToCent (centity_t *cent);
void CG_CopyStaticCentDataToScript (centity_t *cent);
void CG_ResetTimeChange (int serverTime, int ioverf);
void CG_Abort (void);
void CG_GetWeaponFlashOrigin (int clientNum, vec3_t startPoint);
void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] );
void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						 int *torsoOld, int *torso, float *torsoBackLerp );
void CG_SetDuelPlayers (void);
void CG_ClearLerpFrame (clientInfo_t *ci, lerpFrame_t *lf, int animationNumber , int time);
void CG_PrintPlayerState (void);
qboolean CG_CheckQlVersion (int n0, int n1, int n2, int n3);
void CG_CalcVrect (void);
void CG_LoadClientInfo (clientInfo_t *ci, int clientNum, qboolean dontForceTeamSkin);
void CG_SetLerpFrameAnimation (clientInfo_t *ci, lerpFrame_t *lf, int newAnimation);
void CG_SetAnimFrame (lerpFrame_t *lf, int time, float speedScale);
void CG_RunLerpFrame (clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale, int time);
void CG_AddChatLine (const char *line);
void CG_ChangeConfigString (const char *buffer, int index);
void CG_TimedItemPickup (int index, vec3_t origin, int clientNum, int time, qboolean spec);
double CG_CalcZoom (double fov_x);
qboolean CG_FindClientHeadFile (char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext, qboolean dontForceTeamSkin);
qboolean CG_GetSnapshot (int snapshotNumber, snapshot_t *snapshot);
qboolean CG_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot);
int CG_CheckClientEventCpma (int clientNum, entityState_t *es);
float CameraAngleDistance (vec3_t a0, vec3_t a1);
void CG_InterMissionHit (void);
void CG_CreateNewCrosshairs (void);
qboolean CG_EntityFrozen (centity_t *cent);

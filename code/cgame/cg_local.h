#ifndef cg_local_h_included
#define cg_local_h_included

// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
//#include "sc.h"
#include "../qcommon/qfiles.h"  // MAX_MAP_ADVERTISEMENTS
#include "../ui/ui_shared.h"

#include "cg_camera.h"
//#include "cg_q3mme_scripts.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define DEFAULT_FOV 110

#define MAX_RAWCAMERAPOINTS 32768  //4192
#define MAX_TIMED_ITEMS 16   //FIXME
#define MAX_CHAT_LINES  100
#define MAX_OBITUARIES 100
#define MAX_SPAWN_POINTS 128  //FIXME from g_client.c
#define MAX_MIRROR_SURFACES MAX_GENTITIES
#define DEFAULT_RECORD_PATHNAME "path"

//FIXME from game/g_local.h
#define       MAX_SPAWN_VARS                  64
#define       MAX_SPAWN_VARS_CHARS    4096

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
#define	MAX_MARK_POLYS		4096 //256

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

#define TEAM_COLOR_SKIN "-teamColor"

#define MAX_EVENT_FILTER 1024
#define ICON_SCALE_DISTANCE 200.0


///////////////////////////////////////////////

//FIXME from menudef.h

#define ITEM_TEXTSTYLE_NORMAL 0
#define ITEM_TEXTSTYLE_BLINK 1
#define ITEM_TEXTSTYLE_PULSE 2
#define ITEM_TEXTSTYLE_SHADOWED 3
#define ITEM_TEXTSTYLE_OUTLINED 4
#define ITEM_TEXTSTYLE_OUTLINESHADOWED 5
#define ITEM_TEXTSTYLE_SHADOWEDMORE 6

////////////////////////////////

#define AMMO_WARNING_OK 0
#define AMMO_WARNING_LOW 1
#define AMMO_WARNING_EMPTY 2

#define DUEL_PLAYER_INVALID -1

#define MAX_RACE_SCORE 0x7fffffff
#define CROSSHAIR_CLIENT_INVALID -1

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

	int raceStartTime;
	int raceCheckPointNum;
	int raceCheckPointNextEnt;
	int raceCheckPointTime;
	int raceEndTime;

	int deathTime;
	int lastQuadSoundTime;
} playerEntity_t;

//=================================================

typedef struct positionData_s {
	vec3_t intervalPosition;
	double intervalTime;

	vec3_t distancePosition;
	double distanceTime;
} positionData_t;

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

#if 1  //FIXME switch to struct
	// q3mme fx scripting
	vec3_t lastFlashIntervalPosition;
	double lastFlashIntervalTime;
	vec3_t lastFlashDistancePosition;
	double lastFlashDistanceTime;

	vec3_t lastModelIntervalPosition;  // also powerups
	double lastModelIntervalTime;
	vec3_t lastModelDistancePosition;
	double lastModelDistanceTime;

	vec3_t lastTrailIntervalPosition;
	double lastTrailIntervalTime;
	vec3_t lastTrailDistancePosition;
	double lastTrailDistanceTime;

	vec3_t lastImpactIntervalPosition;
	double lastImpactIntervalTime;
	vec3_t lastImpactDistancePosition;
	double lastImpactDistanceTime;

	vec3_t lastLegsIntervalPosition;
	double lastLegsIntervalTime;
	vec3_t lastLegsDistancePosition;
	double lastLegsDistanceTime;

	vec3_t lastTorsoIntervalPosition;
	double lastTorsoIntervalTime;
	vec3_t lastTorsoDistancePosition;
	double lastTorsoDistanceTime;

	vec3_t lastHeadIntervalPosition;
	double lastHeadIntervalTime;
	vec3_t lastHeadDistancePosition;
	double lastHeadDistanceTime;

#endif

	positionData_t flashPositionData;
	positionData_t modelPositionData;
	positionData_t trailPositionData;

	positionData_t flightPositionData;
	positionData_t hastePositionData;

	int lastPointContents;

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

	qhandle_t extraShader;
	double extraShaderEndTime;

} centity_t;

typedef struct {
	int startTime;
	qhandle_t shader;
	sfxHandle_t sfx;
} clientRewards_t;


//======================================================================


//======================================================================


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
	char modelString[MAX_QPATH];
	char headString[MAX_QPATH];
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
	qhandle_t		legsTeamSkinAlt;
	qhandle_t		legsEnemySkinAlt;
	qhandle_t legsOurSkinAlt;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	qhandle_t		torsoTeamSkinAlt;
	qhandle_t		torsoEnemySkinAlt;
	qhandle_t torsoOurSkinAlt;

	qhandle_t		headModel;
	qhandle_t		headSkin;
	qhandle_t setHeadSkin;
	qhandle_t		headTeamSkinAlt;
	qhandle_t		headEnemySkinAlt;
	qhandle_t headOurSkinAlt;

	qhandle_t headModelFallback;
	qhandle_t headSkinFallback;

	qhandle_t		modelIcon;
	qhandle_t setModelIcon;  // model and skin player selected without team substitution

	animation_t		animations[MAX_TOTALANIMATIONS];

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	qboolean knowSkillRating;
	int	skillRating;

	qboolean spectatorOnly;  // not in que to play
	int quePosition;

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
	char fullClanName[MAX_STRING_CHARS];

	qboolean premiumSubscriber;
	float playerModelHeight;
	char steamId[MAX_STRING_CHARS];

	int scoreIndexNum;
	qboolean scoreValid;

	//double hitTime;

	qboolean hasTeamInfo;
	clientRewards_t clientRewards;

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

	void			(*ejectBrassFunc)( const centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
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
	int				client;
	int				score;
	int roundScore;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerups;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				gauntletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	int 		perfect;
	int				team;

	qboolean		alive;
	int				frags;
	int				deaths;
	int				bestWeapon;
	int bestWeaponAccuracy;

	int damageDone;
	int net;

	//qboolean		fake;  // filled in for demo playback
} score_t;

typedef struct {
	int hits;
	int atts;
	int accuracy;
	int damage;
	int kills;

	// cpma
	int deaths;
	int take;
	int drop;
} duelWeaponStats_t;

typedef struct {

	int clientNum;
	//char name[MAX_QPATH * 2];
	clientInfo_t ci;

	int score;
	int ping;
	int time;

	int kills;
	int deaths;
	int accuracy;
	int bestWeapon;
	int damage;

	int awardExcellent;
	int awardImpressive;
	int awardHumiliation;
	qboolean perfect;

	int redArmorPickups;
	float redArmorTime;
	int yellowArmorPickups;
	float yellowArmorTime;
	int greenArmorPickups;
	float greenArmorTime;
	int megaHealthPickups;
	float megaHealthTime;

	duelWeaponStats_t weaponStats[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
} duelScore_t;

typedef struct {
	qboolean valid;
	int clientNum;
	int team;
	int subscriber;
	int score;
	int ping;
	int time;
	int kills;
	int deaths;
	int accuracy;
	int bestWeapon;
	int bestWeaponAccuracy;
	int awardImpressive;
	int awardExcellent;
	int awardHumiliation;
	qboolean perfect;
	int tks;
	int tkd;
	int thaws;
	int damageDone;
	qboolean alive;
} tdmPlayerScore_t;

typedef struct {
	qboolean valid;
	int serverTime;
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

	int numPlayerScores;
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
	int bfgAccuracy;
	int bfgKills;
	int grappleAccuracy;
	int grappleKills;
	int ngAccuracy;
	int ngKills;
	int plAccuracy;
	int plKills;
	int cgAccuracy;
	int cgKills;
	int hmgAccuracy;
	int hmgKills;
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
	qboolean subscriber;
	int score;
	int ping;
	int time;
	int kills;
	int deaths;
	int powerups;
	int accuracy;
	int bestWeapon;
	int awardImpressive;
	int awardExcellent;
	int awardHumiliation;
	int awardDefend;
	int awardAssist;
	int captures;
	int perfect;
	int alive;
} ctfPlayerScore_t;

typedef struct {
	qboolean valid;
	int serverTime;
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

	int numPlayerScores;
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

#define MAX_WHITELIST_AMBIENT_SOUNDS 128

#if 0
typedef struct {
	char *fileName;
	qhandle_t h;
} allowedAmbientSounds_t;
#endif

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

	snapshot_t smoothOldSnap;
	snapshot_t smoothNewSnap;

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
	qboolean scoresValid;
	int scoresVersion;
	int			selectedScore;
	int			teamScores[2];
	int avgRedPing;
	int avgBluePing;
	score_t		scores[MAX_CLIENTS];
	qboolean clientHasScore[MAX_CLIENTS];
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
	float centerPrintY;
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
	clientInfo_t enemyModelRed;
	clientInfo_t enemyModelBlue;
	clientInfo_t	teamModel;
	clientInfo_t	teamModelRed;
	clientInfo_t	teamModelBlue;
	clientInfo_t	ourModel;
	clientInfo_t	ourModelRed;
	clientInfo_t	ourModelBlue;
	clientInfo_t fallbackModel;
	clientInfo_t fallbackModelRed;
	clientInfo_t fallbackModelBlue;
	clientInfo_t serverModel;
	qboolean teamModelTeamSkinFound;
	qboolean teamModelTeamHeadSkinFound;
	qboolean enemyModelTeamSkinFound;
	qboolean enemyModelTeamHeadSkinFound;
	qboolean ourModelUsingTeamColorSkin;
	qboolean ourModelUsingTeamColorHeadSkin;

	byte			enemyColors[3][4];
	byte			teamColors[3][4];
	byte ourColors[3][4];

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

	obituary_t		obituaries[MAX_OBITUARIES];
	int obituaryIndex;

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
	int lastFragKillerTeam;
	int				lastFragWeapon;
	int				lastFragMod;
	char			lastFragq3obitString[MAX_STRING_CHARS];

	int				shotgunHits;
	//int				powerupRespawnSoundIndex;  // from config string
	//int				kamikazeRespawnSoundIndex;
	int proxTickSoundIndex;
	qhandle_t allowedAmbientSounds[MAX_WHITELIST_AMBIENT_SOUNDS];
	//allowedAmbientSounds_t allowedAmbientSounds[MAX_WHITELIST_AMBIENT_SOUNDS];
	int numAllowedAmbientSounds;

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
	float echoPopupX;
	float echoPopupY;
	int echoPopupWideScreen;
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
	cameraPoint_t *cameraPointsPointer;
	vec3_t splinePoints[MAX_SPLINEPOINTS];
	int	splinePointsCameraPoints[MAX_SPLINEPOINTS];
	int numSplinePoints;

	int numCameraPoints;
	int numQ3mmeCameraPoints;
	int selectedCameraPointMin;
	int selectedCameraPointMax;

	int selectedCameraPointField;

	qboolean cameraPlaying;
	qboolean cameraQ3mmePlaying;
	qboolean cameraPlayedLastFrame;
	int currentCameraPoint;
	qboolean atCameraPoint;
	qboolean cameraWaitToSync;
	qboolean cameraJustStarted;
	qboolean playCameraCommandIssued;
	qboolean playQ3mmeCameraCommandIssued;
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
	duelScore_t duelScores[MAX_CLIENTS * 2];  //FIXME can ql send more than 2?
	int numDuelScores;
	int duelScoresServerTime;
	qboolean duelScoresValid;
	int duelPlayer1;
	int duelPlayer2;
	qboolean duelForfeit;
	int duelPlayerForfeit;

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
	int numLocalEntities;

	qboolean fontsLoaded;
	qboolean menusLoaded;

	qboolean weaponDamagePlum[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
	int damagePlumModificationCount;

	int infectedSnapshotTime;
	int demoFov;

	// hack for area chat not using default font
	fontInfo_t notosansFont;

	int roundStarts[MAX_DEMO_ROUND_STARTS];
	int numRoundStarts;

	// hack to check if areanewchat is being drawn more than once during frame
	//int drawActiveFrameCount;

	qboolean playerKeyPressForward;
	qboolean playerKeyPressBack;
	qboolean playerKeyPressRight;
	qboolean playerKeyPressLeft;
	qboolean playerKeyPressFire;
	qboolean playerKeyPressCrouch;
	qboolean playerKeyPressJump;

} cg_t;


#define NUM_IMPRESSIVE_SOUNDS 3
#define NUM_EXCELLENT_SOUNDS 3
#define NUM_HUMILIATION_SOUNDS 3

#define NUM_REWARD_VARIATIONS 3

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	//qhandle_t	charsetShader;
	//fontInfo_t charsetFont;
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

	qhandle_t worldDeathIcon;

	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagModel2;
	qhandle_t	blueFlagModel2;
	qhandle_t	neutralFlagModel2;
	qhandle_t   neutralFlagModel3;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[5];

	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

#if 1  //def MPACK
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

	qhandle_t raceStart;
	qhandle_t raceCheckPoint;
	qhandle_t raceFinish;
	qhandle_t raceNav;
	qhandle_t raceWorldTimerHand;

	qhandle_t	armorModel;

	// old q3 code and client item timer icons
	qhandle_t	yellowArmorIcon;

	qhandle_t	redArmorIcon;
	qhandle_t	greenArmorIcon;
	qhandle_t	megaHealthIcon;
	qhandle_t quadIcon;
	qhandle_t battleSuitIcon;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;
	qhandle_t unavailableShader;

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
	qhandle_t friendHitShader;
	qhandle_t friendDeadShader;

	qhandle_t	frozenShader;
	qhandle_t foeShader;
	qhandle_t selfShader;
	qhandle_t selfEnemyShader;
	qhandle_t selfDemoTakerShader;
	qhandle_t selfDemoTakerEnemyShader;
	qhandle_t infectedFoeShader;

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
	// 2016-06-10 unused
	//qhandle_t	bloodTrailShader;
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
	// unused 2016-06-10
	//qhandle_t	bloodMarkShader;
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
	qhandle_t gooShader;
	qhandle_t	hastePuffShader;
#if 1  //def MPACK
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;
#endif
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

	qhandle_t wallHackShader;

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
	qhandle_t	invulnerabilityPowerupModel;
#endif

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
	qhandle_t headShotIcon;

	// new ql awards
	qhandle_t medalComboKill;
	qhandle_t medalDamage;
	qhandle_t medalFirstFrag;
	qhandle_t medalAccuracy;
	qhandle_t medalPerfect;
	qhandle_t medalPerforated;
	qhandle_t medalQuadGod;
	qhandle_t medalRampage;
	qhandle_t medalRevenge;
	qhandle_t medalHeadshot;
	qhandle_t medalTiming;
	qhandle_t medalMidAir;

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
	//sfxHandle_t	sfx_railg;
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
	sfxHandle_t kamikazeRespawnSound;
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
	sfxHandle_t impressiveSound[NUM_IMPRESSIVE_SOUNDS];
	sfxHandle_t excellentSound[NUM_EXCELLENT_SOUNDS];
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound[NUM_HUMILIATION_SOUNDS];
	sfxHandle_t assistSound;
	sfxHandle_t defendSound;
	sfxHandle_t firstImpressiveSound;
	sfxHandle_t firstExcellentSound;
	sfxHandle_t firstHumiliationSound;

	// new ql rewards
	sfxHandle_t comboKillRewardSound[NUM_REWARD_VARIATIONS];
	sfxHandle_t damageRewardSound;
	sfxHandle_t firstFragRewardSound;
	sfxHandle_t accuracyRewardSound;
	sfxHandle_t perfectRewardSound;
	sfxHandle_t perforatedRewardSound;
	sfxHandle_t quadGodRewardSound;
	sfxHandle_t rampageRewardSound[NUM_REWARD_VARIATIONS];
	sfxHandle_t revengeRewardSound[NUM_REWARD_VARIATIONS];
	sfxHandle_t timingRewardSound;
	sfxHandle_t midAirRewardSound[NUM_REWARD_VARIATIONS];
	sfxHandle_t headshotRewardSound;

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

#if 1  //def MPACK
	sfxHandle_t weaponHoverSound;
#endif

	// teamplay sounds
	sfxHandle_t captureAwardSound;
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;
	sfxHandle_t yourTeamScoredSound;
	sfxHandle_t enemyTeamScoredSound;
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
	sfxHandle_t yourFlagReturnedSound;
	sfxHandle_t enemyFlagReturnedSound;

#if 1  //def MPACK
	sfxHandle_t neutralFlagReturnedSound;
#endif
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t	youHaveFlagSound;
#if 1  //def MPACK
	sfxHandle_t     enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;
#endif
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
	sfxHandle_t countGoSound;
	sfxHandle_t countBiteSound;
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
	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;
#endif

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
	sfxHandle_t nightmareSound;
	sfxHandle_t survivorSound;
	sfxHandle_t bellSound;
	sfxHandle_t infectedSound;

	// don't actually load these
	fontInfo_t	giantchar;
	fontInfo_t	bigchar;
	fontInfo_t	smallchar;
	fontInfo_t	tinychar;

	fontInfo_t	qlfont12;
	fontInfo_t	qlfont16;
	//fontInfo_t qlfont20;
	fontInfo_t	qlfont24;
	fontInfo_t	qlfont48;

	//fontInfo_t q3font16;

	fontInfo_t	centerPrintFont;
	int centerPrintFontModificationCount;
	int centerPrintFontPointSizeModificationCount;
	fontInfo_t	fragMessageFont;
	int fragMessageFontModificationCount;
	int fragMessageFontPointSizeModificationCount;
	fontInfo_t rewardsFont;
	int rewardsFontModificationCount;
	int rewardsFontPointSizeModificationCount;
	fontInfo_t fpsFont;
	int fpsFontModificationCount;
	int fpsFontPointSizeModificationCount;
	fontInfo_t clientItemTimerFont;
	int clientItemTimerFontModificationCount;
	int clientItemTimerFontPointSizeModificationCount;
	fontInfo_t ammoWarningFont;
	int ammoWarningFontModificationCount;
	int ammoWarningFontPointSizeModificationCount;

	// such crap
	fontInfo_t playerNamesFont;
	int playerNamesFontModificationCount;
	int playerNamesStyleModificationCount;
	int playerNamesPointSizeModificationCount;
	int playerNamesColorModificationCount;
	int playerNamesAlphaModificationCount;

	fontInfo_t snapshotFont;
	int snapshotFontModificationCount;
	int snapshotFontPointSizeModificationCount;
	fontInfo_t crosshairNamesFont;
	int crosshairNamesFontModificationCount;
	int crosshairNamesFontPointSizeModificationCount;
	fontInfo_t crosshairTeammateHealthFont;
	int crosshairTeammateHealthFontModificationCount;
	int crosshairTeammateHealthFontPointSizeModificationCount;
	fontInfo_t warmupStringFont;
	int warmupStringFontModificationCount;
	int warmupStringFontPointSizeModificationCount;
	fontInfo_t waitingForPlayersFont;
	int waitingForPlayersFontModificationCount;
	int waitingForPlayersFontPointSizeModificationCount;
	fontInfo_t voteFont;
	int voteFontModificationCount;
	int voteFontPointSizeModificationCount;
	fontInfo_t teamVoteFont;
	int teamVoteFontModificationCount;
	int teamVoteFontPointSizeModificationCount;
	fontInfo_t followingFont;
	int followingFontModificationCount;
	int followingFontPointSizeModificationCount;
	fontInfo_t weaponBarFont;
	int weaponBarFontModificationCount;
	int weaponBarFontPointSizeModificationCount;
	fontInfo_t itemPickupsFont;
	int itemPickupsFontModificationCount;
	int itemPickupsFontPointSizeModificationCount;
	fontInfo_t originFont;
	int originFontModificationCount;
	int originFontPointSizeModificationCount;
	fontInfo_t speedFont;
	int speedFontModificationCount;
	int speedFontPointSizeModificationCount;
	fontInfo_t lagometerFont;
	int lagometerFontModificationCount;
	int lagometerFontPointSizeModificationCount;
	fontInfo_t attackerFont;
	int attackerFontModificationCount;
	int attackerFontPointSizeModificationCount;
	fontInfo_t teamOverlayFont;
	int teamOverlayFontModificationCount;
	int teamOverlayFontPointSizeModificationCount;
	fontInfo_t cameraPointInfoFont;
	int cameraPointInfoFontModificationCount;
	int cameraPointInfoFontPointSizeModificationCount;
	fontInfo_t jumpSpeedsFont;
	int jumpSpeedsFontModificationCount;
	int jumpSpeedsFontPointSizeModificationCount;
	fontInfo_t jumpSpeedsTimeFont;
	int jumpSpeedsTimeFontModificationCount;
	int jumpSpeedsTimeFontPointSizeModificationCount;
	fontInfo_t proxWarningFont;
	int proxWarningFontModificationCount;
	int proxWarningFontPointSizeModificationCount;
	fontInfo_t dominationPointStatusFont;
	int dominationPointStatusFontModificationCount;
	int dominationPointStatusFontPointSizeModificationCount;
	fontInfo_t damagePlumFont;
	int damagePlumFontModificationCount;
	int damagePlumFontPointSizeModificationCount;

	qhandle_t	gametypeIcon[GT_MAX_GAME_TYPE];
	qhandle_t singleShader;

	qhandle_t infiniteAmmo;
	qhandle_t premiumIcon;

	qhandle_t defragItemShader;
	qhandle_t weaplit;
	qhandle_t flagCarrier;
	qhandle_t flagCarrierNeutral;
	qhandle_t flagCarrierHit;
	qhandle_t ghostWeaponShader;
	qhandle_t noPlayerClipShader;
	qhandle_t rocketAimShader;
	qhandle_t bboxShader;
	qhandle_t bboxShader_nocull;

	qhandle_t silverKeyIcon;
	qhandle_t goldKeyIcon;
	qhandle_t masterKeyIcon;

	qhandle_t timerSlice5;
	qhandle_t timerSlice5Current;
	qhandle_t timerSlice7;
	qhandle_t timerSlice7Current;
	qhandle_t timerSlice12;
	qhandle_t timerSlice12Current;
	qhandle_t timerSlice24;
	qhandle_t timerSlice24Current;
	qhandle_t wcTimerSlice5;
	qhandle_t wcTimerSlice5Current;

	qhandle_t powerupIncoming;
	qhandle_t regenAvailable;
	qhandle_t quadAvailable;
	qhandle_t invisAvailable;
	qhandle_t hasteAvailable;
	qhandle_t bsAvailable;

	int activeCheckPointRaceFlagModel;

	qhandle_t mme_additiveWhiteShader;

	// announcer
	sfxHandle_t quadPickupVo;
	sfxHandle_t battleSuitPickupVo;
	sfxHandle_t hastePickupVo;
	sfxHandle_t invisibilityPickupVo;
	sfxHandle_t regenPickupVo;

	qhandle_t playerKeyPressForwardShader;
	qhandle_t playerKeyPressBackShader;
	qhandle_t playerKeyPressRightShader;
	qhandle_t playerKeyPressLeftShader;
	qhandle_t playerKeyPressMiscShader;
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
	//float			screenXBias;

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
	char realMapName[MAX_QPATH];
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
	qboolean isQuakeLiveBetaDemo;
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
	int realProtocol;
	int protocol;
	qboolean cpma;
	qboolean cpm;
	qboolean osp;
	qboolean ospEncrypt;
	qboolean defrag;
	//int cpmaTimeoutTime;
	int cpmaLastTe;
	int cpmaLastTd;

	int startRaceFlagModel;
	int endRaceFlagModel;
	int checkPointRaceFlagModel;

	int dominationControlPointModel;
	int dominationRedPoints;
	int dominationBluePoints;
	centity_t *dominationControlPointEnts[5];

	timeOut_t timeOuts[MAX_TIMEOUTS];
	int numTimeouts;

	int customServerSettings;
	clientInfo_t bonesInfected;
	clientInfo_t urielInfected;

	float rocketSpeed;

	gitem_t *redArmorItem;
	gitem_t *yellowArmorItem;
	gitem_t *greenArmorItem;

	qboolean armorTiered;

	int numberOfRaceCheckPoints;

	int numOverTimes;

} cgs_t;


//==============================================================================

extern displayContextDef_t cgDC;
extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES + 1];  //FIXME hack for viewEnt
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
//extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

extern int				EM_Loaded;

//extern	vmCvar_t		cg_centertime;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
extern	vmCvar_t		cg_swingSpeed;
extern	vmCvar_t		cg_shadows;
extern vmCvar_t cg_thawGibs;
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
extern vmCvar_t cg_drawClientItemTimerFilter;
extern	vmCvar_t		cg_drawClientItemTimerX;
extern	vmCvar_t		cg_drawClientItemTimerY;
extern	vmCvar_t		cg_drawClientItemTimerScale;
extern vmCvar_t cg_drawClientItemTimerTextColor;
extern	vmCvar_t		cg_drawClientItemTimerFont;
extern	vmCvar_t		cg_drawClientItemTimerPointSize;
extern vmCvar_t cg_drawClientItemTimerAlpha;
extern	vmCvar_t		cg_drawClientItemTimerStyle;
extern	vmCvar_t		cg_drawClientItemTimerAlign;
extern vmCvar_t			cg_drawClientItemTimerSpacing;
extern vmCvar_t cg_drawClientItemTimerIcon;
extern vmCvar_t cg_drawClientItemTimerIconSize;
extern vmCvar_t cg_drawClientItemTimerIconXoffset;
extern vmCvar_t cg_drawClientItemTimerIconYoffset;
extern vmCvar_t cg_drawClientItemTimerWideScreen;

extern vmCvar_t cg_itemSpawnPrint;

extern vmCvar_t cg_itemTimers;
extern vmCvar_t cg_itemTimersScale;
extern vmCvar_t cg_itemTimersOffset;
extern vmCvar_t cg_itemTimersAlpha;

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
extern vmCvar_t cg_drawFPSWideScreen;

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
extern vmCvar_t cg_drawSnapshotWideScreen;

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
extern vmCvar_t cg_drawAmmoWarningWideScreen;

extern vmCvar_t cg_lowAmmoWarningStyle;
extern vmCvar_t cg_lowAmmoWarningPercentile;
extern vmCvar_t cg_lowAmmoWarningSound;
extern vmCvar_t cg_lowAmmoWeaponBarWarning;
extern vmCvar_t cg_lowAmmoWarningGauntlet;
extern vmCvar_t cg_lowAmmoWarningMachineGun;
extern vmCvar_t cg_lowAmmoWarningShotgun;
extern vmCvar_t cg_lowAmmoWarningGrenadeLauncher;
extern vmCvar_t cg_lowAmmoWarningRocketLauncher;
extern vmCvar_t cg_lowAmmoWarningLightningGun;
extern vmCvar_t cg_lowAmmoWarningRailGun;
extern vmCvar_t cg_lowAmmoWarningPlasmaGun;
extern vmCvar_t cg_lowAmmoWarningBFG;
extern vmCvar_t cg_lowAmmoWarningGrapplingHook;
extern vmCvar_t cg_lowAmmoWarningNailGun;
extern vmCvar_t cg_lowAmmoWarningProximityLauncher;
extern vmCvar_t cg_lowAmmoWarningChainGun;


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
extern vmCvar_t cg_drawCrosshairNamesWideScreen;

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
extern vmCvar_t cg_drawCrosshairTeammateHealthWideScreen;

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
extern vmCvar_t cg_drawRewardsWideScreen;
extern vmCvar_t cg_rewardsStack;

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
extern vmCvar_t cg_drawTeamOverlayWideScreen;
extern vmCvar_t cg_drawTeamOverlayLineOffset;
extern vmCvar_t cg_drawTeamOverlayMaxPlayers;
extern vmCvar_t cg_selfOnTeamOverlay;

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
extern vmCvar_t cg_drawJumpSpeedsWideScreen;

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
extern vmCvar_t cg_drawJumpSpeedsTimeWideScreen;

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
//extern vmCvar_t cg_crosshairWideScreen;

extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_drawTeamBackground;
extern	vmCvar_t		cg_draw2D;
extern	vmCvar_t		cg_animSpeed;

extern	vmCvar_t		cg_debugAnim;
extern	vmCvar_t		cg_debugPosition;
extern	vmCvar_t		cg_debugEvents;
extern vmCvar_t cg_debugServerCommands;

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
extern vmCvar_t cg_simpleItemsBob;
extern vmCvar_t cg_simpleItemsHeightOffset;
extern vmCvar_t cg_itemsWh;
extern vmCvar_t cg_itemFx;
extern vmCvar_t cg_itemSize;
extern	vmCvar_t		cg_fov;
extern vmCvar_t cg_fovy;
extern vmCvar_t cg_fovStyle;
extern vmCvar_t cg_fovForceAspectWidth;
extern vmCvar_t cg_fovForceAspectHeight;
extern vmCvar_t cg_fovIntermission;

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
extern vmCvar_t cg_lagometerWideScreen;

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
extern vmCvar_t cg_drawAttackerWideScreen;

extern	vmCvar_t		cg_synchronousClients;
extern	vmCvar_t		cg_teamChatTime;
extern	vmCvar_t		cg_teamChatHeight;
extern	vmCvar_t		cg_stats;
extern	vmCvar_t 		cg_forceModel;
extern vmCvar_t cg_forcePovModel;
extern vmCvar_t cg_forcePovModelIgnoreFreecamTeamSettings;
extern	vmCvar_t 		cg_buildScript;
extern	vmCvar_t		cg_paused;
extern	vmCvar_t		cg_blood;
extern vmCvar_t cg_bleedTime;
extern	vmCvar_t		cg_predictItems;
extern	vmCvar_t		cg_deferPlayers;

extern	vmCvar_t		cg_drawFriend;
extern vmCvar_t cg_drawFriendStyle;
extern vmCvar_t cg_drawFriendMinWidth;
extern vmCvar_t cg_drawFriendMaxWidth;
extern vmCvar_t cg_drawFoe;
extern vmCvar_t cg_drawFoeMinWidth;
extern vmCvar_t cg_drawFoeMaxWidth;

extern vmCvar_t cg_drawSelf;
extern vmCvar_t cg_drawSelfMinWidth;
extern vmCvar_t cg_drawSelfMaxWidth;
extern vmCvar_t cg_drawSelfIconStyle;
extern vmCvar_t cg_drawInfected;

extern vmCvar_t cg_drawFlagCarrier;
extern vmCvar_t cg_drawFlagCarrierSize;
extern vmCvar_t cg_drawHitFlagCarrierTime;

extern	vmCvar_t		cg_teamChatsOnly;
#ifdef MISSIONPACK
extern	vmCvar_t		cg_noVoiceChats;
extern	vmCvar_t		cg_noVoiceText;
#endif
extern  vmCvar_t		cg_scorePlums;

extern vmCvar_t cg_damagePlum;
extern vmCvar_t cg_damagePlumColorStyle;
extern vmCvar_t cg_damagePlumTarget;
extern vmCvar_t cg_damagePlumTime;
extern vmCvar_t cg_damagePlumBounce;
extern vmCvar_t cg_damagePlumGravity;
extern vmCvar_t cg_damagePlumRandomVelocity;
extern vmCvar_t cg_damagePlumFont;
extern vmCvar_t cg_damagePlumFontStyle;
extern vmCvar_t cg_damagePlumPointSize;
extern vmCvar_t cg_damagePlumScale;
extern vmCvar_t cg_damagePlumColor;
extern vmCvar_t cg_damagePlumAlpha;

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
#if 1  //def MPACK
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
extern vmCvar_t cg_drawSpeedWideScreen;
extern vmCvar_t cg_drawSpeedChangeColor;

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
extern vmCvar_t cg_drawOriginWideScreen;

extern vmCvar_t	cg_drawScores;
extern vmCvar_t cg_drawPlayersLeft;
extern vmCvar_t cg_drawPowerups;

extern vmCvar_t cg_drawPowerupRespawn;
extern vmCvar_t cg_drawPowerupRespawnScale;
extern vmCvar_t cg_drawPowerupRespawnOffset;
extern vmCvar_t cg_drawPowerupRespawnAlpha;
//FIXME time option

extern vmCvar_t cg_drawPowerupAvailable;
extern vmCvar_t cg_drawPowerupAvailableScale;
extern vmCvar_t cg_drawPowerupAvailableOffset;
extern vmCvar_t cg_drawPowerupAvailableAlpha;
extern vmCvar_t cg_drawPowerupAvailableFadeStart;
extern vmCvar_t cg_drawPowerupAvailableFadeEnd;

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
extern vmCvar_t cg_drawItemPickupsWideScreen;

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
extern vmCvar_t cg_drawFollowingWideScreen;

extern vmCvar_t cg_testQlFont;
//extern vmCvar_t cg_deathSparkRadius;
extern vmCvar_t cg_qlhud;
extern vmCvar_t cg_qlFontScaling;
extern vmCvar_t cg_autoFontScaling;
extern vmCvar_t cg_autoFontScalingThreshold;

extern vmCvar_t cg_weaponBar;
extern vmCvar_t cg_weaponBarX;
extern vmCvar_t cg_weaponBarY;
extern vmCvar_t cg_weaponBarFont;
extern vmCvar_t cg_weaponBarPointSize;
extern vmCvar_t cg_weaponBarWideScreen;

extern vmCvar_t cg_drawFullWeaponBar;

extern vmCvar_t cg_scoreBoardStyle;
extern vmCvar_t cg_scoreBoardSpectatorScroll;
extern vmCvar_t cg_scoreBoardWhenDead;
extern vmCvar_t cg_scoreBoardAtIntermission;
extern vmCvar_t cg_scoreBoardWarmup;
extern vmCvar_t cg_scoreBoardOld;
extern vmCvar_t cg_scoreBoardOldWideScreen;
//extern vmCvar_t cg_scoreBoardCursorAreaWideScreen;
extern vmCvar_t cg_scoreBoardForceLineHeight;
extern vmCvar_t cg_scoreBoardForceLineHeightDefault;
extern vmCvar_t cg_scoreBoardForceLineHeightTeam;
extern vmCvar_t cg_scoreBoardForceLineHeightTeamDefault;

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
extern vmCvar_t cg_chatBeep;
extern vmCvar_t cg_chatBeepMaxTime;
extern vmCvar_t cg_teamChatBeep;
extern vmCvar_t cg_teamChatBeepMaxTime;

extern vmCvar_t cg_serverPrint;
extern vmCvar_t cg_serverPrintToChat;
extern vmCvar_t cg_serverPrintToConsole;

extern vmCvar_t cg_serverCenterPrint;
extern vmCvar_t cg_serverCenterPrintToChat;
extern vmCvar_t cg_serverCenterPrintToConsole;

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
extern vmCvar_t cg_drawCenterPrintWideScreen;

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
extern vmCvar_t cg_drawVoteWideScreen;

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
extern vmCvar_t cg_drawTeamVoteWideScreen;

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
extern vmCvar_t cg_drawWaitingForPlayersWideScreen;

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
extern vmCvar_t cg_drawWarmupStringWideScreen;

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
extern vmCvar_t cg_enemyHeadModel;
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
extern vmCvar_t cg_enemyFlagColor;

extern vmCvar_t cg_useDefaultTeamSkins;
extern vmCvar_t cg_ignoreClientHeadModel;
extern vmCvar_t cg_teamModel;
extern vmCvar_t cg_teamHeadModel;
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
extern vmCvar_t cg_teamFlagColor;

extern vmCvar_t cg_neutralFlagColor;

extern vmCvar_t cg_ourModel;
extern vmCvar_t cg_ourHeadModel;
extern vmCvar_t cg_ourHeadSkin;
extern vmCvar_t cg_ourTorsoSkin;
extern vmCvar_t cg_ourLegsSkin;
extern vmCvar_t cg_ourHeadColor;
extern vmCvar_t cg_ourTorsoColor;
extern vmCvar_t cg_ourLegsColor;

extern vmCvar_t cg_deadBodyColor;
extern vmCvar_t cg_disallowEnemyModelForTeammates;
extern vmCvar_t cg_fallbackModel;
extern vmCvar_t cg_fallbackHeadModel;

extern vmCvar_t cg_crosshairColor;

extern vmCvar_t cg_audioAnnouncer;
extern vmCvar_t cg_audioAnnouncerRewards;
extern vmCvar_t cg_audioAnnouncerRewardsFirst;
extern vmCvar_t cg_audioAnnouncerRound;
extern vmCvar_t cg_audioAnnouncerRoundReward;
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
extern vmCvar_t cg_audioAnnouncerPowerup;
extern vmCvar_t cg_ignoreServerPlaySound;

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
extern vmCvar_t cg_echoPopupWideScreen;

extern vmCvar_t cg_accX;
extern vmCvar_t cg_accY;
extern vmCvar_t cg_accWideScreen;

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
extern vmCvar_t cg_drawFragMessageWideScreen;

extern vmCvar_t cg_obituaryTokens;
extern vmCvar_t cg_obituaryIconScale;
extern vmCvar_t cg_obituaryRedTeamColor;
extern vmCvar_t cg_obituaryBlueTeamColor;
extern vmCvar_t cg_obituaryTime;
extern vmCvar_t cg_obituaryFadeTime;
extern vmCvar_t cg_obituaryStack;

extern vmCvar_t cg_fragTokenAccuracyStyle;
extern vmCvar_t cg_fragIconHeightFixed;

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
extern vmCvar_t cg_demoSmoothingAngles;
extern vmCvar_t cg_demoSmoothingTeleportCheck;
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
extern vmCvar_t cg_drawCameraPointInfoWideScreen;
extern vmCvar_t cg_drawCameraPointInfoBackgroundColor;
extern vmCvar_t cg_drawCameraPointInfoBackgroundAlpha;

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
extern vmCvar_t cg_weaponHeavyMachineGun;

extern vmCvar_t cg_spawnArmorTime;
extern vmCvar_t cg_fxfile;
extern vmCvar_t cg_fxinterval;
extern vmCvar_t cg_fxratio;
extern vmCvar_t cg_fxScriptMinEmitter;
extern vmCvar_t cg_fxScriptMinDistance;
extern vmCvar_t cg_fxScriptMinInterval;
extern vmCvar_t cg_fxLightningGunImpactFps;
extern vmCvar_t cg_fxDebugEntities;
extern vmCvar_t cg_fxCompiled;
extern vmCvar_t cg_fxThreads;
extern vmCvar_t cg_fxq3mmeCompatibility;  //FIXME maybe not

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
extern vmCvar_t cg_drawSpritesDeadPlayers;
extern vmCvar_t cg_playerLeanScale;
extern vmCvar_t cg_cameraRewindTime;
extern vmCvar_t cg_cameraQue;
extern vmCvar_t cg_cameraAddUsePreviousValues;
extern vmCvar_t cg_cameraUpdateFreeCam;
extern vmCvar_t cg_cameraDefaultOriginType;
extern vmCvar_t cg_cameraDebugPath;
extern vmCvar_t cg_cameraSmoothFactor;
extern vmCvar_t cg_q3mmeCameraSmoothPos;

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
extern vmCvar_t cg_whIncludeDeadBody;
extern vmCvar_t cg_whIncludeProjectile;
extern vmCvar_t cg_whShader;
extern vmCvar_t cg_whColor;
extern vmCvar_t cg_whAlpha;
extern vmCvar_t cg_whEnemyShader;
extern vmCvar_t cg_whEnemyColor;
extern vmCvar_t cg_whEnemyAlpha;

extern vmCvar_t cg_playerShader;
extern vmCvar_t cg_adShaderOverride;
extern vmCvar_t cg_debugAds;

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
extern vmCvar_t cg_drawProxWarningWideScreen;

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
extern vmCvar_t cg_allowServerOverride;

extern vmCvar_t cg_powerupLight;
extern vmCvar_t cg_buzzerSound;
extern vmCvar_t cg_flagStyle;
extern vmCvar_t cg_flagTakenSound;

extern vmCvar_t cg_helpIcon;
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
extern vmCvar_t cg_drawDominationPointStatusWideScreen;

extern vmCvar_t cg_roundScoreBoard;
extern vmCvar_t cg_headShots;

extern vmCvar_t cg_spectatorListSkillRating;
extern vmCvar_t cg_spectatorListScore;
extern vmCvar_t cg_spectatorListQue;

extern vmCvar_t cg_rocketAimBot;
extern vmCvar_t cg_drawTieredArmorAvailability;

extern vmCvar_t cg_drawDeadFriendTime;
extern vmCvar_t cg_drawHitFriendTime;
extern vmCvar_t cg_racePlayerShader;
extern vmCvar_t cg_quadSoundRate;
extern vmCvar_t cg_cpmaSound;
extern vmCvar_t cg_soundPvs;
extern vmCvar_t cg_soundBuffer;
extern vmCvar_t cg_drawFightMessage;
extern vmCvar_t cg_winLossMusic;

extern vmCvar_t cg_rewardCapture;
extern vmCvar_t cg_rewardImpressive;
extern vmCvar_t cg_rewardExcellent;
extern vmCvar_t cg_rewardHumiliation;
extern vmCvar_t cg_rewardDefend;
extern vmCvar_t cg_rewardAssist;
extern vmCvar_t cg_rewardComboKill;
extern vmCvar_t cg_rewardRampage;
extern vmCvar_t cg_rewardMidAir;
extern vmCvar_t cg_rewardRevenge;
extern vmCvar_t cg_rewardPerforated;
extern vmCvar_t cg_rewardHeadshot;
extern vmCvar_t cg_rewardAccuracy;
extern vmCvar_t cg_rewardQuadGod;
extern vmCvar_t cg_rewardFirstFrag;
extern vmCvar_t cg_rewardPerfect;

extern vmCvar_t cg_useDemoFov;
//extern vmCvar_t cg_specViewHeight;
extern vmCvar_t cg_specOffsetQL;

extern vmCvar_t cg_drawKeyPress;
extern vmCvar_t cg_useScoresUpdateTeam;

// end cvar_t

//===============================================

typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t; // bk001201 - warning: useless keyword or type name in empty declaration

//////////////////////////////////////////////////////////////

//FIXME forward declarations
//void CG_GetStoredScriptVarsFromLE (localEntity_t *le);

// player health/armor in ql protocol 91
#define SIGNED_16_BIT(x) (((x > 32767) ? -(65536 - x) : x))


#endif  // cg_local_h_included

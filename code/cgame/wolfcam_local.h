#ifndef wolfcam_h_included
#define wolfcam_h_included

//#include "cg_local.h"

#include "../qcommon/version.h"

#define DEBUG_BUFFER_SIZE 4192

//#define WOLFCAM_VERSION "6.2"

#define WOLFCAM_UNKNOWN 0
#define WOLFCAM_BASEQ3 1
//#define WOLFCAM_ETMAIN 1
//#define WOLFCAM_ETPRO 2

// rtcw
#define TEAM_AXIS TEAM_RED
#define TEAM_ALLIES TEAM_BLUE

#define PAIN_EV_TIME 700

#define MAX_SNAPSHOT_BACKUP     256
#define MAX_SNAPSHOT_MASK       (MAX_SNAPSHOT_BACKUP - 1)

enum {
	WOLFCAM_FOLLOW_DEMO_TAKER,
	WOLFCAM_FOLLOW_SELECTED_PLAYER,
	WOLFCAM_FOLLOW_KILLER,
	WOLFCAM_FOLLOW_VICTIM,
};

//#define GetAmmoTableData(ammoIndex) ((ammotable_t*)(&ammoTable[ammoIndex]))

typedef struct {
	vmCvar_t v;
	int modificationCount;
} wcvar_t;

// wolfcam global vars
typedef struct {
    int selectedClientNum;  // client selected by user
    int clientNum;  // client currently following
	int followMode;
	int nextKiller;
	int nextKillerServerTime;
	int nextVictim;
	int nextVictimServerTime;
	int ourLastClientNum;

    vec3_t vieworgDemo;
    vec3_t refdefViewAnglesDemo;

    //snapshot_t oldSnap;
    //qboolean oldSnapValid;

    snapshot_t snaps[MAX_SNAPSHOT_BACKUP];
    int curSnapshotNumber;
	int missedSnapshots;
	snapshot_t *snapOld;
	snapshot_t *snapPrev;
	snapshot_t *snapNext;
	qboolean centityAdded[MAX_GENTITIES];
	qboolean inSnapshot[MAX_GENTITIES];
    qboolean fastForward;
    qboolean fastForwarding;
    char oldTimescale[MAX_CVAR_VALUE_STRING];
    char oldVolume[MAX_CVAR_VALUE_STRING];

    qboolean playHitSound;
	qboolean playTeamHitSound;

	int weaponSelectTime;
} wcg_t;

//FIXME wolfcam mg42
typedef struct {
    unsigned int atts;
    unsigned int hits;
    unsigned int teamHits;
    unsigned int corpseHits;  // gibbing
    unsigned int teamCorpseHits;
    unsigned int headshots;
    unsigned int kills;
    unsigned int deaths;
    unsigned int suicides;
} wweaponStats_t;

typedef struct {
    qboolean currentValid;
	int validCount;  // how many snapshots they are present in
	int invalidCount;

    int invalidTime;
    int validTime;

    int health;
    int healthTime;

    int deathTime;

    int eventHealth;  // guessed from EV_*
    //int eventHealthTime;
    int ev_pain_time;  // EV_PAIN
    int bulletDamagePitch;
    int bulletDamageYaw;

    int landChange;
    int landTime;

    int duckChange;
    int duckTime;

    qboolean zoomedBinoc;
    int binocZoomTime;
    int zoomTime;
    int zoomval;

    //int event_fireWeaponTime;
    int event_akimboFire;
    int event_overheatTime;
    int event_reloadTime;
    int event_weaponSwitchTime;
    int event_newWeapon;
    int event_oldWeapon;

    int grenadeDynoTime;
    int grenadeDynoLastTime;

    int muzzleFlashTime;
    int bulletDamageTime;  //FIXME wolfcam tmp garbage

    entityState_t oldState;

    //FIXME wolfcam the following is messed up
    entityState_t esPrev;
    int prevTime;

    int weapAnim;
    int weapAnimPrev;
    int weapAnimTimer;

    float xyspeed;
    float bobfracsin;
    int bobcycle;
    int psbobCycle;
    float lastvalidBobfracsin;
    int lastvalidBobcycle;

    qboolean crosshairClientNoShoot;
    int oldSolid;

    // blend blobs
    //viewDamage_t viewDamage[MAX_VIEWDAMAGE];
    float damageTime;  // last time any kind of damage was recieved
    int damageIndex;  // slot that was filled in
    float damageX, damageY, damageValue;

    float v_dmg_time;
    float v_dmg_pitch;
    float v_dmg_roll;

    int kills;
    int deaths;
    int suicides;

    //FIXME wolfcam ammo

    int warp;
    int nowarp;
    int warpaccum;
	int noCommandCount;
	int noMoveCount;
	int serverPingAccum;
	int serverPingSamples;
	unsigned long int snapshotPingAccum;
	int snapshotPingSamples;

    wweaponStats_t wstats[WP_NUM_WEAPONS];
	wweaponStats_t perKillwstats[WP_NUM_WEAPONS];
	wweaponStats_t lastKillwstats[WP_NUM_WEAPONS];
	qboolean needToClearPerKillStats;  // clear when fire is released so that the extra spam after a kill doesn't count towards next kill's accuracy
	int fireWeaponPressedTime;
	qboolean fireWeaponPressedClearedStats;
	int lastKillWeapon;
	int lastDeathWeapon;
	qboolean killedOrDied;

} wclient_t;

typedef struct {
    qboolean valid;
	int validCount;  // how many snapshots they are present in
	int invalidCount;

    int clientNum;
    clientInfo_t clientinfo;


    int kills;
    int deaths;
    int suicides;

    int warp;
    int nowarp;
    int warpaccum;
	int noCommandCount;
	int noMoveCount;

	int serverPingAccum;
	int serverPingSamples;
	int snapshotPingAccum;
	int snapshotPingSamples;

    wweaponStats_t wstats[WP_NUM_WEAPONS];

} woldclient_t;

extern int wolfcam_gameVersion;

extern char globalDebugString[DEBUG_BUFFER_SIZE];
extern int wolfcam_initialSnapshotNumber;

extern qboolean wolfcam_following;

extern wcg_t wcg;
extern wclient_t wclients[MAX_CLIENTS];
extern woldclient_t woldclients[MAX_CLIENTS];  // clients that have disconnected
extern int wnumOldClients;
//extern fileHandle_t wc_logfile;
extern char *weapNames[];
extern char *weapNamesCasual[];

extern vmCvar_t wolfcam_debugErrors;
extern vmCvar_t wolfcam_debugDrawGun;
extern vmCvar_t wolfcam_fakelag;
extern vmCvar_t wolfcam_fast_forward_timescale;
extern vmCvar_t wolfcam_fast_forward_invalid_time;
//extern vmCvar_t wolfcam_execIntermission;
//extern vmCvar_t wolfcam_execShutdown;
extern vmCvar_t wolfcam_fixedViewAngles;
extern vmCvar_t cg_useOriginalInterpolation;
extern vmCvar_t cg_drawBBox;

//extern wcvar_t cg_teamModel;
//extern wcvar_t cg_teamColors;
//extern wcvar_t wolfcam_grenadeColor;
//extern wcvar_t cg_deadBodyColor;

void Wolfcam_Players_f (void);
void Wolfcam_Playersw_f (void);


void Wolfcam_Follow_f (void);
void Wolfcam_Server_Info_f (void);

void Wolfcam_Weapon_Stats_f (void);
void Wolfcam_Weapon_Statsall_f (void);
void Wolfcam_List_Player_Models_f (void);

void Wolfcam_DemoKeyClick (int key, qboolean down);

void Wolfcam_OwnerDraw (float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float speial, float scale, vec4_t color, qhandle_t shader, int textStyle);

//void Wolfcam_Draw2D (void);
void Wolfcam_ColorForHealth (vec4_t hcolor);
void Wolfcam_AddViewWeapon (void);

void Wolfcam_TransitionPlayerState (int oldClientNum);

void Wolfcam_ZoomIn (void);
void Wolfcam_ZoomOut (void);
int Wolfcam_CalcFov (void);
int Wolfcam_OffsetThirdPersonView (void);
int Wolfcam_OffsetFirstPersonView (void);

void Wolfcam_EntityEvent (centity_t *cent, vec3_t position);

//void Wolfcam_DrawFollowing (void);
//void Wolfcam_DrawCrosshairNames (void);

void wolfcam_etpro_command (const char *cmd);

void Wolfcam_DamageBlendBlob (void);
void Wolfcam_DrawActive( stereoFrame_t stereoView );
void Wolfcam_MarkValidEntities (void);

void Wolfcam_LogMissileHit (int weapon, vec3_t origin, vec3_t dir, int entityNum);
void Wolfcam_WeaponTrace (trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
qboolean Wolfcam_CalcMuzzlePoint (int entityNum, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, qboolean useLerp);
qboolean Wolfcam_InterpolateEntityPosition (centity_t *cent);
void Wolfcam_AddBox (vec3_t origin, int x, int y, int z, int red, int green, int blue);
void Wolfcam_AddBoundingBox( centity_t *cent );
void Wolfcam_ScoreData (void);
void Wolfcam_SwitchPlayerModels (void);
void Wolfcam_LoadModels (void);

void wolfcam_log_event (centity_t *cent, vec3_t position);
void Wolfcam_NextSnapShotSet (void);
int find_best_target (int attackerClientNum, qboolean useLerp, vec_t *distance, vec3_t offsets);
int Wolfcam_PlayerHealth (int clientNum);
int Wolfcam_PlayerArmor (int clientNum);
void Wolfcam_Reset_Weapon_Stats_f (void);
int wolfcam_find_client_to_follow (void);
void Wolfcam_CheckNoMove (void);

#endif // wolfcam_h_include

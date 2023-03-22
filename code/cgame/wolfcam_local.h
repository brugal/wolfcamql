#ifndef wolfcam_h_included
#define wolfcam_h_included

//#include "cg_local.h"

#define DEBUG_BUFFER_SIZE 4192

#define MAX_SNAPSHOT_BACKUP 256

#define INVALID_WOLFCAM_HEALTH -9999

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

	int painValue;
	int painValueTime;

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

	int freezeCgtime;
	float freezeXyspeed;
	float freezeLandChange;
	int freezeLandTime;

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
	int kamiKills;
    int deaths;
	int kamiDeaths;
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

    wweaponStats_t wstats[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
	wweaponStats_t perKillwstats[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
	wweaponStats_t lastKillwstats[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
	qboolean needToClearPerKillStats;  // clear when fire is released so that the extra spam after a kill doesn't count towards next kill's accuracy
	int fireWeaponPressedTime;
	qboolean fireWeaponPressedClearedStats;
	int lastKillWeapon;
	int lastDeathWeapon;
	qboolean killedOrDied;

	int killCount;
	qboolean aliveThisRound;

	int jumpTime;

	int damagePlumSum;
	int damagePlumTime;

} wclient_t;

typedef struct {
    qboolean valid;
	int validCount;  // how many snapshots they are present in
	int invalidCount;

    int clientNum;
    clientInfo_t clientinfo;


    int kills;
	int kamiKills;
    int deaths;
	int kamiDeaths;
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

    wweaponStats_t wstats[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];

} woldclient_t;


extern qboolean wolfcam_following;

extern wcg_t wcg;
extern wclient_t wclients[MAX_CLIENTS];
extern woldclient_t woldclients[MAX_CLIENTS];  // clients that have disconnected
extern int wnumOldClients;

extern char *weapNames[];
extern char *weapNamesCasual[];

extern vmCvar_t wolfcam_fixedViewAngles;
extern vmCvar_t cg_useOriginalInterpolation;
extern vmCvar_t cg_drawBBox;


#endif // wolfcam_h_include

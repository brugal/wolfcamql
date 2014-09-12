#ifndef bg_public_h_included
#define bg_public_h_included

/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define	GAME_VERSION		BASEGAME "-1"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	0.66

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	LIGHTNING_RANGE		768

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

#define DOMINATION_POINT_DISTANCE 128

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11
//#define CS_TEAMVOTE_TIME		12  //FIXME don't know
#define	CS_GAME_VERSION			12
#define	CS_LEVEL_START_TIME		13		// so the timer only shows the current level
#define	CS_INTERMISSION			14		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define	CS_ITEMS				15		// string of 0's and 1's that tell which items are present


#define CS_MODELS 				17  // was 32.  Same shit as CS_SOUNDS where it is being indexed from 1 so 17 is empty and first model is 18


#define	CS_SOUNDS				274  //(CS_MODELS+MAX_MODELS).  Might be 273 (17 + 256), but using consistant indexing from 0 in source code to fix sound bugs
#define	CS_PLAYERS				529  //(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS			593  //(CS_PLAYERS+MAX_CLIENTS)

#define CS_PARTICLES			(CS_LOCATIONS+MAX_LOCATIONS)  // 657

// not true anymore since quake live adds new config strings after this
//#define CS_MAX					(CS_PARTICLES+MAX_LOCATIONS)  // 721

#define CS_FLAGSTATUS			658  //23		// string indicating flag status in CTF

#define CS_FIRSTPLACE            659
#define CS_SECONDPLACE           660

#define CS_ROUND_STATUS			661  // also used for freezetag
#define CS_ROUND_TIME				662  // when -1 round is over, also used for freezetag

#define CS_RED_PLAYERS_LEFT		663
#define CS_BLUE_PLAYERS_LEFT	664

#define CS_SHADERSTATE			665  // was 24, 2008-69939-ZeRo4 - Clock (qzdm6)qcon08.rar

// 666
// 667  ctf, ca, tdm, duel, ffa
// 668  ctf, ca, tdm, duel, ffa
#define CS_TIMEOUT_BEGIN_TIME	669
#define CS_TIMEOUT_END_TIME		670
#define CS_RED_TEAM_TIMEOUTS_LEFT	671
#define CS_BLUE_TEAM_TIMEOUTS_LEFT	672
// 673
// 674 ca, ffa, ft
// 675 ca, ffa
// 676 ca, ffa
// 677 ffa
// 678 freezetag

#define CS_MAP_CREATOR 679
#define CS_ORIGINAL_MAP_CREATOR 680

// 681
//      ffa:  15000

// 682
//    ffa:  \pmove_AirAccel\1.0\pmove_AirSteps\1\pmove_JumpTimeDeltaMin\100.0\pmove_JumpVelocity\275\pmove_JumpVelocityScaleAdd\0.4\pmove_JumpVelocityTimeThreshold\500.0\pmove_RampJump\0\pmove_RampJumpScale\1.0\pmove_StepHeight\22.0\pmove_StepJump\1\pmove_WalkAccel\10.0\pmove_WalkFriction\6.0\pmove_WeaponDropTime\200\pmove_WeaponRaiseTime\200

#define CS_PMOVE_SETTINGS 682

// 683
//   ffa:  \weapon_reload_bfg\300\weapon_reload_cg\50\weapon_reload_gauntlet\400\weapon_reload_gl\800\weapon_reload_hook\400\weapon_reload_lg\50\weapon_reload_mg\100\weapon_reload_ng\1000\weapon_reload_pg\100\weapon_reload_prox\800\weapon_reload_rg\1500\weapon_reload_rl\800\weapon_reload_sg\1000

//////////////////////////////////////////////////////

#define CS_WEAPON_SETTINGS 683  // 2012-01-14 ql -- now armor tiered settings


// 684
//  ffa:  \g_allowCustomHeadmodels\0\g_playerheadScale\1.0\g_playerheadScaleOffset\1.0\g_playerModelScale\1.1

#define CS_CUSTOM_PLAYER_MODELS 684  // 2012-01-14 ql -- now weapon settings

// 685 in ffa  goes from -1 to 0 when ffa match starts  -- client num in first place?
#define CS_FIRST_PLACE_CLIENT_NUM 685  // 2012-01-14 ql -- now custom player models

// 686 in ffa  goes from -1 to 1 when ffa match starts -- client num in second place?
#define CS_SECOND_PLACE_CLIENT_NUM 686

// 687 in ffa  first place player's score?
#define CS_FIRST_PLACE_SCORE 687

// 688 in ffa  second place player's score?  2012-01-11 now first place score :(
#define CS_SECOND_PLACE_SCORE 688

// 689  2012-01-11  duel  now second place score?? :(

// 690 in ffa, ctf, ca, tdm, ft  award? stats
#define CS_MOST_DAMAGE_DEALT 690

// 691 in ffa, ctf, ca, tdm, ft  award? stats
#define CS_MOST_ACCURATE 691  // 2012-01-07 now most damage :(

// 692 in ffa, ctf,     tdm, ft  award? stats
#define CS_BEST_ITEM_CONTROL 692  // 2012-01-07 now most accurate :(

#define CS_RED_TEAM_CLAN_NAME 693
#define CS_BLUE_TEAM_CLAN_NAME 694
#define CS_RED_TEAM_CLAN_TAG 695  // unless it's "Red Team" :.
#define CS_BLUE_TEAM_CLAN_TAG 696  // unless it's "Blue Team"

// 697 2012-01-07  new -- best control  :(

// 2012-01-07  fucked up new config strings for awards -- they stuck in 683 armor tiered

/////////////////////////////////////////////////

#define CS_ARMOR_TIERED 683

#define CS_WEAPON_SETTINGS2 684
#define CS_CUSTOM_PLAYER_MODELS2 685
#define CS_FIRST_PLACE_CLIENT_NUM2 686
#define CS_SECOND_PLACE_CLIENT_NUM2 687
#define CS_FIRST_PLACE_SCORE2 688
#define CS_SECOND_PLACE_SCORE2 689

#define CS_MOST_DAMAGE_DEALT2 691
#define CS_MOST_ACCURATE2 692
#define CS_BEST_ITEM_CONTROL2 697

// 698 ?


//// fu ql

#define CS_MVP_OFFENSE 699
#define CS_MVP_DEFENSE 700
#define CS_MVP 701
#define CS_DOMINATION_RED_POINTS 702
#define CS_DOMINATION_BLUE_POINTS 703

#define CS_ROUND_WINNERS 705
#define CS_CUSTOM_SERVER_SETTINGS 706
#define CS_MAP_VOTE_INFO 707
#define CS_MAP_VOTE_COUNT 708
#define CS_DISABLE_MAP_VOTE 709

#define CS_READY_UP_TIME 710  // ready up time if one player readied

// 711

// 712  infected:  500.000000

#define CS_NUMBER_OF_RACE_CHECKPOINTS 713


// unknown ones which haven't been seen in quake live, but kept for compiling

#define	CS_TEAMVOTE_YES			MAX_CONFIGSTRINGS - 41  //16
#define	CS_TEAMVOTE_NO			MAX_CONFIGSTRINGS - 39  //18
#define CS_TEAMVOTE_TIME		MAX_CONFIGSTRINGS - 37  //19  //FIXME just anywere to let it compile
#define CS_TEAMVOTE_STRING		MAX_CONFIGSTRINGS - 35  //22  // was 14
#define CS_BOTINFO				MAX_CONFIGSTRINGS - 33  //25


// quake live server custom settings
/*
#define SERVER_SETTING_MODIFIED_GAUNT 0x1
#define SERVER_SETTING_MODIFIED_MG 0x2
4 sg
8 gl
10 rl
20 lg
40 rg
80 pg
100 bfg
200 grapple
400 nailgun
800 prox
1000 cg
*/

// 0x0000 2000 air control
// 0x0000 4000 ramp jumping
// 0x0000 8000 modified physics
// 0x0001 0000 modified weapon switch
// 0x0002 0000 instagib
// 0x0004 0000 quad hog
// 0x0008 0000 regen health
// 0x0010 0000 dropped damage health
// 0x0020 0000 vampiric damage
// 0x0040 0000 modified item spawning
// 0x0080 0000 headshots enabled
// 0x0100 0000 rail jumping

#define SERVER_SETTING_INFECTED 0x04000000


// doesn't work with quakelive
#if 0
#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif
#endif

/////////////////////////////
// q3 config strings
// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CSQ3_SERVERINFO   0  //CS_SERVERINFO
#define CSQ3_SYSTEMINFO   1  //CS_SYSTEMINFO
#define	CSQ3_MUSIC				2
#define	CSQ3_MESSAGE				3		// from the map worldspawn's message field
#define	CSQ3_MOTD					4		// g_motd string for server message of the day
#define	CSQ3_WARMUP				5		// server time when the match will be restarted
#define	CSQ3_SCORES1				6
#define	CSQ3_SCORES2				7
#define CSQ3_VOTE_TIME			8
#define CSQ3_VOTE_STRING			9
#define	CSQ3_VOTE_YES				10
#define	CSQ3_VOTE_NO				11

#define CSQ3_TEAMVOTE_TIME		12
#define CSQ3_TEAMVOTE_STRING		14
#define	CSQ3_TEAMVOTE_YES			16
#define	CSQ3_TEAMVOTE_NO			18

#define	CSQ3_GAME_VERSION			20
#define	CSQ3_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CSQ3_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CSQ3_FLAGSTATUS			23		// string indicating flag status in CTF
#define CSQ3_SHADERSTATE			24
#define CSQ3_BOTINFO				25

#define	CSQ3_ITEMS				27		// string of 0's and 1's that tell which items are present

#define	CSQ3_MODELS				32
#define	CSQ3_SOUNDS				(CSQ3_MODELS+MAX_MODELS)
#define	CSQ3_PLAYERS				(CSQ3_SOUNDS+MAX_SOUNDS)
#define CSQ3_LOCATIONS			(CSQ3_PLAYERS+MAX_CLIENTS)
#define CSQ3_PARTICLES			(CSQ3_LOCATIONS+MAX_LOCATIONS) 

#define CSQ3_MAX					(CSQ3_PARTICLES+MAX_LOCATIONS)

#if (CSQ3_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CSQ3_MAX) > MAX_CONFIGSTRINGS
#endif

// cpma
#define CSCPMA_GAMESTATE 672

#define CSCPMA_SCORES 710

#define CSCPMA_MESSAGE 718  // map message

#define CSCPMA_RATE 726  // not sure
// 727  max packet dup?  seen:  4

#define CSCPMA_SNAPS 746
#define CSCPMA_MAX_PACKETS 747

// 752 -> 759  mod + server message?


typedef enum {
	GT_FFA,				// free for all
	GT_TOURNAMENT,		// one on one tournament
	GT_SINGLE_PLAYER,	// single player ffa

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_CA,				// clan arena
	GT_CTF,				// capture the flag
	GT_1FCTF,
	GT_OBELISK,
	GT_HARVESTER,
	GT_FREEZETAG,
	GT_DOMINATION,
	GT_CTFS,
	GT_RED_ROVER,

	// cpma
	GT_NTF,  // not team fortress
	GT_2V2,
	GT_HM,  // hoony mode
	GT_RACE,
	GT_MAX_GAME_TYPE,
} gametype_t;


typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	qboolean unlockPitch;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd, qboolean unlockPitch );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
#if 0
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
#if 1  //def MPACK
	STAT_PERSISTANT_POWERUP,
#endif
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,

	// quakelive 2011-12  now battle suit kill count
	//STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)  // not used in quakelive
	STAT_BATTLE_SUIT_KILL_COUNT,
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,					// health / armor limit, changable by handicap
	STAT_UNKNOWN_8,  // chaingun spin rate?
	STAT_UNKNOWN_9,  // flight powerup?
	STAT_UNKNOWN_10, // flight powerup?
	STAT_POWERUP_REMAINING,
	STAT_UNKNOWN_12,
	STAT_QUAD_KILL_COUNT,
	STAT_ARMOR_TIER,
	STAT_UNKNOWN_15,
} statIndex_t;

#endif

extern int STAT_HEALTH;
extern int STAT_HOLDABLE_ITEM;
extern int STAT_PERSISTANT_POWERUP;
extern int STAT_WEAPONS;
extern int STAT_ARMOR;
extern int STAT_BATTLE_SUIT_KILL_COUNT;
extern int STAT_CLIENTS_READY;
extern int STAT_MAX_HEALTH;
extern int STAT_UNKNOWN_8;
extern int STAT_UNKNOWN_9;
extern int STAT_UNKNOWN_10;
extern int STAT_POWERUP_REMAINING;
extern int STAT_UNKNOWN_12;
extern int STAT_QUAD_KILL_COUNT;
extern int STAT_ARMOR_TIER;
extern int STAT_UNKNOWN_15;

// hack to support mods

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
#if 0
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter

	PERS_KILLED,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
	PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
	PERS_DEFEND_COUNT,				// defend awards
	PERS_ASSIST_COUNT,				// assist awards
	PERS_GAUNTLET_FRAG_COUNT,		// kills with the gauntlet
	PERS_CAPTURES,					// captures

	PERS_ATTACKEE_ARMOR,			// health/armor of last person we attacked
} persEnum_t;
#endif

extern int PERS_SCORE;
extern int PERS_HITS;
extern int PERS_RANK;
extern int PERS_TEAM;
extern int PERS_SPAWN_COUNT;
extern int PERS_PLAYEREVENTS;
extern int PERS_ATTACKER;
extern int PERS_KILLED;
extern int PERS_IMPRESSIVE_COUNT;
extern int PERS_EXCELLENT_COUNT;
extern int PERS_DEFEND_COUNT;
extern int PERS_ASSIST_COUNT;
extern int PERS_GAUNTLET_FRAG_COUNT;
extern int PERS_CAPTURES;
extern int PERS_ATTACKEE_ARMOR;

// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#if 1  //def MPACK
#define EF_TICKING			0x00000002		// used to make players play the prox mine ticking sound
#endif
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,
	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,

	PW_REGEN,
	PW_FLIGHT,
	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_INVULNERABILITY,
	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	//PW_AMMOREGEN,
	PW_ARMORREGEN,

	PW_FROZEN,
	PW_NUM_POWERUPS,
} powerup_t;

#define PW_SPAWNPROTECTION 1  // hack

typedef enum {
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,

	HI_NUM_HOLDABLE
} holdable_t;

typedef enum {
	HIC_NONE,

	HIC_UNKNOWN1,
	HIC_UNKNOWN2,
	HIC_UNKNOWN3,

	HIC_TELEPORTER,
	HIC_MEDKIT,
	HIC_KAMIKAZE,
	HIC_PORTAL,
	HIC_INVULNERABILITY,

	HIC_NUM_HOLDABLE
} holdableCpma_t;

typedef enum {
	WP_NONE,

	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
#if 1  //def MPACK
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,
#endif

	WP_HEAVY_MACHINEGUN,
	//WP_15,
	WP_NUM_WEAPONS
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP = 1,
	EV_FOOTSTEP_METAL = 2,
	EV_FOOTSPLASH = 3,
	EV_FOOTWADE,  // guess
	EV_SWIM,    // guess

	EV_FALL_SHORT = 6,
    EV_FALL_MEDIUM = 7,
	EV_FALL_FAR = 8,
	EV_JUMP_PAD = 9,
	EV_JUMP = 10,
	EV_WATER_TOUCH = 11,
	EV_WATER_LEAVE = 12,

	EV_WATER_UNDER = 13,
	EV_WATER_CLEAR = 14,

	EV_ITEM_PICKUP = 15,
	EV_GLOBAL_ITEM_PICKUP = 16,

	EV_NOAMMO = 17,
	EV_CHANGE_WEAPON = 18,
	EV_DROP_WEAPON = 19,
	EV_FIRE_WEAPON = 20,

	EV_USE_ITEM0,  // guess
	EV_USE_ITEM1 = 22,
	EV_USE_ITEM2,  // guess
	EV_USE_ITEM3,  // guess
	EV_USE_ITEM4,  // guess
	EV_USE_ITEM5,  // guess
	EV_USE_ITEM6,  // guess
	EV_USE_ITEM7,  // guess
	EV_USE_ITEM8,  // guess
	EV_USE_ITEM9,  // guess
	EV_USE_ITEM10,  // guess
	EV_USE_ITEM11,  // guess
	EV_USE_ITEM12,  // guess
	EV_USE_ITEM13,  // guess
	EV_USE_ITEM14,  // guess
	EV_USE_ITEM15,  // guess


	EV_ITEM_RESPAWN = 37,
	EV_ITEM_POP,  // guess

	EV_PLAYER_TELEPORT_IN = 39,
	EV_PLAYER_TELEPORT_OUT = 40,
	EV_GRENADE_BOUNCE = 41,
	EV_GENERAL_SOUND = 42,
	EV_GLOBAL_SOUND = 43,

	EV_GLOBAL_TEAM_SOUND,  // guess

	EV_BULLET_HIT_FLESH = 45,
	EV_BULLET_HIT_WALL = 46,
	EV_MISSILE_HIT = 47,
	EV_MISSILE_MISS = 48,

	EV_MISSILE_MISS_METAL = 49,
	EV_RAILTRAIL = 50,

	EV_SHOTGUN = 51,

	// not in quakelive    EV_BULLET,

	EV_PAIN = 53,
	EV_DEATH1 = 54,
	EV_DEATH2 = 55,
	EV_DEATH3 = 56,
	EV_DROWN = 57,

	EV_OBITUARY = 58,

	EV_POWERUP_QUAD,  // guess
	EV_POWERUP_BATTLESUIT = 60,
	EV_POWERUP_REGEN = 61,  // 62 in older demo zero4 vs cl0ck
	EV_POWERUP_ARMOR_REGEN = 62,  // ctf silentnight (2010-12-26)  armor regen?
	EV_GIB_PLAYER = 63,
	EV_SCOREPLUM = 64,

	EV_PROXIMITY_MINE_STICK = 65,
	EV_PROXIMITY_MINE_TRIGGER = 66,

	EV_KAMIKAZE = 67,			// kamikaze explodes
	EV_OBELISKEXPLODE = 68,
	EV_OBELISKPAIN = 69,
	EV_INVUL_IMPACT = 70,		// invulnerability sphere impact

	EV_DEBUG_LINE = 72,  // guess
	EV_STOPLOOPINGSOUND = 73, // guess
	EV_TAUNT = 74,
	EV_TAUNT_YES,  // guess
	EV_TAUNT_NO,  // guess
	EV_TAUNT_FOLLOWME,  // guess
	EV_TAUNT_GETFLAG,  // guess
	EV_TAUNT_GUARDBASE,  // guess
	EV_TAUNT_PATROL,  // guess

	EV_FOOTSTEP_SNOW = 81,
	EV_FOOTSTEP_WOOD = 82,
	EV_ITEM_PICKUP_SPEC = 83,
	EV_OVERTIME = 84,
	EV_GAMEOVER = 85,

	EV_THAW_PLAYER = 87,
	EV_THAW_TICK = 88,
	EV_HEADSHOT = 89,
	EV_POI = 90,

	EV_RACE_START = 93,
	EV_RACE_CHECKPOINT = 94,
	EV_RACE_END = 95,

	// just to allow compiling  -- no it's used with predicted player state
	EV_STEP_4 = 196,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_STEP_20,
	EV_STEP_24,

	//FIXME these are definately wrong -- just getting it to compile



	EV_JUICED,				// invulnerability juiced effect
	EV_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere

} entity_event_t;


typedef enum {
	EVQ3_NONE,

	EVQ3_FOOTSTEP,
	EVQ3_FOOTSTEP_METAL,
	EVQ3_FOOTSPLASH,
	EVQ3_FOOTWADE,
	EVQ3_SWIM,

	EVQ3_STEP_4,
	EVQ3_STEP_8,
	EVQ3_STEP_12,
	EVQ3_STEP_16,

	EVQ3_FALL_SHORT,
	EVQ3_FALL_MEDIUM,
	EVQ3_FALL_FAR,

	EVQ3_JUMP_PAD,			// boing sound at origin, jump sound on player

	EVQ3_JUMP,
	EVQ3_WATER_TOUCH,	// foot touches
	EVQ3_WATER_LEAVE,	// foot leaves
	EVQ3_WATER_UNDER,	// head touches
	EVQ3_WATER_CLEAR,	// head leaves

	EVQ3_ITEM_PICKUP,			// normal item pickups are predictable
	EVQ3_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

	EVQ3_NOAMMO,
	EVQ3_CHANGE_WEAPON,
	EVQ3_FIRE_WEAPON,

	EVQ3_USE_ITEM0,
	EVQ3_USE_ITEM1,
	EVQ3_USE_ITEM2,
	EVQ3_USE_ITEM3,
	EVQ3_USE_ITEM4,
	EVQ3_USE_ITEM5,
	EVQ3_USE_ITEM6,
	EVQ3_USE_ITEM7,
	EVQ3_USE_ITEM8,
	EVQ3_USE_ITEM9,
	EVQ3_USE_ITEM10,
	EVQ3_USE_ITEM11,
	EVQ3_USE_ITEM12,
	EVQ3_USE_ITEM13,
	EVQ3_USE_ITEM14,
	EVQ3_USE_ITEM15,

	EVQ3_ITEM_RESPAWN,
	EVQ3_ITEM_POP,
	EVQ3_PLAYER_TELEPORT_IN,
	EVQ3_PLAYER_TELEPORT_OUT,

	EVQ3_GRENADE_BOUNCE,		// eventParm will be the soundindex

	EVQ3_GENERAL_SOUND,
	EVQ3_GLOBAL_SOUND,		// no attenuation
	EVQ3_GLOBAL_TEAM_SOUND,

	EVQ3_BULLET_HIT_FLESH,
	EVQ3_BULLET_HIT_WALL,

	EVQ3_MISSILE_HIT,
	EVQ3_MISSILE_MISS,
	EVQ3_MISSILE_MISS_METAL,
	EVQ3_RAILTRAIL,
	EVQ3_SHOTGUN,
	EVQ3_BULLET,				// otherEntity is the shooter

	EVQ3_PAIN,
	EVQ3_DEATH1,
	EVQ3_DEATH2,
	EVQ3_DEATH3,
	EVQ3_OBITUARY,

	EVQ3_POWERUP_QUAD,
	EVQ3_POWERUP_BATTLESUIT,
	EVQ3_POWERUP_REGEN,

	EVQ3_GIB_PLAYER,			// gib a previously living player
	EVQ3_SCOREPLUM,			// score plum

//#ifdef MISSIONPACK
	EVQ3_PROXIMITY_MINE_STICK,
	EVQ3_PROXIMITY_MINE_TRIGGER,
	EVQ3_KAMIKAZE,			// kamikaze explodes
	EVQ3_OBELISKEXPLODE,		// obelisk explodes
	EVQ3_OBELISKPAIN,			// obelisk is in pain
	EVQ3_INVUL_IMPACT,		// invulnerability sphere impact
	EVQ3_JUICED,				// invulnerability juiced effect
	EVQ3_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere
//#endif

	EVQ3_DEBUG_LINE,
	EVQ3_STOPLOOPINGSOUND,
	EVQ3_TAUNT,
	EVQ3_TAUNT_YES,
	EVQ3_TAUNT_NO,
	EVQ3_TAUNT_FOLLOWME,
	EVQ3_TAUNT_GETFLAG,
	EVQ3_TAUNT_GUARDBASE,
	EVQ3_TAUNT_PATROL

} entity_event_q3_t;

#define EVCPMA_FIRE_GRAPPLE 67
#define EVCPMA_TAUNT 78

typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,  // 10
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE,
	GTS_RED_WINS,
	GTS_BLUE_WINS,   // 15
	GTS_RED_WINS_ROUND,
	GTS_BLUE_WINS_ROUND,
	GTS_ROUND_DRAW,
	GTS_LAST_STANDING,
	GTS_ROUND_OVER,
	GTS_DENIED,  // qldt plays 'denied' sound, in attack defend demo
	GTS_ENEMY_TEAM_KILLED,
	GTS_DOMINATION_POINT_CAPTURE,
	GTS_PLAYER_INFECTED,  // guess, in red rover halloween infected
} global_team_sound_t;

// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE, 
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,

	MOD_GRENADE_SPLASH,  // 5
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_PLASMA,
	MOD_PLASMA_SPLASH,

	MOD_RAILGUN,  // 10
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
	MOD_WATER,

	MOD_SLIME,  // 15
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,

	MOD_SUICIDE,  // 20
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
#if 1  //def MPACK
	MOD_NAIL,
	MOD_CHAINGUN,

	MOD_PROXIMITY_MINE,  // 25
	MOD_KAMIKAZE,
	MOD_JUICED,
#endif
	MOD_GRAPPLE,
	MOD_SWITCH_TEAMS,  // 29

	MOD_THAW = 30,

	// unknown 31

	MOD_HMG = 32,

} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	char		*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model[MAX_ITEM_MODELS];

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

	char		*precaches;		// string of all models and images this item will use
	char		*sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];  // ql dm 90
extern const gitem_t bg_itemlistQ3[];
extern const gitem_t bg_itemlistCpma[];
extern const gitem_t bg_itemlistQldm73[];
extern	int		bg_numItems;
extern	int		bg_numItemsQ3;
extern	int		bg_numItemsCpma;
extern int bg_numItemsQldm73;

gitem_t	*BG_FindItem( const char *pickupName );
gitem_t	*BG_FindItemForWeapon( const weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( const powerup_t pw );
gitem_t	*BG_FindItemForHoldable( const holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( const int gametype, const entityState_t *ent, const playerState_t *ps );


// g_dmflags->integer flags
//#define	DF_NO_FALLING			8
//#define DF_FIXED_FOV			16


#define DF_NO_SELF_HEALTH_DAMAGE 4
#define DF_NO_SELF_ARMOR_DAMAGE 8
#define DF_NO_FALLING_DAMAGE 16
#define	DF_NO_FOOTSTEPS 32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;



void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );
void BG_EvaluateTrajectoryf (const trajectory_t *tr, int atTimeMsec, vec3_t result, float subTime);
void BG_EvaluateTrajectoryDeltaf (const trajectory_t *tr, int atTimeMsec, vec3_t result, float subTime);

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_TouchJumpPad( playerState_t *ps, const entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean	BG_PlayerTouchesItem( const playerState_t *ps, const entityState_t *item, int atTime );


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		(8192 * 2)

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		(8192 * 2)


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704

#endif  // bg_public_h_included

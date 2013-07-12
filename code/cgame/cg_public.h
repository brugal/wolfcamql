#ifndef cg_public_h_included
#define cg_public_h_included

// Copyright (C) 1999-2000 Id Software, Inc.
//


#define MAX_DEMO_OBITS (1024 * 2)
#define MAX_ITEM_PICKUPS (1024 * 4)
#define MAX_TIMEOUTS (256)

typedef struct {
	int startTime;
	int endTime;
	int serverTime;  // time the timeout or timein command is issued, since it takes two config string changes in quake live for the start and end time this is used incase order doesn't matter
	//int cpmaLevelTime;
	//int cpmaTd;
	//int cpmaTimein;
} timeOut_t;

typedef struct {
	int firstServerTime;
	int firstMessageNum;
	int lastServerTime;
	int lastMessageNum;
	int number;
	int killer;
	int victim;
	int mod;
} demoObit_t;

typedef struct {
	int clientNum;
	int index;
	vec3_t origin;
	int pickupTime;
	int specPickupTime;
	int number;  // entity number
	qboolean spec;
} itemPickup_t;


#define	CMD_BACKUP			64
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	512  //256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
	int messageNum;
} snapshot_t;

enum {
  CGAME_EVENT_NONE,
  CGAME_EVENT_TEAMMENU,
  CGAME_EVENT_SCOREBOARD,
  CGAME_EVENT_EDITHUD,
  CGAME_EVENT_DEMO
};


/*
==================================================================

functions imported from the main executable

==================================================================
*/

#define	CGAME_IMPORT_API_VERSION	0x6667

typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_LOADMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_MARKFRAGMENTS,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_R_REGISTERSHADERNOMIP,
	CG_MEMORY_REMAINING,
	CG_R_REGISTERFONT,
	CG_KEY_ISDOWN,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,
 	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,
	CG_S_STOPBACKGROUNDTRACK,
	CG_REAL_TIME,
	CG_SNAPVECTOR,
	CG_REMOVECOMMAND,
	CG_R_LIGHTFORPOINT,
	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,
	CG_R_REMAP_SHADER,
	CG_S_ADDREALLOOPINGSOUND,
	CG_S_STOPLOOPINGSOUND,

	CG_CM_TEMPCAPSULEMODEL,
	CG_CM_CAPSULETRACE,
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_R_ADDADDITIVELIGHTTOSCENE,
	CG_GET_ENTITY_TOKEN,
	CG_R_ADDPOLYSTOSCENE,
	CG_R_INPVS,
	// 1.32
	CG_FS_SEEK,   // 89

	CG_LOADCAMERA,
	CG_STARTCAMERA,
	CG_GETCAMERAINFO,

	CG_MEMSET = 100,
	CG_MEMCPY,
	CG_STRNCPY,
	CG_SIN,
	CG_COS,
	CG_ATAN2,
	CG_SQRT,
	CG_FLOOR,
	CG_CEIL,
	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT,
	CG_ACOS,

	CG_GET_ADVERTISEMENTS,
	CG_DRAW_CONSOLE_LINES,
	CG_KEY_GETBINDING,
	CG_GETLASTEXECUTEDSERVERCOMMAND,
	CG_GETNEXTKILLER,
	CG_GETNEXTVICTIM,
	CG_REPLACESHADERIMAGE,
	CG_REGISTERSHADERFROMDATA,
	CG_GETSHADERIMAGEDIMENSIONS,
	CG_GETSHADERIMAGEDATA,
	CG_PEEKSNAPSHOT,
	CG_CALCSPLINE,
	CG_SETPATHLINES,
	CG_GETGAMESTARTTIME,
	CG_GETGAMEENDTIME,
	CG_GETFIRSTSERVERTIME,
	//CG_ADDAT,
	CG_GETLEGSANIMSTARTTIME,
	CG_GETTORSOANIMSTARTTIME,
	CG_R_REGISTERSHADERLIGHTMAP,
	CG_AUTOWRITECONFIG,
	CG_SENDCONSOLECOMMANDNOW,
	CG_POWF,
	CG_R_CLEAR_REMAPPED_SHADER,
	CG_GETITEMPICKUPNUMBER,
	CG_GETITEMPICKUP,
	CG_R_GETSINGLESHADER,
	CG_CVAR_EXISTS,
	CG_GET_DEMO_TIMEOUTS,
	CG_GET_NUM_PLAYER_INFO,
	CG_GET_EXTRA_PLAYER_INFO,
	CG_GET_REAL_MAP_NAME,
	CG_R_ADDREFENTITYPTRTOSCENE,

} cgameImport_t;


/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_INIT,
//	void CG_Init (int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback)
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( void );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( void );

	CG_KEY_EVENT,
//	void	(*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int dx, int dy );
	CG_EVENT_HANDLING,
//	void (*CG_EventHandling)(int type);
	CG_TIME_CHANGE,
	CG_COLOR_TABLE_CHANGE,
} cgameExport_t;

//----------------------------------------------

#endif  // cg_public_h_included

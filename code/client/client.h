#ifndef client_h_included
#define client_h_included
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
// client.h -- primary header for client

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "../ui/ui_public.h"
//#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"
#include "cl_avi.h"
#include "../sys/sys_local.h"

#ifdef USE_CURL
#include "cl_curl.h"
#endif /* USE_CURL */

#ifdef USE_VOIP
#include "speex/speex.h"
#include "speex/speex_preprocess.h"
#include <opus.h>
#endif

#define MAX_DEMO_FILES 64

// file full of random crap that gets used to create cl_guid
#define QKEY_FILE "qkey"
#define QKEY_SIZE 2048

#define	RETRANSMIT_TIMEOUT	3000	// time between connection packet retransmits

// snapshots are a view of the server at a given time
typedef struct {
	qboolean		valid;			// cleared if delta parsing was invalid
	int				snapFlags;		// rate delayed and dropped commands

	int				serverTime;		// server time the message is valid for (in msec)

	int				messageNum;		// copied from netchan->incoming_sequence
	int				deltaNum;		// messageNum the delta is from
	int				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	int				parseEntitiesNum;		// at the time of this snapshot

	int				serverCommandNum;		// execute all commands up to this before
											// making the snapshot current
} clSnapshot_t;



/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct {
	int		p_cmdNumber;		// cl.cmdNumber when packet was sent
	int		p_serverTime;		// usercmd->serverTime when packet was sent
	int		p_realtime;			// cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define  MAX_PARSE_ENTITIES  ( PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES )

extern int g_console_field_width;

typedef struct {
	int			timeoutcount;		// it requres several frames in a timeout condition
									// to disconnect, preventing debugging breaks from
									// causing immediate disconnects on continue
	clSnapshot_t	snap;			// latest received from server

	int			serverTime;			// may be paused during play
	int			oldServerTime;		// to prevent time from flowing bakcwards
	int			oldFrameServerTime;	// to check tournament restarts
	int			serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
									// this value changes as net lag varies
	qboolean	extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	qboolean	newSnapshots;		// set on parse of any valid packet

	gameState_t	gameState;			// configstrings
	char		mapname[MAX_QPATH];	// extracted from CS_SERVERINFO

	int			parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	int			mouseDx[2], mouseDy[2];	// added to by mouse events
	int			mouseIndex;
	int			joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	// cgame communicates a few values to the client system
	int			cgameUserCmdValue;	// current weapon to add to usercmd_t
	float		cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmdNumber;			// incremented each frame, because multiple
									// frames may need to be packed into a single packet

	outPacket_t	outPackets[PACKET_BACKUP];	// information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			serverId;			// included in each client message so the server
												// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t	snapshots[PACKET_BACKUP][MAX_DEMO_FILES];

	entityState_t	entityBaselines[MAX_GENTITIES];	// for delta compression when not in previous frame

	entityState_t	parseEntities[MAX_PARSE_ENTITIES];
	qboolean draw;
} clientActive_t;

extern	clientActive_t		cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/

#define MAX_TIMEDEMO_DURATIONS	4096

typedef struct {

	int			clientNum;
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts

	netadr_t	serverAddress;
	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog

	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int			reliableSequence;
	int			reliableAcknowledge;		// the last one the server has executed
	char		reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int			serverMessageSequence;

	// reliable messages received from server
	int			serverCommandSequence;
	int			lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char		serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// file transfer from server
	fileHandle_t download;
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
#ifdef USE_CURL
	qboolean	cURLEnabled;
	qboolean	cURLUsed;
	qboolean	cURLDisconnected;
	char		downloadURL[MAX_OSPATH];
	CURL		*downloadCURL;
	CURLM		*downloadCURLM;
#endif /* USE_CURL */
	int		sv_allowDownload;
	char		sv_dlURL[MAX_CVAR_VALUE_STRING];
	int			downloadNumber;
	int			downloadBlock;	// block we are waiting for
	int			downloadCount;	// how many bytes we got
	int			downloadSize;	// how many bytes we got
	char		downloadList[MAX_INFO_STRING]; // list of paks we need to download
	qboolean	downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak

	// demo information
	char		demoName[MAX_QPATH];
	qboolean	spDemoRecording;
	qboolean	demorecording;
	qboolean	demoplaying;
	int demoPlayBegin;
	const char *demoWorkshopsString;
	char currentWorkshop[MAX_STRING_CHARS];

	popenData_t *wfp;

	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	firstDemoFrameSkipped;
	fileHandle_t demoReadFile;
	fileHandle_t	demoWriteFile;

	int			timeDemoFrames;		// counter of rendered frames
	int			timeDemoStart;		// cls.realtime before first frame
	int			timeDemoBaseTime;	// each frame will be at this time + frameNum * 50
	int			timeDemoLastFrame;// time the last frame was rendered
	int			timeDemoMinDuration;	// minimum frame duration
	int			timeDemoMaxDuration;	// maximum frame duration
	unsigned char	timeDemoDurations[ MAX_TIMEDEMO_DURATIONS ];	// log of frame durations
	int	cgameTime;
	int realProtocol;

#ifdef USE_VOIP
	qboolean voipEnabled;
	qboolean speexInitialized;
	int speexFrameSize;
	int speexSampleRate;
	qboolean voipCodecInitialized;

	// incoming data...
	// !!! FIXME: convert from parallel arrays to array of a struct.
	SpeexBits speexDecoderBits[MAX_CLIENTS];
	void *speexDecoder[MAX_CLIENTS];
	OpusDecoder *opusDecoder[MAX_CLIENTS];
	byte voipIncomingGeneration[MAX_CLIENTS];
	int voipIncomingSequence[MAX_CLIENTS];
	float voipGain[MAX_CLIENTS];
	qboolean voipIgnore[MAX_CLIENTS];
	qboolean voipMuteAll;

	// outgoing data...
	// if voipTargets[i / 8] & (1 << (i % 8)),
	// then we are sending to clientnum i.
	uint8_t voipTargets[(MAX_CLIENTS + 7) / 8];
	uint8_t voipFlags;
	SpeexPreprocessState *speexPreprocessor;
	SpeexBits speexEncoderBits;
	void *speexEncoder;
	OpusEncoder *opusEncoder;
	int voipOutgoingDataSize;
	int voipOutgoingDataFrames;
	int voipOutgoingSequence;
	byte voipOutgoingGeneration;
	byte voipOutgoingData[1024];
	float voipPower;
#endif

#ifdef LEGACY_PROTOCOL
	qboolean compat;
#endif

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t	netchan;
} clientConnection_t;

extern	clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct {
	netadr_t	adr;
	int			start;
	int			time;
	char		info[MAX_INFO_STRING];
} ping_t;

typedef struct {
	netadr_t	adr;
	char	  	hostName[MAX_NAME_LENGTH];
	char	  	mapName[MAX_NAME_LENGTH];
	char	  	game[MAX_NAME_LENGTH];
	int			netType;
	int			gameType;
	int		  	clients;
	int		  	maxClients;
	int			minPing;
	int			maxPing;
	int			ping;
	qboolean	visible;
	int			punkbuster;
	int                     g_humanplayers;
	int                     g_needpass;
} serverInfo_t;

typedef struct {
	connstate_t	state;				// connection status

	qboolean	cddialog;			// bring up the cd needed dialog next frame

	char		servername[MAX_OSPATH];		// name of server from original connect (used by reconnect)

	// when the server clears the hunk, all of these must be restarted
	qboolean	rendererStarted;
	qboolean	soundStarted;
	qboolean	soundRegistered;
	qboolean	uiStarted;
	qboolean	cgameStarted;

	int			framecount;
	int			frametime;			// msec since last frame

	int			realtime;			// ignores pause
	int			realFrametime;		// ignoring pause, so console always works

	int			numlocalservers;
	serverInfo_t	localServers[MAX_OTHER_SERVERS];

	int			numglobalservers;
	serverInfo_t  globalServers[MAX_GLOBAL_SERVERS];
	// additional global servers
	int			numGlobalServerAddresses;
	netadr_t		globalServerAddresses[MAX_GLOBAL_SERVERS];

	int			numfavoriteservers;
	serverInfo_t	favoriteServers[MAX_OTHER_SERVERS];

	int pingUpdateSource;		// source currently pinging or updating

	// update server info
	netadr_t	updateServer;
	char		updateChallenge[MAX_TOKEN_CHARS];
	char		updateInfoString[MAX_INFO_STRING];

	netadr_t	authorizeServer;

	// rendering info
	glconfig_t	glconfig;
	qhandle_t	charSetShader;
	qhandle_t	whiteShader;
	qhandle_t	consoleShader;
	fontInfo_t consoleFont;
} clientStatic_t;

extern	clientStatic_t		cls;

typedef struct {
	qboolean valid;
	int num;
	qhandle_t f;
	int serverTime;
	clSnapshot_t snap;
	int serverMessageSequence;
} demoFile_t;

#define MAX_PLAYER_INFO 256

typedef struct {
	char modelName[MAX_QPATH];
} playerInfo_t;

#define MAX_TEAM_SWITCHES 512

typedef struct {
	int clientNum;
	int oldTeam;
	int newTeam;
	int serverTime;
} teamSwitch_t;

typedef struct {
	int numSnaps;
	int lastServerTime;
	int firstServerTime;
	int gameStartTime;
	int gameEndTime;
	int serverFrameTime;

	double wantedTime;
	int snapCount;

	int demoPos;
	int snapsInDemo;
	qboolean gotFirstSnap;
	qboolean skipSnap;

	qboolean endOfDemo;
	qboolean testParse;

	demoObit_t obit[MAX_DEMO_OBITS];
	int obitNum;
	//qboolean clientAlive[MAX_CLIENTS];
	qboolean offlineDemo;
	qboolean hasWarmup;

	int oldLegsAnim[MAX_GENTITIES];
	int oldTorsoAnim[MAX_GENTITIES];
	int oldLegsAnimTime[MAX_GENTITIES];
	int oldTorsoAnimTime[MAX_GENTITIES];

	int oldPsLegsAnim;
	int oldPsTorsoAnim;
	int oldPsLegsAnimTime;
	int oldPsTorsoAnimTime;
	qboolean seeking;

	int firstNonDeltaMessageNumWritten;

	itemPickup_t itemPickups[MAX_ITEM_PICKUPS];
	int numItemPickups;

	int entityPreviousEvent[MAX_GENTITIES];
	//qboolean entityInSnap[MAX_GENTITIES];
	qboolean entityInOldSnap[MAX_GENTITIES];
	entityState_t *oldEs[MAX_GENTITIES];
	int entitySnapShotTime[MAX_GENTITIES];
	int protocol;
	qboolean cpma;
	int cpmaLastTs;
	int cpmaLastTd;
	int cpmaLastTe;

	timeOut_t timeOuts[MAX_TIMEOUTS];
	int numTimeouts;

	demoFile_t demoFiles[MAX_DEMO_FILES];
	int numDemoFiles;

	int roundStarts[MAX_DEMO_ROUND_STARTS];
	int numRoundStarts;

	int pov;

	playerInfo_t playerInfo[MAX_PLAYER_INFO];
	int numPlayerInfo;

	// keeps track of current team
	int clientTeam[MAX_CLIENTS];

	teamSwitch_t teamSwitches[MAX_TEAM_SWITCHES];
	int numTeamSwitches;

	int gametype;
} demoInfo_t;

extern demoInfo_t di;

typedef struct {
    qboolean valid;
    int seekPoint;
	int demoSeekPoints[MAX_DEMO_FILES];
    clientActive_t cl;
    clientConnection_t clc;
    clientStatic_t cls;
    int numSnaps;
} rewindBackups_t;

//FIXME can make dynamic to improve seek performance
#define MAX_REWIND_BACKUPS 12   // 1000 ~ 175 megabytes, sizeof(rewindBackups_t) is 1.753348 mb

extern rewindBackups_t *rewindBackups;
extern int maxRewindBackups;

//=============================================================================

extern	vm_t			*cgvm;	// interface to cgame dll or vm
extern	vm_t			*uivm;	// interface to ui dll or vm
extern	refexport_t		re;		// interface to refresh .dll

//
// cvars
//
extern	cvar_t	*cl_nodelta;
extern	cvar_t	*cl_debugMove;
extern	cvar_t	*cl_noprint;
extern	cvar_t	*cl_timegraph;
extern	cvar_t	*cl_maxpackets;
extern	cvar_t	*cl_packetdup;
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showSend;
extern	cvar_t	*cl_timeNudge;
extern	cvar_t	*cl_showTimeDelta;
extern	cvar_t	*cl_freezeDemo;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;
extern	cvar_t	*cl_run;
extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_sensitivity;
extern	cvar_t	*cl_freelook;

extern	cvar_t	*cl_mouseAccel;
extern	cvar_t	*cl_mouseAccelOffset;
extern	cvar_t	*cl_mouseAccelStyle;
extern	cvar_t	*cl_showMouseRate;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;
extern	cvar_t	*m_filter;

extern  cvar_t  *j_pitch;
extern  cvar_t  *j_yaw;
extern  cvar_t  *j_forward;
extern  cvar_t  *j_side;
extern  cvar_t  *j_up;
extern  cvar_t  *j_pitch_axis;
extern  cvar_t  *j_yaw_axis;
extern  cvar_t  *j_forward_axis;
extern  cvar_t  *j_side_axis;
extern  cvar_t  *j_up_axis;

extern	cvar_t	*cl_timedemo;
extern	cvar_t	*cl_aviFrameRate;
extern cvar_t *cl_aviFrameRateDivider;
extern	cvar_t	*cl_aviCodec;
extern cvar_t *cl_aviAllowLargeFiles;
extern cvar_t *cl_aviFetchMode;
extern cvar_t *cl_aviExtension;
extern cvar_t *cl_freezeDemoPauseVideoRecording;

extern	cvar_t	*cl_activeAction;

extern	cvar_t	*cl_allowDownload;
extern  cvar_t  *cl_downloadMethod;
extern	cvar_t	*cl_conXOffset;
extern	cvar_t	*cl_inGameVideo;

extern	cvar_t	*cl_lanForcePackets;
extern	cvar_t	*cl_autoRecordDemo;

extern	cvar_t	*cl_consoleKeys;

#ifdef USE_MUMBLE
extern	cvar_t	*cl_useMumble;
extern	cvar_t	*cl_mumbleScale;
#endif

#ifdef USE_VOIP
// cl_voipSendTarget is a string: "all" to broadcast to everyone, "none" to
//  send to no one, or a comma-separated list of client numbers:
//  "0,7,2,23" ... an empty string is treated like "all".
extern	cvar_t	*cl_voipUseVAD;
extern	cvar_t	*cl_voipVADThreshold;
extern	cvar_t	*cl_voipSend;
extern	cvar_t	*cl_voipSendTarget;
extern	cvar_t	*cl_voipGainDuringCapture;
extern	cvar_t	*cl_voipCaptureMult;
extern	cvar_t	*cl_voipShowMeter;
extern	cvar_t	*cl_voip;

// 20ms at 48k
#define VOIP_MAX_FRAME_SAMPLES         ( 20 * 48 )

// 3 frame is 60ms of audio, the max opus will encode at once
#define VOIP_MAX_PACKET_FRAMES         3
#define VOIP_MAX_PACKET_SAMPLES                ( VOIP_MAX_FRAME_SAMPLES * VOIP_MAX_PACKET_FRAMES )

#endif

extern cvar_t	*cl_useq3gibs;
extern cvar_t	*cl_consoleAsChat;
extern cvar_t *cl_numberPadInput;
extern cvar_t *cl_maxRewindBackups;
extern cvar_t *cl_keepDemoFileInMemory;
extern cvar_t *cl_demoFileCheckSystem;
extern cvar_t *cl_demoFile;
extern cvar_t *cl_demoFileBaseName;
extern cvar_t *cl_downloadWorkshops;

extern double Overf;

//=================================================

//
// cl_main
//

void CL_Init (void);
void CL_FlushMemory(void);
void CL_ShutdownAll(void);
void CL_AddReliableCommand(const char *cmd, qboolean isDisconnectCmd);

void CL_StartHunkUsers( qboolean rendererOnly );

void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_Vid_Restart_f( void );
void CL_Snd_Restart_f (void);
void CL_StartDemoLoop( void );
void CL_NextDemo( void );
void CL_ReadDemoMessage (qboolean seeking);
void CL_StopRecord_f(void);

void CL_InitDownloads(void);
void CL_NextDownload(void);

void CL_GetPing( int n, char *buf, int buflen, int *pingtime );
void CL_GetPingInfo( int n, char *buf, int buflen );
void CL_ClearPing( int n );
int CL_GetPingQueueCount( void );

void CL_ShutdownRef( void );
void CL_InitRef( void );
qboolean CL_CDKeyValidate( const char *key, const char *checksum );
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen );

qboolean CL_CheckPaused(void);

//
// cl_input
//
typedef struct {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_ClearState (void);
void CL_ReadPackets (void);

void CL_WritePacket( void );
void IN_CenterView (void);

void CL_VerifyCode( void );

float CL_KeyState (kbutton_t *key);

//
// cl_parse.c
//
extern int cl_connectedToPureServer;
extern int cl_connectedToCheatServer;

#ifdef USE_VOIP
void CL_Voip_f( void );
#endif

void CL_SystemInfoChanged( void );
void CL_ParseServerMessage( msg_t *msg );
void CL_ParseExtraServerMessage (demoFile_t *df, msg_t *msg);

//====================================================================

void	CL_ServerInfoPacket( netadr_t from, msg_t *msg );
void	CL_LocalServers_f( void );
void	CL_GlobalServers_f( void );
void	CL_FavoriteServers_f( void );
void	CL_Ping_f( void );
qboolean CL_UpdateVisiblePings_f( int source );


//
// console
//

// not used outside of cl_console.c
//void Con_CheckResize (void);

void Con_Init (void);
void Con_Clear_f (void);
void Con_ToggleConsole_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_DrawConsoleLinesOver (int xpos, int ypos, int numLines);
void Con_RunConsole (void);
void Con_DrawConsole (void);
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
void Con_Close( void );

void CL_LoadConsoleHistory( void );
void CL_SaveConsoleHistory( void );

//
// cl_scrn.c
//
void	SCR_Init (void);
void	SCR_UpdateScreen (void);

void	SCR_DebugGraph (float value, int color);

int		SCR_GetBigStringWidth( const char *str );	// returns in virtual 640x480 coordinates

void	SCR_AdjustFrom640( float *x, float *y, float *w, float *h );
void	SCR_FillRect( float x, float y, float width, float height, 
					 const float *color );
void	SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void	SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname );

void	SCR_DrawBigString( int x, int y, const char *s, float alpha, qboolean noColorEscape );			// draws a string with embedded color control characters with fade
void	SCR_DrawBigStringColor( int x, int y, const char *s, vec4_t color, qboolean noColorEscape );	// ignores embedded color control characters
void	SCR_DrawSmallStringExt( float x, float y, float cwidth, float cheight, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape );
void	SCR_DrawSmallChar( int x, int y, int ch );
void SCR_DrawSmallCharExt( float x, float y, float width, float height, int ch );
//void SCR_DrawChar (int x, int y, float size, int ch);
void SCR_Text_Paint (float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *fontOrig);

//
// cl_cin.c
//

void CL_PlayCinematic_f( void );
void SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status CIN_StopCinematic(int handle);
e_status CIN_RunCinematic (int handle);
void CIN_DrawCinematic (int handle);
void CIN_SetExtents (int handle, int x, int y, int w, int h);
void CIN_SetLooping (int handle, qboolean loop);
void CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);

//
// cl_cgame.c
//
void CL_InitCGame( void );
void CL_ShutdownCGame( void );
qboolean CL_GameCommand( void );
void CL_CGameRendering( stereoFrame_t stereo );
void CL_SetCGameTime( void );
void CL_FirstSnapshot( void );
void CL_ShaderStateChanged(void);

//
// cl_ui.c
//
void CL_InitUI( void );
void CL_ShutdownUI( void );
int Key_GetCatcher( void );
void Key_SetCatcher( int catcher );
void LAN_LoadCachedServers( void );
void LAN_SaveServersToCache( void );


//
// cl_net_chan.c
//
void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg);	//int length, const byte *data );
qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg );

//
// cl_avi.c
//
qboolean CL_OpenAVIForWriting (aviFileData_t *afd, const char *filename, qboolean us, qboolean avi, qboolean noSoundAvi, qboolean wav, qboolean tga, qboolean jpg, qboolean png, qboolean depth, qboolean split, qboolean left);
void CL_TakeVideoFrame (aviFileData_t *afd);
void CL_WriteAVIVideoFrame (aviFileData_t *afd, const byte *imageBuffer, int size);
void CL_WriteAVIAudioFrame (aviFileData_t *afd, const byte *pcmBuffer, int size);
qboolean CL_CloseAVI (aviFileData_t *afd, qboolean us);
//qboolean CL_VideoRecording (aviFileData_t *afd);

//
// cl_main.c
//
void CL_WriteDemoMessage ( msg_t *msg, int headerBytes );

void CL_ParseSnapshot( msg_t *msg, clSnapshot_t *sn, int serverMessageSequence, qboolean justPeek );
void CL_ParseVoipSpeex (msg_t *msg, qboolean checkForFlags, qboolean justPeek);
void CL_ParseVoip (msg_t *msg, qboolean ignoreData);
qboolean CL_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot);
void CL_Pause_f (void);
void CL_AddAt (int serverTime, const char *clockTime, const char *command);

qboolean CL_GetServerCommand (int serverCommandNumber);

#endif  // client_h_included

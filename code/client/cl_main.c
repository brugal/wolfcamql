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
// cl_main.c  -- client main loop

#include "client.h"
#include "cl_avi.h"
#include "keys.h"
#include "snd_local.h"
#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"
#include <limits.h>

#ifdef USE_LOCAL_HEADERS
#       include "SDL_opengl.h"
#else
#       include <SDL_opengl.h>
#endif

#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif


#ifdef USE_MUMBLE
cvar_t	*cl_useMumble;
cvar_t	*cl_mumbleScale;
#endif

#ifdef USE_VOIP
cvar_t	*cl_voipUseVAD;
cvar_t	*cl_voipVADThreshold;
cvar_t	*cl_voipSend;
cvar_t	*cl_voipSendTarget;
cvar_t	*cl_voipGainDuringCapture;
cvar_t	*cl_voipCaptureMult;
cvar_t	*cl_voipShowMeter;
cvar_t	*cl_voipProtocol;
cvar_t	*cl_voip;
cvar_t	*cl_voipOverallGain;
cvar_t	*cl_voipGainOtherPlayback;
#endif

#ifdef USE_RENDERER_DLOPEN
cvar_t	*cl_renderer;
#endif

cvar_t	*cl_nodelta;
cvar_t	*cl_debugMove;

cvar_t	*cl_noprint;
#ifdef UPDATE_SERVER_NAME
cvar_t	*cl_motd;
#endif

cvar_t	*rcon_client_password;
cvar_t	*rconAddress;

cvar_t	*cl_timeout;
cvar_t	*cl_maxpackets;
cvar_t	*cl_packetdup;
cvar_t	*cl_timeNudge;
cvar_t	*cl_showTimeDelta;
cvar_t	*cl_freezeDemo;

cvar_t	*cl_shownet;
cvar_t	*cl_showSend;
cvar_t	*cl_timedemo;
cvar_t	*cl_timedemoLog;
cvar_t	*cl_autoRecordDemo;
cvar_t	*cl_aviFrameRate;
cvar_t *cl_aviFrameRateDivider;
cvar_t	*cl_aviCodec;
cvar_t *cl_aviAllowLargeFiles;
cvar_t *cl_aviFetchMode;
cvar_t *cl_aviExtension;
cvar_t *cl_aviPipeCommand;
cvar_t *cl_aviPipeExtension;
cvar_t *cl_aviNoAudioHWOutput;
cvar_t *cl_aviPrimeAudioRate;
cvar_t *cl_aviAudioWaitForVideoFrame;

cvar_t	*cl_forceavidemo;
cvar_t *cl_freezeDemoPauseVideoRecording;
cvar_t *cl_freezeDemoPauseMusic;

cvar_t	*cl_freelook;
cvar_t	*cl_sensitivity;

cvar_t	*cl_mouseAccel;
cvar_t	*cl_mouseAccelOffset;
cvar_t	*cl_mouseAccelStyle;
cvar_t	*cl_showMouseRate;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;
cvar_t	*m_filter;

cvar_t  *j_pitch;
cvar_t  *j_yaw;
cvar_t  *j_forward;
cvar_t  *j_side;
cvar_t *j_up;
cvar_t  *j_pitch_axis;
cvar_t  *j_yaw_axis;
cvar_t  *j_forward_axis;
cvar_t  *j_side_axis;
cvar_t *j_up_axis;

cvar_t	*cl_activeAction;

cvar_t	*cl_motdString;

cvar_t	*cl_allowDownload;
cvar_t	*cl_conXOffset;
cvar_t	*cl_inGameVideo;
cvar_t *cl_cinematicIgnoreSeek;

cvar_t	*cl_serverStatusResendTime;

cvar_t	*cl_lanForcePackets;

cvar_t	*cl_guidServerUniq;

cvar_t	*cl_consoleKeys;

cvar_t	*cl_rate;

cvar_t	*cl_useq3gibs;
cvar_t	*cl_consoleAsChat;
cvar_t *cl_numberPadInput;
cvar_t *cl_maxRewindBackups;
cvar_t *cl_keepDemoFileInMemory;
cvar_t *cl_demoFileCheckSystem;
cvar_t *cl_demoFile;
cvar_t *cl_demoFileBaseName;
cvar_t *cl_downloadWorkshops;

clientActive_t		cl;
clientConnection_t	clc;
clientStatic_t		cls;
vm_t				*cgvm;

char                            cl_reconnectArgs[MAX_OSPATH];
char		cl_oldGame[MAX_QPATH];
qboolean	cl_oldGameSet;

// Structure containing functions exported from refresh DLL
refexport_t	re;
#ifdef USE_RENDERER_DLOPEN
static void		*rendererLib = NULL;
#endif

demoInfo_t di;
rewindBackups_t *rewindBackups;
int maxRewindBackups;

ping_t	cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];

double Overf = 0.0;

aviFileData_t afdMain;
aviFileData_t afdLeft;
aviFileData_t afdRight;

aviFileData_t afdDepth;
aviFileData_t afdDepthLeft;
aviFileData_t afdDepthRight;

GLfloat *Video_DepthBuffer = NULL;
byte *ExtraVideoBuffer = NULL;
qboolean SplitVideo = qfalse;

#if defined __USEA3D && defined __A3D_GEOM
	void hA3Dg_ExportRenderGeom (refexport_t *incoming_re);
#endif

static int noGameRestart = qfalse;

//extern dma_t dma;
extern cvar_t *s_backend;

extern void SV_BotFrame( int time );

static void CL_CheckForResend( void );
static void CL_ShowIP_f(void);
static void CL_ServerStatus_f(void);
static void CL_ServerStatusResponse( netadr_t from, msg_t *msg );

static void CL_ReadExtraDemoMessage (demoFile_t *df);
static void CL_CheckWorkshopDownload (void);

void CL_StopVideo_f (void);
static void stream_demo_look_ahead (void);
static qboolean check_for_older_uncompressed_demo (void);

/*
===============
CL_CDDialog

Called by Com_Error when a cd is needed
===============
*/
void CL_CDDialog( void ) {
	cls.cddialog = qtrue;	// start it next frame
}

#ifdef USE_MUMBLE
static
void CL_UpdateMumble(void)
{
	vec3_t pos, forward, up;
	float scale = cl_mumbleScale->value;
	float tmp;

	if(!cl_useMumble->integer)
		return;

	// !!! FIXME: not sure if this is even close to correct.
	AngleVectors( cl.snap.ps.viewangles, forward, NULL, up);

	pos[0] = cl.snap.ps.origin[0] * scale;
	pos[1] = cl.snap.ps.origin[2] * scale;
	pos[2] = cl.snap.ps.origin[1] * scale;

	tmp = forward[1];
	forward[1] = forward[2];
	forward[2] = tmp;

	tmp = up[1];
	up[1] = up[2];
	up[2] = tmp;

	if(cl_useMumble->integer > 1) {
		fprintf(stderr, "%f %f %f, %f %f %f, %f %f %f\n",
			pos[0], pos[1], pos[2],
			forward[0], forward[1], forward[2],
			up[0], up[1], up[2]);
	}

	mumble_update_coordinates(pos, forward, up);
}
#endif


#ifdef USE_VOIP
static
void CL_UpdateVoipIgnore(const char *idstr, qboolean ignore)
{
	if ((*idstr >= '0') && (*idstr <= '9')) {
		const int id = atoi(idstr);
		if ((id >= 0) && (id < MAX_CLIENTS)) {
			clc.voipIgnore[id] = ignore;
			CL_AddReliableCommand(va("voip %s %d",
			                         ignore ? "ignore" : "unignore", id), qfalse);
			Com_Printf("VoIP: %s ignoring player #%d\n",
			            ignore ? "Now" : "No longer", id);
			return;
		}
	}
	Com_Printf("VoIP: invalid player ID#\n");
}

static
void CL_UpdateVoipGain(const char *idstr, float gain)
{
	if ((*idstr >= '0') && (*idstr <= '9')) {
		const int id = atoi(idstr);
		if (gain < 0.0f)
			gain = 0.0f;
		if ((id >= 0) && (id < MAX_CLIENTS)) {
			clc.voipGain[id] = gain;
			Com_Printf("VoIP: player #%d gain now set to %f\n", id, gain);
		}
	}
}

void CL_Voip_f( void )
{
	const char *cmd = Cmd_Argv(1);
	const char *reason = NULL;

	if (clc.state != CA_ACTIVE)
		reason = "Not connected to a server";
	else if (!clc.voipCodecInitialized)
		reason = "Voip codec not initialized";
	else if (!clc.voipEnabled  &&  !clc.demoplaying)
		reason = "Server doesn't support VoIP";
	else if (!clc.demoplaying && (Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")))
		reason = "running in single-player mode";

	if (reason != NULL) {
		Com_Printf("VoIP: command ignored: %s\n", reason);
		return;
	}

	if (strcmp(cmd, "ignore") == 0) {
		CL_UpdateVoipIgnore(Cmd_Argv(2), qtrue);
	} else if (strcmp(cmd, "unignore") == 0) {
		CL_UpdateVoipIgnore(Cmd_Argv(2), qfalse);
	} else if (strcmp(cmd, "gain") == 0) {
		if (Cmd_Argc() > 3) {
			CL_UpdateVoipGain(Cmd_Argv(2), atof(Cmd_Argv(3)));
		} else if (Q_isanumber(Cmd_Argv(2))) {
			int id = atoi(Cmd_Argv(2));
			if (id >= 0 && id < MAX_CLIENTS) {
				Com_Printf("VoIP: current gain for player #%d "
					"is %f\n", id, clc.voipGain[id]);
			} else {
				Com_Printf("VoIP: invalid player ID#\n");
			}
		} else {
			Com_Printf("usage: voip gain <playerID#> [value]\n");
		}
	} else if (strcmp(cmd, "muteall") == 0) {
		Com_Printf("VoIP: muting incoming voice\n");
		CL_AddReliableCommand("voip muteall", qfalse);
		clc.voipMuteAll = qtrue;
	} else if (strcmp(cmd, "unmuteall") == 0) {
		Com_Printf("VoIP: unmuting incoming voice\n");
		CL_AddReliableCommand("voip unmuteall", qfalse);
		clc.voipMuteAll = qfalse;
	} else {
		Com_Printf("usage: voip [un]ignore <playerID#>\n"
		           "       voip [un]muteall\n"
		           "       voip gain <playerID#> [value]\n");
	}
}


static
void CL_VoipNewGeneration(void)
{
	// don't have a zero generation so new clients won't match, and don't
	//  wrap to negative so MSG_ReadLong() doesn't "fail."
	clc.voipOutgoingGeneration++;
	if (clc.voipOutgoingGeneration <= 0)
		clc.voipOutgoingGeneration = 1;
	clc.voipPower = 0.0f;
	clc.voipOutgoingSequence = 0;

	opus_encoder_ctl(clc.opusEncoder, OPUS_RESET_STATE);
}

/*
===============
CL_VoipParseTargets

sets clc.voipTargets according to cl_voipSendTarget
Generally we don't want who's listening to change during a transmission,
so this is only called when the key is first pressed
===============
*/
void CL_VoipParseTargets(void)
{
	const char *target = cl_voipSendTarget->string;
	char *end;
	int val;

	Com_Memset(clc.voipTargets, 0, sizeof(clc.voipTargets));
	clc.voipFlags &= ~VOIP_SPATIAL;

	while(target)
	{
		while(*target == ',' || *target == ' ')
			target++;

		if(!*target)
			break;
		
		if(isdigit(*target))
		{
			val = strtol(target, &end, 10);
			target = end;
		}
		else
		{
			if(!Q_stricmpn(target, "all", 3))
			{
				Com_Memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
				return;
			}
			if(!Q_stricmpn(target, "spatial", 7))
			{
				clc.voipFlags |= VOIP_SPATIAL;
				target += 7;
				continue;
			}
			else
			{
				if(!Q_stricmpn(target, "attacker", 8))
				{
					val = VM_Call(cgvm, CG_LAST_ATTACKER);
					target += 8;
				}
				else if(!Q_stricmpn(target, "crosshair", 9))
				{
					val = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
					target += 9;
				}
				else
				{
					while(*target && *target != ',' && *target != ' ')
						target++;

					continue;
				}

				if(val < 0)
					continue;
			}
		}

		if(val < 0 || val >= MAX_CLIENTS)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: VoIP "
				   "target %d is not a valid client "
				   "number\n", val);

			continue;
		}

		clc.voipTargets[val / 8] |= 1 << (val % 8);
	}
}

/*
===============
CL_CaptureVoip

Record more audio from the hardware if required and encode it into Opus
 data for later transmission.
===============
*/
static
void CL_CaptureVoip(void)
{
	const float audioMult = cl_voipCaptureMult->value;
	const qboolean useVad = (cl_voipUseVAD->integer != 0);
	qboolean initialFrame = qfalse;
	qboolean finalFrame = qfalse;

#if USE_MUMBLE
	// if we're using Mumble, don't try to handle VoIP transmission ourselves.
	if (cl_useMumble->integer)
		return;
#endif

	// If your data rate is too low, you'll get Connection Interrupted warnings
	//  when VoIP packets arrive, even if you have a broadband connection.
	//  This might work on rates lower than 25000, but for safety's sake, we'll
	//  just demand it. Who doesn't have at least a DSL line now, anyhow? If
	//  you don't, you don't need VoIP.  :)
	if (cl_voip->modified || cl_rate->modified) {
		if ((cl_voip->integer) && (cl_rate->integer < 25000)) {
			Com_Printf(S_COLOR_YELLOW "Your network rate is too slow for VoIP.\n");
			Com_Printf("Set 'Data Rate' to 'LAN/Cable/xDSL' in 'Setup/System/Network'.\n");
			Com_Printf("Until then, VoIP is disabled.\n");
			Cvar_Set("cl_voip", "0");
		}
		Cvar_Set("cl_voipProtocol", cl_voip->integer ? "opus" : "");
		cl_voip->modified = qfalse;
		cl_rate->modified = qfalse;
	}

	if (!clc.voipCodecInitialized)
		return;  // just in case this gets called at a bad time.

	if (clc.voipOutgoingDataSize > 0)
		return;  // packet is pending transmission, don't record more yet.

	if (cl_voipUseVAD->modified) {
		Cvar_Set("cl_voipSend", (useVad) ? "1" : "0");
		cl_voipUseVAD->modified = qfalse;
	}

	if ((useVad) && (!cl_voipSend->integer))
		Cvar_Set("cl_voipSend", "1");  // lots of things reset this.

	if (cl_voipSend->modified) {
		qboolean dontCapture = qfalse;
		if (clc.state != CA_ACTIVE)
			dontCapture = qtrue;  // not connected to a server.
		else if (!clc.voipEnabled  &&  !clc.demoplaying)
			dontCapture = qtrue;  // server doesn't support VoIP.
		else if (clc.demoplaying  &&  !clc.demorecording)
			dontCapture = qtrue;  // playing back a demo and not recording another demo.
		else if ( cl_voip->integer == 0 )
			dontCapture = qtrue;  // client has VoIP support disabled.
		else if ( audioMult == 0.0f )
			dontCapture = qtrue;  // basically silenced incoming audio.

		cl_voipSend->modified = qfalse;

		if(dontCapture)
		{
			Cvar_Set("cl_voipSend", "0");
			return;
		}

		if (cl_voipSend->integer) {
			initialFrame = qtrue;
		} else {
			finalFrame = qtrue;
		}
	}

	// try to get more audio data from the sound card...

	if (initialFrame) {
		S_MasterGain(Com_Clamp(0.0f, 1.0f, cl_voipGainDuringCapture->value));
		S_StartCapture();
		CL_VoipNewGeneration();
		CL_VoipParseTargets();
	}

	if ((cl_voipSend->integer) || (finalFrame)) { // user wants to capture audio?
		int samples = S_AvailableCaptureSamples();
		const int packetSamples = (finalFrame) ? VOIP_MAX_FRAME_SAMPLES : VOIP_MAX_PACKET_SAMPLES;

		// enough data buffered in audio hardware to process yet?
		if (samples >= packetSamples) {
			// audio capture is always MONO16.
			static int16_t sampbuffer[VOIP_MAX_PACKET_SAMPLES];
			float voipPower = 0.0f;
			int voipFrames;
			int i, bytes;

			if (samples > VOIP_MAX_PACKET_SAMPLES)
				samples = VOIP_MAX_PACKET_SAMPLES;

			// !!! FIXME: maybe separate recording from encoding, so voipPower
			// !!! FIXME:  updates faster than 4Hz?

			samples -= samples % VOIP_MAX_FRAME_SAMPLES;
			if (samples != 120 && samples != 240 && samples != 480 && samples != 960 && samples != 1920 && samples != 2880 ) {
				Com_Printf("Voip: bad number of samples %d\n", samples);
				return;
			}
			voipFrames = samples / VOIP_MAX_FRAME_SAMPLES;

			S_Capture(samples, (byte *) sampbuffer);  // grab from audio card.

			// check the "power" of this packet...
			for (i = 0; i < samples; i++) {
				const float flsamp = (float) sampbuffer[i];
				const float s = fabs(flsamp);
				voipPower += s * s;
				sampbuffer[i] = (int16_t) ((flsamp) * audioMult);
			}

			// encode raw audio samples into Opus data...
			bytes = opus_encode(clc.opusEncoder, sampbuffer, samples,
								(unsigned char *) clc.voipOutgoingData,
								sizeof (clc.voipOutgoingData));
			if ( bytes <= 0 ) {
				Com_DPrintf("VoIP: Error encoding %d samples\n", samples);
				bytes = 0;
			}

			clc.voipPower = (voipPower / (32768.0f * 32768.0f *
							  ((float) samples))) * 100.0f;

			if ((useVad) && (clc.voipPower < cl_voipVADThreshold->value)) {
				CL_VoipNewGeneration();  // no "talk" for at least 1/4 second.
			} else {
				clc.voipOutgoingDataSize = bytes;
				clc.voipOutgoingDataFrames = voipFrames;

				Com_DPrintf("VoIP: Send %d frames, %d bytes, %f power\n",
				            voipFrames, bytes, clc.voipPower);

				#if 0
				static FILE *encio = NULL;
				if (encio == NULL) encio = fopen("voip-outgoing-encoded.bin", "wb");
				if (encio != NULL) { fwrite(clc.voipOutgoingData, bytes, 1, encio); fflush(encio); }
				static FILE *decio = NULL;
				if (decio == NULL) decio = fopen("voip-outgoing-decoded.bin", "wb");
				if (decio != NULL) { fwrite(sampbuffer, voipFrames * VOIP_MAX_FRAME_SAMPLES * 2, 1, decio); fflush(decio); }
				#endif
			}
		}
	}

	// User requested we stop recording, and we've now processed the last of
	//  any previously-buffered data. Pause the capture device, etc.
	if (finalFrame) {
		S_StopCapture();
		S_MasterGain(1.0f);
		clc.voipPower = 0.0f;  // force this value so it doesn't linger.
	}
}
#endif

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is guaranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char *cmd, qboolean isDisconnectCmd)
{
	int unacknowledged = clc.reliableSequence - clc.reliableAcknowledge;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// also leave one slot open for the disconnect command in this case.

	if (clc.demoplaying) {
		return;
	}

	if ((isDisconnectCmd && unacknowledged > MAX_RELIABLE_COMMANDS) ||
	    (!isDisconnectCmd && unacknowledged >= MAX_RELIABLE_COMMANDS))
	{
		if(com_errorEntered)
			return;
		else
			Com_Error(ERR_DROP, "Client command overflow");
	}

	Q_strncpyz(clc.reliableCommands[++clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1)],
		   cmd, sizeof(*clc.reliableCommands));
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/

void CL_WriteDemoMessage ( msg_t *msg, int headerBytes ) {
	int		len, swlen;

	// write the packet sequence
	len = clc.serverMessageSequence;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demoWriteFile);
	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demoWriteFile);
	FS_Write(msg->data + headerBytes, len, clc.demoWriteFile);
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f( void ) {
	int		len;

	if ( !clc.demorecording ) {
		Com_Printf ("Not recording a demo.\n");
		return;
	}

	Com_Printf("done recording, last snapshot number %d\n", cl.snap.messageNum);

	// finish up
	len = -1;
	FS_Write (&len, 4, clc.demoWriteFile);
	FS_Write (&len, 4, clc.demoWriteFile);
	FS_FCloseFile (clc.demoWriteFile);
	clc.demoWriteFile = 0;
	clc.demorecording = qfalse;
	clc.spDemoRecording = qfalse;
	Com_Printf ("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename( int number, char *fileName, int fileNameSize ) {
	int		a,b,c,d;

	if(number < 0 || number > 9999)
		number = 9999;

	a = number / 1000;
	number -= a*1000;
	b = number / 100;
	number -= b*100;
	c = number / 10;
	number -= c*10;
	d = number;

	Com_sprintf( fileName, fileNameSize, "demo%i%i%i%i"
		, a, b, c, d );
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static char		demoName[MAX_QPATH];	// compiler bug workaround
void CL_Record_f( void ) {
	char		name[MAX_OSPATH];
	byte		bufData[MAX_MSGLEN];
	msg_t	buf;
	int			i;
	int			len;
	entityState_t	*ent;
	entityState_t	nullstate;
	char		*s;

	if ( Cmd_Argc() > 2 ) {
		Com_Printf ("record <demoname>\n");
		return;
	}

	if ( clc.demorecording ) {
#if 0
		if (!clc.spDemoRecording) {
			Com_Printf ("Already recording.\n");
		}
		return;
#endif
		CL_StopRecord_f();
	}

	if ( clc.state != CA_ACTIVE ) {
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

#if 0
  // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	if ( NET_IsLocalAddress( clc.serverAddress ) && !Cvar_VariableValue( "g_synchronousClients" ) ) {
		Com_Printf (S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}
#endif

	if ( Cmd_Argc() == 2 ) {
		s = Cmd_Argv(1);
		Q_strncpyz( demoName, s, sizeof( demoName ) );
		if (clc.realProtocol < 48) {
			Com_sprintf(name, sizeof(name), "demos/%s.dm3", demoName);
		} else {
			Com_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName, DEMOEXT, clc.realProtocol);
		}
	} else {
		int		number;

		// scan for a free demo name
		for ( number = 0 ; number <= 9999 ; number++ ) {
			CL_DemoFilename( number, demoName, sizeof( demoName ) );
			if (clc.realProtocol < 48) {
				Com_sprintf(name, sizeof(name), "demos/%s.dm3", demoName);
			} else {
				Com_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName, DEMOEXT, clc.realProtocol);
			}

			if (!FS_FileExists(name))
				break;	// file doesn't exist
		}
	}

	// open the demo file

	Com_Printf ("recording to %s.\n", name);
	clc.demoWriteFile = FS_FOpenFileWrite( name );
	if ( !clc.demoWriteFile ) {
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = qtrue;
	if (Cvar_VariableValue("ui_recordSPDemo")) {
	  clc.spDemoRecording = qtrue;
	} else {
	  clc.spDemoRecording = qfalse;
	}

	Q_strncpyz( clc.demoName, demoName, sizeof( clc.demoName ) );

	// don't start saving messages until a non-delta compressed message is received
	clc.demowaiting = qtrue;

	// write out the gamestate message
	if (clc.realProtocol <= 48) {
		MSG_InitOOB(&buf, bufData, sizeof(bufData));
	} else {
		MSG_Init (&buf, bufData, sizeof(bufData));
		MSG_Bitstream(&buf);
	}

	// NOTE, MRE: all server->client messages now acknowledge
	// protocol 43 doesn't have this
	//FIXME what about 44, 45 ?
	if (clc.realProtocol > 43) {
		MSG_WriteLong( &buf, clc.reliableSequence );
	}

	MSG_WriteByte (&buf, svc_gamestate);
	MSG_WriteLong (&buf, clc.serverCommandSequence );

	// configstrings
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( !cl.gameState.stringOffsets[i] ) {
			continue;
		}
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		MSG_WriteByte (&buf, svc_configstring);
		MSG_WriteShort (&buf, i);
		MSG_WriteBigString (&buf, s);
		//Com_Printf("%d: '%s'\n", i, s);
	}

	// baselines
	Com_Memset (&nullstate, 0, sizeof(nullstate));
	for ( i = 0; i < MAX_GENTITIES ; i++ ) {
		ent = &cl.entityBaselines[i];
		if ( !ent->number ) {
			continue;
		}
		MSG_WriteByte (&buf, svc_baseline);
		MSG_WriteDeltaEntity (&buf, &nullstate, ent, qtrue );
		//Com_Printf("wrote ent %d\n", i);
	}

	if (clc.realProtocol > 48) {
		MSG_WriteByte( &buf, svc_EOF );
	}

	// finished writing the gamestate stuff

	//FIXME protocol >= 66
	if (clc.realProtocol <= 48) {
		// pass, no clientNum or checksumFeed
		//FIXME is this right?  -- 2024-08-22 yes
		MSG_WriteByte( &buf, svc_bad );
		//MSG_WriteByte( &buf, svc_EOF );
#if 1
		if ((buf.bit & 7) != 0) {
			buf.cursize++;
			buf.bit = buf.cursize << 3;
		}
#endif
		//MSG_WriteByte( &buf, svc_EOF );
	} else {
		// write the client num
		MSG_WriteLong(&buf, clc.clientNum);
		// write the checksum feed
		MSG_WriteLong(&buf, clc.checksumFeed);

		// finished writing the client packet
		MSG_WriteByte( &buf, svc_EOF );
	}


	// write it to the demo file
	len = LittleLong( clc.serverMessageSequence - 1 );

	FS_Write (&len, 4, clc.demoWriteFile);

	len = LittleLong (buf.cursize);
	FS_Write (&len, 4, clc.demoWriteFile);
	FS_Write (buf.data, buf.cursize, clc.demoWriteFile);

	di.firstNonDeltaMessageNumWritten = -1;

	// the rest of the demo file will be copied from net messages or from
	// the demo file being read
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoFrameDurationSDev
=================
*/
static float CL_DemoFrameDurationSDev( void )
{
	int i;
	int numFrames;
	float mean = 0.0f;
	float variance = 0.0f;

	if( ( clc.timeDemoFrames - 1 ) > MAX_TIMEDEMO_DURATIONS )
		numFrames = MAX_TIMEDEMO_DURATIONS;
	else
		numFrames = clc.timeDemoFrames - 1;

	for( i = 0; i < numFrames; i++ )
		mean += clc.timeDemoDurations[ i ];
	mean /= numFrames;

	for( i = 0; i < numFrames; i++ )
	{
		float x = clc.timeDemoDurations[ i ];

		variance += ( ( x - mean ) * ( x - mean ) );
	}
	variance /= numFrames;

	return sqrt( variance );
}

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted( void )
{
	char buffer[ MAX_STRING_CHARS ];

	if (di.testParse) {
		di.endOfDemo = qtrue;
		Com_Printf("end of demo\n");
		return;
	}

	if( cl_timedemo && cl_timedemo->integer )
	{
		int	time;

		time = Sys_Milliseconds() - clc.timeDemoStart;
		if( time > 0 )
		{
			// Millisecond times are frame durations:
			// minimum/average/maximum/std deviation
			Com_sprintf( buffer, sizeof( buffer ),
					"%i frames %3.1f seconds %3.1f fps %d.0/%.1f/%d.0/%.1f ms  cgameTime: %f seconds\n",
					clc.timeDemoFrames,
					time/1000.0,
					clc.timeDemoFrames*1000.0 / time,
					clc.timeDemoMinDuration,
					time / (float)clc.timeDemoFrames,
					clc.timeDemoMaxDuration,
						 CL_DemoFrameDurationSDev( ),
						 (float)clc.cgameTime / 1000.0);
			Com_Printf( "%s", buffer );

			// Write a log of all the frame durations
			if( cl_timedemoLog && strlen( cl_timedemoLog->string ) > 0 )
			{
				int i;
				int numFrames;
				fileHandle_t f;

				if( ( clc.timeDemoFrames - 1 ) > MAX_TIMEDEMO_DURATIONS )
					numFrames = MAX_TIMEDEMO_DURATIONS;
				else
					numFrames = clc.timeDemoFrames - 1;

				f = FS_FOpenFileWrite( cl_timedemoLog->string );
				if( f )
				{
					FS_Printf( f, "# %s", buffer );

					for( i = 0; i < numFrames; i++ )
						FS_Printf( f, "%d\n", clc.timeDemoDurations[ i ] );

					FS_FCloseFile( f );
					Com_Printf( "%s written\n", cl_timedemoLog->string );
				}
				else
				{
					Com_Printf( "Couldn't open %s for writing\n",
							cl_timedemoLog->string );
				}
			}
		}
	}

	CL_Disconnect( qtrue );
	CL_NextDemo();
}

static void CL_WriteNonDeltaDemoMessage (msg_t *inMsg)
{
	msg_t outMsg;
	byte data[MAX_MSGLEN];
	int r;
	int cmd;
	int seq;
	char *s;
	entityState_t nullstate;
	entityState_t *es;
	byte	areamask[MAX_MAP_AREA_BYTES];
	clSnapshot_t *old;
	int i;
	int len;
	playerState_t tmpps;
	qboolean haveSnapshot = qfalse;

	if (clc.realProtocol <= 48) {
		MSG_BeginReadingOOB(inMsg);
		MSG_InitOOB(&outMsg, data, sizeof(data));
	} else {
		MSG_BeginReading(inMsg);
		MSG_Init(&outMsg, data, sizeof(data));
		MSG_Bitstream(&outMsg);
	}

	// CL_ParseServerMessage

	// reliable seq ack
	// NOTE, MRE: all server->client messages now acknowledge
	// protocol 43 doesn't have this
	//FIXME what about 44, 45 ?
	if (clc.realProtocol >= 46) {
		r = MSG_ReadLong(inMsg);
		MSG_WriteLong(&outMsg, r);
	}

	while (1) {
		qboolean dataFollowsEOF = qfalse;

		if (clc.realProtocol <= 48) {
			if (inMsg->readcount == inMsg->cursize) {
				// "END OF MESSAGE"
				//Com_Printf("wrnd END OF MESSAGE\n");
				break;
			}
		}

		cmd = MSG_ReadByte(inMsg);
		MSG_WriteByte(&outMsg, cmd);

		// See if this is an extension command after the EOF, which means we
		// have speex voip data.
		if (cmd == svc_EOF  &&  MSG_LookaheadByte(inMsg) == svc_extension) {
			dataFollowsEOF = qtrue;
			r = MSG_ReadByte(inMsg);
			MSG_WriteByte(&outMsg, r);

			cmd = MSG_ReadByte(inMsg);
			MSG_WriteByte(&outMsg, cmd);
			if (cmd == -1) {
				cmd = svc_EOF;
			}
		}

		if (cmd == svc_EOF) {
			break;
		}

		//Com_Printf("wrnd cmd: %d  (%d  %d)\n", cmd, inMsg->readcount, inMsg->cursize);

		switch ( cmd ) {
		default:
			Com_Error(ERR_DROP, "%s: Illegible server message %d", __FUNCTION__, cmd);
			break;
		case svc_nop:
			MSG_WriteByte(&outMsg, svc_nop);
			break;
		case svc_serverCommand:
			//CL_ParseCommandString( msg );
			seq = MSG_ReadLong(inMsg);
			MSG_WriteLong(&outMsg, seq);

			s = MSG_ReadString(inMsg);
			MSG_WriteString(&outMsg, s);

			break;
		case svc_gamestate:
			//CL_ParseGamestate( msg );
			while (1) {
				cmd = MSG_ReadByte(inMsg);
				MSG_WriteByte(&outMsg, cmd);

				if (cmd == svc_EOF) {
					break;
				}

				if (cmd == svc_configstring) {
					r = MSG_ReadShort(inMsg);
					MSG_WriteShort(&outMsg, r);

					s = MSG_ReadBigString(inMsg);
					MSG_WriteBigString(&outMsg, s);
				} else if (cmd == svc_baseline) {
					r = MSG_ReadBits(inMsg, GENTITYNUM_BITS);
					MSG_WriteBits(&outMsg, r, GENTITYNUM_BITS);

					Com_Memset(&nullstate, 0, sizeof(nullstate));
					es = &cl.entityBaselines[r];
					MSG_ReadDeltaEntity(inMsg, &nullstate, es, r);
					MSG_WriteDeltaEntity(&outMsg, es, &nullstate, qtrue);
				}
			}

			//FIXME protocol >= 66
			if (clc.realProtocol <= 48) {
				// pass, no clientNum or checksumFeed
				//FIXME is this right?
				if ((outMsg.bit & 7) != 0) {
					outMsg.cursize++;
					outMsg.bit = outMsg.cursize << 3;
				}
			} else {
				// client num
				r = MSG_ReadLong(inMsg);
				MSG_WriteLong(&outMsg, r);

				// checksum feed
				r = MSG_ReadLong(inMsg);
				MSG_WriteLong(&outMsg, r);
			}

			break;
		case svc_snapshot: {
			entityState_t esFrom, esTo;

			//CL_ParseSnapshot( msg, NULL, qfalse );

			haveSnapshot = qtrue;

			if (clc.realProtocol < 46) {
				r = MSG_ReadLong(inMsg);  // Client command sequence.
				MSG_WriteLong(&outMsg, r);
			}

			// server time
			r = MSG_ReadLong(inMsg);
			MSG_WriteLong(&outMsg, r);

			// delta num
			r = MSG_ReadByte(inMsg);
			MSG_WriteByte(&outMsg, 0);
			// we know it's definately delta
			old = &cl.snapshots[0][(cl.snap.messageNum - r) & PACKET_MASK];

			// snap flags
			r = MSG_ReadByte(inMsg);
			MSG_WriteByte(&outMsg, r);

			// areamask
			r = MSG_ReadByte(inMsg);
			MSG_WriteByte(&outMsg, r);

			if (r > sizeof(areamask)) {
				Com_Printf("^1%s snapshot: invalid size for areamask %d\n", __FUNCTION__, r);
				goto done;
			}
			MSG_ReadData(inMsg, areamask, r);
			MSG_WriteData(&outMsg, areamask, r);

			MSG_ReadDeltaPlayerstate(inMsg, &old->ps, &tmpps);
			MSG_WriteDeltaPlayerstate(&outMsg, NULL, &cl.snap.ps);

			// CL_ParsePacketEntities

			while (1) {
				int newnum = MSG_ReadBits(inMsg, GENTITYNUM_BITS);

				//Com_Printf("write: %d   (%d - %d)\n", newnum, inMsg->readcount, inMsg->cursize);

				if (newnum == (MAX_GENTITIES - 1)) {
					break;
				}

				if (inMsg->readcount > inMsg->cursize) {
					Com_Printf("^1%s parse packet entities end of message: %d > %d entity: %d\n", __FUNCTION__, inMsg->readcount, inMsg->cursize, newnum);
					break;
				}

				MSG_ReadDeltaEntity(inMsg, &esFrom, &esTo, newnum);
			}

			for (i = 0;  i < cl.snap.numEntities;  i++) {
				es = &cl.parseEntities[(cl.snap.parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
				MSG_WriteDeltaEntity(&outMsg, &cl.entityBaselines[es->number], es, qtrue);
			}
			MSG_WriteBits(&outMsg, (MAX_GENTITIES - 1), GENTITYNUM_BITS);

			// sv_padPackets

			break;
		}
		case svc_download:
			//CL_ParseDownload( msg );

			//r = MSG_ReadShort(inMsg);
			//MSG_WriteShort(&outMsg, r);
			Com_Printf("FIXME %s  download\n", __FUNCTION__);

			break;
		case svc_extension:  // libspeex voip protocol 70 or 71
			if (dataFollowsEOF) {
				// shouldn't happen
				Com_Printf("^1%s unknown svc_extension\n", __FUNCTION__);
				break;
			} else {
				// speex with protocol checking to determine if flags present
 				// fall through and check 'cmd == svc_extension' below
			}
		case svc_voip: {
			if (!dataFollowsEOF  &&  cmd == svc_voip) {  // opus voip
				int sender;
				int generation;
				int sequence;
				int frames;
				int packetsize;
				int flags;
				unsigned char encoded[4000];
				int bytesleft;

				sender = MSG_ReadShort(inMsg);
				generation = MSG_ReadByte(inMsg);
				sequence = MSG_ReadLong(inMsg);
				frames = MSG_ReadByte(inMsg);
				packetsize = MSG_ReadShort(inMsg);
				flags = MSG_ReadBits(inMsg, VOIP_FLAGCNT);

				// write

				MSG_WriteShort(&outMsg, sender);
				MSG_WriteByte(&outMsg, generation);
				MSG_WriteLong(&outMsg, sequence);
				MSG_WriteByte(&outMsg, frames);
				MSG_WriteShort(&outMsg, packetsize);
				MSG_WriteBits(&outMsg, flags, VOIP_FLAGCNT);

				if (packetsize < 0) {
					Com_Printf("^1%s voip opus invalid packetsize %d\n", __FUNCTION__, packetsize);
					// skip this and the rest of the demo message
					goto done;
				}

				bytesleft = packetsize;
				while (bytesleft) {
					int br = bytesleft;
					if (br > sizeof(encoded)) {
						br = sizeof(encoded);
					}
					MSG_ReadData(inMsg, encoded, br);
					MSG_WriteData(&outMsg, encoded, br);
					bytesleft -= br;
				}

				break;
			} else {  // speex
				int sender;
				int generation;
				int sequence;
				int frames;
				int packetsize;
				int flags = VOIP_DIRECT;
				char encoded[1024];
				int bytesleft;

				//Com_Printf("^2%s  voip\n", __FUNCTION__);

				sender = MSG_ReadShort(inMsg);
				generation = MSG_ReadByte(inMsg);
				sequence = MSG_ReadLong(inMsg);
				frames = MSG_ReadByte(inMsg);
				packetsize = MSG_ReadShort(inMsg);

				if (!dataFollowsEOF  &&  cmd == svc_extension  &&  clc.realProtocol == 71) {
					flags = MSG_ReadBits(inMsg, VOIP_FLAGCNT);
				}

				MSG_WriteShort(&outMsg, sender);
				MSG_WriteByte(&outMsg, generation);
				MSG_WriteLong(&outMsg, sequence);
				MSG_WriteByte(&outMsg, frames);
				MSG_WriteShort(&outMsg, packetsize);

				if (!dataFollowsEOF  &&  cmd == svc_extension  &&  clc.realProtocol == 71) {
					MSG_WriteShort(&outMsg, flags);
				}

				if (packetsize < 0) {
					Com_Printf("^1%s voip invalid packetsize %d\n", __FUNCTION__, packetsize);
					// skip this and the rest of the demo message
					goto done;
				}

				bytesleft = packetsize;
				while (bytesleft) {
					int br = bytesleft;
					if (br > sizeof(encoded)) {
						br = sizeof(encoded);
					}
					MSG_ReadData(inMsg, encoded, br);
					MSG_WriteData(&outMsg, encoded, br);
					bytesleft -= br;
				}

				break;
			}
		}

		}

		if (clc.realProtocol <= 48) {
			// _inMsg.GoToNextByte();
			//Com_Printf("nextbyte  %d  %d\n", msg->readcount, msg->bit);
			if ((inMsg->bit & 7) != 0) {
				inMsg->readcount++;
				inMsg->bit = inMsg->readcount << 3;
			}
		}
	}

 done:

	if (!haveSnapshot) {
		// probably fake message with voip, return to make sure it's known that a non delta message still needs to be written
		//Com_Printf("^2 no snap, writing\n");
		CL_WriteDemoMessage(inMsg, 0);
		return;
	}

	//////////////////////////////////////////

	//FIXME is this right for protocol <= 48 ?, 2024-08-18 no error in quake3 127
	if (clc.realProtocol < 48) {
		MSG_WriteByte(&outMsg, svc_bad);
	} else {
		MSG_WriteByte(&outMsg, svc_EOF);
	}

	len = LittleLong(cl.snap.messageNum);
	FS_Write(&len, 4, clc.demoWriteFile);

	len = LittleLong(outMsg.cursize);
	FS_Write(&len, 4, clc.demoWriteFile);
	FS_Write(outMsg.data, outMsg.cursize, clc.demoWriteFile);
	if (di.firstNonDeltaMessageNumWritten == -1) {
		di.firstNonDeltaMessageNumWritten = cl.snap.messageNum;
	}
	//Com_Printf("writing non delta for %d %d\n", cl.snap.messageNum, clc.serverMessageSequence);
}

// not very strict validation, just checks if it's close enough to something valid
static qboolean CL_CheckForValidServerMessage (msg_t *msg)
{
	int cmd;
	int v;

	if (!di.olderUncompressedDemo) {
		MSG_Bitstream(msg);
	}

	// get the reliable sequence acknowledge number
	v = MSG_ReadLong( msg );
	if (v < 0) {
		Com_Printf("%s: invalid sequence ackowledge number %d\n", __FUNCTION__, v);
		return qfalse;
	}

	// parse the message
	while (1) {
		qboolean dataFollowsEOF = qfalse;

		if (msg->readcount > msg->cursize) {
			Com_Printf("%s: readcount > cursize   %d > %d\n", __FUNCTION__, msg->readcount, msg->cursize);
			return qfalse;
		}

		if (clc.realProtocol <= 48) {
			if (msg->readcount == msg->cursize) {
				//Com_Printf("slkdjfsldkfj\n");
				break;
			}
		}

		cmd = MSG_ReadByte(msg);

		// See if this is an extension command after the EOF, which means we
		// have speex voip data.
		if ((cmd == svc_EOF)  &&  (MSG_LookaheadByte(msg) == svc_extension)) {
			dataFollowsEOF = qtrue;
			MSG_ReadByte(msg);  // throw the svc_extension byte away

			cmd = MSG_ReadByte(msg);
			if (cmd == -1) {
				cmd = svc_EOF;
			}
		}

		if (cmd == svc_EOF) {
			break;
		}

		// other commands
		switch (cmd) {
		default:
			Com_Printf("%s: Illegible server message %d\n", __FUNCTION__, cmd);
			return qfalse;
		case svc_nop:
			break;
		case svc_serverCommand: {
			int seq;
			char *s;

			seq = MSG_ReadLong(msg);
			if (seq < 0) {
				Com_Printf("%s: invalid command string seq %d\n", __FUNCTION__, seq);
				return qfalse;
			}
			s = MSG_ReadString(msg);
			//FIXME debugging
			Com_Printf("command string: '%s'\n", s);
			break;
		}
		case svc_gamestate: {
			//FIXME CL_ParseGamestate(msg);
			Com_Printf("%s: gamestate\n", __FUNCTION__);
			return qtrue;
			break;
		}
		case svc_snapshot: {
			int serverTime;
			int deltaNum;
			int len;
			clSnapshot_t newSnap;

			//FIXME CL_ParseSnapshot(msg, NULL, clc.serverMessageSequence, qfalse);
			Com_Printf("%s: snapshot\n", __FUNCTION__);

			if (clc.realProtocol < 48) {
				MSG_ReadLong(msg);  // Client command sequence
			}

			serverTime = MSG_ReadLong(msg);
			if (serverTime < 0) {
				Com_Printf("%s: invalid server time %d\n", __FUNCTION__, serverTime);
				return qfalse;
			}

			deltaNum = MSG_ReadByte(msg);
#if 0
			if (deltaNum < 0) {
				Com_Printf("%s: invalid deltaNum %d\n", __FUNCTION__, deltaNum);
				return qfalse;
			}
#endif
			Com_Printf("deltaNum %d\n", deltaNum);

			MSG_ReadByte(msg);  // newSnap.snapFlags

			// read areamask
			len = MSG_ReadByte(msg);
			if (len > sizeof(newSnap.areamask)) {
				Com_Printf("%s: invalid areamask size %d\n", __FUNCTION__, len);
				return qfalse;
			}

			MSG_ReadData(msg, &newSnap.areamask, len);

			// read playerinfo
			MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);

			// read packet entities
			//FIXME
			//CL_ParsePacketEntities(msg, old, &newSnap);
			return qtrue;
			break;
		}
		case svc_download:
			//FIXME CL_ParseDownload(msg);
			Com_Printf("%s: download\n", __FUNCTION__);
			return qtrue;
			break;
		case svc_extension:  // libspeex voip protocol 70 or 71
			//FIXME check
			if (dataFollowsEOF) {
				// shouldn't happen
				//Com_Printf("^1%s unknown svc_extension\n", __FUNCTION__);
			} else {
				// check protocol to see if it contains flags
				//CL_ParseVoipSpeex(msg, qtrue, false);
			}
			Com_Printf("%s: extension\n", __FUNCTION__);
			return qtrue;
			break;
		case svc_voip:
			if (dataFollowsEOF) {
				// old speex without flags
				//CL_ParseVoipSpeex(msg, qfalse, qfalse);
			} else {
				//CL_ParseVoip(msg, qfalse);
			}
			Com_Printf("%s: voip\n", __FUNCTION__);
			return qtrue;
			break;
		}
	}

	return qtrue;
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage (qboolean seeking)
{
	int			r;
	msg_t		buf;
	byte		bufData[ MAX_MSGLEN ];
	int			s;
	int i;
	rewindBackups_t *rb;
	//double currentTime;
	int prevServerMessageSequence;
	qboolean badMessageFound = qfalse;

	if (cl_freezeDemo->integer) {
		//return;
	}

	if ( !clc.demoReadFile ) {
		CL_DemoCompleted ();
		return;
	}

	if (di.testParse) {
		di.snapsInDemo++;
	} else {
		di.numSnaps++;
	}

	//Com_Printf("snaps in demo: %d\n", di.snapsInDemo);


	//FIXME hack
	if (di.streaming) {
		di.snapsInDemo = 22965;
	}

	if ( !di.testParse  &&  di.snapCount < maxRewindBackups  &&
		 ((!di.gotFirstSnap  &&  !(clc.state >= CA_CONNECTED && clc.state < CA_PRIMED))
      ||
		  (di.gotFirstSnap  &&  di.numSnaps % (di.snapsInDemo / maxRewindBackups) == 0))) {
		if (!di.skipSnap) {
			// first snap triggers loading screen when rewinding
			di.skipSnap = qtrue;
			goto keep_reading;
		}
		di.gotFirstSnap = qtrue;
		rb = &rewindBackups[di.snapCount];
		if (!rb->valid) {
			//Com_Printf("setting rewind backup %d  numSnaps %d\n", di.snapCount, di.numSnaps);
			//Com_Printf("active %d  conn %d  static %d\n", sizeof(cl), sizeof(clc), sizeof(cls));
			rb->valid = qtrue;
			rb->numSnaps = di.numSnaps;
			rb->seekPoint = FS_FTell(clc.demoReadFile);
			for (i = 1;  i < di.numDemoFiles;  i++) {
				demoFile_t *df;

				df = &di.demoFiles[i];
				if (df->valid) {
					rb->demoSeekPoints[i] = FS_FTell(df->f);
				} else {
					rb->demoSeekPoints[i] = -1;
				}
			}
			memcpy(&rb->cl, &cl, sizeof(clientActive_t));
			memcpy(&rb->clc, &clc, sizeof(clientConnection_t));
			memcpy(&rb->cls, &cls, sizeof(clientStatic_t));
		}
		di.snapCount++;
	}

keep_reading:

	if (di.streaming) {
		int currentPos;
		int endPos;
		int retVal;
		int seqNum;
		int messageLength;
		qboolean debug = qfalse;

		if (Cvar_VariableIntegerValue("debug_demo_stream")) {
			debug = qtrue;
		}

		//FIXME 2024-07-28  this is a mess, should have commented why it was needed, the stream can have data written after this returns and what this would have detected as not enough data for message might have it afterwards, that could mess up the snap/message count, for something like that maybe only read the demo/stream end position once

		// changeset:   9050:074cb4fb2345
	    // user:        acano
		// date:        Thu Oct 21 06:22:18 2021 -0400
		// summary:     demo streaming look ahead to allow fast forwarding

		stream_demo_look_ahead();

		if (!di.waitingForStream) {
			di.streamWaitTime = cls.realtime;
		}

		di.waitingForStream = qtrue;

		currentPos = FS_FTell(clc.demoReadFile);
		retVal = FS_Seek(clc.demoReadFile, 0L, FS_SEEK_END);
		if (retVal != 0) {
			Com_Printf("^1demo streaming couldn't seek to end of demo, current position %d\n", currentPos);
			CL_DemoCompleted();
			return;
		}
		endPos = FS_FTell(clc.demoReadFile);
		if ((endPos - currentPos) < 8) {
			if (debug) {
				Com_Printf("^2demo streaming waiting for sequence number and message length, current position %d\n", currentPos);
			}
			retVal = FS_Seek(clc.demoReadFile, currentPos, FS_SEEK_SET);
			if (retVal != 0) {
				Com_Printf("^1demo streaming couldn't set current position after checking for sequence number and message length sizes, current position %d\n", currentPos);
				CL_DemoCompleted();
				return;
			}

			// keep waiting
			di.numSnaps--;
			return;
		}

		retVal = FS_Seek(clc.demoReadFile, currentPos, FS_SEEK_SET);
		if (retVal != 0) {
			Com_Printf("^1demo streaming couldn't set current position preparing to read sequence number, current position %d\n", currentPos);
			CL_DemoCompleted();
			return;
		}

		// get the sequence number
		r = FS_Read( &seqNum, 4, clc.demoReadFile);
		if ( r != 4 ) {
			Com_Printf("^1demo streaming couldn't get sequence number, current position %d\n", currentPos);
			CL_DemoCompleted ();
			return;
		}

		// get the length
		r = FS_Read (&messageLength, 4, clc.demoReadFile);
		if ( r != 4 ) {
			Com_Printf("^1demo streaming couldn't get message length, current position %d\n", currentPos);
			CL_DemoCompleted ();
			return;
		}

		messageLength = LittleLong(messageLength);

		if (messageLength == -1) {
			// /stoprecord writes last sequence number and length as -1
			if (debug) {
				Com_Printf("^2demo streaming end of demo, current position %d\n", currentPos);
			}

			// just in case it's needed for recording demo from demo
			clc.serverMessageSequence = LittleLong(seqNum);

			CL_DemoCompleted ();
			return;
		}

		if ((endPos - currentPos - 8) < messageLength) {
			if (debug) {
				//Com_Printf("cl.serverTime %d\n", cl.serverTime);
				Com_Printf("^2demo streaming waiting for message, need %d, current position %d\n", messageLength - (endPos - currentPos - 8), currentPos);
			}
			retVal = FS_Seek(clc.demoReadFile, currentPos, FS_SEEK_SET);
			if (retVal != 0) {
				Com_Printf("^1demo streaming couldn't set current position after checking for enough message data, current position %d\n", currentPos);
				CL_DemoCompleted();
				return;
			}

			// keep waiting
			di.numSnaps--;
			return;
		}

		// there is enough demo file data to read message, reset file position and keep reading normally
		retVal = FS_Seek(clc.demoReadFile, currentPos, FS_SEEK_SET);
		if (retVal != 0) {
			Com_Printf("^1demo streaming couldn't set current position after enough message data validated, current position %d\n", currentPos);
			CL_DemoCompleted();
			return;
		}

		di.waitingForStream = qfalse;
		if (di.streamWaitTime) {
			cls.realtime = di.streamWaitTime;
			di.streamWaitTime = 0;
		}
	}

	// get the sequence number
	r = FS_Read( &s, 4, clc.demoReadFile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}

	prevServerMessageSequence = clc.serverMessageSequence;
	clc.serverMessageSequence = LittleLong( s );

	if (clc.serverMessageSequence != (prevServerMessageSequence + 1)  &&  prevServerMessageSequence != 0  &&  clc.serverMessageSequence != -1) {
		//Com_Printf("^3server message sequence: %d -> %d\n", prevServerMessageSequence, clc.serverMessageSequence);
	}

	if (!di.checkedForOlderUncompressedDemo) {
		int demoPos;

		demoPos = FS_FTell(clc.demoReadFile);
		di.checkedForOlderUncompressedDemo = qtrue;
		//di.olderUncompressedDemo = qtrue;
		//di.olderUncompressedDemoProtocol = 48;
		di.olderUncompressedDemo = check_for_older_uncompressed_demo();
		FS_Seek(clc.demoReadFile, demoPos, FS_SEEK_SET);
		//Com_Printf("^2 fffffffffffff hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh  %d\n", di.olderUncompressedDemoProtocol);
		//FIXME hack for com_protocol check in msg.c
		if (di.olderUncompressedDemo) {
			Cvar_Set("protocol", va("%d", di.olderUncompressedDemoProtocol));
		}
	}

	// init the message
	if (di.olderUncompressedDemo) {
		MSG_InitOOB( &buf, bufData, sizeof( bufData ) );
	} else {
		MSG_Init( &buf, bufData, sizeof( bufData ) );
	}

	// get the length
	r = FS_Read (&buf.cursize, 4, clc.demoReadFile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}
	buf.cursize = LittleLong( buf.cursize );
	if ( buf.cursize == -1 ) {
		// /stoprecord writes last sequence number and length as -1
		CL_DemoCompleted ();
		return;
	}

#if 0
	if (1) {  //(buf.cursize > 16000) {
		static int biggest = 0;

		if (buf.cursize > biggest) {
			biggest = buf.cursize;
		}

		Com_Printf("^3buf size: %d  ^2%d\n", buf.cursize, biggest);
	}
#endif

	// hack for broken demos
	//FIXME also check if clc.serverMessageSequence is valid
	if (!di.streaming  &&  com_brokenDemo->integer  &&  (buf.cursize > MAX_MSGLEN  ||  buf.cursize < 0)) {  //  ||  clc.serverMessageSequence < 0) {
		Com_Printf("  ^6broken demo pos: %d  seq: %d -> %d  bad size: %d\n", FS_FTell(clc.demoReadFile) - 8, prevServerMessageSequence, clc.serverMessageSequence, buf.cursize);
		//buf.cursize = 16;
		// -7
		FS_Seek(clc.demoReadFile, -7, FS_SEEK_CUR);
		badMessageFound = qtrue;
		goto keep_reading;
	}

	if (badMessageFound) {
		int currentPosition;

		currentPosition = FS_FTell(clc.demoReadFile);
		Com_Printf("^3found possible valid message %d -> %d length %d at %d\n", prevServerMessageSequence, clc.serverMessageSequence, buf.cursize, FS_FTell(clc.demoReadFile) - 8);

		//FIXME double check message, buf.cursize already checked if < 0
		r = FS_Read( buf.data, buf.cursize, clc.demoReadFile );
		if ( r != buf.cursize ) {
			Com_Printf( "Demo file was truncated.\n");
			CL_DemoCompleted ();
			return;
		}

		if (!CL_CheckForValidServerMessage(&buf)) {
			// bad, keep trying
			Com_Printf("  ^1not valid server message\n");
			FS_Seek(clc.demoReadFile, currentPosition - 7, FS_SEEK_SET);
			goto keep_reading;
		} else {
			// all good, reset and continue
			Com_Printf("   maybe valid server message\n");
			FS_Seek(clc.demoReadFile, currentPosition, FS_SEEK_SET);
		}
	}

	if ( buf.cursize > buf.maxsize ) {
		if (di.testParse) {
			Com_Printf("^1demo_error: 'CL_ReadDemoMessage: demoMsglen (%d) > MAX_MSGLEN (%d)' at %d / %ld\n", buf.cursize, buf.maxsize, FS_FTell(clc.demoReadFile), FS_filelength(clc.demoReadFile));
			CL_DemoCompleted();
			return;
		} else {
			Com_Error (ERR_DROP, "CL_ReadDemoMessage: demoMsglen (%d) > MAX_MSGLEN (%d)", buf.cursize, buf.maxsize);
		}
	}

	if (buf.cursize < 0) {
		if (di.testParse) {
			Com_Printf("^1demo_error: 'CL_ReadDemoMessage: negative message size: %d\n", buf.cursize);
			CL_DemoCompleted();
			return;
		} else {
			Com_Error (ERR_DROP, "CL_ReadDemoMessage: negative message size: %d", buf.cursize);
		}
	}

	r = FS_Read( buf.data, buf.cursize, clc.demoReadFile );
	if ( r != buf.cursize ) {
		Com_Printf( "Demo file was truncated.\n");
		CL_DemoCompleted ();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;

#if 0
	// testing CL_CheckForValidServerMessage
	if (CL_CheckForValidServerMessage(&buf)) {
		Com_Printf("gooood\n");
	}
	buf.readcount = 0;
	buf.bit = 0;
#endif

	CL_ParseServerMessage( &buf );

	if (!di.testParse  &&   clc.demorecording  &&  clc.demoplaying  &&  !seeking) {
		if (cl.snap.deltaNum < di.firstNonDeltaMessageNumWritten  ||  di.firstNonDeltaMessageNumWritten == -1) {
			CL_WriteNonDeltaDemoMessage(&buf);
		} else {
			// write demo of demo message
			CL_WriteDemoMessage(&buf, 0);
		}

		//FIXME duplicate code
#ifdef USE_VOIP
		//FIXME if demo already contains voip packets sound playback is garbled
		if (clc.voipOutgoingDataSize > 0)
		{
			if((clc.voipFlags & VOIP_SPATIAL) || Com_IsVoipTarget(clc.voipTargets, sizeof(clc.voipTargets), -1))
			{
				// If we're recording a demo, we have to fake a server packet with
				//  this VoIP data so it gets to disk; the server doesn't send it
				//  back to us, and we might as well eliminate concerns about dropped
				//  and misordered packets here.

				//FIXME wait for non delta writing?
				if(clc.demorecording) // && !clc.demowaiting)
				{
					const int voipSize = clc.voipOutgoingDataSize;
					msg_t fakemsg;
					byte fakedata[MAX_MSGLEN];

					if (di.olderUncompressedDemo) {
						MSG_InitOOB (&fakemsg, fakedata, sizeof(fakedata));
					} else {
						MSG_Init (&fakemsg, fakedata, sizeof (fakedata));
						MSG_Bitstream (&fakemsg);
					}


					if (di.olderUncompressedDemo  &&  di.olderUncompressedDemoProtocol < 48) {
						// pass
					} else {
						MSG_WriteLong (&fakemsg, clc.reliableAcknowledge);
					}
					MSG_WriteByte (&fakemsg, svc_voip);
					//MSG_WriteShort (&fakemsg, clc.clientNum);
					//FIXME hack in case demo already has voip
					MSG_WriteShort(&fakemsg, MAX_CLIENTS - 1);
					MSG_WriteByte (&fakemsg, clc.voipOutgoingGeneration);
					MSG_WriteLong (&fakemsg, clc.voipOutgoingSequence);
					MSG_WriteByte (&fakemsg, clc.voipOutgoingDataFrames);
					MSG_WriteShort (&fakemsg, clc.voipOutgoingDataSize );
					MSG_WriteBits (&fakemsg, clc.voipFlags, VOIP_FLAGCNT);
					MSG_WriteData (&fakemsg, clc.voipOutgoingData, voipSize);
					MSG_WriteByte (&fakemsg, svc_EOF);
					CL_WriteDemoMessage (&fakemsg, 0);
				}

				clc.voipOutgoingSequence += clc.voipOutgoingDataFrames;
				clc.voipOutgoingDataSize = 0;
				clc.voipOutgoingDataFrames = 0;
			}
			else
			{
				// We have data, but no targets. Silently discard all data
				clc.voipOutgoingDataSize = 0;
				clc.voipOutgoingDataFrames = 0;
			}
		}
#endif
	}

	if (!di.testParse  &&  clc.demoplaying  &&  !di.streaming) {
		for (i = 1;  i < di.numDemoFiles;  i++) {
			demoFile_t *df;
			int pos;

			df = &di.demoFiles[i];
			if (!df->valid) {
				continue;
			}

			while (1) {
				int oldServerTime;

				oldServerTime = df->serverTime;
				pos = FS_FTell(df->f);
				//Com_Printf("pack %d\n", cl.parseEntitiesNum);
				CL_ReadExtraDemoMessage(df);
				//Com_Printf("pack %d\n", cl.parseEntitiesNum);

				//Com_Printf("%d  (%d)demoFile %d  serverTime %d\n", cl.snap.serverTime, i, df->f, df->serverTime);

				if (df->serverTime < cl.snap.serverTime) {
					continue;
				}

				if (df->serverTime > cl.snap.serverTime) {
					FS_Seek(df->f, pos, FS_SEEK_SET);
					df->serverTime = oldServerTime;
					break;
				}

				// got it
				//Com_Printf("%d  demoFile %d  serverTime %d\n", cl.snap.serverTime, df->f, df->serverTime);
				break;
			}
		}
	}
}

/*
====================
CL_CompleteDemoName
====================
*/
static void CL_CompleteDemoName( char *args, int argNum )
{
	char origArgs[MAX_STRING_CHARS];
	qboolean foundMatch;

	if( argNum == 2 )
	{
		//Field_Clear(&g_consoleField);

		Q_strncpyz(origArgs, args, sizeof(origArgs));

		//Com_Printf("^1wolfcam\n");

		// don't use extension since multiple protocols are supported
		Field_CompleteFilename( "demos", "", qfalse, qfalse, &foundMatch );

		//Com_Printf("^1ql\n");

		// don't use extension since multiple protocols are supported
		Field_CompleteFilename("ql:demos", "", qfalse, qtrue, &foundMatch);

	}
}

#if 0
static void testParseCommandString (msg_t *msg)
{
	//char *s;
	//	int seq;

	return;

#if 0
	seq = MSG_ReadLong(msg);
	s = MSG_ReadString(msg);

	Com_Printf("command string: %s\n", s);
#endif
}
#endif

static void check_events (const entityState_t *es, int serverTime)
{
	int event;
	int clientNum;
	int i;
	demoObit_t *d;
	itemPickup_t *ip;
	const gitem_t *item;
	int index;

	event = es->event & ~EV_EVENT_BITS;
	//Com_Printf("%d  ent %d  event %d  (with bits %d)  looking for obit %d  event bits %d\n", cl.snap.serverTime, es->number, event, es->event, EV_OBITUARY, EV_EVENT_BITS);
	if (!event) {
		//Com_Printf("zero event\n");
		return;
	}


	//Com_Printf("serverTime %d snap %d ent %d  event %d\n", cl.snap.serverTime, cl.snap.messageNum, es->number, event);
	clientNum = es->clientNum;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		Com_Printf("bad clientnum %d\n", clientNum);
		clientNum = 0;
	}

	//Com_Printf("di.protocol %d  com_protocol->integer %d  event %d\n", di.protocol, com_protocol->integer, event);

	if (((di.protocol == PROTOCOL_QL  ||  di.protocol == 73  ||  di.protocol == 90)  &&  event == EV_OBITUARY)  ||  ((di.protocol <= 71  &&  di.protocol >= 48)  &&  event == EVQ3_OBITUARY)  ||  (di.protocol < 48  &&  event == EVQ3DM3_OBITUARY)) {
		int target, attacker, mod;

		if (di.obitNum >= MAX_DEMO_OBITS) {
			return;
		}

		//CG_Obituary(es);
		target = es->otherEntityNum;
		attacker = es->otherEntityNum2;
		mod = es->eventParm;

		//Com_Printf("%d snap %d ent %d  %d  killed  %d  with %d\n", cl.snap.serverTime, cl.snap.messageNum, es->number, attacker, target, mod);

		for (i = di.obitNum - 1;  i >= 0;  i--) {
			//demoObit_t *d;

			d = &di.obit[i];
			//FIXME dropped packets?

			if (d->number == es->number) {
				if (d->lastMessageNum == cl.snap.messageNum) {
					// a duplicate in same snap
					return;
				}
				if (d->lastMessageNum == cl.snap.messageNum - 1) {
					// already added, update
					d->lastMessageNum = cl.snap.messageNum;
					d->lastServerTime = cl.snap.serverTime;
					return;
				}
				if (d->firstServerTime + EVENT_VALID_MSEC > cl.snap.serverTime) {
					d->lastMessageNum = cl.snap.messageNum;
					d->lastServerTime = cl.snap.serverTime;
					return;
				}
				//Com_Printf("di.obitNum %d\n", di.obitNum);
				// found a new obit
				d = &di.obit[di.obitNum];
				d->firstServerTime = cl.snap.serverTime;
				d->lastServerTime = cl.snap.serverTime;
				d->firstMessageNum = cl.snap.messageNum;
				d->lastMessageNum = cl.snap.messageNum;
				d->number = es->number;
				d->killer = attacker;
				d->victim = target;
				d->mod = mod;
				di.obitNum++;
				//Com_Printf("%d snap %d ent %d  %d  killed  %d  with %d\n", cl.snap.serverTime, cl.snap.messageNum, es->number, attacker, target, mod);
				//if (target >= 0  &&  target < MAX_CLIENTS) {
				//	di.clientAlive[target] = qfalse;
				//}
				return;
			}

			if (d->lastMessageNum < cl.snap.messageNum - 2) {
				// found a new obit
				d = &di.obit[di.obitNum];
				d->firstServerTime = cl.snap.serverTime;
				d->lastServerTime = cl.snap.serverTime;
				d->firstMessageNum = cl.snap.messageNum;
				d->lastMessageNum = cl.snap.messageNum;
				d->number = es->number;
				d->killer = attacker;
				d->victim = target;
				d->mod = mod;
				di.obitNum++;
				//Com_Printf("%d snap %d ent %d  %d  killed  %d  with %d\n", cl.snap.serverTime, cl.snap.messageNum, es->number, attacker, target, mod);
				//if (target >= 0  &&  target < MAX_CLIENTS) {
				//	di.clientAlive[target] = qfalse;
				//}
				return;
			}
		}
		// found a new obit
		d = &di.obit[di.obitNum];
		d->firstServerTime = cl.snap.serverTime;
		d->lastServerTime = cl.snap.serverTime;
		d->firstMessageNum = cl.snap.messageNum;
		d->lastMessageNum = cl.snap.messageNum;
		d->number = es->number;
		d->killer = attacker;
		d->victim = target;
		d->mod = mod;
		di.obitNum++;
		//Com_Printf("%d snap %d ent %d  %d  killed  %d  with %d\n", cl.snap.serverTime, cl.snap.messageNum, es->number, attacker, target, mod);
		//if (target >= 0  &&  target < MAX_CLIENTS) {
		//	di.clientAlive[target] = qfalse;
		//}
		return;
	} else if (((com_protocol->integer == PROTOCOL_QL  ||  com_protocol->integer == 73  ||  com_protocol->integer == 90)  &&  (event == EV_ITEM_PICKUP_SPEC  ||  event == EV_ITEM_PICKUP))  ||  ((com_protocol->integer <= 71  &&  com_protocol->integer >= 48)  &&  event == EVQ3_ITEM_PICKUP)  ||  (com_protocol->integer < 48  &&  event == EVQ3DM3_ITEM_PICKUP)) {
		//Com_Printf("%d  pickup item %d\n", cl.snap.serverTime, es->eventParm);
		if (di.numItemPickups >= MAX_ITEM_PICKUPS) {
			Com_Printf("^3max pickups\n");
			return;
		}

		//Com_Printf("^3item pickup\n");
		if (com_protocol->integer == PROTOCOL_QL  ||  com_protocol->integer == 73  ||  com_protocol->integer == 90) {
			if (event == EV_ITEM_PICKUP_SPEC) {
				index = es->modelindex;
			} else {
				index = es->eventParm;
			}

			if (index < 1  ||  index >= bg_numItems) {
				Com_Printf("^3invalid item: %d\n", index);
				return;
			}

			item = &bg_itemlist[index];
		} else {
			index = es->eventParm;

			// bg_itemListCpma should be ok if you only check armor and megahealth pickups (ex: quake3 1.25p has different bg_itemlist but it's the same up to those items)
			if (1) {  //(di.cpma) {  //FIXME
				item = &bg_itemlistCpma[index];
			} else {
				item = &bg_itemlistQ3[index];
			}
		}

		if (item->giType == IT_ARMOR  ||  item->giType == IT_HEALTH) {
			if (!Q_stricmp(item->pickup_name, "Green Armor")  ||  !Q_stricmp(item->pickup_name, "Yellow Armor")  ||  !Q_stricmp(item->pickup_name, "Red Armor")  ||  !Q_stricmp(item->pickup_name, "Mega Health")) {
				//Com_Printf("^3picked up: '%s'\n", item->pickup_name);

				if (di.numItemPickups < 0) {
					Com_Printf("^1wtf..... %d\n", di.numItemPickups);
				}
				ip = &di.itemPickups[di.numItemPickups];
				ip->clientNum = clientNum;
				//Com_Printf("%d    %d\n", di.cpma, es->number);
				if (di.cpma  &&  es->number >= MAX_CLIENTS) {
					ip->clientNum = es->modelindex2 & 0xf;
					//Com_Printf("it %d  -->  %d\n", es->number, es->modelindex2 & 0xf);
				}
				ip->index = index;
				ip->pickupTime = serverTime;
				ip->spec = qfalse;
				if ((com_protocol->integer == PROTOCOL_QL  ||  com_protocol->integer == 73  ||  com_protocol->integer == 90)  &&  event == EV_ITEM_PICKUP_SPEC) {
					ip->spec = qtrue;
					ip->specPickupTime = es->time;  // if es->time == 0 means item has respawned
				}
				ip->number = es->number;
				VectorCopy(es->pos.trBase, ip->origin);
				di.numItemPickups++;

				//Com_Printf("^3%d %d %d  %s %s\n", di.numItemPickups, serverTime, index, item->pickup_name, event == EV_ITEM_PICKUP_SPEC ? "(spec)" : "");
			}
		}
	} else if ((com_protocol->integer == PROTOCOL_QL  ||  com_protocol->integer == 73  ||  com_protocol->integer == 90)  &&  event == EV_GLOBAL_ITEM_PICKUP) {
		if (di.numItemPickups >= MAX_ITEM_PICKUPS) {
			Com_Printf("^3powerup max pickups\n");
			return;
		}

		//Com_Printf("^3global item pickup\n");
		index = es->eventParm;
		if (index < 1  ||  index >= bg_numItems) {
			Com_Printf("^3global pickup invalid item: %d\n", index);
			return;
		}
		item = &bg_itemlist[index];

		if (item->giType == IT_POWERUP) {
			//Com_Printf("^2%d  powerup pickup %d\n", cl.snap.serverTime, item->giTag);
			ip = &di.itemPickups[di.numItemPickups];
			ip->clientNum = clientNum;
			ip->index = index;
			ip->pickupTime = serverTime;
			ip->spec = qfalse;
			ip->number = es->number;
			VectorCopy(es->pos.trBase, ip->origin);
			di.numItemPickups++;
			//Com_Printf("^3%d %d %d   %s\n", di.numItemPickups, serverTime, index, item->pickup_name);
		}
	}
}

static void parse_new_snapshot (const clSnapshot_t *snap, const clSnapshot_t *oldSnap)
{
	entityState_t es;
	int i;
	int count;
	int et_events;

	if (!oldSnap) {
		Com_Printf("%s() !oldSnap\n", __FUNCTION__);
		return;
	}

	if (!snap) {
		Com_Printf("%s() !snap\n", __FUNCTION__);
		return;
	}


	if (snap->ps.clientNum != oldSnap->ps.clientNum) {
		// treat new ps as old
	}

	memset(&es, 0, sizeof(es));
	VectorCopy(snap->ps.origin, es.pos.trBase);
	SnapVector(es.pos.trBase);
	es.number = snap->ps.clientNum;
	es.clientNum = snap->ps.clientNum;
	//es.number = cl.snap.ps.clientNum;
	if (snap->ps.externalEvent  &&  snap->ps.externalEvent != oldSnap->ps.externalEvent) {
		es.event = snap->ps.externalEvent;
		es.eventParm = snap->ps.externalEventParm;
		//Com_Printf("%d ps external event %d %d\n", snap->serverTime, es.event  & ~EV_EVENT_BITS, es.eventParm);
		check_events(&es, cl.snap.serverTime);
	}

	if (snap->ps.clientNum == oldSnap->ps.clientNum) {
		for (i = snap->ps.eventSequence - MAX_PS_EVENTS;  i < snap->ps.eventSequence;  i++) {
			if (i >= oldSnap->ps.eventSequence
				// or the server told us to play another event instead of a predicted event we already issued
				// or something the server told us changed our prediction causing a different event
				|| (i > oldSnap->ps.eventSequence - MAX_PS_EVENTS && snap->ps.events[i & (MAX_PS_EVENTS-1)] != oldSnap->ps.events[i & (MAX_PS_EVENTS-1)]) ) {
				es.event = snap->ps.events[i & (MAX_PS_EVENTS - 1)];
				es.eventParm = snap->ps.eventParms[i & (MAX_PS_EVENTS - 1)];
				//Com_Printf("event seq  %d\n", es.event);
				check_events(&es, cl.snap.serverTime);
			}
		}
	}

	memset(di.entityInOldSnap, 0, sizeof(di.entityInOldSnap));
	//memset(di.entityInSnap, 0, sizeof(di.entityInSnap));

	count = oldSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT) {
		Com_Printf("%s old snap truncated %i entities to %i\n", __FUNCTION__, count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}


	for (i = 0;  i < count;  i++) {
		entityState_t *ent;
		int num;

		ent = &cl.parseEntities[(oldSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
		num = ent->number;
		di.entityInOldSnap[num] = qtrue;
		di.oldEs[num] = ent;
	}

	count = snap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT) {
		Com_Printf("%s snap truncated %i entities to %i\n", __FUNCTION__, count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}

	for (i = 0;  i < count;  i++) {
		entityState_t *oldEs;
		int num;

		es = cl.parseEntities[(snap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
		num = es.number;
		if (!di.entityInOldSnap[num]) {
			// reset
			if (di.entitySnapShotTime[num] < snap->serverTime - EVENT_VALID_MSEC) {
				di.entityPreviousEvent[num] = 0;
			}
		} else {
			oldEs = di.oldEs[num];
			// reset if teleported
			if ((oldEs->eFlags ^ es.eFlags) & EF_TELEPORT_BIT) {
				if (di.entitySnapShotTime[num] < snap->serverTime - EVENT_VALID_MSEC) {
					di.entityPreviousEvent[num] = 0;
				}
			}
		}

		if (di.protocol < 48) {
			et_events = 12;
		} else {
			et_events = ET_EVENTS;  // 13
		}

		// CG_CheckEvents
		//FIXME 2020-06-15 encrypted cpma mvd
		if (es.eType > et_events) {
			if (di.entityPreviousEvent[num]) {
				goto skip;  // already fired
			}
			// if this is a player event set the entity number of the client entity number
			if (es.eFlags & EF_PLAYER_EVENT) {
				es.number = es.otherEntityNum;
			}
			di.entityPreviousEvent[num] = 1;
			es.event = es.eType - et_events;
		} else {
			// check for events riding with another entity
			if (es.event == di.entityPreviousEvent[num]) {
				goto skip;
			}
			di.entityPreviousEvent[num] = es.event;
			if ((es.event & ~EV_EVENT_BITS) == 0) {
				goto skip;
			}
		}

		//FIXME note that this fire a serverframe earlier than in cgame
		check_events(&es, cl.snap.serverTime);

	skip:
		//
		di.entitySnapShotTime[num] = snap->serverTime;
	}
}

static qboolean check_for_older_uncompressed_demo (void)
{
	int r;
	msg_t buf;
	byte bufData[MAX_MSGLEN];
	int s;
	int i;
	qboolean hasProtocol = qfalse;
	qboolean hasGamename = qfalse;
	qboolean hasVersion = qfalse;
	qboolean hasMapname = qfalse;

	//FIXME demo streaming

	FS_Seek(clc.demoReadFile, 0, FS_SEEK_SET);

	// get the sequence number
	r = FS_Read(&s, 4, clc.demoReadFile);
	if (r != 4) {
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}

	MSG_InitOOB(&buf, bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demoReadFile);
	if (r != 4) {
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1) {
		// /stoprecord writes last sequence number and length as -1
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}

	if (buf.cursize > buf.maxsize) {
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}

	if (buf.cursize < 0) {
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}

	r = FS_Read( buf.data, buf.cursize, clc.demoReadFile );
	if ( r != buf.cursize ) {
		// maybe truncated demo file
		//Com_Printf( "Demo file was truncated.\n");
		FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		return qfalse;
	}

	//clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	//CL_ParseServerMessage( &buf );

	// parse server message

	// get the reliable sequence acknowledge number
	// protocol 43 doesn't have this
	// clc.reliableAcknowledge = MSG_ReadLong( msg );

	//cmd = MSG_ReadByte(&buf);

	//Com_Printf("    cmd %d\n", cmd);

	// just look through the data to find required settings:
	//     protocol, gamename, version, mapname

	for (i = 0;  i < buf.cursize;  i++) {
		int bytesLeft = buf.cursize - i;
		const char *str = (const char *) (buf.data + i);

		// protocol/46
		if (!hasProtocol  &&  bytesLeft >= 11) {
			if (!Q_strncmp(str, "protocol\\", 9)) {
				char b[3];

				hasProtocol = qtrue;

				b[0] = str[9];
				b[1] = str[10];
				b[2] = '\0';

				di.olderUncompressedDemoProtocol = atoi(b);
			}
		}

		// gamename/
		if (!hasGamename  &&  bytesLeft >= 10) {
			if (!Q_strncmp(str, "gamename\\", 9)) {
				hasGamename = qtrue;
			}
		}

		// version/
		if (!hasVersion  &&  bytesLeft >= 9) {
			if (!Q_strncmp(str, "version\\", 8)) {
				hasVersion = qtrue;
			}
		}

		// mapname/
		if (!hasMapname  &&  bytesLeft >= 9) {
			if (!Q_strncmp(str, "mapname\\", 8)) {
				hasMapname = qtrue;
			}
		}
	}

	FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);

	if (hasProtocol  &&  hasGamename  &&  hasVersion  &&  hasMapname) {
		Com_Printf("^5older demo protocol detected: %d\n", di.olderUncompressedDemoProtocol);
		return qtrue;
	}

	return qfalse;
}

extern qboolean Msg_TestParse;
extern qboolean Msg_Abort;

static void parse_demo (void)
{
	int msec;
	int fps;
	int demofile;
	int tstart;
	int serverTime;
	int lastMessageNum = -1;
	clSnapshot_t *oldSnap = NULL;
	int i;

	tstart = Sys_Milliseconds();
	//memset(&di, 0, sizeof(di));
	di.gameStartTime = -1;
	di.gameEndTime = -1;
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		di.clientTeam[i] = TEAM_NUM_TEAMS;
	}

	FS_Seek(clc.demoReadFile, 0, FS_SEEK_SET);
    clc.state = CA_CONNECTED;
    clc.demoplaying = qtrue;
	di.testParse = qtrue;
	Msg_TestParse = qtrue;

	di.demoPos = FS_FTell(clc.demoReadFile);

	// check for protocol 46 - 48
	//di.olderUncompressedDemo = check_for_older_uncompressed_demo();
	if (!di.checkedForOlderUncompressedDemo) {
		int demoPos;

		demoPos = FS_FTell(clc.demoReadFile);
		di.checkedForOlderUncompressedDemo = qtrue;
		//di.olderUncompressedDemo = qtrue;
		//di.olderUncompressedDemoProtocol = 48;
		di.olderUncompressedDemo = check_for_older_uncompressed_demo();
		FS_Seek(clc.demoReadFile, demoPos, FS_SEEK_SET);
		//FIXME hack for com_protocol check in msg.c
		if (di.olderUncompressedDemo) {
			Cvar_Set("protocol", va("%d", di.olderUncompressedDemoProtocol));
		}
	}

    // get gameState
    CL_ReadDemoMessage(qfalse);
	//CL_SkipDemoMessage();
#if 0
	if (Msg_Abort) {
		CL_Disconnect(qfalse);
		di.testParse = qfalse;
		Msg_TestParse = qfalse;
		Msg_Abort = qfalse;
		return;
	}
#endif

    if (!cl.gameState.dataCount) {
        Com_Printf("couldn't get gameState\n");
	}

	clc.state = CA_PRIMED;

	//CG_Init (clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	clc.firstDemoFrameSkipped = qfalse;

    fps = 125;
    msec = 0;
    while (1) {  // CL_Frame (int msec)
        //printf ("%d  %d\n", msec, cl.serverTime);
        msec = 1000 / fps;
        // CL_Frame (msec):
        cls.realFrametime = msec;
        cls.frametime = msec;
        cls.realtime += cls.frametime;
        ///////////////////////////////
        //printf ("clc.state:%d\n", clc.state);

		di.demoPos = FS_FTell(clc.demoReadFile);
        CL_SetCGameTime();
		if (Msg_Abort) {
			//CL_Disconnect(qfalse);
			break;
		}

		if (di.endOfDemo) {
			break;
		}

		serverTime = 0;
        // SCR_UpdateScreen() -> SCR_DrawScreenField()
        if (clc.state == CA_PRIMED  ||  clc.state == CA_ACTIVE) {

			int lastServerTime;
			clSnapshot_t *snap;
			//int count;
			//int i;

            //CG_DrawActiveFrame (cl.serverTime, STEREO_CENTER, clc.demoplaying);
			snap = &cl.snap;
			lastServerTime = serverTime;
			serverTime = snap->serverTime;
			if (serverTime  &&  serverTime == lastServerTime) {
				//goto newloop;
				//continue;
			}
			//Com_Printf("^3serverTime %d\n", serverTime);
			if (!snap->valid) {
				Com_Printf("invalid snap %d  serverTime %d\n", snap->messageNum, snap->serverTime);
				goto newloop;
			}

			if (di.lastServerTime  &&  di.lastServerTime == snap->serverTime) {
				//Com_Printf("offline demo detected\n");
				di.offlineDemo = qtrue;
			}
			if (di.firstServerTime  &&  serverTime - di.lastServerTime > 0  &&  (serverTime - di.lastServerTime < di.serverFrameTime  ||  !di.serverFrameTime)) {
				di.serverFrameTime = serverTime - di.lastServerTime;
				Com_Printf("parse demo serverFrameTime %d ms\n", di.serverFrameTime);
			}
			di.lastServerTime = serverTime;

			if (di.firstServerTime == 0) {
				di.firstServerTime = serverTime;
				Com_Printf("firstServerTime %d\n", serverTime);
			}

			if (snap->deltaNum <= 0) {
				//Com_Printf("uncompressed serverTime %d   demoPos %d   snap %d\n", serverTime, di.demoPos, di.snapsInDemo);
			}

			if (lastMessageNum != snap->messageNum) {
				oldSnap = &cl.snapshots[0][(cl.snap.messageNum - 1) & PACKET_MASK];
				//Com_Printf("new snap: %d  %p %p  %d\n", snap->messageNum, oldSnap, snap, oldSnap->messageNum);

				if ( cl.parseEntitiesNum - snap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
					Com_Printf("%s cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES", __FUNCTION__);
					//return qfalse;
				}

				parse_new_snapshot(snap, oldSnap);
#if 0
				if (Msg_Abort) {
					CL_Disconnect(qfalse);
				}
#endif
			} else {
				//Com_Printf("...\n");
			}

			//oldSnap = snap;
		}

		lastMessageNum = cl.snap.messageNum;

	newloop:
        cls.framecount++;
    }

	Msg_TestParse = qfalse;
	Msg_Abort = qfalse;

	//FIXME quake live hack for timeouts
	if (di.protocol == PROTOCOL_QL  ||  di.protocol == 73  ||  di.protocol == 90) {
		for (i = 0;  i < di.numTimeouts;  i++) {
			if (di.timeOuts[i].endTime) {
				di.timeOuts[i].endTime += di.serverFrameTime;
			}
		}

		if (di.numTimeouts) {
			if (di.timeOuts[di.numTimeouts - 1].endTime == 0) {
				// demo ends before ref or client timeout ends
				di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime;
			}
		}
	} else if (di.cpma) {  // hack for cpma demos
		if (di.numTimeouts) {
			if (di.timeOuts[di.numTimeouts - 1].endTime == 0) {
				// demo ends before timeout is over
				di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime;
			}
		}
	}

	Com_Printf("last serverTime %d   total %f minutes\n", cl.snap.serverTime, (cl.snap.serverTime - di.firstServerTime) / 1000.0 / 60.0);
	Com_Printf("%d snaps in demo\n", di.snapsInDemo);
	Com_Printf("%d obits in demo\n", di.obitNum);
	Com_Printf("%d item pickups in demo\n", di.numItemPickups);
	Com_Printf("%d timeout spans in demo\n", di.numTimeouts);
	Com_Printf("parse time %f seconds\n", (float)(Sys_Milliseconds() - tstart) / 1000.0);
	FS_Seek(clc.demoReadFile, 0, FS_SEEK_SET);
	clc.demoplaying = qfalse;
	di.testParse = qfalse;
	// CL_Disconnect(qtrue);
	demofile = clc.demoReadFile;
	CL_ClearState();
	Com_Memset(&clc, 0, sizeof(clc));
	clc.demoReadFile = demofile;
	clc.state = CA_DISCONNECTED;
	cl_connectedToPureServer = qfalse;
#ifdef USE_VOIP
	// not connected to voip server anymore.
	clc.voipEnabled = qfalse;
#endif
	//CL_UpdateGUID( NULL, 0 );
	Con_Close();
	//Sys_Exit(0);
	Cmd_RemoveCommand("voip");
}

static void stream_demo_look_ahead (void)
{
	int currentPos;
	int r;
	msg_t buf;
	byte bufData[MAX_MSGLEN];
	demoFile_t df;
	qboolean debug = qfalse;

	if (di.endOfDemo) {
		return;
	}

	if (Cvar_VariableIntegerValue("debug_demo_stream_look_ahead")) {
		debug = qtrue;
	}

	currentPos = FS_FTell(clc.demoReadFile);

	while (!di.endOfDemo) {
		//FIXME duplicate code
		int endPos;
		int seqNum;
		int messageLength;

		r = FS_Seek(clc.demoReadFile, 0L, FS_SEEK_END);
		if (r != 0) {
			Com_Printf("^1%s() couldn't seek to end of demo, current position %d\n", __FUNCTION__, currentPos);
			CL_DemoCompleted();
			return;
		}

		endPos = FS_FTell(clc.demoReadFile);
		if ((endPos - di.demoPos) < 8) {
			if (debug) {
				Com_Printf("^4%s() waiting for sequence number and message length, demo position %d\n", __FUNCTION__, di.demoPos);
			}
			// keep waiting
			break;
		}

		r = FS_Seek(clc.demoReadFile, di.demoPos, FS_SEEK_SET);
		if (r != 0) {
			Com_Printf("^1%s() couldn't set current position preparing to read sequence number, demo position %d\n", __FUNCTION__, di.demoPos);
			CL_DemoCompleted();
			return;
		}

		// get the sequence number
		r = FS_Read(&seqNum, 4, clc.demoReadFile);
		if (r != 4) {
			Com_Printf("^1%s() couldn't get sequence number, demo position %d\n", __FUNCTION__, di.demoPos);
			CL_DemoCompleted ();
			return;
		}

		// get the length
		r = FS_Read(&messageLength, 4, clc.demoReadFile);
		if (r != 4) {
			Com_Printf("^1%s() couldn't get message length, demo position %d\n", __FUNCTION__, di.demoPos);
			CL_DemoCompleted ();
			return;
		}

		messageLength = LittleLong(messageLength);

		if (messageLength == -1) {
			// /stoprecord writes last sequence number and length as -1
			if (debug) {
				Com_Printf("^4%s() end of demo, demo position %d\n", __FUNCTION__, di.demoPos);
			}
			di.endOfDemo = qtrue;
			break;
		}

		if ((endPos - di.demoPos - 8) < messageLength) {
			if (debug) {
				Com_Printf("^4%s() waiting for message, need %d, demo position %d\n", __FUNCTION__, messageLength - (endPos - di.demoPos - 8), di.demoPos);
			}
			// keep waiting
			break;
		}

		// there is enough demo file data to read message

		if (messageLength > sizeof(bufData)) {
			Com_Printf("^1%s() demo message length greater than max size, demo position %d\n", __FUNCTION__, di.demoPos);
			di.endOfDemo = qtrue;
			break;
		}

		if (messageLength < 0) {  // -1 handled above
			Com_Printf("^1%s() negative message size, demo position %d\n", __FUNCTION__, di.demoPos);
			di.endOfDemo = qtrue;
			break;
		}

		if (!di.checkedForOlderUncompressedDemo) {
			int demoPos;

			demoPos = FS_FTell(clc.demoReadFile);
			di.checkedForOlderUncompressedDemo = qtrue;
			//di.olderUncompressedDemo = qtrue;
			//di.olderUncompressedDemoProtocol = 48;
			di.olderUncompressedDemo = check_for_older_uncompressed_demo();
			FS_Seek(clc.demoReadFile, demoPos, FS_SEEK_SET);
			//FIXME hack for com_protocol check in msg.c
			if (di.olderUncompressedDemo) {
				Cvar_Set("protocol", va("%d", di.olderUncompressedDemoProtocol));
			}
			//Com_Printf("^2hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh  %d\n", di.olderUncompressedDemoProtocol);
		}

		if (di.olderUncompressedDemo) {
			MSG_InitOOB(&buf, bufData, sizeof(bufData));
		} else {
			MSG_Init(&buf, bufData, sizeof(bufData));
		}

		buf.cursize = messageLength;

		r = FS_Read(buf.data, buf.cursize, clc.demoReadFile);
		if (r != buf.cursize) {
			// truncated demo
			// should have been handled in waiting check before
			Com_Printf("^3%s() truncated demo shouldn't happen, demo position %d\n", __FUNCTION__, di.demoPos);
			di.endOfDemo = qtrue;
			break;
		}

		buf.readcount = 0;

		//FIXME obits, timeouts, item pickups, etc...
		//  for now just checking for future server times to allow fast forwarding

		memset(&df, 0, sizeof(df));
		// df.f must be zero since this will be accessed:
		//    &cl.snapshots[df->num][newSnap.deltaNum & PACKET_MASK];
		CL_ParseExtraServerMessage(&df, &buf, qtrue);

		if (df.serverTime > di.lastServerTime) {
			di.lastServerTime = df.serverTime;
		}

		if (di.firstServerTime <= 0) {
			di.firstServerTime = df.serverTime;
		}

		//FIXME this is really server messages in demo
		di.snapsInDemo++;
		di.demoPos = FS_FTell(clc.demoReadFile);
	}

	r = FS_Seek(clc.demoReadFile, currentPos, FS_SEEK_SET);
	if (r != 0) {
		Com_Printf("^1%s() couldn't reset position, current position %d\n", __FUNCTION__, currentPos);
		CL_DemoCompleted();
		return;
	}
}

// used with playback from multiple demo files at once
static void CL_ReadExtraDemoMessage (demoFile_t *df)
{
	int			r;
	msg_t		buf;
	byte		bufData[ MAX_MSGLEN ];
	int			s;

	if ( !df  ||  !df->valid ) {
		//CL_DemoCompleted ();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, df->f);
	if ( r != 4 ) {
		//CL_DemoCompleted ();
		Com_Printf("demoFile %d ended\n", df->f);
		//FIXME
		//Com_Error(ERR_DROP, "FIXME other demo file truncated");
		return;
	}
	df->serverMessageSequence = LittleLong( s );
	//Com_Printf("demoFile %d  sequence %d (%d)\n", df->f, df->serverMessageSequence, clc.serverMessageSequence);

	// init the message
	if (clc.realProtocol <= 48) {
		MSG_InitOOB(&buf, bufData, sizeof(bufData));
	} else {
		MSG_Init(&buf, bufData, sizeof(bufData));
	}

	// get the length
	r = FS_Read (&buf.cursize, 4, df->f);
	if ( r != 4 ) {
		//CL_DemoCompleted ();
		Com_Printf("demoFile %d truncated\n", df->f);
		//FIXME
		//Com_Error(ERR_DROP, "FIXME other demo file truncated");
		return;
	}
	buf.cursize = LittleLong( buf.cursize );

	if ( buf.cursize == -1 ) {
		// /stoprecord writes last sequence number and length as -1
		//CL_DemoCompleted ();
		Com_Printf("demoFile %d done\n", df->f);
		return;
	}

	if ( buf.cursize > buf.maxsize ) {
		//Com_Error (ERR_DROP, "CL_ReadDemoMessage: demoMsglen (%d) > MAX_MSGLEN (%d)", buf.cursize, buf.maxsize);
		Com_Printf ("^1CL_ReadExtraDemoMessage: demoMsglen (%d) > MAX_MSGLEN (%d) for demoFile %d\n", buf.cursize, buf.maxsize, df->f);
		return;
	}
	r = FS_Read( buf.data, buf.cursize, df->f );
	if ( r != buf.cursize ) {
		Com_Printf( "Demo file %d was truncated(2)\n", df->f);
		//CL_DemoCompleted ();
		return;
	}

	//clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseExtraServerMessage(df, &buf, qfalse);
}

static qhandle_t CL_OpenDemoFile (const char *arg, qboolean streaming)
{
	char name[MAX_OSPATH];
	char demoPathName[MAX_OSPATH];
	qhandle_t file = 0;
	int i;

	if (!arg  ||  !*arg) {
		return 0;
	}

	//Com_Printf("playdemo '%s'\n", arg);

	if (cl_demoFileCheckSystem->integer == 1) {
		FS_FOpenSysFileRead(arg, &file);
		if (file) {
			Q_strncpyz(name, arg, sizeof(name));
			goto done;
		}
	}

	if (!Q_stricmpn(arg, "ql:", strlen("ql:"))) {
		// try steam quake live directory first
		Com_sprintf(name, sizeof(name), "%s/baseq3/%s", Sys_QuakeLiveDir(), arg + 3);
		FS_FOpenSysFileRead(name, &file);

		if (!file) {  // try stand alone directory
			Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s", Sys_QuakeLiveDir(), arg + 3);
			FS_FOpenSysFileRead(name, &file);
		}

		if (!file) {
			Com_Error(ERR_DROP, "couldn't open '%s' in quake live directory", arg + 3);
			return 0;
		} else {
			goto done;
		}
	} else if (strstr(arg, "demos/") != arg) {  // name doesn't start with 'demos/'

		Com_sprintf(demoPathName, sizeof(demoPathName), "demos/%s", arg);
	} else {  // name starts with 'demos/'
		Q_strncpyz(demoPathName, arg, sizeof(demoPathName));
	}

	// demoPathName starts with 'demos/'

	//Com_Printf("opening %s\n", demoPathName);
	FS_FOpenFileRead(demoPathName, &file, qtrue);
	if (file) {
		Q_strncpyz(name, arg, sizeof(name));
		goto done;
	}

	// check if demo name didn't have an extension
	if (!file) {
		for (i = 0;  i < ARRAY_LEN(demo_protocols)  &&  !file;  i++) {
			if (demo_protocols[i] > 43  &&  demo_protocols[i] < 48) {
				// already checked .dm3 with protocol 43
				continue;
			}

			if (demo_protocols[i] == 43) {
				Com_sprintf(name, sizeof(name), "%s.dm3", demoPathName);
				FS_FOpenFileRead(name, &file, qtrue);
				if (!file) {
					Com_sprintf(name, sizeof(name), "%s.DM3", demoPathName);
					FS_FOpenFileRead(name, &file, qtrue);
					if (!file) {
						// :/
						Com_sprintf(name, sizeof(name), "%s.dM3", demoPathName);
						FS_FOpenFileRead(name, &file, qtrue);
						if (!file) {
							// :{   :/
							Com_sprintf(name, sizeof(name), "%s.Dm3", demoPathName);
							FS_FOpenFileRead(name, &file, qtrue);
						}
					}
				}

				if (file) {
					break;
				}
			}

			Com_sprintf(name, sizeof(name), "%s.dm_%d", demoPathName, demo_protocols[i]);
			FS_FOpenFileRead(name, &file, qtrue);
			if (!file) {
				Com_sprintf(name, sizeof(name), "%s.DM_%d", demoPathName, demo_protocols[i]);
				FS_FOpenFileRead(name, &file, qtrue);
				if (!file) {
					// :/
					Com_sprintf(name, sizeof(name), "%s.dM_%d", demoPathName, demo_protocols[i]);
					FS_FOpenFileRead(name, &file, qtrue);
					if (!file) {
						// :{   :/
						Com_sprintf(name, sizeof(name), "%s.Dm_%d", demoPathName, demo_protocols[i]);
						FS_FOpenFileRead(name, &file, qtrue);
					}
				}
			}
		}
	}

	// check steam quakelive dir
	if (!file) {
		Com_sprintf(name, sizeof(name), "%s/baseq3/%s", Sys_QuakeLiveDir(), demoPathName);
		FS_FOpenSysFileRead(name, &file);
		if (file) {
			goto done;
		}

		// check to see if name was given without protocol extension

		for (i = 0;  i < ARRAY_LEN(demo_protocols)  &&  !file;  i++) {
			Com_sprintf(name, sizeof(name), "%s/baseq3/%s.dm_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
			FS_FOpenSysFileRead(name, &file);
			if (!file) {
				Com_sprintf(name, sizeof(name), "%s/baseq3/%s.DM_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
				FS_FOpenSysFileRead(name, &file);
				if (!file) {
					// :/
					Com_sprintf(name, sizeof(name), "%s/baseq3/%s.dM_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
					FS_FOpenSysFileRead(name, &file);
					if (!file) {
						// :{   :/
						Com_sprintf(name, sizeof(name), "%s/baseq3/%s.Dm_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
						FS_FOpenSysFileRead(name, &file);
					}
				}
			}
		}  // end protocol list
	}

	// check stand alone quakelive dir
	if (!file) {
		Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s", Sys_QuakeLiveDir(), demoPathName);
		FS_FOpenSysFileRead(name, &file);
		if (file) {
			goto done;
		}

		// check to see if name was given without protocol extension

		for (i = 0;  i < ARRAY_LEN(demo_protocols)  &&  !file;  i++) {
			Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s.dm_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
			FS_FOpenSysFileRead(name, &file);
			if (!file) {
				Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s.DM_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
				FS_FOpenSysFileRead(name, &file);
				if (!file) {
					// :/
					Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s.dM_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
					FS_FOpenSysFileRead(name, &file);
					if (!file) {
						// :{   :/
						Com_sprintf(name, sizeof(name), "%s/home/baseq3/%s.Dm_%d", Sys_QuakeLiveDir(), demoPathName, demo_protocols[i]);
						FS_FOpenSysFileRead(name, &file);
					}
				}
			}
		}  // end protocol list
	}


	if (!file  &&  cl_demoFileCheckSystem->integer == 2) {
		FS_FOpenSysFileRead(arg, &file);
		Q_strncpyz(name, arg, sizeof(name));
	}

done:

	if (!file) {
		Com_Error(ERR_DROP, "couldn't open demo %s", arg);
		return 0;
	}

	// we have a valid demo file
	Cvar_Set("cl_demoFile", name);
	Cvar_Set("cl_demoFileBaseName", FS_BaseName(name));

	if (file  &&  cl_keepDemoFileInMemory->integer  &&  !streaming) {
		FS_FileLoadInMemory(file);
	}

	return file;
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/

//FIXME ...
static char DemoNames[MAX_DEMO_FILES][MAX_OSPATH];

void CL_PlayDemo_f (void)
{
	const char *arg;
	int i;
	int n;

#if 0
	if (clc.state == CA_CONNECTING) {
		// recursive call
		Com_Printf("^1CL_PlayDemo_f: invalid state\n");
		return;
	}
#endif

	n = Cmd_Argc();

	if (n < 2) {
		Com_Printf("demo <demoname1> <demoname2> ...\n");
		return;
	}

	// make sure a local server is killed
	// 2 means don't force disconnect of local client
	Cvar_Set( "sv_killserver", "2" );
	Cvar_Set("com_workshopids", "");

	for (i = 1;  i < n  &&  i <= MAX_DEMO_FILES;  i++) {
		arg = Cmd_Argv(i);
		Q_strncpyz(DemoNames[i - 1], arg, MAX_OSPATH);
	}

	arg = DemoNames[0];

	CL_Disconnect( qtrue );

	clc.demoReadFile = CL_OpenDemoFile(arg, qfalse);
	if (!clc.demoReadFile) {
		Com_Printf("^1CL_PlayDemo_f() couldn't open demo file '%s'\n", arg);
		return;
	}

	//FIXME
	memset(&di, 0, sizeof(di));
	//FIXME
	for (i = 0;  i < MAX_DEMO_FILES;  i++) {
		di.demoFiles[i].num = i;
	}
	di.demoFiles[0].f = clc.demoReadFile;
	di.demoFiles[0].valid = qtrue;
	di.numDemoFiles++;

	//n = Cmd_Argc();
	for (i = 2;  i < n  &&  i <= MAX_DEMO_FILES;  i++) {
		qhandle_t file;
		demoFile_t *df;

#if 0
		if (di.numDemoFiles > MAX_DEMO_FILES) {
			Com_Printf("^3CL_PlayDemo_f MAX_DEMO_FILES(%d)\n", MAX_DEMO_FILES);
			break;
		}
#endif

		//arg = Cmd_Argv(i);
		arg = DemoNames[i - 1];
		Com_Printf("^4'%s'\n", arg);
		//di.demoFiles[di.numDemoFiles].f = CL_OpenDemoFile(arg, qfalse);
		file = CL_OpenDemoFile(arg, qfalse);
		if (!file) {
			Com_Printf("^3CL_PlayDemo_f couldn't open '%s'\n", arg);
			continue;
		}
		Com_Printf("%d\n", file);
		df = &di.demoFiles[di.numDemoFiles];
		df->f = file;
		df->valid = qtrue;
		di.numDemoFiles++;
	}

	//Q_strncpyz( clc.demoName, Cmd_Argv(1), sizeof( clc.demoName ) );

	Con_Close();

	parse_demo();

	// CL_CheckWorkshopDownload() advances to CA_CONNECTED
	clc.state = CA_DOWNLOADINGWORKSHOPS;
	clc.demoplaying = qtrue;
	clc.demoPlayBegin = Sys_Milliseconds();

	// let CL_CheckWorkshopDownload() know it needs to initialize
	clc.demoWorkshopsString = NULL;
	Q_strncpyz( clc.servername, arg, sizeof( clc.servername ) );


#ifdef LEGACY_PROTOCOL
#if 0  //FIXME check
	if(protocol <= com_legacyprotocol->integer)
		clc.compat = qtrue;
	else
		clc.compat = qfalse;
#endif
#endif

	Com_Printf("^5checking workshops\n");
	// check immediately so it doesn't flash download screen if the file
	// is present or steamcmd not available
	CL_CheckWorkshopDownload();
}

void CL_StreamDemo_f (void)
{
	const char *arg;
	int i;
	int n;

#if 0
	if (clc.state == CA_CONNECTING) {
		// recursive call
		Com_Printf("^1CL_PlayDemo_f: invalid state\n");
		return;
	}
#endif

	n = Cmd_Argc();

	if (n < 2) {
		Com_Printf("streamdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	// 2 means don't force disconnect of local client
	Cvar_Set( "sv_killserver", "2" );
	Cvar_Set("com_workshopids", "");

#if 0
	for (i = 1;  i < n  &&  i <= MAX_DEMO_FILES;  i++) {
		arg = Cmd_Argv(i);
		Q_strncpyz(DemoNames[i - 1], arg, MAX_OSPATH);
	}

	arg = DemoNames[0];
#endif

	arg = Cmd_Argv(1);

	CL_Disconnect( qtrue );

	clc.demoReadFile = CL_OpenDemoFile(arg, qtrue);
	if (!clc.demoReadFile) {
		Com_Printf("^1CL_StreamDemo_f() couldn't open demo file '%s'\n", arg);
		return;
	}

	//FIXME
	memset(&di, 0, sizeof(di));

	//FIXME
	for (i = 0;  i < MAX_DEMO_FILES;  i++) {
		di.demoFiles[i].num = i;
	}
	di.demoFiles[0].f = clc.demoReadFile;
	di.demoFiles[0].valid = qtrue;
	di.numDemoFiles++;

#if 0
	//n = Cmd_Argc();
	for (i = 2;  i < n  &&  i <= MAX_DEMO_FILES;  i++) {
		qhandle_t file;
		demoFile_t *df;

#if 0
		if (di.numDemoFiles > MAX_DEMO_FILES) {
			Com_Printf("^3CL_PlayDemo_f MAX_DEMO_FILES(%d)\n", MAX_DEMO_FILES);
			break;
		}
#endif

		//arg = Cmd_Argv(i);
		arg = DemoNames[i - 1];
		Com_Printf("^4'%s'\n", arg);
		//di.demoFiles[di.numDemoFiles].f = CL_OpenDemoFile(arg, qtrue);
		file = CL_OpenDemoFile(arg, qtrue);
		if (!file) {
			Com_Printf("^3CL_PlayDemo_f couldn't open '%s'\n", arg);
			continue;
		}
		Com_Printf("%d\n", file);
		df = &di.demoFiles[di.numDemoFiles];
		df->f = file;
		df->valid = qtrue;
		di.numDemoFiles++;
	}
#endif

	di.streaming = qtrue;

	//Q_strncpyz( clc.demoName, Cmd_Argv(1), sizeof( clc.demoName ) );

	Con_Close();

	//parse_demo();
	//FIXME without parse_demo() how do you check for protocol <= 48?

	// CL_CheckWorkshopDownload() advances to CA_CONNECTED
	clc.state = CA_DOWNLOADINGWORKSHOPS;
	clc.demoplaying = qtrue;
	clc.demoPlayBegin = Sys_Milliseconds();

	// let CL_CheckWorkshopDownload() know it needs to initialize
	clc.demoWorkshopsString = NULL;
	Q_strncpyz( clc.servername, arg, sizeof( clc.servername ) );


#ifdef LEGACY_PROTOCOL
#if 0  //FIXME check
	if(protocol <= com_legacyprotocol->integer)
		clc.compat = qtrue;
	else
		clc.compat = qfalse;
#endif
#endif

	Com_Printf("^5checking workshops\n");
	// check immediately so it doesn't flash download screen if the file
	// is present or steamcmd not available
	CL_CheckWorkshopDownload();
}


/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop( void ) {
	// start the demo loop again
	Cbuf_AddText ("d1\n");
	Key_SetCatcher( 0 );
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo( void ) {
	char	v[MAX_STRING_CHARS];

	Q_strncpyz( v, Cvar_VariableString ("nextdemo"), sizeof(v) );
	v[MAX_STRING_CHARS-1] = 0;
	Com_DPrintf("CL_NextDemo: %s\n", v );
	if (!v[0]) {
		return;
	}

	Cvar_Set ("nextdemo","");
	Cbuf_AddText (v);
	Cbuf_AddText ("\n");
	Cbuf_Execute();
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(qboolean shutdownRef)
{

	if(CL_VideoRecording(&afdMain))
		CL_StopVideo_f();

	if(clc.demorecording)
		CL_StopRecord_f();

	//Com_Printf("CL_ShutdownAll()\n");
#ifdef USE_CURL
	CL_cURL_Shutdown();
#endif
	// clear sounds
	S_DisableSounds();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	if(shutdownRef)
		CL_ShutdownRef();
	else if (re.Shutdown)
		re.Shutdown(qfalse);			// don't destroy window or context

	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.rendererStarted = qfalse;
	cls.soundRegistered = qfalse;
}

/*
=================
CL_ClearMemory

Called by Com_GameRestart
=================
*/
void CL_ClearMemory(qboolean shutdownRef)
{
	// shutdown all the client stuff
	CL_ShutdownAll(shutdownRef);

	// if not running a server clear the whole hunk
	if ( !com_sv_running || !com_sv_running->integer ) {
		// clear the whole hunk
		Hunk_Clear();
		// clear collision map data
		CM_ClearMap();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// hack for devmap and demo both specified on the command line (same map)
	//   +devmap spidercrossings +demo spiderdemo1
	//
	//   That causes the server code above to run instead of client.
	//   This can crash with /vid_restart since the cm data is invalid but
	//   the check for cm.name in cm loading can still pass.
	if ((com_sv_running  &&  com_sv_running->integer)  &&  clc.demoReadFile) {
		CM_ClearMap();
	}
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{
	CL_ClearMemory(qfalse);
	CL_StartHunkUsers(qfalse);
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading( void ) {
	if ( com_dedicated->integer ) {
		clc.state = CA_DISCONNECTED;
		Key_SetCatcher( KEYCATCH_CONSOLE );
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	Con_Close();
	Key_SetCatcher( 0 );

	// if we are already connected to the local host, stay connected
	if ( clc.state >= CA_CONNECTED && !Q_stricmp( clc.servername, "localhost" ) ) {
		clc.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
		Com_Memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
		Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	} else {
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set( "nextmap", "" );
		CL_Disconnect( qtrue );
		Q_strncpyz( clc.servername, "localhost", sizeof(clc.servername) );
		clc.state = CA_CHALLENGING;		// so the connect screen is drawn
		Key_SetCatcher( 0 );
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( clc.servername, &clc.serverAddress, NA_UNSPEC);
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState (void) {

//	S_StopAllSounds();

	Com_Memset( &cl, 0, sizeof( cl ) );
}

/*
====================
CL_UpdateGUID

update cl_guid using QKEY_FILE and optional prefix
====================
*/
static void CL_UpdateGUID( const char *prefix, int prefix_len )
{
	fileHandle_t f;
	int len;

	len = FS_SV_FOpenFileRead( QKEY_FILE, &f );
	FS_FCloseFile( f );

	if( len != QKEY_SIZE )
		Cvar_Set( "cl_guid", "" );
	else
		Cvar_Set( "cl_guid", Com_MD5File( QKEY_FILE, QKEY_SIZE,
			prefix, prefix_len ) );
}

static void CL_OldGame(void)
{
	if(cl_oldGameSet)
	{
		// change back to previous fs_game
		cl_oldGameSet = qfalse;
		Cvar_Set2("fs_game", cl_oldGame, qtrue);
		FS_ConditionalRestart(clc.checksumFeed, qfalse);
	}
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( qboolean showMainMenu ) {
	int i;

	if ( !com_cl_running || !com_cl_running->integer ) {
		return;
	}

	// Stop recording any video
	if (CL_VideoRecording(&afdMain)) {
		// Finish rendering current frame
		SCR_UpdateScreen( );
		CL_CloseAVI(&afdMain, qfalse);
		if (Video_DepthBuffer) {
			CL_CloseAVI(&afdDepth, qfalse);
			CL_CloseAVI(&afdDepthLeft, qfalse);
			CL_CloseAVI(&afdDepthRight, qfalse);
			free(Video_DepthBuffer);
			Video_DepthBuffer = NULL;
		}

		if (SplitVideo) {
			CL_CloseAVI(&afdLeft, qfalse);
			CL_CloseAVI(&afdRight, qfalse);
			free(ExtraVideoBuffer);
			ExtraVideoBuffer = NULL;
		}
	}

	Com_Printf("disconnect\n");
	Cvar_Set("cl_freezeDemo", "0");

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if ( clc.demorecording ) {
		CL_StopRecord_f ();
	}

	if (clc.wfp) {
		Sys_PopenClose(clc.wfp);
		free(clc.wfp);
		clc.wfp = NULL;
		clc.currentWorkshop[0] = '\0';
		clc.demoWorkshopsString = NULL;
	}

	if (clc.download) {
		FS_FCloseFile( clc.download );
		clc.download = 0;
	}
	*clc.downloadTempName = *clc.downloadName = 0;
	Cvar_Set( "cl_downloadName", "" );

#ifdef USE_MUMBLE
	if (cl_useMumble->integer && mumble_islinked()) {
		Com_Printf("Mumble: Unlinking from Mumble application\n");
		mumble_unlink();
	}
#endif

#ifdef USE_VOIP
	if (cl_voipSend->integer) {
		int tmp = cl_voipUseVAD->integer;
		cl_voipUseVAD->integer = 0;  // disable this for a moment.
		clc.voipOutgoingDataSize = 0;  // dump any pending VoIP transmission.
		Cvar_Set("cl_voipSend", "0");
		CL_CaptureVoip();  // clean up any state...
		cl_voipUseVAD->integer = tmp;
	}

	if (clc.speexInitialized) {
		int i;
		speex_bits_destroy(&clc.speexEncoderBits);
		speex_encoder_destroy(clc.speexEncoder);
		speex_preprocess_state_destroy(clc.speexPreprocessor);
		for (i = 0; i < MAX_CLIENTS; i++) {
			speex_bits_destroy(&clc.speexDecoderBits[i]);
			speex_decoder_destroy(clc.speexDecoder[i]);
		}
		clc.speexInitialized = qfalse;
	}

	if (clc.voipCodecInitialized) {
		int i;
		opus_encoder_destroy(clc.opusEncoder);
		for (i = 0; i < MAX_CLIENTS; i++) {
			opus_decoder_destroy(clc.opusDecoder[i]);
		}
		clc.voipCodecInitialized = qfalse;
	}
	Cmd_RemoveCommand ("voip");
#endif

	if ( clc.demoReadFile ) {
		FS_FCloseFile( clc.demoReadFile );
		clc.demoReadFile = 0;
		//di.demoFiles[0].f = 0;
		//di.demoFiles[0].valid = qfalse;

		for (i = 1;  i < di.numDemoFiles;  i++) {
			demoFile_t *df;

			df = &di.demoFiles[i];
			if (!df->valid) {
				continue;
			}

			FS_FCloseFile(df->f);
		}

		for (i = 0;  i < maxRewindBackups;  i++) {
			rewindBackups[i].valid = qfalse;
		}

		memset(&di, 0, sizeof(demoInfo_t));
	}

	if ( uivm && showMainMenu ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}

	SCR_StopCinematic ();
	S_ClearSoundBuffer();

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if ( clc.state >= CA_CONNECTED ) {
		CL_AddReliableCommand("disconnect", qtrue);
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	// Remove pure paks
	FS_PureServerSetLoadedPaks("", "");
	FS_PureServerSetReferencedPaks( "", "" );

	CL_ClearState ();

	// wipe the client connection
	Com_Memset( &clc, 0, sizeof( clc ) );

	clc.state = CA_DISCONNECTED;

	// allow cheats locally
	Cvar_Set( "sv_cheats", "1" );

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;

#ifdef USE_VOIP
	// not connected to voip server anymore.
	clc.voipEnabled = qfalse;
#endif

	CL_UpdateGUID( NULL, 0 );
	CL_ShutdownCGame();

	if(!noGameRestart)
		CL_OldGame();
	else
		noGameRestart = qfalse;

	Cvar_Set("protocol", va("%i", SERVER_PROTOCOL));
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer( const char *string ) {
	char	*cmd;

	cmd = Cmd_Argv(0);

	//printf("cmd: '%s'  '%s'\n", cmd, string);

	// ignore key up commands
	if ( cmd[0] == '-' ) {
		return;
	}

	if ( clc.demoplaying || clc.state < CA_CONNECTED || cmd[0] == '+' ) {
		Com_Printf ("Unknown command \"%s" S_COLOR_WHITE "\"\n", cmd);
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand(string, qfalse);
	} else {
		CL_AddReliableCommand(cmd, qfalse);
	}
}

/*
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd( void ) {
#ifdef UPDATE_SERVER_NAME
	char		info[MAX_INFO_STRING];

	if ( !cl_motd->integer ) {
		return;
	}
	Com_Printf( "Resolving %s\n", UPDATE_SERVER_NAME );
	if ( !NET_StringToAdr( UPDATE_SERVER_NAME, &cls.updateServer, NA_IP ) ) {
		Com_Printf( "Couldn't resolve address\n" );
		return;
	}
	cls.updateServer.port = BigShort( PORT_UPDATE );
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", UPDATE_SERVER_NAME,
		cls.updateServer.ip[0], cls.updateServer.ip[1],
		cls.updateServer.ip[2], cls.updateServer.ip[3],
		BigShort( cls.updateServer.port ) );

	info[0] = 0;

	Com_sprintf( cls.updateChallenge, sizeof( cls.updateChallenge ), "%i", (int)((((unsigned int)rand() << 16) ^ (unsigned int)rand()) ^ Com_Milliseconds()));

	Info_SetValueForKey( info, "challenge", cls.updateChallenge );
	Info_SetValueForKey( info, "renderer", cls.glconfig.renderer_string );
	Info_SetValueForKey( info, "version", com_version->string );

	NET_OutOfBandPrint( NS_CLIENT, cls.updateServer, "getmotd \"%s\"\n", info );
#endif
}

/*
===================
CL_RequestAuthorization

Authorization server protocol
-----------------------------

All commands are text in Q3 out of band packets (leading 0xff 0xff 0xff 0xff).

Whenever the client tries to get a challenge from the server it wants to
connect to, it also blindly fires off a packet to the authorize server:

getKeyAuthorize <challenge> <cdkey>

cdkey may be "demo"


#OLD The authorize server returns a:
#OLD
#OLD keyAthorize <challenge> <accept | deny>
#OLD
#OLD A client will be accepted if the cdkey is valid and it has not been used by any other IP
#OLD address in the last 15 minutes.


The server sends a:

getIpAuthorize <challenge> <ip>

The authorize server returns a:

ipAuthorize <challenge> <accept | deny | demo | unknown >

A client will be accepted if a valid cdkey was sent by that ip (only) in the last 15 minutes.
If no response is received from the authorize server after two tries, the client will be let
in anyway.
===================
*/
#ifndef STANDALONE
void CL_RequestAuthorization( void ) {
	char	nums[64];
	int		i, j, l;
	cvar_t	*fs;

	if ( !cls.authorizeServer.port ) {
		Com_Printf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );
		if ( !NET_StringToAdr( AUTHORIZE_SERVER_NAME, &cls.authorizeServer, NA_IP ) ) {
			Com_Printf( "Couldn't resolve address\n" );
			return;
		}

		cls.authorizeServer.port = BigShort( PORT_AUTHORIZE );
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
			cls.authorizeServer.ip[0], cls.authorizeServer.ip[1],
			cls.authorizeServer.ip[2], cls.authorizeServer.ip[3],
			BigShort( cls.authorizeServer.port ) );
	}
	if ( cls.authorizeServer.type == NA_BAD ) {
		return;
	}

	// only grab the alphanumeric values from the cdkey, to avoid any dashes or spaces
	j = 0;
	l = strlen( cl_cdkey );
	if ( l > 32 ) {
		l = 32;
	}
	for ( i = 0 ; i < l ; i++ ) {
		if ( ( cl_cdkey[i] >= '0' && cl_cdkey[i] <= '9' )
				|| ( cl_cdkey[i] >= 'a' && cl_cdkey[i] <= 'z' )
				|| ( cl_cdkey[i] >= 'A' && cl_cdkey[i] <= 'Z' )
			 ) {
			nums[j] = cl_cdkey[i];
			j++;
		}
	}
	nums[j] = 0;

	fs = Cvar_Get ("cl_anonymous", "0", CVAR_INIT|CVAR_SYSTEMINFO );

	NET_OutOfBandPrint(NS_CLIENT, cls.authorizeServer, "getKeyAuthorize %i %s", fs->integer, nums );
}
#endif
/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f( void ) {
	if ( clc.state != CA_ACTIVE || clc.demoplaying ) {
		Com_Printf ("Not connected to a server.\n");
		return;
	}

	// don't forward the first argument
	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand(Cmd_Args(), qfalse);
	}
}

/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f( void ) {
	SCR_StopCinematic();
	Cvar_Set("ui_singlePlayerActive", "0");
	if ( clc.state != CA_DISCONNECTED && clc.state != CA_CINEMATIC ) {
		Com_Error (ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f( void ) {
	if ( !strlen( cl_reconnectArgs ) )
		return;

	Cvar_Set("ui_singlePlayerActive", "0");
	Cbuf_AddText( va("connect %s\n", cl_reconnectArgs ) );
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f( void ) {
	char	server[MAX_OSPATH];
	const char	*serverString;
	int argc = Cmd_Argc();
	netadrtype_t family = NA_UNSPEC;

	if ( argc != 2 && argc != 3 ) {
		Com_Printf( "usage: connect [-4|-6] server\n");
		return;
	}

	if(argc == 2)
		Q_strncpyz( server, Cmd_Argv(1), sizeof( server ) );
	else
	{
		if(!strcmp(Cmd_Argv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(Cmd_Argv(1), "-6"))
			family = NA_IP6;
		else
			Com_Printf( "warning: only -4 or -6 as address type understood.\n");

		Q_strncpyz( server, Cmd_Argv(2), sizeof( server ) );
	}

	// save arguments for reconnect
	Q_strncpyz( cl_reconnectArgs, Cmd_Args(), sizeof( cl_reconnectArgs ) );

	Cvar_Set("ui_singlePlayerActive", "0");

	// fire a message off to the motd server
	CL_RequestMotd();

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		// if running a local server, kill it
		SV_Shutdown( "Server quit" );
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0, 0 );

	noGameRestart = qtrue;
	CL_Disconnect( qtrue );
	Con_Close();

	Q_strncpyz( clc.servername, server, sizeof(clc.servername) );

	if (!NET_StringToAdr(clc.servername, &clc.serverAddress, family) ) {
		Com_Printf ("Bad server address\n");
		clc.state = CA_DISCONNECTED;
		return;
	}
	if (clc.serverAddress.port == 0) {
		clc.serverAddress.port = BigShort( PORT_SERVER );
	}

	serverString = NET_AdrToStringwPort(clc.serverAddress);

	Com_Printf( "%s resolved to %s\n", clc.servername, serverString);

	if( cl_guidServerUniq->integer )
		CL_UpdateGUID( serverString, strlen( serverString ) );
	else
		CL_UpdateGUID( NULL, 0 );

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if(NET_IsLocalAddress(clc.serverAddress))
		clc.state = CA_CHALLENGING;
	else
	{
		clc.state = CA_CONNECTING;

		// Set a client challenge number that ideally is mirrored back by the server.
		clc.challenge = (((unsigned int)rand() << 16) ^ (unsigned int)rand()) ^ Com_Milliseconds();
	}

	Key_SetCatcher( 0 );
	clc.connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
}

#define MAX_RCON_MESSAGE 1024

/*
==================
CL_CompleteRcon
==================
*/
static void CL_CompleteRcon( char *args, int argNum )
{
	if( argNum == 2 )
	{
		// Skip "rcon "
		char *p = Com_SkipTokens( args, 1, " " );

		if( p > args )
			Field_CompleteCommand( p, qtrue, qtrue );
	}
}

/*
==================
CL_CompletePlayerName
==================
*/
static void CL_CompletePlayerName( char *args, int argNum )
{
	if( argNum == 2 )
	{
		char		names[MAX_CLIENTS][MAX_NAME_LENGTH];
		const char	*namesPtr[MAX_CLIENTS];
		int			i;
		int			clientCount;
		int			nameCount;
		const char *info;
		const char *name;

		//configstring
		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
		clientCount = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

		nameCount = 0;

		for( i = 0; i < clientCount; i++ ) {
			if( i == clc.clientNum )
				continue;

			info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+i];

			name = Info_ValueForKey( info, "n" );
			if( name[0] == '\0' )
				continue;
			Q_strncpyz( names[nameCount], name, sizeof(names[nameCount]) );
			Q_CleanStr( names[nameCount] );

			namesPtr[nameCount] = names[nameCount];
			nameCount++;
		}
		qsort( (void*)namesPtr, nameCount, sizeof( namesPtr[0] ), Com_strCompare );

		Field_CompletePlayerName( namesPtr, nameCount );
	}
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f( void ) {
	char	message[MAX_RCON_MESSAGE];
	netadr_t	to;

	if ( !rcon_client_password->string[0] ) {
		Com_Printf ("You must set 'rconpassword' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	Q_strcat (message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat (message, MAX_RCON_MESSAGE, rcon_client_password->string);
	Q_strcat (message, MAX_RCON_MESSAGE, " ");

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
	Q_strcat (message, MAX_RCON_MESSAGE, Cmd_Cmd()+5);

	if ( clc.state >= CA_CONNECTED ) {
		to = clc.netchan.remoteAddress;
	} else {
		if (!strlen(rconAddress->string)) {
			Com_Printf ("You must either be connected,\n"
						"or set the 'rconAddress' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rconAddress->string, &to, NA_UNSPEC);
		if (to.port == 0) {
			to.port = BigShort (PORT_SERVER);
		}
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
	cls.rconAddress = to;
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums( void ) {
	char cMsg[MAX_INFO_VALUE];

	// if we are pure we need to send back a command with our referenced pk3 checksums
	Com_sprintf(cMsg, sizeof(cMsg), "cp %d %s", cl.serverId, FS_ReferencedPakPureChecksums());

	CL_AddReliableCommand(cMsg, qfalse);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer( void ) {
	CL_AddReliableCommand("vdr", qfalse);
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void CL_Vid_Restart_f (void)
{
	connstate_t state;

	state = clc.state;

	// Settings may have changed so stop recording now
	if(CL_VideoRecording(&afdMain)) {
		CL_CloseAVI(&afdMain, qfalse);
		if (Video_DepthBuffer) {
			CL_CloseAVI(&afdDepth, qfalse);
			CL_CloseAVI(&afdDepthLeft, qfalse);
			CL_CloseAVI(&afdDepthRight, qfalse);
			free(Video_DepthBuffer);
			Video_DepthBuffer = NULL;
		}

		if (SplitVideo) {
			CL_CloseAVI(&afdLeft, qfalse);
			CL_CloseAVI(&afdRight, qfalse);
			free(ExtraVideoBuffer);
			ExtraVideoBuffer = NULL;
		}
	}

	if(clc.demorecording)
		CL_StopRecord_f();

	// don't let them loop during the restart
	S_StopAllSounds();

	if(!FS_ConditionalRestart(clc.checksumFeed, qtrue))
	{
		// if not running a server clear the whole hunk
		if(com_sv_running->integer)
		{
			// clear all the client data on the hunk
			Hunk_ClearToMark();
		}
		else
		{
			// clear the whole hunk
			Hunk_Clear();
		}
	
		// shutdown the UI
		CL_ShutdownUI();
		// shutdown the CGame
		CL_ShutdownCGame();
		// shutdown the renderer and clear the renderer interface
		CL_ShutdownRef();
		// client is no longer pure until new checksums are sent
		CL_ResetPureClientAtServer();
		// clear pak references
		FS_ClearPakReferences( FS_UI_REF | FS_CGAME_REF );
		// reinitialize the filesystem if the game directory or checksum has changed

		cls.rendererStarted = qfalse;
		cls.uiStarted = qfalse;
		cls.cgameStarted = qfalse;
		cls.soundRegistered = qfalse;

		// unpause so the cgame definitely gets a snapshot and renders a frame
		Cvar_Set("cl_paused", "0");

		// initialize the renderer interface
		CL_InitRef();

		// startup all the client stuff
		CL_StartHunkUsers(qfalse);

		// start the cgame if connected
		if(clc.state > CA_CONNECTED && clc.state != CA_CINEMATIC)
		{
			cls.cgameStarted = qtrue;
			CL_InitCGame();
			// send pure checksums
			CL_SendPureChecksums();
			if (clc.demoplaying  &&  cl_freezeDemo->integer) {
				clc.state = state;
			}
		}

	}

}

/*
=================
CL_Snd_Shutdown

Restart the sound subsystem
=================
*/
void CL_Snd_Shutdown(void)
{
	S_Shutdown();
	cls.soundStarted = qfalse;
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void)
{
	CL_Snd_Shutdown();
	// sound will be reinitialized by vid_restart
	CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f( void ) {
	Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f( void ) {
	Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f( void ) {
	int		i;
	int		ofs;
	int		numArgs;
	int		n = 0;

	if ( clc.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n");
		return;
	}

	numArgs = Cmd_Argc();
	if (numArgs > 1) {
		n = atoi(Cmd_Argv(1));
	}

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		ofs = cl.gameState.stringOffsets[ i ];
		if ( !ofs ) {
			continue;
		}
		if (numArgs < 2  ||  i == n) {
			Com_Printf( "%4i: %s\n", i, cl.gameState.stringData + ofs );
		}
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f( void ) {
	Com_Printf( "--------- Client Information ---------\n" );
	Com_Printf( "state: %i\n", clc.state );
	Com_Printf( "Server: %s\n", clc.servername );
	Com_Printf ("User info settings:\n");
	Info_Print( Cvar_InfoString( CVAR_USERINFO ) );
	Com_Printf( "--------------------------------------\n" );
}


//====================================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete( void ) {

#ifdef USE_CURL
	// if we downloaded with cURL
	if(clc.cURLUsed) { 
		clc.cURLUsed = qfalse;
		CL_cURL_Shutdown();
		if( clc.cURLDisconnected ) {
			if(clc.downloadRestart) {
				FS_Restart(clc.checksumFeed);
				clc.downloadRestart = qfalse;
			}
			clc.cURLDisconnected = qfalse;
			CL_Reconnect_f();
			return;
		}
	}
#endif

	// if we downloaded files we need to restart the file system
	if (clc.downloadRestart) {
		clc.downloadRestart = qfalse;

		FS_Restart(clc.checksumFeed); // We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand("donedl", qfalse);

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	clc.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if ( clc.state != CA_LOADING ) {
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory();

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload( const char *localName, const char *remoteName ) {

	Com_DPrintf("***** CL_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

	Q_strncpyz ( clc.downloadName, localName, sizeof(clc.downloadName) );
	Com_sprintf( clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName );

	// Set so UI gets access to it
	Cvar_Set( "cl_downloadName", remoteName );
	Cvar_Set( "cl_downloadSize", "0" );
	Cvar_Set( "cl_downloadCount", "0" );
	Cvar_SetValue( "cl_downloadTime", cls.realtime );

	clc.downloadBlock = 0; // Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName), qfalse);
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void)
{
	char *s;
	char *remoteName, *localName;
	qboolean useCURL = qfalse;

	// A download has finished, check whether this matches a referenced checksum
	if(*clc.downloadName)
	{
		char *zippath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), clc.downloadName, "");
		zippath[strlen(zippath)-1] = '\0';

		if(!FS_CompareZipChecksum(zippath))
			Com_Error(ERR_DROP, "Incorrect checksum for file: %s", clc.downloadName);
	}

	*clc.downloadTempName = *clc.downloadName = 0;
	Cvar_Set("cl_downloadName", "");

	// We are looking to start a download here
	if (*clc.downloadList) {
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
			s++;
		remoteName = s;

		if ( (s = strchr(s, '@')) == NULL ) {
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if ( (s = strchr(s, '@')) != NULL )
			*s++ = 0;
		else
			s = localName + strlen(localName); // point at the nul byte
#ifdef USE_CURL
		if(!(cl_allowDownload->integer & DLF_NO_REDIRECT)) {
			if(clc.sv_allowDownload & DLF_NO_REDIRECT) {
				Com_Printf("WARNING: server does not "
					"allow download redirection "
					"(sv_allowDownload is %d)\n",
					clc.sv_allowDownload);
			}
			else if(!*clc.sv_dlURL) {
				Com_Printf("WARNING: server allows "
					"download redirection, but does not "
					"have sv_dlURL set\n");
			}
			else if(!CL_cURL_Init()) {
				Com_Printf("WARNING: could not load "
					"cURL library\n");
			}
			else {
				CL_cURL_BeginDownload(localName, va("%s/%s",
					clc.sv_dlURL, remoteName));
				useCURL = qtrue;
			}
		}
		else if(!(clc.sv_allowDownload & DLF_NO_REDIRECT)) {
			Com_Printf("WARNING: server allows download "
				"redirection, but it disabled by client "
				"configuration (cl_allowDownload is %d)\n",
				cl_allowDownload->integer);
		}
#endif /* USE_CURL */
		if(!useCURL) {
			if((cl_allowDownload->integer & DLF_NO_UDP)) {
				Com_Error(ERR_DROP, "UDP Downloads are "
					"disabled on your client. "
					"(cl_allowDownload is %d)",
					cl_allowDownload->integer);
				return;
			}
			else {
				CL_BeginDownload( localName, remoteName );
			}
		}
		clc.downloadRestart = qtrue;

		// move over the rest
		memmove( clc.downloadList, s, strlen(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void) {
  char missingfiles[1024];

  //Com_Printf("^1init downloads\n");
  if ( !(cl_allowDownload->integer & DLF_ENABLE) )
  {
    // autodownload is disabled on the client
    // but it's possible that some referenced files on the server are missing
    if (FS_ComparePaks( missingfiles, sizeof( missingfiles ), qfalse ) )
    {
      // NOTE TTimo I would rather have that printed as a modal message box
      //   but at this point while joining the game we don't know wether we will successfully join or not
      Com_Printf( "\nWARNING: You are missing some files referenced by the server:\n%s"
                  "You might not be able to join the game\n"
                  "Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n", missingfiles );
    }
  }
  else if ( FS_ComparePaks( clc.downloadList, sizeof( clc.downloadList ) , qtrue ) ) {

    Com_Printf("Need paks: %s\n", clc.downloadList );

		if ( *clc.downloadList ) {
			// if autodownloading is not enabled on the server
			clc.state = CA_CONNECTED;

			*clc.downloadTempName = *clc.downloadName = 0;
			Cvar_Set( "cl_downloadName", "" );

			CL_NextDownload();
			return;
		}

	}

	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend( void ) {
	int		port;
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING + 10];

	// don't send anything if playing back a demo
	if ( clc.demoplaying ) {
		return;
	}

	// resend if we haven't gotten a reply yet
	if ( clc.state != CA_CONNECTING && clc.state != CA_CHALLENGING ) {
		return;
	}

	if ( cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT ) {
		return;
	}

	clc.connectTime = cls.realtime;	// for retransmit requests
	clc.connectPacketCount++;


	switch ( clc.state ) {
	case CA_CONNECTING:
		// requesting a challenge .. IPv6 users always get in as authorize server supports no ipv6.
#ifndef STANDALONE
		if (!com_standalone->integer && clc.serverAddress.type == NA_IP && !Sys_IsLANAddress( clc.serverAddress ) )
			CL_RequestAuthorization();
#endif

		// The challenge request shall be followed by a client challenge so no malicious server can hijack this connection.
		// Add the gamename so the server knows we're running the correct game or can reject the client
		// with a meaningful message
		Com_sprintf(data, sizeof(data), "getchallenge %d %s", clc.challenge, com_gamename->string);

		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "%s", data);
		break;

	case CA_CHALLENGING:
		// sending back the challenge
		port = Cvar_VariableValue ("net_qport");

		Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO ), sizeof( info ) );

#ifdef LEGACY_PROTOCOL
		if(com_legacyprotocol->integer == com_protocol->integer)
			clc.compat = qtrue;

		if(clc.compat)
			Info_SetValueForKey(info, "protocol", va("%i", com_legacyprotocol->integer));
		else
#endif
			Info_SetValueForKey(info, "protocol", va("%i", com_protocol->integer));
		Info_SetValueForKey( info, "qport", va("%i", port ) );
		Info_SetValueForKey( info, "challenge", va("%i", clc.challenge ) );

		Com_sprintf( data, sizeof(data), "connect \"%s\"", info );
		NET_OutOfBandData( NS_CLIENT, clc.serverAddress, (byte *) data, strlen ( data ) );
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error( ERR_FATAL, "CL_CheckForResend: bad clc.state" );
	}
}

#if 0  // removed in ioquake3 svn 2366
/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket( netadr_t from ) {
	if ( clc.state < CA_AUTHORIZING ) {
		return;
	}

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		return;
	}

	// if we have received packets within three seconds, ignore it
	// (it might be a malicious spoof)
	if ( cls.realtime - clc.lastPacketTime < 3000 ) {
		return;
	}

	// drop the connection
	Com_Printf( "Server disconnected for unknown reason\n" );
	Cvar_Set("com_errorMessage", "Server disconnected for unknown reason\n" );
	CL_Disconnect( qtrue );
}
#endif

/*
===================
CL_MotdPacket

===================
*/
void CL_MotdPacket( netadr_t from ) {
#ifdef UPDATE_SERVER_NAME
	char	*challenge;
	char	*info;

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, cls.updateServer ) ) {
		return;
	}

	info = Cmd_Argv(1);

	// check challenge
	challenge = Info_ValueForKey( info, "challenge" );
	if ( strcmp( challenge, cls.updateChallenge ) ) {
		return;
	}

	challenge = Info_ValueForKey( info, "motd" );

	Q_strncpyz( cls.updateInfoString, info, sizeof( cls.updateInfoString ) );
	Cvar_Set( "cl_motdString", challenge );
#endif
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo( serverInfo_t *server, const netadr_t *address ) {
	server->adr = *address;
	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType = 0;
	server->netType = 0;
	server->punkbuster = 0;
	server->g_humanplayers = 0;
	server->g_needpass = 0;
}

#define MAX_SERVERSPERPACKET	256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket( const netadr_t* from, msg_t *msg, qboolean extended ) {
	int				i, j, count, total;
	netadr_t addresses[MAX_SERVERSPERPACKET];
	int				numservers;
	byte*			buffptr;
	byte*			buffend;

	Com_Printf("CL_ServersResponsePacket from %s\n", NET_AdrToStringwPort(*from));

	if (cls.numglobalservers == -1) {
		// state to detect lack of servers or lack of response
		cls.numglobalservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->data;
	buffend    = buffptr + msg->cursize;

	// advance to initial token
	do
	{
		if(*buffptr == '\\' || (extended && *buffptr == '/'))
			break;

		buffptr++;
	} while (buffptr < buffend);

	while (buffptr + 1 < buffend)
	{
		// IPv4 address
		if (*buffptr == '\\')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip) + sizeof(addresses[numservers].port) + 1)
				break;

			for(i = 0; i < sizeof(addresses[numservers].ip); i++)
				addresses[numservers].ip[i] = *buffptr++;

			addresses[numservers].type = NA_IP;
		}
		// IPv6 address, if it's an extended response
		else if (extended && *buffptr == '/')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip6) + sizeof(addresses[numservers].port) + 1)
				break;

			for(i = 0; i < sizeof(addresses[numservers].ip6); i++)
				addresses[numservers].ip6[i] = *buffptr++;

			addresses[numservers].type = NA_IP6;
			addresses[numservers].scope_id = from->scope_id;
		}
		else
			// syntax error!
			break;

		// parse out port
		addresses[numservers].port = (*buffptr++) << 8;
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port = BigShort( addresses[numservers].port );

		// syntax check
		if (*buffptr != '\\' && *buffptr != '/')
			break;

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET)
			break;
	}

	count = cls.numglobalservers;

	for (i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++) {
		// build net address
		serverInfo_t *server = &cls.globalServers[count];

		// Tequila: It's possible to have sent many master server requests. Then
		// we may receive many times the same addresses from the master server.
		// We just avoid to add a server if it is still in the global servers list.
		for (j = 0; j < count; j++)
		{
			if (NET_CompareAdr(cls.globalServers[j].adr, addresses[i]))
				break;
		}

		if (j < count)
			continue;

		CL_InitServerInfo( server, &addresses[i] );
		// advance to next slot
		count++;
	}

	// if getting the global list
	if ( count >= MAX_GLOBAL_SERVERS && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS )
	{
		// if we couldn't store the servers in the main list anymore
		for (; i < numservers && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS; i++)
		{
			// just store the addresses in an additional list
			cls.globalServerAddresses[cls.numGlobalServerAddresses++] = addresses[i];
		}
	}

	cls.numglobalservers = count;
	total = count + cls.numGlobalServerAddresses;

	Com_Printf("%d servers parsed (total %d)\n", numservers, total);
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char	*s;
	char	*c;
	int challenge = 0;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );	// skip the -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);

	Com_DPrintf ("CL packet %s: %s\n", NET_AdrToStringwPort(from), c);

	// challenge from the server we are connecting to
	if (!Q_stricmp(c, "challengeResponse"))
	{
		char *strver;
		int ver;

		if (clc.state != CA_CONNECTING)
		{
			Com_DPrintf("Unwanted challenge response received.  Ignored.\n");
			return;
		}

		c = Cmd_Argv(2);
		if(*c)
			challenge = atoi(c);

		strver = Cmd_Argv(3);
		if(*strver)
		{
			ver = atoi(strver);

			if (ver != com_protocol->integer)
			{
#ifdef LEGACY_PROTOCOL
				if(com_legacyprotocol->integer > 0)
				{
					// Server is ioq3 but has a different protocol than we do.
					// Fall back to idq3 protocol.
					clc.compat = qtrue;

					Com_Printf(S_COLOR_YELLOW "Warning: Server reports protocol version %d, "
							   "we have %d. Trying legacy protocol %d.\n",
							   ver, com_protocol->integer, com_legacyprotocol->integer);
				}
				else
#endif
				{
					Com_Printf(S_COLOR_YELLOW "Warning: Serer reports protocol version %d, we have %d. "
							   "Trying anyways.\n", ver, com_protocol->integer);
				}
			}
		}
#ifdef LEGACY_PROTOCOL
		else
			clc.compat = qtrue;

		if(clc.compat)
		{
			if(!NET_CompareAdr(from, clc.serverAddress))
			{
				// This challenge response is not coming from the expected address.
				// Check whether we have a matching client challenge to prevent
				// connection hi-jacking.

				if(!*c || challenge != clc.challenge)
				{
					Com_DPrintf("Challenge response received from unexpected source. Ignored.\n");
					return;
				}
			}
		}
		else
#endif
		{
			if(!*c || challenge != clc.challenge)
			{
				Com_Printf("Bad challenge for challengeResponse. Ignored.\n");
				return;
			}
		}

		// start sending challenge response instead of challenge request packets
		clc.challenge = atoi(Cmd_Argv(1));
		clc.state = CA_CHALLENGING;
		clc.connectPacketCount = 0;
		clc.connectTime = -99999;

		// take this address as the new server address.  This allows
		// a server proxy to hand off connections to multiple servers
		clc.serverAddress = from;
		Com_DPrintf ("challengeResponse: %d\n", clc.challenge);
		return;
	}

	// server connection
	if ( !Q_stricmp(c, "connectResponse") ) {
		if ( clc.state >= CA_CONNECTED ) {
			Com_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		if ( clc.state != CA_CHALLENGING ) {
			Com_Printf ("connectResponse packet while not connecting. Ignored.\n");
			return;
		}
		if ( !NET_CompareAdr( from, clc.serverAddress ) ) {
			Com_Printf( "connectResponse from wrong address. Ignored.\n" );
			return;
		}
#ifdef LEGACY_PROTOCOL
		if(!clc.compat)
#endif
		{
			c = Cmd_Argv(1);

			if(*c)
				challenge = atoi(c);
			else
			{
				Com_Printf("Bad connectResponse received. Ignored.\n");
				return;
			}

			if(challenge != clc.challenge)
			{
				Com_Printf("ConnectResponse with bad challenge received. Ignored.\n");
				return;
			}
		}

#ifdef LEGACY_PROTOCOL
		Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"),
					  clc.challenge, clc.compat);
#else
		Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"),
					  clc.challenge, qfalse);
#endif

		clc.state = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if ( !Q_stricmp(c, "infoResponse") ) {
		CL_ServerInfoPacket( from, msg );
		return;
	}

	// server responding to a get playerlist
	if ( !Q_stricmp(c, "statusResponse") ) {
		CL_ServerStatusResponse( from, msg );
		return;
	}

#if 0  // removed in ioquake3 svn 2075:  "Fix serveral UDP spoofing security issues
	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect")) {
		CL_DisconnectPacket( from );
		return;
	}
#endif

	// echo request from server
	if ( !Q_stricmp(c, "echo") ) {
		// NOTE: we may have to add exceptions for auth and update servers
		if ( NET_CompareAdr( from, clc.serverAddress ) || NET_CompareAdr( from, cls.rconAddress ) ) {
			NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv(1) );
		}
		return;
	}

	// cd check
	if ( !Q_stricmp(c, "keyAuthorize") ) {
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD from id
	if ( !Q_stricmp(c, "motd") ) {
		CL_MotdPacket( from );
		return;
	}

	// echo request from server
	// printf '\xFF\xFF\xFF\xFFprint\n hello world!\n' | nc -u -n -w 1 127.0.0.1 27960
	if( !Q_stricmp(c, "print") ) {
		// NOTE: we may have to add exceptions for auth and update servers
		if ( NET_CompareAdr( from, clc.serverAddress ) || NET_CompareAdr( from, cls.rconAddress ) ) {
			s = MSG_ReadString( msg );

			Q_strncpyz( clc.serverMessage, s, sizeof( clc.serverMessage ) );
			Com_Printf( "%s", s );
		}
		return;
	}

	// list of servers sent back by a master server (classic)
	if ( !Q_strncmp(c, "getserversResponse", 18) ) {
		CL_ServersResponsePacket( &from, msg, qfalse );
		return;
	}

	// list of servers sent back by a master server (extended)
	if ( !Q_strncmp(c, "getserversExtResponse", 21) ) {
		CL_ServersResponsePacket( &from, msg, qtrue );
		return;
	}

	Com_DPrintf ("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int		headerBytes;

	clc.lastPacketTime = cls.realtime;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	if ( clc.state < CA_CONNECTED ) {
		return;		// can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 ) {
		Com_Printf ("%s: Runt packet\n", NET_AdrToStringwPort( from ));
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		Com_DPrintf ("%s:sequenced packet without connection\n"
			, NET_AdrToStringwPort( from ) );
		// FIXME: send a client disconnect?
		return;
	}

	if (!CL_Netchan_Process( &clc.netchan, msg) ) {
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong( *(int *)msg->data );

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if ( clc.demorecording && !clc.demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout( void ) {
	//
	// check timeout
	//

	if (clc.demoplaying) {
		cl.timeoutcount = 0;
		return;
	}

	if ( ( !CL_CheckPaused() || !sv_paused->integer )
		&& clc.state >= CA_CONNECTED && clc.state != CA_CINEMATIC
	    && cls.realtime - clc.lastPacketTime > cl_timeout->value*1000) {
		if (++cl.timeoutcount > 5) {	// timeoutcount saves debugger
			Com_Printf ("\nServer connection timed out.\n");
			CL_Disconnect( qtrue );
			return;
		}
	} else {
		cl.timeoutcount = 0;
	}
}

/*
==================
CL_CheckPaused
Check whether client has been paused.
==================
*/
qboolean CL_CheckPaused(void)
{
	// if cl_paused->modified is set, the cvar has only been changed in
	// this frame. Keep paused in this frame to ensure the server doesn't
	// lag behind.
	if(cl_paused->integer || cl_paused->modified)
		return qtrue;

	return qfalse;
}

//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo( void ) {
	// don't add reliable commands when not yet connected
	if(clc.state < CA_CONNECTED)
		return;

	// don't overflow the reliable command buffer when paused
	if(CL_CheckPaused())
		return;

	// send a reliable userinfo update if needed
	if(cvar_modifiedFlags & CVAR_USERINFO)
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString( CVAR_USERINFO ) ), qfalse);
	}
}


static char downloadDir[MAX_OSPATH];
static char checkPath[MAX_OSPATH];

static char inputFile[MAX_OSPATH];
static char outputFile[MAX_OSPATH];

static void CL_CheckWorkshopDownload (void)
{
	qboolean doneParsing;

	if (clc.state != CA_DOWNLOADINGWORKSHOPS) {
		return;
	}

	if (cl_downloadWorkshops->integer == 0) {
		goto done;
	}

	//FIXME testing
	if (Sys_Milliseconds() - clc.demoPlayBegin < 11500) {
		//return;
	}

	if (clc.demoWorkshopsString == NULL) {
		clc.demoWorkshopsString = Cvar_VariableString("com_workshopids");
		clc.currentWorkshop[0] = '\0';
	}

 keepChecking:

	//Com_Printf("^2 CL_CheckWorkshopDownload:  '%s'\n", clc.demoWorkshopsString);
	doneParsing = qfalse;

	if (clc.currentWorkshop[0] == '\0') {
		const char *s;
		char *cw;
		int count;

		count = 0;
		cw = clc.currentWorkshop;
		s = clc.demoWorkshopsString;
		while (qtrue) {
			if (count >= sizeof(clc.currentWorkshop)) {
				Com_Printf("^1CL_CheckWorkshopDownload:  currentWorkshop buffer overrun : %d >= %d\n", count, (unsigned int)sizeof(clc.currentWorkshop));
				cw[0] = '\0';
				break;
			}
			cw[0] = '\0';

			if (*s == '\0') {
				doneParsing = qtrue;
				break;
			}

			if (*s == ' ') {
				s++;
				break;
			}

			cw[0] = s[0];

			count++;
			cw++;
			s++;
		}

		clc.demoWorkshopsString = s;
	}

	if (clc.currentWorkshop[0] != '\0') {  // downloading workshop
		char buffer[1024];
		const char *retp;
		const char *homePath;

		// get new workshop
		//Com_Printf("^6getting workshop %s\n", currentWorkshop);

		//FIXME check if directory exists and has files

		if (clc.wfp == NULL) {
			const char *s;

			homePath = Cvar_VariableString("fs_homepath");
			if (!*homePath) {
				homePath = Sys_DefaultHomePath();
			}

			//FIXME not here
			// make sure wolfcam workshop folder exists
			Com_sprintf(checkPath, sizeof(checkPath), "%s%cworkshop", homePath, PATH_SEP);
			Sys_Mkdir(checkPath);

			//Com_sprintf(checkPath, sizeof(checkPath), "%s/workshop/%s", homePath, currentWorkshop);
			Com_sprintf(checkPath, sizeof(checkPath), "%s%cworkshop%c%s", homePath, PATH_SEP, PATH_SEP, clc.currentWorkshop);

			if (Sys_FileExists(checkPath)  &&  Sys_FileIsDirectory(checkPath)) {
				Com_Printf("^6workshop %s already present\n", clc.currentWorkshop);
				clc.currentWorkshop[0] = '\0';
				downloadDir[0] = '\0';
				// don't 'return' so that the download screen doesn't popup
				// unless downloading is needed
				goto keepChecking;
			}

			Com_Printf("^6need '%s'\n", checkPath);
			//s = va("steamcmd.sh +login anonymous +workshop_download_item 282440 %s +quit", currentWorkshop);
			//s = va("steamcmd.sh +login anonymous +workshop_download_item 282440 %s +quit 2>&1", currentWorkshop);

			s = va("%s +login anonymous +workshop_download_item %d %s +quit", Sys_GetSteamCmd(), QUAKELIVE_STEAM_APP_ID, clc.currentWorkshop);
			//s = "ping google.com";
			//system("\\share\\wc\\steamcmd\\pcat.exe sdkfj");
			clc.wfp = Sys_PopenAsync(s);
			if (clc.wfp == NULL) {
				Com_Printf("^1CL_CheckWorkshopDownload: couldn't open process pipe\n");
				goto done;
			}
		}

		//FIXME testing
		// steamcmd.sh +login anonymous +workshop_download_item 282440 547252823 +quit
		//s = va("steamcmd.sh +login anonymous +workshop_download_item 282440 %s +quit", currentWorkshop);
		//r = system(s);
		//Com_Printf("system steamcmd.sh return: %d\n", r);

		//currentWorkshop[0] = '\0';

		buffer[0] = '\0';
		retp = Sys_PopenGetLine(clc.wfp, buffer, sizeof(buffer));

		if (retp == NULL) {
			if (Sys_PopenIsDone(clc.wfp)) {
				Sys_PopenClose(clc.wfp);
				free(clc.wfp);
				clc.wfp = NULL;
				//FIXME get exit code
				//Com_Printf("popen done\n");

				clc.currentWorkshop[0] = '\0';
				downloadDir[0] = '\0';
			}
		}

		if (buffer[0] != '\0') {
			Com_Printf("%s", buffer);
			char **fileList;
			int numFiles;
			int i;

			//FIXME hack to check for download location
			// Success. Downloaded item 549600167 to "/home/acano/steamcmd/x/steamapps/workshop/content/282440/549600167" (1658883 bytes)
			if (!Q_stricmpn(buffer, "Success. Downloaded item", strlen("Success. Downloaded item"))) {
				char *s;

				// goto to first quote "
				s = buffer;
				while (*s) {
					if (s[0] == '"') {
						s++;
						break;
					}
					s++;
				}

				Com_Printf("\n");
				//Com_Printf("xxxxxxxxxx buffer: '%s'\n", s);
				Q_strncpyz(downloadDir, s, sizeof(downloadDir));

				// strip last quote marker "
				s = downloadDir;
				while (*s) {
					if (s[0] == '"') {
						s[0] = '\0';
						break;
					}
					s++;
				}

				Com_Printf("^6download dir:  '%s'\n", downloadDir);
				Sys_Mkdir(checkPath);
				fileList = Sys_ListFiles(downloadDir, "", NULL, &numFiles, qfalse);
				for (i = 0;  i < numFiles;  i++) {
					Com_Printf("filelist[%d]: '%s'\n", i, fileList[i]);
					Com_sprintf(inputFile, sizeof(inputFile), "%s/%s", downloadDir, fileList[i]);
					Com_sprintf(outputFile, sizeof(outputFile), "%s/%s", checkPath, fileList[i]);
					Sys_CopyFile(inputFile, outputFile);
				}
				Sys_FreeFileList(fileList);
			}
		}

		//Sys_Sleep(1000);
	}

	if (doneParsing == qfalse) {
		return;
	}

 done:
	// all done
	clc.demoWorkshopsString = NULL;
	clc.state = CA_CONNECTED;

	// read demo messages until connected
	while ( clc.state >= CA_CONNECTED && clc.state < CA_PRIMED ) {
		//Com_Printf("reading demo messages until connected\n");
		CL_ReadDemoMessage(qfalse);
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.firstDemoFrameSkipped = qfalse;
}

//static float overf = 0.0;

/*
==================
CL_Frame

==================
*/
void CL_Frame ( int msec, double fmsec ) {
	double f;
	double blurFramesFactor;

	if ( !com_cl_running->integer ) {
		return;
	}

	Sys_DisableScreenBlanking();

	blurFramesFactor = 1.0;
	//Com_Printf("CL_Frame msec: %d  fmsec: %f  timescale: %f\n", msec, fmsec, com_timescale->value);

	if (di.seeking) {
		goto seeking;
	}

	//Com_Printf("msec %d   %d\n", msec, Sys_Milliseconds());

#ifdef USE_CURL
	if(clc.downloadCURLM) {
		CL_cURL_PerformDownload();
		// we can't process frames normally when in disconnected
		// download mode since the ui vm expects clc.state to be
		// CA_CONNECTED
		if(clc.cURLDisconnected) {
			cls.realFrametime = msec;
			cls.frametime = msec;
			cls.realtime += cls.frametime;
			cls.scaledtime += cls.frametime;
			SCR_UpdateScreen();
			S_Update();
			Con_RunConsole();
			cls.framecount++;
			return;
		}
	}
#endif

	if ( cls.cddialog ) {
		// bring up the cd error dialog if needed
		cls.cddialog = qfalse;
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NEED_CD );
	} else	if ( clc.state == CA_DISCONNECTED && !( Key_GetCatcher( ) & KEYCATCH_UI )
		&& !com_sv_running->integer && uivm ) {
		// if disconnected, bring up the menu
		S_StopAllSounds();
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
	}

	if (!msec) {
		Com_Printf("CL_Frame() !msec\n");
	}

	//Com_Printf("video: %d\n", CL_VideoRecording(&afdMain));
	// if recording an avi, lock to a fixed fps
	if ((CL_VideoRecording(&afdMain) && cl_aviFrameRate->integer && msec)  &&  !(cl_freezeDemoPauseVideoRecording->integer  &&  cl_freezeDemo->integer)) {
		//Com_Printf("yes\n");
		// save the current screen
		if ( clc.state == CA_ACTIVE || cl_forceavidemo->integer) {
			int blurFrames;
			double frameRateDivider;

			frameRateDivider = (double)cl_aviFrameRateDivider->integer;
			if (frameRateDivider < 1.0) {
				frameRateDivider = 1.0;
			}

			CL_TakeVideoFrame(&afdMain);

			// fixed time for next frame'
			blurFrames = Cvar_VariableIntegerValue("mme_blurFrames");
			if (blurFrames > 1) {
				blurFramesFactor = 1.0 / (double)blurFrames;
			}
			f = ( ((double)1000.0f / ((double)cl_aviFrameRate->value * frameRateDivider)) * (double)com_timescale->value ) * blurFramesFactor;
			//msec = (int)ceil(f);
			msec = (int)floor(f);
			//overf += ceil(f) - f;
			Overf += f - floor(f);
			if (Overf > 1.0) {
				//msec -= (int)floor(overf);
				//overf -= floor(overf);
				msec += (int)floor(Overf);
				Overf -= floor(Overf);
			}
			if (msec == 0) {
				//Com_Printf("msec too small\n");
				//msec = 1;
			}
			//Com_Printf("video msec: %lf %d  overf: %f\n", f, msec, overf);
		}
	} else if (1) {  //(com_timescale->value < 1.0) {
		//FIXME clampTime changes to msec
		// msec will always be 1
		if (!cl_freezeDemo->integer) {
			//Com_Printf("fmsec %f\n", fmsec);
			if (fmsec < 0.0) {
				fmsec = 0;
			}
			msec = (int)floor(fmsec);
			Overf += fmsec - floor(fmsec);
			if (Overf > 1.0) {
				if (com_timescale->value >= 1.0) {
					//Com_Printf("? the f..\n");
				}
				msec += (int)floor(Overf);
				Overf -= floor(Overf);
			}
		}
	} else {
		Overf = 0.0;
	}

	if( cl_autoRecordDemo->integer ) {
		if( clc.state == CA_ACTIVE && !clc.demorecording && !clc.demoplaying ) {
			// If not recording a demo, and we should be, start one
			qtime_t	now;
			char		*nowString;
			char		*p;
			char		mapName[ MAX_QPATH ];
			char		serverName[ MAX_OSPATH ];

			Com_RealTime(&now, qtrue, 0);
			nowString = va( "%04d%02d%02d%02d%02d%02d",
					1900 + now.tm_year,
					1 + now.tm_mon,
					now.tm_mday,
					now.tm_hour,
					now.tm_min,
					now.tm_sec );

			Q_strncpyz( serverName, clc.servername, MAX_OSPATH );
			// Replace the ":" in the address as it is not a valid
			// file name character
			p = strstr( serverName, ":" );
			if( p ) {
				*p = '.';
			}

			Q_strncpyz( mapName, COM_SkipPath( cl.mapname ), sizeof( cl.mapname ) );
			COM_StripExtension(mapName, mapName, sizeof(mapName));

			Cbuf_ExecuteText( EXEC_NOW,
					va( "record %s-%s-%s", nowString, serverName, mapName ) );
		}
		else if( clc.state != CA_ACTIVE && clc.demorecording ) {
			// Recording, but not CA_ACTIVE, so stop recording
			CL_StopRecord_f( );
		}
	}


	// save the msec before checking pause
	cls.realFrametime = msec;

	if (clc.demoplaying  &&  cl_freezeDemo->integer) {
		msec = 0;
	}

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;
	cls.scaledtime += cls.frametime;

	//Com_Printf("^5cls.realtime %d\n", cls.realtime);

	if ( cl_timegraph->integer ) {
		SCR_DebugGraph ( cls.realFrametime * 0.25 );
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	CL_CheckWorkshopDownload();

 seeking:
	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update audio
	S_Update();

#ifdef USE_VOIP
	CL_CaptureVoip();
#endif

#ifdef USE_MUMBLE
	CL_UpdateMumble();
#endif

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
	di.seeking = qfalse;
}


//============================================================================

/*
================
CL_RefPrintf

DLL glue
================
*/
static __attribute__ ((format (printf, 2, 3))) void QDECL CL_RefPrintf( int print_level, const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( print_level == PRINT_ALL ) {
		Com_Printf ("%s", msg);
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf (S_COLOR_YELLOW "%s", msg);		// yellow
	} else if ( print_level == PRINT_ERROR ) {
		Com_Printf (S_COLOR_RED "%s", msg);                     // red
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf (S_COLOR_RED "%s", msg);            // red - developer only
	}
}



/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef( void ) {
	if ( re.Shutdown ) {
		re.Shutdown( qtrue );
	}

	Com_Memset( &re, 0, sizeof( re ) );

#ifdef USE_RENDERER_DLOPEN
	if ( rendererLib ) {
		Sys_UnloadLibrary( rendererLib );
		rendererLib = NULL;
	}
#endif
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer ( void ) {
	// this sets up the renderer and calls R_Init
	re.BeginRegistration( &cls.glconfig );

	// load character sets
	cls.charSetShader = re.RegisterShader( "gfx/2d/bigchars" );
	if (!cls.charSetShader) {
		cls.charSetShader = re.RegisterShader("gfx/wc/openarenachars");
	}
	cls.whiteShader = re.RegisterShader( "white" );
	if (!cls.whiteShader) {
		cls.whiteShader = re.RegisterShader("wcwhite");
	}
	cls.consoleShader = re.RegisterShader("wc/console");
	g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	g_consoleField.widthInChars = g_console_field_width;

	re.RegisterFont("q3big", 16, &cls.consoleFont);
	cl.draw = qtrue;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers( qboolean rendererOnly ) {
	if (!com_cl_running) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.rendererStarted ) {
		cls.rendererStarted = qtrue;
		CL_InitRenderer();
	}

	if ( rendererOnly ) {
		return;
	}

	if ( !cls.soundStarted ) {
		cls.soundStarted = qtrue;
		S_Init();
	}

	if ( !cls.soundRegistered ) {
		cls.soundRegistered = qtrue;
		S_BeginRegistration();
	}

	if( com_dedicated->integer ) {
		return;
	}

	if ( !cls.uiStarted ) {
		cls.uiStarted = qtrue;
		CL_InitUI();
	}
}

/*
============
CL_RefMalloc
============
*/
void *CL_RefMalloc( int size ) {
	return Z_TagMalloc( size, TAG_RENDERER );
}

// for renderer shader/cinematic times
int CL_ScaledMilliseconds(void) {
	// Use cls.scaledtime instead of Sys_Milliseconds()*com_timescale->value
	// since fake frame times are generated while recording video.
	// cls.scaledtime is also adjusted by timescale.

	return cls.scaledtime;
}

/*
============
CL_InitRef
============
*/
void CL_InitRef ( void ) {
	refimport_t	ri;
	refexport_t	*ret;
#ifdef USE_RENDERER_DLOPEN
	GetRefAPI_t	GetRefAPI;
	char		dllName[MAX_OSPATH];
#endif
	vec4_t color;

	Com_Printf( "----- Initializing Renderer ----\n" );

#ifdef USE_RENDERER_DLOPEN
	cl_renderer = Cvar_Get("cl_renderer", "opengl1", CVAR_ARCHIVE | CVAR_LATCH);

	Com_sprintf(dllName, sizeof(dllName), "renderer_%s_" ARCH_STRING DLL_EXT, cl_renderer->string);

	if(!(rendererLib = Sys_LoadDll(dllName, qfalse)) && strcmp(cl_renderer->string, cl_renderer->resetString))
	{
		Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());
		Cvar_ForceReset("cl_renderer");

		Com_sprintf(dllName, sizeof(dllName), "renderer_opengl1_" ARCH_STRING DLL_EXT);
		rendererLib = Sys_LoadDll(dllName, qfalse);
	}

	if(!rendererLib)
	{
		Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());
		Com_Error(ERR_FATAL, "Failed to load renderer");
	}

	GetRefAPI = Sys_LoadFunction(rendererLib, "GetRefAPI");
	if(!GetRefAPI)
	{
		Com_Error(ERR_FATAL, "Can't load symbol GetRefAPI: '%s'",  Sys_LibraryError());
	}
#endif

	ri.sse2_supported = com_sse2_supported;
	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Printf = CL_RefPrintf;
	ri.Error = Com_Error;
	ri.ScaledMilliseconds = CL_ScaledMilliseconds;
	ri.RealMilliseconds = Sys_Milliseconds;
	ri.Malloc = CL_RefMalloc;
	ri.Free = Z_Free;
#ifdef HUNK_DEBUG
	ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	ri.Hunk_Alloc = Hunk_Alloc;
#endif
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;

	ri.CM_ClusterPVS = CM_ClusterPVS;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;

	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_WriteFile = FS_WriteFile;
	ri.FS_Write = FS_Write;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_FileIsInPAK = FS_FileIsInPAK;
	ri.FS_FileExists = FS_FileExists;
	ri.FS_FindSystemFile = FS_FindSystemFile;
	ri.FS_FCloseFile = FS_FCloseFile;
	ri.FS_FOpenFileWrite = FS_FOpenFileWrite;

	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Cvar_ForceReset = Cvar_ForceReset;
	ri.Cvar_CheckRange = Cvar_CheckRange;
	ri.Cvar_FindVar = Cvar_FindVar;
	ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
	ri.Cvar_VariableValue = Cvar_VariableValue;
	ri.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;
	ri.Cvar_SetDescription = Cvar_SetDescription;

	// cinematic stuff

	ri.CIN_UploadCinematic = CIN_UploadCinematic;
	ri.CIN_PlayCinematic = CIN_PlayCinematic;
	ri.CIN_RunCinematic = CIN_RunCinematic;

	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	ri.IN_Init = IN_Init;
	ri.IN_Shutdown = IN_Shutdown;
	ri.IN_Restart = IN_Restart;

	ri.ftol = Q_ftol;

	ri.Sys_SetEnv = Sys_SetEnv;
	ri.Sys_GLimpSafeInit = Sys_GLimpSafeInit;
	ri.Sys_GLimpInit = Sys_GLimpInit;
	ri.Sys_LowPhysicalMemory = Sys_LowPhysicalMemory;

	// video recording stuff
	ri.SplitVideo = &SplitVideo;

	ri.afdMain = &afdMain;
	ri.afdLeft = &afdLeft;
	ri.afdRight = &afdRight;

	ri.afdDepth = &afdDepth;
	ri.afdDepthLeft = &afdDepthLeft;
	ri.afdDepthRight = &afdDepthRight;

	ri.Video_DepthBuffer = &Video_DepthBuffer;
	ri.ExtraVideoBuffer = &ExtraVideoBuffer;

	// misc
	ri.MapNames = MapNames;

	ret = GetRefAPI( REF_API_VERSION, &ri );

#if defined __USEA3D && defined __A3D_GEOM
	hA3Dg_ExportRenderGeom (ret);
#endif

	Com_Printf( "-------------------------------\n");

	if ( !ret ) {
		Com_Error (ERR_FATAL, "Couldn't initialize refresh" );
	}

	re = *ret;

	re.SetPathLines(NULL, NULL, NULL, NULL, color);

	// unpause so the cgame definitely gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );
}


//===========================================================================================


void CL_SetModel_f( void ) {
	char	*arg;
	char	name[256];  //FIXME why 256?

	arg = Cmd_Argv( 1 );
	if (arg[0]) {
		Cvar_Set( "model", arg );
	} else {
		Cvar_VariableStringBuffer( "model", name, sizeof(name) );
		Com_Printf("model is set to %s\n", name);
	}
}

void CL_SetHeadModel_f( void ) {
	char	*arg;
	char	name[256];  //FIXME why 256?

	arg = Cmd_Argv( 1 );
	if (arg[0]) {
		Cvar_Set( "headmodel", arg );
	} else {
		Cvar_VariableStringBuffer( "headmodel", name, sizeof(name) );
		Com_Printf("headmodel is set to %s\n", name);
	}
}


//===========================================================================================


extern int s_soundtime;

/*
===============
CL_Video_f

video
video [filename]
===============
*/
void CL_Video_f( void )
{
  char  filename[ MAX_OSPATH ];
  int   i;  //, last;
  qboolean avi;
  qboolean wav;
  qboolean tga;
  qboolean jpg;
  qboolean png;
  qboolean noSoundAvi;
  qboolean pipe;

  if (!clc.demoplaying) {  //  ||  clc.state == CA_CONNECTING) {
	  Com_Printf( "^1The video command can only be used when playing back demos\n" );
	  return;
  }

  avi = qfalse;
  wav = qfalse;
  tga = qfalse;
  jpg = qfalse;
  png = qfalse;
  noSoundAvi = qfalse;
  pipe = qfalse;
  filename[0] = '\0';
  SplitVideo = qfalse;

  for (i = 1;  i < Cmd_Argc();  i++) {
	  if (!Q_stricmp(Cmd_Argv(i), "avi")) {
		  avi = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "avins")) {
		  noSoundAvi = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "wav")) {
		  wav = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "tga")) {
		  tga = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "jpg")) {
		  jpg = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "jpeg")) {
		  jpg = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "png")) {
		  png = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "split")) {
		  SplitVideo = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "pipe")) {
		  pipe = qtrue;
	  } else if (!Q_stricmp(Cmd_Argv(i), "name")) {
		  if (!Q_stricmp(Cmd_Argv(i + 1), ":demoname")) {
			  char dnameBuffer[MAX_OSPATH];

			  COM_StripExtension(Cvar_VariableString("cl_demoFileBaseName"), dnameBuffer, MAX_OSPATH);
			  Q_strncpyz(filename, dnameBuffer, MAX_OSPATH);
		  } else {
			  Q_strncpyz(filename, Cmd_Argv(i + 1), MAX_OSPATH);
		  }
		  i++;
	  }
  }

  if (!avi  &&  !tga  &&  !jpg  &&  !png) {
	  avi = qtrue;
  }

  if (avi  &&  (tga | jpg | png)) {
	  Com_Printf("^1can't record video and screenshots at the same time\n");
	  return;
  }

#if 0
  if (Cmd_Argc() < 2) {
	  avi = qtrue;
  }
#endif

  if (avi  ||  wav) {
	  if (!Q_stricmp(s_backend->string, "OpenAL")) {
		  Com_Printf("^1can't record sound using OpenAL\n");
		  return;
	  }
	  if (dma.samplebits != 16) {
		  Com_Printf("^1can't record sound if audio sample bits not equal to 16 (see s_sdlBits)\n");
		  return;
	  }
	  if (dma.channels != 2) {
		  Com_Printf("^1can't record sound if audio channels not equal to 2 (see s_sdlChannels)\n");
		  return;
	  }
  }

  if (Cvar_VariableIntegerValue("mme_saveDepth")) {
	  Video_DepthBuffer = malloc(cls.glconfig.vidWidth * cls.glconfig.vidHeight * sizeof(GLfloat) + 18);
	  if (!Video_DepthBuffer) {
		  Com_Error(ERR_DROP, "Couldn't allocate memory for depth buffer");
	  }

	  if (SplitVideo  &&  Cvar_VariableIntegerValue("r_anaglyphMode") == 19) {
		  CL_OpenAVIForWriting(&afdDepthLeft, filename, qfalse, avi, avi ? qtrue : noSoundAvi, wav, tga, jpg, png, pipe, qtrue, qtrue, qtrue);
		  CL_OpenAVIForWriting(&afdDepthRight, filename, qfalse, avi, avi ? qtrue : noSoundAvi, wav, tga, jpg, png, pipe, qtrue, qtrue, qfalse);
	  } else {
		  CL_OpenAVIForWriting(&afdDepth, filename, qfalse, avi, avi ? qtrue : noSoundAvi, wav, tga, jpg, png, pipe, qtrue, qfalse, qfalse);
	  }
  }

  if (SplitVideo) {
	  ExtraVideoBuffer = malloc(18 + cls.glconfig.vidWidth * cls.glconfig.vidHeight * 4);
	  if (!ExtraVideoBuffer) {
		  Com_Error(ERR_DROP, "Couldn't allocate memory for extra video buffer");
	  }
	  CL_OpenAVIForWriting(&afdLeft, filename, qfalse, avi, avi ? qtrue : noSoundAvi, wav, tga, jpg, png, pipe, qfalse, qtrue, qtrue);
	  CL_OpenAVIForWriting(&afdRight, filename, qfalse, avi, avi ? qtrue : noSoundAvi, wav, tga, jpg, png, pipe, qfalse, qtrue, qfalse);
  }
  //Com_Printf("^2video cl_aviFrameRate %d\n", cl_aviFrameRate->integer);
  CL_OpenAVIForWriting(&afdMain, filename, qfalse, avi, noSoundAvi, wav, tga, jpg, png, pipe, qfalse, qfalse, qfalse);

  if (CL_VideoRecording(&afdMain)) {
	  s_soundtime = s_paintedtime;
  }
}

/*
===============
CL_StopVideo_f
===============
*/
void CL_StopVideo_f( void )
{
	if (Video_DepthBuffer) {
		CL_CloseAVI(&afdDepth, qfalse);
		CL_CloseAVI(&afdDepthLeft, qfalse);
		CL_CloseAVI(&afdDepthRight, qfalse);
		free(Video_DepthBuffer);
		Video_DepthBuffer = NULL;
	}

	if (SplitVideo) {
		CL_CloseAVI(&afdLeft, qfalse);
		CL_CloseAVI(&afdRight, qfalse);
		free(ExtraVideoBuffer);
		ExtraVideoBuffer = NULL;
	}

	CL_CloseAVI(&afdMain, qfalse);
}

static void fast_forward_demo (double wantedTime)
{
	int loopCount;
	int stream;

	if ( clc.state < CA_CONNECTED ) {
		return;
	}

	if (wantedTime >= di.lastServerTime) {
		return;
	}

	//Com_Printf("fast_forward %f\n", wantedTime);

	if (wantedTime >= (double)cl.serverTime  &&  wantedTime < (double)cl.snap.serverTime) {
		//Com_Printf("stay\n");
		cl.serverTime = floor(wantedTime);
		cls.realtime = floor(wantedTime);
		Overf = wantedTime - floor(wantedTime);
		cl.serverTimeDelta = 0;
		return;
	}

	if (wantedTime < (double)cl.snap.serverTime) {
		Com_Printf("wtf..... %f < %d\n", wantedTime, cl.snap.serverTime);
		return;
	}

	if (clc.demorecording) {
		CL_StopRecord_f();
	}

	if (wantedTime > (double)di.lastServerTime) {
		Com_Printf("would seek past end of demo (%f > %d)\n", wantedTime, di.lastServerTime);
		return;
	}

	if (Cvar_VariableIntegerValue("debug_seek")) {
		Com_Printf("fastforwarding from %f to %f\n", (double)cl.serverTime + Overf, wantedTime);
	}

	//Com_Printf("ff  %d\n", clc.lastExecutedServerCommand);

	loopCount = 0;
	while ((double)cl.snap.serverTime <= wantedTime) {  // (wantedTime + (double)di.serverFrameTime)) {
		CL_ReadDemoMessage(qtrue);
		while (clc.lastExecutedServerCommand < clc.serverCommandSequence) {
			if (clc.lastExecutedServerCommand + 1 <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS) {
				//if (cl.snap.serverTime == di.firstServerTime  ||  cl.snap.serverTime == di.secondServerTime) {
				if (cl.snap.serverTime <= di.firstServerTime) {
					clc.lastExecutedServerCommand = clc.serverCommandSequence - 1;
					Com_Printf("%s setting clc.lastExecutedServerCommand %d (%d)\n", __FUNCTION__, clc.lastExecutedServerCommand, loopCount);
				} else {
					Com_Printf("FIXME %s  (%d) + 1 <= (%d) - MAX_RELIABLE_COMMANDS (%d %d)\n", __FUNCTION__, clc.lastExecutedServerCommand, clc.serverCommandSequence, cl.snap.serverTime, di.firstServerTime);
					break;
				}
			}
			CL_GetServerCommand(clc.lastExecutedServerCommand + 1);
#if 0
			if (!CL_GetServerCommand(clc.lastExecutedServerCommand + 1)) {
				//Com_Printf("%s couldn't get server command %d\n", __FUNCTION__, clc.lastExecutedServerCommand + 1);
				break;
			}
#endif
		}
		loopCount++;
	}

	if (Cvar_VariableIntegerValue("debug_seek")) {
		Com_Printf("fastforward:  read %d demo messages, cl.snap.serverTime %d, wantedTime %f\n", loopCount, cl.snap.serverTime, wantedTime);
	}

	// reset sound streams that might have had data added in CL_ReadDemoMessage() above
	//FIXME better to check if demo is seeking before even adding stream data
	for (stream = 0;  stream < MAX_RAW_STREAMS;  stream++) {
		s_rawend[stream] = 0;
	}

	cl.serverTime = floor(wantedTime);
	cls.realtime = floor(wantedTime);
	Overf = wantedTime - floor(wantedTime);
	cl.serverTimeDelta = 0;
	VM_Call(cgvm, CG_TIME_CHANGE, cl.serverTime, (int)(Overf * SUBTIME_RESOLUTION));
	di.seeking = qtrue;
	di.firstNonDeltaMessageNumWritten = -1;
}

static void rewind_demo (double wantedTime)
{
	int i;
	rewindBackups_t *rb;
	int j;
	int scaledtimeOrig;
	clientConnection_t clcOrig;

	if (wantedTime < (double)di.firstServerTime) {
		wantedTime = di.firstServerTime;
	}

	if (!rewindBackups[0].valid  ||  di.snapCount == 0) {
		//Com_Printf("wtf di.snapCount %d\n", di.snapCount);
		fast_forward_demo(wantedTime);
		return;
	}

	rb = NULL;
	for (i = di.snapCount - 1;  i >= 0;  i--) {
		rb = &rewindBackups[i];
		// go back a second before wanted time in order to have snapshot backups available for screen matching
		//if (rb->cl.snap.serverTime < (currentTime - di.rewindRequestTime) - 1000) {
		if ((double)rb->cl.snap.serverTime < wantedTime - 1000.0) {
			//Com_Printf("snapCount index: %d  %d  from %f  want %f\n", i, rb->cl.snap.serverTime, currentTime, di.rewindRequestTime);
			break;
		}
	}
	if (rb == NULL  ||  i < 0) {
		if (rb == NULL) {
			Com_Printf("FIXME rewind couldn't find valid snap  rb:%p  i:%d  rb serverTime %d   wanted %f\n", rb, i, rewindBackups[0].cl.snap.serverTime, wantedTime);
		}
		rb = &rewindBackups[0];
		i = 0;
	}

	//Com_Printf("seeking to index %d %d   cl.serverTime:%d  cl.snap.serverTime:%d, new clc.lastExecutedServercommand %d  clc.serverCommandSequence %d\n", i, rb->seekPoint, cl.serverTime, cl.snap.serverTime, rb->clc.lastExecutedServerCommand, rb->clc.serverCommandSequence);
	FS_Seek(clc.demoReadFile, rb->seekPoint, FS_SEEK_SET);
	for (j = 1;  j < di.numDemoFiles;  j++) {
		demoFile_t *df;

		df = &di.demoFiles[j];
		if (!df->valid) {
			continue;
		}
		if (rb->demoSeekPoints[j] < 0) {
			continue;
		}
		FS_Seek(df->f, rb->demoSeekPoints[j], FS_SEEK_SET);
		df->serverTime = 0;
	}
	di.numSnaps = rb->numSnaps;
	di.snapCount = i + 1;  //FIXME hack

	//FIXME check if demo has voip
	memcpy(&clcOrig, &clc, sizeof(clientConnection_t));

	memcpy(&cl, &rb->cl, sizeof(clientActive_t));
	memcpy(&clc, &rb->clc, sizeof(clientConnection_t));

	// voip stuff
	//FIXME check if demo even has voip
	clc.voipEnabled = clcOrig.voipEnabled;
	clc.speexInitialized = clcOrig.speexInitialized;
	clc.speexFrameSize = clcOrig.speexFrameSize;
	clc.speexSampleRate = clcOrig.speexSampleRate;
	clc.voipCodecInitialized = clcOrig.voipCodecInitialized;
	memcpy(&clc.speexDecoderBits, &clcOrig.speexDecoderBits, sizeof(SpeexBits) * MAX_CLIENTS);
	memcpy(&clc.speexDecoder, &clcOrig.speexDecoder, sizeof(void *) * MAX_CLIENTS);
	memcpy(&clc.opusDecoder, &clcOrig.opusDecoder, sizeof(OpusDecoder *) * MAX_CLIENTS);
	// incoming ... skip
	memcpy(&clc.voipGain, &clcOrig.voipGain, sizeof(float) * MAX_CLIENTS);
	memcpy(&clc.voipIgnore, &clcOrig.voipIgnore, sizeof(qboolean) * MAX_CLIENTS);
	clc.voipMuteAll = clcOrig.voipMuteAll;

	scaledtimeOrig = cls.scaledtime;
	memcpy(&cls, &rb->cls, sizeof(clientStatic_t));
	cls.scaledtime = scaledtimeOrig;

	Overf = 0;
	//Com_Printf("%s clc.state %d\n", __FUNCTION__, clc.state);
	//FIXME hack
	clc.state = CA_ACTIVE;
	fast_forward_demo(wantedTime);
}

void CL_Rewind_f (void)
{
	double t;
	double wantedTime;

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: rewind <time in seconds>\n");
		return;
	}

	if (!Q_isdigit(Cmd_Argv(1)[0])) {
		t = Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	} else {
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	if (!(clc.demoplaying  ||  clc.state == CA_CINEMATIC)) {
		Com_Printf("not playing demo or cinematic, can't rewind\n");
		return;
	}

	wantedTime = (double)cl.serverTime + Overf - t;

	// cinematics first since times can be changed with demo rewind/fastforward
	if (clc.demoplaying) {
		if (wantedTime < (double)di.firstServerTime) {
			// don't seek cinematic past demo start
			t -= (double)di.firstServerTime - wantedTime;
		}
	}

	// don't call CIN_SeekCinematic() from fast_forward_demo() or rewind_demo() since those functions call each other
	CIN_SeekCinematic(-t);

	if (clc.demoplaying) {
		rewind_demo(wantedTime);
	}
}


void CL_FastForward_f (void)
{
	double t;
	double wantedTime;

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: fastforward <time in seconds>\n");
		return;
	}

	if (!Q_isdigit(Cmd_Argv(1)[0])) {
		t = Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	} else {
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	if (cl.snap.serverTime) {
		//wantedTime = (double)cl.snap.serverTime + t;
		wantedTime = (double)cl.serverTime + Overf + t;
	} else {
		wantedTime = (double)di.firstServerTime + t;
	}

	if (!(clc.demoplaying  ||  clc.state == CA_CINEMATIC)) {
		Com_Printf("not playing demo or cinematic, can't fast forward\n");
		return;
	}

	// cinematics first since times can be changed with demo rewind/fastforward
	if (clc.demoplaying) {
		if (wantedTime >= (double)di.lastServerTime) {
			// don't seek cinematic past demo end
			t = 0;
		}
	}

	// don't call CIN_SeekCinematic() from fast_forward_demo() or rewind_demo() since those functions call each other
	CIN_SeekCinematic(t);

	if (clc.demoplaying) {
		fast_forward_demo(wantedTime);
	}
}

static void demo_seek (double ms, int exactServerTime)  // server time in milliseconds
{
	double wantedTime;
	double cinSeekTime;

	if (!clc.demoplaying) {
		Com_Printf("not playing demo can't seek\n");
		return;
	}

	wantedTime = ms;

	if (exactServerTime > 0) {
		wantedTime = exactServerTime;
	}

	cinSeekTime = wantedTime - (double)cl.serverTime;

	if (Cvar_VariableIntegerValue("debug_seek")) {
		Com_Printf("demo_seek(): seek want %f\n", wantedTime);
	}

	if (wantedTime > (double)cl.serverTime + Overf) {
		// cinematic before demo seek since times can change
		if (wantedTime >= (double)di.lastServerTime) {
			// don't seek cinematic past demo end
			cinSeekTime = 0;
		}
		CIN_SeekCinematic(cinSeekTime);

		fast_forward_demo(wantedTime);
	} else {
		// cinematic before demo seek since times can change
		if (wantedTime < (double)di.firstServerTime) {
			// don't seek cinematic past demo start
			cinSeekTime -= (double)di.firstServerTime - wantedTime;
		}
		CIN_SeekCinematic(cinSeekTime);

		rewind_demo(wantedTime);
	}
}

void CL_SeekServerTime_f (void)
{
	double f;

	if (!clc.demoplaying) {
		Com_Printf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: seekservertime <time in milliseconds>\n");
		return;
	}

	f = atof(Cmd_Argv(1));
	if (Cvar_VariableIntegerValue("debug_seek")) {
		Com_Printf("seekservertime():  %f\n", f);
	}
	demo_seek(f, -1);
}

void CL_Seek_f (void)
{
	double t;

	if (!clc.demoplaying) {
		Com_Printf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: seek <time in seconds>\n");
		return;
	}

	if (!Q_isdigit(Cmd_Argv(1)[0])) {
		t = Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	} else {
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	demo_seek((double)di.firstServerTime + t, -1);
}

void CL_SeekEnd_f (void)
{
	double t;

	if (!clc.demoplaying) {
		Com_Printf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: seek <time in seconds>\n");
		return;
	}

	if (!Q_isdigit(Cmd_Argv(1)[0])) {
		t = Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	} else {
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	demo_seek((double)di.lastServerTime - t, -1);
}

void CL_SeekNext_f (void)
{
	snapshot_t snapshot;
	qboolean r;
	int i;

#if 0
	r = CL_PeekSnapshot(cl.snap.messageNum + 1, &snapshot);
	if (!r) {
		Com_Printf("seeknext() couldn't get next snapshot\n");
		return;
	}

	Com_Printf("snapshot.serverTime %d  cl.snap.serverTime %d\n", snapshot.serverTime, cl.snap.serverTime);
#endif

	//Com_Printf("seeknext()  cl.serverTime %d  cl.snap.serverTime %d\n", cl.serverTime, cl.snap.serverTime);

	if (cl.snap.serverTime == di.lastServerTime) {
		Com_Printf("seeknext() at last snap\n");
		return;
	}

	// takes into account offline demos with snaps with same server time

	if (cl.serverTime < cl.snap.serverTime) {
		demo_seek(0, cl.snap.serverTime);
		return;
	}

	i = 1;
	while (1) {
		r = CL_PeekSnapshot(cl.snap.messageNum + i, &snapshot);
		if (!r) {
			Com_Printf("seeknext() couldn't get next snapshot\n");
			return;
		}
		//if (snapshot.serverTime > cl.snap.serverTime) {
		if (snapshot.serverTime > cl.serverTime) {
			break;
		}
		i++;
	}

	demo_seek(0, snapshot.serverTime);
}

void CL_SeekPrev_f (void)
{
	clSnapshot_t *clSnap;
	int i;

	if (cl.snap.serverTime == di.firstServerTime) {
		Com_Printf("seekprev() at first snap\n");
		return;
	}

	//Com_Printf("seekprev()  cl.serverTime %d  cl.snap.serverTime %d\n", cl.serverTime, cl.snap.serverTime);
	//clSnap = &cl.snapshots[0][(cl.snap.messageNum - 1) & PACKET_MASK];

	// takes into account offline demos with snapshots having same server time
	i = 0;
	while (1) {
		clSnap = &cl.snapshots[0][(cl.snap.messageNum - i) & PACKET_MASK];
		//if (clSnap->serverTime < cl.snap.serverTime) {
		if (clSnap->serverTime < cl.serverTime) {
			break;
		}
		i++;
	}

	demo_seek(0, clSnap->serverTime);
}

void CL_Pause_f (void)
{
	int val;

	val = Cvar_VariableIntegerValue("cl_freezeDemo");
	Cvar_SetValue("cl_freezeDemo", !val);
}

#if 0
static void CL_ToggleMouseGrab_f (void)
{
	cls.dontGrabMouse = !cls.dontGrabMouse;
}
#endif

/*
===============
CL_GenerateQKey

test to see if a valid QKEY_FILE exists.  If one does not, try to generate
it by filling it with 2048 bytes of random data.
===============
*/
static void CL_GenerateQKey(void)
{
	int len = 0;
	unsigned char buff[ QKEY_SIZE ];
	fileHandle_t f;

	len = FS_SV_FOpenFileRead( QKEY_FILE, &f );
	FS_FCloseFile( f );
	if( len == QKEY_SIZE ) {
		Com_Printf( "QKEY found.\n" );
		return;
	}
	else {
		if( len > 0 ) {
			Com_Printf( "QKEY file size != %d, regenerating\n",
				QKEY_SIZE );
		}

		Com_Printf( "QKEY building random string\n" );
		Com_RandomBytes( buff, sizeof(buff) );

		f = FS_SV_FOpenFileWrite( QKEY_FILE );
		if( !f ) {
			Com_Printf( "QKEY could not open %s for write\n",
				QKEY_FILE );
			return;
		}
		FS_Write( buff, sizeof(buff), f );
		FS_FCloseFile( f );
		Com_Printf( "QKEY generated\n" );
	}
}

void CL_Sayto_f( void ) {
	char		*rawname;
	char		name[MAX_NAME_LENGTH];
	char		cleanName[MAX_NAME_LENGTH];
	const char	*info;
	int			count;
	int			i;
	int			clientNum;
	char		*p;

	if ( Cmd_Argc() < 3 ) {
		Com_Printf ("sayto <player name> <text>\n");
		return;
	}

	rawname = Cmd_Argv(1);

	Com_FieldStringToPlayerName( name, MAX_NAME_LENGTH, rawname );

	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

	clientNum = -1;
	for( i = 0; i < count; i++ ) {

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+i];
		Q_strncpyz( cleanName, Info_ValueForKey( info, "n" ), sizeof(cleanName) );
		Q_CleanStr( cleanName );

		if ( !Q_stricmp( cleanName, name ) ) {
			clientNum = i;
			break;
		}
	}
	if( clientNum <= -1 )
	{
		Com_Printf ("No such player name: %s.\n", name);
		return;
	}

	p = Cmd_ArgsFrom(2);

	if ( *p == '"' ) {
		p++;
		p[strlen(p)-1] = 0;
	}

	CL_AddReliableCommand(va("tell %i \"%s\"", clientNum, p ), qfalse);
}

static void CL_PrintDataDir_f (void)
{
	Com_Printf("homepath: '%s'\n", Sys_DefaultHomePath());
	Com_Printf("qlpath: '%s'\n", Sys_QuakeLiveDir());
}

static void CL_Stall_f (void)
{
	Sys_Sleep(1000 * 5);
}

static void CL_SetColorTable_f (void)
{
	int n;
	float r, g, b, a;

	if (Cmd_Argc() < 6) {
		Com_Printf("usage: /setcolortable <index number> r g b a\n");
		Com_Printf("       0 black, 1 red, 2 green, 3 yellow, 4 blue, 5 cyan, 6 magenta, 7 white, 8 light grey, 9, medium grey, 10 dark grey\n");
		Com_Printf("       r g b a  from 0.0 to 1.0\n");
		return;
	}

	// 0 black, 1 red, 2 green, 3 yellow, 4 blue, 5 cyan, 6 magenta, 7 white, 8 light grey, 9, medium grey, 10 dark grey

	n = atoi(Cmd_Argv(1));
	if (n > 10) {
		Com_Printf("invalid index\n");
		return;
	}

	r = atof(Cmd_Argv(2));
	g = atof(Cmd_Argv(3));
	b = atof(Cmd_Argv(4));
	a = atof(Cmd_Argv(5));

	Q_SetColorTable(n, r, g, b, a);
	if (cgvm) {
		VM_Call(cgvm, CG_COLOR_TABLE_CHANGE, n, (int)(255.0 * r), (int)(255.0 * g), (int)(255.0 * b), (int)(255.0 * a));
	}
	if (uivm) {
		VM_Call(uivm, UI_COLOR_TABLE_CHANGE, n, (int)(255.0 * r), (int)(255.0 * g), (int)(255.0 * b), (int)(255.0 * a));
	}
}

static void CL_Pov_f (void)
{
	int pov;
	char *arg;
	int i;

	if (Cmd_Argc() < 2) {
		Com_Printf("usage: pov <demo number, or 'next'>\n");
		return;
	}
	if (!clc.demoplaying) {
		Com_Printf("not playing back demos\n");
		return;
	}

	arg = Cmd_Argv(1);
	if (!Q_stricmp(arg, "next")) {
		int currentPov;

		if (di.numDemoFiles < 2) {
			Com_Printf("only one demo point of view available\n");
			return;
		}

		currentPov = di.pov;
		i = di.pov;
		while (1) {
			demoFile_t *df;

			if (i >= di.numDemoFiles) {
				i = 0;
			} else {
				i++;
			}

			if (i == currentPov) {
				Com_Printf("CL_Pov_f() FIXME pov cycled\n");
				return;
			}

			df = &di.demoFiles[i];
			if (df->valid) {
				di.pov = i;
				break;
			}
		}

		return;
	} else {
		pov = atoi(arg);
		if (pov >= di.numDemoFiles) {
			Com_Printf("only %d demo point of views available\n", di.numDemoFiles);
			return;
		}
		if (!di.demoFiles[pov].valid) {
			Com_Printf("invalid demo pov\n");
			return;
		}

		di.pov = pov;
	}
}

/*
====================
CL_Init
====================
*/
void CL_Init ( void ) {
	Com_Printf( "----- Client Initialization -----\n" );

	//Com_Printf("%f mb\n", (float)sizeof(rewindBackups_t) / 1024.0 / 1024.0);
	//Com_Printf("%f mb\n", (float)sizeof(cl.entityBaselines) / 1024.0 / 1024.0);
	Con_Init ();

	if(!com_fullyInitialized)
	{
		CL_ClearState();
		clc.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED
		cl_oldGameSet = qfalse;
	}

	cls.realtime = 0;
	cls.scaledtime = 0;

	CL_InitInput ();

	//
	// register our variables
	//
	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );
#ifdef UPDATE_SERVER_NAME
	cl_motd = Cvar_Get ("cl_motd", "1", 0);
#endif

	cl_timeout = Cvar_Get ("cl_timeout", "200", 0);

	cl_timeNudge = Cvar_Get ("cl_timeNudge", "0", CVAR_TEMP );
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
	cl_showSend = Cvar_Get ("cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get ("cl_showTimeDelta", "0", CVAR_TEMP );
	cl_freezeDemo = Cvar_Get ("cl_freezeDemo", "0", CVAR_TEMP );
	rcon_client_password = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	cl_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );

	cl_timedemo = Cvar_Get ("timedemo", "0", 0);
	cl_timedemoLog = Cvar_Get ("cl_timedemoLog", "", CVAR_ARCHIVE);
	cl_autoRecordDemo = Cvar_Get ("cl_autoRecordDemo", "0", CVAR_ARCHIVE);
	cl_aviFrameRate = Cvar_Get ("cl_aviFrameRate", "50", CVAR_ARCHIVE);
	cl_aviFrameRateDivider = Cvar_Get("cl_aviFrameRateDivider", "1", CVAR_ARCHIVE);
	cl_aviCodec = Cvar_Get ("cl_aviCodec", "uncompressed", CVAR_ARCHIVE);
	cl_aviAllowLargeFiles = Cvar_Get("cl_aviAllowLargeFiles", "1", CVAR_ARCHIVE);
	cl_aviFetchMode = Cvar_Get("cl_aviFetchMode", "GL_RGB", CVAR_ARCHIVE);
	cl_aviExtension = Cvar_Get("cl_aviExtension", "avi", CVAR_ARCHIVE);
	cl_aviPipeCommand = Cvar_Get("cl_aviPipeCommand", "-threads 0 -c:a aac -c:v libx264 -preset ultrafast -y -pix_fmt yuv420p -crf 19", CVAR_ARCHIVE);
	cl_aviPipeExtension = Cvar_Get("cl_aviPipeExtension", "mkv", CVAR_ARCHIVE);
	cl_aviNoAudioHWOutput = Cvar_Get("cl_aviNoAudioHWOutput", "1", CVAR_ARCHIVE);
	cl_aviPrimeAudioRate = Cvar_Get("cl_aviPrimeAudioRate", "1", CVAR_ARCHIVE);
	cl_aviAudioWaitForVideoFrame = Cvar_Get("cl_aviAudioWaitForVideoFrame", "1", CVAR_ARCHIVE);

	cl_freezeDemoPauseVideoRecording = Cvar_Get("cl_freezeDemoPauseVideoRecording", "0", CVAR_ARCHIVE);
	cl_freezeDemoPauseMusic = Cvar_Get("cl_freezeDemoPauseMusic", "1", CVAR_ARCHIVE);
	cl_forceavidemo = Cvar_Get ("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get ("rconAddress", "", 0);

	cl_yawspeed = Cvar_Get ("cl_yawspeed", "140", CVAR_ARCHIVE);
	cl_pitchspeed = Cvar_Get ("cl_pitchspeed", "140", CVAR_ARCHIVE);
	cl_anglespeedkey = Cvar_Get ("cl_anglespeedkey", "1.5", 0);

	cl_maxpackets = Cvar_Get ("cl_maxpackets", "30", CVAR_ARCHIVE );
	cl_packetdup = Cvar_Get ("cl_packetdup", "1", CVAR_ARCHIVE );

	cl_run = Cvar_Get ("cl_run", "1", CVAR_ARCHIVE);
	cl_sensitivity = Cvar_Get ("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel = Cvar_Get ("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_freelook = Cvar_Get( "cl_freelook", "1", CVAR_ARCHIVE );

	// 0: legacy mouse acceleration
	// 1: new implementation
	cl_mouseAccelStyle = Cvar_Get( "cl_mouseAccelStyle", "0", CVAR_ARCHIVE );
	// offset for the power function (for style 1, ignored otherwise)
	// this should be set to the max rate value
	cl_mouseAccelOffset = Cvar_Get( "cl_mouseAccelOffset", "5", CVAR_ARCHIVE );
	Cvar_CheckRange(cl_mouseAccelOffset, 0.001f, 50000.0f, qfalse);

	cl_showMouseRate = Cvar_Get ("cl_showmouserate", "0", 0);

	cl_allowDownload = Cvar_Get ("cl_allowDownload", "0", CVAR_ARCHIVE);
#ifdef USE_CURL_DLOPEN
	cl_cURLLib = Cvar_Get("cl_cURLLib", DEFAULT_CURL_LIB, CVAR_ARCHIVE);
#endif

	cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
#ifdef __APPLE__
	// In game video is REALLY slow in Mac OS X right now due to driver slowness
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "0", CVAR_ARCHIVE);
#else
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "1", CVAR_ARCHIVE);
#endif

	cl_cinematicIgnoreSeek = Cvar_Get("cl_cinematicIgnoreSeek", "0", CVAR_ARCHIVE);

	cl_serverStatusResendTime = Cvar_Get ("cl_serverStatusResendTime", "750", 0);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get ("cg_autoswitch", "0", CVAR_ARCHIVE);

	m_pitch = Cvar_Get ("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get ("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get ("m_forward", "0.25", CVAR_ARCHIVE);
	m_side = Cvar_Get ("m_side", "0.25", CVAR_ARCHIVE);
#ifdef __APPLE__
	// Input is jittery on OS X w/o this
	m_filter = Cvar_Get ("m_filter", "1", CVAR_ARCHIVE);
#else
	m_filter = Cvar_Get ("m_filter", "0", CVAR_ARCHIVE);
#endif

	j_pitch =        Cvar_Get ("j_pitch",        "0.022", CVAR_ARCHIVE);
	j_yaw =          Cvar_Get ("j_yaw",          "-0.022", CVAR_ARCHIVE);
	j_forward =      Cvar_Get ("j_forward",      "-0.25", CVAR_ARCHIVE);
	j_side =         Cvar_Get ("j_side",         "0.25", CVAR_ARCHIVE);
	j_up =         Cvar_Get ("j_up",         "0", CVAR_ARCHIVE);

	j_pitch_axis =   Cvar_Get ("j_pitch_axis",   "3", CVAR_ARCHIVE);
	j_yaw_axis =     Cvar_Get ("j_yaw_axis",     "2", CVAR_ARCHIVE);
	j_forward_axis = Cvar_Get ("j_forward_axis", "1", CVAR_ARCHIVE);
	j_side_axis =    Cvar_Get ("j_side_axis",    "0", CVAR_ARCHIVE);
	j_up_axis =    Cvar_Get ("j_up_axis",    "4", CVAR_ARCHIVE);

	Cvar_CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);

	cl_motdString = Cvar_Get( "cl_motdString", "", CVAR_ROM );

	Cvar_Get( "cl_maxPing", "800", CVAR_ARCHIVE );

	cl_lanForcePackets = Cvar_Get ("cl_lanForcePackets", "1", CVAR_ARCHIVE);

	cl_guidServerUniq = Cvar_Get ("cl_guidServerUniq", "1", CVAR_ARCHIVE);

	// ~ and `, as keys and characters
	//cl_consoleKeys = Cvar_Get( "cl_consoleKeys", "~ ` 0x7e 0x60 K_F1", CVAR_ARCHIVE);
	cl_consoleKeys = Cvar_Get( "cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE);

	// userinfo
	Cvar_Get ("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE );
	cl_rate = Cvar_Get ("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("g_redTeam", DEFAULT_REDTEAM_NAME, CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get ("g_blueTeam", DEFAULT_BLUETEAM_NAME, CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get ("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("teamtask", "0", CVAR_USERINFO );
	Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	Cvar_Get ("password", "", CVAR_USERINFO);
	Cvar_Get ("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE );

#ifdef USE_MUMBLE
	cl_useMumble = Cvar_Get ("cl_useMumble", "0", CVAR_ARCHIVE | CVAR_LATCH);
	cl_mumbleScale = Cvar_Get ("cl_mumbleScale", "0.0254", CVAR_ARCHIVE);
#endif

#ifdef USE_VOIP
	cl_voipSend = Cvar_Get ("cl_voipSend", "0", 0);
	cl_voipSendTarget = Cvar_Get ("cl_voipSendTarget", "spatial", 0);
	cl_voipGainDuringCapture = Cvar_Get ("cl_voipGainDuringCapture", "0.2", CVAR_ARCHIVE);
	cl_voipCaptureMult = Cvar_Get ("cl_voipCaptureMult", "2.0", CVAR_ARCHIVE);
	cl_voipUseVAD = Cvar_Get ("cl_voipUseVAD", "0", CVAR_ARCHIVE);
	cl_voipVADThreshold = Cvar_Get ("cl_voipVADThreshold", "0.25", CVAR_ARCHIVE);
	cl_voipShowMeter = Cvar_Get ("cl_voipShowMeter", "1", CVAR_ARCHIVE);

	cl_voip = Cvar_Get ("cl_voip", "1", CVAR_ARCHIVE);
	Cvar_CheckRange( cl_voip, 0, 1, qtrue );
	cl_voipProtocol = Cvar_Get ("cl_voipProtocol", cl_voip->integer ? "opus" : "", CVAR_USERINFO | CVAR_ROM);
	cl_voipOverallGain = Cvar_Get ("cl_voipOverallGain", "1.0", CVAR_ARCHIVE);
	cl_voipGainOtherPlayback = Cvar_Get ("cl_voipGainOtherPlayback", "0.2", CVAR_ARCHIVE);
#endif

	// cgame might not be initialized before menu is used
	Cvar_Get ("cg_viewsize", "100", CVAR_ARCHIVE );
	// Make sure cg_stereoSeparation is zero as that variable is deprecated and should not be used anymore.
	Cvar_Get ("cg_stereoSeparation", "0", CVAR_ROM);

	cl_useq3gibs = Cvar_Get ("cl_useq3gibs", "0", CVAR_ARCHIVE);
	cl_consoleAsChat = Cvar_Get("cl_consoleAsChat", "0", CVAR_ARCHIVE);
	cl_numberPadInput = Cvar_Get("cl_numberPadInput", "0", CVAR_ARCHIVE);

	cl_maxRewindBackups = Cvar_Get("cl_maxRewindBackups", "12", CVAR_ARCHIVE | CVAR_LATCH);

	maxRewindBackups = cl_maxRewindBackups->integer;
	if (maxRewindBackups <= 0) {
		maxRewindBackups = MAX_REWIND_BACKUPS;
	}

	rewindBackups = malloc(sizeof(rewindBackups_t) * maxRewindBackups);
	if (!rewindBackups) {
		Com_Printf("ERROR:  CL_Init couldn't allocate %.2f MB for rewind backups\n", (sizeof(rewindBackups_t) * maxRewindBackups) / 1024.0 / 1024.0);
		exit(1);
	}
	Com_Printf("allocated %.2f MB for rewind backups\n", (sizeof(rewindBackups_t) * maxRewindBackups) / 1024.0 / 1024.0);

	cl_keepDemoFileInMemory = Cvar_Get("cl_keepDemoFileInMemory", "1", CVAR_ARCHIVE);
	cl_demoFileCheckSystem = Cvar_Get("cl_demoFileCheckSystem", "2", CVAR_ARCHIVE);
	cl_demoFile = Cvar_Get("cl_demoFile", "", CVAR_ROM);
	cl_demoFileBaseName = Cvar_Get("cl_demoFileBaseName", "", CVAR_ROM);
	cl_downloadWorkshops = Cvar_Get("cl_downloadWorkshops", "1", CVAR_ARCHIVE);

	//
	// register our commands
	//
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("configstrings", CL_Configstrings_f);
	Cmd_AddCommand ("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand ("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("demo", CL_PlayDemo_f);
	Cmd_SetCommandCompletionFunc( "demo", CL_CompleteDemoName );
	Cmd_AddCommand ("streamdemo", CL_StreamDemo_f);
	Cmd_AddCommand ("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand ("cinematic_restart", CL_RestartCinematic_f);
	Cmd_AddCommand ("cinematiclist", CL_ListCinematic_f);
	Cmd_AddCommand ("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("localservers", CL_LocalServers_f);
	Cmd_AddCommand ("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_SetCommandCompletionFunc( "rcon", CL_CompleteRcon );
	Cmd_AddCommand ("ping", CL_Ping_f );
	Cmd_AddCommand ("serverstatus", CL_ServerStatus_f );
	Cmd_AddCommand ("showip", CL_ShowIP_f );
	Cmd_AddCommand ("fs_openedList", CL_OpenedPK3List_f );
	Cmd_AddCommand ("fs_referencedList", CL_ReferencedPK3List_f );
	//Cmd_AddCommand ("model", CL_SetModel_f );
	//Cmd_AddCommand ("headmodel", CL_SetHeadModel_f );
	Cmd_AddCommand ("video", CL_Video_f );
	Cmd_AddCommand ("stopvideo", CL_StopVideo_f );

	if( !com_dedicated->integer ) {
		Cmd_AddCommand ("sayto", CL_Sayto_f );
		Cmd_SetCommandCompletionFunc( "sayto", CL_CompletePlayerName );
	}

	Cmd_AddCommand("rewind", CL_Rewind_f);
	Cmd_AddCommand("fastforward", CL_FastForward_f);
	Cmd_AddCommand("seekservertime", CL_SeekServerTime_f);
	Cmd_AddCommand("seek", CL_Seek_f);
	Cmd_AddCommand("seekend", CL_SeekEnd_f);
	Cmd_AddCommand("seeknext", CL_SeekNext_f);
	Cmd_AddCommand("seekprev", CL_SeekPrev_f);
	Cmd_AddCommand("pause", CL_Pause_f);
	//Cmd_AddCommand("togglemousegrab", CL_ToggleMouseGrab_f);
	Cmd_AddCommand("printdatadir", CL_PrintDataDir_f);
	Cmd_AddCommand("stall", CL_Stall_f);
	Cmd_AddCommand("setcolortable", CL_SetColorTable_f);
	Cmd_AddCommand("pov", CL_Pov_f);

	CL_InitRef();

	SCR_Init ();

//	Cbuf_Execute ();

	Cvar_Set( "cl_running", "1" );

	CL_GenerateQKey();
	Cvar_Get( "cl_guid", "", CVAR_USERINFO | CVAR_ROM );
	CL_UpdateGUID( NULL, 0 );

	Com_Printf( "----- Client Initialization Complete -----\n" );
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown(char *finalmsg, qboolean disconnect, qboolean quit)
{
	static qboolean recursive = qfalse;

	// check whether the client is running at all.
	if(!(com_cl_running && com_cl_running->integer))
		return;

	Com_Printf( "----- Client Shutdown (%s) -----\n", finalmsg );

	if ( recursive ) {
		Com_Printf( "WARNING: Recursive shutdown\n" );
		return;
	}
	recursive = qtrue;

	noGameRestart = quit;

	if(disconnect)
		CL_Disconnect(qtrue);

	CL_ClearMemory(qtrue);
	CL_Snd_Shutdown();

	Cmd_RemoveCommand ("cmd");
	Cmd_RemoveCommand ("configstrings");
	Cmd_RemoveCommand ("clientinfo");
	Cmd_RemoveCommand ("snd_restart");
	Cmd_RemoveCommand ("vid_restart");
	Cmd_RemoveCommand ("disconnect");
	Cmd_RemoveCommand ("record");
	Cmd_RemoveCommand ("demo");
	Cmd_RemoveCommand ("cinematic");
	Cmd_RemoveCommand ("cinematic_restart");
	Cmd_RemoveCommand ("cinematiclist");
	Cmd_RemoveCommand ("stoprecord");
	Cmd_RemoveCommand ("connect");
	Cmd_RemoveCommand ("reconnect");
	Cmd_RemoveCommand ("localservers");
	Cmd_RemoveCommand ("globalservers");
	Cmd_RemoveCommand ("rcon");
	Cmd_RemoveCommand ("ping");
	Cmd_RemoveCommand ("serverstatus");
	Cmd_RemoveCommand ("showip");
	Cmd_RemoveCommand ("fs_openedList");
	Cmd_RemoveCommand ("fs_referencedList");
	//Cmd_RemoveCommand ("model");
	//Cmd_RemoveCommand ("headmodel");
	Cmd_RemoveCommand ("video");
	Cmd_RemoveCommand ("stopvideo");
	Cmd_RemoveCommand("stall");

	CL_ShutdownInput();
	Con_Shutdown();

	Cvar_Set( "cl_running", "0" );

	recursive = qfalse;

	Com_Memset( &cls, 0, sizeof( cls ) );
	Key_SetCatcher( 0 );

	free(rewindBackups);
	Com_Printf( "-----------------------\n" );
}

static void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping) {
	if (server) {
		if (info) {
			server->clients = atoi(Info_ValueForKey(info, "clients"));
			Q_strncpyz(server->hostName,Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
			Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
			server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
			Q_strncpyz(server->game,Info_ValueForKey(info, "game"), MAX_NAME_LENGTH);
			server->gameType = atoi(Info_ValueForKey(info, "gametype"));
			server->netType = atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
			server->punkbuster = atoi(Info_ValueForKey(info, "punkbuster"));
			server->g_humanplayers = atoi(Info_ValueForKey(info, "g_humanplayers"));
			server->g_needpass = atoi(Info_ValueForKey(info, "g_needpass"));
		}
		server->ping = ping;
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping) {
	int i;

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.localServers[i].adr)) {
			CL_SetServerInfo(&cls.localServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_GLOBAL_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.globalServers[i].adr)) {
			CL_SetServerInfo(&cls.globalServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.favoriteServers[i].adr)) {
			CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
		}
	}

}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket( netadr_t from, msg_t *msg ) {
	int		i, type;
	char	info[MAX_INFO_STRING];
	char	*infoString;
	int		prot;
	char 	*gamename;
	qboolean gameMismatch;

	infoString = MSG_ReadString( msg );

	// if this isn't the correct gamename, ignore it
	gamename = Info_ValueForKey( infoString, "gamename" );

#ifdef LEGACY_PROTOCOL
	// gamename is optional for legacy protocol
	if (com_legacyprotocol->integer && !*gamename)
		gameMismatch = qfalse;
	else
#endif
		gameMismatch = !*gamename || strcmp(gamename, com_gamename->string) != 0;

	if (gameMismatch)
	{
		Com_DPrintf( "Game mismatch in info packet: %s\n", infoString );
		return;
	}

	// if this isn't the correct protocol version, ignore it
	prot = atoi( Info_ValueForKey( infoString, "protocol" ) );

	if(prot != com_protocol->integer
#ifdef LEGACY_PROTOCOL
	   && prot != com_legacyprotocol->integer
#endif
	   )
	{
		Com_DPrintf( "Different protocol info packet: %s\n", infoString );
		return;
	}

	// iterate servers waiting for ping response
	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		if ( cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr( from, cl_pinglist[i].adr ) )
		{
			// calc ping time
			cl_pinglist[i].time = Sys_Milliseconds() - cl_pinglist[i].start;
			Com_DPrintf( "ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString( from ) );

			// save of info
			Q_strncpyz( cl_pinglist[i].info, infoString, sizeof( cl_pinglist[i].info ) );

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch (from.type)
			{
				case NA_BROADCAST:
				case NA_IP:
					type = 1;
					break;
				case NA_IP6:
					type = 2;
					break;
				default:
					type = 0;
					break;
			}
			Info_SetValueForKey( cl_pinglist[i].info, "nettype", va("%d", type) );
			CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if (cls.pingUpdateSource != AS_LOCAL) {
		return;
	}

	for ( i = 0 ; i < MAX_OTHER_SERVERS ; i++ ) {
		// empty slot
		if ( cls.localServers[i].adr.port == 0 ) {
			break;
		}

		// avoid duplicate
		if ( NET_CompareAdr( from, cls.localServers[i].adr ) ) {
			return;
		}
	}

	if ( i == MAX_OTHER_SERVERS ) {
		Com_DPrintf( "MAX_OTHER_SERVERS hit, dropping infoResponse\n" );
		return;
	}

	// add this to the list
	cls.numlocalservers = i+1;
	CL_InitServerInfo( &cls.localServers[i], &from );

	Q_strncpyz( info, MSG_ReadString( msg ), MAX_INFO_STRING );
	if (strlen(info)) {
		if (info[strlen(info)-1] != '\n') {
			Q_strcat(info, sizeof(info), "\n");
		}
		Com_Printf( "%s: %s", NET_AdrToStringwPort( from ), info );
	}
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t *CL_GetServerStatus( netadr_t from ) {
	int i, oldest, oldestTime;

	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			return &cl_serverStatusList[i];
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( cl_serverStatusList[i].retrieved ) {
			return &cl_serverStatusList[i];
		}
	}
	oldest = -1;
	oldestTime = 0;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime) {
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	return &cl_serverStatusList[oldest];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen ) {
	int i;
	netadr_t	to;
	serverStatus_t *serverStatus;

	// if no server address then reset all server status requests
	if ( !serverAddress ) {
		for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	// get the address
	if ( !NET_StringToAdr( serverAddress, &to, NA_UNSPEC) ) {
		return qfalse;
	}
	serverStatus = CL_GetServerStatus( to );
	// if no server status string then reset the server status request for this address
	if ( !serverStatusString ) {
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	// if this server status request has the same address
	if ( NET_CompareAdr( to, serverStatus->address) ) {
		// if we received a response for this server status request
		if (!serverStatus->pending) {
			Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		// resend the request regularly
		else if ( serverStatus->startTime < Com_Milliseconds() - cl_serverStatusResendTime->integer ) {
			serverStatus->print = qfalse;
			serverStatus->pending = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = Com_Milliseconds();
			NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
			return qfalse;
		}
	}
	// if retrieved
	else if ( serverStatus->retrieved ) {
		serverStatus->address = to;
		serverStatus->print = qfalse;
		serverStatus->pending = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Com_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
		return qfalse;
	}
	return qfalse;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse( netadr_t from, msg_t *msg ) {
	char	*s;
	char	info[MAX_INFO_STRING];
	int		i, l, score, ping;
	int		len;
	serverStatus_t *serverStatus;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if (!serverStatus) {
		return;
	}

	s = MSG_ReadStringLine( msg );

	len = 0;
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "%s", s);

	if (serverStatus->print) {
		Com_Printf("Server settings:\n");
		// print cvars
		while (*s) {
			for (i = 0; i < 2 && *s; i++) {
				if (*s == '\\')
					s++;
				l = 0;
				while (*s) {
					info[l++] = *s;
					if (l >= MAX_INFO_STRING-1)
						break;
					s++;
					if (*s == '\\') {
						break;
					}
				}
				info[l] = '\0';
				if (i) {
					Com_Printf("%s\n", info);
				}
				else {
					Com_Printf("%-24s", info);
				}
			}
		}
	}

	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	if (serverStatus->print) {
		Com_Printf("\nPlayers:\n");
		Com_Printf("num: score: ping: name:\n");
	}
	for (i = 0, s = MSG_ReadStringLine( msg ); *s; s = MSG_ReadStringLine( msg ), i++) {

		len = strlen(serverStatus->string);
		Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\%s", s);

		if (serverStatus->print) {
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if (s)
				s = strchr(s+1, ' ');
			if (s)
				s++;
			else
				s = "unknown";
			Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s );
		}
	}
	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	serverStatus->time = Com_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if (serverStatus->print) {
		serverStatus->retrieved = qtrue;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f( void ) {
	char		*message;
	int			i, j;
	netadr_t	to;

	Com_Printf( "Scanning for servers on the local network...\n");

	// reset the list, waiting for response
	cls.numlocalservers = 0;
	cls.pingUpdateSource = AS_LOCAL;

	for (i = 0; i < MAX_OTHER_SERVERS; i++) {
		qboolean b = cls.localServers[i].visible;
		Com_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
		cls.localServers[i].visible = b;
	}
	Com_Memset( &to, 0, sizeof( to ) );

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for ( i = 0 ; i < 2 ; i++ ) {
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for ( j = 0 ; j < NUM_SERVER_PORTS ; j++ ) {
			to.port = BigShort( (short)(PORT_SERVER + j) );

			to.type = NA_BROADCAST;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );
			to.type = NA_MULTICAST6;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );
		}
	}
}

/*
==================
CL_GlobalServers_f

Originally master 0 was Internet and master 1 was MPlayer.
ioquake3 2008; added support for requesting five separate master servers using 0-4.
ioquake3 2017; made master 0 fetch all master servers and 1-5 request a single master server.
==================
*/
void CL_GlobalServers_f( void ) {
	netadr_t	to;
	int			count, i, masterNum;
	char		command[1024], *masteraddress;

	if ((count = Cmd_Argc()) < 3 || (masterNum = atoi(Cmd_Argv(1))) < 0 || masterNum > MAX_MASTER_SERVERS)
	{
		Com_Printf("usage: globalservers <master# 0-%d> <protocol> [keywords]\n", MAX_MASTER_SERVERS);
		return;
	}

	// request from all master servers
	if ( masterNum == 0 ) {
		int numAddress = 0;

		for ( i = 1; i <= MAX_MASTER_SERVERS; i++ ) {
			sprintf(command, "sv_master%d", i);
			masteraddress = Cvar_VariableString(command);

			if(!*masteraddress)
				continue;

			numAddress++;

			Com_sprintf(command, sizeof(command), "globalservers %d %s %s\n", i, Cmd_Argv(2), Cmd_ArgsFrom(3));
			Cbuf_AddText(command);
		}

		if ( !numAddress ) {
			Com_Printf( "CL_GlobalServers_f: Error: No master server addresses.\n");
		}
		return;
	}

	sprintf(command, "sv_master%d", masterNum);
	masteraddress = Cvar_VariableString(command);

	if(!*masteraddress)
	{
		Com_Printf( "CL_GlobalServers_f: Error: No master server address given.\n");
		return;
	}

	// reset the list, waiting for response
	// -1 is used to distinguish a "no response"

	i = NET_StringToAdr(masteraddress, &to, NA_UNSPEC);

	if(!i)
	{
		Com_Printf( "CL_GlobalServers_f: Error: could not resolve address of master %s\n", masteraddress);
		return;
	}
	else if(i == 2)
		to.port = BigShort(PORT_MASTER);

	Com_Printf("Requesting servers from %s (%s)...\n", masteraddress, NET_AdrToStringwPort(to));

	cls.numglobalservers = -1;
	cls.pingUpdateSource = AS_GLOBAL;

	// Use the extended query for IPv6 masters
	if (to.type == NA_IP6 || to.type == NA_MULTICAST6)
	{
		int v4enabled = Cvar_VariableIntegerValue("net_enabled") & NET_ENABLEV4;

		if(v4enabled)
		{
			Com_sprintf(command, sizeof(command), "getserversExt %s %s",
						com_gamename->string, Cmd_Argv(2));
		}
		else
		{
			Com_sprintf(command, sizeof(command), "getserversExt %s %s ipv6",
						com_gamename->string, Cmd_Argv(2));
		}
	}
	else if ( !Q_stricmp( com_gamename->string, LEGACY_MASTER_GAMENAME ) )
		Com_sprintf(command, sizeof(command), "getservers %s",
					Cmd_Argv(2));
	else
		Com_sprintf(command, sizeof(command), "getservers %s %s",
					com_gamename->string, Cmd_Argv(2));

	for (i=3; i < count; i++)
	{
		Q_strcat(command, sizeof(command), " ");
		Q_strcat(command, sizeof(command), Cmd_Argv(i));
	}

	NET_OutOfBandPrint( NS_SERVER, to, "%s", command );
}


/*
==================
CL_GetPing
==================
*/
void CL_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	const char	*str;
	int		time;
	int		maxPing;

	if (n < 0  ||  n >= MAX_PINGREQUESTS  ||  !cl_pinglist[n].adr.port)
	{
		// empty or invalid slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToStringwPort( cl_pinglist[n].adr );
	Q_strncpyz( buf, str, buflen );

	time = cl_pinglist[n].time;
	if (!time)
	{
		// check for timeout
		time = Sys_Milliseconds() - cl_pinglist[n].start;
		maxPing = Cvar_VariableIntegerValue( "cl_maxPing" );
		if( maxPing < 100 ) {
			maxPing = 100;
		}
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

	*pingtime = time;
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo( int n, char *buf, int buflen )
{
	if (n < 0  ||  n >= MAX_PINGREQUESTS  ||  !cl_pinglist[n].adr.port)
	{
		// empty slot or invalid slot
		if (buflen)
			buf[0] = '\0';
		return;
	}

	Q_strncpyz( buf, cl_pinglist[n].info, buflen );
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing( int n )
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
		return;

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount( void )
{
	int		i;
	int		count;
	ping_t*	pingptr;

	count   = 0;
	pingptr = cl_pinglist;

	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ ) {
		if (pingptr->adr.port) {
			count++;
		}
	}

	return (count);
}

/*
==================
CL_GetFreePing
==================
*/
ping_t* CL_GetFreePing( void )
{
	ping_t*	pingptr;
	ping_t*	best;
	int		oldest;
	int		i;
	int		time;

	pingptr = cl_pinglist;
	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ )
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (Sys_Milliseconds() - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return (pingptr);
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best    = cl_pinglist;
	oldest  = INT_MIN;
	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ )
	{
		// scan for oldest
		time = Sys_Milliseconds() - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return (best);
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f( void ) {
	netadr_t	to;
	ping_t*		pingptr;
	char*		server;
	int			argc;
	netadrtype_t	family = NA_UNSPEC;

	argc = Cmd_Argc();

	if ( argc != 2 && argc != 3 ) {
		Com_Printf( "usage: ping [-4|-6] server\n");
		return;
	}

	if(argc == 2)
		server = Cmd_Argv(1);
	else
	{
		if(!strcmp(Cmd_Argv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(Cmd_Argv(1), "-6"))
			family = NA_IP6;
		else
			Com_Printf( "warning: only -4 or -6 as address type understood.\n");

		server = Cmd_Argv(2);
	}

	Com_Memset( &to, 0, sizeof(netadr_t) );

	if ( !NET_StringToAdr( server, &to, family ) ) {
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy( &pingptr->adr, &to, sizeof (netadr_t) );
	pingptr->start = Sys_Milliseconds();
	pingptr->time  = 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean CL_UpdateVisiblePings_f(int source) {
	int			slots, i;
	char		buff[MAX_STRING_CHARS];
	int			pingTime;
	int			max;
	qboolean status = qfalse;

	if (source < 0 || source > AS_FAVORITES) {
		return qfalse;
	}

	cls.pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if (slots < MAX_PINGREQUESTS) {
		serverInfo_t *server = NULL;

		switch (source) {
			case AS_LOCAL :
				server = &cls.localServers[0];
				max = cls.numlocalservers;
			break;
			case AS_GLOBAL :
				server = &cls.globalServers[0];
				max = cls.numglobalservers;
			break;
			case AS_FAVORITES :
				server = &cls.favoriteServers[0];
				max = cls.numfavoriteservers;
			break;
			default:
				return qfalse;
		}
		for (i = 0; i < max; i++) {
			if (server[i].visible) {
				if (server[i].ping == -1) {
					int j;

					if (slots >= MAX_PINGREQUESTS) {
						break;
					}
					for (j = 0; j < MAX_PINGREQUESTS; j++) {
						if (!cl_pinglist[j].adr.port) {
							continue;
						}
						if (NET_CompareAdr( cl_pinglist[j].adr, server[i].adr)) {
							// already on the list
							break;
						}
					}
					if (j >= MAX_PINGREQUESTS) {
						status = qtrue;
						for (j = 0; j < MAX_PINGREQUESTS; j++) {
							if (!cl_pinglist[j].adr.port) {
								break;
							}
						}
						memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
						cl_pinglist[j].start = Sys_Milliseconds();
						cl_pinglist[j].time = 0;
						NET_OutOfBandPrint( NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx" );
						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (server[i].ping == 0) {
					// if we are updating global servers
					if (source == AS_GLOBAL) {
						//
						if ( cls.numGlobalServerAddresses > 0 ) {
							// overwrite this server with one from the additional global servers
							cls.numGlobalServerAddresses--;
							CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if (slots) {
		status = qtrue;
	}
	for (i = 0; i < MAX_PINGREQUESTS; i++) {
		if (!cl_pinglist[i].adr.port) {
			continue;
		}
		CL_GetPing( i, buff, MAX_STRING_CHARS, &pingTime );
		if (pingTime != 0) {
			CL_ClearPing(i);
			status = qtrue;
		}
	}

	return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f(void) {
	netadr_t	to, *toptr = NULL;
	char		*server;
	serverStatus_t *serverStatus;
	int			argc;
	netadrtype_t	family = NA_UNSPEC;

	argc = Cmd_Argc();

	if ( argc != 2 && argc != 3 )
	{
		if (clc.state != CA_ACTIVE || clc.demoplaying)
		{
			Com_Printf ("Not connected to a server.\n");
			Com_Printf( "usage: serverstatus [-4|-6] server\n");
			return;
		}

		toptr = &clc.serverAddress;
	}

	if(!toptr)
	{
		Com_Memset( &to, 0, sizeof(netadr_t) );

		if(argc == 2)
			server = Cmd_Argv(1);
		else
		{
			if(!strcmp(Cmd_Argv(1), "-4"))
				family = NA_IP;
			else if(!strcmp(Cmd_Argv(1), "-6"))
				family = NA_IP6;
			else
				Com_Printf( "warning: only -4 or -6 as address type understood.\n");

			server = Cmd_Argv(2);
		}

		toptr = &to;
		if ( !NET_StringToAdr( server, toptr, family ) )
			return;
	}

	NET_OutOfBandPrint( NS_CLIENT, *toptr, "getstatus" );

	serverStatus = CL_GetServerStatus( *toptr );
	serverStatus->address = *toptr;
	serverStatus->print = qtrue;
	serverStatus->pending = qtrue;
}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void) {
	Sys_ShowIP();
}

/*
=================
CL_CDKeyValidate
=================
*/
qboolean CL_CDKeyValidate( const char *key, const char *checksum ) {
#ifdef STANDALONE
	return qtrue;
#else
	char	ch;
	byte	sum;
	char	chs[3];
	int i, len;

	return qtrue;

	len = strlen(key);
	if( len != CDKEY_LEN ) {
		return qfalse;
	}

	if( checksum && strlen( checksum ) != CDCHKSUM_LEN ) {
		return qfalse;
	}

	sum = 0;
	// for loop gets rid of conditional assignment warning
	for (i = 0; i < len; i++) {
		ch = *key++;
		if (ch>='a' && ch<='z') {
			ch -= 32;
		}
		switch( ch ) {
		case '2':
		case '3':
		case '7':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'G':
		case 'H':
		case 'J':
		case 'L':
		case 'P':
		case 'R':
		case 'S':
		case 'T':
		case 'W':
			sum += ch;
			continue;
		default:
			return qfalse;
		}
	}

	sprintf(chs, "%02x", sum);

	if (checksum && !Q_stricmp(chs, checksum)) {
		return qtrue;
	}

	if (!checksum) {
		return qtrue;
	}

	return qfalse;
#endif
}

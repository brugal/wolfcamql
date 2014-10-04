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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

static char *svc_strings[256] = {
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
	"svc_EOF",
	"svc_extension",
	"svc_voip",
};

void SHOWNET( const msg_t *msg, const char *s) {
	if ( cl_shownet->integer >= 2) {
		Com_Printf ("%3i:%s\n", msg->readcount-1, s);
	}
}


static void Parse_Error (int code, const char *fmt, ...)
{
	va_list argptr;
	char errorMsg[MAX_PRINT_MSG];

    va_start(argptr, fmt);
	Q_vsnprintf (errorMsg, sizeof(errorMsg), fmt, argptr);
	va_end(argptr);

	if (di.testParse) {
		Com_Printf(S_COLOR_RED "demo error: '%s'\n", errorMsg);
	} else {
		Com_Error(code, "%s", errorMsg);
	}
}

/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old,
					 qboolean unchanged) {
	entityState_t	*state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

	if ( unchanged ) {
		*state = *old;
	} else {
		MSG_ReadDeltaEntity( msg, old, state, newnum );
	}

	if ( state->number == (MAX_GENTITIES-1) ) {
		return;		// entity was delta removed
	}

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( msg_t *msg, const clSnapshot_t *oldframe, clSnapshot_t *newframe) {
	int			newnum;
	entityState_t	*oldstate;
	int			oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = NULL;
	if (!oldframe) {
		oldnum = 99999;
	} else {
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while ( 1 ) {
		// read the entity index number
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum == (MAX_GENTITIES-1) ) {
			break;
		}

		if ( msg->readcount > msg->cursize ) {
			Parse_Error (ERR_DROP,"CL_ParsePacketEntities: end of message: %d > %d  entity: %d", msg->readcount, msg->cursize, newnum);
			//Com_Printf ("^3CL_ParsePacketEntities: end of message: %d > %d", msg->readcount, msg->cursize);
			return;
		}

		while ( oldnum < newnum ) {
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum) {
			// delta from previous state
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum ) {
			// delta from baseline
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[newnum], qfalse );
			//Com_Printf("%d > %d  delta from baseline  %d  etype %d\n", oldnum, newnum, cl.entityBaselines[newnum].number, cl.entityBaselines[newnum].eType);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != 99999 ) {
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->integer == 3 ) {
			Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

		oldindex++;

		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

static void CL_ParseExtraSnapshot (demoFile_t *df, msg_t *msg, clSnapshot_t *sn, qboolean justPeek)
{
	int			len;
	clSnapshot_t	*old;
	clSnapshot_t	newSnap;
	int			deltaNum;
	int			oldMessageNum;
	int			i, packetNum;

	//Com_Printf("CL_ParseExtraSnapshot\n");

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset (&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to svc_snapshot

	//FIXME
	newSnap.serverCommandNum = clc.serverCommandSequence;

	//Com_Printf("parse snap: %d\n", newSnap.serverCommandNum);

	newSnap.serverTime = MSG_ReadLong( msg );
	df->serverTime = newSnap.serverTime;

	//FIXME
	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;		// uncompressed frame
		old = NULL;
		//clc.demowaiting = qfalse;	// we can start recording now
		//Com_Printf("%d newSnap.deltaNum <= 0\n", clc.serverMessageSequence);
	} else {
		//Com_Printf("deltaNum: %d (us %d)  newSnap servertime %d\n", newSnap.deltaNum, clc.serverMessageSequence, newSnap.serverTime);
		old = &cl.snapshots[df->num][newSnap.deltaNum & PACKET_MASK];
		if ( !old->valid ) {
			// should never happen
			if (1) {  //(!clc.demoplaying) {
				//Com_Printf ("Delta from invalid frame (not supposed to happen!)  %d -> %d\n", newSnap.deltaNum, clc.serverMessageSequence);
				newSnap.valid = qfalse;
			} else {
				newSnap.valid = qtrue;
			}
		} else if ( old->messageNum != newSnap.deltaNum ) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			if (!clc.demoplaying) {
				Com_Printf ("Delta frame too old.\n");
			} else {
				// could be seeking in demo
				newSnap.valid = qtrue;
			}
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - MAX_SNAPSHOT_ENTITIES ) {
			if (!clc.demoplaying) {
				Com_Printf ("Delta parseEntitiesNum too old.\n");
			} else {
				newSnap.valid = qtrue;
			}
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if(len > sizeof(newSnap.areamask))
	{
		Com_Printf("^1CL_ParseExtraSnapshot: Invalid size %d for areamask for demoFile %d", len, df->f);
		return;
	}

	MSG_ReadData( msg, &newSnap.areamask, len);

	// read playerinfo
	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	if (sn) {
		*sn = newSnap;
	}

	//FIXME
	//return;

	if (justPeek) {
		return;
	}
	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid ) {
		Com_Printf("%s invalid snap returning\n", __FUNCTION__);
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}
	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ ) {
		cl.snapshots[df->num][oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) {
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}

	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[df->num][cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if (cl_shownet->integer == 3) {
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
		cl.snap.deltaNum, cl.snap.ping );
	}

	//cl.newSnapshots = qtrue;


	//Com_Printf("%s snap set\n", __FUNCTION__);
}

/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[][].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/

//#define MAX_PEEK_SNAPSHOTS 64

static clSnapshot_t PeekSnapshots[PACKET_BACKUP];

void CL_ParseSnapshot (msg_t *msg, clSnapshot_t *sn, int serverMessageSequence, qboolean justPeek)
{
	int			len;
	clSnapshot_t	*old;
	clSnapshot_t	newSnap;
	int			deltaNum;
	int			oldMessageNum;
	int			i, packetNum;

	//Com_Printf("CL_ParseSnapshot\n");

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset (&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = clc.serverCommandSequence;
	//Com_Printf("parse snap: %d\n", newSnap.serverCommandNum);

	newSnap.serverTime = MSG_ReadLong( msg );

	// if we were just unpaused, we can only *now* really let the
	// change come into effect or the client hangs.
	cl_paused->modified = 0;

	newSnap.messageNum = serverMessageSequence;  //clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;		// uncompressed frame
		old = NULL;
		clc.demowaiting = qfalse;	// we can start recording now
		//Com_Printf("%d newSnap.deltaNum <= 0\n", clc.serverMessageSequence);
	} else {
		if (deltaNum >= PACKET_BACKUP) {
			Com_Printf("^3CL_ParseSnapshot:  deltaNum %d invalid\n", deltaNum);
		}
		//Com_Printf("deltaNum: %d (us %d)  newSnap servertime %d\n", newSnap.deltaNum, clc.serverMessageSequence, newSnap.serverTime);
		//FIXME &cl.snapshots[] and multiple demos
		if (newSnap.deltaNum <= clc.serverMessageSequence) {
			old = &cl.snapshots[0][newSnap.deltaNum & PACKET_MASK];
		} else {
			//Com_Printf("^1need delta from read ahead\n");
			//FIXME  // cl.snapshots[][] take into account current demo used
			//old = &cl.snapshots[0][newSnap.deltaNum & PACKET_MASK];
			old = &PeekSnapshots[newSnap.deltaNum & PACKET_MASK];
		}
		if ( !old->valid ) {
			// should never happen
			if (1) {  //(!clc.demoplaying) {
				Com_Printf ("Delta from invalid frame (not supposed to happen!)  %d -> %d\n", newSnap.deltaNum, clc.serverMessageSequence);
				newSnap.valid = qfalse;
			} else {
				newSnap.valid = qtrue;
			}
		} else if ( old->messageNum != newSnap.deltaNum ) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			//if (!clc.demoplaying) {
			if (1) {
				Com_Printf ("Delta frame too old.\n");
			} else {
				// could be seeking in demo
				newSnap.valid = qtrue;
			}
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - MAX_SNAPSHOT_ENTITIES ) {
			if (!clc.demoplaying) {
				Com_Printf ("Delta parseEntitiesNum too old.\n");
			} else {
				newSnap.valid = qtrue;
			}
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if(len > sizeof(newSnap.areamask))
	{
		Parse_Error (ERR_DROP,"CL_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}

	MSG_ReadData( msg, &newSnap.areamask, len);

	// read playerinfo
	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	if (sn) {
		*sn = newSnap;
	}
	if (justPeek) {
		if (newSnap.messageNum > clc.serverMessageSequence) {
			//cl.snapshots[0][cl.snap.messageNum & PACKET_MASK] = cl.snap;
			PeekSnapshots[newSnap.messageNum & PACKET_MASK] = newSnap;
		}
		return;
	}
	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid ) {
		//Com_Printf("%s invalid snap returning\n", __FUNCTION__);
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}
	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ ) {
		cl.snapshots[0][oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) {
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}

	//FIXME check anims
	if (!di.testParse  &&  !justPeek) {
		int count;
		clSnapshot_t *frame;

		frame = &cl.snap;

		if (frame->ps.legsAnim != di.oldPsLegsAnim) {
			//Com_Printf("new ps legsAnim %d (%d old) for %d  %d\n", frame->ps.legsAnim, OldPsLegsAnim, frame->ps.clientNum, frame->serverTime);
			di.oldPsLegsAnim = frame->ps.legsAnim;
			di.oldPsLegsAnimTime = frame->serverTime;
		}
		if (frame->ps.torsoAnim != di.oldPsTorsoAnim) {
			//Com_Printf("new ps torsoAnim %d (%d old) for %d  %d\n", frame->ps.torsoAnim, OldPsTorsoAnim, frame->ps.clientNum, frame->serverTime);
			di.oldPsTorsoAnim = frame->ps.torsoAnim;
			di.oldPsTorsoAnimTime = frame->serverTime;
		}

		// hack to store when anim changes
		//frame->ps.stats[STAT_UNKNOWN_14] = di.oldPsLegsAnimTime;
		//frame->ps.stats[STAT_UNKNOWN_15] = di.oldPsTorsoAnimTime;

		count = cl.snap.numEntities;
        if (count > MAX_ENTITIES_IN_SNAPSHOT) {
			count = MAX_ENTITIES_IN_SNAPSHOT;
        }

        for (i = 0;  i < count;  i++) {
			entityState_t *state;

			state = &cl.parseEntities[(cl.snap.parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];

			if (di.oldLegsAnim[state->number] != state->legsAnim) {
				//Com_Printf("new legsAnim %d (%d old)  for %d  %d\n", state->legsAnim, OldLegsAnim[state->number], state->number, frame->serverTime);
				di.oldLegsAnim[state->number] = state->legsAnim;
				di.oldLegsAnimTime[state->number] = frame->serverTime;
			}

			if (di.oldTorsoAnim[state->number] != state->torsoAnim) {
				//Com_Printf("new torsoAnim %d (%d old) for %d  %d\n", state->torsoAnim, OldTorsoAnim[state->number], state->number, frame->serverTime);
				di.oldTorsoAnim[state->number] = state->torsoAnim;
				di.oldTorsoAnimTime[state->number] = frame->serverTime;
			}

			// hack to store when anim changes
			//state->apos.gravity = OldLegsAnimTime[state->number];
			//state->pos.gravity = OldTorsoAnimTime[state->number];
		}
	}

	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[0][cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if (cl_shownet->integer == 3) {
		Com_Printf( "0   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
		cl.snap.deltaNum, cl.snap.ping );
	}

	cl.newSnapshots = qtrue;


	//Com_Printf("%s snap set\n", __FUNCTION__);
}


//=====================================================================

int cl_connectedToPureServer;
int cl_connectedToCheatServer;

#ifdef USE_VOIP
int cl_connectedToVoipServer;
#endif

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_SystemInfoChanged( void ) {
	char			*systemInfo;
	const char		*s, *t;
	char			key[BIG_INFO_KEY];
	char			value[BIG_INFO_VALUE];
	qboolean		gameSet;

	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SYSTEMINFO ];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.serverId = atoi( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	// don't set any vars when playing a demo
	if ( clc.demoplaying ) {
		return;
	}

#ifdef USE_VOIP
	// in the future, (val) will be a protocol version string, so only
	//  accept explicitly 1, not generally non-zero.
	s = Info_ValueForKey( systemInfo, "sv_voip" );
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive"))
		cl_connectedToVoipServer = qfalse;
	else
		cl_connectedToVoipServer = (atoi( s ) == 1);
#endif

	s = Info_ValueForKey( systemInfo, "sv_cheats" );
	cl_connectedToCheatServer = atoi( s );
	if ( !cl_connectedToCheatServer ) {
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey( systemInfo, "sv_paks" );
	t = Info_ValueForKey( systemInfo, "sv_pakNames" );
	FS_PureServerSetLoadedPaks( s, t );

	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	FS_PureServerSetReferencedPaks( s, t );

	gameSet = qfalse;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while ( s ) {
		int cvar_flags;

		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}

		// ehw!
		if (!Q_stricmp(key, "fs_game"))
		{
			if(FS_CheckDirTraversal(value))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: Server sent invalid fs_game value %s\n", value);
				continue;
			}

			gameSet = qtrue;
		}

		if((cvar_flags = Cvar_Flags(key)) == CVAR_NONEXISTENT)
			Cvar_Get(key, value, CVAR_SERVER_CREATED | CVAR_ROM);
		else
		{
			// If this cvar may not be modified by a server discard the value.
			if(!(cvar_flags & (CVAR_SYSTEMINFO | CVAR_SERVER_CREATED | CVAR_USER_CREATED)))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: server is not allowed to set %s=%s\n", key, value);
				continue;
			}

			Cvar_SetSafe(key, value);
		}
	}
	// if game folder should not be set and it is set at the client side
	if ( !gameSet && *Cvar_VariableString("fs_game") ) {
		Cvar_Set( "fs_game", "" );
	}
	cl_connectedToPureServer = 0;  //Cvar_VariableValue( "sv_pure" );
}

/*
==================
CL_ParseServerInfo
==================
*/
static void CL_ParseServerInfo(void)
{
	const char *serverInfo;
	const char *value;

	serverInfo = cl.gameState.stringData
		+ cl.gameState.stringOffsets[ CS_SERVERINFO ];

	clc.sv_allowDownload = atoi(Info_ValueForKey(serverInfo,
		"sv_allowDownload"));
	Q_strncpyz(clc.sv_dlURL,
		Info_ValueForKey(serverInfo, "sv_dlURL"),
		sizeof(clc.sv_dlURL));
}

static void CL_ParseExtraGamestate (demoFile_t *df, msg_t *msg)
{
	int				i;
	entityState_t	*es;
	int				newnum;
	entityState_t	nullstate;
	int				cmd;
	char			*s;
	int serverCommandSequence;
	entityState_t tmpEntity;
	int clientNum;
	int checksumFeed;

	//clc.connectPacketCount = 0;

	// wipe local client state
	//CL_ClearState();

	// a gamestate always marks a server command sequence
	serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	//cl.gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings

	//Com_Printf("parse gamestate...\n");

	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			//int		len;

			//Com_Printf("config string...");

			i = MSG_ReadShort( msg );
			if ( i < 0 || i >= MAX_CONFIGSTRINGS ) {
				//Parse_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
				Com_Printf("^1CL_ParseExtraGamestate configstring(%d) > MAX_CONFIGSTRINGS(%d) demoFile %d\n", i, MAX_CONFIGSTRINGS, df->f);
				return;
			}
			s = MSG_ReadBigString( msg );
			//len = strlen( s );

#if 0
			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
				Parse_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
				return;
			}
#endif

			//Com_Printf("cs %d '%s'\n", i, s);

			// append it to the gameState string buffer
			//cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			//Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			//cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				//Parse_Error( ERR_DROP, "Baseline number out of range: %i", newnum );
				Com_Printf("^1CL_ParseExtraGamestate baseline number out of range: %d for demoFile %d\n", newnum, df->f);
				return;
			}
			//Com_Memset (&nullstate, 0, sizeof(nullstate));
			//es = &cl.entityBaselines[ newnum ];
			es = &tmpEntity;
			//MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
			MSG_ReadDeltaEntity(msg, &nullstate, es, newnum);
		} else {
			//Parse_Error( ERR_DROP, "CL_ParseGamestate: bad command byte" );
			Com_Printf("^1CL_ParseExtraGamestate bad command byte %d for demoFile %d\n", cmd, df->f);
			return;
		}
	}

	//clc.clientNum = MSG_ReadLong(msg);
	clientNum = MSG_ReadLong(msg);
	// read the checksum feed
	//clc.checksumFeed = MSG_ReadLong( msg );
	checksumFeed = MSG_ReadLong(msg);

	// parse useful values out of CS_SERVERINFO
	//CL_ParseServerInfo();
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate( msg_t *msg ) {
	int				i;
	entityState_t	*es;
	int				newnum;
	entityState_t	nullstate;
	int				cmd;
	char			*s;

	if (!di.testParse) {
		Con_Close();
	} else {
		//Con_Close();
	}

	//Com_Printf("parse gamestate...\n");

	clc.connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	cl.gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			int		len;

			i = MSG_ReadShort( msg );

			//FIXME parse protocol dynamically here

			//Com_Printf("config string %d\n", i);

			if ( i < 0 || i >= MAX_CONFIGSTRINGS ) {
				Parse_Error(ERR_DROP, "CL_ParseGamestate configstring(%d) > MAX_CONFIGSTRINGS(%d)", i, MAX_CONFIGSTRINGS);
				return;
			}
			s = MSG_ReadBigString( msg );
			len = strlen( s );

			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
				Parse_Error( ERR_DROP, "CL_ParseGamestate MAX_GAMESTATE_CHARS(%d) exceeded", MAX_GAMESTATE_CHARS);
				return;
			}

			// server info, get protocol here
			if (i == 0) {
				const char *value;
				int p;

				value = Info_ValueForKey(s, "protocol");
				p = atoi(value);

				Com_Printf("^5real gamestate protocol %d\n", p);
				Cvar_Set("real_protocol", value);

				if (p >= 66  &&  p <= 71) {
					Cvar_Set("protocol", va("%d", PROTOCOL_Q3));
				} else if (p == 73) {  //FIXME define
					Cvar_Set("protocol", "73");
				} else if (p == PROTOCOL_QL) {
					Cvar_Set("protocol", va("%d", PROTOCOL_QL));
				} else {
					Com_Printf("^3unknown protocol %d, trying dm %d\n", p, PROTOCOL_QL);
					Cvar_Set("protocol", va("%d", PROTOCOL_QL));

				}

				// sometimes in quake the protocol is found in this key...
				value = Info_ValueForKey(s, "com_protocol");  // fucking...
				p = atoi(value);
				if (p >= 66  &&  p <= 71) {
					Com_Printf("^5real gamestate using com_protocol %d (%s)\n", PROTOCOL_Q3, value);
					Cvar_Set("protocol", va("%d", PROTOCOL_Q3));
				}

				if (di.testParse) {
					di.protocol = atoi(com_protocol->string);
					Com_Printf("^5demo parse protocol %d\n", di.protocol);
				}

			}

			//Com_Printf("cs %d '%s'\n", i, s);

			// append it to the gameState string buffer
			cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				Parse_Error( ERR_DROP, "CL_ParseGamesate Baseline number out of range: %i", newnum );
				return;
			}
			Com_Memset (&nullstate, 0, sizeof(nullstate));
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		} else {
			Parse_Error(ERR_DROP, "CL_ParseGamestate: bad command byte", cmd);
			return;
		}
	}

	clc.clientNum = MSG_ReadLong(msg);
	// read the checksum feed
	clc.checksumFeed = MSG_ReadLong( msg );

	// parse useful values out of CS_SERVERINFO
	CL_ParseServerInfo();

	if (di.testParse) {
		const char *info;

		if (com_protocol->integer == PROTOCOL_Q3) {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CSQ3_GAME_VERSION];
			if (!Q_stricmp("cpma-1", info)) {
				Com_Printf("^5cpma\n");
				di.cpma = qtrue;
			} else {
				di.cpma = qfalse;
			}
		} else {
			di.cpma = qfalse;
		}

		if ((com_protocol->integer == PROTOCOL_QL  ||  com_protocol->integer == 73)) {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_WARMUP];
			di.hasWarmup = atoi(Info_ValueForKey(info, "time"));
		} else {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CSQ3_WARMUP];
			di.hasWarmup = atoi(info);
		}
		if (!di.hasWarmup) {
			if (com_protocol->integer == PROTOCOL_Q3) {
				if (!di.cpma) {
					di.gameStartTime = atoi(cl.gameState.stringData + cl.gameState.stringOffsets[CSQ3_LEVEL_START_TIME]);
				}
			} else {
				di.gameStartTime = atoi(cl.gameState.stringData + cl.gameState.stringOffsets[CS_LEVEL_START_TIME]);
			}
		}

		// check if demo starts in a timeout
		if (di.protocol == PROTOCOL_QL) {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_TIMEOUT_BEGIN_TIME];
			if (Q_isdigit(info[0])) {
				// cl.snap.serverTime is 0, but that's ok
				di.timeOuts[di.numTimeouts].startTime = cl.snap.serverTime;
				di.timeOuts[di.numTimeouts].serverTime = cl.snap.serverTime;
				//Com_Printf("^3start inside timeout %d\n", cl.snap.serverTime);
				di.numTimeouts++;
			}
		} else if (di.cpma) {
			int te, td;

			info = cl.gameState.stringData + cl.gameState.stringOffsets[CSCPMA_GAMESTATE];
			te = atoi(Info_ValueForKey(info, "te"));
			td = atoi(Info_ValueForKey(info, "td"));
			if (te) {
				di.timeOuts[di.numTimeouts].startTime = 0;
				di.timeOuts[di.numTimeouts].endTime = 0;
				di.cpmaLastTe = te;
				di.cpmaLastTd = td;
				di.numTimeouts++;
			}
		}
	}

	if (!di.testParse) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();

		// stop recording now so the demo won't have an unnecessary level load at the end.
		if(cl_autoRecordDemo->integer && clc.demorecording)
			CL_StopRecord_f();

		// reinitialize the filesystem if the game directory has changed

		FS_ConditionalRestart( clc.checksumFeed );
		// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
		// cgame
		CL_InitDownloads();

		// make sure the game starts
		Cvar_Set( "cl_paused", "0" );
	}
}


//=====================================================================

static void CL_ParseExtraDownload (demoFile_t *df, msg_t *msg)
{
	Com_Printf("FIXME CL_ParseExtraDownload\n");
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload ( msg_t *msg ) {
	int		size;
	unsigned char data[MAX_MSGLEN];
	int block;

	if (!*clc.downloadTempName) {
		Com_Printf("Server sending download, but no download was requested\n");
		CL_AddReliableCommand("stopdl", qfalse);
		return;
	}

	// read the data
	block = MSG_ReadShort ( msg );

	if ( !block )
	{
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong ( msg );

		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if (clc.downloadSize < 0)
		{
			Parse_Error( ERR_DROP, "%s", MSG_ReadString( msg ) );
			return;
		}
	}

	size = MSG_ReadShort ( msg );
	if (size < 0 || size > sizeof(data))
	{
		Parse_Error(ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size);
		return;
	}

	MSG_ReadData(msg, data, size);

	if (clc.downloadBlock != block) {
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_SV_FOpenFileWrite( clc.downloadTempName );

		if (!clc.download) {
			Com_Printf( "Could not create %s\n", clc.downloadTempName );
			CL_AddReliableCommand("stopdl", qfalse);
			CL_NextDownload();
			return;
		}
	}

	if (size)
		FS_Write( data, size, clc.download );

	CL_AddReliableCommand(va("nextdl %d", clc.downloadBlock), qfalse);
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );

	if (!size) { // A zero length block means EOF
		if (clc.download) {
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename ( clc.downloadTempName, clc.downloadName, qfalse );
		}

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload ();
	}
}

#ifdef USE_VOIP
static
qboolean CL_ShouldIgnoreVoipSender(int sender)
{
	if (!cl_voip->integer)
		return qtrue;  // VoIP is disabled.
	else if ((sender == clc.clientNum) && (!clc.demoplaying))
		return qtrue;  // ignore own voice (unless playing back a demo).
	else if (clc.voipMuteAll)
		return qtrue;  // all channels are muted with extreme prejudice.
	else if (clc.voipIgnore[sender])
		return qtrue;  // just ignoring this guy.
	else if (clc.voipGain[sender] == 0.0f)
		return qtrue;  // too quiet to play.

	return qfalse;
}

static void CL_ParseExtraVoip (demoFile_t *df, msg_t *msg)
{
	Com_Printf("FIXME CL_ParseExtraVoip\n");
}

/*
=====================
CL_ParseVoip

A VoIP message has been received from the server
=====================
*/
static
void CL_ParseVoip ( msg_t *msg ) {
	static short decoded[4096];  // !!! FIXME: don't hardcode.

	const int sender = MSG_ReadShort(msg);
	const int generation = MSG_ReadByte(msg);
	const int sequence = MSG_ReadLong(msg);
	const int frames = MSG_ReadByte(msg);
	const int packetsize = MSG_ReadShort(msg);
	char encoded[1024];
	int seqdiff = sequence - clc.voipIncomingSequence[sender];
	int written = 0;
	int i;

	Com_DPrintf("VoIP: %d-byte packet from client %d\n", packetsize, sender);

	if (sender < 0)
		return;   // short/invalid packet, bail.
	else if (generation < 0)
		return;   // short/invalid packet, bail.
	else if (sequence < 0)
		return;   // short/invalid packet, bail.
	else if (frames < 0)
		return;   // short/invalid packet, bail.
	else if (packetsize < 0)
		return;   // short/invalid packet, bail.

	if (packetsize > sizeof (encoded)) {  // overlarge packet?
		int bytesleft = packetsize;
		while (bytesleft) {
			int br = bytesleft;
			if (br > sizeof (encoded))
				br = sizeof (encoded);
			MSG_ReadData(msg, encoded, br);
			bytesleft -= br;
		}
		return;   // overlarge packet, bail.
	}

	if (!clc.speexInitialized) {
		MSG_ReadData(msg, encoded, packetsize);  // skip payload.
		return;   // can't handle VoIP without libspeex!
	} else if (sender >= MAX_CLIENTS) {
		MSG_ReadData(msg, encoded, packetsize);  // skip payload.
		return;   // bogus sender.
	} else if (CL_ShouldIgnoreVoipSender(sender)) {
		MSG_ReadData(msg, encoded, packetsize);  // skip payload.
		return;   // Channel is muted, bail.
	}

	// !!! FIXME: make sure data is narrowband? Does decoder handle this?

	Com_DPrintf("VoIP: packet accepted!\n");

	// This is a new "generation" ... a new recording started, reset the bits.
	if (generation != clc.voipIncomingGeneration[sender]) {
		Com_DPrintf("VoIP: new generation %d!\n", generation);
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		clc.voipIncomingGeneration[sender] = generation;
		seqdiff = 0;
	} else if (seqdiff < 0) {   // we're ahead of the sequence?!
		// This shouldn't happen unless the packet is corrupted or something.
		Com_DPrintf("VoIP: misordered sequence! %d < %d!\n",
		            sequence, clc.voipIncomingSequence[sender]);
		// reset the bits just in case.
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		seqdiff = 0;
	} else if (seqdiff > 100) { // more than 2 seconds of audio dropped?
		// just start over.
		Com_DPrintf("VoIP: Dropped way too many (%d) frames from client #%d\n",
		            seqdiff, sender);
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		seqdiff = 0;
	}

	if (seqdiff != 0) {
		Com_DPrintf("VoIP: Dropped %d frames from client #%d\n",
		            seqdiff, sender);
		// tell speex that we're missing frames...
		for (i = 0; i < seqdiff; i++) {
			assert((written + clc.speexFrameSize) * 2 < sizeof (decoded));
			speex_decode_int(clc.speexDecoder[sender], NULL, decoded + written);
			written += clc.speexFrameSize;
		}
	}

	for (i = 0; i < frames; i++) {
		const int len = MSG_ReadByte(msg);
		if (len < 0) {
			Com_DPrintf("VoIP: Short packet!\n");
			break;
		}
		MSG_ReadData(msg, encoded, len);

		// shouldn't happen, but just in case...
		if ((written + clc.speexFrameSize) * 2 > sizeof (decoded)) {
			Com_DPrintf("VoIP: playback %d bytes, %d samples, %d frames\n",
			            written * 2, written, i);
			S_RawSamples(sender + 1, written, clc.speexSampleRate, 2, 1,
			             (const byte *) decoded, clc.voipGain[sender]);
			written = 0;
		}

		speex_bits_read_from(&clc.speexDecoderBits[sender], encoded, len);
		speex_decode_int(clc.speexDecoder[sender],
		                 &clc.speexDecoderBits[sender], decoded + written);

		#if 0
		static FILE *encio = NULL;
		if (encio == NULL) encio = fopen("voip-incoming-encoded.bin", "wb");
		if (encio != NULL) { fwrite(encoded, len, 1, encio); fflush(encio); }
		static FILE *decio = NULL;
		if (decio == NULL) decio = fopen("voip-incoming-decoded.bin", "wb");
		if (decio != NULL) { fwrite(decoded+written, clc.speexFrameSize*2, 1, decio); fflush(decio); }
		#endif

		written += clc.speexFrameSize;
	}

	Com_DPrintf("VoIP: playback %d bytes, %d samples, %d frames\n",
	            written * 2, written, i);

	if (written > 0) {
		S_RawSamples(sender + 1, written, clc.speexSampleRate, 2, 1,
		             (const byte *) decoded, clc.voipGain[sender]);
	}

	clc.voipIncomingSequence[sender] = sequence + frames;
}
#endif


static void CL_ParseExtraCommandString (demoFile_t *df, msg_t *msg)
{
	char	*s;
	int		seq;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	//Com_Printf("^3'%s'\n", s);

	if (cl_shownet->integer == 3) {
		Com_Printf("demoFile %d cs (%d) '%s'\n", df->f, seq, s);
	}

	//FIXME other stuff
}

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString( msg_t *msg ) {
	char	*s;
	const char *s2;
	int		seq;
	int		index;
	char *val;
	int n;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	//Com_Printf("^3'%s'\n", s);

	if (cl_shownet->integer == 3) {
		Com_Printf("cs (%d) '%s'\n", seq, s);
	}

	// see if we have already executed stored it off
	if ( clc.serverCommandSequence >= seq ) {
		return;
	}
	clc.serverCommandSequence = seq;

	index = seq & (MAX_RELIABLE_COMMANDS-1);
	Q_strncpyz( clc.serverCommands[ index ], s, sizeof( clc.serverCommands[ index ] ) );

	if (di.testParse) {
#define BUFFER_SIZE 32
		char txtNum[BUFFER_SIZE];  //FIXME
		char *p;
		int csnum;
		int count;

		// "cs ??? blahblah"
		txtNum[0] = '\0';
		count = 0;
		p = s + strlen("cs ");
		while (1) {
			if (count >= BUFFER_SIZE) {
				txtNum[BUFFER_SIZE - 1] = '\0';
				break;
			}
			txtNum[count] = p[count];
			if (txtNum[count] == '\0'  ||  txtNum[count] == ' ') {
				txtNum[count] = '\0';
				break;
			}
			count++;
		}
		csnum = atoi(txtNum);
#undef BUFFER_SIZE

		// models used in demos
		if (  (di.protocol == PROTOCOL_QL  &&  (csnum >= CS_PLAYERS  &&  csnum < (CS_PLAYERS + MAX_CLIENTS)))  ||
			  (di.protocol == PROTOCOL_Q3  &&  (csnum >= CSQ3_PLAYERS  &&  csnum < (CSQ3_PLAYERS + MAX_CLIENTS)))
			) {
			char *model;
			//char *skin;
			int i;

			p = s + strlen("cs XXX ");
			model = Info_ValueForKey(p, "model");
#if 0
			skin = strrchr(model, '/');
			if (skin) {
				*skin++ = '\0';
			}
#endif
			if (*model) {
				qboolean found = qfalse;

				for (i = 0;  i < di.numPlayerInfo;  i++) {
					if (!Q_stricmp(di.playerInfo[i].modelName, model)) {
						found = qtrue;
						break;
					}
				}
				if (!found) {
					Q_strncpyz(di.playerInfo[di.numPlayerInfo].modelName, model, MAX_QPATH);
					di.numPlayerInfo++;
				}
			}
			model = Info_ValueForKey(p, "hmodel");
#if 0
			skin = strrchr(model, '/');
			if (skin) {
				*skin++ = '\0';
			}
#endif
			if (*model) {
				qboolean found = qfalse;

				for (i = 0;  i < di.numPlayerInfo;  i++) {
					if (!Q_stricmp(di.playerInfo[i].modelName, model)) {
						found = qtrue;
						break;
					}
				}
				if (!found) {
					Q_strncpyz(di.playerInfo[di.numPlayerInfo].modelName, model, MAX_QPATH);
					di.numPlayerInfo++;
				}
			}
		}

		// timeouts
		if (di.protocol == PROTOCOL_QL) {
			if (!Q_stricmpn(s, "cs 669 ", strlen("cs 669 "))) {
				//Com_Printf("^2%d  timeout start %s\n", cl.snap.serverTime, s);
				s2 = s + strlen("cs 669 ") + 1;
				if (Q_isdigit(s2[0])) {
					n = atoi(s2);
					//Com_Printf("%d\n", n);

					if (di.numTimeouts >= MAX_TIMEOUTS) {
						Com_Printf("^3MAX_TIMEOUTS(%d) couldn't set timeout start time\n", MAX_TIMEOUTS);
					} else {
						//di.timeOuts[di.numTimeouts].startTime = n;
						di.timeOuts[di.numTimeouts].startTime = cl.snap.serverTime;
						di.timeOuts[di.numTimeouts].serverTime = cl.snap.serverTime;
						//Com_Printf("^5 snap %d  %d\n", cl.snap.serverTime, n);
						di.numTimeouts++;
					}
				} else {
					//Com_Printf("^2timeout end %d -> %d (%d)\n", di.timeOuts[di.numTimeouts - 1].startTime, di.timeOuts[di.numTimeouts - 1].endTime, cl.snap.serverTime);
					di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime;
					//di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime + 25;
					//Com_Printf("servertime %d\n", cl.serverTime);
					//Com_Printf("clSnap %d\n", clSnap.serverTime);
				}
			}
		} else if (di.cpma) {

#define DEFAULT_TIMEOUT_AMOUNT (1000 * 1000)  //FIXME
			if (!Q_stricmpn(s, "cs 672 ", strlen("cs 672 "))) {
				int te, td;

				//Com_Printf("^2%d  timeout check start %s\n", cl.snap.serverTime, s);
				s2 = s + strlen("cs 672 ") + 1;
				te = atoi(Info_ValueForKey(s2, "te"));
				td = atoi(Info_ValueForKey(s2, "td"));

				if (te  &&  di.cpmaLastTe == 0) {  // timeout called
					if (di.numTimeouts >= MAX_TIMEOUTS) {
						Com_Printf("^3MAX_TIMEOUTS(%d) couldn't set cpma timeout\n", MAX_TIMEOUTS);
					} else {
						di.timeOuts[di.numTimeouts].startTime = cl.snap.serverTime;
						di.timeOuts[di.numTimeouts].endTime = cl.snap.serverTime + DEFAULT_TIMEOUT_AMOUNT;
						//Com_Printf("^2timeout %d at %d (%d)\n", di.numTimeouts, cl.snap.serverTime, ts + td + te);
						di.numTimeouts++;
					}
				} else if (te  &&  td != di.cpmaLastTd) {  // timein called
					di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime;
					if (di.numTimeouts >= MAX_TIMEOUTS) {
						Com_Printf("^3MAX_TIMEOUTS(%d) couldn't set cpma timeout\n", MAX_TIMEOUTS);
					} else {
						di.timeOuts[di.numTimeouts].startTime = cl.snap.serverTime;
						di.timeOuts[di.numTimeouts].endTime = cl.snap.serverTime + DEFAULT_TIMEOUT_AMOUNT;
						//Com_Printf("^2timeout %d at %d (%d)\n", di.numTimeouts, cl.snap.serverTime, ts + td + te);
						di.numTimeouts++;
					}
				} else if (!te  &&  di.cpmaLastTe) {  // timeout over
					if (di.numTimeouts <= 0) {
						Com_Printf("^3FIXME cpma timeout end without timeout detected\n");
					} else if (di.numTimeouts <= MAX_TIMEOUTS) {
						di.timeOuts[di.numTimeouts - 1].endTime = cl.snap.serverTime;
					}
				}

				di.cpmaLastTd = td;
				di.cpmaLastTe = te;
			}
#undef DEFAULT_TIMEOUT_AMOUNT

		}  // done with timeout check

		// game start and end times
		//Com_Printf("xx %s\n", s);
		if (di.gameStartTime == -1) {
			//Com_Printf("test %s\n", s);
			if (di.protocol == PROTOCOL_QL) {
				//Com_Printf("test parse command: '%s'\n", s);
				if (!Q_stricmpn(s, "cs 0 ", strlen("cs 0 "))) {
					//Com_Printf("xx: %s\n", s + strlen("cs 0 "));
					val = Info_ValueForKey(s + strlen("cs 0 ") + 1, "g_gamestate");
					//Com_Printf("val: '%s'\n", val);
					if (!Q_stricmp(val, "in_progress")) {
						Com_Printf("game start time: %d\n", cl.snap.serverTime);
						di.gameStartTime = cl.snap.serverTime;
					}
				}
			} else if (di.cpma) {
				if (!Q_stricmpn(s, "cs 672 ", strlen("cs 672 "))) {
					//if (!Q_stricmpn(s, "cs 657 ", strlen("cs 657 "))) {
					val = Info_ValueForKey(s + strlen("cs 672 ") + 1, "tw");
					//Com_Printf("val: %s\n", val);
					if (atoi(val) == 0) {
						val = Info_ValueForKey(s + strlen("cs 672 ") + 1, "ts");
						di.gameStartTime = atoi(val);
						Com_Printf("game start time: %d\n", di.gameStartTime);
					}
				}
			} else {  // q3  CS_WARMUP
				//Com_Printf("ss %s\n", s);
				if (!Q_stricmpn(s, "cs 5 ", strlen("cs 5 "))) {
					//Com_Printf("^3'%s'\n", s);

					s2 = s + strlen("cs 5 ");
					if (*s2) {
						n = atoi(s + strlen("cs 5"));
					} else {
						n = 0;
					}

					if (n == 0) {
						di.gameStartTime = cl.snap.serverTime;
						Com_Printf("game start time: %d\n", cl.snap.serverTime);
					}
				}
			}
		} else if (di.gameEndTime == -1) {
			if (di.protocol == PROTOCOL_QL) {
				if (!Q_stricmpn(s, "cs 14 ", strlen("cs 14 "))) {  // CS_INTERMISSION
					//int n;

					//Com_Printf("'%s'\n", s);
					//n = atoi(s + strlen("cs 14 "));
					//if (n == 1) {
					if ((s + strlen("cs 14 \""))[0] == '1') {
						Com_Printf("game end time: %d\n", cl.snap.serverTime);
						di.gameEndTime = cl.snap.serverTime;
					}
				}
			} else if (!di.cpma) {
				if (!Q_stricmpn(s, "cs 22 ", strlen("cs 22 "))) {  // CS_INTERMISSION
					//int n;

					//Com_Printf("'%s'\n", s);
					//n = atoi(s + strlen("cs 14 "));
					//if (n == 1) {
					if ((s + strlen("cs 22 \""))[0] == '1') {
						Com_Printf("game end time: %d\n", cl.snap.serverTime);
						di.gameEndTime = cl.snap.serverTime;
					}
				}
			} else if (di.cpma) {
				if (cl.snap.ps.pm_type == PM_INTERMISSION) {
					di.gameEndTime = cl.snap.serverTime;
					Com_Printf("game end time: %d\n", di.gameEndTime);
				}
			}
		}
	}
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( msg_t *msg ) {
	int			cmd;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf ("%i ",msg->cursize);
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf ("------------------\n");
	}

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong( msg );

	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	//
	// parse the message
	//
	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			Parse_Error (ERR_DROP,"CL_ParseServerMessage: read past end of server message");
			return;
		}

		cmd = MSG_ReadByte( msg );

		// See if this is an extension command after the EOF, which means we
		//  got data that a legacy client should ignore.
		if ((cmd == svc_EOF) && (MSG_LookaheadByte( msg ) == svc_extension)) {
			SHOWNET( msg, "EXTENSION" );
			MSG_ReadByte( msg );  // throw the svc_extension byte away.
			cmd = MSG_ReadByte( msg );  // something legacy clients can't do!
			// sometimes you get a svc_extension at end of stream...dangling
			//  bits in the huffman decoder giving a bogus value?
			if (cmd == -1) {
				cmd = svc_EOF;
			}
		}

		if (cmd == svc_EOF) {
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( (cmd < 0) || (!svc_strings[cmd]) ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount-1, cmd );
			} else {
				SHOWNET( msg, svc_strings[cmd] );
			}
		}

	// other commands
		switch ( cmd ) {
		default:
			Parse_Error (ERR_DROP, "CL_ParseServerMessage: Illegible server message %d", cmd);
			return;
		case svc_nop:
			break;
		case svc_serverCommand:
			CL_ParseCommandString( msg );
			break;
		case svc_gamestate:
			CL_ParseGamestate( msg );
			break;
		case svc_snapshot:
			CL_ParseSnapshot( msg, NULL, clc.serverMessageSequence, qfalse );
			break;
		case svc_download:
			CL_ParseDownload( msg );
			break;
		case svc_voip:
#ifdef USE_VOIP
			CL_ParseVoip( msg );
#endif
			break;
		}
	}
}

void CL_ParseExtraServerMessage (demoFile_t *df, msg_t *msg)
{
	int			cmd;
	int reliableAcknowledge;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf ("%i ",msg->cursize);
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf ("------------------\n");
	}

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	reliableAcknowledge = MSG_ReadLong( msg );

#if 0
	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
		clc.reliableAcknowledge = clc.reliableSequence;
	}
#endif

	//
	// parse the message
	//
	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			//Parse_Error (ERR_DROP,"CL_ParseServerMessage: read past end of server message");
			Com_Printf("^1CL_ParseExtraServerMessage() read past end of server message demoFile %d", df->f);
			return;
		}

		cmd = MSG_ReadByte( msg );

		// See if this is an extension command after the EOF, which means we
		//  got data that a legacy client should ignore.
		if ((cmd == svc_EOF) && (MSG_LookaheadByte( msg ) == svc_extension)) {
			SHOWNET( msg, "EXTENSION" );
			MSG_ReadByte( msg );  // throw the svc_extension byte away.
			cmd = MSG_ReadByte( msg );  // something legacy clients can't do!
			// sometimes you get a svc_extension at end of stream...dangling
			//  bits in the huffman decoder giving a bogus value?
			if (cmd == -1) {
				cmd = svc_EOF;
			}
		}

		if (cmd == svc_EOF) {
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( (cmd < 0) || (!svc_strings[cmd]) ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount-1, cmd );
			} else {
				SHOWNET( msg, svc_strings[cmd] );
			}
		}

	// other commands
		switch ( cmd ) {
		default:
			//Parse_Error (ERR_DROP, "CL_ParseServerMessage: Illegible server message %d", cmd);
			Com_Printf("^1CL_ParseExtraServerMessage: Illegible server message %d for demoFile %d", cmd, df->f);
			return;
		case svc_nop:
			break;
		case svc_serverCommand:
			CL_ParseExtraCommandString(df,  msg);
			break;
		case svc_gamestate:
			CL_ParseExtraGamestate(df, msg);
			break;
		case svc_snapshot:
			CL_ParseExtraSnapshot(df, msg, NULL, qfalse);
			break;
		case svc_download:
			CL_ParseExtraDownload(df, msg);
			break;
		case svc_voip:
#ifdef USE_VOIP
			CL_ParseExtraVoip(df, msg);
#endif
			break;
		}
	}
}



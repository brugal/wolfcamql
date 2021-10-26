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
// cl_cgame.c  -- client system interaction with client game

#include "client.h"
#include "keys.h"
#include <inttypes.h>

#include "../botlib/botlib.h"

#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

extern	botlib_export_t	*botlib_export;

extern qboolean loadCamera(const char *name);
extern void startCamera(int time);
extern qboolean getCameraInfo(int time, vec3_t *origin, vec3_t *angles, float *fov);

void CL_CalcSpline (int step, float tension, float *out)
{
	float f;

	f = 0;

	//Com_Printf("calc spline step %d  tension %f  float addr %p\n", step, tension, out);

    switch(step) {
    case 0: f = (pow(1 - tension, 3)) / 6; break;
    case 1: f = (3 * pow(tension, 3) - 6 * pow(tension, 2) + 4) / 6; break;
	case 2: f = (-3 * pow(tension, 3) + 3 * pow(tension, 2) + 3 * tension + 1) / 6; break;
    case 3: f = pow(tension, 3) / 6; break;
	default:
		break;
    }

	*out = f;
}

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean	CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	clSnapshot_t *extraSnap;
	int				i, count;
	int j;
	qboolean added[MAX_GENTITIES];
	int addExtraEnts;
	int pov;

	if ( snapshotNumber > cl.snap.messageNum ) {
		Com_Printf("%s snapshotNumber > cl.snap.messageNum\n", __FUNCTION__);
		//Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
		return qfalse;
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		Com_Printf("%s cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP\n", __FUNCTION__);
		return qfalse;
	}

	if (clc.demoplaying) {
		pov = di.pov;
	} else {
		pov = 0;
	}

	clSnap = &cl.snapshots[pov][snapshotNumber & PACKET_MASK];
	//extraSnap = &cl.snapshots[1][snapshotNumber & PACKET_MASK];

	// if the frame is not valid, we can't return it

	if ( !clSnap->valid ) {  // &&  !clc.demoplaying ) {
		// old snapshots get marked as invalid
		//Com_Printf("%s snapshot %d invalid\n", __FUNCTION__, snapshotNumber);
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES) {
		Com_Printf("%s cl.parseEntitiesNum(%d) - clSnap->parseEntitiesNum(%d) >= MAX_PARSE_ENTITIES(%d)   %d ents  want snap %d  current %d\n", __FUNCTION__, cl.parseEntitiesNum, clSnap->parseEntitiesNum, MAX_PARSE_ENTITIES, cl.parseEntitiesNum - clSnap->parseEntitiesNum, snapshotNumber, cl.snap.messageNum);
		return qfalse;
	}

	memset(added, 0, sizeof(added));

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	snapshot->messageNum = clSnap->messageNum;
	Com_Memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	//snapshot->ps = extraSnap->ps;
	added[snapshot->ps.clientNum] = qtrue;
	count = clSnap->numEntities;
	//Com_Printf("0 ents %d\n", count);
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		//Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		Com_Printf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] =
			cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
		added[snapshot->entities[i].number] = qtrue;
	}

	addExtraEnts = Cvar_VariableIntegerValue("all_ents");

	//Com_Printf("0 %d   1 %d\n", clSnap->parseEntitiesNum, extraSnap->parseEntitiesNum);

	if (clc.demoplaying  &&  addExtraEnts) {
		int newAdded = 0;

		for (j = 1;  j < di.numDemoFiles;  j++) {
			if (!di.demoFiles[j].valid) {
				continue;
			}

			extraSnap = &cl.snapshots[j][snapshotNumber & PACKET_MASK];
			//snapshot->numEntities += extraSnap->numEntities;
			for (i = 0;  i < extraSnap->numEntities;  i++) {
				entityState_t *es;

				es = &cl.parseEntities[(extraSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
				if (added[es->number]) {
					if (addExtraEnts > 1) {
						Com_Printf("%d demo %d skipping %d (already added)\n", cl.snap.serverTime, j, es->number);
					}
					continue;
				}
				snapshot->entities[count + newAdded] = *es;
				//cl.parseEntities[ ( extraSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
				snapshot->numEntities++;
				newAdded++;
			}
			if (!added[extraSnap->ps.clientNum]) {
				playerState_t ps;

				memset(&ps, 0, sizeof(ps));
				ps = extraSnap->ps;
				if (addExtraEnts > 1) {
					Com_Printf("new ps %d  entityEventSequence %d  eventSequence %d\n", ps.clientNum, ps.entityEventSequence, ps.eventSequence);
				}
				//FIXME external events -- hack until cgame supports more than one playerstate
				BG_PlayerStateToEntityState(&ps, &snapshot->entities[count + newAdded], qfalse);
				snapshot->numEntities++;
				newAdded++;
			} else {
				if (addExtraEnts > 1) {
					Com_Printf("%d demo %d skipping ps %d (already added)\n", cl.snap.serverTime, j, extraSnap->ps.clientNum);
				}
			}
		}
	}

	// FIXME: configstring changes and server commands!!!

	//Com_Printf("good snapshot %d\n", clSnap->messageNum);
	return qtrue;
}

//static entityState_t   tmpParseEntities[MAX_PARSE_ENTITIES];


qboolean CL_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot)
{
	clSnapshot_t	*clSnap;
	clSnapshot_t    csn;
	int				i, count;
	int origPosition;
	int cmd;
	char buffer[16];
	qboolean success = qfalse;
	int r;
	msg_t buf;
	byte bufData[MAX_MSGLEN];
	int j;
	int lastPacketTimeOrig;
	int parseEntitiesNumOrig;
	int currentSnapNum;
	int serverMessageSequence;
	int cmdCount;
	qboolean snapshotInMessage;
	qboolean silent = qfalse;

	clSnap = &csn;

	if (!clc.demoplaying) {
		return qfalse;
	}

	if (di.streaming) {
		silent = qtrue;
	}

	if (snapshotNumber <= cl.snap.messageNum) {
		//Com_Printf("FIXME CL_PeekSnapshot snapshotNumber <= cl.snap.messageNum  %d  %d\n", snapshotNumber, cl.snap.messageNum);
		success = CL_GetSnapshot(snapshotNumber, snapshot);
		if (!success) {
			Com_Printf("^3CL_PeekSnapshot snapshot number outside of backup buffer\n");
			return qfalse;
		}
		//Com_Printf("got old\n");
		return qtrue;
	}

	if (snapshotNumber > cl.snap.messageNum + 1) {
		//Com_Printf("FIXME CL_PeekSnapshot  %d >  cl.snap.messageNum + 1 (%d)\n", snapshotNumber, cl.snap.messageNum);
		//return qfalse;
	}

	parseEntitiesNumOrig = cl.parseEntitiesNum;
	lastPacketTimeOrig = clc.lastPacketTime;

	// CL_ReadDemoMessage()
	origPosition = FS_FTell(clc.demoReadFile);

	currentSnapNum = cl.snap.messageNum;

	for (j = 0;  j < snapshotNumber - currentSnapNum;  ) {
		success = qfalse;
		snapshotInMessage = qfalse;
		// get the sequence number
		memset(buffer, 0, sizeof(buffer));
		r = FS_Read( &buffer, 4, clc.demoReadFile);
		if ( r != 4 ) {
			if (!silent) {
				Com_Printf("CL_PeekSnapshot couldn't read sequence number\n");
			}
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}
		serverMessageSequence = LittleLong(*((int *)buffer));
		//Com_Printf("seq %d (%d)\n", serverMessageSequence, j);

		// init the message
		memset(&buf, 0, sizeof(msg_t));
		MSG_Init(&buf, bufData, sizeof(bufData));

		// get the length
		r = FS_Read (&buf.cursize, 4, clc.demoReadFile);
		if ( r != 4 ) {
			if (!silent) {
				Com_Printf("CL_PeekSnapshot couldn't get length\n");
			}
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}
		buf.cursize = LittleLong( buf.cursize );
		if ( buf.cursize == -1 ) {
			//Com_Printf("CL_PeekSnapshot buf.cursize == -1\n");
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}
		if ( buf.cursize > buf.maxsize ) {
			if (com_brokenDemo->integer) {
				Com_Printf("^1CL_PeekSnapshot:  demoMsglen > MAX_MSGLEN\n");
				FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
				clc.lastPacketTime = lastPacketTimeOrig;
				cl.parseEntitiesNum = parseEntitiesNumOrig;
				return qfalse;
			} else {
				Com_Error (ERR_DROP, "CL_PeekSnapshot: demoMsglen > MAX_MSGLEN");
			}
		}

		if (buf.cursize < 0) {
			Com_Printf("^1CL_PeekSnapshot negative message size: %d\n", buf.cursize);
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		r = FS_Read( buf.data, buf.cursize, clc.demoReadFile );
		if ( r != buf.cursize ) {
			if (!silent) {
				Com_Printf("CL_PeekSnapshot Demo file was truncated.\n");
			}
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		clc.lastPacketTime = cls.realtime;
		buf.readcount = 0;

		//  CL_ParseServerMessage( &buf );
		MSG_Bitstream(&buf);
		// get the reliable sequence acknowledge number
		//clc.reliableAcknowledge = MSG_ReadLong( msg );
		MSG_ReadLong(&buf);

		//
		// parse the message
		//
		cmdCount = 0;
		while ( 1 ) {
			qboolean dataFollowsEOF = qfalse;

			//cmdCount++;
			if ( buf.readcount > buf.cursize ) {
				if (com_brokenDemo->integer) {
					Com_Printf("^1CL_PeekSnapshot: read past end of server message");
					FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
					return qfalse;
				} else {
					Com_Error (ERR_DROP,"CL_PeekSnapshot: read past end of server message");
				}
				break;
			}

			cmd = MSG_ReadByte(&buf);

			// See if this is an extension command after the EOF, which means we
			// have speex voip data.
			if ((cmd == svc_EOF) && (MSG_LookaheadByte( &buf ) == svc_extension)) {
				dataFollowsEOF = qtrue;
				MSG_ReadByte( &buf );  // throw the svc_extension byte away.
				cmd = MSG_ReadByte( &buf );

				if (cmd == -1) {
					cmd = svc_EOF;
				}
			}

			//Com_Printf("cmd: %d\n", cmd);

			if (cmd == svc_EOF) {
				break;
			}

			switch (cmd) {
			default:
				Com_Error (ERR_DROP,"CL_PeekSnapshot: Illegible server message");
				break;
			case svc_nop:
				break;
			case svc_serverCommand: {
				//char *s;
				//CL_ParseCommandString( msg );
				MSG_ReadLong(&buf);  // seq
				//s = MSG_ReadString(&buf);
				MSG_ReadString(&buf);
				//Com_Printf("^2cs: '%s'\n", s);
				break;
			}
			case svc_gamestate:
				//CL_ParseGamestate( msg );
				//MSG_ReadLong(msg);  // clc.serverCommandSequence
				Com_Printf("FIXME CL_PeekSnapshot: gamestate\n");
				goto alldone;
				break;
			case svc_snapshot:
				snapshotInMessage = qtrue;
				CL_ParseSnapshot(&buf, &csn, serverMessageSequence, qtrue);
				//Com_Printf(" -- snapshot seq:%d  cmdCount:%d  loop:%d\n", serverMessageSequence, cmdCount, j);
				if (csn.valid) {
					success = qtrue;
				} else {
					//Com_Printf("bad snap\n");
				}
				j++;
				break;
			case svc_download:
				//CL_ParseDownload( msg );
				Com_Printf("FIXME CL_PeekSnapshot: download\n");
				goto alldone;
				break;
#ifdef USE_VOIP
			case svc_extension: {  // libspeex voip protocol 70 or 71
				if (dataFollowsEOF) {
					// shouldn't happen
					Com_Printf("^1%s unknown svc_extension\n", __FUNCTION__);
				} else {
					// check protocol to see if it contains flags
					CL_ParseVoipSpeex(&buf, qtrue, qtrue);
				}
				break;
			}
			case svc_voip:
				//Com_Printf("voip... seq:%d  cmdCount:%d  loop:%d\n", serverMessageSequence, cmdCount, j);
				if (dataFollowsEOF) {
					// old speex without flags
					CL_ParseVoipSpeex(&buf, qfalse, qtrue);
				} else {
					CL_ParseVoip(&buf, qtrue);
				}
				break;
#endif
			}
		}  // while (1)  reading commands

 alldone:

		if (!snapshotInMessage) {
			// could be a fake voip message written to demo
			continue;
		}

		//Com_Printf("^3 total cmds: %d\n", cmdCount);

		if (!success) {
			Com_Printf("^1CL_PeekSnapshot failed seq:%d  cmdCount:%d  loop:%d\n", serverMessageSequence, cmdCount, j);
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		//FIXME other ents not supported yet

		// if the entities in the frame have fallen out of their
		// circular buffer, we can't return it
		if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
			Com_Printf("%s cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES", __FUNCTION__);
			FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			//return qfalse;
			return qtrue;  //FIXME if you fix other ents
		}

		// write the snapshot
		snapshot->messageNum = serverMessageSequence;
		//Com_Printf("peek got %d\n", snapshot->messageNum);
		snapshot->snapFlags = clSnap->snapFlags;
		snapshot->serverCommandSequence = clSnap->serverCommandNum;
		snapshot->ping = clSnap->ping;
		snapshot->serverTime = clSnap->serverTime;
		Com_Memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
		snapshot->ps = clSnap->ps;
		count = clSnap->numEntities;
		if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
			//Com_DPrintf( "CL_PeekSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
			Com_Printf( "CL_PeekSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
			count = MAX_ENTITIES_IN_SNAPSHOT;
		}
		snapshot->numEntities = count;
		for ( i = 0 ; i < count ; i++ ) {
			snapshot->entities[i] =
				cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
		}

	}

	FS_Seek(clc.demoReadFile, origPosition, FS_SEEK_SET);
	clc.lastPacketTime = lastPacketTimeOrig;
	cl.parseEntitiesNum = parseEntitiesNumOrig;
	// FIXME: configstring changes and server commands!!!

	//Com_Printf("good snapshot %d\n", clSnap->messageNum);
	return qtrue;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
}

/*
=====================
CL_AddCgameCommand
=====================
*/
void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	if (Cvar_VariableIntegerValue("debug_configstring")) {
		Com_Printf("client: %d  %s\n", index, s);
	}

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;
		
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}
}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	char	*cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	//Com_Printf("CL_GetServerCommand %d %d\n", serverCommandNumber, clc.serverCommandSequence);

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying ) {
			Com_Printf("CL_GetServerCommand: a reliable command was cycled out:  %d <= %d - MAX_RELIABLE_COMMANDS(%d)\n", serverCommandNumber, clc.serverCommandSequence, MAX_RELIABLE_COMMANDS);
			return qfalse;
		}
		Com_Error(ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out:  %d <= %d - MAX_RELIABLE_COMMANDS(%d)", serverCommandNumber, clc.serverCommandSequence, MAX_RELIABLE_COMMANDS);
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );
	//Com_Printf( "serverCommand: %i : %s\n", serverCommandNumber, s );

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) ) {
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if ( argc >= 2 )
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected - %s", Cmd_Argv( 1 ) );
		else
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected" );
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		Com_Memset( cl.cmds, 0, sizeof( cl.cmds ) );
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make appropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname ) {
	int		checksum;

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	vec4_t color;
	//qboolean force = qfalse;

	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
	cls.cgameStarted = qfalse;
	if ( !cgvm ) {
		return;
	}

	//Com_Printf("^5clc.state: %d\n", clc.state);

	if (clc.state == CA_LOADING) {
		//force = qtrue;
	}


	VM_Call( cgvm, CG_SHUTDOWN );

#if 0
	if (force) {
		VM_Forced_Unload_Start();  // could be called from loading screen
	}
#endif
	VM_Free( cgvm );
#if 0
	if (force) {
		VM_Forced_Unload_Done();
	}
#endif
	cgvm = NULL;
	re.SetPathLines(NULL, NULL, NULL, NULL, color);
}

static int	FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

static qboolean CG_GetNextKiller (int us, int serverTime, int *killer, int *foundServerTime, qboolean onlyOtherClient)
{
	int i;
	demoObit_t *d;

	for (i = 0;  i < di.obitNum;  i++) {
		d = &di.obit[i];

		if (d->firstServerTime < serverTime) {
			continue;
		}

		if (d->victim == us) {
			if (onlyOtherClient) {
				if (d->killer == us  ||  d->killer < 0  ||  d->killer >= MAX_CLIENTS) {
					continue;
				}
			}

			*foundServerTime = d->firstServerTime;
			*killer = d->killer;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_GetNextVictim (int us, int serverTime, int *victim, int *foundServerTime, qboolean onlyOtherClient)
{
	int i;
	demoObit_t *d;

	for (i = 0;  i < di.obitNum;  i++) {
		d = &di.obit[i];

		if (d->firstServerTime < serverTime) {
			continue;
		}

		if (d->killer == us  &&  d->mod != MOD_THAW) {
			if (onlyOtherClient) {
				if (d->victim == us  ||  d->victim < 0  ||  d->victim >= MAX_CLIENTS) {
					continue;
				}
			}

			*foundServerTime = d->firstServerTime;
			*victim = d->victim;
			return qtrue;
		}
	}

	return qfalse;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
intptr_t CL_CgameSystemCalls( intptr_t *args ) {
	int i;

	switch( args[0] ) {
	case CG_PRINT:
		Com_Printf( "%s", (const char*)VMA(1) );
		return 0;
	case CG_ERROR:
		Com_Error( ERR_DROP, "%s", (const char*)VMA(1) );
		return 0;
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	case CG_CVAR_REGISTER:
		Cvar_Register( VMA(1), VMA(2), VMA(3), args[4] ); 
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update( VMA(1) );
		return 0;
	case CG_CVAR_SET:
		Cvar_SetSafe( VMA(1), VMA(2) );
		return 0;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( VMA(1), VMA(2), args[3] );
		return 0;
	case CG_CVAR_EXISTS:
		return Cvar_Exists(VMA(1));
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], VMA(2), args[3] );
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer( VMA(1), args[2] );
		return 0;
	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( VMA(1), VMA(2), args[3] );
	case CG_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;
	case CG_FS_SEEK:
		return FS_Seek( args[1], args[2], args[3] );
	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( VMA(1) );
		return 0;
	case CG_SENDCONSOLECOMMANDNOW:
		Cbuf_ExecuteText(EXEC_NOW, VMA(1));
		return 0;
	case CG_ADDCOMMAND:
		CL_AddCgameCommand( VMA(1) );
		return 0;
	case CG_REMOVECOMMAND:
		Cmd_RemoveCommandSafe( VMA(1) );
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand(VMA(1), qfalse);
		return 0;
	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		if (cl.draw) {


#if 0  // still a problem with /vid_restart, etc.. that calls VM shutdown
			// get input and pump events to make window responsive during the
			// loading screen
			IN_Frame();
			// option to skip network processing added to Com_EventLoop() to
			// prevent crashing mentioned above.
			Com_EventLoop(qfalse);
			//Com_EventLoop(qtrue);
			Cbuf_Execute();
#endif

			SCR_UpdateScreen();

#if 0
			S_Update();
			Con_RunConsole();
#endif
		}
		return 0;
	case CG_CM_LOADMAP:
		CL_CM_LoadMap( VMA(1) );
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( VMA(1), VMA(2), /*int capsule*/ qfalse );
	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( VMA(1), VMA(2), /*int capsule*/ qtrue );
	case CG_CM_POINTCONTENTS:
		return CM_PointContents( VMA(1), args[2] );
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents( VMA(1), args[2], VMA(3), VMA(4) );
	case CG_CM_BOXTRACE:
		CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse );
		return 0;
	case CG_CM_CAPSULETRACE:
		CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qtrue );
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qfalse );
		return 0;
	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qtrue );
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return re.MarkFragments( args[1], VMA(2), VMA(3), args[4], VMA(5), args[6], VMA(7) );
	case CG_S_STARTSOUND:
		S_StartSound( VMA(1), args[2], args[3], args[4] );
		return 0;
	case CG_S_STARTLOCALSOUND:
		if (Cvar_VariableValue("cg_audioAnnouncer") == 0) {
			if (args[2] != CHAN_ANNOUNCER) {
				S_StartLocalSound( args[1], args[2] );
			}
		} else {
			S_StartLocalSound( args[1], args[2] );
		}
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(args[1]);
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		S_AddLoopingSound( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_ADDREALLOOPINGSOUND:
		S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_STOPLOOPINGSOUND:
		S_StopLoopingSound( args[1] );
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], VMA(2) );
		return 0;
	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound( VMA(1), args[2] );
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( VMA(1), VMA(2) );
		return 0;
	case CG_S_PRINTSFXFILENAME:
		S_PrintSfxFilename(args[1]);
		return 0;
	case CG_R_LOADWORLDMAP: {
		int v;
		int gm;

		gm = Cvar_VariableValue("r_mapgreyscale");

		if (gm) {
			v = Cvar_VariableValue("r_greyscale");
			if (gm == 2) {
				Cvar_Set("r_greyscale", "2");
			} else {
				Cvar_Set( "r_greyscale", "1" );
			}
		}

		re.LoadWorld( VMA(1) );

		if (gm) {
			Cvar_Set( "r_greyscale", va("%d", v) );
		}

		return 0;
	}
	case CG_R_REGISTERMODEL:
		return re.RegisterModel( VMA(1) );
	case CG_R_REGISTERSKIN:
		return re.RegisterSkin( VMA(1) );
	case CG_R_REGISTERSHADER:
		return re.RegisterShader( VMA(1) );
	case CG_R_REGISTERSHADERLIGHTMAP:
		return re.RegisterShaderLightMap( VMA(1), args[2] );
	case CG_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( VMA(1) );
	case CG_R_REGISTERFONT:
		re.RegisterFont( VMA(1), args[2], VMA(3));
		return 0;
	case CG_R_GETGLYPHINFO:
		return re.GetGlyphInfo(VMA(1), args[2], VMA(3));
	case CG_R_GETFONTINFO:
		return re.GetFontInfo(args[1], VMA(2));
	case CG_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		if (cl.draw) {
			re.AddRefEntityToScene( VMA(1) );
		}
		return 0;
	case CG_R_ADDREFENTITYPTRTOSCENE:
		if (cl.draw) {
			re.AddRefEntityPtrToScene(VMA(1));
		}
		return 0;
	case CG_R_ADDPOLYTOSCENE:
		if (cl.draw) {
			re.AddPolyToScene( args[1], args[2], VMA(3), 1, args[4] );
		}
		return 0;
#if 0
	case CG_R_ADDPOLYSTOSCENE:
		if (cl.draw) {
			re.AddPolyToScene( args[1], args[2], VMA(3), args[4] );
		}
		return 0;
#endif
	case CG_R_LIGHTFORPOINT:
		return re.LightForPoint( VMA(1), VMA(2), VMA(3), VMA(4) );
	case CG_R_ADDLIGHTTOSCENE:
		if (cl.draw) {
			re.AddLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		}
		return 0;
	case CG_R_ADDADDITIVELIGHTTOSCENE:
		if (cl.draw) {
			re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		}
		return 0;
	case CG_R_RENDERSCENE:
		if (cl.draw) {
			re.RenderScene( VMA(1) );
		}
		return 0;
	case CG_R_SETCOLOR:
		if (cl.draw) {
			re.SetColor( VMA(1) );
		}
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		if (cl.draw) {
			re.DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		}
		return 0;
	case CG_R_MODELBOUNDS:
		re.ModelBounds( args[1], VMA(2), VMA(3) );
		return 0;
	case CG_R_LERPTAG:
		return re.LerpTag( VMA(1), args[2], args[3], args[4], VMF(5), VMA(6) );
	case CG_GETGLCONFIG:
		CL_GetGlconfig( VMA(1) );
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState( VMA(1) );
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( VMA(1), VMA(2) );
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], VMA(2) );
	case CG_PEEKSNAPSHOT:
		return CL_PeekSnapshot(args[1], VMA(2));
	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], VMA(2) );
	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue( args[1], VMF(2) );
		return 0;
	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
	case CG_KEY_ISDOWN:
		return Key_IsDown( args[1] );
	case CG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case CG_KEY_SETCATCHER:
		// Don't allow the cgame module to close the console
		Key_SetCatcher( args[1] | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
		return 0;
	case CG_KEY_GETKEY:
		return Key_GetKey( VMA(1) );

	case CG_MEMSET:
		Com_Memset( VMA(1), args[2], args[3] );
		return 0;
	case CG_MEMCPY:
		Com_Memcpy( VMA(1), VMA(2), args[3] );
		return 0;
	case CG_STRNCPY:
		strncpy( VMA(1), VMA(2), args[3] );
		return args[1];
	case CG_SIN:
		return FloatAsInt( sin( VMF(1) ) );
	case CG_COS:
		return FloatAsInt( cos( VMF(1) ) );
	case CG_ATAN2:
		return FloatAsInt( atan2( VMF(1), VMF(2) ) );
	case CG_SQRT:
		return FloatAsInt( sqrt( VMF(1) ) );
	case CG_FLOOR:
		return FloatAsInt( floor( VMF(1) ) );
	case CG_CEIL:
		return FloatAsInt( ceil( VMF(1) ) );
	case CG_ACOS:
		return FloatAsInt( Q_acos( VMF(1) ) );
	case CG_POWF:
		return FloatAsInt(powf(VMF(1), VMF(2)));

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( VMA(1) );
	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( VMA(1) );
	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], VMA(2) );
	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime(VMA(1), args[2], args[3]);
	case CG_SNAPVECTOR:
		Q_SnapVector(VMA(1));
		return 0;

	case CG_CIN_PLAYCINEMATIC:
	  return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case CG_CIN_STOPCINEMATIC:
	  return CIN_StopCinematic(args[1]);

	case CG_CIN_RUNCINEMATIC:
	  return CIN_RunCinematic(args[1]);

	case CG_CIN_DRAWCINEMATIC:
	  CIN_DrawCinematic(args[1]);
	  return 0;

	case CG_CIN_SETEXTENTS:
	  CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
	  return 0;

	case CG_R_REMAP_SHADER:
		re.RemapShader( VMA(1), VMA(2), VMA(3), args[4], args[5] );
		return 0;

	case CG_R_CLEAR_REMAPPED_SHADER:
		re.ClearRemappedShader(VMA(1));
		return 0;

	case CG_LOADCAMERA:
		return loadCamera(VMA(1));

	case CG_STARTCAMERA:
		startCamera(args[1]);
		return 0;

	case CG_GETCAMERAINFO:
		return getCameraInfo(args[1], VMA(2), VMA(3), VMA(4));

	case CG_GET_ENTITY_TOKEN:
		return re.GetEntityToken( VMA(1), args[2] );
	case CG_R_INPVS:
		return re.inPVS( VMA(1), VMA(2) );

	case CG_GET_ADVERTISEMENTS:
		re.Get_Advertisements(VMA(1), VMA(2), VMA(3));
		return 0;

	case CG_R_BEGIN_HUD:
		re.BeginHud();
		return 0;

	case CG_R_UPDATE_DOF:
		re.UpdateDof(VMF(1), VMF(2));
		return 0;

	case CG_DRAW_CONSOLE_LINES:
		Con_DrawNotify();
		return 0;
	case CG_KEY_GETBINDING:
		Cgame_Key_GetBinding(args[1], VMA(2));
		return 0;
	case CG_GETLASTEXECUTEDSERVERCOMMAND:
		return clc.lastExecutedServerCommand;
	case CG_GETNEXTKILLER:
		return CG_GetNextKiller(args[1], args[2], VMA(3), VMA(4), args[5]);
	case CG_GETNEXTVICTIM:
		return CG_GetNextVictim(args[1], args[2], VMA(3), VMA(4), args[5]);
	case CG_REPLACESHADERIMAGE:
		re.ReplaceShaderImage(args[1], VMA(2), args[3], args[4]);
		return 0;
	case CG_REGISTERSHADERFROMDATA:
		return re.RegisterShaderFromData(VMA(1), VMA(2), args[3], args[4], args[5], args[6], args[7], args[8]);
	case CG_GETSHADERIMAGEDIMENSIONS:
		re.GetShaderImageDimensions(args[1], VMA(2), VMA(3));
		return 0;
	case CG_GETSHADERIMAGEDATA:
		re.GetShaderImageData(args[1], VMA(2));
		return 0;
	case CG_CALCSPLINE:
		CL_CalcSpline(args[1], VMF(2), VMA(3));
		return 0;
	case CG_SETPATHLINES:
		re.SetPathLines(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5));
		return 0;
	case CG_GETGAMESTARTTIME:
		return di.gameStartTime;
	case CG_GETGAMEENDTIME:
		return di.gameEndTime;
	case CG_GETFIRSTSERVERTIME:
		return di.firstServerTime;
	case CG_GETLASTSERVERTIME:
		return di.lastServerTime;
	case CG_GETLEGSANIMSTARTTIME: {
		if (args[1] < 0  ||  args[1] >= MAX_GENTITIES) {
			return -1;
		}
		if (args[1] == cl.snap.ps.clientNum) {
			return di.oldPsLegsAnimTime;
		} else {
			return di.oldLegsAnimTime[args[1]];
		}
	}
	case CG_GETTORSOANIMSTARTTIME: {
		if (args[1] < 0  ||  args[1] >= MAX_GENTITIES) {
			return -1;
		}
		if (args[1] == cl.snap.ps.clientNum) {
			return di.oldPsTorsoAnimTime;
		} else {
			return di.oldTorsoAnimTime[args[1]];
		}
	}
	case CG_AUTOWRITECONFIG:
		com_writeConfig = args[1];
		return 0;
	case CG_GETITEMPICKUPNUMBER:
		if (di.numItemPickups <= 0) {
			return -1;
		}
		for (i = di.numItemPickups - 1;  i >= 0;  i--) {
			if (di.itemPickups[i].pickupTime <= args[1]) {
				return i;
			}
		}
		return 0;
	case CG_GETITEMPICKUP: {
		itemPickup_t *ip;

		if (di.numItemPickups <= 0  ||  args[1] < 0  ||  args[1] >= di.numItemPickups) {
			return -1;
		}

		ip = VMA(2);
		*ip = di.itemPickups[args[1]];

		return 0;
	}
	case CG_R_GETSINGLESHADER:
		return re.GetSingleShader();
	case CG_GET_DEMO_TIMEOUTS: {
		timeOut_t *to;
		int *numTimeouts;

		numTimeouts = (int *)VMA(1);

		*numTimeouts = di.numTimeouts;
		to = (timeOut_t *)VMA(2);
		memcpy(to, di.timeOuts, sizeof(di.timeOuts));

		return 0;
	}
	case CG_GETROUNDSTARTTIMES: {
		int *numRoundStartTimes;
		int *roundStarts;

		numRoundStartTimes = (int *)VMA(1);
		roundStarts = (int *)VMA(2);

		*numRoundStartTimes = di.numRoundStarts;
		memcpy(roundStarts, di.roundStarts, sizeof(int) * di.numRoundStarts);

		return 0;
	}
	case CG_GETTEAMSWITCHTIME: {
		int clientNum;
		int startTime;
		int *teamSwitchTime;
		int i;

		clientNum = args[1];
		startTime = args[2];
		teamSwitchTime = (int *)VMA(3);

		*teamSwitchTime = 0;
		for (i = 0;  i < di.numTeamSwitches;  i++) {
			if (di.teamSwitches[i].serverTime >= startTime) {
				if (di.teamSwitches[i].clientNum == clientNum) {
					*teamSwitchTime = di.teamSwitches[i].serverTime;
					return qtrue;
				}
			}
		}

		return qfalse;
	}
	case CG_GET_NUM_PLAYER_INFO: {
		return di.numPlayerInfo;
	}
	case CG_GET_EXTRA_PLAYER_INFO: {
		char *output;

		if (args[1] >= di.numPlayerInfo) {
			// 2021-08-25 Debian stable x86_64-w64-mingw32-gcc (GCC) 10-win32 20210110 is defining __USE_MINGW_ANSI_STDIO which uses %lld for PRIdPTR
#if defined(__MINGW32__)
			Com_Printf("^1CL_CgameSystemCalls CG_GET_EXTRA_PLAYER_INFO invalid number %" "I64d"  "\n", args[1]);
#else
			Com_Printf("^1CL_CgameSystemCalls CG_GET_EXTRA_PLAYER_INFO invalid number %" PRIdPTR "\n", args[1]);
#endif
			return 0;
		}

		output = (char *)VMA(2);
		Q_strncpyz(output, di.playerInfo[args[1]].modelName, MAX_QPATH);
		return 0;
	}
	case CG_GET_REAL_MAP_NAME: {
		const char *name;
		char *output;
		int sz;
		int i;

		name = (const char *)VMA(1);
		output = (char *)VMA(2);
		sz = args[3];

		//output[0] = '\0';
		Q_strncpyz(output, name, sz);

		if (!FS_VirtualFileExists(name)) {
			i = 0;
			while (1) {
				if (MapNames[i].oldName == NULL) {
					break;
				}
				if (!Q_stricmp(name, MapNames[i].oldName)) {
					if (!FS_VirtualFileExists(MapNames[i].newName)) {
						// couldn't find alt map
						//Com_Printf("newName '%s' doesn't exist\n", MapNames[i].newName);
						break;
					} else {
						Q_strncpyz(output, MapNames[i].newName, sz);
					}
					break;
				}

				if (!Q_stricmp(name, MapNames[i].newName)) {
					if (!FS_VirtualFileExists(MapNames[i].oldName)) {
						// couldn't find alt map
						//Com_Printf("oldName '%s' doesn't exist\n", MapNames[i].oldName);
						break;
					} else {
						Q_strncpyz(output, MapNames[i].oldName, sz);
					}
					break;
				}

				i++;
			}
		}

		return 0;
	}
	case CG_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();
	case CG_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case CG_KEY_SETBINDING:
		Key_SetBinding(args[1], VMA(2));
		return 0;

	case CG_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], VMA(2), args[3]);
		return 0;

	case CG_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
		return 0;

	default:
		//assert(0);
		Com_Error( ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0] );
	}
	return 0;
}

#ifdef CGAME_HARD_LINKED
int cgame_syscall (int arg, ...)
{
	return CL_CgameSystemCalls(&arg);
}
#endif


extern int StartTime;

/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	int					t1, t2;
	vmInterpret_t		interpret;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	// load the dll or bytecode
	interpret = Cvar_VariableValue("vm_cgame");
	if(cl_connectedToPureServer)
	{
		// if sv_pure is set we only allow qvms to be loaded
		if(interpret != VMI_COMPILED && interpret != VMI_BYTECODE)
			interpret = VMI_COMPILED;
	}

#ifdef CGAME_HARD_LINKED
	cgvm = (vm_t *)1;
#else
	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, interpret );
#endif
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
	clc.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call(cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demoplaying);

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !clc.demoplaying && !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	clc.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds, total time: %5.2f\n", (t2-t1)/1000.0, (t2 - StartTime) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if (!Sys_LowPhysicalMemory()) {
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify ();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	//Com_Printf("game command\n");
	if ( !cgvm ) {
		return qfalse;
	}

	//Com_Printf("xxx\n");

	return VM_Call( cgvm, CG_CONSOLE_COMMAND );
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	int startTime;

	startTime = Sys_Milliseconds();
	VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying, di.streaming, di.waitingForStream, CL_VideoRecording(&afdMain), (int)(Overf * SUBTIME_RESOLUTION), qtrue);
	clc.cgameTime += (Sys_Milliseconds() - startTime);
	VM_Debug( 0 );
	//cl.draw = qtrue;
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

void CL_AdjustTimeDelta( void ) {
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	clc.state = CA_ACTIVE;

	//cl.serverTime = cl.snap.serverTime - 100;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;
	//clc.lastExecutedServerCommand = cl.snap.serverCommandSequence;
	clc.lastExecutedServerCommand = clc.serverCommandSequence;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Com_Printf("active action: '%s'\n", cl_activeAction->string);
		Cvar_Set( "activeAction", "" );
	}

#ifdef USE_MUMBLE
	if ((cl_useMumble->integer) && !mumble_islinked()) {
		int ret = mumble_link(CLIENT_WINDOW_TITLE);
		Com_Printf("Mumble: Linking to Mumble application %s\n", ret==0?"ok":"failed");
	}
#endif

#ifdef USE_VOIP
	// speex used for demo playback
	if (!clc.speexInitialized) {
		int i;
		speex_bits_init(&clc.speexEncoderBits);
		speex_bits_reset(&clc.speexEncoderBits);

		clc.speexEncoder = speex_encoder_init(&speex_nb_mode);

		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_FRAME_SIZE,
		                  &clc.speexFrameSize);
		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_SAMPLING_RATE,
		                  &clc.speexSampleRate);

		clc.speexPreprocessor = speex_preprocess_state_init(clc.speexFrameSize,
		                                                  clc.speexSampleRate);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_DENOISE, &i);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_AGC, &i);

		for (i = 0; i < MAX_CLIENTS; i++) {
			speex_bits_init(&clc.speexDecoderBits[i]);
			speex_bits_reset(&clc.speexDecoderBits[i]);
			clc.speexDecoder[i] = speex_decoder_init(&speex_nb_mode);
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.speexInitialized = qtrue;
	}

	if (!clc.voipCodecInitialized) {
		int i;
		int error;

		clc.opusEncoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);

		if ( error ) {
			Com_DPrintf("VoIP: Error opus_encoder_create %d\n", error);
			return;
		}

		for (i = 0; i < MAX_CLIENTS; i++) {
			clc.opusDecoder[i] = opus_decoder_create(48000, 1, &error);
			if ( error ) {
				Com_DPrintf("VoIP: Error opus_decoder_create(%d) %d\n", i, error);
				return;
			}
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.voipCodecInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand ("voip", CL_Voip_f);
		Cvar_Set("cl_voipSendTarget", "spatial");
		Com_Memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
	}
#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	int loopCount = 0;
	int startTime;
	static int lastTime = -1;  //FIXME static

	//Com_Printf("%s  clc.state:%d\n", __FUNCTION__, clc.state);

	// getting a valid frame message ends the connection process
	if ( clc.state != CA_ACTIVE ) {
		if ( clc.state != CA_PRIMED ) {
			//Com_Printf("%s clc.state != CA_PRIMED returning\n", __FUNCTION__);
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			if (!cl_freezeDemo->integer) {  //  ||  (cl_freezeDemo->integer  &&  cl.vidRestarted)) {
				//Com_Printf("%s read demo message\n", __FUNCTION__);
				CL_ReadDemoMessage(qfalse);
				if (di.waitingForStream) {
					return;
				}
			}
		}
		if ( cl.newSnapshots ) {
			//Com_Printf("%s new snapshots\n", __FUNCTION__);
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if (clc.state != CA_ACTIVE) { //  &&  !(cl_freezeDemo->integer  &&  cl.vidRestarted)) {
			return;
		}
	} else {
		//Com_Printf("%s clc.state == CA_ACTIVE no read\n", __FUNCTION__);
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if (clc.demoplaying  &&  (cl_freezeDemo->integer  ||  di.waitingForStream)) {
		//

	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;
		if (tn<-30) {
			tn = -30;
		} else if (tn>30) {
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		//Com_Printf("cl.serverTime %d\n", cl.serverTime);

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime ) {
			if (clc.demoplaying) {
				Com_Printf("%s cl.serverTime < cl.oldServerTime changing  %d < %d\n", __FUNCTION__, cl.serverTime, cl.oldServerTime);
			}
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definitely
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		int now = Sys_Milliseconds( );
		int frameDuration;

		//Com_Printf("timedemo\n");
		if (!clc.timeDemoStart) {
			clc.timeDemoStart = clc.timeDemoLastFrame = now;
			clc.timeDemoMinDuration = INT_MAX;
			clc.timeDemoMaxDuration = 0;
		}

		frameDuration = now - clc.timeDemoLastFrame;
		clc.timeDemoLastFrame = now;

		// Ignore the first measurement as it'll always be 0
		if( clc.timeDemoFrames > 0 )
		{
			if( frameDuration > clc.timeDemoMaxDuration )
				clc.timeDemoMaxDuration = frameDuration;

			if( frameDuration < clc.timeDemoMinDuration )
				clc.timeDemoMinDuration = frameDuration;

			// 255 ms = about 4fps
			if( frameDuration > UCHAR_MAX )
				frameDuration = UCHAR_MAX;

			clc.timeDemoDurations[ ( clc.timeDemoFrames - 1 ) %
				MAX_TIMEDEMO_DURATIONS ] = frameDuration;
		}

		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	if (cl_freezeDemo->integer) {
		return;
	}

	loopCount = 0;
	startTime = cl.snap.serverTime;
	//lastTime = -1;

	//Com_Printf("setcgametime() cl.serverTime %d  cl.snap.serverTime %d\n", cl.serverTime, cl.snap.serverTime);

	// note:  engine advances snapshots based on server time, while cgame
	//        advances based on snapshot number (processed in cgame snapshot
	//        number)


	while (cl.serverTime >= cl.snap.serverTime) {
		loopCount++;
		// feed another message, which should change
		// the contents of cl.snap
		if (!cl_freezeDemo->integer) {
			if (Cvar_VariableIntegerValue("debug_snapshots")) {
				if (1) {  //(loopCount > 1) {
					Com_Printf("%s cl.serverTime >= cl.snap.serverTime   %d  %d  %d\n", __FUNCTION__, cl.serverTime, cl.snap.serverTime, loopCount);
				}
			}

			CL_ReadDemoMessage(qfalse);
			if (di.waitingForStream) {
				return;
			}

			if (!di.testParse  &&  clc.state == CA_ACTIVE  &&  cl.serverTime > cl.snap.serverTime  &&  com_timescaleSafe->integer) {  //  &&  com_timescale->value > 1.0) {
				if (com_timescale->value > 1.0) {  //(!di.offlineDemo) {  //FIXME offlinedemo  ||  (di.offlineDemo  &&  cl.serverTime > cl.snap.serverTime)) {
					//Com_Printf("setcgametime timescale %f  framecount %d\n", com_timescale->value, cls.framecount);
					//Com_Printf("setcgametime timescale %f\n", com_timescale->value);

					if (startTime == cl.snap.serverTime  ||  lastTime == cl.snap.serverTime) {  // offline demo
						lastTime = cl.snap.serverTime;
						cl.oldServerTime = lastTime;
						continue;
					} else {
						if (cl.serverTime > cl.snap.serverTime) {
#if 1
							cl.serverTime = cl.snap.serverTime;
							cls.realtime = cl.snap.serverTime;
							cl.serverTimeDelta = 0;
							//cl.draw = qfalse;
							cl.oldServerTime = lastTime;
#endif

#if 0
							cl.draw = qfalse;
							VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.snap.serverTime, STEREO_CENTER, clc.demoplaying, di.streaming, di.waitingForStream, CL_VideoRecording(&afdMain), (int)(Overf * SUBTIME_RESOLUTION), qfalse);
							cl.draw = qtrue;
							cls.framecount++;
#endif
						}
					}

				}
			}
		}

		lastTime = cl.snap.serverTime;

		if ( clc.state != CA_ACTIVE ) {
			return;		// end of demo
		}
		if (di.testParse  &&  di.endOfDemo) {
			return;
		}
	}

	if (Cvar_VariableIntegerValue("debug_snapshots")) {
		Com_Printf("%s cl.snap.serverTime %d  cl.serverTime %d  wantedTime %f\n", __FUNCTION__, cl.snap.serverTime, cl.serverTime, di.wantedTime);
	}

	if (loopCount == 0) {
		if (Cvar_VariableIntegerValue("debug_snapshots")) {
			Com_Printf("%s loop 0   cl.serverTime: %d  cl.snap.serverTime: %d\n", __FUNCTION__, cl.serverTime, cl.snap.serverTime);
		}
	}
}

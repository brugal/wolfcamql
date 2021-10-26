// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

#include "cg_draw.h"  // reset lagometer
#include "cg_event.h"
#include "cg_freeze.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_players.h"
#include "cg_playerstate.h"
#include "cg_predict.h"
#include "cg_q3mme_demos_camera.h"  // demo.play.time, demo.play.fraction, demo.serverTime
#include "cg_servercmds.h"
#include "cg_snapshot.h"
#include "cg_syscalls.h"
#include "sc.h"
#include "wolfcam_ents.h"
#include "wolfcam_snapshot.h"

#include "wolfcam_local.h"
//#include "../qcommon/qcommon.h"

/*
==================
CG_ResetEntity
==================
*/
void CG_ResetEntity( centity_t *cent ) {

	if (cg_useOriginalInterpolation.integer == 0) {
		//return;
	}

	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if ( cent->snapShotTime < cg.time - EVENT_VALID_MSEC ) {
		cent->previousEvent = 0;
	}


	//VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	//VectorCopy (cent->currentState.angles, cent->lerpAngles);
	VectorCopy(cent->currentState.pos.trBase, cent->lerpOrigin);
	VectorCopy(cent->currentState.pos.trBase, cent->lerpAngles);

	CG_ResetFXIntervalAndDistance(cent);
	cent->trailTime = cg.snap->serverTime;

	CG_UpdatePositionData(cent, &cent->flightPositionData);
	CG_UpdatePositionData(cent, &cent->hastePositionData);

	//Com_Printf("reset: %f %f %f  --  %f %f %f\n", cent->currentState.origin[0], cent->currentState.origin[1], cent->currentState.origin[2], cent->currentState.pos.trBase[0], cent->currentState.pos.trBase[1], cent->currentState.pos.trBase[2]);

	cent->wasReset = qtrue;

	if (cent->currentState.number >= MAX_CLIENTS  &&  cent->currentState.eType == ET_PLAYER) {
		//Com_Printf("CG_ResetEntity %d  %d\n", cent->currentState.number, cent->currentState.clientNum);
		if (cg_useOriginalInterpolation.integer == 0) {
			//int number;

			//number = cent->currentState.number;
			CG_ResetPlayerEntity(cent);
			return;
			//cent->pe = cg_entities[cent->currentState.clientNum].pe;
			//memcpy(cent, &cg_entities[cent->currentState.clientNum], sizeof(*cent));
			//cent->currentState.number = number;
			//return;
		}
	}

	if ( cent->currentState.eType == ET_PLAYER ) {
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity( centity_t *cent ) {
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	} else {
		//Com_Printf("not resetting: %d\n", cent - cg_entities);
	}

#if 0  // testing
	if (SC_Cvar_Get_Int("debugabort")) {
		CG_Abort();  // testing crash
	}
#endif

	// clear the next state.  if will be set by the next CG_SetNextSnap
	//Com_Printf("transition %d interp false\n", cent->currentState.number);
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents( cent );
}


/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot.
All other times will use CG_TransitionSnapshot instead.
==================
*/
static void CG_SetInitialSnapshot( snapshot_t *snap ) {
	int				i;
	centity_t		*cent;
	entityState_t	*state;

	CG_Printf("set initial snapshot\n");

	if (!cgs.gotFirstSnap) {
		cgs.gotFirstSnap = qtrue;
		cgs.firstSnapServerTime = snap->serverTime;
		Com_Printf("serverCommandSequence %d\n", snap->serverCommandSequence);
		//cgs.serverCommandSequence = trap_GetLastExecutedServerCommand();
		cgs.serverCommandSequence = snap->serverCommandSequence;
	}

	cg.snap = snap;

    memcpy (&wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP], snap, sizeof(snapshot_t));
    wcg.curSnapshotNumber++;

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );
	CG_ResetEntity(&cg_entities[snap->ps.clientNum]);

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy(&cent->currentState, state, sizeof(entityState_t));
		//cent->currentState = *state;
		//Com_Printf("set initial %d interp false\n", state->number);
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}
}


/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/

static int SortClients (const void *a, const void *b) {
	return *(int *)a - *(int *)b;
}

static void CG_TransitionSnapshot( void ) {
	centity_t			*cent;
	snapshot_t			*oldFrame;
	int					i;
	//int sfps;
	int serverFrameTime;

	if ( !cg.snap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	cg.infectedSnapshotTime = 0;

	if (cg.demoPlayback) {
#if 0
		trap_GetCurrentSnapshotNumber(&n, &t);
		//r = CG_PeekSnapshot(cgs.processedSnapshotNum + 2, &cg.nextNextSnap);
		cg.nextNextSnap = &cg.nnSnap;
		r = CG_PeekSnapshot(n + 1, cg.nextNextSnap);
		if (!r) {
			Com_Printf("couldn't get nextNextSnap\n");
			cg.nextNextSnapValid = qfalse;
		} else {
			cg.nextNextSnapValid = qtrue;
		}
#endif
	}

	if (1) {  //(cg.nextSnap->messageNum - cg.snap->messageNum == 1) {
	serverFrameTime = cg.nextSnap->serverTime - cg.snap->serverTime;
	if (serverFrameTime > 0) {
		if (serverFrameTime < cg.serverFrameTime) {
			cg.serverFrameTime = serverFrameTime;
			Com_Printf("sv_fps %d  %d ms\n", 1000 / serverFrameTime, serverFrameTime);
		}
	} else {
		//Com_Printf("^1FIXME server time 0???\n");
	}
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclients[i].killedOrDied = qfalse;
	}

	//Com_Printf("transition cg.time %d  snap %d %d  time %d %d   %d\n", cg.time, cg.snap->messageNum, cg.nextSnap->messageNum, cg.snap->serverTime, cg.nextSnap->serverTime, cg.nextSnap->serverTime - cg.snap->serverTime);

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// if we had a map_restart, set everything with initial
	if ( cg.mapRestart ) {
	}

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	cg.prevSnap = cg.snap;
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	if (SC_Cvar_Get_Int("debug_snapshots")) {
		Com_Printf("serverTime: %d\n", cg.snap->serverTime);
	}

	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, qfalse );
	//Com_Printf("^6armor:  %d\n", cg_entities[cg.snap->ps.clientNum].currentState.armor);
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qfalse;

	if ((cgs.realProtocol >= 91  &&  CG_IsTeamGame(cgs.gametype))  ||  CG_IsCpmaMvd()) {
		numSortedTeamPlayers = 0;

		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			numSortedTeamPlayers = 1;
			sortedTeamPlayers[0] = cg.snap->ps.clientNum;
			//Com_Printf("adding %d\n", cg.snap->ps.clientNum);
		}
	}

	if (!wolfcam_following  ||  wcg.clientNum == cg.snap->ps.clientNum) {
		if (cg_drawJumpSpeeds.integer == 1  &&  sqrt(cg.snap->ps.velocity[0] * cg.snap->ps.velocity[0] + cg.snap->ps.velocity[1] * cg.snap->ps.velocity[1]) < 1.0) {
			cg.jumpsNeedClearing = qtrue;
		}
	}

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];

		if (cg.snap->entities[i].eType == ET_MISSILE) {
			//Com_Printf("missile %d pos.trTime %d  server time %d  %d  %s\n", cg.snap->entities[i].number, cg.snap->entities[i].pos.trTime, cg.snap->serverTime, cg.snap->serverTime - cg.snap->entities[i].pos.trTime, weapNames[cg.snap->entities[i].weapon]);
		}

		if (cgs.protocol == PROTOCOL_QL  &&  cg.snap->entities[i].eType == ET_ITEM) {
			const entityState_t *es = &cg.snap->entities[i];
			// check if we can update client item timers with ingame timer
			// pie info
			if (es->time > 0  &&  es->time2 > 0) {
				//FIXME
			}
		}

        if (cg.snap->entities[i].number < MAX_CLIENTS) {
            memcpy (&wclients[cg.snap->entities[i].number].oldState, &cg_entities[cg.snap->entities[i].number].currentState, sizeof (entityState_t));
#if 0
			CG_Printf ("%d  %f %f %f\n", cg.snap->entities[i].number,
					   cg.snap->entities[i].pos.trBase[0],
					   cg.snap->entities[i].pos.trBase[1],
					   cg.snap->entities[i].pos.trBase[2]
					   );
#endif

			if ((cgs.realProtocol >= 91  &&  CG_IsTeamGame(cgs.gametype))  ||  CG_IsCpmaMvd()) {
				const clientInfo_t *ci;
				int ourTeam;

				if (wolfcam_following) {
					ourTeam = cgs.clientinfo[wcg.clientNum].team;
				} else {
					if (CG_IsCpmaMvd()) {
						ourTeam = TEAM_SPECTATOR;
					} else {
						ourTeam = cg.snap->ps.persistant[PERS_TEAM];
					}
				}

				ci = cgs.clientinfo + cg.snap->entities[i].number;
				if (ci->infoValid  &&  ci->team == ourTeam  &&  ourTeam != TEAM_SPECTATOR) {
					if (numSortedTeamPlayers < TEAM_MAXOVERLAY) {
						numSortedTeamPlayers++;
						sortedTeamPlayers[numSortedTeamPlayers - 1] = cg.snap->entities[i].number;
						//Com_Printf("^2numSorted added: %s\n", ci->name);
					}
				}
			}
        }

		if ((cgs.realProtocol >= 91  &&  CG_IsTeamGame(cgs.gametype))  ||  CG_IsCpmaMvd()) {
			qsort(sortedTeamPlayers, numSortedTeamPlayers, sizeof(sortedTeamPlayers[0]), SortClients);
		}

		CG_TransitionEntity( cent );

		if (wolfcam_following  &&  cg.snap->entities[i].number == wcg.clientNum) {
			if (cg_drawJumpSpeeds.integer == 1  &&  sqrt(cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] + cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1]) < 1.0) {
				cg.jumpsNeedClearing = qtrue;
			}
		}

		// remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	cg.nextSnap = NULL;


	// check for playerstate transition events
	if ( oldFrame ) {
		playerState_t *ops;
		const playerState_t *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;	// will be cleared by prediction code
			cg_entities[cg.snap->ps.clientNum].lastLgFireFrameCount = 0;
			cg_entities[cg.snap->ps.clientNum].lastLgImpactFrameCount = 0;
			cg_entities[cg.snap->ps.clientNum].lastLgImpactTime = 0;
			cg.predictedPlayerEntity.lastLgFireFrameCount = 0;
			cg.predictedPlayerEntity.lastLgImpactFrameCount = 0;
			cg.predictedPlayerEntity.lastLgImpactTime = 0;
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg_nopredict.integer || cg_synchronousClients.integer ) {
			CG_TransitionPlayerState( ps, ops );
		}
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclient_t *wc;

		wc = &wclients[i];
		if (wc->killedOrDied == qfalse) {
			continue;
		}
		memcpy(&wc->lastKillwstats, &wc->perKillwstats, sizeof(wc->lastKillwstats));
		memset(&wc->perKillwstats, 0, sizeof(wc->perKillwstats));
	}

	// set first and second place names if possible
	if (cgs.protocol != PROTOCOL_QL) {
		if (CG_IsDuelGame(cgs.gametype)) {
			if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR) {
				int otherDuelPlayer;

				// we can figure it out
				otherDuelPlayer = -1;
				for (i = 0;  i < MAX_CLIENTS;  i++) {
					if (!cgs.clientinfo[i].infoValid) {
						continue;
					}
					if (cgs.clientinfo[i].team != TEAM_FREE) {
						continue;
					}
					if (i == cg.snap->ps.clientNum) {
						continue;
					}

					otherDuelPlayer = i;
					break;
				}

				if (otherDuelPlayer != -1) {
					if (CG_ScoresEqual(cgs.scores1, cg.snap->ps.persistant[PERS_SCORE])) {
						Q_strncpyz(cgs.firstPlace, cgs.clientinfo[cg.snap->ps.clientNum].name, sizeof(cgs.firstPlace));
						Q_strncpyz(cgs.secondPlace, cgs.clientinfo[otherDuelPlayer].name, sizeof(cgs.secondPlace));
					} else {
						Q_strncpyz(cgs.firstPlace, cgs.clientinfo[otherDuelPlayer].name, sizeof(cgs.firstPlace));
						Q_strncpyz(cgs.secondPlace, cgs.clientinfo[cg.snap->ps.clientNum].name, sizeof(cgs.secondPlace));
					}
				}
			}
		}
	}
}

void CG_FixCpmaMvdEntity (entityState_t *es)
{
#if 0  //FIXME testing cpma mvd 0.99x7
	//if (cent->currentState.eType == 70) {
	if (cent->currentState.number < MAX_CLIENTS) {
		cent->currentState.eType = ET_PLAYER;
		cent->currentState.weapon = WP_GAUNTLET;
		cent->currentState.eFlags &= ~EF_NODRAW;
	}
#endif

	// older cpma mvd demos (ex: 0.99x7) don't transmit some values
	// 0.99z3 appears to be ok
	// 0.99x4 drex-vs-ZeRo4(MVD)-cpm1a-2002.07.18-19.44.39 positions

	if (!CG_IsCpmaMvd()) {
		return;
	}

#if 0
	// 0.99x4 sending all 64 entities every frame?
	if (es->number < MAX_CLIENTS   &&  cgs.clientinfo[es->number].infoValid  &&  cgs.clientinfo[es->number].team != TEAM_SPECTATOR) {
		CG_Printf("%d: eType %d, clientNum %d, weapon %d, eflags 0x%x, powerups %d, event %d, eventParm %d\n", es->number, es->eType, es->clientNum, es->weapon, es->eFlags, es->powerups, es->event, es->eventParm);

		es->eType = ET_PLAYER;
		es->clientNum = es->number;


		//es->weapon &= 0xf;
		//es->weapon = WP_GAUNTLET;
		//es->weapon = es->weapon % 11 + 1;
		//es->weapon = es->weapon % 10 + 1;
		es->weapon = es->weapon % 11;
		//Com_Printf("new weapon %d\n", es->weapon);
		//es->powerups = 0;
	}
#endif
}

/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void CG_SetNextSnap( snapshot_t *snap ) {
	int					num;
	entityState_t		*es;
	centity_t			*cent;
	int r;
	int t;
	int n;
	int i;
	projectile_t *proj;

	//Com_Printf("set next snap\n");
	cg.nextSnap = snap;

	cg.numProjectiles = 0;

    if (cg.snap) {
        memcpy(&wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP], cg.snap, sizeof(snapshot_t));
        wcg.curSnapshotNumber++;

		for (i = 0;  i < cg.snap->numEntities;  i++) {
			es = &cg.snap->entities[i];

			if (es->eType == ET_MISSILE) {
				proj = &cg.projectiles[cg.numProjectiles];

				memcpy(&proj->pos, &es->pos, sizeof(proj->pos));
				VectorCopy(es->angles, proj->angles);

				proj->numUses = 0;
				proj->weapon = es->weapon;

				cg.numProjectiles++;
			}
		}

		if (cg.demoPlayback) {
			trap_GetCurrentSnapshotNumber(&n, &t);
			//r = CG_PeekSnapshot(cgs.processedSnapshotNum + 2, &cg.nextNextSnap);
			cg.nextNextSnap = &cg.nnSnap;
			r = CG_PeekSnapshot(n + 1, cg.nextNextSnap);
			if (!r) {
				//Com_Printf("couldn't get nextNextSnap\n");
				cg.nextNextSnapValid = qfalse;
			} else {
				cg.nextNextSnapValid = qtrue;
			}

			Wolfcam_NextSnapShotSet();
		}
    }

    if (cg.snap) {
		int oldTime;

		oldTime = wcg.snaps[(wcg.curSnapshotNumber - 1) % MAX_SNAPSHOT_BACKUP].serverTime;
		if (cg.snap->serverTime - oldTime > 33) {
			//CG_Printf ("%d\n", cg.snap->serverTime - oldTime);
		}
        //memcpy (&wcg.oldSnap, cg.snap, sizeof (snapshot_t));
        //wcg.oldSnapValid = qtrue;
        //memcpy (&wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP], cg.snap, sizeof(snapshot_t));
        //wcg.curSnapshotNumber++;
        //CG_Printf ("snapshot: %d\n", wcg.curSnapshotNumber);
    } else {
        //wcg.oldSnapValid = qfalse;
    }

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ ) {
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy(&cent->nextState, es, sizeof(entityState_t));
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			//Com_Printf("set next snap %d interp false teleport\n", es->number);
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		cg.nextFrameTeleport = qtrue;
	} else {
		cg.nextFrameTeleport = qfalse;
	}

	// if changing follow mode, don't interpolate
	if ( cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum ) {
		cg.nextFrameTeleport = qtrue;
	}

	// if changing server restarts, don't interpolate
	if ( ( cg.nextSnap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
		cg.nextFrameTeleport = qtrue;
	}

	if (cg.demoPlayback  &&  cg.snap  &&  cg.nextSnap) {
		const playerState_t *ops;
		const playerState_t *ps;
		float diff;

		ops = &cg.snap->ps;
		ps = &cg.nextSnap->ps;

		// step
		//if (cg.thisFrameTeleport) {
		if (!((ps->eFlags ^ ops->eFlags) & EF_TELEPORT_BIT)  &&  ps->clientNum == ops->clientNum) {
			if (ps->groundEntityNum == ENTITYNUM_WORLD  &&  ops->groundEntityNum == ENTITYNUM_WORLD) {
				diff = ps->origin[2] - ops->origin[2];
				if (diff > 2) {  // high fall can trigger step event <= 2
					//FIXME duplicate code
					float oldStep;
					int delta;
					int step;

					//Com_Printf("^3%f snap step %f\n", cg.ftime, diff);
					//delta = cg.time - cg.stepTime;
					delta = cg.snap->serverTime - cg.stepTime;
					if (delta < cg_stepSmoothTime.value) {
						oldStep = cg.stepChange * (cg_stepSmoothTime.value - delta) / cg_stepSmoothTime.value;
					} else {
						oldStep = 0;
					}

					// add this amount
					//step = 4 * (event - EV_STEP_4 + 1);
#if 0
					if (diff > 22) {
						diff = 22;
					}
#endif
					step = diff;
					cg.stepChange = oldStep + step;
					if (cg.stepChange > cg_stepSmoothMaxChange.value) {
						cg.stepChange = cg_stepSmoothMaxChange.value;
					}
					//cg.stepTime = cg.time;
					cg.stepTime = cg.snap->serverTime;
				}
			}
		}

	}

	// sort out solid entities
	CG_BuildSolidList();
}

void CG_ResetTimeChange (int serverTime, int ioverf)
{
	int i;
	int n;
	qboolean ret;
	int ival;
	int highScore;
	int timeLeft;
	int skipCount;
	int lastSnapServerTime;
	int lastPing;
	centity_t *cent;
	entityState_t *state;
	int j;
	snapshot_t *snap;
	//qboolean rewinding;
	int snapshotBackups;
	int lastPickup;
	itemPickup_t ip;
	double lastftime;
	double timeDiff;

	if (!cg.demoPlayback) {
		return;
	}

	cg.demoSeeking = qtrue;

	if (cg.recordPath) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.recordPathFile = 0;
		Com_Printf("done recording path\n");
		cg.echoPopupStartTime = 0;
		cg.recordPath = qfalse;
	}

#if 0
	if (cg.time > serverTime) {
		rewinding = qtrue;
	} else {
		rewinding = qfalse;
	}
#endif

    lastftime = cg.ftime;

	cg.time = serverTime;
	cg.ioverf = ioverf;
	cg.foverf = (double)ioverf / SUBTIME_RESOLUTION;
	cg.ftime = (double)cg.time + cg.foverf;

	// q3mme camera and dof
	demo.play.time = cg.time;
	demo.play.fraction = (float)cg.foverf;
	demo.serverTime = serverTime;

	timeDiff = cg.ftime - lastftime;

	if (cg.snap  &&  cg.nextSnap) {
		if (cg.ftime >= (double)cg.snap->serverTime  &&  cg.ftime < (double)cg.nextSnap->serverTime) {
			// snaps still valid
			return;
		}
	}

	trap_GetCurrentSnapshotNumber(&n, &cg.latestSnapshotTime);
	//Com_Printf("%d (%d) -> %d\n", cg.snap->serverTime, cg.time, cg.latestSnapshotTime);
	//CG_ResetLagometer();

	//FIXME offline demos and cg_checkForOfflineDemo
	//FIXME has high as backup snapshots?
	if (n == cgs.processedSnapshotNum + 1) {
		//Com_Printf("transition\n");
		return;
	}

	//cgs.processedSnapshotNum = n;

	cg.snap = NULL;
	cg.nextSnap = NULL;
	cg.nextNextSnap = NULL;

	trap_S_ClearLoopingSounds(qtrue);
	CG_ResetTimedItemPickupTimes();
	lastPickup = trap_GetItemPickupNumber(serverTime - cg.maxItemRespawnTime * 1000 - 1000);
	if (lastPickup > -1) {
		while (1) {
			if (trap_GetItemPickup(lastPickup, &ip) == -1) {
				break;
			}

			if ((double)ip.pickupTime > cg.ftime) {
				break;
			}
			lastPickup++;
			if (ip.spec) {
				//FIXME  sometimes 0 so just just pickupTime
				//ip.specPickupTime -=
				// titem->specPickupTime = es->time - (titem->respawnLength * 1000);
				if (ip.specPickupTime != 0) {
					CG_TimedItemPickup(ip.index, ip.origin, ip.clientNum, ip.specPickupTime, qtrue);
				}
				//CG_TimedItemPickup(ip.index, ip.origin, ip.clientNum, ip.pickupTime, qtrue);
			} else {
				CG_TimedItemPickup(ip.index, ip.origin, ip.clientNum, ip.pickupTime, qfalse);
				//continue;
			}
			//Com_Printf("item pickup:  (%d)  %d  %d  clientNum %d %s\n", cg.time, ip.pickupTime, ip.index, ip.clientNum, ip.spec ? "(spec)" : "");
		}
	}

	//memset(cg_entities, 0, sizeof(cg_entities));
	for (i = 0;  i < MAX_GENTITIES;  i++) {
		centity_t *cent;
		//lerpFrame_t legs, torso, flag;

		cent = &cg_entities[i];
		if (i < MAX_CLIENTS * 2) {
			//pe = cent->pe;
			memset(cent, 0, sizeof(*cent));

			//cent->pe = pe;
			//FIXME hack
#if 0
			if (cg.latestSnapshotTime >= origServerTime) {
				cent->pe.legs = pe.legs;
				cent->pe.torso = pe.torso;
				cent->pe.flag = pe.flag;
			}
#endif
			if (i < MAX_CLIENTS) {
				cent->currentState.clientNum = i;
				cent->currentState.number = i;
			}

		} else {
			memset(cent, 0, sizeof(*cent));
		}

		// let fx system reset
		cent->lastFlashIntervalTime = -1;
		cent->lastFlashDistanceTime = -1;
		cent->lastModelIntervalTime = -1;
		cent->lastModelDistanceTime = -1;
		cent->lastTrailIntervalTime = -1;
		cent->lastTrailDistanceTime = -1;
		cent->lastImpactIntervalTime = -1;
		cent->lastImpactDistanceTime = -1;
		//VectorCopy(cent->currentState.origin, cent->lastDistancePosition);
		//VectorCopy(cent->currentState.origin, cent->lastModelDistancePosition);
		cent->flightPositionData.intervalTime = -1;
		cent->flightPositionData.distanceTime = -1;
		cent->hastePositionData.intervalTime = -1;
		cent->hastePositionData.distanceTime = -1;
	}

	memset(&cg.predictedPlayerEntity, 0, sizeof(cg.predictedPlayerEntity));

	if (1) {  //(rewinding) {
		CG_InitLocalEntities();
		//CG_ClearLocalEntitiesTimeChange();
		CG_ClearParticles();
		CG_InitMarkPolys();
	}

	cg.thisFrameTeleport = qfalse;
	cg.nextFrameTeleport = qfalse;
	cg.infectedSnapshotTime = 0;

	memset(cgs.teamChatMsgTimes, 0, sizeof(cgs.teamChatMsgTimes));
	cgs.orderTime = 0;
	cgs.acceptOrderTime = 0;
	cgs.scrFadeStartTime = 0;
	cgs.firstPlace[0] = '\0';
	cgs.secondPlace[0] = '\0';
	trap_GetGameState(&cgs.gameState);
	CG_ParseServerinfo(qfalse, qtrue);
	//memset(cgs.clientinfo, 0, sizeof(cgs.clientinfo));
	//memset(cgs.clientinfoOrig, 0, sizeof(cgs.clientinfoOrig));
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		const char *clientString;
		//clientInfo_t *ci;

		//ci = &cgs.clientinfo[i];
		//ci->medkitUsageTime = 0;
		//ci->invulnerabilityStartTime = 0;
		//ci->invulnerabilityStopTime = 0;
		//ci->breathPuffTime = 0;

		//cgs.clientinfo[i].hitTime = 0;

		clientString = CG_ConfigString(CS_PLAYERS + i);
		if (!clientString[0]) {
			cgs.clientinfo[i].infoValid = qfalse;
			cgs.clientinfo[i].override = qfalse;
			continue;
		}
		CG_NewClientInfo(i);
		//CG_ResetPlayerEntity(&cg_entities[i]);  // CG_ResetPlayerEntity ?
	}
	memset(&cg.predictedPlayerState, 0, sizeof(playerState_t));
	//pe = cg.predictedPlayerEntity.pe;
	memset(&cg.predictedPlayerEntity, 0, sizeof(cg.predictedPlayerEntity));
	//cg.predictedPlayerEntity.pe = pe;  //FIXME hack
	cg.scoresValid = qfalse;
	CG_CreateScoresFromClientInfo();
	CG_BuildSpectatorString();
	cgs.lastConnectedDisconnectedPlayer = -1;
	cgs.needToCheckSkillRating = qfalse;
	cgs.serverCommandSequence = trap_GetLastExecutedServerCommand();

	CG_SetConfigValues();

	//FIXME should be in CG_SetConfigValues()
	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );

	cg.warmup = 0;
	CG_ParseWarmup();
	cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	cg.intermissionStarted = atoi(CG_ConfigString(CS_INTERMISSION));

	if (cgs.protocol == PROTOCOL_QL) {
		int roundTime;

		//Com_Printf("^2 round status: '%s'\n", CG_ConfigString(CS_ROUND_STATUS));
		if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER) {
			if (CG_ConfigString(CS_ROUND_STATUS)[0] == '\0'  /* dm_73 */
				||  Info_ValueForKey(CG_ConfigString(CS_ROUND_STATUS), "time")[0] == '\0'  /* newer ql demos */
				) {
				cgs.roundBeginTime = -999;
				cgs.roundNum = 0;  //-999;
			} else {
				cgs.roundBeginTime = atoi(Info_ValueForKey(CG_ConfigString(CS_ROUND_STATUS), "time"));
				cgs.roundNum = atoi(Info_ValueForKey(CG_ConfigString(CS_ROUND_STATUS), "round"));
			}

			roundTime = atoi(CG_ConfigString(CS_ROUND_TIME));
			if (roundTime > 0) {
				cgs.roundStarted = qtrue;
			} else {
				cgs.roundStarted = qfalse;
			}
		}
	} else {
		cgs.roundStarted = qfalse;
		cgs.roundBeginTime = 0;
		cgs.roundNum = 0;
	}

	if (cgs.protocol == PROTOCOL_QL) {
		cgs.timeoutBeginTime = atoi(CG_ConfigString(CS_TIMEOUT_BEGIN_TIME));
		cgs.timeoutEndTime = atoi(CG_ConfigString(CS_TIMEOUT_END_TIME));
		cgs.timeoutCountingDown = qfalse;
		cgs.redTeamTimeoutsLeft = atoi(CG_ConfigString(CS_RED_TEAM_TIMEOUTS_LEFT));
		cgs.blueTeamTimeoutsLeft = atoi(CG_ConfigString(CS_BLUE_TEAM_TIMEOUTS_LEFT));
		if ((cg.time < cgs.timeoutEndTime)  ||  (cgs.timeoutBeginTime  &&  cgs.timeoutEndTime == 0)) {
			CG_ResetTimedItemPickupTimes();  //FIXME change eventually
		}
	} else {
		cgs.timeoutBeginTime = 0;
		cgs.timeoutEndTime = 0;
		cgs.timeoutCountingDown = qfalse;
		cgs.redTeamTimeoutsLeft = 0;
		cgs.blueTeamTimeoutsLeft = 0;
	}

	if (cgs.cpma) {
		const char *s;
		// correct level start time, round info, timeouts
		//cgs.cpmaLastTd = 0;
		cgs.cpmaLastTe = 0;
		CG_CpmaParseGameState(qfalse);
		s = CG_ConfigStringNoConvert(CSCPMA_GAMESTATE);
		if (atoi(Info_ValueForKey(s, "te"))) {
#if 0
			Com_Printf("in timeout %d\n", serverTime);
			for (i = 0;  i < cgs.numTimeouts;  i++) {
				timeOut_t *t;

				t = &cgs.timeOuts[i];
				Com_Printf("^5%d  %d -> %d\n", i, t->startTime, t->endTime);

				if (t->endTime < serverTime) {
					continue;
				}
				if (t->startTime > serverTime) {
					continue;
				}
				if (t->startTime <= serverTime  &&  t->endTime >= serverTime) {
					Com_Printf("^3in timeout #%d\n", i + 1);
					//cgs.cpmaLastTd = t->cpmaTd;
					//CG_CpmaParseGameState(qtrue);  // bah
					break;
				}
			}
#endif
			CG_ResetTimedItemPickupTimes();  //FIXME change eventually
		}
		//CG_Printf("^4seeking 1\n");
		CG_CpmaParseScores(qtrue);
	}

	//FIXME clear other stuff as well?
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclients[i].landTime = 0;
		wclients[i].jumpTime = 0;
	}

	cg.centerPrintLines = 0;
	cg.lastWeapon = -1;

	//FIXME all the cg.*time* garbage
	cg.frameInterpolation = 0;
	//FIXME cg.oldTime, cg.time, cg.physicsTime
	//cg.time = 0;
	//cg.frametime = cg.time;
	cg.oldTime = cg.time;
	cg.vibrateCameraTime = 0;
	cg.vibrateCameraValue = 0;
	cg.vibrateCameraPhase = 0;

	cg.validPPS = qfalse;
	cg.predictedErrorTime = 0;
	cg.stepTime = 0;
	cg.duckTime = 0;
	cg.landTime = 0;
	//cg.zoomTime = 0;
	cg.zoomTime += timeDiff;
	cg.zoomBrokenTime += timeDiff;
	cg.scoresRequestTime = 0;
	cg.scoreFadeTime = 0;
	cg.duelScoresServerTime = 0;
	cg.tdmScore.serverTime = 0;
	cg.ctfScore.serverTime = 0;
	cg.duelScoresValid = qfalse;
	cg.tdmScore.valid = qfalse;
	cg.ctfScore.valid = qfalse;
	cg.spectatorTime = 0;
	cg.centerPrintTime = 0;
	cg.crosshairClientTime = -(cg_drawCrosshairNamesTime.integer + 2000);  //FIXME 2000, 1000 fixed number
	cg.powerupTime = 0;
	cg.attackerTime = 0;
	cg.voiceTime = 0;
	cg.rewardTime = 0;
	cg.soundTime = 0;
#ifdef MISSIONPACK
	cg.voiceChatTime = 0;
#endif
	cg.itemPickupTime = 0;
	cg.itemPickupClockTime = 0;
	cg.weaponSelectTime = 0;
	cg.weaponAnimationTime = 0;
	cg.damageTime = 0;
	cg.headEndTime = 0;
	cg.headStartTime = 0;
	cg.v_dmg_time = 0;
	cg.nextOrbitTime = 0;
	cg.pingCalculateTime = 0;
	cg.lastChatBeepTime = 0;
	cg.lastFragTime = 0;
	memset(&cg.obituaries, 0, sizeof(cg.obituaries));
	cg.obituaryIndex = 0;
	cg.lastTeamChatBeepTime = 0;
	memset(cg.chatAreaStringsTime, 0, sizeof(cg.chatAreaStringsTime));
	cg.chatAreaStringsIndex = 0;
	//cg.fMoveTime = 0;
	cg.freecamFireTime = 0;
	cg.damageDoneTime = 0;
	cg.serverAccuracyStatsTime = 0;
	cg.serverAccuracyStatsClientNum = -1;
	cg.proxTime = 0;
	cg.proxCounter = 0;
	cg.proxTick = 0;
	//cgs.cpmaTimeoutTime = 0;
	cg.duelForfeit = qfalse;

	if (CG_IsDuelGame(cgs.gametype)) {
		CG_SetDuelPlayers();
	}
	//cg.echoPopupStartTime = 0;
	//cg.echoPopupTime = 0;
	//cg.cameraWaitToSync = qfalse;
	//cg.cameraJustStarted = qfalse;

	cg.latestSnapshotNum = n;
	cgs.processedSnapshotNum = n - 1;

	//memset(cg.activesnapshots, 0, sizeof(snapshot_t) * 2);

	//FIXME wcg. stuff
	skipCount = 0;
	lastSnapServerTime = 0;
	lastPing = 0;
	wcg.curSnapshotNumber = 0;
	cg.nextNextSnapValid = qfalse;

	snapshotBackups = 10;

	for (i = -snapshotBackups;  i <= 0;  i++) {
		int snapNum;
		qboolean checkNoMove = qfalse;

		cg.nextNextSnapValid = qfalse;
		snapNum = (n - 0) + i + skipCount;
		if (snapNum < cgs.firstServerMessageNum) {
			break;
		}
		snap = &wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP];
		ret = CG_GetSnapshot(snapNum, snap);
		cgs.processedSnapshotNum = snapNum;
		if (ret) {
			if (snap->serverTime == lastSnapServerTime) {
				snap->ping = lastPing;
			} else if (cg.serverFrameTime != 99999) {
				snap->ping = (snap->serverTime - snap->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
			} else {
				snap->ping = (snap->serverTime - snap->ps.commandTime) - 25 - 25 / 2.0;
			}
			lastPing = snap->ping;

			// offline demos with snaps having same server time  //FIXME problem with demosmoothing
			if (cg_checkForOfflineDemo.integer == 0  ||  wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP].serverTime > lastSnapServerTime) {
				lastSnapServerTime = wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP].serverTime;
				if (i < 0) {  // last one not saved, since CG_SetNextSnap() does it
					wcg.curSnapshotNumber++;
				}

				if (!cg.snap) {
					cg.snap = &wcg.snaps[(wcg.curSnapshotNumber - 1) % MAX_SNAPSHOT_BACKUP];
				} else if (!cg.nextSnap) {
					cg.nextSnap = &wcg.snaps[(wcg.curSnapshotNumber - 1) % MAX_SNAPSHOT_BACKUP];
				} else if (!cg.nextNextSnap) {
					cg.nextNextSnap = &wcg.snaps[(wcg.curSnapshotNumber - 1) % MAX_SNAPSHOT_BACKUP];
					checkNoMove = qtrue;
					cg.nextNextSnapValid = qtrue;
				} else {
					checkNoMove = qtrue;
					cg.nextNextSnapValid = qtrue;

					cg.snap = cg.nextSnap;
					cg.nextSnap = cg.nextNextSnap;
					cg.nextNextSnap = &wcg.snaps[(wcg.curSnapshotNumber - 1) % MAX_SNAPSHOT_BACKUP];
				}

				if (checkNoMove) {

					for (j = 0;  j < MAX_GENTITIES;  j++) {
						cg_entities[j].inCurrentSnapshot = qfalse;
						cg_entities[j].inNextSnapshot = qfalse;
						cg_entities[j].currentValid = qfalse;
					}

					cg_entities[cg.snap->ps.clientNum].inCurrentSnapshot = qtrue;
					cg_entities[cg.snap->ps.clientNum].currentValid = qtrue;

					for (j = 0;  j < cg.snap->numEntities;  j++) {
						cg_entities[cg.snap->entities[j].number].inCurrentSnapshot = qtrue;
						cg_entities[cg.snap->entities[j].number].currentValid = qtrue;
					}

					cg_entities[cg.nextSnap->ps.clientNum].inNextSnapshot = qtrue;
					for (j = 0;  j < cg.nextSnap->numEntities;  j++) {
						cg_entities[cg.nextSnap->entities[j].number].inNextSnapshot = qtrue;
					}

					//cg.time = cg.snap->serverTime;
					Wolfcam_CheckNoMove();
				}
			}
		} else {
			cg.nextNextSnapValid = qfalse;
			Com_Printf("CG_ResetTimeChange() couldn't get snapshot %d\n", snapNum);
		}
	}

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cent = &cg_entities[i];
		cent->currentValid = qfalse;
		cent->inCurrentSnapshot = qfalse;
		cent->inNextSnapshot = qfalse;
		cent->interpolate = qfalse;
	}

	cg.snap = &cg.activeSnapshots[0];

	if (!cg.nextSnap) {
		CG_Printf("CG_ResetTimeChange() couldn't get first snapshot\n");
		ret = CG_GetSnapshot(n, cg.snap);
		if (!ret) {
			CG_Printf("FIXME shouldn't happen CG_ResetTimeChange() couldn't get subsequent snapshot\n");
			cg.snap = NULL;
			CG_Abort();
		}
		if (cg.serverFrameTime != 99999) {
			cg.snap->ping = (cg.snap->serverTime - cg.snap->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
		} else {
			cg.snap->ping = (cg.snap->serverTime - cg.snap->ps.commandTime) - 25 - 25 / 2.0;
		}
		cg.nextSnap = NULL;
		cg.latestSnapshotNum = n;
		cgs.processedSnapshotNum = n;
	} else {
		int legsAnimTime;
		int torsoAnimTime;
		float speedScale;
		int tm;
		lerpFrame_t *lerp;
		animation_t *anim;
		clientInfo_t *ci;

		*cg.snap = *cg.nextSnap;

		//Com_Printf("got next snap\n");
		if (cg.serverFrameTime != 99999) {
			cg.snap->ping = (cg.snap->serverTime - cg.snap->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
		} else {
			cg.snap->ping = (cg.snap->serverTime - cg.snap->ps.commandTime) - 25 - 25 / 2.0;
		}

		if (cgs.cpma) {
			//FIXME hack since we use nextSnap to calc score placement :(
			CG_ParseServerinfo(qfalse, qtrue);

			// hack to avoid player/drawing FIGHT after score changes
			cgs.roundStarted = qfalse;
			if (cg.snap->serverTime > cgs.roundBeginTime) {
				cgs.roundBeginTime = 0;
			}
		}

		// don't do it if setting next snap in this function
		//memcpy(&wcg.snaps[wcg.curSnapshotNumber % MAX_SNAPSHOT_BACKUP], cg.snap, sizeof(snapshot_t));
        //wcg.curSnapshotNumber++;

		// --- CG_SetInitialSnapshot() ---
		cent = &cg_entities[cg.snap->ps.clientNum];
		state = &cent->currentState;
		BG_PlayerStateToEntityState(&cg.snap->ps, state, qfalse);
		cent->interpolate = qfalse;
		//FIXME anim
		//FIXME flag
		ci = &cgs.clientinfo[cg.snap->ps.clientNum];
		legsAnimTime = trap_GetLegsAnimStartTime(cg.snap->ps.clientNum);
		torsoAnimTime = trap_GetTorsoAnimStartTime(cg.snap->ps.clientNum);

		if (state->powerups & (1 << PW_HASTE)) {
			speedScale = 1.5;
		} else {
			speedScale = 1;
		}

		if (CG_FreezeTagFrozen(state->number)) {
			speedScale = 0;
		}

		lerp = &cent->pe.legs;
		CG_ClearLerpFrame(ci, lerp, state->legsAnim, legsAnimTime);
		anim = &ci->animations[state->legsAnim];
		if (anim) {
			for (tm = legsAnimTime;  tm < cg.time;  tm += 8) {
				CG_RunLerpFrame(ci, lerp, state->legsAnim, speedScale, tm);
			}
			//Com_Printf("cg.time %d  ent %d seeking oldFrameTime %d  frameTime %d  lf->frame %d lf->animationTime %d\n", cg.time, state->number, lerp->oldFrameTime, lerp->frameTime, lerp->frame, lerp->animationTime);
		}

		lerp = &cent->pe.torso;
		CG_ClearLerpFrame(ci, lerp, state->torsoAnim, torsoAnimTime);
		anim = &ci->animations[state->torsoAnim];
		if (anim) {
			for (tm = torsoAnimTime;  tm < cg.time;  tm += 8) {
				CG_RunLerpFrame(ci, lerp, state->torsoAnim, speedScale, tm);
			}
		}
		memcpy(&cg.predictedPlayerEntity, cent, sizeof(centity_t));

		//     CG_BuildSolidList();   // makes no sense in setinitialsnap()

		//     CG_Respawn()
		// no error decay on player movement
        //cg.thisFrameTeleport = qtrue;

        // display weapons available
        cg.weaponSelectTime = cg.snap->serverTime;

        // select the weapon the server says we are using
        cg.weaponSelect = cg.snap->ps.weapon;

		for (i = 0;  i < MAX_GENTITIES;  i++) {
			cg_entities[i].inCurrentSnapshot = qfalse;
			cg_entities[i].currentValid = qfalse;
		}

		cg_entities[cg.snap->ps.clientNum].inCurrentSnapshot = qtrue;
		//cg_entities[cg.nextSnap->ps.clientNum].inNextSnapshot = qtrue;
		cg_entities[cg.snap->ps.clientNum].currentValid = qtrue;

		for (i = 0;  i < cg.snap->numEntities;  i++) {
			state = &cg.snap->entities[i];
			cent = &cg_entities[state->number];

			memcpy(&cent->currentState, state, sizeof(entityState_t));
			cent->interpolate = qfalse;
			cent->currentValid = qtrue;
			cent->inCurrentSnapshot = qtrue;

			// check for events
			//CG_CheckEvents( cent );

			if (state->eType != ET_PLAYER) {
				continue;
			}

			//FIXME flag
			ci = &cgs.clientinfo[state->clientNum];

			//Com_Printf("%d:  legs:%d  torso:%d\n", state->number, trap_GetLegsAnimStartTime(state->number), trap_GetTorsoAnimStartTime(state->number));
			legsAnimTime = trap_GetLegsAnimStartTime(state->number);
			torsoAnimTime = trap_GetTorsoAnimStartTime(state->number);

			if (state->powerups & (1 << PW_HASTE)) {
				speedScale = 1.5;
			} else {
				speedScale = 1;
			}

			if (CG_FreezeTagFrozen(state->number)) {
				speedScale = 0;
			}

			lerp = &cent->pe.legs;
			CG_ClearLerpFrame(ci, lerp, state->legsAnim, legsAnimTime);
			anim = &ci->animations[state->legsAnim];
			if (anim) {
				for (tm = legsAnimTime;  tm < cg.time;  tm += 8) {
					CG_RunLerpFrame(ci, lerp, state->legsAnim, speedScale, tm);
				}
				//Com_Printf("cg.time %d  ent %d seeking oldFrameTime %d  frameTime %d  lf->frame %d lf->animationTime %d\n", cg.time, state->number, lerp->oldFrameTime, lerp->frameTime, lerp->frame, lerp->animationTime);
			}

			lerp = &cent->pe.torso;
			CG_ClearLerpFrame(ci, lerp, state->torsoAnim, torsoAnimTime);
			anim = &ci->animations[state->torsoAnim];
			if (anim) {
				for (tm = torsoAnimTime;  tm < cg.time;  tm += 8) {
					CG_RunLerpFrame(ci, lerp, state->torsoAnim, speedScale, tm);
				}
			}
		}

		// ----------------------------------

		cg.latestSnapshotNum = n;
		cgs.processedSnapshotNum = n - 1;
		cg.nextSnap = NULL;
		cg.nextNextSnapValid = qfalse;
	}

	//
	// sound warnings, count downs, etc.
	//

	if (cg.snap  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
		CG_PlayWinLossMusic();
	} else {
		CG_StartMusic();
	}

	cgs.thirtySecondWarningPlayed = qfalse;
	if (cgs.roundStarted  &&  cgs.protocol == PROTOCOL_QL) {
		ival = cg.time - atoi(CG_ConfigString(CS_ROUND_TIME));
		if (cgs.roundtimelimit - (ival / 1000) < 30) {
			cgs.thirtySecondWarningPlayed = qtrue;
		}
	} else {
		cgs.thirtySecondWarningPlayed = qtrue;
	}

	// round count down
	if (!cgs.roundStarted) {
		timeLeft = (cgs.roundBeginTime - cg.time) / 1000;
		timeLeft++;

		cgs.countDownSoundPlayed = -999;
		//Com_Printf("timeleft %d\n", timeLeft);
		if (timeLeft >= 0  &&  timeLeft <= 5) {
			cgs.countDownSoundPlayed = timeLeft;
		}
	}

	cg.timelimitWarnings = 0;
	cg.fraglimitWarnings = 0;
	//FIXME copy paste from cg_playerstate.c

	if (cgs.realTimelimit > 0) {
		int msec;

		msec = cg.time - cgs.levelStartTime;

		if ( !( cg.timelimitWarnings & 4 ) && msec > ( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 1 | 2 | 4;
			//CG_StartLocalSound( cgs.media.suddenDeathSound, CHAN_ANNOUNCER );
		}
		else if ( !( cg.timelimitWarnings & 2 ) && msec > (cgs.timelimit - 1) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1 | 2;
			//CG_StartLocalSound( cgs.media.oneMinuteSound, CHAN_ANNOUNCER );
		}
		else if ( cgs.timelimit > 5 && !( cg.timelimitWarnings & 1 ) && msec > (cgs.timelimit - 5) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			//CG_StartLocalSound( cgs.media.fiveMinuteSound, CHAN_ANNOUNCER );
		}
	}

	// fraglimit warnings
	if (cgs.fraglimit > 0  &&  (cgs.gametype == GT_FFA  ||  cgs.gametype == GT_TOURNAMENT  ||  cgs.gametype == GT_SINGLE_PLAYER  ||  cgs.gametype == GT_TEAM)) {
		highScore = cgs.scores1;
		if ( !( cg.fraglimitWarnings & 4 ) && highScore == (cgs.fraglimit - 1) ) {
			cg.fraglimitWarnings |= 1 | 2 | 4;
			//CG_AddBufferedSound(cgs.media.oneFragSound);
		}
		else if ( cgs.fraglimit > 2 && !( cg.fraglimitWarnings & 2 ) && highScore == (cgs.fraglimit - 2) ) {
			cg.fraglimitWarnings |= 1 | 2;
			//CG_AddBufferedSound(cgs.media.twoFragSound);
		}
		else if ( cgs.fraglimit > 3 && !( cg.fraglimitWarnings & 1 ) && highScore == (cgs.fraglimit - 3) ) {
			cg.fraglimitWarnings |= 1;
			//CG_AddBufferedSound(cgs.media.threeFragSound);
		}
	}

	cg.rewardStack = 0;

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cg_entities[i].trailTime = cg.time;
	}

	//CG_Respawn();

	cg.menuScoreboard = NULL;
	//CG_ResetLagometer();

	//FIXME force
	//CG_ClearFxExternalForces();

	//Com_Printf("teleport this: %d  next: %d\n", cg.thisFrameTeleport, cg.nextFrameTeleport);


	// get alive status for scoreboard

	if (cgs.protocol == PROTOCOL_QL  ||  cgs.cpma) {
		int roundStartTime = 0;

		for (i = cg.numRoundStarts - 1;  i >= 0;  i--) {
			if (cg.roundStarts[i] < cg.time) {
				roundStartTime = cg.roundStarts[i];
				break;
			}
		}

		//Com_Printf("^6round start time: %d\n", roundStartTime);

		if (roundStartTime > 0) {
			for (i = 0;  i < MAX_CLIENTS;  i++) {
				int killerClientNum;
				int killTime;
				int teamSwitchTime;

				wclients[i].aliveThisRound = qtrue;
				if (trap_GetNextKiller(i, roundStartTime, &killerClientNum, &killTime, qfalse)) {
					if (killTime <= cg.time) {
						wclients[i].aliveThisRound = qfalse;
						//Com_Printf("^5%d already died this round (%s)\n", i, cgs.clientinfo[i].name);
					}
				}

				if (trap_GetTeamSwitchTime(i, roundStartTime, &teamSwitchTime)) {
					//Com_Printf("^2switch time: %d %d\n", i, teamSwitchTime);
					if (teamSwitchTime <= cg.time) {
						//Com_Printf("^5got team switch for %d '%s'\n", i, cgs.clientinfo[i].name);
						wclients[i].aliveThisRound = qfalse;
					}
				}
			}
		}
	}

	if (SC_Cvar_Get_Int("debug_seek")) {
		Com_Printf("cgame: reset time change %f  snap->serverTime %d", cg.ftime, cg.snap->serverTime);
		if (cg.nextSnap) {
			Com_Printf(" nextSnap->serverTime %d\n", cg.nextSnap->serverTime);
		} else {
			Com_Printf("\n");
		}
	}
}

/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean	r;
	snapshot_t	*dest;
	int	count;
	//snapshot_t xxx;

	if (SC_Cvar_Get_Int("debug_snapshots")) {
		CG_Printf("Read next snapshot: %d  %d\n", cg.latestSnapshotNum, cgs.processedSnapshotNum);
	}

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 ) {
#if 0
		CG_Printf( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
			cg.latestSnapshotNum, cgs.processedSnapshotNum );
#endif
	}

	count = 0;

	if (cgs.processedSnapshotNum >= cg.latestSnapshotNum  &&  !SC_Cvar_Get_Int("cl_freezeDemo")  &&  !(SC_Cvar_Get_Float("timescale") > 1.0)) {
		// happens with listen server
		//CG_Printf("readnextsnapshot()  cgs.processedSnapshotNum >= cg.latestSnapshotNum  %d > %d\n", cgs.processedSnapshotNum, cg.latestSnapshotNum);
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
		count++;
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else {
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		if (SC_Cvar_Get_Int("debug_snapshots")) {
			CG_Printf("%d getting snapshot %d  latest is %d, offset is %d\n", count, cgs.processedSnapshotNum, cg.latestSnapshotNum, cg.snapshotOffset);
		}

		if (cg.latestSnapshotNum - cgs.processedSnapshotNum >= PACKET_BACKUP) {
			// high timescale value, don't even bother trying to get
			r = qfalse;
		} else {
			r = CG_GetSnapshot( cgs.processedSnapshotNum, dest );

			if (!r) {
				//CG_Printf("cgame: couldn't get snapshot %d from engine, latest %d\n", cgs.processedSnapshotNum, cg.latestSnapshotNum);
			}
		}

		// FIXME: why would trap_GetSnapshot return a snapshot with the same
		//        server time --  wolfcam:  because of listen servers (ex:
		//        sv_fps 25 with com_maxfps 125 server sends 125 snaps with
		//        every 5 having same server time)

#if 0
		if ( cg.snap && r && dest->serverTime == cg.snap->serverTime ) {
			//continue;
		}
#endif

		// if it succeeded, return
		if ( r ) {
			static int lastServerTime = 0;
			static int lastPing = 0;

			if (dest->ps.commandTime <= 0) {
				//Com_Printf("^1ps.commandTime <= 0 (%d)\n", dest->ps.commandTime);
			}
			if (dest->serverTime == lastServerTime) {
				dest->ping = lastPing;
				//Com_Printf("%d reusing ping %d\n", dest->serverTime, lastPing);
			} else if (cg.serverFrameTime != 99999) {
				dest->ping = (dest->serverTime - dest->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
			} else {  // something for first ping
				dest->ping =  (dest->serverTime - dest->ps.commandTime) - 25 - 25 / 2.0;
			}
			lastServerTime = dest->serverTime;
			lastPing = dest->ping;
			//Com_Printf("^5%d -> %d  ping %d\n", dest->serverTime, dest->ps.commandTime, dest->ping);
			CG_AddLagometerSnapshotInfo( dest );
			if (SC_Cvar_Get_Int("debug_snapshots")) {
				//Com_Printf("readnextsnap() got snap %d  serverTime %d\n", cgs.processedSnapshotNum++, dest->serverTime);
				Com_Printf("readnextsnap() got snap %d  serverTime %d\n", cgs.processedSnapshotNum, dest->serverTime);
			}
			return dest;
		}

		if (cg.demoPlayback) {
			//CG_AddLagometerSnapshotInfo( dest );
			//return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	//Com_Printf("looped out\n");
	// nothing left to read
	return NULL;
}

static void CG_TemporarySnapshotTransition (snapshot_t *snap)
{
	int num;
	const entityState_t *es;
	centity_t *cent;
	int i;
	snapshot_t *oldFrame;

	oldFrame = cg.snap;


	//FIXME check if really needed
	//BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	//FIXME why qtrue?
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;
	//cg_entities[snap->ps.clientNum].currentState = cg_entities[snap->ps.clientNum].nextState;

	//BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );
	//BG_PlayerStateToEntityState( &snap->ps, &cg.predictedPlayerEntity.currentState, qfalse );

	for (num = 0;  num < snap->numEntities;  num++) {
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy(&cent->nextState, es, sizeof(entityState_t));
		//cent->currentState = cent->nextState;
		//cent->currentValid = qtrue;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}

		// if the next frame is a teleport for the playerstate, we
        // can't interpolate during demos
        if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
			cg.nextFrameTeleport = qtrue;
			//Com_Printf("teleport\n");
        } else {
			cg.nextFrameTeleport = qfalse;
        }

        // if changing follow mode, don't interpolate
        if ( snap->ps.clientNum != cg.snap->ps.clientNum ) {
			cg.nextFrameTeleport = qtrue;
        }

        // if changing server restarts, don't interpolate
        if ( ( snap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
			cg.nextFrameTeleport = qtrue;
			//Com_Printf("snap flags teleport\n");
        }

        // sort out solid entities


		//CG_CheckEvents( cent );
	}
	CG_BuildSolidList();

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclients[i].killedOrDied = qfalse;
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// snap is now cg.snap
	// cg.snap is now oldFrame

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );

	for (num = 0;  num < snap->numEntities;  num++) {
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		//CG_CheckEvents( cent );
		CG_TransitionEntity(cent);
	}


	if (oldFrame) {
		playerState_t *ops;
		const playerState_t *ps;

		ops = &oldFrame->ps;
		ps = &snap->ps;

		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;   // will be cleared by prediction code
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		CG_TransitionPlayerState( ps, ops );
	}

	//CG_CheckEvents(&cg.predictedPlayerEntity);
	//CG_CheckEvents(&cg_entities[snap->ps.clientNum]);

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclient_t *wc;

		wc = &wclients[i];
		if (wc->killedOrDied == qfalse) {
			continue;
		}
		memcpy(&wc->lastKillwstats, &wc->perKillwstats, sizeof(wc->lastKillwstats));
		memset(&wc->perKillwstats, 0, sizeof(wc->perKillwstats));
	}
}

/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
//static snapshot_t sn;

void CG_ProcessSnapshots( void ) {
	snapshot_t		*snap;
	int				n;

#if 0
	if (cg.demoSeeking) {
		return;
	}
	if (SC_Cvar_Get_Int("cl_freezeDemo")) {
		return;
	}
#endif

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	//CG_Printf("getting latestsnapshotnum %d\n", n);

	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			if (!cg.demoPlayback) {
				CG_Error( "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
			} else {
				// rewind
				CG_Printf("FIXME CG_ProcessSnapshots: n < cg.latestSnapshotNum  %d  %d\n", n, cg.latestSnapshotNum);
				//CG_ResetTimeChange();
			}
		} else {   //(!cg.snap  ||  !cg.nextSnap) {
			//CG_Printf("n latest  %d  %d\n", n, cg.latestSnapshotNum);
			cg.latestSnapshotNum = n;
		}
	}
	//cg.latestSnapshotNum += cg.snapshotOffset;

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			//Com_Printf("couldn't get initial snapshot\n");
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
			CG_SetInitialSnapshot( snap );
			//CG_Printf("set initial snapshot %d\n", cgs.processedSnapshotNum);
			trap_SendConsoleCommand("exec cgamepostinit.cfg\n");
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			//CG_Printf("trying to get nextsnap %d\n", cgs.processedSnapshotNum);
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				if (cg.demoPlayback  &&  !SC_Cvar_Get_Int("cl_freezeDemo")  &&  !cg.demoWaitingForStream  &&  SC_Cvar_Get_Float("timescale") == 1.0) {
					CG_Printf("processsnapshots()  couldn't get nextsnap %d\n", cgs.processedSnapshotNum);
				}
				break;
			}

			//Com_Printf("snap->serverTime %d\n", snap->serverTime);
			if (cg.demoPlayback  &&  cg_checkForOfflineDemo.integer  &&  snap->serverTime == cg.snap->serverTime) {

				if (SC_Cvar_Get_Int("debuglisten")) {
					Com_Printf("^5listen server %d\n", snap->serverTime);
				}
				cg.offlineDemoSkipEvents = qtrue;
				CG_TemporarySnapshotTransition(snap);
				//CG_SetNextSnap(snap);
				//CG_TransitionSnapshot();
				//cg.nextSnap = NULL;
				continue;
			}
			cg.offlineDemoSkipEvents = qfalse;
			CG_SetNextSnap( snap );


			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				if (!cg.demoPlayback) {
					CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
				} else {
					CG_Printf("CG_ProcessSnapshots: Server time went backwards\n");
				}
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
			//CG_Printf("our time is < nextFrame's  cgtime: %d\n", cg.time);
			break;
		} else {
			if (cg.demoPlayback  &&  cg.time < cg.nextSnap->serverTime) {
				// don't extrapolate with demos
				//CG_Printf("os %d   cg.time %d   ns %d  foverf %f\n", cg.snap->serverTime, cg.time, cg.nextSnap->serverTime, cg.foverf);
				break;
			}
		}

		// we have passed the transition from nextFrame to frame
		//Com_Printf("real trans cg.time %d  cg.snap->serverTime %d  cg.nextSnap->serverTime %d\n", cg.time, cg.snap->serverTime, cg.nextSnap->serverTime);
		CG_TransitionSnapshot();
		Wolfcam_MarkValidEntities();
	} while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		if (!cg.demoPlayback) {
			CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
		}
		return;
	}
	if ( cg.time < cg.snap->serverTime ) {
		// this can happen right after a vid_restart
		//Com_Printf("cg.time < cg.snap->serverTime  %d  %d\n", cg.time, cg.snap->serverTime);
		cg.time = cg.snap->serverTime;
	}
	if (cg.demoPlayback) {
		//cg.time = cg.snap->serverTime;
		//CG_Printf("snap serverTime %d\n", cg.snap->serverTime);
	}

	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		//Com_Printf("CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time %d %d\n", cg.nextSnap->serverTime, cg.time);
		if (!cg.demoPlayback) {
			CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
		}
	}

}

qboolean CG_GetSnapshot (int snapshotNumber, snapshot_t *snapshot)
{
	qboolean r;
	entityState_t *es;
	int i;

	r = trap_GetSnapshot(snapshotNumber, snapshot);
	if (!r) {
		//Com_Printf("^1couldn't get snapshot %d\n", snapshotNumber);
	}

	if (cgs.ospEncrypt) {
		for (i = 0;  i < snapshot->numEntities;  i++) {
			es = &snapshot->entities[i];

			if (es->number < MAX_CLIENTS) {
				es->pos.trBase[0] += (677 - 7 * es->number);
				es->pos.trBase[1] += (411 - 12 * es->number);
				es->pos.trBase[2] += (243 - 2 * es->number);
			}
		}
	}

#if 0  // testing
	if (CG_IsCpmaMvd()) {
		for (i = 0;  i < snapshot->numEntities;  i++) {
			es = &snapshot->entities[i];

			//Com_Printf("ent %d  :::  %d\n", es->number, es->clientNum);

			if (es->number < MAX_CLIENTS) {
				CG_FixCpmaMvdEntity(es);
			}
		}
	}
#endif

	return r;
}

qboolean CG_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot)
{
	qboolean r;
	entityState_t *es;
	int i;

	r = trap_PeekSnapshot(snapshotNumber, snapshot);

	if (cgs.ospEncrypt) {
		for (i = 0;  i < snapshot->numEntities;  i++) {
			es = &snapshot->entities[i];

			if (es->number < MAX_CLIENTS) {
				es->pos.trBase[0] += (677 - 7 * es->number);
				es->pos.trBase[1] += (411 - 12 * es->number);
				es->pos.trBase[2] += (243 - 2 * es->number);
			}
		}
	}

#if 0  // testing
	if (CG_IsCpmaMvd()) {
		for (i = 0;  i < snapshot->numEntities;  i++) {
			es = &snapshot->entities[i];

			if (es->number < MAX_CLIENTS) {
				CG_FixCpmaMvdEntity(es);
			}
		}
	}
#endif

	return r;
}

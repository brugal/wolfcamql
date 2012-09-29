#include "cg_local.h"
#include "wolfcam_local.h"

void Wolfcam_MarkValidEntities (void)
{
	centity_t *cent;
	int num;
    int i;

#if 0
    for (i = 0;  i < MAX_CLIENTS;  i++) {
        if (cgs.clientinfo[i].infoValid  &&  cg_entities[i].currentValid) {
            wclients[i].currentValid = qtrue;
            wclients[i].validTime = cg.time;
        } else {
            wclients[i].currentValid = qfalse;
            wclients[i].invalidTime = cg.time;
        }
    }
#endif

	for (i = 0;  i < MAX_CLIENTS;  i++) {
        wclients[i].currentValid = qfalse;
    }

    for (num = 0;  num < cg.snap->numEntities;  num++) {
        cent = &cg_entities[cg.snap->entities[num].number];
        //FIXME ET_CORPSE ?

        //if ((cent->currentState.eType == ET_PLAYER  ||  cent->currentState.eType == ET_CORPSE)  &&
        //     cgs.clientinfo[cent->currentState.clientNum].infoValid) {
        //if (cent->currentState.eType == ET_PLAYER) {
		if (cg.snap->entities[num].eType == ET_PLAYER  &&  cg.snap->entities[num].number == cg.snap->entities[num].clientNum) {
            wclients[cent->currentState.clientNum].currentValid = qtrue;
            wclients[cent->currentState.clientNum].validTime = cg.time;
        }
    }

    wclients[cg.snap->ps.clientNum].currentValid = qtrue;
    wclients[cg.snap->ps.clientNum].validTime = cg.time;
}

static void Wolfcam_ResetPlayerEntity (centity_t *cent, entityState_t *es, int time)
{
	//return;
	if (es->number == es->clientNum) {
		//return;
	}

	BG_EvaluateTrajectoryf(&es->pos, time, cent->lerpOrigin, cg.foverf);
	BG_EvaluateTrajectoryf(&es->apos, time, cent->lerpAngles, cg.foverf);

#if 0
	if (esNext->number == 6) {
		Com_Printf("xx\n");
	}
#endif

	//CG_Printf("Wolfcam_ResetPlayerEntity for %d %d\n", es->number, es->clientNum);

	//VectorCopy(esPrev->pos.trBase, cent->lerpOrigin);
	//VectorCopy(esPrev->apos.trBase, cent->lerpAngles);

	//VectorCopy(esNext->pos.trBase, cent->lerpOrigin);
	//VectorCopy(esNext->apos.trBase, cent->lerpAngles);

	//esNext = esPrev;
	//FIXME
	//cent->currentState.eFlags |= EF_NODRAW;
	//CG_ResetEntity(cent);
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;

	//Com_Printf("clearing %d %d\n", es->number, es->clientNum);

#if 1
	//time = snapPrev->serverTime + (cg.time - cg.snap->serverTime);
	//CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim, time);
	//CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim, time);
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, es->legsAnim, time);
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, es->torsoAnim, time);
#endif

	//return;

	//BG_EvaluateTrajectory( &es->pos, time, cent->lerpOrigin );
	//BG_EvaluateTrajectory( &es->apos, time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	cent->lastLgFireFrameCount = 0;
	cent->lastLgImpactFrameCount = 0;
	cent->lastLgImpactTime = 0;

#if 0
	if (1) {  //(cent->currentState.clientNum == 4) {
		Com_Printf("%d %d rawAngles %f %f\n", cent->currentState.number, cent->currentState.clientNum, cent->rawAngles[YAW], cent->rawAngles[PITCH]);
	}
#endif
}

static void Wolfcam_ResetEntity (centity_t *cent, entityState_t *es, int time)
{
	//return;

#if 0
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if (cent->snapShotTime < cg.time - EVENT_VALID_MSEC) {
		cent->previousEvent = 0;
	}

	//Com_Printf("reset ent %d\n", cent - cg_entities);
	VectorCopy(cent->currentState.pos.trBase, cent->lerpOrigin);
	VectorCopy(cent->currentState.apos.trBase, cent->lerpAngles);

	cent->trailTime = cg.snap->serverTime;

	cent->lastFlashIntervalTime = cg.ftime;
	cent->lastFlashDistanceTime = cg.ftime;

	cent->lastModelIntervalTime = cg.ftime;
	cent->lastModelDistanceTime = cg.ftime;

	cent->lastTrailIntervalTime = cg.ftime;
	cent->lastTrailDistanceTime = cg.ftime;

	cent->lastImpactIntervalTime = cg.ftime;
	cent->lastImpactDistanceTime = cg.ftime;

	VectorCopy(cent->currentState.pos.trBase, cent->lastFlashIntervalPosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastFlashDistancePosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastModelIntervalPosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastModelDistancePosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastTrailIntervalPosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastTrailDistancePosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastImpactIntervalPosition);
	VectorCopy(cent->currentState.pos.trBase, cent->lastImpactDistancePosition);

	//VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	//VectorCopy (cent->currentState.angles, cent->lerpAngles);
#endif

	//CG_Printf("Wolfcam_ResetEntity() cent->cgtime  %d  to %d\n", cent->cgtime, time);
	cent->cgtime = time;

	BG_EvaluateTrajectoryf(&es->pos, time, cent->lerpOrigin, cg.foverf);
	BG_EvaluateTrajectoryf(&es->apos, time, cent->lerpAngles, cg.foverf);

	//Com_Printf("Wolfcam_ResetEntity %d %d  etype %d\n", cent->currentState.number, cent->currentState.clientNum, cent->currentState.eType);

	//if (cent->currentState.eType == ET_PLAYER) {
	if (es->eType == ET_PLAYER) {
		Wolfcam_ResetPlayerEntity(cent, es, time);
	}
}


static snapshot_t tmpSnapshot;

static qboolean IsCorpse (centity_t *cent)
{
	if (cent->currentState.eType == ET_PLAYER  &&  cent->currentState.number >= MAX_CLIENTS  &&  cent->currentState.clientNum < MAX_CLIENTS  &&  cent->currentState.clientNum >= 0) {
		return qtrue;
	}

	return qfalse;
}

#if 0
static qboolean IsDeadPlayer (centity_t *cent)
{
	if (cent->currentState.number < MAX_CLIENTS  &&  cent->currentState.eFlags & EF_DEAD) {
		return qtrue;
	}

	return qfalse;
}
#endif

qboolean Wolfcam_InterpolateEntityPosition (centity_t *cent)
{
	vec3_t		current, next;
	float		f;
	qboolean useCurrent = qfalse;
	entityState_t *esOld, *esPrev, *esNext;
	entityState_t esPrevTmp, esNextTmp;
	int i;
	snapshot_t *snapOld, *snapPrev, *snapNext;
	qboolean presentInSnapOld, presentInSnapPrev, presentInSnapNext;
	int trPrevOrig, trNextOrig;
	int delta;
	qboolean isBot;
	int s1, s2;
	int ping;
	int serverFrameTime;
	int totalSum;
	int sft;
	snapshot_t *cgnextSnap;
	qboolean r = qfalse;
	int number;
	int clientNum;

	clientNum = cent->currentState.clientNum;
	number = cent->currentState.number;

	cent->serverTimeOffset = 0;

	//FIXME no, you'll have backup snaps available
#if 1
	if (0) {  //(cg.demoSeeking) {
		// useoriginterp 1 deals with it on it's own, just interpolates
		if (SC_Cvar_Get_Int("cl_freezeDemo")) {
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
			BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, cent->lerpAngles);
			//Com_Printf("just copying %d\n", cent->currentState.number);
		}
		wcg.centityAdded[number] = qtrue;
		return qtrue;
	}
#endif

	if ((cg.freecam  ||  cg_thirdPerson.integer)  &&  cg_freecam_useServerView.integer) {
		wcg.centityAdded[number] = qtrue;
		return qfalse;
	}

	if (!wolfcam_following  &&  cg_thirdPerson.integer  &&  cg_freecam_useServerView.integer) {
		wcg.centityAdded[number] = qtrue;
		return qfalse;
	}

	if (wcg.curSnapshotNumber < 4) {
		wcg.centityAdded[number] = qtrue;
		return qfalse;
	}

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one  -- old comment
	if (cg.nextSnap == NULL) {
		//CG_Printf( "^3Wolfcam_InterpoateEntityPosition: cg.nextSnap == NULL\n" );
		// offline demos nextsnap might have same server time
		i = 1;
		while (1) {
			r = trap_PeekSnapshot(cgs.processedSnapshotNum + i, &tmpSnapshot);
			if (!r) {
				Com_Printf("FIXME wc interp couldn't peek at next snapshot\n");
				wcg.centityAdded[number] = qtrue;
				return qtrue;  //FIXME don't return since you might not even need the next snapshot
			}
			if (cg.snap->serverTime >= tmpSnapshot.serverTime) {
				//Com_Printf("same server time %d, peeking next %d\n", cg.snap->serverTime, i);
				i++;
			} else {
				cgnextSnap = &tmpSnapshot;
				break;
			}
		}
	} else {
		cgnextSnap = cg.nextSnap;
	}

	f = cg.frameInterpolation;

	//FIXME cg.snap->ps.clientNum

	if (!cg.demoPlayback  ||  useCurrent  ||  cg_useOriginalInterpolation.integer) {
		wcg.centityAdded[number] = qtrue;
		return qfalse;  // use original CG_InterpolateEntityPosition()
	}

	if (!wolfcam_following  &&  cent->currentState.number == cg.snap->ps.clientNum) {
		// already took into account
		wcg.centityAdded[number] = qtrue;
		return qfalse;
	}

	if (wolfcam_following  &&  cent->currentState.number == wcg.clientNum) {
		cent->currentState.eFlags |= EF_NODRAW;
		wcg.centityAdded[number] = qtrue;
		return qfalse;
	}

	//Wolfcam_AddBox(cent->lerpOrigin, 30, 30, 56, 255, 0, 0);

	if (cent->currentState.eType == ET_MISSILE) {
		if (!cg_interpolateMissiles.integer) {
			wcg.centityAdded[number] = qtrue;
			return qfalse;
		}
	}

	////////////////////////////////////////////////

	f = cg.frameInterpolation;

#if 1
	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	if (cent != &cg.predictedPlayerEntity) {
		//FIXME BG_EvaluateTrajectoryf
		BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
		BG_EvaluateTrajectory( &cent->nextState.pos, cgnextSnap->serverTime, next );

		cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
		cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
		cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

		BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
		BG_EvaluateTrajectory( &cent->nextState.apos, cgnextSnap->serverTime, next );

		cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
		cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
		cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );
	}

	//Wolfcam_AddBoundingBox (cent);
	if (SC_Cvar_Get_Int("wolfcam_debug_interp") == 1)
	{
		vec3_t origin;

		origin[0] = cent->lerpOrigin[0];
		origin[1] = cent->lerpOrigin[1];
		origin[2] = cent->lerpOrigin[2] + 70;

		Wolfcam_AddBox(origin, 10, 10, 10, 255, 0, 0);
	}
#endif

	if (cent->currentState.number < MAX_CLIENTS  &&  cgs.clientinfo[cent->currentState.number].botSkill > 0) {
		isBot = qtrue;
	} else {
		isBot = qfalse;
	}

	esOld = NULL;
	esPrev = NULL;
	esNext = NULL;
	presentInSnapOld = qfalse;
	presentInSnapPrev = qfalse;
	presentInSnapNext = qfalse;

	// wcg.curSnapshotNumber - 1  == currentSnap
	// wcg.curSnapshotNumber - 2  == prevSnap

	if (0) {  //(isBot) {
		s1 = 1;  //2;
		s2 = 0;  //1;
	} else {
		s1 = 2;  //3;  //2;  //3;  // tested with video 2010-02-05, seems ok
		s2 = 1;  //1;  //2;
	}

	if (wolfcam_following) {
		//FIXME following a bot?
#if 0
		s1--;
		s2--;
#endif
	}

	serverFrameTime = cgnextSnap->serverTime - cg.snap->serverTime;
	//serverFrameTime = cg.serverFrameTime;
	//serverFrameTime = 1000 / cgs.sv_fps;  // no sometimes not reported

	if (cg.time - cg.pingCalculateTime > 5000) {
		//int samples;

		for (totalSum = 0, i = 0 /*, samples = 0*/;  i < LAG_SAMPLES  &&  i < lagometer.snapshotCount;  i++) {
			int val;

			val = lagometer.snapshotSamples[(lagometer.snapshotCount - i) & (LAG_SAMPLES - 1)];
			if (val < 0) {
				continue;
			}

			totalSum += val;
		}
		if (i > 0) {
			cg.avgPing = totalSum / i;
		} else {
			cg.avgPing = 0;
		}
		cg.pingCalculateTime = cg.time;
	}

	ping = cg.avgPing;

	if (wolfcam_following) {
		ping = 0;
		for (i = 0;  i < cg.numScores;  i++) {
			if (cg.scores[i].client == wcg.clientNum) {
				ping = cg.scores[i].ping;
				break;
			}
		}
	}

	if (ping > 80  &&  cgs.protocol == PROTOCOL_QL) {
		ping = 80;
	}

	if (cgs.protocol == PROTOCOL_Q3  &&  !cgs.cpma) {
		ping = 0;  // no antilag
	}

	sft = cg.serverFrameTime;

	//Com_Printf("ping: %d\n", ping);

	if (sft <= 0  ||  sft > 200) {
		sft = serverFrameTime;
	}

	if (isBot) {
		ping -= (sft / 2);
	}

	while ((float)ping > ((float)sft / 2.0)) {
		s1++;
		ping -= sft;  //serverFrameTime;
	}

	if (SC_Cvar_Get_Int("wolfcam_snap")) {
		s1 = SC_Cvar_Get_Int("wolfcam_snap");
		if (s1 < 1)
			s1 = 1;
		//s2 = s1 - 1;
		Com_Printf("s1: %d\n", s1);
	}

	cent->serverTimeOffset = (s1 - 1) * sft;

	s2 = s1 - 1;

	if (s1 == 1) {
		snapOld = cg.prevSnap;
		snapPrev = cg.snap;
		snapNext = cgnextSnap;
		//Com_Printf("xx\n");
	} else {
		if (wcg.curSnapshotNumber - s2 < 0) {
			wcg.centityAdded[number] = qtrue;
			return qfalse;
		}
		snapOld = &wcg.snaps[(wcg.curSnapshotNumber - s1 - 1) & MAX_SNAPSHOT_MASK];
		snapPrev = &wcg.snaps[(wcg.curSnapshotNumber - s1) & MAX_SNAPSHOT_MASK];
		snapNext = &wcg.snaps[(wcg.curSnapshotNumber - s2) & MAX_SNAPSHOT_MASK];
	}

	wcg.snapOld = snapOld;
	wcg.snapPrev = snapPrev;
	wcg.snapNext = snapNext;

	//Com_Printf("cg.snap->serverTime %d  %d\n", cg.snap->serverTime, snapPrev->serverTime);

	for (i = 0;  i < snapOld->numEntities;  i++) {
		esOld = &snapOld->entities[i];
		if (esOld->number == cent->currentState.number) {
			presentInSnapOld = qtrue;
			break;
		} else if (cent->currentState.number >= MAX_CLIENTS  &&  cent->currentState.eType == ET_PLAYER) {
			// corpse transition hack
			if (esOld->number == cent->currentState.clientNum  &&  esOld->eFlags & EF_DEAD) {
				//presentInSnapPrev = qtrue;
				//break;
			}
		}
	}

	// the ones that match up are snapPrev == 2, snapNext == 1
	for (i = 0;  i < snapPrev->numEntities;  i++) {
		esPrev = &snapPrev->entities[i];
		if (esPrev->number == cent->currentState.number) {
			presentInSnapPrev = qtrue;
			break;
		} else if (cent->currentState.number >= MAX_CLIENTS  &&  cent->currentState.eType == ET_PLAYER) {
			// corpse transition hack
			if (esPrev->number == cent->currentState.clientNum  &&  esPrev->eFlags & EF_DEAD) {
				//presentInSnapPrev = qtrue;
				//break;
			}
		}
	}

	for (i = 0;  i < snapNext->numEntities;  i++) {
		esNext = &snapNext->entities[i];
		if (esNext->number == cent->currentState.number) {
			presentInSnapNext = qtrue;
			break;
		} else if (cent->currentState.number >= MAX_CLIENTS  &&  cent->currentState.eType == ET_PLAYER) {
			// corpse transition hack
			if (esNext->number == cent->currentState.clientNum  &&  esNext->eFlags & EF_DEAD) {
				//presentInSnapNext = qtrue;
				//break;
			}
		}
	}

	//if (cent == &cg.predictedPlayerEntity) {
	if (cent->currentState.number == cg.snap->ps.clientNum) {
		playerState_t psTmp;

		presentInSnapPrev = qtrue;
		presentInSnapNext = qtrue;

		//FIXME assuming player to entity state called()  check
		//esPrev = &snapPrev->ps.currentState;
		//esNext = &snapNext->ps.currentState;
		memset(&psTmp, 0, sizeof(psTmp));
		memcpy(&psTmp, &snapPrev->ps, sizeof(psTmp));
		BG_PlayerStateToEntityState(&psTmp, &esPrevTmp, qfalse);
		memset(&psTmp, 0, sizeof(psTmp));
		memcpy(&psTmp, &snapNext->ps, sizeof(psTmp));
		BG_PlayerStateToEntityState(&psTmp, &esNextTmp, qfalse);

		esPrev = &esPrevTmp;
		esNext = &esNextTmp;
#if 0
		if (cent->currentState.eFlags & EF_NODRAW) {
			CG_Printf("nodraw...\n");
		}
		if (cent->currentState.eType == ET_INVISIBLE) {
			//CG_Printf("invis...\n");
			//FIXME hack since spec sets ET_INVISIBLE
			if (cent->currentState.number >= 0  &&  cent->currentState.number < MAX_CLIENTS) {
				esPrev->eType = ET_PLAYER;
				esNext->eType = ET_PLAYER;
			}
		}
#endif
		//Com_Printf("playerstate hack\n");
	}

	if (!presentInSnapPrev) {  //  ||  !presentInSnapNext) {

		//Com_Printf("%d  %s not in both snaps\n", cent->currentState.number, cgs.clientinfo[cent->currentState.number].name);

		if (cent->currentState.number == cg.snap->ps.clientNum) {
			CG_Printf("bleh........\n");
		}
		//return qfalse;  // use original CG_InterpolateEntityPosition()
#if 0
		if (cent->currentState.eType == ET_PLAYER) {
			Com_Printf("corpse skip %d   %d %d\n", cent - cg_entities, presentInSnapPrev, presentInSnapNext);
		}
#endif

		//FIXME corpse hack, might be a problem if corpse still has velocity
		if (0) {  //(IsCorpse(cent)  &&  presentInSnapNext) {  //(presentInSnapNext  &&  esNext->number >= MAX_CLIENTS  &&  esNext->eType == ET_PLAYER) {
			esPrev = esNext;
			Com_Printf("corpse hack adding %d %d\n", esNext->number, clientNum);
		} else {
			//FIXME  -- mark as invalid trying EF_NODRAW;
			cent->currentState.eFlags |= EF_NODRAW;

			//return qtrue;
			if (IsCorpse(cent)  ||  cent->currentState.number < MAX_CLIENTS) {
				//goto clear;
			}
			//Com_Printf("skipping %d %d not in snapPrev  presentInNext: %d\n", number, clientNum, presentInSnapNext);
			wcg.centityAdded[number] = qfalse;
			cent->trailTime = cg.time;
			return qtrue;
		}
	}


	// check for ps.clientNum since ca marks speced view from limbo as ET_INVISIBLE
	if (cent->currentState.eType != esPrev->eType  &&  cent->currentState.number != cg.snap->ps.clientNum) {
#if 0
		CG_PrintEntityState(cent - cg_entities);
		Com_Printf("\n");
		Com_Printf("eType %d != prev %d\n", cent->currentState.eType, esPrev->eType);
		Com_Printf("\n");
		CG_PrintEntityStatep(esPrev);
#endif

#if 0
		if (esPrev->number == 6) {
			Com_Printf("nope prev\n");
		}
#endif
#if 0
		if (esPrev->eType != ET_MISSILE) {
			Com_Printf("wrong prev eType skipping %d (%d) current %d  prev %d\n", number, esPrev->clientNum, cent->currentState.eType, esPrev->eType);
		}
#endif
		cent->currentState.eFlags |= EF_NODRAW;
		if (esPrev->eType == ET_PLAYER) {
			// hack to prevent player and gibs drawn at once
			wcg.centityAdded[number] = qtrue;
		} else {
			wcg.centityAdded[number] = qfalse;
		}
		return qtrue;
	}

	//FIXME also for snap->ps
	if (cent->currentState.number != cg.snap->ps.clientNum  &&  !presentInSnapOld) {
		//CG_Printf("%d %d not in snapOld\n", number, clientNum);
		Wolfcam_ResetEntity(cent, esPrev, snapOld->serverTime + (cg.time - cg.snap->serverTime));
		//goto interp_done;
	}

	//if (cent->currentState.number == cg.snap->ps.clientNum) {
	if (snapPrev->ps.clientNum != snapNext->ps.clientNum) {
#if 0
		BG_EvaluateTrajectory(&esPrev->pos, snapPrev->serverTime, cent->lerpOrigin);
		BG_EvaluateTrajectory(&esPrev->apos, snapPrev->serverTime, cent->lerpAngles);
		Com_Printf("clientNum changed\n");
		goto interp_done;
#endif
	}

	if (!presentInSnapNext) {
		if (IsCorpse(cent)) {
			//Com_Printf("corpse %d %d not in next\n", cent->currentState.clientNum, cent->currentState.number);
		}
		//Com_Printf("%d %d not in snapNext\n", number, clientNum);
		esNext = esPrev;
		BG_EvaluateTrajectory(&esPrev->pos, snapPrev->serverTime, cent->lerpOrigin);
		BG_EvaluateTrajectory(&esPrev->apos, snapPrev->serverTime, cent->lerpAngles);
		goto interp_done;

		//goto clear;
	}


	if (cent->currentState.eType != esNext->eType  &&  cent->currentState.number != cg.snap->ps.clientNum) {
		//Com_Printf("eType %d != next %d\n", cent->currentState.eType, esNext->eType);
#if 0
		if (esNext->number == 6) {
			Com_Printf("nope next\n");
		}
#endif
		//Com_Printf("wrong next eType skipping %d\n", number);
		cent->currentState.eFlags |= EF_NODRAW;
		wcg.centityAdded[number] = qtrue;
		return qtrue;
	}

#if 0
	if (cent->currentState.eType == ET_MISSILE) {
		// check to see if it's the same missile
		if (esPrev->pos.trTime != esNext->pos.trTime  ||  cent->currentState.pos.trTime != esPrev->pos.trTime) {
			//Com_Printf("ignoring missile %d\n", cent->currentState.number);
			cent->currentState.eFlags |= EF_NODRAW;
			wcg.centityAdded[number] = qtrue;
			return qtrue;
		}
	}
#endif

	//FIXME until explosions are moved/interp as well
	if (cent->currentState.eType == ET_MISSILE) {
		int event;

		event = cent->currentState.event & ~EV_EVENT_BITS;
		//Com_Printf("missile: %d\n", cent - cg_entities);
		if (event == EV_MISSILE_HIT  ||  event == EV_MISSILE_MISS  ||  event == EV_MISSILE_MISS_METAL) {  //(esNext->event) {  // exploded
			//Com_Printf("%d exploded, skipping\n", cent - cg_entities);
			cent->currentState.eFlags |= EF_NODRAW;
			wcg.centityAdded[number] = qtrue;
			return qtrue;
		}
	}

	if (esPrev->eFlags & EF_DEAD) {
		if (SC_Cvar_Get_Int("wolfcam_debug_interp") == 1) {
			vec3_t origin;

			origin[0] = cent->lerpOrigin[0];
			origin[1] = cent->lerpOrigin[1];
			origin[2] = cent->lerpOrigin[2] + 20;
			//Com_Printf("corpse %p\n", cent);
			//FIXME smooth corpse falling
			Wolfcam_AddBox(origin, 20, 20, 20, 255, 255, 255);
		}
		//return qfalse;
	}

	cent->cgtime = snapPrev->serverTime + (cg.time - cg.snap->serverTime);

// clear:
	if (  ((esPrev->eFlags ^ esNext->eFlags) & EF_TELEPORT_BIT)) {
		//int time;

#if 0
		if ((esPrev->eFlags ^ esNext->eFlags) & EF_TELEPORT_BIT) {
			Com_Printf("teleport %d %d\n", number, clientNum);
		} else {
			Com_Printf("esclear %d %d\n", number, clientNum);
		}
#endif
		//time = snapPrev->serverTime + (cg.time - cg.snap->serverTime);
		//Wolfcam_ResetEntity(cent, esPrev, time);

		if (cent->currentState.number == cg.snap->ps.clientNum) {
			//Com_Printf("demo taker teleport\n");
		}
		BG_EvaluateTrajectory(&esPrev->pos, snapPrev->serverTime, cent->lerpOrigin);
		BG_EvaluateTrajectory(&esPrev->apos, snapPrev->serverTime, cent->lerpAngles);

	//cent->interpolate = qtrue;
		//cent->interpolate = qfalse;
		//cent->currentState.eFlags |= EF_NODRAW;
		//return qtrue;
		goto interp_done;
		//return qtrue;
	}

#if 0
	if (esNext->number == 6) {
		float dist;
		//Com_Printf("%d  eflags  prev 0x%x  next 0x%x  current 0x%x\n", esNext->number, esPrev->eFlags, esNext->eFlags, cent->currentState.eFlags);
		//cent->currentState.eFlags |= EF_NODRAW;
		dist = Distance(esPrev->pos.trBase, esNext->pos.trBase);
		if (1) {  //(dist > 300) {
			Com_Printf("dist %f\n", dist);
		}
		//return qtrue;
	}
#endif

	trPrevOrig = esPrev->pos.trType;
	trNextOrig = esNext->pos.trType;

	//delta = (cgnextSnap->serverTime - cg.snap->serverTime);
	delta = sft;
	if (delta == 0)
		f = 0;
	else
		f = (float)( cg.time - (delta / 2) - cg.snap->serverTime ) / delta;

	f = cg.frameInterpolation;

	BG_EvaluateTrajectory(&esNext->pos, snapNext->serverTime, next);
	BG_EvaluateTrajectory(&esPrev->pos, snapPrev->serverTime, current);

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );


	BG_EvaluateTrajectory( &esPrev->apos, snapPrev->serverTime, current );
	BG_EvaluateTrajectory( &esNext->apos, snapNext->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

	esPrev->pos.trType = trPrevOrig;
	esNext->pos.trType = trNextOrig;

#if 0
	for (i = 0;  i < cg.snap->numEntities;  i++) {
		esPrev = &snapPrev->entities[i];
		if (esPrev->number == cent->currentState.number) {
			vec3_t origin;

			origin[0] = esPrev->pos.trBase[0];
			origin[1] = esPrev->pos.trBase[1];
			origin[2] = esPrev->pos.trBase[2] + 150;
			//presentInSnapPrev = qtrue;
			//Wolfcam_AddBox(esPrev->pos.trBase, 40, 40, 66, 0, 255, 0);
			//CG_Printf("adding box %d\n", esPrev->number);
			break;
		}
	}
	if (i >= cg.snap->numEntities) {
		//Com_Printf("wtf: %d\n", cent->currentState.number);
	}
#endif

	//Wolfcam_AddBoundingBox(cent);
	//CG_Printf("..done %d\n", cent->currentState.number);

 interp_done:

	//return qtrue;

	esNext = esPrev;

#if 0
	if (/*!cent->interpolate  &&*/  cent->currentState.clientNum == 6) {
		Com_Printf("%d %d etype prev %d (eflags 0x%x) next %d (eflags 0x%x) curr %d (eflags 0x%x)\n", number, clientNum, esPrev->eType, esPrev->eFlags, esNext->eType, esNext->eFlags, cent->currentState.eType, cent->currentState.eFlags);
		if (!cent->interpolate) {
			//CG_ResetPlayerEntity(cent);
		}
		//cent->currentState.eFlags |= EF_NODRAW;
	}
#endif

	cent->currentState.frame = esNext->frame;
	cent->currentState.solid = esNext->solid;
	cent->currentState.groundEntityNum = esNext->groundEntityNum;
	if (number != clientNum) { //(0) {  //(cent->currentState.number != cent->currentState.clientNum  && cent->currentState.eType == ET_PLAYER) {

	} else {
	cent->currentState.legsAnim = esNext->legsAnim;
	cent->currentState.torsoAnim = esNext->torsoAnim;
	}
	//if (cent->currentState.number != cg.snap->ps.clientNum) {
		cent->currentState.eType = esNext->eType;
		//}
	cent->currentState.powerups = esNext->powerups;
	cent->currentState.weapon = esNext->weapon;
	//if (cent->currentState.number != cg.snap->ps.clientNum) {
	cent->currentState.eFlags = esNext->eFlags;
	//cent->currentState.eFlags = esPrev->eFlags;
		//}

	//FIXME what else from entityState_ ?

	if (cent->currentState.number < MAX_CLIENTS) {
		//cent->currentState.eFlags |= EF_NODRAW;
	}

#if 0
	if (cent->currentState.number != cent->currentState.clientNum  && cent->currentState.eType == ET_PLAYER) {  //  &&  cent->currentState.clientNum == 6) {
		//cent->currentState.eFlags |= EF_NODRAW;
		//cent->lerpOrigin[2] += 60;
		if (!cent->interpolate) {
			//Com_Printf("corpse %d %d not interpolating\n", cent->currentState.clientNum, cent->currentState.number);
			//cent->currentState.eFlags |= EF_NODRAW;
		}
	}
#endif

#if 0
	if (cent->currentState.number == 6) {
		static int legsAnim = -1;

		if (!cent->interpolate) {
			Com_Printf("interp %d\n", cent->interpolate);
		}
		//cent->currentState.eFlags |= EF_NODRAW;
		if (cent->currentState.legsAnim != legsAnim) {  //(cent->currentState.legsAnim != 132) {
			//Com_Printf("legsAnim %d\n", cent->currentState.legsAnim);
		}

		legsAnim = cent->currentState.legsAnim;
	}

	// hack when teleporting from corpse
	if (!cent->interpolate) {//  &&  esNext->eFlags & EF_DEAD) {
		//cent->currentState.eFlags |= EF_NODRAW;
	}

	//cent->interpolate = qtrue;
#endif

	if (SC_Cvar_Get_Int("wolfcam_debug_interp") == 1)
	{
		vec3_t origin;

		origin[0] = cent->lerpOrigin[0];
		origin[1] = cent->lerpOrigin[1];
		origin[2] = cent->lerpOrigin[2] + DEFAULT_VIEWHEIGHT;  // + 70;

		Wolfcam_AddBox(origin, 30, 30, 56, 0, 0, 255);
	}

	if (cent->currentState.eType == ET_MISSILE) {
		//Com_Printf("missile %d  time %d  (%f %f %f)\n", cent->currentState.number, cent->currentState.pos.trTime, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2]);
		//cent->currentState.eFlags |= EF_NODRAW;
	}
	//Com_Printf("interp %d\n", cent->currentState.number);
	wcg.centityAdded[number] = qtrue;
	return qtrue;
}


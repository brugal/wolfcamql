#include "cg_local.h"

#include "cg_draw.h"  //  CG_lagometerMarkNoMove
#include "cg_main.h"
#include "cg_players.h"
#include "wolfcam_snapshot.h"

#include "wolfcam_local.h"


#ifdef __LINUX__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

void Wolfcam_CheckNoMove (void)
{
	int clientNum;
	int i;
	const centity_t *cent;
	byte color[4] = { 0, 255, 0, 255 };
	vec3_t origin;
	const entityState_t *es;
	entityState_t *nes;
	const entityState_t *nnes;
	qboolean inNextNextSnapshot;
	qboolean nextNextReset;

	cg.noMove = qfalse;
	cg.numNoMoveClients = 0;

	//if (cg.time < cgs.timeoutEndTime) {
	if (cg.snap->serverTime < cgs.timeoutEndTime) {
		return;
	}

	//FIXME other snaps as well?
	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}

#if 1
	if (!cg.nextNextSnapValid) {
		//Com_Printf("FIXME Wolfcam_CheckNoMove() !cg.nextNextSnapValid\n");
		//CG_Abort();
		return;
	}
#endif

	if ((cg.snap->snapFlags ^ cg.nextSnap->snapFlags) & SNAPFLAG_SERVERCOUNT) {
		return;
	}

	if ((cg.nextSnap->snapFlags ^ cg.nextNextSnap->snapFlags) & SNAPFLAG_SERVERCOUNT) {
		nextNextReset = qtrue;
	} else {
		nextNextReset = qfalse;
	}

	for (clientNum = 0;  clientNum < MAX_CLIENTS;  clientNum++) {
		cent = &cg_entities[clientNum];

		if (!cent->inCurrentSnapshot) {
			continue;
		}

		// view switching
		if ((cg.snap->ps.clientNum != cg.nextSnap->ps.clientNum)  &&  (clientNum == cg.snap->ps.clientNum  ||  clientNum == cg.nextSnap->ps.clientNum)) {
            continue;
        }

		if (clientNum != cg.snap->ps.clientNum  &&  !cent->inNextSnapshot) {
			continue;
		}

#if 0
		if (clientNum == cg.nextSnap->ps.clientNum  &&  cg.snap->ps.clientNum != cg.nextSnap->ps.clientNum) {
			continue;
		}
#endif

		if (clientNum == cg.snap->ps.clientNum) {
			if ((cg.snap->ps.eFlags ^ cg.nextSnap->ps.eFlags) & EF_TELEPORT_BIT) {
				continue;
			}
			if (Distance(cg.snap->ps.origin, cg.nextSnap->ps.origin) == 0.0f  &&  VectorLength(cg.snap->ps.velocity) > 0.0  &&  VectorLength(cg.nextSnap->ps.velocity) > 0.0) {
				CG_LagometerMarkNoMove();
				cg.noMove = qtrue;
				wclients[clientNum].noMoveCount++;

				if (cg_demoSmoothing.integer > 1) {
					Com_Printf("smooth demo taker\n");
					VectorCopy(cg.snap->ps.origin, origin);
					origin[2] += 90;
					CG_FloatNumber(clientNum, origin, RF_DEPTHHACK, color, 1.0);
				}
				if (cg.snap->ps.clientNum != cg.nextNextSnap->ps.clientNum) {
					continue;
				}
				if ((cg.nextSnap->ps.eFlags ^ cg.nextNextSnap->ps.eFlags) & EF_TELEPORT_BIT) {
					continue;
				}
				if (nextNextReset) {
					continue;
				}
				if (cg_demoSmoothing.integer) {
					cg.nextSnap->ps.origin[0] = cg.snap->ps.origin[0] + (cg.nextNextSnap->ps.origin[0] - cg.snap->ps.origin[0]) / 2;
					cg.nextSnap->ps.origin[1] = cg.snap->ps.origin[1] + (cg.nextNextSnap->ps.origin[1] - cg.snap->ps.origin[1]) / 2;
					cg.nextSnap->ps.origin[2] = cg.snap->ps.origin[2] + (cg.nextNextSnap->ps.origin[2] - cg.snap->ps.origin[2]) / 2;


					cg.nextSnap->ps.viewangles[0] = LerpAngle(cg.snap->ps.viewangles[0], cg.nextNextSnap->ps.viewangles[0], 0.5);
					cg.nextSnap->ps.viewangles[1] = LerpAngle(cg.snap->ps.viewangles[1], cg.nextNextSnap->ps.viewangles[1], 0.5);
					cg.nextSnap->ps.viewangles[2] = LerpAngle(cg.snap->ps.viewangles[2], cg.nextNextSnap->ps.viewangles[2], 0.5);
				}
			}
		} else {
			qboolean inSnapshot;
			qboolean inNextSnapshot;

			inSnapshot = qfalse;
			for (i = 0;  i < cg.snap->numEntities;  i++) {
				es = &cg.snap->entities[i];
				if (es->number == clientNum) {
					inSnapshot = qtrue;
					break;
				}
			}
			if (!inSnapshot) {
				CG_Printf("^1Wolfcam_CheckNoMove() not in snap %d\n", clientNum);
				//es = NULL;
				continue;  // shouldn't get here
			}

			inNextSnapshot = qfalse;
			for (i = 0;  i < cg.nextSnap->numEntities;  i++) {
				nes = &cg.nextSnap->entities[i];
				if (nes->number == clientNum) {
					inNextSnapshot = qtrue;
					break;
				}
			}
			if (!inNextSnapshot) {
				CG_Printf("^1Wolfcam_CheckNoMove() not in next snap %d : %d\n", clientNum, cent->inNextSnapshot);
				//nes = NULL;
				continue;  // shouldn't get here
			}

#if 0
			if (!nes) {
				Com_Printf("^3wtf:  clientNum %d  cg.nextSnap->numEntities %d\n", clientNum, cg.nextSnap->numEntities);
				continue;
			}
#endif

			inNextNextSnapshot = qfalse;
			for (i = 0;  i < cg.nextNextSnap->numEntities;  i++) {
				nnes = &cg.nextNextSnap->entities[i];
				if (nnes->number == clientNum) {
					inNextNextSnapshot = qtrue;
					break;
				}
			}
			if (!inNextNextSnapshot) {
				nnes = NULL;
			}

			if ((es->eFlags ^ nes->eFlags) & EF_TELEPORT_BIT) {
				continue;
			}

			if (Distance(es->pos.trBase, nes->pos.trBase) == 0.0f  &&  VectorLength(es->pos.trDelta) > 0.0  &&  VectorLength(nes->pos.trDelta) > 0.0) {

				wclients[clientNum].noMoveCount++;

				if (cg_demoSmoothing.integer > 1) {
					Com_Printf("smooth %d  (eType %d) %f %s\n", clientNum, es->eType, VectorLength(nes->pos.trDelta), cgs.clientinfo[clientNum].name);
					VectorCopy(es->pos.trBase, origin);
					origin[2] += 90;
					CG_FloatNumber(clientNum, origin, RF_DEPTHHACK, color, 1.0);
				}

				if (!inNextNextSnapshot) {
					continue;
				}
				if ((nes->eFlags ^ nnes->eFlags) & EF_TELEPORT_BIT) {
					continue;
				}
				if (nextNextReset) {
					continue;
				}
				if (cg_demoSmoothing.integer) {
					nes->pos.trBase[0] = es->pos.trBase[0] + (nnes->pos.trBase[0] - es->pos.trBase[0]) / 2;
					nes->pos.trBase[1] = es->pos.trBase[1] + (nnes->pos.trBase[1] - es->pos.trBase[1]) / 2;
					nes->pos.trBase[2] = es->pos.trBase[2] + (nnes->pos.trBase[2] - es->pos.trBase[2]) / 2;

					nes->apos.trBase[0] = LerpAngle(es->apos.trBase[0], nnes->apos.trBase[0], 0.5f);
					nes->apos.trBase[1] = LerpAngle(es->apos.trBase[1], nnes->apos.trBase[1], 0.5f);
					nes->apos.trBase[2] = LerpAngle(es->apos.trBase[2], nnes->apos.trBase[2], 0.5f);
				}
			}
		}
	}
}

#ifdef __LINUX__
#pragma GCC diagnostic pop
#endif


#ifdef __LINUX__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

static void Wolfcam_CheckWarp (void)
{
    int clientNum;
    int i;
	int numClients;

	if (cg.time < cgs.timeoutEndTime) {
		return;
	}

	numClients = 0;

    for (clientNum = 0;  clientNum < MAX_CLIENTS;  clientNum++) {
        //clientInfo_t *ci;
        const centity_t *cent;
        const entityState_t *es = NULL;
		const entityState_t *nes = NULL;
        vec3_t oldPosition = { 0, 0, 0 };
        vec3_t newPosition = { 0, 0, 0 };
        float xv;
        float yv;
        float xyspeed;
		//float xyvel;
		float xyerr;
		float velerr;
        int timeDiff;
        int oldGroundEntityNum;
		vec3_t oldVelocity;
		vec3_t newVelocity;
		int oldEflags;
		int newEflags;

        //ci = &cgs.clientinfo[clientNum];
        cent = &cg_entities[clientNum];

		if (!cent->inCurrentSnapshot) {
			if (clientNum != cg.snap->ps.clientNum) {
				wclients[clientNum].invalidCount++;
				continue;
			}
		}

		numClients++;
		wclients[clientNum].validCount++;
		wclients[clientNum].snapshotPingSamples++;

		if (cg.snap->ps.pm_type == PM_INTERMISSION) {
			continue;
		}

		// view switching
		if ((cg.snap->ps.clientNum != cg.nextSnap->ps.clientNum)  &&  (clientNum == cg.snap->ps.clientNum  ||  clientNum == cg.nextSnap->ps.clientNum)) {
            continue;
        }

		if (clientNum != cg.snap->ps.clientNum  &&  !cent->inNextSnapshot) {
			continue;
		}

		if ((cg.snap->snapFlags ^ cg.nextSnap->snapFlags) & SNAPFLAG_SERVERCOUNT) {
			continue;
		}

		if (clientNum == cg.snap->ps.clientNum) {
            VectorCopy(cg.snap->ps.origin, oldPosition);
            SnapVector(oldPosition);
			VectorCopy(cg.snap->ps.velocity, oldVelocity);
			oldEflags = cg.snap->ps.eFlags;
            oldGroundEntityNum = cg.snap->ps.groundEntityNum;

			VectorCopy(cg.nextSnap->ps.origin, newPosition);
			SnapVector(newPosition);
			VectorCopy(cg.nextSnap->ps.velocity, newVelocity);
			newEflags = cg.nextSnap->ps.eFlags;
		} else {
			qboolean inSnapshot, inNextSnapshot;

			if (cg.snap->numEntities <= 0  ||  cg.nextSnap->numEntities <= 0) {
				continue;
			}

			inSnapshot = qfalse;
			for (i = 0;  i < cg.snap->numEntities;  i++) {
				es = &cg.snap->entities[i];
				if (es->number == clientNum) {
					VectorCopy(es->pos.trBase, oldPosition);
					VectorCopy(es->pos.trDelta, oldVelocity);
					oldEflags = es->eFlags;
					oldGroundEntityNum = es->groundEntityNum;
					inSnapshot = qtrue;
					break;
				}
			}
			if (!inSnapshot) {
				CG_Printf("^1Wolfcam_CheckWarp() not in snapshot %d\n", clientNum);
				continue;  // shouldn't get here
			}

			inNextSnapshot = qfalse;
			for (i = 0;  i < cg.nextSnap->numEntities;  i++) {
				nes = &cg.nextSnap->entities[i];
				if (nes->number == clientNum) {
					VectorCopy(nes->pos.trBase, newPosition);
					VectorCopy(nes->pos.trDelta, newVelocity);
					newEflags = nes->eFlags;
					inNextSnapshot = qtrue;
					break;
				}
			}
			if (!inNextSnapshot) {
				CG_Printf("^1Wolfcam_CheckWarp() not in next snapshot %d\n", clientNum);
				continue;  // shouldn't get here
			}

		}

		if ((oldEflags ^ newEflags) & EF_TELEPORT_BIT) {
			continue;
		}

		if (clientNum == cg.snap->ps.clientNum) {
			//wclients[clientNum].snapshotPingAccum += (cg.snap->serverTime - cg.snap->ps.commandTime);
			wclients[clientNum].snapshotPingAccum += (cg.snap->serverTime - cg.snap->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
			if (cg.nextSnap->ps.commandTime - cg.snap->ps.commandTime == 0) {
				wclients[clientNum].noCommandCount++;
			}
		} else {
			//FIXME quakelive has es->pos.trTime and trDuration zeroed out
			wclients[clientNum].snapshotPingAccum += (cg.snap->serverTime - es->pos.trTime);
#if 0
			if (nes->pos.trTime - es->pos.trTime == 0) {  // could have situation where clientNum is ps.clientNum but in last snap was an entity (player switching in spec demos)
				//wclients[clientNum].noCommandCount++;
			}
#endif
		}

        if (oldGroundEntityNum == ENTITYNUM_NONE) {
            continue;  // in air, don't check
		}

		if (oldVelocity[2] != 0.0  ||  newVelocity[2] != 0.0) {
			//CG_Printf("^2FIXME %d  trDelta[2]=%f %s\n", clientNum, es->pos.trDelta[2], ci->name);
			continue;
		}

        if (oldPosition[2] != newPosition[2]) {
            continue;
		}

        xv = (newPosition[0] - oldPosition[0]);   // *   1000.0 / (float)(cg.snap->serverTime - oldsnap->serverTime);  // / 50.0;  // 50 ms
        yv = (newPosition[1] - oldPosition[1]);  //  *   1000.0 / (float)(cg.snap->serverTime - oldsnap->serverTime);  // / 50.0;

        // snap the vectors in case positions are from playerState
#if 1
        xv = (int)xv;
        yv = (int)yv;
#endif
        xyspeed = sqrt(xv * xv  +  yv * yv);

        timeDiff = cg.nextSnap->serverTime - cg.snap->serverTime;
        xyspeed *= (1000.0 / (float)timeDiff);

        // give people the benifit of the doubt with respect to rounding errors
		// position are ints with +-1 error:
		// xyerr = sqrt (2 * 2  +  2 * 2)
		//         = 2.8284...
		xyerr = 2.83 * (1000.0 / (float)timeDiff);

        //CG_Printf ("%s: xv:%f  yv:%f  xyspeed:%f\n", cgs.clientinfo[clientNum].name, xv, yv, xyspeed);


        //FIXME take into account snapped vectors and give the benifit of the doubt
#if 0
		xyvel = sqrt(es->pos.trDelta[0] * es->pos.trDelta[0]  +
					 es->pos.trDelta[1] * es->pos.trDelta[1]);
#endif
		//xyvel = sqrt(oldVelocity[0] * oldVelocity[0] + oldVelocity[1] * oldVelocity[1]);

		// pm_accel: 10.0,   pm_friction: 6.0
		velerr = (10.0 * (1000.0 / (float)timeDiff) * 320) -
			     (320.0 * 6.0 * (1000.0 / (float)timeDiff));
		//FIXME check that they aren't hitting something
		if (xyspeed > 320.0 - velerr) {
			// assume max error 4
			//if (xyspeed > 352.0 + 40.0) {
			if (xyspeed > 320.0 + xyerr) {
				wclients[clientNum].warp++;
				//wclients[clientNum].warpaccum = (xyspeed + wclients[clientNum].warpaccum) / wclients[clientNum].warp;
				wclients[clientNum].warpaccum += xyspeed;
				//CG_Printf ("%02d   %s  ^7%f\n", clientNum, cgs.clientinfo[clientNum].name, xyspeed);
		    //} else if (xyspeed > 352.0 - 40.0) {  // give the the benifit of the doubt
			} else if (xyspeed > 320.0 - xyerr) {  // give the the benifit of the doubt
				wclients[clientNum].nowarp++;
			}
		}
    }
}

#ifdef __LINUX__
#pragma GCC diagnostic pop
#endif

void Wolfcam_NextSnapShotSet (void)
{
	int i;
	const entityState_t *es;

    //Wolfcam_MarkValidEntities ();  //FIXME yes, want here

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cg_entities[i].inCurrentSnapshot = qfalse;
		cg_entities[i].inNextSnapshot = qfalse;
	}

	cg_entities[cg.snap->ps.clientNum].inCurrentSnapshot = qtrue;
	cg_entities[cg.nextSnap->ps.clientNum].inNextSnapshot = qtrue;

	for (i = 0;  i < cg.snap->numEntities;  i++) {
		es = &cg.snap->entities[i];
		cg_entities[es->number].inCurrentSnapshot = qtrue;
	}

	for (i = 0;  i < cg.nextSnap->numEntities;  i++) {
		es = &cg.nextSnap->entities[i];
		cg_entities[es->number].inNextSnapshot = qtrue;
	}

    Wolfcam_CheckWarp();
	Wolfcam_CheckNoMove();
}

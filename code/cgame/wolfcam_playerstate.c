#include "cg_local.h"
#include "wolfcam_local.h"

#if 0
int wolfcam_find_client_to_follow (void)
{
    int i;
    qboolean foundNewClient;
    qboolean foundBackupClient;
    int backupClient = -1;  // cg.snap->ps.clientNum acts as second backup client
    float newClientDist;
    float backupDist;

    if (wclients[wcg.selectedClientNum].currentValid)
        return wcg.selectedClientNum;

    foundNewClient = qfalse;
    foundBackupClient = qfalse;
    newClientDist = 9999999;
    backupDist = 9999999;

    for (i = wcg.clientNum;  i < MAX_CLIENTS;  i++) {
        if (!cgs.clientinfo[i].infoValid)
            continue;
        if (!wclients[i].currentValid)
            continue;
        //if (i == cg.snap->ps.clientNum  &&  wolfcam_avoidDemoPov.integer)
        if (i == cg.clientNum  &&  1)  //wolfcam_avoidDemoPov.integer)
            continue;

        if (cg_entities[i].currentState.eFlags & EF_DEAD) {
            //continue;
            //Com_Printf("%d dead\n", cg.time);
        }

        if (cg_entities[i].currentState.number != cg_entities[i].currentState.clientNum) {
            Com_Printf("num: %d  clientNum: %d\n", cg_entities[i].currentState.number, cg_entities[i].currentState.clientNum);
        }

        if (cgs.clientinfo[i].team != cgs.clientinfo[wcg.selectedClientNum].team  &&  1)  { //wolfcam_tryToStickWithTeam.integer) {
            if (!foundBackupClient) {
                foundBackupClient = qtrue;
                backupClient = i;
            }
            continue;
        }
        foundNewClient = qtrue;
        break;
    }

    if (foundNewClient)
        return i;

    for (i = 0;  i < wcg.clientNum;  i++) {
        if (!cgs.clientinfo[i].infoValid)
            continue;
        if (!wclients[i].currentValid)
            continue;
        //if (i == cg.snap->ps.clientNum  &&  wolfcam_avoidDemoPov.integer)
        if (i == cg.clientNum  &&  1)  //wolfcam_avoidDemoPov.integer)
            continue;

        if (cg_entities[i].currentState.eFlags & EF_DEAD) {
            //continue;
            //Com_Printf("%d dead\n", cg.time);
        }

        if (cg_entities[i].currentState.number != cg_entities[i].currentState.clientNum) {
            Com_Printf("num: %d  clientNum: %d\n", cg_entities[i].currentState.number, cg_entities[i].currentState.clientNum);
        }

        if (cgs.clientinfo[i].team != cgs.clientinfo[wcg.selectedClientNum].team  &&  1)  { //wolfcam_tryToStickWithTeam.integer) {
            if (!foundBackupClient) {
                foundBackupClient = qtrue;
                backupClient = i;
            }
            continue;
        }
        foundNewClient = qtrue;
        break;
    }

    if (foundNewClient)
        return i;

    if (foundBackupClient)
        return backupClient;

    if (wclients[wcg.clientNum].currentValid)
        return wcg.clientNum;

    return cg.snap->ps.clientNum;
}
#endif

int wolfcam_find_client_to_follow (void)
{
    int i;
    qboolean foundNewClient;
    qboolean foundBackupClient;
    int newClient = -1;
    int backupClient = -1;  // cg.snap->ps.clientNum acts as second backup client
    float newClientDist;
    float backupClientDist;
    //int foundServerTime;
    qboolean forceGetNewVictim = qfalse;
    int hoverTime = wolfcam_hoverTime.integer;

    if (!cg.snap) {
        return wcg.selectedClientNum;
    }

    if ( (wcg.nextKillerServerTime >= 0  &&  cg.snap->serverTime > wcg.nextKillerServerTime + hoverTime) ||  wcg.ourLastClientNum != cg.snap->ps.clientNum) {
        int c;

        //if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
        if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
            c = cg.clientNum;
        } else {
            c = cg.snap->ps.clientNum;
        }
#if 0
        if (cg_fragForwardStyle.integer == 1) {
            c = cg.snap->ps.clientNum;
        }
#endif
        if (trap_GetNextKiller(c, cg.snap->serverTime, &wcg.nextKiller, &wcg.nextKillerServerTime, qtrue)) {
            if (wcg.ourLastClientNum != cg.snap->ps.clientNum) {
                //Com_Printf("client switch\n");
                forceGetNewVictim = qtrue;
            }
            //Com_Printf("next person to kill %s: %d %d %s\n", cgs.clientinfo[cg.snap->ps.clientNum].name, wcg.nextKiller, wcg.nextKillerServerTime, cgs.clientinfo[wcg.nextKiller].name);
            //FIXME hack to not switch away so fast
            wcg.nextKillerServerTime += 0;  //2000;
            wcg.ourLastClientNum = cg.snap->ps.clientNum;
        } else {
            //Com_Printf("no next killer\n");
            wcg.nextKiller = -1;
            wcg.nextKillerServerTime = -1;
        }
    }
    if (!cg.fragForwarding) {  // next victim already set up in fragforwarding code
        if (forceGetNewVictim  ||  (wcg.nextVictimServerTime >= 0  &&  cg.snap->serverTime > wcg.nextVictimServerTime + hoverTime)  ||  wcg.ourLastClientNum != cg.snap->ps.clientNum) {
            int c;

            //if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
            if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
                c = cg.clientNum;
            } else {
                c = cg.snap->ps.clientNum;
            }
            if (trap_GetNextVictim(c, cg.snap->serverTime, &wcg.nextVictim, &wcg.nextVictimServerTime, qtrue)) {
                //Com_Printf("next victim for %s: %d %d %s\n", cgs.clientinfo[cg.snap->ps.clientNum].name, wcg.nextVictim, wcg.nextVictimServerTime, cgs.clientinfo[wcg.nextVictim].name);
                //FIXME hack to not switch away so fast
                wcg.nextVictimServerTime += 0;  //2000;
                wcg.ourLastClientNum = cg.snap->ps.clientNum;
            } else {
                //Com_Printf("no next victim\n");
                wcg.nextVictim = -1;
                wcg.nextVictimServerTime = -1;
            }
        }
    }

    if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
        int t;

        t = wcg.nextKillerServerTime - cg.snap->serverTime;

        if (wcg.clientNum == wcg.nextKiller  &&  t < 0  &&  t > -hoverTime) {
            //Com_Printf("hover killer\n");
            return wcg.clientNum;
        }

        if (wcg.nextKiller > -1  &&  cgs.clientinfo[wcg.nextKiller].infoValid  &&  wclients[wcg.nextKiller].currentValid) {
            return wcg.nextKiller;
        }
        if (wolfcam_switchMode.integer == 1) {
            return cg.snap->ps.clientNum;
        }
        if (wcg.nextVictim > -1  &&  cgs.clientinfo[wcg.nextVictim].infoValid  &&  wclients[wcg.nextVictim].currentValid) {
            return wcg.nextVictim;
        }
        if (wolfcam_switchMode.integer == 2) {
            return cg.snap->ps.clientNum;
        }
        goto default_follow;
    }

    if (wcg.followMode == WOLFCAM_FOLLOW_VICTIM) {
        int t;

        t = wcg.nextVictimServerTime - cg.snap->serverTime;

        if (t < 0  &&  t > -hoverTime) {
            return wcg.nextVictim;
        }

#if 0
        if (wcg.clientNum == wcg.nextVictim  &&  t < 0  &&  t > -hoverTime) {
            Com_Printf("hover victim\n");
            return wcg.clientNum;
        }
#endif

        if (wcg.nextVictim > -1  &&  cgs.clientinfo[wcg.nextVictim].infoValid  &&  wclients[wcg.nextVictim].currentValid) {
            //Com_Printf("victim: %s\n", cgs.clientinfo[wcg.nextVictim].name);
            return wcg.nextVictim;
        }
        if (wolfcam_switchMode.integer == 1) {
            return cg.snap->ps.clientNum;
        }
        if (wcg.nextKiller > -1  &&  cgs.clientinfo[wcg.nextKiller].infoValid  &&  wclients[wcg.nextKiller].currentValid) {
            return wcg.nextKiller;
        }
        if (wolfcam_switchMode.integer == 2) {
            return cg.snap->ps.clientNum;
        }
        goto default_follow;
    }

    //FIXME maybe dead check as well
    if (wclients[wcg.selectedClientNum].currentValid) {
        return wcg.selectedClientNum;
    }
    if (wolfcam_switchMode.integer == 1  ||  wolfcam_switchMode.integer == 2) {
        return cg.snap->ps.clientNum;
    }

 default_follow:

    //Com_Printf("default follow\n");

    if (wcg.clientNum != cg.snap->ps.clientNum) {
        if (wclients[wcg.clientNum].currentValid) {
            if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
                if (cg.time - wclients[wcg.clientNum].deathTime < hoverTime) {
                    return wcg.clientNum;
                }
            } else {
                if (cgs.gametype < GT_TEAM) {
                    return wcg.clientNum;
                }
            }
        }
    }

    foundNewClient = qfalse;
    foundBackupClient = qfalse;
    newClientDist = 9999999;
    backupClientDist = 9999999;

    for (i = 0;  i < MAX_CLIENTS;  i++) {
        if (!cgs.clientinfo[i].infoValid) {
            continue;
        }
        if (!wclients[i].currentValid) {
            continue;
        }
        if (i == cg.clientNum) {
            continue;
        }
        if (i == cg.snap->ps.clientNum) {
            continue;
        }
        if (cg_entities[i].currentState.eFlags & EF_DEAD) {
            continue;
        }
        if (cg_entities[i].currentState.number != cg_entities[i].currentState.clientNum) {
            continue;
        }
        //FIXME afk

        if (cgs.gametype >= GT_TEAM) {
            if (cgs.clientinfo[i].team == cgs.clientinfo[wcg.selectedClientNum].team) {
                if (!foundBackupClient) {
                    foundBackupClient = qtrue;
                    backupClient = i;
                    backupClientDist = Distance(cg.snap->ps.origin, cg_entities[i].currentState.pos.trBase);
                } else {
                    float dist;

                    dist = Distance(cg.snap->ps.origin, cg_entities[i].currentState.pos.trBase);
                    if (dist < backupClientDist) {
                        backupClient = i;
                        backupClientDist = dist;
                    }
                }
                continue;
            }  // same team
        }  // if gametype >= GT_TEAM
        if (!foundNewClient) {
            foundNewClient = qtrue;
            newClient = i;
            newClientDist = Distance(cg.snap->ps.origin, cg_entities[i].currentState.pos.trBase);
        } else {
            float dist;

            dist = Distance(cg.snap->ps.origin, cg_entities[i].currentState.pos.trBase);
            if (dist < newClientDist) {
                newClient = i;
                newClientDist = dist;
            }
        }
    }

    if (cgs.gametype >= GT_TEAM  &&  foundBackupClient) {
        return backupClient;
    }

    if (foundNewClient) {
        return newClient;
    }

    if (foundBackupClient) {
        return backupClient;
    }

    //if (wclients[wcg.clientNum].currentValid)
    //    return wcg.clientNum;

    return cg.snap->ps.clientNum;
}



//FIXME wolfcam move stuff from Wolfcam_Offset* here
void Wolfcam_TransitionPlayerState (int oldClientNum)
{
    clientInfo_t *ci;
    clientInfo_t *cio;
    centity_t *cent;
    entityState_t *cs, *os;
    wclient_t *wc;
    //qboolean wasCrouching, wasStanding;
    //qboolean isCrouching, isStanding;
    //float duckChange;
    //vec3_t velocity;
    float bobmove = 0.0;
    //int i;
    //int grenadeTimeLeft;
    //int oldClientNum;
    //int cn;

    //oldClientNum = wcg.clientNum;

#if 0
    //if (wclients[wcg.selectedClientNum].currentValid  &&  !(cg_entities[wcg.selectedClientNum].currentState.eFlags & EF_DEAD)) {
    if (wclients[wcg.selectedClientNum].currentValid) {
        wcg.clientNum = wcg.selectedClientNum;
        if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
            //Com_Printf("^3%d following dead\n", cg.time);
        }
    //} else if (!wclients[wcg.clientNum].currentValid  ||  cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD  ||  wcg.clientNum == cg.snap->ps.clientNum) {
    } else {  //if (!wclients[wcg.clientNum].currentValid  ||  wcg.clientNum == cg.snap->ps.clientNum) {
        //wcg.clientNum = wolfcam_find_next_valid_team_client (wcg.clientNum);
        //wcg.clientNum = wolfcam_find_next_valid_team_client (cg.snap->ps.clientNum);
        wcg.clientNum = wolfcam_find_client_to_follow();
    }
#endif

#if 0
    cn = wcg.clientNum;
    wcg.clientNum = wolfcam_find_client_to_follow();
    if (wcg.clientNum != cn) {
        //Com_Printf("switching from %d to %d\n", cn, wcg.clientNum);
    }
#endif

    ci = &cgs.clientinfo[wcg.clientNum];
    cio = &cgs.clientinfoOrig[wcg.clientNum];
    ci->legsModel = cio->legsModel;
    ci->legsSkin = cio->legsSkin;
    //FIXME check
    // ci->legsSkinAlt = cio->legsSkinAlt
    ci->torsoModel = cio->torsoModel;
    ci->torsoSkin = cio->torsoSkin;
    //FIXME check
    // ci->torsoSkinAlt = cio->torsoSkinAlt
    ci->headModel = cio->headModel;
    ci->headSkin = cio->headSkin;
    //FIXME check
    // ci->headSkinAlt = cio->headSkinAlt
    ci->modelIcon = cio->modelIcon;
    memcpy(ci->animations, cio->animations, sizeof(ci->animations));
    memcpy(ci->sounds, cio->sounds, sizeof(ci->sounds));

    cent = &cg_entities[wcg.clientNum];
    cs = &cent->currentState;
    wc = &wclients[wcg.clientNum];
    os = &wc->esPrev;

    if (cs->weapon != os->weapon) {
        wcg.weaponSelectTime = cg.time;
    }

    if (wcg.clientNum != oldClientNum) {
        memcpy (os, cs, sizeof(entityState_t));
        wcg.weaponSelectTime = 0;
    }

#if 0  // wolfcam testing
    //FIXME wolfcam, as 'follow' option, 
    //FIXME wolfcam not in this function
    //FIXME wolfcam finer grained cg_main timescale (don't skip too far ahead)
    if ((wcg.fastForwarding  &&  cg.clientNum == wcg.clientNum)  ||  ci->team == TEAM_SPECTATOR) {
        trap_Cvar_Set ("s_volume", wcg.oldVolume);
        trap_Cvar_Set ("timescale", wcg.oldTimescale);
        trap_SendConsoleCommand ("s_stop\n");
        wcg.fastForwarding = qfalse;
    }

    if (cg.clientNum != wcg.clientNum) {  //if (wcg.fastForward) {
        if (cg.time - wcg.validTime > (wolfcam_fast_forward_invalid_time.value * 1000.0)) {
            if (!wcg.fastForwarding) {
                //CG_Printf ("timescale: %f\n", cg_timescale.value);
                trap_Cvar_VariableStringBuffer ("s_volume", wcg.oldVolume, sizeof(wcg.oldVolume));
                trap_Cvar_VariableStringBuffer ("timescale", wcg.oldTimescale, sizeof(wcg.oldTimescale));
                trap_Cvar_Set ("s_volume", "0");
                trap_Cvar_Set ("timescale", wolfcam_fast_forward_timescale.string);
                wcg.fastForwarding = qtrue;
            }
        } else {
            if (wcg.fastForwarding) {
                //CG_Printf ("^3 setting timescale back: %s", wcg.oldTimescale);
                trap_Cvar_Set ("s_volume", wcg.oldVolume);
                trap_Cvar_Set ("timescale", wcg.oldTimescale);
                trap_SendConsoleCommand ("s_stop\n");
                wcg.fastForwarding = qfalse;
            }
        }
    }
#endif

    if (wc->weapAnimTimer > 0) {
        wc->weapAnimTimer -= cg.frametime;
        if (wc->weapAnimTimer < 0)
            wc->weapAnimTimer = 0;
    }
    //Com_Printf ("%d\n", cg.frametime);

#if 0  //FIXME wolfcam
    // OSP - MV client handling
    if(cg.mvTotalClients > 0) {
        if (ps->clientNum != ops->clientNum) {
            cg.thisFrameTeleport = qtrue;

            // clear voicechat
            cg.predictedPlayerEntity.voiceChatSpriteTime = 0;
            // CHECKME: should we do this here?
            cg_entities[ps->clientNum].voiceChatSpriteTime = 0;

            *ops = *ps;
        }
        CG_CheckLocalSounds( ps, ops );
        return;
    }
#endif

    // check for changing follow mode
    //FIXME wolfcam todo
#if 0
    if ( ps->clientNum != ops->clientNum ) {
        cg.thisFrameTeleport = qtrue;

        // clear voicechat
        cg.predictedPlayerEntity.voiceChatSpriteTime = 0;
        cg_entities[ps->clientNum].voiceChatSpriteTime = 0;

        // make sure we don't get any unwanted transition effects
        *ops = *ps;

        // DHM - Nerve :: After Limbo, make sure and do a CG_Respawn
        if ( ps->clientNum == cg.clientNum )
            ops->persistant[PERS_SPAWN_COUNT]--;
    }
#endif

#if 0  // wolfcam handled in events
    if (cs->eFlags & EF_FIRING) {
        wolfcam_lastFiredWeaponTime = 0;
        wolfcam_weaponFireTime += cg.frametime;
    } else {
        if (wolfcam_weaponFireTime > 500  && wolfcam_weaponFireTime) {  // wolfcam huh?
            wolfcam_lastFiredWeaponTime = cg.time;
        }
        wolfcam_weaponFireTime = 0;
    }
#endif

    //FIXME wolfcam EF_FIRING problem with rapid taps
    if (cs->eFlags & EF_FIRING  &&  !(os->eFlags & EF_FIRING)) {
#if 0  // wolfcam-q3
        //CG_Printf ("^3now firing...  %d  old:%d\n", cs->weapon, os->weapon);
        if (cs->weapon == WP_GRENADE_LAUNCHER   ||
            cs->weapon == WP_GRENADE_PINEAPPLE  ||
            cs->weapon == WP_DYNAMITE) {
            //CG_Printf ("grenade time set: %d\n", cg.time);
            wc->grenadeDynoTime = cg.time;
        }
#endif
    }

    if (!(cs->eFlags & EF_FIRING))
        wc->grenadeDynoTime = 0;

#if 0
    if (os->weapon != cs->weapon) {
        wolfcam_weaponSwitchTime = cg.time;
        wolfcam_oldweapon = os->weapon;
    }
#endif
    // damage events (player is getting wounded)
    //FIXME wolfcam
#if 0
    if( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
        CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
    }
#endif

    // respawning 
    //FIXME wolfcam
#if 0
    if( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT]) {
        CG_Respawn( ps->persistant[PERS_REVIVE_COUNT] != ops->persistant[PERS_REVIVE_COUNT] ? qtrue : qfalse );
    }
#endif

#if 0  //FIXME wolfcam
    if ( cg.mapRestart ) {
        CG_Respawn( qfalse );
        cg.mapRestart = qfalse;
    }

    if ( cg.snap->ps.pm_type != PM_INTERMISSION 
         && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
        CG_CheckLocalSounds( ps, ops );
    }

#endif

    // check for going low on ammo
    //CG_CheckAmmo();
    //Com_Printf("testing check ammo\n");


    //FIXME wolfcam
    // run events
    //CG_CheckPlayerstateEvents( ps, ops );

    // smooth the ducking viewheight change

#if 0
    wasCrouching = wasStanding = qfalse;

    if (os->eFlags & EF_CROUCHING)
        wasCrouching = qtrue;
    else
        wasStanding = qtrue;

    isCrouching = isStanding = qfalse;

    if (cs->eFlags & EF_CROUCHING)
        isCrouching = qtrue;
    else
        isStanding = qtrue;

    duckChange = 0;

    if (isCrouching) {
        if (wasStanding)
            duckChange = CROUCH_VIEWHEIGHT - DEFAULT_VIEWHEIGHT;
    } else if (isStanding) {
        if (wasCrouching)
            duckChange = DEFAULT_VIEWHEIGHT - CROUCH_VIEWHEIGHT;
    }

    if (duckChange) {
        wc->duckChange = duckChange;
        wc->duckTime = cg.time;
    }
#endif

#if 1  //FIXME wolfcam doesn't seem to work
 {
     vec3_t velocity;

     //BG_EvaluateTrajectoryDelta (&cs->pos, cg.time, velocity, qfalse, -1);
     //BG_EvaluateTrajectoryDelta (&cs->pos, cg.time, velocity);
     VectorCopy (cs->pos.trDelta, velocity);
 
     if (wcg.clientNum != cg.snap->ps.clientNum)
         wc->xyspeed = sqrt (velocity[0] * velocity[0] + velocity[1] * velocity[1]);
     else  {// view what demo pov
         playerState_t *ps = &cg.predictedPlayerState;
         wc->xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
                             ps->velocity[1] * ps->velocity[1] );
     }
 }
#endif
    //    Com_Printf ("%f %f %f\n", wolfcam_prevOrigin[0], wolfcam_prevOrigin[1], wolfcam_prevOrigin[2]);
#if 0
    {
        float xv, yv;

        //xv = (cent->lerpOrigin[0] - wolfcam_prevOrigin[0]) / cg.frametime;
        //yv = (cent->lerpOrigin[1] - wolfcam_prevOrigin[1]) / cg.frametime;
        //FIXME wolfcam 16 - 8
        xv = (cent->lerpOrigin[0] - wolfcam_prevOrigins[16 - 16][0]) / (cg.time - wolfcam_prevTimes[16 - 16]) * 1000;
        yv = (cent->lerpOrigin[1] - wolfcam_prevOrigins[16 - 16][1]) / (cg.time - wolfcam_prevTimes[16 - 16]) * 1000;
        wolfcam_xyspeed = sqrt ( xv * xv   +   yv * yv);
        //wolfcam_xyspeed = sqrt(xv * xv)   +  sqrt(yv * yv);
        //wolfcam_xyspeed = sqrt (xv * yv);
        //wolfcam_xyspeed *= 1000;
        //wolfcam_xyspeed *= 1000;
    }
    //wolfcam_xyspeed = 300;
#endif

    // wc->psbobCyle from bg_pmove.c
#if 0  //FIXME wolfcam detect "crash landing"
    if (crash_land)  // PM_CrsahLand ()
        wc->psbobCycle = 0;
#endif
    // PM_Footsteps ()
    //  if mg42 pm->ps->persistant[PERS_HWEAPON_USE], swimming, DEAD, in the air, then return

    //FIXME wolfcam, if not trying to move:
#if 0
    if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
        if (  pm->xyspeed < 5 ) {
            pm->ps->bobCycle = 0;   // start at beginning of cycle again
        }
        if (pm->xyspeed > 120) {
            return; // continue what they were doing last frame, until we stop
        }
        /* etc .. */
    }
#endif

    if (wc->xyspeed < 5) {
        wc->psbobCycle = 0;
        bobmove = 0.0;
    } else if (0)  //(cs->eFlags & EF_CROUCHING)  //FIXME wolfcam-q3
        bobmove = 0.5;
    /*FIXME wolfcam else if (walking) {
      bobmove = 0.3;
}
    */
    else
        bobmove = 0.4;
    //pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;
    //FIXME wolfcam  pml.msec relation to cg.frametime ?
    //    wolfcam_psbobCycle = (int) (wolfcam_psbobCycle + bobmove * cg.frametime ) & 255;
    //wolfcam_psbobCycle = (int) (wolfcam_psbobCycle + bobmove * (cg.frametime * 1000)) & 255;
    wc->psbobCycle = (int) (wc->psbobCycle + bobmove * (cg.frametime * 1)) & 255;
    //Com_Printf ("wolfcam_psbobCycle: %d    ps->bobCycle: %d\n", wolfcam_psbobCycle, cg.predictedPlayerState.bobCycle);
    //Com_Printf ("wolfcam_xyspeed: %f  cg.xyspeed: %f\n", wolfcam_xyspeed, cg.xyspeed);


    if (wc->bobfracsin > 0  &&  !(wc->psbobCycle)) {
        wc->lastvalidBobcycle = wc->bobcycle;
        wc->lastvalidBobfracsin = wc->bobfracsin;
    }
    wc->bobcycle = (wc->psbobCycle & 128) >> 7;
    wc->bobfracsin = fabs( sin( (wc->psbobCycle & 127 ) / 127.0 * M_PI ));

    //Com_Printf ("wolfcam_bobcycle: %d  cg.bobcycle: %d\n", wolfcam_bobcycle, cg.bobcycle);
    //    Com_Printf ("wolfcam_bobfracsin: %f  cg.bobfracsin: %f\n", wolfcam_bobfracsin, cg.bobfracsin);

#if 0
    for (i = 1;  i < WOLFCAM_MAXBACKUPS;  i++) {
        memcpy (&(wolfcam_prevOrigins[i - 1]), &(wolfcam_prevOrigins[i]), sizeof(vec3_t));
        wolfcam_prevTimes[i - 1] = wolfcam_prevTimes[i];
        memcpy (&(wolfcam_prevEs[i - 1]), &(wolfcam_prevEs[i]), sizeof(entityState_t));
    }
    memcpy (&(wolfcam_prevOrigins[WOLFCAM_MAXBACKUPS - 1]), cent->lerpOrigin, sizeof(vec3_t));
    wolfcam_prevTimes[WOLFCAM_MAXBACKUPS - 1] = cg.time;
    memcpy (&wolfcam_esPrev, cs, sizeof(entityState_t));
    memcpy (&(wolfcam_prevEs[WOLFCAM_MAXBACKUPS - 1]), cs, sizeof(entityState_t));
#endif

    memcpy (os, cs, sizeof(entityState_t));

#if 0
    /////////////////////  sldkfjsldkfj
    if ((cs->eFlags & EF_TELEPORT_BIT) != (wolfcam_prevEs[WOLFCAM_MAXBACKUPS - 1].eFlags & EF_TELEPORT_BIT)) {
        Com_Printf ("^3teleport\n");
    }
#endif
}

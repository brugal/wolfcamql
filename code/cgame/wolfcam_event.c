#include "cg_local.h"

#include "cg_event.h"
#include "cg_main.h"
#include "cg_syscalls.h"
#include "wolfcam_event.h"

#include "wolfcam_local.h"

#if 0
static qboolean viewangles_have_changed (int clientNum, int snapshots)
{
    int i;
    qboolean angleChange;
    vec3_t angs;

    //FIXME what if clientNum and/or cg.snap->ps.clietNum have changed?
    if (wcg.curSnapshotNumber < snapshots)
        return qfalse;

    if (clientNum == cg.snap->ps.clientNum) {
        VectorCopy (cg.snap->ps.viewangles, angs);
    } else {
        VectorCopy (cg_entities[clientNum].currentState.apos.trBase, angs);
    }

    angleChange = qfalse;

    for (i = 1;  i <= snapshots;  i++) {
        //vec3_t a;
        const snapshot_t *s;
        int num;
        qboolean foundEnt;

        s = &wcg.snaps[(wcg.curSnapshotNumber - i) & MAX_SNAPSHOT_MASK];

        if (s->ps.clientNum == clientNum) {
            if (s->ps.viewangles[0] != angs[0]  ||  s->ps.viewangles[1] != angs[1]) {
                CG_Printf ("viewangles changed: -%d (%d)  %s  (%f %f)  (%f %f)\n",
                           i,
                           clientNum,
                           cgs.clientinfo[clientNum].name,
                           s->ps.viewangles[0],
                           s->ps.viewangles[1],
                           angs[0], angs[1]);
                return qtrue;
            }
            continue;
        }

        foundEnt = qfalse;
        for (num = 0;  num < s->numEntities;  num++) {
            const entityState_t *es;

            es = &s->entities[num];
            if (es->number == clientNum) {
                foundEnt = qtrue;
                // do check
                //CG_Printf ("found client\n");
                if (es->apos.trBase[0] != angs[0]  ||  es->apos.trBase[1] != angs[1]) {
                    CG_Printf ("viewangles changed: -%d  (%d)  %s  (%f %f)  (%f %f)\n",
                               i,
                               clientNum,
                               cgs.clientinfo[clientNum].name,
                               es->apos.trBase[0], es->apos.trBase[1],
                               angs[0], angs[1]);
                    return qtrue;
                }
                break;
            }
        }
        if (!foundEnt) {
            CG_Printf ("ent %d not even found\n", clientNum);
            return qfalse;
        }
    }

    return angleChange;
}
#endif

#if 1

/* returns -1 if no possible target found */
//FIXME duck, prone, and whatever shit
int find_best_target (int attackerClientNum, qboolean useLerp, vec_t *distance, vec3_t offsets)
{
    //clientInfo_t *tci;
    const centity_t *ac;
    const centity_t *tc;
    const entityState_t *aes;
    const entityState_t *tes;
    vec3_t avec;
    vec3_t tvec;
    vec3_t aangs;
    //vec3_t oldAangs;
    //vec3_t tangs;
    int possibleTarget;  // if -1, no targets found
    vec3_t dirVec;
    vec3_t dirAngs;
    vec3_t yxerr;
    vec_t dist = 0.0f;
    int i;
    int anim;

    possibleTarget = -1;

    ac = &cg_entities[attackerClientNum];
    aes = &ac->currentState;

    if ((!cgs.clientinfo[attackerClientNum].infoValid  ||  !ac->currentValid)  &&  attackerClientNum != cg.snap->ps.clientNum)
        return -1;

    if (useLerp) {
        VectorCopy (ac->lerpOrigin, avec);
        VectorCopy (ac->lerpAngles, aangs);
    } else {
        VectorCopy (aes->pos.trBase, avec);
        VectorCopy (aes->apos.trBase, aangs);
    }

    anim = aes->legsAnim & ~ANIM_TOGGLEBIT;
    if (anim == LEGS_WALKCR  ||  anim == LEGS_IDLECR)
        avec[2] += CROUCH_VIEWHEIGHT;
    else
        avec[2] += DEFAULT_VIEWHEIGHT;  //FIXME crouching or +8 ?

    possibleTarget = -1;

    for (i = 0;  i < MAX_CLIENTS;  i++) {
        //tci = &cgs.clientinfo[i];

        if ((!cgs.clientinfo[i].infoValid  ||  !cg_entities[i].currentValid)  &&  i != cg.snap->ps.clientNum)
            continue;
        //FIXME w team stuff
        if (cgs.clientinfo[i].team != TEAM_FREE)
            continue;

        if (i == attackerClientNum)
            continue;

        tc = &cg_entities[i];
        tes = &tc->currentState;
        if (useLerp) {
            VectorCopy (tc->lerpOrigin, tvec);
        } else {
            VectorCopy (tes->pos.trBase, tvec);
        }

        anim = tes->legsAnim & ~ANIM_TOGGLEBIT;
        if (anim == LEGS_WALKCR  ||  anim == LEGS_IDLECR)
            tvec[2] += CROUCH_VIEWHEIGHT;
        else
            tvec[2] += DEFAULT_VIEWHEIGHT;

        //        tvec[2] += 8;  //FIXME viewheight?

        VectorSubtract (tvec, avec, dirVec);
        vectoangles (dirVec, dirAngs);

        if (possibleTarget == -1) {
            possibleTarget = i;
            //AnglesSubtract (aangs, dirAngs, yxerr);
            AnglesSubtract (dirAngs, aangs, yxerr);
            dist = VectorLength (dirVec);
        } else {
            vec3_t tmpyxerr;

            //AnglesSubtract (aangs, dirAngs, tmpyxerr);
            AnglesSubtract (dirAngs, aangs, tmpyxerr);
            if (VectorLength(tmpyxerr) < VectorLength(yxerr)) {
                // better target found
                possibleTarget = i;
                VectorCopy (tmpyxerr, yxerr);
                dist = VectorLength (dirVec);
            }
        }
    }

    if (possibleTarget > -1  &&  !(cg_entities[possibleTarget].currentState.eFlags & EF_DEAD)) {
        VectorCopy (yxerr, offsets);
        //        *distance = VectorLength (dirVec);
        *distance = dist;
    } else
        possibleTarget = -1;

    return possibleTarget;
}
#endif

void wolfcam_log_event (const centity_t *cent, const vec3_t position)
{
    const entityState_t *es;
    int event;
    int clientNum;
    //clientInfo_t *ci;
    //char *s;
    //vec_t distance;

    es = &cent->currentState;
    event = es->event & ~EV_EVENT_BITS;

    clientNum = es->clientNum;
    if (clientNum < 0  ||  clientNum >= MAX_CLIENTS)
        return;

    //ci = &cgs.clientinfo[clientNum];

    //FIXME shotgun
    switch (event) {
    case EV_FIRE_WEAPON: {
#if 0
        int target;
        vec3_t offsets;

        //s = va("EV_FIRE_WEAPON: %d %d %d %s %s
        s = va("EV_FIRE_WEAPON\n");
        trap_FS_Write (s, strlen(s), wc_logfile);
        s = va("%d %d %s\n", clientNum, es->weapon, weapNames[es->weapon]);
        trap_FS_Write (s, strlen(s), wc_logfile);
        s = va("%s\n", cgs.clientinfo[clientNum].name);
        trap_FS_Write (s, strlen(s), wc_logfile);

        target = find_best_target (clientNum, qfalse, &distance, offsets);
        if (target > -1) {  //  &&  viewangles_have_changed(clientNum, 1)) {
            s = va("%d\n", target);
#if 0
            s = va("%d %f %f %f\n", target, cg_entities[target].currentState.pos.trBase[0],
                   cg_entities[target].currentState.pos.trBase[1],
                   cg_entities[target].currentState.pos.trBase[2]);
#endif
            trap_FS_Write (s, strlen(s), wc_logfile);
            s = va("%s\n", cgs.clientinfo[target].name);
            trap_FS_Write (s, strlen(s), wc_logfile);
            s = va("%f %f %f\n", offsets[YAW], offsets[PITCH], distance);
            trap_FS_Write (s, strlen(s), wc_logfile);

#if 0
            // testing
            s = va("%f %f %f    %f %f %f  %s\n",
                   cg_entities[clientNum].currentState.pos.trBase[0],
                   cg_entities[clientNum].currentState.pos.trBase[1],
                   cg_entities[clientNum].currentState.pos.trBase[2],
                   cg_entities[clientNum].currentState.apos.trBase[0],
                   cg_entities[clientNum].currentState.apos.trBase[1],
                   cg_entities[clientNum].currentState.apos.trBase[2],
                   cgs.clientinfo[clientNum].name);
            trap_FS_Write (s, strlen(s), wc_logfile);

            s = va("%f %f %f    %f %f %f  %s\n",
                   cg_entities[target].currentState.pos.trBase[0],
                   cg_entities[target].currentState.pos.trBase[1],
                   cg_entities[target].currentState.pos.trBase[2],
                   cg_entities[target].currentState.apos.trBase[0],
                   cg_entities[target].currentState.apos.trBase[1],
                   cg_entities[target].currentState.apos.trBase[2],
                   cgs.clientinfo[target].name);
            trap_FS_Write (s, strlen(s), wc_logfile);
#endif

        } else {
            s = va ("-1\n\n0.0 0.0 0.0\n");
            trap_FS_Write (s, strlen(s), wc_logfile);
        }

        s = va("---\n");
        trap_FS_Write (s, strlen(s), wc_logfile);
        break;
#endif  //
    }
        break;
    case EV_OBITUARY:
        if (es->otherEntityNum < MAX_CLIENTS) {
            wclients[es->otherEntityNum].deathTime = cg.time;
            wclients[es->otherEntityNum].eventHealth = 9999; //FIXME actually set to full
            //Com_Printf("setting 100 health %s\n", cgs.clientinfo[wcg.clientNum].name);
        }
        break;
    case EV_PAIN:
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}
        //Com_Printf("^3%d (pain %d)  %d  %s\n", cg.time, es->eventParm, clientNum, cgs.clientinfo[clientNum].name);
        if (es->eventParm != 0) {
            wclients[clientNum].eventHealth = es->eventParm;
            wclients[clientNum].ev_pain_time = cg.time;
        } else {
            CG_Printf("^3FIXME %d (pain %d)  %d  %s\n", cg.time, es->eventParm, clientNum, cgs.clientinfo[clientNum].name);
        }
        break;
    default:
        break;
    }
}

void Wolfcam_LogMissileHit (int weapon, const vec3_t origin, const vec3_t dir, int entityNum)
{
    const clientInfo_t *ci;
    const centity_t *cent;
    const entityState_t *es;
    int i;
    int possibleClient = -1;
    vec3_t dirVec;
    vec3_t dirAngs;
    vec3_t yxerr;
    vec3_t tmpyxerr;
    //vec_t dist;

    //FIXME grenade in newer ql versions have clientNum

    //CG_Printf ("Wolfcam_LogMissileHit: %d %s    %d %s\n", weapon, weapNames[weapon], entityNum, cgs.clientinfo[entityNum].name);

    //FIXME wolfcam for lightning also exclude by distance

    if (weapon != WP_LIGHTNING) { //  &&  weapon != WP_PLASMAGUN)  //FIXME wolfcam other weapons
        //return;
    }

    //CG_Printf ("about to check...\n");

    for (i = 0;  i < MAX_CLIENTS;  i++) {
        ci = &cgs.clientinfo[i];
        cent = &cg_entities[i];
        es = &cent->currentState;

        if (i == entityNum)
            continue;

        if (!ci->infoValid)
            continue;
        if (!cent->currentValid  &&  cg.snap->ps.clientNum != i)
            continue;
        //FIXME wolfcam team stuff
        //if (ci->team != TEAM_FREE)
        //    continue;
        if (ci->team == TEAM_SPECTATOR)
            continue;
        if (es->weapon != weapon)  //FIXME wolfcam only for lightning
            continue;

        //FIXME wolfcam viewheights ?

        //CG_Printf ("checking angles for %d  %s\n", i, ci->name);

        VectorSubtract (origin, es->pos.trBase, dirVec);
        vectoangles (dirVec, dirAngs);

        if (possibleClient == -1) {
            possibleClient = i;
            AnglesSubtract (dirAngs, es->apos.trBase, yxerr);
            //dist = VectorLength (dirVec);
        } else {
            AnglesSubtract (dirAngs, es->apos.trBase, tmpyxerr);
            if (VectorLength(tmpyxerr) < VectorLength(yxerr)) {
                // better attacker found
                //CG_Printf ("found better: %d  %s\n", i, cgs.clientinfo[i].name);
                possibleClient = i;
                VectorCopy (tmpyxerr, yxerr);
                //dist = VectorLength (dirVec);
            }
        }
    }

    //Com_Printf("Wolfcam log missile hit %d\n", entityNum);

    //FIXME wolfcam can be dead for other than lightning
    if (possibleClient > -1  &&  !(cg_entities[possibleClient].currentState.eFlags & EF_DEAD)) {
        //CG_Printf ("logging %s hit for %s\n", weapNames[weapon], cgs.clientinfo[possibleClient].name);
        //wclients[possibleClient].wstats[weapon].hits++;
        wclients[possibleClient].wstats[weapon].hits++;
        wclients[possibleClient].perKillwstats[weapon].hits++;
        if (wolfcam_following  &&  wcg.clientNum == possibleClient) {
            //FIXME check if entityNum client info valid?
            //Com_Printf("missile hit\n");
            if (cgs.gametype >= GT_TEAM) {
                if (cgs.clientinfo[wcg.clientNum].team != cgs.clientinfo[entityNum].team) {
                    //trap_S_StartLocalSound (cgs.media.hitSound, CHAN_LOCAL_SOUND);
                    wcg.playHitSound = qtrue;
                } else {
                    wcg.playTeamHitSound = qtrue;
                }
            } else {
                wcg.playHitSound = qtrue;
            }
        }

    } else {
        //CG_Printf ("couldn't find attacker for missile\n");
    }
}

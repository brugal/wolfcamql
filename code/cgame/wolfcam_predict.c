#include "cg_local.h"
#include "wolfcam_local.h"

extern int cg_numSolidEntities;
extern centity_t    *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];

/* clientNum for calcMuzzle */
void Wolfcam_WeaponTrace (trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask)
{
    int addedEntities;
    int i;

    //FIXME cg.snap->ps.clientNum special case, right now just adds automaticaly
    /* add players that have died this frame to check for killshot, and also
     * cg.snap->ps.clientNum
     */
    addedEntities = 0;
    for (i = 0;  i < MAX_CLIENTS;  i++) {
        if (i == cg.snap->ps.clientNum) {
            if (cg_entities[i].currentState.eType == ET_INVISIBLE) {
                if (cgs.gametype == GT_CA  &&  cg.clientNum != cg.snap->ps.clientNum) {
                    // pass
                } else {
                    continue;
                }
            }
#if 0
            if (cg_entities[i].currentState.eFlags & EF_DEAD) {
                continue;
            }
#endif
            if (wolfcam_following  &&  wcg.clientNum == i) {
                continue;
            }
            //CG_Printf("clientnum\n");
            if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR) {
                //CG_Printf("team: %d\n", cgs.clientinfo[cg.snap->ps.clientNum].team);
                goto mark_as_solid;
            }
        }
        if (!cgs.clientinfo[i].infoValid)
            continue;
        if (!cg_entities[i].currentValid  &&  i != cg.snap->ps.clientNum)
            continue;
        //if (cg_entities[i].currentState.solid == 4200463)
        //    continue;
        if (cg_entities[i].currentState.solid != 0)
            continue;
        //if (wclients[i].deathTime != cg.time  &&  i != cg.snap->ps.clientNum)
        //    continue;
mark_as_solid:
        wclients[i].oldSolid = cg_entities[i].currentState.solid;
        cg_entities[i].currentState.solid = 4200463;
        cg_solidEntities[cg_numSolidEntities] = &cg_entities[i];
        cg_numSolidEntities++;
        addedEntities++;
        if (i == cg.snap->ps.clientNum) {
            //CG_Printf("added cg.snap->ps.clientNum to solid list\n");
        }
        //CG_Printf ("--- %s\n", cgs.clientinfo[i].name);
    }

    CG_Trace (result, start, mins, maxs, end, skipNumber, mask);

#if 0
    CG_Printf ("--- %d  ", cg.time);
    for (i = 0;  i < cg_numSolidEntities;  i++) {
        int cn;

        cn = cg_solidEntities[i]->currentState.clientNum;
        if (cn >= MAX_CLIENTS)
            continue;

        if (!cgs.clientinfo[cg_solidEntities[i]->currentState.clientNum].infoValid)
            continue;
        CG_Printf ("[%s^7]  ", cgs.clientinfo[cg_solidEntities[i]->currentState.clientNum].name);
    }
    CG_Printf ("\n");
#endif

    for (i = cg_numSolidEntities - addedEntities;  i < cg_numSolidEntities;  i++) {
        cg_solidEntities[i]->currentState.solid = 0;
    }

    cg_numSolidEntities -= addedEntities;
}


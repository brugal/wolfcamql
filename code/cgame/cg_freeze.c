// freeze tag code

#include "cg_local.h"

qboolean CG_FreezeTagFrozen (int clientNum)
{
    if (cgs.gametype != GT_FREEZETAG) {
        return qfalse;
    }

    if (cg.snap->ps.pm_type == PM_INTERMISSION) {
        return qfalse;
    }

    //Com_Printf("%d  0x%x\n", clientNum, cg_entities[clientNum].currentState.eFlags);

#if 0
    if (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << clientNum)) {
        return (!(cg.warmup  ||  cg.predictedPlayerState.pm_type == PM_INTERMISSION));
    }
#endif

#if 0
    if (cgs.q3plus) {
        if (cg_entities[clientNum].currentState.powerups & (1 << PW_BATTLESUIT)) {
            return qtrue;
        } else {
            return qfalse;
        }
    }
#endif

    //if (cg_entities[clientNum].currentState.eFlags & EF_DEAD) {
    //if (cg_entities[clientNum].currentState.weapon == WP_NONE) {
    //if (cg_entities[clientNum].currentState.powerups & 0x8000) {
    if (cg_entities[clientNum].currentState.powerups & (1 << PW_FROZEN)) {
        return qtrue;
    }

    return qfalse;
}

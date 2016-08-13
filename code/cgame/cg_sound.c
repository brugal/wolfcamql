#include "cg_local.h"
#include "cg_main.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "wolfcam_local.h"

static qboolean CG_CheckCanPlaySound (const vec3_t origin, int entityNum, sfxHandle_t sfx)
{
    if (cg_cpmaSound.integer > 1  ||  (cg_cpmaSound.integer  &&  cgs.cpma)) {
        vec3_t ourOrigin;
        vec3_t soundOrigin;

        if (wolfcam_following) {
            VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trBase, ourOrigin);
        } else {
            VectorCopy(cg.snap->ps.origin, ourOrigin);
        }

        if (origin != NULL  &&  entityNum != 0) {
            //Com_Printf("^3cpma sound check:  origin != null and entityNum %d\n", entityNum);
        }

        if (origin == NULL) {
            if (entityNum == ENTITYNUM_WORLD) {
                Com_Printf("^3WARNING cpma sound WORLD entity and null origin\n");
                return qtrue;
            }
            if (entityNum < 0  ||  entityNum >= MAX_GENTITIES) {
                Com_Printf("^3WARNING cpma sound invalid entityNum: %d\n", entityNum);
                return qtrue;
            }
            VectorCopy(cg_entities[entityNum].currentState.pos.trBase, soundOrigin);
        } else {
            VectorCopy(origin, soundOrigin);
        }

        //FIXME check cpma mvd coach

        if (Distance(ourOrigin, soundOrigin) > 1280.0f) {
            if (cg_cpmaSound.integer > 2) {
                CG_Printf("^4cpma sound:  skipping, too far  %d\n", sfx);
                trap_S_PrintSfxFilename(sfx);
            }
            return qfalse;
        }

        if (!trap_R_inPVS(ourOrigin, soundOrigin)) {
            if (cg_cpmaSound.integer > 2) {
                CG_Printf("^4cpma sound:  skipping, not in pvs  %d\n", sfx);
                trap_S_PrintSfxFilename(sfx);
            }
            return qfalse;
        }

        //Com_Printf("^5yes... playing ent %d at origin %f %f %f  dist^7: %f\n", entityNum, soundOrigin[0], soundOrigin[1], soundOrigin[2], Distance(ourOrigin, soundOrigin));
        return qtrue;
    } else if ((cg_soundPvs.integer == 1  &&  cgs.realProtocol >= 91)  ||  cg_soundPvs.integer > 1) {
        // ql spec demos send entity info that isn't in pvs
        //FIXME duplicate code with cpma sound
        vec3_t ourOrigin;
        vec3_t soundOrigin;

        if (wolfcam_following) {
            VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trBase, ourOrigin);
        } else {
            VectorCopy(cg.snap->ps.origin, ourOrigin);
        }

        if (origin != NULL  &&  entityNum != 0) {
            //Com_Printf("^3sound check:  origin != null and entityNum %d\n", entityNum);
        }

        if (origin == NULL) {
            if (entityNum == ENTITYNUM_WORLD) {
                Com_Printf("^3WARNING sound WORLD entity and null origin\n");
                return qtrue;
            }
            if (entityNum < 0  ||  entityNum >= MAX_GENTITIES) {
                Com_Printf("^3WARNING sound invalid entityNum: %d\n", entityNum);
                return qtrue;
            }
            VectorCopy(cg_entities[entityNum].currentState.pos.trBase, soundOrigin);
        } else {
            VectorCopy(origin, soundOrigin);
        }

        if (!trap_R_inPVS(ourOrigin, soundOrigin)) {
            if (cg_soundPvs.integer > 2) {
                CG_Printf("^4sound:  skipping, not in pvs  %d\n", sfx);
                trap_S_PrintSfxFilename(sfx);
            }
            return qfalse;
        }
    }

    return qtrue;
}

void CG_StartSound (const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx)
{
    if (CG_CheckCanPlaySound(origin, entityNum, sfx)) {
        trap_S_StartSound(origin, entityNum, entchannel, sfx);
    }
}

void CG_StartLocalSound (sfxHandle_t sfx, int channelNum)
{
    trap_S_StartLocalSound(sfx, channelNum);
}

void CG_AddLoopingSound (int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
    if (CG_CheckCanPlaySound(origin, entityNum, sfx)) {
        trap_S_AddLoopingSound(entityNum, origin, velocity, sfx);
    }
}

void CG_AddRealLoopingSound (int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
    if (CG_CheckCanPlaySound(origin, entityNum, sfx)) {
        trap_S_AddRealLoopingSound(entityNum, origin, velocity, sfx);
    }
}

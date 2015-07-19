#include "cg_local.h"
#include "cg_main.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "wolfcam_local.h"

static qboolean CG_CheckCanPlaySound (const vec3_t origin, int entityNum, sfxHandle_t sfx)
{
    if (cg_cpmaSound.integer > 1  ||  (cg_cpmaSound.integer  &&  cgs.cpma)) {
        //int ourClientNum;
        vec3_t ourOrigin;
        vec3_t soundOrigin;

        if (wolfcam_following) {
            //ourClientNum = wcg.clientNum;
            VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, ourOrigin);
        } else {
            //ourClientNum = cg.snap->ps.clientNum;
            VectorCopy(cg.snap->ps.origin, ourOrigin);
        }

        if (origin != NULL  &&  entityNum != 0) {
            //Com_Printf("^3cpma sound check:  origin != null and entityNum %d\n", entityNum);
        }

        if (origin == NULL) {
            if (entityNum == (MAX_GENTITIES - 1)) {
                //FIXME hack for freecam rail
                Com_Printf("^3FIXME cpma sound, hack for freecam rail\n");
                return qtrue;
            }
            if (entityNum == ENTITYNUM_WORLD) {
                Com_Printf("^3WARNING cpma sound WORLD entity and null origin\n");
                return qtrue;
            }
            if (entityNum < 0  ||  entityNum >= MAX_GENTITIES) {
                Com_Printf("^3WARNING cpma sound invalid entityNum: %d\n", entityNum);
                return qtrue;
            }
            VectorCopy(cg_entities[entityNum].lerpOrigin, soundOrigin);
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

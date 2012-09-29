#include "cg_local.h"
#include "wolfcam_local.h"

qboolean Wolfcam_CalcMuzzlePoint (int entityNum, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, qboolean useLerp)
{
    vec3_t f;
    centity_t *cent;
    int anim;

    cent = &cg_entities[entityNum];

    if (entityNum == cg.snap->ps.clientNum) {
        if (useLerp) {
            VectorCopy (cent->lerpOrigin, muzzle);
            muzzle[2] += cg.snap->ps.viewheight;
            AngleVectors (cent->lerpAngles, f, right, up);
            if (forward)
                VectorCopy (f, forward);
            VectorMA (muzzle, 14, f, muzzle);
            return qtrue;
        } else {
            VectorCopy (cg.snap->ps.origin, muzzle);
            muzzle[2] += cg.snap->ps.viewheight;
            AngleVectors (cg.snap->ps.viewangles, f, right, up);
            if (forward)
                VectorCopy (f, forward);
            VectorMA (muzzle, 14, f, muzzle);
            return qtrue;
        }
    }

    if (!cent->currentValid)
        return qfalse;

    if (useLerp) {
        VectorCopy (cent->lerpOrigin, muzzle);
        AngleVectors (cent->lerpAngles, f, right, up);
    } else {
        VectorCopy( cent->currentState.pos.trBase, muzzle );
        AngleVectors (cent->currentState.apos.trBase, f, right, up);
    }
    if (forward)
        VectorCopy (f, forward);

    anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
    if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
        muzzle[2] += CROUCH_VIEWHEIGHT;
    } else {
        muzzle[2] += DEFAULT_VIEWHEIGHT;
    }

    VectorMA (muzzle, 14, f, muzzle);

    return qtrue;
}

void Wolfcam_AddViewWeapon (void)
{
    vec3_t origin;
    centity_t *cent;
    entityState_t *es;

    if (!wolfcam_following) {
        return;
    }

    cent = &cg_entities[wcg.clientNum];
    es = &cent->currentState;

    if (es->eFlags & EF_FIRING  &&  es->weapon == WP_LIGHTNING) {
        if (1) {  //FIXME check for draw gun
            // special hack for lightning gun...
            VectorCopy( cg.refdef.vieworg, origin );
            VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
            CG_LightningBolt( &cg_entities[es->number], origin );
        }
        //trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
        trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cg_weapons[es->weapon].firingSound );
    }
}

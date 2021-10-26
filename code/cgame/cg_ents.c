// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

#include "cg_ents.h"
#include "cg_effects.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_players.h"
#include "cg_predict.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_ents.h"

#include "wolfcam_local.h"

static void CG_Item ( centity_t *cent );

void CG_PrintEntityStatep (const entityState_t *ent)
{
	Com_Printf("number %d\n", ent->number);
	Com_Printf("eType %d\n", ent->eType);
	Com_Printf("eFlags %d\n", ent->eFlags);

	Com_Printf("pos.trType %d\n", ent->pos.trType);
	Com_Printf("pos.trTime %d\n", ent->pos.trTime);
	Com_Printf("pos.trDuration %d\n", ent->pos.trDuration);
	Com_Printf("pos.trBase %f %f %f\n", ent->pos.trBase[0], ent->pos.trBase[1], ent->pos.trBase[2]);
	Com_Printf("pos.trDelta %f %f %f\n", ent->pos.trDelta[0], ent->pos.trDelta[1], ent->pos.trDelta[2]);
	Com_Printf("pos.gravity %d\n", ent->pos.gravity);
	Com_Printf("apos.trType %d\n", ent->apos.trType);
	Com_Printf("apos.trTime %d\n", ent->apos.trTime);
	Com_Printf("apos.trDuration %d\n", ent->apos.trDuration);
	Com_Printf("apos.trBase %f %f %f\n", ent->apos.trBase[0], ent->apos.trBase[1], ent->apos.trBase[2]);
	Com_Printf("apos.trDelta %f %f %f\n", ent->apos.trDelta[0], ent->apos.trDelta[1], ent->apos.trDelta[2]);
	Com_Printf("apos.gravity %d\n", ent->apos.gravity);

	Com_Printf("time %d\n", ent->time);
	Com_Printf("time2 %d\n", ent->time2);

	Com_Printf("origin %f %f %f\n", ent->origin[0], ent->origin[1], ent->origin[2]);
	Com_Printf("origin2 %f %f %f\n", ent->origin2[0], ent->origin2[1], ent->origin2[2]);

	Com_Printf("angles %f %f %f\n", ent->angles[0], ent->angles[1], ent->angles[2]);
	Com_Printf("angles2 %f %f %f\n", ent->angles2[0], ent->angles2[1], ent->angles2[2]);

	Com_Printf("otherEntityNum %d\n", ent->otherEntityNum);
	Com_Printf("otherEntityNum2 %d\n", ent->otherEntityNum2);
	Com_Printf("groundEntityNum %d\n", ent->groundEntityNum);
	Com_Printf("constantLight %d\n", ent->constantLight);  //FIXME r g b intensity
	Com_Printf("loopSound %d\n", ent->loopSound);
	Com_Printf("modelindex %d\n", ent->modelindex);
	Com_Printf("modelindex2 %d\n", ent->modelindex2);
	Com_Printf("clientNum %d\n", ent->clientNum);
	Com_Printf("frame %d\n", ent->frame);
	Com_Printf("solid %d\n", ent->solid);
	Com_Printf("event %d\n", ent->event);
	Com_Printf("eventParm %d\n", ent->eventParm);
	Com_Printf("powerups %d\n", ent->powerups);
	Com_Printf("weapon %d\n", ent->weapon);
	Com_Printf("legsAnim %d\n", ent->legsAnim);
	Com_Printf("torsoAnim %d\n", ent->torsoAnim);
	Com_Printf("generic1 %d\n", ent->generic1);
	Com_Printf("jumpTime %d\n", ent->jumpTime);
	Com_Printf("doubleJumped %d\n", ent->doubleJumped);
	Com_Printf("health %d\n", ent->health);
	Com_Printf("armor %d\n", ent->armor);
	Com_Printf("location %d\n", ent->location);
	Com_Printf("--------------------------\n");
}

void CG_PrintEntityState (int n)
{
	Com_Printf("*entity %d\n", n);
	CG_PrintEntityStatep(&cg_entities[n].currentState);
}

qboolean CG_AllowedAmbientSound (qhandle_t h)
{
	int i;

	for (i = 0;  i < cg.numAllowedAmbientSounds;  i++) {
		if (h == cg.allowedAmbientSounds[i]) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName ) {
	int				i;
	orientation_t	lerped;

	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	//MatrixMultiply( lerped.axis, parent->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( const centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( const centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	//if (cent->currentState.eType == ET_MISSILE  &&  cent->currentState.weapon == WP_PROX_LAUNCHER) {
	if (cent->currentState.loopSound == cg.proxTickSoundIndex) {
		//Com_Printf("prox\n");
		if (cg_proxMineTick.integer) {
			CG_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[cent->currentState.loopSound]);
			//CG_AddRealLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[cent->currentState.loopSound]);
		}
	} else if (cent->currentState.loopSound) {

		//Com_Printf("%d loop %d  '%s'\n", cent->currentState.number, cent->currentState.loopSound, CG_ConfigString(CS_SOUNDS + cent->currentState.loopSound - (cgs.protocol == PROTOCOL_QL ? 1 : 0)));

		if (cent->currentState.eType != ET_SPEAKER) {
			//Com_Printf("^3adding loop sound to %d  %d  %s\n", cent->currentState.number, cent->currentState.loopSound, CG_ConfigString( CS_SOUNDS + cent->currentState.loopSound));
			//Com_Printf("^3                         %d  %s\n", cent->currentState.loopSound - 1, CG_ConfigString(CS_SOUNDS + cent->currentState.loopSound - 1));
			if (cg_ambientSounds.integer == 1  ||  (cg_ambientSounds.integer == 2  &&  CG_AllowedAmbientSound(cgs.gameSounds[cent->currentState.loopSound]))) {
				//Com_Printf("loop not speaker playing %d\n", cent->currentState.loopSound);
				//Com_Printf("%s  %d %d\n", CG_ConfigString( CS_SOUNDS+cent->currentState.loopSound - 1 ), cent->currentState.eType, cent->currentState.weapon);
				CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound] );
			} else {
				//trap_S_StopLoopingSound(cent->currentState.number);
			}
		} else {
			//FIXME this is wrong, maybe not

			// q3
			//Com_Printf("ceenteffe addrealloopsound %d  %s\n", cent->currentState.loopSound, CG_ConfigString( CS_SOUNDS + cent->currentState.loopSound));

			// ql
			//Com_Printf("                           %d  %s\n", cent->currentState.loopSound - 1, CG_ConfigString( CS_SOUNDS + cent->currentState.loopSound - 1));

			if (cg_ambientSounds.integer == 1  ||  (cg_ambientSounds.integer == 2  &&  CG_AllowedAmbientSound(cgs.gameSounds[cent->currentState.loopSound]))) {
				//Com_Printf("playing speaker sound %f %f %f\n", cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2]);
				CG_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ] );
				//Com_Printf("^3real: %s  %d %d\n", CG_ConfigString( CS_SOUNDS+cent->currentState.loopSound - 1 ), cent->currentState.eType, cent->currentState.weapon);
			} else {
				//trap_S_StopLoopingSound(cent->currentState.number);
			}
		}
	}


	// constant light glow
	if ( cent->currentState.constantLight ) {
		int		cl;
		float		i, r, g, b;

		cl = cent->currentState.constantLight;

		//Com_Printf("constant light for %d  (%d)\n", cent->currentState.number, cl);

#if 0
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;
#endif

		r = (float) (cl & 0xFF) / 255.0;
		g = (float) ((cl >> 8) & 0xFF) / 255.0;
		b = (float) ((cl >> 16) & 0xFF) / 255.0;
		i = (float) ((cl >> 24) & 0xFF) * 4.0;

		trap_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
	}

}

static void CG_DominationControlPoint (centity_t *cent)
{
	refEntity_t ent;
	float scale;
	int team;
	trace_t trace;
	vec3_t end;
	//float r, g, b, a;
	vec3_t color;
	float alpha;
	float size;
	int ourClientNum;
	int ourTeam;
	qhandle_t shader;
	int num;
	qboolean distress;
	qboolean altColor;
	//vec3_t dir;

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}
	ourTeam = cgs.clientinfo[ourClientNum].team;

	//team = cent->currentState.weapon;
	team = cent->currentState.modelindex2;
	num = cent->currentState.powerups;
	distress = cent->currentState.weapon;

	if (num < 6  &&  num > 0) {
		cgs.dominationControlPointEnts[num - 1] = cent;
	}

	alpha = 0.8;
	altColor = qfalse;

	if (team == ourTeam) {  // team
		if (*cg_dominationPointTeamColor.string) {
			SC_Vec3ColorFromCvar(color, &cg_dominationPointTeamColor);
			alpha = (float)cg_dominationPointTeamAlpha.integer / 255.0;
			altColor = qtrue;
		} else {
			if (team == TEAM_RED) {
				VectorSet(color, 1, 0, 0);
			} else if (team == TEAM_BLUE) {
				VectorSet(color, 0, 0.5, 1);  // 0x007be3  0x007bff
			} else {
				VectorSet(color, 0.65, 0.65, 0.65);
			}
		}
	} else if (team > 0) {  // enemy
		if (*cg_dominationPointEnemyColor.string) {
			SC_Vec3ColorFromCvar(color, &cg_dominationPointEnemyColor);
			alpha = (float)cg_dominationPointEnemyAlpha.integer / 255.0;
			altColor = qtrue;
		} else {
			if (team == TEAM_RED) {
				VectorSet(color, 1, 0, 0);
			} else if (team == TEAM_BLUE) {
				VectorSet(color, 0, 0.5, 1);  // 0x007be3  0x007bff
			} else {
				VectorSet(color, 0.65, 0.65, 0.65);
			}
		}
	} else {  // neutral
		if (*cg_dominationPointNeutralColor.string) {
			SC_Vec3ColorFromCvar(color, &cg_dominationPointNeutralColor);
			alpha = (float)cg_dominationPointNeutralAlpha.integer / 255.0;
			altColor = qtrue;
		} else {
			if (team == TEAM_RED) {
				VectorSet(color, 1, 0, 0);
			} else if (team == TEAM_BLUE) {
				VectorSet(color, 0, 0.5, 1);  // 0x007be3  0x007bff
			} else {
				VectorSet(color, 0.65, 0.65, 0.65);
			}
		}
	}

	// float sprite
	if (ourTeam != TEAM_SPECTATOR) {
		shader = 0;
		if (distress) {
			if (team == 0  ||  team != ourTeam) {
				switch (num) {
				case 1:
					shader = cgs.media.dominationCapADist;
					break;
				case 2:
					shader = cgs.media.dominationCapBDist;
					break;
				case 3:
					shader = cgs.media.dominationCapCDist;
					break;
				case 4:
					shader = cgs.media.dominationCapDDist;
					break;
				case 5:
					shader = cgs.media.dominationCapEDist;
					break;
				default:
					break;
				}
			} else if (team == ourTeam) {
				switch (num) {
				case 1:
					shader = cgs.media.dominationDefendADist;
					break;
				case 2:
					shader = cgs.media.dominationDefendBDist;
					break;
				case 3:
					shader = cgs.media.dominationDefendCDist;
					break;
				case 4:
					shader = cgs.media.dominationDefendDDist;
					break;
				case 5:
					shader = cgs.media.dominationDefendEDist;
					break;
				default:
					break;
				}
			}
		} else {
			if (team == 0  ||  team != ourTeam) {
				switch (num) {
				case 1:
					shader = cgs.media.dominationCapA;
					break;
				case 2:
					shader = cgs.media.dominationCapB;
					break;
				case 3:
					shader = cgs.media.dominationCapC;
					break;
				case 4:
					shader = cgs.media.dominationCapD;
					break;
				case 5:
					shader = cgs.media.dominationCapE;
					break;
				default:
					break;
				}
			} else if (team == ourTeam) {
				switch (num) {
				case 1:
					shader = cgs.media.dominationDefendA;
					break;
				case 2:
					shader = cgs.media.dominationDefendB;
					break;
				case 3:
					shader = cgs.media.dominationDefendC;
					break;
				case 4:
					shader = cgs.media.dominationDefendD;
					break;
				case 5:
					shader = cgs.media.dominationDefendE;
					break;
				default:
					break;
				}
			}
		}

		if (cg_helpIconStyle.integer > 0  &&  cg_helpIcon.integer != 0) {  //(team == 0  ||  team != ourTeam) {
			// capture
			//CG_PlayerFloatSprite(cent, cgs.media.dominationCapA);
			memset(&ent, 0, sizeof(ent));
			VectorCopy(cent->lerpOrigin, ent.origin);
			ent.origin[2] += 64 + 16;
			ent.reType = RT_SPRITE;
			ent.customShader = shader;
			ent.radius = 16;
			if (cg_helpIconStyle.integer == 1) {
				ent.radius = 16;
			} else if (cg_helpIconStyle.integer == 2) {
				ent.radius = 16;
				ent.renderfx = RF_DEPTHHACK;
			} else {
				vec3_t org;
				float dist;
				float minWidth;
				float maxWidth;
				float radius;

				ent.renderfx = RF_DEPTHHACK;
				if (wolfcam_following) {
					VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
				} else if (cg.freecam) {
					VectorCopy(cg.fpos, org);
				} else {
					VectorCopy(cg.refdef.vieworg, org);
				}

				minWidth = cg_helpIconMinWidth.value;
				maxWidth = cg_helpIconMaxWidth.value;

				radius = maxWidth / 2.0;
				dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

				if (minWidth > 0.1) {
					dist *= (16.0 / minWidth);
				}

				ent.radius = radius;

				if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
					ent.radius = radius * (Distance(ent.origin, org) / dist);
				}

				////
#if 0
				ent.radius = 16;

				dist = 400;
				if (Distance(ent.origin, org) > dist) {
					VectorSubtract(ent.origin, org, dir);
					VectorNormalize(dir);
					VectorMA(org, dist, dir, ent.origin);
				}
#endif
			}

			ent.origin[2] += ent.radius;

			ent.shaderRGBA[0] = 255 * color[0];
			ent.shaderRGBA[1] = 255 * color[1];
			ent.shaderRGBA[2] = 255 * color[2];
			ent.shaderRGBA[3] = 255 * alpha;
			CG_AddRefEntity(&ent);
		} else {  // no sprite
			// pass
		}
	}

	// domShadow
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 1000;
	CG_Trace(&trace, cent->lerpOrigin, NULL, NULL, end, -1, CONTENTS_SOLID);
	//CG_ImpactMark(cgs.media.energyMarkShader, tr.endpos, tr.plane.normal, random() * 360, color[0], color[1], color[2], 1, qtrue, 8, qfalse, qtrue, qfalse);
	//CG_ImpactMark(cgs.media.dominationFloorMark, trace.endpos, trace.plane.normal, random() * 360, 1, 1, 1, 1, qtrue, 60, qfalse, qtrue, qfalse);


	//trap_R_AddLightToScene(cent->lerpOrigin, r, g, b, a);
	trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand()&31), color[0], color[1], color[2]);

	size = DOMINATION_POINT_DISTANCE;

	if (!SC_Cvar_Get_Int("r_debugMarkSurface")) {
		CG_ImpactMark(cgs.media.dominationFloorMark, trace.endpos, trace.plane.normal, 0, color[0], color[1], color[2], alpha, qtrue, size, qtrue, qfalse, qfalse);
	}

	// show domination model
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_MODEL;
#if 0
	VectorCopy(cent->lerpOrigin, ent.lightingOrigin);
	VectorCopy(cent->lerpOrigin, ent.origin);
#endif
	VectorCopy(trace.endpos, ent.lightingOrigin);
	VectorCopy(trace.endpos, ent.origin);
	AnglesToAxis(cent->currentState.angles, ent.axis);

	ent.hModel = cgs.media.dominationModel;

	if (altColor) {
		ent.customSkin = cgs.media.dominationNeutralSkin;
	} else if (team == TEAM_RED) {
		ent.customSkin = cgs.media.dominationRedSkin;
	}
	else if (team == TEAM_BLUE) {
		ent.customSkin = cgs.media.dominationBlueSkin;
	}
	else {
		ent.customSkin = cgs.media.dominationNeutralSkin;
	}

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;

	CG_AddRefEntity(&ent);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_MODEL;
	//ent.frame = cent->currentState.frame;
	//ent.oldframe = ent.frame;
	//ent.backlerp = 0;
	//VectorCopy(cent->lerpOrigin, ent.oldorigin);

	scale = 0.005 + cent->currentState.number * 0.00001;
	// if cg_itemFx.integer & 0x1
	cent->lerpOrigin[2] += 4 + cos((cg.time + 1000) * scale) * 4;

	VectorCopy(cent->lerpOrigin, ent.lightingOrigin);
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	//AnglesToAxis(cent->lerpAngles, ent.axis);
	VectorCopy(cg.autoAngles, cent->lerpAngles);
	AxisCopy(cg.autoAxis, ent.axis);
	ent.nonNormalizedAxes = qfalse;

	// ent.renderfx |= RF_MINLIGHT;  // ?

	ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;

	CG_AddRefEntity(&ent);
}

static void CG_DrawRaceFlagBase (centity_t *cent)
{
	refEntity_t ent;
	vec3_t end;
	trace_t trace;
	vec3_t color;
	float alpha;
	int nextEnt;

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 1000;
	CG_Trace(&trace, cent->lerpOrigin, NULL, NULL, end, -1, CONTENTS_SOLID);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_MODEL;
#if 0
	VectorCopy(cent->lerpOrigin, ent.lightingOrigin);
	VectorCopy(cent->lerpOrigin, ent.origin);
#endif
	VectorCopy(trace.endpos, ent.lightingOrigin);
	VectorCopy(trace.endpos, ent.origin);
	AnglesToAxis(cent->currentState.angles, ent.axis);

	ent.hModel = cgs.media.dominationModel;

	alpha = 0.8;

	if (wolfcam_following) {
		nextEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
	} else {
		nextEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
	}

	if (cent->currentState.modelindex == cgs.endRaceFlagModel) {
		VectorSet(color, 1, 0, 0);
		ent.customSkin = cgs.media.dominationRedSkin;
	} else if (cent->currentState.modelindex == cgs.startRaceFlagModel) {
		//FIXME what if still at checkpoints
		VectorSet(color, 0.65, 0.65, 0.65);
		ent.customSkin = cgs.media.dominationNeutralSkin;
	} else {  // check point
		if (nextEnt == cent->currentState.number) {
			VectorSet(color, 0.196, 0.4, 0.96);  // colorBlue
			ent.customSkin = cgs.media.dominationBlueSkin;
		} else {
			VectorSet(color, 0.65, 0.65, 0.65);
			ent.customSkin = cgs.media.dominationNeutralSkin;
		}
	}

#if 0
	if (altColor) {
		ent.customSkin = cgs.media.dominationNeutralSkin;
	} else if (team == TEAM_RED) {
		ent.customSkin = cgs.media.dominationRedSkin;
	}
	else if (team == TEAM_BLUE) {
		ent.customSkin = cgs.media.dominationBlueSkin;
	}
	else {
		ent.customSkin = cgs.media.dominationNeutralSkin;
	}
#endif

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;

	CG_AddRefEntity(&ent);
}

static void CG_DrawRaceHelpIcon (centity_t *cent)
{
	refEntity_t ent;
	vec4_t color;
	float alpha;
	int nextEnt;

	if (cgs.gametype != GT_RACE) {
		return;
	}

	if (cg_helpIconStyle.integer == 0  ||  cg_helpIcon.integer == 0) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  &&  !wolfcam_following) {
		return;
	}

	if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
		return;
	}

	if (wolfcam_following) {
		nextEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
	} else {
		nextEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
	}

	if (nextEnt > 0  &&  nextEnt != cent->currentState.number) {
		return;
	}

	//Com_Printf("nextEnt %d\n", nextEnt);

	alpha = 0.8;

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 64 + 16;  //32;
	ent.reType = RT_SPRITE;

	if (nextEnt == 0  &&  cent->currentState.modelindex != cgs.startRaceFlagModel) {
		return;
	}

	if (cent->currentState.modelindex == cgs.startRaceFlagModel) {
		VectorSet(color, 0, 0.96, 0);
		ent.customShader = cgs.media.raceStart;
	} else if (cent->currentState.modelindex == cgs.endRaceFlagModel) {
		VectorSet(color, 0.96, 0, 0);
		ent.customShader = cgs.media.raceFinish;
	} else {  // check point
		VectorSet(color, 0.196, 0.4, 0.96);  // colorBlue
		ent.customShader = cgs.media.raceCheckPoint;
	}

	ent.radius = 16;
	if (cg_helpIconStyle.integer == 1) {
		ent.radius = 16;
	} else if (cg_helpIconStyle.integer == 2) {
		ent.radius = 16;
		ent.renderfx = RF_DEPTHHACK;
	} else {
		vec3_t org;
		float dist;
		float minWidth;
		float maxWidth;
		float radius;

		ent.renderfx = RF_DEPTHHACK;
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
		} else if (cg.freecam) {
			VectorCopy(cg.fpos, org);
		} else {
			VectorCopy(cg.refdef.vieworg, org);
		}

		minWidth = cg_helpIconMinWidth.value;
		maxWidth = cg_helpIconMaxWidth.value;

		radius = maxWidth / 2.0;
		dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

		if (minWidth > 0.1) {
			dist *= (16.0 / minWidth);
		}

		ent.radius = radius;

		if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
			ent.radius = radius * (Distance(ent.origin, org) / dist);
		}
	}

	ent.origin[2] += ent.radius;

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;
	CG_AddRefEntity(&ent);

}

/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t		*s1;
	float scale;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}


	if (cgs.gametype == GT_DOMINATION  &&  s1->modelindex == cgs.dominationControlPointModel) {
		CG_DominationControlPoint(cent);
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	if (cgs.gametype == GT_RACE) {
		if (s1->modelindex == cgs.startRaceFlagModel  ||  s1->modelindex == cgs.endRaceFlagModel  ||  s1->modelindex == cgs.checkPointRaceFlagModel) {
			int nextEnt;

			CG_DrawRaceFlagBase(cent);
			CG_DrawRaceHelpIcon(cent);

			if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
				return;
			}

			if (wolfcam_following) {
				nextEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
			} else {
				nextEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
			}

			// draw race flag

			if (s1->modelindex == cgs.startRaceFlagModel) {
				// always draw
			} else if (nextEnt == 0) {
				// draw all
			} else {
				if (nextEnt != cent->currentState.number  &&  s1->modelindex != cgs.startRaceFlagModel) {
					return;
				}
				if (s1->modelindex == cgs.checkPointRaceFlagModel) {
					ent.hModel = cgs.media.activeCheckPointRaceFlagModel;
				}
			}


			//CG_Item(cent);

			//FIXME duplicate code

			// items bob up and down continuously
			scale = 0.005 + cent->currentState.number * 0.00001;
			if (cg_itemFx.integer & 0x1) {
				cent->lerpOrigin[2] += 4 + cos( ( cg.time + 1000 ) *  scale ) * 4;
			}

			VectorCopy( cent->lerpOrigin, ent.origin);
			VectorCopy( cent->lerpOrigin, ent.oldorigin);

			// autorotate at one of two speeds
			if (cg_itemFx.integer & 0x2) {
				VectorCopy( cg.autoAngles, cent->lerpAngles );
				AxisCopy( cg.autoAxis, ent.axis );
			} else {
				VectorCopy(cent->currentState.apos.trBase, cent->lerpAngles);
				AnglesToAxis(cent->lerpAngles, ent.axis);
			}

			ent.nonNormalizedAxes = qfalse;
			if (cg_itemsWh.integer) {
				ent.renderfx |= RF_DEPTHHACK;
			}
		}
	}

	// add to refresh list
	CG_AddRefEntity(&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	int n;

	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	n = cent->currentState.eventParm;


	if (cg_ambientSounds.integer == 1  ||  cg_ambientSounds.integer == 3  ||  (cg_ambientSounds.integer == 2  &&  CG_AllowedAmbientSound(cgs.gameSounds[n]))) {
		//Com_Printf("CG_Speaker()  %d speaker %d  %s\n", cent->currentState.number, n, CG_ConfigString(CS_SOUNDS + n - 1));

		CG_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[n] );
	}

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

static void CG_DrawFlagHelpIcon (const centity_t *cent, const gitem_t *item)
{
	int ourTeam;
	int ourClientNum;
	//centity_t *pcent;
	refEntity_t ent;
	vec4_t color;
	float alpha;
	//vec3_t dir;
	qhandle_t shader;

	if (cgs.gametype == GT_RACE) {
		return;
	}

	if (cg_helpIconStyle.integer == 0  ||  cg_helpIcon.integer == 0) {
		return;
	}

	if (cgs.gametype != GT_CTFS  &&  cgs.gametype != GT_CTF  &&  cgs.gametype != GT_NTF) {  //  &&  cgs.gametype != GT_1FCTF) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  &&  !wolfcam_following) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	//FIXME cg_freecam_useTeamSettings
	//pcent = &cg_entities[ourClientNum];
	ourTeam = cgs.clientinfo[ourClientNum].team;
	//Com_Printf("our team: %d\n", ourTeam);

	shader = 0;
	alpha = 255.0;
	if (item->giTag == PW_REDFLAG) {
		VectorSet(color, 1, 0, 0);
		if (cgs.gametype == GT_CTFS) {
			if (ourTeam == TEAM_RED) {
				if (cgs.roundTurn % 2 == 0) {  // we are attacking
					return;
				}
				shader = cgs.media.adDefend;
			} else {
				if (cgs.roundTurn % 2 == 0) {  // we are defending
					return;
				}
				shader = cgs.media.adAttack;
				//Com_Printf("attack.... %d\n", cgs.media.adAttack);
			}
		} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_NTF) {
			//FIXME flag status
			if (ourTeam == TEAM_RED) {
				shader = cgs.media.adDefend;
			} else {
				shader = cgs.media.adCapture;
			}
		}
	} else if (item->giTag == PW_BLUEFLAG) {
		VectorSet(color, 0, 0.5, 1);
		if (cgs.gametype == GT_CTFS) {
			if (ourTeam == TEAM_RED) {
				if (cgs.roundTurn % 2 != 0) {  // we are defending
					return;
				}
				shader = cgs.media.adAttack;
			} else {
				if (cgs.roundTurn % 2 != 0) {  // we are attacking
					return;
				}
				shader = cgs.media.adDefend;
			}
		} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_NTF) {
			//FIXME flag status
			if (ourTeam == TEAM_BLUE) {
				shader = cgs.media.adDefend;
			} else {
				shader = cgs.media.adCapture;
			}
		}
	} else {
		VectorSet(color, 0.65, 0.65, 0.65);
	}

	memset(&ent, 0, sizeof(ent));
	// don't use cent->lerpOrigin so icon doesn't bob
	VectorCopy(cent->currentState.pos.trBase, ent.origin);

	ent.origin[2] += 64 + 16;  //32;
	ent.reType = RT_SPRITE;
	//ent.customShader = cgs.media.harvesterCapture;
	ent.customShader = shader;
	ent.radius = 16;
	if (cg_helpIconStyle.integer == 1) {
		ent.radius = 16;
	} else if (cg_helpIconStyle.integer == 2) {
		ent.radius = 16;
		ent.renderfx = RF_DEPTHHACK;
	} else {
		vec3_t org;
		float dist;
		float minWidth;
		float maxWidth;
		float radius;

		ent.renderfx = RF_DEPTHHACK;
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
		} else if (cg.freecam) {
			VectorCopy(cg.fpos, org);
		} else {
			VectorCopy(cg.refdef.vieworg, org);
		}

		minWidth = cg_helpIconMinWidth.value;
		maxWidth = cg_helpIconMaxWidth.value;

		radius = maxWidth / 2.0;
		dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

		if (minWidth > 0.1) {
			dist *= (16.0 / minWidth);
		}

		ent.radius = radius;

		if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
			ent.radius = radius * (Distance(ent.origin, org) / dist);
		}

#if 0
		ent.radius = 16;

		dist = 400;
		if (Distance(ent.origin, org) > dist) {
			VectorSubtract(ent.origin, org, dir);
			VectorNormalize(dir);
			VectorMA(org, dist, dir, ent.origin);
		}
#endif
	}

	ent.origin[2] += ent.radius;

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;
	CG_AddRefEntity(&ent);
	//Com_Printf("adding ent %f  radius: %f\n", cg.ftime, ent.radius);
}

static void CG_TieredArmorAvailability (const centity_t *cent, const gitem_t *item)
{
	enum { greenAmor = 0, yellowArmor, redArmor };
	int ourArmorAmount;
	int ourArmorType;
	qboolean available;

	if (cg_drawTieredArmorAvailability.integer == 0) {
		return;
	}

	if (wolfcam_following) {
		return;
	}

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (!cgs.armorTiered) {
		return;
	}

	if (cg.freecam) {
		return;
	}

	if (item != cgs.greenArmorItem  &&  item != cgs.yellowArmorItem  &&  item != cgs.redArmorItem) {
		return;
	}

	ourArmorAmount = cg.snap->ps.stats[STAT_ARMOR];
	ourArmorType = cg.snap->ps.stats[STAT_ARMOR_TIER];

	if (ourArmorType != redArmor  &&  ourArmorType != yellowArmor) {
		return;
	}

	available = qtrue;
	switch (ourArmorType) {
	case redArmor:
		if (item == cgs.yellowArmorItem  &&  ourArmorAmount > 132) {
			available = qfalse;
		} else if (item == cgs.greenArmorItem  &&  ourArmorAmount > 66) {
			available = qfalse;
		}
		break;
	case yellowArmor:
		if (item == cgs.greenArmorItem  &&  ourArmorAmount > 75) {
			available = qfalse;
		}
		break;
	default:
		break;
	}

	if (!available) {
		vec3_t dir;
		vec3_t origin;

		VectorCopy(cent->currentState.pos.trBase, origin);

		if (cg.freecam) {
			VectorSubtract(cg.freecamPlayerState.origin, origin, dir);
		} else {
			VectorSubtract(cg.snap->ps.origin, origin, dir);
		}

		VectorNormalize(dir);
		VectorMA(origin, 32, dir, origin);
		origin[2] += 10;

		CG_FloatSprite(cgs.media.unavailableShader, origin, 0, NULL, 12);
	}
}

static void CG_DrawTimerPie (const centity_t *cent)
{
	const gitem_t *item;
	const entityState_t *es;
	refEntity_t ent;
	int i;
	int sliceSize;
	int totalTime;
	int currentSlice;
	qhandle_t currentShader;
	int num5Chunks;

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (cg_itemTimers.integer == 0) {
		return;
	}

	es = &cent->currentState;
	if (es->modelindex <= 0) {
		return;
	}


	item = &bg_itemlist[es->modelindex];

	// time:  time when it will respawn
	// time2:  spawn length

	if (es->time <= 0  ||  es->time2 <= 0) {
		// will be zero with quad initial spawn
		return;
	}

	//FIXME is it all powerups?  regen and flight not in shader list.
	// 2015-07-09 regen is ok
	if (! (
		   item->giType == IT_POWERUP  ||  (item->giType == IT_ARMOR  &&  item->quantity > 5)  ||   (item->giType == IT_HEALTH  &&  item->quantity >= 100)  ||  (item->giType == IT_HOLDABLE  &&  item->giTag == HI_MEDKIT)
		   )
		) {
		return;
	}

	// icon

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.radius = 3.5 * cg_itemTimersScale.value;
	if (ent.radius < 0) {
		return;
	}
	if (cg_itemTimers.integer == 2) {
		ent.renderfx |= RF_DEPTHHACK;
	}

	//FIXME ql uses different icons?
	ent.customShader = cg_items[es->modelindex].icon;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = cg_itemTimersAlpha.integer;

	ent.origin[2] += cg_itemTimersOffset.value;

	CG_AddRefEntity(&ent);

	// slices
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.radius = 14.0 * cg_itemTimersScale.value;
	if (ent.radius < 0) {
		return;
	}
	if (cg_itemTimers.integer == 2) {
		ent.renderfx |= RF_DEPTHHACK;
	}

	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = cg_itemTimersAlpha.integer;

	ent.origin[2] += cg_itemTimersOffset.value;

	totalTime = es->time2;
	// testing
	//totalTime = 15000;

	num5Chunks = (totalTime / 1000) / 5;

#if 0
	if (num5Chunks < 1) {
		return;
	}
#endif

	sliceSize = num5Chunks;

	if (num5Chunks < 5) {  // less than 25 seconds
		// this will not work based on chunks of 5 seconds
		// just divide up into 5 chunks of whatever time
		sliceSize = 5;
		ent.customShader = cgs.media.wcTimerSlice5;
		currentShader = cgs.media.wcTimerSlice5Current;
		ent.shaderRGBA[0] = 200;
		ent.shaderRGBA[1] = 150;
		ent.shaderRGBA[2] = 50;
		ent.shaderRGBA[3] = cg_itemTimersAlpha.integer;
	} else if (num5Chunks == 5) {  // equal to 25 seconds
		ent.customShader = cgs.media.timerSlice5;
		currentShader = cgs.media.timerSlice5Current;
	} else if (num5Chunks > 5  &&  num5Chunks <= 7) {
		ent.customShader = cgs.media.timerSlice7;
		currentShader = cgs.media.timerSlice7Current;
	} else if (num5Chunks > 7  &&  num5Chunks <= 12) {
		ent.customShader = cgs.media.timerSlice12;
		currentShader = cgs.media.timerSlice12Current;
	} else {
		ent.customShader = cgs.media.timerSlice24;
		currentShader = cgs.media.timerSlice24Current;
	}


	// totalTime checked for zero above
	currentSlice = floor(((float)(cg.time - (es->time - totalTime)) / (float)(totalTime)) * (float)sliceSize);


	//CG_Printf("current slice: %d  cgtime: %d  (%d : %d)\n", currentSlice, cg.time, es->time, es->time2);

	// start at upper right
	ent.rotation = +((360 / sliceSize) * (sliceSize / 2));
	for (i = 0;  i < sliceSize;  i++) {
		if (i == currentSlice) {
			// hack for time less than 25sec
			if (num5Chunks < 5) {
				ent.shaderRGBA[2] = 200;
			}
			ent.customShader = currentShader;
		}
		CG_AddRefEntity(&ent);
		if (i == currentSlice) {
			break;
		}
		// rotate right
		ent.rotation -= (360 / sliceSize);
	}
}

static void CG_DrawPowerupRespawnPOI (const centity_t *cent)
{
	const gitem_t *item;
	const entityState_t *es;
	refEntity_t ent;
	int timeLeft;
	vec3_t org;
	float minWidth, maxWidth, radius, dist;

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (cg_drawPowerupRespawn.integer == 0) {
		return;
	}

	es = &cent->currentState;
	if (es->modelindex <= 0) {
		return;
	}

	item = &bg_itemlist[es->modelindex];

	// time:  time when it will respawn
	// time2:  spawn length

	if (es->time <= 0  ||  es->time2 <= 0) {
		// will be zero with quad initial spawn
		return;
	}

	if (item->giType != IT_POWERUP) {
		return;
	}

	timeLeft = es->time - cg.time;
	if (timeLeft <= 0) {
		return;
	} else if (timeLeft > 10000) {
		return;
	}

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.radius = 14.0 * cg_drawPowerupRespawnScale.value;
	if (ent.radius < 0) {
		return;
	}

	//FIXME duplicate code
	// distance hack
	if (wolfcam_following) {
		VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
	} else if (cg.freecam) {
		VectorCopy(cg.fpos, org);
	} else {
		VectorCopy(cg.refdef.vieworg, org);
	}

	minWidth = 14.0 * cg_drawPowerupRespawnScale.value;
	maxWidth = 14.0 * cg_drawPowerupRespawnScale.value;

	radius = maxWidth / 2.0;
	dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

	if (minWidth > 0.1) {
		dist *= (16.0 / minWidth);
	}

	ent.radius = radius;

	if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
		ent.radius = radius * (Distance(ent.origin, org) / dist);
	}

	ent.renderfx |= RF_DEPTHHACK;

	ent.customShader = cgs.media.powerupIncoming;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = cg_drawPowerupRespawnAlpha.integer;

	ent.origin[2] += cg_drawPowerupRespawnOffset.value;

	CG_AddRefEntity(&ent);
}

static void CG_DrawPowerupAvailable (const centity_t *cent)
{
	const gitem_t *item;
	const entityState_t *es;
	refEntity_t ent;
	vec3_t org;
	float minWidth, maxWidth, radius, dist;
	qhandle_t shader;
	float fadeStart;
	float fadeEnd;
	float frac;
	float total;

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (cg_drawPowerupAvailable.integer == 0) {
		return;
	}

	es = &cent->currentState;
	if (es->modelindex <= 0) {
		return;
	}
	if (es->eFlags & EF_NODRAW) {
		return;
	}

	item = &bg_itemlist[es->modelindex];

	if (item->giType != IT_POWERUP) {
		return;
	}

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.radius = 14.0 * cg_drawPowerupAvailableScale.value;
	if (ent.radius < 0) {
		return;
	}

	//FIXME duplicate code
	// distance hack
	if (wolfcam_following) {
		VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
	} else if (cg.freecam) {
		VectorCopy(cg.fpos, org);
	} else {
		VectorCopy(cg.refdef.vieworg, org);
	}

	minWidth = 14.0 * cg_drawPowerupAvailableScale.value;
	maxWidth = 14.0 * cg_drawPowerupAvailableScale.value;

	radius = maxWidth / 2.0;
	dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

	if (minWidth > 0.1) {
		dist *= (16.0 / minWidth);
	}

	ent.radius = radius;

	if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
		ent.radius = radius * (Distance(ent.origin, org) / dist);
	}

	// now distance fade if too close
	fadeStart = cg_drawPowerupAvailableFadeStart.value;
	fadeEnd = cg_drawPowerupAvailableFadeEnd.value;

	dist = Distance(ent.origin, org);
	if (dist < fadeEnd) {
		return;
	}
	if (dist < fadeStart) {
		total = fadeStart - fadeEnd;
		if (total > 0.0f) {
			frac = (fadeStart - dist) / total;
		} else {
			// invalid values for fadeStart and/or fadeEnd, ignoring
			frac = 0.0f;
		}
		ent.shaderRGBA[3] = 255 - (255.0f * frac);
	} else {
		ent.shaderRGBA[3] = 255;
	}

	ent.renderfx |= RF_DEPTHHACK;

	if (item->giTag == PW_QUAD) {
		shader = cgs.media.quadAvailable;
	} else if (item->giTag == PW_BATTLESUIT) {
		shader = cgs.media.bsAvailable;
	} else if (item->giTag == PW_HASTE) {
		shader = cgs.media.hasteAvailable;
	} else if (item->giTag == PW_INVIS) {
		shader = cgs.media.invisAvailable;
	} else if (item->giTag == PW_REGEN) {
		shader = cgs.media.regenAvailable;
	} else if (item->giTag == PW_FLIGHT) {
		//FIXME not in ql, draw something
		// 2015-07-16 no, looks bad in demos, will make it look like
		// the powerup is right in front of you, draw own icon or skip
		//shader = cg_items[es->modelindex].icon;

		return;
	} else {
		// icon shader looks bad
		//shader = cg_items[es->modelindex].icon;

		return;
	}

	ent.customShader = shader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] *= (cg_drawPowerupAvailableAlpha.value / 255.0f);

	ent.origin[2] += cg_drawPowerupAvailableOffset.value;

	CG_AddRefEntity(&ent);
}

/*
==================
CG_Item
==================
*/
static void CG_Item ( centity_t *cent ) {
	refEntity_t		ent;
	const entityState_t	*es;
	gitem_t			*item;
	int				msec;
	float			frac;
	float			scale;
	const weaponInfo_t	*wi;
	int modelindex;

	es = &cent->currentState;
	modelindex = es->modelindex;

	if (cgs.cpma  &&  cgs.gametype == GT_NTF) {
		// from em92:
		//    According to http://games.linuxdude.com/tamaps/archive/cpm1_dev_docs/step3.txt
		//    EF_BACKPACK is used to set dropped entity as backpack.
		//
		// non zero modelindex2 value for items indicates it is dropped
		//FIXME need to check for only certain giType?  (weapon, ammo, etc...)
		if (es->modelindex2 == 1  &&  es->eFlags == EF_BACKPACK) {
			modelindex = cgs.backpackItemIndex;
		}
	}

	if ( modelindex >= bg_numItems ) {
		//CG_Error( "Bad item index %i on entity", modelindex );
		CG_Printf( "Bad item index %i on entity\n", modelindex );
        //return;
	}

	// ql ingame icons
	if (es->eFlags & EF_NODRAW) {
		CG_DrawTimerPie(cent);
		CG_DrawPowerupRespawnPOI(cent);
	} else {
		CG_DrawPowerupAvailable(cent);
	}

	// if set to invisible, skip
	if ( !modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	item = &bg_itemlist[ modelindex ];

	if (cgs.gametype == GT_1FCTF) {
		if (item->giTag == PW_REDFLAG  ||  item->giTag == PW_BLUEFLAG) {
			return;
		}
	}

	if ( cg_simpleItems.integer && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14.0 * cg_simpleItemsScale.value;
		if (ent.radius < 0) {
			return;
		}
		if (cg_itemsWh.integer) {
			ent.renderfx |= RF_DEPTHHACK;
		}
		ent.customShader = cg_items[modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;

		if (cg_simpleItemsBob.integer) {
			scale = 0.005 + cent->currentState.number * 0.00001;
			cent->lerpOrigin[2] += 4 + cos( ( cg.time + 1000 ) *  scale ) * 4;
			ent.origin[2] = cent->lerpOrigin[2];
		}

		ent.origin[2] += cg_simpleItemsHeightOffset.value;

		CG_AddRefEntity(&ent);

		if (item->giType == IT_ARMOR) {
			CG_TieredArmorAvailability(cent, item);
		}
		return;
	}

	// items bob up and down continuously
	scale = 0.005 + cent->currentState.number * 0.00001;
	if (cg_itemFx.integer & 0x1) {
		cent->lerpOrigin[2] += 4 + cos( ( cg.time + 1000 ) *  scale ) * 4;
	}

	memset (&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if (cg_itemFx.integer & 0x2) {
		if ( item->giType == IT_HEALTH ) {
			VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
			AxisCopy( cg.autoAxisFast, ent.axis );
		} else {
			VectorCopy( cg.autoAngles, cent->lerpAngles );
			AxisCopy( cg.autoAxis, ent.axis );
		}
	} else {
		VectorCopy(cent->currentState.apos.trBase, cent->lerpAngles);
		AnglesToAxis(cent->lerpAngles, ent.axis);
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -=
			wi->weaponMidpoint[0] * ent.axis[0][0] +
			wi->weaponMidpoint[1] * ent.axis[1][0] +
			wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -=
			wi->weaponMidpoint[0] * ent.axis[0][1] +
			wi->weaponMidpoint[1] * ent.axis[1][1] +
			wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -=
			wi->weaponMidpoint[0] * ent.axis[0][2] +
			wi->weaponMidpoint[1] * ent.axis[1][2] +
			wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;	// an extra height boost
	}

	ent.hModel = cg_items[modelindex].models[0];

	// flag
	if (item->giType == IT_TEAM  &&  (item->giTag == PW_REDFLAG  ||  item->giTag == PW_BLUEFLAG  ||  item->giTag == PW_NEUTRALFLAG)) {
		if (cg_flagStyle.integer == 3  &&  item->giTag != PW_NEUTRALFLAG) {
			int clientNum;
			int team;

			if (wolfcam_following) {
				clientNum = wcg.clientNum;
			} else {
				clientNum = cg.snap->ps.clientNum;
			}
			team = cgs.clientinfo[clientNum].team;

			if ((cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0)  ||  cgs.gametype == GT_RACE) {
				team = TEAM_FREE;
			}

			ent.hModel = cgs.media.neutralFlagModel3;

			if (item->giTag == PW_REDFLAG) {
				ent.shaderRGBA[0] = 255;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 0;
				ent.shaderRGBA[3] = 255;
			} else if (item->giTag == PW_BLUEFLAG) {
				ent.shaderRGBA[0] = 0;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 255;
				ent.shaderRGBA[3] = 255;
			} else {
				ent.shaderRGBA[0] = 255;
				ent.shaderRGBA[1] = 255;
				ent.shaderRGBA[2] = 255;
				ent.shaderRGBA[3] = 255;
			}

			if (cg_useCustomRedBlueFlagColor.integer == 1  ||  cg_useCustomRedBlueFlagColor.integer == 2) {
				// fallback or override for enemy and teammate settings
				if (item->giTag == PW_REDFLAG) {
					if (*cg_redTeamFlagColor.string) {
						SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_redTeamFlagColor);
					}
				} else if (item->giTag == PW_BLUEFLAG) {
					if (*cg_blueTeamFlagColor.string) {
						SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_blueTeamFlagColor);
					}
				}
			}

			if (team == TEAM_RED  ||  team == TEAM_BLUE) {
				if (cg_useCustomRedBlueFlagColor.integer != 2) {
					if ((team == TEAM_RED  &&  item->giTag == PW_REDFLAG)  ||  (team == TEAM_BLUE  &&  item->giTag == PW_BLUEFLAG)) {
						// teammate color
						if (*cg_teamFlagColor.string) {
							SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_teamFlagColor);
							ent.shaderRGBA[3] = 255;
						}
					} else {
						// enemy color
						if (*cg_enemyFlagColor.string) {
							SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_enemyFlagColor);
							ent.shaderRGBA[3] = 255;
						}
					}
				}
			} else {  // team free or spec
#if 0
				//FIXME 2021-09-20 why is the model changed?  these are brighter?
				if (item->giTag == PW_REDFLAG) {
					ent.hModel = cgs.media.redFlagModel2;
				} else if (item->giTag == PW_BLUEFLAG) {
					ent.hModel = cgs.media.blueFlagModel2;
				} else {
					ent.hModel = cgs.media.neutralFlagModel2;
				}
#endif
			}
		} else if (cg_flagStyle.integer == 3  &&  item->giTag == PW_NEUTRALFLAG) {
			ent.hModel = cgs.media.neutralFlagModel3;
			SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_neutralFlagColor);
			ent.shaderRGBA[3] = 255;
		} else if (cg_flagStyle.integer == 2) {
			//ent.hModel = cg_items[modelindex].models[1];
			if (item->giTag == PW_REDFLAG) {
				ent.hModel = cgs.media.redFlagModel2;
			} else if (item->giTag == PW_BLUEFLAG) {
				ent.hModel = cgs.media.blueFlagModel2;
			} else {
				ent.hModel = cgs.media.neutralFlagModel2;
			}
		} else {
			ent.hModel = cg_items[modelindex].models[0];
		}
		CG_DrawFlagHelpIcon(cent, item);
#if 0
		ent.hModel = cgs.media.neutralFlagModel3;
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 255;
		VectorCopy(cent->lerpOrigin, ent.lightingOrigin);
		VectorCopy(cent->lerpOrigin, ent.origin);
		VectorCopy(cent->lerpOrigin, ent.oldorigin);
#endif
	}

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if (msec >= 0  &&  msec < ITEM_SCALEUP_TIME  &&  cg_itemFx.integer & 0x4) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		frac *= cg_itemSize.value;
		VectorScale( ent.axis[0], frac, ent.axis[0] );
		VectorScale( ent.axis[1], frac, ent.axis[1] );
		VectorScale( ent.axis[2], frac, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
		frac *= cg_itemSize.value;
		VectorScale( ent.axis[0], frac, ent.axis[0] );
		VectorScale( ent.axis[1], frac, ent.axis[1] );
		VectorScale( ent.axis[2], frac, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ( ( item->giType == IT_WEAPON ) ||
		 ( item->giType == IT_ARMOR ) ) {
		ent.renderfx |= RF_MINLIGHT;
	}
	if (cg_itemsWh.integer) {
		ent.renderfx |= RF_DEPTHHACK;
	}

	// increase the size of the weapons when they are presented as items
	if ( item->giType == IT_WEAPON ) {
		VectorScale( ent.axis[0], 1.5, ent.axis[0] );
		VectorScale( ent.axis[1], 1.5, ent.axis[1] );
		VectorScale( ent.axis[2], 1.5, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
#ifdef MISSIONPACK  //FIXME check a demo
		CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.weaponHoverSound );
#endif
	}

#if 1  //def MPACK
	if (item->giType == IT_HOLDABLE && ((!cgs.cpma  && item->giTag == HI_KAMIKAZE)  ||  (cgs.cpma  &&  item->giTag == HIC_KAMIKAZE))) {
		VectorScale( ent.axis[0], 2, ent.axis[0] );
		VectorScale( ent.axis[1], 2, ent.axis[1] );
		VectorScale( ent.axis[2], 2, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}
#endif

	if (item->giType == IT_WEAPON  &&  item->giTag == WP_RAILGUN) {
#if 0
		ent.shaderRGBA[0] = cg.enemyColors[1][0];
		ent.shaderRGBA[1] = cg.enemyColors[1][1];
		ent.shaderRGBA[2] = cg.enemyColors[1][2];
#endif
		SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_railItemColor);
		ent.shaderRGBA[3] = 255;
	}

	if (cgs.defrag  &&  cent->currentState.time & (1 << cg.snap->ps.clientNum)) {
		ent.customShader = cgs.media.defragItemShader;
	}

	// add to refresh list
	CG_AddRefEntity(&ent);

	if ( item->giType == IT_WEAPON && wi && wi->barrelModel ) {
		refEntity_t	barrel;
		vec3_t angles;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( ent.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = 0;
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		if (cg_itemsWh.integer) {
			barrel.renderfx |= RF_DEPTHHACK;
		}
		CG_AddRefEntity(&barrel);
	}

	// accompanying rings / spheres for powerups
	if ( !cg_simpleItems.integer  &&  !((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_NTF)  &&  item->giTag == PW_FLIGHT) )
	{
		vec3_t spinAngles;

		VectorClear( spinAngles );

		if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
		{
			if ( ( ent.hModel = cg_items[modelindex].models[1] ) != 0 )
			{
				if ( item->giType == IT_POWERUP )
				{
					ent.origin[2] += 12;
					spinAngles[1] = ( cg.time & 1023 ) * 360 / -1024.0f;
				}
				AnglesToAxis( spinAngles, ent.axis );

				// scale up if respawning
				if ( frac != 1.0 ) {
					VectorScale( ent.axis[0], frac, ent.axis[0] );
					VectorScale( ent.axis[1], frac, ent.axis[1] );
					VectorScale( ent.axis[2], frac, ent.axis[2] );
					ent.nonNormalizedAxes = qtrue;
				}
				if (cg_itemsWh.integer) {
					ent.renderfx |= RF_DEPTHHACK;
				}
				CG_AddRefEntity(&ent);
			}
		}
	}

	if (item->giType == IT_ARMOR) {
		CG_TieredArmorAvailability(cent, item);
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;
//	int	col;
	int currentClientNum;
	int contents;
	//int lastContents;
	//int ttime;
	vec3_t lastOrigin;
	int cgtime;

	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}

	if (cent->currentState.eFlags & EF_NODRAW) {
		return;
	}

	//CG_Printf("missile %p\n", cent);

	cgtime = cent->cgtime;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];
	if (!weapon->registered) {
		CG_RegisterWeapon(s1->weapon);
	}
	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

	if (EffectScripts.weapons[s1->weapon].hasProjectileScript) {
		//Com_Printf("projectile script\n");
		//BG_EvaluateTrajectoryDeltaf(&cent->currentState.pos, cgtime, ScriptVars.velocity, cg.foverf);
		//memset(&ScriptVars, 0, sizeof(ScriptVars));
		CG_ResetScriptVars();
		//memcpy(&ScriptVars.currentState, s1, sizeof(ScriptVars.currentState));
		if (s1->weapon == WP_GRENADE_LAUNCHER) {
			int clientNum;

			// older demos don't set clientNum
			//Com_Printf("^1other %d\n", s1->otherEntityNum);

			if (s1->otherEntityNum > 0) {
				clientNum = s1->otherEntityNum;
			} else {
				clientNum = s1->clientNum;
			}

			if (cgs.protocol == PROTOCOL_QL  &&  (clientNum >= 0  &&  clientNum < MAX_CLIENTS)) {
				CG_CopyPlayerDataToScriptData(&cg_entities[clientNum]);
			} else {
				ScriptVars.inEyes = qfalse;
				ScriptVars.clientNum = -2;
				ScriptVars.team = 0;
				ScriptVars.enemy = qtrue;
				ScriptVars.teamMate = qfalse;
			}
		}
		VectorCopy(s1->pos.trDelta, ScriptVars.velocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.dir);
		VectorNormalize(ScriptVars.dir);
		//ScriptVars.entNumber = cent->currentState.number;
		ScriptVars.rotate = s1->time;
		VectorCopy(cent->lerpOrigin, ScriptVars.origin);
		VectorCopy(cent->lerpAngles, ScriptVars.angles);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(cent->lerpAngles, ScriptVars.parentAngles);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentVelocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentDir);
		VectorNormalize(ScriptVars.parentDir);

		if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
			ent.axis[0][2] = 1;
		}
		VectorCopy(ent.axis[0], ScriptVars.axis[0]);
		VectorCopy(ent.axis[1], ScriptVars.axis[1]);
		VectorCopy(ent.axis[2], ScriptVars.axis[2]);
		ScriptVars.size = 1;

		VectorCopy(cent->lastModelIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastModelIntervalTime;
		VectorCopy(cent->lastModelDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastModelDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[s1->weapon].projectileScript, NULL);
		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastModelIntervalPosition);
		cent->lastModelIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastModelDistancePosition);
		cent->lastModelDistanceTime = ScriptVars.lastDistanceTime;

	}

	// add trails
	//if (weapon->hasTrailScript  &&  s1->pos.trType != TR_STATIONARY) {
	if (EffectScripts.weapons[s1->weapon].hasTrailScript  &&  s1->pos.trType != TR_STATIONARY  &&  s1->weapon != WP_GRAPPLING_HOOK) {
		//Com_Printf("%f  ent trail\n", cg.ftime);
		//CG_FX_MissileTrail(cent);
		//return;
		//memset(&ScriptVars, 0, sizeof(ScriptVars));
		CG_ResetScriptVars();
		//memcpy(&ScriptVars.currentState, s1, sizeof(ScriptVars.currentState));
		if (s1->weapon == WP_GRENADE_LAUNCHER) {
			int clientNum;

			clientNum = s1->clientNum;
			if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
				clientNum = 0;
			}
			CG_CopyPlayerDataToScriptData(&cg_entities[clientNum]);
		}
		VectorCopy(cent->lerpOrigin, ScriptVars.origin);
		VectorCopy(cent->lerpAngles, ScriptVars.angles);
		VectorCopy(s1->pos.trDelta, ScriptVars.velocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.dir);
		VectorNormalize(ScriptVars.dir);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(cent->lerpAngles, ScriptVars.parentAngles);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentVelocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentDir);
		VectorNormalize(ScriptVars.parentDir);

		//ScriptVars.entNumber = cent->currentState.number;
		if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
			ent.axis[0][2] = 1;
		}
		VectorCopy(ent.axis[0], ScriptVars.axis[0]);
		VectorCopy(ent.axis[1], ScriptVars.axis[1]);
		VectorCopy(ent.axis[2], ScriptVars.axis[2]);
		VectorCopy(cent->lastTrailIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastTrailIntervalTime;
		VectorCopy(cent->lastTrailDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastTrailDistanceTime;

		ScriptVars.color[0] = ScriptVars.color[1] = ScriptVars.color[2] = ScriptVars.color[3] = 1;

#if 0
		if (s1->weapon == WP_GRAPPLING_HOOK) {  // q3mme treats it like a rail
			vec3_t start, end;

			Com_Printf("clientNum: %d\n", s1->clientNum);
			VectorCopy(s1->pos.trBase, start);
			VectorCopy(cent->lerpOrigin, end);

			VectorCopy(start, ScriptVars.origin);
			VectorCopy(start, ScriptVars.parentOrigin);
			VectorCopy(end, ScriptVars.end);
			VectorCopy(end, ScriptVars.parentEnd);
			VectorSubtract(end, start, ScriptVars.dir);
			VectorCopy(ScriptVars.dir, ScriptVars.parentDir);
		}
#endif

		contents = CG_PointContents(cent->lerpOrigin, -1);

		//lastContents = CG_PointContents(ScriptVars.lastOrigin, -1);
		if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
			//if (contents & lastContents & CONTENTS_WATER) {
			if (contents & CONTENTS_WATER) {
				if (*EffectScripts.bubbles) {
					if (!(cent->lastPointContents & CONTENTS_WATER)) {
						VectorCopy(cent->lerpOrigin, ScriptVars.lastIntervalPosition);
						ScriptVars.lastIntervalTime = cgtime;
						VectorCopy(cent->lerpOrigin, ScriptVars.lastDistancePosition);
						ScriptVars.lastDistanceTime = cgtime;
					}
					CG_RunQ3mmeScript(EffectScripts.bubbles, NULL);
					VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTrailIntervalPosition);
					cent->lastTrailIntervalTime = ScriptVars.lastIntervalTime;
					VectorCopy(ScriptVars.lastDistancePosition, cent->lastTrailDistancePosition);
					cent->lastTrailDistanceTime = ScriptVars.lastDistanceTime;
					cent->trailTime = cgtime;
				} else {
					//BG_EvaluateTrajectory(&s1->pos, cent->trailTime, ScriptVars.lastOrigin);
					BG_EvaluateTrajectoryf(&s1->pos, cent->trailTime, lastOrigin, cg.foverf);
					//CG_BubbleTrail(ScriptVars.lastOrigin, cent->lerpOrigin, 8);
					CG_BubbleTrail(lastOrigin, cent->lerpOrigin, 8);
					cent->trailTime = cgtime;
				}
			}
		} else {
			CG_RunQ3mmeScript((char *)EffectScripts.weapons[s1->weapon].trailScript, NULL);
			VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTrailIntervalPosition);
			cent->lastTrailIntervalTime = ScriptVars.lastIntervalTime;
			VectorCopy(ScriptVars.lastDistancePosition, cent->lastTrailDistancePosition);
			cent->lastTrailDistanceTime = ScriptVars.lastDistanceTime;
			cent->trailTime = cgtime;
			//Com_Printf("cg_ents trail time %d\n", cg.time - cent->lastTrailTime);
		}

		cent->lastPointContents = contents;

		//cent->trailTime = cg.time;
	} else if ( weapon->missileTrailFunc ) {
		weapon->missileTrailFunc( cent, weapon );
		cent->lastPointContents = 0;  //FIXME hack for fx
	}
/*
	if ( cent->currentState.modelindex == TEAM_RED ) {
		col = 1;
	}
	else if ( cent->currentState.modelindex == TEAM_BLUE ) {
		col = 2;
	}
	else {
		col = 0;
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
			weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	}
*/
	//if (!weapon->hasTrailScript) {
	if (!EffectScripts.weapons[s1->weapon].hasTrailScript) {
		// add dynamic light
		if ( weapon->missileDlight ) {
			trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
		}
	}

	if (!EffectScripts.weapons[s1->weapon].hasProjectileScript) {
	// add missile sound
	if ( weapon->missileSound ) {
		vec3_t	velocity;

		//FIXME maybe for entityfreeze use cent->cgtime
		BG_EvaluateTrajectoryDeltaf (&cent->currentState.pos, cgtime, velocity, cg.foverf);
		//Com_Printf("missile velocity %f\n", VectorLength(velocity));

		CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound );
	}

#if 0  // test flare
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.reType = RT_SPRITE;
	ent.radius = 64;
	ent.rotation = 0;
	ent.customShader = trap_R_RegisterShader("flareShader");  //cgs.media.plasmaBallShader;
	ent.shaderRGBA[0] = 0xff;
	ent.shaderRGBA[1] = 0xff;
	ent.shaderRGBA[2] = 0xff;
	ent.shaderRGBA[3] = 0x8f;
	CG_AddRefEntity(&ent);
#endif

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	if ( cent->currentState.weapon == WP_PLASMAGUN ) {
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		CG_AddRefEntity(&ent);
		return;
	}

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	if (cent->currentState.weapon == WP_GRENADE_LAUNCHER) {
#if 0
		if (wolfcam_grenadeColor.v.modificationCount != wolfcam_grenadeColor.modificationCount) {
			int red, green, blue;

			SC_ParseColorFromStr(wolfcam_grenadeColor.v.string, &red, &green, &blue);
			cg.grenadeColor[0] = red;
			cg.grenadeColor[1] = green;
			cg.grenadeColor[2] = blue;
			wolfcam_grenadeColor.modificationCount = wolfcam_grenadeColor.v.modificationCount;
		}
		if (wolfcam_grenadeColor.v.string[0] != '\0') {
			ent.shaderRGBA[0] = cg.grenadeColor[0];
			ent.shaderRGBA[1] = cg.grenadeColor[1];
			ent.shaderRGBA[2] = cg.grenadeColor[2];
			ent.shaderRGBA[3] = 255;
		}
#endif

		//FIXME freecam
		if (wolfcam_following) {
			currentClientNum = wcg.clientNum;
		} else {
			currentClientNum = cg.snap->ps.clientNum;
		}
		if (s1->clientNum == currentClientNum) {
			SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_grenadeColor);
			ent.shaderRGBA[3] = cg_grenadeColorAlpha.integer;
			//Com_Printf("0x%x 0x%x %d\n", cg_grenadeColor.integer, (int)cg_grenadeColor.value, SC_RedFromCvar(cg_grenadeColor));
			//Com_Printf("%d %d %d %d\n", ent.shaderRGBA[0], ent.shaderRGBA[1], ent.shaderRGBA[2], ent.shaderRGBA[3]);
			//Com_Printf("own grenade color set to %d %d %d\n", ent.shaderRGBA[0], ent.shaderRGBA[1], ent.shaderRGBA[2]);
		} else if (CG_IsTeamGame(cgs.gametype)  &&  cgs.clientinfo[currentClientNum].team == cgs.clientinfo[s1->clientNum].team) {
			SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_grenadeTeamColor);
			ent.shaderRGBA[3] = cg_grenadeTeamColorAlpha.integer;
			//Com_Printf("team grenade color set to %d %d %d\n", ent.shaderRGBA[0], ent.shaderRGBA[1], ent.shaderRGBA[2]);
		} else {
			SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &cg_grenadeEnemyColor);
			ent.shaderRGBA[3] = cg_grenadeEnemyColorAlpha.integer;
			//Com_Printf("enemy grenade color set to %d %d %d\n", ent.shaderRGBA[0], ent.shaderRGBA[1], ent.shaderRGBA[2]);
		}
	}

#if 1  //def MPACK
	if ( cent->currentState.weapon == WP_PROX_LAUNCHER ) {
		if (s1->generic1 == TEAM_BLUE) {
			ent.hModel = cgs.media.blueProxMine;
		}
	}
#endif

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY ) {
		RotateAroundDirection( ent.axis, cgtime / 4 );
		//Com_Printf("rotating around axis\n");
	} else {
#if 1  //def MPACK
		if ( s1->weapon == WP_PROX_LAUNCHER ) {
			AnglesToAxis( cent->lerpAngles, ent.axis );
		}
		else
#endif
		{
			RotateAroundDirection( ent.axis, s1->time );
			//Com_Printf("aha\n");
		}
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE );

#if 0
	if (cg.timef > cent->extraShaderEntTime  &&  cent->extraShader) {
		ent->customShader = cent->extraShader;
		CG_AddRefEntity(ent);
	}
#endif
	}
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
static void CG_Grapple( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];
	if (!weapon->registered) {
		//Com_Printf("wtf....\n");
		CG_RegisterWeapon(s1->weapon);
	}

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

#if 0 // FIXME add grapple pull sound here..?
	// add missile sound
	if ( weapon->missileSound ) {
		CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound );
	}
#endif

	// Will draw cable if needed
	CG_GrappleTrail ( cent, weapon );

	if (EffectScripts.weapons[s1->weapon].hasProjectileScript) {
		//FIXME check if CG_ResetEntity() is called for this or if
		// ET_MISSILE just converts to ET_GRAPPLE (for distance, interval
		// scripts)

		//FIXME duplicate code:  see CG_Missile()

		CG_ResetScriptVars();
		VectorCopy(s1->pos.trDelta, ScriptVars.velocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.dir);
		VectorNormalize(ScriptVars.dir);
		//ScriptVars.entNumber = cent->currentState.number;
		ScriptVars.rotate = s1->time;
		VectorCopy(cent->lerpOrigin, ScriptVars.origin);
		VectorCopy(cent->lerpAngles, ScriptVars.angles);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(cent->lerpAngles, ScriptVars.parentAngles);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentVelocity);
		VectorCopy(s1->pos.trDelta, ScriptVars.parentDir);
		VectorNormalize(ScriptVars.parentDir);

		if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
			ent.axis[0][2] = 1;
		}
		VectorCopy(ent.axis[0], ScriptVars.axis[0]);
		VectorCopy(ent.axis[1], ScriptVars.axis[1]);
		VectorCopy(ent.axis[2], ScriptVars.axis[2]);
		ScriptVars.size = 1;

		VectorCopy(cent->lastModelIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastModelIntervalTime;
		VectorCopy(cent->lastModelDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastModelDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[s1->weapon].projectileScript, NULL);
		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastModelIntervalPosition);
		cent->lastModelIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastModelDistancePosition);
		cent->lastModelDistanceTime = ScriptVars.lastDistanceTime;
		return;
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);


	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

#if 1
	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}
#endif

	AnglesToAxis(cent->lerpAngles, ent.axis);
	//RotateAroundDirection( ent.axis, s1->time );

	//CG_Printf("adding grapple weapon %d  model %d\n", s1->weapon, ent.hModel);
	//CG_Printf("grapple %p\n", cent);
	//ent.renderfx |= RF_DEPTHHACK;

	//CG_ScaleModel(&ent, 5.0);

	CG_AddRefEntity(&ent);

#if 0  // test flare
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.reType = RT_SPRITE;
	ent.radius = 64;
	ent.rotation = 0;
	ent.customShader = trap_R_RegisterShader("flareShader");  //cgs.media.plasmaBallShader;
	ent.shaderRGBA[0] = 0xff;
	ent.shaderRGBA[1] = 0xff;
	ent.shaderRGBA[2] = 0xff;
	ent.shaderRGBA[3] = 0x8f;
	CG_AddRefEntity(&ent);
#endif

}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t		*s1;
	vec3_t mins;
	vec3_t maxs;
	//byte color[4];
	//vec3_t origin;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));

	VectorCopy( cent->lerpOrigin, ent.origin);
	//Com_Printf("%d:  %f %f %f\n", s1->number, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2]);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	CG_AddRefEntity(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		CG_AddRefEntity(&ent);
	}

	trap_R_ModelBounds(ent.hModel, mins, maxs);

#if 0
	Com_Printf("%d mover %f %f %f  %f %f %f  bmodel:%d %d  modelindex2: %d\n", s1->number, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2], cent->lerpAngles[0], cent->lerpAngles[1], cent->lerpAngles[2], s1->solid == SOLID_BMODEL, s1->modelindex, s1->modelindex2);

	Com_Printf("mins %f %f %f   maxs %f %f %f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);

	//VectorCopy(maxs, origin);
	//origin[2] += 42;

	//MAKERGBA(color, 55, 55, 255, 255);

	//CG_FloatNumber(s1->number, origin, RF_DEPTHHACK, color, 1.0);
#endif

	// hack for cg_drawEntNumbers
	VectorCopy(maxs, cent->lerpOrigin);
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( const centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	CG_AddRefEntity(&ent);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal (const centity_t *cent)
{
	refEntity_t			ent;
	const entityState_t		*s1;
	vec3_t transformed;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);

	ByteToDir(s1->eventParm, ent.axis[0]);

#if 0
	PerpendicularVector(ent.axis[1], ent.axis[0]);
	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	VectorCopy(ent.axis[2], transformed);
	//RotatePointAroundVector(ent.axis[2], ent.axis[0], ent.axis[2], s1->angles2[ROLL]);
	RotatePointAroundVector(ent.axis[2], ent.axis[0], transformed, s1->angles2[ROLL]);
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

	//RotatePointAroundVector(ent.axis[2], ent.axis[0], ent.axis[2], s1->angles2[ROLL]);
#endif

	// maps: purgatory

#if 0
	ent.axis[2][0] = 0;
	ent.axis[2][1] = 0;
	ent.axis[2][2] = 1;
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);
	RotatePointAroundVector(ent.axis[2], ent.axis[0], ent.axis[2], s1->angles2[ROLL]);
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);
#endif

#if 0
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	//VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	//CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );

	ent.axis[1][0] = 0;
	ent.axis[1][1] = 0;
	ent.axis[1][2] = -1;

	//RotatePointAroundVector(ent.axis[1], ent.axis[0], ent.axis[1], s1->angles2[ROLL]);
	//CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);
	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
#endif

#if 1
	PerpendicularVector(ent.axis[1], ent.axis[0]);
	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	//CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	VectorCopy(ent.axis[1], transformed);
	//RotatePointAroundVector(ent.axis[1], ent.axis[0], ent.axis[1], s1->angles2[ROLL]);
	//RotatePointAroundVector(ent.axis[1], ent.axis[0], transformed, s1->angles2[ROLL]);
	RotatePointAroundVector(ent.axis[1], ent.axis[0], transformed, -s1->angles2[ROLL]);
	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
#endif

	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	//Com_Printf("portal %d eventparm %d\n", s1->number, s1->eventParm);

	// add to refresh list
	CG_AddRefEntity(&ent);
}

/*
================
CG_CreateRotationMatrix
================
*/
void CG_CreateRotationMatrix(vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
CG_TransposeMatrix
================
*/
void CG_TransposeMatrix(vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
CG_RotatePoint
================
*/
void CG_RotatePoint(vec3_t point, vec3_t matrix[3]) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover (const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, float subTime, const vec3_t angles_in, vec3_t angles_out)
{
	const centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;
	vec3_t  matrix[3], transpose[3];
	vec3_t  org, org2, move2;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		VectorCopy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		VectorCopy(angles_in, angles_out);
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectoryf (&cent->currentState.pos, toTime, origin, subTime);
	BG_EvaluateTrajectoryf (&cent->currentState.apos, toTime, angles, subTime);

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	// origin change when on a rotating object
	CG_CreateRotationMatrix( deltaAngles, transpose );
	CG_TransposeMatrix( transpose, matrix );
	VectorSubtract( in, oldOrigin, org );
	VectorCopy( org, org2 );
	CG_RotatePoint( org2, matrix );
	VectorSubtract( org2, org, move2 );
	VectorAdd( deltaOrigin, move2, deltaOrigin );

	VectorAdd( in, deltaOrigin, out );
	VectorAdd( angles_in, deltaAngles, angles_out );
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;
	//float dist;

	//Com_Printf("xx %d\n", cent->currentState.clientNum);

	if (cg.demoPlayback  &&  cg_useOriginalInterpolation.integer == 0) {
		if (Wolfcam_InterpolateEntityPosition(cent)) {
			//Wolfcam_AddBoundingBox(cent);
			return;
		} else {
			if (cent->currentState.number != cg.snap->ps.clientNum) {
				//Com_Printf("ent %d not rolled back\n", cent->currentState.number);
			}
#if 0
			if (cent->currentState.number == 6) {
				Com_Printf("6 using original interp\n");
			}
#endif
			cent->serverTimeOffset = 0;
		}
	}

	if (!cent->interpolate) {
		//Com_Printf("don't interpolate %d\n", cent->currentState.number);
		BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
		BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, cent->lerpAngles);
		return;
	}

	//Com_Printf("orig cg_iterp %d   %d\n", cent->currentState.number, cg.snap->ps.clientNum);
	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		if (cg.demoPlayback) {
			// can happen with demo seeking
			//VectorCopy(cent->currentState.pos.trBase, cent->lerpOrigin);
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
			BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, cent->lerpAngles);
			//VectorCopy(cent->currentState.apos.trBase, cent->lerpAngles);
			//Com_Printf("FIXME CG_InterpolateEntityPosition: cg.nextSnap == NULL\n");
			return;
		} else {
			CG_Error( "CG_InterpolateEntityPosition: cg.nextSnap == NULL" );
		}
	}

	f = cg.frameInterpolation;
	//Com_Printf("%f\n", f);
	//Com_Printf("cg.time %d  serverTimes %d  %d\n", cg.time, cg.snap->serverTime, cg.nextSnap->serverTime);

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

	//Wolfcam_AddBoundingBox(cent);
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {
	vec3_t tmpOrigin;
	vec3_t tmpAngles;
	double f;

	if (cg.demoPlayback  &&  cg_useOriginalInterpolation.integer == 0) {
		//FIXME
		if (cent->currentState.eType == ET_PLAYER  &&  cent->currentState.number != cg.snap->ps.clientNum) {
			CG_InterpolateEntityPosition( cent );
			return;
		}
		if (cent->currentState.number != cg.snap->ps.clientNum) {
			CG_InterpolateEntityPosition( cent );
			return;
		}
	}

	if (cent->currentState.number != cg.snap->ps.clientNum) {
		//Com_Printf("%d calclerp\n", cent->currentState.number);
	}

	// if this player does not want to see extrapolated players
	if ( !cg_smoothClients.integer ) {
		// make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS ) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	//FIXME wolfcam wrong values if using previous snapshots, same for rest of function
	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	//Com_Printf("wetf\n");

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	//Com_Printf("not interpolating %d  %s\n", cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name);
	if (!cg_useOriginalInterpolation.integer) {
		if (!wolfcam_following  &&  cent->currentState.number == cg.snap->ps.clientNum) {
			//
		} else if (wolfcam_following  &&  cent->currentState.number == cg.snap->ps.clientNum  &&  wcg.clientNum == cg.snap->ps.clientNum) {
			//
		} else {
			if (cent->currentState.number < MAX_CLIENTS) {
				CG_InterpolateEntityPosition(cent);
				return;
			}
		}
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time + 1, tmpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time + 1, tmpAngles );

	//f = (float)cg.ioverf / SUBTIME_RESOLUTION;
	f = cg.foverf;

	cent->lerpOrigin[0] = cent->lerpOrigin[0] + f * (tmpOrigin[0] - cent->lerpOrigin[0]);
	cent->lerpOrigin[1] = cent->lerpOrigin[1] + f * (tmpOrigin[1] - cent->lerpOrigin[1]);
	cent->lerpOrigin[2] = cent->lerpOrigin[2] + f * (tmpOrigin[2] - cent->lerpOrigin[2]);

	cent->lerpAngles[0] = LerpAngle( cent->lerpAngles[0], tmpAngles[0], f );
	cent->lerpAngles[1] = LerpAngle( cent->lerpAngles[1], tmpAngles[1], f );
	cent->lerpAngles[2] = LerpAngle( cent->lerpAngles[2], tmpAngles[2], f );

	if (cent->currentState.eType == ET_PLAYER) {
		//Com_Printf("fuck %d  %d\n", cent->currentState.number, cg.snap->ps.clientNum);
	}

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
								   cg.snap->serverTime, cg.time, cent->lerpOrigin, cg.foverf, cent->lerpAngles, cent->lerpAngles );
	}
}

static void CG_DrawHarversterHelpIcons (const centity_t *cent)
{
	int ourTeam;
	int ourClientNum;
	const centity_t *pcent;
	refEntity_t ent;
	vec4_t color;
	float alpha;
	//vec3_t dir;

	if (cgs.gametype != GT_HARVESTER) {
		return;
	}

	if (cg_helpIconStyle.integer == 0  ||  cg_helpIcon.integer == 0) {
		return;
	}

	if (cg.freecam  &&  !cg_freecam_useTeamSettings.integer) {
        return;
    }

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  &&  !wolfcam_following) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	pcent = &cg_entities[ourClientNum];
	if ((pcent->currentState.generic1 & 0x3f) == 0) {
		return;
	}

	ourTeam = cgs.clientinfo[ourClientNum].team;
	if (cent->currentState.modelindex == ourTeam) {
		return;
	}

	if (cent->currentState.modelindex == TEAM_RED) {
		VectorSet(color, 1, 0, 0);
		//color[3] = cg_helpIconAlpha.value / 255.0;
		alpha = 255.0;
	} else {
		VectorSet(color, 0, 0.5, 1);
		//color[3] = cg_helpIconAlpha.value / 255.0;
		alpha = 255.0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 64 + 16;  //32;
	ent.reType = RT_SPRITE;
	ent.customShader = cgs.media.harvesterCapture;
	ent.radius = 16;
	if (cg_helpIconStyle.integer == 1) {
		ent.radius = 16;
	} else if (cg_helpIconStyle.integer == 2) {
		ent.radius = 16;
		ent.renderfx = RF_DEPTHHACK;
	} else {
		vec3_t org;
		float dist;
		float minWidth;
		float maxWidth;
		float radius;

		ent.renderfx = RF_DEPTHHACK;
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
		} else if (cg.freecam) {
			VectorCopy(cg.fpos, org);
		} else {
			VectorCopy(cg.refdef.vieworg, org);
		}

		minWidth = cg_helpIconMinWidth.value;
		maxWidth = cg_helpIconMaxWidth.value;

		radius = maxWidth / 2.0;
		dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

		if (minWidth > 0.1) {
			dist *= (16.0 / minWidth);
		}

		ent.radius = radius;

		if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
			ent.radius = radius * (Distance(ent.origin, org) / dist);
		}

#if 0
		ent.radius = 16;

		dist = 400;
		if (Distance(ent.origin, org) > dist) {
			VectorSubtract(ent.origin, org, dir);
			VectorNormalize(dir);
			VectorMA(org, dist, dir, ent.origin);
		}
#endif
	}

	ent.origin[2] += ent.radius;

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;
	CG_AddRefEntity(&ent);
}

static void CG_Draw1FctfHelpIcons (const centity_t *cent)
{
	int ourTeam;
	int ourClientNum;
	//centity_t *pcent;
	refEntity_t ent;
	vec4_t color;
	float alpha;
	//vec3_t dir;

	if (cgs.gametype != GT_1FCTF) {
		return;
	}

	if (cg_helpIconStyle.integer == 0  ||  cg_helpIcon.integer == 0) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  &&  !wolfcam_following) {
		return;
	}

	if (cgs.realProtocol < 91) {
		if (cent->currentState.modelindex == 0  &&  (cgs.flagStatus == FLAG_TAKEN_RED  ||  cgs.flagStatus == FLAG_TAKEN_BLUE)) {
			return;
		}

		if (cent->currentState.modelindex != 0  &&  (cgs.flagStatus != FLAG_TAKEN_RED  &&  cgs.flagStatus != FLAG_TAKEN_BLUE)) {
			return;
		}
	} else {  // protocol >= 91
		if (cent->currentState.modelindex == 0  &&  (cgs.flagStatus == FLAG_QL_TAKEN_RED  ||  cgs.flagStatus == FLAG_QL_TAKEN_BLUE)) {
			return;
		}

		if (cent->currentState.modelindex != 0  &&  (cgs.flagStatus != FLAG_QL_TAKEN_RED  &&  cgs.flagStatus != FLAG_QL_TAKEN_BLUE)) {
			return;
		}
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	//pcent = &cg_entities[ourClientNum];

	ourTeam = cgs.clientinfo[ourClientNum].team;

	// colors switched to match the player's team
	alpha = 255.0;
	if (cent->currentState.modelindex == TEAM_RED) {
		VectorSet(color, 0, 0.5, 1);
	} else if (cent->currentState.modelindex == TEAM_BLUE) {
		VectorSet(color, 1, 0, 0);
	} else {
		// flag not taken
		if (cgs.flagStatus != FLAG_ATBASE) {
			return;
		}
		VectorSet(color, 0.65, 0.65, 0.65);
	}

	if (cent->currentState.modelindex == TEAM_RED  ||  cent->currentState.modelindex == TEAM_BLUE) {
		qboolean hasFlag;

		if (ourClientNum == cg.snap->ps.clientNum) {
			hasFlag = cg.snap->ps.powerups[PW_NEUTRALFLAG];
		} else {
			hasFlag = cg_entities[ourClientNum].currentState.powerups & (1 << PW_NEUTRALFLAG);
		}

		// don't show if person isn't the flag carrier
		if (!hasFlag) {
			return;
		}

		// we have the flag
		if (cg.freecam  &&  cg_freecam_useTeamSettings.integer != 2) {
			return;
		}
	}

	//Com_Printf("%d  ..  %d\n", ourTeam, cent->currentState.modelindex);

	if (ourTeam == cent->currentState.modelindex) {
		return;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 64 + 16;  //32;
	ent.reType = RT_SPRITE;
	//ent.customShader = cgs.media.harvesterCapture;
	ent.customShader = cgs.media.adCapture;
	ent.radius = 16;
	if (cg_helpIconStyle.integer == 1) {
		ent.radius = 16;
	} else if (cg_helpIconStyle.integer == 2) {
		ent.radius = 16;
		ent.renderfx = RF_DEPTHHACK;
	} else {
		vec3_t org;
		float dist;
		float minWidth;
		float maxWidth;
		float radius;

		ent.renderfx = RF_DEPTHHACK;
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
		} else if (cg.freecam) {
			VectorCopy(cg.fpos, org);
		} else {
			VectorCopy(cg.refdef.vieworg, org);
		}

		minWidth = cg_helpIconMinWidth.value;
		maxWidth = cg_helpIconMaxWidth.value;

		radius = maxWidth / 2.0;
		dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

		if (minWidth > 0.1) {
			dist *= (16.0 / minWidth);
		}

		ent.radius = radius;

		if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
			ent.radius = radius * (Distance(ent.origin, org) / dist);
		}
	}

	ent.origin[2] += ent.radius;

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * alpha;
	CG_AddRefEntity(&ent);
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase( centity_t *cent ) {
	refEntity_t model;
#if 1  //def MPACK
	vec3_t angles;
	int t, h;
	float c;

	//if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_CTFS) {
	if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF) {
#else
	if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
#endif
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );
		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		CG_AddRefEntity(&model);
		// base help icons can't be rendered here since base isn't
		// broadcast to all clients, need to add with flag item
		//CG_DrawCtfHelpIcons(cent);
	}
#if 1  //def MPACK
	else if ( cgs.gametype == GT_OBELISK ) {
		// show the obelisk
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		model.hModel = cgs.media.overloadBaseModel;
		CG_AddRefEntity(&model);
		// if hit
		if ( cent->currentState.frame == 1) {
			// show hit model
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			//
			model.hModel = cgs.media.overloadEnergyModel;
			CG_AddRefEntity(&model);
		}
		// if respawning
		if ( cent->currentState.frame == 2) {
			if ( !cent->miscTime ) {
				cent->miscTime = cg.time;
			}
			t = cg.time - cent->miscTime;
			h = (cg_obeliskRespawnDelay.integer - 5) * 1000;
			//
			if (t > h) {
				c = (float) (t - h) / h;
				if (c > 1)
					c = 1;
			}
			else {
				c = 0;
			}
			// show the lights
			AnglesToAxis( cent->currentState.angles, model.axis );
			//
			model.shaderRGBA[0] = c * 0xff;
			model.shaderRGBA[1] = c * 0xff;
			model.shaderRGBA[2] = c * 0xff;
			model.shaderRGBA[3] = c * 0xff;

			model.hModel = cgs.media.overloadLightsModel;
			CG_AddRefEntity(&model);
			// show the target
			if (t > h) {
				if ( !cent->pe.muzzleFlashTime ) {
					CG_StartSound (cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY,  cgs.media.obeliskRespawnSound);
					cent->pe.muzzleFlashTime = 1;
				}
				VectorCopy(cent->currentState.angles, angles);
				angles[YAW] += (float) 16 * acos(1-c) * 180 / M_PI;
				AnglesToAxis( angles, model.axis );

				VectorScale( model.axis[0], c, model.axis[0]);
				VectorScale( model.axis[1], c, model.axis[1]);
				VectorScale( model.axis[2], c, model.axis[2]);

				model.shaderRGBA[0] = 0xff;
				model.shaderRGBA[1] = 0xff;
				model.shaderRGBA[2] = 0xff;
				model.shaderRGBA[3] = 0xff;
				//
				model.origin[2] += 56;
				model.hModel = cgs.media.overloadTargetModel;
				CG_AddRefEntity(&model);
			}
			else {
				//FIXME: show animated smoke
			}
		}
		else {
			cent->miscTime = 0;
			cent->pe.muzzleFlashTime = 0;
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			// show the lights
			model.hModel = cgs.media.overloadLightsModel;
			CG_AddRefEntity(&model);
			// show the target
			model.origin[2] += 56;
			model.hModel = cgs.media.overloadTargetModel;
			CG_AddRefEntity(&model);
		}
	}
	else if (cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_1FCTF) {
		int team;

		team = cent->currentState.modelindex;

		// show harvester model
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		if (team == TEAM_RED) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterRedSkin;
		}
		else if (team == TEAM_BLUE) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterBlueSkin;
		}
		else {
			if (cgs.gametype == GT_1FCTF) {
				model.hModel = cgs.media.neutralFlagBaseModel;
			} else {
				model.hModel = cgs.media.harvesterNeutralModel;
			}
			model.customSkin = 0;
		}

		CG_AddRefEntity(&model);

		CG_DrawHarversterHelpIcons(cent);
		CG_Draw1FctfHelpIcons(cent);
	}
#endif
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	qboolean frozen;
	int entNum;

	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	if (cg.demoSeeking) {
		//return;
	}

	frozen = qfalse;
	if (cent == &cg.predictedPlayerEntity) {
		if (cg.freezeEntity[cg.snap->ps.clientNum]) {
			frozen = qtrue;
			memcpy(&cg.predictedPlayerEntity, &cg.freezeCent[cg.snap->ps.clientNum], sizeof(cg.predictedPlayerEntity));
		}
	} else {
		entNum = cent - cg_entities;
		if (cg.freezeEntity[entNum]) {
			frozen = qtrue;
			memcpy(cent, &cg.freezeCent[entNum], sizeof(*cent));
		}
	}

	if (!frozen) {
		cent->cgtime = cg.time;

		// calculate the current origin
		CG_CalcEntityLerpPositions( cent );
	}


	if (cg.filterEntity[cent->currentState.number]) {
		return;
	}

	switch (cent->currentState.eType) {
	case ET_GENERAL:
		if (cg.filter.general) {
			return;
		}
		break;
	case ET_PLAYER:
		if (cg.filter.player) {
			return;
		}
		break;
	case ET_ITEM:
		if (cg.filter.item) {
			return;
		}
		break;
	case ET_MISSILE:
		if (cg.filter.missile) {
			return;
		}
		break;
	case ET_MOVER:
		if (cg.filter.mover) {
			return;
		}
		break;
	case ET_BEAM:
		if (cg.filter.beam) {
			return;
		}
		break;
	case ET_PORTAL:
		if (cg.filter.portal) {
			return;
		}
		break;
	case ET_SPEAKER:
		if (cg.filter.speaker) {
			return;
		}
		break;
	case ET_PUSH_TRIGGER:
		if (cg.filter.pushTrigger) {
			return;
		}
		break;
	case ET_TELEPORT_TRIGGER:
		if (cg.filter.teleportTrigger) {
			return;
		}
		break;
	case ET_INVISIBLE:
		if (cg.filter.invisible) {
			return;
		}
		break;
	case ET_GRAPPLE:
		if (cg.filter.grapple) {
			return;
		}
		break;
	case ET_TEAM:
		if (cg.filter.team) {
			return;
		}
		break;
	default:
		break;
	}

	// add automatic effects
	CG_EntityEffects( cent );

#define MAX_FRAMETIME 12
	if (cent->wasReset) {  //  ||  cg.frametime > MAX_FRAMETIME) {
		if (cg.frametime > MAX_FRAMETIME) {
			//Com_Printf("^3cg.frametime %d  resetting %d\n", cg.frametime, cent->currentState.number);
		}

		CG_ResetFXIntervalAndDistance(cent);
		cent->wasReset = qfalse;
	}

	//Com_Printf("ent %d  %d\n", cent->currentState.number, cent->currentState.eType);

	switch ( cent->currentState.eType ) {
	default: {
		//int *p = NULL;
		CG_Printf("Bad entity type: %i\n", cent->currentState.eType);
		CG_PrintEntityStatep(&cent->currentState);
		//*p = 666;
		CG_Error( "Bad entity type: %i", cent->currentState.eType );
		break;
	}
	case ET_INVISIBLE:  //FIXME specs have this set
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		//Wolfcam_Players_f();
		//CG_PrintEntityState(cent->currentState.number);
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_GRAPPLE:
		CG_Grapple( cent );
		break;
	case ET_TEAM:
		CG_TeamBase( cent );
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;
	int i;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			//cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
			//cg.frameInterpolation = (float)( (float)cg.time + (float)cg.ioverf / SUBTIME_RESOLUTION - (float)cg.snap->serverTime ) / (float)delta;
			cg.frameInterpolation = (cg.ftime - (double)cg.snap->serverTime) / (double)delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;  //FIXME take into account ioverf
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;  //FIXME take into account ioverf
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	//CG_Printf("^5copying to predicted\n");

	//CG_AddCEntity( &cg.predictedPlayerEntity );
	{  //FIXME not calling AddCEntity since spec demos will have ET_INVISIBLE and not call CG_Player()

    cg_entities[cg.snap->ps.clientNum].currentState.eType = ET_PLAYER;
	cg.predictedPlayerEntity.currentState.eType = ET_PLAYER;
	// calculate the current origin
	CG_CalcEntityLerpPositions(&cg.predictedPlayerEntity);
	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );
	cg_entities[cg.snap->ps.clientNum].currentState.clientNum = cg.snap->ps.clientNum;
	cg_entities[cg.snap->ps.clientNum].currentState.number = cg.snap->ps.clientNum;
	//Com_Printf("%f %f %f\n", cg.predictedPlayerEntity.lerpOrigin[0], cg.predictedPlayerEntity.lerpOrigin[1], cg.predictedPlayerEntity.lerpOrigin[2]);
	if (!(cg.predictedPlayerEntity.currentState.eFlags & EF_FIRING)  ||  cg.predictedPlayerEntity.currentState.weapon != WP_LIGHTNING) {
		cg.predictedPlayerEntity.pe.lightningFiring = qfalse;
		cg_entities[cg.snap->ps.clientNum].pe.lightningFiring = qfalse;
	}

	memset(wcg.inSnapshot, 0, sizeof(wcg.inSnapshot));
	wcg.inSnapshot[cg.snap->ps.clientNum] = qtrue;

	//FIXME swap effects() and player() for fx scripting?
	// add automatic effects
	if (!cg.filterEntity[cg.predictedPlayerEntity.currentState.clientNum]  &&  !cg.filter.player) {
		CG_EntityEffects(&cg.predictedPlayerEntity);
	}

	cg.predictedPlayerEntity.cgtime = cg.time;

	if (!cg.filterEntity[cg.predictedPlayerEntity.currentState.clientNum]  &&  !cg.filter.player) {
		CG_Player(&cg.predictedPlayerEntity);
	}

	//trap_R_AddLightToScene(cg.predictedPlayerEntity.lerpOrigin, 300, 1.0, 1.0, 1.0);
	}


	// add each entity sent over by the server

#if 1
	if (cg_useOriginalInterpolation.integer == 0) {
		memset(&wcg.centityAdded, 0, sizeof(wcg.centityAdded));
	}
#endif

	if (cg_customMirrorSurfaces.integer) {
		for (i = 0;  i < cg.numMirrorSurfaces;  i++) {
			refEntity_t ent;

			//Com_Printf("mirror: %d\n", i + 1);
			memset(&ent, 0, sizeof(ent));
			VectorCopy(cg.mirrorSurfaces[i], ent.origin);
			VectorCopy(cg.mirrorSurfaces[i], ent.oldorigin);
			ent.reType = RT_PORTALSURFACE;
			// EF_NODRAW ?
			CG_AddRefEntity(&ent);
		}
	}

	// add players first since fx scripting can reference currentState
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		wcg.inSnapshot[cg.snap->entities[num].number] = qtrue;
		if (cent->currentState.eType != ET_PLAYER) {
			continue;
		}

		// cg_useOriginalInterpolation 0  changes cg_entities eFlags
		if (!(cg.snap->entities[num].eFlags & EF_FIRING)  ||  cg.snap->entities[num].weapon != WP_LIGHTNING) {
			cent->pe.lightningFiring = qfalse;
		}

        if (!wolfcam_following) {
            CG_AddCEntity( cent );
			//wcg.centityAdded[cg.snap->entities[num].number] = qtrue;
			//trap_R_AddLightToScene(cent->lerpOrigin, 300, 1.0, 1.0, 1.0);
        } else {
            if (wcg.clientNum == cg.snap->entities[num].number) {
                cent->currentState.eFlags |= EF_NODRAW;
                cent->nextState.eFlags |= EF_NODRAW;
            }

            CG_AddCEntity( cent );
			//wcg.centityAdded[cg.snap->entities[num].number] = qtrue;
        }
	}

	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if (cent->currentState.eType == ET_PLAYER) {
			continue;
		}
        if (!wolfcam_following) {
            CG_AddCEntity( cent );
			//wcg.centityAdded[cg.snap->entities[num].number] = qtrue;
			//trap_R_AddLightToScene(cent->lerpOrigin, 300, 1.0, 1.0, 1.0);
        } else {
            if (wcg.clientNum == cg.snap->entities[num].number) {
                cent->currentState.eFlags |= EF_NODRAW;
                cent->nextState.eFlags |= EF_NODRAW;
            }

            CG_AddCEntity( cent );
			//wcg.centityAdded[cg.snap->entities[num].number] = qtrue;
        }
	}

#if 1
	if (cg_useOriginalInterpolation.integer == 0) {  //  &&  (!cg.freecam  ||  ((cg.freecam  ||  cg.renderingThirdPerson)  &&  !cg_freecam_useServerView.integer))) {
		//if ((cg.freecam  ||  cg.renderingThirdPerson)  &&  cg_freecam_useServerView.integer) {
		if ((cg.freecam  ||  cg_thirdPerson.integer)  &&  cg_freecam_useServerView.integer) {
			//
		} else {
		if (wcg.snapOld  &&  wcg.snapPrev  &&  wcg.snapNext) {
			centity_t centTmp;
			const entityState_t *es;
			for (num = 0;  num < wcg.snapPrev->numEntities;  num++) {
				es = &wcg.snapPrev->entities[num];
				cent = &cg_entities[es->number];
				//Com_Printf("ss %d %d\n", es->number, es->clientNum);
				if (wcg.centityAdded[es->number]) {
					continue;
				}
				if (es->eType == ET_PLAYER) {
					//CG_Printf("wc interp corpse hack %d %d\n", es->number, es->clientNum);
					//FIXME hack to prevent corpses from disappearing briefly
					memcpy(&centTmp, cent, sizeof(centTmp));
					cent->currentValid = qtrue;
					memcpy(&cent->currentState, es, sizeof(cent->currentState));
					Wolfcam_InterpolateEntityPosition(cent);
					cent->currentState.eFlags &= ~EF_NODRAW;
					CG_Player(cent);
					memcpy(cent, &centTmp, sizeof(*cent));
				} else if (es->eType == ET_MISSILE) {
#if 0
					memcpy(&centTmp, cent, sizeof(centTmp));
					cent->currentValid = qtrue;
					memcpy(&cent->currentState, es, sizeof(cent->currentState));
					Wolfcam_InterpolateEntityPosition(cent);
					cent->currentState.eFlags &= ~EF_NODRAW;
					CG_Missile(cent);
					memcpy(cent, &centTmp, sizeof(*cent));
					//Com_Printf("missile %d\n", es->number);
#endif
				}
			}
		}
		}
	}
#endif
}

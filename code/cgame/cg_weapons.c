// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

#include "cg_draw.h"  // CG_Text_Width CG_Text_Paint_Bottom
#include "cg_drawtools.h"
#include "cg_effects.h"
#include "cg_ents.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_players.h"
#include "cg_predict.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_event.h"
#include "wolfcam_predict.h"
#include "wolfcam_weapons.h"

#include "wolfcam_local.h"

static qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle );


/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( const centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( const centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				i;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	for ( i = 0; i < 2; i++ ) {
		float	waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if ( i == 0 ) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis( cent->lerpAngles, v );

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
		VectorCopy( re->origin, le->pos.trBase );
		if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale( xvelocity, waterScale, le->pos.trDelta );

		AxisCopy( axisDefault, re->axis );
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}


#if 1  //def MPACK
/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void CG_NailgunEjectBrass( const centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	if (cg_smokeRadius_NG.integer > 0) {
		smoke = CG_SmokePuff( origin, up, cg_smokeRadius_NG.integer, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}
}
#endif

void CG_SimpleRailTrail (const vec3_t start, const vec3_t end, float railTime, const byte color[4])
{
	localEntity_t *le;
	refEntity_t *re;

	le = CG_AllocLocalEntityRealTime();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->leFlags = LEF_REAL_TIME;
	le->startTime = cg.realTime;  // cg.time;
	le->endTime = cg.realTime + railTime;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);

	re->shaderRGBA[0] = color[0];
	re->shaderRGBA[1] = color[1];
	re->shaderRGBA[2] = color[2];
	re->shaderRGBA[3] = color[3];

	le->color[0] = (float)color[0] / 255.0 * 0.75;
	le->color[1] = (float)color[1] / 255.0 * 0.75;
	le->color[2] = (float)color[2] / 255.0 * 0.75;
	le->color[3] = (float)color[3] / 255.0;  //1.0f;

}

static void CG_FX_RailTrail (const clientInfo_t *ci, const vec3_t start, const vec3_t end)
{
	//weaponInfo_t *weapon;
	centity_t *cent;
	int clientNum;

	//CG_Printf("CG_FX_RailTrail() clientNum %d  %f\n", ci - cgs.clientinfo, Distance(start, end));

	clientNum = ci - cgs.clientinfo;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		clientNum = 0;
	}

	cent = &cg_entities[clientNum];

	//Com_Printf("fx rail trail: %s  origin(%f %f %f)  (%f %f %f)\n", ci->name, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2], start[0], start[1], start[2]);

	//weapon = &cg_weapons[WP_RAILGUN];

	CG_ResetScriptVars();
	CG_CopyPlayerDataToScriptData(cent);
	VectorCopy(start, ScriptVars.origin);
	VectorCopy(start, ScriptVars.parentOrigin);
	VectorCopy(end, ScriptVars.end);
	VectorCopy(end, ScriptVars.parentEnd);
	VectorSubtract(end, start, ScriptVars.dir);
	VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

	VectorCopy(ci->color1, ScriptVars.color);
	ScriptVars.color[3] = 1;
	VectorCopy(ci->color1, ScriptVars.color1);
	VectorCopy(ci->color2, ScriptVars.color2);

	//Com_Printf("rail:\n'%s'\n", EffectScripts.weapons[WP_RAILGUN].trailScript);
	CG_RunQ3mmeScript(EffectScripts.weapons[WP_RAILGUN].trailScript, NULL);
	//cent->trailTime = cg.time;
}

/*
==========================
CG_RailTrail
==========================
*/

void CG_RailTrail (const clientInfo_t *ci, const vec3_t start, const vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp;
	float  len;
	int    i, j, skip;

	localEntity_t *le;
	refEntity_t   *re;
	qboolean enemyRail;
	qboolean teamRail;
	byte recolor[4];
	float lecolor[4];
	vec3_t ourStart;
	float w;
	int RADIUS, ROTATION, SPACING;

	if (EffectScripts.weapons[WP_RAILGUN].hasTrailScript) {
		CG_FX_RailTrail(ci, start, end);
		return;
	}

	//#define RADIUS   4// 4
	//#define ROTATION 1 //1
	//#define SPACING  5//5

	RADIUS = cg_railRadius.integer;
	if (RADIUS <= 0) {
		RADIUS = 1;
	}
	ROTATION = cg_railRotation.integer;
	if (ROTATION <= 0) {
		ROTATION = 1;
	}
	SPACING = cg_railSpacing.integer;
	if (SPACING <= 0) {
		SPACING = 1;
	}

	if (cg_railQL.integer) {
		VectorCopy(start, ourStart);

		if (!cg_railFromMuzzle.integer) {
			ourStart[2] -= 4;
		}
		VectorCopy (ourStart, move);
		VectorSubtract (end, ourStart, vec);
		len = VectorNormalize (vec);
		PerpendicularVector(temp, vec);
		for (i = 0 ; i < 36; i++) {
			RotatePointAroundVector(axis[i], vec, temp, i * 10);//banshee 2.4 was 10
		}

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_FADE_RGB;
		le->startTime = cg.time;
		le->endTime = cg.time + cg_railTrailTime.value;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);

		re->shaderTime = cg.time / 1000.0f;
		re->reType = RT_RAIL_RINGS;
		re->customShader = cgs.media.railRingsShader;

		VectorCopy(ourStart, re->origin);
		VectorCopy(end, re->oldorigin);
		w = cg_railQLRailRingWhiteValue.value;

		re->shaderRGBA[0] = ci->color2[0] * 255.0 * w;
		re->shaderRGBA[1] = ci->color2[1] * 255.0 * w;
		re->shaderRGBA[2] = ci->color2[2] * 255.0 * w;
		re->shaderRGBA[3] = 255.0 * w;

		le->color[0] = ci->color2[0] * w;
		le->color[1] = ci->color2[1] * w;
		le->color[2] = ci->color2[2] * w;
		le->color[3] = 1.0 * w;

		if (!cg_railFromMuzzle.integer) {
			if ((CG_IsUs(ci)  &&  cg_railNudge.integer)  ||
				(cgs.gametype >= GT_TEAM  &&  CG_IsTeammate(ci)  &&  cg_teamRailNudge.integer  &&  !CG_IsUs(ci))  ||
				(cgs.gametype >= GT_TEAM  &&  CG_IsEnemy(ci)  &&  cg_enemyRailNudge.integer  &&  !CG_IsUs(ci))  ||
				(cgs.gametype < GT_TEAM  &&  CG_IsEnemy(ci)  &&  cg_enemyRailNudge.integer  &&  !CG_IsUs(ci))) {

				re->origin[2] -= 8;
				re->oldorigin[2] -= 8;
			}
		}
	}

	///////

	VectorCopy(start, ourStart);
	if (!cg_railFromMuzzle.integer) {
		ourStart[2] -= 4;
	}
	VectorCopy (ourStart, move);
	VectorSubtract (end, ourStart, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);
	for (i = 0 ; i < 36; i++) {
		RotatePointAroundVector(axis[i], vec, temp, i * 10);  //banshee 2.4 was 10
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(ourStart, re->origin);
	VectorCopy(end, re->oldorigin);

	if (ci->override  &&  cg_clientOverrideIgnoreTeamSettings.integer) {
		re->shaderRGBA[0] = ci->color1[0] * 255;
		re->shaderRGBA[1] = ci->color1[1] * 255;
		re->shaderRGBA[2] = ci->color1[2] * 255;
		re->shaderRGBA[3] = 255;

		le->color[0] = ci->color1[0] * 0.75;
		le->color[1] = ci->color1[1] * 0.75;
		le->color[2] = ci->color1[2] * 0.75;
		le->color[3] = 1.0f;

		recolor[0] = ci->color2[0] * 255;
		recolor[1] = ci->color2[1] * 255;
		recolor[2] = ci->color2[2] * 255;
		recolor[3] = 255;

		lecolor[0] = ci->color2[0] * 0.75;
		lecolor[1] = ci->color2[1] * 0.75;
		lecolor[2] = ci->color2[2] * 0.75;
		lecolor[3] = 1.0f;
	} else if (cgs.gametype < GT_TEAM) {
		enemyRail = CG_IsEnemy(ci);

		if (enemyRail  &&  (*cg_enemyRailColor1.string  ||  *cg_enemyRailColor2.string)  && !CG_IsUs(ci)) {
			if (*cg_enemyRailColor1.string) {
				SC_ByteVec3ColorFromCvar(re->shaderRGBA, &cg_enemyRailColor1);
				re->shaderRGBA[3] = 255;
				SC_ByteVec3ColorFromCvar(recolor, &cg_enemyRailColor1);
				recolor[3] = 255;

				SC_Vec3ColorFromCvar(le->color, &cg_enemyRailColor1);
				le->color[0] *= 0.75;
				le->color[1] *= 0.75;
				le->color[2] *= 0.75;
				le->color[3] = 1.0f;
				Vector4Copy(le->color, lecolor);
			} else {
				re->shaderRGBA[0] = ci->color1[0] * 255;
				re->shaderRGBA[1] = ci->color1[1] * 255;
				re->shaderRGBA[2] = ci->color1[2] * 255;
				re->shaderRGBA[3] = 255;

				le->color[0] = ci->color1[0] * 0.75;
				le->color[1] = ci->color1[1] * 0.75;
				le->color[2] = ci->color1[2] * 0.75;
				le->color[3] = 1.0f;

				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}

			if (*cg_enemyRailColor2.string) {
				SC_ByteVec3ColorFromCvar(recolor, &cg_enemyRailColor2);
				recolor[3] = 255;
				SC_Vec3ColorFromCvar(lecolor, &cg_enemyRailColor2);
				lecolor[0] *= 0.75;
				lecolor[1] *= 0.75;
				lecolor[2] *= 0.75;
				lecolor[3] = 1.0f;
			} else {
				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}


		} else {
			re->shaderRGBA[0] = ci->color1[0] * 255;
			re->shaderRGBA[1] = ci->color1[1] * 255;
			re->shaderRGBA[2] = ci->color1[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color1[0] * 0.75;
			le->color[1] = ci->color1[1] * 0.75;
			le->color[2] = ci->color1[2] * 0.75;
			le->color[3] = 1.0f;

            recolor[0] = ci->color2[0] * 255;
            recolor[1] = ci->color2[1] * 255;
            recolor[2] = ci->color2[2] * 255;
            recolor[3] = 255;

            lecolor[0] = ci->color2[0] * 0.75;
            lecolor[1] = ci->color2[1] * 0.75;
            lecolor[2] = ci->color2[2] * 0.75;
            lecolor[3] = 1.0f;
		}
	} else {  // team game
		enemyRail = CG_IsEnemy(ci);
		teamRail = CG_IsTeammate(ci);

		if (!CG_IsUs(ci)  &&  enemyRail  &&  (*cg_enemyRailColor1.string  ||  cg_enemyRailColor1Team.integer  ||  *cg_enemyRailColor2.string  ||  cg_enemyRailColor2Team.integer)) {
			const vmCvar_t *v;

			if (cg_enemyRailColor1Team.integer) {
				if (ci->team == TEAM_RED) {
					v = &cg_weaponRedTeamColor;
				} else {
					v = &cg_weaponBlueTeamColor;
				}
			} else {
				v = &cg_enemyRailColor1;
			}

			if (*v->string) {
				SC_ByteVec3ColorFromCvar(re->shaderRGBA, v);
				re->shaderRGBA[3] = 255;
				SC_ByteVec3ColorFromCvar(recolor, v);
				recolor[3] = 255;

				SC_Vec3ColorFromCvar(le->color, v);
				le->color[0] *= 0.75;
				le->color[1] *= 0.75;
				le->color[2] *= 0.75;
				le->color[3] = 1.0f;
				Vector4Copy(le->color, lecolor);
			} else {
				re->shaderRGBA[0] = ci->color1[0] * 255;
				re->shaderRGBA[1] = ci->color1[1] * 255;
				re->shaderRGBA[2] = ci->color1[2] * 255;
				re->shaderRGBA[3] = 255;

				le->color[0] = ci->color1[0] * 0.75;
				le->color[1] = ci->color1[1] * 0.75;
				le->color[2] = ci->color1[2] * 0.75;
				le->color[3] = 1.0f;

				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}

			if (cg_enemyRailColor2Team.integer) {
				if (ci->team == TEAM_RED) {
					v = &cg_weaponRedTeamColor;
				} else {
					v = &cg_weaponBlueTeamColor;
				}
			} else {
				v = &cg_enemyRailColor2;
			}

			if (*v->string) {
				SC_ByteVec3ColorFromCvar(recolor, v);
				recolor[3] = 255;
				SC_Vec3ColorFromCvar(lecolor, v);
				lecolor[0] *= 0.75;
				lecolor[1] *= 0.75;
				lecolor[2] *= 0.75;
				lecolor[3] = 1.0f;
			} else {
				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}
		} else if (!CG_IsUs(ci)  &&  teamRail  &&  (*cg_teamRailColor1.string  ||  cg_teamRailColor1Team.integer  ||  *cg_teamRailColor2.string  ||  cg_teamRailColor2Team.integer)) {
			const vmCvar_t *v;

			if (cg_teamRailColor1Team.integer) {
				if (ci->team == TEAM_RED) {
					v = &cg_weaponRedTeamColor;
				} else {
					v = &cg_weaponBlueTeamColor;
				}
			} else {
				v = &cg_teamRailColor1;
			}

			if (*v->string) {
				SC_ByteVec3ColorFromCvar(re->shaderRGBA, v);
				re->shaderRGBA[3] = 255;
				SC_ByteVec3ColorFromCvar(recolor, v);
				recolor[3] = 255;

				SC_Vec3ColorFromCvar(le->color, v);
				le->color[0] *= 0.75;
				le->color[1] *= 0.75;
				le->color[2] *= 0.75;
				le->color[3] = 1.0f;
				Vector4Copy(le->color, lecolor);
			} else {
				re->shaderRGBA[0] = ci->color1[0] * 255;
				re->shaderRGBA[1] = ci->color1[1] * 255;
				re->shaderRGBA[2] = ci->color1[2] * 255;
				re->shaderRGBA[3] = 255;

				le->color[0] = ci->color1[0] * 0.75;
				le->color[1] = ci->color1[1] * 0.75;
				le->color[2] = ci->color1[2] * 0.75;
				le->color[3] = 1.0f;

				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}

			if (cg_teamRailColor2Team.integer) {
				if (ci->team == TEAM_RED) {
					v = &cg_weaponRedTeamColor;
				} else {
					v = &cg_weaponBlueTeamColor;
				}
			} else {
				v = &cg_teamRailColor2;
			}

			if (*v->string) {
				SC_ByteVec3ColorFromCvar(recolor, v);
				recolor[3] = 255;
				SC_Vec3ColorFromCvar(lecolor, v);
				lecolor[0] *= 0.75;
				lecolor[1] *= 0.75;
				lecolor[2] *= 0.75;
				lecolor[3] = 1.0f;
			} else {
				recolor[0] = ci->color2[0] * 255;
				recolor[1] = ci->color2[1] * 255;
				recolor[2] = ci->color2[2] * 255;
				recolor[3] = 255;

				lecolor[0] = ci->color2[0] * 0.75;
				lecolor[1] = ci->color2[1] * 0.75;
				lecolor[2] = ci->color2[2] * 0.75;
				lecolor[3] = 1.0f;
			}
		} else {
			re->shaderRGBA[0] = ci->color1[0] * 255;
			re->shaderRGBA[1] = ci->color1[1] * 255;
			re->shaderRGBA[2] = ci->color1[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color1[0] * 0.75;
			le->color[1] = ci->color1[1] * 0.75;
			le->color[2] = ci->color1[2] * 0.75;
			le->color[3] = 1.0f;

            recolor[0] = ci->color2[0] * 255;
            recolor[1] = ci->color2[1] * 255;
            recolor[2] = ci->color2[2] * 255;
            recolor[3] = 255;

            lecolor[0] = ci->color2[0] * 0.75;
            lecolor[1] = ci->color2[1] * 0.75;
            lecolor[2] = ci->color2[2] * 0.75;
            lecolor[3] = 1.0f;
		}
	}

	AxisClear( re->axis );

	VectorMA(move, 20, vec, move);
	VectorScale (vec, SPACING, vec);

	if (!cg_railFromMuzzle.integer) {
		// nudge down a bit so it isn't exactly in center
		if ((CG_IsUs(ci)  &&  cg_railNudge.integer)  ||
			(cgs.gametype >= GT_TEAM  &&  CG_IsTeammate(ci)  &&  cg_teamRailNudge.integer  &&  !CG_IsUs(ci))  ||
			(cgs.gametype >= GT_TEAM  &&  CG_IsEnemy(ci)  &&  cg_enemyRailNudge.integer  &&  !CG_IsUs(ci))  ||
			(cgs.gametype < GT_TEAM  &&  CG_IsEnemy(ci)  &&  cg_enemyRailNudge.integer  &&  !CG_IsUs(ci))) {
			//Com_Printf("nudging  us:%d  enemy%d\n", CG_IsUs(ci), CG_IsEnemy(ci));
			re->origin[2] -= 8;
			re->oldorigin[2] -= 8;
		}
	}

	if ((CG_IsUs(ci)  &&  !cg_railRings.integer)  ||
			(cgs.gametype >= GT_TEAM  &&  CG_IsTeammate(ci)  &&  !cg_teamRailRings.integer)  ||
			(cgs.gametype >= GT_TEAM  &&  CG_IsEnemy(ci)  &&  !cg_enemyRailRings.integer)  ||
			(cgs.gametype < GT_TEAM  &&  CG_IsEnemy(ci)  &&  !cg_enemyRailRings.integer)) {
		return;
	}

	skip = -1;

	j = 18;
    for (i = 0; i < len; i += SPACING) {
		if (i != skip) {
			skip = i + SPACING;
			le = CG_AllocLocalEntity();
            re = &le->refEntity;
            le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
            le->startTime = cg.time;
            le->endTime = cg.time + (i>>1) + 600;
            le->lifeRate = 1.0 / (le->endTime - le->startTime);

            re->shaderTime = cg.time / 1000.0f;
            re->reType = RT_SPRITE;
            re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;


			memcpy(re->shaderRGBA, recolor, sizeof(re->shaderRGBA));
			Vector4Copy(lecolor, le->color);

            le->pos.trType = TR_LINEAR;
            le->pos.trTime = cg.time;

			VectorCopy(move, move2);
            VectorMA(move2, RADIUS , axis[j], move2);
            VectorCopy(move2, le->pos.trBase);

            le->pos.trDelta[0] = axis[j][0]*6;
            le->pos.trDelta[1] = axis[j][1]*6;
            le->pos.trDelta[2] = axis[j][2]*6;
		}

        VectorAdd (move, vec, move);

        j = j + ROTATION < 36 ? j + ROTATION : (j + ROTATION) % 36;
	}
}

/*
==========================
CG_ProjectileTrail
==========================
*/
static void CG_ProjectileTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	entityState_t tmpes;
	vec3_t	up;
	localEntity_t	*smoke;
	int radius;
	int weapon;
	int cgtime;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	cgtime = ent->cgtime;

	weapon = wi - cg_weapons;

	radius = 0;
	switch (weapon) {
	case WP_ROCKET_LAUNCHER:
		radius = cg_smokeRadius_RL.integer;
		break;
	case WP_GRENADE_LAUNCHER:
		radius = cg_smokeRadius_GL.integer;
		break;
	case WP_PROX_LAUNCHER:
		radius = cg_smokeRadius_PL.integer;
		break;
	default:
		break;
	}

	if (radius < 1) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	//es = &ent->currentState;
	memcpy(&tmpes, &ent->currentState, sizeof(tmpes));
	es = &tmpes;
	//VectorCopy(ent->lerpOrigin, es->pos.trBase);
	//es->pos.trTime -= ent->serverTimeOffset;
	es->pos.trTime += ent->serverTimeOffset;
	if (ent->serverTimeOffset) {
		//Com_Printf("^2projectiletrail servertimeoffset %d\n", ent->serverTimeOffset);
	}

	//es->pos.trTime -= 40;
	//startTime = ent->trailTime;
	startTime = ent->trailTime;

	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectoryf( &es->pos, cgtime, origin, cg.foverf );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cgtime;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cgtime;
	//ent->trailTime -= ent->serverTimeOffset;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	{
		int count;

		count = 0;

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up,
							  //wi->trailRadius,
							  //count == 0 ? wi->trailRadius : wi->trailRadius,
							  count == 0 ? radius : radius,
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime,
					  t,
					  0,
					  0,
							  count == 0 ? cgs.media.smokePuffShader : cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
		count++;
	}
	}
}

#if 1  //def MPACK
/*
==========================
CG_NailTrail
==========================
*/
static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	entityState_t tmpes;
	vec3_t	up;
	localEntity_t	*smoke;
	int cgtime;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	if (cg_smokeRadius_NG.integer < 1) {
		return;
	}

	cgtime = ent->cgtime;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	//es = &ent->currentState;
	memcpy(&tmpes, &ent->currentState, sizeof(tmpes));
	es = &tmpes;
	es->pos.trTime += ent->serverTimeOffset;
	if (ent->serverTimeOffset) {
		//Com_Printf("^2nailtrail servertimeoffset %d\n", ent->serverTimeOffset);
	}
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectoryf( &es->pos, cgtime, origin, cg.foverf );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cgtime;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cgtime;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up,
							  //wi->trailRadius,
							  cg_smokeRadius_NG.integer,
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime,
					  t,
					  0,
					  0,
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}
#endif

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail( centity_t *cent, const weaponInfo_t *wi ) {
	localEntity_t	*le;
	refEntity_t		*re;
	entityState_t	*es;
	entityState_t tmpes;
	vec3_t			velocity, xvelocity, origin;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	float	waterScale = 1.0f;
	int cgtime;

#if 0
	if ( cg_noProjectileTrail.integer || cg_oldPlasma.integer ) {
		return;
	}
#endif
	if (cg_plasmaStyle.integer != 2) {
		return;
	}

	cgtime = cent->cgtime;

	//es = &cent->currentState;
	memcpy(&tmpes, &cent->currentState, sizeof(tmpes));
	es = &tmpes;
	//es->pos.trTime += cent->serverTimeOffset;

	BG_EvaluateTrajectoryf( &es->pos, cgtime, origin, cg.foverf );

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime = cgtime;
	le->endTime = le->startTime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cgtime;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	VectorAdd( origin, xoffset, re->origin );
	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->shaderTime = cgtime / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bounceFactor = 0.3f;

	re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
	re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
	re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashDlightColor[0] * 0.2;
	le->color[1] = wi->flashDlightColor[1] * 0.2;
	le->color[2] = wi->flashDlightColor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cgtime;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;

}

static void CG_FX_GrappleTrail (const clientInfo_t *ci, const vec3_t start, const vec3_t end)
{
	//weaponInfo_t *weapon;
	centity_t *cent;
	int clientNum;

	//CG_Printf("CG_FX_RailTrail() clientNum %d  %f\n", ci - cgs.clientinfo, Distance(start, end));

	clientNum = ci - cgs.clientinfo;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		clientNum = 0;
	}

	cent = &cg_entities[clientNum];

	//Com_Printf("fx rail trail: %s  origin(%f %f %f)  (%f %f %f)\n", ci->name, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2], start[0], start[1], start[2]);

	//weapon = &cg_weapons[WP_RAILGUN];

	CG_ResetScriptVars();
	CG_CopyPlayerDataToScriptData(cent);
	VectorCopy(start, ScriptVars.origin);
	VectorCopy(start, ScriptVars.parentOrigin);
	VectorCopy(end, ScriptVars.end);
	VectorCopy(end, ScriptVars.parentEnd);
	VectorSubtract(end, start, ScriptVars.dir);
	VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

	//VectorCopy(ci->color1, ScriptVars.color);
	//ScriptVars.color[3] = 1;
	//VectorCopy(ci->color1, ScriptVars.color1);
	//VectorCopy(ci->color2, ScriptVars.color2);

	//Com_Printf("rail:\n'%s'\n", EffectScripts.weapons[WP_RAILGUN].trailScript);
	CG_RunQ3mmeScript(EffectScripts.weapons[WP_GRAPPLING_HOOK].trailScript, NULL);
	//cent->trailTime = cg.time;
}

/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi ) {
	vec3_t	origin;
	entityState_t	*es;
	entityState_t tmpes;
	vec3_t			forward, up;
	refEntity_t		beam;
	clientInfo_t *ci;
	int clientNum;
	int cgtime;
	//centity_t *playerCent;
	//vec3_t end;

	//Com_Printf("%f grapple trail\n", cg.ftime);

	cgtime = ent->cgtime;

	//es = &ent->currentState;
	memcpy(&tmpes, &ent->currentState, sizeof(tmpes));
	es = &tmpes;
	//es->pos.trTime += ent->serverTimeOffset;

	BG_EvaluateTrajectoryf( &es->pos, cgtime, origin, cg.foverf );
	ent->trailTime = cgtime;

	clientNum = ent->currentState.otherEntityNum;
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("CG_GrappleTrail() invalid client number %d\n", clientNum);
		clientNum = 0;
	}
	ci = &cgs.clientinfo[clientNum];
	//playerCent = &cg_entities[clientNum];

	memset( &beam, 0, sizeof( beam ) );
	//FIXME adjust for muzzle position
	VectorCopy ( cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin );
	beam.origin[2] += 26;
	AngleVectors( cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up );
	VectorMA( beam.origin, -6, up, beam.origin );
	VectorCopy( origin, beam.oldorigin );

#if 0
	if (Distance( beam.origin, beam.oldorigin ) < 64 )
		return; // Don't draw if close
#endif

	//VectorCopy(playerCent->lerpOrigin, beam.origin);
	//Wolfcam_CalcMuzzlePoint (int entityNum, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, qboolean useLerp);
	//Wolfcam_CalcMuzzlePoint(clientNum, beam.origin, NULL, NULL, NULL, qtrue);

	CG_GetWeaponFlashOrigin(clientNum, beam.oldorigin);
	BG_EvaluateTrajectoryf(&es->pos, cgtime, beam.origin, cg.foverf );


	if (EffectScripts.weapons[WP_GRAPPLING_HOOK].hasTrailScript) {
		CG_FX_GrappleTrail(ci, beam.origin, beam.oldorigin);
		return;
	}

	beam.reType = RT_GRAPPLE;  //RT_BEAM;  //RT_LIGHTNING;  //RT_RAIL_RINGS;  //RT_LIGHTNING;
	//beam.customShader = cgs.media.lightningShader;
	beam.customShader = trap_R_RegisterShader("grapplingChain");
	beam.radius = 8;  //SC_Cvar_Get_Int("grad");  //24;  //8;  //124;  //8;
	beam.width = 4;  //8;  //SC_Cvar_Get_Float("gwidth");  //256;

	//AxisClear( beam.axis );
	//AnglesToAxis(ent->lerpAngles, beam.axis);
	//AnglesToAxis(cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	CG_AddRefEntity(&beam);
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail (centity_t *ent, const weaponInfo_t *wi)
{
	CG_ProjectileTrail(ent, wi);
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t *item;
	const gitem_t *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	if (weaponNum < 0  ||  weaponNum >= WP_NUM_WEAPONS) {  // crash tutorial
		static qboolean warningIssued = qfalse;

		if (!warningIssued) {
			Com_Printf("^3CG_RegisterWeapon %d invalid weapon\n", weaponNum);
			warningIssued = qtrue;
		}
		return;
	}

	//Com_Printf("register weapon %d\n", weaponNum);

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	//FIXME looped multiple times
	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
		//CG_Printf("^2Couldn't find weapon %i\n", weaponNum);
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	if (weaponNum == WP_MACHINEGUN) {
		//Q_strcat( path, sizeof(path), "_barrel_1.md3" );  // testing
		Q_strcat( path, sizeof(path), "_barrel.md3" );
	} else {
		Q_strcat( path, sizeof(path), "_barrel.md3" );
	}
	if (CG_FileExists(path)) {
		weaponInfo->barrelModel = trap_R_RegisterModel( path );
	}

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	switch ( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/fstrun.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav", qfalse );
		break;

	case WP_LIGHTNING:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/lightning/lg_hum.wav", qfalse );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/lightning/lg_fire.wav", qfalse );
		//cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBolt1");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel( "models/weaphits/crackle.md3" );
		cgs.media.sfx_lghit1 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit.wav", qfalse );
		cgs.media.sfx_lghit2 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit2.wav", qfalse );
		cgs.media.sfx_lghit3 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit3.wav", qfalse );

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		//weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileModel = trap_R_RegisterModel("models/weapons2/grapple/grapple_hook.md3");
		//FIXME sounds
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/fstrun.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBolt1");
		break;

#if 1  //def MPACK
	case WP_CHAINGUN:
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/vulcan/wvulfire.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;
#endif

	case WP_MACHINEGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shotgun/sshotf1b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_ProjectileTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;

		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

#if 1  //def MPACK
	case WP_PROX_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/proxmine.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/proxmine/wstbfire.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;
#endif

	case WP_GRENADE_LAUNCHER: {
		int vlight;

		// colored grenades
		vlight = SC_Cvar_Get_Int("r_vertexlight");
		if (vlight)
			trap_Cvar_Set("r_vertexlight", "0");

		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/grenade1.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/grenade/grenlf1a.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );

		trap_Cvar_Set("r_vertexlight", va("%d", vlight));
		break;
	}
#if 1  //def MPACK
	case WP_NAILGUN:
		weaponInfo->ejectBrassFunc = CG_NailgunEjectBrass;
		weaponInfo->missileTrailFunc = CG_NailTrail;
//		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/nail.md3" );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/nailgun/wnalfire.wav", qfalse );
		break;
#endif

	case WP_PLASMAGUN:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileTrailFunc = CG_PlasmaTrail;
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/plasma/lasfly.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/plasma/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/railgun/rg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.5f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/railgun/railgf1a.wav", qfalse );
		cgs.media.railExplosionShader = trap_R_RegisterShader( "railExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );
		break;

	case WP_BFG:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/bfg/bfg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.7f, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/bfg/bfg_fire.wav", qfalse );
		cgs.media.bfgExplosionShader = trap_R_RegisterShader( "bfgExplosion" );
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/bfg.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;

	case WP_HEAVY_MACHINEGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/hmg/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/hmg/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/hmg/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/hmg/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	const gitem_t			*item;
	int i;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		//CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
		Com_Printf( "^1CG_RegisterItemVisuals: itemNum %d out of range [0-%d]\n", itemNum, bg_numItems-1 );
		//CG_Abort();
		return;
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];
	//Com_Printf("loading %s\n", item->pickup_name);

	memset( itemInfo, 0, sizeof( itemInfo_t ) );
	itemInfo->registered = qtrue;

	//itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );
#if 1
	for (i = 0;  i < 4;  i++) {
		if (!item->world_model[i]) {
			//Com_Printf("skip %d\n", i);
			continue;
		}
		itemInfo->models[i] = trap_R_RegisterModel(item->world_model[i]);
		//Com_Printf("model %d\n", itemInfo->models[i]);
	}
#endif

	// some quake live icons don't define a shader, make sure to nomip
	// ex:  icons/iconh_borb
	itemInfo->icon = trap_R_RegisterShaderNoMip( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
#if 1
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH ||
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
#endif
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
int CG_MapTorsoToWeaponFrame( const clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}

	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;
	int cgtime;
	float xyspeed;
	float landChange;
	int landTime;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	if (cg_drawGun.integer <= 0  ||  cg_drawGun.integer == 2  ||  cg_drawGun.integer == 3) {
		return;
	}

	if (cg.freezeEntity[cg.snap->ps.clientNum]) {
		cgtime = cg.freezeCgtime;
		xyspeed = cg.freezeXyspeed;
		landChange = cg.freezeLandChange;
		landTime = cg.freezeLandTime;
	} else {
		cgtime = cg.time;
		xyspeed = cg.xyspeed;
		landChange = cg.landChange;
		landTime = cg.landTime;
	}

	//Com_Printf("xyspeed %f\n", xyspeed);
	//Com_Printf("bob: %f\n", cg.bobfracsin);
	//Com_Printf("bobcycle:  %d\n", cg.snap->ps.bobCycle);
	
	//FIXME with wolfcam_following cg.xyspeed : xyspeed hasn't been calculated

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -xyspeed;
	} else {
		scale = xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cgtime - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += landChange*0.25 *
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cgtime - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = xyspeed + 40;
	fracsin = sin( cgtime * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
void CG_LightningBolt (centity_t *cent, vec3_t origin)
{
	trace_t  trace;
	refEntity_t  beam;
	vec3_t   forward;
	vec3_t   muzzlePoint, endPoint;
	int anim;
	vec3_t dir;
	vec3_t tmpVector;
	float tm;
	qboolean firstFire;
	qboolean impactFpsIndependent;

	if (cent->currentState.weapon != WP_LIGHTNING) {
		return;
	}
	if (!(cent->currentState.eFlags & EF_FIRING)) {
		return;
	}

	memset(&beam, 0, sizeof(beam));

	// CPMA  "true" lightning
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)  &&  !wolfcam_following  &&  !cg.freecam  &&  !cg.renderingThirdPerson  &&  cg_lightningAngleOriginStyle.integer != 2) {
		vec3_t angle;
		int i;

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdefViewAngles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );

		if (cg_lightningAngleOriginStyle.integer == 0) {
			// baseq3
			VectorCopy(cent->lerpOrigin, muzzlePoint);
			anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
			if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
				muzzlePoint[2] += CROUCH_VIEWHEIGHT;
			} else {
				muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
			}
		} else {
			// ql, cpma
			VectorCopy(cg.refdef.vieworg, muzzlePoint);
		}
	} else {
		// !CPMA
		AngleVectors(cent->lerpAngles, forward, NULL, NULL);
		VectorCopy(cent->lerpOrigin, muzzlePoint);
		anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
		if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
			muzzlePoint[2] += CROUCH_VIEWHEIGHT;
		} else {
			muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
		}
	}

	VectorMA(muzzlePoint, 14, forward, muzzlePoint);

	// project forward by the lightning range
	VectorMA(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);

	// see if it hit a wall
	Wolfcam_WeaponTrace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cent->currentState.number, MASK_SHOT);

	if (cent->currentState.number == cg.snap->ps.clientNum  &&  cg_debugLightningImpactDistance.integer) {
		//FIXME subtract 14?
		Com_Printf("lg impact dist %f\n", Distance(muzzlePoint, trace.endpos));
	}

	if (cg_fxLightningGunImpactFps.value > 0.0) {
		tm = 1000.0 / cg_fxLightningGunImpactFps.value;
		impactFpsIndependent = qtrue;
	} else {
		// fps dependent
		tm = 1;
		//cent->lastLgImpactTime = cg.ftime - tm;
		//cent->lastLgImpactTime = 0;
		impactFpsIndependent = qfalse;
	}


	if (trace.fraction < 1.0  &&  EffectScripts.weapons[WP_LIGHTNING].hasImpactScript  &&  (trace.entityNum < 0  ||  trace.entityNum >= MAX_CLIENTS)  &&  !(trace.surfaceFlags & SURF_SKY)) {
		float diff;
		double ftimeOrig;
		int i;
		float dist;
		vec3_t lastDir;
		//vec3_t origin;
		float distChunk;
		vec3_t newOrigin;
		vec3_t newEndPoint;
		vec3_t newForward;

	if (cent->lastLgFireFrameCount != cg.clientFrame - 1  ||  cent->lastLgImpactFrameCount != cg.clientFrame - 1  ||  cent->lastLgImpactTime == 0  ||  !impactFpsIndependent) {
		//Com_Printf("%f  %d %p  not firing %d  hit %d\n", cg.ftime, cent->currentState.clientNum, cent, cg.clientFrame - cent->lastLgFireFrameCount, cg.clientFrame - cent->lastLgImpactFrameCount);

		cent->lastLgImpactTime = cg.ftime - tm;  // draw mark immediately
		//cent->lastLgImpactTime = cg.ftime;
		firstFire = qtrue;
	} else {
		firstFire = qfalse;
		//Com_Printf("first fire false\n");
	}
		diff = cg.ftime - cent->lastLgImpactTime;

		if (diff < tm  &&  !firstFire) {  //  &&  !firstFire) {
			//Com_Printf("skippy %f < %f  first %d  %f - %f\n", diff, tm, firstFire, cg.ftime, cent->lastLgImpactTime);
			goto skippy;
		}

		if (firstFire) {
			VectorCopy(trace.endpos, cent->lastLgImpactPosition);
			//Com_Printf("first fire....\n");
		}

		dist = Distance(trace.endpos, cent->lastLgImpactPosition);
		VectorSubtract(trace.endpos, cent->lastLgImpactPosition, lastDir);
		VectorNormalize(lastDir);
		distChunk = dist / (diff / tm);

		ftimeOrig = cg.ftime;

		cent->lastLgImpactTime += tm;
		i = 0;
		for (; cg.ftime - cent->lastLgImpactTime >= 0;  cent->lastLgImpactTime += tm, i++) {
			trace_t trace2;

			//Com_Printf("%d\n", i);
		//VectorMA(origin, 8192 * 16, newForward, end
		//VectorMA(cent->lastLgImpactPosition, distChunk , lastDir, ScriptVars.origin);
		if (distChunk > 0.0) {
			VectorMA(cent->lastLgImpactPosition, distChunk , lastDir, newOrigin);
		} else {
			VectorCopy(cent->lastLgImpactPosition, newOrigin);
		}


		VectorCopy(newOrigin, newEndPoint);

		VectorSubtract(newOrigin, muzzlePoint, newForward);
		//VectorSubtract(muzzlePoint, newOrigin, newForward);
		VectorNormalize(newForward);

		VectorMA(muzzlePoint, LIGHTNING_RANGE, newForward, newEndPoint);
		Wolfcam_WeaponTrace(&trace2, muzzlePoint, vec3_origin, vec3_origin, newEndPoint, cent->currentState.number, MASK_SHOT);
		if (trace2.fraction >= 1.0  ||  (trace2.entityNum >= 0  &&  trace2.entityNum < MAX_CLIENTS)) {
			//Com_Printf("skipping\n");
			VectorCopy(newOrigin, cent->lastLgImpactPosition);
			continue;
		}

		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		if (trace2.surfaceFlags & SURF_METALSTEPS) {
			ScriptVars.surfaceType = 1;
		} else if (trace2.surfaceFlags & SURF_WOOD) {
			ScriptVars.surfaceType = 2;
		} else if (trace2.surfaceFlags & SURF_DUST) {
			ScriptVars.surfaceType = 3;
		} else if (trace2.surfaceFlags & SURF_SNOW) {
			ScriptVars.surfaceType = 4;
		}

		//FIXME SURF_SLICK ?   can it combine with other SURF_ ?



		//VectorCopy(newOrigin, ScriptVars.origin);
		VectorCopy(trace2.endpos, ScriptVars.origin);

		//VectorCopy(trace.endpos, ScriptVars.origin);
		//VectorSubtract(trace.endpos, muzzlePoint, ScriptVars.dir);
		//VectorSubtract(muzzlePoint, trace.endpos, ScriptVars.dir);

		//VectorSubtract(origin, trace.endpos, ScriptVars.dir);
		//VectorSubtract(origin, trace.endpos, tmpVector);
		//VectorSubtract(trace.endpos, origin, tmpVector);
		//VectorStartEndDir(muzzlePoint, origin, tmpVector);
		//VectorStartEndDir(origin, muzzlePoint, tmpVector);
		VectorStartEndDir(muzzlePoint, trace2.endpos, tmpVector);

		//RotatePointAroundVector(ScriptVars.velocity, trace.plane.normal, tmpVector, 180.0);
		VectorReflect(tmpVector, trace.plane.normal, ScriptVars.velocity);

		//VectorCopy(tmpVector, ScriptVars.velocity);

		VectorCopy(trace.plane.normal, ScriptVars.dir);

		VectorNormalize(ScriptVars.velocity);
		VectorNormalize(ScriptVars.dir);

		cg.ftime = cent->lastLgImpactTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[WP_LIGHTNING].impactScript, NULL);
		cg.ftime = ftimeOrig;
		//cent->lastLgImpactTime = cg.ftime;
		//VectorCopy(trace.endpos, cent->lastLgImpactPosition);
		VectorCopy(newOrigin, cent->lastLgImpactPosition);

		}

		cent->lastLgImpactTime -= tm;
		cg.ftime = ftimeOrig;
		//Com_Printf("%f  run %d\n", cg.ftime, i);
	}

 skippy:

	cent->lastLgFireFrameCount = cg.clientFrame;

	if (trace.fraction < 1.0  &&  (trace.entityNum < 0  ||  trace.entityNum >= MAX_CLIENTS)  &&  !(trace.surfaceFlags & SURF_SKY)) {
		cent->lastLgImpactFrameCount = cg.clientFrame;
	}

	if (EffectScripts.weapons[WP_LIGHTNING].hasTrailScript) {
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		//VectorCopy(muzzlePoint, ScriptVars.origin);
		VectorCopy(origin, ScriptVars.origin);
		VectorCopy(trace.endpos, ScriptVars.end);
		//VectorSubtract(trace.endpos, muzzlePoint, ScriptVars.dir);
		VectorSubtract(trace.endpos, origin, ScriptVars.dir);
		//VectorSubtract(muzzlePoint, trace.endpos, ScriptVars.dir);
		//VectorNormalize(ScriptVars.dir);

		VectorCopy(origin, ScriptVars.parentOrigin);
		VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

		VectorCopy(cent->lastTrailIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastTrailIntervalTime;
		VectorCopy(cent->lastTrailDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastTrailDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[WP_LIGHTNING].trailScript, NULL);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTrailIntervalPosition);
		cent->lastTrailIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastTrailDistancePosition);
		cent->lastTrailDistanceTime = ScriptVars.lastDistanceTime;
		cent->trailTime = cg.time;
		return;
	}

	// this is the endpoint
	VectorCopy(trace.endpos, beam.oldorigin);

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy(origin, beam.origin);

	beam.reType = RT_LIGHTNING;

	if (cg_lightningStyle.integer > 0  &&  cg_lightningStyle.integer < 6) {
		beam.customShader = trap_R_RegisterShader(va("lightningBolt%d", cg_lightningStyle.integer));
	} else {
		beam.customShader = cgs.media.lightningShader;
	}

	if (CG_IsFirstPersonView(cent->currentState.number)  &&  cg_lightningRenderStyle.integer == 1) {
		beam.renderfx = RF_DEPTHHACK;
	}

	beam.radius = cg_lightningSize.value;

	CG_AddRefEntity(&beam);

#if 0
	//return;  //FIXME testing

	if (trace.entityNum < MAX_CLIENTS) {
		if (CG_IsUs(&cgs.clientinfo[cent->currentState.clientNum])) {
			//CG_Printf("hit player %d %d\n", trace.entityNum, cg.time);
		}
	}
#endif

	// add the impact flare if it hit something
	if (((cg_lightningImpact.integer == 1  &&   trace.fraction < 1.0)
		    ||
		 (cg_lightningImpact.integer == 2  &&  trace.fraction < 1.0  &&  (trace.entityNum < 0  ||  trace.entityNum >= MAX_CLIENTS) && /*trace.entityNum != 1022*/  (trace.endpos[0] != endPoint[0]  &&  trace.endpos[1] != endPoint[1]  &&  trace.endpos[2] != endPoint[2])))

		&&  !(trace.surfaceFlags & SURF_SKY)

) {
		vec3_t	angles;
		float scale;
		float dist;
		float wantedAngle;
		vec3_t mins, maxs;
		float newRadius;

		VectorSubtract(trace.endpos, beam.origin, dir);
		dist = VectorLength(dir);
		VectorNormalize(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA(trace.endpos, -cg_lightningImpactProject.value, dir, beam.origin);

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis(angles, beam.axis);
		scale = cg_lightningImpactSize.value;
		if (dist < cg_lightningImpactCap.value) {
			if (dist < cg_lightningImpactCapMin.value) {
				dist = cg_lightningImpactCapMin.value;
			}
			trap_R_ModelBounds(beam.hModel, mins, maxs);
			wantedAngle = atan2(maxs[0], cg_lightningImpactCap.value);
			newRadius = sin(wantedAngle) * dist;
			scale *= (newRadius / maxs[0]);
		}
		if (trace.entityNum >= 0  &&  trace.entityNum < MAX_CLIENTS  &&  CG_IsUs(&cgs.clientinfo[trace.entityNum])  &&  !cg.freecam  &&  !cg.renderingThirdPerson) {
			if (*cg_lightningImpactOthersSize.string) {
				scale = cg_lightningImpactOthersSize.value;
			}
		}
		CG_ScaleModel(&beam, scale);
		CG_AddRefEntity(&beam);
	}
}

/*

static void CG_LightningBolt( const centity_t *cent, vec3_t origin ) {
	trace_t		trace;
	refEntity_t		beam;
	vec3_t			forward;
	vec3_t			muzzlePoint, endPoint;

	if ( cent->currentState.weapon != WP_LIGHTNING ) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// find muzzle point for this frame
	VectorCopy( cent->lerpOrigin, muzzlePoint );
	AngleVectors( cent->lerpAngles, forward, NULL, NULL );

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	beam.radius = 8;
	CG_AddRefEntity(&beam);

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		CG_AddRefEntity(&beam);
	}
}
*/

/*
===============
CG_SpawnRailTrail

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
===============
*/
void CG_SpawnRailTrail( centity_t *cent, const vec3_t origin ) {
	const clientInfo_t	*ci;

	if ( cent->currentState.weapon != WP_RAILGUN ) {
		return;
	}
	if ( !cent->pe.railgunFlash ) {
		return;
	}

	//CG_Printf("CG_SpawnRailTrail cent %d  clientNum %d\n", cent - cg_entities, cent->currentState.clientNum);

	cent->pe.railgunFlash = qtrue;
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RailTrail( ci, origin, cent->pe.railgunImpact );
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
float CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
#if 1  //def MPACK
		if ( cent->currentState.weapon == WP_CHAINGUN && !cent->pe.barrelSpinning ) {
			CG_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, trap_S_RegisterSound( "sound/weapons/vulcan/wvulwind.wav", qfalse ) );
		}
#endif
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {
		gun->customShader = cgs.media.invisShader;
		CG_AddRefEntity(gun);
	} else {
		CG_AddRefEntity(gun);

		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			gun->customShader = cgs.media.battleWeaponShader;
			CG_AddRefEntity(gun);
		}
		if ( powerups & ( 1 << PW_QUAD ) ) {
			gun->customShader = cgs.media.quadWeaponShader;
			CG_AddRefEntity(gun);
		}
	}
}

#if 0
static void CG_CheckFxWeaponFlash (centity_t *cent, int weaponNum, const vec3_t origin)
{
	if (!EffectScripts.weapons[weaponNum].hasFlashScript) {
		return;
	}

	CG_ResetScriptVars();
	CG_CopyPlayerDataToScriptData(cent);
	VectorCopy(origin, ScriptVars.origin);
	VectorCopy(origin, ScriptVars.parentOrigin);

	VectorCopy(cent->lastFlashIntervalPosition, ScriptVars.lastIntervalPosition);
	ScriptVars.lastIntervalTime = cent->lastFlashIntervalTime;
	VectorCopy(cent->lastFlashDistancePosition, ScriptVars.lastDistancePosition);
	ScriptVars.lastDistanceTime = cent->lastFlashDistanceTime;

	CG_RunQ3mmeScript((char *)EffectScripts.weapons[weaponNum].flashScript, NULL);

	VectorCopy(ScriptVars.lastIntervalPosition, cent->lastFlashIntervalPosition);
	cent->lastFlashIntervalTime = ScriptVars.lastIntervalTime;
	VectorCopy(ScriptVars.lastDistancePosition, cent->lastFlashDistancePosition);
	cent->lastFlashDistanceTime = ScriptVars.lastDistanceTime;
}
#endif

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.

wolfcam only called once for main player
=============
*/
void CG_AddPlayerWeapon( const refEntity_t *parent, const playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	const weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
//	int	col
	clientInfo_t	*ci;
	float flashSize;
	float dlight[3];
	float f;
	qboolean revertColors = qfalse;
	vec3_t origColor1;
	vec3_t origColor2;

	//Com_Printf("cent %d %p\n", cent == &cg.predictedPlayerEntity, cent);
	//Com_Printf("ps %p\n", ps);

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	weaponNum = cent->currentState.weapon;

	if (weaponNum <= WP_NONE  ||  weaponNum >= WP_NUM_WEAPONS) {
		return;
	}

	if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  team == TEAM_RED  &&  weaponNum != WP_PLASMAGUN  &&  !ps) {
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	//Com_Printf("AddPlayerWeapon  ps:%p  %d  clientNum:%d\n", ps, cent->currentState.number, cent->currentState.clientNum);

	// set custom shading for railgun refire rate
	if ( ps ) {
		if (cg_railUseOwnColors.integer  &&  CG_IsUs(&cgs.clientinfo[cg.snap->ps.clientNum])) {
			VectorCopy(ci->color1, origColor1);
			VectorCopy(ci->color2, origColor2);
			VectorCopy(cg.color1, ci->color1);
			VectorCopy(cg.color2, ci->color2);
			revertColors = qtrue;
		}

		if ( cg.predictedPlayerState.weapon == WP_RAILGUN
			&& cg.predictedPlayerState.weaponstate == WEAPON_FIRING ) {
			//float	f;

			f = (float)cg.predictedPlayerState.weaponTime / 1500;
#if 0
			gun.shaderRGBA[1] = 0;
			gun.shaderRGBA[0] =   //0;
			gun.shaderRGBA[2] = 255 * ( 1.0 - f );
#endif
			//Com_Printf("%d\n", cg.predictedPlayerState.weaponTime);

			if (cg.predictedPlayerState.weaponTime > 60) {
				gun.shaderRGBA[0] = 80 * (1.0f - f) * ci->color1[0];
				gun.shaderRGBA[1] = 80 * (1.0f - f) * ci->color1[1];
				gun.shaderRGBA[2] = 80 * (1.0f - f) * ci->color1[2];
				gun.shaderRGBA[3] = 255;  // * (1.0f - f);
			} else {
				gun.shaderRGBA[0] = 255 * ci->color1[0];
				gun.shaderRGBA[1] = 255 * ci->color1[1];
				gun.shaderRGBA[2] = 255 * ci->color1[2];
				gun.shaderRGBA[3] = 255;
			}
		} else {
			gun.shaderRGBA[0] = 255 * ci->color1[0];
			gun.shaderRGBA[1] = 255 * ci->color1[1];
			gun.shaderRGBA[2] = 255 * ci->color1[2];
			gun.shaderRGBA[3] = 255;
		}
	} else {
		if (weaponNum == WP_RAILGUN) {
			qboolean teamRail;
			qboolean enemyRail;

			if (cg_railUseOwnColors.integer  &&  CG_IsUs(ci)) {
				VectorCopy(ci->color1, origColor1);
				VectorCopy(ci->color2, origColor2);
				VectorCopy(cg.color1, ci->color1);
				VectorCopy(cg.color2, ci->color2);
				revertColors = qtrue;
			}

			teamRail = CG_IsTeammate(ci);
			enemyRail = CG_IsEnemy(ci);
			if (cgs.gametype < GT_TEAM) {
				if (!CG_IsUs(ci)) {
					if (*cg_enemyRailItemColor.string) {
						SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_enemyRailItemColor);
						gun.shaderRGBA[3] = 255;
					} else {
						gun.shaderRGBA[0] = 255 * ci->color1[0];
						gun.shaderRGBA[1] = 255 * ci->color1[1];
						gun.shaderRGBA[2] = 255 * ci->color1[2];
						gun.shaderRGBA[3] = 255;
					}
				} else {
					gun.shaderRGBA[0] = 255 * ci->color1[0];
					gun.shaderRGBA[1] = 255 * ci->color1[1];
					gun.shaderRGBA[2] = 255 * ci->color1[2];
					gun.shaderRGBA[3] = 255;
				}
			} else {  // team game
				if (!CG_IsUs(ci)  &&  teamRail) {
					if (cg_teamRailItemColorTeam.integer) {
						if (ci->team == TEAM_RED) {
							SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_weaponRedTeamColor);
						} else {
							SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_weaponBlueTeamColor);
						}
						gun.shaderRGBA[3] = 255;
					} else if (*cg_teamRailItemColor.string) {
						SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_teamRailItemColor);
						gun.shaderRGBA[3] = 255;
					} else {
						gun.shaderRGBA[0] = 255 * ci->color1[0];
						gun.shaderRGBA[1] = 255 * ci->color1[1];
						gun.shaderRGBA[2] = 255 * ci->color1[2];
						gun.shaderRGBA[3] = 255;
					}
				} else if (!CG_IsUs(ci)  &&  enemyRail) {
					if (cg_enemyRailItemColorTeam.integer) {
						if (ci->team == TEAM_RED) {
							SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_weaponRedTeamColor);
						} else {
							SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_weaponBlueTeamColor);
						}
						gun.shaderRGBA[3] = 255;
					} else if (*cg_enemyRailItemColor.string) {
						SC_ByteVec3ColorFromCvar(gun.shaderRGBA, &cg_enemyRailItemColor);
						gun.shaderRGBA[3] = 255;
					} else {
						gun.shaderRGBA[0] = 255 * ci->color1[0];
						gun.shaderRGBA[1] = 255 * ci->color1[1];
						gun.shaderRGBA[2] = 255 * ci->color1[2];
						gun.shaderRGBA[3] = 255;
					}
				} else {  // us
					gun.shaderRGBA[0] = 255 * ci->color1[0];
					gun.shaderRGBA[1] = 255 * ci->color1[1];
					gun.shaderRGBA[2] = 255 * ci->color1[2];
					gun.shaderRGBA[3] = 255;
				}
			}

			// end weapon == WP_RAILGUN
			//cent->pe.muzzleFlashTime
			//Com_Printf("yes....\n");
			//f = cg.time - (cent->pe.muzzleFlashTime + 1500);
			f = cg.time - (cent->pe.muzzleFlashTime + 1460);  // hack
			//Com_Printf("f %f\n", f);
			if (f < 0) {
				f = 1.0 - (f / -1500);
				gun.shaderRGBA[0] *= 0.314 * f;
				gun.shaderRGBA[1] *= 0.314 * f;
				gun.shaderRGBA[2] *= 0.314 * f;
			}
		}
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		//Com_Printf("no gun model '%s'\n", weapNamesCasual[weaponNum]);
		//FIXME grapple returns here
		//FIXME fx
		//CG_PositionEntityOnTag(&gun, parent, parent->hModel, "tag_weapon");
		//CG_CheckFxWeaponFlash(cent, weaponNum, gun.origin);
		//return;
	}

	if ( !ps ) {
#if 1
		// add weapon ready sound
		//cent->pe.lightningFiring = qfalse;  // what in the fuck
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and gauntlet make a different sound when fire is held down
			if (1) {  //(cent->currentState.weapon == WP_LIGHTNING) {
				//Com_Printf("lg fire %d  %d\n", cent->currentState.number, cg.time);
				//cent->pe.lightningFiring = qtrue;
				CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			} else {
				//cent->pe.lightningFiring = qtrue;
				CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			}
		} else if ( weapon->readySound ) {
			//Com_Printf("weapon ready cn %d  wp %d\n", cent->currentState.number, weapon->readySound);
			CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
#endif

#if 0
		if (!(cent->currentState.eFlags & EF_FIRING)) {
			//cent->pe.lightningFiring = qfalse;
			cg_entities[cent->currentState.number].pe.lightningFiring = qfalse;
		}
#endif
	}

	CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");
	if (ps  &&  CG_IsFirstPersonView(ps->clientNum)) {
		CG_ScaleModel(&gun, cg_gunSize.value);
	} else {
		CG_ScaleModel(&gun, cg_gunSizeThirdPerson.value);
	}
	if (gun.hModel) {
		if (ps  &&  cg_drawGun.integer > 2) {
			gun.customShader = cgs.media.ghostWeaponShader;
			gun.shaderRGBA[0] = 255;
			gun.shaderRGBA[1] = 255;
			gun.shaderRGBA[2] = 255;
			gun.shaderRGBA[3] = 255;
		}

		if (!ps  &&  cgs.gametype == GT_RACE  &&  cg_racePlayerShader.integer) {
			if (!CG_IsUs(&cgs.clientinfo[cent->currentState.clientNum])  ||  cg_racePlayerShader.integer == 2  ||  cg_racePlayerShader.integer == 3) {
				gun.customShader = cgs.media.noPlayerClipShader;
				gun.shaderRGBA[0] = 255;
				gun.shaderRGBA[1] = 255;
				gun.shaderRGBA[2] = 255;
				gun.shaderRGBA[3] = 255;
			}
		}

		CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );
	}

	// add the spinning barrel
	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );
		if (ps  &&  CG_IsFirstPersonView(ps->clientNum)) {
			CG_ScaleModel(&barrel, cg_gunSize.value);
		} else {
			CG_ScaleModel(&barrel, cg_gunSizeThirdPerson.value);
		}
		if (ps  &&  cg_drawGun.integer > 2) {
			barrel.customShader = cgs.media.ghostWeaponShader;
			barrel.shaderRGBA[0] = 255;
			barrel.shaderRGBA[1] = 255;
			barrel.shaderRGBA[2] = 255;
			barrel.shaderRGBA[3] = 255;
		}

		if (!ps  &&  cgs.gametype == GT_RACE  &&  cg_racePlayerShader.integer) {
			if (!CG_IsUs(&cgs.clientinfo[cent->currentState.clientNum])  ||  cg_racePlayerShader.integer == 2  ||  cg_racePlayerShader.integer == 3) {
				gun.customShader = cgs.media.noPlayerClipShader;
				gun.shaderRGBA[0] = 255;
				gun.shaderRGBA[1] = 255;
				gun.shaderRGBA[2] = 255;
				gun.shaderRGBA[3] = 255;
			}
		}

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
		//Com_Printf("fake player %d  ->  %d\n", nonPredictedCent - cg_entities, cent->currentState.clientNum);
	}

	if (wolfcam_following  &&  cent->currentState.clientNum == cg.snap->ps.clientNum) {
		//nonPredictedCent = cent;
		//Com_Printf("using cent\n");
	}


	// add the flash
	//if ( ( weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK )  &&  (cent->currentState.eFlags & EF_FIRING)) {
	if ( ( weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK )  &&  (nonPredictedCent->currentState.eFlags & EF_FIRING)) {
		// && ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) {
		// continuous flash
	} else {
		//int ftime;

		if (weaponNum == WP_LIGHTNING  &&  cent->currentState.eFlags & EF_FIRING) {
			//Com_Printf("%f wtf ps %p\n", cg.ftime, ps);
		}
		// impulse flash
		//if ( cg.time - cent->pe.muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash ) {
		if ( cg.time - nonPredictedCent->pe.muzzleFlashTime > MUZZLE_FLASH_TIME && !nonPredictedCent->pe.railgunFlash ) {
			//Com_Printf("returning for %d (%d)\n", cent - cg_entities, cent->currentState.number);
			//goto bolt;
			// not called, in case code changes
			if (revertColors) {
				VectorCopy(origColor1, ci->color1);
				VectorCopy(origColor2, ci->color2);
			}
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	/*
	if (weaponNum == WP_HEAVY_MACHINEGUN) {
		flash.hModel = cg_weapons[WP_MACHINEGUN].flashModel;
	}
	*/

	if (!flash.hModel) {
		//Com_Printf("no flash model '%s'\n", weapNamesCasual[weaponNum]);
		//FIXME fx
		//return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	//return;

	// colorize the railgun blast
	if ( weaponNum == WP_RAILGUN ) {
		//clientInfo_t	*ci;

		//ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		if (cg_railUseOwnColors.integer  &&  CG_IsUs(ci)) {
			flash.shaderRGBA[0] = 255 * cg.color1[0];
			flash.shaderRGBA[1] = 255 * cg.color1[1];
			flash.shaderRGBA[2] = 255 * cg.color1[2];
		} else {
			flash.shaderRGBA[0] = 255 * ci->color1[0];
			flash.shaderRGBA[1] = 255 * ci->color1[1];
			flash.shaderRGBA[2] = 255 * ci->color1[2];
		}
	}

	if (0) {  //(weapon->hasFlashScript) {
		//CG_RunQ3mmeFlashScript(weapon, dlight, flash.shaderRGBA, &flashSize);
		//VectorCopy(flash.origin, ScriptVars.origin);
		//CG_RunQ3mmeScript((char *)weapon->flashScript);
		//return;
	} else {
		dlight[0] = weapon->flashDlightColor[0];
		dlight[1] = weapon->flashDlightColor[1];
		dlight[2] = weapon->flashDlightColor[2];


		/*
		flash.shaderRGBA[0] = 255;
		flash.shaderRGBA[1] = 255;
		flash.shaderRGBA[2] = 255;
		flash.shaderRGBA[3] = 0;
		*/

		flashSize = 300 + (rand()&31);
	}

	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	//Com_Printf("ps:%d  %p\n", ps != NULL, cent);

	if (cent == &cg.predictedPlayerEntity  &&  !cg.renderingThirdPerson  &&  !ps) {
		// don't run flash script twice for first person view
	} else if (EffectScripts.weapons[weaponNum].hasFlashScript) {
		//CG_RunQ3mmeFlashScript(weapon, dlight, flash.shaderRGBA, &flashSize);
		//memset(&ScriptVars, 0, sizeof(ScriptVars));
		//CG_Printf("addplayerweapon()  flash script cent %d\n", cent - cg_entities);
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		VectorCopy(flash.origin, ScriptVars.origin);
		VectorCopy(flash.origin, ScriptVars.parentOrigin);

		VectorCopy(cent->lastFlashIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastFlashIntervalTime;
		VectorCopy(cent->lastFlashDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastFlashDistanceTime;

		CG_RunQ3mmeScript((char *)EffectScripts.weapons[weaponNum].flashScript, NULL);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastFlashIntervalPosition);
		cent->lastFlashIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastFlashDistancePosition);
		cent->lastFlashDistanceTime = ScriptVars.lastDistanceTime;
		//return;
	}

	if (cent == &cg.predictedPlayerEntity  &&  !cg_muzzleFlash.integer  &&  ps) {
		// pass
	} else {
		if (flash.hModel) {
			CG_AddRefEntity(&flash);
		}
	}

	// bolt:
	if ( ps || cg.renderingThirdPerson ||
		 cent->currentState.number != cg.predictedPlayerState.clientNum  ||  (wolfcam_following  &&  cent->currentState.number != wcg.clientNum)) {
		// add lightning bolt
		if ((!wolfcam_following  ||  (wolfcam_following  &&  wcg.clientNum == cg.snap->ps.clientNum))
			 ||  !ps) {
			CG_LightningBolt( nonPredictedCent, flash.origin );

			//Com_Printf("adding bolt\n");
			// add rail trail
			CG_SpawnRailTrail( cent, flash.origin );

			//if ((dlight[0]  ||  dlight[1]  ||  dlight[2])  &&  !weapon->hasFlashScript) {
			if ((dlight[0]  ||  dlight[1]  ||  dlight[2])  &&  !EffectScripts.weapons[weaponNum].hasFlashScript) {
				trap_R_AddLightToScene(flash.origin, flashSize, dlight[0], dlight[1], dlight[2]);
			}
		}
	} else {
		//Com_Printf("%f no...\n", cg.ftime);
	}

	if (revertColors) {
		VectorCopy(origColor1, ci->color1);
		VectorCopy(origColor2, ci->color2);
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( const playerState_t *ps ) {
	refEntity_t	hand;
	const centity_t	*cent;
	const clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	const weaponInfo_t	*weapon;
	float gunX;
	int fov;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if (!CG_IsFirstPersonView(ps->clientNum)) {
		//if ( cg.renderingThirdPerson ) {
		return;
	}
	if (cg.snap->ps.pm_type == PM_SPECTATOR  &&  cg.snap->ps.clientNum == cg.clientNum) {
		return;
	}


	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	gunX = cg_gun_x.value;
	if (ps->weapon == WP_GRAPPLING_HOOK) {
		gunX += 8.9;
	}

	if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
		fov = cg.demoFov;
	} else {
		fov = cg_fov.integer;
	}
	// drop gun lower at higher fov
	if ( fov > 90 ) {
		fovOffset = -0.2 * ( fov - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, gunX, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {  //if (0) {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	float x, y, w;
	const char	*name;
	float	*color;
	int weaponSelectTime;
	int selectedWeapon;
	const fontInfo_t *font;

	// don't display if dead
	if (!wolfcam_following  &&  cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	if (wolfcam_following  &&  cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
		return;
	}

	if (*cg_weaponBarFont.string) {
		font = &cgs.media.weaponBarFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	//if (1) {  //FIXME cvar , ql hud
	//	CG_DrawWeaponBar();
	//	return;
	//}

	if (wolfcam_following) {
		weaponSelectTime = wcg.weaponSelectTime;
		//weaponSelectTime = cg.time;
		selectedWeapon = cg_entities[wcg.clientNum].currentState.weapon;
	} else {
		weaponSelectTime = cg.weaponSelectTime;
		selectedWeapon = cg.snap->ps.weapon;
		if (!cg.demoPlayback) {
			selectedWeapon = cg.weaponSelect;
		}
	}

	if (cg_weaponBar.integer == 5) {
		weaponSelectTime = cg.time;
	}

	color = CG_FadeColor( weaponSelectTime, WEAPON_SELECT_TIME );
	//color = CG_FadeColor( cg.weaponSelectTime, 9999999);
	if ( !color ) {
		//Com_Printf("returning\n");
		return;
	}
	//Com_Printf("color: %f \n", color[3]);
	trap_R_SetColor( color );

	// showing weapon select clears pickup item display, but not the blend blob
	//FIXME wolfcam
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	if (wolfcam_following) {
		bits = (1 << cg_entities[wcg.clientNum].currentState.weapon);
	} else {
		bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	}

	count = 0;
	for ( i = 1 ; i < 16 ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	x = 320 - count * 20;
	y = 380;
	if (cg_weaponBarX.string[0] != '\0') {
		x = cg_weaponBarX.value;
	}
	if (cg_weaponBarY.string[0] != '\0') {
		y = cg_weaponBarY.value;
	}

	for ( i = 1 ; i < 16 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeapon( i );

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == selectedWeapon ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !wolfcam_following  &&  !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		}

		x += 40;
	}

	// draw the selected name
	if ( cg_weapons[ selectedWeapon ].item ) {
		float scale = 0.4;

		name = cg_weapons[ selectedWeapon ].item->pickup_name;
		if ( name ) {
			//w = CG_DrawStrlen( name, &cgs.media.bigchar );
			//x = ( SCREEN_WIDTH - w ) / 2;
			//CG_DrawBigStringColor(x, y - 22, name, color);
			w = CG_Text_Width(name, scale, 0, font);
			x = 320 - w / 2;
			CG_Text_Paint_Bottom(x, y - 22, scale, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, font);
		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if ( !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == 16 ) {
			cg.weaponSelect = 0;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = 15;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > 15 ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = 15 ; i > 0 ; i-- ) {
		if ( CG_WeaponSelectable( i ) ) {
			cg.weaponSelect = i;
			break;
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent ) {
	const entityState_t *ent;
	int				c;
	const weaponInfo_t	*weap;
	qboolean player = qtrue;  // can have world/map weapons

	ent = &cent->currentState;
	if (ent->number >= MAX_CLIENTS) {
		player = qfalse;
	}

	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	if (player) {
		if (ent->weapon == WP_SHOTGUN) {
			wclients[ent->clientNum].wstats[ent->weapon].atts += 11;
			wclients[ent->clientNum].perKillwstats[ent->weapon].atts += 11;
		} else {
			wclients[ent->clientNum].wstats[ent->weapon].atts++;
			wclients[ent->clientNum].perKillwstats[ent->weapon].atts++;
		}
	}

	if ((ent->weapon == WP_MACHINEGUN  ||  ent->weapon == WP_CHAINGUN  ||  ent->weapon == WP_HEAVY_MACHINEGUN)  &&  EffectScripts.weapons[ent->weapon].hasTrailScript) {
		vec3_t start, end;
		vec3_t forward;
		trace_t tr;

		if (!player) {
			CG_Printf("^3FIXME CG_FireWeapon() mg or cg with trail script and non-player fired weapon...  need angles for muzzle point\n");
		}

		// see if player origin is even available
		if (!Wolfcam_CalcMuzzlePoint(ent->clientNum, start, forward, NULL, NULL, qtrue)) {
			VectorCopy(cent->lerpOrigin, start);
			AngleVectors (cent->lerpAngles, forward, NULL, NULL);
		}

		VectorMA(start, 131072 /*8192 * 1*/, forward, end);

		if (wolfcam_following  &&  ent->clientNum == wcg.clientNum  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			//FIXME if 1st person weapon added
			VectorCopy(ent->origin2, start);
			start[2] -= 4;
		} else {
			//FIXME what if player not added
			if (ent->clientNum >= 0  &&  ent->clientNum < MAX_CLIENTS  &&  wcg.inSnapshot[ent->clientNum]) {
				CG_GetWeaponFlashOrigin(ent->clientNum, start);
			} else {
				VectorCopy(ent->origin2, start);
			}
		}

		Wolfcam_WeaponTrace(&tr, start, NULL, NULL, end, ent->clientNum, CONTENTS_SOLID | CONTENTS_BODY);
		VectorCopy(tr.endpos, end);

		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		VectorCopy(start, ScriptVars.origin);
		VectorCopy(end, ScriptVars.end);
		VectorSubtract(end, start, ScriptVars.dir);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

		//FIXME these vars used by everyone
		VectorCopy(cent->lastTrailIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastTrailIntervalTime;
		VectorCopy(cent->lastTrailDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastTrailDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[ent->weapon].trailScript, NULL);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTrailIntervalPosition);
		cent->lastTrailIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastTrailDistancePosition);
		cent->lastTrailDistanceTime = ScriptVars.lastDistanceTime;
		cent->trailTime = cg.time;
	}

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	//Com_Printf("ent %d (%d)  muzzleflash\n", cent - cg_entities, cent->currentState.number);
	cent->pe.muzzleFlashTime = cg.time;
	if (!cg.demoPlayback  &&  cent == &cg_entities[cg.snap->ps.clientNum]) {
	//if (!cg.demoPlayback) { //  &&  cent == &cg.predictedPlayerEntity) {
	//if (cent == &cg.predictedPlayerEntity) {
		cg.predictedPlayerEntity.pe.muzzleFlashTime = cg.time;
		//cg_entities[cg.predictedPlayerEntity.currentState.clientNum].pe.muzzleFlashTime = cg.time;
		//Com_Printf("pred %d  %p %p\n", cent == &cg.predictedPlayerEntity, cent, &cg.predictedPlayerEntity);
	}
	//Com_Printf("muzzleflash for %d (%d)\n", cent - cg_entities, cent->currentState.number);

	// lightning gun only does this this on initial press
	if (ent->weapon == WP_LIGHTNING) {
		if (!player) {
			CG_Printf("^3FIXME CG_FireWeapon() lg not a player\n");
		}
		if (cent->pe.lightningFiring) {
			//Com_Printf("lightning firing for %d (%s) returning\n", ent->number, cgs.clientinfo[ent->number].name);
			return;
		} else {
			cent->pe.lightningFiring = qtrue;
		}
	}

	// play quad sound if needed
	if (cg_quadFireSound.integer  &&   cent->currentState.powerups & ( 1 << PW_QUAD )) {
		// limit quad sound for rapid fire weapons
		if (ent->weapon == WP_MACHINEGUN  ||  ent->weapon == WP_HEAVY_MACHINEGUN  ||  ent->weapon == WP_CHAINGUN  ||  ent->weapon == WP_PLASMAGUN) {
			if (cg.time - cent->pe.lastQuadSoundTime >= cg_quadSoundRate.integer) {
				CG_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound);
				cent->pe.lastQuadSoundTime = cg.time;
			}
		} else {
			CG_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound);
		}
	}

	//FIXME duplicate code
	// have to check for player, if world/map weapon fire in ql might
	// not have angles to calculate weapon point
	if (*EffectScripts.weapons[ent->weapon].fireScript) {  //  &&  player) {
		vec3_t start, end;
		vec3_t forward;
		trace_t tr;

		//Com_Printf("fire script:  client %d  weapon %d\n", ent->clientNum, ent->weapon);

		if (player) {
			if (!Wolfcam_CalcMuzzlePoint(ent->clientNum, start, forward, NULL, NULL, qtrue)) {
				CG_Printf("wtf.....\n");
				VectorCopy(cent->lerpOrigin, start);
				AngleVectors (cent->lerpAngles, forward, NULL, NULL);
			}

			VectorMA(start, 131072 /*8192 * 1*/, forward, end);

			if (wolfcam_following  &&  ent->clientNum == wcg.clientNum  &&  wcg.clientNum != cg.snap->ps.clientNum) {
				//FIXME if 1st person weapon added
				VectorCopy(ent->origin2, start);
				start[2] -= 4;
			} else {
				//FIXME what if player not added
				if (ent->clientNum >= 0  &&  ent->clientNum < MAX_CLIENTS  &&  wcg.inSnapshot[ent->clientNum]) {
					CG_GetWeaponFlashOrigin(ent->clientNum, start);
				} else {
					VectorCopy(ent->origin2, start);
				}
			}

			Wolfcam_WeaponTrace(&tr, start, NULL, NULL, end, ent->clientNum, CONTENTS_SOLID | CONTENTS_BODY);
			VectorCopy(tr.endpos, end);
		} else {
			VectorCopy(ent->pos.trBase, start);
			VectorCopy(ent->pos.trBase, end);
		}

		CG_ResetScriptVars();

		if (player) {
			CG_CopyPlayerDataToScriptData(cent);
		} else {
			VectorSet(ScriptVars.color1, 1, 1, 1);
			VectorSet(ScriptVars.color2, 1, 1, 1);
			ScriptVars.clientNum = -2;
			ScriptVars.team = 0;
			ScriptVars.enemy = qtrue;
			ScriptVars.teamMate = qfalse;
			ScriptVars.inEyes = qfalse;
		}

		VectorCopy(start, ScriptVars.origin);
		VectorCopy(end, ScriptVars.end);
		VectorSubtract(end, start, ScriptVars.dir);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

		VectorCopy(cent->lerpAngles, ScriptVars.parentAngles);
		CG_RunQ3mmeScript(EffectScripts.weapons[ent->weapon].fireScript, NULL);
		return;
	}

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			if (cent->currentState.weapon == WP_LIGHTNING) {
				if (!player) {
					CG_Printf("^3FIXME CG_WeaponFire() lg flash for non-player\n");
				}
				cent->pe.lgSoundTime = cg.time;
			}
			CG_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
			//CG_StartSound(cent->lerpOrigin, ent->number, CHAN_WEAPON, weap->flashSound[c]);
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		if (!player) {
			CG_Printf("^3FIXME CG_WeaponFire()  brass function for non-player..  need angles\n");
		}
		weap->ejectBrassFunc( cent );
	}
}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
//FIXME changes dir
void CG_MissileHitWall( int weapon, int clientNum, const vec3_t origin, const vec3_t dirx, impactSound_t soundType, qboolean knowClientNum ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec3_t			sprOrg;
	vec3_t			sprVel;
	qboolean energy = qfalse;
	//weaponInfo_t *wi;
	int i;
	centity_t *cent;
	//qboolean clientValid;
	vec3_t velocity;
	projectile_t *proj;
	qboolean found;
	int foundNum;
	vec3_t missileOrigin;
	vec3_t foundOrigin;
	vec3_t tmpVector;
	vec3_t muzzlePoint;
	vec3_t ourDir;

	//Com_Printf("clientNum: %d  weapon %d  %s\n", clientNum, weapon, weapNamesCasual[weapon]);
	//Com_Printf("CG_MissileHitWall() dirx %f %f %f\n", dirx[0], dirx[1], dirx[2]);

	if (SC_Cvar_Get_Int("r_debugMarkSurface")) {
		Com_Printf("mark origin: %f %f %f\n", origin[0], origin[1], origin[2]);
	}

	//FIXME check usage
	VectorCopy(dirx, ourDir);

	//FIXME ql bug hack
	if (weapon == 0) {
		weapon = WP_LIGHTNING;
		//Com_Printf("lg...\n");
	}

	//VectorClear(velocity);
	VectorCopy(ourDir, velocity);

	switch (weapon) {
	case WP_ROCKET_LAUNCHER:
	case WP_BFG:
	case WP_PLASMAGUN:
	case WP_GRENADE_LAUNCHER:
	case WP_NAILGUN:
	case WP_PROX_LAUNCHER:
		//VectorCopy(ourDir, velocity);

		found = qfalse;
		foundNum = 0;

		for (i = 0;  i < cg.numProjectiles;  i++) {
			proj = &cg.projectiles[i];

			if (proj->weapon != weapon) {
				continue;
			}
			if (proj->numUses) {
				continue;
			}

			BG_EvaluateTrajectory(&proj->pos, cg.snap->serverTime, missileOrigin);
			if (!found) {
				found = qtrue;
				foundNum = i;
				VectorCopy(missileOrigin, foundOrigin);
				continue;
			}

			if (Distance(origin, missileOrigin) < Distance(origin, foundOrigin)) {
				foundNum = i;
				VectorCopy(missileOrigin, foundOrigin);
				continue;
			}
		}

		if (found) {
			cg.projectiles[foundNum].numUses++;
			VectorCopy(cg.projectiles[foundNum].pos.trDelta, tmpVector);
			if (VectorLength(tmpVector) <= 10.0) {
				VectorCopy(ourDir, velocity);
			} else {
				//VectorScale(tmpVector, -1, tmpVector);
				//RotatePointAroundVector(velocity, ourDir, tmpVector, 180.0);
				VectorReflect(tmpVector, ourDir, velocity);
				//Com_Printf("rotating\n");
				//VectorCopy(tmpVector, velocity);
			}
			if (weapon == WP_PROX_LAUNCHER) {
				AngleVectors(cg.projectiles[foundNum].angles, NULL, NULL, ourDir);
				VectorCopy(ourDir, velocity);
				VectorNormalize(ourDir);
				//Com_Printf("point contents: 0x%x\n", CG_PointContents(cg.projectiles[foundNum].pos.trBase, -1));
			}
			VectorNormalize(velocity);
			//Com_Printf("got velocity  %f %f %f distance %f  dir %f %f %f\n", velocity[0], velocity[1], velocity[2], Distance(origin, foundOrigin), ourDir[0], ourDir[1], ourDir[2]);
		}

		break;
	case WP_MACHINEGUN:
	case WP_CHAINGUN:
	case WP_RAILGUN:
	case WP_SHOTGUN:
		if (knowClientNum) {
			// clientNum might not be present in snapshot
			if (Wolfcam_CalcMuzzlePoint(clientNum, muzzlePoint, NULL, NULL, NULL, qfalse)) {
				VectorSubtract(origin, muzzlePoint, tmpVector);

				//VectorCheck(origin);
				//VectorCheck(ourDir);

				VectorReflect(tmpVector, ourDir, velocity);
				VectorNormalize(velocity);
			} else {
				VectorCopy(ourDir, velocity);
			}
		}
		break;
	default:
		break;
	}

	//clientValid = qfalse;
	cent = NULL;

	if (clientNum >= 0  &&  clientNum < MAX_CLIENTS) {
		//Com_Printf("...\n");
		cent = &cg_entities[clientNum];
		if (clientNum == cg.snap->ps.clientNum  ||  cent->currentValid) {
			//Com_Printf("valid\n");
			//clientValid = qtrue;
			if (cent->currentState.weapon == WP_CHAINGUN) {
				weapon = WP_CHAINGUN;
			}
		}
	}

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		clientNum = 0;
	}

	if (weapon < 0  ||  weapon >= WP_NUM_WEAPONS) {
		weapon = 0;
	}

	//wi = &cg_weapons[weapon];
	if (EffectScripts.weapons[weapon].hasImpactScript  &&  weapon != WP_LIGHTNING) {
		trace_t trace;
		vec3_t ourOrigin;

		VectorCopy(origin, ourOrigin);

		for (i = 0;  i < 4;  i++) {
			Wolfcam_WeaponTrace(&trace, ourOrigin, vec3_origin, vec3_origin, ourOrigin, clientNum, MASK_SHOT);
			//Com_Printf("contents: 0x%x\n", CG_PointContents(origin, -1));

			if (!(CG_PointContents(ourOrigin, -1) & CONTENTS_SOLID)) {
				break;
			}
			VectorMA(ourOrigin, 1, ourDir, ourOrigin);
			//Com_Printf("nudge %d\n", i);
		}

		//CG_Printf("missilehitwall() impact script clientNum %d contents: 0x%x\n", clientNum, CG_PointContents(ourOrigin, -1));
		//Com_Printf("dir:  %f %f %f  weapon %d\n", ourDir[0], ourDir[1], ourDir[2], weapon);

		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(&cg_entities[clientNum]);
		if (trace.surfaceFlags & SURF_METALSTEPS) {
			ScriptVars.surfaceType = 1;
		} else if (trace.surfaceFlags & SURF_WOOD) {
			ScriptVars.surfaceType = 2;
		}
		VectorCopy(ourOrigin, ScriptVars.origin);
		VectorCopy(ourDir, ScriptVars.dir);
		VectorCopy(velocity, ScriptVars.velocity);
		//CG_Printf("impact %s\n", weapNamesCasual[weapon]);
		CG_RunQ3mmeScript((char *)EffectScripts.weapons[weapon].impactScript, NULL);
		return;
	}

	if (weapon == WP_SHOTGUN  &&  cg_shotgunMarks.integer == 0) {
		return;
	}

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
#if 1  //def MPACK
	case WP_NAILGUN:
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_nghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_nghitmetal;
		} else {
			sfx = cgs.media.sfx_nghit;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#endif
		//case WP_NONE:
	case WP_LIGHTNING:
		//FIXME weapon wrong quakelive  -- WP_NONE being transmitted for lg
		//Com_Printf("lg...\n");
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#if 1  //def MPACK
	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
#endif
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		if (cg_oldRocket.integer == 0) {
			// explosion sprite animation
			VectorMA( origin, 24, ourDir, sprOrg );
			VectorScale( ourDir, 64, sprVel );

			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 20, 30 );
		}

#if 0  //FIXME force
		//FIXME testing
		{
			fxExternalForce_t force;

			memset(&force, 0, sizeof(force));

			force.startTime = cg.time;
			force.duration = 0;
			VectorCopy(origin, force.origin);
			force.radial = qtrue;
			force.power = 100;
			force.valid = qtrue;

			CG_AddFxExternalForce(&force);
		}
#endif
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		energy = qtrue;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		energy = qtrue;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

#if 1  //def MPACK
	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_chghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_chghitmetal;
		} else {
			sfx = cgs.media.sfx_chghit;
		}
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		// 2017-07-09 this rand snd fx code is taken out in ioquake3 patch 2104
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
#endif

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;

	case WP_HEAVY_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;

	}

	if (EffectScripts.weapons[weapon].hasImpactScript  &&  weapon == WP_LIGHTNING) {
		return;
	}

	if ( sfx ) {
		if (weapon == WP_LIGHTNING  ||  weapon == WP_NONE) {   // WP_NONE quakelive bug?
			//Com_Printf("lg %d\n", clientNum);
			//CG_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
		} else {
			//Com_Printf("missile hit wall sound %d\n", weapon);
			CG_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
		}
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, ourDir,
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
		if ( weapon == WP_RAILGUN ) {
			// colorize with client color
			VectorCopy( cgs.clientinfo[clientNum].color1, le->color );
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if ( weapon == WP_RAILGUN ) {
		float	*color;

		//FIXME enemy colors
		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		if (cg_railUseOwnColors.integer  &&  CG_IsUs(&cgs.clientinfo[clientNum])) {
			color = cg.color1;
		}
		CG_ImpactMark( mark, origin, ourDir, random()*360, color[0],color[1], color[2],1, alphaFade, radius, qfalse, energy, qtrue );
	} else {
		CG_ImpactMark( mark, origin, ourDir, random()*360, 1,1,1,1, alphaFade, radius, qfalse, energy, qtrue );
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, const vec3_t origin, const vec3_t dir, int entityNum, int shooterClientNum )
{
	//weaponInfo_t *wi;
	int n;

	//Com_Printf("mishitpl  %d -> %d\n", shooterClientNum, entityNum);

	// shooterClientNum == -1  if shooter not known

	n = entityNum;
	if (n < 0  ||  n >= MAX_CLIENTS) {
		n = 0;
	}

	//wi = &cg_weapons[weapon];
	if (EffectScripts.weapons[weapon].hasImpactFleshScript) {
		//CG_Printf("CG_MissileHitPlayer() person shot %d\n", entityNum);
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(&cg_entities[n]);
		VectorCopy(origin, ScriptVars.origin);
		VectorCopy(dir, ScriptVars.dir);
		//Com_Printf("fx missile hit player %s\n", weapNamesCasual[weapon]);
		VectorSet(ScriptVars.end, 0, 0, 0);
		if (CG_ClientInSnapshot(shooterClientNum)) {
			VectorSubtract(cg_entities[shooterClientNum].lerpOrigin, origin, ScriptVars.end);
			//Com_Printf("^2%d valid end\n", shooterClientNum);
		} else {
			//Com_Printf("^1%d not valid end %s\n", shooterClientNum,  cgs.clientinfo[shooterClientNum].name);
		}
		CG_RunQ3mmeScript((char *)EffectScripts.weapons[weapon].impactFleshScript, NULL);
		return;
	} else if (*EffectScripts.impactFlesh) {
		//Com_Printf("impact flesh\n");
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(&cg_entities[n]);
		VectorCopy(origin, ScriptVars.origin);
		VectorCopy(dir, ScriptVars.dir);
		VectorSet(ScriptVars.end, 0, 0, 0);
		if (CG_ClientInSnapshot(shooterClientNum)) {
			VectorSubtract(cg_entities[shooterClientNum].lerpOrigin, origin, ScriptVars.end);
			//Com_Printf("^2%d valid end\n", shooterClientNum);
		} else {
			//Com_Printf("^1%d not valid end %s\n", shooterClientNum,  cgs.clientinfo[shooterClientNum].name);
		}
		CG_RunQ3mmeScript((char *)EffectScripts.impactFlesh, NULL);
		return;
	}

	if (weapon == WP_SHOTGUN) {
		if (shooterClientNum == cg.snap->ps.clientNum) {
			//Com_Printf("%d CG_MissileHitPlayer, hits: %d\n", cg.time, cg.shotgunHits);
		}
	}

	if (weapon == WP_SHOTGUN  &&  cg_shotgunImpactSparks.integer == 0) {
		return;
	}

	CG_Bleed( origin, entityNum );
	CG_ImpactSparks(weapon, origin, dir, entityNum);

	//Com_Printf("missile hit player %s\n", weapNamesCasual[weapon]);


	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	//case WP_PLASMAGUN:   // skipping... distracting
	case WP_BFG:
#if 1  //def MPACK
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:
#endif
		//CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		CG_MissileHitWall( weapon, shooterClientNum, origin, dir, IMPACTSOUND_FLESH, qtrue );
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet (const vec3_t start, const vec3_t end, int skipNum)
{
	trace_t		tr;
    trace_t     wtr;
	int sourceContentType, destContentType;

    Wolfcam_WeaponTrace(&wtr, start, NULL, NULL, end, skipNum, CONTENTS_SOLID | CONTENTS_BODY);
    //Wolfcam_WeaponTrace(&wtr, start, NULL, NULL, end, skipNum, MASK_SHOT);
    if (wtr.entityNum < MAX_CLIENTS  &&  cg_entities[wtr.entityNum].currentState.eType != ET_PLAYER) {
        //CG_Printf ("          ^2SHOTGUN: ^7<MAX_CLIENTS && != ET_PLAYER  %d -> %d\n", skipNum, wtr.entityNum);
    }
	if ( cg_entities[wtr.entityNum].currentState.eType == ET_PLAYER ) {
		//FIXME broken
        //wclients[skipNum].wstats[WP_SHOTGUN].hits++;
		CG_MissileHitPlayer( WP_SHOTGUN, wtr.endpos, wtr.plane.normal, wtr.entityNum, skipNum );
        if (wolfcam_following  &&  skipNum == wcg.clientNum) {
            //CG_StartLocalSound (cgs.media.hitSound, CHAN_LOCAL_SOUND);
            //wcg.playHitSound = qtrue;  // shotgun just too broken
        }
    }

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	if (EffectScripts.weapons[WP_SHOTGUN].hasTrailScript  &&  (skipNum >= 0  &&  skipNum < MAX_CLIENTS)) {
		centity_t *cent;
		const entityState_t *es;
		vec3_t newStart;

		cent = &cg_entities[skipNum];
		es = &cent->currentState;

		VectorCopy(start, newStart);
		if (wolfcam_following  &&  es->clientNum == wcg.clientNum  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			//FIXME if 1st person weapon added
			VectorCopy(es->origin2, newStart);
			newStart[2] -= 4;
		} else {
			//FIXME what if player not added
			if (es->clientNum >= 0  &&  es->clientNum < MAX_CLIENTS  &&  wcg.inSnapshot[es->clientNum]) {
				CG_GetWeaponFlashOrigin(es->clientNum, newStart);
			} else {
				//Com_Printf("^3  ---- client %d  skipNum %d\n", es->clientNum, skipNum);
				//VectorCopy(es->origin2, newStart);
				// just keep start as origin
			}
		}

#if 0
		if (VectorLength(newStart) <= 0.0) {
			CG_Printf("^1fx shotgun newStart == 0   origin2 %f %f %f\n", es->origin2[0], es->origin2[1], es->origin2[2]);
			CG_Abort();
		}
#endif

		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		VectorCopy(newStart, ScriptVars.origin);
		VectorCopy(end, ScriptVars.end);
		VectorSubtract(end, newStart, ScriptVars.dir);

		VectorCopy(cent->lerpOrigin, ScriptVars.parentOrigin);
		VectorCopy(ScriptVars.dir, ScriptVars.parentDir);

		//FIXME these vars used by everyone
		VectorCopy(cent->lastTrailIntervalPosition, ScriptVars.lastIntervalPosition);
		ScriptVars.lastIntervalTime = cent->lastTrailIntervalTime;
		VectorCopy(cent->lastTrailDistancePosition, ScriptVars.lastDistancePosition);
		ScriptVars.lastDistanceTime = cent->lastTrailDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.weapons[WP_SHOTGUN].trailScript, NULL);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTrailIntervalPosition);
		cent->lastTrailIntervalTime = ScriptVars.lastIntervalTime;
		VectorCopy(ScriptVars.lastDistancePosition, cent->lastTrailDistancePosition);
		cent->lastTrailDistanceTime = ScriptVars.lastDistanceTime;
		cent->trailTime = cg.time;
	}

	sourceContentType = CG_PointContents( start, 0 );
	destContentType = CG_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	//Com_Printf("shotgun surfaceFlags  0x%x\n", tr.surfaceFlags);
	if (  tr.surfaceFlags & SURF_NOIMPACT ) { // ||  tr.fraction == 1.0) {
		return;
	}

#if 0
	if (tr.fraction == 1.0) {
		localEntity_t *le;
		refEntity_t *re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.ftime;
		le->endTime = cg.ftime + 10000.0;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 20;
		re->customShader = cgs.media.balloonShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
		le->color[3] = 1.0;
		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy(start, le->pos.trBase);

		Com_Printf("^3start %f %f %f   player origin %f %f %f\n", start[0], start[1], start[2], cg_entities[skipNum].lerpOrigin[0], cg_entities[skipNum].lerpOrigin[1], cg_entities[skipNum].lerpOrigin[2]);

		CG_SpawnEffect(start);
		return;
	}
#endif

	//cg_entities[wtr.entityNum].currentState.eType == ET_PLAYER
	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		//CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum );
	} else {
		//Com_Printf("shotgun pellet missed eType == %d\n", cg_entities[tr.entityNum].currentState.eType);
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if (cg_entities[wtr.entityNum].currentState.eType == ET_PLAYER) {
			//CG_Printf("wtf....... ^1slkdjflskdfj\n");
			//return;
		}

		//Com_Printf("skip %d\n", skipNum);

		//Com_Printf(" pellet trace endpos %f %f %f  normal %f %f %f  fraction %f  startsolid %d  allsolid %d\n", tr.endpos[0], tr.endpos[1], tr.endpos[2], tr.plane.normal[0], tr.plane.normal[1], tr.plane.normal[2], tr.fraction, tr.startsolid, tr.allsolid);
		//Com_Printf("wpellet trace endpos %f %f %f  normal %f %f %f\n", wtr.endpos[0], wtr.endpos[1], wtr.endpos[2], wtr.plane.normal[0], wtr.plane.normal[1], wtr.plane.normal[2]);

		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, skipNum, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL, qtrue );
		} else {
			CG_MissileHitWall( WP_SHOTGUN, skipNum, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT, qtrue );
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern (const vec3_t origin, const vec3_t origin2, int seed, int otherEntNum)
{
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	//int hits;
	//int target;
	//vec3_t offsets;
	//vec_t distance;
	const centity_t *cent;

	cent = &cg_entities[otherEntNum];
	//Com_Printf("^5shotgun pattern origin %f %f %f\n", origin[0], origin[1], origin[2]);
	//Com_Printf("%d:  origin %f %f %f\n", otherEntNum, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2]);
	//CG_PrintEntityState(otherEntNum);

	// QuakeLive: using player angles instead of the provided forward vector
	// since it appears to be wrong

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information

	{



		//Com_Printf("angle forward %f %f %f (%f %f %f)\n", f[0], f[1], f[2], forward[0], forward[1], forward[2]);
	}

	//VectorNormalize2( origin2, forward );
	AngleVectors(cent->lerpAngles, forward, NULL, NULL);
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );


	//FIXME
	if (0) {  //(cg_shotgunImpactSparks.integer == 2  &&  otherEntNum == cg.snap->ps.clientNum) {
#if 0
		//Com_Printf("own showgun\n");
		target = find_best_target(otherEntNum, qfalse, &distance, offsets);
		if (target == -1) {
			//Com_Printf("couldn't find best target\n");
		}
		// generate the "random" spread pattern
		for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
			r = Q_crandom( &seed ) * 32;
			u = Q_crandom( &seed ) * 32;
			VectorMA( origin, 8192 * 16, forward, end);
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);

			CG_ShotgunPellet( origin, end, otherEntNum );
		}
#endif
	} else if (cg_shotgunStyle.integer == 0) {  //(0) {  //(cg_trueShotgun.integer == 0) {
		// quake3
		// generate the "random" spread pattern
		for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
			r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
			u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
			VectorMA( origin, 8192 * 16, forward, end);
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);

			CG_ShotgunPellet( origin, end, otherEntNum );
		}
	} else {  // cg_trueShotgun.integer > 0
		// quakelive
#if 0
		float angles[20][2] = {
			// inner ring
			{ 0, 0.0341 }, { 0.0307, 0.0171 }, { 0.0307, -0.0171 }, { 0, -0.0341 }, { -0.0307, -0.0171 }, { -0.0307, 0.0171 },
			// middle ring
			{ 0.0443, 0.0546 }, { 0.0716, -0.0102 }, { 0.0239, -0.0682 }, { -0.0511, -0.0580 }, { -0.0716, 0.0102 }, { -0.0273, 0.0682 },
			// outer ring
			{ 0, 0.1122 }, { 0.0749, 0.0749 }, { 0.1122, 0 }, { 0.0749, -0.0749 }, { 0, -0.1122 }, { -0.0749, -0.0749 }, { -0.1122, 0 }, { -0.0749, 0.0749 }
		};
#endif
		for (i = 0;  i < 20;  i++) {
			vec3_t newForward;
			vec3_t tmp;

			//r = RAD2DEG(angles[i][0]);
			//u = RAD2DEG(angles[i][1]);
			r = cg.shotgunPattern[i][0];
			u = cg.shotgunPattern[i][1];
			//r = RAD2DEG(cg.shotgunPattern[i][0]);
			//u = RAD2DEG(cg.shotgunPattern[i][1]);
			//Com_Printf("%f  %f\n", cg.shotgunPattern[i][0] - angles[i][0], cg.shotgunPattern[i][1] - angles[i][1]);
			//Com_Printf("(%f  %f)  (%f  %f)\n", cg.shotgunPattern[i][0], cg.shotgunPattern[i][1], angles[i][0], angles[i][1]);
			//Com_Printf("(%f  %f)  (%f  %f)\n", RAD2DEG(cg.shotgunPattern[i][0]), RAD2DEG(cg.shotgunPattern[i][1]), RAD2DEG(angles[i][0]), RAD2DEG(angles[i][1]));
			if (cg_shotgunStyle.integer > 1) {
				r += crandom() * cg_shotgunRandomness.value;
				u += crandom() * cg_shotgunRandomness.value;
			}
			VectorCopy(forward, newForward);

			RotatePointAroundVector(tmp, up, newForward, r);
			VectorCopy(tmp, newForward);
			VectorNormalize(newForward);
			RotatePointAroundVector(tmp, right, newForward, u);
			VectorCopy(tmp, newForward);
			VectorNormalize(newForward);

			VectorMA(origin, 8192 * 16, newForward, end);
			//Com_Printf("----------- pellet %d  %f %f %f\n", i, origin[0], origin[1], origin[2]);
			CG_ShotgunPellet(origin, end, otherEntNum);
		}
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( const entityState_t *es ) {
	vec3_t	v;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t			up;

		contents = CG_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			if (cg_smokeRadius_SG.integer > 0) {
				VectorSet( up, 0, 0, 8 );
				CG_SmokePuff( v, up, cg_smokeRadius_SG.integer, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
			}
		}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
static void CG_Tracer( const vec3_t source, const vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts, qfalse );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	CG_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );
}

void CG_GetWeaponFlashOrigin (int clientNum, vec3_t startPoint)
{
	float fovOffset;
	refEntity_t hand;
	refEntity_t head;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t gun;
	refEntity_t flash;
	vec3_t angles;
	centity_t *cent;
	const clientInfo_t *ci;
	const weaponInfo_t *weapon;
	float gunX;
	float fov;

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return;
	}

	//Com_Printf("CG_GetWeaponFlashOrigin() %d\n", clientNum);

	//weapon = &cg_weapons[WP_RAILGUN];

	if (clientNum == cg.snap->ps.clientNum) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[clientNum];
	}

	ci = &cgs.clientinfo[clientNum];
	weapon = &cg_weapons[cent->currentState.weapon];

	if (CG_IsFirstPersonView(clientNum)) {
		if (wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			//FIXME if view weapon gets added
			return;
		}

		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.integer;
		}
		// from hand
		// drop gun lower at higher fov
        if ( fov > 90 ) {
			fovOffset = -0.2 * ( fov - 90 );
        } else {
			fovOffset = 0;
        }
		// hand
		memset(&hand, 0, sizeof(hand));
		// set up gun position
		gunX = cg_gun_x.value;
		if (cent->currentState.weapon == WP_GRAPPLING_HOOK) {
			gunX += 8.9;
		}
        CG_CalculateWeaponPosition(hand.origin, angles);
		VectorMA(hand.origin, gunX, cg.refdef.viewaxis[0], hand.origin);
        VectorMA(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
        VectorMA(hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin);

        AnglesToAxis(angles, hand.axis);
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
		hand.hModel = weapon->handsModel;

		memset( &gun, 0, sizeof( gun ) );
		gun.hModel = weapon->weaponModel;
		CG_PositionEntityOnTag(&gun, &hand, hand.hModel, "tag_weapon");
		CG_ScaleModel(&gun, cg_gunSize.value);
	} else {
		// torso
		memset( &legs, 0, sizeof(legs) );
        memset( &torso, 0, sizeof(torso) );
        memset( &head, 0, sizeof(head) );

		if (clientNum == cg.snap->ps.clientNum) {
			cent = &cg.predictedPlayerEntity;
		}
		CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
		CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp, &torso.oldframe, &torso.frame, &torso.backlerp );
		legs.hModel = ci->legsModel;
		VectorCopy( cent->lerpOrigin, legs.origin );
		torso.hModel = ci->torsoModel;
		CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");
		memset( &gun, 0, sizeof( gun ) );
		gun.hModel = weapon->weaponModel;
		CG_PositionEntityOnTag(&gun, &torso, torso.hModel, "tag_weapon");
		CG_ScaleModel(&gun, cg_gunSizeThirdPerson.value);
	}

	memset( &flash, 0, sizeof( flash ) );
	flash.hModel = weapon->flashModel;
	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	VectorCopy(flash.origin, startPoint);
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	const centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( const vec3_t end, int sourceEntityNum, const vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;
	qboolean knowClientNum = qfalse;
	qboolean hasMuzzlePoint = qfalse;
	int weapon = -1;
	const centity_t *cent;
	const entityState_t *es;

	//Com_Printf("CG_Bullet %d\n", sourceEntityNum);

	if (sourceEntityNum >= 0  &&  sourceEntityNum < MAX_CLIENTS) {
		knowClientNum = qtrue;
	}

	if (knowClientNum) {
		hasMuzzlePoint = CG_CalcMuzzlePoint(sourceEntityNum, start);
		//hasMuzzlePoint = Wolfcam_CalcMuzzlePoint(sourceEntityNum, start, NULL, NULL, NULL, qtrue);
		if (sourceEntityNum == cg.snap->ps.clientNum) {
			weapon = cg.snap->ps.weapon;
		} else {
			weapon = cg_entities[sourceEntityNum].currentState.weapon;
		}
	}

	if (hasMuzzlePoint) {
		cent = &cg_entities[sourceEntityNum];
		es = &cent->currentState;

		if (wolfcam_following  &&  es->clientNum == wcg.clientNum  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			//FIXME if 1st person weapon added
			VectorCopy(es->origin2, start);
			start[2] -= 4;
		} else {
			//FIXME what if player not added
			if (es->clientNum >= 0  &&  es->clientNum < MAX_CLIENTS  &&  wcg.inSnapshot[es->clientNum]) {
				CG_GetWeaponFlashOrigin(es->clientNum, start);
			} else {
				VectorCopy(es->origin2, start);
			}
		}
	}

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 ) {  //&& cg_tracerChance.value > 0 ) {
		if (hasMuzzlePoint) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			if (cg_tracerChance.value > 0) {
				// do a complete bubble trail if necessary
				if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
					CG_BubbleTrail( start, end, 32 );
				}
				// bubble trail from water into air
				else if ( ( sourceContentType & CONTENTS_WATER ) ) {
					trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
					CG_BubbleTrail( start, trace.endpos, 32 );
				}
				// bubble trail from air into water
				else if ( ( destContentType & CONTENTS_WATER ) ) {
					trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
					CG_BubbleTrail( trace.endpos, end, 32 );
				}
			}

			// draw a tracer


			if ((weapon == WP_MACHINEGUN  ||  weapon == WP_CHAINGUN  ||  weapon == WP_HEAVY_MACHINEGUN)  &&  EffectScripts.weapons[weapon].hasTrailScript) {
				// pass
			} else {
				if ( random() < cg_tracerChance.value ) {
					CG_Tracer( start, end );
				}
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_MACHINEGUN, sourceEntityNum, end, normal, IMPACTSOUND_DEFAULT, knowClientNum );
	}

}

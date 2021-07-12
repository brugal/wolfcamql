#include "cg_local.h"

#include "cg_players.h"
#include "cg_effects.h"
#include "cg_ents.h"
#include "cg_main.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_weapons.h"

#include "wolfcam_local.h"

qboolean Wolfcam_CalcMuzzlePoint (int entityNum, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, qboolean useLerp)
{
    vec3_t f;
    const centity_t *cent;
    int anim;

    cent = &cg_entities[entityNum];

    if (entityNum == cg.snap->ps.clientNum) {
        if (useLerp) {
            VectorCopy (cent->lerpOrigin, muzzle);
            muzzle[2] += cg.snap->ps.viewheight;
            AngleVectors (cent->lerpAngles, f, right, up);
            if (forward) {
                VectorCopy (f, forward);
            }
            VectorMA (muzzle, 14, f, muzzle);
            return qtrue;
        } else {
            VectorCopy (cg.snap->ps.origin, muzzle);
            muzzle[2] += cg.snap->ps.viewheight;
            AngleVectors (cg.snap->ps.viewangles, f, right, up);
            if (forward) {
                VectorCopy (f, forward);
            }
            VectorMA (muzzle, 14, f, muzzle);
            return qtrue;
        }
    }

    if (!cent->currentValid) {
        //CG_Printf("^3Wolfcam_CalcMuzzlePoint() invalid cent %d\n", entityNum);
        return qfalse;
    }

    if (useLerp) {
        VectorCopy (cent->lerpOrigin, muzzle);
        AngleVectors (cent->lerpAngles, f, right, up);
    } else {
        VectorCopy( cent->currentState.pos.trBase, muzzle );
        AngleVectors (cent->currentState.apos.trBase, f, right, up);
    }
    if (forward) {
        VectorCopy (f, forward);
    }

    anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
    if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
        muzzle[2] += CROUCH_VIEWHEIGHT;
    } else {
        muzzle[2] += DEFAULT_VIEWHEIGHT;
    }

    VectorMA (muzzle, 14, f, muzzle);

    return qtrue;
}

static void Wolfcam_CalculateWeaponPosition (vec3_t origin, vec3_t angles)
{
	float	scale;
	int		delta;
	float	fracsin;
	int cgtime;
	float xyspeed;
	float landChange;
	int landTime;
	const wclient_t *wc;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	if (cg_drawGun.integer <= 0  ||  cg_drawGun.integer == 2  ||  cg_drawGun.integer == 3) {
		return;
	}

	wc = &wclients[wcg.clientNum];

	//FIXME stepTime

	//FIXME freeze 1st person weapon settings not needed since it isn't shown
	if (cg.freezeEntity[wcg.clientNum]) {
		cgtime = wc->freezeCgtime;
		xyspeed = wc->freezeXyspeed;
		landChange = wc->freezeLandChange;
		landTime = wc->freezeLandTime;
	} else {
		cgtime = cg.time;
		xyspeed = wc->xyspeed;
		landChange = wc->landChange;
		landTime = wc->landTime;
	}

	// on odd legs, invert some angles
	//FIXME check if this applies with non ps players
	if (wc->bobcycle & 1) {
		scale = -xyspeed;
	} else {
		scale = xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * wc->bobfracsin * 0.005;
	angles[YAW] += scale * wc->bobfracsin * 0.01;
	angles[PITCH] += xyspeed * wc->bobfracsin * 0.005;

	// drop the weapon when landing
	if (cg_fallKick.integer > 0) {
		delta = cgtime - landTime;
		if ( delta < LAND_DEFLECT_TIME ) {
			origin[2] += landChange*0.25 * delta / LAND_DEFLECT_TIME;
		} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
			origin[2] += landChange*0.25 *
				(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
		}
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

void Wolfcam_AddPlayerWeapon (const refEntity_t *parent, centity_t *cent, int team)
{
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

	if (!cent->inCurrentSnapshot) {
		// this can happen with /follow which can stay in victim position
		// (frag hover)
		return;
	}

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	weaponNum = cent->currentState.weapon;

	if (weaponNum <= WP_NONE  ||  weaponNum >= WP_NUM_WEAPONS) {
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if (0) {  //( ps ) {
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

	CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

    CG_ScaleModel(&gun, cg_gunSize.value);

	// custom weapon shaders
	{
		vmCvar_t *firstPersonShaders[MAX_WEAPONS] = { NULL, &cg_firstPersonShaderWeaponGauntlet, &cg_firstPersonShaderWeaponMachineGun, &cg_firstPersonShaderWeaponShotgun, &cg_firstPersonShaderWeaponGrenadeLauncher, &cg_firstPersonShaderWeaponRocketLauncher, &cg_firstPersonShaderWeaponLightningGun, &cg_firstPersonShaderWeaponRailGun, &cg_firstPersonShaderWeaponPlasmaGun, &cg_firstPersonShaderWeaponBFG, &cg_firstPersonShaderWeaponGrapplingHook, &cg_firstPersonShaderWeaponNailGun, &cg_firstPersonShaderWeaponProximityLauncher, &cg_firstPersonShaderWeaponChainGun, &cg_firstPersonShaderWeaponHeavyMachineGun };

		if (firstPersonShaders[weaponNum]  &&  *(firstPersonShaders[weaponNum]->string)) {
			gun.customShader = trap_R_RegisterShader(firstPersonShaders[weaponNum]->string);
		}
	}

	if (gun.hModel) {
		if (cg_drawGun.integer > 2) {
			gun.customShader = cgs.media.ghostWeaponShader;
			gun.shaderRGBA[0] = 255;
			gun.shaderRGBA[1] = 255;
			gun.shaderRGBA[2] = 255;
			gun.shaderRGBA[3] = 255;
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

        CG_ScaleModel(&barrel, cg_gunSize.value);

		if (cg_drawGun.integer > 2) {
			barrel.customShader = cgs.media.ghostWeaponShader;
			barrel.shaderRGBA[0] = 255;
			barrel.shaderRGBA[1] = 255;
			barrel.shaderRGBA[2] = 255;
			barrel.shaderRGBA[3] = 255;
		}

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

#if 0
	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
		//Com_Printf("fake player %d  ->  %d\n", nonPredictedCent - cg_entities, cent->currentState.clientNum);
	}
#endif


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

	if (0) {  //(cent == &cg.predictedPlayerEntity  &&  !cg.renderingThirdPerson  &&  !ps) {
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

	if (!cg_muzzleFlash.integer) {
		// pass
	} else {
		if (flash.hModel) {
			CG_AddRefEntity(&flash);
		}
	}

	// bolt:
	if (1) {
		// add lightning bolt
		if (1) {
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

void Wolfcam_AddViewWeapon (void)
{
    vec3_t origin;
    centity_t *cent;
    const entityState_t *es;
    refEntity_t hand;
    const clientInfo_t *ci;
    float fovOffset;
    vec3_t angles;
    const weaponInfo_t *weapon;
    float gunX;
    int fov;

    if (!wolfcam_following) {
        return;
    }

    cent = &cg_entities[wcg.clientNum];
    es = &cent->currentState;

    if (!cg_drawGun.integer) {
        if (es->eFlags & EF_FIRING  &&  es->weapon == WP_LIGHTNING) {
            // special hack for lightning gun...
            VectorCopy( cg.refdef.vieworg, origin );
            VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
            CG_LightningBolt( &cg_entities[es->number], origin );

            //FIXME is this adding the sound twice?
            CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cg_weapons[es->weapon].firingSound );
        }

        return;
    }

    // don't draw if testing a gun model
    if (cg.testGun) {
        return;
    }

    gunX = cg_gun_x.value;
    if (es->weapon == WP_GRAPPLING_HOOK) {
        gunX += 8.9;
    }

    fov = cg_fov.integer;

    //FIXME option
    // drop gun lower at higher fov
    if (fov > 90) {
        fovOffset = -0.2 * (fov - 90);
    } else {
        fovOffset = 0;
    }

    CG_RegisterWeapon(es->weapon);
    weapon = &cg_weapons[es->weapon];

    memset(&hand, 0, sizeof(hand));

    // set up gun position
    Wolfcam_CalculateWeaponPosition(hand.origin, angles);

    VectorMA( hand.origin, gunX, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	//cg_gun_frame.integer = 1;
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {  //if (0) {
		// these are just for calling CG_PlayerAnimation()
		int legsOld;
		int legs;
		float legsBackLerp;
		int torsoOld;
		int torso;
		float torsoBackLerp;

		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];

		// animations weren't run for /follow'ed player
		CG_PlayerAnimation(cent, &legsOld, &legs, &legsBackLerp, &torsoOld, &torso, &torsoBackLerp);

		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	Wolfcam_AddPlayerWeapon(&hand, cent, cgs.clientinfo[cent->currentState.clientNum].team);
}

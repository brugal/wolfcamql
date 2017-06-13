// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

#include "cg_draw.h"  // reset lagometer
#include "cg_event.h"
#include "cg_freeze.h"
#include "cg_main.h"
#include "cg_playerstate.h"
#include "cg_predict.h"
#include "cg_servercmds.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"

#include "wolfcam_local.h"

void CG_PrintPlayerState (void)
{
	const playerState_t *ps;
	int i;

	ps = &cg.snap->ps;

	Com_Printf("commandTime %d\n", ps->commandTime);
	Com_Printf("pm_type %d\n", ps->pm_type);
	Com_Printf("bobCycle %d\n", ps->bobCycle);
	Com_Printf("pm_flags 0x%x\n", ps->pm_flags);
	Com_Printf("pm_time %d\n", ps->pm_time);
	Com_Printf("origin %f %f %f\n", ps->origin[0], ps->origin[1], ps->origin[2]);
	Com_Printf("velocity %f %f %f\n", ps->velocity[0], ps->velocity[1], ps->velocity[2]);
	Com_Printf("weaponTime %d\n", ps->weaponTime);
	Com_Printf("gravity %d\n", ps->gravity);
	Com_Printf("speed %d\n", ps->speed);
	Com_Printf("delta_angles %d %d %d\n", ps->delta_angles[0], ps->delta_angles[1], ps->delta_angles[2]);
	Com_Printf("groundEntityNum %d\n", ps->groundEntityNum);
	Com_Printf("legsTimer %d\n", ps->legsTimer);
	Com_Printf("legsAnim %d  (%d)\n", ps->legsAnim, ps->legsAnim & ~ANIM_TOGGLEBIT);
	Com_Printf("torsoTimer %d\n", ps->torsoTimer);
	Com_Printf("torsoAnim %d  (%d)\n", ps->torsoAnim, ps->torsoAnim & ~ANIM_TOGGLEBIT);
	Com_Printf("movementDir %d\n", ps->movementDir);
	Com_Printf("grapplePoint %f %f %f\n", ps->grapplePoint[0], ps->grapplePoint[1], ps->grapplePoint[2]);
	Com_Printf("eFlags 0x%x\n", ps->eFlags);
	Com_Printf("eventSequence %d\n", ps->eventSequence);

	Com_Printf("events:");
	for (i = 0;  i < MAX_PS_EVENTS;  i++) {
		Com_Printf(" (%d)", ps->events[i]);
	}
	Com_Printf("\n");

	Com_Printf("eventParms:");
	for (i = 0;  i < MAX_PS_EVENTS;  i++) {
		Com_Printf(" (%d)", ps->eventParms[i]);
	}
	Com_Printf("\n");

	Com_Printf("externalEvent %d\n", ps->externalEvent);
	Com_Printf("externalEventParm %d\n", ps->externalEventParm);
	Com_Printf("externalEventTime %d\n", ps->externalEventTime);
	Com_Printf("clientNum %d\n", ps->clientNum);
	Com_Printf("weapon %d\n", ps->weapon);
	Com_Printf("weaponstate %d\n", ps->weaponstate);
	Com_Printf("viewangles %f %f %f\n", ps->viewangles[0], ps->viewangles[1], ps->viewangles[2]);
	Com_Printf("viewheight %d\n", ps->viewheight);
	Com_Printf("damageEvent %d\n", ps->damageEvent);
	Com_Printf("damageYaw %d\n", ps->damageYaw);
	Com_Printf("damagePitch %d\n", ps->damagePitch);
	Com_Printf("damageCount %d\n", ps->damageCount);

	Com_Printf("stats:");
	for (i = 0;  i < MAX_STATS;  i++) {
		Com_Printf(" (%d)", ps->stats[i]);
	}
	Com_Printf("\n");

	Com_Printf("persistant:");
	for (i = 0;  i < MAX_PERSISTANT;  i++) {
		Com_Printf(" (%d)", ps->persistant[i]);
	}
	Com_Printf("\n");

	Com_Printf("powerups:");
	for (i = 0;  i < MAX_POWERUPS;  i++) {
		Com_Printf(" (%d)", ps->powerups[i]);
	}
	Com_Printf("\n");

	Com_Printf("ammo:");
	for (i = 0;  i < MAX_WEAPONS;  i++) {
		Com_Printf(" (%d)", ps->ammo[i]);
	}
	Com_Printf("\n");

	Com_Printf("generic1 %d\n", ps->generic1);
	Com_Printf("loopSound %d\n", ps->loopSound);
	Com_Printf("jumppad_ent %d\n", ps->jumppad_ent);
	Com_Printf("ping %d\n", ps->ping);
	Com_Printf("pmove_framecount %d\n", ps->pmove_framecount);
	Com_Printf("jumppad_frame %d\n", ps->jumppad_frame);
	Com_Printf("entityEventSequence %d\n", ps->entityEventSequence);

	Com_Printf("jumpTime %d\n", ps->jumpTime);
	Com_Printf("doubleJumped %d\n", ps->doubleJumped);
	Com_Printf("crouchTime %d\n", ps->crouchTime);
	Com_Printf("crouchSlideTime %d\n", ps->crouchSlideTime);

	Com_Printf("location %d\n", ps->location);
	Com_Printf("fov %d\n", ps->fov);
	Com_Printf("forwardmove %d\n", ps->forwardmove);
	Com_Printf("rightmove %d\n", ps->rightmove);
	Com_Printf("upmove %d\n", ps->upmove);
}

// ammoOffset hack for quakelive weapon bar not in sync with sound
int CG_GetAmmoWarning (int weapon, int style, int ammoOffset)
{
	int i;
	int weapons;
	int ammoWarning;
	int total;
	int maxAmmo;

	ammoWarning = AMMO_WARNING_OK;

	//FIXME other gametypes
	if (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
		//return;
	}

	if (wolfcam_following) {
		return AMMO_WARNING_OK;
	}

	if (cg.snap->ps.ammo[weapon] < 0) {
		return AMMO_WARNING_OK;
	}

	if (style == 0) {  // old q3 (broken) style
		// see about how many seconds of ammo we have remaining
		weapons = cg.snap->ps.stats[ STAT_WEAPONS ];
		total = 0;
		for ( i = WP_MACHINEGUN ; i < WP_NUM_WEAPONS ; i++ ) {
			if ( ! ( weapons & ( 1 << i ) ) ) {
				continue;
			}
			switch ( i ) {
			case WP_ROCKET_LAUNCHER:
			case WP_GRENADE_LAUNCHER:
			case WP_RAILGUN:
			case WP_SHOTGUN:
#if 1  //def MPACK
			case WP_PROX_LAUNCHER:
#endif
				total += cg.snap->ps.ammo[i] * 1000;
				break;
			default:
				total += cg.snap->ps.ammo[i] * 200;
				break;
			}
			if ( total >= 5000 ) {
				ammoWarning = AMMO_WARNING_OK;
				return ammoWarning;
			}
		}

		if (total == 0) {
			ammoWarning = AMMO_WARNING_EMPTY;
		} else {
			ammoWarning = AMMO_WARNING_LOW;
		}
	} else if (style == 1) {  // quake live style bassed on low ammo percentage
		total = cg.snap->ps.ammo[weapon] + ammoOffset;

		if (cgs.armorTiered) {
			switch (weapon) {
			case WP_MACHINEGUN:
			case WP_LIGHTNING:
			case WP_PLASMAGUN:
				maxAmmo = 100;
				break;
			case WP_SHOTGUN:
			case WP_GRENADE_LAUNCHER:
			case WP_ROCKET_LAUNCHER:
			case WP_RAILGUN:
				maxAmmo = 20;
				break;
			case WP_CHAINGUN:
				maxAmmo = 100;
				break;
			case WP_PROX_LAUNCHER:
				maxAmmo = 5;
				break;
			case WP_BFG:
				maxAmmo = 10;
			case WP_NAILGUN:
				maxAmmo = 20;
				break;
			default:
				maxAmmo = 150;
				break;
			}
		} else {
			switch (weapon) {
			case WP_MACHINEGUN:
			case WP_LIGHTNING:
			case WP_PLASMAGUN:
				maxAmmo = 150;
				break;
			case WP_SHOTGUN:
			case WP_GRENADE_LAUNCHER:
			case WP_ROCKET_LAUNCHER:
			case WP_RAILGUN:
				maxAmmo = 25;
				break;
			case WP_CHAINGUN:
				maxAmmo = 200;  // yes, broken
				break;
			case WP_PROX_LAUNCHER:
				maxAmmo = 5;
				break;
			case WP_BFG:
			case WP_NAILGUN:
				maxAmmo = 25;  // yes, broken
				break;
			default:
				maxAmmo = 150;
				break;
			}
		}

		if (total == ammoOffset) {
			ammoWarning = AMMO_WARNING_EMPTY;
		} else if ((float)total / (float)maxAmmo < cg_lowAmmoWarningPercentile.value) {
			ammoWarning = AMMO_WARNING_LOW;
		} else {
			return AMMO_WARNING_OK;
		}
	} else {  // based on fixed values from cvars
		const vmCvar_t *weapCvar[] = {
			NULL, // weapon none
			&cg_lowAmmoWarningGauntlet,
			&cg_lowAmmoWarningMachineGun,
			&cg_lowAmmoWarningShotgun,
			&cg_lowAmmoWarningGrenadeLauncher,
			&cg_lowAmmoWarningRocketLauncher,
			&cg_lowAmmoWarningLightningGun,
			&cg_lowAmmoWarningRailGun,
			&cg_lowAmmoWarningPlasmaGun,
			&cg_lowAmmoWarningBFG,
			&cg_lowAmmoWarningGrapplingHook,
			&cg_lowAmmoWarningNailGun,
			&cg_lowAmmoWarningProximityLauncher,
			&cg_lowAmmoWarningChainGun,
		};

		if (weapon <= WP_NONE  ||  weapon >= WP_NUM_WEAPONS) {
			return AMMO_WARNING_OK;
		}

		total = cg.snap->ps.ammo[weapon];

		if (total == 0) {
			ammoWarning = AMMO_WARNING_EMPTY;
		} else if (total < weapCvar[weapon]->integer) {
			ammoWarning = AMMO_WARNING_LOW;
		} else {
			return AMMO_WARNING_OK;
		}
	}

	return ammoWarning;
}

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
static void CG_CheckAmmo( void ) {
	int		previous;

	previous = cg.lowAmmoWarning;

	cg.lowAmmoWarning = AMMO_WARNING_OK;

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	//FIXME other gametypes
	if (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
		//return;
	}

	if (cgs.gametype == GT_FREEZETAG  &&  CG_FreezeTagFrozen(cg.snap->ps.clientNum)) {
		return;
	}

	if (cg.snap->ps.ammo[cg.snap->ps.weapon] < 0) {
		return;
	}

	cg.lowAmmoWarning = CG_GetAmmoWarning(cg.snap->ps.weapon, cg_lowAmmoWarningStyle.integer, 0);
	if (cg.lowAmmoWarning == AMMO_WARNING_OK) {
		return;
	}

	// play a sound on transitions
	if ((!wolfcam_following  ||  (wolfcam_following  && wcg.clientNum == cg.snap->ps.clientNum))  &&  !cg.freecam) {
		if (cg_lowAmmoWarningSound.integer == 0) {
			return;
		} else if (cg_lowAmmoWarningSound.integer == 1) {  // only out of ammo
			if (cg.lowAmmoWarning != previous  &&  cg.lowAmmoWarning == AMMO_WARNING_EMPTY) {
				CG_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
			}
		} else {  // both out of ammo and low ammo
			if (cg.lowAmmoWarning != previous) {
				CG_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
			}
		}
	}
}

/*
==============
CG_DamageFeedback
==============
*/
static void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float		left, front, up;
	float		kick;
	int			health;
	float		scale;
	vec3_t		dir;
	vec3_t		angles;
	float		dist;
	float		yaw, pitch;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 ) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	
	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );
		VectorSubtract( vec3_origin, dir, dir );

		front = DotProduct (dir, cg.refdef.viewaxis[0] );
		left = DotProduct (dir, cg.refdef.viewaxis[1] );
		up = DotProduct (dir, cg.refdef.viewaxis[2] );

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength( dir );
		if ( dist < 0.1 ) {
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;
		
		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1 ) {
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if ( cg.damageX > 1.0 ) {
		cg.damageX = 1.0;
	}
	if ( cg.damageX < - 1.0 ) {
		cg.damageX = -1.0;
	}

	if ( cg.damageY > 1.0 ) {
		cg.damageY = 1.0;
	}
	if ( cg.damageY < - 1.0 ) {
		cg.damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if ( kick > 10 ) {
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}




/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
	cg.weaponSelectTime = cg.time;

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
}

//extern char *eventnames[];

/*
==============
CG_CheckPlayerstateEvents
==============
*/
static void CG_CheckPlayerstateEvents( const playerState_t *ps, const playerState_t *ops ) {
	int			i;
	int			event;
	centity_t	*cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		//Com_Printf("ps extern %d %d\n", ps->externalEvent, ps->externalEventParm);
		if (cg_checkForOfflineDemo.integer  &&  cg.offlineDemoSkipEvents) {
			//
		} else {
			CG_EntityEvent( cent, cent->lerpOrigin );
		}
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
			|| (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)]) ) {

			event = ps->events[ i & (MAX_PS_EVENTS-1) ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];
			if (cg_checkForOfflineDemo.integer  &&  cg.offlineDemoSkipEvents) {
				//Com_Printf("ps (%d) %d seq %d %d\n", cg.eventSequence, i >= ops->eventSequence, event, cent->currentState.eventParm);
			} else {
				//Com_Printf("ps events\n");
				CG_EntityEvent( cent, cent->lerpOrigin );
			}

			cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS-1) ] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents( const playerState_t *ps ) {
	int i;
	int event;
	centity_t	*cent;

	cent = &cg.predictedPlayerEntity;
	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		//
		if (i >= cg.eventSequence) {
			continue;
		}
		// if this event is not further back in than the maximum predictable events we remember
		if (i > cg.eventSequence - MAX_PREDICTED_EVENTS) {
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[i & (MAX_PS_EVENTS-1)] != cg.predictableEvents[i & (MAX_PREDICTED_EVENTS-1) ] ) {

				event = ps->events[ i & (MAX_PS_EVENTS-1) ];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];
				CG_EntityEvent( cent, cent->lerpOrigin );

				cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS-1) ] = event;

				if ( cg_showmiss.integer ) {
					CG_Printf("WARNING: changed predicted event\n");
				}
			}
		}
	}
}

/*
==================
CG_PushReward
==================
*/
void CG_PushReward (sfxHandle_t sfx, qhandle_t shader, int rewardCount)
{
	int i;

	if (cg_rewardsStack.integer == 0  &&  (cg.time - cg.rewardTime) < cg_drawRewardsTime.integer) {  // uhmm, nice name
		for (i = (cg.rewardStack);  i >= 0;  i--) {
			//Com_Printf("%d:  %d  --> %d\n", i, shader, cg.rewardShader[i]);
			if (cg.rewardShader[i] == shader) {
				cg.rewardCount[i] = rewardCount;
				return;
			}
		}
	}

	if (cg.rewardStack < (MAX_REWARDSTACK-1)) {
		cg.rewardStack++;
		cg.rewardSound[cg.rewardStack] = sfx;
		cg.rewardShader[cg.rewardStack] = shader;
		cg.rewardCount[cg.rewardStack] = rewardCount;
	} else {
		//FIXME replace older reward?
	}
}

#if 0
static void print_bits (int number)
{
	char bitString[16 + 3 + 1] = "aaaa aaaa aaaa aaaa";
	int i;
	int bit;
	int pos;

	bitString[16 + 3] = '\0';

	number = number & 0x0000ffff;
	pos = 0;
	for (i = 15;  i >= 0;  i--) {
		bit = number << (15 - i);
		bit = bit & 0x0000ffff;
		bit = bit >> 15;
		if (bit) {
			//bitString[15 - i] = '1';
			bitString[pos] = '1';
		} else {
			//bitString[15 - i] = '0';
			bitString[pos] = '0';
		}
		pos++;
		if (i % 4 == 0) {
			pos++;
		}
	}

	Com_Printf("%s\n", bitString);
}
#endif

static void CG_CheckShotgunHits (const playerState_t *ps, const playerState_t *ops)
{
	int hits;

	if (ps->weapon == WP_SHOTGUN) {
		hits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];
		//Com_Printf("shotgun:   %d\n", hits);
		if (hits > 0) {
			if (hits > 11) {
				//Com_Printf("^3FIXME shotgun hits %d\n", hits);
			}
			cg.shotgunHits = hits;
			//Com_Printf("%d shotgun hits %d\n", cg.time, cg.shotgunHits);
			//FIXME real accuracy  max(1, hits / 20)
			//wclients[ps->clientNum].wstats[WP_SHOTGUN].hits += hits;
			wclients[ps->clientNum].wstats[WP_SHOTGUN].hits += 11;
			wclients[ps->clientNum].perKillwstats[WP_SHOTGUN].hits += 11;
		} else {
			cg.shotgunHits = 0;
		}
	} else {
		cg.shotgunHits = 0;
	}
}


/*
==================
CG_CheckLocalSounds
==================
*/
static void CG_CheckLocalSounds( const playerState_t *ps, const playerState_t *ops ) {
	int			highScore, armor, reward;
	//int health;
	sfxHandle_t sfx;
	int n;
	int contents;

	//Com_Printf("cgtime %d\n", cg.time);

	//if (ps->serverTime = cg.matchRestartServerTime) {
	//	Com_Printf("local sounds match restart returning\n");
	//}

	// don't play the sounds if the player just changed teams
	if ( ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM] ) {
		//Com_Printf("^1team...............\n");
		return;
	}

	// health changes of more than -1 should make pain sounds
	if ( ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 ) {
		if ( ps->stats[STAT_HEALTH] > 0 ) {
			contents = CG_PointContents( cg.refdef.vieworg, -1 );
			if (!(contents & CONTENTS_WATER)) {
				//Com_Printf("1st person pain\n");
				CG_PainEvent( &cg.predictedPlayerEntity, ps->stats[STAT_HEALTH] );
			}
		}
	}

	if ((wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum)  ||  cg.freecam  ||  cg.cameraMode) {
		//Com_Printf("^1wtf....\n");
		if (cg.intermissionStarted) {
			return;
		}
		if (ops->pm_type != ps->pm_type) {
			//Com_Printf("^1%d != %d\n", ops->pm_type, ps->pm_type);
			return;
		}
		goto timelimit_warnings;
	}

	//FIXME not here
#if 0
	// key pickups
	if (ps->stats[STAT_MAP_KEYS] > ops->stats[STAT_MAP_KEYS]) {
		Com_Printf("new key state: %d\n", ps->stats[STAT_MAP_KEYS]);
	}
#endif

	// hit changes
	if ( ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS] ) {
		//int health;

		//Com_Printf(" hits:  %d  >  %d\n", ps->persistant[PERS_HITS], ops->persistant[PERS_HITS]);
		cg.damageDoneTime = cg.time;
		//cg.damageHits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];

		armor  = ps->persistant[PERS_ATTACKEE_ARMOR] & 0xff;
		//health = ps->persistant[PERS_ATTACKEE_ARMOR] >> 8;
		cg.damageDone = armor;  //FIXME fucked up -- cg_crosshairHitStyle 2 and fucking rail
		//Com_Printf("hits: %d  armor: %d  health: %d\n",  ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS], armor, health);

#if 0  // MISSIONPACK
		if (armor > 50 ) {
			CG_StartLocalSound( cgs.media.hitSoundHighArmor, CHAN_LOCAL_SOUND );
		} else if (armor || health > 100) {
			CG_StartLocalSound( cgs.media.hitSoundLowArmor, CHAN_LOCAL_SOUND );
		} else {
			CG_StartLocalSound( cgs.media.hitSound, CHAN_LOCAL_SOUND );
		}
#else
		//Com_Printf("eflags: 0x%x\n", ps->eFlags);
		//Com_Printf("PERS_PLAYEREVENTS 0x%x\n", ps->persistant[PERS_PLAYEREVENTS]);
		//Com_Printf("gravity: %d\n", ps->gravity);
		//Com_Printf("^5%s generic1: %d\n", weapNames[ps->weapon], ps->generic1);

		//FIXME hack for shotgun hits

        //FIXME wolfcam-q3
		//FIXME ql doesn't use this for cg_hitBeeps > 1
#if 0
		if (1) {  //(cg.snap->ps.weapon == WP_SHOTGUN) {
			int a;
			int aold;

			a = ps->persistant[PERS_ATTACKEE_ARMOR];
			aold = ops->persistant[PERS_ATTACKEE_ARMOR];
			//Com_Printf("%d  %s  hits: %d   armor:%d  health:%d  attackee_armor:%d  spawnCount:%d  killed:%d   \n",  cg.time, weapNames[cg.snap->ps.weapon], ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS], armor, health,  ps->persistant[PERS_ATTACKEE_ARMOR], ps->persistant[PERS_SPAWN_COUNT], ps->persistant[PERS_KILLED]);
			Com_Printf("%d  %s  hits: %d   armor:%d  olarmor:%d  spawnCount:%d  killed:%d   \n",  cg.time, weapNames[cg.snap->ps.weapon], ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS], a, aold, ps->persistant[PERS_SPAWN_COUNT], ps->persistant[PERS_KILLED]);
			// 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000
			//Com_Printf("15:%d\n", ps->persistant[15]);
			Com_Printf("  ---- hits:%d  events:%d  attacker:%d\n", ps->persistant[PERS_HITS], ps->persistant[PERS_PLAYEREVENTS], ps->persistant[PERS_ATTACKER]);
			print_bits(a);

			//Com_Printf("firing: %d  bskillcount: %d  x7:%d  x8:%d  x9:%d  x10:%d  x11:%d  x12:%d  x13:%d  x14:%d  x15:%d  ready:%d  maxhealth:%d\n", cg.snap->ps.eFlags & EF_FIRING, cg.snap->ps.stats[STAT_BATTLE_SUIT_KILL_COUNT], cg.snap->ps.stats[7], cg.snap->ps.stats[8], cg.snap->ps.stats[9], cg.snap->ps.stats[10], cg.snap->ps.stats[11], cg.snap->ps.stats[12], cg.snap->ps.stats[13], cg.snap->ps.stats[14], cg.snap->ps.stats[15], cg.snap->ps.stats[STAT_CLIENTS_READY], cg.snap->ps.stats[STAT_MAX_HEALTH]);
		}
#endif

		//Com_Printf("^3%d\n", ps->generic1);

		if (cgs.protocol == PROTOCOL_QL) {
			n = ps->generic1 / 64;
		} else if (cgs.cpma) {
			n = (ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS]) / 26;
			if (n > 3) {
				n = 3;
			}
		} else {
			n = 0;
		}

		switch (n) {
		case 0:
			//sh = cgs.media.hitSound0;
			cg.hitSound = 0;
			break;
		case 1:
			//sh = cgs.media.hitSound1;
			cg.hitSound = 1;
			break;
		case 2:
			//sh = cgs.media.hitSound2;
			cg.hitSound = 2;
			break;
		case 3:
			//sh = cgs.media.hitSound3;
			cg.hitSound = 3;
			break;
		default:
			//sh = cgs.media.hitSound0;
			cg.hitSound = 0;
			break;
		}
		//Com_Printf("^5generic1 %d\n", ps->generic1);

        if ((!wolfcam_following  ||  (wolfcam_following  &&  wcg.clientNum == cg.snap->ps.clientNum))  &&  !cg.freecam  &&  !cg.cameraMode) {
			if (cg_hitBeep.integer == 1) {
				CG_StartLocalSound( cgs.media.hitSound, CHAN_LOCAL_SOUND );
			} else if (cg_hitBeep.integer == 2) {
				int n;
				qhandle_t sh;

				//FIXME
				// it's not based on info in peristant[], tested with godmode + bots
				// it's in ps->generic1
				if (cgs.protocol == PROTOCOL_QL) {
					n = ps->generic1 / 64;
				} else if (cgs.cpma) {
					n = (ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS]) / 26;
					if (n > 3) {
						n = 3;
					}
				} else {
					n = 0;
				}

				switch (n) {
				case 0:
					sh = cgs.media.hitSound0;
					break;
				case 1:
					sh = cgs.media.hitSound1;
					break;
				case 2:
					sh = cgs.media.hitSound2;
					break;
				case 3:
					sh = cgs.media.hitSound3;
					break;
				default:
					sh = cgs.media.hitSound0;
					break;
				}
				//CG_StartLocalSound(sh, CHAN_LOCAL_SOUND);
				CG_StartLocalSound(sh, CHAN_LOCAL_SOUND);
				//CG_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, sh);
				//Com_Printf("%f hitbeep\n", cg.ftime);
			} else if (cg_hitBeep.integer == 3) {
				int n;
				qhandle_t sh;

				//FIXME
				// it's not based on info in peristant[], tested with godmode + bots
				// it's in ps->generic1
				if (cgs.protocol == PROTOCOL_QL) {
					n = ps->generic1 / 64;
				} else if (cgs.cpma) {
					n = (ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS]) / 26;
					if (n > 3) {
						n = 3;
					}
				} else {
					n = 0;
				}

				switch (n) {
				case 3:
					sh = cgs.media.hitSound0;
					break;
				case 2:
					sh = cgs.media.hitSound1;
					break;
				case 1:
					sh = cgs.media.hitSound2;
					break;
				case 0:
					sh = cgs.media.hitSound3;
					break;
				default:
					sh = cgs.media.hitSound0;
					break;
				}
				CG_StartLocalSound(sh, CHAN_LOCAL_SOUND);
			}
		}
#endif
	} else if ( ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS] ) {
        if ((!wolfcam_following  ||  (wolfcam_following  &&  wcg.clientNum == cg.snap->ps.clientNum))  &&  !cg.freecam  &&  !cg.cameraMode) {
			if (cg_hitBeep.integer  &&  cgs.protocol == PROTOCOL_QL) {
				CG_StartLocalSound( cgs.media.hitTeamSound, CHAN_LOCAL_SOUND );
			}
		}
	}



	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	//if (ops->pm_type == PM_INTERMISSION  &&  ps->pm_type != PM_INTERMISSION) {
	//	return;
	//}

	//FIXME why do this??  -- not doing it fixes missing holyshit sound in ctfs
	//      must have added because of ca
	if (ops->pm_type != ps->pm_type) {
		//Com_Printf("^5%d != %d\n", ops->pm_type, ps->pm_type);
		//if (cgs.gametype == GT_CTFS  &&  ps->pm_type != PM_NORMAL) {
		if (ps->pm_type != PM_NORMAL  &&  cgs.cpma) {
			// this is ok, will play last second reward sounds
		} else {
			return;
		}
	}

	// reward sounds
	reward = qfalse;
#if 0
	cg.rewardImpressive = qfalse;
	cg.rewardExcellent = qfalse;
	cg.rewardHumiliation = qfalse;
	cg.rewardDefend = qfalse;
	cg.rewardAssist = qfalse;
#endif

	if (ps->persistant[PERS_CAPTURES] > ops->persistant[PERS_CAPTURES]  &&  cg_rewardCapture.integer) {
		CG_PushReward(cgs.media.captureAwardSound, cgs.media.medalCapture, ps->persistant[PERS_CAPTURES]);
		reward = qtrue;
		//Com_Printf("capture\n");
	}
	if (ps->persistant[PERS_IMPRESSIVE_COUNT] > ops->persistant[PERS_IMPRESSIVE_COUNT]  &&  cg_rewardImpressive.integer) {
		//cg.rewardImpressive = qtrue;
		//Com_Printf("^3impressive\n");
#if 1  //def MPACK
		if (ps->persistant[PERS_IMPRESSIVE_COUNT] == 1  &&  cg_audioAnnouncerRewardsFirst.integer) {
			sfx = cgs.media.firstImpressiveSound;
		} else {
			sfx = cgs.media.impressiveSound[rand() % NUM_IMPRESSIVE_SOUNDS];
		}
#else
		sfx = cgs.media.impressiveSound;
#endif
		//Com_Printf("impres %d  %d\n", ops->persistant[PERS_IMPRESSIVE_COUNT], ps->persistant[PERS_IMPRESSIVE_COUNT]);
		CG_PushReward(sfx, cgs.media.medalImpressive, ps->persistant[PERS_IMPRESSIVE_COUNT]);
		reward = qtrue;
		//Com_Printf("impressive\n");
	}
	if (ps->persistant[PERS_EXCELLENT_COUNT] > ops->persistant[PERS_EXCELLENT_COUNT]  &&  cg_rewardExcellent.integer) {
		//cg.rewardExcellent = qtrue;

#if 1  //def MPACK
		if (ps->persistant[PERS_EXCELLENT_COUNT] == 1  &&  cg_audioAnnouncerRewardsFirst.integer) {
			sfx = cgs.media.firstExcellentSound;
		} else {
			sfx = cgs.media.excellentSound[rand() % NUM_EXCELLENT_SOUNDS];
		}
#else
		sfx = cgs.media.excellentSound;
#endif
		CG_PushReward(sfx, cgs.media.medalExcellent, ps->persistant[PERS_EXCELLENT_COUNT]);
		reward = qtrue;
		//Com_Printf("excellent %d\n", ps->persistant[PERS_ATTACKEE_ARMOR]);
	}
	if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] > ops->persistant[PERS_GAUNTLET_FRAG_COUNT]  &&  cg_rewardHumiliation.integer) {
		//cg.rewardHumiliation = qtrue;

#if 1  //def MPACK
		if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] == 1  &&  cg_audioAnnouncerRewardsFirst.integer) {
			sfx = cgs.media.firstHumiliationSound;
		} else {
			sfx = cgs.media.humiliationSound[rand() % NUM_HUMILIATION_SOUNDS];
		}
#else
		sfx = cgs.media.humiliationSound;
#endif
		CG_PushReward(sfx, cgs.media.medalGauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT]);
		reward = qtrue;
		//Com_Printf("gauntlet frag\n");
	}
	if (ps->persistant[PERS_DEFEND_COUNT] > ops->persistant[PERS_DEFEND_COUNT]  &&  cg_rewardDefend.integer) {
		//cg.rewardDefend = qtrue;

		CG_PushReward(cgs.media.defendSound, cgs.media.medalDefend, ps->persistant[PERS_DEFEND_COUNT]);
		reward = qtrue;
		//Com_Printf("defend\n");
	}
	if (ps->persistant[PERS_ASSIST_COUNT] > ops->persistant[PERS_ASSIST_COUNT]  &&  cg_rewardAssist.integer) {
		//cg.rewardAssist = qtrue;

		CG_PushReward(cgs.media.assistSound, cgs.media.medalAssist, ps->persistant[PERS_ASSIST_COUNT]);
		reward = qtrue;
		//Com_Printf("assist\n");
	}
	// if any of the player event bits changed
	//Com_Printf("%d:  %d  %d -> %d  client %d %d\n", cg.time, PERS_PLAYEREVENTS, ops->persistant[PERS_PLAYEREVENTS], ps->persistant[PERS_PLAYEREVENTS], ops->clientNum, ps->clientNum);

	if (ps->persistant[PERS_PLAYEREVENTS] > ops->persistant[PERS_PLAYEREVENTS]) {
		//Com_Printf("^3checking.........\n");
		if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) !=
				(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD)) {
			if (cg_audioAnnouncerRewards.integer) {
				CG_StartLocalSound( cgs.media.deniedSound, CHAN_ANNOUNCER );
			}
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) >
				(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD)) {
			if (cg_audioAnnouncerRewards.integer) {
				//FIXME twice checked?  what about PERS_GAUNT_ frag?
				CG_StartLocalSound(cgs.media.humiliationSound[rand() % NUM_HUMILIATION_SOUNDS], CHAN_ANNOUNCER);
			}
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT) >
				(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT)) {
			//Com_Printf("^5yes.......\n");
			if (cg_audioAnnouncerRewards.integer) {
				CG_StartLocalSound( cgs.media.holyShitSound, CHAN_ANNOUNCER );
			}
		}
		reward = qtrue;
	}

	// check for flag pickup
	if ( cgs.gametype > GT_TEAM ) {  //FIXME better fix?
		if ((ps->powerups[PW_REDFLAG] != ops->powerups[PW_REDFLAG] && ps->powerups[PW_REDFLAG]) ||
			(ps->powerups[PW_BLUEFLAG] != ops->powerups[PW_BLUEFLAG] && ps->powerups[PW_BLUEFLAG]) ||
			(ps->powerups[PW_NEUTRALFLAG] != ops->powerups[PW_NEUTRALFLAG] && ps->powerups[PW_NEUTRALFLAG]) )
		{
			if (cg_audioAnnouncerFlagStatus.integer) {
				CG_StartLocalSound( cgs.media.youHaveFlagSound, CHAN_ANNOUNCER );
			}

			if (cg_flagTakenSound.integer) {
				//CG_AddBufferedSound(cgs.media.takenYourTeamSound);
				CG_AddBufferedSound(cgs.media.takenOpponentSound);
			}
		}
	}

	// lead changes
	if (!reward  &&  cg_audioAnnouncerLead.integer) {
		if (!cg.warmup) {
			// never play lead changes during warmup
			if ( ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK] ) {
				if ( cgs.gametype < GT_TEAM) {
					if (  ps->persistant[PERS_RANK] == 0 ) {
						CG_AddBufferedSound(cgs.media.takenLeadSound);
					} else if ( ps->persistant[PERS_RANK] == RANK_TIED_FLAG ) {
						CG_AddBufferedSound(cgs.media.tiedLeadSound);
					} else if ( ( ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG ) == 0 ) {
						CG_AddBufferedSound(cgs.media.lostLeadSound);
					}
				}
			}
		}
	}

 timelimit_warnings:
	// timelimit warnings
	//if ( cgs.timelimit > 0 ) {
	if ( cgs.realTimelimit > 0  &&  cg.warmup == 0) {
		int		msec;

		msec = cg.time - cgs.levelStartTime;
		if ( !( cg.timelimitWarnings & 4 ) && msec > ( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 1 | 2 | 4;
			//CG_StartLocalSound( cgs.media.suddenDeathSound, CHAN_ANNOUNCER );
		}
		else if ( !( cg.timelimitWarnings & 2 ) && msec > (cgs.timelimit - 1) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1 | 2;
			if (cg_audioAnnouncerTimeLimit.integer) {
				CG_StartLocalSound( cgs.media.oneMinuteSound, CHAN_ANNOUNCER );
			}
		}
		else if ( cgs.timelimit > 5 && !( cg.timelimitWarnings & 1 ) && msec > (cgs.timelimit - 5) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			if (cg_audioAnnouncerTimeLimit.integer) {
				CG_StartLocalSound( cgs.media.fiveMinuteSound, CHAN_ANNOUNCER );
			}
		}
	}

	// fraglimit warnings
	//FIXME game type
	if (cgs.fraglimit > 0  &&  cgs.gametype < GT_CA  &&  cg.warmup == 0) {
		highScore = cgs.scores1;

		if (cgs.gametype == GT_TEAM  &&  cgs.scores2 > highScore) {
			highScore = cgs.scores2;
		}

		if ( !( cg.fraglimitWarnings & 4 ) && highScore == (cgs.fraglimit - 1) ) {
			cg.fraglimitWarnings |= 1 | 2 | 4;
			if (cg_audioAnnouncerFragLimit.integer) {
				CG_AddBufferedSound(cgs.media.oneFragSound);
			}
		}
		else if ( cgs.fraglimit > 2 && !( cg.fraglimitWarnings & 2 ) && highScore == (cgs.fraglimit - 2) ) {
			cg.fraglimitWarnings |= 1 | 2;
			if (cg_audioAnnouncerFragLimit.integer) {
				CG_AddBufferedSound(cgs.media.twoFragSound);
			}
		}
		else if ( cgs.fraglimit > 3 && !( cg.fraglimitWarnings & 1 ) && highScore == (cgs.fraglimit - 3) ) {
			cg.fraglimitWarnings |= 1;
			if (cg_audioAnnouncerFragLimit.integer) {
				CG_AddBufferedSound(cgs.media.threeFragSound);
			}
		}
	}
}

#if 0
static void CG_DefragStats (void)
{
	int l;

	unsigned local18;
	float local3c;
	int local24;
	int local40;
	float local44;
	int local48;
	int local4c;
	char *local14;
	unsigned local1c;
	int local20;

	local18 = (cg.snap->ps.stats[7] << 0x10) | (cg.snap->ps.stats[8] & 0xffff);

	// origin
	local3c = floor(cg.snap->ps.origin[0]);
	local24 = local3c;
	local40;
	if (local24 > 0)
		local40 = local24 & 0xffff;
	else
		local40 = -local24 & 0xffff;
	local18 = local18 ^ local40;

	// velocity
	local44 = floor(cg.snap->ps.velocity[0]);
	local24 = local44;
	local48;
	if (local24 > 0)
		local48 = local24 << 0x10;
	else
		local48 = -local24 << 0x10;
	local18 = local18 ^ local48;

	// health
	local24 = cg.snap->ps.stats[0];
	local4c;
	if (local24 > 0)
		local4c = local24 & 0xff;
	else
		local4c = 150;
	local18 = local18 ^ local4c;

	// movementDir
	local18 = local18 ^ ((cg.snap->ps.movementDir & 0xf) << 0x1c);

	// loop
	local14 = (char *) &local18;
	local14 += 3;
	for (l = 0; l < 3; l++) {
		*local14 = *local14 ^ *(local14 - 1);
		local14--;
	}

	// this works only for demo replaying, there is a drift when playing
	local1c = cg.snap->ps.commandTime << 2;
	// values 19120 and 196 are actually variables, not consts!
	// 19120 = probably defrag version
	// 196 = strafe map? (197 = map with weapons?)
	/*
// offsets are valid for df 1.91.20
i = *(int *) (cgvm->dataBase + 0x11c2e8);
j = *(unsigned char *) (cgvm->dataBase + 0x11c698);
Com_Printf("%d %d\n", i, j);
	*/
	local1c = local1c + ((19120 + 196) << 8);
	local1c = local1c ^ (cg.snap->ps.commandTime << 0x18);
	local18 = local18 ^ local1c;
	local1c = local18 >> 0x1c;
	local1c = local1c | ((~local1c << 4) & 0xff);
	local1c = local1c | (local1c << 8);
	local1c = local1c | (local1c << 0x10);
	local18 = local18 ^ local1c;
	local1c = (local18 >> 0x16) & 0x3f;
	local18 = local18 & 0x3fffff;

	// loop 2
	local20 = 0;
	for (l = 0; l < 3; l++)
		local20 = local20 + ((local18 >> (6 * l)) & 0x3f);
	local20 = local20 + ((local18 >> 0x12) & 0xf);
	if (local1c != (local20 & 0x3f)) {
		//Com_Printf("WRONG\n");
		// call some function that will probably print that checksum is wrong
	}
	// local18 contains run time
	Com_Printf("%u\n", local18);
}
#endif

static void test_flags (const playerState_t *ps)
{
	if (!SC_Cvar_Get_Int("cg_testflags")) {
		return;
	}

	//ent->s.eFlags &= ~EF_NODRAW
	if (ps->eFlags >= EF_TEAMVOTED * 2) {
		Com_Printf("^3unknown ps->eFlag 0x%x\n", ps->eFlags);
	}

	if (ps->pm_flags >= PMF_INVULEXPAND * 2) {
		Com_Printf("^3unknown ps->pm_flag 0x%x\n", ps->pm_flags);
	}
}

static void test_persStats (const playerState_t *ps)
{
	int i;

	if (cgs.protocol == PROTOCOL_Q3) {
		for (i = STAT_MAX_HEALTH + 1;  i < 16;  i++) {
			if (ps->stats[i]) {
				if (SC_Cvar_Get_Int("cg_debugstats")) {
					Com_Printf("^3FIXME protocol %d stats unknown  %d  %d\n", PROTOCOL_Q3, i, ps->stats[i]);
				}
			}
		}
		if (1) {  //(cgs.defrag) {
			//CG_DefragStats();
		}
	} else {
#if 0
	if (ps->stats[STAT_UNKNOWN_8]) {
		Com_Printf("^3FIXME stats unknown 8 %d\n", ps->stats[STAT_UNKNOWN_8]);
	}
	if (ps->stats[STAT_UNKNOWN_9]) {
		Com_Printf("^3FIXME stats unknown 9 %d\n", ps->stats[STAT_UNKNOWN_9]);
	}
	if (ps->stats[STAT_UNKNOWN_10]) {
		Com_Printf("^3FIXME stats unknown 10 %d\n", ps->stats[STAT_UNKNOWN_10]);
	}

	if (ps->stats[STAT_UNKNOWN_12]) {
		Com_Printf("^3FIXME stats unknown 12 %d\n", ps->stats[STAT_UNKNOWN_12]);
	}

	if (ps->stats[STAT_UNKNOWN_14]) {
		Com_Printf("^3FIXME stats unknown 14 %d\n", ps->stats[STAT_UNKNOWN_14]);
	}
#endif

#if 0  // new ql map keys
	if (ps->stats[STAT_UNKNOWN_15]) {
		Com_Printf("^3FIXME stats unknown 15 %d\n", ps->stats[STAT_UNKNOWN_15]);
	}
#endif
	}
}


/*
===============
CG_TransitionPlayerState

===============
*/
//FIXME writing to ops
void CG_TransitionPlayerState( const playerState_t *ps, playerState_t *ops ) {
	int i;
	timedItem_t *ti;
	int weapons;

	// check for changing follow mode
	//if (ps != cg.nextSnap) {
	//	Com_Printf("wtf\n");
	//}

	if (cgs.realProtocol >= 91) {
		cg.demoFov = ps->fov;

		if (cg_useDemoFov.integer == 2  &&  !wolfcam_following) {
			//Com_Printf("fov:  %d\n", ps->fov);
			if (ps->clientNum == ops->clientNum  &&  ps->pm_type == ops->pm_type) {  // pm_type check fixes clan arena switching player views to free spec when dead (entire team dead)
				//Com_Printf("team: %d  %d\n", ps->persistant[PERS_TEAM], ps->clientNum);
				if ((ps->pm_type != PM_INTERMISSION  &&  ops->pm_type != PM_INTERMISSION)  &&
					//(ps->pm_type != PM_SPECTATOR  &&  ops->pm_type != PM_SPECTATOR)  /* clan arena dead-spec fix */  &&  // 2016-04-01 no -- also disables zoom for spec demos
					(ps->fov > 0)  /* 2016-02-12 min ql zoom seems to be 10 and this avoids probable entity reset (fov == 0) at start of match translating to zoom */

					) {

					// 2016-04-01 clan arena switch from other player when dead to free spec if your entire team dies seems to keep the fov of the last player for one player state when the switch occurs
					if (ps->pm_type == PM_SPECTATOR) {
						//Com_Printf("^3spec %d %d\n", ps->clientNum, cg.clientNum);
						//CG_PrintPlayerState();
					}

					if (ps->fov < ops->fov) {
						//Com_Printf("^1zoom down ps pm_type: %d %d  zoom: %d %d  tele: %d %d realClientNum:%d clientNum:%d %d\n", ops->pm_type, ps->pm_type, ops->fov, ps->fov, ops->eFlags & EF_TELEPORT_BIT, ps->eFlags & EF_TELEPORT_BIT, cg.clientNum, ops->clientNum, ps->clientNum);
						//Com_Printf("^2zoom down ps\n");
						CG_ZoomDown_f();
					} else if (ps->fov > ops->fov) {
						//Com_Printf("^2zoom up ps spec:%d  zoom: %d %d\n", ps->pm_type == PM_SPECTATOR, ops->fov, ps->fov);
						CG_ZoomUp_f();
					}
				}
			} else {  // client num or pm_type not equal
				//Com_Printf("^2zoom up ps\n");
				CG_ZoomUp_f();
			}
		}
	}

	//FIXME ops ?
	weapons = ps->stats[STAT_WEAPONS];
	for (i = WP_GAUNTLET;  i < WP_NUM_WEAPONS;  i++) {
		qboolean haveWeapon;
		const weaponInfo_t *wi;

		haveWeapon = weapons & (1 << i);
		if (!haveWeapon) {
			continue;
		}

		wi = &cg_weapons[i];
		if (!wi->registered){
			CG_RegisterWeapon(i);
		}
	}

	if (cgs.cpma) {
		//if (!cg.intermissionStarted  &&  ps->pm_type == PM_INTERMISSION) {
		if (!cg.intermissionStarted  &&  ops->pm_type == PM_INTERMISSION) {
			// hack for overtime and end of buzzer scoring and switching
			// to PM_INTERMISSION without having updated scores
			if ((cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS)  &&  (cgs.scores1 != cgs.scores2)) {
				trap_SendConsoleCommand("exec gameend.cfg\n");
				CG_InterMissionHit();
				cg.intermissionStarted = cg.time;
			}  // else have to wait until score change config string
		}

		if (ps->clientNum != ops->clientNum) {
			for (i = 0;  i < cg.numMegaHealths;  i++) {
				cg.megaHealths[i].clientNum = -1;  //FIXME hack
			}
		}

		if (cgs.cpm  &&  ps->stats[STAT_HEALTH] <= 100) {
			for (i = 0;  i < cg.numMegaHealths;  i++) {
				ti = &cg.megaHealths[i];

				if (ti->clientNum != ps->clientNum) {
					continue;
				}
				if (ti->countDownTrigger >= 0) {
					continue;
				}
				ti->countDownTrigger = cg.time;
			}
		}
	}

	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;  //FIXME const,  write hmmmmm
		CG_ResetLagometer();
	}

	test_flags(ps);
	test_persStats(ps);

	if (cg.demoPlayback) {
		if (ps->weapon != ops->weapon) {
			cg.weaponSelectTime = cg.time;
			cg.weaponSelect = ps->weapon;
			// this seems to fix bug in weapon instant switch demos where
			// the fire sound of the old weapon is heard:
			//cg.predictedPlayerEntity.currentState.weapon = ps->weapon;
			// not added since fx changes made earlier seem to fix it
		}
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// respawning
	if ( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT] ) {
		CG_Respawn();
	}

	if ( cg.mapRestart ) {
		CG_Respawn();
		cg.mapRestart = qfalse;
	}

	// run events, done before checklocalsounds() so falling doesn't trigger
	// two pain events
	CG_CheckPlayerstateEvents(ps, ops);

	if ( cg.snap->ps.pm_type != PM_INTERMISSION
		&& ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		if (cg.prevSnap  &&  cg.prevSnap->serverTime == cg.matchRestartServerTime) {
			//Com_Printf("match restart, not checking local sounds\n");
			// pass
		} else if (cg.prevSnap  &&  cg.snap  &&  cg.prevSnap->serverTime == cg.snap->serverTime) {
			// pass, offline bot demo
		} else {
			CG_CheckShotgunHits(ps, ops);
			CG_CheckLocalSounds( ps, ops );
		}
	}

	// check for going low on ammo
	//Com_Printf("testing checking for low ammo\n");
	CG_CheckAmmo();

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight ) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}

	if (!(ps->eFlags & EF_TICKING)) {
		cg.proxTime = 0;
	} else {
		if (cg.proxTime == 0) {
			cg.proxTime = cg.time + 5000;
			cg.proxCounter = 5;
			cg.proxTick = 0;
		}

		if (cg.time > cg.proxTime) {
			cg.proxTick = cg.proxCounter--;
			cg.proxTime = cg.time + 1000;
		}
	}
}


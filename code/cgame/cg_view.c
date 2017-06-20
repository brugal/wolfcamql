// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

#include "cg_camera.h"
#include "cg_consolecmds.h"  // popup()
#include "cg_draw.h"  // CG_Fade()
#include "cg_effects.h"
#include "cg_ents.h"
#include "cg_info.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_players.h"  // CG_Q3ColorFromString()
#include "cg_predict.h"
#include "cg_q3mme_camera.h"
#include "cg_servercmds.h"  // CG_PlayBufferedVoiceChats()
#include "cg_snapshot.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_playerstate.h"
#include "wolfcam_predict.h"
#include "wolfcam_view.h"
#include "wolfcam_weapons.h"

#include "wolfcam_local.h"
#include "../client/keycodes.h"
#include "../game/bg_local.h"

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	CG_AddRefEntity(&cg.testModelEntity);
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
void CG_CalcVrect (void)
{
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================


/*
===============
CG_OffsetThirdPersonView

===============
*/
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;
	vec3_t killerAngles;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	//FIXME testing quakelive
	//cg.refdef.vieworg[2] -= 15;

#if 0
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR) {
		//cg.refdef.vieworg[2] -= 56;
		//Com_Printf("yeah...\n");
		if (*cg_specViewHeight.string) {
			cg.refdef.vieworg[2] -= cg.predictedPlayerState.viewheight;
			cg.refdef.vieworg[2] += cg_specViewHeight.value;
			//Com_Printf("new view %f\n", cg_specViewHeight.value);
		}
	}
#endif

	//Com_Printf("vh %d\n", cg.predictedPlayerState.viewheight);

	VectorCopy( cg.refdefViewAngles, focusAngles );

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0  &&

		 // hack for setting spec free health to 0 like quake live
		 !(!cg.demoPlayback  &&  cg.clientNum == cg.snap->ps.clientNum  &&  cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)

	   ) {
		if (cg_deathStyle.integer == 1) {
			if (cg.killerClientNum >= 0  &&  cg.killerClientNum < MAX_CLIENTS  &&  cg.killerClientNum != cg.snap->ps.clientNum) {
				focusAngles[YAW] = cg.deadAngles[YAW];
				cg.refdefViewAngles[YAW] = cg.deadAngles[YAW];
			}
		} else if (cg_deathStyle.integer == 2) {
			if (cg.killerClientNum >= 0  &&  cg.killerClientNum < MAX_CLIENTS  &&  cg.killerClientNum != cg.snap->ps.clientNum) {
				// face killer
				//VectorSubtract(cg.killerOrigin, cg.snap->ps.origin, forward);
				VectorSubtract(cg_entities[cg.killerClientNum].lerpOrigin, cg.snap->ps.origin, forward);
				vectoangles(forward, killerAngles);
				VectorCopy(killerAngles, cg.refdefViewAngles);
				return;

				//focusAngles[YAW] = killerAngles[YAW];
				//cg.refdefViewAngles[YAW] = killerAngles[YAW];
			}
		} else if (cg_deathStyle.integer == 3) {
			//Com_Printf("%d\n", cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
			focusAngles[YAW] = 0;
			cg.refdefViewAngles[YAW] = 0;
		}

	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out

			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
			VectorCopy( trace.endpos, view );
		}
	}

	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieworg
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180.0 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}

static void CG_OffsetQuakeLiveSpec (void)
{
	//Com_Printf("offset ql spec...\n");
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	float timeDelta;

	if (cg.demoPlayback  &&  !cg_demoStepSmoothing.integer) {
		return;
	}

	if (cg_stepSmoothTime.value <= 0.0) {
		return;
	}

	// smooth out stair climbing
	timeDelta = cg.ftime - (float)cg.stepTime;
	if (timeDelta < cg_stepSmoothTime.value) {
		if (cg.demoPlayback  &&  cg.snap  &&  cg.nextSnap) {
			cg.refdef.vieworg[2] = cg.nextSnap->ps.origin[2] + cg.nextSnap->ps.viewheight;
			//Com_Printf("lerp %f  %f -> %f\n", cg.refdef.vieworg[2], cg.snap->ps.origin[2] + cg.snap->ps.viewheight, cg.nextSnap->ps.origin[2] + cg.nextSnap->ps.viewheight);
		}
		cg.refdef.vieworg[2] -= (float)cg.stepChange
			* (cg_stepSmoothTime.value - timeDelta) / cg_stepSmoothTime.value;
		//Com_Printf("xx %f\n", cg.refdef.vieworg[2]);
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView (qboolean ignoreHealth)
{
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	int				timeDelta;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if (!ignoreHealth  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		//angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		angles[YAW] = cg.deadAngles[YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		//Com_Printf("dead...\n");
		return;
	}


	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		ratio *= cg_kickScale.value;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

#if 0
	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	//VectorNormalize(predictedVelocity);
	//AnglesToAxis (cg.refdefViewAngles, cg.refdef.viewaxis );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	//Com_Printf("delta: %f (%f %f %f)  (%f %f %f)\n", delta, predictedVelocity[0], predictedVelocity[1], predictedVelocity[2], cg.refdef.viewaxis[0][0], cg.refdef.viewaxis[0][1], cg.refdef.viewaxis[0][2]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;
#endif

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching accentuates roll
	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;

	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

void CG_ZoomDown_f( void ) {
	double diff;

	//Com_Printf("zoom down\n");
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;

	diff = cg.ftime - cg.zoomTime;
	if (diff > cg_zoomTime.value) {
		diff = cg_zoomTime.value;
	}
	cg.zoomTime = cg.ftime - (cg_zoomTime.value - diff);

	diff = cg.realTime - cg.zoomRealTime;
	if (diff > cg_zoomTime.value) {
		diff = cg_zoomTime.value;
	}
	cg.zoomRealTime = cg.realTime - (cg_zoomTime.value - diff);

	cg.zoomBrokenTime = cg.ftime;
	cg.zoomBrokenRealTime = cg.realTime;
}

void CG_ZoomUp_f( void ) {
	double diff;

	//Com_Printf("zoom up\n");
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;

	diff = cg.ftime - cg.zoomTime;
	if (diff > cg_zoomTime.value) {
		diff = cg_zoomTime.value;
	}
	cg.zoomTime = cg.ftime - (cg_zoomTime.value - diff);

	diff = cg.realTime - cg.zoomRealTime;
	if (diff > cg_zoomTime.value) {
		diff = cg_zoomTime.value;
	}
	cg.zoomRealTime = cg.realTime - (cg_zoomTime.value - diff);

	cg.zoomBrokenTime = cg.ftime;
	cg.zoomBrokenRealTime = cg.realTime;
}


double CG_CalcZoom (double fov_x)
{
	double zoomFov, ourTime, zoomTime, f;

	zoomFov = cg_zoomFov.value;
	if ( zoomFov < 1 ) {
		zoomFov = 1;
	} else if ( zoomFov > 160 ) {
		//zoomFov = 160;
	}

	if (cg_zoomIgnoreTimescale.integer  ||  cg.paused) {
		ourTime = cg.realTime;
		if (cg_zoomBroken.integer) {
			zoomTime = cg.zoomBrokenRealTime;
		} else {
			zoomTime = cg.zoomRealTime;
		}
	} else {
		ourTime = cg.ftime;
		if (cg_zoomBroken.integer) {
			zoomTime = cg.zoomBrokenTime;
		} else {
			zoomTime = cg.zoomTime;
		}
	}

	if ( cg.zoomed ) {
		if (cg_zoomTime.value > 0.0) {
			f = ( ourTime - zoomTime ) / cg_zoomTime.value;
		} else {
			f = 2.0;
		}
		if ( f > 1.0 ) {
			fov_x = zoomFov;
		} else {
			fov_x = fov_x + f * ( zoomFov - fov_x );
			//Com_Printf("zooming up fov %f  f %f  ourTime %f  zoomTime %f  paused %d\n", fov_x, f, ourTime, zoomTime, cg.paused);
		}
	} else {
		if (cg_zoomTime.value > 0.0) {
			f = ( ourTime - zoomTime ) / cg_zoomTime.value;
		} else {
			f = 2.0;
		}
		if ( f <= 1.0 ) {
			fov_x = zoomFov + f * ( fov_x - zoomFov );
		}
	}

	return fov_x;
}

void CG_AdjustedFov (float fov_x, float *new_fov_x, float *new_fov_y)
{
	double x;
	double fov_y;
	double aspectWidth;
	double aspectHeight;

	/*

	  r = w / h = tan(Hfov/2) / tan(Vfov/2)

	  Hfov = 2 * atan(tan(Vfov / 2) * (w/h))
	  Vfov = 2 * atan(tan(Hfov / 2) * (h/w))


	 */

	if (cg_fovStyle.integer == 0) {   // quake3, keep x fov as is and adjust y fov to avoid stretching, zooms in with higher screen width
		if (*cg_fovy.string) {
			fov_y = cg_fovy.value;
		} else {
			x = cg.refdef.width / tan(fov_x / 360.0 * M_PI);
			fov_y = atan2(cg.refdef.height, x);
			fov_y = fov_y * 360.0 / M_PI;
		}
		*new_fov_x = fov_x;
		*new_fov_y = fov_y;
		return;
	}

	// preserves y fov (as if viewing area was 4:3) and adjusts x fov so that more is shown when screen width increases.  This avoids stretching.

	aspectWidth = cg.refdef.width;
	aspectHeight = cg.refdef.height;

	// avoid divide by zero in fovstyle 2 check
	if (aspectWidth <= 0) {
		aspectWidth = 1;
	}

	if (aspectHeight <= 0) {
		aspectHeight = 1;
	}

	if (cg_fovStyle.integer == 2) {  // quake live, keep y fov as is, set x fov using preset aspect ratios and stretch image horizontally if real aspect doesn't match
		double ratio;

		ratio = aspectWidth / aspectHeight;

		if (ratio >= (16.0 / 9.0)) {  // 1.777...
			aspectWidth = 16.0;
			aspectHeight = 9.0;
		} else if (ratio >= 16.0 / 10.0) {  // 1.6
			aspectWidth = 16.0;
			aspectHeight = 10.0;
		} else if (ratio >= 4.0 / 3.0) {  // 1.333...
			aspectWidth = 4.0;
			aspectHeight = 3.0;
		} else {  // 5:4   1.25
			aspectWidth = 5.0;
			aspectHeight = 4.0;
		}
	}

	if (*cg_fovForceAspectWidth.string) {
		aspectWidth = cg_fovForceAspectWidth.value;
	}

	if (*cg_fovForceAspectHeight.string) {
		aspectHeight = cg_fovForceAspectHeight.value;
	}

	if (aspectWidth <= 0) {
		aspectWidth = 1;
	}

	if (aspectHeight <= 0) {
		aspectHeight = 1;
	}

	// reference:  fov_x = 2 * atan(tan(fov_y/2) * width/height)

	if (fov_x < 1.0) {
		fov_x = 1.0;
	} else if (fov_x > 180.0) {
		//fov_x = 180.0;
	}

	if (*cg_fovy.string) {
		fov_y = cg_fovy.value;
	} else {
		fov_y = 2.0 * atan2(tan((fov_x * (M_PI / 180.0)) / 2.0) * (480.0 / 640.0), 1);
		fov_y *= (180.0 / M_PI);
	}

	//Com_Printf("yfov %f\n", fov_y);

	//fov_x = 360.0 / M_PI * atan2(aspectWidth / aspectHeight * tan(fov_y * M_PI / 360.0), 1);

	fov_x = 2.0 * atan2(tan(fov_y * (M_PI / 180.0) / 2.0) * (aspectWidth / aspectHeight), 1);
	fov_x *= (180.0 / M_PI);

	//Com_Printf("%f  ->  %f\n", cg_fov.value, fov_x);

	*new_fov_x = fov_x;
	*new_fov_y = fov_y;
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

static int CG_CalcFov( void ) {
	float	phase;
	float	v;
	int		contents;
	double	fov_x, fov_y;
	int		inwater;
	float nx, ny;

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		if (*cg_fovIntermission.string) {
			fov_x = cg_fovIntermission.value;
		} else {
			if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
				fov_x = cg.demoFov;
			} else {
				fov_x = cg_fov.value;
			}
		}
	} else {
		// user selectable
		//FIXME enable for q3 demos?
		if (0) {  //( cgs.dmflags & DF_FIXED_FOV ) {  //FIXME some ql demos have it set,   maybe dmflags have changed
			// dmflag to prevent wide fov for all clients
			Com_Printf("DF_FIXED_FOV\n");
			fov_x = 90;
		} else {
			if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
				fov_x = cg.demoFov;
			} else {
				fov_x = cg_fov.value;
			}

			if ( fov_x < 1 ) {
				fov_x = 1;
			} else if ( fov_x > 160 ) {
				//fov_x = 160;
			}
		}

		fov_x = CG_CalcZoom(fov_x);
	}

	CG_AdjustedFov(fov_x, &nx, &ny);
	fov_x = nx;
	fov_y = ny;

	// warp if underwater
	if (cg_waterWarp.integer) {
		contents = CG_PointContents( cg.refdef.vieworg, -1 );
	} else {
		contents = 0;
	}
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.ftime / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}


	//Com_Printf("fov_x  %f\n", fov_x);

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}



/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void ) {
	int			t;
	int			maxTime;
	refEntity_t		ent;
	int attacker;
	vmCvar_t colorCvar;
	int alpha;

	if ( !cg.damageValue ) {
		return;
	}

	if (cg.cameraMode) {
		return;
	}

	// ragePro systems can't fade blends, so don't obscure the screen
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	attacker = cg.predictedPlayerState.persistant[PERS_ATTACKER];

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;

	if (attacker == cg.snap->ps.clientNum) {
		colorCvar = cg_screenDamage_Self;
		alpha = cg_screenDamageAlpha_Self.integer;
	} else if (attacker >= 0  &&  attacker < MAX_CLIENTS  &&  cgs.gametype >= GT_TEAM  &&  cgs.clientinfo[attacker].team == cg.snap->ps.persistant[PERS_TEAM]) {
		//FIXME clan arena check
		colorCvar = cg_screenDamage_Team;
		alpha = cg_screenDamageAlpha_Team.integer;
	} else {
		colorCvar = cg_screenDamage;
		alpha = cg_screenDamageAlpha.integer;
	}

	ent.shaderRGBA[0] = SC_RedFromCvar(&colorCvar);
	ent.shaderRGBA[1] = SC_GreenFromCvar(&colorCvar);
	ent.shaderRGBA[2] = SC_BlueFromCvar(&colorCvar);
	ent.shaderRGBA[3] = alpha * ( 1.0 - ((float)t / maxTime) );

	//Com_Printf("%x %x %x  %d\n", ent.shaderRGBA[0], ent.shaderRGBA[1], ent.shaderRGBA[2], alpha);

	CG_AddRefEntity(&ent);
}


/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/

static int CG_CalcViewValues( void ) {
	const playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap_getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/

// camera script
	if (cg.cameraMode) {
		vec3_t origin, angles;
		float fov = 90;  //FIXME cg_fovIdCamera
		//float x;
		float nx, ny;

		//if (trap_getCameraInfo(cg.time, &origin, &angles, &fov)) {
		if (trap_getCameraInfo(cg.realTime, &origin, &angles, &fov)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			angles[PITCH] = -angles[PITCH]; // Bug Fix for GtkRadiant cameras
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			/*
			x = cg.refdef.width / tan(fov / 360.0 * M_PI);
			cg.refdef.fov_y = atan2(cg.refdef.height, x);
			cg.refdef.fov_y = cg.refdef.fov_y * 360.0 / M_PI;
			cg.refdef.fov_x = fov;
			*/
			CG_AdjustedFov(fov, &nx, &ny);
			cg.refdef.fov_x = nx;
			cg.refdef.fov_y = ny;
			trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);
			return 0;
		} else {
			cg.cameraMode = qfalse;
			CG_Fade(255, 0, 0);				// go black
			//CG_Fade(0, cg.time + 200, 1500);	// then fadeup
			CG_Fade(0, cg.realTime + 200, 1500);	// then fadeup
			//
			// letterbox look
			//
			//black_bars=0;  //FIXME
		}
	}
	// end camera script

	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	if (cg.demoPlayback  &&  cg_demoSmoothing.integer >= 2) {
		//FIXME not here, temporarily doing it because of the inconsistency
		//  of cg.predictedPlayerEntity and cg_entities[cg.snap->clientNum].currentState
		if (cg.snap  &&  cg.nextSnap) {
			float f;
			vec3_t origin;
			vec3_t angles;
			int currentNum;
			static int oldCurrentNum = 0;
			int nextNum;
			qboolean r;
			const snapshot_t *old;
			const snapshot_t *new;

			if (cg_demoSmoothing.integer == 2) {
				old = cg.snap;
				new = cg.nextSnap;
			} else {
				int smoothNum;

				smoothNum = cg_demoSmoothing.integer;
				if (smoothNum >= PACKET_BACKUP) {
					smoothNum = PACKET_BACKUP - 1;
				}

				//FIXME store values and only get on snapshot transition
				currentNum = cg.snap->messageNum - cg.snap->messageNum % (smoothNum -  1);
				nextNum = currentNum + (smoothNum - 1);

				//Com_Printf("current %d  next %d\n", currentNum, nextNum);

				//FIXME check returns
				r = CG_GetSnapshot(currentNum, &cg.smoothOldSnap);
				if (!r) {
					//Com_Printf("^3smooth couldn't get old snapshot %d\n", currentNum);
				}
				if (cg.smoothOldSnap.messageNum != currentNum) {
					//Com_Printf("^3smooth got wrong old snap number %d != %d (want)\n", cg.smoothOldSnap.messageNum, currentNum);
				}
				r = CG_PeekSnapshot(nextNum, &cg.smoothNewSnap);
				if (!r) {
					//Com_Printf("^3smooth couldn't get next snapshot %d\n", nextNum);
				}
				if (cg.smoothNewSnap.messageNum != nextNum) {
					//Com_Printf("^1smooth got wrong next snap number %d != %d (want)\n", cg.smoothNewSnap.messageNum, nextNum);
				}

				old = &cg.smoothOldSnap;
				new = &cg.smoothNewSnap;

				if (oldCurrentNum != currentNum) {
				    //FIXME this is being done every drawn frame instead
				    // of every snapshot transition frame
				    //Com_Printf("^2transition yaw : (%d) %f -> (%d) %f\n", currentNum, old->ps.viewangles[YAW], nextNum, new->ps.viewangles[YAW]);
				}
				oldCurrentNum = currentNum;
			}

			//Com_Printf("old %d  new %d\n", old->ps.eFlags & EF_TELEPORT_BIT, new->ps.eFlags & EF_TELEPORT_BIT);

				if (cg_demoSmoothingTeleportCheck.integer && (
				        old->ps.clientNum != new->ps.clientNum  ||  ((old->ps.eFlags ^ new->ps.eFlags) & EF_TELEPORT_BIT)
					  ||
				        old->ps.persistant[PERS_SPAWN_COUNT] != new->ps.persistant[PERS_SPAWN_COUNT])
				) {
				//Com_Printf("^3teleport %f\n", cg.ftime);
				// don't interp
				f = 0;
			} else {
				f = (cg.ftime - (double)old->serverTime) / (double)(new->serverTime - old->serverTime);
			}

			//CG_Printf("cg.time %f  old %d  new %d\n", cg.ftime, old->serverTime, new->serverTime);
			//CG_Printf("lll %d  f %f\n", cg.snap->serverTime, f);

			origin[0] = old->ps.origin[0] + f * (new->ps.origin[0] - old->ps.origin[0]);
			origin[1] = old->ps.origin[1] + f * (new->ps.origin[1] - old->ps.origin[1]);
			origin[2] = old->ps.origin[2] + f * (new->ps.origin[2] - old->ps.origin[2]);

			VectorCopy(origin, cg.refdef.vieworg);

			if (cg_demoSmoothingAngles.integer) {
				angles[0] = LerpAngleNear(old->ps.viewangles[0], new->ps.viewangles[0], f);
				angles[1] = LerpAngleNear(old->ps.viewangles[1], new->ps.viewangles[1], f);
				angles[2] = LerpAngleNear(old->ps.viewangles[2], new->ps.viewangles[2], f);
				VectorCopy(angles, cg.refdefViewAngles);
			} else {
				VectorCopy(ps->viewangles, cg.refdefViewAngles);
			}
		} else {  // snaps invalid, possibly start of demo
			VectorCopy( ps->origin, cg.refdef.vieworg );
			VectorCopy( ps->viewangles, cg.refdefViewAngles );
		}
	} else {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	}

	//FIXME player shadow -- no, it has already been added
	//VectorCopy(cg.refdef.vieworg, cg.predictedPlayerEntity.lerpOrigin);
	//VectorCopy(cg.refdef.vieworg, cg_entities[cg.snap->ps.clientNum].lerpOrigin);

	if (!cg.freezeEntity[cg.snap->ps.clientNum]) {
		cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
		cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
		cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] );
	} else {
		if (cg.renderingThirdPerson) {
			CG_OffsetThirdPersonView();
		} else {
			cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
		}

		goto done;
	}

	if (cg_cameraOrbit.integer) {
		//if (cg.time > cg.nextOrbitTime) {
		if (cg.realTime > cg.nextOrbitTime) {
			//cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg.nextOrbitTime = cg.realTime + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson ) {
		//if ( cg.renderingThirdPerson  ||  (!wolfcam_following  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0)) {
		// back away from character

		// hack for ql setting spec health to 0, don't use third person offset
		if (cgs.protocol == PROTOCOL_QL  &&   cg.snap->ps.clientNum == cg.clientNum  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0  &&  cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
			if (cg_specOffsetQL.integer == 1) {
				CG_OffsetQuakeLiveSpec();
			} else if (cg_specOffsetQL.integer == 2) {
				CG_OffsetThirdPersonView();
			} else {
				CG_OffsetFirstPersonView(qtrue);
			}
		} else {
			CG_OffsetThirdPersonView();
		}
		//Com_Printf("third...\n");
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView(qfalse);
		//Com_Printf("first...\n");
	}


 done:

	// position eye relative to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			CG_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;

	if (cg_soundBuffer.integer) {
		cg.soundBuffer[cg.soundBufferIn] = sfx;
		cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
		if (cg.soundBufferIn == cg.soundBufferOut) {
			cg.soundBufferOut++;
		}
	} else {
		CG_StartLocalSound(sfx, CHAN_ANNOUNCER);
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			CG_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

#if 0
static void Wolfcam_Stall (unsigned int ticks)
{
    int i;
    int x;
    //qtime_t ct;

    x = 600;

    for (i = 0;  i < ticks;  i++) {
        if (i % 2 == 0)
            x++;
        else
            x--;
    }
}
#endif

//FIXME hack to avoid spamming
static char BlackListedShader[MAX_QPATH];

static void CG_DrawAdvertisements (void)
{
	float *vt;
	int i;
	polyVert_t verts[4];
	//vec3_t		normal;
	//float		cellId;
	int j;
	int scaleGuess;
	qboolean useSingleShader;
	int r;
	//int lightmap;
	const char *shaderName;
	qboolean shaderOverride;
	qboolean turn90 = qfalse;

	if (!cgs.adsLoaded) {
		trap_Get_Advertisements(&cgs.numAds, cgs.adverts, cgs.adShaders);
		cgs.adsLoaded = qtrue;
		Com_Printf("ads: %d\n", cgs.numAds);
		for (i = 0;  i < cgs.numAds;  i++) {
			//FIXME not using ad shaders?
			Com_Printf("ad %d: '%s'\n", i + 1, cgs.adShaders[i]);
#if 0
			//CG_Printf("%d : %f\n", i + 1, cgs.adverts[i * 16  + 15]);
			if (i > 0) {
				if (cgs.adverts[i * 16 + 15] == cgs.adverts[(i - 1) * 16 + 15]) {
					cgs.transAds[i] = cgs.transAds[i - 1] = qtrue;
					//Com_Printf("adds %d and %d transparent\n", i, i + 1);
				}
			}
#endif
			if (strstr(cgs.adShaders[i], "trans")) {
				// ex: beyondreality
				cgs.transAds[i] = qtrue;
			}
		}
	}

	if (cg.hyperspace) {
		return;
	}

	vt = cgs.adverts;

	r = SC_Cvar_Get_Int("r_singleShader");
	if (r == 1  ||  r == 2) {
		useSingleShader = qtrue;
	} else {
		useSingleShader = qfalse;
	}

	for (i = 0;  i < cgs.numAds;  i++) {
		qhandle_t shader;
		int width, height;
		float scale;

		shaderOverride = qfalse;
		if (cg_adShaderOverride.integer) {
			shaderName = SC_Cvar_Get_String(va("cg_adShader%d", i + 1));
			if (*shaderName  &&  !Q_stricmp(shaderName, BlackListedShader)) {
				shaderName = "";
			} else {
				if (*shaderName) {
					shaderOverride = qtrue;
				}
			}
		} else {
			shaderName = "";
		}

		verts[3].xyz[0] = vt[i * 16 + 0];
		verts[3].xyz[1] = vt[i * 16 + 1];
		verts[3].xyz[2] = vt[i * 16 + 2];

		verts[2].xyz[0] = vt[i * 16 + 3];
		verts[2].xyz[1] = vt[i * 16 + 4];
		verts[2].xyz[2] = vt[i * 16 + 5];

		verts[1].xyz[0] = vt[i * 16 + 6];
		verts[1].xyz[1] = vt[i * 16 + 7];
		verts[1].xyz[2] = vt[i * 16 + 8];

		verts[0].xyz[0] = vt[i * 16 + 9];
		verts[0].xyz[1] = vt[i * 16 + 10];
		verts[0].xyz[2] = vt[i * 16 + 11];

		//normal[0] = vt[i * 16 + 12];
		//normal[1] = vt[i * 16 + 13];
		//normal[2] = vt[i * 16 + 14];

		//lightmap = vt[i * 16 + 14];
		//cellId = vt[i * 16 + 15];

		width = Distance(verts[3].xyz, verts[0].xyz);
		height = Distance(verts[3].xyz, verts[2].xyz);

		scale = (float)width / (float)height;

		//Com_Printf("ad %d  %d x %d  (%f)\n", i + 1, width, height, scale);

		//FIXME lookup when the 2x1 will fail to scale down (how close to 1.0? )
		// qzdm18  128x120

		//scale = 0.5f;

		if (scale < 0.3f) {  // 0.5
			shader = cgs.media.adboxblack;
			scaleGuess = 0;
		} else if (scale >= 9.0f) {
			shader = cgs.media.adboxblack;
			scaleGuess = 0;
		} else if (scale > 7.0f) {
			shader = cgs.media.adbox8x1;
			scaleGuess = 8;
		} else if (scale > 3.0f) {
			shader = cgs.media.adbox4x1;
			scaleGuess = 4;
		} else if (scale > 1.5f) {
			shader = cgs.media.adbox2x1;
			scaleGuess = 2;
		} else if (scale > 0.75) {
			shader = cgs.media.adbox1x1;
			scaleGuess = 1;
			//Com_Printf("using 1x1\n");
		} else if (scale > 0.25) {
			// ex: blackcathedral
			shader = cgs.media.adbox2x1;
			scaleGuess = 2;
			//Com_Printf("add scale > 0.25  &&  scale < 0.75\n");
			turn90 = qtrue;
		} else {
			shader = cgs.media.adboxblack;
			scaleGuess = 0;
		}

		//Com_Printf("ad %d  shader %d\n", i + 1, shader);

		//FIXME transparent ads that arent 2x1, haven't seen any
		if (cgs.transAds[i]) {
			if (scaleGuess == 2) {
				shader = cgs.media.adbox2x1_trans;
				//Com_Printf("using trans\n");
			}
		}

		// testing
		//FIXME why are they greyish?  lightmap?
		//shader = trap_R_RegisterShader(cgs.adShaders[i]);

		if (shaderOverride) {
		//if (*shaderName) {
			shader = trap_R_RegisterShader(shaderName);
			if (!shader) {
				Q_strncpyz(BlackListedShader, shaderName, sizeof(BlackListedShader));
			}
		}

		if (useSingleShader) {
			shader = trap_R_GetSingleShader();
		}

		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;

		verts[0].st[0] = 0;  //0;  //0;
		verts[0].st[1] = 1;  //0;  //1;

		verts[1].st[0] = 0;  //1;  //1;
		verts[1].st[1] = 0;  //0;  //1;

		verts[2].st[0] = 1;  //1;  //1;
		verts[2].st[1] = 0;  //1;  //0;

		verts[3].st[0] = 1;  //0;  //0;
		verts[3].st[1] = 1;  //1;  //0;

		//FIXME maybe yes for useSingleShader?
		if (!shaderOverride  &&  !useSingleShader) {
			if (turn90) {
				verts[0].st[0] = 0;  //1;  //0;
				verts[0].st[1] = 0;  //1;  //1;

				verts[1].st[0] = 1;  //0;  //1;
				verts[1].st[1] = 0;  //1;  //1;

				verts[2].st[0] = 1;  //0;  //1;
				verts[2].st[1] = 1;  //0;  //0;

				verts[3].st[0] = 0;  //1;  //0;
				verts[3].st[1] = 1;  //0;  //0;

			}
		}

				//FIXME no clue what I'm doing
		if (verts[0].xyz[2] > verts[1].xyz[2]) {
			// ex:  ad 3 blackcathedral
			//Com_Printf("flip ad %i\n", i + 1);
			verts[0].st[0] = 1;
			verts[0].st[1] = 0;

			verts[1].st[0] = 1;
			verts[1].st[1] = 1;  //1;

			verts[2].st[0] = 0;  //1;
			verts[2].st[1] = 1;  //1;

			verts[3].st[0] = 0;  //1;
			verts[3].st[1] = 0;
		}



		//Com_Printf("advert: %d\n", (int)cellId);

		//lightmap = SC_Cvar_Get_Int("adlightmap");

		//shader = cellId;
		//trap_R_AddPolyToScene(shader, 4, verts, 0);
		//shader = trap_R_RegisterShaderLightMap(cgs.adShaders[i], lightmap);
		//shader = trap_R_RegisterShader(cgs.adShaders[i]);

		//trap_R_AddPolyToScene(shader, 4, verts, lightmap);  //lightmap);  //lightmap);
		trap_R_AddPolyToScene(shader, 4, verts, 0);  //lightmap);  //lightmap);

		if (cg_debugAds.integer) {
			byte cl[4];

			cl[0] = 0;
			cl[1] = 0;
			cl[2] = 255;
			cl[3] = 255;

			CG_FloatNumber(i + 1, verts[0].xyz, RF_DEPTHHACK, NULL, 1.0);
			for (j = 0;  j < 4;  j++) {
				CG_FloatNumber(j, verts[j].xyz, 0, cl, 1.0);
			}
		}
#if 0
		{
			refEntity_t ent;
			qhandle_t shader;

			memset( &ent, 0, sizeof( ent ) );
			ent.origin[0] = verts[0].xyz[0];
			ent.origin[1] = verts[0].xyz[1];
			ent.origin[2] = verts[0].xyz[2];

			ent.reType = RT_SPRITE;

		/*

this works ok  qzdm6

LoadEntities()  ambient : 2
LoadEntities()  _minvertexlight : 25
LoadEntities()  angle : 360
LoadEntities()  message : Campgrounds Redux
LoadEntities()  classname : worldspawn
LoadEntities()  music : music/fla22k_02.wav
LoadEntities()  loadingContentCellId : 12
LoadEntities()  loadingContentLoc : 0 0
LoadEntities()  loadingContentDim : 256 128
Advertisement 1:
  cellId: 5
  normal: 0.000000 -1.000000 0.000000
  rect[0]:  -448.000000 -57.000000 456.000000
  rect[1]:  -448.000000 -57.000000 584.000000
  rect[2]:  -704.000000 -57.000000 584.000000
  rect[3]:  -704.000000 -57.000000 456.000000
  model:  *4
Advertisement 2:
  cellId: 7
  normal: 0.000000 -1.000000 0.000000
  rect[0]:  -1344.000000 500.000000 592.000000
  rect[1]:  -1344.000000 500.000000 720.000000
  rect[2]:  -1600.000000 500.000000 720.000000
  rect[3]:  -1600.000000 500.000000 592.000000
  model:  *5
Advertisement 3:
  cellId: 6
  normal: 1.000000 0.000000 0.000000
  rect[0]:  -894.000000 -160.000000 64.000000
  rect[1]:  -894.000000 -160.000000 176.000000
  rect[2]:  -894.000000 -384.000000 176.000000
  rect[3]:  -894.000000 -384.000000 64.000000
  model:  *6
Advertisement 4:
  cellId: 10
  normal: 0.000000 1.000000 0.000000
  rect[0]:  127.999985 -1404.000000 248.000000
  rect[1]:  127.999985 -1404.000000 376.000000
  rect[2]:  384.000000 -1404.000000 376.000000
  rect[3]:  384.000000 -1404.000000 248.000000
  model:  *7
Advertisement 5:
  cellId: 8
  normal: 0.000000 -0.894427 -0.447214
  rect[0]:  352.000000 -960.000000 384.000000
  rect[1]:  352.000000 -1016.000000 496.000000
  rect[2]:  128.000000 -1016.000000 496.000000
  rect[3]:  128.000000 -960.000000 384.000000
  model:  *8
Advertisement 6:
  cellId: 9
  normal: 0.000000 0.894427 -0.447214
  rect[0]:  128.000000 -704.000000 384.000000
  rect[1]:  128.000000 -648.000000 496.000000
  rect[2]:  352.000000 -648.000000 496.000000
  rect[3]:  352.000000 -704.000000 384.000000
  model:  *9
		*/

		/*
this doesn't work  5,6 at least  qzteam4

LoadEntities()  _minvertexlight : 25
LoadEntities()  music : music\fla_mp02.wav
LoadEntities()  message : Scornforge
LoadEntities()  classname : worldspawn
LoadEntities()  ambient : 3
LoadEntities()  angle : 315
LoadEntities()  loadingContentCellId : 187
LoadEntities()  loadingContentLoc : 0 0
LoadEntities()  loadingContentDim : 256 128
Advertisement 1:
  cellId: 273
  normal: -0.748405 0.663242 0.000000
  rect[0]:  368.413910 -241.151688 168.000000
  rect[1]:  368.413910 -241.151688 0.000000
  rect[2]:  143.586075 -494.848328 0.000000
  rect[3]:  143.586075 -494.848328 168.000000
  model:  *28
Advertisement 2:
  cellId: 273
  normal: 0.748405 -0.663242 -0.000000
  rect[0]:  369.413910 -242.151688 0.000000
  rect[1]:  369.413910 -242.151688 168.000000
  rect[2]:  144.586075 -495.848328 168.000000
  rect[3]:  144.586075 -495.848328 0.000000
  model:  *29
Advertisement 3:
  cellId: 272
  normal: -0.748405 0.663242 -0.000000
  rect[0]:  -207.586090 302.848328 168.000000
  rect[1]:  -207.586090 302.848328 0.000000
  rect[2]:  -432.413940 49.151672 0.000000
  rect[3]:  -432.413940 49.151672 168.000000
  model:  *30
Advertisement 4:
  cellId: 272
  normal: 0.748405 -0.663242 -0.000000
  rect[0]:  -206.586090 301.848328 0.000000
  rect[1]:  -206.586090 301.848328 168.000000
  rect[2]:  -431.413940 48.151672 168.000000
  rect[3]:  -431.413940 48.151672 0.000000
  model:  *31
Advertisement 5:
  cellId: 183
  normal: -1.000000 0.000000 0.000000
  rect[0]:  -1424.000244 2592.000000 368.000000
  rect[1]:  -1424.000244 2592.000000 112.000000
  rect[2]:  -1424.000244 2080.000000 112.000000
  rect[3]:  -1424.000244 2080.000000 368.000000
  model:  *32
Advertisement 6:
  cellId: 184
  normal: 0.000000 1.000000 0.000000
  rect[0]:  -2208.000000 1296.000244 368.000000
  rect[1]:  -2208.000000 1296.000244 112.000000
  rect[2]:  -2720.000000 1296.000244 112.000000
  rect[3]:  -2720.000000 1296.000244 368.000000
  model:  *33
Advertisement 7:
  cellId: 185
  normal: 1.000000 0.000000 0.000000
  rect[0]:  1360.000244 -2784.000000 368.000000
  rect[1]:  1360.000244 -2784.000000 112.000000
  rect[2]:  1360.000244 -2272.000000 112.000000
  rect[3]:  1360.000244 -2272.000000 368.000000
  model:  *34
Advertisement 8:
  cellId: 186
  normal: 0.000000 -1.000000 0.000000
  rect[0]:  2144.000000 -1488.000244 368.000000
  rect[1]:  2144.000000 -1488.000244 112.000000
  rect[2]:  2656.000000 -1488.000244 112.000000
  rect[3]:  2656.000000 -1488.000244 368.000000
  model:  *35

		*/

#if 0
			if (i < 9) {
				shader = cgs.media.numberShaders[i + 1];
			} else {
				shader = cgs.media.connectionShader;
			}
#endif
			shader = cgs.media.numberShaders[(i + 1) % 10];
			ent.customShader = shader;
			ent.radius = 30;
			ent.renderfx = RF_DEPTHHACK;
			if ((i + 1) >= 10) {
				ent.shaderRGBA[0] = 0;
			} else {
				ent.shaderRGBA[0] = 255;
			}
			if ((i + 1) >= 20) {
				ent.shaderRGBA[1] = 0;
			} else {
				ent.shaderRGBA[1] = 255;
			}
			if ((i + 1) >= 30) {
				ent.shaderRGBA[2] = 0;
			} else {
				ent.shaderRGBA[2] = 255;
			}
			ent.shaderRGBA[3] = 255;
			CG_AddRefEntity(&ent);
		}
#endif
	}
}

static void CG_DrawDecals (void)
{
	int i;
	const char *st = "";
	vec3_t origin;
	vec3_t frontPoint;
	qhandle_t shader;
	char shaderName[MAX_QPATH];
	trace_t trace;
	float orientation;
	float r, g, b, a;
	float radius;
	vec3_t dir;

	i = 1;
	while (1) {
		st = SC_Cvar_Get_String(va("cg_decal%d", i));
		if (!*st) {
			break;
		}

		sscanf(st, "%f %f %f %f %f %f %f %f %f %f %f %f %63s", &origin[0], &origin[1], &origin[2], &frontPoint[0], &frontPoint[1], &frontPoint[2], &orientation, &r, &g, &b, &a, &radius, shaderName);  // FIXME 63 MAX_QPATH
		shader = trap_R_RegisterShader(shaderName);

		VectorSubtract(origin, frontPoint, dir);
		VectorMA(origin, 131072, dir, origin);

		CG_Trace(&trace, frontPoint, NULL, NULL, origin, 0, MASK_SOLID);

		CG_ImpactMark(shader, trace.endpos, trace.plane.normal, orientation, r, g, b, a, qfalse, radius, qtrue, qfalse, qfalse);
		//CG_ImpactMark(shader, trace.endpos, trace.plane.normal, orientation, r, g, b, a, qfalse, radius, qfalse, qfalse);
		i++;
	}
}

static void CG_ForceBModels (void)
{
	int i;
	const char *st = NULL;
	refEntity_t ent;
	int modelindex;
	vec3_t lerpAngles = { 0, 0, 0 };
	int ret;

	i = 1;
	while (1) {
		st = SC_Cvar_Get_String(va("cg_forceBModel%d", i));
		if (!*st) {
			break;
		}

		modelindex = SC_Cvar_Get_Int(va("cg_forceBModel%d", i));
		memset(&ent, 0, sizeof(ent));
		AnglesToAxis(lerpAngles, ent.axis );
		ent.renderfx = RF_NOSHADOW;

		// flicker between two skins (FIXME?)
		ent.skinNum = (cg.time >> 6) & 1;

		ent.hModel = cgs.inlineDrawModel[modelindex];
		st = SC_Cvar_Get_String(va("cg_forceBModelPosition%d", i));
		if (*st) {
			vec3_t angles;

			VectorSet(ent.origin, 0, 0, 0);
			VectorSet(angles, 0, 0, 0);
			ret = sscanf(st, "%f %f %f %f %f %f", &ent.origin[0], &ent.origin[1], &ent.origin[2], &angles[0], &angles[1], &angles[2]);
			if (ret <= 0) {
				Com_Printf("^3invalid string for cg_forceBModelPosition%d\n", i);
			}

			VectorCopy(ent.origin, ent.oldorigin);
			AnglesToAxis(angles, ent.axis);
		}
		st = SC_Cvar_Get_String(va("cg_forceBModelTrajectory%d", i));
		if (*st) {
			trajectory_t tr;

			memset(&tr, 0, sizeof(tr));

			sscanf(st, "%d %d %d %f %f %f", (int *)&tr.trType, &tr.trTime, &tr.trDuration, &tr.trDelta[0], &tr.trDelta[1], &tr.trDelta[2]);
			VectorCopy(ent.origin, tr.trBase);
			BG_EvaluateTrajectoryf(&tr, cg.time, ent.origin, cg.foverf);
			VectorCopy(ent.origin, ent.oldorigin);
		}
		CG_AddRefEntity(&ent);
		//Com_Printf("adding model %d\n", modelindex);

		i++;
	}
}

static void CG_DrawSpawns (void)
{
	int i;
	refEntity_t ent;
	qhandle_t shader;
	spawnPoint_t *sp;
	byte bcolor[4];
	int count;

	if (cg_drawSpawns.integer == 0) {
		return;
	}

	bcolor[0] = bcolor[1] = bcolor[2] = bcolor[3] = 255;

	count = 0;
	for (i = 0;  i < cg.numSpawnPoints;  i++) {
		sp = &cg.spawnPoints[i];

#if 0
		if (cgs.gametype >= GT_CA) {  //FIXME is this right?
			if (sp->blueSpawn == qfalse  &&  sp->redSpawn == qfalse) {
				continue;
			}
		} else {
			if (sp->blueSpawn == qtrue  ||  sp->redSpawn == qtrue) {
				continue;
			}
		}
#endif

		memset( &ent, 0, sizeof( ent ) );
		VectorCopy(sp->origin, ent.origin);

		ent.reType = RT_SPRITE;

#if 0
		if (count < 9) {
			shader = cgs.media.numberShaders[count + 1];
		} else {
			shader = cgs.media.connectionShader;
		}
#endif
		shader = cgs.media.numberShaders[(count + 1) % 10];
		ent.customShader = shader;
		ent.radius = 30;
		if (cg_drawSpawns.integer == 2) {
			ent.renderfx = RF_DEPTHHACK;
		}

		if ((count + 1) >= 10) {
			ent.shaderRGBA[0] = 0;
		} else {
			ent.shaderRGBA[0] = 255;
		}
		if ((count + 1) >= 20) {
			ent.shaderRGBA[1] = 0;
		} else {
			ent.shaderRGBA[1] = 255;
		}
		if ((count + 1) >= 30) {
			ent.shaderRGBA[2] = 0;
		} else {
			ent.shaderRGBA[2] = 255;
		}

		ent.shaderRGBA[3] = 255;

		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 0;
		ent.shaderRGBA[2] = 0;

		//if (cgs.gametype == GT_TOURNAMENT) {
		if (cgs.gametype <= GT_TEAM) {  //  ||  cgs.gametype == GT_FREEZETAG) {
			if (sp->spawnflags & 0x1) {
				if (cg_drawSpawnsInitial.integer) {
					ent.shaderRGBA[1] = 255;
					ent.origin[2] += cg_drawSpawnsInitialZ.value;
				} else {
					continue;
				}
			} else {
				if (cg_drawSpawnsRespawns.integer) {
					ent.origin[2] += cg_drawSpawnsRespawnsZ.value;
				} else {
					continue;
				}
			}
		} else if (cgs.gametype > GT_TEAM) {

			if (sp->deathmatch) {  // shared spawn
				if (!cg_drawSpawnsShared.integer) {
					continue;
				}
				ent.shaderRGBA[0] = 255;
				if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
					ent.shaderRGBA[1] = 0;
					ent.shaderRGBA[2] = 0;
				} else {
					ent.shaderRGBA[1] = 255;
					ent.shaderRGBA[2] = 255;
				}
				ent.origin[2] += cg_drawSpawnsSharedZ.value;
				//Com_Printf("shared %d\n", i);
				//CG_FloatSprite(cgs.media.balloonShader, ent.origin, 0, bcolor, 20);
			} else if (sp->redSpawn) {
				ent.shaderRGBA[0] = 100;
				if (sp->initial) {
					if (!cg_drawSpawnsInitial.integer) {
						continue;
					}
					ent.shaderRGBA[0] = 255;
					ent.shaderRGBA[1] = 100;
					ent.shaderRGBA[2] = 255;
					//CG_FloatSprite(cgs.media.botSkillShaders[4], ent.origin, 0, bcolor, 20);
					//CG_FloatSprite(cgs.media.balloonShader, ent.origin, 0, bcolor, 20);
					//ent.radius = 200;
					//ent.origin[2] += 50;
					ent.origin[2] += cg_drawSpawnsInitialZ.value;
				} else {
					if (!cg_drawSpawnsRespawns.integer) {
						continue;
					}
					ent.shaderRGBA[1] = 0;
					ent.shaderRGBA[2] = 0;
					ent.origin[2] += cg_drawSpawnsRespawnsZ.value;
				}
				//Com_Printf("red\n");
			} else if (sp->blueSpawn) {
				ent.shaderRGBA[2] = 255;
				if (sp->initial) {
					if (!cg_drawSpawnsInitial.integer) {
						continue;
					}
					//FIXME
					ent.shaderRGBA[2] = 100;
					ent.shaderRGBA[0] = 50; //125;
					ent.shaderRGBA[1] = 255; //150;  //125;
					//Com_Printf("initial\n");
					//ent.origin[2] += 50;
					ent.origin[2] += cg_drawSpawnsInitialZ.value;
				} else {
					if (!cg_drawSpawnsRespawns.integer) {
						continue;
					}
					ent.shaderRGBA[0] = 0;
					ent.shaderRGBA[1] = 150;
					//Com_Printf("reg\n");
					ent.origin[2] += cg_drawSpawnsRespawnsZ.value;
				}
				//Com_Printf("blue\n");
			} else {  //FIXME unused spawn for team games?
#if 0
				ent.shaderRGBA[0] = 255;
				ent.shaderRGBA[1] = 255;
				ent.shaderRGBA[2] = 255;
#endif
				continue;
			}
		}

		if (SC_Cvar_Get_Int("wolfcam_spawncolor")) {
			vmCvar_t v;
			v.integer = SC_Cvar_Get_Int("wolfcam_spawncolor");
			v.value = v.integer;
			//Com_Printf("color %f\n", v.value);
			SC_ByteVec3ColorFromCvar(ent.shaderRGBA, &v);
			ent.shaderRGBA[3] = 255;
		}

		ent.reType = RT_MODEL;
		ent.hModel = cgs.media.teleportEffectModel;
		AxisClear(ent.axis);
		ent.origin[2] += 16;
		ent.customShader = 0;

		CG_AddRefEntity(&ent);

		count++;
	}
}

#if 0  // old
static void CG_PlayPath (void)
{
		int len;
		int nve;
		float f;
		vec3_t o, no;
		vec3_t a, na;
		int i;
		const char *s = NULL;
		int x;

		if (cg.snap->serverTime < cg.pathCurrentServerTime) {
			return;   // wait for game to catch up
		}

		if (cg.playPathCount == 0  &&  cg.snap->serverTime > cg.pathCurrentServerTime) {
			return;  // wait for rewind command to be issued
		}

		if (cg.snap->serverTime > cg.pathNextServerTime) {
			//Com_Printf("FIXME cg.snap->serverTime > cg.pathNextServerTime\n");
			while (cg.snap->serverTime > cg.pathNextServerTime) {
				s = CG_FS_ReadLine(cg.recordPathFile, &len);
				if (!s  ||  !*s  ||  len == 0) {
					trap_FS_FCloseFile(cg.recordPathFile);
					cg.playPath = qfalse;
					Com_Printf("done playing path\n");
					trap_SendConsoleCommand("echopopup done playing path\n");
					return;
				}
				sscanf(s, "%d %f %f %f %f %f %f %d", &cg.pathNextServerTime, &cg.pathNextOrigin[0], &cg.pathNextOrigin[1], &cg.pathNextOrigin[2], &cg.pathNextAngles[0], &cg.pathNextAngles[1], &cg.pathNextAngles[2], &nve);  //FIXME nve
			}
		} else if (cg.snap->serverTime == cg.pathNextServerTime) {
			VectorCopy(cg.pathNextOrigin, cg.pathCurrentOrigin);
			VectorCopy(cg.pathNextAngles, cg.pathCurrentAngles);
			cg.pathCurrentServerTime = cg.pathNextServerTime;
			for (i = 0;  i < cg.playPathSkipNum;  i++) {
				s = CG_FS_ReadLine(cg.recordPathFile, &len);
			}
			if (!s  ||  !*s  ||  len == 0) {
				trap_FS_FCloseFile(cg.recordPathFile);
				cg.playPath = qfalse;
				Com_Printf("done playing path\n");
				trap_SendConsoleCommand("echopopup done playing path\n");
				return;
			}
			sscanf(s, "%d %f %f %f %f %f %f %d", &cg.pathNextServerTime, &cg.pathNextOrigin[0], &cg.pathNextOrigin[1], &cg.pathNextOrigin[2], &cg.pathNextAngles[0], &cg.pathNextAngles[1], &cg.pathNextAngles[2], &nve);  //FIXME nve
		}

		// interp
		f = (float)(cg.time - cg.pathCurrentServerTime) / (float)(cg.pathNextServerTime - cg.pathCurrentServerTime);
		VectorCopy(cg.pathCurrentOrigin, o);
		VectorCopy(cg.pathCurrentAngles, a);
		VectorCopy(cg.pathNextOrigin, no);
		VectorCopy(cg.pathNextAngles, na);

		cg.refdef.vieworg[0] = o[0] + f * (no[0] - o[0]);
		cg.refdef.vieworg[1] = o[1] + f * (no[1] - o[1]);
		cg.refdef.vieworg[2] = o[2] + f * (no[2] - o[2]);

		//FIXME check DEFAULT_VIEWHEIGHT
		cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;   //FIXME should be recorded

		cg.refdefViewAngles[0] = LerpAngle(a[0], na[0], f);
		cg.refdefViewAngles[1] = LerpAngle(a[1], na[1], f);
		cg.refdefViewAngles[2] = LerpAngle(a[2], na[2], f);
		//Com_Printf("^4%f  (%f %f %f)\n", f, cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2]);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			cg.refdef.fov_x = cg.demoFov;
		} else {
			cg.refdef.fov_x = cg_fov.value;
		}

		if ( cg.refdef.fov_x < 1 ) {
			cg.refdef.fov_x = 1;
		}

		CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

		cg.refdef.time = cg.time;  //cg.realTime;  //cg.time;
		trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);

		cg.playPathCount++;
		return;
}
#endif

static void CG_PlayPath (void)
{
		int len;
		int nve;
		float f;
		vec3_t o, no;
		vec3_t a, na;
		int i;
		const char *s = NULL;
		int skipNum;

		if (cg.paused) {
			return;
		}

		if (cg.ftime < cg.pathCurrentTime) {
			//Com_Printf("waiting ...  %f < %f\n", cg.ftime, cg.pathCurrentTime);
			return;   // wait for game to catch up
		}

#if 0
		if (cg.playPathCount == 0  &&  cg.ftime > cg.pathCurrentTime) {
			Com_Printf("^3waiting to rewind\n");
			return;  // wait for rewind command to be issued
		}
#endif

		skipNum = cg_pathSkipNum.integer;
		if (skipNum <= 0) {
			skipNum = 1;
		}

		if (cg.ftime >= cg.pathNextTime) {
			//Com_Printf("FIXME cg.snap->serverTime > cg.pathNextServerTime\n");
#if 1
			VectorCopy(cg.pathNextOrigin, cg.pathCurrentOrigin);
			VectorCopy(cg.pathNextAngles, cg.pathCurrentAngles);
			cg.pathCurrentTime = cg.pathNextTime;
#endif
			while (cg.ftime >= cg.pathNextTime) {
				//s = CG_FS_ReadLine(cg.recordPathFile, &len);
				for (i = 0;  i < skipNum;  i++) {
					s = CG_FS_ReadLine(cg.recordPathFile, &len);
				}
				if (!s  ||  !*s  ||  len == 0) {
					trap_FS_FCloseFile(cg.recordPathFile);
					cg.playPath = qfalse;
					Com_Printf("done playing path\n");
					trap_SendConsoleCommand("echopopup done playing path\n");
					return;
				}

#if 0
				VectorCopy(cg.pathNextOrigin, cg.pathCurrentOrigin);
				VectorCopy(cg.pathNextAngles, cg.pathCurrentAngles);
				cg.pathCurrentTime = cg.pathNextTime;
#endif

				sscanf(s, "%lf %f %f %f %f %f %f %d", &cg.pathNextTime, &cg.pathNextOrigin[0], &cg.pathNextOrigin[1], &cg.pathNextOrigin[2], &cg.pathNextAngles[0], &cg.pathNextAngles[1], &cg.pathNextAngles[2], &nve);  //FIXME nve
				//Com_Printf("^3%f\n", cg.pathNextTime);
			}
			//Com_Printf("new point %f -> %f\n", cg.pathCurrentTime, cg.pathNextTime);
		}

		// interp
		f = (float)(cg.ftime - cg.pathCurrentTime) / (float)(cg.pathNextTime - cg.pathCurrentTime);
		if (f > 1.0) {
			//Com_Printf("^1xxx f > 1.0\n");
			f = 1;
		}

		VectorCopy(cg.pathCurrentOrigin, o);
		VectorCopy(cg.pathCurrentAngles, a);
		VectorCopy(cg.pathNextOrigin, no);
		VectorCopy(cg.pathNextAngles, na);

		cg.refdef.vieworg[0] = o[0] + f * (no[0] - o[0]);
		cg.refdef.vieworg[1] = o[1] + f * (no[1] - o[1]);
		cg.refdef.vieworg[2] = o[2] + f * (no[2] - o[2]);

		//FIXME check DEFAULT_VIEWHEIGHT
		cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;   //FIXME should be recorded

		cg.refdefViewAngles[0] = LerpAngle(a[0], na[0], f);
		cg.refdefViewAngles[1] = LerpAngle(a[1], na[1], f);
		cg.refdefViewAngles[2] = LerpAngle(a[2], na[2], f);
		//Com_Printf("^4%f  (%f %f %f)\n", f, cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2]);

		//Com_Printf("(%f %f %f)  %f:  (%f %f %f)   -> (%f %f %f)\n", cg.pathCurrentOrigin[0], cg.pathCurrentOrigin[1], cg.pathCurrentOrigin[2], f, cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2], cg.pathNextOrigin[0], cg.pathNextOrigin[1], cg.pathNextOrigin[2]);


		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			cg.refdef.fov_x = cg.demoFov;
		} else {
			cg.refdef.fov_x = cg_fov.value;
		}

		if ( cg.refdef.fov_x < 1 ) {
			cg.refdef.fov_x = 1;
		}

		CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

		cg.refdef.time = cg.time;  //cg.realTime;  //cg.time;
		trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);

		//cg.playPathCount++;
		//Com_Printf("^5 %d\n", cg.playPathCount);

		return;
}

float CG_CalcSpline (int step, float tension)
{
	float out;

	trap_CalcSpline(step, tension, &out);
	return out;
}

#if 0
static float CosineInterp (float y1, float y2, double t)
{
	double f;

	f = (1.0 - cos(M_PI * t)) / 2.0;
	return (y1 * (1.0 - f) + y2 * f);
}
#endif

#if 0
static float LerpAngle2 (float from, float to, float frac)
{
	float a;
	float t, f;
	float d;

	//return from;

	return LerpAngle(from, to, frac);

	//f = AngleNormalize180(from);
	//t = AngleNormalize180(to);

	//d = CG_CameraAngleLongestDistanceNoRoll(from, to);
	d = fabs(AngleSubtract(from, to));
	Com_Printf("lerpangle2:  %f\n", d);

#if 0
	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);
#endif

	if ( to + from > 180 ) {
		to -= 360;
	}
	if ( to + from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}
#endif

// from q3mme
static centity_t *demoTargetEntity( int num ) {
	if (num <0 || num >= (MAX_GENTITIES -1) )
		return 0;
	if (num == cg.snap->ps.clientNum)
		return &cg.predictedPlayerEntity;
	if (cg_entities[num].currentValid)
		return &cg_entities[num];
	return 0;
}

// from q3mme
static void chaseEntityOrigin( centity_t *cent, vec3_t origin ) {
	VectorCopy( cent->lerpOrigin, origin );
	switch(cent->currentState.eType) {
	case ET_PLAYER:
		origin[2] += DEFAULT_VIEWHEIGHT;
		break;
	}
}

static qboolean CG_PlayQ3mmeCamera (void)
{
	//int i;
	vec3_t refOriginOrig, refAnglesOrig;
	float refFovxOrig;
	float refFovyOrig;
	vec3_t velocityOrig;
	demoCameraPoint_t *last, *next;
	//int time;
	//float timeFraction;
	qboolean cameraDebugPath;
	float fov;
	centity_t *targetCent;
	
	last = demo.camera.points;

	if (!last) {
		return qfalse;
	}

	while (last->next) {
		next = last->next;
		last = next;
	}

	if (cg_cameraDebugPath.integer) {
		cameraDebugPath = qtrue;
	} else {
		cameraDebugPath = qfalse;
	}

	//Com_Printf("cg.cameraQ3mmePlaying %d\n", cg.cameraQ3mmePlaying);
	
	//FIXME right place for cameraque ?
	//if (cg.cameraQ3mme  &&  cg_cameraQue.integer) {
	if (cg.cameraQ3mmePlaying  ||  (cg_cameraQue.integer  &&  cg.time <= last->time)) {
		int time;
		float timeFraction;

		// ahahhh, seek hasn't taken effect yet so this will always be set so cg.time might be greater

		if (cg.playQ3mmeCameraCommandIssued) {
#if 0
			// race, they got deleted
			if (!demo.camera.points) {
				cg.playQ3mmeCameraCommandIssued = qfalse;
				cg.cameraQ3mmePlaying = qfalse;
				return qfalse;
			}

			if (cg.time > demo.camera.points->time) {
				// still not synced
				//Com_Printf("waiting to sync q3mme camera  %d  ->  %d\n", cg.time, demo.camera.points->time);
				return qfalse;
			} else {
				// no synced
				cg.playQ3mmeCameraCommandIssued = qfalse;
			}
#endif

			//Com_Printf("waiting for seek\n");
			cg.playQ3mmeCameraCommandIssued = qfalse;
			return qfalse;
		}
		if (cg.time > last->time) {
			//Com_Printf("q3mme camera cg.time > last->time\n");
			cg.cameraQ3mmePlaying = qfalse;
			return qfalse;
		}

		if (cameraDebugPath) {
			VectorCopy(cg.refdef.vieworg, refOriginOrig);
			VectorCopy(cg.refdefViewAngles, refAnglesOrig);
			refFovxOrig = cg.refdef.fov_x;
			refFovyOrig = cg.refdef.fov_y;
			VectorCopy(cg.freecamPlayerState.velocity, velocityOrig);
		}

		//time = floor(cg.time);
		//timeFraction = cg.time - time;
		time = cg.time;
		timeFraction = (float)cg.foverf;

		CG_Q3mmeCameraOriginAt(time, timeFraction, cg.refdef.vieworg);
		CG_Q3mmeCameraAnglesAt(time, timeFraction, cg.refdefViewAngles);

		targetCent = demoTargetEntity(demo.camera.target);
		if (targetCent) {
			vec3_t targetOrigin, targetAngles;
			chaseEntityOrigin(targetCent, targetOrigin);
			VectorSubtract(targetOrigin, cg.refdef.vieworg, targetOrigin);
			vectoangles(targetOrigin, targetAngles);
			//demo.camera.angles[YAW] = targetAngles[YAW];
			//demo.camera.angles[PITCH] = targetAngles[PITCH];
			cg.refdefViewAngles[YAW] = targetAngles[YAW];
			cg.refdefViewAngles[PITCH] = targetAngles[PITCH];
		}

		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		//Com_Printf("angles: %f %f %f\n", cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2]);

		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.value;
		}

		if (!CG_Q3mmeCameraFovAt(time, timeFraction, &demo.camera.fov)) {
			demo.camera.fov = 0;
		} else {
			fov += demo.camera.fov;
		}

		cg.refdef.fov_x = fov;
		if (cg.refdef.fov_x < 1) {
			cg.refdef.fov_x = 1;
		}

		CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

		//Com_Printf("q3mme camera %f %f %f\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

		trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);


		if (cg_cameraUpdateFreeCam.integer  &&  !cg_cameraDebugPath.integer) {
			cg.fMoveTime = 0;
			VectorCopy(cg.refdef.vieworg, cg.fpos);
			cg.fpos[2] -= DEFAULT_VIEWHEIGHT;
			VectorCopy(cg.refdefViewAngles, cg.fang);
			cg.freecamSet = qtrue;
			//Com_Printf("xxx %f %f %f\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

#if 0  //FIXME
			if (cg.ftime >= lastPoint->cgtime) {
				VectorClear(cg.freecamPlayerState.velocity);
			} else if (cg.ftime > cg.positionSetTime) {
				for (i = 0;  i < 3;  i++) {
					cg.freecamPlayerState.velocity[i] = (cg.refdef.vieworg[i] - cg.cameraLastOrigin[i]) / ((cg.ftime - cg.positionSetTime) / 1000.0);
				}
			} else if (cg.ftime < cg.positionSetTime) {
				VectorSet(cg.freecamPlayerState.velocity, 0, 0, 0);
			}
#endif
		}

#if 0  //FIXME
		if (cg.ftime >= lastPoint->cgtime) {
			VectorClear(cg.cameraVelocity);
		} else if (cg.ftime > cg.positionSetTime) {
			for (i = 0;  i < 3;  i++) {
				cg.cameraVelocity[i] = (cg.refdef.vieworg[i] - cg.cameraLastOrigin[i]) / ((cg.ftime - cg.positionSetTime) / 1000.0);
			}
		} else if (cg.ftime < cg.positionSetTime) {
			VectorSet(cg.cameraVelocity, 0, 0, 0);
		}
#endif

#if 0  //FIXME
		if (cg.ftime != cg.positionSetTime) {
			cg.positionSetTime = cg.ftime;
			VectorCopy(cg.refdef.vieworg, cg.cameraLastOrigin);
		}
#endif

		if (cameraDebugPath) {
			refEntity_t ent;
			byte color[4] = { 255, 0, 0, 255 };
			vec3_t dir;
			vec3_t origin;

			memset(&ent, 0, sizeof(ent));
			VectorCopy(cg.refdef.vieworg, ent.origin);
			ent.reType = RT_MODEL;
			ent.hModel = cgs.media.teleportEffectModel;
			//ent.hModel = trap_R_RegisterModel("models/powerups/health/large_sphere.md3");
			//ent.hModel = trap_R_RegisterModel("models/powerups/health/mega_sphere.md3");
			AxisClear(ent.axis);
			//ent.origin[2] += 16;
			ent.radius = 300;
			ent.customShader = 0;
			ent.shaderRGBA[0] = 255;
			ent.shaderRGBA[1] = 0;
			ent.shaderRGBA[2] = 0;
			ent.shaderRGBA[3] = 255;
			ent.renderfx = RF_DEPTHHACK;

			CG_AddRefEntity(&ent);

			AngleVectors(cg.refdefViewAngles, dir, NULL, NULL);
			VectorCopy(ent.origin, origin);
			VectorMA(origin, 300, dir, origin);
			CG_SimpleRailTrail(ent.origin, origin, cg_railTrailTime.value, color);

			VectorCopy(refOriginOrig, cg.refdef.vieworg);
			VectorCopy(refAnglesOrig, cg.refdefViewAngles);
			AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
			cg.refdef.fov_x = refFovxOrig;
			cg.refdef.fov_y = refFovyOrig;
			VectorCopy(velocityOrig, cg.freecamPlayerState.velocity);

			cg.freecamSet = qtrue;
		}  // cameraDebugPath
		return qtrue;
	}  // q3mme camera playing

	return qfalse;
}


static qboolean CG_PlayCamera (void)
{
	cameraPoint_t *cp;
	const cameraPoint_t *cpnext;
	const cameraPoint_t *cpprev;
	const cameraPoint_t *lastPoint;
	const cameraPoint_t *cpOrig;
	const cameraPoint_t *cpnextOrig;
	const cameraPoint_t *cpprevOrig;
	double f;
	vec3_t o, a, no, na;
	//int x;
	const centity_t *cent;
	vec3_t dir;
	vec3_t origin;
	float fov, nfov;
	const cameraPoint_t *c1;
	const cameraPoint_t *c2;
	double roll, nroll;
	double totalDist;
	double totalTime;
	double accel;
	double dist;
	double t;
	const cameraPoint_t *pointNext;
	int i;
	qboolean foundMatch;
	vec3_t refOriginOrig;
	vec3_t refAnglesOrig;
	float refFovxOrig;
	float refFovyOrig;
	vec3_t velocityOrig;
	qboolean cameraDebugPath = qfalse;
	qboolean returnValue = qtrue;

	CG_PlayQ3mmeCamera();

	if (cg.cameraQ3mmePlaying  &&  !cg_cameraDebugPath.integer) {
		return qtrue;
	} else {
		// Keep going and also play regular camera if available.  This is
		// to allow debugging of both at the same time.
	}

	if (cg.numCameraPoints < 2) {
		cg.cameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		return qfalse;
	}

	if (cg.freecam) {
		VectorCopy(cg.fpos, cg.refdef.vieworg);
		//FIXME check DEFAULT_VIEWHEIGHT
		cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;

		VectorCopy(cg.fang, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
	}

	if (cg_cameraDebugPath.integer) {
		cameraDebugPath = qtrue;
	}

	if (cameraDebugPath) {
		VectorCopy(cg.refdef.vieworg, refOriginOrig);
		VectorCopy(cg.refdefViewAngles, refAnglesOrig);
		refFovxOrig = cg.refdef.fov_x;
		refFovyOrig = cg.refdef.fov_y;
		VectorCopy(cg.freecamPlayerState.velocity, velocityOrig);
	} else {
		// silence gcc warnings that they may be used unintialized, the
		// warning seems invalid and is triggered by the long function
		VectorSet(refOriginOrig, 0, 0, 0);
		VectorSet(refAnglesOrig, 0, 0, 0);
		refFovxOrig = 0;
		refFovyOrig = 0;
		VectorSet(velocityOrig, 0, 0, 0);
	}

//#if 1  // gcc warning

	lastPoint = &cg.cameraPoints[cg.numCameraPoints - 1];

	if (!cg.cameraPlaying  &&  cg_cameraQue.integer) {
		// check for last cam point being hit
		//
		if (cg.ftime == lastPoint->cgtime  ||  (cg.cameraPlayedLastFrame  &&  cg.ftime >= lastPoint->cgtime)) {
			if (cg_cameraQue.integer > 1  &&  *lastPoint->command  &&  lastPoint->cgtime != cg.cameraPointCommandTime) {
				trap_SendConsoleCommand(va("%s\n", lastPoint->command));
				cg.cameraPointCommandTime = lastPoint->cgtime;
			}
			cpnext = lastPoint;
			cp = &cg.cameraPoints[cg.numCameraPoints - 2];
			if (cg.numCameraPoints > 2) {
				cpprev = &cg.cameraPoints[cg.numCameraPoints - 3];
			} else {
				cpprev = NULL;
			}
			cg.currentCameraPoint = cg.numCameraPoints - 1;
		} else {
			for (i = 0;  i < cg.numCameraPoints - 1;  i++) {
				cp = &cg.cameraPoints[i];
				cpnext = &cg.cameraPoints[i + 1];

				foundMatch = qfalse;
				if (cg.ftime >= cp->cgtime  &&  cg.ftime < cpnext->cgtime) {
					foundMatch = qtrue;
					break;
				}
			}
			if (foundMatch == qfalse) {
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				return qtrue;
			}
			cg.currentCameraPoint = i;
			cpprev = NULL;
			if (cg.currentCameraPoint > 0) {
				cpprev = &cg.cameraPoints[cg.currentCameraPoint - 1];
			}
			//if (cg_cameraQue.integer > 1  &&  cg.ftime == cp->cgtime  &&  cg.ftime != cg.cameraPointCommandTime) {
			if (cg_cameraQue.integer > 1  &&  *cp->command  &&  cp->cgtime != cg.cameraPointCommandTime) {
				trap_SendConsoleCommand(va("%s\n", cp->command));
				cg.cameraPointCommandTime = cp->cgtime;
			}
		}
	} else {  // /playcamera
		// check for last cam point being hit
		//
		if (cg.ftime == lastPoint->cgtime  ||  (cg.cameraPlayedLastFrame  &&  cg.ftime >= lastPoint->cgtime)) {
			if (*lastPoint->command  &&  lastPoint->cgtime != cg.cameraPointCommandTime) {
				//trap_SendConsoleCommand(va("%s\n", lastPoint->command));
				//cg.cameraPointCommandTime = lastPoint->cgtime;
			}
			cpnext = lastPoint;
			cp = &cg.cameraPoints[cg.numCameraPoints - 2];
			if (cg.numCameraPoints > 2) {
				cpprev = &cg.cameraPoints[cg.numCameraPoints - 3];
			} else {
				cpprev = NULL;
			}
			//cg.currentCameraPoint = cg.numCameraPoints - 1;
			//Com_Printf("^3last point\n");
		} else {  // not last point

			cp = &cg.cameraPoints[cg.currentCameraPoint];
			cpnext = NULL;
			if (cg.currentCameraPoint < cg.numCameraPoints - 1) {
				cpnext = &cg.cameraPoints[cg.currentCameraPoint + 1];
			}
			cpprev = NULL;
			if (cg.currentCameraPoint > 0) {
				cpprev = &cg.cameraPoints[cg.currentCameraPoint - 1];
			}

			if (!cpnext) {
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				return qfalse;
			}
		}

		if (cg.playCameraCommandIssued) {
			cg.playCameraCommandIssued = qfalse;
			cg.cameraJustStarted = qtrue;
			if (1) {  //(cp->cgtime != cg.ftime) {
				double extraTime;

				cg.cameraWaitToSync = qtrue;
				extraTime = 1000.0 * cg_cameraRewindTime.value;
				if (extraTime < 0) {
					extraTime = 0;
				}
				trap_SendConsoleCommand(va("seekservertime %f\n", cp->cgtime - extraTime));
				cg.cameraPlayedLastFrame = qfalse;
				return qfalse;
			} else {
				cg.cameraWaitToSync = qfalse;
			}
		}

		if (cg.cameraWaitToSync) {
			if (cg.ftime >= cp->cgtime) {
				cg.cameraWaitToSync = qfalse;
				cg.cameraJustStarted = qtrue;
			} else {
				//Com_Printf("wait to sync.. (time) %f  != %f\n", cg.ftime, cp->cgtime);
				cg.cameraPlayedLastFrame = qfalse;
				return qfalse;
			}
		}

		if (cg.ftime < cp->cgtime) {
			if (cg.cameraWaitToSync) {
				cg.cameraPlayedLastFrame = qfalse;
				return qfalse;
			} else {
				Com_Printf("error camera not waiting to sync and cg.ftime < cp->cgtime\n");
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				return qfalse;
			}
		}

		if (cg.cameraJustStarted) {
			//Com_Printf("camera started current point %d  cg.ftime %f  cp->cgtime %f\n", cg.currentCameraPoint, cg.ftime, cp->cgtime);
			if (*cp->command) {
				trap_SendConsoleCommand(va("%s\n", cp->command));
			}
			cg.cameraJustStarted = qfalse;
		}

		if (cg.ftime >= cpnext->cgtime) {
			if (*cpnext->command) {
				trap_SendConsoleCommand(va("%s\n", cpnext->command));
			}

			if (cpnext == lastPoint) {
				cg.currentCameraPoint = cg.numCameraPoints - 1;
				cg.cameraPlaying = qfalse;
			} else {
				while (1) {
					cg.currentCameraPoint++;
					if (cg.currentCameraPoint >= cg.numCameraPoints - 1) {
						cg.cameraPlaying = qfalse;
						cg.cameraPlayedLastFrame = qfalse;
						//Com_Printf("done\n");
						return qfalse;
					}
					cp = &cg.cameraPoints[cg.currentCameraPoint];
					cpnext = &cg.cameraPoints[cg.currentCameraPoint + 1];

					if (cg.ftime < cpnext->cgtime) {
						break;
					}
				}
			}
			//Com_Printf("new camera: %d\n", cg.currentCameraPoint);
		}
	}

//#if 1  // gcc warning

	if (cg.ftime < lastPoint->cgtime) {
		cg.cameraPlayedLastFrame = qtrue;
	} else {
		cg.cameraPlayedLastFrame = qfalse;
	}

	if (!cg.freecam) {
		return qtrue;
	}

	VectorClear(cg.freecamPlayerState.velocity);

	// d = (Vf + Vi) / 2  * t
	// a = (Vf - Vi) / t
	// d = Vi * t + 1/2 * a * t^2

//#if 1  // gcc warning

	// origin

	// save these for later (angles, fov, roll, etc..)
	cpOrig = cp;
	cpnextOrig = cpnext;
	cpprevOrig = cpprev;

	// masking
	while (cp  &&  !(cp->flags & CAM_ORIGIN)) {
		cp = cp->prev;
	}
	while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
		cpprev = cpprev->prev;
	}
	while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
		cpnext = cpnext->next;
	}

	if (cp == NULL) {
		//Com_Printf("couldn't find cp\n");
		goto handleCameraAngles;
	}

	// only one camera point with origin
	if (cpnext == NULL) {
		//Com_Printf("couldn't find cpnext\n");
		VectorCopy(cp->origin, cg.refdef.vieworg);
		goto handleCameraAngles;
	}

	// cp and cpnext need to be valid at this point
	if (cp->type == CAMERA_JUMP) {
		VectorCopy(cp->origin, cg.refdef.vieworg);
	} else if (cp->type == CAMERA_INTERP) {
		f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (cpprev  &&  cpprev->type == CAMERA_SPLINE) {
			VectorCopy(cg.splinePoints[cp->splineStart], o);
		} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_BEZIER) {
			CG_CameraSplineOriginAt(cp->cgtime, posBezier, o);
		} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_CATMULLROM) {
			CG_CameraSplineOriginAt(cp->cgtime, posCatmullRom, o);
		} else {
			VectorCopy(cp->origin, o);
		}

		if (cpnext->type == CAMERA_SPLINE) {
			VectorCopy(cg.splinePoints[cpnext->splineStart], no);
		} else if (cpnext->type == CAMERA_SPLINE_BEZIER) {
			CG_CameraSplineOriginAt(cpnext->cgtime, posBezier, no);
		} else if (cpnext->type == CAMERA_SPLINE_CATMULLROM) {
			CG_CameraSplineOriginAt(cpnext->cgtime, posCatmullRom, no);
		} else {
			VectorCopy(cpnext->origin, no);
		}

		if (cp->useOriginVelocity) {
			totalDist = cp->originDistance;
			totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->originFinalVelocity - cp->originInitialVelocity) / totalTime;
			t = (cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->originInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
			//Com_Printf("f: %f  (%f -> %f) %f %f\n", f, cp->originInitialVelocity, cp->originFinalVelocity, cp->originAccel, cp->originThrust);
		}

		cg.refdef.vieworg[0] = o[0] + f * (no[0] - o[0]);
		cg.refdef.vieworg[1] = o[1] + f * (no[1] - o[1]);
		cg.refdef.vieworg[2] = o[2] + f * (no[2] - o[2]);
	} else if (cp->type == CAMERA_SPLINE) {
		int numSplines;
		double singleTime;
		int snum;
		double snumf;
		double remainder;

		numSplines = cpnext->splineStart - cp->splineStart;
		singleTime = (double)(cpnext->cgtime - cp->cgtime) / (double)numSplines;
		if (cp->useOriginVelocity) {
			totalDist = cp->originDistance;
			totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->originFinalVelocity - cp->originInitialVelocity) / totalTime;
			t = (double)(cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->originInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
			snumf = f * (double)(numSplines - 0);

			//Com_Printf("f: %f  (%f -> %f) %f (%f / %d)\n", f, cp->originInitialVelocity, cp->originFinalVelocity, cp->originAccel, snumf, numSplines);
		} else {
			if (cg.ftime < cpnext->cgtime) {
				snumf = (cg.ftime - cp->cgtime) / singleTime;
			} else {
				snumf = (cpnext->cgtime - cp->cgtime) / singleTime;
			}
		}

		snum = snumf;  //FIXME ugg

		if (SC_Cvar_Get_Int("debug_camera")) {
			Com_Printf("(%d) snum: %d/%d   %f\n", cg.currentCameraPoint, snum, numSplines, (cg.ftime - cp->cgtime) / singleTime);
		}

		remainder = snumf - (double)snum;  //FIXME platform -- floor() needs float arg, not double
		VectorCopy(cg.splinePoints[cp->splineStart + snum], o);
		VectorCopy(cg.splinePoints[cp->splineStart + snum + 1], no);

		if (SC_Cvar_Get_Int("debug_camera")) {
			Com_Printf("singleTime %f remainder %f\n", singleTime, remainder);
			Com_Printf("%d -> %d  (%f, %f, %f)  (%f, %f, %f)\n", cp->splineStart + snum, cp->splineStart + snum + 1, o[0], o[1], o[2], no[0], no[1], no[2]);
		}

		f = remainder;
		if (SC_Cvar_Get_Int("debug_camera")) {
			Com_Printf("f  %f\n", f);
		}
		cg.refdef.vieworg[0] = o[0] + f * (no[0] - o[0]);
		cg.refdef.vieworg[1] = o[1] + f * (no[1] - o[1]);
		cg.refdef.vieworg[2] = o[2] + f * (no[2] - o[2]);
	} else if (cp->type == CAMERA_SPLINE_BEZIER  ||  cp->type == CAMERA_SPLINE_CATMULLROM) {
		posInterpolate_t posType;

		posType = posBezier;
		if (cp->type == CAMERA_SPLINE_CATMULLROM) {
			posType = posCatmullRom;
		}

		if (cp->useOriginVelocity) {
			totalDist = cp->originDistance;
			totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->originFinalVelocity - cp->originInitialVelocity) / totalTime;
			t = (double)(cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->originInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}

			//CG_CameraSplineOriginAt(cg.ftime, posBezier, cg.refdef.vieworg);
			CG_CameraSplineOriginAt(cp->cgtime + f * (cpnext->cgtime - cp->cgtime), posType, cg.refdef.vieworg);
		} else {
			f = -1;
#if 0
			if (cg.ftime < cpnext->cgtime) {
				snumf = (cg.ftime - cp->cgtime) / singleTime;
			} else {
				snumf = (cpnext->cgtime - cp->cgtime) / singleTime;
			}
#endif
			CG_CameraSplineOriginAt(cg.ftime, posType, cg.refdef.vieworg);
		}
	} else if (cp->type == CAMERA_CURVE) {
		f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (cp->hasQuadratic) {
			const cameraPoint_t *p1, *p2, *p3, *cpnextnext, *prev;

			cpnextnext = NULL;
			if (cpnext) {
				cpnextnext = cpnext->next;
				while (cpnextnext != NULL  &&  !(cpnextnext->flags & CAM_ORIGIN)) {
					cpnextnext = cpnextnext->next;
				}
			}

			prev = cp->prev;
			while (prev != NULL  &&  !(prev->flags & CAM_ORIGIN)) {
				prev = prev->prev;
			}

			p1 = p2 = p3 = NULL;

			if (cp->curveCount % 2 == 0) {
				p1 = cp;
				p2 = cpnext;
				p3 = cpnextnext;
			} else {
				p1 = prev;
				p2 = cp;
				p3 = cpnext;
			}

			if (p1  &&  p2  &&  p3) {
				qboolean cr;

				cr = qfalse;
				for (i = 0;  i < 3;  i++) {
					long double r;
					long double tm;

#if 0
					// test quadratic
					tm = p1->cgtime / 1000.0;
					r = a * tm * tm  +  b * tm  +  c;

					if (p1->origin[i] != r) {
						Com_Printf("p1[%d]  (%f -> %f  %f)  ", i, p1->origin[i], r, p1->origin[i] - r);
						cr = qtrue;
					}

					tm = p2->cgtime / 1000.0;
					r = a * tm * tm  +  b * tm  +  c;

					if (p2->origin[i] != r) {
						Com_Printf("p2[%d]  (%f -> %f  %f)  ", i, p2->origin[i], r, p2->origin[i] - r);
						cr = qtrue;
					}

					tm = p3->cgtime / 1000.0;
					r = a * tm * tm  +  b * tm  +  c;

					if (p3->origin[i] != r) {
						Com_Printf("p3[%d]  (%f -> %f  %f)  ", i, p3->origin[i], r, p3->origin[i] - r);
						cr = qtrue;
					}
#endif

					if (f >= 1.0) {
						tm = cpnext->cgtime - p1->cgtime / 1000.0;
					} else {
						tm = (cg.ftime - p1->cgtime) / 1000.0;
					}

					if (cp->useOriginVelocity) {
						float f2;

						totalDist = cp->originDistance;
						totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
						accel = (cp->originFinalVelocity - cp->originInitialVelocity) / totalTime;
						t = (cg.ftime - cp->cgtime) / 1000.0;
						dist = (cp->originInitialVelocity * t) + 0.5 * accel * t * t;

						f2 = dist / totalDist;
						if (f2 > 1.0) {
							f2 = 1;
						}
						//Com_Printf("f2: %f  (%f -> %f) %f  totalDist %f\n", f2, cp->originInitialVelocity, cp->originFinalVelocity, accel, totalDist);
						tm = totalTime * f2 + (cp->cgtime - cp->quadraticStartTime) / 1000.0;
					}

					if (f >= 1.0) {
						VectorCopy(cpnext->origin, cg.refdef.vieworg);
					} else {
						r = (cp->a[i] * tm) * tm  +  cp->b[i] * tm  +  cp->c[i];
						cg.refdef.vieworg[i] = (float)r;
					}
				}
				//Com_Printf("%f %f %f\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
				if (cr) {
					Com_Printf("\n");
				}
				//Com_Printf("(%f %f %f) -> (%f %f %f)\n", cp->origin[0], cp->origin[1], cp->origin[2], cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
			} else {
				//Com_Printf("wtf: %d\n", cg.currentCameraPoint);
			}
		} else {  // no quadratic defined
			f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cpprev  &&  cpprev->type == CAMERA_SPLINE) {
				VectorCopy(cg.splinePoints[cp->splineStart], o);
			} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_BEZIER) {
				CG_CameraSplineOriginAt(cp->cgtime, posBezier, o);
			} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_CATMULLROM) {
				CG_CameraSplineOriginAt(cp->cgtime, posCatmullRom, o);

			} else {
				VectorCopy(cp->origin, o);
			}

			if (cpnext->type == CAMERA_SPLINE) {
				VectorCopy(cg.splinePoints[cpnext->splineStart], no);
			} else if (cpnext->type == CAMERA_SPLINE_BEZIER) {
				CG_CameraSplineOriginAt(cpnext->cgtime, posBezier, no);
			} else if (cpnext->type == CAMERA_SPLINE_CATMULLROM) {
				CG_CameraSplineOriginAt(cpnext->cgtime, posCatmullRom, no);
			} else {
				VectorCopy(cpnext->origin, no);
			}

			if (cp->useOriginVelocity) {
				totalDist = cp->originDistance;
				totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
				accel = (cp->originFinalVelocity - cp->originInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->originInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
				//Com_Printf("f: %f  (%f -> %f) %f %f\n", f, cp->originInitialVelocity, cp->originFinalVelocity, cp->originAccel, cp->originThrust);
			}

			cg.refdef.vieworg[0] = o[0] + f * (no[0] - o[0]);
			cg.refdef.vieworg[1] = o[1] + f * (no[1] - o[1]);
			cg.refdef.vieworg[2] = o[2] + f * (no[2] - o[2]);
		}
	} else {
		// unknown cp->type
		CG_ErrorPopup(va("bad camera point %d unknown origin type", cg.currentCameraPoint));
		cg.cameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		returnValue = qfalse;
		goto cameraFinish;
	}

//#if 1  // gcc warning

handleCameraAngles:
	// now angles

	cp = (cameraPoint_t *)cpOrig;
	cpnext = cpnextOrig;
	cpprev = cpprevOrig;

	// masking
	while (cp  &&  !(cp->flags & CAM_ANGLES)) {
		cp = cp->prev;
	}
	while (cpprev  &&  !(cpprev->flags & CAM_ANGLES)) {
		cpprev = cpprev->prev;
	}
	while (cpnext  &&  !(cpnext->flags & CAM_ANGLES)) {
		cpnext = cpnext->next;
	}

	if (cp == NULL) {
		goto handleCameraFov;
	}
	if (cpnext == NULL) {
		VectorCopy(cp->angles, cg.refdefViewAngles);
		VectorCopy(cg.refdefViewAngles, cp->lastAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		goto handleCameraFov;
	}

	if (cp->viewType == CAMERA_ANGLES_INTERP  ||  cp->viewType == CAMERA_ANGLES_INTERP_USE_PREVIOUS) {

		f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (cp->viewType == CAMERA_ANGLES_INTERP_USE_PREVIOUS) {
			if (!cpprev) {
				CG_ErrorPopup(va("bad camera point %d angles interp and use previous but no previous cam point", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			VectorCopy(cpprev->lastAngles, a);
		} else if (cpprev  &&  cpprev->viewType == CAMERA_ANGLES_SPLINE) {
			CG_CameraSplineAnglesAt(cp->cgtime, a);
			//FIXME roll
		} else {
			VectorCopy(cp->angles, a);
		}

		pointNext = NULL;

		if (cpnext->viewType == CAMERA_ANGLES_INTERP) {
			VectorCopy(cpnext->angles, na);
			pointNext = cpnext;
		} else if (cpnext->viewType == CAMERA_ANGLES_INTERP_USE_PREVIOUS) {
			CG_ErrorPopup(va("bad camera point %d angles interp but next cam point has angles interp and grab previous angles", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		} else if (cpnext->viewType == CAMERA_ANGLES_FIXED) {
			VectorCopy(cpnext->angles, na);
			pointNext = cpnext;
		} else if (cpnext->viewType == CAMERA_ANGLES_FIXED_USE_PREVIOUS) {
			CG_ErrorPopup(va("bad camera point %d angles interp but next cam point has angles fixed and use previous", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		} else if (cpnext->viewType == CAMERA_ANGLES_ENT) {
			if (!cpnext->viewEntStartingOriginSet) {
				CG_ErrorPopup(va("can't play camera point %d  angles interp and next is angles ent but the starting origin hasn't been set", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			VectorCopy(cpnext->viewEntStartingOrigin, o);
			o[0] += cpnext->xoffset;
			o[1] += cpnext->yoffset;
			o[2] += cpnext->zoffset;
			VectorSubtract(o, cg.refdef.vieworg, dir);
			vectoangles(dir, na);
			pointNext = cpnext;
		} else if (cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
			VectorCopy(cpnext->viewPointOrigin, o);
			VectorSubtract(o, cg.refdef.vieworg, dir);
			vectoangles(dir, na);
			pointNext = cpnext;
		} else if (cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_PASS) {
			CG_ErrorPopup(va("bad camera point %d angles interp but next cam is viewpoint pass", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		} else if (cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED) {
			VectorCopy(cpnext->viewPointOrigin, o);
			VectorSubtract(o, cg.refdef.vieworg, dir);
			vectoangles(dir, na);
			pointNext = cpnext;
		} else if (cpnext->viewType == CAMERA_ANGLES_SPLINE) {
			CG_CameraSplineAnglesAt(cpnext->cgtime, na);
			pointNext = cpnext;
		} else {
			CG_ErrorPopup("invalid view type state");
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}

		if (cp->rollType != CAMERA_ROLL_AS_ANGLES) {
			a[ROLL] = 0;
			na[ROLL] = 0;
		}

		if (cp->useAnglesVelocity) {
			if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
				totalDist = CG_CameraAngleLongestDistanceWithRoll(a, na);
			} else {
				totalDist = CG_CameraAngleLongestDistanceNoRoll(a, na);
			}
			totalTime = (double)(pointNext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->anglesFinalVelocity - cp->anglesInitialVelocity) / totalTime;
			t = (cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->anglesInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
		}

		cg.refdefViewAngles[0] = LerpAngle(a[0], na[0], f);  // lerp2
		cg.refdefViewAngles[1] = LerpAngle(a[1], na[1], f);
		cg.refdefViewAngles[2] = LerpAngle(a[2], na[2], f);
	} else if (cp->viewType == CAMERA_ANGLES_FIXED) {
		VectorCopy(cp->angles, cg.refdefViewAngles);
	} else if (cp->viewType == CAMERA_ANGLES_FIXED_USE_PREVIOUS) {
		if (cpprev) {
			VectorCopy(cpprev->lastAngles, cg.refdefViewAngles);
		} else {
			CG_ErrorPopup(va("bad camera point %d fixed angles and use previous but no previous cam point", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
	} else if (cp->viewType == CAMERA_ANGLES_ENT) {
		int x, y, z;
		int nx, ny, nz;

		//x = y = z = 0;
		//nx = ny = nz = 0;

		f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		cent = &cg_entities[cp->viewEnt];
		VectorCopy(cent->lerpOrigin, origin);
		if (cent->currentState.eType == ET_PLAYER) {
			//origin[2] += DEFAULT_VIEWHEIGHT;
		}
		if (cp->offsetType == CAMERA_OFFSET_INTERP) {
			if (cpnext->viewType != CAMERA_ANGLES_ENT) {
				CG_ErrorPopup(va("bad camera point %d ent with offset interp but next cam point isn't ent", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			if (cp->offsetPassStart > -1) {
				c1 = &cg.cameraPoints[cp->offsetPassStart];
				c2 = &cg.cameraPoints[cp->offsetPassEnd];
				x = c1->xoffset;
				y = c1->yoffset;
				z = c1->zoffset;
				nx = c2->xoffset;
				ny = c2->yoffset;
				nz = c2->zoffset;
				f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
				if (f > 1.0) {
					f = 1;
				}
			} else {
				c1 = cp;
				c2 = cpnext;
				x = cp->xoffset;
				y = cp->yoffset;
				z = cp->zoffset;
				nx = cpnext->xoffset;
				ny = cpnext->yoffset;
				nz = cpnext->zoffset;
				// f set above
			}
		} else if (cp->offsetType == CAMERA_OFFSET_FIXED) {
			c1 = cp;
			c2 = cpnext;
			x = cp->xoffset;
			y = cp->yoffset;
			z = cp->zoffset;
			nx = ny = nz = 0;
			f = 0;
		} else if (cp->offsetType == CAMERA_OFFSET_PASS) {
			if (cp->offsetPassStart < 0) {
				CG_ErrorPopup(va("bad camera point %d offset pass but no pass starting point", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			c1 = &cg.cameraPoints[cp->offsetPassStart];
			c2 = &cg.cameraPoints[cp->offsetPassEnd];
			x = c1->xoffset;
			y = c1->yoffset;
			z = c1->zoffset;
			nx = c2->xoffset;
			ny = c2->yoffset;
			nz = c2->zoffset;
			f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
			if (f > 1.0) {
				f = 1;
			}
		} else {
			CG_ErrorPopup("invalid camera angles entity state");
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}

		totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
		if (cg.ftime > cpnext->cgtime) {
			t = (cpnext->cgtime - c1->cgtime) / 1000.0;
		} else {
			t = (cg.ftime - c1->cgtime) / 1000.0;
		}

 		if (c1->useXoffsetVelocity) {
			totalDist = abs(nx - x);
			accel = (c1->xoffsetFinalVelocity - c1->xoffsetInitialVelocity) / totalTime;
			dist = (c1->xoffsetInitialVelocity * t) + 0.5 * accel * t * t;
			x = x + (dist / totalDist) * (nx - x);
		} else {
			x = x + f * (nx - x);
		}

 		if (c1->useYoffsetVelocity) {
			totalDist = abs(ny - y);
			accel = (c1->yoffsetFinalVelocity - c1->yoffsetInitialVelocity) / totalTime;
			dist = (c1->yoffsetInitialVelocity * t) + 0.5 * accel * t * t;
			y = y + (dist / totalDist) * (ny - y);
		} else {
			y = y + f * (ny - y);
		}

 		if (c1->useZoffsetVelocity) {
			totalDist = abs(nz - z);
			accel = (c1->zoffsetFinalVelocity - c1->zoffsetInitialVelocity) / totalTime;
			dist = (c1->zoffsetInitialVelocity * t) + 0.5 * accel * t * t;
			z = z + (dist / totalDist) * (nz - z);
		} else {
			z = z + f * (nz - z);
		}

		origin[0] += x;
		origin[1] += y;
		origin[2] += z;
		VectorSubtract(origin, cg.refdef.vieworg, dir);
		vectoangles(dir, cg.refdefViewAngles);
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
		VectorCopy(cp->viewPointOrigin, o);
		if (cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP  ||  cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED) {

			VectorCopy(cpnext->viewPointOrigin, no);
			f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useAnglesVelocity) {
				totalDist = CG_CameraAngleLongestDistanceNoRoll(o, no);
				totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
				accel = (cp->anglesFinalVelocity - cp->anglesInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->anglesInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else if (cp->viewPointPassStart < 0) {
			CG_ErrorPopup(va("bad camera point %d viewpoint but not followed by either viewpoint pass or viewpoint", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		} else if (cp->viewPointPassStart > -1) {
			VectorCopy(cg.cameraPoints[cp->viewPointPassEnd].viewPointOrigin, no);
			f = (cg.ftime - cp->cgtime) / (cg.cameraPoints[cp->viewPointPassEnd].cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useAnglesVelocity) {
				totalDist = CG_CameraAngleLongestDistanceNoRoll(o, no);
				totalTime = (double)(cg.cameraPoints[cp->viewPointPassEnd].cgtime - cp->cgtime) / 1000.0;
				accel = (cp->anglesFinalVelocity - cp->anglesInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->anglesInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		}

		origin[0] = o[0] + f * (no[0] - o[0]);
		origin[1] = o[1] + f * (no[1] - o[1]);
		origin[2] = o[2] + f * (no[2] - o[2]);

		VectorSubtract(origin, cg.refdef.vieworg, dir);
		vectoangles(dir, cg.refdefViewAngles);
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_PASS) {
		//VectorCopy(cp->viewPointOrigin, o);
		if (cp->viewPointPassStart < 0) {
			CG_ErrorPopup(va("bad camera point %d  viewpointpass with no viewpoint set previously", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
		c1 = &cg.cameraPoints[cp->viewPointPassStart];
		c2 = &cg.cameraPoints[cp->viewPointPassEnd];
		VectorCopy(c1->viewPointOrigin, o);
		VectorCopy(c2->viewPointOrigin, no);

		f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (c1->useAnglesVelocity) {
			totalDist = CG_CameraAngleLongestDistanceNoRoll(o, no);
			totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
			accel = (c1->anglesFinalVelocity - c1->anglesInitialVelocity) / totalTime;
			t = (cg.ftime - c1->cgtime) / 1000.0;
			dist = (c1->anglesInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
		}

		origin[0] = o[0] + f * (no[0] - o[0]);
		origin[1] = o[1] + f * (no[1] - o[1]);
		origin[2] = o[2] + f * (no[2] - o[2]);

		VectorSubtract(origin, cg.refdef.vieworg, dir);
		vectoangles(dir, cg.refdefViewAngles);
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED) {
		VectorSubtract(cp->viewPointOrigin, cg.refdef.vieworg, dir);
		vectoangles(dir, cg.refdefViewAngles);
	} else if (cp->viewType == CAMERA_ANGLES_SPLINE) {
		if (cp->useAnglesVelocity) {
			CG_CameraSplineAnglesAt(cp->cgtime, a);
			CG_CameraSplineAnglesAt(cpnext->cgtime, na);

			totalDist = CG_CameraAngleLongestDistanceWithRoll(a, na);
			totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->anglesFinalVelocity - cp->anglesInitialVelocity) / totalTime;
			t = (cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->anglesInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}

			CG_CameraSplineAnglesAt(cp->cgtime + f * (cpnext->cgtime - cp->cgtime), cg.refdefViewAngles);
		} else {
			CG_CameraSplineAnglesAt(cg.ftime, cg.refdefViewAngles);
		}
	} else {
		// unknown cp->viewType
		CG_ErrorPopup(va("bad camera point %d  unknown view type", cg.currentCameraPoint));
		cg.cameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		returnValue = qfalse;
		goto cameraFinish;
	}

#if 1  // gcc warning  // up to here

	// roll


	if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
		// pass, already handled
	} else if (cp->rollType == CAMERA_ROLL_INTERP) {
		//roll = nroll = 0;  // silence compiler warning
		if (cpnext->rollType == CAMERA_ROLL_INTERP  ||  cpnext->rollType == CAMERA_ROLL_FIXED) {
			//
			roll = cp->angles[ROLL];
			nroll = cpnext->angles[ROLL];
			f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useRollVelocity) {
				totalDist = fabs(nroll - roll);
				totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
				accel = (cp->rollFinalVelocity - cp->rollInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->rollInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else if (cpnext->rollType == CAMERA_ROLL_PASS) {
			//
			if (cp->rollPassStart < 0) {
				CG_ErrorPopup(va("bad camera point %d roll pass but no starting point", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			c1 = &cg.cameraPoints[cp->rollPassStart];
			c2 = &cg.cameraPoints[cp->rollPassEnd];
			roll = c1->angles[ROLL];
			nroll = c2->angles[ROLL];
			f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useRollVelocity) {
				totalDist = fabs(nroll - roll);
				totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
				accel = (c1->rollFinalVelocity - c1->rollInitialVelocity) / totalTime;
				t = (cg.ftime - c1->cgtime) / 1000.0;
				dist = (c1->rollInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else {
			CG_ErrorPopup("invalid camera interp roll state");
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
		cg.refdefViewAngles[ROLL] = LerpAngle(roll, nroll, f);
	} else if (cp->rollType == CAMERA_ROLL_FIXED) {
		cg.refdefViewAngles[ROLL] = cp->angles[ROLL];
	} else if (cp->rollType == CAMERA_ROLL_PASS) {
		if (cp->rollPassStart < 0) {
			CG_ErrorPopup(va("bad camera point %d camera roll pass and starting point < 0", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
		c1 = &cg.cameraPoints[cp->rollPassStart];
		c2 = &cg.cameraPoints[cp->rollPassEnd];
		roll = c1->angles[ROLL];
		nroll = c2->angles[ROLL];
		f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (cp->useRollVelocity) {
			totalDist = fabs(nroll - roll);
			totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
			accel = (c1->rollFinalVelocity - c1->rollInitialVelocity) / totalTime;
			t = (cg.ftime - c1->cgtime) / 1000.0;
			dist = (c1->rollInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
		}
		//cg.refdefViewAngles[ROLL] = LerpAngle(roll, nroll, f);
		cg.refdefViewAngles[ROLL] = roll + f * (nroll - roll);
	} else {
		CG_ErrorPopup(va("bad camera point %d unknown roll type", cg.currentCameraPoint));
		cg.cameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		returnValue = qfalse;
		goto cameraFinish;
	}

	VectorCopy(cg.refdefViewAngles, cp->lastAngles);
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

handleCameraFov:

	cp = (cameraPoint_t *)cpOrig;
	cpnext = cpnextOrig;
	cpprev = cpprevOrig;

	// masking
	while (cp  &&  !(cp->flags & CAM_FOV)) {
		cp = cp->prev;
	}
	while (cpprev  &&  !(cpprev->flags & CAM_FOV)) {
		cpprev = cpprev->prev;
	}
	while (cpnext  &&  !(cpnext->flags & CAM_FOV)) {
		cpnext = cpnext->next;
	}

	if (cp == NULL) {
		goto cameraFinish;
	}
	if (cpnext == NULL) {
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.value;
		}

		cg.refdef.fov_x = fov;
		if (cg.refdef.fov_x < 1) {
			cg.refdef.fov_x = 1;
		}

		CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);
		goto cameraFinish;
	}

	if (cp->fovType == CAMERA_FOV_USE_CURRENT) {
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.value;
		}
	} else if (cp->fovType == CAMERA_FOV_SPLINE) {
		float cfov;

		if (cp->useFovVelocity) {
			totalDist = cp->fovDistance;
			totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
			accel = (cp->fovFinalVelocity - cp->fovInitialVelocity) / totalTime;
			t = (cg.ftime - cp->cgtime) / 1000.0;
			dist = (cp->fovInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}

			if (!CG_CameraSplineFovAt(cp->cgtime + f * (cpnext->cgtime - cp->cgtime), &cfov)) {
				if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
					fov = cg.demoFov;
				} else {
					fov = cg_fov.value;
				}
			} else {
				fov = cfov;
			}
		} else {
			if (!CG_CameraSplineFovAt(cg.ftime, &cfov)) {
				if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
					fov = cg.demoFov;
				} else {
					fov = cg_fov.value;
				}
			} else {
				fov = cfov;
			}
		}
	} else if (cp->fovType == CAMERA_FOV_INTERP) {
		if (cpnext->fovType == CAMERA_FOV_INTERP  ||  cpnext->fovType == CAMERA_FOV_FIXED) {
			fov = cp->fov;
			nfov = cpnext->fov;
			f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useFovVelocity) {
				totalDist = fabs(nfov - fov);
				totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
				accel = (cp->fovFinalVelocity - cp->fovInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->fovInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else if (cpnext->fovType == CAMERA_FOV_USE_CURRENT) {
			fov = cp->fov;
			if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
				nfov = cg.demoFov;
			} else {
				nfov = cg_fov.value;
			}
			f = (cg.ftime - cp->cgtime) / (cpnext->cgtime - cp->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useFovVelocity) {
				totalDist = fabs(nfov - fov);
				totalTime = (double)(cpnext->cgtime - cp->cgtime) / 1000.0;
				accel = (cp->fovFinalVelocity - cp->fovInitialVelocity) / totalTime;
				t = (cg.ftime - cp->cgtime) / 1000.0;
				dist = (cp->fovInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else if (cpnext->fovType == CAMERA_FOV_PASS) {
			if (cp->fovPassStart < 0) {
				CG_ErrorPopup(va("bad camera point %d fov pass and fovPassStart < 0", cg.currentCameraPoint));
				cg.cameraPlaying = qfalse;
				cg.cameraPlayedLastFrame = qfalse;
				returnValue = qfalse;
				goto cameraFinish;
			}
			c1 = &cg.cameraPoints[cp->fovPassStart];
			c2 = &cg.cameraPoints[cp->fovPassEnd];
			fov = c1->fov;
			nfov = c2->fov;
			f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
			if (f > 1.0) {
				f = 1;
			}

			if (cp->useFovVelocity) {
				totalDist = fabs(nfov - fov);
				totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
				accel = (c1->fovFinalVelocity - c1->fovInitialVelocity) / totalTime;
				t = (cg.ftime - c1->cgtime) / 1000.0;
				dist = (c1->fovInitialVelocity * t) + 0.5 * accel * t * t;
				f = dist / totalDist;
				if (f > 1.0) {
					f = 1;
				}
			}
		} else {
			CG_ErrorPopup("invalid fov interp state");
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
		fov = fov + f * (nfov - fov);
	} else if (cp->fovType == CAMERA_FOV_PASS) {
		if (cp->fovPassStart < 0) {
			CG_ErrorPopup(va("bad camera point %d fov pass and no pass start", cg.currentCameraPoint));
			cg.cameraPlaying = qfalse;
			cg.cameraPlayedLastFrame = qfalse;
			returnValue = qfalse;
			goto cameraFinish;
		}
		c1 = &cg.cameraPoints[cp->fovPassStart];
		c2 = &cg.cameraPoints[cp->fovPassEnd];
		fov = c1->fov;
		nfov = c2->fov;
		f = (cg.ftime - c1->cgtime) / (c2->cgtime - c1->cgtime);
		if (f > 1.0) {
			f = 1;
		}

		if (c1->useFovVelocity) {
			totalDist = fabs(nfov - fov);
			totalTime = (double)(c2->cgtime - c1->cgtime) / 1000.0;
			accel = (c1->fovFinalVelocity - c1->fovInitialVelocity) / totalTime;
			t = (cg.ftime - c1->cgtime) / 1000.0;
			dist = (c1->fovInitialVelocity * t) + 0.5 * accel * t * t;
			f = dist / totalDist;
			if (f > 1.0) {
				f = 1;
			}
		}
		fov = fov + f * (nfov - fov);
	} else {
		CG_ErrorPopup(va("bad camera point %d unknown fov type", cg.currentCameraPoint));
		cg.cameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		returnValue = qfalse;
		goto cameraFinish;
	}

	cg.refdef.fov_x = fov;
	if (cg.refdef.fov_x < 1) {
		cg.refdef.fov_x = 1;
	}

	CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

	returnValue = qtrue;

cameraFinish:

	cg.refdef.time = cg.time;
	trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);


	if (cg_cameraUpdateFreeCam.integer  &&  !cg_cameraDebugPath.integer) {
		cg.fMoveTime = 0;  //cg.realTime;  //cg.time;
		//cg.fMoveTime = cg.realTime - 10;
		//cg.mousex = cg.mousey = 0;
		VectorCopy(cg.refdef.vieworg, cg.fpos);
		cg.fpos[2] -= DEFAULT_VIEWHEIGHT;
		VectorCopy(cg.refdefViewAngles, cg.fang);
		cg.freecamSet = qtrue;
		//Com_Printf("xxx %f %f %f\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
		if (cg.ftime >= lastPoint->cgtime) {
			VectorClear(cg.freecamPlayerState.velocity);
		} else if (cg.ftime > cg.positionSetTime) {
			for (i = 0;  i < 3;  i++) {
				cg.freecamPlayerState.velocity[i] = (cg.refdef.vieworg[i] - cg.cameraLastOrigin[i]) / ((cg.ftime - cg.positionSetTime) / 1000.0);
			}
		} else if (cg.ftime < cg.positionSetTime) {
			VectorSet(cg.freecamPlayerState.velocity, 0, 0, 0);
		}
	}

	if (cg.ftime >= lastPoint->cgtime) {
		VectorClear(cg.cameraVelocity);
	} else if (cg.ftime > cg.positionSetTime) {
		for (i = 0;  i < 3;  i++) {
			cg.cameraVelocity[i] = (cg.refdef.vieworg[i] - cg.cameraLastOrigin[i]) / ((cg.ftime - cg.positionSetTime) / 1000.0);
		}
	} else if (cg.ftime < cg.positionSetTime) {
		VectorSet(cg.cameraVelocity, 0, 0, 0);
	}

	if (cg.ftime != cg.positionSetTime) {
		cg.positionSetTime = cg.ftime;
		VectorCopy(cg.refdef.vieworg, cg.cameraLastOrigin);
	}

#endif  // gcc warning

	if (cameraDebugPath) {  //cg_cameraDebugPath.integer) {  //(1) {
		refEntity_t ent;
		byte color[4] = { 0, 255, 0, 255 };

		memset(&ent, 0, sizeof(ent));
		VectorCopy(cg.refdef.vieworg, ent.origin);
		ent.reType = RT_MODEL;
		ent.hModel = cgs.media.teleportEffectModel;
		AxisClear(ent.axis);
		//ent.origin[2] += 16;
		ent.radius = 300;
		ent.customShader = 0;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 100;
		ent.renderfx = RF_DEPTHHACK;

		CG_AddRefEntity(&ent);

		AngleVectors(cg.refdefViewAngles, dir, NULL, NULL);
		VectorCopy(ent.origin, origin);
		VectorMA(origin, 300, dir, origin);
		CG_SimpleRailTrail(ent.origin, origin, cg_railTrailTime.value, color);

		VectorCopy(refOriginOrig, cg.refdef.vieworg);
		VectorCopy(refAnglesOrig, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		cg.refdef.fov_x = refFovxOrig;
		cg.refdef.fov_y = refFovyOrig;
		VectorCopy(velocityOrig, cg.freecamPlayerState.velocity);

		cg.freecamSet = qtrue;
	}


	return returnValue;
}

void CG_AdjustOriginToAvoidSolid (vec3_t origin, const centity_t *cent)
{
	trace_t         trace;
	vec3_t mins, maxs;

	VectorCopy(bg_playerMins, mins);  //FIXME option
	VectorCopy(bg_playerMaxs, maxs);  //FIXME option

	//FIXME sometimes you still get stuck, maybe increas mins
	CG_Trace( &trace, cent->lerpOrigin, mins, maxs, origin, cent->currentState.number, MASK_SOLID );
	if ( trace.fraction != 1.0 ) {
		VectorCopy( trace.endpos, origin);
		//origin[2] += (1.0 - trace.fraction) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace( &trace, cent->lerpOrigin, mins, maxs, origin, cent->currentState.number, MASK_SOLID );
		VectorCopy( trace.endpos, origin );
	}
}

static void CG_FreeCam (void)
{
	int fmove;
	int smove;
	int umove;
	vec3_t maxs;
	vec3_t mins;
	int i;
	int tm;
	playerState_t *ps;
	pmove_t pmove;
	//int x;
	//char *s = NULL;

	ps = &cg.freecamPlayerState;
	tm = cg.realTime;  //cg.time;

	VectorCopy(bg_playerMins, mins);
	VectorCopy(bg_playerMaxs, maxs);

	if (cg.mouseSeeking) {
		//return;
	}

	if (cg.playPath) {
		CG_PlayPath();
		return;
	}

#if 0
	if (cg.cameraPlaying) {
		if (CG_PlayCamera()) {
			return;
		}
	}
#endif

	if (cg.cameraPlaying  &&  !SC_Cvar_Get_Int("cl_freezeDemo")) {
		//return;
	}

	if (cg.fMoveTime == 0) {
		//Com_Printf("^3fmovetime == 0\n");
		cg.fMoveTime = cg.realTime;  //cg.time;
		VectorCopy(cg.fpos, ps->origin);
		VectorCopy(cg.fang, ps->viewangles);
		VectorSet(ps->delta_angles, 0, 0, 0);
		VectorSet(ps->velocity, 0, 0, 0);

		VectorCopy(cg.fpos, cg.refdef.vieworg);
		//FIXME check DEFAULT_VIEWHEIGHT
		cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;

		VectorCopy(cg.fang, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			cg.refdef.fov_x = cg.demoFov;
		} else {
			cg.refdef.fov_x = cg_fov.value;
		}

		if ( cg.refdef.fov_x < 1 ) {
			cg.refdef.fov_x = 1;
		}

		CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

		cg.refdef.time = cg.time;  //dcg.realTime;  //cg.time;

		//goto finish;
		return;
	}

	//FIXME hack, something is unsetting it
	//trap_Cvar_Set ("cg_thirdPerson", "1");
	//cg.renderingThirdPerson = 1;


	if (cg.viewEnt == -1  &&  !cg.useViewPointMark) {
		float roll;
		float newRoll;

		if (!cg.mouseSeeking) {
			if (!cg.freecamAnglesSet) {
				cg.fang[YAW] -= cg.mousex * cg_freecam_sensitivity.value * cg_freecam_yaw.value;
				//cg.fang[ROLL] += cg.mousex * cg_freecam_sensitivity.value * cg_freecam_yaw.value;
				//Com_Printf("roll %f\n", cg.fang[ROLL]);
				cg.mousex = 0;

				cg.fang[PITCH] += cg.mousey * cg_freecam_sensitivity.value * cg_freecam_pitch.value;
				cg.mousey = 0;
			} else {
				VectorCopy(cg.freecamPlayerState.viewangles, cg.fang);
				cg.freecamAnglesSet = qfalse;
			}
		}

		roll = cg.fang[ROLL];
		if (cg.keyrollright) {
			cg.fang[ROLL] += cg_freecam_rollValue.value;
		}
		if (cg.keyrollleft) {
			cg.fang[ROLL] -= cg_freecam_rollValue.value;
		}
		newRoll = AngleNormalize180(cg.fang[ROLL]);
		if (cg.keyrollstopzero) {
			if (roll == 0.0  ||  (roll > 0.0  &&  newRoll <= 0.0)  ||  (roll < 0.0  &&  newRoll >= 0.0)) {
				cg.fang[ROLL] = 0;
			}
		}
	} else {  // view locked to an entity or a viewpoint
		static vec3_t dir;
		vec3_t angles;
		float roll;
		float newRoll;
		//vec3_t forward, right, up;
		centity_t *cent;
		vec3_t origin;
		vec3_t ourOrigin;

		if (cg.useViewPointMark) {
			VectorCopy(cg.viewPointMarkOrigin, origin);
		} else {
			cent = &cg_entities[cg.viewEnt];
			VectorCopy(cent->lerpOrigin, origin);
		}

#if 0
		if (cent->currentState.eType == ET_MISSILE) {
			//AngleVectors(cent->currentState.angles, forward, right, up);
			AngleVectors(cent->currentState.pos.trDelta, forward, right, up);
		} else if (cent->currentState.eType == ET_PLAYER) {
			//static vec3_t dir;
			//static int lastDirTime = -1;
			//AngleVectors(cent->currentState.pos.trDelta, forward, right, up);
			//AngleVectors(cg.freecamPlayerState.viewangles, forward, right, up);
			if (1) {  //(trap_Milliseconds() - lastDirTime > 1500) {
				VectorSubtract(cent->nextState.pos.trBase, cent->currentState.pos.trBase, dir);
				AngleVectors(dir, forward, right, up);
			}
		} else {
			// the fuck
			//Com_Printf("the fuck viewent offset\n");
			AngleVectors(cent->lerpAngles, forward, right, up);
		}

		//AngleVectors(cent->lerpAngles, forward, right, up);
		VectorMA(origin, cg.viewEntOffsetForward, forward, origin);
		VectorMA(origin, cg.viewEntOffsetRight, right, origin);
		VectorMA(origin, cg.viewEntOffsetUp, up, origin);
#endif
		origin[0] += cg.viewEntOffsetX;
		origin[1] += cg.viewEntOffsetY;
		origin[2] += cg.viewEntOffsetZ;

		//VectorSubtract(cg_entities[cg.viewEnt].lerpOrigin, ps->origin, dir);
		//VectorSubtract(origin, ps->origin, dir);
		VectorCopy(ps->origin, ourOrigin);
		ourOrigin[2] += DEFAULT_VIEWHEIGHT;
		VectorSubtract(origin, ourOrigin, dir);
		vectoangles(dir, angles);
		if (cg.viewUnlockYaw  &&  !cg.mouseSeeking) {
			cg.fang[YAW] -= cg.mousex * cg_freecam_sensitivity.value * cg_freecam_yaw.value;
		} else {
			cg.fang[YAW] += angles[YAW] - ps->viewangles[YAW];
		}
		if (cg.viewUnlockPitch  &&  !cg.mouseSeeking) {
			cg.fang[PITCH] += cg.mousey * cg_freecam_sensitivity.value * cg_freecam_pitch.value;
		} else {
			cg.fang[PITCH] += angles[PITCH] - ps->viewangles[PITCH];
		}


		roll = cg.fang[ROLL];
		if (cg.keyrollright) {
			cg.fang[ROLL] += cg_freecam_rollValue.value;
		}
		if (cg.keyrollleft) {
			cg.fang[ROLL] -= cg_freecam_rollValue.value;
		}
		newRoll = AngleNormalize180(cg.fang[ROLL]);
		if (cg.keyrollstopzero) {
			if (roll == 0.0  ||  (roll > 0.0  &&  newRoll <= 0.0)  ||  (roll < 0.0  &&  newRoll >= 0.0)) {
				cg.fang[ROLL] = 0;
			}
		}

		if (!cg.mouseSeeking) {
			cg.mousex = 0;
			cg.mousey = 0;
		}
	}

#if 0
	if (cg.viewEnt > -1  &&  cg.chase) {
		VectorCopy(cg_entities[cg.viewEnt].lerpOrigin, cg.freecamPlayerState.origin);
		//return;
	}
#endif

	fmove = smove = umove = 0;

	if (cg.keyf) {
		fmove += 127;
	}
	if (cg.keyb) {
		fmove -= 127;
	}
	if (cg.keyr) {
		smove += 127;
	}
	if (cg.keyl) {
		smove -= 127;
	}
	if (cg.keyu) {
		umove += 127;
	}
	if (cg.keyd) {
		umove -= 127;
	}

	if (fmove > 0) {
		fmove = 127;
	}
	if (fmove < 0) {
		fmove = -127;
	}
	if (smove > 0) {
		smove = 127;
	}
	if (smove < 0) {
		smove = -127;
	}
	if (umove > 0) {
		umove = 127;
	}
	if (umove < 0) {
		umove = -127;
	}

	if (cg.mouseSeeking) {
		fmove = smove = umove = 0;
	}

	for (i = 0;  i < 3;  i++) {
		cg.fang[i] = AngleNormalize180(cg.fang[i]);
		//cg.fang[i] = AngleNormalize360(cg.fang[i]);
	}

	// pmove
	pmove.ps = ps;
	pmove.trace = CG_Trace;
	pmove.pointcontents = CG_PointContents;
	pmove.tracemask = MASK_PLAYERSOLID;
	pmove.tracemask &= ~CONTENTS_BODY;
	pmove.noFootsteps = 0;
	pmove.pmove_fixed = pmove_fixed.integer;
	pmove.pmove_msec = pmove_msec.integer;
	pmove.debugLevel = 0;
	VectorCopy(mins, pmove.mins);
	VectorCopy(maxs, pmove.maxs);

	// playerstate
	if (cg_freecam_noclip.integer) {
		ps->pm_type = PM_NOCLIP;
	} else {
		ps->pm_type = PM_SPECTATOR;
	}
	// ps.groundEntityNum ?

	//ps->viewheight = DEFAULT_VIEWHEIGHT;
	ps->commandTime = cg.fMoveTime;  // - 8;
	ps->speed = cg_freecam_speed.integer;
	ps->stats[STAT_HEALTH] = 100;
	ps->persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ps->gravity = 800;
	ps->clientNum = -1;  //FIXME check

	// cmd
	pmove.cmd.forwardmove = fmove;
	pmove.cmd.buttons = 0;
	pmove.cmd.rightmove = smove;
	pmove.cmd.upmove = umove;
	pmove.cmd.serverTime = cg.realTime;  //cg.time;
	pmove.cmd.weapon = WP_RAILGUN;

	if (!cg_freecam_unlockPitch.integer) {
		if (cg.fang[PITCH] - ps->viewangles[PITCH] > 90) {
			cg.fang[PITCH] = ps->viewangles[PITCH] + 90;
		} else if (ps->viewangles[PITCH] - cg.fang[PITCH] > 90) {
			cg.fang[PITCH] = ps->viewangles[PITCH] - 90;
		}
	}

	pmove.unlockPitch = cg_freecam_unlockPitch.integer;

	for (i = 0;  i < 3;  i++) {
		pmove.cmd.angles[i] = ANGLE2SHORT(cg.fang[i]);
	}

	VectorSet(ps->delta_angles, 0, 0, 0);
	//VectorCopy(cg.fpos, ps->origin);
	//Com_Printf("^3pre pmove %f %f %f\n", cg.fpos[0], cg.fpos[1], cg.fpos[2]);
	//Com_Printf("f %d r %d u %d  commandtime %d  %d\n", fmove, smove, umove, ps->commandTime, cg.realTime);
	//Com_Printf("vel %f %f %f\n", ps->velocity[0], ps->velocity[1], ps->velocity[2]);
	Pmove(&pmove);
	VectorCopy(ps->origin, cg.fpos);
	//Com_Printf("^3pst pmove %f %f %f\n", cg.fpos[0], cg.fpos[1], cg.fpos[2]);
	//Com_Printf("vel %f %f %f\n", ps->velocity[0], ps->velocity[1], ps->velocity[2]);
	VectorCopy(ps->viewangles, cg.fang);
	VectorCopy(ps->velocity, cg.fvelocity);

#if 0
	if (cg.keyf) {
		vec3_t right;
		float scale;

		AngleVectors(cg.fang, NULL, right, NULL);
		scale = 0.5 * 180.0 / cg.fang[ROLL];
		Com_Printf("scale %f\n", scale);
		VectorMA(cg.fpos, scale, right, cg.fpos);
		VectorCopy(cg.fpos, cg.refdef.vieworg);
		//cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;
	}
#endif


	VectorCopy(cg.fpos, cg.refdef.vieworg);
	//FIXME check DEFAULT_VIEWHEIGHT
	cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;

	VectorCopy(cg.fang, cg.refdefViewAngles);
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	for (i = 0;  i < 3;  i++) {
		cg.fang[i] += SHORT2ANGLE(ps->delta_angles[i]);
	}
	//VectorSet(ps->delta_angles, 0, 0, 0);


	// finish:
	if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
		cg.refdef.fov_x = cg.demoFov;
	} else {
		cg.refdef.fov_x = cg_fov.value;
	}

	if ( cg.refdef.fov_x < 1 ) {
		cg.refdef.fov_x = 1;
	}

	cg.refdef.fov_x = CG_CalcZoom(cg.refdef.fov_x);

	CG_AdjustedFov(cg.refdef.fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);
	
	cg.refdef.time = cg.time;  //dcg.realTime;  //cg.time;

	if (cg.keya  &&  !cg.mouseSeeking) {
		if (tm - cg.freecamFireTime > 1500) {
			byte color[4];
			trace_t tr;
			vec3_t muzzle, forward, right, up, end;

#define CVAL 0x60
			color[0] = CVAL;
			color[1] = 0;
			color[2] = CVAL;
			color[3] = 255;
#undef CVAL
			// fire
			// muzzle point
			VectorCopy(cg.fpos, muzzle);
			//FIXME check DEFAULT_VIEWHEIGHT
			muzzle[2] += DEFAULT_VIEWHEIGHT;
			AngleVectors(cg.refdefViewAngles, forward, right, up);
			//VectorMA(muzzle, DEFAULT_VIEWHEIGHT, up, muzzle);
			VectorMA(muzzle, 14, forward, muzzle);

			VectorMA (muzzle, 131072 /*8192 * 1*/, forward, end);

			Wolfcam_WeaponTrace(&tr, muzzle, NULL, NULL, end, MAX_CLIENTS - 1, CONTENTS_SOLID | CONTENTS_BODY);
			//CG_RailTrail(&ci, muzzle, end);
			VectorMA(muzzle, 8, right, muzzle);
			//CG_SimpleRailTrail(muzzle, end, color);
			CG_SimpleRailTrail(muzzle, tr.endpos, cg_railTrailTime.value, color);
			CG_StartSound(cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_WEAPON, cg_weapons[WP_RAILGUN].flashSound[0]);
			if (tr.entityNum < MAX_CLIENTS  &&  tr.entityNum >= 0) {
				CG_GibPlayer(&cg_entities[tr.entityNum]);
			} else {

			}
			// CG_MissileHitWall
			//Com_Printf("impact mark:  %f %f %f  distance: %f\n", tr.endpos[0], tr.endpos[1], tr.endpos[2], Distance(cg.fpos, tr.endpos));
			CG_ImpactMark(cgs.media.energyMarkShader, tr.endpos, tr.plane.normal, random() * 360, color[0], color[1], color[2], 1, qtrue, 8, qfalse, qtrue, qtrue);

			cg.freecamFireTime = tm;  //trap_Milliseconds();  //cg.time;
			//FIXME clientInfo
		}
	}

	if (cg.chaseEnt > -1) {
		const centity_t *cent;
		vec3_t origin;
		//vec3_t forward, right, up;
		//vec3_t dir;

		//VectorCopy(cg_entities[cg.chaseEnt].lerpOrigin, cg.refdef.vieworg);
		cent = &cg_entities[cg.chaseEnt];
		VectorCopy(cent->lerpOrigin, origin);

#if 0
		if (cent->currentState.eType == ET_MISSILE) {
			//AngleVectors(cent->currentState.angles, forward, right, up);
			AngleVectors(cent->currentState.pos.trDelta, forward, right, up);
		} else if (cent->currentState.eType == ET_PLAYER) {
			//static vec3_t dir;
			//static int lastDirTime = -1;
			//AngleVectors(cent->currentState.pos.trDelta, forward, right, up);
			//AngleVectors(cg.freecamPlayerState.viewangles, forward, right, up);
			if (1) {  //(1) {  //(trap_Milliseconds() - lastDirTime > 1500) {
				VectorSubtract(cent->nextState.pos.trBase, cent->currentState.pos.trBase, dir);
				AngleVectors(dir, forward, right, up);
			}
		} else {
			// the fuck
			//Com_Printf("the fuck viewent offset\n");
			AngleVectors(cent->lerpAngles, forward, right, up);
		}

		//AngleVectors(cent->lerpAngles, forward, right, up);
#if 1
		VectorMA(origin, cg.chaseEntOffsetForward, forward, origin);
		VectorMA(origin, cg.chaseEntOffsetRight, right, origin);
		VectorMA(origin, cg.chaseEntOffsetUp, up, origin);
#endif
#endif
		VectorCopy(origin, cg.refdef.vieworg);
		cg.refdef.vieworg[0] += cg.chaseEntOffsetX;
		cg.refdef.vieworg[1] += cg.chaseEntOffsetY;
		cg.refdef.vieworg[2] += cg.chaseEntOffsetZ;

	}
	//done:
	//FIXME inwater);
	//trap_S_Respatialize (-1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);
	//FIXME hack for entity num
	trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);

	//Com_Printf("touchents: num %d\n", pmove.numtouch);
	cg.fMoveTime = tm;
	cg.freecamSet = qtrue;
}

static void CG_CheckSkillRating (void)
{
	if (!cgs.needToCheckSkillRating  ||  cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (cgs.realProtocol >= 91) {
		// no more skill ratings
		return;
	}

	// there was a connect or disconnect and sv_skillRating wasn't updated,
	// so the client had the same skill rating as the server average
	if (!cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].infoValid) {
		cgs.lastConnectedDisconnectedPlayerClientInfo->skillRating = cgs.skillRating;
		cgs.lastConnectedDisconnectedPlayerClientInfo->knowSkillRating = qtrue;
		CG_Printf("%s ^5had skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, cgs.skillRating);
		if (cg_printSkillRating.integer) {
			CG_PrintToScreen("%s ^5had skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, cgs.skillRating);
		}
	} else {
		cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].skillRating = cgs.skillRating;
		cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].knowSkillRating = qtrue;
		CG_BuildSpectatorString();
		CG_Printf("%s ^5has skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, cgs.skillRating);
		if (cg_printSkillRating.integer) {
			CG_PrintToScreen("%s ^5has skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, cgs.skillRating);
		}
	}

	//Com_Printf("^3skill rating %d\n", cgs.skillRating);
	cgs.needToCheckSkillRating = qfalse;
}

//FIXME name
static void CG_DrawRawCameraPathKeyPoints (void)
{
	int i;
	byte color[4];
	byte lineColor[4];
	//clientInfo_t ci;

	//VectorSet(ci.color1, 1, 0.5, 1);
	//VectorSet(ci.color2, 1, 0.5, 1);
	MAKERGBA(color, 255, 40, 40, 255);
	MAKERGBA(lineColor, 255, 127, 255, 255);

	for (i = 0;  i < cg.numRawCameraPoints;  i++) {
		if (i == 0  ||  i == (cg.numRawCameraPoints - 1)  ||  (i % (cg.numRawCameraPoints / 5) == 0)) {
			CG_FloatNumber(i + 1, cg.rawCameraPoints[i].origin, RF_DEPTHHACK, color, 1.0);
		}
		if (i > 0) {
			//FIXME
			//CG_RailTrail(&ci, cg.rawCameraPoints[i - 1].origin, cg.rawCameraPoints[i].origin);
			CG_SimpleRailTrail(cg.rawCameraPoints[i - 1].origin, cg.rawCameraPoints[i].origin, cg_railTrailTime.value, lineColor);
		}
	}
}

static void	CG_Q3mmeDrawCross (const vec3_t origin, const vec4_t color)
{
	unsigned int i;
	vec3_t start, end;
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	/* Create a few lines indicating the point */
	for(i = 0; i < 3; i++) {
		VectorCopy(origin, start);
		VectorCopy(origin, end );
		start[i] -= 10;
		end[i] += 10;
		demoDrawRawLine( start, end, 0.6f, verts );
	}
}

static vec4_t SplineColor;
static int SplineNumPoints;

static void CG_DrawSplinePoints (void)
{
	int i;
	byte color[4];
	byte color2[4];
	//byte color3[4];
	byte selectedColor[4];
	//clientInfo_t ci;
	//centity_t cent;
	//vec3_t lastDrawn;
	//int lastCameraPoint;
	const cameraPoint_t *cp;

	if (!cg_drawCameraPath.integer) {
		SplineNumPoints = 0;
		trap_SetPathLines(&cg.numCameraPoints, cg.cameraPoints, &SplineNumPoints, cg.splinePoints, SplineColor);
		return;
	}

	//FIXME
	// q3mme camera
	if (cg.numQ3mmeCameraPoints > 0) {
		demoCameraPoint_t *next, *point;
		int count;
		vec3_t origin;

		MAKERGBA(color, 255, 165, 9, 255);
		point = demo.camera.points;
		count = 0;
		while (point) {
			next = point->next;

			VectorCopy(point->origin, origin);
			// draw cross like q3mme
			CG_Q3mmeDrawCross(origin, colorWhite);
			CG_Q3mmeCameraDrawPath(point, colorRed);
			origin[2] += 4;
			CG_FloatNumber(count, origin, RF_DEPTHHACK, color, 1.0);
			point = next;
			count++;
		}
	}

	if (cg.numCameraPoints < 2) {
		SplineNumPoints = 0;
		trap_SetPathLines(&cg.numCameraPoints, cg.cameraPoints, &SplineNumPoints, cg.splinePoints, SplineColor);
		//return;
		goto drawCameraPoints;
	}


	Vector4Set(SplineColor, 1, 0.5, 1, 1);
	SplineNumPoints = cg.numSplinePoints - cg.cameraPoints[0].splineStart;
	if (SplineNumPoints < 0) {
		SplineNumPoints = 0;
	}
	//trap_SetPathLines (int *numPoints, vec3_t *points, vec4_t color);
	//trap_SetPathLines (&cg.numCameraPoints, cg.cameraPoints, &SplineNumPoints, &cg.splinePoints[cg.cameraPoints[0].splineStart], SplineColor);
	trap_SetPathLines (&cg.numCameraPoints, cg.cameraPoints, &cg.numSplinePoints, cg.splinePoints, SplineColor);
	//return;

	//VectorSet(ci.color1, 1, 0.5, 1);
	//VectorSet(ci.color2, 1, 0.5, 1);
	MAKERGBA(color, 255, 40, 40, 255);
	MAKERGBA(color2, 0, 255, 40, 255);
	//MAKERGBA(color3, 255, 128, 255, 255);
	//MAKERGBA(color3, 255, 128, 255, 100);

	//memset(&cent, 0, sizeof(cent));

#if 0
	//VectorCopy(cg.splinePoints[0], lastDrawn);
	VectorCopy(cg.splinePoints[cg.cameraPoints[0].splineStart], lastDrawn);

	for (i = cg.cameraPoints[0].splineStart;  i < cg.numSplinePoints;  i++) {
		//CG_FloatNumber(i + 1, cg.splinePoints[i], RF_DEPTHHACK, color, 1.0);
#if 0
		if (i % 50 == 0) {
			vec3_t origin;
			CG_FloatNumber(i, cg.splinePoints[i], RF_DEPTHHACK, color, 1.0);
			VectorCopy(cg.splinePoints[i], origin);
			origin[2] += 30;
			CG_FloatNumber(cg.splinePointsCameraPoints[i], origin, RF_DEPTHHACK, color2, 1.0);
		}
#endif
		if (i > 0  &&  i % 10 == 0) {
			//CG_RailTrail(&ci, cg.splinePoints[i - 1], cg.splinePoints[i]);
			//CG_RailTrail(&ci, lastDrawn, cg.splinePoints[i]);
			CG_SimpleRailTrail(lastDrawn, cg.splinePoints[i], cg_railTrailTime.value, color3);
			VectorCopy(cg.splinePoints[i], lastDrawn);
			//VectorCopy(cg.splinePoints[i - 1], cent.currentState.pos.trBase);
			//VectorCopy(cg.splinePoints[i], cent.currentState.origin2);
			//CG_Beam(&cent);
		}
	}
	if (i > 0) {
		//CG_RailTrail(&ci, cg.splinePoints[cg.numSplinePoints - 1], lastDrawn);
		CG_SimpleRailTrail(cg.splinePoints[cg.numSplinePoints - 1], lastDrawn, cg_railTrailTime.value, color3);
	}
#endif

 drawCameraPoints:
	MAKERGBA(color, 0, 255, 255, 255);
	MAKERGBA(color2, 255, 0, 255, 255);
	MAKERGBA(selectedColor, 255, 255, 0, 255);
	for (i = 0;  i < cg.numCameraPoints;  i++) {
		cp = &cg.cameraPoints[i];

		if (cg.numCameraPoints > 1  &&  (cp->flags & CAM_ORIGIN)) {
			CG_FloatNumber(i, cg.splinePoints[cp->splineStart], RF_DEPTHHACK, color2, 1.0);
		}

		if (i >= cg.selectedCameraPointMin  &&  i <= cg.selectedCameraPointMax) {
			CG_FloatNumber(i, cp->origin, RF_DEPTHHACK, selectedColor, 1.0);
		} else {
			CG_FloatNumber(i, cp->origin, RF_DEPTHHACK, color, 1.0);
		}

	}

#if 0
	MAKERGBA(color, 0, 0, 255, 255);
	lastCameraPoint = 0;
	for (i = 0;  i < cg.numSplinePoints;  i++) {
		int c;
		vec3_t origin;

		c = cg.splinePointsCameraPoints[i];
		if (c != lastCameraPoint) {
			VectorCopy(cg.splinePoints[i], origin);
			origin[2] += 60;
			CG_FloatNumber(c, origin, RF_DEPTHHACK, color, 1.0);
			lastCameraPoint = c;
		}
	}
#endif
}

//FIXME name since viewpoint for camera point this and next are also shown
static void CG_DrawViewPointMark (void)
{
	qhandle_t shader;
	byte color[4];

	if (!cg_drawViewPointMark.integer) {
		return;
	}

	//if (!cg.useViewPointMark) {
	if (!cg.viewPointMarkSet) {
		return;
	}

	//shader = cgs.media.selectShader;
	//shader = cgs.media.redCubeIcon;
	shader = cgs.media.whiteShader;
	MAKERGBA(color, 255, 255, 255, 255);
	CG_FloatSprite(shader, cg.viewPointMarkOrigin, RF_DEPTHHACK, color, 8);

	if (cg.selectedCameraPointMin == cg.selectedCameraPointMax) {
		if (cg.cameraPoints[cg.selectedCameraPointMin].viewType == 2) {  //FIXME define == 2
			MAKERGBA(color, 255, 0, 0, 255);
			CG_FloatSprite(shader, cg.cameraPoints[cg.selectedCameraPointMin].viewPointOrigin, RF_DEPTHHACK, color, 4);
		}
		if (cg.selectedCameraPointMin < cg.numCameraPoints + 1) {
			if (cg.cameraPoints[cg.selectedCameraPointMin + 1].viewType == 2) {
				MAKERGBA(color, 0, 255, 0, 255);
				CG_FloatSprite(shader, cg.cameraPoints[cg.selectedCameraPointMin + 1].viewPointOrigin, RF_DEPTHHACK, color, 2);
			}
		}
	}
}

static void CG_VibrateCamera (void)
{
	double x, val;
	double vibrateTime;
	double damper;

	if (!cg_vibrate.integer) {
		cg.vibrateCameraTime = 0;
		cg.vibrateCameraValue = 0;
		cg.vibrateCameraPhase = 0;
		return;
	}

	vibrateTime = cg_vibrateTime.value;

	if (cg.ftime - cg.vibrateCameraTime > vibrateTime) {
		cg.vibrateCameraValue = 0;
		cg.vibrateCameraPhase = 0;
		//Com_Printf("returning time\n");
		return;
	}

	x = 1.0 - (float)(cg.ftime - cg.vibrateCameraTime) / vibrateTime;
	//Com_Printf("x %f\n", x);
	val = sin(M_PI * 7 * x + cg.vibrateCameraPhase) * x;

	damper = 0.25 * cg_vibrateForce.value;
	val *= cg.vibrateCameraValue * damper;
	cg.refdef.vieworg[2] += val;
	if (cg.ftime - cg.vibrateCameraTime < 50.0) {
		vec3_t ang;

		//cg.refdefViewAngles[ROLL] += 5 * cg.vibrateCameraValue;
		cg.refdefViewAngles[ROLL] += (5.0 * cg.vibrateCameraValue / 100.0);
		VectorCopy(cg.refdefViewAngles, ang);
		AnglesToAxis(ang, cg.refdef.viewaxis);
	}
}

static void CG_DrawFloatNumbers (void)
{
	int i;
	int j;

	for (i = 0;  i < cg.numFloatNumbers;  i++) {
		const floatNumber_t *f;

		f = &cg.floatNumbers[i];
		if ((f->startTime + f->time) >= cg.time) {
			CG_FloatNumber(f->number, f->origin, f->renderfx, f->color, 0.1);
		} else {
			if (cg.numFloatNumbers > 1) {
				for (j = i + 1;  j < cg.numFloatNumbers;  j++) {
					cg.floatNumbers[j - 1] = cg.floatNumbers[j];
				}
				i--;
			}
			cg.numFloatNumbers--;
		}
	}
}

static void CG_DrawPoiPics (void)
{
	int i;
	int j;
	int team;
	refEntity_t ent;

	if (cg_helpIconStyle.integer  ||  cg_helpIcon.integer == 0) {
		return;
	}

	if (wolfcam_following) {
		team = cgs.clientinfo[wcg.clientNum].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

	for (i = 0;  i < cg.numPoiPics;  i++) {
		poiPic_t *p;

		p = &cg.poiPics[i];
		if ((p->startTime + p->length) >= cg.time  &&  cg.time >= p->startTime) {
			//CG_FloatNumber(f->number, f->origin, f->renderfx, f->color, 0.1);
			//FIXME draw pic
			if (p->team != team  &&  cg_helpIconStyle.integer) {
				//Com_Printf("drawing %d/%d  team us %d  %d\n", i + 1, cg.numPoiPics, team, p->team);
				memset(&ent, 0, sizeof(ent));
				VectorCopy(p->origin, ent.origin);
				ent.origin[2] += 48;
				ent.reType = RT_SPRITE;
				if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED) {
					ent.customShader = cgs.media.infectedFoeShader;
				} else {
					if (cgs.gametype == GT_1FCTF) {
						ent.customShader = cgs.media.flagCarrierNeutral;
					} else {
						//ent.customShader = cgs.media.flagCarrier;
						//CG_Printf("^3using flag carrier shader...\n");
						ent.customShader = cgs.media.flagCarrierNeutral;
					}
				}
				ent.radius = 16;
				if (cg_helpIconStyle.integer == 1) {
					ent.radius = 16;
				} else if (cg_helpIconStyle.integer == 2) {
					ent.renderfx = RF_DEPTHHACK;
				} else {
					vec3_t org;
					float dist;
					//vec3_t dir;
					float minWidth;
					float maxWidth;
					float radius;

					ent.renderfx = RF_DEPTHHACK;
					if (wolfcam_following) {
						VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
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

				if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED) {
					ent.shaderRGBA[0] = 255;
					ent.shaderRGBA[1] = 255;
					ent.shaderRGBA[2] = 255;
				} else {
					ent.shaderRGBA[0] = 255;
					ent.shaderRGBA[1] = 255;
					if (cgs.gametype == GT_1FCTF) {
						ent.shaderRGBA[2] = 255;
					} else {
						ent.shaderRGBA[2] = 255;
						//CG_Printf("^3yellow\n");
					}
				}
				if ((p->startTime + p->length) - cg.time <= 1000) {
					ent.shaderRGBA[3] = 255.0 * ((p->startTime + p->length) - cg.time) / 1000.0;
				} else {
					ent.shaderRGBA[3] = 255;
				}
				CG_AddRefEntity(&ent);
			}
		} else {
			if (cg.numPoiPics > 1) {
				for (j = i + 1;  j < cg.numPoiPics;  j++) {
					cg.poiPics[j - 1] = cg.poiPics[j];
				}
				i--;
			}
			cg.numPoiPics--;
		}
	}
}
//=========================================================================


static void CG_CheckRepeatKeys (void)
{
    int t;
    char binding[MAX_STRING_CHARS];
    const char *s;
    char token[MAX_TOKEN_CHARS];
    qboolean newLine;
    int timing;
	int i;

    t = trap_Milliseconds();

	for (i = 0;  i < 256;  i++) {
		if (cgs.fKeyRepeat[i]) {
			trap_Key_GetBinding(i, binding);

            s = binding + strlen("++vstr");
            s = CG_GetToken(s, token, qfalse, &newLine);  // timing
            timing = atoi(token);
            if (t - cgs.fKeyPressedLastTime[i] >= timing) {
                s = CG_GetToken(s, token, qfalse, &newLine);
                if (*token) {
                    trap_SendConsoleCommand(va("vstr %s\n", token));
                }
                cgs.fKeyPressedLastTime[i] = t;
                //Com_Printf("yes %d  key[%d]\n", t, i);
            } else {
                //Com_Printf("no %d - %d < %d key[%d]\n", t, cgs.fKeyPressedLastTime[i], timing, i);
            }
		}
	}
}

static void CG_CheckCvarInterp (void)
{
	int i;
	float f = 0.0;
	cvarInterp_t *c;

	for (i = 0;  i < MAX_CVAR_INTERP;  i++) {
		c = &cg.cvarInterp[i];
		if (!c->valid) {
			continue;
		}

		if (SC_Cvar_Get_Int("com_autoWriteConfig") == 2  &&  cg.configWriteDisabled == qfalse) {
			trap_autoWriteConfig(qfalse);
			cg.configWriteDisabled = qtrue;
		}

		if (c->realTime) {
			if (cg.realTime < c->startTime) {
				c->valid = qfalse;
				continue;
			}
			if (cg.realTime >= c->endTime) {
				c->valid = qfalse;
				trap_Cvar_Set(c->cvar, va("%f", c->endValue));
				//Com_Printf("setting '%s' to %f\n", c->cvar, c->endValue);
				continue;
			} else {
				f = (cg.realTime - c->startTime) / (c->endTime - c->startTime);
			}
		} else {  // clock time
			if (cg.ftime < c->startTime) {
				c->valid = qfalse;
				continue;
			}
			if (cg.ftime >= c->endTime) {
				c->valid = qfalse;
				trap_Cvar_Set(c->cvar, va("%f", c->endValue));
				//Com_Printf("setting '%s' to %f\n", c->cvar, c->endValue);
				continue;
			} else {
				f = (cg.ftime - c->startTime) / (c->endTime - c->startTime);
			}
		}

		// change to new value
		trap_Cvar_Set(c->cvar, va("%f", c->startValue + (c->endValue - c->startValue) * f));
		//Com_Printf("real:%d setting '%s' to %f\n", c->realTime, c->cvar,  c->startValue + (c->endValue - c->startValue) * f);
	}
}

static void CG_CheckAtCommands (void)
{
	int i;
	atCommand_t *at;
	const char *s;

	for (i = 0;  i < MAX_AT_COMMANDS;  i++) {
		at = &cg.atCommands[i];
		if (!at->valid) {
			break;
		}

#if 0
		if (cg.ftime < at->lastCheckedTime) {
			// rewind
			at->lastCheckedTime = cg.ftime;
			continue;
		}
#endif

		if (cg_enableAtCommands.integer) {
			//Com_Printf("checking (%d) cg.ftime:%f  at->ftime:%f  at->lastCheckedTime:%f\n", i + 1, cg.ftime, at->ftime, at->lastCheckedTime);
			if (cg.ftime >= at->ftime  &&  at->lastCheckedTime < at->ftime) {
				s = va("%s\n", at->command);
				trap_SendConsoleCommand(s);
				//Com_Printf("executing %d:  '%s'\n", i + 1, at->command);
			}
		}

		at->lastCheckedTime = cg.ftime;
	}
}

static void CG_CheckCvarChange (void)
{
	qboolean val;
	//FIXME
	static float zoomTime = 0;
	static float q3mmeCameraSmoothPos = 0;

	if (zoomTime != cg_zoomTime.value) {
		cg.zoomTime = 0;
		cg.zoomRealTime = 0;
		cg.zoomBrokenTime = 0;
		cg.zoomBrokenRealTime = 0;
		zoomTime = cg_zoomTime.value;
	}

	val = SC_Cvar_Get_Int("com_qlcolors");
	if (val != cg.qlColors) {
		Q_SetColors(val);
		cg.qlColors = val;
	}

	if (q3mmeCameraSmoothPos != cg_q3mmeCameraSmoothPos.value) {
		CG_Q3mmeCameraResetInternalLengths();
		CG_CameraResetInternalLengths();
		q3mmeCameraSmoothPos = cg_q3mmeCameraSmoothPos.value;
	}
}

static void CG_DumpEntity (const centity_t *cent)
{
	const char *s;
	const entityState_t *es;

	es = &cent->currentState;

	// server time, entity number, entity type, origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], weapon, dead, teleported, legs anim number, torso anim number
	s = va("%f %d %d %f %f %f %f %f %f %d %d %d %d %d\n", cg_dumpEntsUseServerTime.integer ? (float)cg.snap->serverTime : cg.ftime, es->number, es->eType, cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2], cent->lerpAngles[0], cent->lerpAngles[1], cent->lerpAngles[2], es->weapon, es->eFlags & EF_DEAD, (es->eFlags ^ cent->nextState.eFlags) & EF_TELEPORT_BIT, es->legsAnim  & ~ANIM_TOGGLEBIT, es->torsoAnim  & ~ANIM_TOGGLEBIT);

	//Com_Printf("%s\n", s);
	trap_FS_Write(s, strlen(s), cg.dumpFile);
}

static void CG_DumpEntities (void)
{
	int i;
	const centity_t *cent;
	centity_t fakeCent;

	if (!cg.dumpEntities) {
		return;
	}

	if (cg.paused) {  //FIXME maybe not?
		return;
	}

	if (!cg.snap) {
		return;
	}

	if (cg_dumpEntsUseServerTime.integer  &&  cg.snap->serverTime == cg.dumpLastServerTime) {
		return;
	}

	if (cg.dumpFreecam) {
		memset(&fakeCent, 0, sizeof(fakeCent));
		VectorCopy(cg.refdef.vieworg, fakeCent.lerpOrigin);
		VectorCopy(cg.refdefViewAngles, fakeCent.lerpAngles);
		fakeCent.currentState.number = -1;
		fakeCent.currentState.eFlags = 0;
		fakeCent.nextState.eFlags = 0;
		fakeCent.currentState.legsAnim = 0;
		fakeCent.currentState.torsoAnim = 0;
		fakeCent.currentState.eType = 0;
		fakeCent.currentState.weapon = 0;
		CG_DumpEntity(&fakeCent);
	}

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		if (i == cg.snap->ps.clientNum) {
			cent = &cg.predictedPlayerEntity;
		} else {
			cent = &cg_entities[i];
		}

		if (!cg.dumpValid[i]) {
			continue;
		}

		if (!cent->currentValid  &&  cent != &cg.predictedPlayerEntity) {
			continue;
		}
		//Com_Printf("%d\n", cent->currentState.number);

		CG_DumpEntity(cent);
	}

	cg.dumpLastServerTime = cg.snap->serverTime;
}


static void CG_CheckPlayerKeyPress (void)
{
	vec3_t forward, back, right, up;
	//vec3_t left;
	//vec3_t prevForward, prevBack, prevRight, prevLeft, prevUp;
	//vec3_t down;
	vec3_t velocity;
	//vec3_t prevVelocity;
	float threshold;
	qboolean f, b, r, l, u, d;
	int movementDir;
	int legsAnim;

	cg.playerKeyPressCrouch = qfalse;
	cg.playerKeyPressFire = qfalse;
	cg.playerKeyPressJump = qfalse;

	if (wolfcam_following) {
		legsAnim = cg_entities[wcg.clientNum].currentState.legsAnim & ~ANIM_TOGGLEBIT;

		if (cg_entities[wcg.clientNum].currentState.eFlags & EF_FIRING) {
			cg.playerKeyPressFire = qtrue;
		}
		if (legsAnim == LEGS_WALKCR  ||  legsAnim == LEGS_IDLECR) {
			cg.playerKeyPressCrouch = qtrue;
		}

#if 0
		if (legsAnim == LEGS_JUMP  ||  legsAnim == LEGS_JUMPB) {
			// this is also set in free fall
			if (cg_entities[wcg.clientNum].currentState.groundEntityNum != ENTITYNUM_NONE) {
				// check doesn't work, already airborne
				//cg.playerKeyPressJump = qtrue;
			}
		}
#endif

		// key press jump checked with EV_JUMP

		if (cg.time - wclients[wcg.clientNum].jumpTime <= 150) {
			cg.playerKeyPressJump = qtrue;
		}
	} else {
		legsAnim = cg.snap->ps.legsAnim & ~ANIM_TOGGLEBIT;

		if (cg.snap->ps.eFlags & EF_FIRING) {
			cg.playerKeyPressFire = qtrue;
		}
		if (cg.snap->ps.pm_flags & PMF_DUCKED) {
			cg.playerKeyPressCrouch = qtrue;
		}
		if (cg.snap->ps.pm_flags & PMF_JUMP_HELD) {
			cg.playerKeyPressJump = qtrue;
		}
	}

	if (wolfcam_following) {
		movementDir = cg_entities[wcg.clientNum].currentState.angles2[YAW];
	} else {
		movementDir = cg.snap->ps.movementDir;
	}

	f = b = r = l = u = d = qfalse;

	switch (movementDir) {
	case 0:
		f = qtrue;
		break;
	case 1:
		l = f = qtrue;
		break;
	case 2:
		l = qtrue;
		break;
	case 3:
		l = b = qtrue;
		break;
	case 4:
		b = qtrue;
		break;
	case 5:
		r = b = qtrue;
		break;
	case 6:
		r = qtrue;
		break;
	case 7:
		r = f = qtrue;
		break;
	default:
		break;
	}

	// these are not accurate
	if (movementDir == 1  ||  movementDir == 7) {
		//Com_Printf("forward unknown: %d\n", movementDir);
	}

	f = qfalse;
	//Com_Printf("%f\n", AngleBetweenVectors(velocity, forward));

	cg.playerKeyPressForward = f;
	cg.playerKeyPressBack = b;
	cg.playerKeyPressLeft = l;
	cg.playerKeyPressRight = r;

	if (wolfcam_following) {
		VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trDelta, velocity);
	} else {
		VectorCopy(cg.snap->ps.velocity, velocity);
		//VectorCopy(cg.prevSnap->ps.velocity, prevVelocity);
	}

	//FIXME can still use feet animation
	//if (VectorLength(velocity) == 0) {
	if (legsAnim == LEGS_IDLE  ||  legsAnim == LEGS_IDLECR) {
		//return;
#if 1
		cg.playerKeyPressForward = qfalse;
		cg.playerKeyPressBack = qfalse;
		cg.playerKeyPressLeft = qfalse;
		cg.playerKeyPressRight = qfalse;
#endif
	} else {
		//Com_Printf("^3legs anim: %d\n", legsAnim);
	}

	VectorNormalize(velocity);

	if (wolfcam_following) {
		AngleVectors(cg_entities[wcg.clientNum].currentState.apos.trBase, forward, right, up);
	} else {
		AngleVectors(cg.snap->ps.viewangles, forward, right, up);
		//AngleVectors(cg.prevSnap->ps.viewangles, prevForward, prevRight, prevUp);
	}

	VectorScale(forward, -1, back);
	//VectorScale(right, -1, left);
	//VectorScale(up, -1, down);

	// testing just velocity based
#if 0
	{
		vec3_t start;
		vec3_t p;
		vec_t vlen;
		vec3_t point;

		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trBase, start);
		} else {
			VectorCopy(cg.snap->ps.origin, start);
		}

		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trDelta, velocity);
		} else {
			VectorCopy(cg.snap->ps.velocity, velocity);
			//VectorCopy(cg.prevSnap->ps.velocity, prevVelocity);
		}

		VectorClear(start);

		//f = b = l = r = qfalse;
		cg.playerKeyPressForward = cg.playerKeyPressBack = cg.playerKeyPressRight = cg.playerKeyPressLeft = qfalse;

		VectorMA(start, 1, velocity, point);

		//ProjectPointOntoVector(velocity, start, forward, p);
		ProjectPointOntoVector(point, start, forward, p);
		//f = VectorLength(p);
		vlen = VectorLength(p);
		//Com_Printf("viewangles: %f %f %f\n", cg.snap->ps.viewangles[0], cg.snap->ps.viewangles[1], cg.snap->ps.viewangles[2]);

		if (vlen > 0.101) {
			cg.playerKeyPressForward = qtrue;
			Com_Printf("vlen forw %f  vel: %f\n", vlen, VectorLength(velocity));
		}
		//ProjectPointOntoVector(velocity, start, right, p);
		ProjectPointOntoVector(point, start, right, p);
		//r = VectorLength(p);
		vlen = VectorLength(p);

		if (vlen > 0.101) {
			cg.playerKeyPressRight = qtrue;
			Com_Printf("vlen right %f  vel: %f\n", vlen, VectorLength(velocity));
		}
		//ProjectPointOntoVector(velocity, start, up, p);
		//u = VectorLength(p);
	}
#endif

	threshold = 10;

	//if (AngleBetweenVectors(velocity, forward)) { };
	if ((RAD2DEG(AngleBetweenVectors(velocity, forward)) - threshold) < 45) {
		cg.playerKeyPressForward = qtrue;
	} else if ((RAD2DEG(AngleBetweenVectors(velocity, back)) - threshold) < 45) {
		//FIXME check?
	}

#if 0
	if ((RAD2DEG(AngleBetweenVectors(velocity, right)) - threshold) < 45) {
		// right
	} else if ((RAD2DEG(AngleBetweenVectors(velocity, left)) - threshold) < 45) {
		// left
	}
#endif

	//Com_Printf("backwards run: %d  backward jump: %d\n", cg.snap->ps.pm_flags & PMF_BACKWARDS_RUN, cg.snap->ps.pm_flags & PMF_BACKWARDS_JUMP);
#if 0
	Com_Printf("f: %f   r:  %f   u:  %f\n", f, r, u);
	CG_PrintToScreen("f: %f   r:  %f   u:  %f", f, r, u);
#endif
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/

#if 0  //def CGAMESO

//extern int Q_stricmpTotalTime;  // testing
extern int64_t QstrcmpCount;
#else
extern int QstrcmpCount;

#endif

void CG_DrawActiveFrame (int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, qboolean videoRecording, int ioverf, qboolean draw)
{
	int		inwater = qfalse;
	int currentWeapon;
	//int startTime;
	int oldClientNum;

	//cg.drawActiveFrameCount++;

	if (SC_Cvar_Get_Int("debug_cgame_time")) {
		Com_Printf("cgame time: %d  %d\n", serverTime, ioverf);
	}

#if 0
	if (!SC_Cvar_Get_Int("cl_freezeDemo")) {
		cg.demoSeeking = qfalse;
	}
#endif

	FxLoopSounds = FX_LOOP_SOUNDS_BASE;
	CG_ClearLocalFrameEntities();
	CG_CleanUpFieldNumber();

	//FIXME if r_mode changes
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;
	//Com_Printf("w %d  h %d\n", cgs.glconfig.vidWidth, cgs.glconfig.vidHeight);
	memcpy(&cgDC.glconfig, &cgs.glconfig, sizeof(glconfig_t));
	cgDC.widescreen = cg_wideScreen.integer;

    //CG_Printf ("CG_DrawActiveFrame: serverTime:%d\n", serverTime);
#if 0
	if (cg.snap  &&  cg.nextSnap) {
		Com_Printf("%d\n", cg.nextSnap->serverTime - cg.snap->serverTime);
	}
#endif

#if 0
	if (serverTime == cg.time  &&  cg.ioverf == ioverf) {  //FIXME call it something else
		//if (SC_Cvar_Get_Int("cl_freezeDemo")) {
			cg.paused = qtrue;
			//CG_Printf("paused\n");
			//}
	} else {
		cg.paused = qfalse;
	}
#endif

	cg.paused = SC_Cvar_Get_Int("cl_freezeDemo");

	if (!cg.paused) {
		cg.atCameraPoint = qfalse;
	}

	cg.videoRecording = videoRecording;
	cg.ioverf = ioverf;
	//Com_Printf("ioverf %d\n", ioverf);
	cg.foverf = (double)ioverf / SUBTIME_RESOLUTION;
	cg.draw = draw;
	cg.realTime = trap_Milliseconds();
	cgDC.realTime = cg.realTime;
	//CG_Printf("serverTime: %d\n", serverTime);

	cg.time = serverTime;
	cg.ftime = (double)cg.time + cg.foverf;
	cgDC.cgTime = cg.ftime;

	//Com_Printf("%d  %f  %f\n", cg.time, cg.ftime, cg.foverf);

	if (cg.ftime != cg.cameraPointCommandTime) {
		//cg.cameraPointCommandTime = -1;
	}

#if 0
	if (cg.demoSeeking) {
		Com_Printf("seek %f\n", cg.ftime);
	}
#endif

	cg.demoPlayback = demoPlayback;

	if (cg.configWriteDisabled) {
		trap_autoWriteConfig(qtrue);
		cg.configWriteDisabled = qfalse;
	}

	CG_CheckCvarChange();
	CG_CheckRepeatKeys();
	CG_CheckCvarInterp();
	CG_CheckAtCommands();

	if (cg.looping  &&  serverTime >= cg.loopEndTime) {
		//Com_Printf("looping\n");
		trap_SendConsoleCommand(va("seekservertime %f\n", (double)cg.loopStartTime));
		//cg.demoSeeking = qfalse;  //FIXME maybe?
		return;
	}

	if (cg.fragForwarding) {
		int cn, st;
		int cn2, st2;
		qboolean ourClientChanged = qfalse;

		st = st2 = 0;

		if ( (wcg.nextVictimServerTime >= 0  &&  cg.snap->serverTime > wcg.nextVictimServerTime + cg.fragForwardDeathHoverTime)  ||  wcg.ourLastClientNum != cg.snap->ps.clientNum) {
			qboolean haveVictims, haveVictims2;

			haveVictims = trap_GetNextVictim(cg.snap->ps.clientNum, wcg.nextVictimServerTime + 200, &cn, &st, qtrue);
			haveVictims2 = trap_GetNextVictim(cg.clientNum, wcg.nextVictimServerTime + 200, &cn2, &st2, qtrue);
			if (haveVictims  ||  haveVictims2) {
				//Com_Printf("next victim for %s: %d %d %s\n", cgs.clientinfo[cg.snap->ps.clientNum].name, wcg.nextVictim, wcg.nextVictimServerTime, cgs.clientinfo[wcg.nextVictim].name);
				//FIXME hack to not switch away so fast
				if (st2 > 0) {
					if (st == 0  ||  st2 < st) {
						st = st2;
						cn = cn2;
					}
				}
				wcg.nextVictimServerTime += 0;  //2000;
				if (wcg.ourLastClientNum != cg.snap->ps.clientNum) {
					wcg.nextVictimServerTime = st;
					wcg.nextVictim = cn;
					wcg.ourLastClientNum = cg.snap->ps.clientNum;
					ourClientChanged = qtrue;
				}
				cg.fragForwardFragCount++;
				Com_Printf("fragforward frag count %d\n", cg.fragForwardFragCount);

				if (!ourClientChanged) {
					if (st - cg.fragForwardPreKillTime < cg.snap->serverTime) {
						wcg.nextVictim = cn;
						wcg.nextVictimServerTime = st;
						// don't rewind to a frag,
						Com_Printf("^5frag run\n");
						goto dontseek;
					}
				}
				wcg.nextVictim = cn;
				wcg.nextVictimServerTime = st;

				trap_SendConsoleCommand("exec fragforwardnext.cfg\n");
				trap_SendConsoleCommand(va("seekservertime %f\n", (double)(wcg.nextVictimServerTime - cg.fragForwardPreKillTime)));
			} else {
				//cg.showScores = qfalse;
				cg.fragForwarding = qfalse;
				wcg.nextVictim = -1;
				wcg.nextVictimServerTime = -1;
				trap_SendConsoleCommand("exec fragforwarddone.cfg\n");
			}
		}
	}
 dontseek:
	// update cvars
	CG_UpdateCvars();
	CG_Q3ColorFromString(SC_Cvar_Get_String("color1"), cg.color1);
	CG_Q3ColorFromString(SC_Cvar_Get_String("color2"), cg.color2);
	CG_CheckFontUpdates();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation(qtrue);
		cg.demoSeeking = qfalse;
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	if (draw) {  //(1)  {  //!paused) {
		trap_R_ClearScene();
		CG_DrawDecals();
		//CG_ForceBModels();
	}

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();
	oldClientNum = wcg.clientNum;
	if (wolfcam_following) {  //  &&  !cg.fragForwarding) {
		static int wolfcamLastClientNum = -1;

		wcg.clientNum = wolfcam_find_client_to_follow();
		if (wolfcamLastClientNum == -1) {
			wolfcamLastClientNum = wcg.clientNum;
		}

		if (wolfcamLastClientNum != wcg.clientNum) {
			if (*cg_firstPersonSwitchSound.string) {
				sfxHandle_t sfx;
				qboolean play = qfalse;

				if (wolfcam_firstPersonSwitchSoundStyle.integer == 0) {
					play = qfalse;
				} else if (wolfcam_firstPersonSwitchSoundStyle.integer == 2) {
					play = qtrue;
				} else if (wcg.followMode == WOLFCAM_FOLLOW_VICTIM) {
					if (wolfcamLastClientNum == wcg.nextVictim  ||  wcg.clientNum == wcg.nextVictim) {
						play = qtrue;
					}
				} else if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
					if (wolfcamLastClientNum == wcg.nextKiller  ||  wcg.clientNum == wcg.nextKiller) {
						play = qtrue;
					}
				} else {
					if (wolfcamLastClientNum == wcg.selectedClientNum  ||  wcg.clientNum == wcg.selectedClientNum) {
						play = qtrue;
					}
				}

				if (play) {
					sfx = trap_S_RegisterSound(cg_firstPersonSwitchSound.string, qfalse);
					CG_StartLocalSound(sfx, CHAN_LOCAL_SOUND);
				}
			}
			if (cg_useDemoFov.integer == 2) {
				//Com_Printf("^2zoom up cg_view.c\n");
				CG_ZoomUp_f();
			}
			trap_SendConsoleCommand("exec wolfcamfirstpersonswitch.cfg\n");
			if (wcg.clientNum == cg.snap->ps.clientNum) {
				trap_SendConsoleCommand("exec wolfcamfirstpersonviewdemotaker.cfg\n");
			} else {
				trap_SendConsoleCommand("exec wolfcamfirstpersonviewother.cfg\n");
			}
		}
		wolfcamLastClientNum = wcg.clientNum;
	}

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation(qtrue);
		cg.demoSeeking = qfalse;
		return;
	}

	CG_CheckPlayerKeyPress();

	//FIXME hack for scoreboard auto exec.  Done here since checking in
	// cg_draw.c is alot more complicated
	{
		static int scoreBoardShowing = -1;  //FIXME hack int

		if (scoreBoardShowing != cg.scoreBoardShowing) {
			if (cg.scoreBoardShowing) {
				trap_SendConsoleCommand("exec scoreboardon.cfg\n");
			} else {
				trap_SendConsoleCommand("exec scoreboardoff.cfg\n");
			}
		}

		scoreBoardShowing = cg.scoreBoardShowing;
	}

	if (cg.snap  &&  cg.snap->ps.pm_type != PM_INTERMISSION  &&  !wolfcam_following  &&  !cg.freecam) {
		static int lastTeam = 0;  //FIXME vid restart?
		static int lastpm_type = 0;
		static int lastClientNum = -1;

		int team;
		int pm_type;

		team = cgs.clientinfo[cg.clientNum].team;
		pm_type = cg.snap->ps.pm_type;

		if (lastTeam != team) {
			if (team == TEAM_SPECTATOR) {
				trap_SendConsoleCommand("exec spectator.cfg\n");
				//Menu_Paint(Menus_FindByName("spechud_menu"), qtrue);
			} else if (team != TEAM_SPECTATOR) {
				trap_SendConsoleCommand("exec ingame.cfg\n");
			}
		}
		if (lastpm_type != pm_type) {  //FIXME PM_NOCLIP
			if (pm_type == PM_NORMAL  &&  lastpm_type == PM_SPECTATOR) {
				trap_SendConsoleCommand("exec ingame.cfg\n");
			} else if (pm_type == PM_SPECTATOR) {
				trap_SendConsoleCommand("exec spectator.cfg\n");
			}
		}

		if (lastClientNum == -1) {
			lastClientNum = cg.snap->ps.clientNum;
		}

		if (cg.snap->ps.clientNum != lastClientNum) {
			if (*cg_firstPersonSwitchSound.string) {
				sfxHandle_t sfx;

				sfx = trap_S_RegisterSound(cg_firstPersonSwitchSound.string, qfalse);
				CG_StartLocalSound(sfx, CHAN_LOCAL_SOUND);
			}
			if (cg_useDemoFov.integer == 2) {
				//Com_Printf("^2zoom up cg_view.c 2\n");
				CG_ZoomUp_f();
			}
			trap_SendConsoleCommand("exec firstpersonswitch.cfg\n");
		}
		lastTeam = team;
		lastpm_type = cg.snap->ps.pm_type;
		lastClientNum = cg.snap->ps.clientNum;
	}

	if (draw) {
		CG_DrawAdvertisements();
		CG_ForceBModels();
		//CG_DrawDecals();
		CG_DrawSpawns();
		CG_DrawRawCameraPathKeyPoints();
		CG_DrawSplinePoints();
		CG_DrawViewPointMark();
		CG_DrawFloatNumbers();
		CG_DrawPoiPics();
	}

#if 0  // testing
	{
		refEntity_t ent;

		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_MODEL;
		ent.hModel = cgs.media.teleportEffectModel;
		AxisClear(ent.axis);
		//ent.origin[2] += 16;
		ent.customShader = 0;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;

		VectorCopy(cg.startTimerOrigin, ent.origin);
		CG_AddRefEntity(&ent);

	}
#endif

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// decide on third person view
	//cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0) || cg.freecam  ||  (cg.snap->ps.pm_type == PM_SPECTATOR  &&  cg.snap->ps.clientNum == cg.clientNum);
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0  &&  cg_deathStyle.integer != 4) || cg.freecam;
	//cg.renderingThirdPerson = cg_thirdPerson.integer ||  cg.freecam;

	// build cg.refdef
    if (!wolfcam_following  ||  (wolfcam_following  &&  wcg.clientNum == cg.snap->ps.clientNum)) {
        inwater = CG_CalcViewValues();
	}

	Wolfcam_LoadModels();

	// first person blend blobs, done after AnglesToAxis
	//if (!wolfcam_following  &&  !cg.renderingThirdPerson) {
	if (CG_IsFirstPersonView(cg.snap->ps.clientNum)) {
		CG_DamageBlendBlob();
	} else if (wolfcam_following  &&  !(cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD)) {
        //Wolfcam_DamageBlendBlob ();
    }

	// build the render lists
	if ( !cg.hyperspace  ||  wolfcam_following) {
		CG_AddPacketEntities();			// alter calcViewValues, so predicted player state is correct
        //Wolfcam_MarkValidEntities();
		//Com_Printf("cgs.gametype: %d\n", cgs.gametype);
		// cg.snap->ps.persistant[PERS_HWEAPON_USE]
#if 0  // these seem ok
		Com_Printf("score:%d  hits:%d  rank:%d  team:%d  spawnCount:%d\n",
				   cg.snap->ps.persistant[PERS_SCORE],
				   cg.snap->ps.persistant[PERS_HITS],
				   cg.snap->ps.persistant[PERS_RANK],
				   cg.snap->ps.persistant[PERS_TEAM],
				   cg.snap->ps.persistant[PERS_SPAWN_COUNT]);
#endif

#if 0  // ATTACKEE_ARMOR is wrong
		Com_Printf("playerEvents:%d  attacker:%d  killed:%d  imCount:%d\n",
				   cg.snap->ps.persistant[PERS_PLAYEREVENTS],
				   cg.snap->ps.persistant[PERS_ATTACKER],
				   //cg.snap->ps.persistant[PERS_ATTACKEE_ARMOR],
				   cg.snap->ps.persistant[PERS_KILLED],
				   cg.snap->ps.persistant[PERS_IMPRESSIVE_COUNT]);
#endif

#if 0
		Com_Printf("exCount:%d  defendCount:%d  assistCount:%d  gauntFragCount:%d  attArmor:%d\n",
				   cg.snap->ps.persistant[PERS_EXCELLENT_COUNT],
				   cg.snap->ps.persistant[PERS_DEFEND_COUNT],
				   cg.snap->ps.persistant[PERS_ASSIST_COUNT],
				   cg.snap->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
				   cg.snap->ps.persistant[PERS_CAPTURES],
				   cg.snap->ps.persistant[PERS_ATTACKEE_ARMOR]);
#endif

        if (wolfcam_following) {
			//FIXME player models and also select client to view should happen before CG_AddPacketEntities()  -- probable cause of flashing when changing views
            Wolfcam_TransitionPlayerState(oldClientNum);
			Wolfcam_OffsetFirstPersonView();
			if (wolfcam_fixedViewAngles.integer > 0) {
				static int oldTime = 0;
				static vec3_t oldViewAngles = { 0.0f, 0.0f, 0.0f };
				//static vec3_t oldViewaxis = { 0.0f, 0.0f, 0.0f };

				if (cg.time - oldTime > wolfcam_fixedViewAngles.integer) {
					VectorCopy (cg.refdefViewAngles, oldViewAngles);
					//VectorCopy (cg.refdef.viewaxis, oldViewaxis);
					oldTime = cg.time;
					//CG_Printf ("new angle\n");
				} else {
					//CG_Printf ("using old %f %f %f\n", oldViewAngles);
				}

				VectorCopy (oldViewAngles, cg.refdefViewAngles);
				//VectorCopy (oldViewaxis, cg.refdef.viewaxis);
				AnglesToAxis (cg.refdefViewAngles, cg.refdef.viewaxis);
			}

#if 0
            if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD)
                Wolfcam_OffsetThirdPersonView ();
            else {
                Wolfcam_OffsetFirstPersonView ();
                //Wolfcam_DamageBlendBlob ();  //FIXME wolfcam
            }
#endif
            inwater = Wolfcam_CalcFov ();
        }
		if (draw) {
			CG_AddMarks();
			CG_AddParticles();
		}
#if 0  //def CGAMESO
		//Q_stricmpTotalTime = 0;
		QstrcmpCount = 0;
#else
		QstrcmpCount = 0;
#endif

		//startTime = trap_Milliseconds();
		//CG_AddLocalEntities();
		//Com_Printf("AddLocalEntities: %d\n", trap_Milliseconds() - startTime);
		//Com_Printf("qstricmp count %lld\n", QstrcmpCount);
#if 0  //def CGAMESO
		//Com_Printf("qstricmp time: %lld\n", QstrcmpCount);
#endif

	}
	if (cg.freecam) {  //FIXME what else? idcamera intermission?
		currentWeapon = WP_NONE;
	} else if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		currentWeapon = WP_NONE;
	} else if (wolfcam_following) {
		currentWeapon = cg_entities[wcg.clientNum].currentState.weapon;
	} else {
		currentWeapon = cg.snap->ps.weapon;
	}

	if (currentWeapon != cg.lastWeapon) {
		const vmCvar_t *weapCvar[] = {
			&cg_weaponNone,
			&cg_weaponGauntlet,
			&cg_weaponMachineGun,
			&cg_weaponShotgun,
			&cg_weaponGrenadeLauncher,
			&cg_weaponRocketLauncher,
			&cg_weaponLightningGun,
			&cg_weaponRailGun,
			&cg_weaponPlasmaGun,
			&cg_weaponBFG,
			&cg_weaponGrapplingHook,
			&cg_weaponNailGun,
			&cg_weaponProximityLauncher,
			&cg_weaponChainGun,
			&cg_weaponHeavyMachineGun,
		};

		//Com_Printf("oldweapon %d  newWeapon %d\n", cg.lastWeapon, currentWeapon);
		if (currentWeapon > WP_HEAVY_MACHINEGUN  ||  currentWeapon <= WP_NONE) {
			if (*cg_weaponNone.string) {
				trap_SendConsoleCommand(va("%s\n", cg_weaponNone.string));
			}
		} else {
			if (*weapCvar[currentWeapon]->string) {
				trap_SendConsoleCommand(va("%s\n", weapCvar[currentWeapon]->string));
			} else {
				if (*cg_weaponDefault.string) {
					trap_SendConsoleCommand(va("%s\n", cg_weaponDefault.string));
				}
			}
		}
		cg.lastWeapon = currentWeapon;
	}

	CG_VibrateCamera();

	//FIXME should check wolfcam_following ?
	if (1) { //(!wolfcam_following) {
		CG_AddViewWeapon( &cg.predictedPlayerState );
	}

	if (wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum) {
		Wolfcam_AddViewWeapon();
	}
	//CG_VibrateCameraAngles();

	// add buffered sounds
	CG_PlayBufferedSounds();

#ifdef MISSIONPACK
	// play buffered voice chats
	CG_PlayBufferedVoiceChats();
#endif

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// update audio positions
    if (wolfcam_following) {
        if (wcg.playHitSound  &&  cg_hitBeep.integer  &&  wcg.clientNum != cg.snap->ps.clientNum) {
            CG_StartLocalSound (cgs.media.hitSound, CHAN_LOCAL_SOUND);
            wcg.playHitSound = qfalse;
			wcg.playTeamHitSound = qfalse;
        } else if (wcg.playTeamHitSound  &&  cg_hitBeep.integer  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			if (cgs.gametype != GT_CA  &&  cgs.gametype != GT_FREEZETAG) {
				CG_StartLocalSound (cgs.media.hitTeamSound, CHAN_LOCAL_SOUND);
			}
            wcg.playHitSound = qfalse;
			wcg.playTeamHitSound = qfalse;
		}
        trap_S_Respatialize (wcg.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);
    } else {
        trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );
	}

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		if (cg.oldTime == 0) {
			cg.frametime = 0;
		}
		// just in case
		if (cg.frametime > 200) {  //  &&  cg.draw) {
			if (cg.draw  &&  SC_Cvar_Get_Float("timescale") <= 1.0) {
				int snapNum = 0;
				//int snapNextNum = 0;

				if (cg.snap) {
					snapNum = cg.snap->messageNum;
				}
#if 0
				if (cg.nextSnap) {
					snapNextNum = cg.nextSnap->messageNum;
				}
#endif
				Com_Printf("cg.frametime > 200  (%d)  oldSnapNum %d  snapNum %d\n", cg.frametime, cg.oldSnapNum, snapNum);
			}
			cg.frametime = 200;
		}
		cg.oldTime = cg.time;
		if (cg.snap) {
			cg.oldSnapNum = cg.snap->messageNum;
		} else {
			cg.oldSnapNum = 0;
		}
		if (!cg.paused) {
			CG_AddLagometerFrameInfo();
		}
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	if (cg.freecam) {
		CG_FreeCam();
		CG_VibrateCamera();
	}

	if (cg.cameraPlaying  ||  cg.cameraQ3mmePlaying  ||  cg_cameraQue.integer) {
		CG_PlayCamera();
		cg.refdef.time = cg.time;
		if (cg.freecam) {
			trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);
		}
	} else {
		cg.cameraPlayedLastFrame = qfalse;
	}

	//Com_Printf("fpos %f %f %f\n", cg.fpos[0], cg.fpos[1], cg.fpos[2]);

#if 0  // old
	if (cg.recordPath  &&  !cg.paused) {
		if (cg.snap->serverTime != cg.recordPathLastServerTime) {
			const char *s;

			//s = va("%d %f %f %f %f %f %f %d\n", cg.snap->serverTime, ps->origin[0], ps->origin[1], ps->origin[2], ps->viewangles[0], ps->viewangles[1], ps->viewangles[2], cg.viewEnt);
			s = va("%d %f %f %f %f %f %f %d\n", cg.snap->serverTime, cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2], cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2], cg.viewEnt);
			trap_FS_Write(s, strlen(s), cg.recordPathFile);
			cg.recordPathLastServerTime = cg.snap->serverTime;
			trap_SendConsoleCommand("echopopup recording path\n");
		}
	}
#endif

	if (cg.recordPath  &&  !cg.paused) {
		//if (cg.snap->serverTime != cg.recordPathLastServerTime) {
			const char *s;

			//s = va("%d %f %f %f %f %f %f %d\n", cg.snap->serverTime, ps->origin[0], ps->origin[1], ps->origin[2], ps->viewangles[0], ps->viewangles[1], ps->viewangles[2], cg.viewEnt);
			s = va("%f %f %f %f %f %f %f %d\n", cg.ftime, cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2], cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2], cg.viewEnt);
			trap_FS_Write(s, strlen(s), cg.recordPathFile);
			cg.recordPathLastServerTime = cg.snap->serverTime;
			trap_SendConsoleCommand("echopopup recording path\n");
			//}
	}

	if (cg.cameraMode) {
		trap_S_Respatialize(MAX_GENTITIES - 1, cg.refdef.vieworg, cg.refdef.viewaxis, qfalse);
		CG_VibrateCamera();  //FIXME double check
	}

	if (draw) {
		CG_AddLocalEntities();
	}
	//FIXME force
	//CG_UpdateFxExternalForces();

	CG_DumpEntities();

	//CG_VibrateCamera();
	CG_CheckSkillRating();

	{
		static int lastWarmupCount = 0;

		if (cg.warmup == 0  &&  cg.warmupCount == -1  &&  lastWarmupCount == 0) {
			//Com_Printf("warmup %d  warmupcount %d\n", cg.warmup, cg.warmupCount);
			// game has started
			cg.menuScoreboard = NULL;  //FIXME hack
			trap_SendConsoleCommand("exec gamestart.cfg\n");
		}
		lastWarmupCount = cg.warmupCount;
	}

	// actually issue the rendering calls
	if (draw) {
		CG_DrawActive( stereoView );
	}

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}

    //if (cg_timescale.value < 0.1)
        //Wolfcam_Stall (1073741824 / 4);

	if (!draw) {
		//trap_R_ClearScene();
	}

	cg.demoSeeking = qfalse;
}

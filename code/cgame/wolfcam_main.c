#include "cg_local.h"

#include "cg_main.h"
#include "cg_syscalls.h"
#include "sc.h"
#include "wolfcam_main.h"

#include "wolfcam_local.h"
#include "../game/bg_local.h"

wcg_t wcg;
wclient_t wclients[MAX_CLIENTS];
woldclient_t woldclients[MAX_CLIENTS];
int wnumOldClients;

qboolean wolfcam_following;

vmCvar_t wolfcam_fixedViewAngles;
vmCvar_t cg_useOriginalInterpolation;
vmCvar_t cg_drawBBox;

// 2016-07-12 unused
/*
char *weapNames[] = {
    "WP_NONE",
    "WP_GAUNTLET",
    "WP_MACHINEGUN",
    "WP_SHOTGUN",
    "WP_GRENADE_LAUNCHER",
    "WP_ROCKET_LAUNCHER",
    "WP_LIGHTNING",
    "WP_RAILGUN",
    "WP_PLASMAGUN",
    "WP_BFG",
    "WP_GRAPPLING_HOOK",
    "WP_NAILGUN",
    "WP_PROX_LAUNCHER",
    "WP_CHAINGUN",
	"WP_HEAVY_MACHINEGUN",
	"WP_KAMIKAZE",
	"UNDEFINED",
};
*/

char *weapNamesCasual[] = {
    "",
    "Gauntlet",
    "Machine Gun",
    "Shotgun",
    "Grenade Launcher",
    "Rocket Launcher",
    "Lightning Gun",
    "Rail Gun",
    "Plasma Gun",
    "BFG",
    "Grappling Hook",
    "Nail Gun",
    "Proximity Launcher",
    "Chain Gun",
	"Heavy Machine Gun",
	"Kamikaze",
	"undefined",
};

void Wolfcam_AddBox (const vec3_t origin, int x, int y, int z, int red, int green, int blue)
{
	polyVert_t verts[4];
	int i;
	float extx, exty, extz;
	vec3_t corners[8];
	vec3_t mins, maxs;

	// if they don't exist, forget it
	if (!cgs.media.bboxShader  ||  !cgs.media.bboxShader_nocull) {
		return;
	}

	VectorCopy(bg_playerMins, mins);
	VectorCopy(bg_playerMaxs, maxs);

	mins[0] = 0 - (x / 2);
	mins[1] = 0 - (y / 2);
	mins[2] = 0 - (z / 2);
	maxs[0] = (x / 2);
	maxs[1] = (y / 2);
	maxs[2] = (z / 2);

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];

	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	for (i = 0;  i < 4;  i++) {
		verts[i].modulate[0] = red;
		verts[i].modulate[1] = green;
		verts[i].modulate[2] = blue;
		verts[i].modulate[3] = 255;
	}

	VectorAdd(origin, maxs, corners[3]);

	VectorCopy(corners[3], corners[2]);
	corners[2][0] -= extx;

	VectorCopy(corners[2], corners[1]);
	corners[1][1] -= exty;

	VectorCopy(corners[1], corners[0]);
	corners[0][0] += extx;

	for (i = 0;  i < 4;  i++) {
		VectorCopy( corners[i], corners[i + 4] );
		corners[i + 4][2] -= extz;
	}

	// top
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader, 4, verts, qfalse );

	// bottom
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader, 4, verts, qfalse );

	// top side
	VectorCopy( corners[3], verts[0].xyz );
	VectorCopy( corners[2], verts[1].xyz );
	VectorCopy( corners[6], verts[2].xyz );
	VectorCopy( corners[7], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// left side
	VectorCopy( corners[2], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[6], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// right side
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[3], verts[1].xyz );
	VectorCopy( corners[7], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// bottom side
	VectorCopy( corners[1], verts[0].xyz );
	VectorCopy( corners[0], verts[1].xyz );
	VectorCopy( corners[4], verts[2].xyz );
	VectorCopy( corners[5], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );
}

/*
=================
Wolfcam_AddBoundingBox

Draws a bounding box around a player.  Called from CG_Player.
=================
*/
void Wolfcam_AddBoundingBox( const centity_t *cent ) {
	polyVert_t verts[4];
	const clientInfo_t *ci;
	int i;
	vec3_t mins, maxs;
	float extx, exty, extz;
	vec3_t corners[8];

	if ( !cg_drawBBox.integer ) {
		return;
	}

	// don't draw it if it's us in first-person
	if ( cent->currentState.number == cg.predictedPlayerState.clientNum &&
			!cg.renderingThirdPerson ) {
		return;
	}

	// don't draw it for dead players
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	VectorCopy(bg_playerMins, mins);
	VectorCopy(bg_playerMaxs, maxs);

	// if they don't exist, forget it
	if ( !cgs.media.bboxShader || !cgs.media.bboxShader_nocull ) {
		return;
	}

	// get the player's client info
	ci = &cgs.clientinfo[cent->currentState.clientNum];

	// if it's us
	if ( cent->currentState.number == cg.predictedPlayerState.clientNum ) {
		// use the view height
		maxs[2] = cg.predictedPlayerState.viewheight + 6;
	}
	else {
		int x, zd, zu;

		// otherwise grab the encoded bounding box
		x = (cent->currentState.solid & 255);
		zd = ((cent->currentState.solid>>8) & 255);
		zu = ((cent->currentState.solid>>16) & 255) - 32;

		mins[0] = mins[1] = -x;
		maxs[0] = maxs[1] = x;
		mins[2] = -zd;
		maxs[2] = zu;
	}

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];

	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	// set the polygon's vertex colors
	if ( ci->team == TEAM_RED ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 160;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}
	else if ( ci->team == TEAM_BLUE ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 192;
			verts[i].modulate[3] = 255;
		}
	}
	else {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 128;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}

	VectorAdd( cent->lerpOrigin, maxs, corners[3] );

	VectorCopy( corners[3], corners[2] );
	corners[2][0] -= extx;

	VectorCopy( corners[2], corners[1] );
	corners[1][1] -= exty;

	VectorCopy( corners[1], corners[0] );
	corners[0][0] += extx;

	for ( i = 0; i < 4; i++ ) {
		VectorCopy( corners[i], corners[i + 4] );
		corners[i + 4][2] -= extz;
	}

	// top
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader, 4, verts, qfalse );

	// bottom
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader, 4, verts, qfalse );

	// top side
	VectorCopy( corners[3], verts[0].xyz );
	VectorCopy( corners[2], verts[1].xyz );
	VectorCopy( corners[6], verts[2].xyz );
	VectorCopy( corners[7], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// left side
	VectorCopy( corners[2], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[6], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// right side
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[3], verts[1].xyz );
	VectorCopy( corners[7], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );

	// bottom side
	VectorCopy( corners[1], verts[0].xyz );
	VectorCopy( corners[0], verts[1].xyz );
	VectorCopy( corners[4], verts[2].xyz );
	VectorCopy( corners[5], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.bboxShader_nocull, 4, verts, qfalse );
}

int Wolfcam_PlayerHealth (int clientNum)
{
	//int value;
	const clientInfo_t *ci;

	if (clientNum >= MAX_CLIENTS) {
		return INVALID_WOLFCAM_HEALTH;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid) {
		return INVALID_WOLFCAM_HEALTH;
	}

	if (clientNum == cg.snap->ps.clientNum) {
		return cg.snap->ps.stats[STAT_HEALTH];
	}

	if (cgs.realProtocol >= 91  &&  cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		// we have info for all players
		return SIGNED_16_BIT(cg_entities[clientNum].currentState.health);
	}

	if (CG_IsTeamGame(cgs.gametype)) {
		if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team) {
			if (cgs.realProtocol >= 91) {
				return SIGNED_16_BIT(cg_entities[clientNum].currentState.health);
			} else {
				// demos don't have team info values for spec players
				if (cg.snap->ps.clientNum == cg.clientNum) {
					return ci->health;
				}
			}
		}
	}

	return INVALID_WOLFCAM_HEALTH;
#if 0
	//FIXME broken in ql

	if (wclients[clientNum].ev_pain_time == 0) {
		return INVALID_WOLFCAM_HEALTH;
	}

	value = wclients[clientNum].eventHealth;
	if (value >= 9999) {
		if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
			value = 0;
		} else {
			value = INVALID_WOLFCAM_HEALTH;
		}
	}

	return value;
#endif
}

int Wolfcam_PlayerArmor (int clientNum) {
	//int value;
	const clientInfo_t *ci;

	if (clientNum >= MAX_CLIENTS) {
		return -1;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid) {
		return -1;
	}

	if (clientNum == cg.snap->ps.clientNum) {
		return cg.snap->ps.stats[STAT_ARMOR];
	}

	if (cgs.realProtocol >= 91  &&  cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		// we have info for all players
		return SIGNED_16_BIT(cg_entities[clientNum].currentState.armor);
	}

	if (CG_IsTeamGame(cgs.gametype)) {
		if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team) {
			if (cgs.realProtocol >= 91) {
				return SIGNED_16_BIT(cg_entities[clientNum].currentState.armor);
			} else {
				// demos don't have team info values for spec players
				if (cg.snap->ps.clientNum == cg.clientNum) {

					//Com_Printf("armor == %d\n", ci->armor);
					return ci->armor;
				}
			}
		}
	}

	return -1;
}

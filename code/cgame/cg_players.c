// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

#include "cg_draw.h"
#include "cg_effects.h"
#include "cg_ents.h"
#include "cg_freeze.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_players.h"
#include "cg_predict.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_main.h"
#include "wolfcam_view.h"

#include "wolfcam_local.h"
#include "../game/bg_local.h"
#include "../ui/ui_shared.h"


char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};


static void CG_CheckForModelChange (const centity_t *cent, clientInfo_t *ci, refEntity_t *legs, refEntity_t *torso, refEntity_t *head);


/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;  // CG_CheckForModelChange
	//clientInfo_t ciTmp;
	refEntity_t legs, torso, head;
	int			i;

	//Com_Printf("custom sound:  %d  '%s'\n", clientNum, soundName);

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if (cgs.cpma) {
		if (clientNum < 0  ||  clientNum >= MAX_GENTITIES) {
			Com_Printf("^1cpma invalid clientNum %d\n", clientNum);
			clientNum = 0;
		}

		if (clientNum >= MAX_CLIENTS) {
			clientNum = cg_entities[clientNum].currentState.modelindex2 & 0xf;
			//Com_Printf("^5new player ^5%d '%s'\n", clientNum, cgs.clientinfo[clientNum].name);
		}
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		Com_Printf("^3CG_CustomSound() invalid clientNum %d  '%s'\n", clientNum, soundName);
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];
	//memcpy(&ciTmp, ci, sizeof(ciTmp));
	//Com_Printf("snd %d '%s'\n", clientNum, ci->name);

	//CG_CheckForModelChange(&cg_entities[clientNum], &ciTmp, &legs, &torso, &head);
	CG_CheckForModelChange(&cg_entities[clientNum], ci, &legs, &torso, &head);

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
			//return ciTmp.sounds[i];
		}
	}

	//CG_Error( "Unknown custom sound: %s", soundName );
	CG_Printf("^1unknown custom sound: '%s'\n", soundName);

	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	const char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				ci->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				ci->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				ci->footsteps = FOOTSTEP_ENERGY;
			} else {
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				ci->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			ci->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}

	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !*token ) {
			if( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE ) {
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstFrame = atoi( token );
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == LEGS_WALKCR ) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if ( i >= LEGS_WALKCR && i<TORSO_GETFLAG) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if ( i != MAX_ANIMATIONS ) {
		CG_Printf( "Error parsing animation file: %s", filename );
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
qboolean CG_FileExists (const char *filename)
{
	int len;

	len = trap_FS_FOpenFile( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

qhandle_t CG_RegisterSkinVertexLight (const char *name)
{
	int vlight;
	qhandle_t h;

	vlight = SC_Cvar_Get_Int("r_vertexlight");
	if (vlight) {
		trap_Cvar_Set("r_vertexlight", "0");
	}

	h = trap_R_RegisterSkin(name);
	//Com_Printf("^6register skin: '%s'\n", name);
	trap_Cvar_Set("r_vertexlight", va("%d", vlight));

	return h;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean	CG_FindClientModelFile( char *filename, int length, const clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext, qboolean dontForceTeamSkin ) {
	char *team, *charactersFolder;
	int i;

	if (CG_IsTeamGame(cgs.gametype) &&  cg_useDefaultTeamSkins.integer) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			//FIXME check for red ?
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}
	charactersFolder = "";
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/james/stroggs/lower_lily_red.skin"
#if 0  // testing
				Com_Printf("^5about to call first sprintf\n");
				Com_Printf("  charatersfolder '%s'\n", charactersFolder);
				Com_Printf("  modelName '%s'\n", modelName);
				Com_Printf("  teamName '%s'\n", teamName);
				Com_Printf("  base '%s'\n", base);
				Com_Printf("  skinName '%s'\n", skinName);
				Com_Printf("  team '%s'\n", team);
				Com_Printf("  ext '%s'\n", ext);
#endif
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"models/players/characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if (CG_IsTeamGame(cgs.gametype) &&  !dontForceTeamSkin  &&  cg_useDefaultTeamSkins.integer) {  //   &&   !*cg_enemyModel.string) {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext );
				}
				else {
					//								"models/players/characters/james/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"models/players/characters/james/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
qboolean CG_FindClientHeadFile (char *filename, int length, const clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext, qboolean dontForceTeamSkin)
{
	char *team, *headsFolder;
	int i;

	if (CG_IsTeamGame(cgs.gametype)  &&  cg_useDefaultTeamSkins.integer) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}

	if ( headModelName[0] == '*' ) {
		headsFolder = "heads/";
		headModelName++;
	}
	else {
		headsFolder = "";
	}
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext );
			}
			else {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if (CG_IsTeamGame(cgs.gametype)  &&  !dontForceTeamSkin  &&  cg_useDefaultTeamSkins.integer) {  //   &&   !*cg_enemyModel.string) {
				if ( i == 0 &&  teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, team, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( headsFolder[0] ) {
			break;
		}
		headsFolder = "heads/";
	}

	return qfalse;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean	CG_RegisterClientSkin (clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, qboolean dontForceTeamSkin)
{
	char filename[MAX_QPATH];
	char skinNameSet[MAX_QPATH];
	char headSkinNameSet[MAX_QPATH];

	//Com_Printf("^2  register client skin:  '%s'  '%s' team:%d\n", ci->name, skinName, ci->team);

	if (!Q_stricmp(skinName, TEAM_COLOR_SKIN)) {

		if (CG_IsTeamGame(cgs.gametype)) {
			//Com_Printf("^6yes... '%s'\n", ci->name);
			if (ci->team == TEAM_RED) {
				Q_strncpyz(skinNameSet, "red", sizeof(skinNameSet));
				//Com_Printf("   red\n");
			} else if (ci->team == TEAM_BLUE) {
				Q_strncpyz(skinNameSet, "blue", sizeof(skinNameSet));
				//Com_Printf("   blue..\n");
			} else {
				Q_strncpyz(skinNameSet, "default", sizeof(skinNameSet));
				//Com_Printf("   default for '%s'\n", ci->name);
				//Q_strncpyz(skinNameSet, "red", sizeof(skinNameSet));
			}
		} else {
			Q_strncpyz(skinNameSet, "default", sizeof(skinNameSet));
		}
	} else {
		Q_strncpyz(skinNameSet, skinName, sizeof(skinNameSet));
	}

	if (!Q_stricmp(headSkinName, TEAM_COLOR_SKIN)) {
		if (CG_IsTeamGame(cgs.gametype)) {
			if (ci->team == TEAM_RED) {
				Q_strncpyz(headSkinNameSet, "red", sizeof(headSkinNameSet));
			} else if (ci->team == TEAM_BLUE) {
				Q_strncpyz(headSkinNameSet, "blue", sizeof(headSkinNameSet));
			} else {
				Q_strncpyz(headSkinNameSet, "default", sizeof(headSkinNameSet));
			}
		} else {
			Q_strncpyz(headSkinNameSet, headSkinName, sizeof(headSkinNameSet));
		}
	} else {
		Q_strncpyz(headSkinNameSet, headSkinName, sizeof(headSkinNameSet));
	}

	if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinNameSet, "lower", "skin", dontForceTeamSkin)) {
		//Com_Printf("legs: '%s'\n", filename);
		ci->legsSkin = CG_RegisterSkinVertexLight(filename);
	}
	if (!ci->legsSkin) {
		if (cgs.cpma  &&  !Q_stricmp("pm", skinNameSet)) {
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "color", "lower", "skin", dontForceTeamSkin);
			ci->legsSkin = CG_RegisterSkinVertexLight(filename);
		}
		if (!ci->legsSkin) {
			//Com_Printf("Leg skin load failure: %s\n", filename);
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "default", "lower", "skin", dontForceTeamSkin);
			ci->legsSkin = CG_RegisterSkinVertexLight(filename);
		}

	}

	if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinNameSet, "head", "skin", dontForceTeamSkin)) {
		ci->headSkinFallback = CG_RegisterSkinVertexLight(filename);
	}

	if (CG_FindClientModelFile( filename, sizeof(filename), ci, teamName, modelName, skinNameSet, "upper", "skin", dontForceTeamSkin)) {
		//Com_Printf("torso: '%s'\n", filename);
		ci->torsoSkin = CG_RegisterSkinVertexLight(filename);
	}
	if (!ci->torsoSkin) {
		if (cgs.cpma  &&  !Q_stricmp("pm", skinNameSet)) {
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "color", "upper", "skin", dontForceTeamSkin);
			ci->torsoSkin = CG_RegisterSkinVertexLight(filename);
		}
		if (!ci->torsoSkin) {
			//Com_Printf("Torso skin load failure: %s\n", filename);
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "default", "upper", "skin", dontForceTeamSkin);
			ci->torsoSkin = CG_RegisterSkinVertexLight(filename);

		}
	}

	if (CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headModelName, headSkinNameSet, "head", "skin", dontForceTeamSkin)) {
		//Com_Printf("head: '%s'\n", filename);
		ci->headSkin = CG_RegisterSkinVertexLight(filename);
	}
	if (!ci->headSkin) {
		if (cgs.cpma  &&  !Q_stricmp("pm", skinNameSet)) {
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "color", "head", "skin", dontForceTeamSkin);
			ci->headSkin = CG_RegisterSkinVertexLight(filename);
		}
		if (!ci->headSkin) {
			//Com_Printf("Head skin load failure: %s\n", filename);
			CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, "default", "head", "skin", dontForceTeamSkin);
			ci->headSkin = CG_RegisterSkinVertexLight(filename);
		}
	}

	// if any skins failed to load
	if (!ci->legsSkin  ||  !ci->torsoSkin  ||  !ci->headSkin) {
		return qfalse;
	}
	return qtrue;
}

static void CG_GetPlayerHeight (clientInfo_t *ci)
{
	vec3_t mins, maxs;
	refEntity_t l, t, h, w;
	vec3_t angles = { 0, 0, 0 };
	//int renderfx = RF_DEPTHHACK;
	float height;

	memset(&l, 0, sizeof(l));
	memset(&t, 0, sizeof(t));
	memset(&h, 0, sizeof(h));
	memset(&w, 0, sizeof(w));

	l.hModel = ci->legsModel;
	t.hModel = ci->torsoModel;
	h.hModel = ci->headModel;

	//l.renderfx = renderfx;
	//t.renderfx = renderfx;
	//h.renderfx = renderfx;

	//CG_PlayerAngles(cent, l.axis, t.axis, h.axis);
	AnglesToAxis(angles, l.axis);
	AnglesToAxis(angles, t.axis);
	AnglesToAxis(angles, h.axis);

	l.oldframe = l.frame = ci->animations[LEGS_IDLE].firstFrame;
	t.oldframe = t.frame = ci->animations[TORSO_STAND].firstFrame;

	//UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
	//VectorCopy(cent->lerpOrigin, l.origin);
	VectorSet(l.origin, 0, 0, -bg_playerMins[2]);
	VectorCopy(l.origin, l.oldorigin);
	//CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");
	CG_PositionEntityOnTag(&t, &l, l.hModel, "tag_torso");
	CG_PositionEntityOnTag(&h, &t, t.hModel, "tag_head");
	CG_PositionEntityOnTag(&w, &t, t.hModel, "tag_weapon");


	trap_R_ModelBounds(h.hModel, mins, maxs);

#if 0
	if (clientNum != cg.snap->ps.clientNum) {
		vec3_t origin;

		VectorCopy(h.origin, origin);
		origin[2] += maxs[2];
		//Com_Printf("%d  z:  %f  %f\n", clientNum, head.origin[2] + maxs[2], origin[2]);
		//Com_Printf("frame %d  oldframe %d\n", legs.frame, legs.oldframe);
#if 0
		CG_FloatNumber(6, origin, RF_DEPTHHACK, NULL);
		CG_AddRefEntity(&l);
		CG_AddRefEntity(&t);
		CG_AddRefEntity(&h);
#endif
	}
#endif

	if (!Q_stricmpn(ci->modelName, "orbb", strlen("orbb") - 1)) {
		height = w.origin[2];
		//Com_Printf("orbb....\n");
		//height += maxs[2] - mins[2];
		trap_R_ModelBounds(cg_weapons[WP_MACHINEGUN].weaponModel, mins, maxs);
		height += maxs[2];  // - mins[2];
	} else {
		height = h.origin[2];
		height += maxs[2];  // - mins[2];
	}

#if 0
	if (l.origin[2] > height) {
		height = l.origin[2];
		Com_Printf("legs\n");
	}
#endif
#if 0
	if (w.origin[2] > height) {
		height = w.origin[2];
		Com_Printf("weapon\n");
	}
#endif

	//Com_Printf("height %f  [%f -> %f] %s\n", height, maxs[2], mins[2], ci->modelName);

	ci->playerModelHeight = height;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName, qboolean dontForceTeamSkin ) {
	char	filename[MAX_QPATH*2];
	const char		*headName;
	char newTeamName[MAX_QPATH*2];
	qboolean status;

	status = qtrue;
	ci->legsModel = ci->torsoModel = ci->headModel = 0;

	if ( headModelName[0] == '\0' ) {
		headName = modelName;
	} else {
		headName = headModelName;
	}
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
	ci->legsModel = trap_R_RegisterModel( filename );
	if ( !ci->legsModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
		if ( !ci->legsModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			//return qfalse;
			status = qfalse;
		}
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", modelName);
	ci->headModelFallback = trap_R_RegisterModel(filename);

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
	ci->torsoModel = trap_R_RegisterModel( filename );
	if ( !ci->torsoModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );
		if ( !ci->torsoModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			//return qfalse;
			status = qfalse;
		}
	}

	if( headName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1] );
	}
	else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", headName );
	}
	ci->headModel = trap_R_RegisterModel( filename );
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->headModel && headName[0] != '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", headModelName, headModelName );
		ci->headModel = trap_R_RegisterModel( filename );
	}
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		//return qfalse;
		status = qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, teamName, modelName, skinName, headName, headSkinName, dontForceTeamSkin ) ) {
		//Com_Printf("wtf\n");
		if ( teamName && *teamName) {
			Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelName, skinName, headName, headSkinName );
			if( ci->team == TEAM_BLUE ) {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			}
			else {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			}
			if ( !CG_RegisterClientSkin( ci, newTeamName, modelName, skinName, headName, headSkinName, dontForceTeamSkin ) ) {
				Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", newTeamName, modelName, skinName, headName, headSkinName );
				//return qfalse;
				status = qfalse;
			}
		} else {
			Com_Printf( "Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headName, headSkinName );
			//return qfalse;
			status = qfalse;
		}
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/animation.cfg", modelName );
		if ( !CG_ParseAnimationFile( filename, ci ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			//return qfalse;
			status = qfalse;
		}
	}

	if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "skin", dontForceTeamSkin ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	else if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "png", dontForceTeamSkin ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}

	if ( !ci->modelIcon ) {
		//Com_Printf("!ci->modelIcon  %s\n", filename);
		// who cares -- actually need it to disable 3Dicons
		//return qfalse;
		if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, "default", "icon", "skin", dontForceTeamSkin)) {
			ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
		} else if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, "default", "icon", "png", dontForceTeamSkin)) {
			ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
		}
	}

	//Com_Printf("^5CG_RegisterClientModelname() %s\n", modelName);

	CG_GetPlayerHeight(ci);

	return status;
}

static vec3_t g_color_table_ql_0_1_0_303[] = {
	{ 0.965, 0.000, 0.000 },

	{ 0.965, 0.000, 0.000 },  // 1
	{ 0.965, 0.380, 0.000 },
	{ 0.965, 0.588, 0.000 },
	{ 0.965, 0.769, 0.000 },

	{ 0.965, 0.965, 0.000 },  // 5
	{ 0.769, 0.965, 0.000 },
	{ 0.588, 0.965, 0.000 },
	{ 0.380, 0.965, 0.000 },

	{ 0.000, 0.965, 0.000 },  // 9
	{ 0.000, 0.965, 0.380 },
	{ 0.000, 0.965, 0.588 },
	{ 0.000, 0.965, 0.769 },

	{ 0.000, 0.965, 0.965 },  // 13
	{ 0.000, 0.769, 0.965 },
	{ 0.000, 0.588, 0.965 },
	{ 0.000, 0.380, 0.965 },

	{ 0.000, 0.000, 0.965 },  // 17
	{ 0.380, 0.000, 0.965 },
	{ 0.588, 0.000, 0.965 },
	{ 0.769, 0.000, 0.965 },

	{ 0.965, 0.000, 0.965 },  // 21
	{ 0.965, 0.000, 0.769 },
	{ 0.965, 0.000, 0.588 },
	{ 0.965, 0.000, 0.380 },

	{ 0.965, 0.965, 0.965 },  // 25
};

void CG_Q3ColorFromString (const char *v, vec3_t color)
{
	int n;

	VectorClear(color);

	// allow non quakelive demos and /devmap to use new rail colors
	if (!cgs.isQuakeLiveDemo  ||  CG_CheckQlVersion(0, 1, 0, 303)) {
		n = atoi(v);
		if (n < 1) {
			n = 1;
		}
		if (n > 25) {
			n = 25;
		}

		VectorCopy(g_color_table_ql_0_1_0_303[n], color);
	} else {
		color[0] = g_color_table_q3[ColorIndex(v[0])][0];
		color[1] = g_color_table_q3[ColorIndex(v[0])][1];
		color[2] = g_color_table_q3[ColorIndex(v[0])][2];
	}
}

void CG_CpmaColorFromPicString (const floatint_t *v, vec3_t color)
{
	int n;
	char c;

	VectorClear(color);
	c = v[0].i;

	if (c == 'y'  ||  c == 'Y'  ||  c == 'z'  ||  c == 'Z') {
		VectorSet(color, 1, 1, 1);
	} else if ((c >= 'a'  &&  c < 'y')  ||  (c >= 'A'  &&  c < 'Y')) {
		c = (char)tolower(v[0].i);
		n = c - 'a';
		VectorCopy(g_color_table_ql_0_1_0_303[n], color);
	} else {
#if 0
		color[0] = g_color_table_q3[ColorIndex(v[0].i)][0];
		color[1] = g_color_table_q3[ColorIndex(v[0].i)][1];
		color[2] = g_color_table_q3[ColorIndex(v[0].i)][2];
#endif
		color[0] = g_color_table[ColorIndex(v[0].i)][0];
		color[1] = g_color_table[ColorIndex(v[0].i)][1];
		color[2] = g_color_table[ColorIndex(v[0].i)][2];
	}
}

void CG_CpmaColorFromString (const char *v, vec3_t color)
{
	int n;
	char c;

	VectorClear(color);
	c = v[0];

	if (c == 'y'  ||  c == 'Y'  ||  c == 'z'  ||  c == 'Z') {
		VectorSet(color, 1, 1, 1);
	} else if ((c >= 'a'  &&  c < 'y')  ||  (c >= 'A'  &&  c < 'Y')) {
		c = (char)tolower((int)v[0]);
		n = c - 'a';
		VectorCopy(g_color_table_ql_0_1_0_303[n], color);
	} else {
#if 0
		color[0] = g_color_table_q3[ColorIndex(v[0])][0];
		color[1] = g_color_table_q3[ColorIndex(v[0])][1];
		color[2] = g_color_table_q3[ColorIndex(v[0])][2];
#endif
		color[0] = g_color_table[ColorIndex(v[0])][0];
		color[1] = g_color_table[ColorIndex(v[0])][1];
		color[2] = g_color_table[ColorIndex(v[0])][2];
	}
}

void CG_OspColorFromPicString (const floatint_t *v, vec3_t color)
{
	char hexString[16];
	int c;

	//VectorClear(color);

	c = v[0].i;

	if (v[0].i == 'x'  ||  v[0].i == 'X') {
		vmCvar_t tmpCvar;

		//Com_sprintf(hexString, sizeof(hexString), "0x%s", v + 1);
		hexString[0] = '0';
		hexString[1] = 'x';
		//memcpy(hexString + 2, v + 1, 6);
		hexString[2] = (char)v[1].i;
		hexString[3] = (char)v[2].i;
		hexString[4] = (char)v[3].i;
		hexString[5] = (char)v[4].i;
		hexString[6] = (char)v[5].i;
		hexString[7] = (char)v[6].i;
		hexString[8] = '\0';
		//tmpCvar.integer = Com_HexStrToInt(va("0x%s", v + 1));
		tmpCvar.integer = Com_HexStrToInt(hexString);
		//Com_Printf("osp color 0x%s\n", v + 1);
		SC_Vec3ColorFromCvar(color, &tmpCvar);
	} else if (v[0].i == 'b'  ||  v[0].i == 'B'  ||  v[0].i == 'f'  ||  v[0].i == 'F'  ||  v[0].i == 'N') {  // 'n' ?
		//FIXME  this is dumb...  caller has to make sure color is reset

		// do nothing

	} else if (v[0].i >= '0'  &&  v[0].i <= '9') {
		if (v[0].i == '8') {
			c = '3';
		}
		color[0] = g_color_table[ColorIndex(c)][0];
		color[1] = g_color_table[ColorIndex(c)][1];
		color[2] = g_color_table[ColorIndex(c)][2];
	}
}

void CG_OspColorFromString (const char *v, vec3_t color)
{
	char hexString[16];
	char c;

	//VectorClear(color);
	c = v[0];

	if (v[0] == 'x'  ||  v[0] == 'X') {
		vmCvar_t tmpCvar;

		//Com_sprintf(hexString, sizeof(hexString), "0x%s", v + 1);
		hexString[0] = '0';
		hexString[1] = 'x';
		memcpy(hexString + 2, v + 1, 6);
		hexString[8] = '\0';
		//tmpCvar.integer = Com_HexStrToInt(va("0x%s", v + 1));
		tmpCvar.integer = Com_HexStrToInt(hexString);
		//Com_Printf("osp color 0x%s\n", v + 1);
		SC_Vec3ColorFromCvar(color, &tmpCvar);
	} else if (v[0] == 'b'  ||  v[0] == 'B'  ||  v[0] == 'f'  ||  v[0] == 'F'  ||  v[0] == 'N') {  // 'n' ?
		//FIXME  this is dumb...  caller has to make sure color is reset

		// do nothing

	} else if (c >= '0'  &&  c <= '9') {
		if (c == '8') {
			c = '3';
		}
		color[0] = g_color_table[ColorIndex(c)][0];
		color[1] = g_color_table[ColorIndex(c)][1];
		color[2] = g_color_table[ColorIndex(c)][2];
	}
}

#if 0
/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	//int val;

	VectorClear( color );

	color[0] = g_color_table[ColorIndex(v[0])][0];
	color[1] = g_color_table[ColorIndex(v[0])][1];
	color[2] = g_color_table[ColorIndex(v[0])][2];

	return;

#if 0
	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
#endif
}
#endif

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
void CG_LoadClientInfo (clientInfo_t *ci, int clientNum, qboolean dontForceTeamSkin)
{
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	//int			clientNum;
	char		teamname[MAX_QPATH];

	teamname[0] = 0;
#ifdef MISSIONPACK
	if( CG_IsTeamGame(cgs.gametype) ) {
		if( ci->team == TEAM_BLUE ) {
			Q_strncpyz(teamname, cg_blueTeamName.string, sizeof(teamname) );
		} else {
			Q_strncpyz(teamname, cg_redTeamName.string, sizeof(teamname) );
		}
	}
	if( teamname[0] ) {
		strcat( teamname, "/" );
	}
#endif
	modelloaded = qtrue;
	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname, dontForceTeamSkin ) ) {
		CG_Printf("couldn't load '%s' '%s' '%s' '%s' '%s' for %s\n", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname, ci->name);
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
		}

		// fall back to default team name
		if( CG_IsTeamGame(cgs.gametype) ) {
			// keep skin name
			if( ci->team == TEAM_BLUE ) {
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
			} else {
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
			}
			if ( !CG_RegisterClientModelname( ci, DEFAULT_TEAM_MODEL, ci->skinName, DEFAULT_TEAM_HEAD, ci->skinName, teamname, dontForceTeamSkin ) ) {
				//CG_Error( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinName );
				CG_Printf( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register\n", DEFAULT_TEAM_MODEL, ci->skinName );
			}
		} else {
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname, dontForceTeamSkin ) ) {
				//CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
				CG_Printf( "DEFAULT_MODEL (%s) failed to register\n", DEFAULT_MODEL );
			}
		}
		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap_R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = CG_IsTeamGame(cgs.gametype) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", dir, s + 1), qfalse );
		}
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", fallback, s + 1), qfalse );
		}
	}

	ci->deferred = qfalse;

	if (clientNum >= 0  &&  clientNum < MAX_CLIENTS) {
		// reset any existing players and bodies, because they might be in bad
		// frames for this new model
		//clientNum = ci - cgs.clientinfo;
		for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
			if ( cg_entities[i].currentState.clientNum == clientNum
				 && cg_entities[i].currentState.eType == ET_PLAYER ) {
				CG_ResetPlayerEntity( &cg_entities[i] );
			}
		}
	}

	//Com_Printf("^5loading client info %d  %s\n", clientNum, ci->name);
}

/*
======================
CG_CopyClientInfoModel
======================
*/
void CG_CopyClientInfoModel (const clientInfo_t *from, clientInfo_t *to)
{
	if (from->legsModel  &&  from->torsoModel) {
		VectorCopy(from->headOffset, to->headOffset);
		to->footsteps = from->footsteps;
		to->gender = from->gender;

		to->legsModel = from->legsModel;
		to->legsSkin = from->legsSkin;
		to->torsoModel = from->torsoModel;
		to->torsoSkin = from->torsoSkin;

		to->modelIcon = from->modelIcon;
		//to->setModelIcon = from->setModelIcon;
		//to->setHeadSkin = from->setHeadSkin;

		to->newAnims = from->newAnims;
		to->playerModelHeight = from->playerModelHeight;

		memcpy( to->animations, from->animations, sizeof( to->animations ) );
		memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
	}

	if (from->headModel) {
		to->headModel = from->headModel;
		to->headSkin = from->headSkin;
	}

	if (cgs.cpma) {
		memcpy(to->headColor, from->headColor, sizeof(to->headColor));
		memcpy(to->torsoColor, from->torsoColor, sizeof(to->torsoColor));
		memcpy(to->legsColor, from->legsColor, sizeof(to->legsColor));
	}

}

#if 0  // wolfcam
/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	const clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->headModelName, match->headModelName )
			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (!CG_IsTeamGame(cgs.gametype) || ci->team == match->team) ) {
			// this clientinfo is identical, so use its handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo (clientInfo_t *ci, int clientNum)
{
	int		i;
	const clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			 (CG_IsTeamGame(cgs.gametype) && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo(ci, clientNum);
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( CG_IsTeamGame(cgs.gametype) ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ||
				(CG_IsTeamGame(cgs.gametype) && ci->team != match->team) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo(ci, clientNum);
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo(ci, clientNum);
}

#endif

static score_t tmpScores[MAX_CLIENTS];

static void CG_RemoveClientFromScores (int clientNum)
{
	int tmpNumScores;
	int i;

	memcpy(tmpScores, cg.scores, sizeof(tmpScores));
	tmpNumScores = cg.numScores;

	cg.numScores = 0;
	for (i = 0;  i < tmpNumScores;  i++) {
		if (tmpScores[i].client == clientNum) {
			continue;
		}
		memcpy(&cg.scores[cg.numScores], &tmpScores[i], sizeof(score_t));
		cg.numScores++;
	}
}

static void	CG_AddNewPlayerToScores (int clientNum, const clientInfo_t *info)
{
	score_t *sc;

	sc = &cg.scores[cg.numScores];
	cg.numScores++;

	memset(sc, 0, sizeof(score_t));
	sc->client = clientNum;
	sc->team = info->team;
	sc->bestWeapon = 2;
	sc->alive = qfalse;
}

static void CG_UpdatePlayerScores (int clientNum, const clientInfo_t *info)
{
	const clientInfo_t *oldinfo;
	int i;

	if (cgs.gametype == GT_RED_ROVER) {
		return;  // fuck me :(
	}

	oldinfo = &cgs.clientinfo[clientNum];
	//Com_Printf("ci %s\n", oldinfo->name);

	// pretty much just check team change
	if (oldinfo->team == info->team) {
		return;
	}

	for (i = 0;  i < cg.numScores;  i++) {
		if (cg.scores[i].client == clientNum) {
			cg.scores[i].team = info->team;
			cg.scores[i].alive = qfalse;
			cg.scores[i].score = 0;
			return;
		}
	}
}

//FIXME size
static void CG_SafeColorName (char *out, const char *in)
{
	int i;
	int j;
	char lastColor = '7';

	if (!out ||  !in) {
		return;
	}
	if (!*in) {
		return;
	}

	i = 0;
	j = 0;
	out[j] = '^';
	j++;
	out[j] = '7';
	j++;

	while (1) {
		if (in[i] == '\0') {
			out[j] = '\0';
			break;
		}
		if (in[i] == '^') {
			if (in[i + 1] == '\0') {
				out[j] = '\0';
				break;
			}
			out[j] = '^';
			i++;
			j++;
			lastColor = in[i];
		} else if (in[i] == ' ') {
			i++;
			out[j] = ' ';
			j++;
			out[j] = '^';
			j++;
			out[j] = lastColor;  //'7';
			j++;
		}
		out[j] = in[i];
		i++;
		j++;
	}

#if 0
	Com_Printf("new color safe name: ");
	i = 0;
	while (1) {
		if (out[i] == '\0') {
			Com_Printf("\n");
			return;
		}
		Com_Printf("%c", out[i]);
		i++;
	}
#endif
}

//FIXME size
static void CG_WhiteName (char *out, const char *in)
{
	int i;
	int j;
	int len;

	if (!out ||  !in) {
		return;
	}
	if (!*in) {
		return;
	}

	out[0] = '\0';

	len = strlen(in);
	i = 0;
	j = 0;
	while (1) {
		if (i >= len  ||  in[i] == '\0') {
			out[j] = '\0';
			break;
		}
		if (in[i] == '^') {
			if (in[i + 1] == '\0') {
				out[j] = '\0';
				break;
			}

			if (cgs.osp) {
				if (in[i + 1] == 'x'  ||  in[i + 1] == 'X') {
					i += 8;
				}
			}
			i += 2;
			continue;
		}
		out[j] = in[i];
		i++;
		j++;
	}

#if 0
	Com_Printf("new white name: ");
	i = 0;
	while (1) {
		if (out[i] == '\0') {
			Com_Printf("\n");
			return;
		}
		Com_Printf("%c", out[i]);
		i++;
	}
#endif
}

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t *ciOrig;
	clientInfo_t newInfo;
	const char	*configstring;
	char	*v;
	const char *s;
	char *slash;
	const char *name;
	char *skin;
	char filename[MAX_QPATH];
	char tmpString[MAX_QPATH];
	char iconSkin[MAX_QPATH];
	qhandle_t h;
	int oldTeam;
	int newTeam;

	ci = &cgs.clientinfo[clientNum];
	ciOrig = &cgs.clientinfoOrig[clientNum];

	// hack for keeping track of players that are alive in round based games
	oldTeam = TEAM_SPECTATOR;
	if (ci->infoValid) {
		oldTeam = ci->team;
	}
	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	v = Info_ValueForKey( configstring, "t" );
	newTeam = atoi(v);
	if (newTeam == TEAM_RED  ||  newTeam == TEAM_BLUE) {
		if (newTeam != oldTeam) {
			wclients[clientNum].aliveThisRound = qfalse;
		}
	}

	//Com_Printf("override %d  -> %d\n", clientNum, ci->override);

	if (ci->override) {
		return;
	}

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);

	if ( !configstring[0] ) {
		// client has disconnected

        if (wnumOldClients >= MAX_CLIENTS) {
            CG_Printf ("CG_NewClientInfo: wnumOldClients >= MAX_CLIENTS\n");
        } else {
            woldclient_t *wold = &woldclients[wnumOldClients];
            wclient_t *wc = &wclients[clientNum];

            wold->valid = qtrue;
            wold->clientNum = clientNum;
            memcpy (&(wold->clientinfo), ci, sizeof (*ci));
            //            wold->kills = wclients[clientNum].kills;
            wold->kills = wc->kills;
            wold->deaths = wclients[clientNum].deaths;
            wold->suicides = wclients[clientNum].suicides;
            wold->warp = wc->warp;
            wold->nowarp = wclients[clientNum].nowarp;
            wold->warpaccum = wc->warpaccum;
			wold->validCount = wc->validCount;
			wold->invalidCount = wc->invalidCount;
			wold->serverPingAccum = wc->serverPingAccum;
			wold->serverPingSamples = wc->serverPingSamples;
			wold->snapshotPingAccum = wc->snapshotPingAccum;
			wold->snapshotPingSamples = wc->snapshotPingSamples;
			wold->noCommandCount = wc->noCommandCount;
			wold->noMoveCount = wc->noMoveCount;
			cgs.lastConnectedDisconnectedPlayerClientInfo = &wold->clientinfo;
            memcpy (&(wold->wstats), &(wclients[clientNum].wstats), sizeof(wweaponStats_t) * WP_NUM_WEAPONS);
            wnumOldClients++;
        }

		Q_strncpyz(cgs.lastConnectedDisconnectedPlayerName, ci->name, sizeof(cgs.lastConnectedDisconnectedPlayerName));

        memset (&wclients[clientNum], 0, sizeof(wclient_t));
		memset( ci, 0, sizeof( *ci ) );
		memset(ciOrig, 0, sizeof(*ciOrig));

		//cgs.newConnectedClient[clientNum] = qfalse;
		//CG_BuildSpectatorString();  // don't need always called in cg_servercmds.c
		CG_RemoveClientFromScores(clientNum);
		cgs.lastConnectedDisconnectedPlayer = clientNum;
		cgs.needToCheckSkillRating = qtrue;

		//Com_Printf("^5player disconnected sv_skillrating %s\n", Info_ValueForKey(CG_ConfigString( CS_SERVERINFO ), "sv_skillrating"));
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	//Q_strncpyz( newInfo.name, va("^7%s", v), sizeof( newInfo.name ) );
	//Com_sprintf(newInfo.name, sizeof(newInfo.name), "^7%s", v);
	//Com_sprintf(newInfo.name, sizeof(newInfo.name), "^7%s", v);
	CG_SafeColorName(newInfo.name, v);
	CG_WhiteName(newInfo.whiteName, v);
	//Com_Printf("new name: %s %c %c\n", newInfo.name, newInfo.name[0], newInfo.name[1]);

	v = Info_ValueForKey(configstring, "cn");
	if (*v) {
		CG_SafeColorName(newInfo.clanTag, v);
		CG_WhiteName(newInfo.whiteClanTag, v);
		//Com_Printf("^1:  %d  ^7'%s^7'  ^7'%s^7'\n", clientNum, newInfo.name, newInfo.clanTag);
	} else {
		newInfo.clanTag[0] = '\0';
		newInfo.whiteClanTag[0] = '\0';
	}

	v = Info_ValueForKey(configstring, "xcn");
	if (*v) {
		CG_SafeColorName(newInfo.fullClanName, v);
	} else {
		newInfo.fullClanName[0] = '\0';
	}

	if (!cgs.clientinfo[clientNum].infoValid  ||  strcmp(newInfo.name, cgs.clientinfo[clientNum].name)  ||  strcmp(newInfo.clanTag, cgs.clientinfo[clientNum].clanTag)) {
		const fontInfo_t *font;

		if (*cg_drawPlayerNamesFont.string) {
			font = &cgs.media.playerNamesFont;
		} else {
			font = &cgDC.Assets.textFont;
		}
		//Com_Printf("new name for '%s'  %s\n", cgs.clientinfo[clientNum].name, newInfo.name);
		if (*newInfo.clanTag) {
			name = va("%s ^7%s", newInfo.clanTag, newInfo.name);
		} else {
			name = newInfo.name;
		}
		//Com_Printf("name: '%s'\n", name);
		//CG_CreateNameSprite(0, 0, 1.0, colorWhite, name, 0, 0, 0, font, cgs.clientNameImage[clientNum], 48 * MAX_QPATH, 48);
		//CG_CreateNameSprite(0, 0, 1.0, colorWhite, name, 0, 0, 0, font, cgs.clientNameImage[clientNum], NAME_SPRITE_GLYPH_DIMENSION * MAX_QPATH, NAME_SPRITE_GLYPH_DIMENSION + (NAME_SPRITE_SHADOW_OFFSET * 2));
		CG_CreateNameSprite(0, 0, 1.0, colorWhite, name, 0, 0, 0, font, cgs.clientNameImage[clientNum]);
	}

	// colors
	if (cgs.cpma) {
		vec3_t color;

		VectorSet(newInfo.color1, 1, 1, 1);
		VectorSet(newInfo.color2, 1, 1, 1);
		Vector4Set(newInfo.headColor, 255, 255, 255, 255);
		Vector4Set(newInfo.torsoColor, 255, 255, 255, 255);
		Vector4Set(newInfo.legsColor, 255, 255, 255, 255);

		v = Info_ValueForKey(configstring, "c1");
		if (*v) {
			CG_CpmaColorFromString(v, newInfo.color1);
			v++;
			if (*v) {
				CG_CpmaColorFromString(v, color);
				newInfo.headColor[0] = 255 * color[0];
				newInfo.headColor[1] = 255 * color[1];
				newInfo.headColor[2] = 255 * color[2];
				newInfo.headColor[3] = 255;
				v++;
				if (*v) {
					CG_CpmaColorFromString(v, color);
					newInfo.torsoColor[0] = 255 * color[0];
					newInfo.torsoColor[1] = 255 * color[1];
					newInfo.torsoColor[2] = 255 * color[2];
					newInfo.torsoColor[3] = 255;
					v++;
					if (*v) {
						CG_CpmaColorFromString(v, color);
						newInfo.legsColor[0] = 255 * color[0];
						newInfo.legsColor[1] = 255 * color[1];
						newInfo.legsColor[2] = 255 * color[2];
						newInfo.legsColor[3] = 255;
						v++;
						if (*v) {
							CG_CpmaColorFromString(v, newInfo.color2);
						}
					}
				}
			}
		}
	} else {
		v = Info_ValueForKey( configstring, "c1" );
		CG_Q3ColorFromString( v, newInfo.color1 );

		v = Info_ValueForKey( configstring, "c2" );
		CG_Q3ColorFromString( v, newInfo.color2 );
	}

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

	// only spectating or waiting to play
	v = Info_ValueForKey( configstring, "so" );
	newInfo.spectatorOnly = atoi(v);

	// player que position
	v = Info_ValueForKey( configstring, "pq" );
	newInfo.quePosition = atoi(v);

	v = Info_ValueForKey(configstring, "su");
	newInfo.premiumSubscriber = atoi(v);

	v = Info_ValueForKey( configstring, "g_redteam" );
	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey( configstring, "g_blueteam" );
	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey(configstring, "st");
	if (*v  &&  cgs.realProtocol >= 91) {
		Q_strncpyz(newInfo.steamId, v, MAX_STRING_CHARS);
	} else {
		newInfo.steamId[0] = '\0';
	}

	v = Info_ValueForKey(configstring, "c");
	if (*v  &&  cgs.protocol == PROTOCOL_QL) {
		if (!Q_stricmp("n/a", v)) {
			newInfo.countryFlag = trap_R_RegisterShaderNoMip("ui/assets/flags/none.png");
		} else if (*v) {
			newInfo.countryFlag = trap_R_RegisterShaderNoMip(va("ui/assets/flags/%s.png", Info_ValueForKey(configstring, "c")));
		} else {
			newInfo.countryFlag = 0;
		}
	} else {
		newInfo.countryFlag = 0;
	}

	//FIXME are v and h needed at this point?
	if (cg_ignoreClientHeadModel.integer == 2  &&  cgs.protocol == PROTOCOL_QL) {
		v = Info_ValueForKey(configstring, "model");
	} else if (cg_ignoreClientHeadModel.integer) {
		v = Info_ValueForKey(configstring, "model");
	} else {
		v = Info_ValueForKey(configstring, "hmodel");
	}

	s = Info_ValueForKey(configstring, "model");
	Q_strncpyz(newInfo.modelString, s, sizeof(newInfo.modelString));
	Q_strncpyz(newInfo.headString, v, sizeof(newInfo.headString));

	if (!cgs.clientinfo[clientNum].infoValid  ||  strcmp(s, cgs.clientinfo[clientNum].modelString)  ||  strcmp(v, cgs.clientinfo[clientNum].headString)) {
		// unedited model

		//v = Info_ValueForKey(configstring, "hmodel");
		if (cg_ignoreClientHeadModel.integer == 2  &&  cgs.protocol == PROTOCOL_QL) {
			v = Info_ValueForKey(configstring, "model");
		} else if (cg_ignoreClientHeadModel.integer) {
			v = Info_ValueForKey(configstring, "model");
		} else {
			v = Info_ValueForKey(configstring, "hmodel");
		}

		if (!*v) {
			v = Info_ValueForKey(configstring, "model");
		}
		if (!*v) {
			Q_strncpyz(tmpString, DEFAULT_MODEL"/default", sizeof(v));
			v = tmpString;
		}

		skin = strrchr(v, '/');
		if (skin) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		if (!Q_stricmp(skin, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (newInfo.team == TEAM_RED) {
					Q_strncpyz(iconSkin, "red", sizeof(iconSkin));
				} else if (newInfo.team == TEAM_BLUE) {
					Q_strncpyz(iconSkin, "blue", sizeof(iconSkin));
				} else {
					Q_strncpyz(iconSkin, "default", sizeof(iconSkin));
				}
			} else {
				Q_strncpyz(iconSkin, "default", sizeof(iconSkin));
			}
		} else {
			Q_strncpyz(iconSkin, skin, sizeof(iconSkin));
		}

		newInfo.setModelIcon = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", v, iconSkin));
		if (!newInfo.setModelIcon) {
			newInfo.setModelIcon = trap_R_RegisterShaderNoMip(va("models/players/characters/%s/icon_%s", v, iconSkin));
		}
		if (!newInfo.setModelIcon) {
			if (1) {  //(cgs.cpma  &&  !Q_stricmp("pm", skin)) {
				newInfo.setModelIcon = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", v, "default"));
			}
		}
		if (!newInfo.setModelIcon) {
			newInfo.setModelIcon = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", DEFAULT_MODEL, "default"));
		}

		//FIXME duplicate code
		h = 0;
		if (CG_FindClientModelFile(filename, sizeof(filename), &newInfo, NULL, v, skin, "head", "skin", qtrue)) {
			h = CG_RegisterSkinVertexLight(filename);
		}
		if (!h) {
			if (cgs.cpma  &&  !Q_stricmp("pm", skin)) {
				CG_FindClientModelFile(filename, sizeof(filename), &newInfo, NULL, v, "color", "head", "skin", qtrue);
				h = CG_RegisterSkinVertexLight(filename);
			}
			if (!h) {
				//Com_Printf("Head skin load failure: %s\n", filename);
				CG_FindClientModelFile(filename, sizeof(filename), &newInfo, NULL, v, "default", "head", "skin", qtrue);
				h = CG_RegisterSkinVertexLight(filename);
			}
		}

		//Com_Printf("^4'%s'  '%s'\n", v, skin);

		newInfo.setHeadSkin = h;
	} else {
		newInfo.setModelIcon = cgs.clientinfo[clientNum].setModelIcon;
		newInfo.setHeadSkin = cgs.clientinfo[clientNum].setHeadSkin;
	}

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer  ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		//char *skin;

		if(0) {  //( CG_IsTeamGame(cgs.gametype) ) {
			Q_strncpyz( newInfo.modelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.modelName ) );
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );
		}

		if (0) {  //( CG_IsTeamGame(cgs.gametype) ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
			//Com_Printf("^3new skin: '%s'\n", newInfo.skinName);
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
			//Com_Printf("skinname: '%s'\n", newInfo.skinName);
		}
	}

	// head model
	//v = Info_ValueForKey( configstring, "hmodel" );
	if (cg_ignoreClientHeadModel.integer == 2  &&  cgs.protocol == PROTOCOL_QL) {
		v = Info_ValueForKey(configstring, "model");
	} else if (cg_ignoreClientHeadModel.integer) {
		v = Info_ValueForKey(configstring, "model");
	} else {
		v = Info_ValueForKey(configstring, "hmodel");
	}

	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if(0) {  //( CG_IsTeamGame(cgs.gametype) ) {
			Q_strncpyz( newInfo.headModelName, DEFAULT_TEAM_HEAD, sizeof( newInfo.headModelName ) );
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "headmodel", modelStr, sizeof( modelStr ) );
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
			Q_strncpyz( newInfo.headModelName, modelStr, sizeof( newInfo.headModelName ) );
		}

		if (0) {  //( CG_IsTeamGame(cgs.gametype) ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );

		slash = strchr( newInfo.headModelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

#if 0   // screw this
	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean	forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo(&newInfo, clientNum);
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo(&newInfo, clientNum);
		}
	}
#endif
	newInfo.headTeamSkinAlt = 0;
	newInfo.torsoTeamSkinAlt = 0;
	newInfo.legsTeamSkinAlt = 0;
	newInfo.headEnemySkinAlt = 0;
	newInfo.torsoEnemySkinAlt = 0;
	newInfo.legsEnemySkinAlt = 0;

	//Com_Printf("^3skin: '%s'\n", newInfo.skinName);
	CG_LoadClientInfo(&newInfo, clientNum, cg_forceModel.integer == 2 ? qtrue : qfalse);

	if (!ci->infoValid) {
		// this is a new client connecting
		//cgs.newConnectedClient[clientNum] = qtrue;
		//CG_BuildSpectatorString();  // will do it already in cg_servercmds.c
		cgs.lastConnectedDisconnectedPlayer = clientNum;
		cgs.needToCheckSkillRating = qtrue;
		Q_strncpyz(cgs.lastConnectedDisconnectedPlayerName, newInfo.name, sizeof(cgs.lastConnectedDisconnectedPlayerName));
		// hack in case player has same skill rating as sv_skillrating, in
		// which case sv_skillrating isn't modified
		//FIXME
		//newInfo.knowSkillRating = qtrue;
		//newInfo.skillRating = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO ), "sv_skillrating"));
		//Com_Printf("^5player connected sv_skillrating %s  %s\n", Info_ValueForKey(CG_ConfigString( CS_SERVERINFO ), "sv_skillrating"), newInfo.name);
		//Com_Printf("new client %d  %s\n", clientNum, newInfo.name);
		cgs.lastConnectedDisconnectedPlayerClientInfo = ci;
		CG_AddNewPlayerToScores(clientNum, &newInfo);
	} else {
		// an already valid client who has changed info
		newInfo.knowSkillRating = ci->knowSkillRating;
		newInfo.skillRating = ci->skillRating;
		//newInfo.score = 0;  //ci->score;  // no  .. why not?
		newInfo.score = ci->score;
		//FIXME anything else?
		newInfo.scoreIndexNum = ci->scoreIndexNum;
		newInfo.scoreValid = ci->scoreValid;
		newInfo.location = ci->location;
		newInfo.health = ci->health;
		newInfo.armor = ci->armor;
		newInfo.curWeapon = ci->curWeapon;
		newInfo.powerups = ci->powerups;
		newInfo.medkitUsageTime = ci->medkitUsageTime;
		newInfo.invulnerabilityStartTime = ci->invulnerabilityStartTime;
		newInfo.invulnerabilityStopTime = ci->invulnerabilityStopTime;
		newInfo.breathPuffTime = ci->breathPuffTime;

		CG_UpdatePlayerScores(clientNum, &newInfo);
	}

	//Com_Printf("new client info %s\n", newInfo.name);
	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
	*ciOrig = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo(ci, i, qfalse);
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation (const clientInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	const animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = (animation_t *)anim;  //FIXME const
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}

static void CG_SetAnimFrame (lerpFrame_t *lf, int time, float speedScale)
{
	const animation_t *anim;
	int f;
	int numFrames;

	anim = lf->animation;
	if (!anim->frameLerp) {
		return;
	}

	if ( time < lf->animationTime ) {
		//CG_Printf("initial lerp for %d  time %d < lf->animationTime %d\n", ci - cgs.clientinfo, time, lf->animationTime);
		lf->frameTime = lf->animationTime;		// initial lerp
	} else {
		lf->frameTime = lf->oldFrameTime + anim->frameLerp;
	}
	f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
	f *= speedScale;		// adjust for haste, etc

	numFrames = anim->numFrames;
	if (anim->flipflop) {
		numFrames *= 2;
	}
	if ( f >= numFrames ) {
		f -= numFrames;
		if ( anim->loopFrames ) {
			f %= anim->loopFrames;
			f += anim->numFrames - anim->loopFrames;
		} else {
			f = numFrames - 1;
			// the animation is stuck at the end, so it
			// can immediately transition to another sequence
			lf->frameTime = time;
		}
	}
	if ( anim->reversed ) {
		lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
	}
	else if (anim->flipflop && f>=anim->numFrames) {
		lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
	}
	else {
		lf->frame = anim->firstFrame + f;
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame( const clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale, int time ) {
	//int			f;
	const animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		//Com_Printf("new anim for %d\n", ci - cgs.clientinfo);
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}

		CG_SetAnimFrame(lf, time, speedScale);

#if 0
		if (cg.demoSeeking) {
			Com_Printf("%d  ent %d  frame %d  (lf->frameTime %d  lf->animationTime %d\n", cg.time, ci - cgs.clientinfo, lf->frame, lf->frameTime, lf->animationTime);
		}
#endif

		if ( time > lf->frameTime ) {
			lf->frameTime = time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n");
			}
		}
	} else {
		//Com_Printf("lerp rewind\n");
	}

	if ( lf->frameTime > time + 200 ) {
		lf->frameTime = time;
	}

	if ( lf->oldFrameTime > time ) {
		lf->oldFrameTime = time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
void CG_ClearLerpFrame (const clientInfo_t *ci, lerpFrame_t *lf, int animationNumber , int time)
{
	//Com_Printf("clear lerp frame %d  %d\n", cg.time, time);

	lf->frameTime = lf->oldFrameTime = time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	const clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;
	qboolean entFreeze;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	//Com_Printf("%d %s  0x%x\n", clientNum, cgs.clientinfo[clientNum].name, cent->currentState.powerups);

	if ( cent->currentState.powerups & ( 1 << PW_HASTE ) ) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	if (CG_FreezeTagFrozen(clientNum)) {
		speedScale = 0;
	}

	ci = &cgs.clientinfo[ clientNum ];

	entFreeze = qfalse;
	if (cent == &cg.predictedPlayerEntity) {
		if (cg.freezeEntity[cg.snap->ps.clientNum]) {
			entFreeze = qtrue;
		}
	} else {
		if (cg.freezeEntity[cent - cg_entities]) {
			entFreeze = qtrue;
		}
	}

	// do the shuffle turn frames locally
	if (!entFreeze) {
		if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
			CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale, cent->cgtime );
		} else {
			CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale, cent->cgtime );
		}
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	if (!entFreeze) {
		CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale, cent->cgtime );
	}

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( const centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles separate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir, clientNum;
	const clientInfo_t	*ci;


	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE 
		 || (( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND   &&  (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	//goto done;
	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;

	//goto done;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedtorso ) {
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	if (CG_FreezeTagFrozen(cent->currentState.clientNum)) {
		//VectorSet(velocity, 0, 0, 0);
		//Com_Printf("%s frozen\n", cgs.clientinfo[cent->currentState.clientNum].name);
	}

	speed = VectorNormalize( velocity );
	speed *= cg_playerLeanScale.value;
	if (speed < 0) {
		speed = 0;
	}

	if ( speed > 0 ) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedlegs ) {
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// done:
	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	int				anim;

	if (!cg_hasteTrail.integer) {
		return;
	}

	if ( cent->trailTime > cg.time ) {
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if ( anim != LEGS_RUN && anim != LEGS_BACK ) {
		return;
	}

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;

	smoke = CG_SmokePuff( origin, vec3_origin,
						  cg_smokeRadius_haste.value,  // 8
				  1, 1, 1, 1,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.hastePuffShader );

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

static void CG_FlightTrail (centity_t *cent)
{
	localEntity_t	*smoke;
	vec3_t			origin;

	if (!cg_flightTrail.integer) {
		return;
	}

	if ( cent->trailTime > cg.time ) {
		return;
	}

	if (cent->currentState.groundEntityNum != ENTITYNUM_NONE) {
		//return;
	}

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;

	smoke = CG_SmokePuff( origin, vec3_origin,
						  cg_smokeRadius_flight.value,  // 8
				  1, 1, 1, 1,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.hastePuffShader );

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

#if 1  //def MPACK
/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs( const centity_t *cent, const refEntity_t *head) {
	clientInfo_t *ci;
	vec3_t up, origin;
	int contents;
	int clientNum;

	if (!cg_enableBreath.integer) {
		return;
	}

	clientNum = cent->currentState.number;

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return;
	}
	ci = &cgs.clientinfo[clientNum];

	if (cg_enableBreath.integer == 1  &&  !cg_serverEnableBreath.integer) {
		return;
	}
	if (cg_enableBreath.integer == 2  &&  !cg.mapEnableBreath) {
		return;
	}
	//if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}
	if (CG_IsFirstPersonView(clientNum)) {
		return;
	}
	contents = CG_PointContents( head->origin, 0 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

#if 0
	if (*EffectScripts.playerBreath) {
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		CG_CopyStaticCentDataToScript(cent);
		CG_RunQ3mmeScript(EffectScripts.playerFlight, NULL);
		CG_CopyStaticScriptDataToCent(cent);
		return;
	}
#endif

	if ( ci->breathPuffTime > cg.time ) {
		return;
	}

	VectorSet( up, 0, 0, 8 );
	VectorMA(head->origin, 8, head->axis[0], origin);
	VectorMA(origin, -4, head->axis[2], origin);
	CG_SmokePuff( origin, up, cg_smokeRadius_breath.value /* 16 */, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
	ci->breathPuffTime = cg.time + 2000;
}

/*
===============
CG_DustTrail
===============
*/
static void CG_DustTrail( centity_t *cent ) {
	int				anim;
	//localEntity_t	*dust;
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableDust.integer) {
		return;
	}

	if (cg_enableDust.integer == 1  &&  !cg_serverEnableDust.integer) {
		return;
	}

	if (cg_enableDust.integer == 2  &&  !cg.mapEnableDust) {
		return;
	}

	if ( cent->dustTrailTime > cg.time ) {
		return;
	}

	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if ( anim != LEGS_LANDB && anim != LEGS_LAND ) {
		return;
	}

	cent->dustTrailTime += 40;
	if ( cent->dustTrailTime < cg.time ) {
		cent->dustTrailTime = cg.time;
	}

	VectorCopy(cent->currentState.pos.trBase, end);
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_DUST)  &&  cg_enableDust.integer != 4) {
		return;
	}

	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;

	VectorSet(vel, 0, 0, -30);
	//dust = CG_SmokePuff( end, vel,
	CG_SmokePuff( end, vel,
						 cg_smokeRadius_dust.value,  // 24
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.dustPuffShader );
}

#endif

static void CG_TrailItemExt (const centity_t *cent, qhandle_t hModel, int r, int g, int b, int a)
{
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	//Com_Printf("trail item..\n");

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	ent.shaderRGBA[0] = r;
	ent.shaderRGBA[1] = g;
	ent.shaderRGBA[2] = b;
	ent.shaderRGBA[3] = a;

	CG_AddRefEntity(&ent);
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( const centity_t *cent, qhandle_t hModel ) {
	CG_TrailItemExt(cent, hModel, 0, 0, 0, 0);
}


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, const refEntity_t *torso ) {
	const clientInfo_t	*ci;
	refEntity_t	pole;
	refEntity_t	flag;
	vec3_t		angles, dir;
	int			legsAnim, flagAnim, updateangles;
	float		angle, d;

	// show the flag pole model
	memset( &pole, 0, sizeof(pole) );
	pole.hModel = cgs.media.flagPoleModel;
	VectorCopy( torso->lightingOrigin, pole.lightingOrigin );
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	CG_PositionEntityOnTag( &pole, torso, torso->hModel, "tag_flag" );
	CG_AddRefEntity(&pole);

	// show the flag model
	memset( &flag, 0, sizeof(flag) );
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;

	VectorCopy( torso->lightingOrigin, flag.lightingOrigin );
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;
	VectorClear(angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if( legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR ) {
		flagAnim = FLAG_STAND;
	} else if ( legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR ) {
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	} else {
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if ( updateangles ) {

		VectorCopy( cent->currentState.pos.trDelta, dir );
		// add gravity
		dir[2] += 100;
		VectorNormalize( dir );
		d = DotProduct(pole.axis[2], dir);
		// if there is enough movement orthogonal to the flag pole
		if (fabs(d) < 0.9) {
			//
			d = DotProduct(pole.axis[0], dir);
			if (d > 1.0f) {
				d = 1.0f;
			}
			else if (d < -1.0f) {
				d = -1.0f;
			}
			angle = acos(d);

			d = DotProduct(pole.axis[1], dir);
			if (d < 0) {
				angles[YAW] = 360 - angle * 180 / M_PI;
			}
			else {
				angles[YAW] = angle * 180 / M_PI;
			}
			if (angles[YAW] < 0)
				angles[YAW] += 360;
			if (angles[YAW] > 360)
				angles[YAW] -= 360;

			//vectoangles( cent->currentState.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles( angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing );
		}

		/*
		d = DotProduct(pole.axis[2], dir);
		angle = Q_acos(d);

		d = DotProduct(pole.axis[1], dir);
		if (d < 0) {
			angle = 360 - angle * 180 / M_PI;
		}
		else {
			angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
			flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
			flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles[YAW] = cent->pe.flag.yawAngle;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RunLerpFrame( ci, &cent->pe.flag, flagAnim, 1, cent->cgtime );
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis( angles, flag.axis );
	CG_PositionRotatedEntityOnTag( &flag, &pole, pole.hModel, "tag_flag" );

	CG_AddRefEntity(&flag);
}

#if 1  //def MPACK // bk001204
/*
===============
CG_PlayerTokens
===============
*/
static void CG_PlayerTokens( const centity_t *cent, int renderfx ) {
	int			tokens, i, j;
	float		angle;
	refEntity_t	ent;
	vec3_t		dir, origin;
	skulltrail_t *trail;

	if (cent->currentState.number >= MAX_CLIENTS) {
		return;
	}

	trail = &cg.skulltrails[cent->currentState.number];
	tokens = cent->currentState.generic1;
	if ( !tokens ) {
		trail->numpositions = 0;
		return;
	}

	//CG_DrawHarversterHelpIcons(cent);

	if ( tokens > MAX_SKULLTRAIL ) {
		tokens = MAX_SKULLTRAIL;
	}

	// add skulls if there are more than last time
	for (i = 0; i < tokens - trail->numpositions; i++) {
		for (j = trail->numpositions; j > 0; j--) {
			VectorCopy(trail->positions[j-1], trail->positions[j]);
		}
		VectorCopy(cent->lerpOrigin, trail->positions[0]);
	}
	trail->numpositions = tokens;

	// move all the skulls along the trail
	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(trail->positions[i], origin, dir);
		if (VectorNormalize(dir) > 30) {
			VectorMA(origin, 30, dir, trail->positions[i]);
		}
		VectorCopy(trail->positions[i], origin);
	}

	memset( &ent, 0, sizeof( ent ) );
	if( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_BLUE ) {
		ent.hModel = cgs.media.redCubeModel;
	} else {
		ent.hModel = cgs.media.blueCubeModel;
	}
	ent.renderfx = renderfx;

	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(origin, trail->positions[i], ent.axis[0]);
		ent.axis[0][2] = 0;
		VectorNormalize(ent.axis[0]);
		VectorSet(ent.axis[2], 0, 0, 1);
		CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

		VectorCopy(trail->positions[i], ent.origin);
		angle = (((cg.time + 500 * MAX_SKULLTRAIL - 500 * i) / 16) & 255) * (M_PI * 2) / 255;
		ent.origin[2] += sin(angle) * 10;
		CG_AddRefEntity(&ent);
		VectorCopy(trail->positions[i], origin);
	}
}
#endif


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent, const refEntity_t *torso ) {
	int		powerups;
	const clientInfo_t *ci;
	qboolean light;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		CG_UpdatePositionData(cent, &cent->flightPositionData);
		CG_UpdatePositionData(cent, &cent->hastePositionData);
		return;
	}

	light = cg_powerupLight.integer;

	// 2010-08-08 new ql spawn protection powerup == 1
	//  if (powerup == 1) { /* spawn protection */ }

	//Com_Printf("%d %s %x\n", cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name, powerups);

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD )  &&  light) {
		//trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1 );
		//trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.75f, 1 );
		//trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.0f, 0.0f, 0.4 );
	}

	// flight plays a looped sound also flight trail
	if ( powerups & ( 1 << PW_FLIGHT ) ) {
		if (*EffectScripts.playerFlight) {
			if (1) {  //(cent->currentState.groundEntityNum == ENTITYNUM_NONE) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				//CG_CopyFlightDataToScript(cent);
				CG_CopyPositionDataToScript(&cent->flightPositionData);
				CG_RunQ3mmeScript(EffectScripts.playerFlight, NULL);
				//CG_CopyFlightDataToCent(cent);
				CG_CopyPositionDataToCent(&cent->flightPositionData);
			}
		} else {
			CG_FlightTrail(cent);
			CG_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound );
		}
	} else {
		CG_UpdatePositionData(cent, &cent->flightPositionData);
	}

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		if (cg_flagStyle.integer == 3) {
			int clientNum;
			int team;
			byte bcolor[3];

			if (wolfcam_following) {
				clientNum = wcg.clientNum;
			} else {
				clientNum = cg.snap->ps.clientNum;
			}
			team = cgs.clientinfo[clientNum].team;
			if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
				team = TEAM_FREE;
			}

			if (team == TEAM_RED) {
				// team color
				SC_ByteVec3ColorFromCvar(bcolor, &cg_teamFlagColor);
				CG_TrailItemExt(cent, cgs.media.neutralFlagModel3, bcolor[0], bcolor[1], bcolor[2], 255);
			} else if (team == TEAM_BLUE) {
				// enemy color
				SC_ByteVec3ColorFromCvar(bcolor, &cg_enemyFlagColor);
				CG_TrailItemExt(cent, cgs.media.neutralFlagModel3, bcolor[0], bcolor[1], bcolor[2], 255);
			} else {  // no team, free floating spec demo
				CG_TrailItem(cent, cgs.media.redFlagModel2);
			}
		} else if (cg_flagStyle.integer == 2) {
			CG_TrailItem(cent, cgs.media.redFlagModel2);
		} else {
			if (ci->newAnims) {
				CG_PlayerFlag( cent, cgs.media.redFlagFlapSkin, torso );
			}
			else {
				CG_TrailItem( cent, cgs.media.redFlagModel );
			}
		}
		if (light) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f );
		}
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		if (cg_flagStyle.integer == 3) {
			int clientNum;
			int team;
			byte bcolor[3];

			if (wolfcam_following) {
				clientNum = wcg.clientNum;
			} else {
				clientNum = cg.snap->ps.clientNum;
			}
			team = cgs.clientinfo[clientNum].team;
			if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
				team = TEAM_FREE;
			}

			if (team == TEAM_BLUE) {
				// team color
				SC_ByteVec3ColorFromCvar(bcolor, &cg_teamFlagColor);
				CG_TrailItemExt(cent, cgs.media.neutralFlagModel3, bcolor[0], bcolor[1], bcolor[2], 255);
			} else if (team == TEAM_RED) {
				// enemy color
				SC_ByteVec3ColorFromCvar(bcolor, &cg_enemyFlagColor);
				CG_TrailItemExt(cent, cgs.media.neutralFlagModel3, bcolor[0], bcolor[1], bcolor[2], 255);
			} else {  // no team, free floating spec demo
				CG_TrailItem(cent, cgs.media.blueFlagModel2);
			}
		} else if (cg_flagStyle.integer == 2) {
			CG_TrailItem(cent, cgs.media.blueFlagModel2);
		} else {
			if (ci->newAnims){
				CG_PlayerFlag( cent, cgs.media.blueFlagFlapSkin, torso );
			}
			else {
				CG_TrailItem( cent, cgs.media.blueFlagModel );
			}
		}
		if (light) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
		}
	}

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if (cg_flagStyle.integer == 3) {
			byte bcolor[3];

			SC_ByteVec3ColorFromCvar(bcolor, &cg_neutralFlagColor);
			CG_TrailItemExt(cent, cgs.media.neutralFlagModel3, bcolor[0], bcolor[1], bcolor[2], 255);
		} else if (cg_flagStyle.integer == 2) {
			CG_TrailItem(cent, cgs.media.neutralFlagModel2);
		} else {
			if (ci->newAnims) {
				CG_PlayerFlag( cent, cgs.media.neutralFlagFlapSkin, torso );
				//Com_Printf("flag...\n");
			}
			else {
				CG_TrailItem( cent, cgs.media.neutralFlagModel );
				//Com_Printf("flag trail...\n");
			}
		}
		if (light) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 1.0 );
		}
	}

	if (powerups & (1 << PW_BATTLESUIT)  &&  light) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 0.2f );
	}

	// haste leaves smoke trails
	if ( powerups & ( 1 << PW_HASTE ) ) {
		if (*EffectScripts.playerHaste) {
			int anim;

			anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
			if (anim == LEGS_RUN  ||   anim == LEGS_BACK) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				//CG_CopyStaticCentDataToScript(cent);
				CG_CopyPositionDataToScript(&cent->hastePositionData);
				CG_RunQ3mmeScript(EffectScripts.playerHaste, NULL);
				//CG_CopyStaticScriptDataToCent(cent);
				CG_CopyPositionDataToCent(&cent->hastePositionData);
			}
		} else {
			CG_HasteTrail( cent );
		}
	} else {
		CG_UpdatePositionData(cent, &cent->hastePositionData);
	}

	if (powerups & PWEX_SPAWNPROTECTION) {  // 2010-08-08 new ql invuln
		//CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		if (cg.time - cent->spawnArmorTime < cg_spawnArmorTime.integer) {
			//CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		}
	}

}


/*
===============
CG_PlayerFloatSpriteExt

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSpriteExt (const centity_t *cent, qhandle_t shader, int renderEffect, byte color[4], int offset) {
	int				rf;
	refEntity_t		ent;
	float dist;
	vec3_t org;
	float radius;
	float minWidth;
	float maxWidth;

	if (cg_drawSprites.integer == 0) {
		return;
	}

	if (cgs.gametype == GT_FREEZETAG  &&  shader == cgs.media.frozenShader  &&  cg_drawSpriteSelf.integer == 0  &&  CG_IsUs(&cgs.clientinfo[cent->currentState.clientNum])) {
		return;
	}

	//FIXME third person crap;
	//if ( cent->currentState.number == ourClientNum && !cg.renderingThirdPerson ) {
	if (CG_IsFirstPersonView(cent->currentState.number)) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	//FIXME hack
	if (cgs.gametype == GT_FREEZETAG  &&  shader == cgs.media.frozenShader) {
		rf = 0;
	}

	rf |= renderEffect;

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48 + offset;

	ent.radius = 10;

	if (color) {
		ent.shaderRGBA[0] = color[0];
		ent.shaderRGBA[1] = color[1];
		ent.shaderRGBA[2] = color[2];
		ent.shaderRGBA[3] = color[3];
	} else {
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
	}

	//FIXME shader checking at this point is fucked up since they might not have been loaded and will be default values
	if (shader == 0) {
		//FIXME pass
	} else if (shader == cgs.media.friendShader  ||  shader == cgs.media.foeShader  ||  shader == cgs.media.selfShader  ||  shader == cgs.media.selfDemoTakerShader) {
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, org);
		} else if (cg.freecam) {
			VectorCopy(cg.fpos, org);
		} else {
			VectorCopy(cg.refdef.vieworg, org);
		}

		minWidth = 0;
		maxWidth = 0;

		//FIXME fov

		// 16, 16  : 200
		// 16, 32  : 400

		if (shader == cgs.media.friendShader) {
			minWidth = cg_drawFriendMinWidth.value;
			maxWidth = cg_drawFriendMaxWidth.value;
		} else if (shader == cgs.media.foeShader) {
			minWidth = cg_drawFoeMinWidth.value;
			maxWidth = cg_drawFoeMaxWidth.value;
		} else if (shader == cgs.media.selfShader) {
			minWidth = cg_drawSelfMinWidth.value;
			maxWidth = cg_drawSelfMaxWidth.value;
		} else if (shader == cgs.media.selfDemoTakerShader) {
			minWidth = cg_drawSelfMinWidth.value;
			maxWidth = cg_drawSelfMaxWidth.value;
		}

		radius = maxWidth / 2.0;
		dist = ICON_SCALE_DISTANCE * (maxWidth / 16.0);

		if (minWidth > 0.1) {
			dist *= (16.0 / minWidth);
		}

		if (shader == cgs.media.selfDemoTakerShader  &&  cg_drawSelfIconStyle.integer == 1) {
			if (!CG_IsTeammate(&cgs.clientinfo[cg.snap->ps.clientNum])) {
				//radius = -radius;
				//Com_Printf("^3enemy\n");
				//ent.rotation = 90;
				//FIXME hack, shouldn't be changing shaders at this point
				shader = cgs.media.selfDemoTakerEnemyShader;
			} else {
				//Com_Printf("^2teammate\n");
			}
		} else if (shader == cgs.media.selfShader  &&  cg_drawSelfIconStyle.integer == 1) {
			if (!CG_IsTeammate(&cgs.clientinfo[cg.snap->ps.clientNum])) {
				//FIXME hack, shouldn't be changing shaders at this point
				shader = cgs.media.selfEnemyShader;
			} else {
				//Com_Printf("^2teammate\n");
			}
		}

		ent.radius = radius;


		if (Distance(ent.origin, org) > dist  &&  minWidth > 0.1) {
			ent.radius = radius * (Distance(ent.origin, org) / dist);
		}
	} else if (shader == cgs.media.flagCarrier  ||  shader == cgs.media.flagCarrierNeutral  ||  shader == cgs.media.flagCarrierHit) {
		// allowing negative values, flips icon
		//Com_Printf("what the fuck!!!!!  %d\n", shader);
		ent.radius = cg_drawFlagCarrierSize.value;
	}

	//ent.origin[2] += fabs(ent.radius);
	ent.origin[2] += ent.radius;

	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.renderfx = rf;

	CG_AddRefEntity(&ent);
}

static void CG_PlayerFloatSpriteNameExt (const centity_t *cent, qhandle_t shader, int renderEffect) {
	int				rf;
	refEntity_t		ent;
	int ourClientNum;

	if (cg.freecam) {
		ourClientNum = -1;
	} else if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if (cent->currentState.number == ourClientNum) {
		return;
	}

	//if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
	if (CG_IsFirstPersonView(cent->currentState.number)) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	if (cg_drawPlayerNames.integer == 2) {
		rf |= RF_DEPTHHACK;
	} else {
		//Com_Printf("no wall hack\n");
		rf = 0;
	}

	//rf |= renderEffect;

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 20;  // 2014-09-21 new ql font scale
	ent.origin[2] += cg_drawPlayerNamesY.value;  //48;  //48
	ent.reType = RT_SPRITE;
	ent.customShader = shader;

	// assumes max glyph width/height could be the same size as the font image
	// so need to add a factor or 512 / 64.0 to match old values with ql change
	// 2014-08-27

	ent.radius = 10.0 * 4.0 * 4.0 *  (float)cg_drawPlayerNamesScale.value;
	ent.useScale = qtrue;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	CG_AddRefEntity(&ent);
}

static void CG_PlayerFloatSprite (const centity_t *cent, qhandle_t shader)
{
	CG_PlayerFloatSpriteExt(cent, shader, 0, NULL, 0);
}

void CG_FloatSprite (qhandle_t shader, const vec3_t origin, int renderfx, const byte *color, int radius)
{
	refEntity_t		ent;
	//vec3_t forward, right, up;

	//AngleVectors(cg.refdefViewAngles, forward, right, up);
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy(origin, ent.origin);
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = radius;
	ent.renderfx = renderfx;

	if (!color) {
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
	} else {
		ent.shaderRGBA[0] = color[0];
		ent.shaderRGBA[1] = color[1];
		ent.shaderRGBA[2] = color[2];
		ent.shaderRGBA[3] = color[3];
	}

	CG_AddRefEntity(&ent);
}

#define NUMBER_SIZE 8

void CG_FloatNumber (int n, const vec3_t origin, int renderfx, const byte *color, float scale)
{
	refEntity_t		ent;
	int numdigits;
	int tmpNum;
	vec3_t forward, right, up;
	int d;
	int digits[32];
	qboolean drawNegative;

	if (scale < 0.0) {
		scale = 0.01;
	}

	if (n < 0) {
		drawNegative = qtrue;
		n = -n;
	} else {
		drawNegative = qfalse;
	}

	tmpNum = n;
	digits[0] = tmpNum % 10;
	tmpNum /= 10;
	for (numdigits = 1;  numdigits < 32  &&   tmpNum;  numdigits++) {
		digits[numdigits] = tmpNum % 10;
		tmpNum /= 10;
	}

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	if (drawNegative) {
		memset(&ent, 0, sizeof(ent));
		VectorCopy(origin, ent.origin);
		VectorMA(ent.origin, -16 * scale, right, ent.origin);
		ent.reType = RT_SPRITE;
		ent.customShader = cgs.media.numberShaders[STAT_MINUS];
		ent.radius = 8 * scale;
		ent.renderfx = renderfx;

		if (!color) {
			ent.shaderRGBA[0] = 255;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 255;
			ent.shaderRGBA[3] = 255;
		} else {
			ent.shaderRGBA[0] = color[0];
			ent.shaderRGBA[1] = color[1];
			ent.shaderRGBA[2] = color[2];
			ent.shaderRGBA[3] = color[3];
		}

		CG_AddRefEntity(&ent);
	}

	for (d = numdigits;  d > 0;  d--) {
		memset( &ent, 0, sizeof( ent ) );
		VectorCopy(origin, ent.origin);
		VectorMA(ent.origin, 16 * scale * (numdigits - d) - ((numdigits - 1) * 16 * scale / 2), right, ent.origin);
		ent.reType = RT_SPRITE;
		ent.customShader = cgs.media.numberShaders[digits[d - 1]];
		ent.radius = 8 * scale;
		ent.renderfx = renderfx;

		if (!color) {
			ent.shaderRGBA[0] = 255;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 255;
			ent.shaderRGBA[3] = 255;
		} else {
			ent.shaderRGBA[0] = color[0];
			ent.shaderRGBA[1] = color[1];
			ent.shaderRGBA[2] = color[2];
			ent.shaderRGBA[3] = color[3];
		}

		CG_AddRefEntity(&ent);
	}
}
#undef NUMBER_SIZE

void CG_FloatNumberExt (int n, const vec3_t origin, int renderfx, const byte *color, int time)
{
	floatNumber_t *f;

	if (cg.numFloatNumbers >= MAX_FLOAT_NUMBERS) {
		return;
	}

	f = &cg.floatNumbers[cg.numFloatNumbers];
	f->number = n;
	VectorCopy(origin, f->origin);
	f->renderfx = renderfx;
	f->color[0] = color[0];
	f->color[1] = color[1];
	f->color[2] = color[2];
	f->color[3] = color[3];
	f->time = time;
	f->startTime = cg.time;
	cg.numFloatNumbers++;

	//Com_Printf("%d float num: %d\n", cg.numFloatNumbers, n);
}

void CG_FloatEntNumbers (void)
{
	int i;
	const centity_t *cent;
	vec3_t origin;
	byte color[4];
	int n;

	if (!cg_drawEntNumbers.integer) {
		return;
	}

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cent = &cg_entities[i];
		n = cent - cg_entities;  // not cent->currentState.number since it can have a bogus value for invalid entities

		if (!cent->currentValid  &&  n != cg.snap->ps.clientNum) {
			continue;
		}
		if (cent->currentState.eFlags & EF_NODRAW  &&  cg_drawEntNumbers.integer < 3) {
			continue;
		}

		if (cent->currentState.eType >= ET_EVENTS) {
			//continue;
		}

		if (n == cg.snap->ps.clientNum) {
			VectorCopy(cg.predictedPlayerState.origin, origin);
		} else {
			VectorCopy(cent->lerpOrigin, origin);
		}

		origin[2] += 20;  // + (cent - cg_entities) * 3;  //(rand() & 400);

		MAKERGBA(color, 255, 255, 255, 255);

		if (cent->currentState.eType == ET_MISSILE) {
			color[0] = 0;
		} else if (n < MAX_CLIENTS) {
			color[2] = 0;
			origin[2] += 50;
		}

		if (cg_drawEntNumbers.integer > 1) {
			CG_FloatNumber(n, origin, RF_DEPTHHACK, color, 1.0);
		} else {
			CG_FloatNumber(n, origin, 0, color, 1.0);
		}
	}
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	//int		team;
	qhandle_t s;
	const clientInfo_t *ci;
	qboolean flagIconDrawn;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	//team = ci->team;

	if (cg_drawPlayerNames.integer) {
		CG_PlayerFloatSpriteNameExt(cent, cgs.clientNameImage[cent->currentState.clientNum], 0);
		//Com_Printf("%d -> %d\n", cent->currentState.clientNum, cgs.clientNameImage[cent->currentState.clientNum]);
	}

	if (cg_rocketAimBot.integer) {
		qboolean us;

		if (wolfcam_following  &&  cent->currentState.number == wcg.clientNum) {
			us = qtrue;
		} else if (!wolfcam_following  &&  cent->currentState.number == cg.snap->ps.clientNum) {
			us = qtrue;
		} else {
			us = qfalse;
		}

		if (!us) {
			byte color[4] = { 255, 255, 255, 255 };
			vec3_t epos;

			CG_Rocket_Aim(cent, epos);
			CG_SimpleRailTrail(cent->lerpOrigin, epos, 10, color);
			CG_FloatSprite(cgs.media.rocketAimShader, epos, RF_DEPTHHACK, color, 24 /* radius */);
			//CG_FloatSprite(cgs.media.balloonShader, epos, 0, color, 16 /* radius */);
			if (cg_rocketAimBot.integer > 1) {
				vec3_t dir;
				vec3_t angles;

				VectorSubtract(epos, cg.snap->ps.origin, dir);
				vectoangles(dir, angles);
				trap_SendClientCommand(va("view %f %f %f\n", angles[0], angles[1], angles[2]));
			}
		}
	}


	// draw flag icons before anything else
	flagIconDrawn = qfalse;
	if ( (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_CTFS)
         &&  cg_drawFlagCarrier.integer
		 ) {
		qboolean depthHack;
		qboolean isTeammate;
		byte color[4];

		//FIXME cvar?
		depthHack = qtrue;

		isTeammate = CG_IsTeammate(ci);

		s = 0;

		if (cent->currentState.powerups & (1 << PW_REDFLAG)) {
			if (isTeammate) {
				s = cgs.media.flagCarrier;
				Vector4Set(color, 255, 255, 0, 255);
			} else {
				if (cg_drawFlagCarrier.integer > 1) {
					s = cgs.media.flagCarrier;
					Vector4Set(color, 255, 255, 255, 255);
				}
			}
		} else if (cent->currentState.powerups & (1 << PW_BLUEFLAG)) {
			if (isTeammate) {
				s = cgs.media.flagCarrier;
				Vector4Set(color, 255, 255, 0, 255);
			} else {
				if (cg_drawFlagCarrier.integer > 1) {
					s = cgs.media.flagCarrier;
					Vector4Set(color, 255, 255, 255, 255);
				}
			}
		} else if (cent->currentState.powerups & (1 << PW_NEUTRALFLAG)) {
			s = cgs.media.flagCarrierNeutral;
			Vector4Set(color, 255, 255, 255, 255);
		}

		if (s  &&  (cg.ftime - cent->pe.painTime) < cg_drawHitFlagCarrierTime.integer) {
			s = cgs.media.flagCarrierHit;
			Vector4Set(color, 255, 0, 0, 255);
		}

		if (s) {
			CG_PlayerFloatSpriteExt(cent, s, depthHack ? RF_DEPTHHACK : 0, color, 0);
			flagIconDrawn = qtrue;
		}
	}

	if (cent->currentState.eFlags & EF_CONNECTION) {
		int offset = 0;

		if (flagIconDrawn) {
			offset += 32;
		}

		if (*EffectScripts.playerConnection) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48 + offset;
			CG_RunQ3mmeScript(EffectScripts.playerConnection, NULL);
			return;
		}
		CG_PlayerFloatSpriteExt(cent, cgs.media.connectionShader, 0, NULL, offset);
		return;
	}

	if (cent->currentState.eFlags & EF_TALK) {
		int offset = 0;

		if (flagIconDrawn) {
			offset += 32;
		}

		if (*EffectScripts.playerTalk) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48 + offset;
			CG_RunQ3mmeScript(EffectScripts.playerTalk, NULL);
			return;
		}
		CG_PlayerFloatSpriteExt(cent, cgs.media.balloonShader, 0, NULL, offset);
		return;
	}

	if (flagIconDrawn) {
		return;
	}

	// check new ql rewards first
	//FIXME what is the order now?
	//FIXME dead players?
	// cent->currentState.eFlags & EF_DEAD

	if (qtrue) {  //(!(cent->currentState.eFlags & EF_DEAD)) {  // don't draw for dead ppl
		//FIXME time
		if (cg.time <= ci->clientRewards.startTime) {  // demo seeking
			// pass
		} else if (cg.time - ci->clientRewards.startTime <= 1500) {
			// check fx
			if (ci->clientRewards.shader == cgs.media.medalComboKill  &&  *EffectScripts.playerMedalComboKill) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalComboKill, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalMidAir  &&  *EffectScripts.playerMedalMidAir) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalMidAir, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalRevenge  &&  *EffectScripts.playerMedalRevenge) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalRevenge, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalFirstFrag  &&  *EffectScripts.playerMedalFirstFrag) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalFirstFrag, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalRampage  &&  *EffectScripts.playerMedalRampage) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalRampage, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalPerforated  &&  *EffectScripts.playerMedalPerforated) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalPerforated, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalAccuracy  &&  *EffectScripts.playerMedalAccuracy) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalAccuracy, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalHeadshot  &&  *EffectScripts.playerMedalHeadshot) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalHeadshot, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalPerfect  &&  *EffectScripts.playerMedalPerfect) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalPerfect, NULL);
				return;
			}
			if (ci->clientRewards.shader == cgs.media.medalQuadGod  &&  *EffectScripts.playerMedalQuadGod) {
				CG_ResetScriptVars();
				CG_CopyPlayerDataToScriptData(cent);
				ScriptVars.origin[2] += 48;
				CG_RunQ3mmeScript(EffectScripts.playerMedalQuadGod, NULL);
				return;
			}

			// no fx
			CG_PlayerFloatSprite(cent, ci->clientRewards.shader);
			return;
		}
	}

	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
		if (*EffectScripts.playerImpressive) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerImpressive, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
		if (*EffectScripts.playerExcellent) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerExcellent, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_GAUNTLET ) {
		if (*EffectScripts.playerGauntlet) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerGauntlet, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_DEFEND ) {
		if (*EffectScripts.playerDefend) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerDefend, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_ASSIST ) {
		if (*EffectScripts.playerAssist) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerAssist, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_CAP ) {
		if (*EffectScripts.playerCapture) {
			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(cent);
			ScriptVars.origin[2] += 48;
			CG_RunQ3mmeScript(EffectScripts.playerCapture, NULL);
			return;
		}
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}

	if (cg_drawSelf.integer  &&  cent->currentState.number == cg.snap->ps.clientNum) {
		//if (cg.freecam  ||  cg.renderingThirdPerson  ||  (wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum)) {
		if (wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum) {
			if (cent->currentState.number == cg.clientNum) {
				CG_PlayerFloatSpriteExt(cent, cgs.media.selfDemoTakerShader, cg_drawSelf.integer == 2 ? RF_DEPTHHACK : 0, NULL, 0);
				//Com_Printf("^3self...\n");
			} else {
				CG_PlayerFloatSpriteExt(cent, cgs.media.selfShader, cg_drawSelf.integer == 2 ? RF_DEPTHHACK : 0, NULL, 0);
			}
			return;
		}
	}

	if (CG_IsTeamGame(cgs.gametype)) {
		qboolean depthHack;
		qboolean isTeammate;

		depthHack = qfalse;
		if (cgs.gametype == GT_FREEZETAG) {
			if (cg_drawFriend.integer == 3) {
				depthHack = qtrue;
			}
		} else {
			if (cg_drawFriend.integer == 1  ||  cg_drawFriend.integer == 3) {
				depthHack = qtrue;
			}
		}

		isTeammate = CG_IsTeammate(ci);
		if (CG_FreezeTagFrozen(cent->currentState.clientNum)  &&  isTeammate) {
			s = cgs.media.frozenShader;
			CG_PlayerFloatSpriteExt(cent, s, depthHack ? RF_DEPTHHACK : 0, NULL, 0);
			return;
		}

		if (isTeammate) {
			if (cg_drawFriend.integer) {
				if ((cg.ftime - cent->pe.painTime) < cg_drawHitFriendTime.integer) {
					s = cgs.media.friendHitShader;
					CG_PlayerFloatSpriteExt(cent, s, depthHack ? RF_DEPTHHACK : 0, NULL, 0);
					return;
				}
			}
		}

		if (isTeammate) {
			if (cent->currentState.eFlags & EF_DEAD) {
				s = cgs.media.friendDeadShader;
				if (cg_drawFriend.integer) {
					int deathTime;

					if (cent->currentState.clientNum == cg.snap->ps.clientNum) {
						deathTime = cg.predictedPlayerEntity.pe.deathTime;
					} else {
						deathTime = cg_entities[cent->currentState.clientNum].pe.deathTime;
					}
					if (cg.time - deathTime <= cg_drawDeadFriendTime.integer) {
						CG_PlayerFloatSpriteExt(cent, s, depthHack ? RF_DEPTHHACK : 0, NULL, 0);
					}
				}
				return;
			} else {
				if (cg_drawFriend.integer) {
					byte bcolor[4];

					s = cgs.media.friendShader;
					// other q3 mods might not have team info available
					if (cg_drawFriendStyle.integer == 1  &&  ci->hasTeamInfo) {
						//vec4_t fcolor;
						//FIXME armor?
						//CG_GetColorForHealth(ci->health, ci->armor, fcolor);
						//CG_GetColorForHealth(ci->health, 0, fcolor);
						Vector4Set(bcolor, 255, 255, 0, 255);
						if (ci->health < 30) {
							//FIXME what about friend hit shader which is red?
							bcolor[1] = bcolor[2] = 0;  // red
						} else if (ci->health < 66) {
							bcolor[1] = 126;  // orange
						} else if (ci->health < 100) {
							bcolor[1] = 255;  // yellow
						} else {  // ci->health >= 100
							bcolor[2] = 125;  // light yellow
						}

						//Com_Printf("^3color: %d %d %d   health: %d\n", bcolor[0], bcolor[1], bcolor[2], ci->health);
					} else {
						bcolor[0] = 255;
						bcolor[1] = 255;
						bcolor[2] = 0;
						bcolor[3] = 255;
					}
					CG_PlayerFloatSpriteExt(cent, s, depthHack ? RF_DEPTHHACK : 0, bcolor, 0);
				}
				return;
			}
		}
	}  // end 'is team game'

	if (!(cent->currentState.eFlags & EF_DEAD)  &&  CG_IsEnemy(&cgs.clientinfo[cent->currentState.clientNum])) {
		if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE  &&  cg_allowServerOverride.integer) {
			if (cg_drawInfected.integer) {
				CG_PlayerFloatSpriteExt(cent, cgs.media.infectedFoeShader, RF_DEPTHHACK, NULL, 0);
			}
		} else {
			s = cgs.media.foeShader;
			if (cg_drawFoe.integer) {
				CG_PlayerFloatSpriteExt(cent, s, cg_drawFoe.integer == 2 ? RF_DEPTHHACK : 0, NULL, 0);
			}
		}
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( const centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;


#if 0
	// testing, trying to find closest surface point
	{
		//vec3_t end, mins = {-32000, -32000, -32000 }, maxs = { 32000, 32000, 32000 };
		vec3_t end, mins = {-201, -200, -200 }, maxs = { 200, 200, 200 };
		trace_t trace;
		float alpha;

		alpha = 1.0;

		VectorCopy(cent->lerpOrigin, end);

		trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_SOLID);

#if 0
		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		energy = qtrue;
		radius = 24;
#endif
		if (trace.fraction != 1.0  &&  !trace.startsolid  &&  !trace.allsolid) {
			CG_ImpactMark( cgs.media.railExplosionShader, trace.endpos, trace.plane.normal, 
				   cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue, qtrue, qfalse );
		}

	}
#endif

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0  ||  SC_Cvar_Get_Int("r_debugMarkSurface") ) {
		return qfalse;
	}

	// no shadows when invisible
	if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
				   cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue, qfalse, qfalse );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( const centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer  ||  SC_Cvar_Get_Int("r_debugMarkSurface") ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts, qfalse );
}

#if 0
static qboolean CG_ModelIsProjectile (qhandle_t model)
{
	if (model == cg_weapons[WP_GRENADE_LAUNCHER].missileModel  ||  model == cg_weapons[WP_ROCKET_LAUNCHER].missileModel  ||  model == cg_weapons[WP_BFG].missileModel  ||  model == cg_weapons[WP_PROX_LAUNCHER].missileModel  ||  model == cgs.media.blueProxMine) {
		return qtrue;
	} else {
		return qfalse;
	}
}
#endif


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, const entityState_t *state, int team ) {

	if ( state->powerups & ( 1 << PW_INVIS ) ) {
		ent->customShader = cgs.media.invisShader;
		CG_AddRefEntity(ent);
	} else {  // no invis
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
			if (team == TEAM_BLUE)
				ent->customShader = cgs.media.blueKamikazeShader;
			else
				ent->customShader = cgs.media.redKamikazeShader;
			CG_AddRefEntity(ent);
		}
		else {*/
			CG_AddRefEntity(ent);
		//}

		//if (state->powerups == 1) {
		if (cg.time - cg_entities[state->number].spawnArmorTime < cg_spawnArmorTime.integer) {
			ent->customShader = cgs.media.spawnArmorShader;
			//Com_Printf("spawnarmor %d %s\n", state->clientNum, cgs.clientinfo[state->clientNum].name);
			CG_AddRefEntity(ent);
		}
		if ( state->powerups & ( 1 << PW_QUAD ) )
		{
			if (0) //(team == TEAM_RED)  // not in quake live
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			CG_AddRefEntity(ent);
		}
		if ( state->powerups & ( 1 << PW_REGEN ) ) {
			if ( ( ( cg.time / 100 ) % 10 ) == 1 ) {
				ent->customShader = cgs.media.regenShader;
				CG_AddRefEntity(ent);
			}
		}
		if ( state->powerups & ( 1 << PW_BATTLESUIT ) ) {
			ent->customShader = cgs.media.battleSuitShader;
			CG_AddRefEntity(ent);
		}
		if (CG_FreezeTagFrozen(state->clientNum)  &&  ent->hModel != cg_weapons[WP_GRENADE_LAUNCHER].missileModel  &&  ent->hModel != cg_weapons[WP_ROCKET_LAUNCHER].missileModel  &&  ent->hModel != cg_weapons[WP_BFG].missileModel  &&  ent->hModel != cg_weapons[WP_PROX_LAUNCHER].missileModel  &&  ent->hModel != cgs.media.blueProxMine) {
			int n;

			// 3, 1, 0
			switch (state->generic1) {
			case 3:
				n = 0;
				break;
			case 1:
				n = 1;
				break;
			case 0:
				n = 2;
				break;
			default:
				n = 2;
				break;
			}
			ent->customShader = cgs.media.iceShader[n];
			CG_AddRefEntity(ent);
		}

		if (*cg_playerShader.string) {
			ent->shaderRGBA[0] = 255;
			ent->shaderRGBA[1] = 255;
			ent->shaderRGBA[2] = 255;
			ent->shaderRGBA[3] = 255;

			ent->customShader = trap_R_RegisterShader(cg_playerShader.string);
			CG_AddRefEntity(ent);
		}

		if (cg_wh.integer) {
			if (cg_whIncludeDeadBody.integer == 0  &&  (state->eFlags & EF_DEAD)) {
				// pass
			} else if (cg_whIncludeProjectile.integer == 0  &&  state->eType == ET_MISSILE) {
				// pass
			} else {
				if (CG_IsEnemyTeam(team)) {
					if (cg_wh.integer == 3) {
						return;
					}
					SC_ByteVec3ColorFromCvar(ent->shaderRGBA, &cg_whEnemyColor);
					ent->shaderRGBA[3] = cg_whEnemyAlpha.integer;

					if (*cg_whEnemyShader.string) {
						ent->customShader = trap_R_RegisterShader(cg_whEnemyShader.string);
					} else {
						ent->customShader = cgs.media.wallHackShader;
					}
				} else {
					if (cg_wh.integer == 2) {  // only enemies
						return;
					}
					SC_ByteVec3ColorFromCvar(ent->shaderRGBA, &cg_whColor);
					ent->shaderRGBA[3] = cg_whAlpha.integer;

					if (*cg_whShader.string) {
						ent->customShader = trap_R_RegisterShader(cg_whShader.string);
					} else {
						ent->customShader = cgs.media.wallHackShader;
					}
				}

				ent->renderfx |= RF_DEPTHHACK;

				CG_AddRefEntity(ent);
			}
		}
	}
}

#if 0  // unused
/*
=================
CG_LightVerts
=================
*/
static int CG_LightVerts( const vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

#endif

#define SKIN_INVALID -1

static void CG_LoadAltTeamSkins (clientInfo_t *ci, qboolean useTeamModel)
{
	char modelStr[MAX_QPATH];
	char skinStr[MAX_QPATH];

	//Com_Printf("%s  '%s' '%s' '%s'\n", ci->name, ci->modelName, modelStr, skinStr);
	//Com_Printf("new alt skins\n");
	if (*cg_teamHeadSkin.string) {
		CG_GetModelAndSkinName(cg_teamHeadSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_teamHeadModel.string  &&  useTeamModel) {
				CG_GetModelName(cg_teamHeadModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->headModelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->headTeamSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/head_%s.skin", modelStr, skinStr));
		if (!ci->headTeamSkinAlt) {
			Com_Printf("^3couldn't load team head skin %s %s\n", modelStr, skinStr);
			ci->headTeamSkinAlt = SKIN_INVALID;
		}
	}
	if (*cg_teamTorsoSkin.string) {
		CG_GetModelAndSkinName(cg_teamTorsoSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_teamModel.string  &&  useTeamModel) {
				CG_GetModelName(cg_teamModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}
		//Com_Printf("^6team torso:  %s %s\n", modelStr, skinStr);
		ci->torsoTeamSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/upper_%s.skin", modelStr, skinStr));
		if (!ci->torsoTeamSkinAlt) {
			Com_Printf("^3couldn't load team torso skin: %s %s\n", modelStr, skinStr);
			ci->torsoTeamSkinAlt = SKIN_INVALID;
		}
	}
	if (*cg_teamLegsSkin.string) {
		CG_GetModelAndSkinName(cg_teamLegsSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_teamModel.string  &&  useTeamModel) {
				CG_GetModelName(cg_teamModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->legsTeamSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/lower_%s.skin", modelStr, skinStr));
		if (!ci->legsTeamSkinAlt) {
			Com_Printf("^3couldn't load team legs skin: %s %s\n", modelStr, skinStr);
			ci->legsTeamSkinAlt = SKIN_INVALID;
		}
	}
}

static void CG_LoadAltEnemySkins (clientInfo_t *ci)
{
	char modelStr[MAX_QPATH];
	char skinStr[MAX_QPATH];

	//Com_Printf("enemy\n");

	if (*cg_enemyHeadSkin.string) {
		CG_GetModelAndSkinName(cg_enemyHeadSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_enemyHeadModel.string) {
				CG_GetModelName(cg_enemyHeadModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->headModelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->headEnemySkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/head_%s.skin", modelStr, skinStr));
		if (!ci->headEnemySkinAlt) {
			Com_Printf("^3couldn't load enemy head skin: %s %s\n", modelStr, skinStr);
			ci->headEnemySkinAlt = SKIN_INVALID;
		}
		//Com_Printf("models/players/%s/head_%s.skin\n", modelStr, skinStr);
	}
	if (*cg_enemyTorsoSkin.string) {
		CG_GetModelAndSkinName(cg_enemyTorsoSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_enemyModel.string) {
				CG_GetModelName(cg_enemyModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		//Com_Printf("enemy trying torso:  %s %s\n", modelStr, skinStr);
		ci->torsoEnemySkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/upper_%s.skin", modelStr, skinStr));
		if (!ci->torsoEnemySkinAlt) {
			Com_Printf("^3couldn't load enemy torso skin: %s %s\n", modelStr, skinStr);
			ci->torsoEnemySkinAlt = SKIN_INVALID;
		}
	}
	if (*cg_enemyLegsSkin.string) {
		CG_GetModelAndSkinName(cg_enemyLegsSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_enemyModel.string) {
				CG_GetModelName(cg_enemyModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->legsEnemySkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/lower_%s.skin", modelStr, skinStr));
		if (!ci->legsEnemySkinAlt) {
			Com_Printf("^3couldn't load enemy legs skin: %s %s\n", modelStr, skinStr);
			ci->legsEnemySkinAlt = SKIN_INVALID;
		}
	}
}

static void CG_LoadAltOurSkins (clientInfo_t *ci)
{
	char modelStr[MAX_QPATH];
	char skinStr[MAX_QPATH];

	//Com_Printf("our\n");

	if (*cg_ourHeadSkin.string) {
		CG_GetModelAndSkinName(cg_ourHeadSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_ourHeadModel.string) {
				CG_GetModelName(cg_ourHeadModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->headModelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->headOurSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/head_%s.skin", modelStr, skinStr));
		if (!ci->headOurSkinAlt) {
			Com_Printf("^3couldn't load our head skin: %s %s\n", modelStr, skinStr);
			ci->headOurSkinAlt = SKIN_INVALID;
		}
		//Com_Printf("models/players/%s/head_%s.skin\n", modelStr, skinStr);
	}
	if (*cg_ourTorsoSkin.string) {
		CG_GetModelAndSkinName(cg_ourTorsoSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_ourModel.string) {
				CG_GetModelName(cg_ourModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		//Com_Printf("our trying torso:  %s %s\n", modelStr, skinStr);
		ci->torsoOurSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/upper_%s.skin", modelStr, skinStr));
		if (!ci->torsoOurSkinAlt) {
			Com_Printf("^3couldn't load our torso skin: %s %s\n", modelStr, skinStr);
			ci->torsoOurSkinAlt = SKIN_INVALID;
		}
	}
	if (*cg_ourLegsSkin.string) {
		CG_GetModelAndSkinName(cg_ourLegsSkin.string, modelStr, skinStr);
		if (!*modelStr) {
			if (*cg_ourModel.string) {
				CG_GetModelName(cg_ourModel.string, modelStr);
			} else {
				Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
			}
		}
		if (!Q_stricmp(skinStr, TEAM_COLOR_SKIN)) {
			if (CG_IsTeamGame(cgs.gametype)) {
				if (ci->team == TEAM_RED) {
					Q_strncpyz(skinStr, "red", sizeof(skinStr));
				} else if (ci->team == TEAM_BLUE) {
					Q_strncpyz(skinStr, "blue", sizeof(skinStr));
				} else {
					Q_strncpyz(skinStr, "default", sizeof(skinStr));
				}
			} else {
				Q_strncpyz(skinStr, "default", sizeof(skinStr));
			}
		}

		ci->legsOurSkinAlt = CG_RegisterSkinVertexLight(va("models/players/%s/lower_%s.skin", modelStr, skinStr));
		if (!ci->legsOurSkinAlt) {
			Com_Printf("^3couldn't load our legs skin: %s %s\n", modelStr, skinStr);
			ci->legsOurSkinAlt = SKIN_INVALID;
		}
	}
}

// modifies ci-> head|torso|legs models and skins and ci-> alt team|enemy skins

static void CG_CheckForModelChange (const centity_t *cent, clientInfo_t *ci, refEntity_t *legs, refEntity_t *torso, refEntity_t *head)
{
	qboolean usingFallbackTeamModel = qfalse;

	if (cgs.cpma) {
		memcpy(head->shaderRGBA, ci->headColor, sizeof(head->shaderRGBA));
		memcpy(torso->shaderRGBA, ci->torsoColor, sizeof(torso->shaderRGBA));
		memcpy(legs->shaderRGBA, ci->legsColor, sizeof(legs->shaderRGBA));
	}

	if (cg_clientOverrideIgnoreTeamSettings.integer  &&  cgs.clientinfo[cent->currentState.clientNum].override) {
		if (ci != &cgs.clientinfo[cent->currentState.clientNum]) {
			memcpy(ci, &cgs.clientinfo[cent->currentState.clientNum], sizeof(*ci));
		}

		if (ci->hasHeadColor) {
			memcpy(head->shaderRGBA, ci->headColor, sizeof(head->shaderRGBA));
		}
		if (ci->hasTorsoColor) {
			memcpy(torso->shaderRGBA, ci->torsoColor, sizeof(torso->shaderRGBA));
		}
		if (ci->hasLegsColor) {
			memcpy(legs->shaderRGBA, ci->legsColor, sizeof(legs->shaderRGBA));
		}

		if (cg_entities[cent->currentState.clientNum].currentState.eFlags & EF_DEAD  ||  (cent->currentState.clientNum != cent->currentState.number &&  cent !=&cg.predictedPlayerEntity)) {
			//Com_Printf("%s  %d %d\n", cgs.clientinfo[cent->currentState.clientNum].name, cent->currentState.clientNum, cent->currentState.number);
			if (!CG_IsUs(ci)  &&  *cg_deadBodyColor.string) {
				SC_ByteVec3ColorFromCvar(head->shaderRGBA, &cg_deadBodyColor);
				head->shaderRGBA[3] = 255;
				SC_ByteVec3ColorFromCvar(torso->shaderRGBA, &cg_deadBodyColor);
				torso->shaderRGBA[3] = 255;
				SC_ByteVec3ColorFromCvar(legs->shaderRGBA, &cg_deadBodyColor);
				legs->shaderRGBA[3] = 255;
			}
		}

		return;
	}


	if (cg_forceModel.integer) {
		// skip team and enemy model settings
		goto server_settings;
	}

	if (CG_IsUs(ci)) {
		qboolean freecamPovSettings = qfalse;

		if (cg.freecam  &&  CG_IsTeamGame(cgs.gametype)  &&  cg_freecam_useTeamSettings.integer  &&  cg_forcePovModelIgnoreFreecamTeamSettings.integer == 0) {
			freecamPovSettings = qtrue;
		}

		if (cg_forcePovModel.integer == 2  &&  freecamPovSettings == qfalse) {
			CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			CG_CopyClientInfoModel(&cg.ourModel, ci);

			//FIXME -teamcolor skin
			//ci->torsoSkin = cg.ourModelRed.torsoSkin;

			if (CG_IsTeamGame(cgs.gametype)) {
				if (cg.ourModelUsingTeamColorSkin) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						ci->torsoSkin = cg.ourModelRed.torsoSkin;
						ci->legsSkin = cg.ourModelRed.legsSkin;
					} else if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE) {
						ci->torsoSkin = cg.ourModelBlue.torsoSkin;
						ci->legsSkin = cg.ourModelBlue.legsSkin;
					}
				}

				if (cg.ourModelUsingTeamColorHeadSkin) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						ci->headSkin = cg.ourModelRed.headSkin;
					} else if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE) {
						ci->headSkin = cg.ourModelBlue.headSkin;
					}
				}
			}

			memcpy(head->shaderRGBA, &cg.ourColors[0], sizeof(head->shaderRGBA));
			memcpy(torso->shaderRGBA, &cg.ourColors[1], sizeof(torso->shaderRGBA));
			memcpy(legs->shaderRGBA, &cg.ourColors[2], sizeof(legs->shaderRGBA));

			// disallow enemy model for teammates even here?  -- 2016-05-26 don't think so since cg_forcePovModel 2 ignores team settings

			// hack to update alt models if they have fallbacks to
			// original player and the person viewed changes.  Ex:
			// model "", cg_teamTorsoSkin "/color", cg_forcePovModel 1
			{
				static qhandle_t lastHeadModel = -1;
				static qhandle_t lastTorsoModel = -1;
				static qhandle_t lastLegsModel = -1;
				static qhandle_t lastTeam = -1;

				if (lastTorsoModel != ci->torsoModel  ||  lastLegsModel != ci->legsModel  ||  lastHeadModel != ci->headModel  ||  lastTeam != ci->team) {
					//Com_Printf("^2  model change (%d %d %d) -> (%d %d %d)\n", lastHeadModel, lastTorsoModel, lastLegsModel, ci->headModel, ci->torsoModel, ci->legsModel);
					lastTorsoModel = ci->torsoModel;
					lastLegsModel = ci->legsModel;
					lastHeadModel = ci->headModel;
					lastTeam = ci->team;

					// reset alt our skins for own model
					cg.ourModel.headOurSkinAlt = 0;
					cg.ourModel.torsoOurSkinAlt = 0;
					cg.ourModel.legsOurSkinAlt = 0;
					cg.ourModelRed.headTeamSkinAlt = 0;
					cg.ourModelRed.torsoTeamSkinAlt = 0;
					cg.ourModelRed.legsTeamSkinAlt = 0;
					cg.ourModelBlue.headTeamSkinAlt = 0;
					cg.ourModelBlue.torsoTeamSkinAlt = 0;
					cg.ourModelBlue.legsTeamSkinAlt = 0;
				}
			}

			// here is where you do it
			if (*cg_ourHeadSkin.string  ||  *cg_ourTorsoSkin.string  ||  *cg_ourLegsSkin.string) {
				clientInfo_t *cio;

				cio = &cg.ourModel;

				if ((!cio->headOurSkinAlt  &&  *cg_ourHeadSkin.string)  ||
					(!cio->torsoOurSkinAlt  &&  *cg_ourTorsoSkin.string)  ||
					(!cio->legsOurSkinAlt  &&  *cg_ourLegsSkin.string)
					) {
					const clientInfo_t *origCi = &cgs.clientinfoOrig[cent->currentState.clientNum];
					//FIXME hack for changing model and headmodel name
					// temporarily
					char origModelName[MAX_QPATH];
					char origHeadModelName[MAX_QPATH];
					int origTeam;

					origTeam = cio->team;

					Q_strncpyz(origModelName, cio->modelName, sizeof(origModelName));
					Q_strncpyz(origHeadModelName, cio->headModelName, sizeof(origHeadModelName));

					// hack to use original values as defaults
					if (!*cg_ourModel.string) {
						Q_strncpyz(cio->modelName, origCi->modelName, sizeof(cio->modelName));
					}

					if (!*cg_ourHeadModel.string) {
						Q_strncpyz(cio->headModelName, origCi->headModelName, sizeof(cio->headModelName));
					}

					cio->team = ci->team;

					CG_LoadAltOurSkins(cio);
					//Com_Printf("    ^5loaded alt team skins for '%s'\n", ci->name);

					// don't modify [head]model name
					Q_strncpyz(cio->modelName, origModelName, sizeof(cio->modelName));
					Q_strncpyz(cio->headModelName, origHeadModelName, sizeof(cio->headModelName));
					cio->team = origTeam;
				} // end alt skin set

				if ((cio->headOurSkinAlt != 0  &&  cio->headOurSkinAlt != SKIN_INVALID)  &&  *cg_ourHeadSkin.string) {
					ci->headSkin = cio->headOurSkinAlt;
				}
				if ((cio->torsoOurSkinAlt != 0  &&  cio->torsoOurSkinAlt != SKIN_INVALID)  &&  *cg_ourTorsoSkin.string) {
					ci->torsoSkin = cio->torsoOurSkinAlt;
				}
				if ((cio->legsOurSkinAlt != 0  &&  cio->legsOurSkinAlt != SKIN_INVALID)  &&  *cg_ourLegsSkin.string) {
					ci->legsSkin = cio->legsOurSkinAlt;
				}
			}
			return;
		}

		if (cg_forcePovModel.integer  &&  freecamPovSettings == qfalse) {
			if (CG_IsTeamGame(cgs.gametype)) {
				memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
				memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
				memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));

				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);

				if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
					CG_CopyClientInfoModel(&cg.ourModelRed, ci);
				} else if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE) {
					CG_CopyClientInfoModel(&cg.ourModelBlue, ci);
				}

				if (cg.ourModel.headModel) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						ci->headSkin = cg.ourModelRed.headSkin;
					} else {
						ci->headSkin = cg.ourModelBlue.headSkin;
					}
				}

				// disallow enemy teammate model
				if ((cg_disallowEnemyModelForTeammates.integer  &&  cg_disallowEnemyModelForTeammates.integer != 2)  &&  ci->legsModel == cg.enemyModel.legsModel) {
					usingFallbackTeamModel = qtrue;

					// team game
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						CG_CopyClientInfoModel(&cg.fallbackModelRed, ci);
					} else {
						CG_CopyClientInfoModel(&cg.fallbackModelBlue, ci);
					}
				}

				// hack to update alt models if they have fallbacks to
				// original player and the person viewed changes.  Ex:
				// model "", cg_teamTorsoSkin "/color", cg_forcePovModel 1
				{
					static qhandle_t lastHeadModel = -1;
					static qhandle_t lastTorsoModel = -1;
					static qhandle_t lastLegsModel = -1;
					static qhandle_t lastTeam = -1;

					if (lastTorsoModel != ci->torsoModel  ||  lastLegsModel != ci->legsModel  ||  lastHeadModel != ci->headModel  ||  lastTeam != ci->team) {
						//Com_Printf("^2  model change (%d %d %d) -> (%d %d %d)\n", lastHeadModel, lastTorsoModel, lastLegsModel, ci->headModel, ci->torsoModel, ci->legsModel);
						lastTorsoModel = ci->torsoModel;
						lastLegsModel = ci->legsModel;
						lastHeadModel = ci->headModel;
						lastTeam = ci->team;

						// reset alt team skins for own model
						cg.ourModel.headTeamSkinAlt = 0;
						cg.ourModel.torsoTeamSkinAlt = 0;
						cg.ourModel.legsTeamSkinAlt = 0;
						cg.ourModelRed.headTeamSkinAlt = 0;
						cg.ourModelRed.torsoTeamSkinAlt = 0;
						cg.ourModelRed.legsTeamSkinAlt = 0;
						cg.ourModelBlue.headTeamSkinAlt = 0;
						cg.ourModelBlue.torsoTeamSkinAlt = 0;
						cg.ourModelBlue.legsTeamSkinAlt = 0;


					}
				}

				// here is where you do it
				if (*cg_teamHeadSkin.string  ||  *cg_teamTorsoSkin.string  ||  *cg_teamLegsSkin.string) {
					clientInfo_t *cio;

					//FIXME red/blue
					if (ci->team == TEAM_RED) {
						cio = &cg.ourModelRed;
					} else if (ci->team == TEAM_BLUE) {
						cio = &cg.ourModelBlue;
					} else {
						cio = &cg.ourModel;
					}

					if ((!cio->headTeamSkinAlt  &&  *cg_teamHeadSkin.string)  ||
						(!cio->torsoTeamSkinAlt  &&  *cg_teamTorsoSkin.string)  ||
						(!cio->legsTeamSkinAlt  &&  *cg_teamLegsSkin.string)
						) {
						const clientInfo_t *origCi = &cgs.clientinfoOrig[cent->currentState.clientNum];
						//FIXME hack for changing model and headmodel name
						// temporarily
						char origModelName[MAX_QPATH];
						char origHeadModelName[MAX_QPATH];

						Q_strncpyz(origModelName, cio->modelName, sizeof(origModelName));
						Q_strncpyz(origHeadModelName, cio->headModelName, sizeof(origHeadModelName));

						// hack to use original values as defaults
						if (usingFallbackTeamModel) {
							if (*cg_fallbackModel.string) {
								Q_strncpyz(cio->modelName, cg.fallbackModel.modelName, sizeof(cio->modelName));
								//Com_Printf("^2model %s\n", cio->modelName);
							}

							if (*cg_fallbackHeadModel.string) {
								Q_strncpyz(cio->headModelName, cg.fallbackModel.headModelName, sizeof(cio->headModelName));
								//Com_Printf("^2headmodel %s\n", cio->headModelName);
							}
						} else {
							if (!*cg_ourModel.string) {
								Q_strncpyz(cio->modelName, origCi->modelName, sizeof(cio->modelName));
							}

							if (!*cg_ourHeadModel.string) {
								Q_strncpyz(cio->headModelName, origCi->headModelName, sizeof(cio->headModelName));
							}
						}

						CG_LoadAltTeamSkins(cio, qfalse);
						//Com_Printf("    ^5loaded alt team skins for '%s'\n", ci->name);

						// don't modify [head]model name
						Q_strncpyz(cio->modelName, origModelName, sizeof(cio->modelName));
						Q_strncpyz(cio->headModelName, origHeadModelName, sizeof(cio->headModelName));
					} // end alt skin set

					if ((cio->headTeamSkinAlt != 0  &&  cio->headTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamHeadSkin.string) {
						ci->headSkin = cio->headTeamSkinAlt;
					}
					if ((cio->torsoTeamSkinAlt != 0  &&  cio->torsoTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamTorsoSkin.string) {
						ci->torsoSkin = cio->torsoTeamSkinAlt;
					}
					if ((cio->legsTeamSkinAlt != 0  &&  cio->legsTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamLegsSkin.string) {
						ci->legsSkin = cio->legsTeamSkinAlt;
					}
					//Com_Printf("alt: '%s'\n", ci->name);
				}  // if *cg_team[legs|torso|head]skin.string
			} else {  // not team game
				// at this point cg_forcePovModel 1  (not 2)  is set
				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
				CG_CopyClientInfoModel(&cg.ourModel, ci);

				if ((cg_disallowEnemyModelForTeammates.integer  &&  cg_disallowEnemyModelForTeammates.integer != 2)  &&  ci->legsModel == cg.enemyModel.legsModel) {
					CG_CopyClientInfoModel(&cg.fallbackModel, ci);
				}
				//FIXME alt skins?  -- 2016-05-27  no that's team baseed
			}

			return;
		}  // force pov model  &&  freecamPovSettings == qfalse

		//Com_Printf("us: %d  %s %s\n", cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name, ci->name);

		if (cg.freecam  &&  CG_IsTeamGame(cgs.gametype)  &&  cg_freecam_useTeamSettings.integer) {
			if (*cg_teamModel.string  ||  *cg_teamHeadModel.string) {
				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
				if (cg.teamModelTeamSkinFound) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						CG_CopyClientInfoModel(&cg.teamModelRed, ci);
					} else {
						CG_CopyClientInfoModel(&cg.teamModelBlue, ci);
					}
				} else {
					CG_CopyClientInfoModel(&cg.teamModel, ci);
				}

				if (cg.teamModel.headModel) {
					if (cg.teamModelTeamHeadSkinFound) {
						if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
							ci->headSkin = cg.teamModelRed.headSkin;
						} else {
							ci->headSkin = cg.teamModelBlue.headSkin;
						}
					} else {
						ci->headSkin = cg.teamModel.headSkin;
					}
				}

				// cg_forcePovModel is 0 at this point
				if ((cg_disallowEnemyModelForTeammates.integer  &&  cg_disallowEnemyModelForTeammates.integer != 2)  &&  ci->legsModel == cg.enemyModel.legsModel) {
					usingFallbackTeamModel = qtrue;

					// team game
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						CG_CopyClientInfoModel(&cg.fallbackModelRed, ci);
					} else {
						CG_CopyClientInfoModel(&cg.fallbackModelBlue, ci);
					}
				}

				memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
				memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
				memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));
			} else {  // models not set
				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			}

			// here is where you do it
			if (*cg_teamHeadSkin.string  ||  *cg_teamTorsoSkin.string  ||  *cg_teamLegsSkin.string) {
				if ((!ci->headTeamSkinAlt  &&  *cg_teamHeadSkin.string)  ||
					(!ci->torsoTeamSkinAlt  &&  *cg_teamTorsoSkin.string)  ||
					(!ci->legsTeamSkinAlt  &&  *cg_teamLegsSkin.string)
					) {
					//FIXME might be using fallbackmodel
					if (usingFallbackTeamModel) {
						//FIXME hack for changing model and headmodel name temporarily
						char origModelName[MAX_QPATH];
						char origHeadModelName[MAX_QPATH];

						Q_strncpyz(origModelName, ci->modelName, sizeof(origModelName));
						Q_strncpyz(origHeadModelName, ci->headModelName, sizeof(origHeadModelName));
						if (*cg_fallbackModel.string) {
							Q_strncpyz(ci->modelName, cg.fallbackModel.modelName, sizeof(ci->modelName));
						}

						if (*cg_fallbackHeadModel.string) {
							Q_strncpyz(ci->headModelName, cg.fallbackModel.headModelName, sizeof(ci->headModelName));
						}

						CG_LoadAltTeamSkins(ci, qtrue);

						// don't modify [head]model name
						Q_strncpyz(ci->modelName, origModelName, sizeof(ci->modelName));
						Q_strncpyz(ci->headModelName, origHeadModelName, sizeof(ci->headModelName));
					} else {
						CG_LoadAltTeamSkins(ci, qtrue);
					}
				}
				if ((ci->headTeamSkinAlt != 0  &&  ci->headTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamHeadSkin.string) {
					ci->headSkin = ci->headTeamSkinAlt;
				}
				if ((ci->torsoTeamSkinAlt != 0  &&  ci->torsoTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamTorsoSkin.string) {
					ci->torsoSkin = ci->torsoTeamSkinAlt;
				}
				if ((ci->legsTeamSkinAlt != 0  &&  ci->legsTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamLegsSkin.string) {
					ci->legsSkin = ci->legsTeamSkinAlt;
				}
				memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
				memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
				memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));
			}
		} else {  // else !(cg.freecam  &&  CG_IsTeamGame(cgs.gametype)  &&  cg_freecam_useTeamSettings.integer)

			//FIXME use team settings or cg_our* for third person view?  or maybe change freecam check above to also check thirdperson?
			CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			//Com_Printf("us: %d  '%s'  legs:%d  enemyLegs:%d\n", cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name, ci->legsModel, cg.enemyModel.legsModel);

			if ((cg_disallowEnemyModelForTeammates.integer &&  cg_disallowEnemyModelForTeammates.integer != 2)  &&  ci->legsModel == cg.enemyModel.legsModel) {
				//Com_Printf("^6FIXME settting us to enemy model\n");
				// cg_forcePovModel is 0 at this point so we are using demo taker pov model
				if (CG_IsTeamGame(cgs.gametype)) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						CG_CopyClientInfoModel(&cg.fallbackModelRed, ci);
					} else {
						CG_CopyClientInfoModel(&cg.fallbackModelBlue, ci);
					}
				} else {
					CG_CopyClientInfoModel(&cg.fallbackModel, ci);
				}
			}
		}
	} else if (CG_IsEnemy(ci)) {
		//Com_Printf("enemy: %s\n", ci->name);
		if (EM_Loaded == 1) {

			// hack to allow original legs, torso, or head if
			// cg_enemyModel or cg_enemyHeadModel is set to ""
			CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			if (CG_IsTeamGame(cgs.gametype)  &&  cg.enemyModelTeamSkinFound) {
				if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
					CG_CopyClientInfoModel(&cg.enemyModelRed, ci);
				} else {
					CG_CopyClientInfoModel(&cg.enemyModelBlue, ci);
				}
			} else {
				CG_CopyClientInfoModel(&cg.enemyModel, ci);
			}

			if (cg.enemyModel.headModel) {
				if (CG_IsTeamGame(cgs.gametype)  &&  cg.enemyModelTeamHeadSkinFound) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						ci->headSkin = cg.enemyModelRed.headSkin;
					} else {
						ci->headSkin = cg.enemyModelBlue.headSkin;
					}
				} else {
					ci->headSkin = cg.enemyModel.headSkin;
				}
			}

			if (*cg_enemyHeadColor.string) {
				memcpy(head->shaderRGBA, &cg.enemyColors[0], sizeof(head->shaderRGBA));
			}
			if (*cg_enemyTorsoColor.string) {
				memcpy(torso->shaderRGBA, &cg.enemyColors[1], sizeof(torso->shaderRGBA));
			}
			if (*cg_enemyLegsColor.string) {
				memcpy(legs->shaderRGBA, &cg.enemyColors[2], sizeof(legs->shaderRGBA));
			}
		} else {
			CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
		}

		if (*cg_enemyHeadSkin.string  ||  *cg_enemyTorsoSkin.string  ||  *cg_enemyLegsSkin.string) {
			//Com_Printf("%s  %d %d %d\n", ci->name, ci->headEnemySkinAlt, ci->torsoEnemySkinAlt, ci->legsEnemySkinAlt);
			if ((!ci->headEnemySkinAlt  &&  *cg_enemyHeadSkin.string)  ||
				(!ci->torsoEnemySkinAlt  &&  *cg_enemyTorsoSkin.string)  ||
				(!ci->legsEnemySkinAlt  &&  *cg_enemyLegsSkin.string)
				) {
				CG_LoadAltEnemySkins(ci);
			}
			if ((ci->headEnemySkinAlt != 0  &&  ci->headEnemySkinAlt != SKIN_INVALID)  &&  *cg_enemyHeadSkin.string) {
				ci->headSkin = ci->headEnemySkinAlt;
			}
			if ((ci->torsoEnemySkinAlt != 0  &&  ci->torsoEnemySkinAlt != SKIN_INVALID)  &&  *cg_enemyTorsoSkin.string) {
				ci->torsoSkin = ci->torsoEnemySkinAlt;
			}
			if ((ci->legsEnemySkinAlt != 0  &&  ci->legsEnemySkinAlt != SKIN_INVALID)  &&  *cg_enemyLegsSkin.string) {
				ci->legsSkin = ci->legsEnemySkinAlt;
			}
			if (*cg_enemyHeadColor.string) {
				memcpy(head->shaderRGBA, &cg.enemyColors[0], sizeof(head->shaderRGBA));
			}
			if (*cg_enemyTorsoColor.string) {
				memcpy(torso->shaderRGBA, &cg.enemyColors[1], sizeof(torso->shaderRGBA));
			}
			if (*cg_enemyLegsColor.string) {
				memcpy(legs->shaderRGBA, &cg.enemyColors[2], sizeof(legs->shaderRGBA));
			}
		}
	} else if (CG_IsTeammate(ci)) {
		if (*cg_teamModel.string  ||  *cg_teamHeadModel.string) {
			CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			if (cg.teamModelTeamSkinFound) {
				if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
					CG_CopyClientInfoModel(&cg.teamModelRed, ci);
				} else {
					CG_CopyClientInfoModel(&cg.teamModelBlue, ci);
				}
			} else {
				CG_CopyClientInfoModel(&cg.teamModel, ci);
			}

			if (cg.teamModel.headModel) {
				if (cg.teamModelTeamHeadSkinFound) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						ci->headSkin = cg.teamModelRed.headSkin;
					} else {
						ci->headSkin = cg.teamModelBlue.headSkin;
					}
				} else {
					ci->headSkin = cg.teamModel.headSkin;
				}
			}

			if (cg.teamModel.legsModel == 0  &&  cg.enemyModel.legsModel  &&  cg.fallbackModel.legsModel) {  // using player's own models (cg.teamModel.legsModel == 0)
				if (cg_disallowEnemyModelForTeammates.integer  &&  ci->legsModel == cg.enemyModel.legsModel) {
					usingFallbackTeamModel = qtrue;
					//FIXME checK:   not using team skin check since ourModel uses different logic
					//Com_Printf("^6disallow model for '%s'\n", ci->name);

					if (1) {  //(cg.ourModelTeamSkinFound) {
						if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
							CG_CopyClientInfoModel(&cg.fallbackModelRed, ci);
						} else {
							CG_CopyClientInfoModel(&cg.fallbackModelBlue, ci);
						}
					} else {
						CG_CopyClientInfoModel(&cg.fallbackModel, ci);
					}
				}
			}


			memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
			memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
			memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));
		} else {  // team model and headmodel cvars not set

			memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
			memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
			memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));

			if (cg.enemyModel.legsModel  &&  cg.fallbackModel.legsModel  &&  cg_disallowEnemyModelForTeammates.integer  &&  cgs.clientinfoOrig[cent->currentState.clientNum].legsModel == cg.enemyModel.legsModel) {
				usingFallbackTeamModel = qtrue;
				//FIXME check:  not using team skin check since ourModel uses different logic
				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
				//Com_Printf("^6disallow model for '%s'\n", ci->name);
				if (1) {  //(cg.ourModelTeamSkinFound) {
					if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
						CG_CopyClientInfoModel(&cg.fallbackModelRed, ci);
					} else {
						CG_CopyClientInfoModel(&cg.fallbackModelBlue, ci);
					}
				} else {
					CG_CopyClientInfoModel(&cg.fallbackModel, ci);
				}
			} else {
				CG_CopyClientInfoModel(&cgs.clientinfoOrig[cent->currentState.clientNum], ci);
			}
		}  // done set teammate model if team model and headmodel cvars not set

		// here is where you do it
		if (*cg_teamHeadSkin.string  ||  *cg_teamTorsoSkin.string  ||  *cg_teamLegsSkin.string) {
			if ((!ci->headTeamSkinAlt  &&  *cg_teamHeadSkin.string)  ||
				(!ci->torsoTeamSkinAlt  &&  *cg_teamTorsoSkin.string)  ||
				(!ci->legsTeamSkinAlt  &&  *cg_teamLegsSkin.string)
				) {

				if (usingFallbackTeamModel) {
					//FIXME hack for changing model and headmodel name temporarily
					char origModelName[MAX_QPATH];
					char origHeadModelName[MAX_QPATH];

					Q_strncpyz(origModelName, ci->modelName, sizeof(origModelName));
					Q_strncpyz(origHeadModelName, ci->headModelName, sizeof(origHeadModelName));

					if (*cg_fallbackModel.string) {
						Q_strncpyz(ci->modelName, cg.fallbackModel.modelName, sizeof(ci->modelName));
					}

					if (*cg_fallbackHeadModel.string) {
						Q_strncpyz(ci->headModelName, cg.fallbackModel.headModelName, sizeof(ci->headModelName));
					}

					CG_LoadAltTeamSkins(ci, qtrue);

					// don't modify [head]model name
					Q_strncpyz(ci->modelName, origModelName, sizeof(ci->modelName));
					Q_strncpyz(ci->headModelName, origHeadModelName, sizeof(ci->headModelName));
				} else {
					CG_LoadAltTeamSkins(ci, qtrue);
				}
			}
			if ((ci->headTeamSkinAlt != 0  &&  ci->headTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamHeadSkin.string) {
				ci->headSkin = ci->headTeamSkinAlt;
			}
			if ((ci->torsoTeamSkinAlt != 0  &&  ci->torsoTeamSkinAlt != SKIN_INVALID)  &&  *cg_teamTorsoSkin.string) {
				ci->torsoSkin = ci->torsoTeamSkinAlt;
			}
			if ((ci->legsTeamSkinAlt != 0  &&  ci->legsTeamSkinAlt != SKIN_INVALID) &&  *cg_teamLegsSkin.string) {
				ci->legsSkin = ci->legsTeamSkinAlt;
			}
			memcpy(head->shaderRGBA, &cg.teamColors[0], sizeof(head->shaderRGBA));
			memcpy(torso->shaderRGBA, &cg.teamColors[1], sizeof(torso->shaderRGBA));
			memcpy(legs->shaderRGBA, &cg.teamColors[2], sizeof(legs->shaderRGBA));
		}
	}

 server_settings:

	if (cent == &cg.predictedPlayerEntity) {
		//return;
	}

	if (cg_playerModelAllowServerOverride.integer  &&  (*cgs.serverModelOverride  ||  *cgs.serverHeadModelOverride)) {
		if (ci->override) {
			return;
		}
		CG_CopyClientInfoModel(&cg.serverModel, ci);
		//FIXME hack to match ql
		if (!cg.serverModel.legsModel  &&  cg.serverModel.headModel  &&  !cgs.serverAllowCustomHead) {
#if 0
			ci->headModel = cgs.clientinfoOrig[cent->currentState.clientNum].headModel;
			ci->headSkin = cgs.clientinfoOrig[cent->currentState.clientNum].headSkin;
#endif
			ci->headModel = cgs.clientinfoOrig[cent->currentState.clientNum].headModelFallback;
			ci->headSkin = cgs.clientinfoOrig[cent->currentState.clientNum].headSkinFallback;
		}
		//return;
	}


	if (cg_entities[cent->currentState.clientNum].currentState.eFlags & EF_DEAD  ||  (cent->currentState.clientNum != cent->currentState.number &&  cent !=&cg.predictedPlayerEntity)) {
		//Com_Printf("%s  %d %d\n", cgs.clientinfo[cent->currentState.clientNum].name, cent->currentState.clientNum, cent->currentState.number);
		if (!CG_IsUs(ci)  &&  *cg_deadBodyColor.string) {
			SC_ByteVec3ColorFromCvar(head->shaderRGBA, &cg_deadBodyColor);
			head->shaderRGBA[3] = 255;
			SC_ByteVec3ColorFromCvar(torso->shaderRGBA, &cg_deadBodyColor);
			torso->shaderRGBA[3] = 255;
			SC_ByteVec3ColorFromCvar(legs->shaderRGBA, &cg_deadBodyColor);
			legs->shaderRGBA[3] = 255;
		}
	}

	if (ci->override) {
		return;
	}

	if (cgs.protocol == PROTOCOL_QL  &&  cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_playerModelAllowServerOverride.integer) {
		if (ci->team == TEAM_RED) {
			if (!Q_stricmp(ci->whiteName, "Infected Mastermind")) {
				CG_CopyClientInfoModel(&cgs.urielInfected, ci);
			} else {
				CG_CopyClientInfoModel(&cgs.bonesInfected, ci);
			}
			legs->customShader = cgs.media.gooShader;
			torso->customShader = cgs.media.gooShader;
			head->customShader = cgs.media.gooShader;
		}
	}

	if (cgs.gametype == GT_RACE  &&  !CG_IsUs(ci)) {
		if (cg_racePlayerShader.integer) {
			legs->customShader = cgs.media.noPlayerClipShader;
			torso->customShader = cgs.media.noPlayerClipShader;
			head->customShader = cgs.media.noPlayerClipShader;
		}
	} else if (cgs.gametype == GT_RACE  &&  CG_IsUs(ci)) {
		if (cg_racePlayerShader.integer == 2  ||  cg_racePlayerShader.integer == 4) {
			legs->customShader = cgs.media.noPlayerClipShader;
			torso->customShader = cgs.media.noPlayerClipShader;
			head->customShader = cgs.media.noPlayerClipShader;
		}
	}
}

#undef SKIN_INVALID

/*
===============
CG_Player
===============
*/
void CG_Player ( centity_t *cent ) {
	clientInfo_t	*ci;
	clientInfo_t	cinfo;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
#if 1  //def MPACK
	refEntity_t		skull;
	refEntity_t		powerup;
	int				t;
	float			c;
	float			angle;
	vec3_t			dir, angles;
#endif
	float scale;
	float offset;
	//vec3_t mins, maxs;

	if (cgs.gametype == GT_FREEZETAG  &&  cent->currentState.number < MAX_CLIENTS  &&  cent->currentState.clientNum == cg.snap->ps.clientNum  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0  &&  !CG_FreezeTagFrozen(cg.snap->ps.clientNum)) {
		return;
	}

	if (cg.demoSeeking) {
		VectorCopy(cent->lerpOrigin, cent->rawOrigin);
		VectorCopy(cent->lerpAngles, cent->rawAngles);

		cent->pe.legs.yawAngle = cent->rawAngles[YAW];
        cent->pe.legs.yawing = qfalse;
        cent->pe.legs.pitchAngle = 0;
        cent->pe.legs.pitching = qfalse;

		cent->pe.torso.yawAngle = cent->rawAngles[YAW];
        cent->pe.torso.yawing = qfalse;
        cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
        cent->pe.torso.pitching = qfalse;

		// lerplerplerp
		//Com_Printf("lerp\n");
	}

	//FIXME not here
	clientNum = cent->currentState.number;

	if (clientNum >= 0  &&  clientNum < MAX_CLIENTS) {
		wclient_t *wc;

		wc = &wclients[clientNum];

		if (cent->currentState.eFlags & EF_FIRING) {
			wclients[clientNum].fireWeaponPressedTime = cg.time;
			wclients[clientNum].fireWeaponPressedClearedStats = qfalse;
		} else {
			if (cg_perKillStatsClearNotFiringTime.integer  &&  cg.time - wclients[clientNum].fireWeaponPressedTime > cg_perKillStatsClearNotFiringTime.integer  &&  wclients[clientNum].fireWeaponPressedClearedStats == qfalse) {
				if (cg_perKillStatsClearNotFiringExcludeSingleClickWeapons.integer) {
					if (clientNum == cg.snap->ps.clientNum) {
						//CG_Printf("^3cleared not firing excluded click %d\n", cg.time);
					}
					memset(&wc->perKillwstats[WP_MACHINEGUN], 0, sizeof(wweaponStats_t));
					memset(&wc->perKillwstats[WP_LIGHTNING], 0, sizeof(wweaponStats_t));
					memset(&wc->perKillwstats[WP_PLASMAGUN], 0, sizeof(wweaponStats_t));
				} else {
					memset(&wclients[clientNum].perKillwstats, 0, sizeof(wclients[clientNum].perKillwstats));
				}
				wclients[clientNum].fireWeaponPressedClearedStats = qtrue;
				if (clientNum == cg.snap->ps.clientNum) {
					//Com_Printf("^3cleared not firing %d\n", cg.time);
				}
			}
		}
		if (wclients[clientNum].needToClearPerKillStats  &&  !(cent->currentState.eFlags & EF_FIRING)) {
			if (cg_perKillStatsExcludePostKillSpam.integer) {
				memset(&wclients[clientNum].perKillwstats, 0, sizeof(wclients[clientNum].perKillwstats));
			}
			wclients[clientNum].needToClearPerKillStats = qfalse;
			if (clientNum == cg.snap->ps.clientNum) {
				//Com_Printf("cleared per kill stats for ps\n");
			}
		}
	}

	if (cent == &cg.predictedPlayerEntity) {
		if (cg.freezeEntity[cg.snap->ps.clientNum]) {
			memcpy(cent, &cg.freezeCent[cg.snap->ps.clientNum], sizeof(*cent));
		}
	} else {
		if (cg.freezeEntity[cent - cg_entities]) {
			memcpy(cent, &cg.freezeCent[cent->currentState.number], sizeof (*cent));
		}
	}

	//FIXME hack so that players don't flash over corpse when they respawn
	//CG_Printf("legsanim %d  %d\n", cent->currentState.number, cent->currentState.legsAnim & ~ANIM_TOGGLEBIT);

#if 0
	if (!(cent->currentState.eFlags & EF_DEAD)  &&  cent->currentState.number == cent->currentState.clientNum) {
		switch (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) {
		case BOTH_DEAD1:
		case BOTH_DEAD2:
		case BOTH_DEAD3:
			CG_Printf("death anim %d\n", cent->currentState.number);
			return;
		default:
			//
			//CG_Printf("ok %d\n", cent->currentState.number);
			break;
		}
	}
#endif
	if (!(cent->currentState.eFlags & EF_DEAD)  &&  (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) <= 5) {
		//CG_Printf("teleporting %d\n", cent->currentState.number);
		//return;
	}

	if (!cent->interpolate) {
		//if (cent->currentState.number != 3) {  //FIXME testing
			//CG_Printf("!cent->interpolate %d  %d  %s\n", cent->currentState.number, cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name);
			//return;
		//}
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}


	//FIXME hack so you don't draw demo taker in third person when following in ca
	if (cgs.gametype == GT_CA  &&  cent->currentState.number == cg.clientNum  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
		return;
	}


	if (cent->currentState.number == cent->currentState.clientNum  &&  cent->currentState.weapon == WP_NONE  &&  !(cent->currentState.eFlags & EF_DEAD)) {
		//return;
	}
	//if (cent->currentState.eFlags
	if (cent->currentState.number != cent->currentState.clientNum) {
		//return;  // uh ok dude
	}

#if 0
	if (cent->currentState.eFlags & EF_NODRAW) {
		//CG_Printf("EF_NODRAW %d  %d %s\n", cent->currentState.number, cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name);
		return;
	}
#endif
	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity");
	}
	memcpy(&cinfo, &cgs.clientinfo[clientNum], sizeof(clientInfo_t));
	ci = &cgs.clientinfo[ clientNum ];
	//ci = &cinfo;
	//Com_Printf("%d %d\n", clientNum, ci->team);

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		//Com_Printf("info invalid for %d\n", clientNum);
		return;
	}

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

	//if (wolfcam_following  &&  clientNum == wcg.clientNum) {
	//	return;
	//}


	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );

	CG_CheckForModelChange(cent, ci, &legs, &torso, &head);

	if (cent->currentState.eFlags & EF_NODRAW) {
		//CG_Printf("EF_NODRAW %d  %d %s\n", cent->currentState.number, cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name);
		return;
	}

	if (!wolfcam_following  &&  clientNum == cg.snap->ps.clientNum  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0  &&  cg_deathShowOwnCorpse.integer == 0) {
		return;
	}

    if (wolfcam_following) {  //FIXME check that this is ok
		//if (clientNum == wcg.clientNum) {
		//	return;
		//}

		if (wolfcam_following  &&  cent->currentState.number == wcg.clientNum) {
			return;
		}

		//FIXME other gametypes?
		if (cgs.gametype == GT_CA  &&  clientNum == cg.snap->ps.clientNum  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
			//CG_Printf("returning\n");
			//return;
		}

        if (cent->currentState.eFlags & EF_NODRAW) {
#if 0
			CG_Printf ("not drawing client num %d  %s\n", clientNum,
					   cgs.clientinfo[clientNum].name);
#endif
			//if (clientNum == wcg.clientNum)
			//return;
            return;
			//cent->currentState.eFlags |= EF_NODRAW;
		}
    }

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) {
		if ( cg.snap->ps.stats[STAT_HEALTH] <= GIB_HEALTH  &&  cgs.gametype != GT_FREEZETAG) {
			return;
			//Com_Printf("health: %d\n",  cg.snap->ps.stats[STAT_HEALTH]);
		}

		//Com_Printf("demo\n");
		//if (!cg.renderingThirdPerson) {
		if (CG_IsFirstPersonView(cg.snap->ps.clientNum)) {
			renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		} else {
			if (cg_cameraMode.integer) {
				return;
			}
		}
	}

	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp, &torso.oldframe, &torso.frame, &torso.backlerp );

	// add the talk balloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_allowServerOverride.integer) {
		if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
			CG_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.nightmareSound);
		}
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( cg_shadows.integer == 3  &&  shadow  &&  !SC_Cvar_Get_Int("r_debugMarkSurface")) {
		renderfx |= RF_SHADOW_PLANE;
	}

#if 0
	if (SC_Cvar_Get_Int("playerlight")) {
	} else {
		renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	}
#endif

	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

#if 1  //def MPACK
	if( cgs.gametype == GT_HARVESTER ) {
		CG_PlayerTokens( cent, renderfx );
	}
#endif
	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

#if 0
	{
		const char *s;

		s = SC_Cvar_Get_String("cg_skin");
		if (*s) {
			legs.customShader = trap_R_RegisterShaderNoMip(s);  //cgs.media.balloonShader;
			legs.shaderRGBA[0] = 255;
			legs.shaderRGBA[1] = 255;
			legs.shaderRGBA[2] = 255;
			legs.shaderRGBA[3] = 255;
			legs.customSkin = 0;
		}
	}
#endif

	VectorCopy(cent->lerpOrigin, legs.origin);

	//trap_R_ModelBounds(legs.hModel, mins, maxs);

	scale = 1.0;

	if (*cg_playerModelAutoScaleHeight.string) {
		//scale *= (cg_playerModelAutoScaleHeight.value / ci->playerModelHeight);
		scale = 1.0 * (cg_playerModelAutoScaleHeight.value / ci->playerModelHeight);
	}
	if (ci->hasModelAutoScale) {  // override
		scale = 1.0 * (ci->modelAutoScale / ci->playerModelHeight);
	}

	if (cg_playerModelAllowServerScale.integer) {
		if (cgs.serverHaveCustomModelString) {
			scale *= cgs.serverModelScale;
		} else {
			scale *= cg_playerModelAllowServerScaleDefault.value;
		}
	}

	if (ci->hasModelScale  ||  ci->hasLegsModelScale) {  // override
		if (ci->hasModelScale) {
			scale *= ci->modelScale;
		}
		if (ci->hasLegsModelScale) {
			scale *= ci->legsModelScale;
		}
	} else {
		if (*cg_playerModelForceScale.string  &&  cg_playerModelForceScale.value > 0.0) {
			scale *= cg_playerModelForceScale.value;
		}

		if (*cg_playerModelForceLegsScale.string  &&  cg_playerModelForceLegsScale.value > 0.0) {
			scale *= cg_playerModelForceLegsScale.value;
		}
	}

	if (scale <= 0.0) {
		scale = 1.0;
	}

	if (scale != 1.0) {
		float height;

		CG_ScaleModel(&legs, scale);

		height = fabs(bg_playerMins[2]);;  //DEFAULT_VIEWHEIGHT;
		legs.origin[2] -= height;
		legs.origin[2] += height * scale;
	}

	VectorCopy(legs.origin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	if (*EffectScripts.playerLegsTrail) {
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		//CG_CopyStaticCentDataToScript(cent);

		ScriptVars.animFrame = legs.frame;

		VectorCopy(legs.axis[0], ScriptVars.axis[0]);
		VectorCopy(legs.axis[1], ScriptVars.axis[1]);
		VectorCopy(legs.axis[2], ScriptVars.axis[2]);

		VectorCopy(legs.origin, ScriptVars.origin);

		VectorCopy(cent->lastLegsIntervalPosition, ScriptVars.lastIntervalPosition);
        ScriptVars.lastIntervalTime = cent->lastLegsIntervalTime;
        VectorCopy(cent->lastLegsDistancePosition, ScriptVars.lastDistancePosition);
        ScriptVars.lastDistanceTime = cent->lastLegsDistanceTime;

		CG_RunQ3mmeScript(EffectScripts.playerLegsTrail, NULL);
		//CG_CopyStaticScriptDataToCent(cent);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastLegsIntervalPosition);
        cent->lastLegsIntervalTime = ScriptVars.lastIntervalTime;
        VectorCopy(ScriptVars.lastDistancePosition, cent->lastLegsDistancePosition);
        cent->lastLegsDistanceTime = ScriptVars.lastDistanceTime;

	}


	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );
	//CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );

	if (cg.ftime < cent->extraShaderEndTime  &&  cent->extraShader) {
		legs.customShader = cent->extraShader;
		CG_AddRefEntity(&legs);
	}
	Wolfcam_AddBoundingBox(cent);

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	//VectorCopy( cent->lerpOrigin, torso.lightingOrigin );  //FIXME scaled??
	VectorCopy(legs.origin, torso.lightingOrigin);

	scale = 1.0;

	if (ci->hasTorsoModelScale) {  // override
		scale = ci->torsoModelScale;
	} else if (*cg_playerModelForceTorsoScale.string  &&  cg_playerModelForceTorsoScale.value > 0.0) {
		scale = cg_playerModelForceTorsoScale.value;
	}

	if (scale <= 0.0) {
		scale = 1.0;
	}

	if (scale != 1.0) {
		CG_ScaleModel(&torso, scale);
	}

	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	if (*EffectScripts.playerTorsoTrail) {
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		//CG_CopyStaticCentDataToScript(cent);

		//CG_Printf("torso size:  %f\n", scale);
		//ScriptVars.size = scale;
		ScriptVars.animFrame = torso.frame;

		VectorCopy(torso.axis[0], ScriptVars.axis[0]);
		VectorCopy(torso.axis[1], ScriptVars.axis[1]);
		VectorCopy(torso.axis[2], ScriptVars.axis[2]);

		VectorCopy(torso.origin, ScriptVars.origin);

		//Com_Printf("torsotime %f\n", cent->lastTorsoIntervalTime);

		VectorCopy(cent->lastTorsoIntervalPosition, ScriptVars.lastIntervalPosition);
        ScriptVars.lastIntervalTime = cent->lastTorsoIntervalTime;
        VectorCopy(cent->lastTorsoDistancePosition, ScriptVars.lastDistancePosition);
        ScriptVars.lastDistanceTime = cent->lastTorsoDistanceTime;


		CG_RunQ3mmeScript(EffectScripts.playerTorsoTrail, NULL);
		//CG_CopyStaticScriptDataToCent(cent);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastTorsoIntervalPosition);
        cent->lastTorsoIntervalTime = ScriptVars.lastIntervalTime;
        VectorCopy(ScriptVars.lastDistancePosition, cent->lastTorsoDistancePosition);
        cent->lastTorsoDistanceTime = ScriptVars.lastDistanceTime;

	}

	CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team );
	if (cg.ftime < cent->extraShaderEndTime  &&  cent->extraShader) {
		torso.customShader = cent->extraShader;
		CG_AddRefEntity(&torso);
	}

	if (!cg_inheritPowerupShader.integer) {
		torso.customShader = 0;
	}

#if 1  //def MPACK
	if ( cent->currentState.eFlags & EF_KAMIKAZE ) {

		memset( &skull, 0, sizeof(skull) );

		//VectorCopy( cent->lerpOrigin, skull.lightingOrigin );
		VectorCopy(legs.origin, skull.lightingOrigin);

		skull.shadowPlane = shadowPlane;
		skull.renderfx = renderfx;

		if ( cent->currentState.eFlags & EF_DEAD ) {
			// one skull bobbing above the dead body
			angle = ((cg.time / 7) & 255) * (M_PI * 2) / 255;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[2] = 15 + sin(angle) * 8;
			VectorAdd(torso.origin, dir, skull.origin);

			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			CG_AddRefEntity(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			CG_AddRefEntity(&skull);
		}
		else {
			// three skulls spinning around the player
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorInverse(dir);
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			CG_AddRefEntity(&skull);
			// flip the trail because this skull is spinning in the other direction
			VectorInverse(skull.axis[1]);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			CG_AddRefEntity(&skull);

			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255 + M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = cos(angle - 0.5 * M_PI) * 30;
			angles[1] = 360 - (angle * 180 / M_PI);
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			CG_AddRefEntity(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			CG_AddRefEntity(&skull);

			angle = ((cg.time / 3) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = 0;
			VectorAdd(torso.origin, dir, skull.origin);

			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			CG_AddRefEntity(&skull);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			CG_AddRefEntity(&skull);
		}
	}

	if ( cent->currentState.powerups & ( 1 << PW_GUARD ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		CG_AddRefEntity(&powerup);
	}
	if ( cent->currentState.powerups & ( 1 << PW_SCOUT ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.scoutPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		CG_AddRefEntity(&powerup);
	}
	if ( cent->currentState.powerups & ( 1 << PW_DOUBLER ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.doublerPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		CG_AddRefEntity(&powerup);
	}
	if ( cent->currentState.powerups & ( 1 << PW_ARMORREGEN ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.armorRegenPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		CG_AddRefEntity(&powerup);
	}
	if ( cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) {
		if ( !ci->invulnerabilityStartTime ) {
			ci->invulnerabilityStartTime = cg.time;
		}
		ci->invulnerabilityStopTime = cg.time;
	}
	else {
		ci->invulnerabilityStartTime = 0;
		ci->invulnerabilityStopTime = 0;
	}
	if ( (cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) ||
		cg.time - ci->invulnerabilityStopTime < 250 ) {

		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.invulnerabilityPowerupModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		//VectorCopy(cent->lerpOrigin, powerup.origin);
		VectorCopy(legs.origin, powerup.origin);

		if ( cg.time - ci->invulnerabilityStartTime < 250 ) {
			c = (float) (cg.time - ci->invulnerabilityStartTime) / 250;
		}
		else if (cg.time - ci->invulnerabilityStopTime < 250 ) {
			c = (float) (250 - (cg.time - ci->invulnerabilityStopTime)) / 250;
		}
		else {
			c = 1;
		}
		VectorSet( powerup.axis[0], c, 0, 0 );
		VectorSet( powerup.axis[1], 0, c, 0 );
		VectorSet( powerup.axis[2], 0, 0, c );
		CG_AddRefEntity(&powerup);
	}

	t = cg.time - ci->medkitUsageTime;
	if ( ci->medkitUsageTime && t < 500 ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorClear(angles);
		AnglesToAxis(angles, powerup.axis);
		//VectorCopy(cent->lerpOrigin, powerup.origin);
		VectorCopy(legs.origin, powerup.origin);

		powerup.origin[2] += -24 + (float) t * 80 / 500;
		if ( t > 400 ) {
			c = (float) (t - 1000) * 0xff / 100;
			powerup.shaderRGBA[0] = 0xff - c;
			powerup.shaderRGBA[1] = 0xff - c;
			powerup.shaderRGBA[2] = 0xff - c;
			powerup.shaderRGBA[3] = 0xff - c;
		}
		else {
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		CG_AddRefEntity(&powerup);
	}
#endif // MPACK

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;
	//Com_Printf("add head headskin: %d\n", ci->headSkin);

	//VectorCopy( cent->lerpOrigin, head.lightingOrigin );
	VectorCopy(legs.origin, head.lightingOrigin);

	scale = 1.0;

	if (ci->hasHeadModelScale) {
		scale = ci->headModelScale;
	} else {
		if (cg_playerModelAllowServerScale.integer  &&  cgs.serverHaveCustomModelString) {
			scale = cgs.serverHeadModelScale;
		}

		if (*cg_playerModelForceHeadScale.string  &&  cg_playerModelForceHeadScale.value > 0.0) {
			scale = cg_playerModelForceHeadScale.value;
		}
	}

	if (scale <= 0.0) {
		scale = 1.0;
	}

	if (scale != 1.0) {
		CG_ScaleModel(&head, scale);
	}

	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head");

	offset = 1.0;

	if (ci->hasHeadOffset) {
		offset = ci->oheadOffset;
	} else {
		if (*cg_playerModelForceHeadOffset.string) {
			offset = cg_playerModelForceHeadOffset.value;
		} else if (cg_playerModelAllowServerScale.integer  &&  cgs.serverHaveCustomModelString  &&  cgs.serverHaveHeadScaleOffset) {
			offset = cgs.serverHeadModelScaleOffset;
		}
	}

	if (offset != 1.0) {
		//refEntity_t torsoTmp;

		//memcpy(&torsoTmp, &torso, sizeof(torsoTmp));
		//CG_ScaleModel(&torsoTmp, 1.0);
		//CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head");
		//head.origin[2] = (head.origin[2] - legs.origin[2]) * offset;
		//head.origin[2] = legs.origin[2] + (head.origin[2] - legs.origin[2]) * offset;
		//head.origin[2] = cent->lerpOrigin[2] + (head.origin[2] - cent->lerpOrigin[2]) * offset;
		//head.origin[2] = torso.origin[2] + (head.origin[2] - torso.origin[2]) * offset;

		// whatever ... good enough
		//Com_Printf("offset %f\n", offset);
		head.origin[2] = (head.origin[2] - 24.0) + 24.0 * offset;
	}

#if 0  // testing
	{
		vec3_t mins, maxs;
		refEntity_t l, t, h;
		vec3_t angles = { 0, 0, 0 };
		int renderfx = RF_DEPTHHACK;

		memset(&l, 0, sizeof(l));
		memset(&t, 0, sizeof(t));
		memset(&h, 0, sizeof(h));

		l.hModel = ci->legsModel;
		t.hModel = ci->torsoModel;
		h.hModel = ci->headModel;

		l.renderfx = renderfx;
		t.renderfx = renderfx;
		h.renderfx = renderfx;

		//CG_PlayerAngles(cent, l.axis, t.axis, h.axis);
		AnglesToAxis(angles, l.axis);
		AnglesToAxis(angles, t.axis);
		AnglesToAxis(angles, h.axis);

		l.oldframe = l.frame = ci->animations[LEGS_IDLE].firstFrame;
		t.oldframe = t.frame = ci->animations[TORSO_STAND].firstFrame;

		//UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
		VectorCopy(cent->lerpOrigin, l.origin);
		VectorCopy(cent->lerpOrigin, l.oldorigin);
		//CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");
		CG_PositionEntityOnTag(&t, &l, l.hModel, "tag_torso");
		CG_PositionEntityOnTag(&h, &t, t.hModel, "tag_head");

		//VectorCopy( cent->lerpOrigin, legs.origin );
		trap_R_ModelBounds(h.hModel, mins, maxs);
		if (clientNum != cg.snap->ps.clientNum) {
			vec3_t origin;

			VectorCopy(h.origin, origin);
			origin[2] += maxs[2];
			//Com_Printf("%d  z:  %f  %f\n", clientNum, head.origin[2] + maxs[2], origin[2]);
			//Com_Printf("frame %d  oldframe %d\n", legs.frame, legs.oldframe);
#if 0
			CG_FloatNumber(6, origin, RF_DEPTHHACK, NULL);
			CG_AddRefEntity(&l);
			CG_AddRefEntity(&t);
			CG_AddRefEntity(&h);
#endif
		}
	}
#endif

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	if (*EffectScripts.playerHeadTrail) {
		CG_ResetScriptVars();
		CG_CopyPlayerDataToScriptData(cent);
		//ScriptVars.shader = head.customShader;
		ScriptVars.animFrame = head.frame;

		VectorCopy(head.axis[0], ScriptVars.axis[0]);
		VectorCopy(head.axis[1], ScriptVars.axis[1]);
		VectorCopy(head.axis[2], ScriptVars.axis[2]);

		//CG_CopyStaticCentDataToScript(cent);
		VectorCopy(head.origin, ScriptVars.origin);

		//Com_Printf("headtime %f\n", cent->lastHeadIntervalTime);

		VectorCopy(cent->lastHeadIntervalPosition, ScriptVars.lastIntervalPosition);
        ScriptVars.lastIntervalTime = cent->lastHeadIntervalTime;
        VectorCopy(cent->lastHeadDistancePosition, ScriptVars.lastDistancePosition);
        ScriptVars.lastDistanceTime = cent->lastHeadDistanceTime;


		CG_RunQ3mmeScript(EffectScripts.playerHeadTrail, NULL);
		//CG_CopyStaticScriptDataToCent(cent);

		VectorCopy(ScriptVars.lastIntervalPosition, cent->lastHeadIntervalPosition);
        cent->lastHeadIntervalTime = ScriptVars.lastIntervalTime;
        VectorCopy(ScriptVars.lastDistancePosition, cent->lastHeadDistancePosition);
        cent->lastHeadDistanceTime = ScriptVars.lastDistanceTime;
	}

	CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team );
	if (cg.ftime < cent->extraShaderEndTime  &&  cent->extraShader) {
		head.customShader = cent->extraShader;
		CG_AddRefEntity(&head);
	}

#if 1  //def MPACK
	CG_BreathPuffs(cent, &head);

	CG_DustTrail(cent);
#endif

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );

	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &torso );

	//CG_Printf("Drawing  %d  %d  %s\n", cent->currentState.number, cent->currentState.clientNum, cgs.clientinfo[cent->currentState.clientNum].name);
	//Wolfcam_AddBoundingBox (cent);
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity ( centity_t *cent ) {
	if (cg.demoSeeking) {
		//return;
	}

#if 0
	if (1) {  //(cent->currentState.number == cg.snap->ps.clientNum) {
		CG_Printf("CG_ResetPlayerEntity %d %d  %p<addr>\n", cent->currentState.number, cent->currentState.clientNum, cent);
	}
#endif

	if (cg_useOriginalInterpolation.integer == 0) {
		if (cg_freecam_useServerView.integer  &&  !wolfcam_following  &&  (cg.freecam  ||  cg_thirdPerson.integer)) {

			if (cg.demoSeeking) {
				//Com_Printf("...\n");
				return;
			}
			// pass, go ahead and reset
		} else {
			//Com_Printf("returning\n");
			return;
		}
	}

	//CG_Printf("CG_ResetPlayerEntity %d %d  %p<addr>\n", cent->currentState.number, cent->currentState.clientNum, cent);

#if 0
	if (cent->currentState.number == 6) {
		Com_Printf("reset\n");
	}
#endif

	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;

	//CG_Printf("ent %d clear lerp frame cg.time %d  %d\n", cent->currentState.number, cg.time, cg.time);
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim, cg.time );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim, cg.time );

	BG_EvaluateTrajectoryf(&cent->currentState.pos, cg.time, cent->lerpOrigin, cg.foverf);
	BG_EvaluateTrajectoryf(&cent->currentState.apos, cg.time, cent->lerpAngles, cg.foverf);

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	cent->lastLgFireFrameCount = 0;
	cent->lastLgImpactFrameCount = 0;
	cent->lastLgImpactTime = 0;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

qboolean CG_DuelPlayerScoreValid (int clientNum)
{
	if (clientNum == DUEL_PLAYER_INVALID) {
		return qfalse;
	}

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return qfalse;
	}

	if (!cg.scoresValid) {
		return qfalse;
	}

	//FIXME even for score?
	if (!cgs.clientinfo[clientNum].infoValid) {
		return qfalse;
	}

	return cg.clientHasScore[clientNum];
}

qboolean CG_DuelPlayerInfoValid (int clientNum)
{
	if (clientNum == DUEL_PLAYER_INVALID) {
		return qfalse;
	}

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return qfalse;
	}

	if (!cgs.clientinfo[clientNum].infoValid) {
		return qfalse;
	}

	return qtrue;
}

#include "cg_local.h"

#include "cg_players.h"
#include "cg_predict.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "sc.h"
#include "wolfcam_view.h"

#include "wolfcam_local.h"

static void RegisterClientModelnameWithFallback (clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName, qboolean dontForceTeamSkin)
{
	if (CG_RegisterClientModelname(ci, modelName, skinName, headModelName, headSkinName, "", dontForceTeamSkin)) {
			const char *dir, *fallback, *s;
			int i;

			cg.teamModel.newAnims = qfalse;
			if (cg.teamModel.torsoModel) {
				orientation_t tag;
				// if the torso model has the "tag_flag"
				if (trap_R_LerpTag(&tag, cg.teamModel.torsoModel, 0, 0, 1, "tag_flag")) {
					cg.teamModel.newAnims = qtrue;
				}
			}

			// sounds
			dir = cg.teamModel.modelName;
			fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

			for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
				s = cg_customSoundNames[i];
				if (!s) {
					break;
				}
				cg.teamModel.sounds[i] = 0;
				// if the model didn't load use the sounds of the default model
				cg.teamModel.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
				if (!cg.teamModel.sounds[i])
					cg.teamModel.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
			}
		}
}

void CG_GetModelName (const char *s, char *model)
{
	char *p;

	model[0] = '\0';

	Q_strncpyz(model, s, MAX_QPATH);
	if ((p = strchr(model, '/')) == NULL) {
		// just a model listed
		return;
	} else {
		*p++ = '\0';
	}
}

void CG_GetModelAndSkinName (const char *s, char *model, char *skin)
{
	char *p;

	model[0] = '\0';
	skin[0] = '\0';

	Q_strncpyz(model, s, MAX_QPATH);
	if ((p = strchr(model, '/')) == NULL) {
		model[0] = '\0';
		Q_strncpyz(skin, s, MAX_QPATH);
		return;
	} else {
		*p++ = '\0';
	}

	Q_strncpyz(skin, p, MAX_QPATH);
}

//FIXME cg.
static int TeamModel_modc = -1;

static void Wolfcam_LoadTeamModel (void)
{
	char modelStr[MAX_QPATH];
	//char modelName[MAX_QPATH];
	//char skinName[MAX_QPATH];
	char *skin;
	int i;
	//static int modc = -1;
	static int modchead = -1;
	static int modctorso = -1;
	static int modclegs = -1;
	static int modcheadcolor = -1;
	static int modctorsocolor = -1;
	static int modclegscolor = -1;

	if (cg_teamHeadColor.modificationCount != modcheadcolor  ||
		cg_teamTorsoColor.modificationCount != modctorsocolor  ||
		cg_teamLegsColor.modificationCount != modclegscolor
		) {
		SC_ByteVec3ColorFromCvar(cg.teamColors[0], &cg_teamHeadColor);
		cg.teamColors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.teamColors[1], &cg_teamTorsoColor);
		cg.teamColors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.teamColors[2], &cg_teamLegsColor);
		cg.teamColors[2][3] = 255;
		modcheadcolor = cg_teamHeadColor.modificationCount;
		modctorsocolor = cg_teamTorsoColor.modificationCount;
		modclegscolor = cg_teamLegsColor.modificationCount;
		//EC_Loaded = 1;
	}

	if (cg_teamModel.modificationCount != TeamModel_modc  ||
		cg_teamHeadSkin.modificationCount != modchead  ||
		cg_teamTorsoSkin.modificationCount != modctorso  ||
		cg_teamLegsSkin.modificationCount != modclegs

) {
		//EM_Loaded = 0;
		TeamModel_modc = cg_teamModel.modificationCount;
		modchead = cg_teamHeadSkin.modificationCount;
		modctorso = cg_teamTorsoSkin.modificationCount;
		modclegs = cg_teamLegsSkin.modificationCount;
	} else {
		return;
	}

	Com_Printf("loading team models\n");

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		clientInfo_t *ci;

		ci = &cgs.clientinfo[i];
		ci->headSkinAlt = 0;
		ci->torsoSkinAlt = 0;
		ci->legsSkinAlt = 0;
	}

	//if (cg_teamModel.v.string && *cg_teamModel.v.string) {
	//if (*cg_teamModel.string  ||  *cg_teamHeadSkin.string  ||  *cg_teamTorsoSkin.string  ||  *cg_teamLegsSkin.string) {
	if (*cg_teamModel.string) {
		Q_strncpyz (modelStr, cg_teamModel.string, sizeof(modelStr));
	} else {
		//FIXME changes in cg_ourModel
		// use this for black listed enemy model as teammate
		Q_strncpyz(modelStr, cg_ourModel.string, sizeof(modelStr));
	}

		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
			cg.useTeamSkins = qtrue;
		} else {
			*skin++ = 0;
			cg.useTeamSkins = qfalse;
		}

		Q_strncpyz(cg.teamModel.modelName, modelStr, sizeof(cg.teamModel.modelName));
		Q_strncpyz(cg.teamModel.skinName, skin, sizeof(cg.teamModel.skinName));
		Q_strncpyz(cg.teamModelRed.modelName, modelStr, sizeof(cg.teamModelRed.modelName));
		Q_strncpyz(cg.teamModelRed.skinName, skin, sizeof(cg.teamModelRed.skinName));
		Q_strncpyz(cg.teamModelBlue.modelName, modelStr, sizeof(cg.teamModelBlue.modelName));
		Q_strncpyz(cg.teamModelBlue.skinName, skin, sizeof(cg.teamModelBlue.skinName));


		Q_strncpyz(cg.teamModel.headModelName, cg.teamModel.modelName, sizeof(cg.teamModel.headModelName));
		Q_strncpyz(cg.teamModel.headSkinName, cg.teamModel.skinName, sizeof(cg.teamModel.headSkinName));
		Q_strncpyz(cg.teamModelRed.headModelName, cg.teamModelRed.modelName, sizeof(cg.teamModelRed.headModelName));
		Q_strncpyz(cg.teamModelRed.headSkinName, cg.teamModelRed.skinName, sizeof(cg.teamModelRed.headSkinName));
		Q_strncpyz(cg.teamModelBlue.headModelName, cg.teamModelBlue.modelName, sizeof(cg.teamModelBlue.headModelName));
		Q_strncpyz(cg.teamModelBlue.headSkinName, cg.teamModelBlue.skinName, sizeof(cg.teamModelBlue.headSkinName));

		//cg.teamModel.team = TEAM_BLUE;
		//if (CG_RegisterClientModelname(&cg.teamModel, cg.teamModel.modelName, cg.teamModel.skinName, cg.teamModel.headModelName, cg.teamModel.headSkinName, "", useDefaultTeamSkins ? qfalse : qtrue)) {

		RegisterClientModelnameWithFallback(&cg.teamModel, cg.teamModel.modelName, cg.teamModel.skinName, cg.teamModel.headModelName, cg.teamModel.headSkinName, "", qtrue);

		cg.teamModelRed.team = TEAM_RED;
		RegisterClientModelnameWithFallback(&cg.teamModelRed, cg.teamModelRed.modelName, cg.teamModelRed.skinName, cg.teamModelRed.headModelName, cg.teamModelRed.headSkinName, "", qfalse);

		cg.teamModelBlue.team = TEAM_BLUE;
		RegisterClientModelnameWithFallback(&cg.teamModelBlue, cg.teamModelBlue.modelName, cg.teamModelBlue.skinName, cg.teamModelBlue.headModelName, cg.teamModelBlue.headSkinName, "", qfalse);

	if (*cg_teamHeadSkin.string  ||  *cg_teamTorsoSkin.string  ||  *cg_teamLegsSkin.string) {
		Com_Printf("reseting altskins\n");

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			clientInfo_t *ci;

			ci = &cgs.clientinfo[i];
			ci->headSkinAlt = 0;
			ci->torsoSkinAlt = 0;
			ci->legsSkinAlt = 0;
		}
	}
}

static void Wolfcam_LoadOurModel (void)
{
	char modelStr[MAX_QPATH];
	//char modelName[MAX_QPATH];
	//char skinName[MAX_QPATH];
	char *skin;
	const char *dir;
	const char *s;
	int i;
	static int modc = -1;
	//static int modchead = -1;

	if (cg_ourModel.modificationCount != modc) {
		modc = cg_ourModel.modificationCount;
		//FIXME hack for black listing enemy model for teammates
		if (!*cg_teamModel.string) {
			TeamModel_modc = -1;
		}
	} else {
		return;
	}

	Com_Printf("loading our model\n");

	if (*cg_ourModel.string) {
		Q_strncpyz (modelStr, cg_ourModel.string, sizeof(modelStr));
		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
			cg.useTeamSkins = qtrue;
		} else {
			*skin++ = 0;
			cg.useTeamSkins = qfalse;
		}

		Q_strncpyz(cg.ourModel.modelName, modelStr, sizeof(cg.ourModel.modelName));
		Q_strncpyz(cg.ourModel.skinName, skin, sizeof(cg.ourModel.skinName));

		//FIXME hmodel
		Q_strncpyz(cg.ourModel.headModelName, cg.ourModel.modelName, sizeof(cg.ourModel.headModelName));
		Q_strncpyz(cg.ourModel.headSkinName, cg.ourModel.skinName, sizeof(cg.ourModel.headSkinName));

		//RegisterClientModelnameWithFallback(&cg.teamModel, cg.teamModel.modelName, cg.teamModel.skinName, cg.teamModel.headModelName, cg.teamModel.headSkinName, "", qtrue);

		CG_RegisterClientModelname(&cg.ourModel, cg.ourModel.modelName, cg.ourModel.skinName, cg.ourModel.headModelName, cg.ourModel.headSkinName, "", qtrue);

		// sounds
		dir = cg.ourModel.modelName;
		//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

		for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
			s = cg_customSoundNames[i];
			if (!s) {
				break;
			}
			cg.ourModel.sounds[i] = 0;

			// if the model didn't load use the sounds of the default model
			cg.ourModel.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
			//if (!EM_ModelInfo.sounds[i]) {
			//EM_ModelInfo.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
			//}
		}
	}
}

//FIXME testing
//static char enemySkin[MAX_QPATH];

void Wolfcam_LoadModels (void)
{
	char modelStr[MAX_QPATH];
	//char tmpStr[MAX_QPATH];
	//char skinStr[MAX_QPATH];
	char *skin;
	int i;
	//FIXME modification count can't be changed by cgame?
	static int modc = -1;
	static int modchead = -1;
	static int modctorso = -1;
	static int modclegs = -1;
	static int modcheadcolor = -1;
	static int modctorsocolor = -1;
	static int modclegscolor = -1;
	qboolean checkSkins;

	Wolfcam_LoadTeamModel();
	Wolfcam_LoadOurModel();

	checkSkins = qfalse;

	//if (cg_enemyModel.modificationCount) {
	if (cg_enemyModel.modificationCount != modc  ||
		cg_enemyHeadSkin.modificationCount != modchead  ||
		cg_enemyTorsoSkin.modificationCount != modctorso  ||
		cg_enemyLegsSkin.modificationCount != modclegs

) {
		EM_Loaded = 0;
		modc = cg_enemyModel.modificationCount;
		modchead = cg_enemyHeadSkin.modificationCount;
		modctorso = cg_enemyTorsoSkin.modificationCount;
		modclegs = cg_enemyLegsSkin.modificationCount;
		checkSkins = qtrue;
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			clientInfo_t *ci;

			ci = &cgs.clientinfo[i];
			ci->headEnemySkinAlt = 0;
			ci->torsoEnemySkinAlt = 0;
			ci->legsEnemySkinAlt = 0;
		}
	}

	if (EM_Loaded == 0 && cg_enemyModel.string && *cg_enemyModel.string) {
		Q_strncpyz (modelStr, cg_enemyModel.string, sizeof(modelStr));
		if ((skin = strchr(modelStr, '/')) == NULL) {
			//Com_Printf("using default\n");
			skin = "default";
		} else
			*skin++ = 0;

		//memcpy(&EM_ModelInfo, ci, sizeof(EM_ModelInfo));

		Q_strncpyz(EM_ModelInfo.modelName, modelStr, sizeof(EM_ModelInfo.modelName));
		Q_strncpyz(EM_ModelInfo.skinName, skin, sizeof(EM_ModelInfo.skinName));

		if (cgs.gametype >= GT_TEAM  &&  Q_stricmp(EM_ModelInfo.skinName, "pm") != 0) {
			//FIXME why is this here
			//Q_strncpyz(EM_ModelInfo.skinName, "default", sizeof(EM_ModelI\nfo.skinName));
		}

		Q_strncpyz(EM_ModelInfo.headModelName, EM_ModelInfo.modelName, sizeof(EM_ModelInfo.headModelName));
		Q_strncpyz(EM_ModelInfo.headSkinName, EM_ModelInfo.skinName, sizeof(EM_ModelInfo.headSkinName));

		if (CG_RegisterClientModelname(&EM_ModelInfo, EM_ModelInfo.modelName, EM_ModelInfo.skinName, EM_ModelInfo.headModelName, EM_ModelInfo.headSkinName, "", qtrue)) {
			const char *dir, *fallback, *s;

			//char modelName[MAX_QPATH];
			//char skinName[MAX_QPATH];

			EM_Loaded = 1;
			EM_ModelInfo.newAnims = qfalse;
			if (EM_ModelInfo.torsoModel) {
				orientation_t tag;
				// if the torso model has the "tag_flag"
				if (trap_R_LerpTag(&tag, EM_ModelInfo.torsoModel, 0, 0, 1, "tag_flag")) {
					EM_ModelInfo.newAnims = qtrue;
				}
			}

			//FIXME not here
#if 0
			if (cg_enemyHeadSkin.string  &&  *cg_enemyHeadSkin.string) {
				CG_GetModelAndSkinName(cg_enemyHeadSkin.string, modelName, skinName);
				if (!*modelName) {
					Q_strncpyz(modelName, modelStr, sizeof(modelName));
				}

				EM_ModelInfo.headSkin = CG_RegisterSkinVertexLight(va("models/players/%s/head_%s.skin", modelName, skinName));
				if (!EM_ModelInfo.headSkin) {
					Com_Printf("couldn't load head skin '%s %s'\n", modelName, skinName);
				}
			}
			if (cg_enemyTorsoSkin.string  &&  *cg_enemyTorsoSkin.string) {
				CG_GetModelAndSkinName(cg_enemyTorsoSkin.string, modelName, skinName);
				if (!*modelName) {
					Q_strncpyz(modelName, modelStr, sizeof(modelName));
				}

				EM_ModelInfo.torsoSkin = CG_RegisterSkinVertexLight(va("models/players/%s/upper_%s.skin", modelName, skinName));
				if (!EM_ModelInfo.torsoSkin) {
					Com_Printf("couldn't load torso skin '%s %s'\n", modelName, skinName);
				}
			}
			if (cg_enemyLegsSkin.string  &&  *cg_enemyLegsSkin.string) {
				CG_GetModelAndSkinName(cg_enemyLegsSkin.string, modelName, skinName);
				if (!*modelName) {
					Q_strncpyz(modelName, modelStr, sizeof(modelName));
				}

				Com_Printf("legs model: %s  skin: %s\n", modelName, skinName);
				EM_ModelInfo.legsSkin = CG_RegisterSkinVertexLight(va("models/players/%s/lower_%s.skin", modelName, skinName));
				if (!EM_ModelInfo.legsSkin) {
					Com_Printf("couldn't load legs skin '%s %s'\n", modelName, skinName);
				}
			}
#endif
			// sounds
			dir = EM_ModelInfo.modelName;
			fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

			for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
				s = cg_customSoundNames[i];
				if (!s) {
					break;
				}
				EM_ModelInfo.sounds[i] = 0;
				// if the model didn't load use the sounds of the default model
				EM_ModelInfo.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
				if (!EM_ModelInfo.sounds[i])
					EM_ModelInfo.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
			}
		}
		else
			EM_Loaded = -1;
	} else if (EM_Loaded == 0) {
		// request to switch back to default models
		//Com_Printf("back to old models\n");
		EM_Loaded = -1;
	}

	if (cg_enemyHeadColor.modificationCount != modcheadcolor  ||
		cg_enemyTorsoColor.modificationCount != modctorsocolor  ||
		cg_enemyLegsColor.modificationCount != modclegscolor
		) {
		SC_ByteVec3ColorFromCvar(EC_Colors[0], &cg_enemyHeadColor);
		EC_Colors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(EC_Colors[1], &cg_enemyTorsoColor);
		EC_Colors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(EC_Colors[2], &cg_enemyLegsColor);
		EC_Colors[2][3] = 255;
		modcheadcolor = cg_enemyHeadColor.modificationCount;
		modctorsocolor = cg_enemyTorsoColor.modificationCount;
		modclegscolor = cg_enemyLegsColor.modificationCount;
		EC_Loaded = 1;
	}

	if (checkSkins  &&  (*cg_enemyHeadSkin.string  ||  *cg_enemyTorsoSkin.string  ||  *cg_enemyLegsSkin.string)) {
		Com_Printf("reset alt enemy skins\n");
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			clientInfo_t *ci;

			ci = &cgs.clientinfo[i];
			ci->headEnemySkinAlt = 0;
			ci->torsoEnemySkinAlt = 0;
			ci->legsEnemySkinAlt = 0;
		}
	}
}

#define FOCUS_DISTANCE  400     //800   //512
int Wolfcam_OffsetThirdPersonView (void)
{
        vec3_t          forward, right, up;
        vec3_t          view;
        vec3_t          focusAngles;
        trace_t         trace;
        static vec3_t   mins = { -4, -4, -4 };
        static vec3_t   maxs = { 4, 4, 4 };
        vec3_t          focusPoint;
        float           focusDist;
        float           forwardScale, sideScale;
        //clientInfo_t *ci;
        const centity_t *cent;

        //ci = &cgs.clientinfo[wcg.clientNum];
        cent = &cg_entities[wcg.clientNum];

        //cent->currentState.eFlags &= ~EF_NODRAW;
        //cent->nextState.eFlags &= ~EF_NODRAW;

        VectorCopy (cg.refdef.vieworg, wcg.vieworgDemo);
        VectorCopy (cg.refdefViewAngles, wcg.refdefViewAnglesDemo);

        VectorCopy (cent->lerpOrigin, cg.refdef.vieworg);
        VectorCopy (cent->lerpAngles, cg.refdefViewAngles);

        cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;
        VectorCopy (cg.refdefViewAngles, focusAngles);
        if (cent->currentState.eFlags & EF_DEAD) {
            // rain - #254 - force yaw to 0 if we're tracking a medic
            //if( cg.snap->ps.viewlocked != 7 ) {  //FIXME wolfcam
            if (1) {
                //focusAngles[YAW] = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);  //FIXME wolfencam
				focusAngles[YAW] = 0;
                //cg.refdefViewAngles[YAW] = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
				cg.refdefViewAngles[YAW] = 0;
            }
        }

        if ( focusAngles[PITCH] > 45 )
            focusAngles[PITCH] = 45;
        AngleVectors( focusAngles, forward, NULL, NULL );

        if (0) {  //FIXME wolfcam (cg_thirdPerson.integer == 2 )
            //VectorCopy( cg.predictedPlayerState.origin, focusPoint );
        } else
            VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

        VectorCopy (cg.refdef.vieworg, view);
        view[2] += 8;
        cg.refdefViewAngles[PITCH] *= 0.5;
        AngleVectors( cg.refdefViewAngles, forward, right, up );
        forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
        sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
        VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view);
        VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );
        // trace a ray from the origin to the viewpoint to make sure the view isn't
        // in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

        CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, wcg.clientNum, MASK_SOLID );
        if ( trace.fraction != 1.0 ) {
            VectorCopy( trace.endpos, view );
            view[2] += (1.0 - trace.fraction) * 32;
            // try another trace to this position, because a tunnel may have the ceiling close enough that this is poking out
            CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, wcg.clientNum, MASK_SOLID);
            VectorCopy( trace.endpos, view );
        }
        VectorCopy( view, cg.refdef.vieworg);
        // select pitch to look at focus point from vieword
        VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
        focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
        if ( focusDist < 1 ) {
            focusDist = 1;  // should never happen
        }
        cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist);
        cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;

        AnglesToAxis (cg.refdefViewAngles, cg.refdef.viewaxis );

        //VectorCopy (cg.refdef.vieworg, cg.refdef_current->vieworg);
        //AnglesToAxis (cg.refdefViewAngles, cg.refdef_current->viewaxis);
        //FIXME wolfcam CG_CalcFov
        return 0;
}

/*
===============
Wolfcam_OffsetFirstPersonView

Sets cg.refdef view values if following other players
===============
*/
int Wolfcam_OffsetFirstPersonView (void)
{
    int clientNum;
    const clientInfo_t *ci;
    centity_t *cent;
    const entityState_t *es;
    wclient_t *wc;
    float ratio;
    float v_dmg_pitch;
    //qboolean wasBinocZooming;
    int timeDelta;
    //qboolean notDrawing;
    float bob;
    float speed;
    float delta;
    qboolean                useLastValidBob = qfalse;
    int anim;
	int t;

	memset(&cg.refdef, 0, sizeof(cg.refdef));
	CG_CalcVrect();
	if (cg.hyperspace) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

    clientNum = wcg.clientNum;

	if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
		const centity_t *c;

		c = &cg_entities[cg.wcKillerClientNum];
		t = wcg.nextKillerServerTime - cg.snap->serverTime;
		if (t < 0  &&  t > -wolfcam_hoverTime.integer) {

			if (!c->currentValid) {
				// handled at end of function
			} else {
				clientNum = cg.wcKillerClientNum;
			}
		}
	}

    ci = &cgs.clientinfo[clientNum];
    cent = &cg_entities[clientNum];
    es = &cent->currentState;
    wc = &wclients[wcg.clientNum];

    anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

    if (wcg.clientNum == cg.snap->ps.clientNum) {
        //cg.renderingThirdPerson = 0;
        //trap_Cvar_Set ("cg_thirdPerson", "0");
    } else {
        //cg.renderingThirdPerson = 1;
        //trap_Cvar_Set ("cg_thirdPerson", "1");
    }

    //notDrawing = cent->currentState.eFlags & EF_NODRAW;
    cent->currentState.eFlags |= EF_NODRAW;
    cent->nextState.eFlags |= EF_NODRAW;


    VectorCopy (cent->lerpOrigin, cg.refdef.vieworg);
    VectorCopy (cent->lerpAngles, cg.refdefViewAngles);

    // CG_TransitionPlayerState for wcg.clientNum
    //FIXME wolfcam don't know
#if 0
    if (cent->currentState.eFlags & EF_DEAD) {
        trap_SendConsoleCommand ("-zoom\n");
    }
#endif

#if 0
    wasBinocZooming = qfalse;
    if (wolfcam_esPrev.eFlags & EF_ZOOMING  &&  wolfcam_esPrev.weapon == WP_BINOCULARS) {
        wasBinocZooming = qtrue;
    }

    if (wasBinocZooming == qfalse  &&  cent->currentState.weapon == WP_BINOCULARS  &&  cent->currentState.eFlags & EF_ZOOMING) {
        wolfcam_binocZoomTime = cg.time;
        Com_Printf ("ent zooming with binocs\n");
    }
#endif

#if 1
    // add view height
    //if (es->eFlags & EF_CROUCHING) {
    if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
        cg.refdef.vieworg[2] += CROUCH_VIEWHEIGHT;
        //cg.refdef.vieworg[2] += CROUCH_VIEWHEIGHT;
        //Com_Printf ("test.....\n");
    }
    else {
        cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;
        //cg.refdef.vieworg[2] += DEFAULT_VIEWHEIGHT;
        //Com_Printf ("test.....\n");
    }
#endif

    timeDelta = cg.time - wc->duckTime;  //FIXME cg.ioverf
    {
        if (timeDelta < 0)
            wc->duckTime = cg.time - DUCK_TIME;
        if (timeDelta < DUCK_TIME) {
            cg.refdef.vieworg[2] -= wc->duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
        }
    }

#if 0
    //if (es->eFlags & EF_DEAD  &&  ci->infoValid  &&  !notDrawing) {
    if (!cent->currentValid  ||  es->eFlags & EF_DEAD) {
        return qfalse;
    }
#endif

    //FIXME  fall, stepOffset?

    // add angles based on bob

    //    if (cent->currentValid) {
    if (wclients[wcg.clientNum].currentValid) {
    // make sure the bob is visible even at low speeds
    speed = wc->xyspeed > 200 ? wc->xyspeed : 200;


    /////////////////////
    if( !wc->bobfracsin  &&  wc->lastvalidBobfracsin > 0 ) {
        // 200 msec to get back to center from 1
        // that's 1/200 per msec = 0.005 per msec
        wc->lastvalidBobfracsin -= 0.005 * cg.frametime;
        useLastValidBob = qtrue;
    }

    //    delta = useLastValidBob ? wolfcam_lastvalidBobfracsin * cg_bobpitch.value * speed : cg.bobfracsin * cg_bobpitch.value * speed;
    delta = useLastValidBob ? wc->lastvalidBobfracsin * cg_bobpitch.value * speed : wc->bobfracsin * cg_bobpitch.value * speed;

    //if (es->eFlags & EF_CROUCHING)
    if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR )
        delta *= 3;             // crouching
    cg.refdefViewAngles[PITCH] += delta;
    delta = useLastValidBob ? wc->lastvalidBobfracsin * cg_bobroll.value * speed : wc->bobfracsin * cg_bobroll.value * speed;

    //    if (es->eFlags & EF_CROUCHING)
    if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR )
        delta *= 3;             // crouching accentuates roll
    if( useLastValidBob ) {
        if( wc->lastvalidBobcycle & 1 )
            delta = -delta;
    } else if (wc->bobcycle & 1)
        delta = -delta;
    cg.refdefViewAngles[ROLL] += delta;  //FIXME wolfcam ok for weapons, sucks for regular walking


    //FIXME wolfcam using v_dmg_pitch == kick == -5
    v_dmg_pitch = -5;
    ratio = cg.time - wc->bulletDamageTime;
    if (ratio < DAMAGE_TIME  &&  !(es->eFlags & EF_DEAD)  &&  ci->infoValid) {
        if (ratio < DAMAGE_DEFLECT_TIME) {
            ratio /= DAMAGE_DEFLECT_TIME;
            cg.refdefViewAngles[PITCH] += ratio * v_dmg_pitch;
            //FIXME wolfcam ROLL
        } else {
            ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
            if (ratio > 0) {
                cg.refdefViewAngles[PITCH] += ratio * v_dmg_pitch;
                //FIXME wolfcam ROLL
            }
        }
    }

    bob = wc->bobfracsin * wc->xyspeed * cg_bobup.value;
    if (bob > 6) {
        bob = 6;
    }
    cg.refdef.vieworg[2] += bob;  //FIXME wolfcam ok for weapons, not player movement
    }

#if 0
    // ZoomSway
    if (wc->zoomval)
    {
        float spreadfrac, phase;

        if (es->eFlags & EF_MG42_ACTIVE) {  //  ||  es->eFlags & EF_AAGUN_ACTIVE) {
        } else {
            //spreadfrac = (float)cg.snap->ps.aimSpreadScale / 255.0;
            spreadfrac = 0.25;  //FIXME wolfcam
            phase = cg.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
            cg.refdefViewAngles[PITCH] += ZOOM_PITCH_AMPLITUDE * sin( phase ) * (spreadfrac+ZOOM_PITCH_MIN_AMPLITUDE);
            phase = cg.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
            cg.refdefViewAngles[YAW] += ZOOM_YAW_AMPLITUDE * sin( phase ) * (spreadfrac+ZOOM_YAW_MIN_AMPLITUDE);
        }
    }
#endif

	if (wcg.followMode == WOLFCAM_FOLLOW_VICTIM) {
		t = wcg.nextVictimServerTime - cg.snap->serverTime;
		if (t < 0  &&  t > -wolfcam_hoverTime.integer) {
			VectorCopy(cg.victimOrigin, cg.refdef.vieworg);
			VectorCopy(cg.victimAngles, cg.refdefViewAngles);
		}
	} else if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
		const centity_t *c;

		c = &cg_entities[cg.wcKillerClientNum];
		t = wcg.nextKillerServerTime - cg.snap->serverTime;
		if (t < 0  &&  t > -wolfcam_hoverTime.integer) {

			if (!c->currentValid) {
				VectorCopy(cg.wcKillerOrigin, cg.refdef.vieworg);
				VectorCopy(cg.wcKillerAngles, cg.refdefViewAngles);
			}
		}
	}

    AnglesToAxis (cg.refdefViewAngles, cg.refdef.viewaxis );

    //VectorCopy (cg.refdef.vieworg, cg.refdef_current->vieworg);
    //AnglesToAxis (cg.refdefViewAngles, cg.refdef_current->viewaxis);

    return 0;
}

#if 0
static void Wolfcam_AdjustZoomVal (float val, int type)
{
    //wclient_t *wc = &wclients[wcg.clientNum];

#if 0
    wc->zoomval += val;
    if (wc->zoomval > zoomTable[type][ZOOM_OUT])
        wc->zoomval = zoomTable[type][ZOOM_OUT];
    if(wc->zoomval < zoomTable[type][ZOOM_IN])
        wc->zoomval = zoomTable[type][ZOOM_IN];
#endif
}
#endif


#if 0
void Wolfcam_ZoomIn (void)
{
    //centity_t *cent;

#if 0
    cent = &cg_entities[cgs.clientinfo[wcg.clientNum].clientNum];

    if (cent->currentState.weapon == WP_SNIPERRIFLE) {
        Wolfcam_AdjustZoomVal (-(cg_zoomStepSniper.value), ZOOM_SNIPER);
    } else if (wclients[wcg.clientNum].zoomedBinoc) {
        Wolfcam_AdjustZoomVal (-(cg_zoomStepSniper.value), ZOOM_SNIPER);
    }
#endif
}

void Wolfcam_ZoomOut (void)
{
    //centity_t *cent;

#if 0
    cent = &cg_entities[cgs.clientinfo[wcg.clientNum].clientNum];

    if (cent->currentState.weapon == WP_SNIPERRIFLE) {
        Wolfcam_AdjustZoomVal(cg_zoomStepSniper.value, ZOOM_SNIPER);
    } else if (wclients[wcg.clientNum].zoomedBinoc) {
        Wolfcam_AdjustZoomVal(cg_zoomStepSniper.value, ZOOM_SNIPER); // JPW NERVE per atvi request BINOC);
    }
#endif
}


void Wolfcam_Zoom (void)
{
    //entityState_t *es;
    //wclient_t *wc;

#if 0
    es = &(cg_entities[cgs.clientinfo[wcg.clientNum].clientNum].currentState);
    wc = &wclients[wcg.clientNum];

	// OSP - Fix for demo playback
	if ( (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback ) {
		cg.predictedPlayerState.eFlags = cg.snap->ps.eFlags;
		cg.predictedPlayerState.weapon = cg.snap->ps.weapon;

		// check for scope wepon in use, and switch to if necessary
		// OSP - spec/demo scaling allowances
		if(es->weapon == WP_FG42SCOPE)
			wc->zoomval = (wc->zoomval == 0) ? cg_zoomDefaultSniper.value : wc->zoomval; // JPW NERVE was DefaultFG, changed per atvi req
		else if ( es->weapon == WP_SNIPERRIFLE )
			wc->zoomval = (wc->zoomval == 0) ? cg_zoomDefaultSniper.value : wc->zoomval;
		else if(!(es->eFlags & EF_ZOOMING))
			wc->zoomval = 0;
	}

	if(es->eFlags & EF_ZOOMING) {
		if ( wc->zoomedBinoc )
			return;
		wc->zoomedBinoc	= qtrue;
		wc->zoomTime	= cg.time;
		wc->zoomval = cg_zoomDefaultSniper.value; // JPW NERVE was DefaultBinoc, changed per atvi req
	}
	else {
		if (wc->zoomedBinoc) {
			wc->zoomedBinoc	= qfalse;
			wc->zoomTime	= cg.time;

			// check for scope weapon in use, and switch to if necessary
			//if( cg.weaponSelect == WP_FG42SCOPE ) {
            if (es->weapon == WP_FG42SCOPE) {
				wc->zoomval = cg_zoomDefaultSniper.value; // JPW NERVE was DefaultFG, changed per atvi req
			} else if ( es->weapon == WP_SNIPERRIFLE ) {
				wc->zoomval = cg_zoomDefaultSniper.value;
			} else {
				wc->zoomval = 0;
			}
		} else {
//bani - we now sanity check to make sure we can't zoom non-zoomable weapons
//zinx - fix for #423 - don't sanity check while following
			if (!((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)) {
				switch( es->weapon ) {
					case WP_FG42SCOPE:
					case WP_SNIPERRIFLE:
						break;
					default:
						wc->zoomval = 0;
						break;
				}
			}
		}
	}
#endif
}

#endif

#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

int Wolfcam_CalcFov (void)
{
	//FIXME wolfcam-rtcw
	//static float lastfov = 90;		// for transitions back from zoomed in modes
	//float	x;
	float	phase;
	float	v;
	int		contents;
	double	fov_x, fov_y;
	int		inwater;
    //clientInfo_t *ci;
    const centity_t *cent;
    const entityState_t *es;
    wclient_t *wc;

    //ci = &cgs.clientinfo[wcg.clientNum];
    cent = &cg_entities[wcg.clientNum];
    es = &cent->currentState;
    wc = &wclients[wcg.clientNum];

	//Wolfcam_Zoom();

	//if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW) )
    if (es->eFlags & EF_DEAD)
	{
		wc->zoomedBinoc = qfalse;
		wc->zoomTime = 0;
		wc->zoomval = 0;
	}

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
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov_x = cg.demoFov;
		} else {
			fov_x = cg_fov.value;
		}

		//if( !cg.renderingThirdPerson || developer.integer ) {  //FIXME wolfcam
        if ( !(es->eFlags & EF_DEAD) ) {  // ||  developer.integer) {
			// account for zooms
		}
	}

	fov_x = CG_CalcZoom(fov_x);

	CG_AdjustedFov(fov_x, &cg.refdef.fov_x, &cg.refdef.fov_y);

	fov_x = cg.refdef.fov_x;
	fov_y = cg.refdef.fov_y;

	// warp if underwater
	//if ( cg_pmove.waterlevel == 3 ) {
	contents = CG_PointContents( cg.refdef.vieworg, -1 );  // wolfcam this is ok I think
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
		//cg.refdef.rdflags |= RDF_UNDERWATER;
	} else {
#if 0
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
		inwater = qfalse;
#endif
		inwater = qfalse;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

/*
	if( cg.predictedPlayerState.eFlags & EF_PRONE ) {
		cg.zoomSensitivity = cg.refdef.fov_y / 500.0;
	} else
*/
	// rain - allow freelook when dead until we tap out into limbo
	if (es->eFlags & EF_DEAD) {  //if( cg.snap->ps.pm_type == PM_FREEZE || (cg.snap->ps.pm_type == PM_DEAD && (cg.snap->ps.pm_flags & PMF_LIMBO)) || cg.snap->ps.pm_flags & PMF_TIME_LOCKPLAYER ) {  //FIXME wolfcam
		// No movement for pauses
		cg.zoomSensitivity = 0;
	} else if ( !wc->zoomedBinoc ) {
		// NERVE - SMF - fix for zoomed in/out movement bug
		if ( wc->zoomval ) {
			cg.zoomSensitivity = 0.6 * ( wc->zoomval / 90.f );	// NERVE - SMF - changed to get less sensitive as you zoom in
//				cg.zoomSensitivity = 0.1;
		} else {
			cg.zoomSensitivity = 1;
		}
		// -NERVE - SMF
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
    //return 0;
}

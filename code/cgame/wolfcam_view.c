#include "cg_local.h"

#include "cg_main.h"
#include "cg_players.h"
#include "cg_predict.h"
#include "cg_snapshot.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "sc.h"
#include "wolfcam_view.h"

#include "wolfcam_local.h"

static void RegisterTeamClientModelnameWithFallback (clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName, qboolean dontForceTeamSkin)
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

static void Wolfcam_LoadTeamModel (void)
{
	char modelStr[MAX_QPATH];
	char *skin;
	char headModelStr[MAX_QPATH];
	char *headSkin;
	int i;
	//FIXME in cg. ?
	static int modc_model = -1;
	static int modc_headModel = -1;
	static int modc_headSkin = -1;
	static int modc_torsoSkin = -1;
	static int modc_legsSkin = -1;
	static int modc_headColor = -1;
	static int modc_torsoColor = -1;
	static int modc_legsColor = -1;

	if (cg_teamHeadColor.modificationCount != modc_headColor  ||
		cg_teamTorsoColor.modificationCount != modc_torsoColor  ||
		cg_teamLegsColor.modificationCount != modc_legsColor
		) {
		SC_ByteVec3ColorFromCvar(cg.teamColors[0], &cg_teamHeadColor);
		cg.teamColors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.teamColors[1], &cg_teamTorsoColor);
		cg.teamColors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.teamColors[2], &cg_teamLegsColor);
		cg.teamColors[2][3] = 255;
		modc_headColor = cg_teamHeadColor.modificationCount;
		modc_torsoColor = cg_teamTorsoColor.modificationCount;
		modc_legsColor = cg_teamLegsColor.modificationCount;
	}

	//FIXME team*skin change doesn't need to reload model
	if (cg_teamModel.modificationCount != modc_model  ||
		cg_teamHeadModel.modificationCount != modc_headModel  ||

		cg_teamHeadSkin.modificationCount != modc_headSkin  ||
		cg_teamTorsoSkin.modificationCount != modc_torsoSkin  ||
		cg_teamLegsSkin.modificationCount != modc_legsSkin

) {
		modc_model = cg_teamModel.modificationCount;
		modc_headModel = cg_teamHeadModel.modificationCount;

		modc_headSkin = cg_teamHeadSkin.modificationCount;
		modc_torsoSkin = cg_teamTorsoSkin.modificationCount;
		modc_legsSkin = cg_teamLegsSkin.modificationCount;
	} else {
		return;
	}

	Com_Printf("loading team models\n");

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		clientInfo_t *ci;

		ci = &cgs.clientinfo[i];
		ci->headTeamSkinAlt = 0;
		ci->torsoTeamSkinAlt = 0;
		ci->legsTeamSkinAlt = 0;
	}
	// reset since cg.ourModel can fallback to using original player models
	cg.ourModel.headTeamSkinAlt = 0;
	cg.ourModel.torsoTeamSkinAlt = 0;
	cg.ourModel.legsTeamSkinAlt = 0;
	cg.ourModelRed.headTeamSkinAlt = 0;
	cg.ourModelRed.torsoTeamSkinAlt = 0;
	cg.ourModelRed.legsTeamSkinAlt = 0;
	cg.ourModelBlue.headTeamSkinAlt = 0;
	cg.ourModelBlue.torsoTeamSkinAlt = 0;
	cg.ourModelBlue.legsTeamSkinAlt = 0;

	if (*cg_teamModel.string) {
		Q_strncpyz(modelStr, cg_teamModel.string, sizeof(modelStr));
	} else {
		Q_strncpyz(modelStr, DEFAULT_MODEL, sizeof(modelStr));
	}

	if (*cg_teamHeadModel.string) {
		Q_strncpyz(headModelStr, cg_teamHeadModel.string, sizeof(headModelStr));
	} else {
		Q_strncpyz(headModelStr, DEFAULT_MODEL, sizeof(headModelStr));
	}

	if ((skin = strchr(modelStr, '/')) == NULL) {
		skin = "default";
		cg.teamModelTeamSkinFound = qtrue;
	} else {
		*skin++ = 0;
		cg.teamModelTeamSkinFound = qfalse;
	}

	if (!Q_stricmp(skin, TEAM_COLOR_SKIN)) {
		cg.teamModelTeamSkinFound = qtrue;
	}

	if ((headSkin = strchr(headModelStr, '/')) == NULL) {
		headSkin = "default";
		cg.teamModelTeamHeadSkinFound = qtrue;
	} else {
		*headSkin++ = 0;
		cg.teamModelTeamHeadSkinFound = qfalse;
	}

	if (!Q_stricmp(headSkin, TEAM_COLOR_SKIN)) {
		cg.teamModelTeamHeadSkinFound = qtrue;
	}

	Q_strncpyz(cg.teamModel.modelName, modelStr, sizeof(cg.teamModel.modelName));
	Q_strncpyz(cg.teamModel.skinName, skin, sizeof(cg.teamModel.skinName));
	Q_strncpyz(cg.teamModelRed.modelName, modelStr, sizeof(cg.teamModelRed.modelName));
	Q_strncpyz(cg.teamModelRed.skinName, skin, sizeof(cg.teamModelRed.skinName));
	Q_strncpyz(cg.teamModelBlue.modelName, modelStr, sizeof(cg.teamModelBlue.modelName));
	Q_strncpyz(cg.teamModelBlue.skinName, skin, sizeof(cg.teamModelBlue.skinName));


	Q_strncpyz(cg.teamModel.headModelName, headModelStr, sizeof(cg.teamModel.headModelName));
	Q_strncpyz(cg.teamModel.headSkinName, headSkin, sizeof(cg.teamModel.headSkinName));
	Q_strncpyz(cg.teamModelRed.headModelName, headModelStr, sizeof(cg.teamModelRed.headModelName));
	Q_strncpyz(cg.teamModelRed.headSkinName, headSkin, sizeof(cg.teamModelRed.headSkinName));
	Q_strncpyz(cg.teamModelBlue.headModelName, headModelStr, sizeof(cg.teamModelBlue.headModelName));
	Q_strncpyz(cg.teamModelBlue.headSkinName, headSkin, sizeof(cg.teamModelBlue.headSkinName));

	RegisterTeamClientModelnameWithFallback(&cg.teamModel, cg.teamModel.modelName, cg.teamModel.skinName, cg.teamModel.headModelName, cg.teamModel.headSkinName, "", qtrue);

	cg.teamModelRed.team = TEAM_RED;
	RegisterTeamClientModelnameWithFallback(&cg.teamModelRed, cg.teamModelRed.modelName, cg.teamModelRed.skinName, cg.teamModelRed.headModelName, cg.teamModelRed.headSkinName, "", qfalse);

	cg.teamModelBlue.team = TEAM_BLUE;
	RegisterTeamClientModelnameWithFallback(&cg.teamModelBlue, cg.teamModelBlue.modelName, cg.teamModelBlue.skinName, cg.teamModelBlue.headModelName, cg.teamModelBlue.headSkinName, "", qfalse);

	if (!*cg_teamModel.string) {
		// prevent loading legs and torso, use original client values
		cg.teamModel.legsModel = 0;
		cg.teamModel.torsoModel = 0;
		cg.teamModelRed.legsModel = 0;
		cg.teamModelRed.torsoModel = 0;
		cg.teamModelBlue.legsModel = 0;
		cg.teamModelBlue.torsoModel = 0;
	}

	if (!*cg_teamHeadModel.string) {
		cg.teamModel.headModel = 0;
		cg.teamModelRed.headModel = 0;
		cg.teamModelBlue.headModel = 0;
	}
}

static void Wolfcam_LoadOurModel (void)
{
	char modelStr[MAX_QPATH];
	char *skin;
	char headModelStr[MAX_QPATH];
	char *headSkin;
	const char *dir;
	const char *s;
	int i;
	static int modc_model = -1;
	static int modc_headModel = -1;
	static int modc_headSkin = -1;
	static int modc_torsoSkin = -1;
	static int modc_legsSkin = -1;
	static int modc_headColor = -1;
	static int modc_torsoColor = -1;
	static int modc_legsColor = -1;

	//FIXME modelIcon?

	if (cg_ourHeadColor.modificationCount != modc_headColor  ||
		cg_ourTorsoColor.modificationCount != modc_torsoColor  ||
		cg_ourLegsColor.modificationCount != modc_legsColor
		) {
		SC_ByteVec3ColorFromCvar(cg.ourColors[0], &cg_ourHeadColor);
		cg.ourColors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.ourColors[1], &cg_ourTorsoColor);
		cg.ourColors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.ourColors[2], &cg_ourLegsColor);
		cg.ourColors[2][3] = 255;
		modc_headColor = cg_ourHeadColor.modificationCount;
		modc_torsoColor = cg_ourTorsoColor.modificationCount;
		modc_legsColor = cg_ourLegsColor.modificationCount;
	}

	if (modc_headSkin != cg_ourHeadSkin.modificationCount  ||
		modc_torsoSkin != cg_ourTorsoSkin.modificationCount  ||
		modc_legsSkin != cg_ourLegsSkin.modificationCount  ||
		modc_model != cg_ourModel.modificationCount  ||
		modc_headModel != cg_ourHeadModel.modificationCount
		) {
		modc_headSkin = cg_ourHeadSkin.modificationCount;
		modc_torsoSkin = cg_ourTorsoSkin.modificationCount;
		modc_legsSkin = cg_ourLegsSkin.modificationCount;

		cg.ourModel.headOurSkinAlt = 0;
		cg.ourModel.torsoOurSkinAlt = 0;
		cg.ourModel.legsOurSkinAlt = 0;

		modc_model = cg_ourModel.modificationCount;
		modc_headModel = cg_ourHeadModel.modificationCount;
	} else {
		return;
	}

	Com_Printf("loading our model\n");

	memset(&cg.ourModel, 0, sizeof(clientInfo_t));

	//FIXME this is always true since default cvar is set to something (sarge) -- 2016-05-20 they can now be set to ""
	if (1) {  //(*cg_ourModel.string  ||  *cg_ourHeadModel.string) {
		if (*cg_ourModel.string) {
			Q_strncpyz(modelStr, cg_ourModel.string, sizeof(modelStr));
		} else {
			// silence warnings
			Q_strncpyz(modelStr, DEFAULT_MODEL, sizeof(modelStr));
		}
		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		if (!Q_stricmp(skin, TEAM_COLOR_SKIN)) {
			cg.ourModelUsingTeamColorSkin = qtrue;
		} else {
			cg.ourModelUsingTeamColorSkin = qfalse;
		}

		if (*cg_ourHeadModel.string) {
			Q_strncpyz(headModelStr, cg_ourHeadModel.string, sizeof(headModelStr));
		} else {
			// silence warnings
			Q_strncpyz(headModelStr, DEFAULT_MODEL, sizeof(headModelStr));
		}
		if ((headSkin = strchr(headModelStr, '/')) == NULL) {
			headSkin = "default";
		} else {
			*headSkin++ = 0;
		}

		if (!Q_stricmp(headSkin, TEAM_COLOR_SKIN)) {
			cg.ourModelUsingTeamColorHeadSkin = qtrue;
		} else {
			cg.ourModelUsingTeamColorHeadSkin = qfalse;
		}

		Q_strncpyz(cg.ourModel.modelName, modelStr, sizeof(cg.ourModel.modelName));
		Q_strncpyz(cg.ourModel.skinName, skin, sizeof(cg.ourModel.skinName));

		Q_strncpyz(cg.ourModel.headModelName, headModelStr, sizeof(cg.ourModel.headModelName));
		Q_strncpyz(cg.ourModel.headSkinName, headSkin, sizeof(cg.ourModel.headSkinName));

		// note CG_Register* fails if anything is invalid, will not go on to load animations  --  2016-05-24 CG_Register* has been changed so that it keeps loading as much as possible (including animations?)
		if (!CG_RegisterClientModelname(&cg.ourModel, cg.ourModel.modelName, cg.ourModel.skinName, cg.ourModel.headModelName, cg.ourModel.headSkinName, "", qtrue)) {
			//Com_Printf("^1couldn't load our model torso %d  legs %d  head %d\n", cg.ourModel.legsModel, cg.ourModel.torsoModel, cg.ourModel.headModel);
		}

		cg.ourModel.newAnims = qfalse;
		if (cg.ourModel.torsoModel) {
			orientation_t tag;
			// if the torso model has the "tag_flag"
			if (trap_R_LerpTag(&tag, cg.ourModel.torsoModel, 0, 0, 1, "tag_flag")) {
				cg.ourModel.newAnims = qtrue;
			}
		}

		// red and blue skins
		memcpy(&cg.ourModelRed, &cg.ourModel, sizeof(clientInfo_t));
		memcpy(&cg.ourModelBlue, &cg.ourModel, sizeof(clientInfo_t));

		cg.ourModelRed.team = TEAM_RED;
		cg.ourModelBlue.team = TEAM_BLUE;

		CG_RegisterClientModelname(&cg.ourModelRed, cg.ourModelRed.modelName, cg.ourModelRed.skinName, cg.ourModelRed.headModelName, cg.ourModelRed.headSkinName, "red", qfalse);
		CG_RegisterClientModelname(&cg.ourModelBlue, cg.ourModelBlue.modelName, cg.ourModelBlue.skinName, cg.ourModelBlue.headModelName, cg.ourModelBlue.headSkinName, "blue", qfalse);

		if (!*cg_ourModel.string) {
			// prevent loading legs and torso, use original client values
			cg.ourModel.legsModel = 0;
			cg.ourModel.torsoModel = 0;
			cg.ourModelRed.legsModel = 0;
			cg.ourModelRed.torsoModel = 0;
			cg.ourModelBlue.legsModel = 0;
			cg.ourModelBlue.torsoModel = 0;

		}

		if (!*cg_ourHeadModel.string) {
			cg.ourModel.headModel = 0;
			cg.ourModelRed.headModel = 0;
			cg.ourModelBlue.headModel = 0;
		}

		// sounds
		dir = cg.ourModel.modelName;
		//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

		for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
			s = cg_customSoundNames[i];
			if (!s) {
				break;
			}
			cg.ourModel.sounds[i] = cg.ourModelRed.sounds[i] = cg.ourModelBlue.sounds[i] = 0;

			cg.ourModel.sounds[i] = cg.ourModelRed.sounds[i] = cg.ourModelBlue.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
		}  // max custom sounds

	}  // cg_ourModel.string*

}

//FIXME duplicate code Wolfcam_LoadOurModel
static void Wolfcam_LoadFallbackModel (void)
{
	char modelStr[MAX_QPATH];
	char *skin;
	char headModelStr[MAX_QPATH];
	char *headSkin;
	const char *dir;
	const char *s;
	int i;
	static int modc_model = -1;
	static int modc_headModel = -1;
	//static int modc_headSkin = -1;
	//static int modc_torsoSkin = -1;
	//static int modc_legsSkin = -1;
	//static int modc_headColor = -1;
	//static int modc_torsoColor = -1;
	//static int modc_legsColor = -1;


#if 0
	//FIXME modelIcon?

	if (cg_ourHeadColor.modificationCount != modc_headColor  ||
		cg_ourTorsoColor.modificationCount != modc_torsoColor  ||
		cg_ourLegsColor.modificationCount != modc_legsColor
		) {
		SC_ByteVec3ColorFromCvar(cg.ourColors[0], &cg_ourHeadColor);
		cg.ourColors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.ourColors[1], &cg_ourTorsoColor);
		cg.ourColors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.ourColors[2], &cg_ourLegsColor);
		cg.ourColors[2][3] = 255;
		modc_headColor = cg_ourHeadColor.modificationCount;
		modc_torsoColor = cg_ourTorsoColor.modificationCount;
		modc_legsColor = cg_ourLegsColor.modificationCount;
	}
#endif
	
	if (//modc_headSkin != cg_ourHeadSkin.modificationCount  ||
		//modc_torsoSkin != cg_ourTorsoSkin.modificationCount  ||
		//modc_legsSkin != cg_ourLegsSkin.modificationCount  ||

		modc_model != cg_fallbackModel.modificationCount  ||
		modc_headModel != cg_fallbackHeadModel.modificationCount
		) {
		//modc_headSkin = cg_ourHeadSkin.modificationCount;
		//modc_torsoSkin = cg_ourTorsoSkin.modificationCount;
		//modc_legsSkin = cg_ourLegsSkin.modificationCount;

		//cg.ourModel.headOurSkinAlt = 0;
		//cg.ourModel.torsoOurSkinAlt = 0;
		//cg.ourModel.legsOurSkinAlt = 0;

		modc_model = cg_fallbackModel.modificationCount;
		modc_headModel = cg_fallbackHeadModel.modificationCount;
	} else {
		return;
	}

	Com_Printf("loading fallback model\n");

	memset(&cg.fallbackModel, 0, sizeof(clientInfo_t));

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		clientInfo_t *ci;

		ci = &cgs.clientinfo[i];
		ci->headTeamSkinAlt = 0;
		ci->torsoTeamSkinAlt = 0;
		ci->legsTeamSkinAlt = 0;
	}
	// reset since cg.ourModel can fallback to using original player models
	cg.ourModel.headTeamSkinAlt = 0;
	cg.ourModel.torsoTeamSkinAlt = 0;
	cg.ourModel.legsTeamSkinAlt = 0;
	cg.ourModelRed.headTeamSkinAlt = 0;
	cg.ourModelRed.torsoTeamSkinAlt = 0;
	cg.ourModelRed.legsTeamSkinAlt = 0;
	cg.ourModelBlue.headTeamSkinAlt = 0;
	cg.ourModelBlue.torsoTeamSkinAlt = 0;
	cg.ourModelBlue.legsTeamSkinAlt = 0;

	if (1) {  //(*cg_fallbackModel.string  ||  *cg_fallbackHeadModel.string) {
		if (*cg_fallbackModel.string) {
			Q_strncpyz(modelStr, cg_fallbackModel.string, sizeof(modelStr));
		} else {
			// silence warnings
			Q_strncpyz(modelStr, DEFAULT_MODEL, sizeof(modelStr));
		}
		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		if (*cg_fallbackHeadModel.string) {
			Q_strncpyz(headModelStr, cg_fallbackHeadModel.string, sizeof(headModelStr));
		} else {
			// silence warnings
			Q_strncpyz(headModelStr, DEFAULT_MODEL, sizeof(headModelStr));
		}
		if ((headSkin = strchr(headModelStr, '/')) == NULL) {
			headSkin = "default";
		} else {
			*headSkin++ = 0;
		}

		Q_strncpyz(cg.fallbackModel.modelName, modelStr, sizeof(cg.fallbackModel.modelName));
		Q_strncpyz(cg.fallbackModel.skinName, skin, sizeof(cg.fallbackModel.skinName));

		Q_strncpyz(cg.fallbackModel.headModelName, headModelStr, sizeof(cg.fallbackModel.headModelName));
		Q_strncpyz(cg.fallbackModel.headSkinName, headSkin, sizeof(cg.fallbackModel.headSkinName));

		// note CG_Register* fails if anything is invalid, will not go on to load animations  --  2016-05-24 CG_Register* has been changed so that it keeps loading as much as possible (including animations?)
		if (!CG_RegisterClientModelname(&cg.fallbackModel, cg.fallbackModel.modelName, cg.fallbackModel.skinName, cg.fallbackModel.headModelName, cg.fallbackModel.headSkinName, "", qtrue)) {
			//Com_Printf("^1couldn't load fallback model torso %d  legs %d  head %d\n", cg.fallbackModel.legsModel, cg.fallbackModel.torsoModel, cg.fallbackModel.headModel);
		}

		cg.fallbackModel.newAnims = qfalse;
		if (cg.fallbackModel.torsoModel) {
			orientation_t tag;
			// if the torso model has the "tag_flag"
			if (trap_R_LerpTag(&tag, cg.fallbackModel.torsoModel, 0, 0, 1, "tag_flag")) {
				cg.fallbackModel.newAnims = qtrue;
			}
		}

		// red and blue skins
		memcpy(&cg.fallbackModelRed, &cg.fallbackModel, sizeof(clientInfo_t));
		memcpy(&cg.fallbackModelBlue, &cg.fallbackModel, sizeof(clientInfo_t));

		cg.fallbackModelRed.team = TEAM_RED;
		cg.fallbackModelBlue.team = TEAM_BLUE;

		CG_RegisterClientModelname(&cg.fallbackModelRed, cg.fallbackModelRed.modelName, cg.fallbackModelRed.skinName, cg.fallbackModelRed.headModelName, cg.fallbackModelRed.headSkinName, "red", qfalse);
		CG_RegisterClientModelname(&cg.fallbackModelBlue, cg.fallbackModelBlue.modelName, cg.fallbackModelBlue.skinName, cg.fallbackModelBlue.headModelName, cg.fallbackModelBlue.headSkinName, "blue", qfalse);

		if (!*cg_fallbackModel.string) {
			// prevent loading legs and torso, use original client values
			cg.fallbackModel.legsModel = 0;
			cg.fallbackModel.torsoModel = 0;
			cg.fallbackModelRed.legsModel = 0;
			cg.fallbackModelRed.torsoModel = 0;
			cg.fallbackModelBlue.legsModel = 0;
			cg.fallbackModelBlue.torsoModel = 0;

		}

		if (!*cg_fallbackHeadModel.string) {
			cg.fallbackModel.headModel = 0;
			cg.fallbackModelRed.headModel = 0;
			cg.fallbackModelBlue.headModel = 0;
		}

		// sounds
		dir = cg.fallbackModel.modelName;
		//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

		for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
			s = cg_customSoundNames[i];
			if (!s) {
				break;
			}
			cg.fallbackModel.sounds[i] = cg.fallbackModelRed.sounds[i] = cg.fallbackModelBlue.sounds[i] = 0;

			cg.fallbackModel.sounds[i] = cg.fallbackModelRed.sounds[i] = cg.fallbackModelBlue.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
		}  // max custom sounds

	}  // cg_fallbackModel.string*

}

static void Wolfcam_LoadEnemyModel (void)
{
	char modelStr[MAX_QPATH];
	char *skin;
	char headModelStr[MAX_QPATH];
	char *headSkin;
	int i;
	//FIXME modification count can't be changed by cgame?
	static int modc_model = -1;
	static int modc_headModel;
	static int modc_headSkin = -1;
	static int modc_torsoSkin = -1;
	static int modc_legsSkin = -1;
	static int modc_headColor = -1;
	static int modc_torsoColor = -1;
	static int modc_legsColor = -1;

	if (cg_enemyHeadColor.modificationCount != modc_headColor  ||
		cg_enemyTorsoColor.modificationCount != modc_torsoColor  ||
		cg_enemyLegsColor.modificationCount != modc_legsColor
		) {
		SC_ByteVec3ColorFromCvar(cg.enemyColors[0], &cg_enemyHeadColor);
		cg.enemyColors[0][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.enemyColors[1], &cg_enemyTorsoColor);
		cg.enemyColors[1][3] = 255;
		SC_ByteVec3ColorFromCvar(cg.enemyColors[2], &cg_enemyLegsColor);
		cg.enemyColors[2][3] = 255;
		modc_headColor = cg_enemyHeadColor.modificationCount;
		modc_torsoColor = cg_enemyTorsoColor.modificationCount;
		modc_legsColor = cg_enemyLegsColor.modificationCount;
	}

	if (cg_enemyModel.modificationCount != modc_model  ||
		cg_enemyHeadModel.modificationCount != modc_headModel  ||
		cg_enemyHeadSkin.modificationCount != modc_headSkin  ||
		cg_enemyTorsoSkin.modificationCount != modc_torsoSkin  ||
		cg_enemyLegsSkin.modificationCount != modc_legsSkin

) {
		EM_Loaded = 0;
		modc_model = cg_enemyModel.modificationCount;
		modc_headModel = cg_enemyHeadModel.modificationCount;
		modc_headSkin = cg_enemyHeadSkin.modificationCount;
		modc_torsoSkin = cg_enemyTorsoSkin.modificationCount;
		modc_legsSkin = cg_enemyLegsSkin.modificationCount;

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			clientInfo_t *ci;

			ci = &cgs.clientinfo[i];
			ci->headEnemySkinAlt = 0;
			ci->torsoEnemySkinAlt = 0;
			ci->legsEnemySkinAlt = 0;

			// reset these because of fallback models for teammates matching enemy model
			ci->headTeamSkinAlt = 0;
			ci->torsoTeamSkinAlt = 0;
			ci->legsTeamSkinAlt = 0;

		}
		// not used currently, but change here to be consistent with team skins
		cg.ourModel.headEnemySkinAlt = 0;
		cg.ourModel.torsoEnemySkinAlt = 0;
		cg.ourModel.legsEnemySkinAlt = 0;
		cg.ourModelRed.headEnemySkinAlt = 0;
		cg.ourModelRed.torsoEnemySkinAlt = 0;
		cg.ourModelRed.legsEnemySkinAlt = 0;
		cg.ourModelBlue.headEnemySkinAlt = 0;
		cg.ourModelBlue.torsoEnemySkinAlt = 0;
		cg.ourModelBlue.legsEnemySkinAlt = 0;

	} else {
		return;
	}

	if (EM_Loaded == 0) {  //  &&  ((cg_enemyModel.string  &&  *cg_enemyModel.string)  ||  (cg_enemyHeadModel.string  &&  *cg_enemyHeadModel.string))) {

		memset(&cg.enemyModel, 0, sizeof(clientInfo_t));

		if (*cg_enemyModel.string) {
			Q_strncpyz(modelStr, cg_enemyModel.string, sizeof(modelStr));
		} else {
			// just set to something in order to load
			Q_strncpyz(modelStr, DEFAULT_MODEL, sizeof(modelStr));
		}

		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
			cg.enemyModelTeamSkinFound = qtrue;
		} else {
			cg.enemyModelTeamSkinFound = qfalse;
			*skin++ = 0;
		}

		if (!Q_stricmp(skin, TEAM_COLOR_SKIN)) {
			cg.enemyModelTeamSkinFound = qtrue;
		}

		if (*cg_enemyHeadModel.string) {
			Q_strncpyz(headModelStr, cg_enemyHeadModel.string, sizeof(headModelStr));
		} else {
			Q_strncpyz(headModelStr, DEFAULT_MODEL, sizeof(headModelStr));
		}

		if ((headSkin = strchr(headModelStr, '/')) == NULL) {
			headSkin = "default";
			cg.enemyModelTeamHeadSkinFound = qtrue;
		} else {
			cg.enemyModelTeamHeadSkinFound = qfalse;
			*headSkin++ = 0;
		}

		Q_strncpyz(cg.enemyModel.modelName, modelStr, sizeof(cg.enemyModel.modelName));
		Q_strncpyz(cg.enemyModel.skinName, skin, sizeof(cg.enemyModel.skinName));

		Q_strncpyz(cg.enemyModel.headModelName, headModelStr, sizeof(cg.enemyModel.headModelName));
		Q_strncpyz(cg.enemyModel.headSkinName, headSkin, sizeof(cg.enemyModel.headSkinName));

		if (CG_RegisterClientModelname(&cg.enemyModel, cg.enemyModel.modelName, cg.enemyModel.skinName, cg.enemyModel.headModelName, cg.enemyModel.headSkinName, "", qtrue)) {
			const char *dir, *fallback, *s;

			EM_Loaded = 1;
			cg.enemyModel.newAnims = qfalse;
			if (cg.enemyModel.torsoModel) {
				orientation_t tag;
				// if the torso model has the "tag_flag"
				if (trap_R_LerpTag(&tag, cg.enemyModel.torsoModel, 0, 0, 1, "tag_flag")) {
					cg.enemyModel.newAnims = qtrue;
				}
			}

			// red and blue skins
			memcpy(&cg.enemyModelRed, &cg.enemyModel, sizeof(clientInfo_t));
			memcpy(&cg.enemyModelBlue, &cg.enemyModel, sizeof(clientInfo_t));

			cg.enemyModelRed.team = TEAM_RED;
			cg.enemyModelBlue.team = TEAM_BLUE;

			CG_RegisterClientModelname(&cg.enemyModelRed, cg.enemyModelRed.modelName, cg.enemyModelRed.skinName, cg.enemyModelRed.headModelName, cg.enemyModelRed.headSkinName, "red", qfalse);
			CG_RegisterClientModelname(&cg.enemyModelBlue, cg.enemyModelBlue.modelName, cg.enemyModelBlue.skinName, cg.enemyModelBlue.headModelName, cg.enemyModelBlue.headSkinName, "blue", qfalse);

			if (!*cg_enemyModel.string) {
				// prevent loading legs and torso, use original client values
				cg.enemyModel.legsModel = 0;
				cg.enemyModel.torsoModel = 0;
				cg.enemyModelRed.legsModel = 0;
				cg.enemyModelRed.torsoModel = 0;
				cg.enemyModelBlue.legsModel = 0;
				cg.enemyModelBlue.torsoModel = 0;
			}

			if (!*cg_enemyHeadModel.string) {
				// prevent loading head, use original client values
				cg.enemyModel.headModel = 0;
				cg.enemyModelRed.headModel = 0;
				cg.enemyModelBlue.headModel = 0;
			}

			// sounds
			dir = cg.enemyModel.modelName;
			fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

			for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
				s = cg_customSoundNames[i];
				if (!s) {
					break;
				}
				cg.enemyModel.sounds[i] = cg.enemyModelRed.sounds[i] = cg.enemyModelBlue.sounds[i] = 0;
				// if the model didn't load use the sounds of the default model
				cg.enemyModel.sounds[i] = cg.enemyModelRed.sounds[i] = cg.enemyModelBlue.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
				if (!cg.enemyModel.sounds[i])
					cg.enemyModel.sounds[i] = cg.enemyModelRed.sounds[i] = cg.enemyModelBlue.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
			}
		} else {  // couldn't register model
			EM_Loaded = -1;
		}
	}
}

void Wolfcam_LoadModels (void)
{
	static int modc_disallow = -1;
	static int modc_useDefaultTeamSkins = -1;
	static int modc_ignoreClientHeadModel = -1;

	//FIXME check forcemodel change from cg_main.c here

	if (cg_useDefaultTeamSkins.integer != modc_useDefaultTeamSkins  ||
		cg_ignoreClientHeadModel.integer != modc_ignoreClientHeadModel
		) {
		CG_ForceModelChange();
#if 0
		int i;

		//FIXME duplicate code CG_ForceModelChange()
		for (i=0 ; i<MAX_CLIENTS ; i++) {
			const char *clientInfo;

			clientInfo = CG_ConfigString( CS_PLAYERS+i );
			if ( !clientInfo[0] ) {
				continue;
			}
			CG_NewClientInfo( i );
		}
#endif
		//Com_Printf("^6forcing model change\n");
		modc_useDefaultTeamSkins = cg_useDefaultTeamSkins.integer;
		modc_ignoreClientHeadModel = cg_ignoreClientHeadModel.integer;
	}

	Wolfcam_LoadOurModel();
	Wolfcam_LoadTeamModel();
	Wolfcam_LoadEnemyModel();
	Wolfcam_LoadFallbackModel();

	if (cg_disallowEnemyModelForTeammates.modificationCount != modc_disallow) {
		int i;

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			clientInfo_t *ci;

			ci = &cgs.clientinfo[i];
			ci->headTeamSkinAlt = 0;
			ci->torsoTeamSkinAlt = 0;
			ci->legsTeamSkinAlt = 0;
		}
		// reset since cg.ourModel can fallback to using original player models
		cg.ourModel.headTeamSkinAlt = 0;
		cg.ourModel.torsoTeamSkinAlt = 0;
		cg.ourModel.legsTeamSkinAlt = 0;
		cg.ourModelRed.headTeamSkinAlt = 0;
		cg.ourModelRed.torsoTeamSkinAlt = 0;
		cg.ourModelRed.legsTeamSkinAlt = 0;
		cg.ourModelBlue.headTeamSkinAlt = 0;
		cg.ourModelBlue.torsoTeamSkinAlt = 0;
		cg.ourModelBlue.legsTeamSkinAlt = 0;

		modc_disallow = cg_disallowEnemyModelForTeammates.modificationCount;
	}
}

#if 0  // unused
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
#endif

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
    int timeDelta;
    float bob;
    float speed;
    float delta;
    qboolean useLastValidBob = qfalse;
    int anim;
	int t;
	qboolean useSmoothing = qfalse;

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


	//FIXME 1 0 ?
	if (cg_demoSmoothing.integer > 2) {
		float f;
		const snapshot_t *old;
		const snapshot_t *new;
		const entityState_t *esNew;
		const entityState_t *esOld;
		int num;
		vec3_t origin;
		int smoothNum;
		int currentNum;
		int nextNum;
		qboolean r;

		useSmoothing = qtrue;

		smoothNum = cg_demoSmoothing.integer;
		//FIXME why?
		if (smoothNum >= PACKET_BACKUP) {
			smoothNum = PACKET_BACKUP - 1;
		}
		currentNum = cg.snap->messageNum - cg.snap->messageNum % (smoothNum - 1);
		nextNum = currentNum + (smoothNum - 1);
		//FIXME getting snapshots every frame instead of at snapshot transition
		r = CG_GetSnapshot(currentNum, &cg.smoothOldSnap);
		if (!r) {
			//Com_Printf("^3smooth couldn't get old snapshot %d\n", currentNum);
			useSmoothing = qfalse;
		}
		r = CG_PeekSnapshot(nextNum, &cg.smoothNewSnap);
		if (!r) {
			//Com_Printf("^3smooth couldn't get next snapshot %d\n", nextNum);
			useSmoothing = qfalse;
		}
		old = &cg.smoothOldSnap;
		new = &cg.smoothNewSnap;

		//esOld = &cg_entities[wcg.clientNum].currentState;
		esOld = NULL;
		esNew = NULL;

		if (useSmoothing) {
			for (num = 0;  num < new->numEntities;  num++) {
				if (old->entities[num].number == wcg.clientNum) {
					esOld = &old->entities[num];
					break;
				}
			}

			for (num = 0;  num < new->numEntities;  num++) {
				if (new->entities[num].number == wcg.clientNum) {
					esNew = &new->entities[num];
					break;
				}
			}
		}

		if (esNew == NULL  ||  esOld == NULL) {
			useSmoothing = qfalse;
			//Com_Printf("^6not in new snapshot\n");
		} else {
			//Com_Printf("^5got new snap\n");
			if (cg_demoSmoothingTeleportCheck.integer  &&  ((esOld->eFlags ^ esNew->eFlags) & EF_TELEPORT_BIT)) {
				//Com_Printf("^2teleport, skipping\n");
				useSmoothing = qfalse;
			}
			f = (cg.ftime - (double)old->serverTime) / (double)(new->serverTime - old->serverTime);

			origin[0] = esOld->pos.trBase[0] + f * (esNew->pos.trBase[0] - esOld->pos.trBase[0]);
			origin[1] = esOld->pos.trBase[1] + f * (esNew->pos.trBase[1] - esOld->pos.trBase[1]);
			origin[2] = esOld->pos.trBase[2] + f * (esNew->pos.trBase[2] - esOld->pos.trBase[2]);
			VectorCopy(origin, cg.refdef.vieworg);

			if (cg_demoSmoothingAngles.integer) {
				vec3_t angles;

				angles[0] = LerpAngleNear(esOld->apos.trBase[0], esNew->apos.trBase[0], f);
				angles[1] = LerpAngleNear(esOld->apos.trBase[1], esNew->apos.trBase[1], f);
				angles[2] = LerpAngleNear(esOld->apos.trBase[2], esNew->apos.trBase[2], f);
				VectorCopy(angles, cg.refdefViewAngles);
				if (useSmoothing) {
					// for lg beam
					VectorCopy(angles, cent->lerpAngles);
				}
			} else {
				VectorCopy(cent->lerpAngles, cg.refdefViewAngles);
			}
		}
	}

	if (!useSmoothing) {
		VectorCopy(cent->lerpOrigin, cg.refdef.vieworg);
		VectorCopy(cent->lerpAngles, cg.refdefViewAngles);
	}

    // CG_TransitionPlayerState for wcg.clientNum


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

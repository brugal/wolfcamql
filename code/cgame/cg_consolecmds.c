// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../qcommon/q_shared.h"
#include "../game/bg_public.h"
#include "../game/bg_xmlparser.h"

#include "cg_consolecmds.h"
#include "cg_draw.h"  // cg_fade...
#include "cg_ents.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_newdraw.h"
#include "cg_players.h"
#include "cg_playerstate.h"
#include "cg_q3mme_camera.h"
#include "cg_q3mme_scripts.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_consolecmds.h"
#include "wolfcam_view.h"

#include "../ui/ui_shared.h"
#include "wolfcam_local.h"

static void CG_UpdateCameraInfo (void);
static void CG_UpdateCameraInfoExt (int startUpdatePoint);
static void CG_GotoViewPointMark_f (void);
static void CG_ChangeSelectedCameraPoints_f (void);


void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == CROSSHAIR_CLIENT_INVALID ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendClientCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}

/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}

/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	//FIXME maybe viewheight
#if 0
	Com_Printf ("(%f %f %f)   (%f %f %f)\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2] - DEFAULT_VIEWHEIGHT, cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2] );
#endif
	Com_Printf("(%f %f %f) %f %f %f %i\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2] - DEFAULT_VIEWHEIGHT, cg.refdefViewAngles[PITCH], cg.refdefViewAngles[YAW], cg.refdefViewAngles[ROLL], cg.time);
}


void CG_ScoresDown_f( void ) {

#ifdef MISSIONPACK
		CG_BuildSpectatorString();
#endif
	if (cg.demoPlayback) {
		cg.showDemoScores = qtrue;
		cg.showScores = qtrue;
		//return;
	}

	//Com_Printf("scores down\n");
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
		if (cgs.gametype == GT_TOURNAMENT) {
			trap_SendClientCommand("dscores");
		}

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
            if (!cg.demoPlayback)
                cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

void CG_ScoresUp_f( void ) {
	if (cg.demoPlayback) {
		cg.showDemoScores = qfalse;
		cg.showScores = qfalse;
		return;
	}

	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

#if 1  //def MPACK

void Menu_Reset (void);			// FIXME: add to right include file

static void CG_LoadHud_f (void)
{
	char buff[1024];
	const char *hudSet;

	memset(buff, 0, sizeof(buff));
	String_Init();

	//FIXME hack for fx scripts using String_Alloc()
	//memset(&EffectScripts.jitToken, 0, sizeof(EffectScripts.jitToken));
	CG_FreeFxJitTokens();

	Menu_Reset();

	if (cg_loadDefaultMenus.integer) {
		CG_LoadDefaultMenus();
	}

	if (CG_Argc() < 2) {
		trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
		hudSet = buff;
	} else {
		hudSet = CG_Argv(1);
	}

	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
	//FIXME
	//CG_LoadDefaultMenus();
	//CG_LoadMenus(hudSet);
	cg.menuScoreboard = NULL;
}

#if 1  //def MPACK
static void CG_scrollScoresDown_f( void) {
	if (cg.menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (cg.menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(cg.menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


#ifdef MISSIONPACK
static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	if (cg_audioAnnouncerWin.integer) {
		CG_AddBufferedSound(cgs.media.winnerSound);
	}
	//CG_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	if (cg_audioAnnouncerWin.integer) {
		CG_AddBufferedSound(cgs.media.loserSound);
	}
	//CG_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30, 0);
}
#endif
#endif

#endif

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci;

	if (!cg.snap) {
		return;
	}

	ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (cg_currentSelectedPlayer.integer < 0  ||  cg_currentSelectedPlayer.integer > numSortedTeamPlayers) {
			Com_Printf("^3CG_NextOrder_f:  invalid selected player %d\n", cg_currentSelectedPlayer.integer);
			return;
		}
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5\n");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6\n");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	//FIXME check q3 compatability
	trap_SendConsoleCommand("cmd vsay kill_gauntlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

#endif

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		//return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

// camera script
/*
==============
CG_StartidCamera
==============
*/
void CG_StartidCamera( const char *name, qboolean startBlack ) {
	char lname[MAX_QPATH];

	if (cg.cameraMode) {
		//CG_Printf("^1camera already running\n");
		//return;
	}

	COM_StripExtension(name, lname, sizeof(lname));
	Q_strcat( lname, sizeof(lname), ".camera" );
	if (trap_loadCamera(va("cameras/%s", lname)))
		{
			Com_Printf("loaded camera\n");
			cg.cameraMode = qtrue;
			if(startBlack)
				{
					CG_Fade(255, 0, 0);	// go black
					CG_Fade(0, cg.realTime, 1500);
				}
			//
			// letterbox look
			//
			//black_bars=1;  //FIXME
			trap_startCamera(cg.realTime);	// camera on in client
		} else {
		Com_Printf ("^1Unable to load camera %s\n",name);
		cg.cameraMode = qfalse;
	}
}

static void CG_idCamera_f( void ) {
	char name[MAX_QPATH];
	trap_Argv( 1, name, sizeof(name));
	CG_StartidCamera(name, qfalse );
}

static void CG_idCameraStop_f( void ) {
	//FIXME this can lead to crash
	cg.cameraMode = qfalse;
}

// end camera script


typedef struct {
	char *name;
	int *num;
	timedItem_t *tlist;
} titems_list_t;

static void CG_ListTimedItems_f (void)
{
	int i, j;
	const titems_list_t t[] = {
		{ "red armor", &cg.numRedArmors, cg.redArmors },
		{ "yellow armor", &cg.numYellowArmors, cg.yellowArmors },
		{ "green armor", &cg.numGreenArmors, cg.greenArmors },
		{ "mega health", &cg.numMegaHealths, cg.megaHealths },
		{ "quad", &cg.numQuads, cg.quads },
		{ "battle suit", &cg.numBattleSuits, cg.battleSuits },
	};

	for (i = 0;  i < ARRAY_LEN(t);  i++) {
		Com_Printf("%s:  %d\n", t[i].name, cg.time);
		for (j = 0;  j < *t[i].num;  j++) {
			Com_Printf("(%f %f %f)  respawn %d  contents: 0x%x  lastPickupTime: %d\n", t[i].tlist[j].origin[0], t[i].tlist[j].origin[1], t[i].tlist[j].origin[2], t[i].tlist[j].respawnLength, trap_CM_PointContents(t[i].tlist[j].origin, 0), t[i].tlist[j].pickupTime);
		}
	}
}

static void CG_LoadMenu_f (void)
{
	char filename[1024];

	if (CG_Argc() < 2) {
		Com_Printf("usage:  loadmenu <menufile>\n");
		return;
	}

	trap_Argv( 1, filename, sizeof(filename));
	CG_ParseMenu(filename);
}

static void CG_FreeCam_f (void)
{
	int argc;
	vec3_t forward, right, up;
	float f, r, u;

	if (!cg.snap) {
		return;
	}

	if (!Q_stricmp("help", CG_Argv(1))) {
		Com_Printf("usage:  freecam [offset | move | set | last] ...\n");
		Com_Printf("                 default is offset\n\n");
		Com_Printf("        freecam offset [x offset] [y offset] [z offset] [pitch offset] [yaw offset] [roll offset]\n");
		return;
	}

	argc = CG_Argc();
	cg.freecam = !cg.freecam;

	if (cg.freecam) {
		if (!Q_stricmp("last", CG_Argv(1))) {
			if (cg.freecamSet) {
				return;
			} // else -- default to offset
		}
		if (wolfcam_following) {
			VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, cg.fpos);
			VectorCopy(cg_entities[wcg.clientNum].lerpAngles, cg.fang);
		} else {
			VectorCopy(cg_entities[cg.snap->ps.clientNum].lerpOrigin, cg.fpos);
			VectorCopy(cg_entities[cg.snap->ps.clientNum].lerpAngles, cg.fang);
		}
		wolfcam_following = 0;

		VectorCopy(cg.refdef.vieworg, cg.fpos);
		VectorCopy(cg.refdefViewAngles, cg.fang);

		cg.fpos[2] -= DEFAULT_VIEWHEIGHT;

		if (!Q_stricmp("move", CG_Argv(1))) {
			f = atof(CG_Argv(2));
			r = atof(CG_Argv(3));
			u = atof(CG_Argv(4));

			AngleVectors(cg.fang, forward, right, up);

			VectorMA(cg.fpos, f, forward, cg.fpos);
			VectorMA(cg.fpos, r, right, cg.fpos);
			VectorMA(cg.fpos, u, up, cg.fpos);

			cg.fang[0] += atof(CG_Argv(5));
			cg.fang[1] += atof(CG_Argv(6));
			cg.fang[2] += atof(CG_Argv(7));

		} else if (!Q_stricmp("set", CG_Argv(1))) {
			if (argc > 2) {
				cg.fpos[0] = atof(CG_Argv(1));
			}
			if (argc > 3) {
				cg.fpos[1] = atof(CG_Argv(2));
			}
			if (argc > 4) {
				cg.fpos[2] = atof(CG_Argv(3));
			}

			if (argc > 5) {
				cg.fang[0] = atof(CG_Argv(4));
			}
			if (argc > 6) {
				cg.fang[1] = atof(CG_Argv(5));
			}
			if (argc > 7) {
				cg.fang[2] = atof(CG_Argv(6));
			}
		} else if (!Q_stricmp("offset", CG_Argv(1))) {
			cg.fpos[0] += atof(CG_Argv(2));
			cg.fpos[1] += atof(CG_Argv(3));
			cg.fpos[2] += atof(CG_Argv(4));

			cg.fang[0] += atof(CG_Argv(5));
			cg.fang[1] += atof(CG_Argv(6));
			cg.fang[2] += atof(CG_Argv(7));
		} else {  // offset
			cg.fpos[0] += atof(CG_Argv(1));
			cg.fpos[1] += atof(CG_Argv(2));
			cg.fpos[2] += atof(CG_Argv(3));

			cg.fang[0] += atof(CG_Argv(4));
			cg.fang[1] += atof(CG_Argv(5));
			cg.fang[2] += atof(CG_Argv(6));
		}

		cg.mousex = 0;
		cg.mousey = 0;
		cg.fMoveTime = 0;
		cg.keyu = cg.keyd = cg.keyf = cg.keyb = cg.keyr = cg.keyl = cg.keya = 0;
		VectorSet(cg.fvelocity, 0, 0, 0);
		cg.freecamSet = qtrue;
		trap_SendConsoleCommand("exec freecam.cfg\n");
	} else {
		if (wolfcam_following) {
			trap_SendConsoleCommand("exec follow.cfg\n");
		} else {
			if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
				trap_SendConsoleCommand("exec spectator.cfg\n");
			} else if (cg.snap  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
				trap_SendConsoleCommand("exec spectator.cfg\n");
			} else {
				trap_SendConsoleCommand("exec ingame.cfg\n");
			}
		}
	}
}

static void CG_SetViewPos_f (void)
{
	if (CG_Argc() < 4) {
		Com_Printf("need to provide origin (x y z)\n");
		Com_Printf("usage: setviewpos x y z\n");
		Com_Printf("optional: setviewpos x y z  pitch yaw roll   to also set angles\n");
		return;
	}
	//FIXME reset
	cg.fMoveTime = 0;
	cg.fpos[0] = atof(CG_Argv(1));
	cg.fpos[1] = atof(CG_Argv(2));
	cg.fpos[2] = atof(CG_Argv(3));
	cg.fpos[2] -= DEFAULT_VIEWHEIGHT;

	if (CG_Argc() < 7) {
		//Com_Printf("not setting angles\n");
		return;
	}

	cg.fang[0] = atof(CG_Argv(4));
	cg.fang[1] = atof(CG_Argv(5));
	cg.fang[2] = atof(CG_Argv(6));
}

static void CG_SetViewAngles_f (void)
{
	int argc;
	//vec3_t forward, right, up;

	if (CG_Argc() < 2) {
		Com_Printf("usage: setviewangles [pitch] [yaw] [roll]\n");
		return;
	}

	argc = CG_Argc();
	cg.fMoveTime = 0;

#if 0
	if (!Q_stricmp(CG_Argv(1), "flip")) {
		//cg.fang[PITCH] = AngleNormalize180(cg.fang[PITCH] + 180);
		//cg.fang[YAW] = AngleNormalize180(cg.fang[YAW] + 180);
		AngleVectors(cg.fang, forward, right, up);
		return;
	}
#endif

	if (argc > 1) {
		cg.fang[0] = atof(CG_Argv(1));
	}
	if (argc > 2) {
		cg.fang[1] = atof(CG_Argv(2));
	}
	if (argc > 3) {
		cg.fang[2] = atof(CG_Argv(3));
	}
}

// in milliseconds
int CG_AdjustTimeForTimeouts (int timeLength, qboolean forward)
{
	int i;
	int startServerTime;
	int endServerTime;
	const timeOut_t *t;

	if (forward) {
		if (cg.snap) {
			startServerTime = cg.snap->serverTime;
		} else {
			startServerTime = trap_GetFirstServerTime();
		}

		endServerTime = startServerTime + timeLength;

		//Com_Printf("^5start end: %d  %d  %d\n", startServerTime, endServerTime, timeLength);

		for (i = 0;  i < cgs.numTimeouts;  i++) {
			t = &cgs.timeOuts[i];

			//Com_Printf("^3 timeout %d  %d  ->  %d\n", i + 1, t->startTime, t->endTime);
			if (t->endTime < startServerTime) {
				continue;
			}
			if (t->startTime > endServerTime) {
				continue;
			}

			if (t->startTime >= startServerTime  &&  t->endTime <= endServerTime) {
				// timeout contained in interval
				//Com_Printf("%d contained\n", i);
				timeLength += (t->endTime - t->startTime);
			} else if (t->startTime < startServerTime) {
				// started within a timeout, skip remaining time
				//Com_Printf("%d start within\n", i);
				timeLength += (t->endTime - startServerTime);
			} else if (t->endTime > endServerTime) {
				// end inside a timeout
				//Com_Printf("%d end inside  %d > %d\n", i, t->endTime, endServerTime);
				//timeLength += (t->endTime - t->startTime);
				timeLength += (t->endTime - t->startTime);
			} else {
				Com_Printf("^3FIXME CG_AdjustTimeForTimeouts() invalid forward\n");
			}

			//Com_Printf("^3%d  %d -> %d length %f\n", i, t->serverTime, t->endTime, (t->endTime - t->startTime) / 1000.0);
			//Com_Printf("%d new timelength %d\n", i, timeLength);

			if (cg.snap) {
				startServerTime = cg.snap->serverTime;
			} else {
				startServerTime = trap_GetFirstServerTime();
			}
			endServerTime = startServerTime + timeLength;
		}

		return (endServerTime - startServerTime);

	} else {  // rewind
		if (cg.snap) {
			endServerTime = cg.snap->serverTime;
		} else {
			endServerTime = trap_GetFirstServerTime();
		}
		startServerTime = endServerTime - timeLength;

		for (i = cgs.numTimeouts - 1;  i >= 0;  i--) {
			t = &cgs.timeOuts[i];

			if (t->startTime > endServerTime) {
				continue;
			}
			if (t->endTime < startServerTime) {
				continue;
			}

			if (t->startTime >= startServerTime  &&  t->endTime <= endServerTime) {
				// timeout contained in interval
				timeLength += (t->endTime - t->startTime);
			} else if (t->endTime > endServerTime) {
				// start inside a timeout, skip beginning of timeout
				timeLength += (endServerTime - t->startTime);
			} else if (t->startTime < startServerTime) {
				// end within a timeout
				timeLength += (t->endTime - t->startTime);
			} else {
				Com_Printf("^3FIXME CG_AdjustTimeForTimeouts() invalid rewind\n");
			}

			//Com_Printf("^3%d  %d  length %f\n", i, t->serverTime, (t->endTime - t->startTime) / 1000.0);

			//FIXME if you are at a timeout fast forward?

			if (cg.snap) {
				startServerTime = cg.snap->serverTime - timeLength;
				endServerTime = cg.snap->serverTime;
			} else {
				startServerTime = trap_GetFirstServerTime() - timeLength;
				endServerTime = trap_GetFirstServerTime();
			}
		}

		return (endServerTime - startServerTime);
	}
}

static void CG_SeekClock_f (void)
{
	const char *timeString;
	int slen;
	int i;
	int colonCount;
	int clockTimeWanted;
	int currentTime;
	qboolean warmupTime;
	int timeLength;

	if (!cg.demoPlayback) {
		Com_Printf("CG_SeekClock_f() not playing a demo\n");
		return;
	}

	if (CG_Argc() < 2) {
		Com_Printf("usage:  seekclock <game time>\n");
		Com_Printf("example:  seekclock 10:21, seekclock w12:53 to seek to a time within warmup\n");
		return;
	}

	timeString = CG_Argv(1);
	//Com_Printf("%s\n", timeString);
	if (timeString[0] == 'w'  ||  timeString[1] == 'W') {
		if (!cg.demoHasWarmup) {
			Com_Printf("CG_SeekClock_f() demo doesn't have warmup\n");
			return;
		}
		warmupTime = qtrue;
		timeString += 1;
	} else {
		if (trap_GetGameStartTime() < 0) {
			Com_Printf("CG_SeekClock_f() demo doesn't contain a game\n");
			return;
		}
		warmupTime = qfalse;
	}
	slen = strlen(timeString);
	colonCount = 0;
	for (i = 0;  i < slen;  i++) {
		if (timeString[i] == ':') {
			colonCount++;
		}
	}

	if (colonCount == 0) {
		// seconds
		clockTimeWanted = atoi(timeString) * 1000;
	} else {
		clockTimeWanted = Q_ParseClockTime(timeString);
	}

	currentTime = CG_GetCurrentTimeWithDirection(NULL);
	//overTime = CG_NumOverTimes();
	if (cg.warmup) {
		if (!warmupTime) {
			if (cg.snap) {
				currentTime = cg.snap->serverTime - trap_GetGameStartTime();
				//Com_Printf("current time %d  up %d\n", currentTime, cg.timerGoesUp);
			} else {
				currentTime = trap_GetFirstServerTime() - trap_GetGameStartTime();
				//Com_Printf("first server time %d  up %d\n", currentTime, cg.timerGoesUp);
			}
		} else {
			if (cg.snap) {
				currentTime = cg.snap->serverTime - cg.warmupTimeStart;
				//Com_Printf("want warmup -- current time %d\n", currentTime);
			} else {
				currentTime = trap_GetFirstServerTime() - cg.warmupTimeStart;
			}
		}

		//Com_Printf("%d (start)   %d\n", trap_GetGameStartTime(), cg.snap->serverTime);
	} else {
		if (warmupTime) {
			currentTime = cg.snap->serverTime - cg.warmupTimeStart;
		}
	}
	//Com_Printf("want %d\n", clockTimeWanted);

	if (cg.timerGoesUp) {
		if (currentTime > clockTimeWanted) {
			timeLength = currentTime - clockTimeWanted;
			timeLength = CG_AdjustTimeForTimeouts(timeLength, qfalse);
			// using 'now' since other demo seek commands could occur in this
			// frame
			trap_SendConsoleCommandNow(va("rewind %f\n", (double)(timeLength / 1000.0)));
		} else if (currentTime < clockTimeWanted) {
			timeLength = clockTimeWanted - currentTime;
			timeLength = CG_AdjustTimeForTimeouts(timeLength, qtrue);
			// using 'now' since other demo seek commands could occur in this
			// frame
			trap_SendConsoleCommandNow(va("fastforward %f\n", (double)(timeLength / 1000.0)));
		}
	} else {
		if (currentTime > clockTimeWanted) {
			timeLength = currentTime - clockTimeWanted;
			timeLength = CG_AdjustTimeForTimeouts(timeLength, qtrue);
			// using 'now' since other demo seek commands could occur in this
			// frame
			trap_SendConsoleCommandNow(va("fastforward %f\n", (double)(timeLength / 1000.0)));
		} else if (currentTime < clockTimeWanted) {
			timeLength = clockTimeWanted - currentTime;
			timeLength = CG_AdjustTimeForTimeouts(timeLength, qfalse);
			// using 'now' since other demo seek commands could occur in this
			// frame
			trap_SendConsoleCommandNow(va("rewind %f\n", (double)(timeLength / 1000.0)));
		}
	}
}

void CG_ErrorPopup (const char *s)
{
	qboolean messageRepeated = qfalse;

	if (!Q_stricmpn(cg.errorPopupString, s, strlen(s))) {
		messageRepeated = qtrue;
	}
	cg.errorPopupStartTime = cg.realTime;
	cg.errorPopupTime = 3000;  //4000;  //cg_errorPopupTime.integer;
	cg.errorPopupString[0] = '\0';

	Q_strncpyz(cg.errorPopupString, s, sizeof(cg.errorPopupString));
	Com_Printf("^1%s\n", cg.errorPopupString);
	if (!messageRepeated) {
		trap_SendConsoleCommand(va("play sound/weapons/noammo.ogg\n"));
	}
}

static void CG_EchoPopup_f (void)
{
	int i;

	cg.echoPopupStartTime = cg.realTime;
	cg.echoPopupTime = cg_echoPopupTime.integer;
	cg.echoPopupString[0] = '\0';
	i = 1;

	for (i = 1;  i < CG_Argc();  i++) {
		Q_strcat(cg.echoPopupString, sizeof(cg.echoPopupString), va("%s ", CG_Argv(i)));
	}

#if 1
	if (i > 1) {
		// chomp extra space added
		cg.echoPopupString[strlen(cg.echoPopupString) - 1] = '\0';
	}
#endif

	cg.echoPopupX = cg_echoPopupX.value;
	cg.echoPopupY = cg_echoPopupY.value;
	cg.echoPopupWideScreen = cg_echoPopupWideScreen.integer;
	cg.echoPopupScale = cg_echoPopupScale.value;
}

static void CG_EchoPopupClear_f (void)
{
	cg.echoPopupStartTime = 0;
}

static void CG_EchoPopupCvar_f (void)
{
	char buff[MAX_STRING_CHARS];

	if (CG_Argc() < 2) {
		return;
	}

	trap_Cvar_VariableStringBuffer(CG_Argv(1), buff, sizeof(buff));
	if (CG_Argc() > 2  &&  !Q_stricmp("name", CG_Argv(2))) {
		trap_SendConsoleCommand(va("echopopup %s %s\n", CG_Argv(1), buff));
	} else {
		trap_SendConsoleCommand(va("echopopup %s\n", buff));
	}
}

static void CG_AccStatsUp_f (void)
{
	cg.drawAccStats = qfalse;
}

static void CG_AccStatsDown_f (void)
{
	cg.drawAccStats = qtrue;
}

static void CG_ViewEnt_f (void)
{
	int ent;

	if (CG_Argc() < 2) {
		cg.viewEnt = -1;
		//cg.viewUnlockYaw = qfalse;
		//cg.viewUnlockPitch = qfalse;
		cg.useViewPointMark = qfalse;
		//cg.viewPointMarkSet = qfalse;
		return;
	}

	if (!Q_stricmp(CG_Argv(1), "here")) {
		//cg.viewEnt = MAX_GENTITIES;  //FIXME hack
		//VectorCopy(cg.freecamPlayerState.origin, cg_entities[MAX_GENTITIES].lerpOrigin);
		cg.useViewPointMark = qtrue;
		cg.viewPointMarkSet = qtrue;
		VectorCopy(cg.freecamPlayerState.origin, cg.viewPointMarkOrigin);
		return;
	}

	ent = atoi(CG_Argv(1));
	if (ent < 0  ||  ent >= MAX_GENTITIES) {
		ent = -1;
	}
	cg.viewEnt = ent;

	cg.viewEntOffsetX = 0;
	cg.viewEntOffsetY = 0;
	cg.viewEntOffsetZ = 0;

	if (CG_Argc() >= 3) {
		cg.viewEntOffsetX = atof(CG_Argv(2));
	}
	if (CG_Argc() >= 4) {
		cg.viewEntOffsetY = atof(CG_Argv(3));
	}
	if (CG_Argc() >= 5) {
		cg.viewEntOffsetZ = atof(CG_Argv(4));
	}
	Com_Printf("viewent %d  %f %f %f\n", cg.viewEnt, cg.viewEntOffsetX, cg.viewEntOffsetY, cg.viewEntOffsetZ);
}

static void CG_Chase_f (void)
{
	int ent;

	if (CG_Argc() < 2) {
		//cg.viewEnt = -1;
		cg.chaseEnt = -1;
		cg.chase = qfalse;
		//cg.viewUnlockYaw = qfalse;
		//cg.viewUnlockPitch = qfalse;
		return;
	}

	ent = atoi(CG_Argv(1));
	if (ent < 0  ||  ent >= MAX_GENTITIES) {
		//ent = -1;
		cg.chaseEnt = -1;
		cg.chase = qfalse;
		return;
	}
	cg.chaseEnt = ent;
	cg.chase = qtrue;

	cg.chaseEntOffsetX = 0;
	cg.chaseEntOffsetY = 0;
	cg.chaseEntOffsetZ = 0;

	if (CG_Argc() >= 3) {
		cg.chaseEntOffsetX = atof(CG_Argv(2));
	}
	if (CG_Argc() >= 4) {
		cg.chaseEntOffsetY = atof(CG_Argv(3));
	}
	if (CG_Argc() >= 5) {
		cg.chaseEntOffsetZ = atof(CG_Argv(4));
	}
	Com_Printf("chase %d  %f %f %f\n", cg.chaseEnt, cg.chaseEntOffsetX, cg.chaseEntOffsetY, cg.chaseEntOffsetZ);
}

static void CG_ViewUnlockYaw_f (void)
{
	cg.viewUnlockYaw = !cg.viewUnlockYaw;
}

static void CG_ViewUnlockPitch_f (void)
{
	cg.viewUnlockPitch = !cg.viewUnlockPitch;
}

#if 0
static void CG_ViewPos_f (void)
{
	cg.viewEnt = MAX_GENTITIES;  //FIXME hack
	VectorCopy(cg.freecamPlayerState.origin, cg_entities[MAX_GENTITIES].lerpOrigin);
}
#endif

static void CG_TestReplaceShaderImage_f (void)
{
	unsigned char data[16 * 16 * 4];
	int i;

	if (CG_Argc() < 2) {
		return;
	}

	for (i = 0;  i < (16 * 16 * 4);  i++) {
		data[i] = i % 256;
	}

	trap_ReplaceShaderImage(atoi(CG_Argv(1)), data, 16, 16);
}

static void CG_StopMovement_f (void)
{
	VectorSet(cg.freecamPlayerState.velocity, 0, 0, 0);
}

static char *entTypes[] = {
	"general",
	"player",
	"item",
	"missile",
	"mover",
	"beam",
	"portal",
	"speaker",
	"push_trigger",
	"teleport_trigger",
	"invisible",
	"grapple",
	"team",

	"event"
};

static void CG_ListEntities_f (void)
{
	int i;
	const centity_t *cent;
	//int clientNum;
	const entityState_t *ent;
	const char *etypeStr;
	const char *name;
	char weapname[1024];

	for (i = 0;  i < MAX_GENTITIES;  i++) {
		cent = &cg_entities[i];
		if (!cent->currentValid) {
			continue;
		}
		ent = &cent->currentState;
		if (ent->eType < 0) {
			etypeStr = "invalid";
		} else if (ent->eType >= ET_EVENTS) {
			etypeStr = "event";
		} else {
			etypeStr = entTypes[ent->eType];
		}

		if (i < MAX_CLIENTS) {
			name = cgs.clientinfo[i].name;
		} else {
			name = "";
		}
		if (ent->eType == ET_MISSILE) {
			if (ent->weapon >= 0  &&  ent->weapon <= MAX_WEAPONS) {
				Q_strncpyz(weapname, weapNamesCasual[ent->weapon], sizeof(weapname));
			} else {
				Com_Printf("^1CG_ListEntities_f invalid weapon number %d\n", ent->weapon);
				Q_strncpyz(weapname, va("%d", ent->weapon), sizeof(weapname));
			}
		} else {
			Q_strncpyz(weapname, va("%d", ent->weapon), sizeof(weapname));
		}

		Com_Printf("%d %s number %d  clientNum %d weapon %s %s\n", i, etypeStr, ent->number, ent->clientNum, weapname, name);
	}
}

void CG_PrintEntityState_f (void)
{
	if (CG_Argc() < 2) {
		return;
	}

	if (CG_Argv(1)[0] == 'p'  ||  CG_Argv(1)[0] == 'P') {
		CG_PrintEntityStatep(&cg.predictedPlayerEntity.currentState);
	} else {
		CG_PrintEntityState(atoi(CG_Argv(1)));
	}
}

void CG_PrintNextEntityState_f (void)
{
	if (CG_Argc() < 2) {
		return;
	}

	CG_PrintEntityStatep(&cg_entities[atoi(CG_Argv(1))].nextState);
}

static void CG_ClearFragMessage_f (void)
{
	cg.lastFragTime = 0;
	cg.lastFragString[0] = '\0';
}

static void CG_GotoView_f (void)
{
	vec3_t forward, right, up;
	float f, r, u;
	const centity_t *cent;
	qboolean force;

	if (!cg.snap) {
		return;
	}

	if (!cg.freecam) {
		return;
	}

	if (cg.useViewPointMark) {
		CG_GotoViewPointMark_f();
		return;
	}

	f = r = u = 1;
	force = qfalse;

	if (cg.viewEnt > -1) {
		cent = &cg_entities[cg.viewEnt];
	} else {
		int clientNum;

		if (wolfcam_following) {
			clientNum = wcg.clientNum;
		} else {
			clientNum = cg.snap->ps.clientNum;
		}
		cent = &cg_entities[clientNum];
		cg.freecamAnglesSet = qtrue;
	}

	if (CG_Argc() >= 2) {
		f = atof(CG_Argv(1));
	}
	if (CG_Argc() >= 3) {
		r = atof(CG_Argv(2));
	}
	if (CG_Argc() >= 4) {
		u = atof(CG_Argv(3));
	}
	if (CG_Argc() >= 5) {
		force = atoi(CG_Argv(4));
	}

	Com_Printf("%f %f %f %d  %d\n", f, r, u, force, CG_Argc());
	VectorCopy(cent->lerpOrigin, cg.freecamPlayerState.origin);
	cg.freecamPlayerState.origin[2] -= DEFAULT_VIEWHEIGHT;
	VectorCopy(cent->lerpAngles, cg.freecamPlayerState.viewangles);
	AngleVectors(cent->lerpAngles, forward, right, up);
	VectorMA(cg.freecamPlayerState.origin, f, forward, cg.freecamPlayerState.origin);
	VectorMA(cg.freecamPlayerState.origin, r, right, cg.freecamPlayerState.origin);
	VectorMA(cg.freecamPlayerState.origin, u, up, cg.freecamPlayerState.origin);
	if (!force) {
		CG_AdjustOriginToAvoidSolid(cg.freecamPlayerState.origin, cent);
	}
}

static void CG_ServerTime_f (void)
{
	if (!cg.snap) {
		return;
	}

	Com_Printf("serverTime: %d\n", cg.snap->serverTime);
}

#if 0
static void CG_PrintScores_f (void)
{
	int i;
	const score_t *sc;
	const clientInfo_t *ci;

	for (i = 0;  i < cg.numScores;  i++) {
		sc = &cg.scores[i];
		ci = &cgs.clientinfo[sc->client];
		Com_Printf("%d %s %d %d %d\n", sc->client, ci->name, ci->score, sc->team, ci->team);
	}
}
#endif

#define pp(x) Com_Printf(#x ": %d\n", s->x)
#define ppf(x) Com_Printf(#x ": %f\n", s->x)

static void CG_DumpStats_f (void)
{
	qboolean fallbackToOldScores = qtrue;
	int i;
	int j;

	Com_Printf("game stats:\n\n");

	if (cgs.gametype == GT_TOURNAMENT  &&  cg.duelScoresValid) {
		fallbackToOldScores = qfalse;

		for (i = 0;  i < 2;  i++) {
			const duelScore_t *s;
			const duelScore_t *sbackup;
			int clientNum;

			if (i == 0) {
				s = &cg.duelScores[0];
				clientNum = cg.duelPlayer1;
			} else {
				s = &cg.duelScores[1];
				clientNum = cg.duelPlayer2;
			}

			Com_Printf("\n");
			Com_Printf("client: %d\n", clientNum);
			Com_Printf("name: %s\n", s->ci.name);
			pp(ping);
			pp(kills);
			pp(deaths);
			pp(accuracy);
			pp(damage);
			pp(redArmorPickups);
			ppf(redArmorTime);
			pp(yellowArmorPickups);
			ppf(yellowArmorTime);
			pp(greenArmorPickups);
			ppf(greenArmorTime);
			pp(megaHealthPickups);
			ppf(megaHealthTime);
			pp(awardExcellent);
			pp(awardImpressive);
			pp(awardHumiliation);

			sbackup = s;
			for (j = 1;  j < WP_NUM_WEAPONS;  j++) {
				const duelWeaponStats_t *s = &sbackup->weaponStats[j];

				Com_Printf("weapon %d\n", j);
				Com_Printf("weapon name: %s\n", weapNamesCasual[j]);
				pp(hits);
				pp(atts);
				pp(accuracy);
				pp(damage);
				pp(kills);
			}
		}
	} else {
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			const caStats_t *s = &cg.caStats[i];

			if (!s->valid) {
				continue;
			}
			if (!cgs.clientinfo[s->clientNum].infoValid) {
				continue;
			}

			fallbackToOldScores = qfalse;
			Com_Printf("\n");
			Com_Printf("client: %d\n", s->clientNum);
			Com_Printf("name: %s\n", cgs.clientinfo[s->clientNum].name);
			pp(damageDone);
			pp(damageReceived);
			pp(gauntKills);
			pp(mgAccuracy);
			pp(mgKills);
			pp(sgAccuracy);
			pp(sgKills);
			pp(glAccuracy);
			pp(glKills);
			pp(rlAccuracy);
			pp(rlKills);
			pp(lgAccuracy);
			pp(lgKills);
			pp(rgAccuracy);
			pp(rgKills);
			pp(pgAccuracy);
			pp(pgKills);
		}

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			const tdmStats_t *s;

			s = &cg.tdmStats[i];

			if (!s->valid) {
				continue;
			}
			if (!cgs.clientinfo[s->clientNum].infoValid) {
				continue;
			}

			fallbackToOldScores = qfalse;
			Com_Printf("\n");
			Com_Printf("client: %d\n", s->clientNum);
			Com_Printf("name: %s\n", cgs.clientinfo[s->clientNum].name);
			pp(selfKill);
			pp(tks);
			pp(tkd);
			pp(damageDone);
			pp(damageReceived);
			pp(ra);
			pp(ya);
			pp(ga);
			pp(mh);
			pp(quad);
			pp(bs);

			//FIXME per weapon stats
		}

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			const ctfStats_t *s;

			s = &cg.ctfStats[i];

			if (!s->valid) {
				continue;
			}
			if (!cgs.clientinfo[s->clientNum].infoValid) {
				continue;
			}

			fallbackToOldScores = qfalse;
			Com_Printf("\n");
			Com_Printf("client: %d\n", s->clientNum);
			Com_Printf("name: %s\n", cgs.clientinfo[s->clientNum].name);
			pp(selfKill);
			pp(damageDone);
			pp(damageReceived);
			pp(ra);
			pp(ya);
			pp(ga);
			pp(mh);
			pp(quad);
			pp(bs);
			pp(regen);
			pp(haste);
			pp(invis);
		}
	}

	if (!fallbackToOldScores) {
		return;
	}

	for (i = 0;  i < cg.numScores;  i++) {
		const score_t *s;

		s = &cg.scores[i];
		if (!cgs.clientinfo[s->client].infoValid) {
			continue;
		}

		Com_Printf("\n");
		Com_Printf("client: %d\n", s->client);
		Com_Printf("name: %s\n", cgs.clientinfo[s->client].name);
		pp(score);
		pp(ping);
		pp(time);
		pp(scoreFlags);
		// powerups?
		pp(accuracy);
		pp(impressiveCount);
		pp(excellentCount);
		pp(gauntletCount);
		pp(defendCount);
		pp(assistCount);
		pp(captures);
		pp(perfect);
		pp(team);
		// alive?
		pp(frags);
		pp(deaths);
		pp(bestWeapon);
		pp(net);

	}
}
#undef pp
#undef ppf

static void CG_SetLoopStart_f (void)
{
	if (!cg.snap) {
		return;
	}

	if (CG_Argc() > 1) {
		cg.loopStartTime = atoi(CG_Argv(1));
	} else {
		cg.loopStartTime = cg.snap->serverTime;
	}
}

static void CG_SetLoopEnd_f (void)
{
	if (!cg.snap) {
		return;
	}

	if (CG_Argc() > 1) {
		cg.loopEndTime = atoi(CG_Argv(1));
	} else {
		cg.loopEndTime = cg.snap->serverTime;
	}
}

static void CG_Loop_f (void)
{
	if (CG_Argc() > 2) {
		cg.loopStartTime = atoi(CG_Argv(1));
		cg.loopEndTime = atoi(CG_Argv(2));
	}

	cg.looping = !cg.looping;
}

static void CG_FragForward_f (void)
{
	int serverTime;

	if (CG_Argc() > 1  &&  !Q_stricmp("stop", CG_Argv(1))) {
		Com_Printf("fragforward off\n");
		cg.fragForwarding = qfalse;
		return;
	}

	if (!cg.snap) {
		//Com_Printf("fragforward off...  demo hasn't started yet");
		serverTime = 0;
		//return;
	} else {
		serverTime = cg.snap->serverTime;
	}

	cg.fragForwarding = !cg.fragForwarding;

	Com_Printf("fragforward %s\n", cg.fragForwarding ? "on" : "off");

	if (!cg.fragForwarding) {
		//Com_Printf("not fast forwarding to frags\n");
		return;
	}

	if (cg.fragForwarding) {
		if (wolfcam_following) {
			//wolfcam_following = qfalse;
		}
		cg.fragForwardFragCount = 0;
		wcg.nextVictimServerTime = serverTime - 200;
	}

	if (CG_Argc() > 1) {
		cg.fragForwardPreKillTime = atof(CG_Argv(1)) * 1000;
	} else {
		cg.fragForwardPreKillTime = 5000;
	}

	if (CG_Argc() > 2) {
		cg.fragForwardDeathHoverTime = atof(CG_Argv(2)) * 1000;
	} else {
		cg.fragForwardDeathHoverTime = 3000;
	}
}

#if 0
static void CG_LoadModels_f (void)
{
	EM_Loaded = 0;
}
#endif

static void CG_RecordPath_f (void)
{
	const char *s;
	const char *fname;
	qboolean useDefaultFolder = qtrue;
	//qboolean useDefaultName = qtrue;
	int i;

	if (!cg.snap) {
		return;
	}

	if (cg.playPath  ||  cg.recordPath) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.recordPathFile = 0;
		cg.playPath = qfalse;
		if (cg.recordPath) {
			Com_Printf("done recording path\n");
			//CG_EchoPopupClear_f();
			cg.echoPopupStartTime = 0;
			cg.recordPath = qfalse;
			return;
		}
	}

	cg.recordPath = qtrue;

	if (CG_Argc() > 1) {
		fname = CG_Argv(1);
		for (i = 0;  i < strlen(fname);  i++) {
			if (fname[i] == '/') {
				useDefaultFolder = qfalse;
				break;
			}
		}
	} else {
		fname = DEFAULT_RECORD_PATHNAME;
	}

	//trap_FS_FOpenFile("path.record", &cg.recordPathFile, FS_WRITE);
	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("cameras/%s.record", fname), &cg.recordPathFile, FS_WRITE);
	} else {
		trap_FS_FOpenFile(va("%s.record", fname), &cg.recordPathFile, FS_WRITE);
	}

	if (!cg.recordPathFile) {
		Com_Printf("^1couldn't open %s\n", fname);
		cg.recordPath = qfalse;
		return;
	}

	cg.recordPathLastServerTime = -1;
	s = va("%d %d %f\n", cg.snap->serverTime, trap_Milliseconds(), CG_Cvar_Get("timescale"));
	trap_FS_Write(s, strlen(s), cg.recordPathFile);
}

static void CG_PlayPath_f (void)
{
	int len;
	const char *s;
	//char buffer[1024];
	int i;
	int nve;
	int skipNum;
	const char *fname;
	qboolean useDefaultFolder = qtrue;

	if (cg.cameraPlaying) {
		Com_Printf("can't play path, camera is playing\n");
		return;
	}

	if (cg.cameraQ3mmePlaying) {
		Com_Printf("can't play path, q3mme camera is playing\n");
		return;
	}

	//cg.playPath = !cg.playPath;
	//cg.playPathStarted = qfalse;

#if 0
	if (!cg.playPath) {
		trap_SendConsoleCommand("echopopup play path cancelled\n");
		return;
	}
#endif

#if 0
	if (cg.recordPath) {
		CG_RecordPath_f();
	}
#endif

	if (cg.recordPath  ||  cg.playPath) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.recordPathFile = 0;
		if (cg.recordPath) {
			Com_Printf("done recording path\n");
			//CG_EchoPopupClear_f();
			cg.echoPopupStartTime = 0;
			cg.recordPath = qfalse;
		}
	}

	if (CG_Argc() > 1) {
		fname = CG_Argv(1);
		for (i = 0;  i < strlen(fname);  i++) {
			if (fname[i] == '/') {
				useDefaultFolder = qfalse;
				break;
			}
		}
	} else {
		fname = DEFAULT_RECORD_PATHNAME;
	}

	cg.playPath = qtrue;

	//trap_FS_FOpenFile("path.record", &cg.recordPathFile, FS_READ);
	if (useDefaultFolder) {
		//trap_FS_FOpenFile(va("cameras/%s.record", fname), &cg.recordPathFile, FS_WRITE);
		trap_FS_FOpenFile(va("cameras/%s.record", fname), &cg.recordPathFile, FS_READ);
	} else {
		//trap_FS_FOpenFile(va("%s.record", fname), &cg.recordPathFile, FS_WRITE);
		trap_FS_FOpenFile(va("%s.record", fname), &cg.recordPathFile, FS_READ);
	}

	if (!cg.recordPathFile) {
		Com_Printf("^1couldn't open %s\n", fname);
		cg.playPath = qfalse;
		return;
	}
	s = CG_FS_ReadLine(cg.recordPathFile, &len);
	if (len == 0  ||  !s) {
		Com_Printf("^1%s is empty\n", fname);
		cg.playPath = qfalse;
		return;
	}

	Com_Printf("line: %s\n", s);
	cg.playPath = qtrue;
	sscanf(s, "%d %d %f", &cg.pathServerTimeStart, &cg.pathRealTimeStart, &cg.pathTimescale);
	cg.pathCGTimeStart = cg.time;
	//cg.freecam = qtrue;
	if (!cg.freecam) {
		CG_FreeCam_f();
	}
	//cg.playPathCount = 0;
	//cg.playPathCommandStartServerTime = cg.snap->serverTime;
#if 0
	if (CG_Argc() > 1) {
		cg.playPathSkipNum = atoi(CG_Argv(1));
	} else {
		cg.playPathSkipNum = 1;
	}
	if (cg.playPathSkipNum < 1) {
		cg.playPathSkipNum = 1;
	}
#endif

	//cg.pathNextServerTime = 0;

	s = CG_FS_ReadLine(cg.recordPathFile, &len);
	if (!s  ||  !*s  ||  len == 0) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.playPath = qfalse;
		//cg.playPathStarted = qfalse;
		//rt = 0;
		Com_Printf("^1error getting first path point\n");
		return;
	}
	sscanf(s, "%lf %f %f %f %f %f %f %d", &cg.pathCurrentTime, &cg.pathCurrentOrigin[0], &cg.pathCurrentOrigin[1], &cg.pathCurrentOrigin[2], &cg.pathCurrentAngles[0], &cg.pathCurrentAngles[1], &cg.pathCurrentAngles[2], &nve);  //FIXME nve

	skipNum = cg_pathSkipNum.integer;
	if (skipNum <= 0) {
		skipNum = 1;
	}

	for (i = 0;  i < skipNum;  i++) {
		s = CG_FS_ReadLine(cg.recordPathFile, &len);
	}
	if (!s  ||  !*s  ||  len == 0) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.playPath = qfalse;
		Com_Printf("^1error getting second path point\n");
		return;
	}
	sscanf(s, "%lf %f %f %f %f %f %f %d", &cg.pathNextTime, &cg.pathNextOrigin[0], &cg.pathNextOrigin[1], &cg.pathNextOrigin[2], &cg.pathNextAngles[0], &cg.pathNextAngles[1], &cg.pathNextAngles[2], &nve);
	//cg.playPathCount = 0;

	// -150 hack for hack of reading an extra snapshot when seeking in order
	// to draw entities
	//trap_SendConsoleCommand(va("seekservertime %f\n", (double)(cg.pathServerTimeStart - 150)));
	trap_SendConsoleCommand(va("seekservertime %f\n", (double)(cg.pathCurrentTime - cg_pathRewindTime.value)));
}

static void CG_StopPlayPath_f (void)
{
	if (!cg.playPath) {
		return;
	}

	cg.playPath = qfalse;
	trap_FS_FCloseFile(cg.recordPathFile);
	cg.recordPathFile = 0;
	Com_Printf("stop playing path\n");
}

static void CG_CenterRoll_f (void)
{
	cg.fang[ROLL] = 0;
}

static void CG_DrawRawPath_f (void)
{
	qhandle_t f;
	int len;
	const char *s = NULL;
	int linesRead;
	rawCameraPathKeyPoint_t *c;
	int nve;
	const char *fname;
	qboolean useDefaultFolder = qtrue;
	int i;

	if (cg.playPath) {
		Com_Printf("need to stop playback before drawing\n");
		//FIXME is it really needed?  just in case open file returns same handle
		return;
	}
	if (cg.recordPath) {
		Com_Printf("need to stop path recording before drawing\n");
		return;
	}

	if (cg.numRawCameraPoints) {
		cg.numRawCameraPoints = 0;
		return;
	}

	if (CG_Argc() > 1) {
		fname = CG_Argv(1);
		for (i = 0;  i < strlen(fname);  i++) {
			if (fname[i] == '/') {
				useDefaultFolder = qfalse;
				break;
			}
		}
	} else {
		fname = DEFAULT_RECORD_PATHNAME;
	}

	//trap_FS_FOpenFile("path.record", &f, FS_READ);
	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("cameras/%s.record", fname), &f, FS_READ);
	} else {
		trap_FS_FOpenFile(va("%s.record", fname), &f, FS_READ);
	}

	if (!f) {
		Com_Printf("^1couldn't open %s\n", fname);
		return;
	}

	if (cg.numRawCameraPoints) {
		cg.numRawCameraPoints = 0;
		return;
	}

	cg.numRawCameraPoints = 0;
	linesRead = 0;
	CG_FS_ReadLine(f, &len);  // start time, timescale, etc... unused

	while (1) {
		s = CG_FS_ReadLine(f, &len);
		if (len == 0  ||  !s  ||  !*s) {
			break;
		}
		if (cg.numRawCameraPoints >= MAX_RAWCAMERAPOINTS) {
			break;
		}
		if (linesRead % 20 == 0) {  //FIXME 20
			c = &cg.rawCameraPoints[cg.numRawCameraPoints];
			cg.numRawCameraPoints++;
			//Com_Printf("%d line: '%s'", linesRead / 20, s);
			//sscanf(s, "%d %d %f", &cg.pathServerTimeStart, &cg.pathRealTimeStart, &cg.pathTimescale);
			sscanf(s, "%lf %f %f %f %f %f %f %d", &cg.pathNextTime, &c->origin[0], &c->origin[1], &c->origin[2], &c->angles[0], &c->angles[1], &c->angles[2], &nve);
		}
		linesRead++;
	}

	trap_FS_FCloseFile(f);
}


//FIXME vid restart
static cameraPoint_t *LastAddedCameraPointPtr = NULL;
static qboolean LastAddedCameraPointSet = qfalse;

static void CG_AddCameraPoint_f (void)
{
	cameraPoint_t *cp;
	int i;
	int j;
	qboolean incrementCameraPoints;
	qboolean lastAdded;
	int cameraPointNum;

	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	lastAdded = qfalse;
	incrementCameraPoints = qtrue;
	cp = NULL;
	if (cg.atCameraPoint) {
		// we will be modifying the camera point, not adding a new one
		incrementCameraPoints = qfalse;
		cp = &cg.cameraPoints[cg.selectedCameraPointMin];
		cameraPointNum = cg.selectedCameraPointMin;
		//Com_Printf("at camera point %d\n", cameraPointNum);
	} else {
		for (i = 0;  i < cg.numCameraPoints;  i++) {
			//FIXME broken, kept for paused camera point add...
			if ((double)cg.ftime == cg.cameraPoints[i].cgtime) {
				// we will be modifying the camera point, not adding a new one
				incrementCameraPoints = qfalse;
				cp = &cg.cameraPoints[i];
				cameraPointNum = i;
				break;
			}

			if ((double)cg.ftime < cg.cameraPoints[i].cgtime) {
				if (cg.numCameraPoints >= MAX_CAMERAPOINTS - 3) {  // three fake ones
					Com_Printf("too many camera points\n");
					return;
				}
				for (j = cg.numCameraPoints - 1;  j >= i;  j--) {
					memcpy(&cg.cameraPoints[j + 1], &cg.cameraPoints[j], sizeof(cameraPoint_t));
				}
				cp = &cg.cameraPoints[i];
				cameraPointNum = i;
				break;
			}
		}
	}

	if (!cp) {
		if (cg.numCameraPoints >= MAX_CAMERAPOINTS - 3) {  // three fake ones
			Com_Printf("too many camera points\n");
			return;
		}
		cp = &cg.cameraPoints[cg.numCameraPoints];
		cameraPointNum = cg.numCameraPoints;
		lastAdded = qtrue;
	}

	if (cg_cameraAddUsePreviousValues.integer  &&  cg.numCameraPoints > 0  &&  LastAddedCameraPointSet) {
		*cp = *LastAddedCameraPointPtr;
	} else {
		memset(cp, 0, sizeof(*cp));
	}

	VectorCopy(cg.refdef.vieworg, cp->origin);
	VectorCopy(cg.refdefViewAngles, cp->angles);
	cp->cgtime = cg.ftime;

	if (!cg_cameraAddUsePreviousValues.integer  ||  cg.numCameraPoints == 0  ||  LastAddedCameraPointSet == qfalse) {
		char *type;

		type = cg_cameraDefaultOriginType.string;
		if (!Q_stricmp(type, "spline")) {
			cp->type = CAMERA_SPLINE;
		} else if (!Q_stricmp(type, "interp")) {
			cp->type = CAMERA_INTERP;
		} else if (!Q_stricmp(type, "jump")) {
			cp->type = CAMERA_JUMP;
		} else if (!Q_stricmp(type, "curve")) {
			cp->type = CAMERA_CURVE;
		} else if (!Q_stricmp(type, "splinebezier")) {
			cp->type = CAMERA_SPLINE_BEZIER;
		} else if (!Q_stricmp(type, "splinecatmullrom")) {
			cp->type = CAMERA_SPLINE_CATMULLROM;
		} else {
			cp->type = CAMERA_SPLINE_BEZIER;
		}
		cp->viewType = CAMERA_ANGLES_SPLINE;
		cp->rollType = CAMERA_ROLL_AS_ANGLES;
		cp->splineType = SPLINE_FIXED;
		cp->numSplines = DEFAULT_NUM_SPLINES;
		cp->viewEnt = -1;
		cp->fov = -1;
		cp->fovType = CAMERA_FOV_USE_CURRENT;

		cp->flags = CAM_ORIGIN | CAM_ANGLES | CAM_FOV;
	}

	LastAddedCameraPointSet = qtrue;

	cp->useOriginVelocity = qfalse;
	cp->useAnglesVelocity = qfalse;
	cp->useXoffsetVelocity = qfalse;
	cp->useYoffsetVelocity = qfalse;
	cp->useZoffsetVelocity = qfalse;
	cp->useFovVelocity = qfalse;
	cp->useRollVelocity = qfalse;

	if (cg.viewEnt > -1) {
		cp->viewType = CAMERA_ANGLES_ENT;
		cp->viewEnt = cg.viewEnt;
		cp->offsetType = CAMERA_OFFSET_INTERP;
		cp->xoffset = cg.viewEntOffsetX;
		cp->yoffset = cg.viewEntOffsetY;
		cp->zoffset = cg.viewEntOffsetZ;
		VectorCopy(cg_entities[cg.viewEnt].lerpOrigin, cp->viewEntStartingOrigin);
		//VectorCopy(cg_entities[cg.viewEnt].currentState.pos.trBase, cp->viewEntStartingOrigin);
		cp->viewEntStartingOriginSet = qtrue;
	}

	if (incrementCameraPoints) {
		cg.numCameraPoints++;
	}

	LastAddedCameraPointPtr = cp;

	if (lastAdded) {
		CG_UpdateCameraInfoExt(cg.numCameraPoints - 1);
	} else {
		CG_UpdateCameraInfo();
	}

	cg.selectedCameraPointMin = cameraPointNum;
	cg.selectedCameraPointMax = cameraPointNum;

	//Com_Printf("add camera point selected : %d\n", cg.selectedCameraPointMin);
}

// not needed, /q3mmecamera add
#if 0
static void CG_AddQ3mmeCameraPoint_f (void)
{
	demoCameraPoint_t *point;
	//point = CG_Q3mmeCameraPointAdd( demo.play.time, demo.camera.flags );
	point = CG_Q3mmeCameraPointAdd(cg.time, CAM_ORIGIN | CAM_ANGLES | CAM_FOV | CAM_TIME);
	if (point) {
		//VectorCopy( demo.viewOrigin, point->origin );
		//VectorCopy( demo.viewAngles, point->angles );
		//point->fov = demo.viewFov - cg_fov.value;
		VectorCopy(cg.refdef.vieworg, point->origin);
		VectorCopy(cg.refdefViewAngles, point->angles);
		point->fov = 0;
		Com_Printf("^6added q3mme camera point\n");
		// just to view the path and points
		//CG_AddCameraPoint_f();
	} else {
		Com_Printf("^1couldn't add q3mme camera point\n");
	}

}
#endif

static void CG_ClearCameraPoints_f (void)
{
	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	cg.numCameraPoints = 0;
	cg.numSplinePoints = 0;
	cg.cameraPointsPointer = NULL;
	cg.selectedCameraPointMin = 0;
	cg.selectedCameraPointMax = 0;
}

static void CG_PlayCamera_f (void)
{
	double extraTime;

	if (cg.playPath) {
		Com_Printf("can't play camera, path is playing\n");
		return;
	}

	if (cg.numCameraPoints < 2) {
		Com_Printf("can't play camera, need at least 2 camera points\n");
		return;
	}

	//FIXME why this dependency ?
	if (cg_cameraQue.integer) {
		extraTime = 1000.0 * cg_cameraRewindTime.value;
		if (extraTime < 0) {
			extraTime = 0;
		}
		trap_SendConsoleCommand(va("seekservertime %f\n", cg.cameraPoints[0].cgtime - extraTime));
	}

	//cg.cameraQ3mmePlaying = qfalse;
	cg.cameraPlaying = qtrue;
	cg.cameraPlayedLastFrame = qfalse;

	cg.currentCameraPoint = 0;
	//cg.cameraWaitToSync = qfalse;  //FIXME stupid
	cg.playCameraCommandIssued = qtrue;
}

static void CG_StopCamera_f (void)
{
	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;
}

static void CG_WriteString (const char *s, qhandle_t file)
{
	trap_FS_Write(s, strlen(s), file);
}

static void CG_SaveCamera_f (void)
{
	fileHandle_t f;
	const char *s;
	int i;
	const cameraPoint_t *cp;
	int len;
	const char *fname;
	qboolean useDefaultFolder = qtrue;

	if (CG_Argc() < 2) {
		Com_Printf("usage: savecamera <filename>\n");
		return;
	}

	if (cg.numCameraPoints < 2) {
		Com_Printf("need at least 2 camera points\n");
		return;
	}

	fname = CG_Argv(1);
	for (i = 0;  i < strlen(fname);  i++) {
		if (fname[i] == '/') {
			useDefaultFolder = qfalse;
			break;
		}
	}

	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("cameras/%s.cam%d", CG_Argv(1), WOLFCAM_CAMERA_VERSION), &f, FS_WRITE);
	} else {
		trap_FS_FOpenFile(va("%s.cam%d", fname, WOLFCAM_CAMERA_VERSION), &f, FS_WRITE);
	}

	if (!f) {
		Com_Printf("^1couldn't create %s.cam%d\n", CG_Argv(1), WOLFCAM_CAMERA_VERSION);
		return;
	}
	s = va("WolfcamCamera %d\n", WOLFCAM_CAMERA_VERSION);
	trap_FS_Write(s, strlen(s), f);

	for (i = 0;  i < cg.numCameraPoints;  i++) {
		cp = &cg.cameraPoints[i];

		CG_WriteString(va("%d  camera point number\n", i), f);
		CG_WriteString(va("%f %f %f  origin\n", cp->origin[0], cp->origin[1], cp->origin[2]), f);
		CG_WriteString(va("%f %f %f  angles\n", cp->angles[0], cp->angles[1], cp->angles[2]), f);
		CG_WriteString(va("%d  type\n", cp->type), f);
		CG_WriteString(va("%d  viewType\n", cp->viewType), f);
		CG_WriteString(va("%d  rollType\n", cp->rollType), f);
		CG_WriteString(va("%d  flags\n", cp->flags), f);
		CG_WriteString(va("%f  cgtime\n", cp->cgtime), f);
		CG_WriteString(va("%d  splineType\n", cp->splineType), f);
		CG_WriteString(va("%d  numSplines\n", cp->numSplines), f);

		CG_WriteString(va("%f %f %f  viewPointOrigin\n", cp->viewPointOrigin[0], cp->viewPointOrigin[1], cp->viewPointOrigin[2]), f);
		CG_WriteString(va("%d  viewPointOriginSet\n", cp->viewPointOriginSet), f);
		CG_WriteString(va("%d  viewEnt\n", cp->viewEnt), f);
		CG_WriteString(va("%f %f %f  viewEntStartingOrigin\n", cp->viewEntStartingOrigin[0], cp->viewEntStartingOrigin[1], cp->viewEntStartingOrigin[2]), f);
		CG_WriteString(va("%d  viewEntStartingOriginSet\n", cp->viewEntStartingOriginSet), f);
		CG_WriteString(va("%d  offsetType\n", cp->offsetType), f);
		CG_WriteString(va("%f  xoffset\n", cp->xoffset), f);
		CG_WriteString(va("%f  yoffset\n", cp->yoffset), f);
		CG_WriteString(va("%f  zoffset\n", cp->zoffset), f);

		CG_WriteString(va("%f  fov\n", cp->fov), f);
		CG_WriteString(va("%d  fovType\n", cp->fovType), f);

		CG_WriteString(va("%d  useOriginVelocity\n", cp->useOriginVelocity), f);
		CG_WriteString(va("%f  originInitialVelocity\n", cp->originInitialVelocity), f);
		CG_WriteString(va("%f  originFinalVelocity\n", cp->originFinalVelocity), f);

		CG_WriteString(va("%d  useAnglesVelocity\n", cp->useAnglesVelocity), f);
		CG_WriteString(va("%f  anglesInitialVelocity\n", cp->anglesInitialVelocity), f);
		CG_WriteString(va("%f  anglesFinalVelocity\n", cp->anglesFinalVelocity), f);

		CG_WriteString(va("%d  useXoffsetVelocity\n", cp->useXoffsetVelocity), f);
		CG_WriteString(va("%f  xoffsetInitialVelocity\n", cp->xoffsetInitialVelocity), f);
		CG_WriteString(va("%f  xoffsetFinalVelocity\n", cp->xoffsetFinalVelocity), f);

		CG_WriteString(va("%d  useYoffsetVelocity\n", cp->useYoffsetVelocity), f);
		CG_WriteString(va("%f  yoffsetInitialVelocity\n", cp->yoffsetInitialVelocity), f);
		CG_WriteString(va("%f  yoffsetFinalVelocity\n", cp->yoffsetFinalVelocity), f);

		CG_WriteString(va("%d  useZoffsetVelocity\n", cp->useZoffsetVelocity), f);
		CG_WriteString(va("%f  zoffsetInitialVelocity\n", cp->zoffsetInitialVelocity), f);
		CG_WriteString(va("%f  zoffsetFinalVelocity\n", cp->zoffsetFinalVelocity), f);

		CG_WriteString(va("%d  useFovVelocity\n", cp->useFovVelocity), f);
		CG_WriteString(va("%f  fovInitialVelocity\n", cp->fovInitialVelocity), f);
		CG_WriteString(va("%f  fovFinalVelocity\n", cp->fovFinalVelocity), f);

		CG_WriteString(va("%d  useRollVelocity\n", cp->useRollVelocity), f);
		CG_WriteString(va("%f  rollInitialVelocity\n", cp->rollInitialVelocity), f);
		CG_WriteString(va("%f  rollFinalVelocity\n", cp->rollFinalVelocity), f);
		len = strlen(cp->command);
		CG_WriteString(va("%d  commandStrLen\n", len), f);
		if (len) {
			CG_WriteString(cp->command, f);
		}

		CG_WriteString("\n-------------------------------------\n", f);
	}
	trap_FS_FCloseFile(f);
	//Com_Printf("camera saved\n");
}

static void CG_CamtraceSave_f (void)
{
	int i;
	const cameraPoint_t *cp;
	char buffer[MAX_STRING_CHARS];
	qboolean useOldFormat;

	if (cg.numCameraPoints < 2) {
		Com_Printf("need at least 2 camera points\n");
		return;
	}

	useOldFormat = qfalse;
	if (CG_Argc() >= 2) {
		if (!Q_stricmp("old", CG_Argv(1))) {
			useOldFormat = qtrue;
			//Com_Printf("old\n");
		}
	}


	// etqw
	// ]freecamgetpos
	// Copying output to clipboard...
	// (x y z) pitch yaw roll time
	// (0 0 0) 0.00 0.00 0.00 137742
	// TODO: Sys_SetClipboardData

	// old format:
	// "clear; viewpos; cg_fov; condump Cam\Pos\pos01.epcp; Echo Cam01;"
	//
	// (2838 7449 -27) : -83
	// "cg_fov" is:"102" default:"90"
	// Dumped console text to Cam\Pos\pos01.epcp.

	for (i = 0;  i < cg.numCameraPoints;  i++) {
		fileHandle_t f;
		int fov;

		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.integer;
		}

		cp = &cg.cameraPoints[i];
		//Com_Printf ("(%i %i w%i) : %i\n", (int)cp->origin[0], (int)cp->origin[1], (int)cp->origin[2], (int)cp->angles[YAW]);

		if (useOldFormat) {
			trap_FS_FOpenFile(va("cameras/ct3d/pos/pos%03d.epcp", i + 1), &f, FS_WRITE);
		} else {
			trap_FS_FOpenFile(va("cameras/ct3d/pos/pos%03d.qwcp", i + 1), &f, FS_WRITE);
		}
		if (!f) {
			Com_Printf("^1couldn't open file for camera position %d\n", i + 1);
			break;
		}
		//Com_sprintf(buffer, sizeof(buffer), "(%i %i %i) : %i\n\"cg_fov\" is:\"%d\" default:\"90\"\nDumped console text to cameras/ct3d/pos/pos%d03d.epcp.\n\n", (int)cp->origin[0], (int)cp->origin[1], (int)cp->origin[2], (int)cp->angles[YAW], (int)cp->fov, i);
		if (useOldFormat) {
			Com_sprintf(buffer, sizeof(buffer), "(%i %i %i) : %i\n\"cg_fov\" is:\"%d\" default:\"90\"\nDumped console text to cameras/ct3d/pos/pos%03d.epcp.\n\n", (int)cp->origin[0], (int)cp->origin[1], (int)cp->origin[2], (int)cp->angles[YAW], cp->fov == -1 ? fov : (int)cp->fov, i);
		} else {
			Com_sprintf(buffer, sizeof(buffer), "Copying output to clipboard...\n(x y z) pitch yaw roll time\n(%f %f %f) %f %f %f %i\n\"g_fov\" is:\"%d\" default:\"90\"\nDumped console text to cameras/ct3d/pos/pos%03d.qwcp.\n\n", cp->origin[0], cp->origin[1], cp->origin[2], cp->angles[PITCH], cp->angles[YAW], cp->angles[ROLL], cg.time, cp->fov == -1 ? fov : (int)cp->fov, i);
		}
		trap_FS_Write(buffer, strlen(buffer), f);
		trap_FS_FCloseFile(f);
	}
}

static void CG_LoadCamera_f (void)
{
	fileHandle_t f;
	int len;
	int version;
	const char *s;
	cameraPoint_t *cp;
	int slen;
	qboolean useDefaultFolder = qtrue;
	const char *fname;
	int i;

	if (CG_Argc() < 2) {
		Com_Printf("usage: loadcamera <camera name>\n");
		return;
	}

	fname = CG_Argv(1);
	for (i = 0;  i < strlen(fname);  i++) {
		if (fname[i] == '/') {
			useDefaultFolder = qfalse;
			break;
		}
	}

	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("cameras/%s.cam%d", CG_Argv(1), WOLFCAM_CAMERA_VERSION), &f, FS_READ);
		if (!f) {
			trap_FS_FOpenFile(va("cameras/%s.cam%d", CG_Argv(1), WOLFCAM_CAMERA_VERSION - 1), &f, FS_READ);
		}
	} else {
		trap_FS_FOpenFile(va("%s.cam%d", fname, WOLFCAM_CAMERA_VERSION), &f, FS_READ);
		if (!f) {
			trap_FS_FOpenFile(va("%s.cam%d", fname, WOLFCAM_CAMERA_VERSION - 1), &f, FS_READ);
		}
	}

	if (!f) {
		Com_Printf("^1couldn't open %s\n", CG_Argv(1));
		return;
	}

	cg.numCameraPoints = 0;
	cg.numSplinePoints = 0;

	s = CG_FS_ReadLine(f, &len);
	//FIXME check name
	sscanf(s + strlen("WolfcamCamera "), "%d", &version);

	while (len) {
		cp = &cg.cameraPoints[cg.numCameraPoints];
		memset(cp, 0, sizeof(*cp));

		s = CG_FS_ReadLine(f, &len);  // camera point number
		if (len) {
			//Com_Printf(va("%s", s));
		} else {
			//  all done
			break;
		}

		cp->version = version;

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%f %f %f", &cp->origin[0], &cp->origin[1], &cp->origin[2]);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%f %f %f", &cp->angles[0], &cp->angles[1], &cp->angles[2]);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->type);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->viewType);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->rollType);

		if (version > 8) {
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", &cp->flags);
		} else {
			cp->flags = CAM_ORIGIN | CAM_ANGLES | CAM_FOV;
		}

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%lf", &cp->cgtime);

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->splineType);

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->numSplines);

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%f %f %f", &cp->viewPointOrigin[0], &cp->viewPointOrigin[1], &cp->viewPointOrigin[2]);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", (int *)&cp->viewPointOriginSet);

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->viewEnt);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%f %f %f", &cp->viewEntStartingOrigin[0], &cp->viewEntStartingOrigin[1], &cp->viewEntStartingOrigin[2]);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", (int *)&cp->viewEntStartingOriginSet);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->offsetType);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%lf", &cp->xoffset);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%lf", &cp->yoffset);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%lf", &cp->zoffset);

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%lf", &cp->fov);
		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &cp->fovType);

		if (version < 10) {
			// never used
			s = CG_FS_ReadLine(f, &len);  // cp->timescale
			s = CG_FS_ReadLine(f, &len);  // cp->timescaleInterp
		}

		if (version > 7) {
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useOriginVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->originInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->originFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useAnglesVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->anglesInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->anglesFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useXoffsetVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->xoffsetInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->xoffsetFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useYoffsetVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->yoffsetInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->yoffsetFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useZoffsetVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->zoffsetInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->zoffsetFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useFovVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->fovInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->fovFinalVelocity);

			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%d", (int *)&cp->useRollVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->rollInitialVelocity);
			s = CG_FS_ReadLine(f, &len);
			sscanf(s, "%lf", &cp->rollFinalVelocity);
		}

		s = CG_FS_ReadLine(f, &len);
		sscanf(s, "%d", &slen);
		if (slen) {
			trap_FS_Read(cp->command, slen, f);
		}

		if (!len) {
			Com_Printf("^1ERROR  corrupt camera file\n");
			cg.numCameraPoints = 0;
			break;
		}

		s = CG_FS_ReadLine(f, &len);  // \n
		s = CG_FS_ReadLine(f, &len);  // -------------------------\n

		cg.numCameraPoints++;
	}
	trap_FS_FCloseFile(f);

	CG_UpdateCameraInfo();
	cg.selectedCameraPointMin = 0;
	cg.selectedCameraPointMax = 0;
	Com_Printf("camera loaded (version %d)\n", version);
}

static void CG_SelectCameraPoint_f (void)
{
	int n;
	const char *s;

	if (CG_Argc() < 2) {
		Com_Printf("usage: selectcamerapoint <point 1> [point 2]\n");
		Com_Printf("       point 1  is a number or it can be [all, first, last, inner]\n");
		return;
	}

	s = CG_Argv(1);
	if (!Q_stricmp(s, "all")) {
		cg.selectedCameraPointMin = 0;
		cg.selectedCameraPointMax = cg.numCameraPoints - 1;
		return;
	} else if (!Q_stricmp(s, "first")) {
		if (CG_Argc() >= 3) {
			cg.selectedCameraPointMin = 0;
		} else {
			cg.selectedCameraPointMin = 0;
			cg.selectedCameraPointMax = 0;
			return;
		}
	} else if (!Q_stricmp(s, "last")) {
		cg.selectedCameraPointMin = cg.numCameraPoints - 1;
		cg.selectedCameraPointMax = cg.numCameraPoints - 1;
		return;
	} else if (!Q_stricmp(s, "inner")) {
		if (cg.numCameraPoints < 3) {
			return;
		}
		cg.selectedCameraPointMin = 1;
		cg.selectedCameraPointMax = cg.numCameraPoints - 2;
	} else {
		n = atoi(CG_Argv(1));
		if (n >= cg.numCameraPoints  ||  n < 0) {
			Com_Printf("invalid camera point\n");
			cg.selectedCameraPointMin = cg.numCameraPoints - 1;
			cg.selectedCameraPointMax = cg.numCameraPoints - 1;
			return;
		}
		cg.selectedCameraPointMin = n;
		cg.selectedCameraPointMax = n;
	}

	if (CG_Argc() >= 3) {
		if (!Q_stricmp(CG_Argv(2), "last")) {
			cg.selectedCameraPointMax = cg.numCameraPoints - 1;
		} else {
			n = atoi(CG_Argv(2));
			if (n >= cg.numCameraPoints  ||  n < 0) {
				Com_Printf("invalid camera point\n");
				cg.selectedCameraPointMin = cg.numCameraPoints - 1;
				cg.selectedCameraPointMax = cg.numCameraPoints - 1;
				return;
			}
			cg.selectedCameraPointMax = n;
		}
	}

	if (cg.selectedCameraPointMin > cg.selectedCameraPointMax) {
		Com_Printf("invalid camera points\n");
		cg.selectedCameraPointMin = cg.numCameraPoints - 1;
		cg.selectedCameraPointMax = cg.numCameraPoints - 1;
		return;
	}
}

static void CG_EditCameraPoint_f (void)
{
	qboolean gotoRealPoint = qtrue;
	int cameraPoint = cg.numCameraPoints - 1;
	const char *s;

	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	if (CG_Argc() < 2) {
		cameraPoint = cg.selectedCameraPointMin;
	} else {
		s = CG_Argv(1);
		if (!Q_stricmp(s, "next")) {
			cameraPoint = cg.selectedCameraPointMin + 1;
			if (cameraPoint >= cg.numCameraPoints) {
				cameraPoint = 0;
			}
		} else if (!Q_stricmp(s, "previous")) {
			cameraPoint = cg.selectedCameraPointMin - 1;
			if (cameraPoint < 0) {
				cameraPoint = cg.numCameraPoints - 1;
			}
		} else {
			cameraPoint = atoi(CG_Argv(1));
		}
	}

	if (CG_Argc() >= 3) {
		if (!Q_stricmp(CG_Argv(2), "real")) {
			gotoRealPoint = qfalse;
		}
	}

	if (cameraPoint < 0  ||  cameraPoint >= cg.numCameraPoints) {
		Com_Printf("invalid camera point\n");
		return;
	}

	if (cg.numCameraPoints < 2  ||  (cg.cameraPoints[cameraPoint].type != CAMERA_SPLINE  ||  cg.cameraPoints[cameraPoint].type != CAMERA_SPLINE_BEZIER  ||  cg.cameraPoints[cameraPoint].type != CAMERA_SPLINE_CATMULLROM )) {
		gotoRealPoint = qfalse;
	}

	//FIXME
	//cg.editingCameraPoint = qtrue;
	cg.selectedCameraPointMin = cameraPoint;
	cg.selectedCameraPointMax = cameraPoint;

	if (gotoRealPoint) {
		if (cg.cameraPoints[cameraPoint].type == CAMERA_SPLINE) {
			VectorCopy(cg.splinePoints[cg.cameraPoints[cameraPoint].splineStart], cg.freecamPlayerState.origin);
		} else if (cg.cameraPoints[cameraPoint].type == CAMERA_SPLINE_BEZIER) {
			CG_CameraSplineOriginAt(cg.cameraPoints[cameraPoint].cgtime, posBezier, cg.freecamPlayerState.origin);
		} else if (cg.cameraPoints[cameraPoint].type == CAMERA_SPLINE_CATMULLROM) {
			CG_CameraSplineOriginAt(cg.cameraPoints[cameraPoint].cgtime, posCatmullRom, cg.freecamPlayerState.origin);
		} else {
			VectorCopy(cg.cameraPoints[cameraPoint].origin, cg.freecamPlayerState.origin);
		}
		VectorCopy(cg.freecamPlayerState.origin, cg.fpos);

		if (cg.cameraPoints[cameraPoint].viewType == CAMERA_ANGLES_SPLINE) {
			CG_CameraSplineAnglesAt(cg.cameraPoints[cameraPoint].cgtime, cg.freecamPlayerState.viewangles);
		} else {
			VectorCopy(cg.cameraPoints[cameraPoint].angles, cg.freecamPlayerState.viewangles);
		}
		VectorCopy(cg.freecamPlayerState.viewangles, cg.fang);
	} else {  // goto set values
		VectorCopy(cg.cameraPoints[cameraPoint].origin, cg.freecamPlayerState.origin);
		VectorCopy(cg.freecamPlayerState.origin, cg.fpos);
		VectorCopy(cg.cameraPoints[cameraPoint].angles, cg.freecamPlayerState.viewangles);
		VectorCopy(cg.freecamPlayerState.viewangles, cg.fang);
	}
	cg.freecamPlayerState.origin[2] -= DEFAULT_VIEWHEIGHT;
	cg.fpos[2] -= DEFAULT_VIEWHEIGHT;
	VectorSet(cg.freecamPlayerState.velocity, 0, 0, 0);
	//Com_Printf("^3camera time: %f\n", cg.cameraPoints[cameraPoint].cgtime);
	trap_Cvar_Set("cl_freezeDemo", "1");
	//trap_SendConsoleCommand(va("seekservertime %f\n", cg.cameraPoints[cameraPoint].cgtime));
	trap_SendConsoleCommandNow(va("seekservertime %f\n", cg.cameraPoints[cameraPoint].cgtime));
	//FIXME bad hack
	cg.atCameraPoint = qtrue;
}

static void CG_DeleteCameraPoint_f (void)
{
	//int n;
	int i;

	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	if (!cg.numCameraPoints) {
		return;
	}

	for (i = 0;  i < (cg.numCameraPoints - 1) - cg.selectedCameraPointMax;  i++) {
		Com_Printf("%d -> %d\n", cg.selectedCameraPointMax + 1 + i, cg.selectedCameraPointMin + i);
		memcpy(&cg.cameraPoints[cg.selectedCameraPointMin + i], &cg.cameraPoints[cg.selectedCameraPointMax + 1 + i], sizeof(cameraPoint_t));
	}

	cg.numCameraPoints = cg.selectedCameraPointMin + (cg.numCameraPoints - 1) - cg.selectedCameraPointMax;

	cg.selectedCameraPointMin = cg.numCameraPoints - 1;
	cg.selectedCameraPointMax = cg.numCameraPoints - 1;

	CG_UpdateCameraInfo();

	return;
}

#if 0
//static void FindQuadratic (double x0, double y0, double x1, double y1, double x2, double y2, double *a, double *b, double *c)
static void FindQuadratic (long double x0, long double y0, long double x1, long double y1, long double x2, long double y2, long double *a, long double *b, long double *c)
{
	//double e, f;
	long double e, f;

	Com_Printf("x0 %LF y0 %LF x1 %LF y1 %LF x2 %LF y2 %LF\n", x0, y0, x1, y1, x2, y2);

	e = x0 - x1;
	f = x0 - x2;

	*a = (f * (y0 - y1) - e * (y0 - y2))  /   (f * (x0 * x0 - x1 * x1) - e * (x0 * x0 - x2 * x2));

	*b = ((y0 - y1) - (long double)*a * (x0 * x0 - x1 * x1)) / (x0 - x1);

	*c = y0 - (long double)*a * x0 * x0 - (long double)*b * x0;

	Com_Printf("^3quadratic: %LFx**2  +  %LFx  +  %LF\n", *a, *b, *c);
	//Com_Printf("^3quadratic: %LFx**2  +  %LFx  +  %\n", *a, *b, *c);
}
#endif

static void FindQuadratic (long double x0, long double y0, long double x1, long double y1, long double x2, long double y2, long double *a, long double *b, long double *c)
{
	long double e, f;

	//Com_Printf("x0 %LF y0 %LF x1 %LF y1 %LF x2 %LF y2 %LF\n", x0, y0, x1, y1, x2, y2);

	e = x0 - x1;
	f = x0 - x2;

	*a = (f * (y0 - y1) - e * (y0 - y2))  /   (f * (x0 * x0 - x1 * x1) - e * (x0 * x0 - x2 * x2));

	*b = ((y0 - y1) - (long double)(*a) * (x0 * x0 - x1 * x1)) / (x0 - x1);

	*c = y0 - (long double)(*a) * x0 * x0 - (long double)(*b) * x0;

	//Com_Printf("^3quadratic: %LFx**2  +  %LFx  +  %LF\n", *a, *b, *c);
	//Com_Printf("^3quadratic: %LFx**2  +  %LFx  +  %\n", *a, *b, *c);
}

static double CameraCurveDistance (const cameraPoint_t *cp, const cameraPoint_t *cpnext)
{
	long double dist;
	int i;
	vec3_t origin;
	vec3_t newOrigin;
	int samples;
	long double t;
	int j;

	VectorSet(origin, 0, 0, 0);  // silence gcc warning
	dist = 0;
	samples = 500;

	for (i = 0;  i < samples;  i++) {
		t = ((cpnext->cgtime - cp->quadraticStartTime) - (cp->cgtime - cp->quadraticStartTime)) / (long double)samples * (long double)i;

		//t += cp->cgtime;
		t /= 1000.0;

		//Com_Printf("t %LF   start %f  %f\n", t, cp->quadraticStartTime, cp->cgtime);

		for (j = 0;  j < 3;  j++) {
			newOrigin[j] = cp->a[j] * t * t + cp->b[j] * t + cp->c[j];
		}

		if (i > 0) {
			dist += Distance(newOrigin, origin);
		}

		VectorCopy(newOrigin, origin);
	}

	return dist;
}

static void CG_UpdateCameraInfoExt (int startUpdatePoint)
{
	int i;
	float tension;
	float granularity;
	float x, y, z;
	int j;
	vec3_t bpoint0, bpoint1, bpoint2;
	vec3_t point0, point1, point2;
	int k;
	int start;
	qboolean midPointHit;
	vec3_t dir;
	double dist;
	cameraPoint_t *cp;
	cameraPoint_t *cpReal;
	cameraPoint_t *cpnext;
	const cameraPoint_t *cptmp;
	const cameraPoint_t *cpprev;
	const cameraPoint_t *first;
	const cameraPoint_t *second;
	const cameraPoint_t *last;
	int count;
	int passStart;
	int passEnd;
	//int realStart;
	int curveCount;
	vec3_t a0, a1;
	double avg;
	int ourUpdateStartPoint;

	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	if (cg.numCameraPoints < 2) {
		// update pointers for q3mme camera functions
		if (cg.numCameraPoints == 0) {
			cg.cameraPointsPointer = NULL;
		} else {  // 1
			cg.cameraPointsPointer = &cg.cameraPoints[0];
			cg.cameraPoints[0].prev = NULL;
			cg.cameraPoints[0].next = NULL;
			cg.cameraPoints[0].len = -1;
		}

		return;
	}

	// there's at least 2 camera points now
	ourUpdateStartPoint = startUpdatePoint - 3;
	if (ourUpdateStartPoint < 0) {
		ourUpdateStartPoint = 0;
	} else if (ourUpdateStartPoint >= cg.numCameraPoints) {
		Com_Printf("^2ERROR: invalid start camera point give for update camera info (%d >= %d)\n", ourUpdateStartPoint, cg.numCameraPoints);
		return;
	}

	// update camera pointers for q3mme camera functions, needs to be done
	// early since this functions calls q3mme cam functions
	for (i = ourUpdateStartPoint;  i < cg.numCameraPoints;  i++) {
		if (i == 0) {
			cg.cameraPoints[i].prev = NULL;
			cg.cameraPoints[i].next = &cg.cameraPoints[i + 1];
		} else if (i == cg.numCameraPoints - 1) {
			cg.cameraPoints[i].prev = &cg.cameraPoints[i - 1];
			cg.cameraPoints[i].next = NULL;
		} else {
			cg.cameraPoints[i].prev = &cg.cameraPoints[i - 1];
			cg.cameraPoints[i].next = &cg.cameraPoints[i + 1];
		}

		cg.cameraPoints[i].len = -1;
	}
	cg.cameraPointsPointer = &cg.cameraPoints[0];

	// calculate spline points

	cg.numSplinePoints = 0;
	for (i = 0;  i < ourUpdateStartPoint;  i++) {
		cameraPoint_t *cp;

		cp = &cg.cameraPoints[i];
		if (!(cp->flags & CAM_ORIGIN)) {
			continue;
		}
		cg.numSplinePoints += cp->numSplines;
	}

	granularity = 0.025;  //FIXME cvar

	first = cg.cameraPointsPointer;
	//first = &cg.cameraPoints[ourUpdateStartPoint];

	while (first  &&  !(first->flags & CAM_ORIGIN)) {
		first = first->next;
	}

	second = NULL;
	if (first) {
		second = first->next;
		while (second  &&  !(second->flags & CAM_ORIGIN)) {
			second = second->next;
		}
	}

	last = &cg.cameraPoints[cg.numCameraPoints - 1];
	while (last  &&  !(last->flags & CAM_ORIGIN)) {
		last = last->prev;
	}

	if (!first  ||  !second  ||  !last) {
		// nothing to calculate
		goto splinesDone;
	}

	VectorCopy(first->origin, bpoint0);
	VectorCopy(first->origin, bpoint1);
	VectorCopy(first->origin, bpoint2);

	VectorSubtract(first->origin, second->origin, dir);
	dist = Distance(second->origin, first->origin);
	//Com_Printf("beg dist %f\n", dist);
	VectorNormalize(dir);

	// hack to keep spline point 0 with camera point 0
	VectorMA(bpoint0, (float)dist * 1.0  * 3.0, dir, bpoint0);
	VectorMA(bpoint1, (float)dist * 0.66 * 3.0, dir, bpoint1);
	VectorMA(bpoint2, (float)dist * 0.33 * 3.0, dir, bpoint2);

	VectorCopy(last->origin, point0);
	VectorCopy(last->origin, point1);
	VectorCopy(last->origin, point2);

	VectorCopy(point0, cg.cameraPoints[cg.numCameraPoints + 0].origin);
	VectorCopy(point1, cg.cameraPoints[cg.numCameraPoints + 1].origin);
	VectorCopy(point2, cg.cameraPoints[cg.numCameraPoints + 2].origin);

	for (i = 0;  i < 3;  i++) {
		cg.cameraPoints[cg.numCameraPoints + i].numSplines = last->numSplines;
		cg.cameraPoints[cg.numCameraPoints + i].flags = CAM_ORIGIN;

		//FIXME bad hack
		if (i == 0) {
			cg.cameraPoints[cg.numCameraPoints + i].prev = &cg.cameraPoints[cg.numCameraPoints - 1];
		} else {
			cg.cameraPoints[cg.numCameraPoints + i].prev = &cg.cameraPoints[cg.numCameraPoints + i - 1];
		}
	}

	//for (i = 0;  i < cg.numCameraPoints + 3;  i++) {
	for (i = ourUpdateStartPoint;  i < cg.numCameraPoints + 3;  i++) {
		cp = &cg.cameraPoints[i];

		// some clean up
		cp->viewPointPassStart = -1;
		cp->viewPointPassEnd = -1;
		cp->rollPassStart = -1;
		cp->rollPassEnd = -1;
		cp->fovPassStart = -1;
		cp->fovPassEnd = -1;
		cp->offsetPassStart = -1;
		cp->offsetPassEnd = -1;

		if (!(cp->flags & CAM_ORIGIN)) {
			continue;
		}

		//Com_Printf("camera point (%d):  %f %f %f\n", i, cg.cameraPoints[i].origin[0], cg.cameraPoints[i].origin[1], cg.cameraPoints[i].origin[2]);

		// get valid origin cam point i - 2

		cpReal = cp->prev;
		while (cpReal  &&  !(cpReal->flags & CAM_ORIGIN)) {
			cpReal = cpReal->prev;
		}
		if (cpReal) {
			cpReal = cpReal->prev;
			while (cpReal  &&  !(cpReal->flags & CAM_ORIGIN)) {
				cpReal = cpReal->prev;
			}
		}

		if (cpReal) {
			cpReal->splineStart = cg.numSplinePoints;
			//Com_Printf("new  %p\n", cpReal);
		}

		start = cg.numSplinePoints;

		granularity = 1.0 / (float)cg.cameraPoints[i].numSplines;

		midPointHit = qfalse;

		for (tension = 0.0;  tension < 0.999 /*1.001*/;  tension += granularity) {
			x = y = z = 0;
			for (j = 0;  j < 4;  j++) {
				vec3_t origin;

				k = i - (3 - j);

				if (k == -3) {
					VectorCopy(bpoint0, origin);
				} else if (k == -2) {
					VectorCopy(bpoint1, origin);
				} else if (k == -1) {
					VectorCopy(bpoint2, origin);
				} else {
					int n;

					cpReal = cp;
					while (cpReal  &&  !(cpReal->flags & CAM_ORIGIN)) {
						Com_Printf("^1tension spline shouldn't happen...\n");
						cpReal = cpReal->prev;
					}
					n = 3 - j;
					while (n > 0) {
						if (cpReal) {
							cpReal = cpReal->prev;
							while (cpReal  &&  !(cpReal->flags & CAM_ORIGIN)) {
								cpReal = cpReal->prev;
							}
						}
						n--;
					}

					// no, use flags/mask
					//VectorCopy(cg.cameraPoints[k].origin, origin);

					if (!cpReal) {
						//Com_Printf("^1cpReal not found j == %d, i == %d\n", j, i);
						VectorCopy(bpoint0, origin);
					} else {
						VectorCopy(cpReal->origin, origin);
					}
				}

				x += origin[0] * CG_CalcSpline(j, tension);
				y += origin[1] * CG_CalcSpline(j, tension);
				z += origin[2] * CG_CalcSpline(j, tension);
			}
			VectorSet(cg.splinePoints[cg.numSplinePoints], x, y, z);
			cg.splinePointsCameraPoints[cg.numSplinePoints] = i;
			//cg.splinePointsCameraPoints[cg.numSplinePoints] = i - 2;
			cg.numSplinePoints++;
			if (tension > 0.49) {
				if (!midPointHit) {
					//cg.cameraPoints[i - 2].splineStart = cg.numSplinePoints;
				}
				midPointHit = qtrue;
			}
			//Com_Printf("%d  (%d) %f %f %f\n", i, cg.numSplinePoints - 1, x, y, z);
			//Com_Printf("    pt %f %f %f\n", x, y, z);
			if (cg.numSplinePoints >= MAX_SPLINEPOINTS) {
				Com_Printf("^1ERROR cg.numSplinePoints >= MAX_SPLINEPOINTS\n");
				//return;
				goto splinesDone;
			}
		}
		if (SC_Cvar_Get_Int("debug_splines")) {
			if (i < cg.numCameraPoints) {
				Com_Printf("cam point %d  %d splines  granularity %f\n", i, cg.numSplinePoints - start, granularity);
			}
		}
	}

	// other spline types
	//FIXME camera curve done after everything else

	//cp = cg.cameraPointsPointer;

	//for (cp = cg.cameraPointsPointer;  cp != NULL;  cp = cp->next) {
	for (cp = &cg.cameraPoints[ourUpdateStartPoint];  cp != NULL;  cp = cp->next) {
		const cameraPoint_t *next;
		int numSplinePoints;
		double totalTime;
		double timeSlice;
		posInterpolate_t posType;

		if (!(cp->flags & CAM_ORIGIN)) {
			continue;
		}
		if (cp->type != CAMERA_SPLINE_BEZIER  &&  cp->type != CAMERA_SPLINE_CATMULLROM) {
			continue;
		}
		if (cp->type == CAMERA_SPLINE_BEZIER) {
			posType = posBezier;
		} else if (cp->type == CAMERA_SPLINE_CATMULLROM) {
			posType = posCatmullRom;
		} else {
			posType = posBezier;
			Com_Printf("^1alt spline invalid type %d\n", cp->type);
		}

		next = cp->next;
		while (next  &&  !(next->flags & CAM_ORIGIN)) {
			next = next->next;
		}
		if (!next) {
			CG_CameraSplineOriginAt(cp->cgtime, posType, cg.splinePoints[cp->splineStart]);
			break;
		}


		numSplinePoints = next->splineStart - cp->splineStart;
		totalTime = next->cgtime - cp->cgtime;
		timeSlice = totalTime / (double)numSplinePoints;

		for (i = 0;  i < numSplinePoints;  i++) {
			CG_CameraSplineOriginAt(cp->cgtime + (timeSlice * i), posType, cg.splinePoints[cp->splineStart + i]);
			//Com_Printf("new spline %p  %d  (%f %f %f)\n", cp, i, cg.splinePoints[cp->splineStart + i][0], cg.splinePoints[cp->splineStart + i][1], cg.splinePoints[cp->splineStart + i][2]);
		}
	}

 splinesDone:
	// ugh.. this was already set
	//cg.cameraPoints[cg.numCameraPoints - 1].splineStart = cg.numSplinePoints - 1;

	if (SC_Cvar_Get_Int("debug_splines")) {
		Com_Printf("UpdateCameraInfo(): CreateSplines  %d spline points\n", cg.numSplinePoints);
	}

	// origin initial velocities and quadratic calculation

	curveCount = 0;

	for (i = 0;  i < cg.numCameraPoints - 1;  i++) {
		const cameraPoint_t *cpnextnext, *cpprevprev;  // for curve type

		cp = &cg.cameraPoints[i];
		if (!(cp->flags & CAM_ORIGIN)) {
			continue;
		}
		cpnext = cp->next;
		//Com_Printf("next:  %p\n", cpnext);
		while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
			cpnext = cpnext->next;
		}
		if (!cpnext) {
			//Com_Printf("^1%d / %d  next null\n", i, cg.numCameraPoints);
			break;
		}

		cpnextnext = NULL;
		if (cpnext->next) {
			cpnextnext = cpnext->next;
			while (cpnextnext  &&  !(cpnextnext->flags & CAM_ORIGIN)) {
				cpnextnext = cpnextnext->next;
			}
		}
		if (cpnextnext  &&  !(cpnextnext->flags & CAM_ORIGIN)) {
			cpnextnext = NULL;
		}

		cpprev = NULL;
		if (cp->prev) {
			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
				cpprev = cpprev->prev;
			}
		}
		if (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
			cpprev = NULL;
		}

		cpprevprev = NULL;
		if (cpprev  &&  cpprev->prev) {
			cpprevprev = cpprev->prev;
			while (cpprevprev  &&  !(cpprevprev->flags & CAM_ORIGIN)) {
				cpprevprev = cpprevprev->prev;
			}
		}
		if (cpprevprev  &&  !(cpprevprev->flags & CAM_ORIGIN)) {
			cpprevprev = NULL;
		}

		if (cp->type == CAMERA_CURVE) {
			const cameraPoint_t *p1, *p2, *p3;

			cp->curveCount = curveCount;
			curveCount++;

			// we do this check here as well because we will need 'curveCount'
			// later on
			if (i < ourUpdateStartPoint) {
				continue;
			}

			if (!cpnextnext) {
				if (cpnext->type == CAMERA_CURVE) {
					cpnext->curveCount = curveCount;
				}
			}

			p1 = p2 = p3 = NULL;

			if (cp->curveCount % 2 == 0) {
				p1 = cp;
				p2 = cpnext;
				p3 = cpnextnext;

				if (cpprev  &&  cpprev->type == CAMERA_SPLINE) {
					VectorCopy(cg.splinePoints[cp->splineStart], point0);
				} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_BEZIER) {
					CG_CameraSplineOriginAt(cp->cgtime, posBezier, point0);
				} else if (cpprev  &&  cpprev->type == CAMERA_SPLINE_CATMULLROM) {
					CG_CameraSplineOriginAt(cp->cgtime, posCatmullRom, point0);
				} else {
					VectorCopy(cp->origin, point0);
				}

				if (p2) {
					VectorCopy(p2->origin, point1);
				}
				if (p3) {
					if (p3->type == CAMERA_SPLINE) {
						VectorCopy(cg.splinePoints[p3->splineStart], point2);
					} else if (p3->type == CAMERA_SPLINE_BEZIER) {
						CG_CameraSplineOriginAt(p3->cgtime, posBezier, point2);
					} else if (p3->type == CAMERA_SPLINE_CATMULLROM) {
						CG_CameraSplineOriginAt(p3->cgtime, posCatmullRom, point2);
					} else {
						VectorCopy(p3->origin, point2);
					}
				}
			} else {
				// middle point
				p1 = cpprev;
				p2 = cp;
				p3 = cpnext;

				if (p1  &&  cpprevprev  &&  cpprevprev->type == CAMERA_SPLINE) {
					VectorCopy(cg.splinePoints[p1->splineStart], point0);
				} else if (p1  &&  cpprevprev  &&  cpprevprev->type == CAMERA_SPLINE_BEZIER) {
					CG_CameraSplineOriginAt(p1->cgtime, posBezier, point0);
				} else if (p1  &&  cpprevprev  &&  cpprevprev->type == CAMERA_SPLINE_CATMULLROM) {
					CG_CameraSplineOriginAt(p1->cgtime, posCatmullRom, point0);
				} else {
					VectorCopy(p1->origin, point0);
				}

				if (p2) {
					VectorCopy(p2->origin, point1);
				}
				if (p3) {
					if (p3->type == CAMERA_SPLINE) {
						VectorCopy(cg.splinePoints[p3->splineStart], point2);
					} else if (p3->type == CAMERA_SPLINE_BEZIER) {
						CG_CameraSplineOriginAt(p3->cgtime, posBezier, point2);
					} else if (p3->type == CAMERA_SPLINE_CATMULLROM) {
						CG_CameraSplineOriginAt(p3->cgtime, posCatmullRom, point2);
					} else {
						VectorCopy(p3->origin, point2);
					}
				}
			}

			if (p1  &&  p2  &&  p3) {
				for (j = 0;  j < 3;  j++) {
					//FindQuadratic(p1->cgtime / 1000.0, (double)p1->origin[j], p2->cgtime / 1000.0, (double)p2->origin[j], p3->cgtime / 1000.0, (double)p3->origin[j], &cp->a[j], &cp->b[j], &cp->c[j]);
					//FindQuadratic(p1->cgtime / 1000.0, (double)point0[j], p2->cgtime / 1000.0, (double)point1[j], p3->cgtime / 1000.0, (double)point2[j], &cp->a[j], &cp->b[j], &cp->c[j]);

					// base it on p1->cgtime / 1000.0  being zero to increase
					// precision
					FindQuadratic(0.0, (double)point0[j], (p2->cgtime / 1000.0) - (p1->cgtime / 1000.0), (double)point1[j], (p3->cgtime / 1000.0) - (p1->cgtime / 1000.0), (double)point2[j], &cp->a[j], &cp->b[j], &cp->c[j]);
					cp->hasQuadratic = qtrue;
				}
				cp->quadraticStartTime = p1->cgtime;
				//Com_Printf("quad time %f\n", cp->quadraticStartTime);
			} else {
				// hack for distance, just make it linear
				Com_Printf("cam %d no quadratic %p %p %p\n", i, p1, p2, p3);
				cp->hasQuadratic = qfalse;
			}
		}  // cp->type == CAMERA_CURVE

		if (i < ourUpdateStartPoint) {
			continue;
		}

		cp->originDistance = 0;

		if (cp->type == CAMERA_SPLINE) {
			for (j = cp->splineStart;  j < cpnext->splineStart;  j++) {
				cp->originDistance += Distance(cg.splinePoints[j], cg.splinePoints[j + 1]);
			}
			cp->originAvgVelocity = cp->originDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
			cp->originImmediateInitialVelocity = Distance(cg.splinePoints[cp->splineStart], cg.splinePoints[cp->splineStart + 1]) / (((cpnext->cgtime - cp->cgtime) / 1000.0) / (cpnext->splineStart - cp->splineStart));
			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					if (cp->originInitialVelocity > 2.0 * cp->originAvgVelocity) {
						cp->originInitialVelocity = 2.0 * cp->originAvgVelocity;
					}
					cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
					cp->originImmediateInitialVelocity *= (cp->originInitialVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateInitialVelocity = 0;
			}
			cp->originImmediateFinalVelocity = Distance(cg.splinePoints[cpnext->splineStart - 1], cg.splinePoints[cpnext->splineStart - 2]) / (((cpnext->cgtime - cp->cgtime) / 1000.0) / (cpnext->splineStart - cp->splineStart));
			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					cp->originImmediateFinalVelocity *= (cp->originFinalVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateFinalVelocity = 0;
			}
		} else if (cp->type == CAMERA_SPLINE_BEZIER  ||  cp->type == CAMERA_SPLINE_CATMULLROM) {
			posInterpolate_t posType = posBezier;
			double cameraTime;
			double startTime, endTime;
			vec3_t start, end;
			double timeSlice;
			int numSplines;

			if (cp->type == CAMERA_SPLINE_CATMULLROM) {
				posType = posCatmullRom;
			}

			cameraTime = cpnext->cgtime - cp->cgtime;
			if (cameraTime <= 0.0) {
				Com_Printf("^1invalid camera times found during spline calculation:  %f  -> %f\n", cp->cgtime, cpnext->cgtime);
				return;
			}

			//FIXME DEFAULT_NUM_SPLINES
			numSplines = DEFAULT_NUM_SPLINES;
			if (numSplines <= 1) {
				Com_Printf("^1invalid number of splines %d\n", numSplines);
			}

			timeSlice = cameraTime / numSplines;

			for (j = 0;  j < numSplines - 1;  j++) {
				startTime = ((double)(j + 0) * timeSlice) + cp->cgtime;
				endTime = ((double)(j + 1) * timeSlice) + cp->cgtime;

				CG_CameraSplineOriginAt(startTime, posType, start);
				CG_CameraSplineOriginAt(endTime, posType, end);

				cp->originDistance += Distance(start, end);
			}

			cp->originAvgVelocity = cp->originDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

			startTime = cp->cgtime;
			endTime = ((double)(1) * timeSlice) + cp->cgtime;
			CG_CameraSplineOriginAt(startTime, posType, start);
			CG_CameraSplineOriginAt(endTime, posType, end);

			cp->originImmediateInitialVelocity = Distance(start, end) / (endTime - startTime) * 1000.0;

			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					if (cp->originInitialVelocity > 2.0 * cp->originAvgVelocity) {
						cp->originInitialVelocity = 2.0 * cp->originAvgVelocity;
					}
					cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
					cp->originImmediateInitialVelocity *= (cp->originInitialVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateInitialVelocity = 0;
			}

			startTime = ((double)(numSplines - 1) * timeSlice) + cp->cgtime;
			endTime = ((double)(numSplines - 0) * timeSlice) + cp->cgtime;
			CG_CameraSplineOriginAt(startTime, posType, start);
			CG_CameraSplineOriginAt(endTime, posType, end);

			cp->originImmediateFinalVelocity = Distance(start, end) / (endTime - startTime) * 1000.0;
			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					cp->originImmediateFinalVelocity *= (cp->originFinalVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateFinalVelocity = 0;
			}
		} else if (cp->type == CAMERA_CURVE) {
			//double dd;

			if (cp->hasQuadratic) {
				vec3_t vel;
				long double tm;

				cp->originDistance = CameraCurveDistance(cp, cpnext);
				cp->originAvgVelocity = cp->originDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
				tm = ((long double)cp->cgtime - (long double)cp->quadraticStartTime) / (long double)1000.0;
				for (j = 0;  j < 3;  j++) {
					//vel[0] = 2.0 * cp->a[0] * tm + cp->b[0];
					vel[j] = (long double)2.0 * cp->a[j] * tm + cp->b[j];
					//Com_Printf("%d:  %f  (2 * %LFx + %LF)  tm: %LF\n", j, vel[j], cp->a[j], cp->b[j], tm);
				}
				cp->originImmediateInitialVelocity = VectorLength(vel);
				if (cp->originAvgVelocity > 0.001) {
					if (cp->useOriginVelocity) {
						if (cp->originInitialVelocity > 2.0 * cp->originAvgVelocity) {
							cp->originInitialVelocity = 2.0 * cp->originAvgVelocity;
						}
						cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
						cp->originImmediateInitialVelocity *= (cp->originInitialVelocity / cp->originAvgVelocity);
					} else {
						//cp->originImmediateInitialVelocity = cp->originAvgVelocity;
					}
				} else {
					cp->originImmediateInitialVelocity = 0;
				}
				//Com_Printf("vel %f\n", VectorLength(vel));

				//tm += ((long double)cpnext->cgtime - (long double)cp->cgtime) / (long double)1000.0;
				tm = ((long double)cpnext->cgtime - (long double)cp->quadraticStartTime) / (long double)1000.0;
				for (j = 0;  j < 3;  j++) {

					vel[j] = (long double)2.0 * cp->a[j] * tm + cp->b[j];
					//Com_Printf("%d:  %f  (2 * %LFx + %LF)  tm: %LF\n", j, vel[j], cp->a[j], cp->b[j], tm);
				}
				cp->originImmediateFinalVelocity = VectorLength(vel);
				if (cp->originAvgVelocity > 0.001) {
					if (cp->useOriginVelocity) {
						cp->originImmediateFinalVelocity *= (cp->originFinalVelocity / cp->originAvgVelocity);
					}
				} else {
					cp->originImmediateFinalVelocity = 0;
				}
			} else {
				cp->originDistance = Distance(cp->origin, cpnext->origin);
				cp->originAvgVelocity = cp->originDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
				cp->originImmediateInitialVelocity = cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0);
				if (cp->originAvgVelocity > 0.001) {
					if (cp->useOriginVelocity) {
						if (cp->originInitialVelocity > 2.0 * cp->originAvgVelocity) {
							cp->originInitialVelocity = 2.0 * cp->originAvgVelocity;
						}
						cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
						cp->originImmediateInitialVelocity *= (cp->originInitialVelocity / cp->originAvgVelocity);
					}
				} else {
					cp->originImmediateInitialVelocity = 0;
				}
				cp->originImmediateFinalVelocity = cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0);
				if (cp->originAvgVelocity > 0.001) {
					if (cp->useOriginVelocity) {
						cp->originImmediateFinalVelocity *= (cp->originFinalVelocity / cp->originAvgVelocity);
					} else {
						//cp->originImmediateFinalVelocity = cp->originAvgVelocity;
					}
				} else {
					cp->originImmediateFinalVelocity = 0;
				}
			}

#if 0
			dd = 0;
			for (j = cp->splineStart;  j < cpnext->splineStart;  j++) {
				dd += Distance(cg.splinePoints[j], cg.splinePoints[j + 1]);
			}
			Com_Printf("%f  approx  %f  %f\n", cp->originDistance, Distance(cp->origin, cpnext->origin), dd);
#endif
		} else {  // not camera curve, or spline type
			cp->originDistance = Distance(cp->origin, cpnext->origin);
			cp->originAvgVelocity = cp->originDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
			cp->originImmediateInitialVelocity = cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0);
			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					if (cp->originInitialVelocity > 2.0 * cp->originAvgVelocity) {
						cp->originInitialVelocity = 2.0 * cp->originAvgVelocity;
					}
					cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
					cp->originImmediateInitialVelocity *= (cp->originInitialVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateInitialVelocity = 0;
			}
			cp->originImmediateFinalVelocity = cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0);
			if (cp->originAvgVelocity > 0.001) {
				if (cp->useOriginVelocity) {
					cp->originImmediateFinalVelocity *= (cp->originFinalVelocity / cp->originAvgVelocity);
				}
			} else {
				cp->originImmediateFinalVelocity = 0;
			}
		}

		//Com_Printf("cam %d  vel %f i %f  -> f %f\n", i, cp->originAvgVelocity, cp->originImmediateInitialVelocity, cp->originImmediateFinalVelocity);

	}

	// now camera angle velocities
	for (i = ourUpdateStartPoint;  i < cg.numCameraPoints - 1;  i++) {
		cp = &cg.cameraPoints[i];

		if (!(cp->flags & CAM_ANGLES)) {
			continue;
		}
		cpnext = cp->next;
		//Com_Printf("next:  %p\n", cpnext);
		while (cpnext  &&  !(cpnext->flags & CAM_ANGLES)) {
			cpnext = cpnext->next;
		}
		if (!cpnext) {
			//Com_Printf("^1%d / %d angles next null\n", i, cg.numCameraPoints);
			break;
		}

		cpprev = NULL;
		if (cp->prev) {
			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ANGLES)) {
				cpprev = cpprev->prev;
			}
		}
		if (cpprev  &&  !(cpprev->flags & CAM_ANGLES)) {
			cpprev = NULL;
		}

		// prelim values, pass is handled below
		if (cp->viewType == CAMERA_ANGLES_SPLINE) {
			vec3_t atmp;
			double timeSlice;

			//FIXME initial velocity
			CG_CameraSplineAnglesAt(cp->cgtime, a0);
			CG_CameraSplineAnglesAt(cpnext->cgtime, a1);
			//FIXME (maybe) -- not really true, but it doesn't matter that
			// much since the value is only used as a base for setting avg
			// initial and final velocities
			if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
				cp->anglesDistance = CG_CameraAngleLongestDistanceWithRoll(a1, a0);
			} else {
				cp->anglesDistance = CG_CameraAngleLongestDistanceNoRoll(a1, a0);
			}
			cp->anglesAvgVelocity = cp->anglesDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
			//FIXME 40 fixed?
			timeSlice = (cpnext->cgtime - cp->cgtime) / 40.0;
			CG_CameraSplineAnglesAt(cp->cgtime + timeSlice, atmp);
			if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
				cp->anglesImmediateInitialVelocity = CG_CameraAngleLongestDistanceWithRoll(a0, atmp) / timeSlice * 1000.0;
			} else {
				cp->anglesImmediateInitialVelocity = CG_CameraAngleLongestDistanceNoRoll(a0, atmp) / timeSlice * 1000.0;
			}
			CG_CameraSplineAnglesAt(cpnext->cgtime - timeSlice, atmp);
			if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
				cp->anglesImmediateFinalVelocity = CG_CameraAngleLongestDistanceWithRoll(a1, atmp) / timeSlice * 1000.0;
			} else {
				cp->anglesImmediateFinalVelocity = CG_CameraAngleLongestDistanceNoRoll(a1, atmp) / timeSlice * 1000.0;
			}
		} else {
			VectorCopy(cp->angles, a0);
			VectorCopy(cpnext->angles, a1);
			if (cp->rollType != CAMERA_ROLL_AS_ANGLES) {
				a0[ROLL] = 0;
				a1[ROLL] = 0;
			}
			//AnglesSubtract(a1, a0, cp->anglesDistance);
			if (cp->rollType != CAMERA_ROLL_AS_ANGLES) {
				cp->anglesDistance = CG_CameraAngleLongestDistanceNoRoll(a1, a0);
			} else {
				cp->anglesDistance = CG_CameraAngleLongestDistanceWithRoll(a1, a0);
			}
			cp->anglesAvgVelocity = cp->anglesDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
			cp->anglesImmediateInitialVelocity = cp->anglesAvgVelocity;
			cp->anglesImmediateFinalVelocity = cp->anglesAvgVelocity;
		}


		//Com_Printf("%d angles distance %f (%f)\n", i, cp->anglesDistance, cp->anglesAvgVelocity);

		if (cp->useAnglesVelocity) {
			if (cp->anglesInitialVelocity > 2.0 * cp->anglesAvgVelocity) {
				cp->anglesInitialVelocity = 2.0 * cp->anglesAvgVelocity;
			}
			// fucking hell... different pass types:  see /ecam
			cp->anglesFinalVelocity = 2.0 * cp->anglesDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesInitialVelocity;

			cp->anglesImmediateInitialVelocity *= (cp->anglesInitialVelocity / cp->anglesAvgVelocity);
			cp->anglesImmediateFinalVelocity *= (cp->anglesFinalVelocity / cp->anglesAvgVelocity);
		}

		cp->xoffsetDistance = fabs(cpnext->xoffset - cp->xoffset);
		cp->xoffsetAvgVelocity = cp->xoffsetDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		cp->yoffsetDistance = fabs(cpnext->yoffset - cp->yoffset);
		cp->yoffsetAvgVelocity = cp->yoffsetDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		cp->zoffsetDistance = fabs(cpnext->zoffset - cp->zoffset);
		cp->zoffsetAvgVelocity = cp->zoffsetDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		//cp->fovDistance = fabs(cpnext->fov - cp->fov);
		//cp->fovAvgVelocity = cp->fovDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		cp->rollDistance = fabs(cpnext->angles[ROLL] - cp->angles[ROLL]);
		while (cp->rollDistance >= 180.0) {
			cp->rollDistance -= 180.0;
		}
		cp->rollAvgVelocity = cp->rollDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		// pass options
		if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
			count = 0;
			cptmp = NULL;
			for (j = i + 1;  j < cg.numCameraPoints;  j++) {
				cptmp = &cg.cameraPoints[j];
				if (!(cptmp->flags & CAM_ANGLES)) {
					continue;
				}
				if (cptmp->viewType != CAMERA_ANGLES_VIEWPOINT_PASS) {
					break;
				}
				count++;
			}
			if (cptmp  &&  (cptmp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP  ||  cptmp->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED)  &&  count > 0) {
				Com_Printf("viewpoint match for %d -> %d\n", i, j);
				passStart = i;
				passEnd = j;

#if 0
				VectorCopy(cg.cameraPoints[passStart].angles, a0);
				VectorCopy(cg.cameraPoints[passEnd].angles, a1);
				a0[ROLL] = 0;
				a1[ROLL] = 0;
#endif
				VectorCopy(cg.cameraPoints[passStart].viewPointOrigin, a0);
				VectorCopy(cg.cameraPoints[passEnd].viewPointOrigin, a1);

				dist = CG_CameraAngleLongestDistanceNoRoll(a0, a1);
				avg = dist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;

				for (j = i;  j < passEnd;  j++) {
					cg.cameraPoints[j].viewPointPassStart = passStart;
					cg.cameraPoints[j].viewPointPassEnd = passEnd;

					cg.cameraPoints[j].anglesDistance = dist;
					cg.cameraPoints[j].anglesAvgVelocity = avg;

					Com_Printf("viewpoint pass info set for %d\n", j);
				}
			} else if (cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP  ||  cpnext->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED) {
				cp->anglesDistance = CG_CameraAngleLongestDistanceNoRoll(cp->viewPointOrigin, cpnext->viewPointOrigin);
				cp->anglesAvgVelocity = cp->anglesDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;
				//Com_Printf("yes\n");
			}
		}

		if (cp->offsetType == CAMERA_OFFSET_INTERP) {
			count = 0;
			cptmp = NULL;
			for (j = i + 1;  j < cg.numCameraPoints;  j++) {
				cptmp = &cg.cameraPoints[j];
				if (!(cptmp->flags & CAM_ANGLES)) {
					continue;
				}
				if (cptmp->offsetType != CAMERA_OFFSET_PASS) {
					break;
				}
				count++;
			}
			if (cptmp  &&  (cptmp->offsetType == CAMERA_OFFSET_INTERP  ||  cptmp->offsetType == CAMERA_OFFSET_FIXED)  &&  count > 0) {
				double xdist, ydist, zdist;
				double xavg, yavg, zavg;

				Com_Printf("offset match for %d -> %d\n", i, j);
				passStart = i;
				passEnd = j;

				xdist = fabs(cg.cameraPoints[passEnd].xoffset - cg.cameraPoints[passStart].xoffset);
				ydist = fabs(cg.cameraPoints[passEnd].yoffset - cg.cameraPoints[passStart].yoffset);
				zdist = fabs(cg.cameraPoints[passEnd].zoffset - cg.cameraPoints[passStart].zoffset);

				xavg = xdist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;
				yavg = ydist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;
				zavg = zdist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;

				for (j = i;  j < passEnd;  j++) {
					cg.cameraPoints[j].offsetPassStart = passStart;
					cg.cameraPoints[j].offsetPassEnd = passEnd;

					cg.cameraPoints[j].xoffsetDistance = xdist;
					cg.cameraPoints[j].yoffsetDistance = ydist;
					cg.cameraPoints[j].zoffsetDistance = zdist;

					cg.cameraPoints[j].xoffsetAvgVelocity = xavg;
					cg.cameraPoints[j].yoffsetAvgVelocity = yavg;
					cg.cameraPoints[j].zoffsetAvgVelocity = zavg;

					Com_Printf("offset pass info set for %d\n", j);
				}
			}
		}

		if (cp->rollType == CAMERA_ROLL_INTERP) {
			count = 0;
			cptmp = NULL;
			for (j = i + 1;  j < cg.numCameraPoints;  j++) {
				cptmp = &cg.cameraPoints[j];
				if (!(cptmp->flags & CAM_ANGLES)) {
					continue;
				}
				if (cptmp->rollType != CAMERA_ROLL_PASS) {
					break;
				}
				count++;
			}
			if (cptmp  &&  (cptmp->rollType == CAMERA_ROLL_INTERP  ||  cptmp->rollType == CAMERA_ROLL_FIXED  ||  cptmp->rollType == CAMERA_ROLL_AS_ANGLES)  &&  count > 0) {
				Com_Printf("roll match for %d -> %d\n", i, j);
				passStart = i;
				passEnd = j;

				dist = fabs(cg.cameraPoints[passEnd].angles[ROLL] - cg.cameraPoints[passStart].angles[ROLL]);
				while (dist >= 180.0) {
					dist -= 180.0;
				}
				avg = dist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;

				for (j = i;  j < passEnd;  j++) {
					cg.cameraPoints[j].rollPassStart = passStart;
					cg.cameraPoints[j].rollPassEnd = passEnd;

					cg.cameraPoints[j].rollDistance = dist;
					cg.cameraPoints[j].rollAvgVelocity = avg;

					Com_Printf("roll pass info set for %d\n", j);
				}
			}
		}

	}

	// now camera fov velocities
	for (i = ourUpdateStartPoint;  i < cg.numCameraPoints - 1;  i++) {
		cp = &cg.cameraPoints[i];

		if (!(cp->flags & CAM_FOV)) {
			continue;
		}
		cpnext = cp->next;
		//Com_Printf("next:  %p\n", cpnext);
		while (cpnext  &&  !(cpnext->flags & CAM_FOV)) {
			cpnext = cpnext->next;
		}
		if (!cpnext) {
			//Com_Printf("^1%d / %d fov next null\n", i, cg.numCameraPoints);
			break;
		}

		cpprev = NULL;
		if (cp->prev) {
			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_FOV)) {
				cpprev = cpprev->prev;
			}
		}
		if (cpprev  &&  !(cpprev->flags & CAM_FOV)) {
			cpprev = NULL;
		}

		cp->fovDistance = fabs(cpnext->fov - cp->fov);
		cp->fovAvgVelocity = cp->fovDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		if (cp->fovType == CAMERA_FOV_INTERP) {
			count = 0;
			cptmp = NULL;
			for (j = i + 1;  j < cg.numCameraPoints;  j++) {
				cptmp = &cg.cameraPoints[j];
				if (!(cptmp->flags & CAM_FOV)) {
					continue;
				}
				if (cptmp->fovType != CAMERA_FOV_PASS) {
					break;
				}
				count++;
			}
			if (cptmp  &&  (cptmp->fovType == CAMERA_FOV_INTERP  ||  cptmp->fovType == CAMERA_FOV_FIXED  ||  cptmp->fovType == CAMERA_FOV_USE_CURRENT)  &&  count > 0) {
				Com_Printf("fov match for %d -> %d\n", i, j);
				passStart = i;
				passEnd = j;

				dist = fabs(cg.cameraPoints[passEnd].fov - cg.cameraPoints[passStart].fov);
				avg = dist / (cg.cameraPoints[passEnd].cgtime - cg.cameraPoints[passStart].cgtime) * 1000.0;

				for (j = i;  j < passEnd;  j++) {
					cg.cameraPoints[j].fovPassStart = passStart;
					cg.cameraPoints[j].fovPassEnd = passEnd;

					cg.cameraPoints[j].fovDistance = dist;
					cg.cameraPoints[j].fovAvgVelocity = avg;

					Com_Printf("fov pass info set for %d\n", j);
				}
			}
		} else if (cp->fovType == CAMERA_FOV_SPLINE) {
			float startFov, endFov;

			if (!CG_CameraSplineFovAt(cp->cgtime, &startFov)) {
				if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
					startFov = cg.demoFov;
				} else {
					startFov = cg_fov.value;
				}
			}
			if (!CG_CameraSplineFovAt(cpnext->cgtime, &endFov)) {
				if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
					endFov = cg.demoFov;
				} else {
					endFov = cg_fov.value;
				}
			}
			cp->fovDistance = fabs(startFov - endFov);
			cp->fovAvgVelocity = cp->fovDistance / (cpnext->cgtime - cp->cgtime) * 1000.0;

		}  // cp->fovType
	}


	// debugging
#if 0
	for (i = 0;  i < cg.numCameraPoints;  i++) {
		Com_Printf("W %d:  %f %f %f\n", i, cg.cameraPoints[i].origin[0], cg.cameraPoints[i].origin[1], cg.cameraPoints[i].origin[2]);
	}

	{
		int count = 0;
		demoCameraPoint_t *p = demo.camera.points;
		while (p) {
			Com_Printf("Q %d:  %f %f %f\n", count,  p->origin[0], p->origin[1], p->origin[2]);
			count++;
			p = p->next;
		}
	}
#endif

	//FIXME maybe not here
	// curve type spline points for drawpath
	if (cg.numSplinePoints < MAX_SPLINEPOINTS) {
		const cameraPoint_t *cp;

		//for (cp = cg.cameraPointsPointer;  cp != NULL;  cp = cp->next) {
		for (cp = &cg.cameraPoints[ourUpdateStartPoint];  cp != NULL;  cp = cp->next) {
			const cameraPoint_t *cpnext;
			int numSplinePoints;
			long double t;
			long double timeSlice;
			int i;

			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}
			if (cp->type != CAMERA_CURVE) {
				continue;
			}
			if (!cp->hasQuadratic) {  // already handled above
				VectorCopy(cp->origin, cg.splinePoints[cp->splineStart]);
				continue;
			}

			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
				cpnext = cpnext->next;
			}

			if (!cpnext) {
				Com_Printf("^3curve spline calc !cpnext\n");
				break;
			}

			numSplinePoints = cpnext->splineStart - cp->splineStart;
			timeSlice = (cpnext->cgtime - cp->cgtime) / (long double)numSplinePoints;
			for (i = 0;  i < numSplinePoints;  i++) {
				int j;

				t = (cp->cgtime - cp->quadraticStartTime) + (timeSlice * i);
				// in seconds
				t /= 1000.0;

				for (j = 0;  j < 3;  j++) {
					cg.splinePoints[cp->splineStart + i][j] = cp->a[j] * t * t + cp->b[j] * t + cp->c[j];
				}
			}
		}
	}  // else, spline creation was aborted above

	//FIXME not here
	trap_SendConsoleCommand("savecamera wolfcam-autosave\n");
}

static void CG_UpdateCameraInfo (void)
{
	CG_UpdateCameraInfoExt(0);
}

static void CG_ResetCameraVelocities (int cameraNum)
{
	cameraPoint_t *cp;

	if (cameraNum < 0  ||  cameraNum >= cg.numCameraPoints) {
		return;
	}

	cp = &cg.cameraPoints[cameraNum];

	cp->useOriginVelocity = qfalse;
	//cp->originInitialVelocity = cp->originAvgVelocity;
	//cp->originFinalVelocity = cp->originAvgVelocity;

	cp->useAnglesVelocity = qfalse;
	cp->anglesInitialVelocity = cp->anglesAvgVelocity;
	cp->anglesFinalVelocity = cp->anglesAvgVelocity;

	cp->useXoffsetVelocity = qfalse;
	cp->useYoffsetVelocity = qfalse;
	cp->useZoffsetVelocity = qfalse;

	cp->useFovVelocity = qfalse;
	cp->useRollVelocity = qfalse;
}


static vec3_t Old[MAX_CAMERAPOINTS];

static const char *EcamHelpDoc = "\n"
"Edit all currently selected camera points\n"
"<...> are required\n"
"[...] are optional\n"
"\n"
"/ecam type <spline, interp, jump, curve, splinebezier, splinecatmullrom>\n"
"/ecam fov <current, interp, fixed, pass, spline> [fov value]\n"
"/ecam command <command to be executed when cam point is hit>\n"
"/ecam numsplines <number of spline points to use for this key point (default is 40)>\n"  //FIXME default 40 value
"/ecam angles <interp, spline, interpuseprevious, fixed, fixeduseprevious, viewpointinterp, viewpointfixed, viewpointpass, ent>\n"
"   the 'ent' option has additional parameter for the entity\n"
"   /ecam angles ent [entity number]\n"
"/ecam offset <interp, fixed, pass> [x offset] [y offset] [z offset]\n"
"/ecam roll <interp, fixed, pass, angles> [roll value]\n"
"/ecam flags [origin | angles | fov | time]\n"
"/ecam initialVelocity <origin, angles, xoffset, yoffset, zoffset, fov, roll> <value, or 'reset' to reset to default fixed velocity>\n"
"/ecam finalVelocity <origin, angles, xoffset, yoffset, zoffset, fov, roll> <value, or 'reset' to reset to default fixed velocity>\n"
"/ecam rebase [origin | angles | dir | dirna | time | timen <server time>] ...\n"
"   edit camera times to start now or at the time given time, use current\n"
"   angles, origin, or direction as the new starting values\n"
"   note:  dirna updates the camera direction without altering camera angles\n"
"/ecam shifttime <milliseconds>\n"
"/ecam smooth velocity\n"
"   change camera times to have the final immediate velocity of a camera point\n"
"   match the initial immediate velocity of the next camera point\n"
"/ecam smooth avgvelocity\n"
"   change camera times to have all points match the total average velocity\n"
"   run command multiple times for better precision\n"
"/ecam smooth origin\n"
"   match, if possible, the final immediate velocity of a camera point to the\n"
"   immediate initial velocity of the next camera point to have smooth origin\n"
"   transitions  (done by setting the appropriate overall final and initial\n"
"   velocities)\n"
"/ecam smooth originf\n"
"   aggresive origin smoothing which will change origins and camera times in\n"
"   order to match velocities\n"
"/ecam smooth anglesf\n"
"   aggresive angles smoothing which will change angles (but not times) in order\n"
"   to match angle velocities\n"
"/ecam smooth time\n"
"   change camera times so that the points have the same average velocity\n"
"/ecam scale <speed up/down scale value>\n"
"   speed up or down the selected camera points by adjusting camera time (2.0: twice as fast, 0.5: half speed)\n"
"\n";

static void CG_ChangeSelectedCameraPoints_f (void)
{
	cameraPoint_t *cp;
	cameraPoint_t *cpprev;
	cameraPoint_t *cpnext;
	int i;
	int j;
	const char *s;
	vec3_t angs0;
	vec3_t angs1;
	cameraPoint_t *cstart;
	const cameraPoint_t *cend;
	int n;

	cg.cameraPlaying = qfalse;
	cg.cameraPlayedLastFrame = qfalse;

	if (CG_Argc() < 2  ||  !Q_stricmp(CG_Argv(1), "help")) {
		Com_Printf("%s\n", EcamHelpDoc);
		return;
	}

	if (!Q_stricmp(CG_Argv(1), "reset")) {
		for (i = 0;  i < cg.numCameraPoints;  i++) {
			CG_ResetCameraVelocities(i);
		}

		CG_UpdateCameraInfo();
		return;
	}

	if (!Q_stricmp(CG_Argv(1), "rebase")) {
		if (CG_Argc() < 3) {
			Com_Printf("usage:  /ecam rebase [origin | angles | time | timen <server time>] ...\n");
			return;
		}

		if (cg.numCameraPoints < 1) {
			Com_Printf("no camera points\n");
			return;
		}

		n = 2;
		while (*CG_Argv(n)) {
			if (!Q_stricmp(CG_Argv(n), "origin")) {
				vec3_t offset;
				vec3_t newOrigin;

				VectorSubtract(cg.cameraPoints[0].origin, cg.refdef.vieworg, offset);

				for (i = 0;  i < cg.numCameraPoints;  i++) {
					VectorSubtract(cg.cameraPoints[i].origin, offset, newOrigin);
					VectorCopy(newOrigin, cg.cameraPoints[i].origin);
				}

				CG_CameraResetInternalLengths();
				CG_UpdateCameraInfo();
			}

			if (!Q_stricmp(CG_Argv(n), "angles")) {
				vec3_t offset;
				vec3_t newAngles;

				AnglesSubtract(cg.cameraPoints[0].angles, cg.refdefViewAngles, offset);
				for (i = 0;  i < cg.numCameraPoints;  i++) {
					AnglesSubtract(cg.cameraPoints[i].angles, offset, newAngles);
					VectorCopy(newAngles, cg.cameraPoints[i].angles);
				}

				CG_UpdateCameraInfo();
			}


			if (!Q_stricmp(CG_Argv(n), "dir")  ||  !Q_stricmp(CG_Argv(n), "dirna")) {
				vec3_t origForward, origRight, origUp;
				vec3_t newForward, newRight, newUp;
				vec3_t df, dr, du;
				qboolean noAngles;
				vec3_t p;
				vec3_t origOrigin, newOrigin;
				float scale;
				vec3_t angles;
				vec3_t dir;
				vec3_t tmpAngles;
				vec3_t fUp;

				if (cg.numCameraPoints < 2) {
					return;
				}

				if (!Q_stricmp(CG_Argv(n), "dirna")) {
					noAngles = qtrue;
				} else {
					noAngles = qfalse;
				}

				AngleVectors(cg.refdefViewAngles, newForward, newRight, newUp);
				VectorNormalize(newForward);
				VectorNormalize(newRight);
				VectorNormalize(newUp);

				VectorSubtract(cg.cameraPoints[1].origin, cg.cameraPoints[0].origin, origForward);
				VectorNormalize(origForward);
				vectoangles(origForward, angles);
				AngleVectors(angles, origForward, origRight, origUp);
				//AngleVectors(cg.cameraPoints[0].angles, NULL, NULL, aUp);
				//AngleVectors(cg.cameraPoints[0].angles, origForward, origRight, origUp);

				VectorCopy(cg.cameraPoints[0].origin, origOrigin);
				VectorCopy(cg.refdef.vieworg, cg.cameraPoints[0].origin);
				VectorCopy(cg.refdef.vieworg, newOrigin);
				VectorCopy(origOrigin, Old[0]);

				for (i = 1;  i < cg.numCameraPoints;  i++) {
					cp = &cg.cameraPoints[i];

					VectorCopy(cp->origin, Old[i]);

					VectorClear(p);
					ProjectPointOntoVector(cp->origin, origOrigin, origForward, p);
					VectorSubtract(p, origOrigin, df);
					ProjectPointOntoVector(cp->origin, origOrigin, origRight, p);
					VectorSubtract(p, origOrigin, dr);
					ProjectPointOntoVector(cp->origin, origOrigin, origUp, p);
					VectorSubtract(p, origOrigin, du);

					VectorClear(cp->origin);
					scale = VectorGetScale(df, origForward);
					VectorMA(newOrigin, scale, newForward, cp->origin);
					scale = VectorGetScale(dr, origRight);
					VectorMA(cp->origin, scale, newRight, cp->origin);
					scale = VectorGetScale(du, origUp);
					VectorMA(cp->origin, scale, newUp, cp->origin);

				}

				for (i = 0;  i < cg.numCameraPoints;  i++) {
					if (!noAngles) {
						float roll;
						vec3_t forward, anglePoint;
						vec3_t newAnglePoint;
						vec3_t realUp;
						vec3_t finalUp;
						float f, g;

						cp = &cg.cameraPoints[i];

						roll = cp->angles[ROLL];
						VectorCopy(cp->angles, tmpAngles);
						tmpAngles[ROLL] = 0;
						AngleVectors(tmpAngles, forward, NULL, realUp);
						VectorNormalize(forward);
						VectorMA(Old[i], 10, forward, anglePoint);
						VectorSubtract(anglePoint, Old[i], dir);
						vectoangles(dir, tmpAngles);
						AngleVectors(tmpAngles, NULL, NULL, fUp);
						f = AngleBetweenVectors(fUp, realUp);
						Com_Printf("%d:  %f   (%f %f)  (%f %f)\n", i, f, realUp[0], realUp[1], fUp[0], fUp[1]);

						VectorClear(p);
						ProjectPointOntoVector(anglePoint, origOrigin, origForward, p);
						VectorSubtract(p, origOrigin, df);
						ProjectPointOntoVector(anglePoint, origOrigin, origRight, p);
						VectorSubtract(p, origOrigin, dr);
						ProjectPointOntoVector(anglePoint, origOrigin, origUp, p);
						VectorSubtract(p, origOrigin, du);

						VectorClear(newAnglePoint);
						scale = VectorGetScale(df, origForward);
						VectorMA(newOrigin, scale, newForward, newAnglePoint);
						scale = VectorGetScale(dr, origRight);
						VectorMA(newAnglePoint, scale, newRight, newAnglePoint);
						scale = VectorGetScale(du, origUp);
						VectorMA(newAnglePoint, scale, newUp, newAnglePoint);

						VectorSubtract(newAnglePoint, cp->origin, dir);
						vectoangles(dir, cp->angles);
						cp->angles[YAW] = AngleNormalize180(cp->angles[YAW]);
						cp->angles[PITCH] = AngleNormalize180(cp->angles[PITCH]);
						AngleVectors(cp->angles, NULL, NULL, finalUp);

						//f = AngleBetweenVectors(realUp, origUp);
						g = AngleBetweenVectors(finalUp, newUp);

						Com_Printf("%d:  %f  %f\n", i, f, g);
						cp->angles[ROLL] = roll;  //90;  //roll;

					}
				}

				CG_CameraResetInternalLengths();
				CG_UpdateCameraInfo();
			}

			if (!Q_stricmp(CG_Argv(n), "time")) {
				double startTimeOrig;
				double ftime;

				startTimeOrig = cg.cameraPoints[0].cgtime;
				ftime = cg.ftime;

				for (i = 0;  i < cg.numCameraPoints;  i++) {
					cp = &cg.cameraPoints[i];
					cp->cgtime = (cp->cgtime - startTimeOrig) + ftime;
				}

				CG_UpdateCameraInfo();
			}

			if (!Q_stricmp(CG_Argv(n), "timen")) {
				double startTimeOrig;
				double ftime;

				startTimeOrig = cg.cameraPoints[0].cgtime;

				ftime = atof(CG_Argv(n + 1));
				n++;

				for (i = 0;  i < cg.numCameraPoints;  i++) {
					cp = &cg.cameraPoints[i];
					cp->cgtime = (cp->cgtime - startTimeOrig) + ftime;
				}

				CG_UpdateCameraInfo();
			}

			// Time shifting is done below since it works with selected camera
			// points.  'rebase' is for all points.
			n++;
		}

		return;
	}  // end '/ecam rebase ...'

	if (cg.selectedCameraPointMin < 0  ||  cg.selectedCameraPointMax >= cg.numCameraPoints) {
		Com_Printf("invalid min selected point\n");
		return;
	}
	if (cg.selectedCameraPointMax < 0  ||  cg.selectedCameraPointMax >= cg.numCameraPoints) {
		Com_Printf("invalid max selected point\n");
		return;
	}

#if 0
	if (!Q_stricmp(CG_Argv(1), "flip")) {
		for (i = cg.selectedCameraPointMin;  i <= cg.selectedCameraPointMax;  i++) {
			cp = &cg.cameraPoints[i];
			Com_Printf("%d\n", i);
			cp->angles[PITCH] = AngleNormalize180(cp->angles[PITCH] + 180);
			cp->angles[YAW] = AngleNormalize180(cp->angles[YAW] + 180);
		}
		CG_UpdateCameraInfo();
		return;
	}
#endif

	if (!Q_stricmp(CG_Argv(1), "shifttime")) {
		double shift;

		if (CG_Argc() < 3) {
			Com_Printf("usage:  /ecam shifttime <milliseconds>\n");
			return;
		}

		shift = atof(CG_Argv(2));

		for (i = cg.selectedCameraPointMin;  i < cg.selectedCameraPointMax;  i++) {
			cg.cameraPoints[i].cgtime += shift;
		}
		CG_UpdateCameraInfo();
		return;
	}

	if (!Q_stricmp(CG_Argv(1), "scale")) {
		double s;

		if (CG_Argc() < 3) {
			Com_Printf("usage:  /ecam scale <speed up/down scale value>\n");
			return;
		}

		s = atof(CG_Argv(2));

		if (s <= 0.0) {
			Com_Printf("invalid scale value\n");
			Com_Printf("usage:  /ecam scale <speed up/down scale value>\n");
			return;
		}

		s = 1.0 / s;

		for (i = cg.selectedCameraPointMin;  i < cg.selectedCameraPointMax;  i++) {
			double origTimeLength;
			double newTimeLength;
			double diff;
			double origNextTime;

			cp = &cg.cameraPoints[i];
			cpnext = &cg.cameraPoints[i + 1];

			origNextTime = cpnext->cgtime;
			origTimeLength = cpnext->cgtime - cp->cgtime;
			newTimeLength = origTimeLength * s;

			cpnext->cgtime = cp->cgtime + newTimeLength;
			diff = origNextTime - cpnext->cgtime;

			for (j = i + 2;  j <= cg.selectedCameraPointMax;  j++) {
				cg.cameraPoints[j].cgtime -= diff;
			}
		}
		CG_UpdateCameraInfo();
		return;
	}  // end '/ecam scale ...'

	// smoothing

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "time")) {
		int start, end;
		float totalLength;
		double totalTime;
		double avgVelocity;

		start = cg.selectedCameraPointMin;
		end = cg.selectedCameraPointMax;

		if (end > cg.numCameraPoints - 1) {
			end = cg.numCameraPoints - 1;
		}

		// get total length
		totalLength = 0;
		for (i = start;  i <= end;  i++) {
			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}
			totalLength += cp->originDistance;

		}

		if (totalLength <= 0) {
			Com_Printf("^3camera total length is 0, can't smoothen\n");
			return;
		}
		totalTime = cg.cameraPoints[end].cgtime - cg.cameraPoints[start].cgtime;
		avgVelocity = totalLength / totalTime;

		for (i = start;  i <= end;  i++) {
			double origNextTime;
			float scale;
			cameraPoint_t *cpTmp;
			double newTime;
			double oldTime;
			double diff;

			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}
			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
				cpnext = cpnext->next;
			}

			if (!cpnext) {
				break;
			}

			origNextTime = cpnext->cgtime;
			oldTime = cpnext->cgtime - cp->cgtime;

			newTime = cp->originDistance / avgVelocity;
			scale = newTime / oldTime;

			cpnext->cgtime = cp->cgtime + newTime;

			// scale the points in between the two CAM_ORIGIN points
			cpTmp = cp;
			while (cpTmp->next != cpnext) {
				double t;

				t = cpTmp->cgtime - cp->cgtime;
				t *= scale;
				cpTmp->cgtime = cp->cgtime + t;

				cpTmp = cpTmp->next;
			}

			// add the new diff to cpnext and beyond
			diff = cpnext->cgtime - origNextTime;
			cpTmp = cpnext->next;
			while (cpTmp) {
				cpTmp->cgtime += diff;

				cpTmp = cpTmp->next;
			}
		}

		CG_UpdateCameraInfo();
		return;
	}  // end '/ecam smooth time'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "origin")) {
		int start, end;

		start = cg.selectedCameraPointMin;
		end = cg.selectedCameraPointMax;

		if (start < 1) {
			start = 1;
		}
		if (end > cg.numCameraPoints - 1) {
			end = cg.numCameraPoints - 1;
		}

		for (i = start;  i <= end;  i++) {
			long double scale;

			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}

			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
				cpprev = cpprev->prev;
			}
			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
				cpnext = cpnext->next;
			}

			//cp->useOriginVelocity = qtrue;

			if (!cpprev) {
				Com_Printf("skipping %d, no previous camera point\n", i);
				continue;
			}
			if (!cpnext) {
				Com_Printf("skipping %d, no following camera point\n", i);
				continue;
			}

			if (cpprev->originImmediateFinalVelocity < 0.001) {  //FIXME
				Com_Printf("^3skipping %d, prev camera point final immediate velocity close to zero\n", i);
				continue;
			}

			// want
			// cp->originImmediateInitialVelocity == cpprev->originImmediateFinalVelocity;
			scale = cpprev->originImmediateFinalVelocity / cp->originImmediateInitialVelocity;
			if (cp->useOriginVelocity) {
				cp->originInitialVelocity *= scale;
			} else {
				cp->useOriginVelocity = qtrue;
				cp->originInitialVelocity = cp->originAvgVelocity * scale;
			}
			Com_Printf("%d  scale %f (%f / %f)  vel %f\n", i, (double)scale, (double)cpprev->originImmediateFinalVelocity, (double)cp->originImmediateInitialVelocity, (double)cp->originAvgVelocity);
			//cp->originInitialVelocity = cp->originAvgVelocity * scale;

			if (cp->originInitialVelocity < 0.0) {
				Com_Printf("^3velocity is less than zero: couldn't set correct initial velocity (%f)\n", (double)cp->originInitialVelocity);
				cp->originInitialVelocity = 0;
			}

			if (cp->originInitialVelocity > cp->originAvgVelocity * cg_cameraSmoothFactor.value) {
				Com_Printf("^3velocity is greater than %f * avgVelocity:  couldn't set correct initial velocity (%f)\n", cg_cameraSmoothFactor.value, (double)cp->originInitialVelocity);
				cp->originInitialVelocity = cp->originAvgVelocity * cg_cameraSmoothFactor.value;
			}

			cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
			CG_UpdateCameraInfo();
		}
		//CG_UpdateCameraInfo();
		return;
	}  // end '/ecam smooth origin'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "originf")) {
		int start, end;

		start = cg.selectedCameraPointMin;
		end = cg.selectedCameraPointMax;

		//FIXME why?  because prev is changed?
		if (start < 1) {
			start = 1;
		}
		if (end > cg.numCameraPoints - 1) {
			//FIXME why -2 and not -1 like '/ecam smooth origin'  ?
			end = cg.numCameraPoints - 2;
		}

		for (i = start;  i <= end;  i++) {
			vec3_t newOrigin;
			vec3_t dir;
			int runs;

			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}

			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
				cpprev = cpprev->prev;
			}
			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
				cpnext = cpnext->next;
			}

			//cp->useOriginVelocity = qtrue;

			if (!cpprev) {
				Com_Printf("skipping %d, no previous camera point\n", i);
				continue;
			}
			if (!cpnext) {
				Com_Printf("skipping %d, no following camera point\n", i);
				continue;
			}

			if (cpprev->originImmediateFinalVelocity < 0.001) {  //FIXME
				Com_Printf("^3skipping %d  prev camera point final immediate velocity close to zero\n", i);
				continue;
			}

			for (runs = 0;  runs < 30;  runs++) {
				if ((cp->originImmediateInitialVelocity * cg_cameraSmoothFactor.value) <= cpprev->originImmediateFinalVelocity) {
					long double diff;
					long double origCpTime;
					float length;
					cameraPoint_t *cptmp;

					cpprev->useOriginVelocity = qfalse;
					cp->useOriginVelocity = qfalse;
					cpnext->useOriginVelocity = qfalse;

					//cp->originInitialVelocity = cp->originAvgVelocity * 2.0;
					VectorSubtract(cpprev->origin, cp->origin, dir);
					length = VectorNormalize(dir);
					VectorMA(cp->origin, length * 0.1, dir, newOrigin);

					Com_Printf("%d origin:  (%f %f %f) to (%f %f %f)  -->  (%f %f %f)\n", i, cp->origin[0], cp->origin[1], cp->origin[2], newOrigin[0], newOrigin[1], newOrigin[2], cpprev->origin[0], cpprev->origin[1], cpprev->origin[2]);
					Com_Printf("  velocity: %f(imm) * %f(smooth)  <  %f(prev imm final)\n", cp->originImmediateInitialVelocity, cg_cameraSmoothFactor.value, cpprev->originImmediateFinalVelocity);

					VectorCopy(newOrigin, cp->origin);
					//VectorMA(cpprev->origin, -(cpprev->originDistance * 0.1), dir, cpprev->origin);

					//diff = cp->cgtime - cpprev->cgtime;
					diff = cpnext->cgtime - cp->cgtime;
					origCpTime = cp->cgtime;

					cpnext->cgtime -= (diff * 0.1);

					//diff = cp->cgtime - cpprev->cgtime;
					cp->cgtime += (diff * 0.1);

					cptmp = cp->next;
					while (cptmp  &&  !(cptmp->flags & CAM_ORIGIN)) {
						long double origFrac;

						origFrac = (cptmp->cgtime - origCpTime) / diff;
						cptmp->cgtime = origFrac * (cpnext->cgtime - cp->cgtime) + cp->cgtime;
					}


#if 0
					//FIXME 2015-09-04  why move this one?

					diff = cp->cgtime - cpprev->cgtime;
					//FIXME what if cpprev->cgtime now less than cpprev->prev->cgtime ?
					cpprev->cgtime -= (diff * 0.1);
#endif

					CG_UpdateCameraInfo();
				} else {
					break;
				}
			}
			//cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
			//CG_UpdateCameraInfo();
		}

		CG_CameraResetInternalLengths();
		CG_UpdateCameraInfo();
		return;
	}  // end '/ecam smooth originf'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "angles")) {
		int start, end;

		start = cg.selectedCameraPointMin;
		end = cg.selectedCameraPointMax;

		if (start < 1) {
			start = 1;
		}
		if (end > cg.numCameraPoints - 1) {
			end = cg.numCameraPoints - 1;
		}

		for (i = start;  i <= end;  i++) {
			long double scale;
			long double final, initial;

			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ANGLES)) {
				continue;
			}
			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ANGLES)) {
				cpprev = cpprev->prev;
			}
			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ANGLES)) {
				cpnext = cpnext->next;
			}

			if (cpprev == NULL  ||  cpnext == NULL) {
				continue;
			}

			if (cpprev->useAnglesVelocity) {
				final = cpprev->anglesFinalVelocity;
			} else {
				final = cpprev->anglesAvgVelocity;
			}

			if (cp->useAnglesVelocity) {
				initial = cp->anglesInitialVelocity;
			} else {
				initial = cp->anglesAvgVelocity;
			}

			if (initial <= 0.001) {
				initial = 0.01;
				//initial = 1;
			}

			scale = final / initial;

			if (cp->useAnglesVelocity) {
				cp->anglesInitialVelocity *= scale;
			} else {
				cp->useAnglesVelocity = qtrue;
				cp->anglesInitialVelocity = cp->anglesAvgVelocity * scale;
			}
			Com_Printf("%d  scale %f (%f / %f)  vel %f\n", i, (double)scale, (double)cpprev->anglesFinalVelocity, (double)cp->anglesInitialVelocity, (double)cp->anglesAvgVelocity);

			if (cp->anglesInitialVelocity < 0.0) {
			//if (cp->anglesInitialVelocity < cp->anglesAvgVelocity / 2.0) {
				Com_Printf("^3angle velocity is less than zero: couldn't set correct initial velocity (%f)\n", (double)cp->anglesInitialVelocity);
				cp->anglesInitialVelocity = 0;
				//cp->anglesInitialVelocity = cp->anglesAvgVelocity / 2.0;
			}

			if (cp->anglesInitialVelocity > cp->anglesAvgVelocity * cg_cameraSmoothFactor.value) {
				Com_Printf("^3angle velocity is greater than %f * avgVelocity:  couldn't set correct initial velocity (%f)\n", cg_cameraSmoothFactor.value, (double)cp->anglesInitialVelocity);
				cp->anglesInitialVelocity = cp->anglesAvgVelocity * cg_cameraSmoothFactor.value;
			}

			cp->anglesFinalVelocity = 2.0 * cp->anglesDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesInitialVelocity;
			CG_UpdateCameraInfo();
		}

		return;
	}  // end '/ecam smooth angles'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "anglesf")) {
		int start, end;
		int totalRuns;

		start = cg.selectedCameraPointMin;
		end = cg.selectedCameraPointMax;

		if (start < 1) {
			start = 1;
		}
		if (end > cg.numCameraPoints - 1) {
			end = cg.numCameraPoints - 1;
		}

		for (totalRuns = 0;  totalRuns < 10;  totalRuns++) {
			int numScaled;

			numScaled = 0;
			for (i = start;  i <= end;  i++) {
				vec3_t newAngles;
				int runs;
				long double scale;

				cp = &cg.cameraPoints[i];
				if (!(cp->flags & CAM_ANGLES)) {
					continue;
				}
				cpprev = cp->prev;
				while (cpprev  &&  !(cpprev->flags & CAM_ANGLES)) {
					cpprev = cpprev->prev;
				}
				cpnext = cp->next;
				while (cpnext  &&  !(cpnext->flags & CAM_ANGLES)) {
					cpnext = cpnext->next;
				}
				if (cpprev == NULL  ||  cpnext == NULL) {
					continue;
				}

				if (cpprev->anglesAvgVelocity <= 0.0) {
					//Com_Printf("%d skipping\n", i);
					continue;
				}

				scale = cg_cameraSmoothFactor.value;
				for (runs = 0;  runs < 10;  runs++) {
					if (cpprev->anglesAvgVelocity > scale * cp->anglesAvgVelocity) {
						//CG_ResetCameraVelocities(i);

						newAngles[YAW] = LerpAngle(cp->angles[YAW], cpprev->angles[YAW], 0.1);
						newAngles[PITCH] = LerpAngle(cp->angles[PITCH], cpprev->angles[PITCH], 0.1);
						if (cp->viewType == CAMERA_ANGLES_SPLINE) {
							newAngles[ROLL] = LerpAngle(cp->angles[ROLL], cpprev->angles[ROLL], 0.1);
						} else {
							// silence gcc warning
							newAngles[ROLL] = 0;
						}

						if (cp->viewType == CAMERA_ANGLES_SPLINE) {
							Com_Printf("%d:  (%f %f %f) -> (%f %f %f) vel: %f -> %f\n", i, newAngles[YAW], newAngles[PITCH], newAngles[ROLL], cpprev->angles[YAW], cpprev->angles[PITCH], cpprev->angles[ROLL], cp->anglesAvgVelocity, cpprev->anglesAvgVelocity);
						} else {
							Com_Printf("%d:  (%f %f) -> (%f %f) vel: %f -> %f\n", i, newAngles[YAW], newAngles[PITCH], cpprev->angles[YAW], cpprev->angles[PITCH], cp->anglesAvgVelocity, cpprev->anglesAvgVelocity);
						}

						cp->angles[PITCH] = newAngles[PITCH];
						cp->angles[YAW] = newAngles[YAW];
						if (cp->viewType == CAMERA_ANGLES_SPLINE) {
							cp->angles[ROLL] = newAngles[ROLL];
						}
						cp->useAnglesVelocity = qfalse;

						newAngles[YAW] = LerpAngle(cpprev->angles[YAW], cp->angles[YAW], 0.1);
						newAngles[PITCH] = LerpAngle(cpprev->angles[PITCH], cp->angles[PITCH], 0.1);
						if (cp->viewType == CAMERA_ANGLES_SPLINE) {
							newAngles[ROLL] = LerpAngle(cpprev->angles[ROLL], cp->angles[ROLL], 0.1);
						}
						cpprev->angles[YAW] = newAngles[YAW];
						cpprev->angles[PITCH] = newAngles[PITCH];
						if (cp->viewType == CAMERA_ANGLES_SPLINE) {
							cpprev->angles[ROLL] = newAngles[ROLL];
						}
						cpprev->useAnglesVelocity = qfalse;

						CG_UpdateCameraInfo();
						numScaled++;
					} else {
						break;
					}
				}
			}

			if (numScaled == 0) {
				break;
			}
		}

		CG_UpdateCameraInfo();
		return;
	}  // end '/ecam smooth anglesf'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "avgvelocity")) {
		double distance = 0;
		double avgSpeed;
		int runs;
		int pointMin, pointMax;

		pointMin = cg.selectedCameraPointMin;
		cp = &cg.cameraPoints[pointMin];
		while (cp  &&  !(cp->flags & CAM_ORIGIN)) {
			cp = cp->next;
			pointMin++;
		}
		if (cp == NULL) {
			Com_Printf("invalid starting point\n");
			return;
		}
		pointMax = cg.selectedCameraPointMax;
		cp = &cg.cameraPoints[pointMax];
		while (cp  &&  !(cp->flags & CAM_ORIGIN)) {
			cp = cp->prev;
			pointMax--;
		}
		if (cp == NULL) {
			Com_Printf("invalid ending point\n");
			return;
		}
		if (pointMin >= pointMax) {
			Com_Printf("invalid camera point range\n");
			return;
		}

		for (runs = 0;  runs < 30;  runs++) {

			distance = 0;

			for (i = pointMin;  i <= pointMax;  i++) {
				cp = &cg.cameraPoints[i];
				if (!(cp->flags & CAM_ORIGIN)) {
					continue;
				}
				distance += cp->originDistance;
			}
			if (distance <= 0.0) {
				return;
			}

			avgSpeed = distance / ((cg.cameraPoints[pointMax].cgtime - cg.cameraPoints[pointMin].cgtime) / 1000.0);
			if (avgSpeed <= 0.0) {
				return;
			}

			for (i = pointMin;  i < pointMax;  i++) {
				double newTime;

				cp = &cg.cameraPoints[i];
				if (!(cp->flags & CAM_ORIGIN)) {
					continue;
				}
				cpnext = cp->next;
				while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
					cpnext = cpnext->next;
				}
				if (cpnext == NULL) {
					break;
				}

				newTime = (cp->originDistance + avgSpeed * (cp->cgtime / 1000.0)) / avgSpeed;
				cpnext->cgtime = newTime * 1000.0;
			}

			CG_UpdateCameraInfo();
		}

		for (i = pointMin;  i < pointMax;  i++) {
			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}
			Com_Printf("cam %d  vel %f  %f -> %f\n", i, (double)cp->originAvgVelocity, (double)cp->originInitialVelocity, (double)cp->originFinalVelocity);
		}

		return;
	}  // end '/ecam smooth avgvelocity'

	if (!Q_stricmp(CG_Argv(1), "smooth")  &&  !Q_stricmp(CG_Argv(2), "velocity")) {
		long double ctime;
		long double scale;
		long double diff;
		double oldVelocity;
		int start;
		int end;

		start = cg.selectedCameraPointMin;
		if (start < 1) {
			start = 1;
		}

		end = cg.selectedCameraPointMax;
		if (end > cg.numCameraPoints - 2) {  // need cpnext
			end = cg.numCameraPoints - 2;
		}

		for (i = start;  i <= end;  i++) {
			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}

			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
				cpprev = cpprev->prev;
			}
			if (cpprev == NULL) {
				break;
			}

			cpnext = cp->next;
			while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
				cpnext = cpnext->next;
			}
			if (cpnext == NULL) {
				break;
			}

			oldVelocity = cp->originImmediateInitialVelocity;

			ctime = cpnext->cgtime - cp->cgtime;
			if (cpprev->originImmediateFinalVelocity > 0.0) {
				scale = cp->originImmediateInitialVelocity / cpprev->originImmediateFinalVelocity;
			} else {
				scale = 0;
			}

			if (scale > 1.0) {
				diff = (ctime * scale) - ctime;
			} else if (scale < 1.0  &&  scale > 0.001) {
				//diff = ctime - (ctime * (1.0 / scale));
				diff = -(1.0 - scale) * ctime;
			} else {
				diff = 0;
				continue;
			}

			for (j = i + 1;  j < cg.numCameraPoints - 1;  j++) {
				cg.cameraPoints[j].cgtime += diff;
			}
			CG_UpdateCameraInfo();
			Com_Printf("^3%d  scale %f   %f -> %f\n", i, (double)scale, oldVelocity, cp->originImmediateInitialVelocity);
		}

		CG_UpdateCameraInfo();

		for (i = 1;  i < cg.numCameraPoints - 1;  i++) {
			cp = &cg.cameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}
			cpprev = cp->prev;
			while (cpprev  &&  !(cpprev->flags & CAM_ORIGIN)) {
				cpprev = cpprev->prev;
			}
			if (cpprev == NULL) {
				break;
			}

			Com_Printf("%d  ours %f  prev %f\n", i, (double)cp->originImmediateInitialVelocity, (double)cpprev->originImmediateFinalVelocity);
		}

		return;
	}  // end '/ecam smooth velocity'

	if (!Q_stricmp(CG_Argv(1), "smooth")) {
		Com_Printf("usage:  /ecam smooth [origin | originf | angles | anglesf | avgvelocity | velocity | time]\n");
		return;
	}

	// set camera point values

	for (i = cg.selectedCameraPointMin;  i <= cg.selectedCameraPointMax;  i++) {
		cp = &cg.cameraPoints[i];

		if (i > 0) {
			cpprev = &cg.cameraPoints[i - 1];
		} else {
			cpprev = NULL;
		}

		if (i < (cg.numCameraPoints - 1)) {
			cpnext = &cg.cameraPoints[i + 1];
		} else {
			cpnext = NULL;
		}

		j = 1;
		s = CG_Argv(j);

		if (!Q_stricmp(s, "type")) {
			s = CG_Argv(j + 1);
			if (!Q_stricmp(s, "spline")) {
				cp->type = CAMERA_SPLINE;
			} else if (!Q_stricmp(s, "interp")) {
				cp->type = CAMERA_INTERP;
			} else if (!Q_stricmp(s, "jump")) {
				cp->type = CAMERA_JUMP;
			} else if (!Q_stricmp(s, "curve")) {
				cp->type = CAMERA_CURVE;
			} else if (!Q_stricmp(s, "splinebezier")) {
				cp->type = CAMERA_SPLINE_BEZIER;
			} else if (!Q_stricmp(s, "splinecatmullrom")) {
				cp->type = CAMERA_SPLINE_CATMULLROM;
			} else {
				Com_Printf("unknown camera type\n");
				Com_Printf("valid types:  spline interp jump curve splinebezier splinecatmullrom\n");
				return;
			}
		} else if (!Q_stricmp(s, "fov")) {
			s = CG_Argv(j + 1);
			if (!Q_stricmp(s, "current")) {
				cp->fovType = CAMERA_FOV_USE_CURRENT;
			} else if (!Q_stricmp(s, "interp")) {
				cp->fovType = CAMERA_FOV_INTERP;
			} else if (!Q_stricmp(s, "fixed")) {
				cp->fovType = CAMERA_FOV_FIXED;
			} else if (!Q_stricmp(s, "pass")) {
				cp->fovType = CAMERA_FOV_PASS;
			} else if (!Q_stricmp(s, "spline")) {
				cp->fovType = CAMERA_FOV_SPLINE;
			} else {
				Com_Printf("unknown fov type\n");
				Com_Printf("valid types:  current interp fixed pass spline\n");
				Com_Printf("current fov: %f\n", cp->fov);
				Com_Printf("usage:  /ecam fov <fov type> [fov value]\n");
				return;
			}
			if (!*CG_Argv(j + 2)) {
				continue;
			}
			cp->fov = atof(CG_Argv(j + 2));
			if (cp->fov <= 0.0) {
				cp->fov = -1;
			}
		} else if (!Q_stricmp(s, "command")) {
			cp->command[0] = '\0';
			j++;
			while (CG_Argv(j)[0]) {
				Q_strcat(cp->command, sizeof(cp->command), va("%s ", CG_Argv(j)));
				j++;
			}
		} else if (!Q_stricmp(s, "numsplines")) {
			s = CG_Argv(j + 1);
			if (!(s  &&  *s)) {
				//cp->numSplines = 40;  //FIXME define
				Com_Printf("current numsplines: %d\n", cp->numSplines);
				Com_Printf("usage:  /ecam numsplines <value>\n");
				return;
			} else {
				int ns;

				ns = atoi(s);
				if (ns < 2) {
					Com_Printf("invalid number of splines, can't be less than 2\n");
					return;
				} else {
					cp->numSplines = ns;
				}
			}
		} else if (!Q_stricmp(s, "angles")) {
			s = CG_Argv(j + 1);
			if (!Q_stricmp(s, "viewpointinterp")) {
				if (!cg.viewPointMarkSet) {
					Com_Printf("viewpointmark isn't set\n");
					return;
				}
				//FIXME if multiple cam points selected transition them
				VectorCopy(cg.viewPointMarkOrigin, cp->viewPointOrigin);
				cp->viewType = CAMERA_ANGLES_VIEWPOINT_INTERP;
			} else if (!Q_stricmp(s, "viewpointfixed")) {
				if (!cg.viewPointMarkSet) {
					Com_Printf("viewpointmark isn't set\n");
					return;
				}
				//FIXME if multiple cam points selected transition them
				VectorCopy(cg.viewPointMarkOrigin, cp->viewPointOrigin);
				cp->viewType = CAMERA_ANGLES_VIEWPOINT_FIXED;
			} else if (!Q_stricmp(s, "viewpointpass")) {
				cp->viewType = CAMERA_ANGLES_VIEWPOINT_PASS;
			} else if (!Q_stricmp(s, "ent")) {
				cp->viewType = CAMERA_ANGLES_ENT;
				s = CG_Argv(j + 2);
				if (*s) {
					cp->viewEnt = atoi(s);
				} else {
					Com_Printf("value for ent not specified\n");
					Com_Printf("current value: %d\n", cp->viewEnt);
					return;
				}
			} else if (!Q_stricmp(s, "interp")) {
				cp->viewType = CAMERA_ANGLES_INTERP;
			} else if (!Q_stricmp(s, "spline")) {
				cp->viewType = CAMERA_ANGLES_SPLINE;
			} else if (!Q_stricmp(s, "interpuseprevious")) {
				cp->viewType = CAMERA_ANGLES_INTERP_USE_PREVIOUS;
			} else if (!Q_stricmp(s, "fixed")) {
				cp->viewType = CAMERA_ANGLES_FIXED;
			} else if (!Q_stricmp(s, "fixeduseprevious")) {
				cp->viewType = CAMERA_ANGLES_FIXED_USE_PREVIOUS;
			} else {
				Com_Printf("unknown setting for angles '%s'\n", s);
				Com_Printf("valid types:  interp spline fixed fixeduseprevious interpuseprevious ent viewpointpass viewpointfixed viewpointinterp\n");
				return;
			}
		} else if (!Q_stricmp(s, "roll")) {
			j++;
			s = CG_Argv(j);
			if (!Q_stricmp(s, "interp")) {
				cp->rollType = CAMERA_ROLL_INTERP;
			} else if (!Q_stricmp(s, "fixed")) {
				cp->rollType = CAMERA_ROLL_FIXED;
			} else if (!Q_stricmp(s, "pass")) {
				cp->rollType = CAMERA_ROLL_PASS;
			} else if (!Q_stricmp(s, "angles")) {
				cp->rollType = CAMERA_ROLL_AS_ANGLES;
			} else {
				Com_Printf("unknown setting for roll '%s'\n", s);
				Com_Printf("valid values:  interp fixed pass angles\n");
				Com_Printf("current roll value: %f\n", cp->angles[ROLL]);
				Com_Printf("usage:  /ecam roll <roll type> [roll value]\n");
				return;
			}
			j++;
			s = CG_Argv(j);
			if (*s) {
				cp->angles[ROLL] = atof(s);
			}
		} else if (!Q_stricmp(s, "flags")) {
			int k;

			for (k = 2;  k < CG_Argc();  k++) {
				if (!Q_stricmp(CG_Argv(k), "origin")) {
					cp->flags ^= CAM_ORIGIN;
				} else if (!Q_stricmp(CG_Argv(k), "angles")) {
					cp->flags ^= CAM_ANGLES;
				} else if (!Q_stricmp(CG_Argv(k), "fov")) {
					cp->flags ^= CAM_FOV;
				} else if (!Q_stricmp(CG_Argv(k), "time")) {
					cp->flags ^= CAM_TIME;
				} else {
					Com_Printf("unknown flag type: '%s'\n", CG_Argv(k));
				}
			}

			// print flag values
			Com_Printf("[%d] flags: ", i);
			if (cp->flags & CAM_ORIGIN) {
				Com_Printf("origin ");
			}
			if (cp->flags & CAM_ANGLES) {
				Com_Printf("angles ");
			}
			if (cp->flags & CAM_FOV) {
				Com_Printf("fov ");
			}
			if (cp->flags & CAM_TIME) {
				Com_Printf("time ");
			}
			Com_Printf("\n");
		} else if (!Q_stricmp(s, "offset")) {
			j++;
			s = CG_Argv(j);
			if (!Q_stricmp(s, "interp")) {
				cp->offsetType = CAMERA_OFFSET_INTERP;
			} else if (!Q_stricmp(s, "fixed")) {
				cp->offsetType = CAMERA_OFFSET_FIXED;
			} else if (!Q_stricmp(s, "pass")) {
				cp->offsetType = CAMERA_OFFSET_PASS;
			} else {
				Com_Printf("uknown offset type '%s'\n", s);
				Com_Printf("valid values:  interp fixed pass\n");
				Com_Printf("current values: x: %f  y: %f  z: %f\n", cp->xoffset, cp->yoffset, cp->zoffset);
				Com_Printf("usage:  /ecam offset <offset type> [xoffset] [yoffset] [zoffset]\n");
				return;
			}
			j++;
			s = CG_Argv(j);
			if (*CG_Argv(j)) {
				cp->xoffset = atof(CG_Argv(j));
				if (*CG_Argv(j + 1)) {
					cp->yoffset = atof(CG_Argv(j + 1));
					if (*CG_Argv(j + 2)) {
						cp->zoffset = atof(CG_Argv(j + 2));
					}
				}
			}
		} else if (!Q_stricmp(s, "initialVelocity")) {
			j++;
			s = CG_Argv(j);
			if (!Q_stricmp(s, "origin")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (!Q_stricmp(s, "reset")) {
						cp->useOriginVelocity = qfalse;
						continue;
					} else {
						cp->useOriginVelocity = qtrue;
					}
					cp->originInitialVelocity = atof(s);
					if (cp->originInitialVelocity <= 0.0) {
						cp->originInitialVelocity = 0;
					}

					if (cp->originInitialVelocity > cp->originAvgVelocity * 2.0) {
						cp->originInitialVelocity = cp->originAvgVelocity * 2.0;
					}

					cp->originFinalVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originInitialVelocity;
					//Com_Printf("%f %f\n", cp->originInitialVelocity, cp->originFinalVelocity);
				} else {  // not given
					Com_Printf("[%d] origin initial velocity value:  %f\n", i, cp->originInitialVelocity);
				}
			} else if (!Q_stricmp(s, "angles")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->viewPointPassStart > -1) {
						cstart = &cg.cameraPoints[cp->viewPointPassStart];
						cend = &cg.cameraPoints[cp->viewPointPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useAnglesVelocity = qfalse;
						if (cstart) {
							cstart->useAnglesVelocity = qfalse;
						}
						continue;
					} else {
						cp->useAnglesVelocity = qtrue;
					}
					cp->anglesInitialVelocity = atof(s);
					if (cp->anglesInitialVelocity <= 0.0) {
						cp->anglesInitialVelocity = 0;
					}
					if (cp->viewType != CAMERA_ANGLES_INTERP_USE_PREVIOUS) {
						if (cp->anglesInitialVelocity > cp->anglesAvgVelocity * 2.0) {
							cp->anglesInitialVelocity = cp->anglesAvgVelocity * 2.0;
						}
					}
					if (cstart) {
						VectorCopy(cstart->viewPointOrigin, angs0);
						VectorCopy(cg.cameraPoints[cp->viewPointPassEnd].viewPointOrigin, angs1);
						cp->anglesFinalVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(angs0, angs1) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->anglesInitialVelocity;
					} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
						cp->anglesFinalVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(cp->viewPointOrigin, cpnext->viewPointOrigin) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesInitialVelocity;
					} else {
						VectorCopy(cp->angles, angs0);
						VectorCopy(cpnext->angles, angs1);
						angs0[ROLL] = 0;
						angs1[ROLL] = 0;

						cp->anglesFinalVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(angs0, angs1) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesInitialVelocity;
					}

					if (cstart) {
						cstart->useAnglesVelocity = qtrue;
						cstart->anglesInitialVelocity = cp->anglesInitialVelocity;
						cstart->anglesFinalVelocity = cp->anglesFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] angles initial velocity value:  %f\n", i, cp->anglesInitialVelocity);
				}
			} else if (!Q_stricmp(s, "xoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useXoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useXoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useXoffsetVelocity = qtrue;
					}
					cp->xoffsetInitialVelocity = atof(s);
					if (cp->xoffsetInitialVelocity <= 0.0) {
						cp->xoffsetInitialVelocity = 0;
					}
					if (cp->xoffsetInitialVelocity > cp->xoffsetAvgVelocity * 2.0) {
						cp->xoffsetInitialVelocity = cp->xoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->xoffsetFinalVelocity = 2.0 * fabs(cend->xoffset - cstart->xoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->xoffsetInitialVelocity;
					} else {
						cp->xoffsetFinalVelocity = 2.0 * fabs(cpnext->xoffset - cp->xoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->xoffsetInitialVelocity;
					}
					if (cstart) {
						cstart->useXoffsetVelocity = qtrue;
						cstart->xoffsetInitialVelocity = cp->xoffsetInitialVelocity;
						cstart->xoffsetFinalVelocity = cp->xoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] xoffset initial velocity value:  %f\n", i, cp->xoffsetInitialVelocity);
				}
			} else if (!Q_stricmp(s, "yoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useYoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useYoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useYoffsetVelocity = qtrue;
					}
					cp->yoffsetInitialVelocity = atof(s);
					if (cp->yoffsetInitialVelocity <= 0.0) {
						cp->yoffsetInitialVelocity = 0;
					}
					if (cp->yoffsetInitialVelocity > cp->yoffsetAvgVelocity * 2.0) {
						cp->yoffsetInitialVelocity = cp->yoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->yoffsetFinalVelocity = 2.0 * fabs(cend->yoffset - cstart->yoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->yoffsetInitialVelocity;
					} else {
						cp->yoffsetFinalVelocity = 2.0 * fabs(cpnext->yoffset - cp->yoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->yoffsetInitialVelocity;
					}
					if (cstart) {
						cstart->useYoffsetVelocity = qtrue;
						cstart->yoffsetInitialVelocity = cp->yoffsetInitialVelocity;
						cstart->yoffsetFinalVelocity = cp->yoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] yoffset initial velocity value:  %f\n", i, cp->yoffsetInitialVelocity);
				}
			} else if (!Q_stricmp(s, "zoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useZoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useZoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useZoffsetVelocity = qtrue;
					}
					cp->zoffsetInitialVelocity = atof(s);
					if (cp->zoffsetInitialVelocity <= 0.0) {
						cp->zoffsetInitialVelocity = 0;
					}
					if (cp->zoffsetInitialVelocity > cp->zoffsetAvgVelocity * 2.0) {
						cp->zoffsetInitialVelocity = cp->zoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->zoffsetFinalVelocity = 2.0 * fabs(cend->zoffset - cstart->zoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->zoffsetInitialVelocity;
					} else {
						cp->zoffsetFinalVelocity = 2.0 * fabs(cpnext->zoffset - cp->zoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->zoffsetInitialVelocity;
					}
					if (cstart) {
						cstart->useZoffsetVelocity = qtrue;
						cstart->zoffsetInitialVelocity = cp->zoffsetInitialVelocity;
						cstart->zoffsetFinalVelocity = cp->zoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] zoffset initial velocity value:  %f\n", i, cp->zoffsetInitialVelocity);
				}
			} else if (!Q_stricmp(s, "fov")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->fovPassStart > -1) {
						cstart = &cg.cameraPoints[cp->fovPassStart];
						cend = &cg.cameraPoints[cp->fovPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useFovVelocity = qfalse;
						if (cstart) {
							cstart->useFovVelocity = qfalse;
						}
						continue;
					} else {
						cp->useFovVelocity = qtrue;
					}
					cp->fovInitialVelocity = atof(s);
					if (cp->fovInitialVelocity <= 0.0) {
						cp->fovInitialVelocity = 0;
					}
					if (cp->fovInitialVelocity > cp->fovAvgVelocity * 2.0) {
						cp->fovInitialVelocity = cp->fovAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->fovFinalVelocity = 2.0 * fabs(cend->fov - cstart->fov) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->fovInitialVelocity;
					} else {
						cp->fovFinalVelocity = 2.0 * fabs(cpnext->fov - cp->fov) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->fovInitialVelocity;
					}
					if (cstart) {
						cstart->useFovVelocity = qtrue;
						cstart->fovInitialVelocity = cp->fovInitialVelocity;
						cstart->fovFinalVelocity = cp->fovFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] fov initial velocity value:  %f\n", i, cp->fovInitialVelocity);
				}
			} else if (!Q_stricmp(s, "roll")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->rollPassStart > -1) {
						cstart = &cg.cameraPoints[cp->rollPassStart];
						cend = &cg.cameraPoints[cp->rollPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useRollVelocity = qfalse;
						if (cstart) {
							cstart->useRollVelocity = qfalse;
						}
						continue;
					} else {
						cp->useRollVelocity = qtrue;
					}
					cp->rollInitialVelocity = atof(s);
					if (cp->rollInitialVelocity <= 0.0) {
						cp->rollInitialVelocity = 0;
					}
					if (cp->rollInitialVelocity > cp->rollAvgVelocity * 2.0) {
						cp->rollInitialVelocity = cp->rollAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->rollFinalVelocity = 2.0 * fabs(AngleSubtract(cend->angles[ROLL], cstart->angles[ROLL])) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->rollInitialVelocity;
					} else {
						cp->rollFinalVelocity = 2.0 * fabs(AngleSubtract(cpnext->angles[ROLL], cp->angles[ROLL])) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->rollInitialVelocity;
					}
					if (cstart) {
						cstart->useRollVelocity = qtrue;
						cstart->rollInitialVelocity = cp->rollInitialVelocity;
						cstart->rollFinalVelocity = cp->rollFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] roll initial velocity value:  %f\n", i, cp->rollInitialVelocity);
				}
			} else {
				Com_Printf("unknown initial velocity type '%s'\n", s);
				Com_Printf("valid values:  origin angles xoffset yoffset zoffset fov roll\n");
				Com_Printf("usage:  /ecam initialVelocity <type> <value or 'reset'>\n");
				return;
			}
		} else if (!Q_stricmp(s, "finalVelocity")) {
			j++;
			s = CG_Argv(j);
			if (!Q_stricmp(s, "origin")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (!Q_stricmp(s, "reset")) {
						cp->useOriginVelocity = qfalse;
						continue;
					} else {
						cp->useOriginVelocity = qtrue;
					}
					cp->originFinalVelocity = atof(s);
					if (cp->originFinalVelocity <= 0.0) {
						cp->originFinalVelocity = 0;
					}
					if (cp->originFinalVelocity > cp->originAvgVelocity * 2.0) {
						cp->originFinalVelocity = cp->originAvgVelocity * 2.0;
					}
					cp->originInitialVelocity = 2.0 * cp->originDistance / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->originFinalVelocity;
				} else {  // not given
					Com_Printf("[%d] origin final velocity value:  %f\n", i, cp->originFinalVelocity);
				}
			} else if (!Q_stricmp(s, "angles")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->viewPointPassStart > -1) {
						cstart = &cg.cameraPoints[cp->viewPointPassStart];
						cend = &cg.cameraPoints[cp->viewPointPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useAnglesVelocity = qfalse;
						if (cstart) {
							cstart->useAnglesVelocity = qfalse;
						}
						continue;
					} else {
						cp->useAnglesVelocity = qtrue;
					}
					cp->anglesFinalVelocity = atof(s);
					if (cp->anglesFinalVelocity <= 0.0) {
						cp->anglesFinalVelocity = 0;
					}
					if (cp->viewType != CAMERA_ANGLES_INTERP_USE_PREVIOUS) {
						if (cp->anglesFinalVelocity > cp->anglesAvgVelocity * 2.0) {
							cp->anglesFinalVelocity = cp->anglesAvgVelocity * 2.0;
						}
					}
					if (cstart) {
						VectorCopy(cstart->viewPointOrigin, angs0);
						VectorCopy(cg.cameraPoints[cp->viewPointPassEnd].viewPointOrigin, angs1);
						cp->anglesInitialVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(angs0, angs1) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->anglesFinalVelocity;
					} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
						cp->anglesInitialVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(cp->viewPointOrigin, cpnext->viewPointOrigin) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesFinalVelocity;
					} else {
						VectorCopy(cp->angles, angs0);
						VectorCopy(cpnext->angles, angs1);
						angs0[ROLL] = 0;
						angs1[ROLL] = 0;

						cp->anglesInitialVelocity = 2.0 * CG_CameraAngleLongestDistanceNoRoll(angs0, angs1) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->anglesFinalVelocity;
					}
					if (cstart) {
						cstart->useAnglesVelocity = qtrue;
						cstart->anglesInitialVelocity = cp->anglesInitialVelocity;
						cstart->anglesFinalVelocity = cp->anglesFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] angles final velocity value:  %f\n", i, cp->anglesFinalVelocity);
				}
			} else if (!Q_stricmp(s, "xoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useXoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useXoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useXoffsetVelocity = qtrue;
					}
					cp->xoffsetFinalVelocity = atof(s);
					if (cp->xoffsetFinalVelocity <= 0.0) {
						cp->xoffsetFinalVelocity = 0;
					}
					if (cp->xoffsetFinalVelocity > cp->xoffsetAvgVelocity * 2.0) {
						cp->xoffsetFinalVelocity = cp->xoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->xoffsetInitialVelocity = 2.0 * fabs(cend->xoffset - cstart->xoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->xoffsetFinalVelocity;
					} else {
						cp->xoffsetInitialVelocity = 2.0 * fabs(cpnext->xoffset - cp->xoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->xoffsetFinalVelocity;
					}
					if (cstart) {
						cstart->useXoffsetVelocity = qtrue;
						cstart->xoffsetInitialVelocity = cp->xoffsetInitialVelocity;
						cstart->xoffsetFinalVelocity = cp->xoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] xoffset final velocity value:  %f\n", i, cp->xoffsetFinalVelocity);
				}
			} else if (!Q_stricmp(s, "yoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useYoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useYoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useYoffsetVelocity = qtrue;
					}
					cp->yoffsetFinalVelocity = atof(s);
					if (cp->yoffsetFinalVelocity <= 0.0) {
						cp->yoffsetFinalVelocity = 0;
					}
					if (cp->yoffsetFinalVelocity > cp->yoffsetAvgVelocity * 2.0) {
						cp->yoffsetFinalVelocity = cp->yoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->yoffsetInitialVelocity = 2.0 * fabs(cend->yoffset - cstart->yoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->yoffsetFinalVelocity;
					} else {
						cp->yoffsetInitialVelocity = 2.0 * fabs(cpnext->yoffset - cp->yoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->yoffsetFinalVelocity;
					}
					if (cstart) {
						cstart->useYoffsetVelocity = qtrue;
						cstart->yoffsetInitialVelocity = cp->yoffsetInitialVelocity;
						cstart->yoffsetFinalVelocity = cp->yoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] yoffset final velocity value:  %f\n", i, cp->yoffsetFinalVelocity);
				}
			} else if (!Q_stricmp(s, "zoffset")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->offsetPassStart > -1) {
						cstart = &cg.cameraPoints[cp->offsetPassStart];
						cend = &cg.cameraPoints[cp->offsetPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useZoffsetVelocity = qfalse;
						if (cstart) {
							cstart->useZoffsetVelocity = qfalse;
						}
						continue;
					} else {
						cp->useZoffsetVelocity = qtrue;
					}
					cp->zoffsetFinalVelocity = atof(s);
					if (cp->zoffsetFinalVelocity <= 0.0) {
						cp->zoffsetFinalVelocity = 0;
					}
					if (cp->zoffsetFinalVelocity > cp->zoffsetAvgVelocity * 2.0) {
						cp->zoffsetFinalVelocity = cp->zoffsetAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->zoffsetInitialVelocity = 2.0 * fabs(cend->zoffset - cstart->zoffset) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->zoffsetFinalVelocity;
					} else {
						cp->zoffsetInitialVelocity = 2.0 * fabs(cpnext->zoffset - cp->zoffset) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->zoffsetFinalVelocity;
					}
					if (cstart) {
						cstart->useZoffsetVelocity = qtrue;
						cstart->zoffsetInitialVelocity = cp->zoffsetInitialVelocity;
						cstart->zoffsetFinalVelocity = cp->zoffsetFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] zoffset final velocity value:  %f\n", i, cp->zoffsetFinalVelocity);
				}
			} else if (!Q_stricmp(s, "fov")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->fovPassStart > -1) {
						cstart = &cg.cameraPoints[cp->fovPassStart];
						cend = &cg.cameraPoints[cp->fovPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useFovVelocity = qfalse;
						if (cstart) {
							cstart->useFovVelocity = qfalse;
						}
						continue;
					} else {
						cp->useFovVelocity = qtrue;
					}
					cp->fovFinalVelocity = atof(s);
					if (cp->fovFinalVelocity <= 0.0) {
						cp->fovFinalVelocity = 0;
					}
					if (cp->fovFinalVelocity > cp->fovAvgVelocity * 2.0) {
						cp->fovFinalVelocity = cp->fovAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->fovInitialVelocity = 2.0 * fabs(cend->fov - cstart->fov) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->fovFinalVelocity;
					} else {
						cp->fovInitialVelocity = 2.0 * fabs(cpnext->fov - cp->fov) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->fovFinalVelocity;
					}
					if (cstart) {
						cstart->useFovVelocity = qtrue;
						cstart->fovInitialVelocity = cp->fovInitialVelocity;
						cstart->fovFinalVelocity = cp->fovFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] fov final velocity value:  %f\n", i, cp->fovFinalVelocity);
				}
			} else if (!Q_stricmp(s, "roll")) {
				j++;
				s = CG_Argv(j);
				if (*s  &&  cpnext) {
					if (cp->rollPassStart > -1) {
						cstart = &cg.cameraPoints[cp->rollPassStart];
						cend = &cg.cameraPoints[cp->rollPassEnd];
					} else {
						cstart = NULL;
						cend = NULL;
					}
					if (!Q_stricmp(s, "reset")) {
						cp->useRollVelocity = qfalse;
						if (cstart) {
							cstart->useRollVelocity = qfalse;
						}
						continue;
					} else {
						cp->useRollVelocity = qtrue;
					}
					cp->rollFinalVelocity = atof(s);
					if (cp->rollFinalVelocity <= 0.0) {
						cp->rollFinalVelocity = 0;
					}
					if (cp->rollFinalVelocity > cp->rollAvgVelocity * 2.0) {
						cp->rollFinalVelocity = cp->rollAvgVelocity * 2.0;
					}
					if (cstart) {
						cp->rollInitialVelocity = 2.0 * fabs(AngleSubtract(cend->angles[ROLL],  cstart->angles[ROLL])) / ((cend->cgtime - cstart->cgtime) / 1000.0) - cp->rollFinalVelocity;
					} else {
						cp->rollInitialVelocity = 2.0 * fabs(AngleSubtract(cpnext->angles[ROLL], cp->angles[ROLL])) / ((cpnext->cgtime - cp->cgtime) / 1000.0) - cp->rollFinalVelocity;
					}
					if (cstart) {
						cstart->useRollVelocity = qtrue;
						cstart->rollInitialVelocity = cp->rollInitialVelocity;
						cstart->rollFinalVelocity = cp->rollFinalVelocity;
					}
				} else {  // not given
					Com_Printf("[%d] roll final velocity value:  %f\n", i, cp->rollFinalVelocity);
				}
			} else {
				Com_Printf("unknown final velocity type '%s'\n", s);
				Com_Printf("valid values:  origin angles xoffset yoffset zoffset fov roll\n");
				Com_Printf("usage:  /ecam finalVelocity <type> <value or 'reset'>\n");
				return;
			}
		} else {
			Com_Printf("unknown setting:  '%s'\n", s);
			return;
		}
	}

	CG_CameraResetInternalLengths();
	CG_UpdateCameraInfo();
}

static void CG_SetViewPointMark_f (void)
{
	cg.viewPointMarkSet = qtrue;
	VectorCopy(cg.refdef.vieworg, cg.viewPointMarkOrigin);
}

//static void CG_DeleteViewPointMark_f (void)
//{
//	cg.useViewPointMark = qfalse;
//}

static void CG_GotoViewPointMark_f (void)
{
	if (!cg.viewPointMarkSet) {
		Com_Printf("no viewpoint mark set\n");
		return;
	}

	VectorCopy(cg.viewPointMarkOrigin, cg.fpos);
	VectorCopy(cg.viewPointMarkOrigin, cg.freecamPlayerState.origin);
	cg.freecamPlayerState.origin[2] -= DEFAULT_VIEWHEIGHT;
	VectorSet(cg.freecamPlayerState.velocity, 0, 0, 0);
}

//FIXME horrible

void CG_CleanUpFieldNumber (void)
{
	int s;

	//cg.selectedCameraPointField++;
	s = cg.selectedCameraPointField;
	if (s < CEF_ORIGIN) {
		s = CEF_COMMAND;
	} else if (s > CEF_COMMAND) {
		s = CEF_ORIGIN;
	}

	cg.selectedCameraPointField = s;
}

static void CG_SelectNextField_f (void)
{
	cg.selectedCameraPointField++;
	CG_CleanUpFieldNumber();
}

static void CG_SelectPrevField_f (void)
{
	cg.selectedCameraPointField--;
	CG_CleanUpFieldNumber();
}

static void CG_ChangeSelectedField_f (void)
{
	int n;
	cameraPoint_t *cp;

	if (cg.numCameraPoints < 1) {
		return;
	}

	cp = &cg.cameraPoints[cg.selectedCameraPointMin];

	//FIXME edit all selected?

	n = cg.selectedCameraPointField;
	if (n == CEF_ORIGIN) {
		VectorCopy(cg.refdef.vieworg, cp->origin);
	} else if (n == CEF_ANGLES) {
		VectorCopy(cg.refdefViewAngles, cp->angles);
	} else if (n == CEF_CAMERA_TYPE) {
		cp->type++;
		if (cp->type >= CAMERA_ENUM_END) {
			cp->type = CAMERA_SPLINE;
		}
	} else if (n == CEF_VIEW_TYPE) {
		cp->viewType++;
		if (cp->viewType >= CAMERA_ANGLES_ENUM_END) {
			cp->viewType = CAMERA_ANGLES_INTERP;
		}
	} else if (n == CEF_ROLL_TYPE) {
		cp->rollType++;
		if (cp->rollType >= CAMERA_ROLL_ENUM_END) {
			cp->rollType = CAMERA_ROLL_INTERP;
		}
	} else if (n == CEF_FLAGS) {
		CG_ErrorPopup("use /ecam flags");
	} else if (n == CEF_NUMBER_OF_SPLINES) {
		CG_ErrorPopup("use /ecam numsplines <value>");
	} else if (n == CEF_VIEWPOINT_ORIGIN) {
		VectorCopy(cg.refdef.vieworg, cp->viewPointOrigin);
		cp->viewPointOriginSet = qtrue;
		cp->viewType = CAMERA_ANGLES_VIEWPOINT_INTERP;
	} else if (n == CEF_VIEW_ENT) {
		//FIXME maybe find best match
		if (cg.viewEnt > -1) {
			cp->viewType = CAMERA_ANGLES_ENT;
			cp->viewEnt = cg.viewEnt;
			cp->offsetType = CAMERA_OFFSET_INTERP;
			cp->xoffset = cg.viewEntOffsetX;
			cp->yoffset = cg.viewEntOffsetY;
			cp->zoffset = cg.viewEntOffsetZ;
			VectorCopy(cg_entities[cg.viewEnt].currentState.pos.trBase, cp->viewEntStartingOrigin);
			cp->viewEntStartingOriginSet = qtrue;
		} else {
			CG_ErrorPopup("currently not viewing something, use /ecam ent <entity number>");
		}
	} else if (n == CEF_VIEW_ENT_OFFSETS) {
		CG_ErrorPopup("use /ecam offset <interp, fixed, pass> [x] [y] [z]");
	} else if (n == CEF_OFFSET_TYPE) {
		cp->offsetType++;
		if (cp->offsetType >= CAMERA_OFFSET_ENUM_END) {
			cp->offsetType = CAMERA_OFFSET_INTERP;
		}
	} else if (n == CEF_FOV) {
		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			cp->fov = cg.demoFov;
		} else {
			cp->fov = cg_fov.value;
		}
	} else if (n == CEF_FOV_TYPE) {
		cp->fovType++;
		if (cp->fovType >= CAMERA_FOV_ENUM_END) {
			cp->fovType = CAMERA_FOV_USE_CURRENT;
		}
	} else if (n >= CEF_INITIAL_VELOCITY  &&  n <= CEF_ROLL_PREV_VELOCITY) {
		CG_ErrorPopup("use /ecam <initialVelocity | finalVelocity> <type> <value>");
	} else if (n == CEF_COMMAND) {
		CG_ErrorPopup("use /ecam command <command to be executed when this cam point is hit>");
	} else {
		Com_Printf("invalid field number, shouldn't happen\n");
	}
	CG_UpdateCameraInfo();
}

static void CG_FXMath_f (void)
{
	char s[1024];
	int i;
	int argc;
	float f;
	int err;

	s[0] = '\0';
	argc = CG_Argc();
	if (argc < 2) {
		Com_Printf("usage:  fxmath <math expression>\n");
		Com_Printf("example:  fxmath (3 + 8) * 4 + (2.2 * 3.3)\n");
		Com_Printf("          fxmath sin(45.3 / 1.2)\n");
		return;
	}

	for (i = 1;  i < argc;  i++) {
		Q_strcat(s, sizeof(s), CG_Argv(i));
		Q_strcat(s, sizeof(s), " ");
	}

	CG_Q3mmeMath(s, &f, &err);
	Com_Printf("%f\n", f);
}

static void CG_Vibrate_f (void)
{
	float f;

	if (CG_Argc() < 2) {
		f = 100;
	} else {
		f = atof(CG_Argv(1));
	}

	cg.vibrateCameraTime = cg.time;
	cg.vibrateCameraValue = f;
	cg.vibrateCameraPhase = crandom() * M_PI;
}

static void CG_FXLoad_f (void)
{
	if (CG_Argc() > 1) {
		CG_ReloadQ3mmeScripts(CG_Argv(1));
	} else {
		CG_ReloadQ3mmeScripts(cg_fxfile.string);
	}
}

static void CG_LocalEnts_f (void)
{
	CG_ListLocalEntities();
}

static void CG_ClearFX_f (void)
{
	float emitterId;

	if (CG_Argc() < 2) {
		CG_RemoveFXLocalEntities(qtrue, 0);
	} else {
		emitterId = atof(CG_Argv(1));
		CG_RemoveFXLocalEntities(qfalse, emitterId);
	}
}

static void CG_GotoAdvertisement_f (void)
{
	int n;
	vec3_t normal;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  gotoad <add number>\n");
		return;
	}

	if (!cgs.adsLoaded) {
		Com_Printf("ads not loaded yet\n");
		return;
	}

	n = atoi(CG_Argv(1));
	if (n <= 0) {
		Com_Printf("invalid ad number\n");
		return;
	}
	if (n > cgs.numAds) {
		Com_Printf("%d > cgs.numAds (%d)\n", n, cgs.numAds);
		return;
	}

	cg.fMoveTime = 0;
	cg.fpos[0] = cgs.adverts[(n - 1) * 16 + 0];
	cg.fpos[1] = cgs.adverts[(n - 1) * 16 + 1];
	cg.fpos[2] = cgs.adverts[(n - 1) * 16 + 2];

	normal[0] = cgs.adverts[(n - 1) * 16 + 12];
	normal[1] = cgs.adverts[(n - 1) * 16 + 13];
	normal[2] = cgs.adverts[(n - 1) * 16 + 14];

	VectorMA(cg.fpos, 100, normal, cg.fpos);
	VectorScale(normal, -1, normal);
	vectoangles(normal, cg.fang);

	trap_SendConsoleCommand(va("cmd setviewpos %d %d %d %d\n", (int)cg.fpos[0], (int)cg.fpos[1], (int)cg.fpos[2], (int)cg.fang[YAW]));
}

void CG_ChangeConfigString (const char *buffer, int index)
{
	const char *s = buffer;
	gameState_t oldGs;
	int i;
	const char *dup;
	int len;
	const char *old;

	//Com_Printf("%d: '%s'\n", index, buffer);

	old = cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cgs.gameState;

	Com_Memset( &cgs.gameState, 0, sizeof( cgs.gameState ) );

	// leave the first 0 for uninitialized strings
	cgs.gameState.dataCount = 1;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cgs.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Printf("^3CG_ChangeConfigString() MAX_GAMESTATE_CHARS exceeded\n");
			cgs.gameState = oldGs;
			return;
		}

		// append it to the gameState string buffer
		cgs.gameState.stringOffsets[ i ] = cgs.gameState.dataCount;
		Com_Memcpy( cgs.gameState.stringData + cgs.gameState.dataCount, dup, len + 1 );
		cgs.gameState.dataCount += len + 1;
	}
}

static void CG_ClientOverrideClearClient (int clientNum)
{
	clientInfo_t *ci;

	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^3%s invalid client number: %d\n", __func__, clientNum);
	}

	ci = &cgs.clientinfo[clientNum];

	ci->override = qfalse;
	ci->hasHeadColor = qfalse;
	ci->hasTorsoColor = qfalse;
	ci->hasLegsColor = qfalse;
	ci->hasHeadSkin = qfalse;
	ci->hasTorsoSkin = qfalse;
	ci->hasLegsSkin = qfalse;

	ci->hasModelScale = qfalse;
	ci->hasLegsModelScale = qfalse;
	ci->hasTorsoModelScale = qfalse;
	ci->hasHeadModelScale = qfalse;
	ci->hasHeadOffset = qfalse;
	ci->hasModelAutoScale = qfalse;

	if (!ci->infoValid) {
		return;
	}
	CG_NewClientInfo(clientNum);
}

static char HeadSkinName[MAX_QPATH];
static char TorsoSkinName[MAX_QPATH];
static char LegsSkinName[MAX_QPATH];

static void CG_ClientOverride_f (void)
{
	int i;
	int j;
	const char *clientInfoString;
	qboolean useTeam;
	int team = -3;
	int clientNum = -1;
	char key[MAX_QPATH];
	char value[MAX_QPATH];
	char buffer[1024];
	byte headColor[4];
	byte torsoColor[4];
	byte legsColor[4];
	qboolean changeHeadColor;
	qboolean changeTorsoColor;
	qboolean changeLegsColor;
	qboolean changeHeadSkin;
	qboolean changeTorsoSkin;
	qboolean changeLegsSkin;
	int color;
	int start;
	int end;
	clientInfo_t *ci;
	char modelStr[MAX_QPATH];
	char skinStr[MAX_QPATH];
	clientInfo_t tmpCi;

#define T_ENEMY -1
#define T_ALL -2
#define T_FRIENDLY -3

	if (!cg.snap) {
		return;
	}

	// silence compiler warning
	memset(headColor, 0, sizeof(headColor));
	memset(torsoColor, 0, sizeof(torsoColor));
	memset(legsColor, 0, sizeof(legsColor));

	if (CG_Argc() > 1) {
		if (!Q_stricmp("clear", CG_Argv(1))) {
			int t;

			trap_GetGameState(&cgs.gameState);

			if (CG_Argc() > 2) {
				if (!Q_stricmp("red", CG_Argv(2))) {
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						clientInfoString = CG_ConfigString(CS_PLAYERS + i);
						//FIXME option for this
						if (!clientInfoString[0]) {
							continue;
						}
						t = atoi(Info_ValueForKey(clientInfoString, "t"));
						if (t == TEAM_RED) {
							CG_ClientOverrideClearClient(i);
						}
					}
				} else if (!Q_stricmp("blue", CG_Argv(2))) {
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						clientInfoString = CG_ConfigString(CS_PLAYERS + i);
						//FIXME option for this
						if (!clientInfoString[0]) {
							continue;
						}
						t = atoi(Info_ValueForKey(clientInfoString, "t"));
						if (t == TEAM_BLUE) {
							CG_ClientOverrideClearClient(i);
						}
					}
				} else if (!Q_stricmp("enemy", CG_Argv(2))) {
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						clientInfoString = CG_ConfigString(CS_PLAYERS + i);
						//FIXME option for this
						if (!clientInfoString[0]) {
							continue;
						}
						t = atoi(Info_ValueForKey(clientInfoString, "t"));
						if (CG_IsTeamGame(cgs.gametype)  &&  t != cg.snap->ps.persistant[PERS_TEAM]) {
							CG_ClientOverrideClearClient(i);
						}
					}
				} else if (!Q_stricmp("mates", CG_Argv(2))) {
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						clientInfoString = CG_ConfigString(CS_PLAYERS + i);
						//FIXME option for this
						if (!clientInfoString[0]) {
							continue;
						}
						t = atoi(Info_ValueForKey(clientInfoString, "t"));
						if (CG_IsTeamGame(cgs.gametype)  &&  t == cg.snap->ps.persistant[PERS_TEAM]) {
							CG_ClientOverrideClearClient(i);
						}
					}
				} else if (!Q_stricmp("us", CG_Argv(2))) {
					CG_ClientOverrideClearClient(cg.snap->ps.clientNum);
				} else if (!Q_stricmp("all", CG_Argv(2))) {
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						CG_ClientOverrideClearClient(i);
					}
				} else {  // client number
					int cn;

					cn = atoi(CG_Argv(2));
					CG_ClientOverrideClearClient(cn);
				}
			}

			return;
		}
	}

	if (CG_Argc() < 4) {
		Com_Printf("\nusage:  clientoverride <client number or 'red', 'blue', 'enemy', 'mates', 'us', 'all', 'clear'> <key name> <key value> ... with additional key and value pairs as needed\n");
		Com_Printf("\nexample:  clientoverride 3 model ranger hmodel bones\n\n");
		return;
	}

	useTeam = qfalse;
	if (!Q_stricmp("red", CG_Argv(1))) {
		useTeam = qtrue;
		team = TEAM_RED;
	} else if (!Q_stricmp("blue", CG_Argv(1))) {
		useTeam = qtrue;
		team = TEAM_BLUE;
	} else if (!Q_stricmp("enemy", CG_Argv(1))) {
		useTeam = qtrue;
		team = T_ENEMY;
	} else if (!Q_stricmp("mates", CG_Argv(1))) {
		useTeam = qtrue;
		team = T_FRIENDLY;
	} else if (!Q_stricmp("all", CG_Argv(1))) {
		useTeam = qtrue;
		team = T_ALL;
	} else if (!Q_stricmp("us", CG_Argv(1))) {
		useTeam = qfalse;
		clientNum = cg.snap->ps.clientNum;
		if (cgs.clientinfo[clientNum].team == TEAM_SPECTATOR) {
			Com_Printf("not playing or spectating someone playing\n");
			return;
		}
	} else {
		useTeam = qfalse;
		clientNum = atoi(CG_Argv(1));
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			Com_Printf("invalid client number\n");
			return;
		}
	}

	for (i = 2;  i < CG_Argc();  i += 2) {
		Q_strncpyz(key, CG_Argv(i), sizeof(key));
		Q_strncpyz(value, CG_Argv(i + 1), sizeof(value));

		//Com_Printf("key: %s  value: %s\n", key, value);

		if (useTeam) {
			start = 0;
			end = MAX_CLIENTS;
		} else {
			start = clientNum;
			end = start + 1;
		}

		for (j = start;  j < end;  j++) {
			int t;

			clientInfoString = CG_ConfigString(CS_PLAYERS + j);
			//FIXME option for this
			if (!clientInfoString[0]) {
				continue;
			}
			if (useTeam) {
				t = atoi(Info_ValueForKey(clientInfoString, "t"));
				switch (team) {
				case TEAM_RED:
					if (t != TEAM_RED) {
						continue;
					}
					break;
				case TEAM_BLUE:
					if (t != TEAM_BLUE) {
						continue;
					}
					break;
				case T_ENEMY:
					if (j == cg.snap->ps.clientNum) {
						continue;
					} else if (CG_IsTeamGame(cgs.gametype)  &&  t == cg.snap->ps.persistant[PERS_TEAM]) {
						continue;
					}
					break;
				case T_FRIENDLY:
					if (j == cg.snap->ps.clientNum) {
						continue;
					} else if (CG_IsTeamGame(cgs.gametype)  &&  t != cg.snap->ps.persistant[PERS_TEAM]) {
						continue;
					}
					break;
				case T_ALL:
					break;
				default:
					continue;
				}
			}
			ci = &cgs.clientinfo[j];
			if (clientInfoString[0] != '\\') {
				buffer[0] = '\\';
				Q_strncpyz(buffer + 1, clientInfoString, sizeof(buffer) - 1);
			} else {
				Q_strncpyz(buffer, clientInfoString, sizeof(buffer));
			}
			//Info_SetValueForKey(buffer, key, value);
			if (!Q_stricmpn(key, "headcolor", strlen("headcolor"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					cgs.clientinfo[j].hasHeadColor = qfalse;
				} else {
					color = Com_HexStrToInt(value);
					SC_ByteVecColorFromInt(cgs.clientinfo[j].headColor, color);
					cgs.clientinfo[j].headColor[3] = 255;
					cgs.clientinfo[j].hasHeadColor = qtrue;
				}
			} else if (!Q_stricmpn(key, "torsocolor", strlen("torsocolor"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					cgs.clientinfo[j].hasTorsoColor = qfalse;
				} else {
					color = Com_HexStrToInt(value);
					SC_ByteVecColorFromInt(cgs.clientinfo[j].torsoColor, color);
					cgs.clientinfo[j].torsoColor[3] = 255;
					cgs.clientinfo[j].hasTorsoColor = qtrue;
				}
			} else if (!Q_stricmpn(key, "legscolor", strlen("legscolor"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					cgs.clientinfo[j].hasLegsColor = qfalse;
				} else {
					color = Com_HexStrToInt(value);
					SC_ByteVecColorFromInt(cgs.clientinfo[j].legsColor, color);
					cgs.clientinfo[j].legsColor[3] = 255;
					cgs.clientinfo[j].hasLegsColor = qtrue;
				}
			} else if (!Q_stricmpn(key, "headskin", strlen("headskin"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasHeadSkin = qfalse;
				} else {
					ci->hasHeadSkin = qtrue;
					Q_strncpyz(ci->headSkinName, value, sizeof(ci->headSkinName));
				}
			} else if (!Q_stricmpn(key, "torsoskin", strlen("torsoskin"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasTorsoSkin = qfalse;
				} else {
					ci->hasTorsoSkin = qtrue;
					Q_strncpyz(ci->torsoSkinName, value, sizeof(ci->torsoSkinName));
				}
			} else if (!Q_stricmpn(key, "legsskin", strlen("legsskin"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasLegsSkin = qfalse;
				} else {
					ci->hasLegsSkin = qtrue;
					Q_strncpyz(ci->legsSkinName, value, sizeof(ci->legsSkinName));
				}
			} else if (!Q_stricmpn(key, "modelscale", strlen("modelscale"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasModelScale = qfalse;
				} else {
					ci->hasModelScale = qtrue;
					ci->modelScale = atof(value);
				}
			} else if (!Q_stricmpn(key, "legsmodelscale", strlen("legsmodelscale"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasLegsModelScale = qfalse;
				} else {
					ci->hasLegsModelScale = qtrue;
					ci->legsModelScale = atof(value);
				}
			} else if (!Q_stricmpn(key, "torsomodelscale", strlen("torsomodelscale"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasTorsoModelScale = qfalse;
				} else {
					ci->hasTorsoModelScale = qtrue;
					ci->torsoModelScale = atof(value);
				}
			} else if (!Q_stricmpn(key, "headmodelscale", strlen("headmodelscale"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasHeadModelScale = qfalse;
				} else {
					ci->hasHeadModelScale = qtrue;
					ci->headModelScale = atof(value);
				}
			} else if (!Q_stricmpn(key, "headoffset", strlen("headoffset"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasHeadOffset = qfalse;
				} else {
					ci->hasHeadOffset = qtrue;
					ci->oheadOffset = atof(value);
				}
			} else if (!Q_stricmpn(key, "modelautoscale", strlen("modelautoscale"))) {
				if (!Q_stricmpn(value, "clear", strlen("clear"))) {
					ci->hasModelAutoScale = qfalse;
				} else {
					ci->hasModelAutoScale = qtrue;
					ci->modelAutoScale = atof(value);
				}
			} else {
				//Com_Printf("change config string: '%s' '%s' '%s'\n", key, value, buffer);
				Info_SetValueForKey(buffer, key, value);
				if (cgs.protocol == PROTOCOL_Q3) {
					CG_ChangeConfigString(buffer, CSQ3_PLAYERS + j);
				} else {
					CG_ChangeConfigString(buffer, CS_PLAYERS + j);
				}
			}

			if (i + 2 >= CG_Argc()) {  // final arg checked for this client

				//FIMXE oops
				memcpy(&tmpCi, ci, sizeof(tmpCi));

				if (cgs.clientinfo[j].hasHeadColor) {
					changeHeadColor = qtrue;
					memcpy(headColor, cgs.clientinfo[j].headColor, sizeof(headColor));
				} else {
					changeHeadColor = qfalse;
				}
				if (cgs.clientinfo[j].hasTorsoColor) {
					changeTorsoColor = qtrue;
					memcpy(torsoColor, cgs.clientinfo[j].torsoColor, sizeof(torsoColor));
				} else {
					changeTorsoColor = qfalse;
				}
				if (cgs.clientinfo[j].hasLegsColor) {
					changeLegsColor = qtrue;
					memcpy(legsColor, cgs.clientinfo[j].legsColor, sizeof(legsColor));
				} else {
					changeLegsColor = qfalse;
				}
				if (ci->hasHeadSkin) {
					changeHeadSkin = qtrue;
					Q_strncpyz(HeadSkinName, ci->headSkinName, sizeof(HeadSkinName));
				} else {
					changeHeadSkin = qfalse;
				}
				if (ci->hasTorsoSkin) {
					changeTorsoSkin = qtrue;
					Q_strncpyz(TorsoSkinName, ci->torsoSkinName, sizeof(TorsoSkinName));
				} else {
					changeTorsoSkin = qfalse;
				}
				if (ci->hasLegsSkin) {
					changeLegsSkin = qtrue;
					Q_strncpyz(LegsSkinName, ci->legsSkinName, sizeof(LegsSkinName));
				} else {
					changeLegsSkin = qfalse;
				}

				cgs.clientinfo[j].override = qfalse;
				CG_NewClientInfo(j);
				cgs.clientinfo[j].override = qtrue;

				if (changeHeadColor) {
					memcpy(cgs.clientinfo[j].headColor, headColor, sizeof(headColor));
					cgs.clientinfo[j].headColor[3] = 255;
					cgs.clientinfo[j].hasHeadColor = qtrue;
				}
				if (changeTorsoColor) {
					memcpy(cgs.clientinfo[j].torsoColor, torsoColor, sizeof(torsoColor));
					cgs.clientinfo[j].torsoColor[3] = 255;
					cgs.clientinfo[j].hasTorsoColor = qtrue;
				}
				if (changeLegsColor) {
					memcpy(cgs.clientinfo[j].legsColor, legsColor, sizeof(legsColor));
					cgs.clientinfo[j].legsColor[3] = 255;
					cgs.clientinfo[j].hasLegsColor = qtrue;
				}

				CG_LoadClientInfo(&cgs.clientinfo[j], j, qtrue);

				if (changeHeadSkin) {
					CG_GetModelAndSkinName(HeadSkinName, modelStr, skinStr);
					if (!*modelStr) {
						Q_strncpyz(modelStr, ci->headModelName, sizeof(modelStr));
					}
					ci->headSkin = CG_RegisterSkinVertexLight(va("models/players/%s/head_%s.skin", modelStr, skinStr));

				}
				if (changeTorsoSkin) {
					CG_GetModelAndSkinName(TorsoSkinName, modelStr, skinStr);
					if (!*modelStr) {
						Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
					}
					ci->torsoSkin = CG_RegisterSkinVertexLight(va("models/players/%s/upper_%s.skin", modelStr, skinStr));

				}
				if (changeLegsSkin) {
					CG_GetModelAndSkinName(LegsSkinName, modelStr, skinStr);
					if (!*modelStr) {
						Q_strncpyz(modelStr, ci->modelName, sizeof(modelStr));
					}
					ci->legsSkin = CG_RegisterSkinVertexLight(va("models/players/%s/lower_%s.skin", modelStr, skinStr));

				}

				// bleh
				ci->hasModelScale = tmpCi.hasModelScale;
				ci->hasLegsModelScale = tmpCi.hasLegsModelScale;
				ci->hasTorsoModelScale = tmpCi.hasTorsoModelScale;
				ci->hasHeadModelScale = tmpCi.hasHeadModelScale;
				ci->hasHeadOffset = tmpCi.hasHeadOffset;
				ci->hasModelAutoScale = tmpCi.hasModelAutoScale;

				ci->modelScale = tmpCi.modelScale;
				ci->legsModelScale = tmpCi.legsModelScale;
				ci->torsoModelScale = tmpCi.torsoModelScale;
				ci->headModelScale = tmpCi.headModelScale;
				ci->oheadOffset = tmpCi.oheadOffset;
				ci->modelAutoScale = tmpCi.modelAutoScale;
			}
		}
	}

#undef T_FRIENDLY
#undef T_ENEMY
#undef T_ALL
}

static void CG_TestMenu_f (void)
{
	if (CG_Argc() == 1) {
		if (cg.testMenu) {
			cg.testMenu = NULL;
		}
		return;
	}
	//CG_ParseMenu(CG_Argv(1));
	cg.testMenu = Menus_FindByName(CG_Argv(1));
}

static void CG_PrintPlayerState_f (void)
{
	CG_PrintPlayerState();
}

static void CG_ClearScene_f (void)
{
	CG_InitLocalEntities();
	CG_ClearParticles();
	CG_InitMarkPolys();
}

static void CG_InfoDown_f (void)
{
	cg.drawInfo = qtrue;
}

static void CG_InfoUp_f (void)
{
	cg.drawInfo = qfalse;
}

static void CG_AddAtFtimeCommand (double ftime)
{
	int i;
	int j;
	atCommand_t *at;
	qboolean foundSlot = qfalse;


	for (i = 0;  i < MAX_AT_COMMANDS;  i++) {
		at = &cg.atCommands[i];
		if (!at->valid) {
			foundSlot = qtrue;
			break;
		}
		if (at->ftime > ftime) {
			foundSlot = qtrue;
			break;
		}
	}

	if (!foundSlot) {
		Com_Printf("too many at commands\n");
		return;
	}

	for (j = MAX_AT_COMMANDS - 2;  j >= i;  j--) {
		cg.atCommands[j + 1] = cg.atCommands[j];
	}

	at->valid = qtrue;
	at->ftime = ftime;
	at->lastCheckedTime = 0;
	Q_strncpyz(at->command, CG_ArgsFrom(2), sizeof(at->command));
}

static void CG_AddAtCommand_f (void)
{
	char timeString[MAX_STRING_CHARS];
	//int serverTime;
	//qboolean wantWarmup;
	double ftime;

	if (CG_Argc() < 3) {
		Com_Printf("usage:  at <'now' |  server time  |  clock time> <command>\n");
		Com_Printf("   ex:  at now timescale 0.5\n");
		Com_Printf("        at 4546629.50 stopvideo    // server time\n");
		Com_Printf("        at 8:52.33 cg_fov 90       // clock time\n");
		Com_Printf("        at w2:05 r_gamma 1.4    // warmup 2:05\n");
		return;
	}

	Q_strncpyz(timeString, CG_Argv(1), sizeof(timeString));

	//wantWarmup = qfalse;
	if (!Q_stricmp(timeString, "now")) {
		if (!cg.snap) {
			Com_Printf("CG_AtAddCommand_f()  demo/play hasn't started yet\n");
			return;
		}
		//trap_AddAt(cg.snap->serverTime, CG_GetLevelTimerString(), CG_ArgsFrom(2));
		ftime = cg.ftime;
	} else {
		if (cg_levelTimerDirection.integer != 0  &&  cg_levelTimerDirection.integer != 2) {
			Com_Printf("^3warning: level timer counts down and clock values will be incorrect\n");
		}
#if 1
		ftime = CG_GetServerTimeFromClockString(timeString);
		if (ftime < 0) {
			Com_Printf("CG_AddAtCommand_f() invalid server time\n");
			return;
		}
#endif
		//Com_Printf("FIXME clock\n");
		//return;
	}

	CG_AddAtFtimeCommand(ftime);
}

static void CG_ListAtCommands_f (void)
{
	int i;
	const atCommand_t *at;

#if 0
	if (cg_levelTimerDirection.integer != 0  &&  cg_levelTimerDirection.integer != 2) {
		Com_Printf("^3warning: level timer counts down and clock values will be incorrect\n");
	}
#endif

	for (i = 0;  i < MAX_AT_COMMANDS;  i++) {
		at = &cg.atCommands[i];
		if (!at->valid) {
			break;
		}
		Com_Printf("^3%3d %f ^7%s \"%s\"\n", i + 1, at->ftime, "FIXME-clock-string", at->command);
	}
}

static void CG_ClearAtCommands_f (void)
{
	int i;

	for (i = 0;  i < MAX_AT_COMMANDS;  i++) {
		cg.atCommands[i].valid = qfalse;
	}
}

static void CG_RemoveAtCommand_f (void)
{
	int i;
	int num;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  removeat <at number>\n");
		return;
	}

	num = atoi(CG_Argv(1));
	num--;
	if (num < 0  ||  num >= MAX_AT_COMMANDS) {
		Com_Printf("invalid at command number %d\n", num + 1);
		return;
	}

	if (!cg.atCommands[num].valid) {
		Com_Printf("no such command number %d\n", num + 1);
		return;
	}

	for (i = num;  i < (MAX_AT_COMMANDS - 1);  i++) {
		cg.atCommands[i] = cg.atCommands[i + 1];
	}
}

static void CG_SaveAtCommands_f (void)
{
	qhandle_t f;
	int i;
	const char *s;
	const atCommand_t *at;

	if (!cg.atCommands[0].valid) {
		return;
	}

	if (CG_Argc() < 2) {
		trap_FS_FOpenFile("atcommands.cfg", &f, FS_WRITE);
	} else {
		trap_FS_FOpenFile(CG_Argv(1), &f, FS_WRITE);
	}

	if (!f) {
		Com_Printf("^1couldn't open file\n");
		return;
	}

	for (i = 0;  i < MAX_AT_COMMANDS;  i++) {
		at = &cg.atCommands[i];
		if (!at->valid) {
			break;
		}
		s = va("at %f \"%s\"\n", at->ftime, at->command);
		trap_FS_Write(s, strlen(s), f);
	}

	trap_FS_FCloseFile(f);
}

static void CG_ExecAtTime_f (void)
{
	int serverTime;
	char buffer[MAX_STRING_CHARS];

	if (CG_Argc() < 3) {
		Com_Printf("usage:  exec_at_time <servertime> <config file to exec>\n");
		return;
	}

	serverTime = atoi(CG_Argv(1));

	Com_sprintf(buffer, sizeof(buffer), "at %d \"exec %s\"\n", serverTime, CG_Argv(2));
	//Com_Printf("%s\n", buffer);
	trap_SendConsoleCommand(buffer);
}

static void CG_EntityFilter_f (void)
{
	const char *arg;
	int i;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  entityfilter <'clear', 'all', TYPE, entity number>\n");
		return;
	}

	arg = CG_Argv(1);

	if (!Q_stricmpn(arg, "clear", strlen("clear"))) {
		memset(&cg.filterEntity, 0, sizeof(cg.filterEntity));
		memset(&cg.filter, 0, sizeof(cg.filter));
		return;
	} else if (!Q_stricmpn(arg, "all", strlen("all"))) {
		for (i = 0;  i < MAX_GENTITIES;  i++) {
			cg.filterEntity[i] = qtrue;
		}
	} else if (Q_isAnInteger(arg)) {
		i = atoi(arg);
		if (i < 0  ||  i >= MAX_GENTITIES) {
			Com_Printf("invalid entity number\n");
			return;
		}
		cg.filterEntity[i] = !cg.filterEntity[i];
	} else if (!Q_stricmpn(arg, "general", strlen("general"))) {
		cg.filter.general = !cg.filter.general;
	} else if (!Q_stricmpn(arg, "player", strlen("player"))) {
		cg.filter.player = !cg.filter.player;
	} else if (!Q_stricmpn(arg, "item", strlen("item"))) {
		cg.filter.item = !cg.filter.item;
	} else if (!Q_stricmpn(arg, "missile", strlen("missile"))) {
		cg.filter.missile = !cg.filter.missile;
	} else if (!Q_stricmpn(arg, "mover", strlen("mover"))) {
		cg.filter.mover = !cg.filter.mover;
	} else if (!Q_stricmpn(arg, "beam", strlen("beam"))) {
		cg.filter.beam = !cg.filter.beam;
	} else if (!Q_stricmpn(arg, "portal", strlen("portal"))) {
		cg.filter.portal = !cg.filter.portal;
	} else if (!Q_stricmpn(arg, "speaker", strlen("speaker"))) {
		cg.filter.speaker = !cg.filter.speaker;
	} else if (!Q_stricmpn(arg, "pushtrigger", strlen("pushtrigger"))) {
		cg.filter.pushTrigger = !cg.filter.pushTrigger;
	} else if (!Q_stricmpn(arg, "teleporttrigger", strlen("teleporttrigger"))) {
		cg.filter.teleportTrigger = !cg.filter.teleportTrigger;
	} else if (!Q_stricmpn(arg, "invisible", strlen("invisible"))) {
		cg.filter.invisible = !cg.filter.invisible;
	} else if (!Q_stricmpn(arg, "grapple", strlen("grapple"))) {
		cg.filter.grapple = !cg.filter.grapple;
	} else if (!Q_stricmpn(arg, "team", strlen("team"))) {
		cg.filter.team = !cg.filter.team;
	} else if (!Q_stricmpn(arg, "events", strlen("events"))) {
		cg.filter.events = !cg.filter.events;
	}
}

static void CG_ListEntityFilter_f (void)
{
	int i;
	int count;

	count = 0;
	for (i = 0;  i < MAX_GENTITIES;  i++) {
		if (cg.filterEntity[i]) {
			Com_Printf("%d ", i);
			count++;
		}
		if (count >= 9) {
			Com_Printf("\n");
			count = 0;
		}
	}

	Com_Printf("\nfilter types: ");
	if (cg.filter.general) {
		Com_Printf("general ");
	}
	if (cg.filter.player) {
		Com_Printf("player ");
	}
	if (cg.filter.item) {
		Com_Printf("item ");
	}
	if (cg.filter.missile) {
		Com_Printf("missile ");
	}
	if (cg.filter.mover) {
		Com_Printf("mover ");
	}
	if (cg.filter.beam) {
		Com_Printf("beam ");
	}
	if (cg.filter.portal) {
		Com_Printf("portal ");
	}
	if (cg.filter.speaker) {
		Com_Printf("speaker ");
	}
	if (cg.filter.pushTrigger) {
		Com_Printf("pushTrigger ");
	}
	if (cg.filter.teleportTrigger) {
		Com_Printf("teleportTrigger ");
	}
	if (cg.filter.invisible) {
		Com_Printf("invisible ");
	}
	if (cg.filter.grapple) {
		Com_Printf("grapple ");
	}
	if (cg.filter.team) {
		Com_Printf("team ");
	}
	if (cg.filter.events) {
		Com_Printf("events ");
	}
	Com_Printf("\n");
}

static void CG_EntityFreeze_f (void)
{
	const char *arg;
	int i;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  entityfreeze <'clear', entity number>\n");
		return;
	}

	if (!cg.snap) {
		return;
	}

	arg = CG_Argv(1);

	if (!Q_stricmpn(arg, "clear", strlen("clear"))) {
		memset(&cg.freezeEntity, 0, sizeof(cg.freezeEntity));
		return;
#if 0
	} else if (!Q_stricmpn(arg, "all", strlen("all"))) {
		for (i = 0;  i < MAX_GENTITIES;  i++) {
			cg.freezeEntity[i] = qtrue;
		}
#endif
	} else if (Q_isAnInteger(arg)) {
		i = atoi(arg);
		if (i < 0  ||  i >= MAX_GENTITIES) {
			Com_Printf("invalid entity number\n");
			return;
		}
		cg.freezeEntity[i] = !cg.freezeEntity[i];
		if (cg.freezeEntity[i]) {
			const centity_t *cent;

			if (i == cg.snap->ps.clientNum) {
				cent = &cg.predictedPlayerEntity;
				cg.freezePs = cg.predictedPlayerState;
				cg.freezeCgtime = cg.time;
				cg.freezeXyspeed = cg.xyspeed;
				cg.freezeLandChange = cg.landChange;
				cg.freezeLandTime = cg.landTime;
			} else {
				cent = &cg_entities[i];
			}

			wclients[i].freezeCgtime = cg.time;
			wclients[i].freezeXyspeed = wclients[i].xyspeed;
			wclients[i].freezeLandChange = wclients[i].landChange;
			wclients[i].freezeLandTime = wclients[i].landTime;

			memcpy(&cg.freezeCent[i], cent, sizeof(cg.freezeCent[i]));
		}
	}
}

static void CG_ListEntityFreeze_f (void)
{
	int i;
	int count;

	count = 0;
	for (i = 0;  i < MAX_GENTITIES;  i++) {
		if (cg.filterEntity[i]) {
			Com_Printf("%d ", i);
			count++;
		}
		if (count >= 9) {
			Com_Printf("\n");
			count = 0;
		}
	}

	Com_Printf("\n");
}

static void CG_PrintJumps_f (void)
{
	int i;
	int wordCount;
	int num;

	if (cg.numJumps == 0) {
		return;
	}

	num = cg.numJumps;
	if (num >= MAX_JUMPS_INFO) {
		num = MAX_JUMPS_INFO - 1;
	}
	wordCount = 0;
	for (i = 0;  i < num;  i++) {
		Com_Printf("%d ", cg.jumps[(cg.numJumps - num + i) % MAX_JUMPS_INFO].speed);
		wordCount++;
		if (wordCount >= 8) {
			Com_Printf("\n");
			wordCount = 0;
		}
	}
	Com_Printf("\n\ntotal time: %f seconds\n", (float)(cg.jumps[cg.numJumps - 1].time - cg.jumps[0].time) / 1000.0);
}

static void CG_ClearJumps_f (void)
{
	cg.numJumps = 0;
}

static void CG_MouseSeekDown_f (void)
{
	if (!cg.snap) {
		return;
	}

	cg.mouseSeeking = qtrue;
	cg.mousex = 0;
	cg.mousey = 0;
}

static void CG_MouseSeekUp_f (void)
{
	cg.mouseSeeking = qfalse;
	cg.lastMouseSeekTime = 0;
}

static void CG_PrintLegsInfo (int entNumber)
{
	const lerpFrame_t *lf;

	if (!cg.snap) {
		return;
	}

	if (entNumber < 0  ||  entNumber >= MAX_GENTITIES) {
		return;
	}

	lf = &cg_entities[entNumber].pe.legs;

	Com_Printf("cg.time %d\n", cg.time);
	Com_Printf("oldFrame %d\n", lf->oldFrame);
	Com_Printf("oldFrameTime %d\n", lf->oldFrameTime);
	Com_Printf("frame %d\n", lf->frame);
	Com_Printf("frameTime %d\n", lf->frameTime);
	Com_Printf("backlerp %f\n", lf->backlerp);
	Com_Printf("yawAngle %f\n", lf->yawAngle);
	Com_Printf("yawing %d\n", lf->yawing);
	Com_Printf("pitchAngle %f\n", lf->pitchAngle);
	Com_Printf("pitching %d\n", lf->pitching);
	Com_Printf("animationNumber %d\n", lf->animationNumber);
	Com_Printf("animation <%p>\n", lf->animation);
	Com_Printf("animationTime %d\n", lf->animationTime);
	if (lf->animation) {
		Com_Printf("animation frameLerp %d\n", lf->animation->frameLerp);
	}
}


static void CG_PrintLegsInfo_f (void)
{
	int entNumber;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  printlegsinfo <entity number>\n");
		return;
	}

	entNumber = atoi(CG_Argv(1));
	if (entNumber < 0  ||  entNumber >= MAX_GENTITIES) {
		Com_Printf("^1invalid number\n");
		return;
	}

	CG_PrintLegsInfo(entNumber);
}

static void CG_ChatDown_f (void)
{
	cg.forceDrawChat = qtrue;
}

static void CG_ChatUp_f (void)
{
	cg.forceDrawChat = qfalse;
}

static void CG_AddChatLine_f (void)
{
	CG_AddChatLine(CG_ArgsFrom(1));
}

static void CG_CvarInterp_f (void)
{
	int i;
	qboolean foundSlot = qfalse;
	cvarInterp_t *c;
	double tm;

	if (CG_Argc() < 5) {
		Com_Printf("usage:  cvarinterp <cvar name> <starting value> <ending value> <time length in seconds> ['real' or 'game', default: 'game']\n");
		Com_Printf("  ex:  /cvarinterp s_volume 0 0.7 2.0\n");
		Com_Printf("  ex:  /cvarinterp timescale 0.0001 1.0  6.0 real\n");
		return;
	}

	for (i = 0;  i < MAX_CVAR_INTERP;  i++) {
		c = &cg.cvarInterp[i];
		if (!c->valid) {
			foundSlot = qtrue;
			break;
		}
	}

	if (!foundSlot) {
		Com_Printf("too many cvars being changed, can't continue\n");
		return;
	}

	c->valid = qtrue;

	if (CG_Argc() > 5) {
		c->realTime = !Q_stricmp("real", CG_Argv(5));
	}

	if (!c->realTime) {
		c->startTime = cg.ftime;
	} else {
		c->startTime = trap_Milliseconds();
	}

	Q_strncpyz(c->cvar, CG_Argv(1), sizeof(c->cvar));
	c->startValue = atof(CG_Argv(2));
	c->endValue = atof(CG_Argv(3));
	tm = atof(CG_Argv(4));
	tm *= 1000.0;
	c->endTime = c->startTime + tm;
}

static void CG_ClearCvarInterp_f (void)
{
	int i;

	for (i = 0;  i < MAX_CVAR_INTERP;  i++) {
		cg.cvarInterp[i].valid = qfalse;
	}
}

static void CG_ChangeConfigString_f (void)
{
	const char *s;
	qboolean force = qfalse;
	int arg;
	int num;
	int i;

	//Com_Printf("^1change config string\n");

	if (CG_Argc() < 2) {
		Com_Printf("usage: changeconfigstring [force | clear | list ] <string number> <new string>\n");
		Com_Printf("  ex:  changeconfigstring 3 \"Furious Heights\"\n");
		//FIXME 659 changes
		Com_Printf("  ex:  changeconfigstring force 659 ^6noone\n");
		Com_Printf("  ex:  changeconfigstring clear 1\n");
		Com_Printf("  ex:  changeconfigstring clear all\n");
		Com_Printf("  ex:  changeconfigstring list\n");
		Com_Printf("   3 is map name, 659 is player in first place\n");
		return;
	}

	arg = 1;
	s = CG_Argv(arg);
	if (!Q_stricmp(s, "force")) {
		force = qtrue;
		arg++;
	} else if (!Q_stricmp(s, "clear")) {
		arg++;
		s = CG_Argv(arg);
		if (!Q_stricmp(s, "all")) {
			for (i = 0;  i < MAX_CONFIGSTRINGS;  i++) {
				cg.configStringOverride[i] = qfalse;
			}
			return;
		} else {
			if (*s) {
				cg.configStringOverride[atoi(s)] = qfalse;
				return;
			} else {
				Com_Printf("^1ERROR: need config string number\n");
				return;
			}
		}
	} else if (!Q_stricmp(s, "list")) {
		for (i = 0;  i < MAX_CONFIGSTRINGS;  i++) {
			if (cg.configStringOverride[i]) {
				Com_Printf("%d: %s\n", i, cg.configStringOurs[i]);
			}
		}
		return;
	}

	num = atoi(CG_Argv(arg));
	if (num < 0  ||  num > MAX_CONFIGSTRINGS) {
		Com_Printf("^1ERROR: invalid config string number: %d\n", num);
		return;
	}

	s = CG_ArgsFrom(arg + 1);
	if (force) {
		cg.configStringOverride[num] = qtrue;
		Q_strncpyz(cg.configStringOurs[num], s, sizeof(cg.configStringOurs[num]));
	}

	CG_ChangeConfigString(s, num);
}

static void CG_ConfigStrings_f (void)
{
	int i;
	const char *s;

	//Com_Printf("yes\n");

	if (CG_Argc() > 1) {
		i = atoi(CG_Argv(1));
		Com_Printf("%d: %s\n", i, CG_ConfigString(i));
		return;
	}

	for (i = 0;  i < MAX_CONFIGSTRINGS;  i++) {
		s = CG_ConfigString(i);
		if (!s[0]) {
			continue;
		}
		Com_Printf("%d: %s\n", i, s);
	}
}

// hack to support etqw camtrace 3d
static void CG_Gfov_f (void)
{
	if (CG_Argc() < 2) {
		int fov;

		if (cgs.realProtocol >= 91  &&  cg_useDemoFov.integer == 1) {
			fov = cg.demoFov;
		} else {
			fov = cg_fov.integer;
		}
		Com_Printf("\"g_fov\" is:\"%d\" default:\"%d\"\n", fov, DEFAULT_FOV);
		return;
	}

	trap_Cvar_Set("cg_fov", CG_Argv(1));
}

// hack to support etqw camtrace 3d
static void CG_DemoScale_f (void)
{
	//FIXME CG_Argc() < 2
	if (CG_Argc() < 2) {
		//FIXME -- print out?
		return;
	}

	trap_Cvar_Set("timescale", CG_Argv(1));
}

// hack to support etqw camtrace 3d
static void CG_FreecamLookAtPlayer_f (void)
{
	int clientNum;

	if (CG_Argc() < 2) {
		cg.useViewPointMark = qfalse;
		return;
	}

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		Com_Printf("invalid client number\n");
		return;
	}

	trap_SendConsoleCommand(va("view %d\n", clientNum));
}

static void CG_RemapShader_f (void)
{
	int i;
	char args[4][MAX_QPATH];
	qboolean keepLightmap;

	if (CG_Argc() < 3) {
		Com_Printf("usage: remapshader <original shader> <new shader> [time offset: usually 0] [keep original shader lightmap -- 0: no, 1: yes (default)]\n");
		return;
	}

	for (i = 1;  i < 3;  i++) {
		Q_strncpyz(args[i - 1], CG_Argv(i), sizeof(args[i - 1]));
	}

	if (CG_Argc() > 3) {
		Q_strncpyz(args[2], CG_Argv(3), sizeof(args[2]));
	}

	if (CG_Argc() > 4) {
		keepLightmap = atoi(CG_Argv(4));
	} else {
		keepLightmap = qtrue;
	}

	trap_R_RemapShader(args[0], args[1], args[2], keepLightmap, qtrue);
}

static void CG_ClearRemappedShader_f (void)
{
	char arg[MAX_QPATH];

	if (CG_Argc() < 2) {
		Com_Printf("usage: clearremappedshader <shader name>\n");
		return;
	}

	Q_strncpyz(arg, CG_Argv(1), sizeof(arg));
	trap_R_ClearRemappedShader(arg);
}

static void CG_Move_f (void)
{
	vec3_t forward, right, up;
	float f, r, u;

	if (CG_Argc() < 2) {
		Com_Printf("usage: move [forward distance] [right distance] [up distance] [pitch change] [yaw change] [roll change]\n");
		return;
	}

	f = atof(CG_Argv(1));
	r = atof(CG_Argv(2));
	u = atof(CG_Argv(3));

	AngleVectors(cg.fang, forward, right, up);

	VectorMA(cg.fpos, f, forward, cg.fpos);
	VectorMA(cg.fpos, r, right, cg.fpos);
	VectorMA(cg.fpos, u, up, cg.fpos);

	cg.fang[0] += atof(CG_Argv(4));
	cg.fang[1] += atof(CG_Argv(5));
	cg.fang[2] += atof(CG_Argv(6));

	cg.fMoveTime = 0;
}

static void CG_MoveOffset_f (void)
{
	if (CG_Argc() < 2) {
		Com_Printf("usage: moveoffset [x offset] [y offset] [z offset] [pitch change] [yaw change] [roll change]\n");
		return;
	}

	cg.fpos[0] = atof(CG_Argv(1));
	cg.fpos[1] = atof(CG_Argv(2));
	cg.fpos[2] = atof(CG_Argv(3));

	cg.fang[0] += atof(CG_Argv(4));
	cg.fang[1] += atof(CG_Argv(5));
	cg.fang[2] += atof(CG_Argv(6));

	cg.fMoveTime = 0;
}

static void CG_AddMirrorSurface_f (void)
{
	vec3_t origin;

	if (CG_Argc() < 4) {
		Com_Printf("usage: addmirrorsurface <x> <y> <z>\n");
		return;
	}

	if (cg.numMirrorSurfaces >= MAX_MIRROR_SURFACES) {
		Com_Printf("can't add:  too many mirror surfaces (%d)\n", MAX_MIRROR_SURFACES);
		return;
	}

	origin[0] = atof(CG_Argv(1));
	origin[1] = atof(CG_Argv(2));
	origin[2] = atof(CG_Argv(3));

	VectorCopy(origin, cg.mirrorSurfaces[cg.numMirrorSurfaces]);
	cg.numMirrorSurfaces++;
}

static void CG_DumpEnts_f (void)
{
	const char *fname;
	int i;
	qboolean useDefaultFolder = qtrue;
	const char *arg;
	int n;
	int entNum;

	if (cg.dumpEntities) {
		trap_FS_FCloseFile(cg.dumpFile);
		cg.dumpFile = 0;
		cg.dumpLastServerTime = 0;
	}
	cg.dumpEntities = qfalse;

	if (CG_Argc() > 1) {
		fname = CG_Argv(1);
		for (i = 0;  i < strlen(fname);  i++) {
			if (fname[i] == '/') {
				useDefaultFolder = qfalse;
				break;
			}
		}
	} else {
		fname = "dump";
	}

	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("dump/%s.txt", fname), &cg.dumpFile, FS_WRITE);
	} else {
		trap_FS_FOpenFile(va("%s.txt", fname), &cg.dumpFile, FS_WRITE);
	}

	if (!cg.dumpFile) {
		Com_Printf("^1couldn't open %s\n", fname);
		return;
	}

	cg.dumpEntities = qtrue;
	cg.dumpFreecam = qfalse;
	memset(cg.dumpValid, 0, sizeof(cg.dumpValid));

	if (CG_Argc() > 2) {
		n = 2;
		while (n < CG_Argc()) {
			arg = CG_Argv(n);

			if (!Q_stricmp(arg, "freecam")) {
				cg.dumpFreecam = qtrue;
			} else if (!Q_stricmp(arg, "all")) {
				cg.dumpFreecam = qtrue;
				for (i = 0;  i < MAX_GENTITIES;  i++) {
					cg.dumpValid[i] = qtrue;
				}
			} else {
				entNum = atoi(arg);
				if (entNum >= 0  &&  entNum < MAX_GENTITIES) {
					cg.dumpValid[entNum] = qtrue;
				}
			}
			n++;
		}
	} else {
		cg.dumpFreecam = qtrue;
		for (i = 0;  i < MAX_GENTITIES;  i++) {
			cg.dumpValid[i] = qtrue;
		}
	}

	Com_Printf("dumping entities\n");
}

static void CG_StopDumpEnts_f (void)
{
	if (!cg.dumpEntities) {
		return;
	}

	trap_FS_FCloseFile(cg.dumpFile);
	cg.dumpFile = 0;
	cg.dumpEntities = qfalse;
	cg.dumpFreecam = qfalse;
	cg.dumpLastServerTime = 0;

	Com_Printf("stopped dumping entities\n");
}

static void CG_RunFx (const char *name, const vec3_t origin, const vec3_t dir, const vec3_t velocity)
{
	int i;

	CG_ResetScriptVars();

	VectorCopy(velocity, ScriptVars.velocity);
	VectorCopy(dir, ScriptVars.dir);
	VectorNormalize(ScriptVars.dir);

	VectorCopy(origin, ScriptVars.origin);
	VectorCopy(origin, ScriptVars.lastDistancePosition);
	VectorCopy(origin, ScriptVars.lastIntervalPosition);

	ScriptVars.lastDistanceTime = cg.ftime;
	ScriptVars.lastIntervalTime = cg.ftime;

	Vector4Set(ScriptVars.color, 1, 1, 1, 1);

	for (i = 0;  i < EffectScripts.numEffects;  i++) {
		if (!Q_stricmp(name, EffectScripts.names[i])) {
			CG_RunQ3mmeScript(EffectScripts.ptr[i], NULL);
			return;
		}
	}

	Com_Printf("couldn't find fx '%s'\n", name);
}

static void CG_RunFx_f (void)
{
	const centity_t *cent;
	vec3_t origin;
	vec3_t velocity;
	vec3_t dir;
	const char *arg;
	int argc;

	argc = CG_Argc();

	if (argc < 2) {
		Com_Printf("usage:  runfx <fx name> [origin0] [origin1] [origin2] [dir0] [dir1] [dir2] [velocity0] [velocity1] [velocity2]\n");
		Com_Printf("   if origin0 --> dir2 not given will use current (freecam or 1st person view) settings\n");
		return;
	}

	if (cg.freecam) {
		VectorCopy(cg.freecamPlayerState.origin, origin);
		origin[2] += DEFAULT_VIEWHEIGHT;
		VectorCopy(cg.freecamPlayerState.velocity, velocity);
		AngleVectors(cg.freecamPlayerState.viewangles, dir, NULL, NULL);
		VectorNormalize(dir);
	} else {
		if (wolfcam_following) {
			cent = &cg_entities[wcg.clientNum];
		} else {
			cent = &cg.predictedPlayerEntity;
		}
		VectorCopy(cent->lerpOrigin, origin);
		origin[2] += DEFAULT_VIEWHEIGHT;
		VectorCopy(cent->currentState.pos.trDelta, velocity);
		AngleVectors(cent->lerpAngles, dir, NULL, NULL);
		VectorNormalize(dir);
	}

	if (argc > 2) {
		origin[0] = atof(CG_Argv(2));
	}
	if (argc > 3) {
		origin[1] = atof(CG_Argv(3));
	}
	if (argc > 4) {
		origin[2] = atof(CG_Argv(4));
	}
	if (argc > 5) {
		dir[0] = atof(CG_Argv(5));
	}
	if (argc > 6) {
		dir[1] = atof(CG_Argv(6));
	}
	if (argc > 7) {
		dir[2] = atof(CG_Argv(7));
	}
	if (argc > 8) {
		velocity[0] = atof(CG_Argv(8));
	}
	if (argc > 9) {
		velocity[1] = atof(CG_Argv(9));
	}
	if (argc > 10) {
		velocity[2] = atof(CG_Argv(10));
	}

	//Com_Printf("%f %f %f\n", origin[0], origin[1], origin[2]);

	arg = CG_Argv(1);

	CG_RunFx(arg, origin, dir, velocity);
}

static void CG_RunFxAll_f (void)
{
	char name[MAX_STRING_CHARS];

	if (CG_Argc() < 2) {
		Com_Printf("usage:  runfxall <fx name>\n");
		return;
	}
	Q_strncpyz(name, CG_Argv(1), sizeof(name));

	CG_RunFxAll(name);
}

static void CG_RunFxAt_f (void)
{
	const centity_t *cent;
	vec3_t origin;
	vec3_t velocity;
	vec3_t dir;
	char timeString[MAX_STRING_CHARS];
	char script[MAX_QPATH];
	int argc;

	argc = CG_Argc();

	if (argc < 3) {
		Com_Printf("usage:  runfxat <'now' | server time | clock time> <fx name> [origin0] [origin1] [origin2] [dir0] [dir1] [dir2] [velocity0] [velocity1] [velocity2]\n");
		Com_Printf("   if origin0 --> dir2 not given will use current (freecam or 1st person view) settings\n");
		return;
	}

	if (cg.freecam) {
		VectorCopy(cg.freecamPlayerState.origin, origin);
		origin[2] += DEFAULT_VIEWHEIGHT;
		VectorCopy(cg.freecamPlayerState.velocity, velocity);
		AngleVectors(cg.freecamPlayerState.viewangles, dir, NULL, NULL);
		VectorNormalize(dir);
	} else {
		if (wolfcam_following) {
			cent = &cg_entities[wcg.clientNum];
		} else {
			cent = &cg.predictedPlayerEntity;
		}
		VectorCopy(cent->lerpOrigin, origin);
		origin[2] += DEFAULT_VIEWHEIGHT;
		VectorCopy(cent->currentState.pos.trDelta, velocity);
		AngleVectors(cent->lerpAngles, dir, NULL, NULL);
		VectorNormalize(dir);
	}

	Q_strncpyz(timeString, CG_Argv(1), sizeof(timeString));
	Q_strncpyz(script, CG_Argv(2), sizeof(script));

	if (argc > 3) {
		origin[0] = atof(CG_Argv(3));
	}
	if (argc > 4) {
		origin[1] = atof(CG_Argv(4));
	}
	if (argc > 5) {
		origin[2] = atof(CG_Argv(5));
	}
	if (argc > 6) {
		dir[0] = atof(CG_Argv(6));
	}
	if (argc > 7) {
		dir[1] = atof(CG_Argv(7));
	}
	if (argc > 8) {
		dir[2] = atof(CG_Argv(8));
	}
	if (argc > 9) {
		velocity[0] = atof(CG_Argv(9));
	}
	if (argc > 10) {
		velocity[1] = atof(CG_Argv(10));
	}
	if (argc > 11) {
		velocity[2] = atof(CG_Argv(11));
	}

	//Com_Printf("%f %f %f\n", origin[0], origin[1], origin[2]);

	trap_SendConsoleCommand(va("at %s runfx %s %f %f %f %f %f %f %f %f %f\n", timeString, script, origin[0], origin[1], origin[2], dir[0], dir[1], dir[2], velocity[0], velocity[1], velocity[2]));
}

static void CG_ListFxScripts_f (void)
{
	int i;

	for (i = 0;  i < EffectScripts.numEffects;  i++) {
		Com_Printf("%3d: %s\n", i, EffectScripts.names[i]);
	}
}

static void CG_PrintDirVector_f (void)
{
	vec3_t dir;
	const centity_t *cent;

	if (cg.freecam) {
		AngleVectors(cg.freecamPlayerState.viewangles, dir, NULL, NULL);
		VectorNormalize(dir);
	} else {
		if (wolfcam_following) {
			cent = &cg_entities[wcg.clientNum];
		} else {
			cent = &cg.predictedPlayerEntity;
		}
		AngleVectors(cent->lerpAngles, dir, NULL, NULL);
		VectorNormalize(dir);
	}

	Com_Printf("%f %f %f\n", dir[0], dir[1], dir[2]);
}

static void CG_EventFilter_f (void)
{
	int id;
	int i;
	int j;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  eventfilter <'clear', event id>\n");
		Com_Printf("   toggles on and off\n");
		Com_Printf("   'clear' deletes all filters\n");
		return;
	}

	if (!Q_stricmp(CG_Argv(1), "clear")) {
		cg.numEventFilter = 0;
		return;
	}

	id = atoi(CG_Argv(1));

	// see if deleting first
	for (i = 0;  i < cg.numEventFilter;  i++) {
		if (id != cg.eventFilter[i]) {
			continue;
		}

		for (j = i + 1;  j < cg.numEventFilter;  j++) {
			cg.eventFilter[j - 1] = cg.eventFilter[j];
		}
		cg.numEventFilter--;
		return;
	}

	if (cg.numEventFilter >= MAX_EVENT_FILTER) {
		return;
	}

	cg.eventFilter[cg.numEventFilter] = id;
	cg.numEventFilter++;
}

static void CG_ListEventFilter_f (void)
{
	int i;

	for (i = 0;  i < cg.numEventFilter;  i++) {
		Com_Printf("%d ", cg.eventFilter[i]);
		if (i % 5) {
			Com_Printf("\n");
		}
	}

	Com_Printf("\n");
}

static void CG_AddDecal_f (void)
{
	int i;
	char shaderName[MAX_QPATH];
	char cvarName[256];  //FIXME size
	char cvarValue[MAX_CVAR_VALUE_STRING];
	vec3_t ourOrigin;
	const char *st;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  adddecal <shader>\n");
		Com_Printf("ex:  /adddecal wc/poster");
		return;
	}

	i = 1;
	while (1) {
		st = SC_Cvar_Get_String(va("cg_decal%d", i));
		if (!*st) {
			break;
		}
		i++;
	}

	if (wolfcam_following) {
		VectorCopy(cg_entities[wcg.clientNum].lerpOrigin, ourOrigin);
	} else if (cg.freecam) {
		VectorCopy(cg.fpos, ourOrigin);
	} else {
		VectorCopy(cg.refdef.vieworg, ourOrigin);
	}

	Q_strncpyz(shaderName, CG_Argv(1), sizeof(shaderName));
	Com_sprintf(cvarName, sizeof(cvarName), "cg_decal%d", i);
	Com_sprintf(cvarValue, sizeof(cvarValue), "%f %f %f %f %f %f 0.0 1.0 1.0 1.0 1.0 30.0 %s", cg.lastImpactOrigin[0], cg.lastImpactOrigin[1], cg.lastImpactOrigin[2], ourOrigin[0], ourOrigin[1], ourOrigin[2], shaderName);

	trap_Cvar_Set(cvarName, cvarValue);

	//sscanf(st, "%f %f %f %f %f %f %f %f %f %f %f %f %63s", &origin[0], &origin[1], &origin[2], &frontPoint[0], &frontPoint[1], &frontPoint[2], &orientation, &r, &g, &b, &a, &radius, shaderName);  // FIXME 63 MAX_QPATH
}

static void CG_PrintTime_f (void)
{
	int serverTime;

	if (cg.snap) {
		serverTime = cg.snap->serverTime;
	} else {
		serverTime = -1;
	}

	Com_Printf("cgame: %d %f   server: %d\n", cg.time, cg.ftime, serverTime);
}

static void CG_KillCountReset_f (void)
{
	int i;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclients[i].killCount = 0;
	}
}

static void CG_PrintEntityDistance_f (void)
{
	vec3_t org1, org2;
	int num;

	if (CG_Argc() < 2) {
		Com_Printf("usage:  adddecal <shader>\n");
		Com_Printf("ex:  /adddecal wc/poster");

		Com_Printf("usage:  printentitydistance [entity 1] [entity 2]\n");
		Com_Printf(" or     printentitydistance [entity]\n");
		Com_Printf("\n");
		Com_Printf("The second one prints the distance from the current view origin\n");
		return;
	} else if (CG_Argc() < 3) {
		VectorCopy(cg.refdef.vieworg, org1);
	} else {  // 2 entes given
		num = atoi(CG_Argv(2));
		if (num < 0  ||  num >= MAX_GENTITIES) {
			Com_Printf("^3invalid entity number\n");
			return;
		}
		VectorCopy(cg_entities[num].lerpOrigin, org1);
	}

	num = atoi(CG_Argv(1));
	if (num < 0  ||  num >= MAX_GENTITIES) {
		Com_Printf("^3invalid entity number\n");
		return;
	}

	VectorCopy(cg_entities[num].lerpOrigin, org2);

	Com_Printf("distance:  %f\n", Distance(org1, org2));
	Com_Printf("  org1:  %f %f %f\n", org1[0], org1[1], org1[2]);
	Com_Printf("  org2:  %f %f %f\n", org2[0], org2[1], org2[2]);
}


static void CG_PlayQ3mmeCamera_f (void)
{
	double extraTime;

	if (cg.playPath) {
		Com_Printf("can't play camera, path is playing\n");
		return;
	}

	if (!demo.camera.points  ||  !(demo.camera.points->next)) {
		Com_Printf("can't play q3mme camera, need at least 2 camera points\n");
		return;
	}

	//FIXME cameraque ?
	if (1) {  //(cg_cameraQue.integer) {
		extraTime = 1000.0 * cg_cameraRewindTime.value;
		if (extraTime < 0) {
			extraTime = 0;
		}
		trap_SendConsoleCommandNow(va("seekservertime %f\n", demo.camera.points->time - extraTime));
	}

	cg.cameraQ3mmePlaying = qtrue;
	cg.playQ3mmeCameraCommandIssued = qtrue;
	//cg.cameraPlaying = qtrue;
	///cg.cameraPlayedLastFrame = qfalse;

	cg.currentCameraPoint = 0;
	//cg.cameraWaitToSync = qfalse;  //FIXME stupid
	cg.playCameraCommandIssued = qtrue;

}

static void CG_StopQ3mmeCamera_f (void)
{
	cg.cameraQ3mmePlaying = qfalse;
	cg.playQ3mmeCameraCommandIssued = qfalse;
	Com_Printf("stopping q3mme camera\n");
}

static void CG_SaveQ3mmeCamera_f (void)
{
	fileHandle_t f;
	int i;
	const char *fname;
	qboolean useDefaultFolder = qtrue;

	if (CG_Argc() < 2) {
		Com_Printf("usage: saveq3mmecamera <filename>\n");
		return;
	}

	if (!demo.camera.points) {
		Com_Printf("need at least one camera point\n");
		return;
	}

	fname = CG_Argv(1);
	for (i = 0;  i < strlen(fname);  i++) {
		if (fname[i] == '/') {
			useDefaultFolder = qfalse;
			break;
		}
	}

	if (useDefaultFolder) {
		trap_FS_FOpenFile(va("cameras/%s.q3mmeCam", CG_Argv(1)), &f, FS_WRITE);
	} else {
		trap_FS_FOpenFile(va("%s.q3mmeCam", fname), &f, FS_WRITE);
	}

	if (!f) {
		Com_Printf("^1couldn't create %s.q3mmeCam\n", CG_Argv(1));
		return;
	}

	CG_Q3mmeCameraSave(f);
	trap_FS_FCloseFile(f);
}

static void CG_LoadQ3mmeCamera_f (void)
{
	qboolean useDefaultFolder = qtrue;
	const char *fname;
	int i;
	BG_XMLParse_t xmlParse;
	char filename[MAX_OSPATH];
	BG_XMLParseBlock_t loadBlock[] = {
		{ "camera", CG_Q3mmeCameraParse, 0 },
		{ 0, 0, 0 },
	};

	if (CG_Argc() < 2) {
		Com_Printf("usage: loadq3mmecamera <camera name>\n");
		return;
	}

	fname = CG_Argv(1);
	for (i = 0;  i < strlen(fname);  i++) {
		if (fname[i] == '/') {
			useDefaultFolder = qfalse;
			break;
		}
	}

	if (useDefaultFolder) {
		//trap_FS_FOpenFile(va("cameras/%s.q3mmeCam", CG_Argv(1)), &f, FS_READ);
		//ret = BG_XMLOpen(&xmlParse, va("cameras/%s.q3mmeCam", CG_Argv(1)));
		Com_sprintf(filename, sizeof(filename), "cameras/%s.q3mmeCam", CG_Argv(1));
	} else {
		//trap_FS_FOpenFile(va("%s.q3mmeCam", fname), &f, FS_READ);
		Com_sprintf(filename, sizeof(filename), "%s.q3mmeCam", CG_Argv(1));
	}

	if (!BG_XMLOpen(&xmlParse, filename)) {
		Com_Printf("^1couldn't open %s\n", CG_Argv(1));
		return;
	}

	if (!BG_XMLParse(&xmlParse, 0, loadBlock, 0)) {
		Com_Printf("^1Errors while loading q3mme camera\n");
		return;
	}
}

// testing
#if 0
static void CG_BackDown_f (void)
{
	Com_Printf("+back !!!!!!!!!!!!!!!!!!!!\n");
}
#endif

static void CG_SeekNextRound_f (void)
{
	int i;
	float offset;
	int roundStartIndex;

	offset = -3000.0;

	if (CG_Argc() > 1) {
		offset = atof(CG_Argv(1));
		//Com_Printf("^3offset %f\n", offset);
	}

	roundStartIndex = -1;
	for (i = 0;  i < cg.numRoundStarts;  i++) {
		if (cg.roundStarts[i] > cg.time) {
			//trap_SendConsoleCommand(va("seekservertime %f\n", (double)((double)cg.roundStarts[i] + (double)offset)));
			roundStartIndex = i;
			break;
		}
	}


	if (roundStartIndex > -1) {
		int roundTime;

		if (cgs.protocol == PROTOCOL_QL) {
			roundTime = atoi(CG_ConfigString(CS_ROUND_TIME));

			// ql specific
			//if (!cgs.roundStarted  &&  cgs.roundBeginTime > 0) {
			if (roundTime <= 0) {
				// skip current warmup
				roundStartIndex++;
			}
		} else if (cgs.cpma) {
			//FIXME ctfs
			if (cgs.gametype == GT_CA) {
				if (CG_CpmaIsRoundWarmup()) {
					roundStartIndex++;
				}
			}
		}

		if (roundStartIndex < cg.numRoundStarts) {
			trap_SendConsoleCommand(va("seekservertime %f\n", (double)((double)cg.roundStarts[roundStartIndex] + (double)offset)));
		}
	}
}

static void CG_SeekPrevRound_f (void)
{
	int i;
	double offset;

	offset = -3000.0;

	if (CG_Argc() > 1) {
		offset = atof(CG_Argv(1));
		//Com_Printf("^3offset %f\n", offset);
	}

	for (i = cg.numRoundStarts - 1;  i >= 0;  i--) {
		if (cg.roundStarts[i] < cg.time) {
			trap_SendConsoleCommand(va("seekservertime %f\n", (double)((double)cg.roundStarts[i] + (double)offset)));
			break;
		}
	}
}


typedef struct {
	const char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+acc", CG_AccStatsDown_f },
	{ "-acc", CG_AccStatsUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
 #ifdef MISSIONPACK
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
#endif
	{ "loadhud", CG_LoadHud_f },
#ifdef MISSIONPACK
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
#endif
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
	{ "startOrbit", CG_StartOrbit_f },
	{ "idcamera", CG_idCamera_f },
	{ "stopidcamera", CG_idCameraStop_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
    { "players", Wolfcam_Players_f },
    { "playersw", Wolfcam_Playersw_f },
    { "follow", Wolfcam_Follow_f },
    { "wcstats", Wolfcam_Weapon_Stats_f },
    { "wcstatsall", Wolfcam_Weapon_Statsall_f },
	{ "wcstatsresetall", Wolfcam_Reset_Weapon_Stats_f },
	{ "listplayermodels", Wolfcam_List_Player_Models_f },
	{ "listtimeditems", CG_ListTimedItems_f },
	{ "loadmenu", CG_LoadMenu_f },
	{ "freecam", CG_FreeCam_f },
	{ "setviewpos", CG_SetViewPos_f },
	{ "freecamsetpos", CG_SetViewPos_f },  // alias for camtrace 3d
	{ "setviewangles", CG_SetViewAngles_f },
	{ "seekclock", CG_SeekClock_f },
	{ "echopopup", CG_EchoPopup_f },
	{ "echopopupclear", CG_EchoPopupClear_f },
	{ "echopopupcvar", CG_EchoPopupCvar_f },
	{ "listentities", CG_ListEntities_f },
	{ "printentitystate", CG_PrintEntityState_f },
	{ "printnextentitystate", CG_PrintNextEntityState_f },
	{ "clearfragmessage", CG_ClearFragMessage_f },
	{ "view", CG_ViewEnt_f },
	{ "viewunlockyaw", CG_ViewUnlockYaw_f },
	{ "viewunlockpitch", CG_ViewUnlockPitch_f },
	{ "gotoview", CG_GotoView_f },
	{ "testreplace", CG_TestReplaceShaderImage_f },
	{ "stopmovement", CG_StopMovement_f },
	{ "servertime", CG_ServerTime_f },
	//{ "printscores", CG_PrintScores_f },
	{ "dumpstats", CG_DumpStats_f },
	{ "setloopstart", CG_SetLoopStart_f },
	{ "setloopend", CG_SetLoopEnd_f },
	{ "loop", CG_Loop_f },
	{ "fragforward", CG_FragForward_f },
	//{ "loadmodels", CG_LoadModels_f },
	{ "recordpath", CG_RecordPath_f },
	{ "playpath", CG_PlayPath_f },
	{ "stopplaypath", CG_StopPlayPath_f },
	{ "centerroll", CG_CenterRoll_f },
	{ "drawrawpath", CG_DrawRawPath_f },
	{ "chase", CG_Chase_f },
	{ "addcamerapoint", CG_AddCameraPoint_f },
	{ "clearcamerapoints", CG_ClearCameraPoints_f },
	{ "playcamera", CG_PlayCamera_f },
	{ "stopcamera", CG_StopCamera_f },
	{ "savecamera", CG_SaveCamera_f },
	{ "camtracesave", CG_CamtraceSave_f },
	{ "loadcamera", CG_LoadCamera_f },
	{ "selectcamerapoint", CG_SelectCameraPoint_f },
	{ "deletecamerapoint", CG_DeleteCameraPoint_f },
	//{ "drawcamera", CG_DrawCamera_f },
	{ "editcamerapoint", CG_EditCameraPoint_f },
	//{ "changecamerapoint", CG_ChangeThisCameraPoint_f },  //FIXME change name
	{ "ecam", CG_ChangeSelectedCameraPoints_f },
	{ "setviewpointmark", CG_SetViewPointMark_f },
	//{ "deleteviewpointmark", CG_DeleteViewPointMark_f },
	{ "gotoviewpointmark", CG_GotoViewPointMark_f },
	//{ "setcamviewpointmarkhere", CG_SetCamViewPointMarkHere_f },
	//{ "setnextcamviewpointhere", CG_SetNextCamViewPointHere_f },
	//{ "gotocamviewpoint", CG_GotoCamViewPoint_f },
	//{ "gotonextcamviewpoint", CG_GotoNextCamViewPoint_f },
	//{ "ucam", CG_UpdateCameraInfo },  // for testing
	{ "nextfield", CG_SelectNextField_f },
	{ "prevfield", CG_SelectPrevField_f },
	{ "changefield", CG_ChangeSelectedField_f },
	{ "fxmath", CG_FXMath_f },
	{ "fxload", CG_FXLoad_f },
	{ "vibrate", CG_Vibrate_f }, // testing
	{ "localents", CG_LocalEnts_f },
	{ "clearfx", CG_ClearFX_f },
	{ "gotoad", CG_GotoAdvertisement_f },
	{ "clientoverride", CG_ClientOverride_f },
	{ "testmenu", CG_TestMenu_f },
	{ "printplayerstate", CG_PrintPlayerState_f },
	{ "clearscene", CG_ClearScene_f },
	{ "+info", CG_InfoDown_f },
	{ "-info", CG_InfoUp_f },
	{ "at", CG_AddAtCommand_f },
	{ "listat", CG_ListAtCommands_f },
	{ "clearat", CG_ClearAtCommands_f },
	{ "removeat", CG_RemoveAtCommand_f },
	{ "saveat", CG_SaveAtCommands_f },
	{ "exec_at_time", CG_ExecAtTime_f },
	{ "entityfilter", CG_EntityFilter_f },
	{ "listentityfilter", CG_ListEntityFilter_f },
	{ "entityfreeze", CG_EntityFreeze_f },
	{ "listentityfreeze", CG_ListEntityFreeze_f },
	{ "printjumps", CG_PrintJumps_f },
	{ "clearjumps", CG_ClearJumps_f },
	{ "+mouseseek", CG_MouseSeekDown_f },
	{ "-mouseseek", CG_MouseSeekUp_f },
	{ "printlegsinfo", CG_PrintLegsInfo_f },
	{ "+chat", CG_ChatDown_f },
	{ "-chat", CG_ChatUp_f },
	{ "addchatline", CG_AddChatLine_f },
	{ "cvarinterp", CG_CvarInterp_f },
	{ "clearcvarinterp", CG_ClearCvarInterp_f },
	{ "changeconfigstring", CG_ChangeConfigString_f },
	{ "cconfigstrings", CG_ConfigStrings_f },
	{ "g_fov", CG_Gfov_f },  // camtrace3d etqw hack
	{ "demo_scale", CG_DemoScale_f },  // camtrace3d etqw hack
	{ "freecamlookatplayer", CG_FreecamLookAtPlayer_f },  // camtrace3d etqw hack
	{ "remapshader", CG_RemapShader_f },
	{ "clearremappedshader", CG_ClearRemappedShader_f },
	{ "move", CG_Move_f },
	{ "moveoffset", CG_MoveOffset_f },
	{ "addmirrorsurface", CG_AddMirrorSurface_f },
	{ "dumpents", CG_DumpEnts_f },
	{ "stopdumpents", CG_StopDumpEnts_f },
	{ "runfx", CG_RunFx_f },
	{ "runfxat", CG_RunFxAt_f },
	{ "runfxall", CG_RunFxAll_f },
	{ "listfxscripts", CG_ListFxScripts_f },
	{ "printdirvector", CG_PrintDirVector_f },
	//{ "importq3mmecamera", CG_ImportQ3mmeCamera_f },
	{ "eventfilter", CG_EventFilter_f },
	{ "listeventfilter", CG_ListEventFilter_f },
	{ "adddecal", CG_AddDecal_f },
	{ "printtime", CG_PrintTime_f },
	{ "killcountreset", CG_KillCountReset_f },
	{ "printentitydistance", CG_PrintEntityDistance_f },
	{ "q3mmecamera", CG_Q3mmeDemoCameraCommand_f },
	//{ "addq3mmecamerapoint", CG_AddQ3mmeCameraPoint_f },
	{ "playq3mmecamera", CG_PlayQ3mmeCamera_f },
	{ "stopq3mmecamera", CG_StopQ3mmeCamera_f },
	{ "saveq3mmecamera", CG_SaveQ3mmeCamera_f },
	{ "loadq3mmecamera", CG_LoadQ3mmeCamera_f },
	//{ "+back", CG_BackDown_f },
	{ "seeknextround", CG_SeekNextRound_f },
	{ "seekprevround", CG_SeekPrevRound_f },

};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < ARRAY_LEN(commands) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < ARRAY_LEN(commands) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
#ifdef MISSIONPACK
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
#endif
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("where");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("follownext");
	trap_AddCommand ("followprev");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
}

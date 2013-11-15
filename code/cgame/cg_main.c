// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "cg_consolecmds.h"
#include "cg_draw.h"
#include "cg_drawtools.h"
#include "cg_info.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#include "cg_newdraw.h"
#include "cg_players.h"
#include "cg_q3mme_scripts.h"
#include "cg_servercmds.h"
#include "cg_snapshot.h"
#include "cg_spawn.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"

#include "../ui/ui_shared.h"
#include "wolfcam_local.h"

// display context for new ui stuff
displayContextDef_t cgDC;


static int forceModelModificationCount = -1;

const char *gametypeConfigs[] = {
	"ffa.cfg",
	"duel.cfg",
	"singleplayer.cfg",
	"tdm.cfg",
	"ca.cfg",
	"ctf.cfg",
	"1fctf.cfg",
	"obelisk.cfg",
	"harvester.cfg",
	"freezetag.cfg",
	"domination.cfg",
	"ad.cfg",
	"rr.cfg",
	"ntf.cfg",
	"2v2.cfg",
	"hm.cfg",
	"race.cfg",
};


static void CG_Init (int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback);
static void CG_Shutdown( void );
static void CG_TimeChange (int serverTime, int ioverf);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#ifdef CGAME_HARD_LINKED
int CgvmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
#else
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
#endif
{

	switch ( command ) {
	case CG_INIT:
		CG_Init(arg0, arg1, arg2, arg3);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		//Com_Printf("^5active start\n");
		CG_DrawActiveFrame(arg0, arg1, arg2, arg3, arg4, arg5);
		//Com_Printf("^5active end\n");
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
        //CG_Printf ("CG_KEY_EVENT: %d %d\n", arg0, arg1);
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
#if 1  //def MPACK
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
#endif
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	case CG_TIME_CHANGE:
		//Com_Printf("cgame seeking\n");
		CG_TimeChange(arg0, arg1);
		return 0;
	case CG_COLOR_TABLE_CHANGE:
		Q_SetColorTable(arg0, (float)(arg1) / 255.0, (float)(arg2) / 255.0, (float)(arg3) / 255.0, (float)(arg4) / 255.0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES + 1];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

clientInfo_t		EM_ModelInfo;
int					EM_Loaded = 0;
byte				EC_Colors[3][4];
int					EC_Loaded = 0;

vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_railQL;
vmCvar_t	cg_railQLRailRingWhiteValue;
vmCvar_t	cg_railNudge;
vmCvar_t	cg_railRings;
vmCvar_t	cg_railRadius;
vmCvar_t	cg_railRotation;
vmCvar_t	cg_railSpacing;
vmCvar_t cg_railItemColor;
vmCvar_t cg_railUseOwnColors;
vmCvar_t cg_railFromMuzzle;
//vmCvar_t	cg_centertime;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t cg_gibColor;
vmCvar_t	cg_gibJump;
vmCvar_t	cg_gibVelocity;
vmCvar_t cg_gibVelocityRandomness;
vmCvar_t cg_gibDirScale;
vmCvar_t cg_gibOriginOffset;
vmCvar_t cg_gibOriginOffsetZ;
vmCvar_t cg_gibRandomness;
vmCvar_t cg_gibRandomnessZ;
vmCvar_t	cg_gibTime;
vmCvar_t	cg_gibStepTime;
vmCvar_t cg_gibBounceFactor;
vmCvar_t cg_gibGravity;
vmCvar_t	cg_gibSparksSize;
vmCvar_t cg_gibSparksColor;
vmCvar_t cg_gibSparksHighlight;
vmCvar_t cg_gibFloatingVelocity;
vmCvar_t cg_gibFloatingRandomness;
vmCvar_t cg_gibFloatingOriginOffset;
vmCvar_t cg_gibFloatingOriginOffsetZ;
vmCvar_t	cg_impactSparks;
vmCvar_t	cg_impactSparksLifetime;
vmCvar_t	cg_impactSparksSize;
vmCvar_t	cg_impactSparksVelocity;
vmCvar_t cg_impactSparksColor;
vmCvar_t cg_impactSparksHighlight;
vmCvar_t	cg_shotgunImpactSparks;
vmCvar_t	cg_shotgunMarks;
vmCvar_t cg_shotgunStyle;
vmCvar_t cg_shotgunRandomness;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawClientItemTimer;
vmCvar_t	cg_drawClientItemTimerX;
vmCvar_t	cg_drawClientItemTimerY;
vmCvar_t	cg_drawClientItemTimerScale;
vmCvar_t cg_drawClientItemTimerTextColor;
vmCvar_t	cg_drawClientItemTimerFont;
vmCvar_t	cg_drawClientItemTimerPointSize;
vmCvar_t	cg_drawClientItemTimerAlpha;
vmCvar_t	cg_drawClientItemTimerStyle;
vmCvar_t	cg_drawClientItemTimerAlign;
vmCvar_t	cg_drawClientItemTimerSpacing;
vmCvar_t cg_drawClientItemTimerIcon;
vmCvar_t cg_drawClientItemTimerIconSize;
vmCvar_t cg_drawClientItemTimerIconXoffset;
vmCvar_t cg_drawClientItemTimerIconYoffset;

vmCvar_t cg_itemSpawnPrint;

vmCvar_t	cg_drawFPS;
vmCvar_t cg_drawFPSNoText;
vmCvar_t	cg_drawFPSX;
vmCvar_t	cg_drawFPSY;
vmCvar_t cg_drawFPSAlign;
vmCvar_t cg_drawFPSStyle;
vmCvar_t cg_drawFPSFont;
vmCvar_t cg_drawFPSPointSize;
vmCvar_t cg_drawFPSScale;
//vmCvar_t cg_drawFPSTime;
vmCvar_t cg_drawFPSColor;
vmCvar_t cg_drawFPSAlpha;
//vmCvar_t cg_drawFPSFade;
//vmCvar_t cg_drawFPSFadeTime;

vmCvar_t	cg_drawSnapshot;
vmCvar_t    cg_drawSnapshotX;
vmCvar_t	cg_drawSnapshotY;
vmCvar_t cg_drawSnapshotAlign;
vmCvar_t cg_drawSnapshotStyle;
vmCvar_t cg_drawSnapshotFont;
vmCvar_t cg_drawSnapshotPointSize;
vmCvar_t cg_drawSnapshotScale;
vmCvar_t cg_drawSnapshotColor;
vmCvar_t cg_drawSnapshotAlpha;

vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;

vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawAmmoWarningX;
vmCvar_t	cg_drawAmmoWarningY;
vmCvar_t cg_drawAmmoWarningAlign;
vmCvar_t cg_drawAmmoWarningStyle;
vmCvar_t cg_drawAmmoWarningFont;
vmCvar_t cg_drawAmmoWarningPointSize;
vmCvar_t cg_drawAmmoWarningScale;
//vmCvar_t cg_drawAmmoWarningTime;
vmCvar_t cg_drawAmmoWarningColor;
vmCvar_t cg_drawAmmoWarningAlpha;
//vmCvar_t cg_drawAmmoWarningFade;
//vmCvar_t cg_drawAmmoWarningFadeTime;

vmCvar_t cg_lowAmmoWarningStyle;
vmCvar_t cg_lowAmmoWarningPercentile;
vmCvar_t cg_lowAmmoWarningSound;
vmCvar_t cg_lowAmmoWeaponBarWarning;
vmCvar_t cg_lowAmmoWarningGauntlet;
vmCvar_t cg_lowAmmoWarningMachineGun;
vmCvar_t cg_lowAmmoWarningShotgun;
vmCvar_t cg_lowAmmoWarningGrenadeLauncher;
vmCvar_t cg_lowAmmoWarningRocketLauncher;
vmCvar_t cg_lowAmmoWarningLightningGun;
vmCvar_t cg_lowAmmoWarningRailGun;
vmCvar_t cg_lowAmmoWarningPlasmaGun;
vmCvar_t cg_lowAmmoWarningBFG;
vmCvar_t cg_lowAmmoWarningGrapplingHook;
vmCvar_t cg_lowAmmoWarningNailGun;
vmCvar_t cg_lowAmmoWarningProximityLauncher;
vmCvar_t cg_lowAmmoWarningChainGun;


vmCvar_t	cg_drawCrosshair;

vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawCrosshairNamesX;
vmCvar_t cg_drawCrosshairNamesY;
vmCvar_t cg_drawCrosshairNamesAlign;
vmCvar_t cg_drawCrosshairNamesStyle;
vmCvar_t cg_drawCrosshairNamesFont;
vmCvar_t cg_drawCrosshairNamesPointSize;
vmCvar_t cg_drawCrosshairNamesScale;
vmCvar_t cg_drawCrosshairNamesTime;
vmCvar_t cg_drawCrosshairNamesColor;
vmCvar_t cg_drawCrosshairNamesAlpha;
vmCvar_t cg_drawCrosshairNamesFade;
vmCvar_t cg_drawCrosshairNamesFadeTime;

vmCvar_t cg_drawCrosshairTeammateHealth;
vmCvar_t cg_drawCrosshairTeammateHealthX;
vmCvar_t cg_drawCrosshairTeammateHealthY;
vmCvar_t cg_drawCrosshairTeammateHealthAlign;
vmCvar_t cg_drawCrosshairTeammateHealthStyle;
vmCvar_t cg_drawCrosshairTeammateHealthFont;
vmCvar_t cg_drawCrosshairTeammateHealthPointSize;
vmCvar_t cg_drawCrosshairTeammateHealthScale;
vmCvar_t cg_drawCrosshairTeammateHealthTime;
vmCvar_t cg_drawCrosshairTeammateHealthColor;
vmCvar_t cg_drawCrosshairTeammateHealthAlpha;
vmCvar_t cg_drawCrosshairTeammateHealthFade;
vmCvar_t cg_drawCrosshairTeammateHealthFadeTime;

vmCvar_t cg_drawRewards;
vmCvar_t cg_drawRewardsMax;
vmCvar_t cg_drawRewardsX;
vmCvar_t cg_drawRewardsY;
vmCvar_t cg_drawRewardsAlign;
vmCvar_t cg_drawRewardsStyle;
vmCvar_t cg_drawRewardsFont;
vmCvar_t cg_drawRewardsPointSize;
vmCvar_t cg_drawRewardsScale;
vmCvar_t cg_drawRewardsImageScale;
vmCvar_t cg_drawRewardsTime;
vmCvar_t cg_drawRewardsColor;
vmCvar_t cg_drawRewardsAlpha;
vmCvar_t cg_drawRewardsFade;
vmCvar_t cg_drawRewardsFadeTime;
vmCvar_t cg_rewardsStack;

vmCvar_t	cg_crosshairSize;
vmCvar_t cg_crosshairColor;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t cg_crosshairPulse;
vmCvar_t cg_crosshairHitStyle;
vmCvar_t cg_crosshairHitColor;
vmCvar_t cg_crosshairHitTime;
vmCvar_t cg_crosshairBrightness;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairAlphaAdjust;

vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_drawTeamBackground;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_marks;
vmCvar_t cg_markTime;
vmCvar_t cg_markFadeTime;
vmCvar_t cg_debugImpactOrigin;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t cg_gunSize;
vmCvar_t cg_gunSizeThirdPerson;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t cg_simpleItemsScale;
vmCvar_t cg_simpleItemsBob;
vmCvar_t cg_simpleItemsHeightOffset;
vmCvar_t cg_itemsWh;
vmCvar_t cg_itemFx;
vmCvar_t cg_itemSize;
vmCvar_t	cg_fov;
vmCvar_t cg_fovy;
vmCvar_t cg_fovStyle;
vmCvar_t cg_fovIntermission;
vmCvar_t	cg_zoomFov;
vmCvar_t cg_zoomTime;
vmCvar_t cg_zoomIgnoreTimescale;
vmCvar_t cg_zoomBroken;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_stereoSeparation;

vmCvar_t	cg_lagometer;
vmCvar_t	cg_lagometerX;
vmCvar_t	cg_lagometerY;
vmCvar_t	cg_lagometerFlash;
vmCvar_t	cg_lagometerFlashValue;
vmCvar_t cg_lagometerAlign;
vmCvar_t cg_lagometerFontAlign;
vmCvar_t cg_lagometerFontStyle;
vmCvar_t cg_lagometerFont;
vmCvar_t cg_lagometerFontPointSize;
vmCvar_t cg_lagometerFontScale;
vmCvar_t cg_lagometerScale;
vmCvar_t cg_lagometerFontColor;
vmCvar_t cg_lagometerAlpha;
vmCvar_t cg_lagometerFontAlpha;
vmCvar_t cg_lagometerAveragePing;
vmCvar_t cg_lagometerSnapshotPing;

vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_drawAttackerX;
vmCvar_t	cg_drawAttackerY;
vmCvar_t cg_drawAttackerAlign;
vmCvar_t cg_drawAttackerStyle;
vmCvar_t cg_drawAttackerFont;
vmCvar_t cg_drawAttackerPointSize;
vmCvar_t cg_drawAttackerScale;
vmCvar_t cg_drawAttackerImageScale;
vmCvar_t cg_drawAttackerTime;
vmCvar_t cg_drawAttackerColor;
vmCvar_t cg_drawAttackerAlpha;
vmCvar_t cg_drawAttackerFade;
vmCvar_t cg_drawAttackerFadeTime;

vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t cg_forcePovModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;

vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_drawTeamOverlayX;
vmCvar_t	cg_drawTeamOverlayY;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t cg_drawTeamOverlayFont;
vmCvar_t cg_drawTeamOverlayPointSize;
vmCvar_t cg_drawTeamOverlayAlign;
vmCvar_t cg_drawTeamOverlayScale;
//vmCvar_t cg_drawTeamOverlayAlpha;

vmCvar_t cg_drawJumpSpeeds;
vmCvar_t cg_drawJumpSpeedsNoText;
vmCvar_t cg_drawJumpSpeedsMax;
vmCvar_t cg_drawJumpSpeedsX;
vmCvar_t cg_drawJumpSpeedsY;
vmCvar_t cg_drawJumpSpeedsAlign;
vmCvar_t cg_drawJumpSpeedsStyle;
vmCvar_t cg_drawJumpSpeedsFont;
vmCvar_t cg_drawJumpSpeedsPointSize;
vmCvar_t cg_drawJumpSpeedsScale;
vmCvar_t cg_drawJumpSpeedsColor;
vmCvar_t cg_drawJumpSpeedsAlpha;

vmCvar_t cg_drawJumpSpeedsTime;
vmCvar_t cg_drawJumpSpeedsTimeNoText;
vmCvar_t cg_drawJumpSpeedsTimeX;
vmCvar_t cg_drawJumpSpeedsTimeY;
vmCvar_t cg_drawJumpSpeedsTimeAlign;
vmCvar_t cg_drawJumpSpeedsTimeStyle;
vmCvar_t cg_drawJumpSpeedsTimeFont;
vmCvar_t cg_drawJumpSpeedsTimePointSize;
vmCvar_t cg_drawJumpSpeedsTimeScale;
vmCvar_t cg_drawJumpSpeedsTimeColor;
vmCvar_t cg_drawJumpSpeedsTimeAlpha;

vmCvar_t	cg_drawFriend;
vmCvar_t cg_drawFriendMinWidth;
vmCvar_t cg_drawFriendMaxWidth;
vmCvar_t cg_drawFoe;
vmCvar_t cg_drawFoeMinWidth;
vmCvar_t cg_drawFoeMaxWidth;
vmCvar_t cg_drawSelf;
vmCvar_t cg_drawSelfMinWidth;
vmCvar_t cg_drawSelfMaxWidth;
vmCvar_t	cg_teamChatsOnly;
#ifdef MISSIONPACK
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
#endif
vmCvar_t	cg_hudFiles;
//vmCvar_t	wolfcam_hudFiles;
vmCvar_t 	cg_scorePlums;

vmCvar_t 	cg_smoothClients;
vmCvar_t	pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t	pmove_msec;
//vmCvar_t	cg_pmove_msec;
//vmCvar_t g_weapon_plasma_rate;

vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t cg_smokeRadius_SG;
vmCvar_t cg_smokeRadius_GL;
vmCvar_t cg_smokeRadius_NG;
vmCvar_t cg_smokeRadius_RL;
vmCvar_t cg_smokeRadius_PL;
vmCvar_t cg_smokeRadius_breath;
vmCvar_t cg_enableBreath;
vmCvar_t cg_smokeRadius_dust;
vmCvar_t cg_enableDust;
vmCvar_t cg_smokeRadius_flight;
vmCvar_t cg_smokeRadius_haste;
//vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
//vmCvar_t	cg_oldPlasma;
vmCvar_t cg_plasmaStyle;
vmCvar_t	cg_trueLightning;

#if 1  //def MPACK
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
vmCvar_t	cg_currentSelectedPlayer;
vmCvar_t	cg_currentSelectedPlayerName;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_serverEnableDust;
vmCvar_t	cg_serverEnableBreath;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
vmCvar_t	cg_obeliskRespawnDelay;
#endif

vmCvar_t cg_drawSpeed;
vmCvar_t cg_drawSpeedX;
vmCvar_t cg_drawSpeedY;
vmCvar_t cg_drawSpeedNoText;
vmCvar_t cg_drawSpeedAlign;
vmCvar_t cg_drawSpeedStyle;
vmCvar_t cg_drawSpeedFont;
vmCvar_t cg_drawSpeedPointSize;
vmCvar_t cg_drawSpeedScale;
vmCvar_t cg_drawSpeedColor;
vmCvar_t cg_drawSpeedAlpha;

vmCvar_t cg_drawOrigin;
vmCvar_t cg_drawOriginX;
vmCvar_t cg_drawOriginY;
vmCvar_t cg_drawOriginAlign;
vmCvar_t cg_drawOriginStyle;
vmCvar_t cg_drawOriginFont;
vmCvar_t cg_drawOriginPointSize;
vmCvar_t cg_drawOriginScale;
vmCvar_t cg_drawOriginColor;
vmCvar_t cg_drawOriginAlpha;

vmCvar_t cg_drawScores;
vmCvar_t cg_drawPlayersLeft;
vmCvar_t cg_drawPowerups;

vmCvar_t cg_drawItemPickups;
vmCvar_t cg_drawItemPickupsX;
vmCvar_t cg_drawItemPickupsY;
vmCvar_t cg_drawItemPickupsImageScale;
vmCvar_t cg_drawItemPickupsAlign;
vmCvar_t cg_drawItemPickupsStyle;
vmCvar_t cg_drawItemPickupsFont;
vmCvar_t cg_drawItemPickupsPointSize;
vmCvar_t cg_drawItemPickupsScale;
vmCvar_t cg_drawItemPickupsTime;
vmCvar_t cg_drawItemPickupsColor;
vmCvar_t cg_drawItemPickupsAlpha;
vmCvar_t cg_drawItemPickupsFade;
vmCvar_t cg_drawItemPickupsFadeTime;
vmCvar_t cg_drawItemPickupsCount;

vmCvar_t cg_drawFollowing;
vmCvar_t cg_drawFollowingX;
vmCvar_t cg_drawFollowingY;
vmCvar_t cg_drawFollowingAlign;
vmCvar_t cg_drawFollowingStyle;
vmCvar_t cg_drawFollowingFont;
vmCvar_t cg_drawFollowingPointSize;
vmCvar_t cg_drawFollowingScale;
vmCvar_t cg_drawFollowingColor;
vmCvar_t cg_drawFollowingAlpha;

vmCvar_t cg_testQlFont;
vmCvar_t cg_qlhud;
vmCvar_t cg_qlFontScaling;

vmCvar_t cg_weaponBar;
vmCvar_t cg_weaponBarX;
vmCvar_t cg_weaponBarY;
vmCvar_t cg_weaponBarFont;
vmCvar_t cg_weaponBarPointSize;
vmCvar_t cg_drawFullWeaponBar;
vmCvar_t cg_scoreBoardStyle;
vmCvar_t cg_scoreBoardSpectatorScroll;
vmCvar_t cg_scoreBoardWhenDead;
vmCvar_t cg_scoreBoardAtIntermission;
vmCvar_t cg_scoreBoardWarmup;
vmCvar_t cg_scoreBoardOld;

vmCvar_t cg_hitBeep;

vmCvar_t cg_drawSpawns;
vmCvar_t cg_drawSpawnsInitial;
vmCvar_t cg_drawSpawnsInitialZ;
vmCvar_t cg_drawSpawnsRespawns;
vmCvar_t cg_drawSpawnsRespawnsZ;
vmCvar_t cg_drawSpawnsShared;
vmCvar_t cg_drawSpawnsSharedZ;

vmCvar_t cg_freecam_noclip;
vmCvar_t cg_freecam_sensitivity;
vmCvar_t cg_freecam_yaw;
vmCvar_t cg_freecam_pitch;
vmCvar_t cg_freecam_speed;
vmCvar_t cg_freecam_crosshair;
vmCvar_t cg_freecam_useTeamSettings;
vmCvar_t cg_freecam_rollValue;
vmCvar_t cg_freecam_useServerView;
vmCvar_t cg_freecam_unlockPitch;

vmCvar_t cg_chatTime;
vmCvar_t cg_chatLines;
vmCvar_t cg_chatHistoryLength;
vmCvar_t cg_chatBeep;
vmCvar_t cg_chatBeepMaxTime;
vmCvar_t cg_teamChatBeep;
vmCvar_t cg_teamChatBeepMaxTime;

vmCvar_t cg_serverPrint;
vmCvar_t cg_serverPrintToChat;
vmCvar_t cg_serverPrintToConsole;

vmCvar_t cg_serverCenterPrint;
vmCvar_t cg_serverCenterPrintToChat;
vmCvar_t cg_serverCenterPrintToConsole;

vmCvar_t cg_drawCenterPrint;
vmCvar_t cg_drawCenterPrintX;
vmCvar_t cg_drawCenterPrintY;
vmCvar_t cg_drawCenterPrintAlign;
vmCvar_t cg_drawCenterPrintStyle;
vmCvar_t cg_drawCenterPrintFont;
vmCvar_t cg_drawCenterPrintPointSize;
vmCvar_t cg_drawCenterPrintScale;
vmCvar_t cg_drawCenterPrintTime;
vmCvar_t cg_drawCenterPrintColor;
vmCvar_t cg_drawCenterPrintAlpha;
vmCvar_t cg_drawCenterPrintFade;
vmCvar_t cg_drawCenterPrintFadeTime;

vmCvar_t cg_drawVote;
vmCvar_t cg_drawVoteX;
vmCvar_t cg_drawVoteY;
vmCvar_t cg_drawVoteAlign;
vmCvar_t cg_drawVoteStyle;
vmCvar_t cg_drawVoteFont;
vmCvar_t cg_drawVotePointSize;
vmCvar_t cg_drawVoteScale;
vmCvar_t cg_drawVoteColor;
vmCvar_t cg_drawVoteAlpha;

vmCvar_t cg_drawTeamVote;
vmCvar_t cg_drawTeamVoteX;
vmCvar_t cg_drawTeamVoteY;
vmCvar_t cg_drawTeamVoteAlign;
vmCvar_t cg_drawTeamVoteStyle;
vmCvar_t cg_drawTeamVoteFont;
vmCvar_t cg_drawTeamVotePointSize;
vmCvar_t cg_drawTeamVoteScale;
vmCvar_t cg_drawTeamVoteColor;
vmCvar_t cg_drawTeamVoteAlpha;

vmCvar_t cg_drawWaitingForPlayers;
vmCvar_t cg_drawWaitingForPlayersX;
vmCvar_t cg_drawWaitingForPlayersY;
vmCvar_t cg_drawWaitingForPlayersAlign;
vmCvar_t cg_drawWaitingForPlayersStyle;
vmCvar_t cg_drawWaitingForPlayersFont;
vmCvar_t cg_drawWaitingForPlayersPointSize;
vmCvar_t cg_drawWaitingForPlayersScale;
vmCvar_t cg_drawWaitingForPlayersColor;
vmCvar_t cg_drawWaitingForPlayersAlpha;

vmCvar_t cg_drawWarmupString;
vmCvar_t cg_drawWarmupStringX;
vmCvar_t cg_drawWarmupStringY;
vmCvar_t cg_drawWarmupStringAlign;
vmCvar_t cg_drawWarmupStringStyle;
vmCvar_t cg_drawWarmupStringFont;
vmCvar_t cg_drawWarmupStringPointSize;
vmCvar_t cg_drawWarmupStringScale;
vmCvar_t cg_drawWarmupStringColor;
vmCvar_t cg_drawWarmupStringAlpha;

vmCvar_t cg_ambientSounds;
vmCvar_t cg_weather;

vmCvar_t wolfcam_hoverTime;
vmCvar_t wolfcam_switchMode;
//vmCvar_t cg_fragForwardStyle;
vmCvar_t wolfcam_firstPersonSwitchSoundStyle;

vmCvar_t cg_interpolateMissiles;

vmCvar_t cg_weaponBlueTeamColor;
vmCvar_t cg_weaponRedTeamColor;
//vmCvar_t cg_noTeamColor;

vmCvar_t cg_hudRedTeamColor;
vmCvar_t cg_hudBlueTeamColor;
vmCvar_t cg_hudNoTeamColor;
vmCvar_t cg_hudForceRedTeamClanTag;
vmCvar_t cg_hudForceBlueTeamClanTag;

vmCvar_t cg_enemyModel;
vmCvar_t cg_enemyHeadSkin;
vmCvar_t cg_enemyTorsoSkin;
vmCvar_t cg_enemyLegsSkin;
vmCvar_t cg_enemyHeadColor;
vmCvar_t cg_enemyTorsoColor;
vmCvar_t cg_enemyLegsColor;
vmCvar_t cg_enemyRailColor1;
vmCvar_t cg_enemyRailColor2;
vmCvar_t cg_enemyRailColor1Team;
vmCvar_t cg_enemyRailColor2Team;
vmCvar_t cg_enemyRailItemColor;
vmCvar_t cg_enemyRailItemColorTeam;
vmCvar_t cg_enemyRailRings;
vmCvar_t cg_enemyRailNudge;
vmCvar_t cg_enemyFlagColor;

vmCvar_t cg_useDefaultTeamSkins;
vmCvar_t cg_teamModel;
vmCvar_t cg_teamHeadSkin;
vmCvar_t cg_teamTorsoSkin;
vmCvar_t cg_teamLegsSkin;
vmCvar_t cg_teamHeadColor;
vmCvar_t cg_teamTorsoColor;
vmCvar_t cg_teamLegsColor;
vmCvar_t cg_teamRailColor1;
vmCvar_t cg_teamRailColor2;
vmCvar_t cg_teamRailColor1Team;
vmCvar_t cg_teamRailColor2Team;
vmCvar_t cg_teamRailItemColor;
vmCvar_t cg_teamRailItemColorTeam;
vmCvar_t cg_teamRailRings;
vmCvar_t cg_teamRailNudge;
vmCvar_t cg_teamFlagColor;

vmCvar_t cg_neutralFlagColor;

vmCvar_t cg_ourModel;
vmCvar_t cg_deadBodyColor;
vmCvar_t cg_disallowEnemyModelForTeammates;

vmCvar_t cg_audioAnnouncer;
vmCvar_t cg_audioAnnouncerRewards;
vmCvar_t cg_audioAnnouncerRewardsFirst;
vmCvar_t cg_audioAnnouncerRound;
vmCvar_t cg_audioAnnouncerRoundReward;
vmCvar_t cg_audioAnnouncerWarmup;
vmCvar_t cg_audioAnnouncerVote;
vmCvar_t cg_audioAnnouncerTeamVote;
vmCvar_t cg_audioAnnouncerFlagStatus;
vmCvar_t cg_audioAnnouncerLead;
vmCvar_t cg_audioAnnouncerTimeLimit;
vmCvar_t cg_audioAnnouncerFragLimit;
vmCvar_t cg_audioAnnouncerWin;
vmCvar_t cg_audioAnnouncerScore;
vmCvar_t cg_audioAnnouncerLastStanding;
vmCvar_t cg_audioAnnouncerDominationPoint;

vmCvar_t wolfcam_drawFollowing;
vmCvar_t wolfcam_drawFollowingOnlyName;
vmCvar_t cg_printTimeStamps;
vmCvar_t cg_screenDamageAlpha_Team;
vmCvar_t cg_screenDamage_Team;
vmCvar_t cg_screenDamageAlpha_Self;
vmCvar_t cg_screenDamage_Self;
vmCvar_t cg_screenDamageAlpha;
vmCvar_t cg_screenDamage;
vmCvar_t cg_echoPopupTime;
vmCvar_t cg_echoPopupX;
vmCvar_t cg_echoPopupY;
vmCvar_t cg_echoPopupScale;
vmCvar_t cg_accX;
vmCvar_t cg_accY;
vmCvar_t cg_loadDefaultMenus;
vmCvar_t cg_grenadeColor;
vmCvar_t cg_grenadeColorAlpha;
vmCvar_t cg_grenadeTeamColor;
vmCvar_t cg_grenadeTeamColorAlpha;
vmCvar_t cg_grenadeEnemyColor;
vmCvar_t cg_grenadeEnemyColorAlpha;

vmCvar_t cg_fragMessageStyle;
vmCvar_t cg_drawFragMessageSeparate;
vmCvar_t cg_drawFragMessageX;
vmCvar_t cg_drawFragMessageY;
vmCvar_t cg_drawFragMessageAlign;
vmCvar_t cg_drawFragMessageStyle;
vmCvar_t cg_drawFragMessageFont;
vmCvar_t cg_drawFragMessagePointSize;
vmCvar_t cg_drawFragMessageScale;
vmCvar_t cg_drawFragMessageTime;
vmCvar_t cg_drawFragMessageColor;
vmCvar_t cg_drawFragMessageAlpha;
vmCvar_t cg_drawFragMessageFade;
vmCvar_t cg_drawFragMessageFadeTime;
vmCvar_t cg_drawFragMessageTokens;
vmCvar_t cg_drawFragMessageTeamTokens;
vmCvar_t cg_drawFragMessageThawTokens;
vmCvar_t cg_drawFragMessageFreezeTokens;
vmCvar_t cg_drawFragMessageFreezeTeamTokens;
vmCvar_t cg_drawFragMessageIconScale;

vmCvar_t cg_obituaryTokens;
vmCvar_t cg_obituaryIconScale;
vmCvar_t cg_obituaryRedTeamColor;
vmCvar_t cg_obituaryBlueTeamColor;
vmCvar_t cg_obituaryTime;
vmCvar_t cg_obituaryFadeTime;

vmCvar_t cg_fragTokenAccuracyStyle;

vmCvar_t cg_drawPlayerNames;
//vmCvar_t cg_drawPlayerNamesX;
vmCvar_t cg_drawPlayerNamesY;
vmCvar_t cg_drawPlayerNamesStyle;
vmCvar_t cg_drawPlayerNamesFont;
vmCvar_t cg_drawPlayerNamesPointSize;
vmCvar_t cg_drawPlayerNamesScale;
vmCvar_t cg_drawPlayerNamesColor;
vmCvar_t cg_drawPlayerNamesAlpha;

vmCvar_t cg_perKillStatsExcludePostKillSpam;
vmCvar_t cg_perKillStatsClearNotFiringTime;
vmCvar_t cg_perKillStatsClearNotFiringExcludeSingleClickWeapons;

vmCvar_t cg_printSkillRating;

vmCvar_t cg_lightningImpact;
vmCvar_t cg_lightningImpactSize;
vmCvar_t cg_lightningImpactOthersSize;
vmCvar_t cg_lightningImpactCap;
vmCvar_t cg_lightningImpactCapMin;
vmCvar_t cg_lightningImpactProject;
vmCvar_t cg_lightningStyle;
vmCvar_t cg_lightningRenderStyle;
vmCvar_t cg_lightningAngleOriginStyle;
vmCvar_t cg_debugLightningImpactDistance;
vmCvar_t cg_lightningSize;

vmCvar_t cg_drawEntNumbers;
vmCvar_t cg_drawEventNumbers;
vmCvar_t cg_demoSmoothing;
vmCvar_t cg_demoSmoothingAngles;
vmCvar_t cg_demoSmoothingTeleportCheck;
vmCvar_t cg_drawCameraPath;

vmCvar_t cg_drawCameraPointInfo;
vmCvar_t cg_drawCameraPointInfoX;
vmCvar_t cg_drawCameraPointInfoY;
vmCvar_t cg_drawCameraPointInfoAlign;
vmCvar_t cg_drawCameraPointInfoStyle;
vmCvar_t cg_drawCameraPointInfoFont;
vmCvar_t cg_drawCameraPointInfoPointSize;
vmCvar_t cg_drawCameraPointInfoScale;
vmCvar_t cg_drawCameraPointInfoColor;
vmCvar_t cg_drawCameraPointInfoSelectedColor;
vmCvar_t cg_drawCameraPointInfoAlpha;

vmCvar_t cg_drawViewPointMark;

vmCvar_t cg_levelTimerDirection;
//vmCvar_t cg_levelTimerStyle;
vmCvar_t cg_levelTimerDefaultTimeLimit;
vmCvar_t cg_checkForOfflineDemo;
vmCvar_t cg_muzzleFlash;

vmCvar_t cg_weaponDefault;
vmCvar_t cg_weaponNone;
vmCvar_t cg_weaponGauntlet;
vmCvar_t cg_weaponMachineGun;
vmCvar_t cg_weaponShotgun;
vmCvar_t cg_weaponGrenadeLauncher;
vmCvar_t cg_weaponRocketLauncher;
vmCvar_t cg_weaponLightningGun;
vmCvar_t cg_weaponRailGun;
vmCvar_t cg_weaponPlasmaGun;
vmCvar_t cg_weaponBFG;
vmCvar_t cg_weaponGrapplingHook;
vmCvar_t cg_weaponNailGun;
vmCvar_t cg_weaponProximityLauncher;
vmCvar_t cg_weaponChainGun;

vmCvar_t cg_spawnArmorTime;
vmCvar_t cg_fxfile;
vmCvar_t cg_fxinterval;
vmCvar_t cg_fxratio;
vmCvar_t cg_fxScriptMinEmitter;
vmCvar_t cg_fxScriptMinDistance;
vmCvar_t cg_fxScriptMinInterval;
vmCvar_t cg_fxLightningGunImpactFps;
vmCvar_t cg_fxDebugEntities;
vmCvar_t cg_fxCompiled;
#ifdef ENABLE_THREADS
 vmCvar_t cg_fxThreads;
#endif
vmCvar_t cg_vibrate;
vmCvar_t cg_vibrateTime;
vmCvar_t cg_vibrateMaxDistance;
vmCvar_t cg_vibrateForce;

vmCvar_t cg_animationsIgnoreTimescale;
vmCvar_t cg_animationsRate;

vmCvar_t cg_quadFireSound;
vmCvar_t cg_kickScale;

vmCvar_t cg_gameType;
vmCvar_t cg_compMode;
vmCvar_t cg_drawSpecMessages;
vmCvar_t g_training;

vmCvar_t cg_waterWarp;
vmCvar_t cg_allowLargeSprites;
vmCvar_t cg_allowSpritePassThrough;
vmCvar_t cg_drawSprites;
vmCvar_t cg_drawSpriteSelf;
vmCvar_t cg_playerLeanScale;
vmCvar_t cg_cameraRewindTime;
vmCvar_t cg_cameraQue;
vmCvar_t cg_cameraUpdateFreeCam;
vmCvar_t cg_cameraDefaultOriginType;
vmCvar_t cg_cameraAddUsePreviousValues;
vmCvar_t cg_cameraDebugPath;
vmCvar_t cg_cameraSmoothFactor;

vmCvar_t cg_flightTrail;
vmCvar_t cg_hasteTrail;

vmCvar_t cg_noItemUseMessage;
vmCvar_t cg_noItemUseSound;
vmCvar_t cg_itemUseMessage;
vmCvar_t cg_itemUseSound;

vmCvar_t cg_localTime;
vmCvar_t cg_localTimeStyle;
vmCvar_t cg_warmupTime;

vmCvar_t cg_clientOverrideIgnoreTeamSettings;
vmCvar_t cg_killBeep;
vmCvar_t cg_deathStyle;
vmCvar_t cg_deathShowOwnCorpse;
vmCvar_t cg_mouseSeekScale;
vmCvar_t cg_mouseSeekPollInterval;
vmCvar_t cg_mouseSeekUseTimescale;

vmCvar_t cg_teamKillWarning;
vmCvar_t cg_inheritPowerupShader;
vmCvar_t cg_fadeColor;
vmCvar_t cg_fadeAlpha;
vmCvar_t cg_fadeStyle;
vmCvar_t cg_enableAtCommands;
vmCvar_t cg_quadKillCounter;
vmCvar_t cg_battleSuitKillCounter;
vmCvar_t cg_wideScreen;
vmCvar_t cg_wideScreenScoreBoardHack;

vmCvar_t cg_wh;
vmCvar_t cg_whIncludeDeadBody;
vmCvar_t cg_whIncludeProjectile;
vmCvar_t cg_whShader;

vmCvar_t cg_playerShader;
vmCvar_t cg_adShaderOverride;

vmCvar_t cg_firstPersonSwitchSound;
vmCvar_t cg_proxMineTick;

vmCvar_t cg_drawProxWarning;
vmCvar_t cg_drawProxWarningX;
vmCvar_t cg_drawProxWarningY;
vmCvar_t cg_drawProxWarningAlign;
vmCvar_t cg_drawProxWarningStyle;
vmCvar_t cg_drawProxWarningFont;
vmCvar_t cg_drawProxWarningPointSize;
vmCvar_t cg_drawProxWarningScale;
vmCvar_t cg_drawProxWarningColor;
vmCvar_t cg_drawProxWarningAlpha;

vmCvar_t cg_customMirrorSurfaces;
vmCvar_t cg_demoStepSmoothing;
vmCvar_t cg_stepSmoothTime;
vmCvar_t cg_stepSmoothMaxChange;
vmCvar_t cg_pathRewindTime;
vmCvar_t cg_pathSkipNum;
vmCvar_t cg_dumpEntsUseServerTime;

vmCvar_t cg_playerModelForceScale;
vmCvar_t cg_playerModelForceLegsScale;
vmCvar_t cg_playerModelForceTorsoScale;
vmCvar_t cg_playerModelForceHeadScale;
vmCvar_t cg_playerModelForceHeadOffset;
vmCvar_t cg_playerModelAutoScaleHeight;
vmCvar_t cg_playerModelAllowServerScale;
vmCvar_t cg_playerModelAllowServerScaleDefault;
vmCvar_t cg_playerModelAllowServerOverride;
vmCvar_t cg_allowServerOverride;

vmCvar_t cg_powerupLight;
vmCvar_t cg_buzzerSound;
vmCvar_t cg_flagStyle;
vmCvar_t cg_helpIconStyle;
vmCvar_t cg_helpIconMinWidth;
vmCvar_t cg_helpIconMaxWidth;
//vmCvar_t cg_helpIconAlpha;
vmCvar_t cg_dominationPointTeamColor;
vmCvar_t cg_dominationPointTeamAlpha;
vmCvar_t cg_dominationPointEnemyColor;
vmCvar_t cg_dominationPointEnemyAlpha;
vmCvar_t cg_dominationPointNeutralColor;
vmCvar_t cg_dominationPointNeutralAlpha;
vmCvar_t cg_attackDefendVoiceStyle;

vmCvar_t cg_drawDominationPointStatus;
vmCvar_t cg_drawDominationPointStatusX;
vmCvar_t cg_drawDominationPointStatusY;
vmCvar_t cg_drawDominationPointStatusFont;
vmCvar_t cg_drawDominationPointStatusPointSize;
vmCvar_t cg_drawDominationPointStatusScale;
vmCvar_t cg_drawDominationPointStatusEnemyColor;
vmCvar_t cg_drawDominationPointStatusTeamColor;
vmCvar_t cg_drawDominationPointStatusBackgroundColor;
vmCvar_t cg_drawDominationPointStatusAlpha;
vmCvar_t cg_drawDominationPointStatusTextColor;
vmCvar_t cg_drawDominationPointStatusTextAlpha;
vmCvar_t cg_drawDominationPointStatusTextStyle;

vmCvar_t cg_roundScoreBoard;
vmCvar_t cg_headShots;

vmCvar_t cg_spectatorListSkillRating;
vmCvar_t cg_spectatorListScore;
vmCvar_t cg_spectatorListQue;

vmCvar_t cg_rocketAimBot;
vmCvar_t cg_drawTieredArmorAvailability;

vmCvar_t cg_drawDeadFriendTime;
vmCvar_t cg_drawHitFriendTime;
vmCvar_t cg_racePlayerShader;

// end cvar_t

typedef struct {
	vmCvar_t*vmCvar;
	const char *cvarName;
	const char *defaultString;
	int			cvarFlags;
} cvarTable_t;

#define xmstr(x) mstr(x)
#define mstr(x) #x

#define cvp(x) &x, #x

static cvarTable_t cvarTable[] = { // bk001129
	{ &cg_ignore, "cg_ignore", "0", 0 },	// used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "0", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_zoomTime, "cg_zoomTime", "150", CVAR_ARCHIVE },
	{ &cg_zoomIgnoreTimescale, "cg_zoomIgnoreTimescale", "1", CVAR_ARCHIVE },
	{ &cg_zoomBroken, "cg_zoomBroken", "1", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", xmstr(DEFAULT_FOV), CVAR_ARCHIVE },
	{ &cg_fovy, "cg_fovy", "", CVAR_ARCHIVE },
	{ &cg_fovStyle, "cg_fovStyle", "1", CVAR_ARCHIVE },
	{ &cg_fovIntermission, "cg_fovIntermission", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE  },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "15", CVAR_ARCHIVE  },
	{ &cg_gibColor, "cg_gibColor", "", CVAR_ARCHIVE },
	{ &cg_gibJump, "cg_gibJump", "0", CVAR_ARCHIVE },
	{ &cg_gibVelocity, "cg_gibVelocity", "600", CVAR_ARCHIVE },
	{ &cg_gibVelocityRandomness, "cg_gibVelocityRandomness", "250", CVAR_ARCHIVE },
	{ &cg_gibDirScale, "cg_gibDirScale", "1.0", CVAR_ARCHIVE },
	{ &cg_gibOriginOffset, "cg_gibOriginOffset", "0", CVAR_ARCHIVE },
	{ &cg_gibOriginOffsetZ, "cg_gibOriginOffsetZ", "0", CVAR_ARCHIVE },
	{ &cg_gibRandomness, "cg_gibRandomness", "250", CVAR_ARCHIVE },
	{ &cg_gibRandomnessZ, "cg_gibRandomnessZ", "0", CVAR_ARCHIVE },
	{ &cg_gibTime, "cg_gibTime", "1000", CVAR_ARCHIVE },
	{ &cg_gibStepTime, "cg_gibStepTime", "50", CVAR_ARCHIVE },
	{ &cg_gibBounceFactor, "cg_gibBounceFactor", "0.6", CVAR_ARCHIVE },
	{ &cg_gibGravity, "cg_gibGravity", "800", CVAR_ARCHIVE },
	{ &cg_gibSparksSize, "cg_gibSparksSize", "3.5", CVAR_ARCHIVE },
	{ &cg_gibSparksColor, "cg_gibSparksColor", "", CVAR_ARCHIVE },
	{ &cg_gibSparksHighlight, "cg_gibSparksHighlight", "", CVAR_ARCHIVE },
	{ &cg_gibFloatingVelocity, "cg_gibFloatingVelocity", "125", CVAR_ARCHIVE },
	{ &cg_gibFloatingRandomness, "cg_gibFloatingRandomness", "75", CVAR_ARCHIVE },
	{ &cg_gibFloatingOriginOffset, "cg_gibFloatingOriginOffset", "30", CVAR_ARCHIVE },
	{ &cg_gibFloatingOriginOffsetZ, "cg_gibFloatingOriginOffsetZ", "0", CVAR_ARCHIVE },
	{ &cg_impactSparks, "cg_impactSparks", "1", CVAR_ARCHIVE },
	{ &cg_impactSparksLifetime, "cg_impactSparksLifetime", "250", CVAR_ARCHIVE },
	{ &cg_impactSparksSize, "cg_impactSparksSize", "8", CVAR_ARCHIVE },
	{ &cg_impactSparksVelocity, "cg_impactSparksVelocity", "128", CVAR_ARCHIVE },
	{ &cg_impactSparksColor, "cg_impactSparksColor", "", CVAR_ARCHIVE },
	{ &cg_impactSparksHighlight, "cg_impactSparksHighlight", "0", CVAR_ARCHIVE },
	{ &cg_shotgunImpactSparks, "cg_shotgunImpactSparks", "1", CVAR_ARCHIVE },
	{ &cg_shotgunMarks, "cg_shotgunMarks", "1", CVAR_ARCHIVE },
	{ &cg_shotgunStyle, "cg_shotgunStyle", "1", CVAR_ARCHIVE },
	{ &cg_shotgunRandomness, "cg_shotgunRandomness", "2.0", CVAR_ARCHIVE },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTeamBackground, "cg_drawTeamBackground", "1", CVAR_ARCHIVE },
	{ &cg_drawTimer, "cg_drawTimer", "1", CVAR_ARCHIVE  },
	{ &cg_drawClientItemTimer, "cg_drawClientItemTimer", "1", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerX, "cg_drawClientItemTimerX", "635", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerY, "cg_drawClientItemTimerY", "150", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerScale, "cg_drawClientItemTimerScale", "0.4", CVAR_ARCHIVE },
	{ cvp(cg_drawClientItemTimerTextColor), "", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerFont, "cg_drawClientItemTimerFont", "q3small", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerPointSize, "cg_drawClientItemTimerPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerAlpha, "cg_drawClientItemTimerAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerStyle, "cg_drawClientItemTimerStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerAlign, "cg_drawClientItemTimerAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawClientItemTimerSpacing, "cg_drawClientItemTimerSpacing", "", CVAR_ARCHIVE },
	{ cvp(cg_drawClientItemTimerIcon), "1", CVAR_ARCHIVE },
	{ cvp(cg_drawClientItemTimerIconSize), "20", CVAR_ARCHIVE },
	{ cvp(cg_drawClientItemTimerIconXoffset), "-55", CVAR_ARCHIVE },
	{ cvp(cg_drawClientItemTimerIconYoffset), "0", CVAR_ARCHIVE },

	{ &cg_itemSpawnPrint, "cg_itemSpawnPrint", "0", CVAR_ARCHIVE },

	{ &cg_drawFPS, "cg_drawFPS", "1", CVAR_ARCHIVE },
	{ &cg_drawFPSNoText, "cg_drawFPSNoText", "0", CVAR_ARCHIVE },
	{ &cg_drawFPSX, "cg_drawFPSX", "635", CVAR_ARCHIVE },
	{ &cg_drawFPSY, "cg_drawFPSY", "", CVAR_ARCHIVE },
	{ &cg_drawFPSAlign, "cg_drawFPSAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawFPSStyle, "cg_drawFPSStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawFPSFont, "cg_drawFPSFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawFPSPointSize, "cg_drawFPSPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawFPSScale, "cg_drawFPSScale", "0.4", CVAR_ARCHIVE },
	//	{ &cg_drawFPSTime, "cg_drawFPSTime", "", CVAR_ARCHIVE },
	{ &cg_drawFPSColor, "cg_drawFPSColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawFPSAlpha, "cg_drawFPSAlpha", "255", CVAR_ARCHIVE },
	//	{ &cg_drawFPSFade, "cg_drawFPSFade", "1", CVAR_ARCHIVE },
	//	{ &cg_drawFPSFadeTime, "cg_drawFPSFadeTime", "200", CVAR_ARCHIVE },

	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshotX, "cg_drawSnapshotX", "635", CVAR_ARCHIVE },
	{ &cg_drawSnapshotY, "cg_drawSnapshotY", "", CVAR_ARCHIVE },
	{ &cg_drawSnapshotAlign, "cg_drawSnapshotAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawSnapshotStyle, "cg_drawSnapshotStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawSnapshotFont, "cg_drawSnapshotFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawSnapshotPointSize, "cg_drawSnapshotPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawSnapshotScale, "cg_drawSnapshotScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawSnapshotColor, "cg_drawSnapshotColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawSnapshotAlpha, "cg_drawSnapshotAlpha", "255", CVAR_ARCHIVE },

	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },

	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarningX, "cg_drawAmmoWarningX", "320", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningY, "cg_drawAmmoWarningY", "64", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningAlign, "cg_drawAmmoWarningAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningStyle, "cg_drawAmmoWarningStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningFont, "cg_drawAmmoWarningFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningPointSize, "cg_drawAmmoWarningPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningScale, "cg_drawAmmoWarningScale", "0.4", CVAR_ARCHIVE },
	//{ &cg_drawAmmoWarningTime, "cg_drawAmmoWarningTime", "", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningColor, "cg_drawAmmoWarningColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarningAlpha, "cg_drawAmmoWarningAlpha", "255", CVAR_ARCHIVE },
	//{ &cg_drawAmmoWarningFade, "cg_drawAmmoWarningFade", "1", CVAR_ARCHIVE },
	//{ &cg_drawAmmoWarningFadeTime, "cg_drawAmmoWarningFadeTime", "200", CVAR_ARCHIVE },

	{ cvp(cg_lowAmmoWarningStyle), "1", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningPercentile), "0.20", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningSound), "1", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWeaponBarWarning), "2", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningGauntlet), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningMachineGun), "30", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningShotgun), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningGrenadeLauncher), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningRocketLauncher), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningLightningGun), "30", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningRailGun), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningPlasmaGun), "30", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningBFG), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningGrapplingHook), "5", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningNailGun), "30", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningProximityLauncher), "30", CVAR_ARCHIVE },
	{ cvp(cg_lowAmmoWarningChainGun), "30", CVAR_ARCHIVE },

	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  },
	{ &cg_drawAttackerX, "cg_drawAttackerX", "640", CVAR_ARCHIVE },
	{ &cg_drawAttackerY, "cg_drawAttackerY", "", CVAR_ARCHIVE },  //FIXME check default
	{ &cg_drawAttackerAlign, "cg_drawAttackerAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawAttackerStyle, "cg_drawAttackerStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawAttackerFont, "cg_drawAttackerFont", "", CVAR_ARCHIVE },
	{ &cg_drawAttackerPointSize, "cg_drawAttackerPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawAttackerScale, "cg_drawAttackerScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawAttackerImageScale, "cg_drawAttackerImageScale", "3", CVAR_ARCHIVE },
	{ &cg_drawAttackerTime, "cg_drawAttackerTime", "10000", CVAR_ARCHIVE },
	{ &cg_drawAttackerColor, "cg_drawAttackerColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawAttackerAlpha, "cg_drawAttackerAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawAttackerFade, "cg_drawAttackerFade", "1", CVAR_ARCHIVE },
	{ &cg_drawAttackerFadeTime, "cg_drawAttackerFadeTime", "10000", CVAR_ARCHIVE },

	{ &cg_drawCrosshair, "cg_drawCrosshair", "5", CVAR_ARCHIVE },

	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesX, "cg_drawCrosshairNamesX", "320", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesY, "cg_drawCrosshairNamesY", "190", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesAlign, "cg_drawCrosshairNamesAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesStyle, "cg_drawCrosshairNamesStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesFont, "cg_drawCrosshairNamesFont", "", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesPointSize, "cg_drawCrosshairNamesPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesScale, "cg_drawCrosshairNamesScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesTime, "cg_drawCrosshairNamesTime", "1000", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesColor, "cg_drawCrosshairNamesColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesAlpha, "cg_drawCrosshairNamesAlpha", "77", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesFade, "cg_drawCrosshairNamesFade", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNamesFadeTime, "cg_drawCrosshairNamesFadeTime", "1000", CVAR_ARCHIVE },

	{ &cg_drawCrosshairTeammateHealth, "cg_drawCrosshairTeammateHealth", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthX, "cg_drawCrosshairTeammateHealthX", "320", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthY, "cg_drawCrosshairTeammateHealthY", "200", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthAlign, "cg_drawCrosshairTeammateHealthAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthStyle, "cg_drawCrosshairTeammateHealthStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthFont, "cg_drawCrosshairTeammateHealthFont", "", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthPointSize, "cg_drawCrosshairTeammateHealthPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthScale, "cg_drawCrosshairTeammateHealthScale", "0.125", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthTime, "cg_drawCrosshairTeammateHealthTime", "1000", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthColor, "cg_drawCrosshairTeammateHealthColor", "", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthAlpha, "cg_drawCrosshairTeammateHealthAlpha", "77", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthFade, "cg_drawCrosshairTeammateHealthFade", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairTeammateHealthFadeTime, "cg_drawCrosshairTeammateHealthFadeTime", "1000", CVAR_ARCHIVE },

	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_drawRewardsMax, "cg_drawRewardsMax", "10", CVAR_ARCHIVE },
	{ &cg_drawRewardsX, "cg_drawRewardsX", "320", CVAR_ARCHIVE },
	{ &cg_drawRewardsY, "cg_drawRewardsY", "56", CVAR_ARCHIVE },
	{ &cg_drawRewardsAlign, "cg_drawRewardsAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawRewardsStyle, "cg_drawRewardsStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawRewardsFont, "cg_drawRewardsFont", "", CVAR_ARCHIVE },
	{ &cg_drawRewardsPointSize, "cg_drawRewardsPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawRewardsScale, "cg_drawRewardsScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawRewardsImageScale, "cg_drawRewardsImageScale", "1.0", CVAR_ARCHIVE },
	{ &cg_drawRewardsTime, "cg_drawRewardsTime", "3000", CVAR_ARCHIVE },
	{ &cg_drawRewardsColor, "cg_drawRewardsColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawRewardsAlpha, "cg_drawRewardsAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawRewardsFade, "cg_drawRewardsFade", "1", CVAR_ARCHIVE },
	{ &cg_drawRewardsFadeTime, "cg_drawRewardsFadeTime", "200", CVAR_ARCHIVE },
	{ &cg_rewardsStack, "cg_rewardsStack", "1", CVAR_ARCHIVE },

	{ &cg_crosshairSize, "cg_crosshairSize", "32", CVAR_ARCHIVE },
	{ &cg_crosshairColor, "cg_crosshairColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_crosshairPulse, "cg_crosshairPulse", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHitStyle, "cg_crosshairHitStyle", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHitColor, "cg_crosshairHitColor", "0xff0000", CVAR_ARCHIVE },
	{ &cg_crosshairHitTime, "cg_crosshairHitTime", "200", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_crosshairBrightness, "cg_crosshairBrightness", "1.0", CVAR_ARCHIVE },
	{ &cg_crosshairAlpha, "cg_crosshairAlpha", "255", CVAR_ARCHIVE },
	{ &cg_crosshairAlphaAdjust, "cg_crosshairAlphaAdjust", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_simpleItemsScale, "cg_simpleItemsScale", "1.0", CVAR_ARCHIVE },
	{ &cg_simpleItemsBob, "cg_simpleItemsBob", "0", CVAR_ARCHIVE },
	{ &cg_simpleItemsHeightOffset, "cg_simpleItemsHeightOffset", "0", CVAR_ARCHIVE },
	{ &cg_itemsWh, "cg_itemsWh", "0", CVAR_ARCHIVE },
	{ &cg_itemFx, "cg_itemFx", "7", CVAR_ARCHIVE },
	{ &cg_itemSize, "cg_itemSize", "1.0", CVAR_ARCHIVE },
	{ &cg_marks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_markTime, "cg_markTime", "10000", CVAR_ARCHIVE },
	{ &cg_markFadeTime, "cg_markFadeTime", "1000", CVAR_ARCHIVE },
	{ &cg_debugImpactOrigin, "cg_debugImpactOrigin", "0", CVAR_ARCHIVE },

	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_lagometerX, "cg_lagometerX", "640", CVAR_ARCHIVE },
	{ &cg_lagometerY, "cg_lagometerY", "336", CVAR_ARCHIVE },
	{ &cg_lagometerFlash, "cg_lagometerFlash", "1", CVAR_ARCHIVE },
	{ &cg_lagometerFlashValue, "cg_lagometerFlashValue", "80", CVAR_ARCHIVE },
	{ &cg_lagometerFontAlign, "cg_lagometerFontAlign", "0", CVAR_ARCHIVE },
	{ &cg_lagometerAlign, "cg_lagometerAlign", "2", CVAR_ARCHIVE },
	{ &cg_lagometerFontStyle, "cg_lagometerFontStyle", "3", CVAR_ARCHIVE },
	{ &cg_lagometerFont, "cg_lagometerFont", "q3big", CVAR_ARCHIVE },
	{ &cg_lagometerFontPointSize, "cg_lagometerFontPointSize", "24", CVAR_ARCHIVE },
	{ &cg_lagometerFontScale, "cg_lagometerFontScale", "0.25", CVAR_ARCHIVE },
	{ &cg_lagometerScale, "cg_lagometerScale", "1.0", CVAR_ARCHIVE },
	{ &cg_lagometerFontColor, "cg_lagometerFontColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_lagometerAlpha, "cg_lagometerAlpha", "255", CVAR_ARCHIVE },
	{ &cg_lagometerFontAlpha, "cg_lagometerFontAlpha", "255", CVAR_ARCHIVE },
	{ &cg_lagometerAveragePing, "cg_lagometerAveragePing", "1", CVAR_ARCHIVE },
	{ &cg_lagometerSnapshotPing, "cg_lagometerSnapshotPing", "1", CVAR_ARCHIVE },

	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_railQL, "cg_railQL", "1", CVAR_ARCHIVE },
	{ &cg_railQLRailRingWhiteValue, "cg_railQLRailRingWhiteValue", "0.45", CVAR_ARCHIVE },
	{ &cg_railNudge, "cg_railNudge", "1", CVAR_ARCHIVE },
	{ &cg_railRings, "cg_railRings", "0", CVAR_ARCHIVE },
	{ &cg_railRadius, "cg_railRadius", "4", CVAR_ARCHIVE },
	{ &cg_railRotation, "cg_railRotation", "1", CVAR_ARCHIVE },
	{ &cg_railSpacing, "cg_railSpacing", "5", CVAR_ARCHIVE },
	{ &cg_railItemColor, "cg_railItemColor", "0xd4af37", CVAR_ARCHIVE },
	{ &cg_railUseOwnColors, "cg_railUseOwnColors", "0", CVAR_ARCHIVE },
	{ &cg_railFromMuzzle, "cg_railFromMuzzle", "1", CVAR_ARCHIVE },

	{ &cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE },
	{ &cg_gunSize, "cg_gunSize", "1.0", CVAR_ARCHIVE },
	{ &cg_gunSizeThirdPerson, "cg_gunSizeThirdPerson", "1.0", CVAR_ARCHIVE },
	//{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_bobup , "cg_bobup", "0", CVAR_ARCHIVE },        // 0.005
	{ &cg_bobpitch, "cg_bobpitch", "0", CVAR_ARCHIVE }, // 0.002
	{ &cg_bobroll, "cg_bobroll", "0", CVAR_ARCHIVE },   // 0.002
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerChance", "0.4", CVAR_ARCHIVE },
	{ &cg_tracerWidth, "cg_tracerWidth", "1", CVAR_ARCHIVE },
	{ &cg_tracerLength, "cg_tracerLength", "100", CVAR_ARCHIVE },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_forcePovModel, "cg_forcePovModel", "0", CVAR_ARCHIVE },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#endif
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlayX, "cg_drawTeamOverlayX", "640", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlayY, "cg_drawTeamOverlayY", "", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "1", CVAR_ROM | CVAR_USERINFO },
	{ &cg_drawTeamOverlayFont, "cg_drawTeamOverlayFont", "q3tiny", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlayPointSize, "cg_drawTeamOverlayPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlayAlign, "cg_drawTeamOverlayAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlayScale, "cg_drawTeamOverlayScale", "0.4", CVAR_ARCHIVE },
	//{ &cg_drawTeamOverlayAlpha, "cg_drawTeamOverlayAlpha", "255", CVAR_ARCHIVE },

	{ &cg_drawJumpSpeeds, "cg_drawJumpSpeeds", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsNoText, "cg_drawJumpSpeedsNoText", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsMax, "cg_drawJumpSpeedsMax", "12", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsX, "cg_drawJumpSpeedsX", "5", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsY, "cg_drawJumpSpeedsY", "300", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsAlign, "cg_drawJumpSpeedsAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsStyle, "cg_drawJumpSpeedsStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsFont, "cg_drawJumpSpeedsFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsPointSize, "cg_drawJumpSpeedsPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsScale, "cg_drawJumpSpeedsScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsColor, "cg_drawJumpSpeedsColor", "0xffff00", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsAlpha, "cg_drawJumpSpeedsAlpha", "255", CVAR_ARCHIVE },


	{ &cg_drawJumpSpeedsTime, "cg_drawJumpSpeedsTime", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeNoText, "cg_drawJumpSpeedsTimeNoText", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeX, "cg_drawJumpSpeedsTimeX", "5", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeY, "cg_drawJumpSpeedsTimeY", "320", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeAlign, "cg_drawJumpSpeedsTimeAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeStyle, "cg_drawJumpSpeedsTimeStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeFont, "cg_drawJumpSpeedsTimeFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimePointSize, "cg_drawJumpSpeedsTimePointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeScale, "cg_drawJumpSpeedsTimeScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeColor, "cg_drawJumpSpeedsTimeColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawJumpSpeedsTimeAlpha, "cg_drawJumpSpeedsTimeAlpha", "255", CVAR_ARCHIVE },

	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "3", CVAR_ARCHIVE },
	{ &cg_drawFriendMinWidth, "cg_drawFriendMinWidth", "4.0", CVAR_ARCHIVE },
	{ &cg_drawFriendMaxWidth, "cg_drawFriendMaxWidth", "24.0", CVAR_ARCHIVE },
	{ &cg_drawFoe, "cg_drawFoe", "0", CVAR_ARCHIVE },
	{ &cg_drawFoeMinWidth, "cg_drawFoeMinWidth", "4.0", CVAR_ARCHIVE },
	{ &cg_drawFoeMaxWidth, "cg_drawFoeMaxWidth", "24.0", CVAR_ARCHIVE },
	{ &cg_drawSelf, "cg_drawSelf", "2", CVAR_ARCHIVE },
	{ &cg_drawSelfMinWidth, "cg_drawSelfMinWidth", "4.0", CVAR_ARCHIVE },
	{ &cg_drawSelfMaxWidth, "cg_drawSelfMaxWidth", "24.0", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
#endif
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
#if 1  //def MPACK
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_serverEnableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_serverEnableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO},
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
#endif
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_ARCHIVE},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlums, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO },
	//{ &g_weapon_plasma_rate, "g_weapon_plasma_rate", "100", 0 },

	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{ &cg_smokeRadius_SG, "cg_smokeRadius_SG", "32", CVAR_ARCHIVE },
	{ &cg_smokeRadius_GL, "cg_smokeRadius_GL", "32", CVAR_ARCHIVE },
	{ &cg_smokeRadius_NG, "cg_smokeRadius_NG", "16", CVAR_ARCHIVE },
	{ &cg_smokeRadius_RL, "cg_smokeRadius_RL", "64", CVAR_ARCHIVE },
	{ &cg_smokeRadius_PL, "cg_smokeRadius_PL", "32", CVAR_ARCHIVE },
	{ &cg_smokeRadius_breath, "cg_smokeRadius_breath", "16", CVAR_ARCHIVE },
	{ &cg_enableBreath, "cg_enableBreath", "1", CVAR_ARCHIVE },
	{ &cg_smokeRadius_dust, "cg_smokeRadius_dust", "24", CVAR_ARCHIVE },
	{ &cg_enableDust, "cg_enableDust", "1", CVAR_ARCHIVE },
	{ &cg_smokeRadius_flight, "cg_smokeRadius_flight", "8", CVAR_ARCHIVE },
	{ &cg_smokeRadius_haste, "cg_smokeRadius_haste", "8", CVAR_ARCHIVE },
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	//{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE},
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE},
	//{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE},
	{ &cg_plasmaStyle, "cg_plasmaStyle", "1", CVAR_ARCHIVE },
	{ &cg_trueLightning, "cg_trueLightning", "1.0", CVAR_ARCHIVE},
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE },

    { &cg_drawSpeed, "cg_drawSpeed", "1", CVAR_ARCHIVE },
    { &cg_drawSpeedX, "cg_drawSpeedX", "635", CVAR_ARCHIVE },
    { &cg_drawSpeedY, "cg_drawSpeedY", "", CVAR_ARCHIVE },
	{ &cg_drawSpeedNoText, "cg_drawSpeedNoText", "0", CVAR_ARCHIVE },
	{ &cg_drawSpeedAlign, "cg_drawSpeedAlign", "2", CVAR_ARCHIVE },
	{ &cg_drawSpeedStyle, "cg_drawSpeedStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawSpeedFont, "cg_drawSpeedFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawSpeedPointSize, "cg_drawSpeedPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawSpeedScale, "cg_drawSpeedScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawSpeedColor, "cg_drawSpeedColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawSpeedAlpha, "cg_drawSpeedAlpha", "255", CVAR_ARCHIVE },

	{ &cg_drawOrigin, "cg_drawOrigin", "0", CVAR_ARCHIVE },
	{ &cg_drawOriginX, "cg_drawOriginX", "5", CVAR_ARCHIVE },
	{ &cg_drawOriginY, "cg_drawOriginY", "420", CVAR_ARCHIVE },
	{ &cg_drawOriginAlign, "cg_drawOriginAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawOriginStyle, "cg_drawOriginStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawOriginFont, "cg_drawOriginFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawOriginPointSize, "cg_drawOriginPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawOriginScale, "cg_drawOriginScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawOriginColor, "cg_drawOriginColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawOriginAlpha, "cg_drawOriginAlpha", "255", CVAR_ARCHIVE },

	{ &cg_drawScores, "cg_drawScores", "1", CVAR_ARCHIVE },
	{ &cg_drawPlayersLeft, "cg_drawPlayersLeft", "1", CVAR_ARCHIVE },
	{ &cg_drawPowerups, "cg_drawPowerups", "1", CVAR_ARCHIVE },

	{ &cg_drawItemPickups, "cg_drawItemPickups", "3", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsX, "cg_drawItemPickupsX", "8", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsY, "cg_drawItemPickupsY", "360", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsImageScale, "cg_drawItemPickupsImageScale", "0.5", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsAlign, "cg_drawItemPickupsAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsStyle, "cg_drawItemPickupsStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsFont, "cg_drawItemPickupsFont", "q3small", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsPointSize, "cg_drawItemPickupsPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsScale, "cg_drawItemPickupsScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsTime, "cg_drawItemPickupsTime", "3000", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsColor, "cg_drawItemPickupsColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsAlpha, "cg_drawItemPickupsAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsFade, "cg_drawItemPickupsFade", "1", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsFadeTime, "cg_drawItemPickupsFadeTime", "3000", CVAR_ARCHIVE },
	{ &cg_drawItemPickupsCount, "cg_drawItemPickupsCount", "1", CVAR_ARCHIVE },

	{ &cg_drawFollowing, "cg_drawFollowing", "1", CVAR_ARCHIVE },
	{ &cg_drawFollowingX, "cg_drawFollowingX", "320", CVAR_ARCHIVE },
	{ &cg_drawFollowingY, "cg_drawFollowingY", "50", CVAR_ARCHIVE },
	{ &cg_drawFollowingAlign, "cg_drawFollowingAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawFollowingStyle, "cg_drawFollowingStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawFollowingFont, "cg_drawFollowingFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawFollowingPointSize, "cg_drawFollowingPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawFollowingScale, "cg_drawFollowingScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawFollowingColor, "cg_drawFollowingColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawFollowingAlpha, "cg_drawFollowingAlpha", "255", CVAR_ARCHIVE },

	{ &cg_testQlFont, "cg_testQlFont", "0", CVAR_ARCHIVE },
	//{ &cg_deathSparkRadius, "cg_deathSparkRadius", "8.0", CVAR_ARCHIVE },
	//{ &cg_customHud, "cg_customHud", "", CVAR_ARCHIVE },
	{ &cg_qlhud, "cg_qlhud", "1", CVAR_ARCHIVE },
	{ &cg_qlFontScaling, "cg_qlFontScaling", "1", CVAR_ARCHIVE },

	{ &cg_weaponBar, "cg_weaponBar", "1", CVAR_ARCHIVE },
	{ &cg_weaponBarX, "cg_weaponBarX", "", CVAR_ARCHIVE },
	{ &cg_weaponBarY, "cg_weaponBarY", "", CVAR_ARCHIVE },
	{ &cg_weaponBarFont, "cg_weaponBarFont", "", CVAR_ARCHIVE },
	{ &cg_weaponBarPointSize, "cg_weaponBarPointSize", "24", CVAR_ARCHIVE },

	{ &cg_drawFullWeaponBar, "cg_drawFullWeaponBar", "1", CVAR_ARCHIVE },
	{ &cg_scoreBoardStyle, "cg_scoreBoardStyle", "1", CVAR_ARCHIVE },
	{ &cg_scoreBoardSpectatorScroll, "cg_scoreBoardSpectatorScroll", "0", CVAR_ARCHIVE },
	{ &cg_scoreBoardWhenDead, "cg_scoreBoardWhenDead", "1", CVAR_ARCHIVE },
	{ &cg_scoreBoardAtIntermission, "cg_scoreBoardAtIntermission", "1", CVAR_ARCHIVE },
	{ &cg_scoreBoardWarmup, "cg_scoreBoardWarmup", "1", CVAR_ARCHIVE },
	{ &cg_scoreBoardOld, "cg_scoreBoardOld", "0", CVAR_ARCHIVE },

	{ &cg_hitBeep, "cg_hitBeep", "2", CVAR_ARCHIVE },

	{ &cg_drawSpawns, "cg_drawSpawns", "0", CVAR_ARCHIVE },
	{ &cg_drawSpawnsInitial, "cg_drawSpawnsInitial", "1", CVAR_ARCHIVE },
	{ &cg_drawSpawnsInitialZ, "cg_drawSpawnsInitalZ", "0.0", CVAR_ARCHIVE },
	{ &cg_drawSpawnsRespawns, "cg_drawSpawnsRespawns", "1", CVAR_ARCHIVE },
	{ &cg_drawSpawnsRespawnsZ, "cg_drawSpawnsRespawnsZ", "0.0", CVAR_ARCHIVE },
	{ &cg_drawSpawnsShared, "cg_drawSpawnsShared", "1", CVAR_ARCHIVE },
	{ &cg_drawSpawnsSharedZ, "cg_drawSpawnsSharedZ", "0.0", CVAR_ARCHIVE },

	{ &cg_freecam_noclip, "cg_freecam_noclip", "0", CVAR_ARCHIVE },
	{ &cg_freecam_sensitivity, "cg_freecam_sensitivity", "0.1", CVAR_ARCHIVE },
	{ &cg_freecam_yaw, "cg_freecam_yaw", "1.0", CVAR_ARCHIVE },
	{ &cg_freecam_pitch, "cg_freecam_pitch", "1.0", CVAR_ARCHIVE },
	{ &cg_freecam_speed, "cg_freecam_speed", "400", CVAR_ARCHIVE },
	{ &cg_freecam_crosshair, "cg_freecam_crosshair", "1", CVAR_ARCHIVE },
	{ &cg_freecam_useTeamSettings, "cg_freecam_useTeamSettings", "2", CVAR_ARCHIVE },
	{ &cg_freecam_rollValue, "cg_freecam_rollValue", "0.5", CVAR_ARCHIVE },
	{ &cg_freecam_useServerView, "cg_freecam_useServerView", "1", CVAR_ARCHIVE },
	{ &cg_freecam_unlockPitch, "cg_freecam_unlockPitch", "1", CVAR_ARCHIVE },

	{ &cg_chatTime, "cg_chatTime", "5000", CVAR_ARCHIVE },
	{ &cg_chatLines, "cg_chatLines", "10", CVAR_ARCHIVE },
	{ &cg_chatHistoryLength, "cg_chatHistoryLength", "15", CVAR_ARCHIVE },
	{ &cg_chatBeep, "cg_chatBeep", "1", CVAR_ARCHIVE },
	{ &cg_chatBeepMaxTime, "cg_chatBeepMaxTime", "0", CVAR_ARCHIVE },
	{ &cg_teamChatBeep, "cg_teamChatBeep", "1", CVAR_ARCHIVE },
	{ &cg_teamChatBeepMaxTime, "cg_teamChatBeepMaxTime", "0", CVAR_ARCHIVE },

	{ cvp(cg_serverPrint), "0", CVAR_ARCHIVE },
	{ cvp(cg_serverPrintToChat), "1", CVAR_ARCHIVE },
	{ cvp(cg_serverPrintToConsole), "1", CVAR_ARCHIVE },

	{ cvp(cg_serverCenterPrint), "1", CVAR_ARCHIVE },
	{ cvp(cg_serverCenterPrintToChat), "0", CVAR_ARCHIVE },
	{ cvp(cg_serverCenterPrintToConsole), "1", CVAR_ARCHIVE },

	{ &cg_drawCenterPrint, "cg_drawCenterPrint", "1", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintX, "cg_drawCenterPrintX", "320", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintY, "cg_drawCenterPrintY", "", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintAlign, "cg_drawCenterPrintAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintStyle, "cg_drawCenterPrintStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintFont, "cg_drawCenterPrintFont", "", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintPointSize, "cg_drawCenterPrintPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintScale, "cg_drawCenterPrintScale", "0.35", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintTime, "cg_drawCenterPrintTime", "3000", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintColor, "cg_drawCenterPrintColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintAlpha, "cg_drawCenterPrintAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintFade, "cg_drawCenterPrintFade", "1", CVAR_ARCHIVE },
	{ &cg_drawCenterPrintFadeTime, "cg_drawCenterPrintFadeTime", "200", CVAR_ARCHIVE },

	{ &cg_drawVote, "cg_drawVote", "1", CVAR_ARCHIVE },
	{ &cg_drawVoteX, "cg_drawVoteX", "0", CVAR_ARCHIVE },
	{ &cg_drawVoteY, "cg_drawVoteY", "300", CVAR_ARCHIVE },
	{ &cg_drawVoteAlign, "cg_drawVoteAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawVoteStyle, "cg_drawVoteStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawVoteFont, "cg_drawVoteFont", "", CVAR_ARCHIVE },
	{ &cg_drawVotePointSize, "cg_drawVotePointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawVoteScale, "cg_drawVoteScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawVoteColor, "cg_drawVoteColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawVoteAlpha, "cg_drawVoteAlpha", "255", CVAR_ARCHIVE },

	{ &cg_drawTeamVote, "cg_drawTeamVote", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteX, "cg_drawTeamVoteX", "0", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteY, "cg_drawTeamVoteY", "300", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteAlign, "cg_drawTeamVoteAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteStyle, "cg_drawTeamVoteStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteFont, "cg_drawTeamVoteFont", "", CVAR_ARCHIVE },
	{ &cg_drawTeamVotePointSize, "cg_drawTeamVotePointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteScale, "cg_drawTeamVoteScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteColor, "cg_drawTeamVoteColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawTeamVoteAlpha, "cg_drawTeamVoteAlpha", "255", CVAR_ARCHIVE },


	{ &cg_drawWaitingForPlayers, "cg_drawWaitingForPlayers", "1", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersX, "cg_drawWaitingForPlayersX", "320", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersY, "cg_drawWaitingForPlayersY", "60", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersAlign, "cg_drawWaitingForPlayersAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersStyle, "cg_drawWaitingForPlayersStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersFont, "cg_drawWaitingForPlayersFont", "", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersPointSize, "cg_drawWaitingForPlayersPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersScale, "cg_drawWaitingForPlayersScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersColor, "cg_drawWaitingForPlayersColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawWaitingForPlayersAlpha, "cg_drawWaitingForPlayersAlpha", "255", CVAR_ARCHIVE },


	{ &cg_drawWarmupString, "cg_drawWarmupString", "1", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringX, "cg_drawWarmupStringX", "320", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringY, "cg_drawWarmupStringY", "60", CVAR_ARCHIVE },  //FIXME 90?
	{ &cg_drawWarmupStringAlign, "cg_drawWarmupStringAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringStyle, "cg_drawWarmupStringStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringFont, "cg_drawWarmupStringFont", "", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringPointSize, "cg_drawWarmupStringPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringScale, "cg_drawWarmupStringScale", "0.6", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringColor, "cg_drawWarmupStringColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawWarmupStringAlpha, "cg_drawWarmupStringAlpha", "255", CVAR_ARCHIVE },

	{ &cg_ambientSounds, "cg_ambientSounds", "2", CVAR_ARCHIVE },
	{ &cg_weather, "cg_weather", "1", CVAR_ARCHIVE },
	{ &wolfcam_hoverTime, "wolfcam_hoverTime", "2000", CVAR_ARCHIVE },
	{ &wolfcam_switchMode, "wolfcam_switchMode", "0", CVAR_ARCHIVE },
	//{ &cg_fragForwardStyle, "cg_fragForwardStyle", "0", CVAR_ARCHIVE },
	{ &wolfcam_firstPersonSwitchSoundStyle, "wolfcam_firstPersonSwitchSoundStyle", "1", CVAR_ARCHIVE },
	{ &cg_interpolateMissiles, "cg_interpolateMissiles", "1", CVAR_ARCHIVE },

	{ &cg_wh, "cg_wh", "0", CVAR_ARCHIVE },
	{ cvp(cg_whIncludeDeadBody), "1", CVAR_ARCHIVE },
	{ cvp(cg_whIncludeProjectile), "1", CVAR_ARCHIVE },
	{ &cg_whShader, "cg_whShader", "", CVAR_ARCHIVE },
	{ &cg_playerShader, "cg_playerShader", "", CVAR_ARCHIVE },
	{ &wolfcam_fixedViewAngles, "wolfcam_fixedViewAngles", "0", CVAR_ARCHIVE },
	{ &cg_useOriginalInterpolation, "cg_useOriginalInterpolation", "0", CVAR_ARCHIVE },
	{ &cg_drawBBox, "cg_drawBBox", "0", CVAR_ARCHIVE },

	{ &cg_weaponRedTeamColor, "cg_weaponRedTeamColor", "0xf40000", CVAR_ARCHIVE },
	{ &cg_weaponBlueTeamColor, "cg_weaponBlueTeamColor", "0x3266f4", CVAR_ARCHIVE },
	// cg_noTeamColor

	{ &cg_hudRedTeamColor, "cg_hudRedTeamColor", "0xf40000", CVAR_ARCHIVE },
	{ &cg_hudBlueTeamColor, "cg_hudBlueTeamColor", "0x3266f4", CVAR_ARCHIVE },
	{ &cg_hudNoTeamColor, "cg_hudNoTeamColor", "0xf2db1f", CVAR_ARCHIVE },
	{ &cg_hudForceRedTeamClanTag, "cg_hudForceRedTeamClanTag", "", CVAR_ARCHIVE },
	{ &cg_hudForceBlueTeamClanTag, "cg_hudForceBlueTeamClanTag", "", CVAR_ARCHIVE },

	{ &cg_enemyModel, "cg_enemyModel", "keel/bright", CVAR_ARCHIVE },
	{ &cg_enemyHeadSkin, "cg_enemyHeadSkin", "", CVAR_ARCHIVE },
	{ &cg_enemyTorsoSkin, "cg_enemyTorsoSkin", "", CVAR_ARCHIVE },
	{ &cg_enemyLegsSkin, "cg_enemyLegsSkin", "", CVAR_ARCHIVE },
	{ &cg_enemyHeadColor, "cg_enemyHeadColor", "0x2a8000", CVAR_ARCHIVE },
	{ &cg_enemyTorsoColor, "cg_enemyTorsoColor", "0x2a8000", CVAR_ARCHIVE },
	{ &cg_enemyLegsColor, "cg_enemyLegsColor", "0x2a8000", CVAR_ARCHIVE },
	{ &cg_enemyRailColor1, "cg_enemyRailColor1", "", CVAR_ARCHIVE },
	{ &cg_enemyRailColor2, "cg_enemyRailColor2", "", CVAR_ARCHIVE },
	{ &cg_enemyRailColor1Team, "cg_enemyRailColor1Team", "0", CVAR_ARCHIVE },
	{ &cg_enemyRailColor2Team, "cg_enemyRailColor2Team", "0", CVAR_ARCHIVE },
	//	{ &cg_enemyOldRail, "cg_enemyOldRail", "1", CVAR_ARCHIVE },
	{ &cg_enemyRailRings, "cg_enemyRailRings", "0", CVAR_ARCHIVE },
	{ &cg_enemyRailNudge, "cg_enemyRailNudge", "1", CVAR_ARCHIVE },
	{ &cg_enemyRailItemColor, "cg_enemyRailItemColor", "", CVAR_ARCHIVE },
	{ &cg_enemyRailItemColorTeam, "cg_enemyRailItemColorTeam", "0", CVAR_ARCHIVE },
	{ cvp(cg_enemyFlagColor), "0x00ff00", CVAR_ARCHIVE },

	{ &cg_useDefaultTeamSkins, "cg_useDefaultTeamSkins", "1", CVAR_ARCHIVE },
	{ &cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE },
	{ &cg_teamHeadSkin, "cg_teamHeadSkin", "", CVAR_ARCHIVE },
	{ &cg_teamTorsoSkin, "cg_teamTorsoSkin", "", CVAR_ARCHIVE },
	{ &cg_teamLegsSkin, "cg_teamLegsSkin", "", CVAR_ARCHIVE },
	{ &cg_teamHeadColor, "cg_teamHeadColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_teamTorsoColor, "cg_teamTorsoColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_teamLegsColor, "cg_teamLegsColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_teamRailColor1, "cg_teamRailColor1", "", CVAR_ARCHIVE },
	{ &cg_teamRailColor2, "cg_teamRailColor2", "", CVAR_ARCHIVE },
	{ &cg_teamRailColor1Team, "cg_teamRailColor1Team", "0", CVAR_ARCHIVE },
	{ &cg_teamRailColor2Team, "cg_teamRailColor2Team", "0", CVAR_ARCHIVE },
	//{ &cg_teamOldRail, "cg_teamOldRail", "1", CVAR_ARCHIVE },
	{ &cg_teamRailRings, "cg_teamRailRings", "0", CVAR_ARCHIVE },
	{ &cg_teamRailNudge, "cg_teamRailNudge", "1", CVAR_ARCHIVE },
	{ &cg_teamRailItemColor, "cg_teamRailItemColor", "", CVAR_ARCHIVE },
	{ &cg_teamRailItemColorTeam, "cg_teamRailItemColorTeam", "0", CVAR_ARCHIVE },
	{ cvp(cg_teamFlagColor), "0xffffff", CVAR_ARCHIVE },

	{ cvp(cg_neutralFlagColor), "0xf6f600", CVAR_ARCHIVE },

	{ &cg_ourModel, "model", "", CVAR_ARCHIVE },
	//{ (vmCvar_t *)&cg_deadBodyColor, "cg_deadBodyColor", "16 16 16 255", CVAR_ARCHIVE },
	{ &cg_deadBodyColor, "cg_deadBodyColor", "0x101010", CVAR_ARCHIVE },
	{ cvp(cg_disallowEnemyModelForTeammates), "1", CVAR_ARCHIVE },

	{ &cg_audioAnnouncer, "cg_audioAnnouncer", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerRewards, "cg_audioAnnouncerRewards", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerRewardsFirst, "cg_audioAnnouncerRewardsFirst", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerRound, "cg_audioAnnouncerRound", "1", CVAR_ARCHIVE },
	{ cvp(cg_audioAnnouncerRoundReward), "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerWarmup, "cg_audioAnnouncerWarmup", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerVote, "cg_audioAnnouncerVote", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerTeamVote, "cg_audioAnnouncerTeamVote", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerFlagStatus, "cg_audioAnnouncerFlagStatus", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerLead, "cg_audioAnnouncerLead", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerTimeLimit, "cg_audioAnnouncerTimeLimit", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerFragLimit, "cg_audioAnnouncerFragLimit", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerWin, "cg_audioAnnouncerWin", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerScore, "cg_audioAnnouncerScore", "1", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerLastStanding, "cg_audioAnnouncerLastStanding", "0", CVAR_ARCHIVE },
	{ &cg_audioAnnouncerDominationPoint, "cg_audioAnnouncerDominationPoint", "1", CVAR_ARCHIVE },

	{ &wolfcam_drawFollowing, "wolfcam_drawFollowing", "2", CVAR_ARCHIVE },
	{ &wolfcam_drawFollowingOnlyName, "wolfcam_drawFollowingOnlyName", "0", CVAR_ARCHIVE },

	{ &cg_printTimeStamps, "cg_printTimeStamps", "1", CVAR_ARCHIVE },
	{ &cg_screenDamageAlpha_Team, "cg_screenDamageAlpha_Team", "200", CVAR_ARCHIVE },
	{ &cg_screenDamage_Team, "cg_screenDamage_Team", "0x700000", CVAR_ARCHIVE },
	{ &cg_screenDamageAlpha_Self, "cg_screenDamageAlpha_Self", "0", CVAR_ARCHIVE },
	{ &cg_screenDamage_Self, "cg_screenDamage_Self", "0x000000", CVAR_ARCHIVE },
	{ &cg_screenDamageAlpha, "cg_screenDamageAlpha", "200", CVAR_ARCHIVE },
	{ &cg_screenDamage, "cg_screenDamage", "0x700000", CVAR_ARCHIVE },
	{ &cg_echoPopupTime, "cg_echoPopupTime", "1000", CVAR_ARCHIVE },
	{ &cg_echoPopupX, "cg_echoPopupX", "30", CVAR_ARCHIVE },
	{ &cg_echoPopupY, "cg_echoPopupY", "340", CVAR_ARCHIVE },
	{ &cg_echoPopupScale, "cg_echoPopupScale", "0.3", CVAR_ARCHIVE },
	{ &cg_accX, "cg_accX", "450", CVAR_ARCHIVE },
	{ &cg_accY, "cg_accY", "100", CVAR_ARCHIVE },
	{ &cg_loadDefaultMenus, "cg_loadDefaultMenus", "1", CVAR_ARCHIVE },
	{ &cg_grenadeColor, "cg_grenadeColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_grenadeColorAlpha, "cg_grenadeColorAlpha", "255", CVAR_ARCHIVE },
	{ &cg_grenadeTeamColor, "cg_grenadeTeamColor", "0xffff00", CVAR_ARCHIVE },
	{ &cg_grenadeTeamColorAlpha, "cg_grenadeTeamColorAlpha", "255", CVAR_ARCHIVE },
	{ &cg_grenadeEnemyColor, "cg_grenadeEnemyColor", "0x00ff00", CVAR_ARCHIVE },
	{ &cg_grenadeEnemyColorAlpha, "cg_grenadeEnemyColorAlpha", "255", CVAR_ARCHIVE },
	{ &cg_fragMessageStyle, "cg_fragMessageStyle", "1", CVAR_ARCHIVE },
	//{ &cg_drawFragMessageSeparate, "cg_drawFragMessageSeparate", "0", CVAR_ARCHIVE },
	{ &cg_drawFragMessageSeparate, "cg_donka_rtcw_good_spellers", "0", CVAR_ARCHIVE },
	{ &cg_drawFragMessageSeparate, "cg_drawFragMessageSeparate", "0", CVAR_ARCHIVE },
	{ &cg_drawFragMessageX, "cg_drawFragMessageX", "320", CVAR_ARCHIVE },
	{ &cg_drawFragMessageY, "cg_drawFragMessageY", "120", CVAR_ARCHIVE },
	{ &cg_drawFragMessageAlign, "cg_drawFragMessageAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawFragMessageStyle, "cg_drawFragMessageStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawFragMessageFont, "cg_drawFragMessageFont", "", CVAR_ARCHIVE },
	{ &cg_drawFragMessagePointSize, "cg_drawFragMessagePointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawFragMessageScale, "cg_drawFragMessageScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawFragMessageTime, "cg_drawFragMessageTime", "3000", CVAR_ARCHIVE },
	{ &cg_drawFragMessageColor, "cg_drawFragMessageColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawFragMessageAlpha, "cg_drawFragMessageAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawFragMessageFade, "cg_drawFragMessageFade", "1", CVAR_ARCHIVE },
	{ &cg_drawFragMessageFadeTime, "cg_drawFragMessageFadeTime", "200", CVAR_ARCHIVE },
	{ &cg_drawFragMessageTokens, "cg_drawFragMessageTokens", "You fragged %v", CVAR_ARCHIVE },
	{ &cg_drawFragMessageTeamTokens, "cg_drawFragMessageTeamTokens", "You fragged %v, your teammate", CVAR_ARCHIVE },
	{ &cg_drawFragMessageThawTokens, "cg_drawFragMessageThawTokens", "You thawed %v", CVAR_ARCHIVE },
	{ &cg_drawFragMessageFreezeTokens, "cg_drawFragMessageFreezeTokens", "You froze %v", CVAR_ARCHIVE },
	{ &cg_drawFragMessageFreezeTeamTokens, "cg_drawFragMessageFreezeTeamTokens", "You froze %v, your teammate", CVAR_ARCHIVE },
	{ &cg_drawFragMessageIconScale, "cg_drawFragMessageIconScale", "1.5", CVAR_ARCHIVE },

	{ &cg_obituaryTokens, "cg_obituaryTokens", "%k %i %v", CVAR_ARCHIVE },
	{ &cg_obituaryIconScale, "cg_obituaryIconScale", "1.5", CVAR_ARCHIVE },
	{ &cg_obituaryRedTeamColor, "cg_obituaryRedTeamColor", "0xf40000", CVAR_ARCHIVE },
	{ &cg_obituaryBlueTeamColor, "cg_obituaryBlueTeamColor", "0x3266f4", CVAR_ARCHIVE },
	{ &cg_obituaryTime, "cg_obituaryTime", "3000", CVAR_ARCHIVE },
	{ &cg_obituaryFadeTime, "cg_obituaryFadeTime", "1000", CVAR_ARCHIVE },
	{ &cg_fragTokenAccuracyStyle, "cg_fragTokenAccuracyStyle", "0", CVAR_ARCHIVE },

	{ &cg_drawPlayerNames, "cg_drawPlayerNames", "0", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesY, "cg_drawPlayerNamesY", "64", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesStyle, "cg_drawPlayerNamesStyle", "3", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesFont, "cg_drawPlayerNamesFont", "", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesPointSize, "cg_drawPlayerNamesPointSize", "16", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesScale, "cg_drawPlayerNamesScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesColor, "cg_drawPlayerNamesColor", "", CVAR_ARCHIVE },
	{ &cg_drawPlayerNamesAlpha, "cg_drawPlayerNamesAlpha", "255", CVAR_ARCHIVE },

	{ &cg_perKillStatsExcludePostKillSpam, "cg_perKillStatsExcludePostKillSpam", "1", CVAR_ARCHIVE },
	{ &cg_perKillStatsClearNotFiringTime, "cg_perKillStatsClearNotFiringTime", "3000", CVAR_ARCHIVE },
	{ &cg_perKillStatsClearNotFiringExcludeSingleClickWeapons, "cg_perKillStatsClearNotFiringExcludeSingleClickWeapons", "1", CVAR_ARCHIVE },

	{ &cg_printSkillRating, "cg_printSkillRating", "0", CVAR_ARCHIVE },
	{ &cg_lightningImpact, "cg_lightningImpact", "1", CVAR_ARCHIVE },
	{ &cg_lightningImpactSize, "cg_lightningImpactSize", "1.0", CVAR_ARCHIVE },
	{ &cg_lightningImpactOthersSize, "cg_lightningImpactOthersSize", "0.0", CVAR_ARCHIVE },
	{ &cg_lightningImpactCap, "cg_lightningImpactCap", "192", CVAR_ARCHIVE },
	{ &cg_lightningImpactCapMin, "cg_lightningImpactCapMin", "60", CVAR_ARCHIVE },
	{ &cg_lightningImpactProject, "cg_lightningImpactProject", "1", CVAR_ARCHIVE },
	{ &cg_lightningStyle, "cg_lightningStyle", "1", CVAR_ARCHIVE },
	{ &cg_lightningRenderStyle, "cg_lightningRenderStyle", "1", CVAR_ARCHIVE },
	{ &cg_lightningAngleOriginStyle, "cg_lightningAngleOriginStyle", "1", CVAR_ARCHIVE },
	{ &cg_debugLightningImpactDistance, "cg_debugLightningImpactDistance", "0", CVAR_ARCHIVE },
	{ &cg_lightningSize, "cg_lightningSize", "8", CVAR_ARCHIVE },
	{ &cg_drawEntNumbers, "cg_drawEntNumbers", "0", CVAR_ARCHIVE },
	{ &cg_drawEventNumbers, "cg_drawEventNumbers", "0", CVAR_ARCHIVE },
	{ &cg_demoSmoothing, "cg_demoSmoothing", "1", CVAR_ARCHIVE },
	{ cvp(cg_demoSmoothingAngles), "1", CVAR_ARCHIVE },
	{ cvp(cg_demoSmoothingTeleportCheck), "1", CVAR_ARCHIVE },
	{ &cg_drawCameraPath, "cg_drawCameraPath", "1", CVAR_ARCHIVE },

	{ &cg_drawCameraPointInfo, "cg_drawCameraPointInfo", "1", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoX, "cg_drawCameraPointInfoX", "60", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoY, "cg_drawCameraPointInfoY", "60", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoAlign, "cg_drawCameraPointInfoAlign", "0", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoStyle, "cg_drawCameraPointInfoStyle", "0", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoFont, "cg_drawCameraPointInfoFont", "", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoPointSize, "cg_drawCameraPointInfoPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoScale, "cg_drawCameraPointInfoScale", "0.25", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoColor, "cg_drawCameraPointInfoColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoSelectedColor, "cg_drawCameraPointInfoSelectedColor", "0xff5a5a", CVAR_ARCHIVE },
	{ &cg_drawCameraPointInfoAlpha, "cg_drawCameraPointInfoAlpha", "255", CVAR_ARCHIVE },

	{ &cg_drawViewPointMark, "cg_drawViewPointMark", "0", CVAR_ARCHIVE },
	{ &cg_levelTimerDirection, "cg_levelTimerDirection", "0", CVAR_ARCHIVE },
	//{ &cg_levelTimerStyle, "cg_levelTimerStyle", "0", CVAR_ARCHIVE },
	{ &cg_levelTimerDefaultTimeLimit, "cg_levelTimerDefaultTimeLimit", "60", CVAR_ARCHIVE },
	{ &cg_checkForOfflineDemo, "cg_checkForOfflineDemo", "1", CVAR_ARCHIVE },
	{ &cg_muzzleFlash, "cg_muzzleFlash", "1", CVAR_ARCHIVE },

	{ &cg_weaponDefault, "cg_weaponDefault", "", CVAR_ARCHIVE },
	{ &cg_weaponNone, "cg_weaponNone", "", CVAR_ARCHIVE },
	{ &cg_weaponGauntlet, "cg_weaponGauntlet", "", CVAR_ARCHIVE },
	{ &cg_weaponShotgun, "cg_weaponShotgun", "", CVAR_ARCHIVE },
	{ &cg_weaponMachineGun, "cg_weaponMachineGun", "", CVAR_ARCHIVE },
	{ &cg_weaponShotgun, "cg_weaponShotgun", "", CVAR_ARCHIVE },
	{ &cg_weaponGrenadeLauncher, "cg_weaponGrenadeLauncher", "", CVAR_ARCHIVE },
	{ &cg_weaponRocketLauncher, "cg_weaponRocketLauncher", "", CVAR_ARCHIVE },
	{ &cg_weaponLightningGun, "cg_weaponLightningGun", "", CVAR_ARCHIVE },
	{ &cg_weaponRailGun, "cg_weaponRailGun", "", CVAR_ARCHIVE },
	{ &cg_weaponPlasmaGun, "cg_weaponPlasmaGun", "", CVAR_ARCHIVE },
	{ &cg_weaponBFG, "cg_weaponBFG", "", CVAR_ARCHIVE },
	{ &cg_weaponGrapplingHook, "cg_weaponGrapplingHook", "", CVAR_ARCHIVE },
	{ &cg_weaponNailGun, "cg_weaponNailGun", "", CVAR_ARCHIVE },
	{ &cg_weaponProximityLauncher, "cg_weaponProximityLauncher", "", CVAR_ARCHIVE },
	{ &cg_weaponChainGun, "cg_weaponChainGun", "", CVAR_ARCHIVE },

	{ &cg_spawnArmorTime, "cg_spawnArmorTime", "500", CVAR_ARCHIVE },
	{ &cg_fxfile, "cg_fxfile", "", CVAR_ARCHIVE },
	{ &cg_fxinterval, "cg_fxinterval", "25", CVAR_ARCHIVE },
	{ &cg_fxratio, "cg_fxratio", "0.002", CVAR_ARCHIVE },
	{ &cg_fxScriptMinEmitter, "cg_fxScriptMinEmitter", "0.0", CVAR_ARCHIVE },
	{ &cg_fxScriptMinDistance, "cg_fxScriptMinDistance", "0.001", CVAR_ARCHIVE },
	{ &cg_fxScriptMinInterval, "cg_fxScriptMinInterval", "0.001", CVAR_ARCHIVE },
	{ &cg_fxLightningGunImpactFps, "cg_fxLightningGunImpactFps", "125", CVAR_ARCHIVE },
	{ cvp(cg_fxDebugEntities), "0", CVAR_ARCHIVE },
	{ cvp(cg_fxCompiled), "1", CVAR_ARCHIVE },
#ifdef ENABLE_THREADS
	{ cvp(cg_fxThreads), "1", CVAR_ARCHIVE },
#endif
	{ &cg_vibrate, "cg_vibrate", "0", CVAR_ARCHIVE },
	{ &cg_vibrateTime, "cg_vibrateTime", "150.0", CVAR_ARCHIVE },
	{ &cg_vibrateMaxDistance, "cg_vibrateMaxDistance", "800", CVAR_ARCHIVE },
	{ &cg_vibrateForce, "cg_vibrateForce", "1.0", CVAR_ARCHIVE },

	{ &cg_animationsIgnoreTimescale, "cg_animationsIgnoreTimescale", "0", CVAR_ARCHIVE },
	{ &cg_animationsRate, "cg_animationsRate", "1", CVAR_ARCHIVE },
	{ &cg_quadFireSound, "cg_quadFireSound", "1", CVAR_ARCHIVE },
	{ &cg_kickScale, "cg_kickScale", "1.0", CVAR_ARCHIVE },
	{ &cg_gameType, "cg_gameType", "0", CVAR_ARCHIVE },
	{ &cg_compMode, "cg_compMode", "0", CVAR_ARCHIVE },
	{ &cg_drawSpecMessages, "cg_drawSpecMessages", "1", CVAR_ARCHIVE },
	{ &g_training, "g_training", "0", CVAR_ARCHIVE },
	{ &cg_waterWarp, "cg_waterWarp", "1", CVAR_ARCHIVE },
	{ &cg_allowLargeSprites, "cg_allowLargeSprites", "0", CVAR_ARCHIVE },
	{ &cg_allowSpritePassThrough, "cg_allowSpritePassThrough", "1", CVAR_ARCHIVE },
	{ &cg_drawSprites, "cg_drawSprites", "1", CVAR_ARCHIVE },
	{ &cg_drawSpriteSelf, "cg_drawSpriteSelf", "0", CVAR_ARCHIVE },
	{ &cg_playerLeanScale, "cg_playerLeanScale", "1.0", CVAR_ARCHIVE },
	{ &cg_cameraRewindTime, "cg_cameraRewindTime", "0", CVAR_ARCHIVE },
	{ &cg_cameraQue, "cg_cameraQue", "1", CVAR_ARCHIVE },
	{ &cg_cameraUpdateFreeCam, "cg_cameraUpdateFreeCam", "1", CVAR_ARCHIVE },
	{ &cg_cameraAddUsePreviousValues, "cg_cameraAddUsePreviousValues", "1", CVAR_ARCHIVE },
	{ &cg_cameraDefaultOriginType, "cg_cameraDefaultOriginType", "curve", CVAR_ARCHIVE },
	{ &cg_cameraDebugPath, "cg_cameraDebugPath", "0", CVAR_ARCHIVE },
	{ &cg_cameraSmoothFactor, "cg_cameraSmoothFactor", "1.5", CVAR_ARCHIVE },
	{ &cg_flightTrail, "cg_flightTrail", "1", CVAR_ARCHIVE },
	{ &cg_hasteTrail, "cg_hasteTrail", "1", CVAR_ARCHIVE },
	{ &cg_noItemUseMessage, "cg_noItemUseMessage", "1", CVAR_ARCHIVE },
	{ &cg_noItemUseSound, "cg_noItemUseSound", "1", CVAR_ARCHIVE },
	{ &cg_itemUseMessage, "cg_itemUseMessage", "1", CVAR_ARCHIVE },
	{ &cg_itemUseSound, "cg_itemUseSound", "1", CVAR_ARCHIVE },
	{ &cg_localTime, "cg_localTime", "0", CVAR_ARCHIVE },
	{ &cg_localTimeStyle, "cg_localTimeStyle", "1", CVAR_ARCHIVE },
	{ &cg_warmupTime, "cg_warmupTime", "1", CVAR_ARCHIVE },
	{ &cg_clientOverrideIgnoreTeamSettings, "cg_clientOverrideIgnoreTeamSettings", "1", CVAR_ARCHIVE },
	{ &cg_killBeep, "cg_killBeep", "1", CVAR_ARCHIVE },
	{ &cg_deathStyle, "cg_deathStyle", "1", CVAR_ARCHIVE },
	{ &cg_deathShowOwnCorpse, "cg_deathShowOwnCorpse", "1", CVAR_ARCHIVE },
	{ &cg_mouseSeekScale, "cg_mouseSeekScale", "1.0", CVAR_ARCHIVE },
	{ &cg_mouseSeekPollInterval, "cg_mouseSeekPollInterval", "50", CVAR_ARCHIVE },
	{ &cg_mouseSeekUseTimescale, "cg_mouseSeekUseTimescale", "2", CVAR_ARCHIVE },
	{ &cg_teamKillWarning, "cg_teamKillWarning", "1", CVAR_ARCHIVE },
	{ &cg_inheritPowerupShader, "cg_inheritPowerupShader", "0", CVAR_ARCHIVE },
	{ &cg_fadeColor, "cg_fadeColor", "0x000000", CVAR_ARCHIVE },
	{ &cg_fadeAlpha, "cg_fadeAlpha", "0", CVAR_ARCHIVE },
	{ &cg_fadeStyle, "cg_fadeStyle", "0", CVAR_ARCHIVE },
	{ &cg_enableAtCommands, "cg_enableAtCommands", "1", CVAR_ARCHIVE },
	{ &cg_quadKillCounter, "cg_quadKillCounter", "1", CVAR_ARCHIVE },
	{ &cg_battleSuitKillCounter, "cg_battleSuitKillCounter", "1", CVAR_ARCHIVE },
	{ &cg_wideScreen, "cg_wideScreen", "0", CVAR_ARCHIVE },
	{ &cg_wideScreenScoreBoardHack, "cg_wideScreenScoreBoardHack", "0", CVAR_ARCHIVE },
	{ &cg_adShaderOverride, "cg_adShaderOverride", "0", CVAR_ARCHIVE },
	{ &cg_firstPersonSwitchSound, "cg_firstPersonSwitchSound", "sound/wc/beep05", CVAR_ARCHIVE },
	{ &cg_proxMineTick, "cg_proxMineTick", "1", CVAR_ARCHIVE },
	{ &cg_drawProxWarning, "cg_drawProxWarning", "1", CVAR_ARCHIVE },
	{ &cg_drawProxWarningX, "cg_drawProxWarningX", "320", CVAR_ARCHIVE },
	{ &cg_drawProxWarningY, "cg_drawProxWarningY", "80", CVAR_ARCHIVE },
	{ &cg_drawProxWarningAlign, "cg_drawProxWarningAlign", "1", CVAR_ARCHIVE },
	{ &cg_drawProxWarningStyle, "cg_drawProxWarningStyle", "6", CVAR_ARCHIVE },
	{ &cg_drawProxWarningFont, "cg_drawProxWarningFont", "q3big", CVAR_ARCHIVE },
	{ &cg_drawProxWarningPointSize, "cg_drawProxWarningPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawProxWarningScale, "cg_drawProxWarningScale", "0.4", CVAR_ARCHIVE },
	{ &cg_drawProxWarningColor, "cg_drawProxWarningColor", "0xfe0000", CVAR_ARCHIVE },
	{ &cg_drawProxWarningAlpha, "cg_drawProxWarningAlpha", "255", CVAR_ARCHIVE },
	{ &cg_customMirrorSurfaces, "cg_customMirrorSurfaces", "0", CVAR_ARCHIVE },
	{ &cg_demoStepSmoothing, "cg_demoStepSmoothing", "1", CVAR_ARCHIVE },
	{ &cg_stepSmoothTime, "cg_stepSmoothTime", "100", CVAR_ARCHIVE },
	{ &cg_stepSmoothMaxChange, "cg_stepSmoothMaxChange", "32", CVAR_ARCHIVE },
	{ &cg_pathRewindTime, "cg_pathRewindTime", "0", CVAR_ARCHIVE },
	{ &cg_pathSkipNum, "cg_pathSkipNum", "0", CVAR_ARCHIVE },
	{ &cg_dumpEntsUseServerTime, "cg_dumpEntsUseServerTime", "0", CVAR_ARCHIVE },
	{ &cg_playerModelForceScale, "cg_playerModelForceScale", "", CVAR_ARCHIVE },
	{ &cg_playerModelForceLegsScale, "cg_playerModelForceLegsScale", "", CVAR_ARCHIVE },
	{ &cg_playerModelForceTorsoScale, "cg_playerModelForceTorsoScale", "", CVAR_ARCHIVE },
	{ &cg_playerModelForceHeadScale, "cg_playerModelForceHeadScale", "", CVAR_ARCHIVE },
	{ &cg_playerModelForceHeadOffset, "cg_playerModelForceHeadOffset", "", CVAR_ARCHIVE },
	{ &cg_playerModelAutoScaleHeight, "cg_playerModelAutoScaleHeight", "57", CVAR_ARCHIVE },
	{ &cg_playerModelAllowServerScale, "cg_playerModelAllowServerScale", "1", CVAR_ARCHIVE },
	{ &cg_playerModelAllowServerScaleDefault, "cg_playerModelAllowServerScaleDefault", "1.1", CVAR_ARCHIVE },
	{ &cg_playerModelAllowServerOverride, "cg_playerModelAllowServerOverride", "1", CVAR_ARCHIVE },
	{ cvp(cg_allowServerOverride), "1", CVAR_ARCHIVE },
	{ &cg_powerupLight, "cg_powerupLight", "1", CVAR_ARCHIVE },
	{ &cg_buzzerSound, "cg_buzzerSound", "1", CVAR_ARCHIVE },
	{ &cg_flagStyle, "cg_flagStyle", "1", CVAR_ARCHIVE },
	{ &cg_helpIconStyle, "cg_helpIconStyle", "3", CVAR_ARCHIVE },
	{ &cg_helpIconMinWidth, "cg_helpIconMinWidth", "16.0", CVAR_ARCHIVE },
	{ &cg_helpIconMaxWidth, "cg_helpIconMaxWidth", "32.0", CVAR_ARCHIVE },
	//{ &cg_helpIconAlpha, "cg_helpIconAlpha", "255", CVAR_ARCHIVE },
	{ &cg_dominationPointTeamColor, "cg_dominationPointTeamColor", "", CVAR_ARCHIVE },
	{ &cg_dominationPointTeamAlpha, "cg_dominationPointTeamAlpha", "", CVAR_ARCHIVE },
	{ &cg_dominationPointEnemyColor, "cg_dominationPointEnemyColor", "", CVAR_ARCHIVE },
	{ &cg_dominationPointEnemyAlpha, "cg_dominationPointEnemyAlpha", "", CVAR_ARCHIVE },
	{ &cg_dominationPointNeutralColor, "cg_dominationPointNeutralColor", "", CVAR_ARCHIVE },
	{ &cg_dominationPointNeutralAlpha, "cg_dominationPointNeutralAlpha", "", CVAR_ARCHIVE },
	{ &cg_attackDefendVoiceStyle, "cg_attackDefendVoiceStyle", "1", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatus, "cg_drawDominationPointStatus", "1", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusX, "cg_drawDominationPointStatusX", "258", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusY, "cg_drawDominationPointStatusY", "365", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusFont, "cg_drawDominationPointStatusFont", "", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusPointSize, "cg_drawDominationPointStatusPointSize", "24", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusScale, "cg_drawDominationPointStatusScale", "1.0", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusEnemyColor, "cg_drawDominationPointStatusEnemyColor", "", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusTeamColor, "cg_drawDominationPointStatusTeamColor", "", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusBackgroundColor, "cg_drawDominationPointStatusBackgroundColor", "0x000000", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusAlpha, "cg_drawDominationPointStatusAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusTextColor, "cg_drawDominationPointStatusTextColor", "0xffffff", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusTextAlpha, "cg_drawDominationPointStatusTextAlpha", "255", CVAR_ARCHIVE },
	{ &cg_drawDominationPointStatusTextStyle, "cg_drawDominationPointStatusTextStyle", "3", CVAR_ARCHIVE },

	{ &cg_roundScoreBoard, "cg_roundScoreBoard", "1", CVAR_ARCHIVE },
	{ &cg_headShots, "cg_headShots", "1", CVAR_ARCHIVE },
	{ cvp(cg_spectatorListSkillRating), "1", CVAR_ARCHIVE },
	{ cvp(cg_spectatorListScore), "1", CVAR_ARCHIVE },
	{ cvp(cg_spectatorListQue), "1", CVAR_ARCHIVE },

	{ cvp(cg_rocketAimBot), "0", CVAR_ARCHIVE },
	{ cvp(cg_drawTieredArmorAvailability), "1", CVAR_ARCHIVE },

#if 0
	{ &cgr_gauntletHave, "cgr_gauntletHave", "0", CVAR_ROM },
	{ &cgr_machineGunHave, "cgr_machineGunHave", "0", CVAR_ROM },
	{ &cgr_shotgunHave, "cgr_shotgunHave", "0", CVAR_ROM },
	{ &cgr_grenadeLauncherHave, "cgr_grenadeLauncherHave", "0", CVAR_ROM },
	{ &cgr_rocketLauncherHave, "cgr_rocketLauncherHave", "0", CVAR_ROM },
	{ &cgr_lightningGunHave, "cgr_lightningGunHave", "0", CVAR_ROM },
	{ &cgr_railGunHave, "cgr_railGunHave", "0", CVAR_ROM },
	{ &cgr_plasmaGunHave, "cgr_plasmaGunHave", "0", CVAR_ROM },
	{ &cgr_bfgHave, "cgr_bfgHave", "0", CVAR_ROM },
	{ &cgr_grappleHave, "cgr_grappleHave", "0", CVAR_ROM },
	{ &cgr_nailGunHave, "cgr_nailGunHave", "0", CVAR_ROM },
	{ &cgr_proximityLauncherHave, "cgr_proximityLauncherHave", "0", CVAR_ROM },
	{ &cgr_chainGunHave, "cgr_chainGunHave", "0", CVAR_ROM },

	{ &cgr_gauntletAmmo, "cgr_gauntletAmmo", "0", CVAR_ROM },
	{ &cgr_machineGunAmmo, "cgr_machineGunAmmo", "0", CVAR_ROM },
	{ &cgr_shotgunAmmo, "cgr_shotgunAmmo", "0", CVAR_ROM },
	{ &cgr_grenadeLauncherAmmo, "cgr_grenadeLauncherAmmo", "0", CVAR_ROM },
	{ &cgr_rocketLauncherAmmo, "cgr_rocketLauncherAmmo", "0", CVAR_ROM },
	{ &cgr_lightningGunAmmo, "cgr_lightningGunAmmo", "0", CVAR_ROM },
	{ &cgr_railGunAmmo, "cgr_railGunAmmo", "0", CVAR_ROM },
	{ &cgr_plasmaGunAmmo, "cgr_plasmaGunAmmo", "0", CVAR_ROM },
	{ &cgr_bfgAmmo, "cgr_bfgAmmo", "0", CVAR_ROM },
	{ &cgr_grappleAmmo, "cgr_grappleAmmo", "0", CVAR_ROM },
	{ &cgr_nailGunAmmo, "cgr_nailGunAmmo", "0", CVAR_ROM },
	{ &cgr_proximityLauncherAmmo, "cgr_proximityLauncherAmmo", "0", CVAR_ROM },
	{ &cgr_chainGunAmmo, "cgr_chainGunAmmo", "0", CVAR_ROM },

	{ &cgr_selectedWeapon, "cgr_selectedWeapon", "0", CVAR_ROM },
#endif

	{ cvp(cg_drawDeadFriendTime), "3000", CVAR_ARCHIVE },
	{ cvp(cg_drawHitFriendTime), "1500", CVAR_ARCHIVE },
	{ cvp(cg_racePlayerShader), "1", CVAR_ARCHIVE },
};

#undef cvp

// end cvar_t table

static int  cvarTableSize = ARRAY_LEN(cvarTable);

/*
=================
CG_RegisterCvars
=================
*/
static void CG_RegisterCvars( void ) {
	int			i;
	const cvarTable_t *cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}

static void CG_CreateNameSprites (void)
{
	int i;
	const fontInfo_t *font;
	const char *name;

	if (*cg_drawPlayerNamesFont.string) {
		font = &cgs.media.playerNamesFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}

		if (*cgs.clientinfo[i].clanTag) {
			name = va("%s ^7%s", cgs.clientinfo[i].clanTag, cgs.clientinfo[i].name);
		} else {
			name = cgs.clientinfo[i].name;
		}
		CG_CreateNameSprite(0, 0, 1.0, colorWhite, name, 0, 0, 0, font, cgs.clientNameImage[i], 48 * MAX_QPATH, 48);
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	const cvarTable_t *cv;
	int cvarscoreboardold = -1;
	int ambientSoundsOld;  // = -1;

	cvarscoreboardold = cg_scoreBoardOld.integer;
	ambientSoundsOld = cg_ambientSounds.integer;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}

	if (cvarscoreboardold != cg_scoreBoardOld.integer) {
		cg.menuScoreboard = NULL;
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
		// FIXME E3 HACK
		trap_Cvar_Set( "teamoverlay", "1" );
	}

	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}

	// such bullshit
	//FIXME just fucking change it
	if (cgs.media.playerNamesStyleModificationCount != cg_drawPlayerNamesStyle.modificationCount) {
		CG_CreateNameSprites();
		cgs.media.playerNamesStyleModificationCount = cg_drawPlayerNamesStyle.modificationCount;
	} else if (cgs.media.playerNamesPointSizeModificationCount != cg_drawPlayerNamesPointSize.modificationCount) {
		CG_CreateNameSprites();
		cgs.media.playerNamesPointSizeModificationCount = cg_drawPlayerNamesPointSize.modificationCount;
	} else if (cgs.media.playerNamesColorModificationCount != cg_drawPlayerNamesColor.modificationCount) {
		CG_CreateNameSprites();
		cgs.media.playerNamesColorModificationCount = cg_drawPlayerNamesColor.modificationCount;
	} else if (cgs.media.playerNamesAlphaModificationCount != cg_drawPlayerNamesAlpha.modificationCount) {
		CG_CreateNameSprites();
		cgs.media.playerNamesAlphaModificationCount = cg_drawPlayerNamesAlpha.modificationCount;
	}

	//FIXME map restart
	if (cg_ambientSounds.integer != ambientSoundsOld) {
		switch (cg_ambientSounds.integer) {
		case 1:
			break;
		default:
			// half are reserverd for fx, only change map ones
			for (i = 0;  i < (MAX_LOOP_SOUNDS / 2);  i++) {
				trap_S_StopLoopingSound(i);
			}
			break;
		}
	}
}

static qboolean CG_CheckIndividualFontUpdate (fontInfo_t *font, int pointSize, const vmCvar_t *cv, int *modCount)
{
	const char *s;

	if (*modCount == cv->modificationCount) {
		return qfalse;
	}

	if (*modCount != cv->modificationCount) {
		s = cv->string;
		if (!*s) {
			//memcpy(font, &cgDC.Assets.textFont, sizeof(cgDC.Assets.textFont));
			memcpy(font, &cgs.media.qlfont24, sizeof(cgs.media.qlfont24));
			//Com_Printf("using default font for %d\n", cv->handle);
		} else {
			//FIXME testing
			if (!Q_stricmp(s, "smallchar")) {
				//FIXME
			}
			trap_R_RegisterFont(s, pointSize, font);
			if (!font->name[0]) {
				Com_Printf("^1failed to load font %s for %d\n", s, cv->handle);
				//memcpy(font, &cgDC.Assets.textFont, sizeof(cgDC.Assets.textFont));
				memcpy(font, &cgs.media.qlfont24, sizeof(cgs.media.qlfont24));
			} else {
				//Com_Printf("loaded font %s for %d\n", s, cv->handle);
			}
		}
		*modCount = cv->modificationCount;
	}

	return qtrue;
}

void CG_CheckFontUpdates (void)
{
	//int i;

	CG_CheckIndividualFontUpdate(&cgs.media.fragMessageFont, cg_drawFragMessagePointSize.integer, &cg_drawFragMessageFont, &cgs.media.fragMessageFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.centerPrintFont, cg_drawCenterPrintPointSize.integer, &cg_drawCenterPrintFont, &cgs.media.centerPrintFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.rewardsFont, cg_drawRewardsPointSize.integer, &cg_drawRewardsFont, &cgs.media.rewardsFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.fpsFont, cg_drawFPSPointSize.integer, &cg_drawFPSFont, &cgs.media.fpsFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.clientItemTimerFont, cg_drawClientItemTimerPointSize.integer, &cg_drawClientItemTimerFont, &cgs.media.clientItemTimerFontModificationCount);
	if (CG_CheckIndividualFontUpdate(&cgs.media.playerNamesFont, cg_drawPlayerNamesPointSize.integer, &cg_drawPlayerNamesFont, &cgs.media.playerNamesFontModificationCount)) {
		CG_CreateNameSprites();
	}

	CG_CheckIndividualFontUpdate(&cgs.media.snapshotFont, cg_drawSnapshotPointSize.integer, &cg_drawSnapshotFont, &cgs.media.snapshotFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.ammoWarningFont, cg_drawAmmoWarningPointSize.integer, &cg_drawAmmoWarningFont, &cgs.media.ammoWarningFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.crosshairNamesFont, cg_drawCrosshairNamesPointSize.integer, &cg_drawCrosshairNamesFont, &cgs.media.crosshairNamesFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.crosshairTeammateHealthFont, cg_drawCrosshairTeammateHealthPointSize.integer, &cg_drawCrosshairTeammateHealthFont, &cgs.media.crosshairTeammateHealthFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.warmupStringFont, cg_drawWarmupStringPointSize.integer, &cg_drawWarmupStringFont, &cgs.media.warmupStringFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.waitingForPlayersFont, cg_drawWaitingForPlayersPointSize.integer, &cg_drawWaitingForPlayersFont, &cgs.media.waitingForPlayersFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.voteFont, cg_drawVotePointSize.integer, &cg_drawVoteFont, &cgs.media.voteFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.teamVoteFont, cg_drawTeamVotePointSize.integer, &cg_drawTeamVoteFont, &cgs.media.teamVoteFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.followingFont, cg_drawFollowingPointSize.integer, &cg_drawFollowingFont, &cgs.media.followingFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.weaponBarFont, cg_weaponBarPointSize.integer, &cg_weaponBarFont, &cgs.media.weaponBarFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.itemPickupsFont, cg_drawItemPickupsPointSize.integer, &cg_drawItemPickupsFont, &cgs.media.itemPickupsFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.originFont, cg_drawOriginPointSize.integer, &cg_drawOriginFont, &cgs.media.originFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.speedFont, cg_drawSpeedPointSize.integer, &cg_drawSpeedFont, &cgs.media.speedFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.lagometerFont, cg_lagometerFontPointSize.integer, &cg_lagometerFont, &cgs.media.lagometerFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.attackerFont, cg_drawAttackerPointSize.integer, &cg_drawAttackerFont, &cgs.media.attackerFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.teamOverlayFont, cg_drawTeamOverlayPointSize.integer, &cg_drawTeamOverlayFont, &cgs.media.teamOverlayFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.cameraPointInfoFont, cg_drawCameraPointInfoPointSize.integer, &cg_drawCameraPointInfoFont, &cgs.media.cameraPointInfoFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.jumpSpeedsFont, cg_drawJumpSpeedsPointSize.integer, &cg_drawJumpSpeedsFont, &cgs.media.jumpSpeedsFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.jumpSpeedsTimeFont, cg_drawJumpSpeedsTimePointSize.integer, &cg_drawJumpSpeedsTimeFont, &cgs.media.jumpSpeedsTimeFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.proxWarningFont, cg_drawProxWarningPointSize.integer, &cg_drawProxWarningFont, &cgs.media.proxWarningFontModificationCount);
	CG_CheckIndividualFontUpdate(&cgs.media.dominationPointStatusFont, cg_drawDominationPointStatusPointSize.integer, &cg_drawDominationPointStatusFont, &cgs.media.dominationPointStatusFontModificationCount);
}

static void CG_TimeChange (int serverTime, int ioverf)
{

	//Com_Printf("CG_TimeChange %d %d\n", serverTime, ioverf);

	if (!cg.snap) {
		// seeking while still loading cgame
		//Com_Printf("CG_TimeChange() cgame not initialized yet, returning\n");
		//return;
	}

	if (SC_Cvar_Get_Int("debug_seek")) {
		if (cg.snap) {
			Com_Printf("cgame demo seeking cg.time: %f  serverTime: %d\n", cg.ftime, cg.snap->serverTime);
		}
	}

	CG_ResetTimeChange(serverTime, ioverf);

	if (cg.cameraWaitToSync) {
		//cg.cameraWaitToSync = qfalse;
		//Com_Printf("camera can begin.  our time %f  wanted time[current cam point %d]  %f\n", (double)serverTime + (double)((double)ioverf / SUBTIME_RESOLUTION), cg.currentCameraPoint, cg.cameraPoints[cg.currentCameraPoint].cgtime);
	} else if (cg.cameraPlaying) {
		// we have already synced, so this rewind/fastforward will screw things up ... not anymore don't think
		//cg.cameraPlaying = qfalse;
	}

	//FIXME bad hack
	cg.atCameraPoint = qfalse;
}

//FIXME only used once in cg_servercmds.c
int CG_CrosshairPlayer( void ) {
	//FIXME 1000 fixed number
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return CROSSHAIR_CLIENT_INVALID;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void CG_AddChatLine (const char *line)
{
	int i;

	i = cg.chatAreaStringsIndex % MAX_CHAT_LINES_MASK;

	Q_strncpyz(cg.chatAreaStrings[i], line, sizeof(cg.chatAreaStrings[i]));
	cg.chatAreaStringsTime[i] = cg.time;
	cg.chatAreaStringsIndex++;
}

void CG_PrintToScreen ( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	int i;

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	text[1023] = '\0';

	for (i = 0;  i < 1024;  i++) {
		if (text[i] >= ' '  &&  text[i] <= '~') {
			continue;
		}

		if (text[i] == '\0') {
			break;
		}

		text[i] = ' ';
	}

	CG_AddChatLine(text);
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[MAX_PRINT_MSG];
	const char *gameStatusStr;

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg.warmup) {
		gameStatusStr = "^3(warmup) ^7";
	} else if (cg.snap  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
		gameStatusStr = "^3(intermission) ^7";
	} else {
		gameStatusStr = "";
	}

	if (cg_printTimeStamps.integer == 2) {
		trap_Print(va("%d %s", cg.time, text));
	} else if (cg_printTimeStamps.integer == 1) {
		int mins, seconds, tens;
		int msec;

		if (CG_GameTimeout()) {
			msec = cgs.timeoutBeginTime - cgs.levelStartTime;
        } else {
			msec = cg.time - cgs.levelStartTime;
        }

		if (cg.snap) {
			msec = CG_GetCurrentTimeWithDirection();
		} else {
			msec = 0;
		}

        seconds = msec / 1000;
        mins = seconds / 60;
        seconds -= mins * 60;
        tens = seconds / 10;
        seconds -= tens * 10;

		trap_Print(va("%s%d:%d%d %s", gameStatusStr, mins, tens, seconds, text));
	} else if (cg_printTimeStamps.integer == 3) {
		int mins, seconds, tens;
		int msec;

		if (CG_GameTimeout()) {
			msec = cgs.timeoutBeginTime - cgs.levelStartTime;
        } else {
			msec = cg.time - cgs.levelStartTime;
        }

		if (cg.snap) {
			msec = CG_GetCurrentTimeWithDirection();
		} else {
			msec = 0;
		}

        seconds = msec / 1000;
        mins = seconds / 60;
        seconds -= mins * 60;
        tens = seconds / 10;
        seconds -= tens * 10;

		trap_Print(va("%d %d %d:%d%d %s", cg.time, cg.time - cgs.levelStartTime, mins, tens, seconds, text));
	} else {
		trap_Print(text);
	}
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	//CG_Printf ("%s", text);
	trap_Print(text);
}

#endif

/*
================
CG_Argv
================
*/

const char *CG_Argv (int arg)
{
	static char	buffer[MAX_STRING_CHARS];

	if (cg.useFakeArgs) {
		if (arg > cg.fakeArgc) {
			return "";
		} else {
			return cg.fakeArgs[arg];
		}
	} else {
		trap_Argv(arg, buffer, sizeof(buffer));
		return buffer;
	}
}

int CG_Argc (void)
{
	if (cg.useFakeArgs) {
		return cg.fakeArgc;
	} else {
		return trap_Argc();
	}
}

const char *CG_ArgsFrom (int arg)
{
	static char buffer[MAX_STRING_CHARS];
	int i;

	buffer[0] = '\0';

	if (arg >= CG_Argc()) {
		return buffer;
	}

	for (i = arg;  i < CG_Argc();  i++) {
		Q_strcat(buffer, sizeof(buffer), CG_Argv(i));
		if (i < CG_Argc() - 1) {
			Q_strcat(buffer, sizeof(buffer), " ");
		}
	}

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	const gitem_t *item;
	char			data[MAX_QPATH];
	const char *s;
    const char *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space separated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string",
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue );
#if 1  //def MPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );
#endif

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {

		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/feedback/redleads.wav", qtrue );
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/feedback/blueleads.wav", qtrue );
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue );
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue );

		cgs.media.redWinsSound = trap_S_RegisterSound("sound/feedback/red_wins.ogg", qtrue);
		cgs.media.blueWinsSound = trap_S_RegisterSound("sound/feedback/blue_wins.ogg", qtrue);

		cgs.media.redWinsRoundSound = trap_S_RegisterSound("sound/feedback/red_wins_round.ogg", qtrue);
		cgs.media.blueWinsRoundSound = trap_S_RegisterSound("sound/feedback/blue_wins_round.ogg", qtrue);

		cgs.media.roundBeginsInSound = trap_S_RegisterSound("sound/feedback/round_begins_in.ogg", qtrue);
		cgs.media.roundDrawSound = trap_S_RegisterSound("sound/feedback/rou\
nd_draw.ogg", qtrue);
		cgs.media.thirtySecondWarningSound = trap_S_RegisterSound("sound/feedback/30_second_warning.ogg", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );

		cgs.media.lastStandingSound = trap_S_RegisterSound("sound/teamplay/voc_last_standing.ogg", qtrue);
		cgs.media.attackFlagSound = trap_S_RegisterSound("sound/teamplay/voc_attack_the_flag.ogg", qtrue);
		cgs.media.defendFlagSound = trap_S_RegisterSound("sound/teamplay/voc_defend_the_flag.ogg", qtrue);
		cgs.media.perfectSound = trap_S_RegisterSound("sound/feedback/perfect.ogg", qtrue);
		cgs.media.roundOverSound = trap_S_RegisterSound("sound/feedback/round_over.ogg", qtrue);
		cgs.media.aLostSound = trap_S_RegisterSound("sound/teamplay/a_lost.ogg", qtrue);
		cgs.media.bLostSound = trap_S_RegisterSound("sound/teamplay/b_lost.ogg", qtrue);
		cgs.media.cLostSound = trap_S_RegisterSound("sound/teamplay/c_lost.ogg", qtrue);
		cgs.media.dLostSound = trap_S_RegisterSound("sound/teamplay/d_lost.ogg", qtrue);
		cgs.media.eLostSound = trap_S_RegisterSound("sound/teamplay/e_lost.ogg", qtrue);
		cgs.media.aCapturedSound = trap_S_RegisterSound("sound/teamplay/a_captured.ogg", qtrue);
		cgs.media.bCapturedSound = trap_S_RegisterSound("sound/teamplay/b_captured.ogg", qtrue);
		cgs.media.cCapturedSound = trap_S_RegisterSound("sound/teamplay/c_captured.ogg", qtrue);
		cgs.media.dCapturedSound = trap_S_RegisterSound("sound/teamplay/d_captured.ogg", qtrue);
		cgs.media.eCapturedSound = trap_S_RegisterSound("sound/teamplay/e_captured.ogg", qtrue);

		if ((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) || cg_buildScript.integer) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue );
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );
		}

#if 1  //def MPACK
		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
		}

		if ((cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS)  || cg_buildScript.integer) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
			cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		}

		if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_base_attack.wav", qtrue );
		}
#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );

	cgs.media.electroGibSound1 = trap_S_RegisterSound( "sound/misc/electrogib_01.ogg", qfalse );
	cgs.media.electroGibSound2 = trap_S_RegisterSound( "sound/misc/electrogib_02.ogg", qfalse );
	cgs.media.electroGibSound3 = trap_S_RegisterSound( "sound/misc/electrogib_03.ogg", qfalse );
	cgs.media.electroGibSound4 = trap_S_RegisterSound( "sound/misc/electrogib_04.ogg", qfalse );

	cgs.media.electroGibBounceSound1 = trap_S_RegisterSound( "sound/misc/electrogib_bounce_01.ogg", qfalse );
	cgs.media.electroGibBounceSound2 = trap_S_RegisterSound( "sound/misc/electrogib_bounce_02.ogg", qfalse );
	cgs.media.electroGibBounceSound3 = trap_S_RegisterSound( "sound/misc/electrogib_bounce_03.ogg", qfalse );
	cgs.media.electroGibBounceSound4 = trap_S_RegisterSound( "sound/misc/electrogib_bounce_04.ogg", qfalse );

#if 1  //def MPACK
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
	//cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );
	//cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav", qfalse );
	//cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav", qfalse );
	//cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav", qfalse );
	//cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav", qfalse );

	cgs.media.armorRegenSound = trap_S_RegisterSound("sound/items/cl_armorregen.wav", qfalse);
	cgs.media.armorRegenSoundRegen = trap_S_RegisterSound("sound/misc/ar1_pkup.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("sound/items/cl_scout.wav", qfalse);
#endif

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse );
	cgs.media.hitSound0 = trap_S_RegisterSound( "sound/feedback/hit0.ogg", qfalse );
	cgs.media.hitSound1 = trap_S_RegisterSound( "sound/feedback/hit1.ogg", qfalse );
	cgs.media.hitSound2 = trap_S_RegisterSound( "sound/feedback/hit2.ogg", qfalse );
	cgs.media.hitSound3 = trap_S_RegisterSound( "sound/feedback/hit3.ogg", qfalse );
#if 0  //1  //def MPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
	cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue );
#if 1  //def MPACK
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue);

#if 1 //def MPACK
	cgs.media.voteNow = trap_S_RegisterSound( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue);
#endif

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/wood%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound (name, qfalse);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/snow%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	//cg.powerupRespawnSoundIndex = -999;
	//cg.kamikazeRespawnSoundIndex = -999;
	cg.proxTickSoundIndex = -999;

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		if (cgs.protocol == PROTOCOL_QL) {
			soundName = CG_ConfigString(CS_SOUNDS +i - 1);
		} else {
			soundName = CG_ConfigString(CS_SOUNDS + i);
		}
		if ( !soundName[0] ) {
			break;
			//continue;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		COM_StripExtension(soundName, name, sizeof(name));
		cgs.gameSounds[i] = trap_S_RegisterSound(name, qfalse);

#if 0
		if (!Q_stricmpn(soundName, "sound/weapons/proxmine/wstbtick.", sizeof("sound/weapons/proxmine/wstbtick.") - 1)) {
			cg.proxTickSoundIndex = i;
		}
#endif
		if (!Q_stricmpn(name, "sound/weapons/proxmine/wstbtick", strlen("sound/weapons/proxmine/wstbtick"))) {
			cg.proxTickSoundIndex = i;
			Com_Printf("proxTickSoundIndex %d\n", i);
		}
		Com_Printf("registered sound %d  '%s' %d\n", i, name, cgs.gameSounds[i]);
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.klaxon1 = trap_S_RegisterSound("sound/world/klaxon1.wav", qfalse);
	cgs.media.klaxon2 = trap_S_RegisterSound("sound/world/klaxon2.wav", qfalse);
	cgs.media.buzzer = trap_S_RegisterSound("sound/world/buzzer.ogg", qtrue);

	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);
#if 1  //def MPACK
	cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
	cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
	cgs.media.kamikazeRespawnSound = trap_S_RegisterSound( "sound/items/kamikazerespawn.ogg", qfalse );
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );
	cgs.media.youSuckSound = trap_S_RegisterSound( "sound/misc/yousuck.wav", qfalse );

	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);
#endif

	for (i = 0;  i < 6;  i++) {
		cgs.media.killBeep[i] = trap_S_RegisterSound(va("sound/feedback/impact%i.ogg", i + 1), qfalse);
	}
	cgs.media.killBeep[6] = trap_S_RegisterSound("sound/world/bell_01.ogg", qfalse);
	cgs.media.killBeep[7] = trap_S_RegisterSound("sound/misc/chaching.ogg", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);
	cgs.media.thawTick = trap_S_RegisterSound("sound/misc/tim_pump.ogg", qfalse);
	cgs.media.nightmareSound = trap_S_RegisterSound("sound/misc/nightmare.ogg", qfalse);
	cgs.media.survivorSound = trap_S_RegisterSound("sound/feedback/survivor_01.ogg", qfalse);

	cgs.media.bellSound = trap_S_RegisterSound("sound/world/bell_01.ogg", qfalse);

	// why did i put this here?
	cgs.media.gametypeIcon[GT_FFA] = trap_R_RegisterShader("ui/assets/hud/ffa");
	cgs.media.gametypeIcon[GT_TOURNAMENT] = trap_R_RegisterShader("ui/assets/hud/duel");
	cgs.media.gametypeIcon[GT_TEAM] = trap_R_RegisterShader("ui/assets/hud/tdm");
	cgs.media.gametypeIcon[GT_CA] = trap_R_RegisterShader("ui/assets/hud/ca");
	cgs.media.gametypeIcon[GT_CTF] = trap_R_RegisterShader("ui/assets/hud/ctf");
	cgs.media.gametypeIcon[GT_FREEZETAG] = trap_R_RegisterShader("ui/assets/hud/ft");
	cgs.media.gametypeIcon[GT_1FCTF] = trap_R_RegisterShader("ui/assets/hud/1f");
	cgs.media.gametypeIcon[GT_HARVESTER] = trap_R_RegisterShader("ui/assets/hud/har");
	cgs.media.gametypeIcon[GT_DOMINATION] = trap_R_RegisterShader("ui/assets/hud/dom");
	cgs.media.gametypeIcon[GT_CTFS] = trap_R_RegisterShader("ui/assets/hud/ad");
	cgs.media.gametypeIcon[GT_RED_ROVER] = trap_R_RegisterShader("ui/assets/hud/rr");
	cgs.media.gametypeIcon[GT_RACE] = trap_R_RegisterShader("ui/assets/hud/race");

	cgs.media.infiniteAmmo = trap_R_RegisterShader("icons/infinite");
	cgs.media.premiumIcon = trap_R_RegisterShader("ui/assets/score/premium_icon");
	cgs.media.defragItemShader = trap_R_RegisterShader("wc/defragItemShader");
	cgs.media.weaplit = trap_R_RegisterShader("ui/assets/hud/weaplit2");
	cgs.media.flagCarrier = trap_R_RegisterShader("sprites/flagcarrier");
	cgs.media.flagCarrierNeutral = trap_R_RegisterShader("sprites/neutralflagcarrier");

#ifdef MISSIONPACK
	trap_S_RegisterSound("sound/player/james/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/taunt.wav", qfalse );

	trap_S_RegisterSound("sound/player/janet/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/taunt.wav", qfalse );
#endif

}

static void CG_RegisterFonts (void)
{
	trap_R_RegisterFont("fontimage", 12, &cgs.media.qlfont12);
	trap_R_RegisterFont("fontimage", 16, &cgs.media.qlfont16);
	trap_R_RegisterFont("fontimage", 20, &cgs.media.qlfont20);
	trap_R_RegisterFont("fontimage", 24, &cgs.media.qlfont24);
	trap_R_RegisterFont("fontimage", 48, &cgs.media.qlfont48);
	//FIXME this shit
	//trap_R_RegisterFont(cg_drawCenterPrintFont.string, 48, &cgs.media.centerPrintFont);
	//trap_R_RegisterFont("q3font", 16, &cgs.media.q3font16);
	trap_R_RegisterFont("q3tiny", 16, &cgs.media.tinychar);
	trap_R_RegisterFont("q3small", 16, &cgs.media.smallchar);
	trap_R_RegisterFont("q3big", 16, &cgs.media.bigchar);
	trap_R_RegisterFont("q3giant", 16, &cgs.media.giantchar);

	cgs.media.fragMessageFontModificationCount = -100;
	cgs.media.centerPrintFontModificationCount = -100;
	cgs.media.rewardsFontModificationCount = -100;
	cgs.media.fpsFontModificationCount = -100;
	cgs.media.clientItemTimerFontModificationCount = -100;
	cgs.media.playerNamesFontModificationCount = -100;
	cgs.media.playerNamesStyleModificationCount = -100;
	cgs.media.playerNamesPointSizeModificationCount = -100;
	cgs.media.playerNamesColorModificationCount = -100;
	cgs.media.playerNamesAlphaModificationCount = -100;

	cgs.media.snapshotFontModificationCount = -100;
	cgs.media.ammoWarningFontModificationCount = -100;
	cgs.media.crosshairNamesFontModificationCount = -100;
	cgs.media.crosshairTeammateHealthFontModificationCount = -100;

	cgs.media.warmupStringFontModificationCount = -100;
	cgs.media.waitingForPlayersFontModificationCount = -100;
	cgs.media.waitingForPlayersFontModificationCount = -100;
	cgs.media.voteFontModificationCount = -100;
	cgs.media.teamVoteFontModificationCount = -100;
	cgs.media.followingFontModificationCount = -100;
	cgs.media.weaponBarFontModificationCount = -100;
	cgs.media.itemPickupsFontModificationCount = -100;
	cgs.media.originFontModificationCount = -100;
	cgs.media.speedFontModificationCount = -100;
	cgs.media.lagometerFontModificationCount = -100;
	cgs.media.attackerFontModificationCount = -100;
	cgs.media.teamOverlayFontModificationCount = -100;
	cgs.media.cameraPointInfoFontModificationCount = -100;
	cgs.media.jumpSpeedsFontModificationCount = -100;
	cgs.media.jumpSpeedsTimeFontModificationCount = -100;
	cgs.media.proxWarningFontModificationCount = -100;
	cgs.media.dominationPointStatusFontModificationCount = -100;

	CG_CheckFontUpdates();

}

//===================================================================================


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];
	static const char *sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	Com_Printf("cgame: loadworldmap %s\n", cgs.mapname);
	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );
	cgs.media.unavailableShader = trap_R_RegisterShader("gfx/2d/unavailable");

	//cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardName = trap_R_RegisterShader("scoreboardName");
	//cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShader("scoreboardPing");
	//cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShader("scoreboardScore");
	//cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShader("scoreboardTime");

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
#if  1 //def MPACK
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
	cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
#endif
	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	if (cgs.gametype == GT_FREEZETAG) {
		cgs.media.iceTrailShader = trap_R_RegisterShader("iceTrail");
	}

	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.q3bloodTrailShader = trap_R_RegisterShader("q3bloodTrail");

	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for ( i = 1 ; i < NUM_CROSSHAIRS ; i++ ) {
		int w, h;
		//cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%d", i) );
		trap_GetShaderImageDimensions(cgs.media.crosshairShader[i], &w, &h);
		if (w > 64  ||  h > 64) {
			Com_Printf("^1skipping brightness for crosshair %d  (%d x %d)\n", i, w, h);
		} else {
			trap_GetShaderImageData(cgs.media.crosshairShader[i], cgs.media.crosshairOrigImage[i]);
		}
	}
	CG_CreateNewCrosshairs();

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.spawnArmorShader = trap_R_RegisterShader("powerups/spawnarmor");
	cgs.media.spawnArmor2Shader = trap_R_RegisterShader("powerups/spawnarmor2");
	cgs.media.gooShader = trap_R_RegisterShader("powerups/goo");
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

#if 1  //def MPACK
	if ( cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
	}

	cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
	cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );

	cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
	cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );

	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cgs.gametype == GT_CTFS   ||  cg_buildScript.integer) {
#else
	if ( cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  || cg_buildScript.integer ) {
#endif

		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );

		cgs.media.neutralFlagModel2 = trap_R_RegisterModel("models/flag3/n_flag3.md3");
		cgs.media.neutralFlagModel3 = trap_R_RegisterModel("models/flag3/d_flag3.md3");  // can colorize
		cgs.media.redFlagModel2 = trap_R_RegisterModel("models/flag3/r_flag3.md3");
		cgs.media.blueFlagModel2 = trap_R_RegisterModel("models/flag3/b_flag3.md3");

		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );

		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );
#if 1  //def MPACK
		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );
		//cgs.media.neutralFlagFlapSkin = CG_RegisterSkinVertexLight("models/flag2/white.skin");

		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
#endif
	}

#if 1  //def MPACK
	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {

		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = 0;  // unused
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[4] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}

	if ((cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_1FCTF) || cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}

	if (cgs.gametype == GT_DOMINATION  ||  cg_buildScript.integer) {
		cgs.media.dominationModel = trap_R_RegisterModel("models/powerups/domination/dompoint.md3");
		cgs.media.dominationRedSkin = trap_R_RegisterSkin("models/powerups/domination/domred.skin");
		cgs.media.dominationBlueSkin = trap_R_RegisterSkin("models/powerups/domination/domblue.skin");
		cgs.media.dominationNeutralSkin = trap_R_RegisterSkin("models/powerups/domination/domntrl.skin");
		cgs.media.dominationFloorMark = trap_R_RegisterShaderNoMip("domShadow");
		cgs.media.dominationCapA = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_a");
		cgs.media.dominationCapB = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_b");
		cgs.media.dominationCapC = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_c");
		cgs.media.dominationCapD = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_d");
		cgs.media.dominationCapE = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_e");
		cgs.media.dominationCapADist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_a_dist");
		cgs.media.dominationCapBDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_b_dist");
		cgs.media.dominationCapCDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_c_dist");
		cgs.media.dominationCapDDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_d_dist");
		cgs.media.dominationCapEDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_cap_e_dist");

		cgs.media.dominationDefendA = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_a");
		cgs.media.dominationDefendB = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_b");
		cgs.media.dominationDefendC = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_c");
		cgs.media.dominationDefendD = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_d");
		cgs.media.dominationDefendE = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_e");
		cgs.media.dominationDefendADist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_a_dist");
		cgs.media.dominationDefendBDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_b_dist");
		cgs.media.dominationDefendCDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_c_dist");
		cgs.media.dominationDefendDDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_d_dist");
		cgs.media.dominationDefendEDist = trap_R_RegisterShaderNoMip("gfx/2d/dom_point/dom_def_e_dist");
	}

	if (cgs.gametype == GT_RACE) {
		cgs.media.dominationModel = trap_R_RegisterModel("models/powerups/domination/dompoint.md3");
		cgs.media.dominationRedSkin = trap_R_RegisterSkin("models/powerups/domination/domred.skin");
		cgs.media.dominationBlueSkin = trap_R_RegisterSkin("models/powerups/domination/domblue.skin");
		cgs.media.dominationNeutralSkin = trap_R_RegisterSkin("models/powerups/domination/domntrl.skin");
		cgs.media.adCapture = trap_R_RegisterShaderNoMip("gfx/2d/ad/poi_capture");
		cgs.media.raceStart = trap_R_RegisterShaderNoMip("gfx/2d/race/start");
		cgs.media.raceCheckPoint = trap_R_RegisterShaderNoMip("gfx/2d/race/checkpoint");
		cgs.media.raceFinish = trap_R_RegisterShaderNoMip("gfx/2d/race/finish");
		cgs.media.raceNav = trap_R_RegisterShaderNoMip("gfx/misc/racenav");
		cgs.media.raceWorldTimerHand = trap_R_RegisterShaderNoMip("gfx/2d/world_timer_hand");
		cgs.media.activeCheckPointRaceFlagModel = trap_R_RegisterModel("models/flag3/b_flag3.md3");
	}

	cgs.media.harvesterCapture = trap_R_RegisterShaderNoMip("gfx/2d/har/poi_capture");
	cgs.media.adAttack = trap_R_RegisterShaderNoMip("gfx/2d/ad/poi_attack");
	cgs.media.adCapture = trap_R_RegisterShaderNoMip("gfx/2d/ad/poi_capture");
	cgs.media.adDefend = trap_R_RegisterShaderNoMip("gfx/2d/ad/poi_defend");

	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );
#endif

	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );

	if (CG_IsTeamGame(cgs.gametype)  ||  cg_buildScript.integer) {
		//cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
		cgs.media.friendShader = trap_R_RegisterShader( "wc/friend" );
		if (!cgs.media.friendShader) {  // bug fix
			Com_Printf("^1couldn't load wc/friend\n");
			cgs.media.friendShader = trap_R_RegisterShader("sprites/foe");
		}
		cgs.media.friend2Shader = trap_R_RegisterShader("sprites/friend2");
		cgs.media.friendHitShader = trap_R_RegisterShader("sprites/friend_hit");
		cgs.media.friendDeadShader = trap_R_RegisterShader("sprites/friend_dead");
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );

#ifdef MISSIONPACK
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
#endif

	}
	cgs.media.foeShader = trap_R_RegisterShader("wc/foe");
	cgs.media.selfShader = trap_R_RegisterShader("wc/self");
	cgs.media.infectedFoeShader = trap_R_RegisterShader("gfx/2d/infected/bite");

	if (cgs.gametype == GT_FREEZETAG) {
		for (i = 0;  i < 3;  i++) {
			cgs.media.iceShader[i] = trap_R_RegisterShader(va("powerups/ice%d", i + 1));
		}
		cgs.media.frozenShader = trap_R_RegisterShader("sprites/frozen");
		cgs.media.thawIcon = trap_R_RegisterShader("icons/thaw");
		if (!cgs.media.thawIcon) {
			//cgs.media.thawIcon = cgs.media.frozenShader;
			cgs.media.thawIcon = trap_R_RegisterShader("ui/assets/hud/ft");
		}
	}

	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );

	cgs.media.yellowArmorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	cgs.media.redArmorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_red" );
	cgs.media.greenArmorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_green" );
	cgs.media.megaHealthIcon  = trap_R_RegisterShaderNoMip( "icons/iconh_mega" );
	cgs.media.quadIcon = trap_R_RegisterShaderNoMip("icons/quad");
	cgs.media.battleSuitIcon = trap_R_RegisterShaderNoMip("icons/envirosuit");


	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	if (cgs.gametype == GT_FREEZETAG) {
		cgs.media.iceAbdomen = trap_R_RegisterModel("models/gibs/abdomen.md3");
		cgs.media.iceBrain = trap_R_RegisterModel("models/gibs/brain.md3");
	}

	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibsq3/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibsq3/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibsq3/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibsq3/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibsq3/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibsq3/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibsq3/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibsq3/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibsq3/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibsq3/brain.md3" );

	cgs.media.gibSphere = trap_R_RegisterModel("models/gibs/sphere.md3");

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#if 1  //def MPACK
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
#else
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
#endif

	//cgs.media.deathEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
	cgs.media.deathEffectModel = trap_R_RegisterModel( "models/weaphits/bfg.md3" );
	cgs.media.deathEffectShader = trap_R_RegisterShader( "deathEffect" );

#if 1  //def MPACK
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.armorRegenPowerupModel = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/regen.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
#endif

	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );
	cgs.media.headShotIcon = trap_R_RegisterShaderNoMip("icons/headshot");

	cgs.media.adbox1x1 = trap_R_RegisterShader("adbox1x1");
    cgs.media.adbox2x1 = trap_R_RegisterShader("adbox2x1");
	cgs.media.adbox2x1_trans = trap_R_RegisterShader("adbox2x1_trans");
    //cgs.media.adbox2x1 = trap_R_RegisterShader("textures/ad_content/2x1_trans_sfx");
    cgs.media.adbox4x1 = trap_R_RegisterShader("adbox4x1");
    cgs.media.adbox8x1 = trap_R_RegisterShader("adbox8x1");
    cgs.media.adboxblack = trap_R_RegisterShader("adboxblack");


	cgs.media.caScoreRed = trap_R_RegisterShader("ui/assets/score/ca_score_red");
	cgs.media.caScoreBlue = trap_R_RegisterShader("ui/assets/score/ca_score_blu");

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	for (i = 0;  i < MAX_WEAPONS;  i++) {
		cg_weapons[i].weaponIcon = trap_R_RegisterShader("icons/skull_red");
	}
	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		// load all non weapons...  /devmap /spawn lksdjfdkf
		// weapons can't be loaded since it screws up ql weapon bar behavior
		if (items[ i ] == '1' || cg_buildScript.integer ||  cgs.cpma  ||  bg_itemlist[i].giType != IT_WEAPON) {
			gitem_t *item;

			//Com_Printf("^5%d '%s'\n", i, bg_itemlist[i].pickup_name);
			item = &bg_itemlist[i];
			if (!Q_stricmp(item->pickup_name, "Green Armor")) {
				cgs.greenArmorItem = item;
			} else if (!Q_stricmp(item->pickup_name, "Yellow Armor")) {
				cgs.yellowArmorItem = item;
			} else if (!Q_stricmp(item->pickup_name, "Red Armor")) {
				cgs.redArmorItem = item;
			}

			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	if (cgs.gametype == GT_FREEZETAG) {
		cgs.media.iceMarkShader = trap_R_RegisterShader("iceMark");
	}

	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );
	cgs.media.q3bloodMarkShader = trap_R_RegisterShader("q3bloodMark");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
		CG_Printf("gameModels[%d]  %s\n", i, modelName);

		if (!Q_stricmp(modelName, "models/flag3/d_flag3.md3")) {
			cgs.checkPointRaceFlagModel = i;
			cgs.dominationControlPointModel = i;
			//Com_Printf("control point model %d\n", i);
		} else if (!Q_stricmp(modelName, "models/flag3/g_flag3.md3")) {
			cgs.startRaceFlagModel = i;
		} else if (!Q_stricmp(modelName, "models/flag3/f_flag3.md3")) {
			cgs.endRaceFlagModel = i;
		}
	}

#if 1  //def MPACK
	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	//cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/3_cursor3.png");
	//cgs.media.selectCursor = cgs.media.cursor;  //trap_R_RegisterShaderNoMip( "ui/assets/3_cursor3.png");

	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

#if 0  // what's the point?
	trap_R_RegisterModel( "models/players/james/lower.md3" );
	trap_R_RegisterModel( "models/players/james/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/james/james.md3" );

	trap_R_RegisterModel( "models/players/janet/lower.md3" );
	trap_R_RegisterModel( "models/players/janet/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/janet/janet.md3" );
#endif


#endif

	cgs.media.pickup_iconra = trap_R_RegisterShader("pickup_RA");
	cgs.media.pickup_iconya = trap_R_RegisterShader("pickup_YA");
	cgs.media.pickup_iconga = trap_R_RegisterShader("pickup_GA");
	cgs.media.pickup_iconmh = trap_R_RegisterShader("pickup_MH");
	cgs.media.pickup_iconmedkit = trap_R_RegisterShader("pickup_MK");
	cgs.media.pickup_iconredflag = trap_R_RegisterShader("pickup_REDFLAG");
	cgs.media.pickup_iconblueflag = trap_R_RegisterShader("pickup_BLUEFLAG");
	cgs.media.pickup_iconneutralflag = trap_R_RegisterShader("pickup_NTRLFLAG");
	cgs.media.pickup_iconquad = trap_R_RegisterShader("pickup_QUAD");
	cgs.media.pickup_iconbs = trap_R_RegisterShader("pickup_BS");
	cgs.media.pickup_iconregen = trap_R_RegisterShader("pickup_REGEN");
	cgs.media.pickup_iconhaste = trap_R_RegisterShader("pickup_HASTE");
	cgs.media.pickup_iconinvis = trap_R_RegisterShader("pickup_INVIS");

	cgs.media.ghostWeaponShader = trap_R_RegisterShader("ghostWeaponShader");
	cgs.media.noPlayerClipShader = trap_R_RegisterShader("noPlayerClipShader");
	cgs.media.rocketAimShader = trap_R_RegisterShader("wc/wcrocketaim");
	cgs.media.bboxShader = trap_R_RegisterShader( "bbox" );
	cgs.media.bboxShader_nocull = trap_R_RegisterShader( "bbox_nocull" );

	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}



/*
=======================

CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	const char *s;
	int count;
	char slist[MAX_STRING_CHARS];

	//Com_Printf("build spec string\n");

	//cg.spectatorList[0] = 0;
	//slist[0] = '\0';
	memset(slist, 0, sizeof(slist));

#if 0
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
#endif

	//Com_Printf("spec string cg.numScores %d\n", cg.numScores);

	for (i = 0;  i < cg.numScores;  i++) {
		const score_t *sc;

		sc = &cg.scores[i];
		if (sc->team == TEAM_SPECTATOR) {

			if (cg_spectatorListSkillRating.integer  &&  cgs.clientinfo[sc->client].knowSkillRating) {
				Q_strcat(slist, sizeof(slist), va("^5(%d)", cgs.clientinfo[sc->client].skillRating));
			}
			if (*cgs.clientinfo[sc->client].clanTag) {
				Q_strcat(slist, sizeof(slist), va("^7%s ", cgs.clientinfo[sc->client].clanTag));
			}

			Q_strcat(slist, sizeof(slist), va("^7%s", cgs.clientinfo[sc->client].name));
			//Q_strcat(slist, sizeof(slist), va("%s", cgs.clientinfo[sc->client].name));

			if (cg_spectatorListScore.integer) {
				Q_strcat(slist, sizeof(slist), va("^3(%d)", sc->score));
			}
			if (cg_spectatorListQue.integer  &&  cgs.protocol == PROTOCOL_QL  &&  cgs.gametype == GT_TOURNAMENT) {
				if (cgs.clientinfo[sc->client].spectatorOnly) {
					Q_strcat(slist, sizeof(slist), "^7(^5s^7)");
				} else {
					Q_strcat(slist, sizeof(slist), va("^7(%d)", cgs.clientinfo[sc->client].quePosition));
				}
			}

			Q_strcat(slist, sizeof(slist), "^7     ");

#if 0
			//Com_Printf("%s\n", cgs.clientinfo[sc->client].name);
			if (cgs.clientinfo[sc->client].knowSkillRating  &&  cg_spectatorListSkillRating.integer) {
				//Com_Printf("know for %s\n", cgs.clientinfo[sc->client].name);
				if (*cgs.clientinfo[sc->client].clanTag) {
					Q_strcat(slist, sizeof(slist), va("^5(%d)%s %s^3(%d)^7     ", cgs.clientinfo[sc->client].skillRating, cgs.clientinfo[sc->client].clanTag, cgs.clientinfo[sc->client].name, sc->score));
				} else {
					Q_strcat(slist, sizeof(slist), va("^5(%d)%s^3(%d)^7     ", cgs.clientinfo[sc->client].skillRating, cgs.clientinfo[sc->client].name, sc->score));
				}
			} else {
				if (*cgs.clientinfo[sc->client].clanTag) {
					Q_strcat(slist, sizeof(slist), va("%s %s^3(%d)^7     ", cgs.clientinfo[sc->client].clanTag, cgs.clientinfo[sc->client].name, sc->score));
				} else {
					Q_strcat(slist, sizeof(slist), va("%s^3(%d)^7     ", cgs.clientinfo[sc->client].name, sc->score));
				}
			}
#endif
		}
	}

#if 0  //FIXME eliminate this stuff
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (cgs.newConnectedClient[i]) {
			Q_strcat(slist, sizeof(slist), va("%s     ", cgs.clientinfo[i].name));
		}
	}
#endif

#if 0
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
#endif

	// in case last char is ^
#if 0
	slist[MAX_STRING_CHARS - 1] = '\0';
	slist[MAX_STRING_CHARS - 2] = '\0';
#endif
	
	s = slist;  //cg.spectatorList;
	count = 0;
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if (cgs.osp) {
				if (s[1] == 'x'  ||  s[1] == 'X') {
					s += 8;
				} else {
					s += 2;
				}
			} else {
				s += 2;  // what if last char is ^
			}
		} else {
			count++;
			s++;
		}
	}
#if 0
	if (count != cg.spectatorLen) {
		cg.spectatorLen = count;
		cg.spectatorWidth = -1;
	}
#endif

	//if (!Q_stricmp(cg.spectatorList, slist)) {
	if (strcmp(cg.spectatorList, slist)) {
		memset(cg.spectatorList, 0, sizeof(cg.spectatorList));
		Q_strncpyz(cg.spectatorList, slist, sizeof(cg.spectatorList));
		cg.spectatorLen = count;
		cg.spectatorWidth = -1;
	}
	//Com_Printf("spec list: %s   slist: %s\n", cg.spectatorList, slist);
}


/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;
	clientInfo_t tmpCi;
	int numExtraPlayers;

	if (cg.demoPlayback) {
		char modelName[MAX_QPATH];
		char *skin;

		numExtraPlayers = trap_GetNumPlayerInfos();

		for (i = 0;  i < numExtraPlayers;  i++) {
			trap_GetExtraPlayerInfo(i, modelName);
			Com_Printf("^5extra %d  '%s'\n", i, modelName);

			skin = strrchr(modelName, '/');
			if (skin) {
				*skin++ = '\0';
			} else {
				skin = "default";
			}

			// valgrind warnings
			tmpCi.team = TEAM_SPECTATOR;
			Q_strncpyz(tmpCi.modelName, modelName, sizeof(tmpCi.modelName));
			Q_strncpyz(tmpCi.headModelName, modelName, sizeof(tmpCi.modelName));
			CG_LoadingFutureClient(modelName);
			CG_RegisterClientModelname(&tmpCi, modelName, skin, modelName, skin, "", qfalse);
		}
	}

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		//Com_Printf("^5new client %d '%s'\n", i, clientInfo);
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	//CG_BuildSpectatorString();

}

//===========================================================================

qboolean CG_ConfigStringIndexToQ3 (int *index)
{
	int n;

	//Com_Printf("index %d\n", *index);

	if (*index >= CS_MODELS  &&  *index < CS_MODELS + MAX_MODELS) {
		//Com_Printf("md\n");
		n = *index - CS_MODELS;
		*index = CSQ3_MODELS + n;
		return qtrue;
	}
	if (*index >= CS_SOUNDS  &&  *index < CS_PLAYERS) {
		//Com_Printf("sn\n");
		n = *index - CS_SOUNDS;
		*index = CSQ3_SOUNDS + n;
		return qtrue;
	}
	if (*index >= CS_PLAYERS  &&  *index < CS_PLAYERS + MAX_CLIENTS) {
		n = *index - CS_PLAYERS;
		//Com_Printf("nnn %d  ->  %d\n", *index, n);
		*index = CSQ3_PLAYERS + n;
		return qtrue;
	}
	if (*index >= CS_LOCATIONS  &&  *index < CS_LOCATIONS + MAX_LOCATIONS) {
		n = *index - CS_LOCATIONS;
		*index = CSQ3_LOCATIONS + n;
		return qtrue;
	}

	switch (*index) {
	case CS_SERVERINFO:
		n = CSQ3_SERVERINFO;
		break;
	case CS_SYSTEMINFO:
		n = CSQ3_SYSTEMINFO;
		break;
	case CS_MUSIC:
		n = CSQ3_MUSIC;
		break;
	case CS_MESSAGE:
		n = CSQ3_MESSAGE;
		break;
	case CS_MOTD:
		n = CSQ3_MOTD;
		break;
	case CS_WARMUP:
		n = CSQ3_WARMUP;
		break;
	case CS_SCORES1:
		n = CSQ3_SCORES1;
		break;
	case CS_SCORES2:
		n = CSQ3_SCORES2;
		break;
	case CS_VOTE_TIME:
		n = CSQ3_VOTE_TIME;
		break;
	case CS_VOTE_STRING:
		n = CSQ3_VOTE_STRING;
		break;
	case CS_VOTE_YES:
		n = CSQ3_VOTE_YES;
		break;
	case CS_VOTE_NO:
		n = CSQ3_VOTE_NO;
		break;
	case CS_TEAMVOTE_TIME:
		n = CSQ3_TEAMVOTE_TIME;
		break;
	case CS_TEAMVOTE_STRING:
		n = CSQ3_TEAMVOTE_STRING;
		break;
	case CS_TEAMVOTE_YES:
		n = CSQ3_TEAMVOTE_YES;
		break;
	case CS_TEAMVOTE_NO:
		n = CSQ3_TEAMVOTE_NO;
		break;
	case CS_GAME_VERSION:
		n = CSQ3_GAME_VERSION;
		break;
	case CS_LEVEL_START_TIME:
		n = CSQ3_LEVEL_START_TIME;
		break;
	case CS_INTERMISSION:
		n = CSQ3_INTERMISSION;
		break;
	case CS_FLAGSTATUS:
		n = CSQ3_FLAGSTATUS;
		break;
	case CS_SHADERSTATE:
		n = CSQ3_SHADERSTATE;
		break;
	case CS_BOTINFO:
		n = CSQ3_BOTINFO;
		break;
	case CS_ITEMS:
		n = CSQ3_ITEMS;
		break;
	case CS_MODELS:
		n = CSQ3_MODELS;
		break;
	case CS_SOUNDS:
		n = CSQ3_SOUNDS;
		break;
	case CS_LOCATIONS:
		n = CSQ3_LOCATIONS;
		break;
	case CS_PARTICLES:
		n = CSQ3_PARTICLES;
		break;

	default:
		//Com_Printf("unknown q3 config string %d\n", *index);
		n = *index;
		return qfalse;
	}

#if 0
	if (n < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigStringQ3: bad index: %i", n);
	}
	if (cg.configStringOverride[n]) {
		Com_Printf("%d: %s\n", n, cg.configStringOurs[n]);
		return cg.configStringOurs[n];
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[n];
#endif

	*index = n;
	return qtrue;
}

qboolean CG_ConfigStringIndexFromQ3 (int *index)
{
	int n;

	if (cgs.cpma  &&  *index >= CSCPMA_GAMESTATE) {
		return qtrue;
	}

	if (*index >= CSQ3_MODELS  &&  *index < CSQ3_MODELS + MAX_MODELS) {
		n = *index - CSQ3_MODELS;
		*index = CS_MODELS + n;
		return qtrue;
	}
	if (*index >= CSQ3_SOUNDS  &&  *index < CSQ3_SOUNDS + MAX_SOUNDS) {
		n = *index - CSQ3_SOUNDS;
		*index = CS_SOUNDS + n;
		return qtrue;
	}
	if (*index >= CSQ3_PLAYERS  &&  *index < CSQ3_PLAYERS + MAX_CLIENTS) {
		n = *index - CSQ3_PLAYERS;
		*index = CS_PLAYERS + n;
		return qtrue;
	}
	if (*index >= CSQ3_LOCATIONS  &&  *index < CSQ3_LOCATIONS + MAX_LOCATIONS) {
		n = *index - CSQ3_LOCATIONS;
		*index = CS_LOCATIONS + n;
		return qtrue;
	}

	switch (*index) {
	case CSQ3_SERVERINFO:
		n = CS_SERVERINFO;
		break;
	case CSQ3_SYSTEMINFO:
		n = CS_SYSTEMINFO;
		break;
	case CSQ3_MUSIC:
		n = CS_MUSIC;
		break;
	case CSQ3_MESSAGE:
		n = CS_MESSAGE;
		break;
	case CSQ3_MOTD:
		n = CS_MOTD;
		break;
	case CSQ3_WARMUP:
		n = CS_WARMUP;
		break;
	case CSQ3_SCORES1:
		n = CS_SCORES1;
		break;
	case CSQ3_SCORES2:
		n = CS_SCORES2;
		break;
	case CSQ3_VOTE_TIME:
		n = CS_VOTE_TIME;
		break;
	case CSQ3_VOTE_STRING:
		n = CS_VOTE_STRING;
		break;
	case CSQ3_VOTE_YES:
		n = CS_VOTE_YES;
		break;
	case CSQ3_VOTE_NO:
		n = CS_VOTE_NO;
		break;
	case CSQ3_TEAMVOTE_TIME:
		n = CS_TEAMVOTE_TIME;
		break;
	case CSQ3_TEAMVOTE_STRING:
		n = CS_TEAMVOTE_STRING;
		break;
	case CSQ3_TEAMVOTE_YES:
		n = CS_TEAMVOTE_YES;
		break;
	case CSQ3_TEAMVOTE_NO:
		n = CS_TEAMVOTE_NO;
		break;
	case CSQ3_GAME_VERSION:
		n = CS_GAME_VERSION;
		break;
	case CSQ3_LEVEL_START_TIME:
		n = CS_LEVEL_START_TIME;
		break;
	case CSQ3_INTERMISSION:
		n = CS_INTERMISSION;
		break;
	case CSQ3_FLAGSTATUS:
		n = CS_FLAGSTATUS;
		break;
	case CSQ3_SHADERSTATE:
		n = CS_SHADERSTATE;
		break;
	case CSQ3_BOTINFO:
		n = CS_BOTINFO;
		break;
	case CSQ3_ITEMS:
		n = CS_ITEMS;
		break;
	case CSQ3_MODELS:
		n = CS_MODELS;
		break;
	case CSQ3_SOUNDS:
		n = CS_SOUNDS;
		break;
	case CSQ3_LOCATIONS:
		n = CS_LOCATIONS;
		break;
	case CSQ3_PARTICLES:
		n = CS_PARTICLES;
		break;

	default:
		//Com_Printf("unknown q3 config string %d\n", *index);
		n = *index;
		return qfalse;
	}

#if 0
	if (n < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigStringQ3: bad index: %i", n);
	}
	if (cg.configStringOverride[n]) {
		Com_Printf("%d: %s\n", n, cg.configStringOurs[n]);
		return cg.configStringOurs[n];
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[n];
#endif

	*index = n;
	return qtrue;
}


/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	int n;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	if (cg.configStringOverride[index]) {
		Com_Printf("%d: %s\n", index, cg.configStringOurs[index]);
		return cg.configStringOurs[index];
	}

	n = index;
	if (cgs.protocol == PROTOCOL_Q3) {
		CG_ConfigStringIndexToQ3(&n);
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[n];
}

const char *CG_ConfigStringNoConvert (int index)
{
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigStringNoConvert: bad index: %i", index );
	}
	if (cg.configStringOverride[index]) {
		Com_Printf("%d: %s\n", index, cg.configStringOurs[index]);
		return cg.configStringOurs[index];
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	const char *s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];
    char buffer[MAX_STRING_CHARS];
    char *ptr;


	//FIXME
	//CG_Printf("FIXME CG_StartMusic()\n");
	//return;

	// start the background music
	s = CG_ConfigString( CS_MUSIC );
    Q_strncpyz(buffer, s, sizeof(buffer));
    ptr = buffer;
	Q_strncpyz( parm1, COM_Parse( &ptr ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &ptr ), sizeof( parm2 ) );

	//CG_Printf("cgame starting music  %s %s\n", parm1, parm2);

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

#if 1  //def MPACK

#if 0  // unused

static char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		Com_Printf(S_COLOR_RED "menu file not found: %s, using default\n", filename);
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_FS_FCloseFile( f );
		Com_Printf(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE);
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

#endif

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
static qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			if (!cgDC.Assets.cursor) {
				//FIXME
				cgDC.Assets.cursor = trap_R_RegisterShaderNoMip("menu/art/3_cursor2");
				if (!cgDC.Assets.cursor) {
					Com_Printf("no cursor\n");
				}
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}

		if (Q_stricmp(token.string, "setvar") == 0) {
			const char *varName;
			const char *mathScript;
			float f;
			int err;

			if (!PC_String_Parse(handle, &varName)) {
				Com_Printf("^1CG_Asset_Parse() couldn't read menu variable name\n");
				break;
			}
			//FIXME const
			PC_Parenthesis_Parse(handle, (const char **)&mathScript);
			//FIXME const
			Q_MathScript((char *)mathScript, &f, &err);
			MenuVar_Set(varName, f);

			continue;
		}

		Com_Printf("^3CG_Asset_Parse() unknown token '%s'\n", token.string);

	}
	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu (const char *menuFile)
{
	pc_token_t token;
	int handle;
	char altName[MAX_STRING_CHARS];
	const char *fileName = NULL;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		//Q_strncpyz(altName, menuFile, sizeof(altName));
		Com_Printf("CG_ParseMenu() couldn't open %s\n", menuFile);
		COM_StripExtension(menuFile, altName, sizeof(altName));
		//Com_Printf("%s--\n", altName);
		handle = trap_PC_LoadSource(va("%s.smenu", altName));
		if (!handle) {
			Com_Printf("CG_ParseMenu() couldn't open %s.smenu\n", altName);
			return;
		} else {
			fileName = altName;
		}
	} else {
		fileName = menuFile;
	}

	//Com_Printf("CG_ParseMenu: '%s'\n", menuFile);

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if (token.string[0] == '{') {
			continue;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
			continue;
		}

		if (Q_stricmp(token.string, "setvar") == 0) {
			const char *varName;
			char *mathScript;
			float f;
			int err;

			if (!PC_String_Parse(handle, &varName)) {
				Com_Printf("^1CG_ParseMenu() couldn't read menu variable name\n");
				break;
			}
			//FIXME const
			PC_Parenthesis_Parse(handle, (const char **)&mathScript);
			Q_MathScript(mathScript, &f, &err);
			MenuVar_Set(varName, f);

			continue;
		}

		if (1) {
			Com_Printf("^3CG_ParseMenu() %s  unknown token '%s'\n", fileName, token.string);
		}
	}
	trap_PC_FreeSource(handle);
}

static qboolean CG_Load_Menu(char **p) {
	char *token;

	//Com_Printf("CG_Load_Menu() %s\n", *p);

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token);
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	const char*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	Com_Printf("CG_LoadMenus()  %s\n", menuFile);

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f ) {
		//CG_Error(S_COLOR_YELLOW "menu file not found: %s, using default", menuFile);
		Com_Printf(S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile);
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			//CG_Error(S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!");
			Com_Printf(S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n");
			return;
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		//CG_Error(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE);
		Com_Printf(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", menuFile, len, MAX_MENUDEFFILE);
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress(buf);

	//Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if (token[0] == '{') {
			continue;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}

		Com_Printf("^3CG_LoadMenus() unknown token '%s'\n", token);
	}

	Com_Printf("UI menu load time = %d milli seconds  %s\n", trap_Milliseconds() - start, menuFile);

}



static qboolean CG_OwnerDrawHandleKey (int ownerDraw, int flags, int flags2, float *special, int key)
{
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		for (i = 0;  i < cg.numScores;  i++) {
			if (cg.scores[i].team == TEAM_SPECTATOR) {
				continue;
			}
			count++;
		}
	} else if (feederID == FEEDER_REDTEAM_STATS) {
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			if (cgs.gametype == GT_TEAM) {
				if (!cg.tdmStats[i].valid) {
					continue;
				}
				if (cgs.clientinfo[i].team == TEAM_RED) {
					//count++;
				}
			} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_DOMINATION) {
				if (!cg.ctfStats[i].valid) {
					continue;
				}
			} else if (cgs.gametype == GT_CA) {
				if (!cg.caStats[i].valid) {
					continue;
				}
			} else {
				continue;
			}
			if (cgs.clientinfo[i].infoValid  &&  cgs.clientinfo[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_STATS) {
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			if (cgs.gametype == GT_TEAM) {
				if (!cg.tdmStats[i].valid) {
					continue;
				}
			} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_DOMINATION) {
				if (!cg.ctfStats[i].valid) {
					continue;
				}
			} else if (cgs.gametype == GT_CA) {
				if (!cg.caStats[i].valid) {
					continue;
				}
			} else {
				continue;
			}
			if (cgs.clientinfo[i].infoValid  &&  cgs.clientinfo[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else {
		Com_Printf("CG_FeederCount()  unknown feeder id 0x%x\n", (int)feederID);
	}

	return count;
}

void CG_SetScoreSelection (menuDef_t *menu)
{
	const playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	int feeder;

	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if (cgs.gametype == GT_TEAM) {
		if (cg.scores[cg.selectedScore].team == TEAM_RED) {
		}
	}


	if ( cgs.gametype >= GT_TEAM ) {
		feeder = FEEDER_REDTEAM_LIST;
		i = red;

		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;

	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					//Com_Printf("%d  %s\n", i, cgs.clientinfo[cg.scores[i].client].name);
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}

	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

void CG_LimitText (char *s, int len)
{
	char *p;

	if (len < 0) {
		return;
	}

	//s[len] = '\0';
	p = s + strlen(s);
	while (CG_Text_Length(s) > len) {
		p[0] = '\0';
		p--;
		//Com_Printf("'%s'\n", s);
	}
}

#define CA_TEXT_LIMIT 12

static const char *CG_FeederItemTextCa (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	int n;
	const tdmPlayerScore_t *ts;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextCa() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	ts = &cg.tdmScore.playerScore[sp->client];

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			return "";
		case 1: {
			if (ts->subscriber  &&  cg_scoreBoardStyle.integer == 0) {
				*handle = cgs.media.premiumIcon;
			}
			return "";
		}
		case 2:
			if (info->handicap < 100) {
				return va("%d%%", info->handicap);
			}
			return "";

		case 3: {
			qboolean ready = qfalse;
			qboolean alive;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}

			if (cg.tdmScore.version >= 3) {
				alive = ts->alive;
			} else {
				alive = sp->alive;
			}

			if (alive) {
				if (sp->team == TEAM_RED) {
					*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_blue");
				}
				return "";
			}

			return "";
		}
		case 4:
			if (cgs.clientinfoOrig[sp->client].countryFlag) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			}
			break;
		case 5:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol == PROTOCOL_Q3) {
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				CG_LimitText(s, CA_TEXT_LIMIT);
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^1%d  ^7%s ^7%s", info->skillRating, clanTag, info->name);
					} else {
						s = va("^1%d  ^7%s", info->skillRating, info->name);
					}
					CG_LimitText(s, CA_TEXT_LIMIT);
					return s;
				} else {
					if (*clanTag) {
						s = va("^7%s ^7%s", clanTag, info->name);
					} else {
						s = va("^7%s", info->name);
					}
					CG_LimitText(s, CA_TEXT_LIMIT);
					return s;
				}
			}
			break;
		case 6: {
			int score;

			if (cg.tdmScore.version >= 3) {
				score = ts->score;
			} else {
				score = sp->score;
			}

			return va("%d", score);
		}
		case 7:  // net
			if (cgs.cpma) {
				s = va("%d", sp->net);
				return s;
			}
			if (cg.tdmScore.version >= 3) {
				return va("%d/%d", ts->kills, ts->deaths);
			} else {
				return va("%d/%d", sp->frags, sp->deaths);
			}
		case 8:
			if (cg.tdmScore.valid  &&  ts->valid) {
				n = ts->damageDone;
				if (n >= 1000) {
					s = va("%.1fk", (float)n / 1000.0);
				} else {
					s = va("%d", n);
				}
				return s;
			} else {
				return "-";
			}
			break;
		case 9:
			if (cg.tdmScore.valid  &&  ts->valid  &&  ts->damageDone == 0) {
				return "";
			}

			if (cg.tdmScore.version >= 3) {
				*handle = cg_weapons[ts->bestWeapon].weaponIcon;
			} else {
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
			}
			return "";
		case 10:
			if (!cg.tdmScore.valid  ||  !ts->valid) {
				return va("%d%%", sp->accuracy);
			}

			if (ts->damageDone == 0) {
				return "";
			}

			if (cg.tdmScore.version >= 3) {
				return va("%d%%", ts->bestWeaponAccuracy);
			} else {
				return va("%d%%", ts->accuracy);
			}
		case 11:
			if (cg.tdmScore.version >= 3) {
				return va("%d", ts->ping);
			} else {
				return va("%d", sp->ping);
			}
		default:
			//Com_Printf("wtf tdmcolumn %d\n", column);
			break;
		}
	}

	return "";
}

#define TDM_TEXT_LIMIT 20

static const char *CG_FeederItemTextTdm (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	int n;
	const tdmPlayerScore_t *ts;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextTdm() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	ts = &cg.tdmScore.playerScore[sp->client];

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				int bestWeapon;

				if (cg.tdmScore.version >= 3) {
					bestWeapon = ts->bestWeapon;
				} else {
					bestWeapon = sp->bestWeapon;
				}

				*handle = cg_weapons[bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1: {
			int subscriber;

			if (cg.tdmScore.version >= 3) {
				subscriber = ts->subscriber;
			} else {
				subscriber = info->premiumSubscriber;
			}
			//if (cg.tdmScore.playerScore[sp->client].subscriber  &&  cg_scoreBoardStyle.integer == 0) {
			if (subscriber  &&  cg_scoreBoardStyle.integer == 0) {
				*handle = cgs.media.premiumIcon;
			}
			return "";
		}
		case 2:
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";

		case 3: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
			}

			return "";
		}
		case 4:
			if (cgs.clientinfoOrig[sp->client].countryFlag) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			}
			break;
		case 5:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol == PROTOCOL_Q3) {
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				CG_LimitText(s, TDM_TEXT_LIMIT);
				return s;
			} else {
				int accuracy;

				switch (cg.tdmScore.version) {
				case 3:
					accuracy = ts->accuracy;
					break;
				default:
					accuracy = sp->accuracy;
					break;
				}

				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", accuracy, info->skillRating, info->name);
					}
					CG_LimitText(s, TDM_TEXT_LIMIT);
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", accuracy, info->name);
					}
					CG_LimitText(s, TDM_TEXT_LIMIT);
					return s;
				}
			}
			break;
		case 6: {
			int score;

			if (cg.tdmScore.version >= 3) {
				score = ts->score;
			} else {
				score = sp->score;
			}

			return va("%d", score);
		}
		case 7:  // net
			if (cgs.cpma) {
				s = va("%d", sp->net);
				return s;
			}
			switch (cg.tdmScore.version) {
			case 1:
			case 2:
				s = va("%d", sp->frags - sp->deaths - ts->tks + ts->tkd);
				break;
			case 3:
				s = va("%d", ts->kills - ts->deaths - ts->tks + ts->tkd);
				break;
			default:
				s = va("%d", sp->frags - sp->deaths);
				break;
			}
			return s;
		case 8:
			if (cg.tdmScore.valid  &&  ts->valid) {
				n = ts->damageDone;
				if (n >= 1000) {
					s = va("%.1fk", (float)n / 1000.0);
				} else {
					s = va("%d", n);
				}
				return s;
			} else {
				return "-";
			}
			break;
		case 9: {
			int ping;

			if (cg.tdmScore.version >= 3) {
				ping = ts->ping;
			} else {
				ping = sp->ping;
			}
			return va("%d", ping);
		}
		default:
			//Com_Printf("wtf tdmcolumn %d\n", column);
			break;
		}
	}

	return "";
}

#define CTF_TEXT_LIMIT 15

static const char *CG_FeederItemTextCtf (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	const ctfPlayerScore_t *ts;
	int clientTeam;
	int version;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextCtf() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	ts = &cg.ctfScore.playerScore[sp->client];

	version = cg.ctfScore.version;
	if (version >= 1) {
		clientTeam = ts->team;
	} else {
		clientTeam = sp->team;
	}

	if (clientTeam != team) {
		return "";
	}

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				if (version >= 1) {
					*handle = cg_weapons[ts->bestWeapon].weaponIcon;
				} else {
					*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				}
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1: {
			//if (cg.ctfScore.playerScore[sp->client].subscriber  &&  cg_scoreBoardStyle.integer == 0) {
			if (info->premiumSubscriber  &&  cg_scoreBoardStyle.integer == 0) {
				*handle = cgs.media.premiumIcon;
			}
			return "";
		}
		case 2:
			if (info->handicap < 100) {
				return va("%d%%", info->handicap);
			}
			return "";
		case 3: {
			qboolean ready = qfalse;
			int powerups;

			if (version >= 1) {
				powerups = ts->powerups;
			} else {
				powerups = sp->powerups;
			}

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup  ||  cg.snap->ps.pm_type == PM_INTERMISSION) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			} else if (powerups & ( 1 << PW_REDFLAG )) {
					*handle = cgs.media.pickup_iconredflag;
			} else if (powerups & ( 1 << PW_BLUEFLAG )) {
					*handle = cgs.media.pickup_iconblueflag;
			}

			return "";
		}
		case 4:
			if (cgs.clientinfoOrig[sp->client].countryFlag) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			}
			break;
		case 5:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol == PROTOCOL_Q3) {
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
					//Com_Printf("'%s'\n", s);
				} else {
					s = va("%s", info->name);
				}
				CG_LimitText(s, CTF_TEXT_LIMIT);
				return s;
			} else {
				int accuracy;

				if (version >= 1) {
					accuracy = ts->accuracy;
				} else {
					accuracy = sp->accuracy;
				}

				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", accuracy, info->skillRating, info->name);
					}
					CG_LimitText(s, CTF_TEXT_LIMIT);
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", accuracy, info->name);
					}
					CG_LimitText(s, CTF_TEXT_LIMIT);
					return s;
				}
			}
			break;
		case 6:
			if (version >= 1) {
				return va("%d", ts->score);
			} else {
				return va("%d", sp->score);
			}
		case 7:  // net
			if (cgs.cpma) {
				if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
					return "-";
				}
				return va("%d", sp->net);
			}
			if (version >= 1) {
				return va("%d/%d", ts->kills, ts->deaths);
			} else {
				return va("%d/%d", sp->frags, sp->deaths);
			}
		case 8:
			if (version >= 1) {
				return va("%d", ts->captures);
			} else {
				return va("%d", sp->captures);
			}
		case 9:
			if (cgs.cpma) {
				return "-";
			}
			if (version >= 1) {
				return va("%d", ts->awardAssist);
			} else {
				return va("%d", sp->assistCount);
			}
		case 10:
			if (version >= 1) {
				return va("%d", ts->awardDefend);
			} else {
				return va("%d", sp->defendCount);
			}
		case 11:
			if (version >= 1) {
				return va("%d", ts->ping);
			} else {
				return va("%d", sp->ping);
			}
		default:
			//Com_Printf("wtf ctfcolumn %d\n", column);
			break;
		}
	}

	return "";
}

static const char *CG_FeederItemTextCaStats (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	const caStats_t *ts;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_STATS) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_STATS) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextCaStats() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	//ts = &cg.tdmStats[sp->client];
	ts = &cg.caStats[scoreIndex];
	if (sp->team != team) {
		//return "";
	}

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			clanTag = info->clanTag;
			if (*clanTag) {
				s = va("^7%s ^7%s", clanTag, info->name);
			} else {
				s = va("%s", info->name);
			}
			return s;
		case 1: {
			return va("^3%d", sp->score);
		}
		case 2: {
			return va("%d", sp->frags);
		}
		case 3:
			return va("%d", sp->deaths);
		case 4:
			return va("%d", ts->damageDone);
		case 5:
			return va("%d", ts->damageReceived);
		case 6:
			return va("%d%%", sp->accuracy);
		case 7:
			return va("%d", ts->gauntKills);
		case 8:
			return va("^3%d ^7%d%%", ts->mgKills, ts->mgAccuracy);
		case 9:
			return va("^3%d ^7%d%%", ts->sgKills, ts->sgAccuracy);
		case 10:
			return va("^3%d ^7%d%%", ts->glKills, ts->glAccuracy);
		case 11:
			return va("^3%d ^7%d%%", ts->rlKills, ts->rlAccuracy);
		case 12:
			return va("^3%d ^7%d%%", ts->lgKills, ts->lgAccuracy);
		case 13:
			return va("^3%d ^7%d%%", ts->rgKills, ts->rgAccuracy);
		case 14:
			return va("^3%d ^7%d%%", ts->pgKills, ts->pgAccuracy);
		case 15:
			return va("%d", sp->time);
		default:
			break;
		}
	}

	return "";
}

static const char *CG_FeederItemTextTdmStats (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	const tdmStats_t *ts;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_STATS) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_STATS) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextTdmStats() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	//ts = &cg.tdmStats[sp->client];
	ts = &cg.tdmStats[scoreIndex];
	if (sp->team != team) {
		return "";
	}

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			clanTag = info->clanTag;
			if (*clanTag) {
				s = va("^7%s ^7%s", clanTag, info->name);
			} else {
				s = va("%s", info->name);
			}
			return s;
		case 1: {
			return va("^3%d", sp->score);
		}
		case 2: {
			return va("%d", sp->frags);
		}
		case 3:
			//return va("%d", sp->deaths - ts->selfKill - ts->tks);
			return va("%d", sp->deaths - ts->selfKill - ts->tkd);
		case 4:
			return va("%d", ts->selfKill);
		case 5:
			return va("%d", ts->tks);
		case 6:
			return va("%d", ts->tkd);
		case 7:
			return va("%d", sp->frags - sp->deaths - ts->tks + ts->tkd);
		case 8:
			return va("%d", ts->damageDone);
		case 9:
			return va("%d", ts->damageReceived);
		case 10:
			return va("%d%%", sp->accuracy);
		case 11:
			return va("%d", ts->ra);
		case 12:
			return va("%d", ts->ya);
		case 13:
			return va("%d", ts->ga);
		case 14:
			return va("%d", ts->mh);
		case 15:
			return va("%d", ts->quad);
		case 16:
			return va("%d", ts->bs);
		case 17:
			return va("%d", sp->time);
		default:
			break;
		}
	}

	return "";
}

static const char *CG_FeederItemTextCtfStats (float feederID, int index, int column, qhandle_t *handle)
{
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	const char *clanTag;
	const ctfStats_t *ts;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_STATS) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_STATS) {
		team = TEAM_BLUE;
	} else {
		Com_Printf("CG_FeederItemTextCtfStats() unknown feeder id 0x%x\n", (int)feederID);
		return "";
	}

	//Com_Printf("xxx\n");

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);

	sp = &cg.scores[scoreIndex];
	//ts = &cg.tdmStats[sp->client];
	ts = &cg.ctfStats[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			clanTag = info->clanTag;
			if (*clanTag) {
				s = va("^7%s ^7%s", clanTag, info->name);
			} else {
				s = va("%s", info->name);
			}
			return s;
		case 1: {
			return va("^3%d", sp->score);
		}
		case 2: {
			return va("%d", sp->frags);
		}
		case 3:
			//return va("%d", sp->deaths - ts->selfKill - ts->tks);
			//return va("%d", sp->deaths - ts->selfKill - ts->tkd);
			return va("%d", sp->deaths);
		case 4:
			return va("%d", ts->selfKill);
		case 5:
			return va("%d", sp->frags - sp->deaths);  // net
		case 6:
			return va("%d", ts->damageDone);
		case 7:
			return va("%d", ts->damageReceived);
		case 8:
			return va("%d%%", sp->accuracy);
		case 9:
			return va("%d", ts->ra);
		case 10:
			return va("%d", ts->ya);
		case 11:
			return va("%d", ts->ga);
		case 12:
			return va("%d", ts->mh);
		case 13:
			return va("%d", ts->quad);
		case 14:
			return va("%d", ts->bs);
		case 15:
			return va("%d", ts->regen);
		case 16:
			return va("%d", ts->haste);
		case 17:
			return va("%d", ts->invis);
		case 18:
			return va("%d", sp->time);
		default:
			break;
		}
	}

	return "";
}

static const char *CG_FeederItemTextFreezetag (float feederID, int index, int column, qhandle_t *handle)
{
	//gitem_t *item;
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	//int clientNum;
	const char *clanTag;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	//Com_Printf("%d %d %s %d\n", index, sp->client, info->name, info->team);

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else if (cgs.clientinfoOrig[sp->client].countryFlag  &&  cg_scoreBoardStyle.integer == 2) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
#if 0
		case 1:
			if (cg_scoreBoardStyle.integer == 0  &&  cgs.protocol == PROTOCOL_QL) {
				if (cgs.clientinfoOrig[sp->client].premiumSubscriber) {
					*handle = cgs.media.premiumIcon;
				}
			}
			return "";
#endif
		case 2: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}

			if (sp->alive) {
				if (sp->team == TEAM_RED) {
					*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_blue");
				}
				return "";
			}
			return "";
		}
		case 1:
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";
		case 3:
			//*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			if (cg_scoreBoardStyle.integer == 0) {
				//return info->name;
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", sp->accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", sp->accuracy, info->skillRating, info->name);
					}
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", sp->accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", sp->accuracy, info->name);
					}
					return s;
				}
			}
			break;
		case 4:
			return va("%d", sp->score);
		case 5:
			return va("%d", sp->frags);
		case 6:  // thaws
			//return va("%d", sp->deaths);
			//Com_Printf("%s scoreflags %d  defendCount %d  assistCount %d  captures %d\n", info->name, sp->scoreFlags, sp->defendCount, sp->assistCount, sp->captures);
			return va("%d", sp->assistCount);
		case 7:
			return va("%d", sp->deaths);
		case 8:
			return va("%d", sp->ping);
		default:
			return "xxx";
		}
	}

	return "";
}

static const char *CG_FeederItemTextFfa (float feederID, int index, int column, qhandle_t *handle)
{
	//gitem_t *item;
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	//int clientNum;
	const char *clanTag;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	//Com_Printf("%d %d %s %d\n", index, sp->client, info->name, info->team);

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else if (cgs.clientinfoOrig[sp->client].countryFlag  &&  cg_scoreBoardStyle.integer == 2) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1:
			if (cg_scoreBoardStyle.integer == 0  &&  cgs.protocol == PROTOCOL_QL) {
				if (cgs.clientinfoOrig[sp->client].premiumSubscriber) {
					*handle = cgs.media.premiumIcon;
				}
			}
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";
		case 2: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}

			return "";
		}
		case 3:
			//*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			if (cg_scoreBoardStyle.integer == 0) {
				//return info->name;
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", sp->accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", sp->accuracy, info->skillRating, info->name);
					}
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", sp->accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", sp->accuracy, info->name);
					}
					return s;
				}
			}
			break;
		case 4:
			//return "nm4";
			return "";
		case 5:
			return va("%d", sp->score);
		case 6:
			return va("%d", sp->frags);
		case 7:
			return va("%d", sp->deaths);
		case 8:
			return va("%d", sp->time);
		case 9:
			return va("%d", sp->ping);
		default:
			return "xxx";
		}

		//return "xxx";
	}

	return "";
}

static const char *CG_FeederItemTextRedRover (float feederID, int index, int column, qhandle_t *handle)
{
	//gitem_t *item;
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	//int clientNum;
	const char *clanTag;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	//Com_Printf("%d %d %s %d\n", index, sp->client, info->name, info->team);

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else if (cgs.clientinfoOrig[sp->client].countryFlag  &&  cg_scoreBoardStyle.integer == 2) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1:
			if (cg_scoreBoardStyle.integer == 0  &&  cgs.protocol == PROTOCOL_QL) {
				if (cgs.clientinfoOrig[sp->client].premiumSubscriber) {
					*handle = cgs.media.premiumIcon;
				}
			}
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";
		case 2: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}
			if (info->team == TEAM_RED) {
				*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			} else if (info->team == TEAM_BLUE) {
				*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_blue");
			}
			return "";
		}
		case 3:
			//*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			if (cg_scoreBoardStyle.integer == 0) {
				//return info->name;
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", sp->accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", sp->accuracy, info->skillRating, info->name);
					}
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", sp->accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", sp->accuracy, info->name);
					}
					return s;
				}
			}
			break;
		case 4:
			//return "nm4";
			return "";
		case 5:
			return va("%d", sp->score);
		case 6:
			return va("%d", sp->frags);
		case 7:
			return va("%d", sp->deaths);
		case 8:
			return va("%d", sp->time);
		case 9:
			return va("%d", sp->ping);
		default:
			return "xxx";
		}

		//return "xxx";
	}

	return "";
}

static const char *CG_FeederItemTextRace (float feederID, int index, int column, qhandle_t *handle)
{
	//gitem_t *item;
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	//int clientNum;
	const char *clanTag;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	//Com_Printf("%d %d %s %d\n", index, sp->client, info->name, info->team);

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else if (cgs.clientinfoOrig[sp->client].countryFlag  &&  cg_scoreBoardStyle.integer == 2) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1:
			if (cg_scoreBoardStyle.integer == 0  &&  cgs.protocol == PROTOCOL_QL) {
				if (cgs.clientinfoOrig[sp->client].premiumSubscriber) {
					*handle = cgs.media.premiumIcon;
				}
			}
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";
		case 2: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}
			if (info->team == TEAM_RED) {
				*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			} else if (info->team == TEAM_BLUE) {
				*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_blue");
			}
			return "";
		}
		case 3:
			//*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			if (cg_scoreBoardStyle.integer == 0) {
				//return info->name;
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", sp->accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", sp->accuracy, info->skillRating, info->name);
					}
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", sp->accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", sp->accuracy, info->name);
					}
					return s;
				}
			}
			break;
		case 4:
			return CG_GetTimeString(sp->score);
		case 5:
			return va("%d", sp->time);
		case 6:
			return va("%d", sp->ping);
		default:
			return "xxx";
		}

		//return "xxx";
	}

	return "";
}

static const char *CG_FeederItemText (float feederID, int index, int column, qhandle_t *handle)
{
	//gitem_t *item;
	int scoreIndex = 0;
	const clientInfo_t *info = NULL;
	int team = -1;
	const score_t *sp = NULL;
	char *s;
	//int clientNum;
	const char *clanTag;

	if (cgs.gametype == GT_CA  &&  !cg_scoreBoardOld.integer) {
		if (feederID == FEEDER_REDTEAM_STATS  ||  feederID == FEEDER_BLUETEAM_STATS) {
			return CG_FeederItemTextCaStats(feederID, index, column, handle);
		} else {
			return CG_FeederItemTextCa(feederID, index, column, handle);
		}
	}
	if (cgs.gametype == GT_TEAM  &&  !cg_scoreBoardOld.integer) {
		if (feederID == FEEDER_REDTEAM_STATS  ||  feederID == FEEDER_BLUETEAM_STATS) {
			return CG_FeederItemTextTdmStats(feederID, index, column, handle);
		} else {
			return CG_FeederItemTextTdm(feederID, index, column, handle);
		}
	}
	if ((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_DOMINATION)  &&  !cg_scoreBoardOld.integer) {
		if (feederID == FEEDER_REDTEAM_STATS  ||  feederID == FEEDER_BLUETEAM_STATS) {
			return CG_FeederItemTextCtfStats(feederID, index, column, handle);
		} else {
			return CG_FeederItemTextCtf(feederID, index, column, handle);
		}
	}
	if (cgs.gametype == GT_FREEZETAG  &&  !cg_scoreBoardOld.integer) {
		return CG_FeederItemTextFreezetag(feederID, index, column, handle);
	}

	if (cgs.gametype == GT_FFA  &&  !cg_scoreBoardOld.integer) {
		return CG_FeederItemTextFfa(feederID, index, column, handle);
	}

	if (cgs.gametype == GT_RED_ROVER) {  // no old scoreboard suitable
		return CG_FeederItemTextRedRover(feederID, index, column, handle);
	}

	if (cgs.gametype == GT_RACE) {  //FIXME old scoreboard
		return CG_FeederItemTextRace(feederID, index, column, handle);
	}

	// duel

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	//Com_Printf("%d %d %s %d\n", index, sp->client, info->name, info->team);
	//FIXME duel, ffa different

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if (cg_scoreBoardStyle.integer == 0  ||  cgs.protocol != PROTOCOL_QL) {
				*handle = cgs.clientinfoOrig[sp->client].modelIcon;
			} else if (cgs.clientinfoOrig[sp->client].countryFlag  &&  cg_scoreBoardStyle.integer == 2) {
				*handle = cgs.clientinfoOrig[sp->client].countryFlag;
			} else {  // default is cg_scoreBoardStyle.integer == 1
				*handle = cg_weapons[sp->bestWeapon].weaponIcon;
				if (!*handle) {
					//*handle = cgs.media.redCubeIcon;
				}
			}
			return "";
		case 1:
			if (cg_scoreBoardStyle.integer == 0  &&  cgs.protocol == PROTOCOL_QL) {
				if (cgs.clientinfoOrig[sp->client].premiumSubscriber) {
					*handle = cgs.media.premiumIcon;
				}
			}
			if (info->handicap < 100) {
				return va("^3%d%%", info->handicap);
			}
			return "";
		case 2: {
			qboolean ready = qfalse;

			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				ready = qtrue;
			}
			if (cg.warmup) {
				if (ready) {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowg");
				} else {
					*handle = trap_R_RegisterShader("ui/assets/score/arrowr");
				}
				return "";
			}

			if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS) {
				if (sp->alive) {
					//*handle = trap_R_RegisterShader("ui/assets/score/arrow.png");
					//*handle = trap_R_RegisterShader("ui/assets/score/arrow");
					if (sp->team == TEAM_RED) {
						*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
					} else {
						*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_blue");
					}
					return "";
				}
			}
			if (cg_scoreBoardStyle.integer == 0) {
				if (info->handicap < 100) {
					//FIXME spacing
					return va("%d", info->handicap);
				}
			} else if (0) {  //(cg_scoreBoardStyle.integer == 1) {
				int t;

				t = cg.time / 1000;
				if (info->handicap < 100) {
					if (t % 2 == 0) {
						return va("%d", info->handicap);
					}
					else {
						return va("%d", sp->accuracy);
					}
				}
				return va("%d", sp->accuracy);
			}
			return "";
		}
		case 3:
			//*handle = trap_R_RegisterShader("ui/assets/score/ca_arrow_red");
			if (cg_scoreBoardStyle.integer == 0) {
				//return info->name;
				clanTag = info->clanTag;
				if (*clanTag) {
					s = va("^7%s ^7%s", clanTag, info->name);
				} else {
					s = va("%s", info->name);
				}
				return s;
			} else {
				clanTag = info->clanTag;
				if (info->knowSkillRating) {
					if (*clanTag) {
						s = va("^3%3d   ^1%d  ^7%s ^7%s", sp->accuracy, info->skillRating, clanTag, info->name);
					} else {
						s = va("^3%3d   ^1%d  ^7%s", sp->accuracy, info->skillRating, info->name);
					}
					return s;
				} else {
					if (*clanTag) {
						s = va("^3%3d   ^7%s ^7%s", sp->accuracy, clanTag, info->name);
					} else {
						s = va("^3%3d   ^7%s", sp->accuracy, info->name);
					}
					return s;
				}
			}
			break;
		case 4:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d/%d", info->wins, info->losses);
			} else if (cgs.gametype == GT_FFA) {
				return "";
			}

			return va("%d", sp->score);
		case 5:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d", sp->score);
			} else if (cgs.gametype == GT_FFA) {
				return va("%d", sp->score);
			}

			return va("%d", sp->frags);
		case 6:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d", sp->frags);
			} else if (cgs.gametype == GT_FFA) {
				return va("%d", sp->frags);
			}

			return va("%d", sp->deaths);
		case 7:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d", sp->deaths);
			} else if (cgs.gametype == GT_FFA) {
				return va("%d", sp->deaths);
			}

			return va("%d", sp->ping);
		case 8:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d", sp->time);
			} else if (cgs.gametype == GT_FFA) {
				return va("%d", sp->time);
			}

			return "7";
		case 9:
			if (cgs.gametype == GT_TOURNAMENT) {
				return va("%d", sp->ping);
			} else if (cgs.gametype == GT_FFA) {
				return va("%d", sp->ping);
			}

			return "8";
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	Com_Printf("FIXME CG_FeederItemImage(%f, %d)\n", feederID, index);
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}
#endif

#if 1  //def MPACK // bk001204 - only needed there
float CG_Cvar_Get (const char *cvar)
{
	char buff[128];

	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}
#endif

#if 1  //def MPACK
static void CG_Text_PaintWithCursor(float x, float y, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int fontIndex) {
	 const fontInfo_t *font;

	 if (fontIndex <= 0) {
		 font = &cgDC.Assets.textFont;
	 } else {
		 font = &cgDC.Assets.extraFonts[fontIndex];
	 }

	 CG_Text_Paint(x, y, scale, color, text, 0, limit, style, font);

	 Com_Printf("FIXME cursor\n");
	 //FIXME cursor
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale, int fontIndex) {
	 const fontInfo_t *font;

	 if (fontIndex <= 0) {
		 font = &cgDC.Assets.textFont;
	 } else {
		 font = &cgDC.Assets.extraFonts[fontIndex];
	 }

	switch (ownerDraw) {
	  case CG_GAME_TYPE:
		  return CG_Text_Width(CG_GameTypeString(), scale, 0, font);
      case CG_GAME_STATUS: {
		  vec4_t color = { 1, 1, 1, 1 };

		  return CG_Text_Width(CG_GetGameStatusText(color), scale, 0, font);
			break;
	  }
	  case CG_KILLER:
		  return CG_Text_Width(CG_GetKillerText(), scale, 0, font);
			break;
	  case CG_RED_NAME:
		  //return CG_Text_Width(cg_redTeamName.string, scale, 0, font);
		  return CG_Text_Width(cgs.redTeamName, scale, 0, font);
			break;
	  case CG_BLUE_NAME:
		  //return CG_Text_Width(cg_blueTeamName.string, scale, 0, font);
		  return CG_Text_Width(cgs.blueTeamName, scale, 0, font);
			break;
	default:
		Com_Printf("CG_OwnerDrawWidth() unknown ownerDraw %d\n", ownerDraw);
		break;
	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}

/*
=================
CG_InitDC();

=================
*/
static void CG_InitDC (void)
{
	//char buff[1024];
	//const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint_old;
	cgDC.textWidth = &CG_Text_Width_old;
	cgDC.textHeight = &CG_Text_Height_old;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &CG_AddRefEntity;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.cvarExists = trap_Cvar_Exists;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error;
	cgDC.Print = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;

	Init_Display(&cgDC);

	Menu_Reset();


#if 0
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
#endif
}

static void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif

void CG_ResetTimedItemPickupTimes (void)
{
	int val;
	int i;

	val = -999999;
	for (i = 0;   i < cg.numMegaHealths;  i++) {
		cg.megaHealths[i].specPickupTime = 0;
		cg.megaHealths[i].pickupTime = val;
		cg.megaHealths[i].respawnTime = val;
		cg.megaHealths[i].clientNum = -1;
	}
	for (i = 0;   i < cg.numRedArmors;  i++) {
		cg.redArmors[i].specPickupTime = 0;
		cg.redArmors[i].pickupTime = val;
		cg.redArmors[i].respawnTime = val;
		cg.redArmors[i].clientNum = -1;
	}
	for (i = 0;   i < cg.numYellowArmors;  i++) {
		cg.yellowArmors[i].specPickupTime = 0;
		cg.yellowArmors[i].pickupTime = val;
		cg.yellowArmors[i].respawnTime = val;
		cg.yellowArmors[i].clientNum = -1;
	}
	for (i = 0;   i < cg.numGreenArmors;  i++) {
		cg.greenArmors[i].specPickupTime = 0;
		cg.greenArmors[i].pickupTime = val;
		cg.greenArmors[i].respawnTime = val;
		cg.greenArmors[i].clientNum = -1;
	}
	for (i = 0;   i < cg.numQuads;  i++) {
		cg.quads[i].specPickupTime = 0;
		cg.quads[i].pickupTime = val;
		cg.quads[i].respawnTime = val;
	}
	for (i = 0;   i < cg.numBattleSuits;  i++) {
		cg.battleSuits[i].specPickupTime = 0;
		cg.battleSuits[i].pickupTime = val;
		cg.battleSuits[i].respawnTime = val;
	}
}

void CG_CreateScoresFromClientInfo (void)
{
	int i;

	cg.numScores = 0;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		const clientInfo_t *ci;
		score_t *s;

		//cgs.newConnectedClient[i] = qfalse;

		ci = &cgs.clientinfo[i];
		if (!ci->infoValid) {
			continue;
		}
		s = &cg.scores[cg.numScores];
		s->client = i;
		s->team = ci->team;
		s->bestWeapon = 2;  //FIXME check
		s->alive = qfalse;

		cg.numScores++;
	}
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
static void CG_Init (int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback)
{
	const char	*s;
	int i;
	//char mname[MAX_STRING_CHARS];
	char buff[1024];
	const char *hudSet;
	const gitem_t *item;
	qboolean val;

	trap_Cvar_Set("com_errorMessage", "");

	Com_Printf("CG_Init() version: %s\n", WOLFCAM_VERSION);

	Com_Printf("CG_Init()  serverMessageNum %d  serverCommandSequence %d  clientNum %d\n", serverMessageNum, serverCommandSequence, clientNum);

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );

	cg.demoPlayback = demoPlayback;
	cgs.protocol = SC_Cvar_Get_Int("protocol");
	if (cgs.protocol >= 66  &&  cgs.protocol <= 71) {
		cgs.protocol = PROTOCOL_Q3;
	}

	//Com_Printf("^3ctfs  %d\n", GT_CTFS);

	if (cgs.protocol == PROTOCOL_Q3) {  //FIXME hack
		// start as baseq3
		memcpy(&bg_itemlist, &bg_itemlistQ3, sizeof(gitem_t) * bg_numItemsQ3);
		bg_numItems = bg_numItemsQ3;

#if 0
		GT_FFA = 0;
		GT_TOURNAMENT = 1;
		GT_SINGLE_PLAYER = 2;
		GT_TEAM = 3;
		GT_CTF = 4;
		GT_1FCTF = 1005;  // this shit -- get rid of
		GT_OBELISK = 1006;
		GT_HARVESTER = 1007;
		GT_CA = 1008;
		GT_FREEZETAG = 1009;
		GT_CTFS = 1010;
#endif

		STAT_HEALTH = 0;
		STAT_HOLDABLE_ITEM = 1;
		STAT_WEAPONS = 2;
		STAT_ARMOR = 3;
		//STAT_DEAD_YAW = 4;
		STAT_CLIENTS_READY = 5;
		STAT_MAX_HEALTH = 6;

		PERS_SCORE = 0;
		PERS_HITS = 1;
		PERS_RANK = 2;
		PERS_TEAM = 3;
		PERS_SPAWN_COUNT = 4;
		PERS_PLAYEREVENTS = 5;
		PERS_ATTACKER = 6;
		PERS_ATTACKEE_ARMOR = 7;
		PERS_KILLED = 8;
		PERS_IMPRESSIVE_COUNT = 9;
		PERS_EXCELLENT_COUNT = 10;
		PERS_DEFEND_COUNT = 11;
		PERS_ASSIST_COUNT = 12;
		PERS_GAUNTLET_FRAG_COUNT = 13;
		PERS_CAPTURES = 14;
	}

	// fx scripting uses cent->currentState.clientNum and it can happen
	// before a player is added
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cg_entities[i].currentState.clientNum = i;
		cg_entities[i].currentState.number = i;
	}

#if 0
	// testing
#if 0
	for (i = 0;  i < sizeof(cg_entities);  i++) {
		byte *p;

		p = (byte *)cg_entities;
		p[i] = rand() % 256;
	}
#endif
	memset(cg_entities, 255, sizeof(cg_entities));
	Com_Printf("sizeof cg_entities %d\n", sizeof(cg_entities));
#endif

#if 0
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );
#endif

	val = SC_Cvar_Get_Int("com_qlcolors");
	Q_SetColors(val);
	cg.qlColors = val;

	cg.clientNum = clientNum;

	cgs.firstServerMessageNum = serverMessageNum;
	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cg.serverAccuracyStatsClientNum = -1;
	cg.lastWeapon = -1;
	cg.serverFrameTime = 99999;
	cg.viewEnt = -1;
	cg.chaseEnt = -1;
	cg.selectedCameraPointField = 3;
	// CHRUKER: b079 - Background images on the loading screen were not visible on the first call
	//trap_R_SetColor(NULL);  // crosshair color problem

	//Com_Printf("CG_Init()\n");

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		// 48 * 48 * 32 * 4
		//cgs.clientNameImage[i] = trap_RegisterShaderFromData(va("clientName%d", i), (byte *)&cg, MAX_QPATH * 48, 48, qfalse, qfalse, GL_CLAMP_TO_EDGE, LIGHTMAP_2D);
		cgs.clientNameImage[i] = trap_RegisterShaderFromData(va("clientName%d", i), (byte *)&cg, 48 * MAX_QPATH, 48, qfalse, qfalse, GL_CLAMP_TO_EDGE, LIGHTMAP_2D);
		//cgs.clientNameImage[i] = trap_RegisterShaderFromData(va("clientName%d", i), (byte *)&cg, 16, 16, qfalse, qfalse, GL_CLAMP_TO_EDGE, LIGHTMAP_2D);
	}

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;


	trap_SendConsoleCommandNow("exec cgameinit.cfg\n");

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	cgs.rocketSpeed = 900;
	if (cgs.protocol == PROTOCOL_Q3) {
		cgs.rocketSpeed = 800;
		if (!Q_stricmp("cpma-1", CG_ConfigString(CS_GAME_VERSION))) {  //FIXME hack
			cgs.cpma = qtrue;
			memcpy(&bg_itemlist, &bg_itemlistCpma, sizeof(gitem_t) * bg_numItemsCpma);
			bg_numItems = bg_numItemsCpma;
			//GT_CTFS = 7;

			STAT_ARMOR_TIER = 9;

			Com_Printf("^5cpma\n");
		} else if (!Q_stricmp("osp", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "gamename"))  ||  !Q_stricmpn(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "gameversion"), "osp", strlen("osp") - 1)) {
			cgs.osp = qtrue;
			cgs.ospEncrypt = atoi(CG_ConfigString(872));
			memcpy(&bg_itemlist, &bg_itemlistCpma, sizeof(gitem_t) * bg_numItemsCpma);
			bg_numItems = bg_numItemsCpma;
			Com_Printf("^5osp %s\n", cgs.ospEncrypt ? "(encrypted)" : "");
		} else if (!Q_stricmp("defrag", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "gamename"))) {
			cgs.defrag = qtrue;
		} else if (*Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "defrag_vers")) {
			cgs.defrag = qtrue;
		}
	}
	if (cgs.defrag) {
		Com_Printf("^5defrag\n");
	}

	//FIXME cpma
	//trap_Cvar_Set("cg_gametype", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_gametype"));

	if (demoPlayback) {
		// referenced in hud menu files
		trap_Cvar_Set("timelimit", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "timelimit"));
		trap_Cvar_Set("fraglimit", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "fraglimit"));
		trap_Cvar_Set("capturelimit", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "capturelimit"));
	}

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if (strcmp(s, GAME_VERSION)  &&  strcmp(s, "baseqz")) {
		//CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
        Com_Printf ("Client/Server game mismatch: %s/%s\n", GAME_VERSION, s);
        //trap_SendConsoleCommand ("quit\n");
        //return;
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo(qtrue);
	CG_ParseWarmup();
	if (cg.warmup) {
		if (cg.demoPlayback) {
			cg.demoHasWarmup = qtrue;
		}
		cg.warmupTimeStart = cgs.levelStartTime;
	}

	// load the new map
	CG_LoadingString( "collision map" );

	Com_Printf("cgame: load map %s\n", cgs.mapname);
	if (*SC_Cvar_Get_String("r_forceMap")) {
		Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", SC_Cvar_Get_String("r_forceMap"));
		Com_Printf("^3forcing %s\n", cgs.mapname);
	}
	trap_GetRealMapName(cgs.mapname, cgs.realMapName, sizeof(cgs.realMapName));
	//Com_Printf("^3map: '%s' -> '%s'\n", cgs.mapname, cgs.realMapName);
	Q_strncpyz(buff, cgs.realMapName, sizeof(buff));
	i = strlen(buff);
	buff[i - 1] = 'g';
	buff[i - 2] = 'f';
	buff[i - 3] = 'c';
	trap_SendConsoleCommand(va("exec %s\n", buff));
	trap_CM_LoadMap( cgs.mapname );

	//COM_StripExtension(cgs.mapname, mname, sizeof(mname));
	//trap_Cvar_Set("mapname", mname);

#if 1  //def MPACK
	String_Init();
#endif

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	{
		int i;
		const char *as[] = {
			"sound/items/poweruprespawn",
			"sound/items/kamikazerespawn",
			"sound/ambient/1shot_bell_01",  //FIXME map specific -- sinister.bsp
			"sound/movers/doors/dr1_strt",
			"sound/movers/doors/dr1_end",
			"sound/player/gurp1",
			"sound/player/gurp2",

			"sound/world/klaxon1.ogg",
			"sound/world/klaxon2.ogg",
			"sound/world/buzzer.ogg",
			"sound/world/bell_01.ogg",
			"sound/world/hockey_horn.ogg",
		};

		cg.numAllowedAmbientSounds = 0;
		for (i = 0;  i < ARRAY_LEN(as);  i++) {
			qhandle_t h;

			h = trap_S_RegisterSound(as[i], qfalse);
			cg.allowedAmbientSounds[cg.numAllowedAmbientSounds] = h;
			cg.numAllowedAmbientSounds++;

			Com_Printf("registered allowed ambient sound '%s' %d\n", as[i], h);
		}
	}

	CG_RegisterSounds();

	CG_RegisterFonts();  //FIXME better place

	CG_LoadingString( "graphics" );

	//Com_Printf("^1%d -> %d\n", cgs.gametype, GT_RED_ROVER);

	// quakelive custom game modes, /devmap /give item..  just have to
	// always load all weapons so that projectiles will be present
	//FIXME ugh, terrible hack
	if (cgs.protocol == PROTOCOL_QL) {
	    for (item = bg_itemlist + 1;  item->classname;  item++) {
			if (item->giType != IT_WEAPON) {
				continue;
			}

			CG_LoadingItem(item - bg_itemlist);
			CG_RegisterWeapon(item->giTag);
			//Com_Printf("weapon %d '%s'\n", item->giTag, item->pickup_name);
			// for 'full weapon bar' need to know only held or available from
			// gametype/map
			cg_weapons[item->giTag].registered = qfalse;
		}
	}

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_DOMINATION  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER) {
		for (i = WP_GAUNTLET;  i < WP_BFG;  i++) {
			for (item = bg_itemlist + 1;  item->classname;  item++) {
				if (item->giType == IT_WEAPON  &&  item->giTag == i) {
					break;
				}
			}
			if (i >= WP_GAUNTLET  &&  i < WP_BFG  &&  item->classname) {
				CG_LoadingItem(item - bg_itemlist);
				CG_RegisterWeapon(i);
			}
		}
	} else {
		for (i = WP_GAUNTLET;  i < WP_NUM_WEAPONS;  i++) {
			// rail for freecam, lg for grapple, mg for player heights (orbb hack)
			if (i != WP_RAILGUN  &&  i != WP_LIGHTNING  &&  i != WP_MACHINEGUN) {
				continue;
			}
			for (item = bg_itemlist + 1;  item->classname;  item++) {
				if (item->giType == IT_WEAPON  &&  item->giTag == i) {
					break;
				}
			}
			if (i >= WP_GAUNTLET  &&  i < WP_BFG  &&  item->classname) {
				CG_LoadingItem(item - bg_itemlist);
				CG_RegisterWeapon(i);
			}
		}
	}

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred
	CG_CreateScoresFromClientInfo();
	CG_BuildSpectatorString();

#if 1  //def MPACK
	CG_AssetCache();
	//CG_LoadDefaultMenus();
	//CG_LoadHudMenu();      // load new hud stuff
#endif

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

#if 1 //def MPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );

#if 0
    if (!wc_logfile) {
        const char *fname = "shotdata.log";
        int ret;

#if 0
        // clear it first
        ret = trap_FS_FOpenFile (fname, &wc_logfile, FS_WRITE);
        if (!wc_logfile  ||  ret < 0) {
            CG_Printf ("Error clearing %s\n", fname);
        } else {
            trap_FS_FCloseFile (wc_logfile);
            ret = trap_FS_FOpenFile (fname, &wc_logfile, FS_APPEND_SYNC);
            if (!wc_logfile  ||  ret < 0)
                CG_Printf ("Error opening shotdata.log\n");
        }
#endif
        ret = trap_FS_FOpenFile (fname, &wc_logfile, FS_APPEND_SYNC);
        if (!wc_logfile  ||  ret < 0)
            CG_Printf ("Error opening shotdata.log\n");
    }
#endif

	if (cg.demoPlayback) {
		CG_EventHandling(CGAME_EVENT_DEMO);
		//trap_Key_SetCatcher(trap_Key_GetCatcher() | KEYCATCH_CGAME);
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}

	CG_ParseSpawnVars();
	CG_ResetTimedItemPickupTimes();

	//CG_LoadDefaultMenus();
	//FIXME before or after default menus?
#if 1  //def MPACK
	//CG_AssetCache();
	CG_InitDC();      // load new hud stuff
	CG_LoadDefaultMenus();
	//CG_LoadHudMenu();      // load new hud stuff
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
#endif

	cgs.lastConnectedDisconnectedPlayer = -1;
	cgs.needToCheckSkillRating = qfalse;

	if (cg.demoPlayback) {
		//trap_GetCurrentSnapshotNumber( &cg.latestSnapshotNum, &cg.latestSnapshotTime );
	}

	CG_ReloadQ3mmeScripts(cg_fxfile.string);
	CG_CreateNameSprites();
	if (cgs.protocol == PROTOCOL_QL  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cgs.gametype == GT_RED_ROVER) {
		CG_LoadInfectedGameTypeModels();
	}

	trap_SendConsoleCommand("condump cgameboot.log\n");  //FIXME this at end?

	if (SC_Cvar_Get_Int("cl_freezeDemo")) {
		cg.infoScreenText[0] = 0;
		cgs.processedSnapshotNum--;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		CG_SetDuelPlayers();
	}

	// create shotgun pattern
	{
		// from screenshot
		float pattern[20][2] = {
			// inner ring
			{ 0, 10 }, { 9, 5 }, { 9, -5 }, { 0, -10 }, { -9, -5 }, { -9, 5 },
			// middle ring
			{ 13, 16 }, { 21, -3 }, { 7, -20 }, { -15, -17 }, { -21, 3 }, { -8, 20 },
			// outter ring
			{ 0, 33 }, { 22, 22 }, { 33, 0 }, { 22, -22 }, { 0, -33 }, { -22, -22 }, { -33, 0 }, { -22, 22 }
		};
		float scale;

		scale = 40.0 / 39.0;  //38.0;

		for (i = 0;  i < 20;  i++) {
#if 1
			cg.shotgunPattern[i][0] = RAD2DEG(atan2(pattern[i][0] * scale, 292.82));
			cg.shotgunPattern[i][1] = RAD2DEG(atan2(pattern[i][1] * scale, 292.82));
#endif
#if 0
			cg.shotgunPattern[i][0] = atan2(pattern[i][0] * scale, 292.82);
			cg.shotgunPattern[i][1] = atan2(pattern[i][1] * scale, 292.82);
#endif
			//Com_Printf("%d  (%f, %f)\n", i, atan2(pattern[i][0] * scale, 292.82), atan2(pattern[i][1] * scale, 292.82));
		}
	}

	if (cg.demoPlayback) {
		trap_Get_Demo_Timeouts(&cgs.numTimeouts, cgs.timeOuts);
	}
}

void CG_LoadDefaultMenus (void)
{
	if (!cg_loadDefaultMenus.integer) {
		return;
	}

	Com_Printf("loading default menus\n");

	CG_ParseMenu("ui/intro.smenu");
	CG_ParseMenu("ui/ingamescoreteam.smenu");
	CG_ParseMenu("ui/ingamescorenoteam.smenu");
	CG_ParseMenu("ui/endscoreteam.smenu");
	CG_ParseMenu("ui/endscorenoteam.smenu");
	CG_ParseMenu("ui/spectator.smenu");
	CG_ParseMenu("ui/spectator_follow.smenu");
	CG_ParseMenu("ui/comp_spectator.smenu");
	CG_ParseMenu("ui/ingamestats.smenu");
	//CG_Load_Menu("ui/hud.menu");

	if (cgs.gametype == GT_FFA) {
		CG_ParseMenu("ui/ingame_scoreboard_ffa.smenu");
		CG_ParseMenu("ui/end_scoreboard_ffa.smenu");
	} else if (cgs.gametype == GT_TOURNAMENT) {
		CG_ParseMenu("ui/ingame_scoreboard_duel.smenu");
		CG_ParseMenu("ui/end_scoreboard_duel.smenu");
	} else if (cgs.gametype == GT_TEAM) {
		CG_ParseMenu("ui/ingame_scoreboard_tdm.smenu");
		CG_ParseMenu("ui/end_scoreboard_tdm.smenu");
	} else if (cgs.gametype == GT_CA) {
		CG_ParseMenu("ui/ingame_scoreboard_ca.smenu");
		CG_ParseMenu("ui/end_scoreboard_ca.smenu");
	} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
		CG_ParseMenu("ui/ingame_scoreboard_ctf.smenu");
		CG_ParseMenu("ui/end_scoreboard_ctf.smenu");
	} else if (cgs.gametype == GT_FREEZETAG) {
		CG_ParseMenu("ui/ingame_scoreboard_ft.smenu");
		CG_ParseMenu("ui/end_scoreboard_ft.smenu");
	} else if (cgs.gametype == GT_1FCTF) {
		CG_ParseMenu("ui/ingame_scoreboard_1fctf.smenu");
		CG_ParseMenu("ui/end_scoreboard_1fctf.smenu");
	} else if (cgs.gametype == GT_HARVESTER) {
		CG_ParseMenu("ui/ingame_scoreboard_har.smenu");
		CG_ParseMenu("ui/end_scoreboard_har.smenu");
	} else if (cgs.gametype == GT_DOMINATION) {
		CG_ParseMenu("ui/ingame_scoreboard_dom.smenu");
		CG_ParseMenu("ui/end_scoreboard_dom.smenu");
	} else if (cgs.gametype == GT_CTFS) {
		CG_ParseMenu("ui/ingame_scoreboard_ad.smenu");
		CG_ParseMenu("ui/end_scoreboard_ad.smenu");
	} else if (cgs.gametype == GT_RED_ROVER) {
		CG_ParseMenu("ui/ingame_scoreboard_ffa.smenu");
		CG_ParseMenu("ui/end_scoreboard_ffa.smenu");
	} else if (cgs.gametype == GT_RACE) {
		CG_ParseMenu("ui/ingame_scoreboard_race.smenu");
		CG_ParseMenu("ui/end_scoreboard_race.smenu");
	}
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
static void CG_Shutdown( void ) {
	CG_ShutdownLocalEnts(qfalse);
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	if (cg.recordPathFile) {
		trap_FS_FCloseFile(cg.recordPathFile);
		cg.recordPathFile = 0;
	}
	if (cg.dumpFile) {
		trap_FS_FCloseFile(cg.dumpFile);
		cg.dumpFile = 0;
	}

	CG_FreeFxJitTokens();
    trap_SendConsoleCommand("exec shutdown.cfg\n");
}

void CG_SetDuelPlayers (void)
{
	int i;

	cg.duelPlayer1 = DUEL_PLAYER_INVALID;
	cg.duelPlayer2 = DUEL_PLAYER_INVALID;

	if (CG_CheckQlVersion(0, 1, 0, 719)) {
		if (cg.numDuelScores > 0) {
			cg.duelPlayer1 = cg.duelScores[0].clientNum;
		}
		if (cg.numDuelScores > 1) {
			cg.duelPlayer2 = cg.duelScores[1].clientNum;
		}

		//Com_Printf("^2 d1 %d  d2 %d\n", cg.duelPlayer1, cg.duelPlayer2);

		if (cg.numDuelScores > 0) {
			return;
		}
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (cgs.clientinfo[i].infoValid  &&  cgs.clientinfo[i].team == TEAM_FREE) {
			if (cg.duelPlayer1 == DUEL_PLAYER_INVALID) {
				cg.duelPlayer1 = i;
			} else {
				cg.duelPlayer2 = i;
				break;
			}
		}
	}

	//Com_Printf("duel 1: %s\nduel 2: %s\n", cgs.clientinfo[cg.duelPlayer1].name, cgs.clientinfo[cg.duelPlayer2].name);
}

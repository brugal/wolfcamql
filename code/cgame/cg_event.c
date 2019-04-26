// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

#include "cg_draw.h"  // CG_CenterPrint CG_CenterPrintFragMessage
#include "cg_effects.h"
#include "cg_ents.h"
#include "cg_event.h"
#include "cg_main.h"
#include "cg_newdraw.h"
#include "cg_players.h"
#include "cg_playerstate.h"
#include "cg_predict.h"
#ifdef MISSIONPACK
  #include "cg_servercmds.h"  // CG_VoiceChatLocal()
#endif
#include "cg_sound.h"
#include "cg_syscalls.h"  // trap_R_RegisterSound, trap_S_StopLoopingSound
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_event.h"
#include "wolfcam_predict.h"

#include "wolfcam_local.h"
//#include <stdio.h>

// for the voice chats
#ifdef MISSIONPACK // bk001205
#include "../../ui/menudef.h"
#endif
//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char	*CG_PlaceString( int rank ) {
	static char	str[64];
	char	*s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if ( rank == 1 ) {
		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;		// draw in blue
	} else if ( rank == 2 ) {
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;		// draw in red
	} else if ( rank == 3 ) {
		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;		// draw in yellow
	} else if ( rank == 11 ) {
		s = "11th";
	} else if ( rank == 12 ) {
		s = "12th";
	} else if ( rank == 13 ) {
		s = "13th";
	} else if ( rank % 10 == 1 ) {
		s = va("%ist", rank);
	} else if ( rank % 10 == 2 ) {
		s = va("%ind", rank);
	} else if ( rank % 10 == 3 ) {
		s = va("%ird", rank);
	} else {
		s = va("%ith", rank);
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( const entityState_t *ent ) {
	int			mod;
	int			target, attacker;
	char		*message;
	char		*message2;
	const char	*targetInfo;
	//const char	*attackerInfo;
	char		targetName[MAX_QPATH * 2];
	char		attackerName[MAX_QPATH * 2];
	gender_t	gender;
	const clientInfo_t	*ci;
	qhandle_t	icon;
	//FIXME testing
	//static int	count = 0;
	int i;
	int index;
	int ourClientNum;
	obituary_t *lastObituary;

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if (attacker == ourClientNum  &&  attacker != target  &&  !cg.freecam  &&  cg_killBeep.integer > 0  &&  cg_killBeep.integer <= 8) {
		if (target >= 0  &&  target < MAX_CLIENTS  &&  CG_IsEnemy(&cgs.clientinfo[target])) {
			CG_StartLocalSound(cgs.media.killBeep[cg_killBeep.integer - 1], CHAN_KILLBEEP_SOUND);
		}
	}

	for (i = 0;  i < cg.numScores;  i++) {
		if (cg.scores[i].client != target) {
			continue;
		}
		if (mod == MOD_THAW) {
			cg.scores[i].alive = qtrue;
		} else {
			cg.scores[i].alive = qfalse;
		}
		break;
	}

	if (wolfcam_following  &&  wcg.followMode == WOLFCAM_FOLLOW_VICTIM  &&  target == wcg.nextVictim) {
		VectorCopy(cg_entities[target].lerpOrigin, cg.victimOrigin);
		VectorCopy(cg_entities[target].lerpAngles, cg.victimAngles);
	}
	if (wolfcam_following  &&  wcg.followMode == WOLFCAM_FOLLOW_KILLER  &&  attacker == wcg.nextKiller) {
		VectorCopy(cg_entities[attacker].lerpOrigin, cg.wcKillerOrigin);
		VectorCopy(cg_entities[attacker].lerpAngles, cg.wcKillerAngles);
		cg.wcKillerClientNum = attacker;
	}

	//count++;
	//Com_Printf("^3--- %d serverTime %d  snap %d ent %d  event %d  etype %d  %d killed %d with %d\n", count, cg.snap->serverTime, cgs.processedSnapshotNum, ent->number, ent->event, ent->eType, attacker, target, mod);

	//Com_Printf("^3 adding obit %d  %d\n", cg.obituaryIndex, cg.time);

	index = cg.obituaryIndex % MAX_OBITUARIES;
	lastObituary = &cg.obituaries[index];
	cg.obituaryIndex++;

	lastObituary->time = cg.time;
	lastObituary->killerClientNum = attacker;
	lastObituary->victimClientNum = target;

#if 0
	Com_Printf("^3 in CG_Obituary(), cg.obituaryIndex %d  time: %d\n", cg.obituaryIndex, cg.obituaries[(cg.obituaryIndex - 1) % MAX_OBITUARIES].time);
    Com_Printf("last obit time: %d  \n", lastObituary->time);
	Com_Printf("^3 in wtf  ... CG_Obituary(), cg.obituaryIndex %d  time: %d\n", cg.obituaryIndex, cg.obituaries[cg.obituaryIndex - 1].time);
#endif
	//return;
	
    if (attacker >= 0  &&  attacker <= MAX_CLIENTS) {
		wclients[attacker].lastKillWeapon = BG_ModToWeapon(mod);
		if (mod != MOD_THAW) {
			if (attacker == target) {
				wclients[attacker].suicides++;
			} else {
				wclients[attacker].kills++;
				wclients[attacker].killCount++;
				//Com_Printf("%d %s killCount %d\n", attacker, cgs.clientinfo[attacker].name, wclients[attacker].killCount);
			}
			//FIXME huh?  -- 2016-07-27 'They killed someone or they died' I think
			wclients[attacker].killedOrDied = qtrue;
			wclients[attacker].needToClearPerKillStats = qtrue;
		}
		lastObituary->killerTeam = cgs.clientinfo[attacker].team;
		lastObituary->killerClientNum = attacker;
    }
    if (target >= 0  &&  target <= MAX_CLIENTS) {
		if (mod != MOD_THAW) {
			wclients[target].lastDeathWeapon = BG_ModToWeapon(mod);
			wclients[target].deaths++;
			wclients[target].killedOrDied = qtrue;
			wclients[target].killCount = 0;
		}
		lastObituary->victimTeam = cgs.clientinfo[target].team;
		lastObituary->victimClientNum = target;
		wclients[target].aliveThisRound = qfalse;
		//if (target == cg.snap->ps.clientNum  &&  cg.clientNum == cg.snap->ps.clientNum) {
		if (target == cg.clientNum) {
			if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_CTFS) {
				if (cgs.protocol == PROTOCOL_QL) {
					int roundTime;

					roundTime = atoi(CG_ConfigString(CS_ROUND_TIME));

					if (roundTime > 0) {
						trap_SendConsoleCommand("exec demoTakerDieRound.cfg\n");
					}
				} else if (cgs.cpma) {
					if (!CG_CpmaIsRoundWarmup()) {
						trap_SendConsoleCommand("exec demoTakerDieRound.cfg\n");
					}
				}
			}
		}
    }

	if ( target < 0 || target >= MAX_CLIENTS ) {
		CG_Error( "CG_Obituary: target out of range" );
	}
	ci = &cgs.clientinfo[target];
	Q_strncpyz(lastObituary->victim, ci->name, sizeof(lastObituary->victim));
	Q_strncpyz(lastObituary->victimWhiteName, ci->whiteName, sizeof(lastObituary->victimWhiteName));
	lastObituary->victimClientNum = target;

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		//attackerInfo = NULL;
		lastObituary->killer[0] = '\0';
		lastObituary->killerWhiteName[0] = '\0';
		lastObituary->killerClientNum = -1;
	} else {
		//attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
		Q_strncpyz(lastObituary->killer, cgs.clientinfo[attacker].name, sizeof(lastObituary->killer));
		Q_strncpyz(lastObituary->killerWhiteName, cgs.clientinfo[attacker].whiteName, sizeof(lastObituary->killerWhiteName));
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );
	if ( !targetInfo ) {
		Com_Printf("cgobit ???\n");
		return;
	}
	//Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof(targetName) - 2);
	Q_strncpyz(targetName, cgs.clientinfo[target].name, sizeof(targetName) - 2);
	strcat( targetName, S_COLOR_WHITE );

	message2 = "";

	// check for single client messages

	icon = cgs.media.worldDeathIcon;
	lastObituary->weapon = BG_ModToWeapon(mod);

	switch( mod ) {
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	default:
		message = NULL;
		break;
	}

	if (attacker == target) {
		gender = ci->gender;
		switch (mod) {
#if 1  //def MPACK
		case MOD_KAMIKAZE:
			message = "goes out with a bang";
			wclients[attacker].kamiDeaths++;
			//FIXME kami suicide stats?
			break;
#endif
		case MOD_GRENADE_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "tripped on her own grenade";
			else if ( gender == GENDER_NEUTER )
				message = "tripped on its own grenade";
			else
				message = "tripped on his own grenade";
            wclients[attacker].wstats[WP_GRENADE_LAUNCHER].suicides++;
			icon = cg_weapons[WP_GRENADE_LAUNCHER].weaponIcon;
			break;
		case MOD_ROCKET_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "blew herself up";
			else if ( gender == GENDER_NEUTER )
				message = "blew itself up";
			else
				message = "blew himself up";
            wclients[attacker].wstats[WP_ROCKET_LAUNCHER].suicides++;
			icon = cg_weapons[WP_ROCKET_LAUNCHER].weaponIcon;
			break;
		case MOD_PLASMA_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "melted herself";
			else if ( gender == GENDER_NEUTER )
				message = "melted itself";
			else
				message = "melted himself";
            wclients[attacker].wstats[WP_PLASMAGUN].suicides++;
			icon = cg_weapons[WP_PLASMAGUN].weaponIcon;
			break;
		case MOD_BFG_SPLASH:
			message = "should have used a smaller gun";
            wclients[attacker].wstats[WP_BFG].suicides++;
			icon = cg_weapons[WP_BFG].weaponIcon;
			break;
		case MOD_LIGHTNING_DISCHARGE:
			if ( gender == GENDER_FEMALE )
				message = "discharged herself";
			else if ( gender == GENDER_NEUTER )
				message = "discharged itself";
			else
				message = "discharged himself";
            wclients[attacker].wstats[WP_LIGHTNING].suicides++;
			icon = cg_weapons[WP_LIGHTNING].weaponIcon;
			break;

#if 1  //def MPACK
		case MOD_PROXIMITY_MINE:
			if( gender == GENDER_FEMALE ) {
				message = "found her prox mine";
			} else if ( gender == GENDER_NEUTER ) {
				message = "found its prox mine";
			} else {
				message = "found his prox mine";
			}
			wclients[attacker].wstats[WP_PROX_LAUNCHER].suicides++;
			icon = cg_weapons[WP_PROX_LAUNCHER].weaponIcon;
			break;
#endif
		case MOD_SWITCH_TEAMS:
			message = "switched teams";
			break;
		default:
			if ( gender == GENDER_FEMALE )
				message = "killed herself";
			else if ( gender == GENDER_NEUTER )
				message = "killed itself";
			else
				message = "killed himself";
			break;
		}
	} else {
        if ((attacker >= 0  &&  attacker <= MAX_CLIENTS)  &&
            (target >= 0  &&  target <= MAX_CLIENTS))
        {
            switch (mod) {
            case MOD_GAUNTLET:
                wclients[attacker].wstats[WP_GAUNTLET].kills++;
                wclients[target].wstats[WP_GAUNTLET].deaths++;
				icon = cg_weapons[WP_GAUNTLET].weaponIcon;
                break;
            case MOD_MACHINEGUN:
                wclients[attacker].wstats[WP_MACHINEGUN].kills++;
                wclients[target].wstats[WP_MACHINEGUN].deaths++;
				icon = cg_weapons[WP_MACHINEGUN].weaponIcon;
                break;
            case MOD_HMG:
                wclients[attacker].wstats[WP_HEAVY_MACHINEGUN].kills++;
                wclients[target].wstats[WP_HEAVY_MACHINEGUN].deaths++;
				icon = cg_weapons[WP_HEAVY_MACHINEGUN].weaponIcon;
                break;
            case MOD_SHOTGUN:
                wclients[attacker].wstats[WP_SHOTGUN].kills++;
                wclients[target].wstats[WP_SHOTGUN].deaths++;
				icon = cg_weapons[WP_SHOTGUN].weaponIcon;
                break;
            case MOD_GRENADE:
            case MOD_GRENADE_SPLASH:
                wclients[attacker].wstats[WP_GRENADE_LAUNCHER].kills++;
                wclients[target].wstats[WP_GRENADE_LAUNCHER].deaths++;
				icon = cg_weapons[WP_GRENADE_LAUNCHER].weaponIcon;
                break;
            case MOD_ROCKET:
            case MOD_ROCKET_SPLASH:
                wclients[attacker].wstats[WP_ROCKET_LAUNCHER].kills++;
                wclients[target].wstats[WP_ROCKET_LAUNCHER].deaths++;
				icon = cg_weapons[WP_ROCKET_LAUNCHER].weaponIcon;
                break;
            case MOD_PLASMA:
            case MOD_PLASMA_SPLASH:
                wclients[attacker].wstats[WP_PLASMAGUN].kills++;
                wclients[target].wstats[WP_PLASMAGUN].deaths++;
				icon = cg_weapons[WP_PLASMAGUN].weaponIcon;
                break;
            case MOD_RAILGUN:
			case MOD_RAILGUN_HEADSHOT:
                wclients[attacker].wstats[WP_RAILGUN].kills++;
                wclients[target].wstats[WP_RAILGUN].deaths++;
				icon = cg_weapons[WP_RAILGUN].weaponIcon;
                break;
            case MOD_LIGHTNING:
                wclients[attacker].wstats[WP_LIGHTNING].kills++;
                wclients[target].wstats[WP_LIGHTNING].deaths++;
				icon = cg_weapons[WP_LIGHTNING].weaponIcon;
                break;
            case MOD_BFG:
            case MOD_BFG_SPLASH:
                wclients[attacker].wstats[WP_BFG].kills++;
                wclients[target].wstats[WP_BFG].deaths++;
				icon = cg_weapons[WP_BFG].weaponIcon;
                break;
            case MOD_PROXIMITY_MINE:
                wclients[attacker].wstats[WP_PROX_LAUNCHER].kills++;
                wclients[target].wstats[WP_PROX_LAUNCHER].deaths++;
				icon = cg_weapons[WP_PROX_LAUNCHER].weaponIcon;
				break;
            case MOD_NAIL:
                wclients[attacker].wstats[WP_NAILGUN].kills++;
                wclients[target].wstats[WP_NAILGUN].deaths++;
				icon = cg_weapons[WP_NAILGUN].weaponIcon;
				break;
            case MOD_CHAINGUN:
                wclients[attacker].wstats[WP_CHAINGUN].kills++;
                wclients[target].wstats[WP_CHAINGUN].deaths++;
				icon = cg_weapons[WP_CHAINGUN].weaponIcon;
				break;
			case MOD_KAMIKAZE:
				if (attacker != target) {
					wclients[attacker].kamiKills++;
				}
				wclients[target].kamiDeaths++;
				break;
            default:
                break;
            }
        }
    }

	if (mod == MOD_THAW) {
		lastObituary->icon = cgs.media.thawIcon;
	} else {
		lastObituary->icon = icon;
	}

	if (message) {
		CG_Printf( "%s %s.\n", targetName, message);
		return;
	}

	// check for kill messages from the current clientNum
	if ( !wolfcam_following  &&  attacker == cg.snap->ps.clientNum ) {
		const char	*s;

		if (((!CG_IsTeamGame(cgs.gametype)  ||  cgs.gametype == GT_RED_ROVER)  &&  cg_fragMessageStyle.integer == 1)  ||  cg_fragMessageStyle.integer == 2) {
			if (cg_drawFragMessageSeparate.integer) {
				s = va("%s place with %i", CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ), cg.snap->ps.persistant[PERS_SCORE]);
				CG_CenterPrint(s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				s = va("You fragged %s", targetName );
			} else {
				s = va("You fragged %s\n%s place with %i", targetName,
					   CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
					   cg.snap->ps.persistant[PERS_SCORE] );
			}
		} else {
			if (mod == MOD_THAW) {
				s = va("You thawed %s", targetName);
			} else {
				if (cg_teamKillWarning.integer  &&  (cgs.gametype >= GT_TEAM  &&  (cgs.gametype != GT_CA  &&  cgs.gametype != GT_RED_ROVER  &&  cgs.gametype != GT_DOMINATION  &&  cgs.gametype != GT_CTFS))  &&  target >= 0  &&  target < MAX_CLIENTS  &&  CG_IsTeammate(&cgs.clientinfo[target])) {
					s = va("You fragged %s\n\n^1Watch your fire!", targetName);
				} else {
					s = va("You fragged %s", targetName);
				}
			}
		}
#ifdef MISSIONPACK
		if (!(cg_singlePlayerActive.integer && cg_cameraOrbit.integer)) {
			CG_CenterPrint( s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		}
#else
		if (cg_drawFragMessageSeparate.integer) {
			Q_strncpyz(cg.lastFragString, s, sizeof(cg.lastFragString));
		} else {
			CG_CenterPrintFragMessage( s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		}
#endif

		cg.lastFragTime = cg.time;
		cg.lastFragVictim = target;
		cg.lastFragKiller = attacker;
		Q_strncpyz(cg.lastFragVictimName, cgs.clientinfo[target].name, sizeof(cg.lastFragVictimName));
		Q_strncpyz(cg.lastFragVictimWhiteName, cgs.clientinfo[target].whiteName, sizeof(cg.lastFragVictimWhiteName));
		cg.lastFragWeapon = BG_ModToWeapon(mod);
		cg.lastFragMod = mod;
		cg.lastFragVictimTeam = cgs.clientinfo[target].team;
		cg.lastFragKillerTeam = cgs.clientinfo[attacker].team;

		// print the text message as well
	} else if (wolfcam_following) {
        if (attacker == wcg.clientNum) {
            const char *s;
			if (mod == MOD_THAW) {
				s = va("You thawed %s", targetName);
			} else {
				s = va("You fragged %s", targetName);
			}
			if (cg_drawFragMessageSeparate.integer) {
				Q_strncpyz(cg.lastFragString, s, sizeof(cg.lastFragString));
				cg.lastFragTime = cg.time;
			} else {
				CG_CenterPrintFragMessage (s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			}
			cg.lastFragTime = cg.time;
			cg.lastFragVictim = target;
			cg.lastFragKiller = attacker;
			Q_strncpyz(cg.lastFragVictimName, cgs.clientinfo[target].name, sizeof(cg.lastFragVictimName));
			Q_strncpyz(cg.lastFragVictimWhiteName, cgs.clientinfo[target].whiteName, sizeof(cg.lastFragVictimWhiteName));
			cg.lastFragWeapon = BG_ModToWeapon(mod);
			cg.lastFragMod = mod;
			cg.lastFragVictimTeam = cgs.clientinfo[target].team;
			cg.lastFragKillerTeam = cgs.clientinfo[attacker].team;
        }
    }

	// check for double client messages
	if (attacker < 0  ||  attacker >= MAX_CLIENTS) {  //( !attackerInfo ) {
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
		if (target == ourClientNum) {
			cg.killerName[0] = 0;
			cg.killerNameHud[0] = 0;
			cg.killerWeaponIcon = icon;
			cg.killerClientNum = attacker;
		}
	} else {
		//Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof(attackerName) - 2);
		Q_strncpyz(attackerName, cgs.clientinfo[attacker].name, sizeof(attackerName) - 2);
		strcat( attackerName, S_COLOR_WHITE );
		// check for kill messages about the current clientNum
		if ( target == ourClientNum ) {
			vec3_t forward;

			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
			Q_strncpyz( cg.killerNameHud, attackerName, sizeof( cg.killerNameHud ) );
			cg.killerWeaponIcon = icon;
			cg.killerClientNum = attacker;
			VectorCopy(cg_entities[attacker].lerpOrigin, cg.killerOrigin);
			VectorSubtract(cg.killerOrigin, cg.snap->ps.origin, forward);
			vectoangles(forward, cg.deadAngles);
			//cg.deadAngles[PITCH] = 0;
			//cg.deadAngles[ROLL] = 0;
			//cg.deadAngles[YAW] = cg.deadAngles[YAW] - cg.snap->ps.viewangles[YAW];
		}
	}

	if ( attacker != ENTITYNUM_WORLD ) {
		switch (mod) {
		case MOD_GRAPPLE:
			message = "was caught by";
			break;
		case MOD_GAUNTLET:
			message = "was pummeled by";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_GRENADE:
			message = "ate";
			message2 = "'s grenade";
			break;
		case MOD_HMG:
			message = "was ripped up by";
			message2 = "'s HMG";
			break;
		case MOD_GRENADE_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_ROCKET_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_PLASMA:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_PLASMA_SPLASH:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_RAILGUN_HEADSHOT:
			message = "was railed in the head by";
			break;
		case MOD_LIGHTNING:
			message = "was electrocuted by";
			break;
		case MOD_LIGHTNING_DISCHARGE:
			message = "was discharged by";
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			message = "was blasted by";
			message2 = "'s BFG";
			break;
#if 1  //def MPACK
		case MOD_NAIL:
			message = "was nailed by";
			break;
		case MOD_CHAINGUN:
			message = "got lead poisoning from";
			message2 = "'s Chaingun";
			break;
		case MOD_PROXIMITY_MINE:
			message = "was too close to";
			message2 = "'s Prox Mine";
			break;
		case MOD_KAMIKAZE:
			message = "falls to";
			message2 = "'s Kamikaze blast";
			break;
		case MOD_JUICED:
			message = "was juiced by";
			break;
#endif
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		case MOD_THAW:
			message = "was thawed by";
			break;
		default:
			message = "was killed by";
			CG_Printf("^3unknown mod: %d\n", mod);
			//CG_PrintEntityStatep(ent);
			break;
		}

		if (message) {
			const char *s;

			s = va("%s %s %s%s", targetName, message, attackerName, message2);
			Q_strncpyz(lastObituary->q3obitString, s, sizeof(lastObituary->q3obitString));
			//CG_Printf( "%s %s %s%s\n", targetName, message, attackerName, message2);
			if (attacker == ourClientNum) {
				Q_strncpyz(cg.lastFragq3obitString, s, sizeof(cg.lastFragq3obitString));
			}
			CG_Printf("%s\n", s);
			return;
		}
	}

	// we don't know what it was
	CG_Printf( "%s died.\n", targetName );
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( const centity_t *cent ) {
	clientInfo_t *ci;
	int			itemNum, clientNum;
	gitem_t		*item;
	const entityState_t *es;

	es = &cent->currentState;

	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}

#if 1
	item = BG_FindItemForHoldable(itemNum);
	if (item == &bg_itemlist[0]) {
		//FIXME hack
		itemNum = 0;
	}
#endif

	// print a message if the local player
	if ( !wolfcam_following  &&  es->number == cg.snap->ps.clientNum ) {
		if (!itemNum) {
			if (cg_noItemUseMessage.integer) {
				CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			}
		} else {
			item = BG_FindItemForHoldable( itemNum );
			if (cg_itemUseMessage.integer) {
				CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			}
		}
	} else if (wolfcam_following  &&  es->number == wcg.clientNum) {
        if ( !itemNum ) {
			if (cg_noItemUseMessage.integer) {
				CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			}
        } else {
            item = BG_FindItemForHoldable( itemNum );
			if (cg_itemUseMessage.integer) {
				CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			}
        }
    }

	if ((cgs.cpma  &&  itemNum == HIC_NONE)  ||  (!cgs.cpma  &&  itemNum == HI_NONE)) {
		if (cg_noItemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		}
	} else if ((cgs.cpma  &&  itemNum == HIC_MEDKIT)  ||  (!cgs.cpma  &&  itemNum == HI_MEDKIT)) {
		clientNum = cent->currentState.clientNum;
		if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
			ci = &cgs.clientinfo[ clientNum ];
			ci->medkitUsageTime = cg.time;
		}
		if (cg_itemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		}
	} else if ((cgs.cpma  &&  itemNum == HIC_INVULNERABILITY)  ||  (!cgs.cpma  &&  itemNum == HI_INVULNERABILITY)) {
		if (cg_itemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useInvulnerabilitySound );
		}
	}

#if 0
	switch ( itemNum ) {
	default:
	case HI_NONE:
		if (cg_noItemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		}
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		clientNum = cent->currentState.clientNum;
		if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
			ci = &cgs.clientinfo[ clientNum ];
			ci->medkitUsageTime = cg.time;
		}
		if (cg_itemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		}
		break;

#if  1  //def MPACK
	case HI_KAMIKAZE:
		break;

	case HI_PORTAL:
		break;
	case HI_INVULNERABILITY:
		if (cg_itemUseSound.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useInvulnerabilitySound );
		}
		break;
#endif
	}
#endif
}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum ) {
	if (cg_drawItemPickupsCount.integer  &&  cg.time - cg.itemPickupTime < cg_drawItemPickupsTime.integer  &&  itemNum == cg.itemPickup  &&  bg_itemlist[itemNum].giType != IT_POWERUP) {
		cg.itemPickupCount++;
	} else {
		cg.itemPickupCount = 1;
	}

	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupClockTime = CG_GetCurrentTimeWithDirection(NULL);
	cg.itemPickupBlendTime = cg.time;
	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {
		// select it immediately
		if ( cg_autoswitch.integer && bg_itemlist[itemNum].giTag != WP_MACHINEGUN ) {
			cg.weaponSelectTime = cg.time;
			cg.weaponSelect = bg_itemlist[itemNum].giTag;
		}
	}

}

void CG_TimedItemPickup (int index, const vec3_t origin, int clientNum, int time, qboolean spec)
{
	const gitem_t *item;
	int i;
	timedItem_t *titem = NULL;
	float len;
	float newLen;
	int bestItem;
	vec3_t delta;
	qboolean gotIt = qfalse;
	//static int count = 0;
	int colorCode;
	int totalItems;
	static int lastIndex = -1;
	static int lastBestItem = -1;
	static int lastTime = -1;

	item = &bg_itemlist[index];

	if (spec  &&  time == 0) {
		//return;  // just means the item has respawned
	}

	if (item->giType == IT_ARMOR  ||  item->giType == IT_HEALTH) {
		if (!Q_stricmp(item->pickup_name, "Green Armor")) {
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numGreenArmors;  i++) {
				VectorSubtract(origin, cg.greenArmors[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			colorCode = 2;
			totalItems = cg.numGreenArmors;
			titem = &cg.greenArmors[bestItem];

			if (!spec) {
				cg.greenArmors[bestItem].pickupTime = time;
				cg.greenArmors[bestItem].clientNum = clientNum;
			}
			gotIt = qtrue;
		} else if (!Q_stricmp(item->pickup_name, "Yellow Armor")) {
			//Com_Printf("%s picks up Yellow Armor\n", cgs.clientinfo[clientNum].name);
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numYellowArmors;  i++) {
				VectorSubtract(origin, cg.yellowArmors[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			colorCode = 3;
			totalItems = cg.numYellowArmors;
			titem = &cg.yellowArmors[bestItem];

			if (!spec) {
				cg.yellowArmors[bestItem].pickupTime = time;
				cg.yellowArmors[bestItem].clientNum = clientNum;
			}
			gotIt = qtrue;
		} else if (!Q_stricmp(item->pickup_name, "Red Armor")) {
			//Com_Printf("%s picks up Red Armor\n", cgs.clientinfo[clientNum].name);
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numRedArmors;  i++) {
				VectorSubtract(origin, cg.redArmors[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			colorCode = 1;
			totalItems = cg.numRedArmors;
			titem = &cg.redArmors[bestItem];

			if (!spec) {
				cg.redArmors[bestItem].pickupTime = time;
				cg.redArmors[bestItem].clientNum = clientNum;
			}
			gotIt = qtrue;
		} else if (!Q_stricmp(item->pickup_name, "Mega Health")) {
			//Com_Printf("%s picks up Mega Health\n", cgs.clientinfo[clientNum].name);
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numMegaHealths;  i++) {
				VectorSubtract(origin, cg.megaHealths[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			colorCode = 4;
			totalItems = cg.numMegaHealths;
			titem = &cg.megaHealths[bestItem];

			if (!spec) {
				cg.megaHealths[bestItem].pickupTime = time;
				cg.megaHealths[bestItem].clientNum = clientNum;
				cg.megaHealths[bestItem].countDownTrigger = -1;
			}
			gotIt = qtrue;
		}

		if (titem  &&  spec) {
			if (time) {
				titem->specPickupTime = time - (titem->respawnLength * 1000);
			} else {
				if (cg_itemSpawnPrint.integer) {
					if (lastIndex == index  &&  lastBestItem == bestItem  &&  lastTime == cg.time) {
						return;  // don't spam -- server can send extra events if more than 1 spec
					}
					if (totalItems == 1) {
						CG_PrintToScreen("%s ^%d%s ^7spawned\n", CG_GetLevelTimerString(), colorCode, item->pickup_name);
						Com_Printf("%s ^%d%s ^7spawned\n", CG_GetLevelTimerString(), colorCode, item->pickup_name);
					} else {
						CG_PrintToScreen("%s ^%d%s ^7%d spawned\n", CG_GetLevelTimerString(), colorCode, item->pickup_name, bestItem + 1);
						Com_Printf("%s ^%d%s ^7%d spawned\n", CG_GetLevelTimerString(), colorCode, item->pickup_name, bestItem + 1);
					}
					lastIndex = index;
					lastBestItem = bestItem;
					lastTime = cg.time;
				}
				return;
			}
			//Com_Printf("%d %d\n", titem->specPickupTime, titem->pickupTime);
		}
		if (titem  &&  !spec) {
			titem->specPickupTime = 0;
		}

		if (gotIt) {
			//Com_Printf("^5pickup cn %d  %s\n", clientNum, item->pickup_name);
			//count++;
			//Com_Printf(":%d %d %d  %s %s\n", count, time, index, item->pickup_name, spec ? "(spec)" : "");
		}
	} else if (item->giType == IT_POWERUP) {

		bestItem = -1;

		if (item->giTag == PW_QUAD) {
			//Com_Printf("global quad pickup\n");
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numQuads;  i++) {
				VectorSubtract(origin, cg.quads[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			totalItems = cg.numQuads;

			// quad can be picked up again
			if (!spec  &&  (cg.quads[bestItem].pickupTime / 1000) + cg.quads[bestItem].respawnLength <= (time / 1000)) {
				//Com_Printf("quad pickup\n");
				cg.quads[bestItem].pickupTime = time;
				//cg.quads[bestItem].specPickupTime = time;
				//count++;
				//Com_Printf(":%d %d %d  %s\n", count, time, index, item->pickup_name);
			}

		} else if (item->giTag == PW_BATTLESUIT) {
			//Com_Printf("battlesuit pickup\n");
			for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numBattleSuits;  i++) {
				VectorSubtract(origin, cg.battleSuits[i].origin, delta);
				newLen = VectorLength(delta);
				if (newLen < len) {
					bestItem = i;
					len = newLen;
				}
			}

			totalItems = cg.numBattleSuits;

			// bs can be picked up again ?
			if (!spec  &&  (cg.battleSuits[bestItem].pickupTime / 1000) + cg.battleSuits[bestItem].respawnLength <= (time / 1000)) {
				cg.battleSuits[bestItem].pickupTime = time;
				//cg.battleSuits[bestItem].specPickupTime = time;
				//count++;
				//Com_Printf(":%d %d %d  %s\n", count, time, index, item->pickup_name);
			}
		} else if (item->giTag == PW_REGEN) {
			//FIXME
			totalItems = 1;
		} else {
			totalItems = 1;
		}

		if (spec) {
			//FIXME multiple quads -- japanese castles
			if (time == 0) {
				if (cg_itemSpawnPrint.integer) {
					if (lastIndex == index  &&  lastBestItem == bestItem  &&  lastTime == cg.time) {
						return;  // don't spam
					}

					if (totalItems == 1) {
						CG_PrintToScreen("%s ^6%s ^7powerup spawned\n", CG_GetLevelTimerString(), item->pickup_name);
						Com_Printf("%s ^6%s ^7powerup spawned\n", CG_GetLevelTimerString(), item->pickup_name);
					} else {
						CG_PrintToScreen("%s ^6%s ^7%d powerup spawned\n", CG_GetLevelTimerString(), item->pickup_name, bestItem + 1);
						Com_Printf("%s ^6%s ^7%d powerup spawned\n", CG_GetLevelTimerString(), item->pickup_name, bestItem + 1);
					}

					lastIndex = index;
					lastBestItem = bestItem;
					lastTime = cg.time;
				}
			}
			return;  // get it from global pickup sound
		}
	}
}

#if 1
/*
================
CG_WaterLevel

Returns waterlevel for entity origin
================
*/
static int CG_WaterLevel(const centity_t *cent) {
	vec3_t point;
	int contents, sample1, sample2, anim, waterlevel;
	int viewheight;

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		viewheight = CROUCH_VIEWHEIGHT;
	} else {
		viewheight = DEFAULT_VIEWHEIGHT;
	}

	//
	// get waterlevel, accounting for ducking
	//
	waterlevel = 0;

	point[0] = cent->lerpOrigin[0];
	point[1] = cent->lerpOrigin[1];
	point[2] = cent->lerpOrigin[2] + MINS_Z + 1;
	contents = CG_PointContents(point, -1);

	if (contents & MASK_WATER) {
		sample2 = viewheight - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerpOrigin[2] + MINS_Z + sample1;
		contents = CG_PointContents(point, -1);

		if (contents & MASK_WATER) {
			waterlevel = 2;
			point[2] = cent->lerpOrigin[2] + MINS_Z + sample2;
			contents = CG_PointContents(point, -1);

			if (contents & MASK_WATER) {
				waterlevel = 3;
			}
		}
	}

	return waterlevel;
}
#endif

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health ) {
	const char	*snd;

	// 2019-04-17 since protocol 90 ql doesn't send real value just 20, 40, 60, or 80

	//Com_Printf("^5%f  pain event %d  %p  health %d\n", cg.ftime, cent->currentState.number, cent, health);

	//CG_PrintEntityStatep(&cent->currentState);
	//Com_Printf("%d pain %d %s  health:%d\n", cg.time, cent->currentState.number, cgs.clientinfo[cent->currentState.number].name, health);

	// save values for health estimates (ex: /follow)
	if (cent != &cg.predictedPlayerEntity) {
		int clientNum;

		clientNum = cent->currentState.clientNum;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_PainEvent invalid client number %d\n", clientNum);
		} else {
			wclients[clientNum].painValue = health;
			wclients[clientNum].painValueTime = cg.time;
		}
	}

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25_1.wav";
	} else if ( health < 50 ) {
		snd = "*pain50_1.wav";
	} else if ( health < 75 ) {
		snd = "*pain75_1.wav";
	} else {
		snd = "*pain100_1.wav";
	}

	if ((cgs.cpma  ||  cgs.osp)  &&  cent->currentState.number >= MAX_CLIENTS) {
		//Com_Printf("^5pain %d   %d\n", cent->currentState.number, cent->currentState.modelindex2 - 16);
		cent = &cg_entities[cent->currentState.modelindex2 - 0xf];
		//Com_Printf("^5new %d\n", cent->currentState.number);
	}

	// play a gurp sound instead of a normal pain sound
	if (0) {  //(CG_WaterLevel(cent) == 3) {
		if (rand()&1) {
			CG_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp1.wav"));
		} else {
			CG_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp2.wav"));
		}
	} else {
		CG_StartSound( NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound( cent->currentState.number, snd ) );
	}

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}


#define	DEBUGNAME(x) { eventName = x; if(cg_debugEvents.integer  ||  cg_drawEventNumbers.integer){Com_Printf(x"\n");} }

static int CG_EntityEventFromQ3 (int event)
{
	int e;

	if (cgs.cpma  &&  event >= EVQ3_PROXIMITY_MINE_STICK) {
		switch (event) {
		case EVQ3_PROXIMITY_MINE_STICK:
			// this is EV_STEP
			return EV_STEP_4;
		case EVCPMA_TAUNT:
			return EV_TAUNT;
		default:
			Com_Printf("unknown cpma event %d\n", event);
			return 0;
		}
	}

	e = -1;

	switch (event) {
	case EVQ3_NONE:
		e = EV_NONE;
		break;
	case EVQ3_FOOTSTEP:
		e = EV_FOOTSTEP;
		break;
	case EVQ3_FOOTSTEP_METAL:
		e = EV_FOOTSTEP_METAL;
		break;
	case EVQ3_FOOTSPLASH:
		e = EV_FOOTSPLASH;
		break;
	case EVQ3_FOOTWADE:
		e = EV_FOOTWADE;
		break;
	case EVQ3_SWIM:
		e = EV_SWIM;
		break;
	case EVQ3_STEP_4:
		e = EV_STEP_4;
		break;
	case EVQ3_STEP_8:
		e = EV_STEP_8;
		break;
	case EVQ3_STEP_12:
		e = EV_STEP_12;
		break;
	case EVQ3_STEP_16:
		e = EV_STEP_16;
		break;
	case EVQ3_FALL_SHORT:
		e = EV_FALL_SHORT;
		break;
	case EVQ3_FALL_MEDIUM:
		e = EV_FALL_MEDIUM;
		break;
	case EVQ3_FALL_FAR:
		e = EV_FALL_FAR;
		break;
	case EVQ3_JUMP_PAD:			// boing sound at origin, jump sound on player
		e = EV_JUMP_PAD;
		break;
	case EVQ3_JUMP:
		e = EV_JUMP;
		break;
	case EVQ3_WATER_TOUCH:	// foot touches
		e = EV_WATER_TOUCH;
		break;
	case EVQ3_WATER_LEAVE:	// foot leaves
		e = EV_WATER_LEAVE;
		break;
	case EVQ3_WATER_UNDER:	// head touches
		e = EV_WATER_UNDER;
		break;
	case EVQ3_WATER_CLEAR:	// head leaves
		e = EV_WATER_CLEAR;
		break;
	case EVQ3_ITEM_PICKUP:			// normal item pickups are predictable
		e = EV_ITEM_PICKUP;
		break;
	case EVQ3_GLOBAL_ITEM_PICKUP:	// powerup / team sounds are broadcast to everyone
		e = EV_GLOBAL_ITEM_PICKUP;
		break;
	case EVQ3_NOAMMO:
		e = EV_NOAMMO;
		break;
	case EVQ3_CHANGE_WEAPON:
		e = EV_CHANGE_WEAPON;
		break;
	case EVQ3_FIRE_WEAPON:
		e = EV_FIRE_WEAPON;
		break;
	case EVQ3_USE_ITEM0:
		e = EV_USE_ITEM0;
		break;
	case EVQ3_USE_ITEM1:
		e = EV_USE_ITEM1;
		break;
	case EVQ3_USE_ITEM2:
		e = EV_USE_ITEM2;
		break;
	case EVQ3_USE_ITEM3:
		e = EV_USE_ITEM3;
		break;
	case EVQ3_USE_ITEM4:
		e = EV_USE_ITEM4;
		break;
	case EVQ3_USE_ITEM5:
		e = EV_USE_ITEM5;
		break;
	case EVQ3_USE_ITEM6:
		e = EV_USE_ITEM6;
		break;
	case EVQ3_USE_ITEM7:
		e = EV_USE_ITEM7;
		break;
	case EVQ3_USE_ITEM8:
		e = EV_USE_ITEM8;
		break;
	case EVQ3_USE_ITEM9:
		e = EV_USE_ITEM9;
		break;
	case EVQ3_USE_ITEM10:
		e = EV_USE_ITEM10;
		break;
	case EVQ3_USE_ITEM11:
		e = EV_USE_ITEM11;
		break;
	case EVQ3_USE_ITEM12:
		e = EV_USE_ITEM12;
		break;
	case EVQ3_USE_ITEM13:
		e = EV_USE_ITEM13;
		break;
	case EVQ3_USE_ITEM14:
		e = EV_USE_ITEM14;
		break;
	case EVQ3_USE_ITEM15:
		e = EV_USE_ITEM15;
		break;
	case EVQ3_ITEM_RESPAWN:
		e = EV_ITEM_RESPAWN;
		break;
	case EVQ3_ITEM_POP:
		e = EV_ITEM_POP;
		break;
	case EVQ3_PLAYER_TELEPORT_IN:
		e = EV_PLAYER_TELEPORT_IN;
		break;
	case EVQ3_PLAYER_TELEPORT_OUT:
		e = EV_PLAYER_TELEPORT_OUT;
		break;
	case EVQ3_GRENADE_BOUNCE:		// eventParm will be the soundindex
		e = EV_GRENADE_BOUNCE;
		break;
	case EVQ3_GENERAL_SOUND:
		e = EV_GENERAL_SOUND;
		break;
	case EVQ3_GLOBAL_SOUND:		// no attenuation
		e = EV_GLOBAL_SOUND;
		break;
	case EVQ3_GLOBAL_TEAM_SOUND:
		e = EV_GLOBAL_TEAM_SOUND;
		break;
	case EVQ3_BULLET_HIT_FLESH:
		e = EV_BULLET_HIT_FLESH;
		break;
	case EVQ3_BULLET_HIT_WALL:
		e = EV_BULLET_HIT_WALL;
		break;
	case EVQ3_MISSILE_HIT:
		e = EV_MISSILE_HIT;
		break;
	case EVQ3_MISSILE_MISS:
		e = EV_MISSILE_MISS;
		break;
	case EVQ3_MISSILE_MISS_METAL:
		e = EV_MISSILE_MISS_METAL;
		break;
	case EVQ3_RAILTRAIL:
		e = EV_RAILTRAIL;
		break;
	case EVQ3_SHOTGUN:
		e = EV_SHOTGUN;
		break;
	case EVQ3_BULLET:				// otherEntity is the shooter
		Com_Printf("FIXME q3 EV_BULLET\n");
		//e = EV_BULLET;
		e = EV_BULLET_HIT_FLESH;
		break;
	case EVQ3_PAIN:
		e = EV_PAIN;
		break;
	case EVQ3_DEATH1:
		e = EV_DEATH1;
		break;
	case EVQ3_DEATH2:
		e = EV_DEATH2;
		break;
	case EVQ3_DEATH3:
		e = EV_DEATH3;
		break;
	case EVQ3_OBITUARY:
		e = EV_OBITUARY;
		break;
	case EVQ3_POWERUP_QUAD:
		e = EV_POWERUP_QUAD;
		break;
	case EVQ3_POWERUP_BATTLESUIT:
		e = EV_POWERUP_BATTLESUIT;
		break;
	case EVQ3_POWERUP_REGEN:
		e = EV_POWERUP_REGEN;
		break;
	case EVQ3_GIB_PLAYER:			// gib a previously living player
		e = EV_GIB_PLAYER;
		break;
	case EVQ3_SCOREPLUM:			// score plum
		e = EV_SCOREPLUM;
		break;
//#ifdef MISSIONPACK
	case EVQ3_PROXIMITY_MINE_STICK:
		e = EV_PROXIMITY_MINE_STICK ;
		break;
	case EVQ3_PROXIMITY_MINE_TRIGGER:
		e = EV_PROXIMITY_MINE_TRIGGER ;
		break;
	case EVQ3_KAMIKAZE:			// kamikaze explodes
		e = EV_KAMIKAZE;
		break;
	case EVQ3_OBELISKEXPLODE:		// obelisk explodes
		e = EV_OBELISKEXPLODE;
		break;
	case EVQ3_OBELISKPAIN:			// obelisk is in pain
		e = EV_OBELISKPAIN;
		break;
	case EVQ3_INVUL_IMPACT:		// invulnerability sphere impact
		e = EV_INVUL_IMPACT;
		break;
	case EVQ3_JUICED:				// invulnerability juiced effect
		e = EV_JUICED;
		break;
	case EVQ3_LIGHTNINGBOLT:		// lightning bolt bounced of invulnerability sphere
		e = EV_LIGHTNINGBOLT;
		break;
//#endif
	case EVQ3_DEBUG_LINE:
		e = EV_DEBUG_LINE;
		break;
	case EVQ3_STOPLOOPINGSOUND:
		e = EV_STOPLOOPINGSOUND;
		break;
	case EVQ3_TAUNT:
		e = EV_TAUNT;
		break;
	case EVQ3_TAUNT_YES:
		e = EV_TAUNT_YES;
		break;
	case EVQ3_TAUNT_NO:
		e = EV_TAUNT_NO;
		break;
	case EVQ3_TAUNT_FOLLOWME:
		e = EV_TAUNT_FOLLOWME;
		break;
	case EVQ3_TAUNT_GETFLAG:
		e = EV_TAUNT_GETFLAG;
		break;
	case EVQ3_TAUNT_GUARDBASE:
		e = EV_TAUNT_GUARDBASE;
		break;
	case EVQ3_TAUNT_PATROL:
		e = EV_TAUNT_PATROL;
		break;

	default:
		Com_Printf("unknown q3 event %d\n", event);
		e = event;
		break;
	}

	if (e == -1) {
		if (cgs.cpma) {
			Com_Printf("unknown cpma event %d\n", e);
			return 0;
		}
	}

	return e;
}

int CG_CheckClientEventCpma (int clientNum, const entityState_t *es)
{
	if (es->number >= MAX_CLIENTS) {
		clientNum = es->modelindex2 & 0xf;
	}

	return clientNum;
}


/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/

void CG_EntityEvent( centity_t *cent, const vec3_t position ) {
	entityState_t *es;  // sets es->loopSound
	int				event;
	vec3_t			dir;
	const char		*s;
	int				clientNum;
	clientInfo_t *ci;  //FIXME should be const
	int ourClientNum;
	const char *eventName;
	int id;
	int i;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;
	if (cgs.protocol == PROTOCOL_Q3) {
		event = CG_EntityEventFromQ3(event);
	}

	id = es->number + event + cg.snap->serverTime;

	if (cg_debugEvents.integer  ||  cg_drawEventNumbers.integer) {
		CG_Printf("id:%d ent:%3i  event:%3i serverTime: %d ", id, es->number, event, cg.snap->serverTime);
	}

	if (cent->currentState.eType >= ET_EVENTS) {
		//Com_Printf("yes %d\n", cent->currentState.eType);
		if (cg.filter.events) {
			return;
		}
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

    wolfcam_log_event (cent, position);

	if (cg_drawEventNumbers.integer) {
		byte color[4];
		vec3_t origin;

		VectorCopy(es->pos.trBase, origin);
		origin[2] += 50 + rand() % 40;

		MAKERGBA(color, 0, 255, 0, 255);
		if (event == EV_MISSILE_MISS) {
			color[0] = 255;
			color[1] = 50;
		}
		CG_FloatNumberExt(id, origin, RF_DEPTHHACK, color, 2000);

#if 0
		origin[2] += 25;
		MAKERGBA(color, 40, 125, 0, 255);
		CG_FloatNumberExt(cg.snap->serverTime, origin, RF_DEPTHHACK, color, 2000);
#endif
	}

	for (i = 0;  i < cg.numEventFilter;  i++) {
		if (id == cg.eventFilter[i]) {
			return;
		}
	}

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s clientnum %d\n", event, eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ ci->footsteps ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_WOOD:
		DEBUGNAME("EV_FOOTSTEP_WOOD");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_WOOD ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_SNOW:
		DEBUGNAME("EV_FOOTSTEP_SNOW");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SNOW ][rand()&3] );
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event,  eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event,  eventName, es->number);
		}
		if (cg_footsteps.integer) {
			CG_StartSound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		wclients[clientNum].landChange = -8;
		wclients[clientNum].landTime = cg.time;
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		// use normal pain sound
		CG_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		//Com_Printf("fall medium %d\n", es->number);

		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		wclients[clientNum].landChange = -16;
		wclients[clientNum].landTime = cg.time;
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event,  eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}

		wclients[clientNum].landChange = -24;
		wclients[clientNum].landTime = cg.time;
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_STEP_20:
	case EV_STEP_24:		// smooth out step up transitions
		DEBUGNAME("EV_STEP");
	{
		float	oldStep;
		int		delta;
		int		step;

		//Com_Printf("   event step %d %d\n", clientNum, cg.predictedPlayerState.clientNum);
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		//Com_Printf("ev_step\n");
		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			break;
		}

		// if we are interpolating, we don't need to smooth steps
		//FIXME wc allow PMF_FOLLOW?
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			cg_nopredict.integer || cg_synchronousClients.integer ) {
			break;
		}

#if 0
		if (!cg.demoPlayback  &&  (cg_nopredict.integer  ||  cg_synchronousClients.integer)) {
			break;
		}
#endif
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if (delta < cg_stepSmoothTime.value) {
			oldStep = cg.stepChange * (cg_stepSmoothTime.value - delta) / cg_stepSmoothTime.value;
		} else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1 );
		cg.stepChange = oldStep + step;
		if (cg.stepChange > cg_stepSmoothMaxChange.value) {
			cg.stepChange = cg_stepSmoothMaxChange.value;
		}
		cg.stepTime = cg.time;
		//Com_Printf("^5%f  step %d\n", cg.ftime, es->eventParm);
		//Com_Printf("   event ran step\n");
		break;
	}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}

		if (*EffectScripts.jumpPad) {
			CG_ResetScriptVars();
			VectorCopy(cent->lerpOrigin, ScriptVars.origin);
			CG_RunQ3mmeScript(EffectScripts.jumpPad, NULL);
		} else {
//		CG_Printf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		//CG_Printf("jump pad\n");
		{
			vec3_t			up = {0, 0, 1};


			CG_SmokePuff( cent->lerpOrigin, up,
						  32,
						  1, 1, 1, 0.33f,
						  1000,
						  cg.time, 0,
						  LEF_PUFF_DONT_SCALE,
						  cgs.media.smokePuffShader );
		}

		// boing sound at origin, jump sound on player
		CG_StartSound ( cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		}

		//FIXME allow fx ?  -- think so have to add attach sound to entity function
		CG_StartSound (NULL, clientNum, CHAN_VOICE, CG_CustomSound(clientNum, "*jump1.wav" ) );

		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );

		if (es->number >= 0  &&  es->number < MAX_CLIENTS) {
			wclients[es->number].jumpTime = cg.time;
		}

		if (es->number == ourClientNum) {
			if (cg.jumpsNeedClearing) {
				cg.numJumps = 0;
				cg.jumpsNeedClearing = qfalse;
				//CG_Printf("cleared jump info %d\n", cg.time);
			}
			cg.jumps[cg.numJumps % MAX_JUMPS_INFO].time = cg.snap->serverTime;
			if (!wolfcam_following  ||  wcg.clientNum == cg.snap->ps.clientNum) {
				cg.jumps[cg.numJumps % MAX_JUMPS_INFO].speed = sqrt(cg.snap->ps.velocity[0] * cg.snap->ps.velocity[0] + cg.snap->ps.velocity[1] * cg.snap->ps.velocity[1]);
			} else {
				cg.jumps[cg.numJumps % MAX_JUMPS_INFO].speed = sqrt(cg_entities[wcg.clientNum].currentState.pos.trDelta[0] * cg_entities[wcg.clientNum].currentState.pos.trDelta[0] + cg_entities[wcg.clientNum].currentState.pos.trDelta[1] * cg_entities[wcg.clientNum].currentState.pos.trDelta[1]);
			}
			//Com_Printf("jump %d\n", cg.jumps[cg.numJumps].speed);
			if (cg.numJumps == 0) {
				cg.jumpsFirstTime = cg.snap->serverTime;
			}
			cg.numJumps++;
		}
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (!cg_noTaunt.integer) {
			CG_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
		}
		break;
#ifdef MISSIONPACK
	case EV_TAUNT_YES:
		DEBUGNAME("EV_TAUNT_YES");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_YES);
		break;
	case EV_TAUNT_NO:
		DEBUGNAME("EV_TAUNT_NO");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_NO);
		break;
	case EV_TAUNT_FOLLOWME:
		DEBUGNAME("EV_TAUNT_FOLLOWME");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_FOLLOWME);
		break;
	case EV_TAUNT_GETFLAG:
		DEBUGNAME("EV_TAUNT_GETFLAG");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONGETFLAG);
		break;
	case EV_TAUNT_GUARDBASE:
		DEBUGNAME("EV_TAUNT_GUARDBASE");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONDEFENSE);
		break;
	case EV_TAUNT_PATROL:
		DEBUGNAME("EV_TAUNT_PATROL");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONPATROL);
		break;
#endif
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event,  eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
		break;

	case EV_ITEM_PICKUP_SPEC:
		DEBUGNAME("EV_ITEM_PICKUP_SPEC");

		// fall through to EV_ITEM_PICKUP

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		//CG_PrintEntityStatep(es);
		{
			const gitem_t *item;
			int		index;
			qboolean play;

			if (cgs.cpma  ||  cgs.osp) {
				clientNum = CG_CheckClientEventCpma(clientNum, es);
			} else {
				if (event == EV_ITEM_PICKUP  &&  clientNum != es->number) {
					Com_Printf("^1EV_ITEM_PICKUP wtf cn %d !=  esn %d\n", clientNum, es->number);
				}
			}

			if (event == EV_ITEM_PICKUP_SPEC) {
				index = es->modelindex;
			} else {
				index = es->eventParm;		// player predicted
			}

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			if (event == EV_ITEM_PICKUP) {
				//Com_Printf("^2%d %d  num: %d %s\n", cg.time, cent - cg_entities, es->number, item->pickup_name);
				CG_TimedItemPickup(index, es->pos.trBase, clientNum, cg.time, qfalse);
			} else {  // EV_ITEM_PICKUP_SPEC
				//CG_TimedItemPickup(index, es->pos.trBase, clientNum, es->time, qtrue);
				if (1) {  //(es->time != 0) {
					CG_TimedItemPickup(index, es->pos.trBase, clientNum, es->time, qtrue);
				}
				//Com_Printf("^3%d spec pickup es->time %d  %s (%f %f %f)\n", cg.time, es->time, item->pickup_name, es->pos.trBase[0], es->pos.trBase[1], es->pos.trBase[2]);
			}

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if (event != EV_ITEM_PICKUP_SPEC) {
				if ( item->giType == IT_POWERUP || item->giType == IT_TEAM) {
					CG_StartSound (NULL, clientNum, CHAN_AUTO,	cgs.media.n_healthSound );
				} else if (item->giType == IT_PERSISTANT_POWERUP) {
					play = qtrue;
					if (cg_audioAnnouncer.integer == 0  ||  cg_audioAnnouncerPowerup.integer == 0) {
						play = qfalse;
					}

					if (item->giTag == PW_SCOUT) {
						if (play) {
							CG_StartSound (NULL, clientNum, CHAN_ANNOUNCER,	cgs.media.scoutSound );
						}
					} else if (item->giTag == PW_GUARD) {
						if (play) {
							CG_StartSound (NULL, clientNum, CHAN_ANNOUNCER,	cgs.media.guardSound );
						}
					} else if (item->giTag == PW_DOUBLER) {
						if (play) {
							CG_StartSound (NULL, clientNum, CHAN_ANNOUNCER,	cgs.media.doublerSound );
						}
					} else if (item->giTag == PW_ARMORREGEN) {
						if (play) {
							CG_StartSound (NULL, clientNum, CHAN_ANNOUNCER,	cgs.media.armorRegenSound );
						}
					} else if (item->giTag == PWEX_KEY) {
						CG_StartSound(NULL, clientNum, CHAN_AUTO, trap_S_RegisterSound(item->pickup_sound, qfalse));
					}
				} else {
					CG_StartSound (NULL, clientNum, CHAN_AUTO,	trap_S_RegisterSound( item->pickup_sound, qfalse ) );
				}
			}
			// show icon and name on status bar
			if (!wolfcam_following  &&  clientNum == cg.snap->ps.clientNum  &&  event != EV_ITEM_PICKUP_SPEC) {
				CG_ItemPickup( index );
			}
            //FIXME wolfcam wolfcam_following
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			const gitem_t *item;
			int		index;
			qboolean play;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			CG_TimedItemPickup(index, es->pos.trBase, clientNum, cg.time, qfalse);

#if 0  // testing powerupBlink
			if (1) {  //(clientNum == cg.snap->ps.clientNum) {
				if (item->giTag == PW_QUAD  ||  item->giTag == PW_BATTLESUIT  ||  item->giTag == PW_REGEN  ||  item->giTag == PW_ARMORREGEN) {
					cg.powerupActive = item->giTag;
					cg.powerupTime = cg.time;
				}
			}
#endif

			// powerup pickups are global
			play = qtrue;
			if (cg_audioAnnouncer.integer == 0  ||  cg_audioAnnouncerPowerup.integer == 0) {
				play = qfalse;
			}

			if (item->pickup_sound) {
				if (item->giTag == PW_QUAD) {
					if (play) {
						CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.quadPickupVo);
					}
				} else if (item->giTag == PW_BATTLESUIT) {
					if (play) {
						CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.battleSuitPickupVo);
					}
				} else if (item->giTag == PW_HASTE) {
					if (play) {
						CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.hastePickupVo);
					}
				} else if (item->giTag == PW_INVIS) {
					if (play) {
						CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.invisibilityPickupVo);
					}
				} else if (item->giTag == PW_REGEN) {
					if (play) {
						CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.regenPickupVo);
					}
				} else {
					// check if it is an audio announcer
					if (!Q_stricmpn(item->pickup_sound, "sound/vo", strlen("sound/vo"))) {
						CG_Printf("^3FIXME powerup pickup vo played without announcer disabled check\n");
					}
					//CG_Printf("item: %s\n", item->pickup_name);
					CG_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound(item->pickup_sound, qfalse));
				}
			}

			// show icon and name on status bar
			if ( !wolfcam_following  &&  es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
            //FIXME wolfcam wolfcam_following
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if ( !wolfcam_following  &&  es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
        //FIXME wolfcam wolfcam_following
		break;
	case EV_DROP_WEAPON:
		DEBUGNAME("EV_DROP_WEAPON");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		//FIXME some type of click sound is played?
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;
	case EV_FIRE_WEAPON: {
        //char buffer[4192];
        //char *s;

		DEBUGNAME("EV_FIRE_WEAPON");
		if (es->number >= MAX_CLIENTS) {
			if (cgs.protocol == PROTOCOL_QL  &&  es->weapon == WP_GRENADE_LAUNCHER) {
				// this is ok, can have 'world' weapons like the grenades
				// in silentnight.bsp
			} else {
				CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
			}
			//CG_PrintEntityStatep(es);
		}
        //Com_Printf("EV_FIRE_WEAPON: %d %d %p %s\n", clientNum, es->weapon, cent, cgs.clientinfo[clientNum].name);

#if 0
        s = va("EV_FIRE_WEAPON: %d %d %s\n", clientNum, es->weapon, cgs.clientinfo[clientNum].name);
		Com_Printf("%f %s", cg.ftime, s);
		// Q_strncpyz(buffer, s, sizeof(buffer));  //strcpy (buffer, s);
        //CG_Printf ("wc_logfile: %d\n", wc_logfile);
        //trap_FS_Write (buffer, strlen(buffer), wc_logfile);
#endif
		//CG_FireWeapon( cent );
		if (!cg.freezeEntity[es->number]) {
			CG_FireWeapon(&cg_entities[es->number]);
		}
		break;
    }
	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event,  eventName,es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM15:
		DEBUGNAME("EV_USE_ITEM15");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_UseItem( cent );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		if (es->number >= MAX_CLIENTS) {
			//CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (*EffectScripts.playerTeleportIn) {
			CG_ResetScriptVars();
			VectorCopy(position, ScriptVars.origin);
			CG_RunQ3mmeScript(EffectScripts.playerTeleportIn, NULL);
		} else {
			CG_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
			CG_SpawnEffect(position);
		}
		break;
	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		if (es->number >= MAX_CLIENTS) {
			//CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		if (*EffectScripts.playerTeleportOut) {
			CG_ResetScriptVars();
			VectorCopy(position, ScriptVars.origin);
			CG_RunQ3mmeScript(EffectScripts.playerTeleportOut, NULL);
		} else {
			CG_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
			CG_SpawnEffect(position);
		}
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		{
			const gitem_t *item;
			int index;
			int i;
			float len, newLen;
			int bestItem;
			vec3_t delta;

			//index = es->eventParm;
			index = es->modelindex;

			//Com_Printf("item respawn (%f, %f, %f)  modelindex:%d\n", es->pos.trBase[0], es->pos.trBase[1], es->pos.trBase[2], es->modelindex);

			if (index < 1  ||  index >= bg_numItems) {
				Com_Printf("item respawn bad index\n");
				break;
			}
			item = &bg_itemlist[index];

			cent->miscTime = cg.time;	// scale up from this
			//CG_Printf("item respawn %d %s  (%f, %f, %f)  quant:%d\n", es->number, item->pickup_name, es->pos.trBase[0], es->pos.trBase[1], es->pos.trBase[2], item->quantity);
			if (item->giType == IT_ARMOR  ||  item->giType == IT_HEALTH) {
				if (!Q_stricmp(item->pickup_name, "Mega Health")) {
					//Com_Printf("Mega Health respawn\n");
				} else if (!Q_stricmp(item->pickup_name, "Yellow Armor")) {
					//Com_Printf("Yellow Armor respawn\n");
					for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numYellowArmors;  i++) {
						VectorSubtract(es->pos.trBase, cg.yellowArmors[i].origin, delta);
						newLen = VectorLength(delta);
						if (newLen < len) {
							bestItem = i;
							len = newLen;
						}
					}

					cg.yellowArmors[bestItem].respawnTime = cg.time;

				} else if (!Q_stricmp(item->pickup_name, "Red Armor")) {
					//Com_Printf("Red Armor respawn\n");
				} else if (!Q_stricmp(item->pickup_name, "Quad Damage")) {
					//Com_Printf("Quad Damage respawn\n");
				}
			}

			if (item->giType == IT_POWERUP) {
				if (item->giTag == PW_QUAD) {
					//Com_Printf("quad respawn\n");
				} else if (item->giTag == PW_BATTLESUIT) {
					//Com_Printf("battle suit respawn\n");
				} else if (item->giTag == PW_REGEN) {
					//
				}
				//FIXME maybe battlesuit, regen
			}

			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
			break;
		}
	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		if ( rand() & 1 ) {
			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound );
		} else {
			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound );
		}
		break;

#if 1  //def MPACK
	case EV_PROXIMITY_MINE_STICK:
		DEBUGNAME("EV_PROXIMITY_MINE_STICK");
		if( es->eventParm & SURF_FLESH ) {
			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimplSound );
		} else 	if( es->eventParm & SURF_METALSTEPS ) {
			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpmSound );
		} else {
			CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpdSound );
		}
		break;

	case EV_PROXIMITY_MINE_TRIGGER:
		DEBUGNAME("EV_PROXIMITY_MINE_TRIGGER");
		CG_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbactvSound );
		break;
	case EV_KAMIKAZE:
		DEBUGNAME("EV_KAMIKAZE");
		CG_KamikazeEffect( cent->lerpOrigin );
		break;
	case EV_OBELISKEXPLODE:
		DEBUGNAME("EV_OBELISKEXPLODE");
		CG_ObeliskExplode( cent->lerpOrigin, es->eventParm );
		break;
	case EV_OBELISKPAIN:
		DEBUGNAME("EV_OBELISKPAIN");
		CG_ObeliskPain( cent->lerpOrigin );
		break;
	case EV_INVUL_IMPACT:
		DEBUGNAME("EV_INVUL_IMPACT");
		CG_InvulnerabilityImpact( cent->lerpOrigin, cent->currentState.angles );
		break;
	case EV_JUICED:
		DEBUGNAME("EV_JUICED");
		CG_Printf("^3FIXME juiced in quakelive\n");
		CG_InvulnerabilityJuiced( cent->lerpOrigin );
		break;
	case EV_LIGHTNINGBOLT:
		DEBUGNAME("EV_LIGHTNINGBOLT");
		CG_LightningBoltBeam(es->origin2, es->pos.trBase);
		break;

		//FIXME EV_LIGHTNING_DISCHARGE
#endif
	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		CG_ScorePlum( cent->currentState.otherEntityNum, cent->lerpOrigin, cent->currentState.time );
		break;

	case EV_DAMAGEPLUM: {
		DEBUGNAME("EV_DAMAGEPLUM");
		// generic1 is the weapon number
		// time is the damage amount

		CG_DamagePlum(cent->currentState.clientNum, cent->lerpOrigin, cent->currentState.time, cent->currentState.generic1);
		break;

	}

	//
	// missile impacts
	//
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitPlayer( es->weapon, position, dir, es->otherEntityNum, -1);
#if 0
		if (es->otherEntityNum >= 0  &&  es->otherEntityNum < MAX_CLIENTS) {
			cgs.clientinfo[es->otherEntityNum].hitTime = cg.ftime;
		}
#endif

		//Com_Printf("(es %d)  %d '%s' missile hit player %d '%s'\n", es->number, clientNum, cgs.clientinfo[clientNum].name, es->otherEntityNum, cgs.clientinfo[es->otherEntityNum].name);
		//CG_PrintEntityStatep(es);

        //FIXME wolfcam put in wolfcam_event.c
        Wolfcam_LogMissileHit(es->weapon, position, dir, es->otherEntityNum);
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir( es->eventParm, dir );
#if 0
		if (es->weapon == 0) {
			CG_PrintEntityStatep(es);
		}
#endif
		//Com_Printf("base %f %f %f\n", es->pos.trBase[0], es->pos.trBase[1], es->pos.trBase[2]);
		CG_MissileHitWall( es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT, qfalse );
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, 0, position, dir, IMPACTSOUND_METAL, qfalse );
		break;

	case EV_RAILTRAIL: {
		vec3_t origColor1;
		vec3_t origColor2;
		qboolean revertColors = qfalse;

		DEBUGNAME("EV_RAILTRAIL");
		cent->currentState.weapon = WP_RAILGUN;  // ???
		if (cg.freezeEntity[es->clientNum]) {
			return;
		}

		if (cg_railUseOwnColors.integer  &&  CG_IsUs(ci)) {
			VectorCopy(ci->color1, origColor1);
			VectorCopy(ci->color2, origColor2);
			//FIXME  bad..
			VectorCopy(cg.color1, ci->color1);
			VectorCopy(cg.color2, ci->color2);
			revertColors = qtrue;
		}
		if (cg_railFromMuzzle.integer) {
			vec3_t startPoint;

			if (wolfcam_following  &&  es->clientNum == wcg.clientNum  &&  wcg.clientNum != cg.snap->ps.clientNum) {
				//FIXME if 1st person weapon added
				VectorCopy(es->origin2, startPoint);
				startPoint[2] -= 4;
			} else {
				//FIXME what if player not added
				if (es->clientNum >= 0  &&  es->clientNum < MAX_CLIENTS  &&  wcg.inSnapshot[es->clientNum]) {
					CG_GetWeaponFlashOrigin(es->clientNum, startPoint);
				} else {
					VectorCopy(es->origin2, startPoint);
				}
			}
			CG_RailTrail(ci, startPoint, es->pos.trBase);
		} else {
			CG_RailTrail(ci, es->origin2, es->pos.trBase);
		}

		if (revertColors) {  //(cg_railUseOwnColors.integer  &&  CG_IsUs(ci)) {
			VectorCopy(origColor1, ci->color1);
			VectorCopy(origColor2, ci->color2);
		}

		// if the end was on a nomark surface, don't make an explosion
		if ( es->eventParm != 255 ) {
			//Com_Printf("byte dir %d\n", es->eventParm);
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( es->weapon, es->clientNum, position, dir, IMPACTSOUND_DEFAULT, qtrue );
		}

#if 1
		//Com_Printf("is ql demo: %d\n", cgs.isQuakeLiveDemo);
		//Com_Printf("check version: %d\n", CG_CheckQlVersion(0, 1, 0, 284));

		if (cg.demoPlayback  &&  cgs.isQuakeLiveDemo  &&  CG_CheckQlVersion(0, 1, 0, 284)) {
			// don't do anything, rail registers as a missile hit
		} else
        {
            trace_t tr;
            vec3_t or;
            vec3_t forward, right, up;
            //vec3_t newEnd;
            //vec3_t trBase;
            //vec3_t oldTrBase;
            //vec3_t nextTrBase;

            VectorCopy (es->origin2, or);

            if (clientNum == cg.snap->ps.clientNum) {
                AngleVectors (cg.snap->ps.viewangles, forward, right, up);
                //VectorCopy (cg.snap->ps.origin, trBase);
                //VectorCopy (wclients[clientNum].oldState.pos.trBase, oldTrBase);
                //VectorCopy (cg_entities[clientNum].nextState.pos.trBase, nextTrBase);
            } else {
                AngleVectors (cg_entities[clientNum].currentState.apos.trBase, forward, right, up);
                //VectorCopy (cg_entities[clientNum].currentState.pos.trBase, trBase);
                //VectorCopy (wclients[clientNum].oldState.pos.trBase, oldTrBase);
                //VectorCopy (cg_entities[clientNum].nextState.pos.trBase, nextTrBase);
            }
            VectorMA (or, 1, up, or);
            VectorMA (or, -4, right, or);

            //VectorMA (or, 131072, forward, newEnd);
#if 0
            if (wolfcam_following  &&  clientNum == wcg.clientNum) {
                CG_Printf ("EV_RAILTRAIL: %d  es->origin2 (%f, %f, %f)  or (%f, %f, %f)  pos.trBase (%f, %f, %f)  old.pos.trBase (%f, %f, %f)  %s\n",
                           clientNum,
                           es->origin2[0], es->origin2[1], es->origin2[2],
                           or[0], or[1], or[2],
                           trBase[0], trBase[1], trBase[2],
                           oldTrBase[0], oldTrBase[1], oldTrBase[2],
                           //nextTrBase[0], nextTrBase[1], nextTrBase[2],
                           cgs.clientinfo[clientNum].name);
            }
#endif

            //            Wolfcam_WeaponTrace (&tr, or /*es->origin2*/, NULL, NULL, es->pos.trBase, clientNum, CONTENTS_SOLID | CONTENTS_BODY);
            //Wolfcam_WeaponTrace (&tr, or /*es->origin2*/, NULL, NULL, newEnd, clientNum, CONTENTS_SOLID | CONTENTS_BODY);
            Wolfcam_WeaponTrace (&tr, or /*es->origin2*/, NULL, NULL, es->pos.trBase, clientNum, CONTENTS_SOLID | CONTENTS_BODY);
            if (tr.entityNum < MAX_CLIENTS) {
                //CG_Printf ("  ^2logging trace hit to %d  %s for %s\n", tr.entityNum, cgs.clientinfo[tr.entityNum].name, cgs.clientinfo[clientNum].name);
                wclients[clientNum].wstats[WP_RAILGUN].hits++;
                wclients[clientNum].perKillwstats[WP_RAILGUN].hits++;
                if (wolfcam_following  &&  clientNum == wcg.clientNum) {
                    //CG_Printf ("  ^3xxxx hit %d  %s\n", tr.entityNum, cgs.clientinfo[tr.entityNum].name);
					if (cgs.gametype >= GT_TEAM) {
						if (cgs.clientinfo[tr.entityNum].infoValid  &&  cgs.clientinfo[tr.entityNum].team != cgs.clientinfo[clientNum].team) {
							wcg.playHitSound = qtrue;
						} else if (cgs.clientinfo[tr.entityNum].infoValid) {
							wcg.playTeamHitSound = qtrue;
						}
					} else {
						//FIXME check if it's a corpse?
						if (cg_entities[tr.entityNum].currentState.eFlags & EF_DEAD) {
							//Com_Printf("FIXME rail corpse hit sound\n");
						} else {
							wcg.playHitSound = qtrue;
						}
					}
				}
			}
		}
#endif
		break;
	}
	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD );
		//Com_Printf("bullet hit wall\n");
		break;

	case EV_BULLET_HIT_FLESH:
		//FIXME check if ql only uses this for mg
		DEBUGNAME("EV_BULLET_HIT_FLESH");
		//Com_Printf("^2EV_BULLLET_HIT_FLESH  %d\n", es->weapon);

		//CG_PrintEntityState(cent->currentState.number);
		//FIXME fuck, chaingun
		if (es->otherEntityNum >= 0  &&  es->otherEntityNum < MAX_CLIENTS) {
			int weapon;

			weapon = cg_entities[es->otherEntityNum].currentState.weapon;
			if (weapon == WP_MACHINEGUN) {
				wclients[es->otherEntityNum].wstats[WP_MACHINEGUN].hits++;
				wclients[es->otherEntityNum].perKillwstats[WP_MACHINEGUN].hits++;
			} else if (weapon == WP_CHAINGUN) {
				wclients[es->otherEntityNum].wstats[WP_CHAINGUN].hits++;
				wclients[es->otherEntityNum].perKillwstats[WP_CHAINGUN].hits++;
			} else if (weapon == WP_HEAVY_MACHINEGUN) {
				wclients[es->otherEntityNum].wstats[WP_HEAVY_MACHINEGUN].hits++;
				wclients[es->otherEntityNum].perKillwstats[WP_HEAVY_MACHINEGUN].hits++;
			}
		}
        if (wolfcam_following) {
            if (es->otherEntityNum == wcg.clientNum) {
                //CG_Printf ("hitting  %d!!!\n", es->weapon);
                //CG_StartLocalSound( cgs.media.hitSound, CHAN_LOCAL_SOUND);
				if (cgs.gametype < GT_TEAM) {
					//FIXME check if corpse
					wcg.playHitSound = qtrue;
				} else if (cgs.gametype >= GT_TEAM) {
					if (es->eventParm < MAX_CLIENTS  &&  cgs.clientinfo[es->eventParm].infoValid) {
						if (cgs.clientinfo[es->eventParm].team != cgs.clientinfo[wcg.clientNum].team) {
							//FIXME check if corpse
							wcg.playHitSound = qtrue;
						} else {
							wcg.playTeamHitSound = qtrue;
						}
					}
				}
            }
        }
        if (CG_ClientInSnapshot(es->otherEntityNum)  &&  CG_ClientInSnapshot(es->eventParm))
        {
            vec3_t vec, enemyvec, angs;
            //entityState_t *e = &cg_entities[es->otherEntityNum].currentState;
            //static FILE *datafile = NULL;

            //if (!datafile) {
            //    fopen ("/tmp/shotdata.txt", "w");
            //}

            VectorCopy (cg_entities[es->eventParm].lerpOrigin, enemyvec);
            //FIXME ducked
            VectorSubtract (enemyvec, cg_entities[es->otherEntityNum].lerpOrigin, vec);
            vectoangles (vec, angs);
            //CG_Printf ("angs: %f %f ^3%f ^3%f\n", angs[0], angs[1], cg_entities[es->otherEntityNum].lerpAngles[0] - angs[0], cg_entities[es->otherEntityNum].lerpAngles[1] - angs[1]);
            //fprintf (datafile, "%d %s\n", es->otherEntityNum, cgs.clientinfo[es->otherEntityNum].name);
            //fprintf (datafile, "%f %f\n", cg_entities[es->otherEntityNum].lerpAngles[0] - angs[0], cg_entities[es->otherEntityNum].lerpAngles[1] - angs[1]);

#if 0
            CG_Printf ("EV_BULLET_HIT_FLESH: %d %s\n", es->otherEntityNum, cgs.clientinfo[es->otherEntityNum].name);
            CG_Printf ("%f %f\n", cg_entities[es->otherEntityNum].lerpAngles[0] - angs[0], cg_entities[es->otherEntityNum].lerpAngles[1] - angs[1]);
#endif

#if 0
            CG_Printf ("EV_BULLET_HIT_FLESH: %d  time:%d  time2:%d  snapnum:%d  cg.time:%d  %s\n", es->otherEntityNum, es->time, es->time2, cgs.processedSnapshotNum, cg.time, cgs.clientinfo[es->otherEntityNum].name);
            CG_Printf ("    (%f %f %f)  (%f %f %f)\n",
                       e->pos.trBase[0], e->pos.trBase[1], e->pos.trBase[2],
                       e->apos.trBase[0], e->apos.trBase[1], e->apos.trBase[2]);
            CG_Printf ("    (%f %f %f)  (%f %f %f)\n",
                       cg_entities[es->otherEntityNum].lerpOrigin[0], cg_entities[es->otherEntityNum].lerpOrigin[1], cg_entities[es->otherEntityNum].lerpOrigin[2],
                       cg_entities[es->otherEntityNum].lerpAngles[0], cg_entities[es->otherEntityNum].lerpAngles[1], cg_entities[es->otherEntityNum].lerpAngles[2]
                       );
#endif
        }

		if (*EffectScripts.weapons[WP_MACHINEGUN].impactFleshScript) {
			int clientNum;

			//clientNum = es->otherEntityNum;  //es->eventParm;
			clientNum = es->eventParm;
			if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
				clientNum = 0;
			}

			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(&cg_entities[clientNum]);
			VectorCopy(es->pos.trBase, ScriptVars.origin);
			VectorCopy(dir, ScriptVars.dir);
			VectorSet(ScriptVars.end, 0, 0, 0);
			if (CG_ClientInSnapshot(es->otherEntityNum)) {
				VectorSubtract(cg_entities[es->otherEntityNum].lerpOrigin, es->pos.trBase, ScriptVars.end);
			} else {
				//Com_Printf("^1%d not valid end\n", es->otherEntityNum);
			}
			CG_RunQ3mmeScript(EffectScripts.weapons[WP_MACHINEGUN].impactFleshScript, NULL);
		} else if (*EffectScripts.impactFlesh) {
			int clientNum;

			//clientNum = es->otherEntityNum;  //es->eventParm;
			clientNum = es->eventParm;
			if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
				clientNum = 0;
				//Com_Printf("wtf... %d\n", es->eventParm);
			}

			CG_ResetScriptVars();
			CG_CopyPlayerDataToScriptData(&cg_entities[clientNum]);
			VectorCopy(es->pos.trBase, ScriptVars.origin);
			VectorCopy(dir, ScriptVars.dir);
			VectorSet(ScriptVars.end, 0, 0, 0);
			if (CG_ClientInSnapshot(es->otherEntityNum)) {
				VectorSubtract(cg_entities[es->otherEntityNum].lerpOrigin, es->pos.trBase, ScriptVars.end);
				//Com_Printf("^2%d valid end\n", es->otherEntityNum);
			} else {
				//Com_Printf("^1%d not valid end %s\n", es->otherEntityNum, cgs.clientinfo[es->otherEntityNum].name);
			}
			CG_RunQ3mmeScript((char *)EffectScripts.impactFlesh, NULL);
		} else {
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm );
			CG_ImpactSparks(WP_MACHINEGUN, es->pos.trBase, dir, es->eventParm);
		}
		break;

		//FIXME EV_SHOTGUN_KILL
	case EV_SHOTGUN:
		DEBUGNAME("EV_SHOTGUN");
		//CG_PrintEntityStatep(es);
		CG_ShotgunFire( es );
        //CG_Printf ("EV_SHOTGUN: es->otherEntityNum: %d %s\n", es->otherEntityNum, cgs.clientinfo[es->otherEntityNum].name);
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		{
			int n;

			n = es->eventParm;  // - 1;
			//Com_Printf("general sound %d  %s\n", n, CG_ConfigString(CS_SOUNDS + n - 1));

			if ( cgs.gameSounds[ n ] ) {
				if (cg_ambientSounds.integer == 0) {
					break;
				}

				if (cg_ambientSounds.integer == 2  &&  !CG_AllowedAmbientSound(cgs.gameSounds[n])) {
					break;
				}

				//Com_Printf("playing....\n");

				CG_StartSound (NULL, es->number, CHAN_VOICE, cgs.gameSounds[ n ] );
			} else {
				if (cgs.protocol == PROTOCOL_QL) {
					s = CG_ConfigString( CS_SOUNDS + n - 1);
				} else {
					s = CG_ConfigString(CS_SOUNDS + n);
				}

				//Com_Printf("^2(player) general sound playing: %s\n", s);
				CG_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
			}
			break;
	}
	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
	{
		int n;

		n = es->eventParm;  // - 1;

		//FIXME which sounds ok to play if following
		//Com_Printf("global sound %d\n", n);
		//Com_Printf("global sound %d  %s\n", n, CG_ConfigString(CS_SOUNDS + n - 1));
		//Com_Printf("playing %d  %s\n", n, CG_ConfigString(CS_SOUNDS + n));

		if ( cgs.gameSounds[ n ] ) {
			if (cg_ambientSounds.integer == 0) {
				break;
			}
			if (cg_ambientSounds.integer == 2  &&  !CG_AllowedAmbientSound(cgs.gameSounds[n])) {
				break;
			}

            if (!wolfcam_following)
                CG_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ n ] );
            else
                CG_StartSound (NULL, wcg.clientNum, CHAN_AUTO, cgs.gameSounds[n]);
		} else {
			if (cgs.protocol == PROTOCOL_QL) {
				s = CG_ConfigString( CS_SOUNDS + n - 1);
			} else {
				s = CG_ConfigString(CS_SOUNDS + n);
			}
			//Com_Printf("^2(player) global sound custom '%s' %d\n", s, n);

            if (!wolfcam_following)
                CG_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
            else
                CG_StartSound (NULL, wcg.clientNum, CHAN_AUTO, CG_CustomSound(es->number, s));
		}
	}
		break;

	case EV_GLOBAL_TEAM_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		{
			int cn;
			//int powerups;
			int team;

			//Com_Printf("global team sound %d  %s\n", es->eventParm, CG_ConfigString( CS_SOUNDS + es->eventParm - 1));
			if (cg.snap->ps.pm_type == PM_INTERMISSION) {
				break;
			}

			if (wolfcam_following) {
				cn = wcg.clientNum;
				//powerups = cg_entities[cn].currentState.powerups;
			} else {
				cn = cg.snap->ps.clientNum;
				//powerups = cg.snap->ps.powerups;  // array in ps
			}

			team = cgs.clientinfo[cn].team;
			if (cgs.cpma  &&  (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF)) {
#if 0
				if (team == TEAM_RED) {
					team = TEAM_BLUE;
				} else if (team == TEAM_BLUE) {
					team = TEAM_RED;
				}
#endif
				//Com_Printf("^6eventParm %d\n", es->eventParm);
				switch (es->eventParm) {
				case GTS_RED_CAPTURE: // CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
					if ( team == TEAM_RED )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_BLUE_CAPTURE: // CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
					if ( team == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_RED_RETURN: // red flag returned
					if ( team == TEAM_RED )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					if (cg_audioAnnouncerFlagStatus.integer) {
						if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.yourFlagReturnedSound);
						} else if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.enemyFlagReturnedSound);
						} else {
							CG_AddBufferedSound( cgs.media.redFlagReturnedSound );
						}
					}
					break;
				case GTS_BLUE_RETURN: // blue flag returned
					if ( team == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					if (cg_audioAnnouncerFlagStatus.integer) {
						if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.yourFlagReturnedSound);
						} else if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.enemyFlagReturnedSound);
						} else {
							CG_AddBufferedSound( cgs.media.blueFlagReturnedSound );
						}
					}
					break;

				case GTS_RED_TAKEN: // red flag taken
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
						//FIXME wolfcam what?
					}
					else {
						if (cg_audioAnnouncerFlagStatus.integer) {
							if (team == TEAM_BLUE) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
							}
							else if (team == TEAM_RED) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
							}
						}  // audio announcer

						if (cg_flagTakenSound.integer) {
							if (team == TEAM_BLUE) {
								CG_AddBufferedSound(cgs.media.takenOpponentSound);
							} else if (team == TEAM_RED) {
								CG_AddBufferedSound(cgs.media.takenYourTeamSound);
							}

							//FIXME team spec?

						}  // flag taken sound
					}
					break;
				case GTS_BLUE_TAKEN: // blue flag taken
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
						//FIXME wolfcam what?
					}
					else {
						if (cg_audioAnnouncerFlagStatus.integer) {
							if (team == TEAM_RED) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
							}
							else if (team == TEAM_BLUE) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
							}
						}  // audio announcer

						if (cg_flagTakenSound.integer) {
							if (team == TEAM_RED) {
								CG_AddBufferedSound(cgs.media.takenOpponentSound);
							} else if (team == TEAM_BLUE) {
								CG_AddBufferedSound(cgs.media.takenYourTeamSound);
							}
						}
					}
					break;
#if 1  //def MPACK
				case GTS_REDOBELISK_ATTACKED: // Overload: red obelisk is being attacked
					if (team == TEAM_RED) {
						CG_AddBufferedSound( cgs.media.yourBaseIsUnderAttackSound );
					}
					break;
				case GTS_BLUEOBELISK_ATTACKED: // Overload: blue obelisk is being attacked
					if (team == TEAM_BLUE) {
						CG_AddBufferedSound( cgs.media.yourBaseIsUnderAttackSound );
					}
					break;
#endif
				case GTS_REDTEAM_SCORED:
					if (cg_audioAnnouncerScore.integer) {
						if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.yourTeamScoredSound);
						} else if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.enemyTeamScoredSound);
						} else {  // spec
							CG_AddBufferedSound(cgs.media.redScoredSound);
						}
					}
					if (cgs.gametype == GT_FREEZETAG) {
						CG_CenterPrint("Red Scores", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
					break;
				case GTS_REDTEAM_TOOK_LEAD:  //GTS_BLUETEAM_SCORED:  //FIXME
					//FIXME 2015-07-16 change index enums
					if (cg_audioAnnouncerScore.integer) {
						if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.yourTeamScoredSound);
						} else if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.enemyTeamScoredSound);
						} else {  // spec
							CG_AddBufferedSound(cgs.media.blueScoredSound);
						}
					}
					if (cgs.gametype == GT_FREEZETAG) {
						CG_CenterPrint("Blue Scores", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
					break;
				case 999:  //GTS_REDTEAM_TOOK_LEAD:  //FIXME
					//FIXME 2015-07-16 set index enum
					if (cg_audioAnnouncerLead.integer) {
						CG_AddBufferedSound(cgs.media.redLeadsSound);
					}
					break;
				case GTS_BLUETEAM_TOOK_LEAD:
					if (cg_audioAnnouncerLead.integer) {
						CG_AddBufferedSound(cgs.media.blueLeadsSound);
					}
					break;
				case GTS_TEAMS_ARE_TIED:
					if (cg_audioAnnouncerLead.integer) {
						CG_AddBufferedSound( cgs.media.teamsTiedSound );
					}
					break;
#if  1  //def MPACK
				case GTS_KAMIKAZE:
					CG_StartLocalSound(cgs.media.kamikazeFarSound, CHAN_ANNOUNCER);
					break;
#endif
				default:
					CG_Printf("^3cpma unknown global team sound: %d\n", es->eventParm);
					//CG_PrintEntityStatep(es);
					break;
				}

				return;
			}  // end cpma sounds

            //FIXME wolfcam cg.clientNum
			switch( es->eventParm ) {
				case GTS_RED_CAPTURE: // CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
					//FIXME what sound to play for spec?
					if ( team == TEAM_RED )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_BLUE_CAPTURE: // CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
					if ( team == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_RED_RETURN: // CTF: blue flag returned, 1FCTF: never used
					//FIXME spec return sound?

					if ( team == TEAM_RED )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					if (cg_audioAnnouncerFlagStatus.integer) {
						if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.yourFlagReturnedSound);
						} else if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.enemyFlagReturnedSound);
						} else {
							CG_AddBufferedSound( cgs.media.blueFlagReturnedSound );
						}
					}
					break;
				case GTS_BLUE_RETURN: // CTF red flag returned, 1FCTF: neutral flag returned
					if ( team == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					if (cg_audioAnnouncerFlagStatus.integer) {
						if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.yourFlagReturnedSound);
						} else if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.enemyFlagReturnedSound);
						} else {
							CG_AddBufferedSound( cgs.media.redFlagReturnedSound );
						}
					}
					break;

				case GTS_RED_TAKEN: // CTF: red team took blue flag, 1FCTF: blue team took the neutral flag
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
						//FIXME wolfcam what?
					}
					else {
						if (cg_audioAnnouncerFlagStatus.integer  &&  cgs.gametype != GT_CTFS) {
							if (team == TEAM_BLUE) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
							}
							else if (team == TEAM_RED) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
							}
						}  // audio announcer

						if (cg_flagTakenSound.integer) {
							if (team == TEAM_BLUE) {
								CG_AddBufferedSound(cgs.media.takenOpponentSound);
							} else if (team == TEAM_RED) {
								CG_AddBufferedSound(cgs.media.takenYourTeamSound);
							}

							//FIXME team spec?

						}  // flag taken sound

					}
					break;
				case GTS_BLUE_TAKEN: // CTF: blue team took the red flag, 1FCTF red team took the neutral flag
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
						//FIXME wolfcam what?
					}
					else {
						if (cg_audioAnnouncerFlagStatus.integer  &&  cgs.gametype != GT_CTFS) {
							if (team == TEAM_RED) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
							}
							else if (team == TEAM_BLUE) {
								if (cgs.gametype == GT_1FCTF)
									CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
								else
									CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
							}
						}  // audio announcer

						if (cg_flagTakenSound.integer) {
							if (team == TEAM_RED) {
								CG_AddBufferedSound(cgs.media.takenOpponentSound);
							} else if (team == TEAM_BLUE) {
								CG_AddBufferedSound(cgs.media.takenYourTeamSound);
							}
						}

					}
					break;
				case GTS_REDOBELISK_ATTACKED: // Overload: red obelisk is being attacked
					if (team == TEAM_RED) {
						CG_AddBufferedSound( cgs.media.yourBaseIsUnderAttackSound );
					}
					break;
				case GTS_BLUEOBELISK_ATTACKED: // Overload: blue obelisk is being attacked
					if (team == TEAM_BLUE) {
						CG_AddBufferedSound( cgs.media.yourBaseIsUnderAttackSound );
					}
					break;

				case GTS_REDTEAM_SCORED:
					if (cg_audioAnnouncerScore.integer  &&  cgs.gametype != GT_CTFS) {
						if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.yourTeamScoredSound);
						} else if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.enemyTeamScoredSound);
						} else {  // spec
							CG_AddBufferedSound(cgs.media.redScoredSound);
						}
					}
					if (cgs.gametype == GT_FREEZETAG) {
						CG_CenterPrint("Red Scores", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
					break;
				case GTS_BLUETEAM_SCORED:
					if (cg_audioAnnouncerScore.integer  &&  cgs.gametype != GT_CTFS) {
						if (team == TEAM_BLUE) {
							CG_AddBufferedSound(cgs.media.yourTeamScoredSound);
						} else if (team == TEAM_RED) {
							CG_AddBufferedSound(cgs.media.enemyTeamScoredSound);
						} else {  // spec
							CG_AddBufferedSound(cgs.media.blueScoredSound);
						}
					}
					if (cgs.gametype == GT_FREEZETAG) {
						CG_CenterPrint("Blue Scores", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
					break;
				case GTS_REDTEAM_TOOK_LEAD:
					if (cg_audioAnnouncerLead.integer  &&  cgs.gametype != GT_CTFS) {
						CG_AddBufferedSound(cgs.media.redLeadsSound);
					}
					break;
				case GTS_BLUETEAM_TOOK_LEAD:
					if (cg_audioAnnouncerLead.integer  &&  cgs.gametype != GT_CTFS) {
						CG_AddBufferedSound(cgs.media.blueLeadsSound);
					}
					break;
				case GTS_TEAMS_ARE_TIED:
					if (cg_audioAnnouncerLead.integer  &&  cgs.gametype != GT_CTFS) {
						CG_AddBufferedSound( cgs.media.teamsTiedSound );
					}
					break;
#if  1  //def MPACK
				case GTS_KAMIKAZE:
					CG_StartLocalSound(cgs.media.kamikazeFarSound, CHAN_ANNOUNCER);
					break;
#endif

			case GTS_BLUE_WINS_ROUND:
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound(cgs.media.blueWinsRoundSound, CHAN_ANNOUNCER);
				}
				CG_CenterPrint("Blue Wins the Round", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				break;
			case GTS_RED_WINS_ROUND:
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound(cgs.media.redWinsRoundSound, CHAN_ANNOUNCER);
				}
				CG_CenterPrint("Red Wins the Round", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				break;
			case GTS_ROUND_DRAW:
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound(cgs.media.roundDrawSound, CHAN_ANNOUNCER);
				}
				CG_CenterPrint("Round Draw", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				break;
			case GTS_BLUE_WINS:
				if (cg_audioAnnouncerWin.integer) {
					CG_StartLocalSound(cgs.media.blueWinsSound, CHAN_ANNOUNCER);
				}
				break;
			case GTS_RED_WINS:
				if (cg_audioAnnouncerWin.integer) {
					CG_StartLocalSound(cgs.media.redWinsSound, CHAN_ANNOUNCER);
				}
				break;
			case GTS_LAST_STANDING:
				//CG_PrintEntityStatep(es);
				if (cg_audioAnnouncerLastStanding.integer) {
					// check for es->modelindex2 when this vo was originally
					// added team wasn't passed
					if (!wolfcam_following  &&  (cgs.clientinfo[cg.snap->ps.clientNum].team == es->modelindex2  ||  es->modelindex2 == 0)) {
						CG_StartLocalSound(cgs.media.lastStandingSound, CHAN_ANNOUNCER);
					}
				}
				break;
			case GTS_ROUND_OVER: {
				const char *s;
				char *s2;
				char snum[128];
				char buffer[MAX_STRING_CHARS];
				int n;

				if (cg_audioAnnouncerRound.integer) {
					if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg.infectedSnapshotTime == cg.snap->serverTime) {
						// pass, playing 'infected' sound instead
					} else {
						CG_StartLocalSound(cgs.media.roundOverSound, CHAN_ANNOUNCER);
					}
				}
				//CG_CenterPrint("Round Over", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				//CG_CenterPrint("Round Over", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);

				buffer[0] = '\0';
				strcat(buffer, "Round Winners:\n");
				s = CG_ConfigString(CS_ROUND_WINNERS);
				s2 = snum;
				while (*s) {
					s2[0] = s[0];
					if (s2[0] == ' '  ||  s2[0] == '\0') {
						s2[0] = '\0';
						n = atoi(snum);
						if (n >= 0  &&  n < MAX_CLIENTS) {
							strcat(buffer, cgs.clientinfo[n].name);
							strcat(buffer, "\n");
						}

						s2 = snum;
					}
					s2++;
					s++;
				}

				CG_CenterPrint(va("%s", buffer), SCREEN_HEIGHT * 0.20, BIGCHAR_WIDTH);
				break;
			}
			case GTS_DOMINATION_POINT_CAPTURE: {
				const char *teamName;
				const char *pointName;
				int ourTeam;

				if (cg.warmup) {
					break;
				}

				if (wolfcam_following) {
					ourTeam = cgs.clientinfo[wcg.clientNum].team;
				} else {
					ourTeam = cg.snap->ps.persistant[PERS_TEAM];
				}

				teamName = "";
				if (es->modelindex2 == TEAM_BLUE) {
					teamName = "Blue";
				} else {
					teamName = "Red";
				}

				switch (es->powerups) {
				case 1:
					pointName = "A";
					break;
				case 2:
					pointName = "B";
					break;
				case 3:
					pointName = "C";
					break;
				case 4:
					pointName = "D";
					break;
				case 5:
					pointName = "E";
					break;
				default:
					pointName = "";
					break;
				}

				if (ourTeam == TEAM_RED  ||  ourTeam == TEAM_BLUE) {
					if (cg_audioAnnouncerDominationPoint.integer) {
						if (es->modelindex2 == ourTeam) {  // captured
							switch (es->powerups) {
							case 1:
								CG_StartLocalSound(cgs.media.aCapturedSound, CHAN_ANNOUNCER);
								break;
							case 2:
								CG_StartLocalSound(cgs.media.bCapturedSound, CHAN_ANNOUNCER);
								break;
							case 3:
								CG_StartLocalSound(cgs.media.cCapturedSound, CHAN_ANNOUNCER);
								break;
							case 4:
								CG_StartLocalSound(cgs.media.dCapturedSound, CHAN_ANNOUNCER);
								break;
							case 5:
								CG_StartLocalSound(cgs.media.eCapturedSound, CHAN_ANNOUNCER);
								break;
							default:
								break;
							}
						} else {  // lost
							switch (es->powerups) {
							case 1:
								CG_StartLocalSound(cgs.media.aLostSound, CHAN_ANNOUNCER);
								break;
							case 2:
								CG_StartLocalSound(cgs.media.bLostSound, CHAN_ANNOUNCER);
								break;
							case 3:
								CG_StartLocalSound(cgs.media.cLostSound, CHAN_ANNOUNCER);
								break;
							case 4:
								CG_StartLocalSound(cgs.media.dLostSound, CHAN_ANNOUNCER);
								break;
							case 5:
								CG_StartLocalSound(cgs.media.eLostSound, CHAN_ANNOUNCER);
								break;
							default:
								break;
							}
						}
					}
				}

				CG_CenterPrint(va("%s captured point %s!", teamName, pointName), SCREEN_HEIGHT * 0.20, BIGCHAR_WIDTH);
				break;
			}
			case GTS_ENEMY_TEAM_KILLED: {
				if (wolfcam_following) {
					if (cgs.clientinfo[wcg.clientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team) {
						break;
					}
				}
				if (cg_audioAnnouncerRoundReward.integer) {
					CG_StartLocalSound(cgs.media.perfectSound, CHAN_ANNOUNCER);
				}
				break;
			}
			case GTS_DENIED: {
				//FIXME check if it is team specific
				//FIXME err.. also check if this is a bug...
				//CG_PrintEntityStatep(&cent->currentState);
				if (wolfcam_following) {
					if (cgs.clientinfo[wcg.clientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team) {
						break;
					}
				}
				if (cg_audioAnnouncerRoundReward.integer) {
					CG_StartLocalSound(cgs.media.deniedSound, CHAN_ANNOUNCER);
				}
				break;
			}
			case GTS_PLAYER_INFECTED: {
				// red rover infected
				//Com_Printf("^3infected:\n");
				//CG_PrintEntityStatep(es);
#if 0
				if (es->modelindex2 != TEAM_BLUE) {
					Com_Printf("^1index != 2\n");
				}
#endif
				if (es->modelindex2 == TEAM_BLUE  &&  team == TEAM_BLUE) {
					CG_StartLocalSound(cgs.media.survivorSound, CHAN_LOCAL);
				}
				break;
			}
			default:
				CG_Printf("^3unknown global team sound: %d\n", es->eventParm);
				//CG_PrintEntityStatep(es);
				break;
			}
			break;
		}

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
        //FIXME wolfcam
		DEBUGNAME("EV_PAIN");
		//Com_Printf("ev pain esn %d  cn %d\n", es->number, es->clientNum);
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
			cent = &cg_entities[clientNum];
		}

		if ( clientNum != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
			es = &cg_entities[clientNum].currentState;
		}

		//CG_PrintEntityStatep(es);

		if (cgs.protocol == PROTOCOL_Q3  &&  CG_WaterLevel(&cg_entities[clientNum]) == 3) {
			CG_StartSound(NULL, clientNum, CHAN_VOICE, CG_CustomSound(clientNum, "*drown.wav"));
		} else {
			CG_StartSound( NULL, clientNum, CHAN_VOICE,
				CG_CustomSound( clientNum, va("*death%i.wav", event - EV_DEATH1 + 1) ) );
		}

		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		if (es->otherEntityNum < 0  ||  es->otherEntityNum >= MAX_CLIENTS) {
			CG_Printf("^3EV_OBITUARY invalid client number %d\n", es->otherEntityNum);
		} else {
			playerEntity_t *pe;

			if (es->otherEntityNum == cg.snap->ps.clientNum) {
				pe = &cg.predictedPlayerEntity.pe;
			} else {
				pe = &cg_entities[es->otherEntityNum].pe;
			}
			pe->deathTime = cg.time;

			wclients[es->otherEntityNum].painValueTime = 0;
		}
		CG_Obituary( es );
		//Com_Printf("^3 after CG_Obituary(), cg.obituaryIndex %d  time: %d\n", cg.obituaryIndex, cg.obituaries[(cg.obituaryIndex - 1) % MAX_OBITUARIES].time);
		break;

	case EV_DROWN:
		DEBUGNAME("EV_DROWN");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		CG_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*drown.wav"));
		break;
	//
	// powerup events
	//
	case EV_POWERUP_QUAD: {
		int i;
		int bestItem;
		vec3_t delta;
		float len;
		float newLen;

		DEBUGNAME("EV_POWERUP_QUAD");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}
		if ( clientNum == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_QUAD;
			cg.powerupTime = cg.time;
		}
		CG_StartSound (NULL, clientNum, CHAN_ITEM, cgs.media.quadSound );

		//Com_Printf("EV_POWERUP_QUAD\n");
		for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numQuads;  i++) {
			VectorSubtract(es->pos.trBase, cg.quads[i].origin, delta);
			newLen = VectorLength(delta);
			if (newLen < len) {
				bestItem = i;
				len = newLen;
			}
		}
		// quad can be picked up again
		//t = ((cg.redArmors[i].pickupTime / 1000) + cg.redArmors[i].respawnLength) - (cg.time / 1000);

		//if ((cg.time - cg.quads[bestItem].pickupTime) / 1000 >= cg.quads[bestItem].respawnLength) {
		if ( (cg.quads[bestItem].pickupTime / 1000) + cg.quads[bestItem].respawnLength <= (cg.time / 1000)) {
			//Com_Printf("quad pickup\n");
			cg.quads[bestItem].pickupTime = cg.time;
		} else {
			//Com_Printf("quad repicked\n");
		}

		break;
	}
	case EV_POWERUP_BATTLESUIT: {
		int i;
		int bestItem;
		vec3_t delta;
		float len;
		float newLen;

		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}
		if ( clientNum == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		CG_StartSound (NULL, clientNum, CHAN_ITEM, cgs.media.protectSound );

		for (i = 0, len = 99999.0, bestItem = 0;  i < cg.numBattleSuits;  i++) {
			VectorSubtract(es->pos.trBase, cg.battleSuits[i].origin, delta);
			newLen = VectorLength(delta);
			if (newLen < len) {
				bestItem = i;
				len = newLen;
			}
		}

		cg.battleSuits[bestItem].pickupTime = cg.time;
		if ( (cg.battleSuits[bestItem].pickupTime / 1000) + cg.battleSuits[bestItem].respawnLength < (cg.time / 1000)) {
			//Com_Printf("quad pickup\n");
			cg.battleSuits[bestItem].pickupTime = cg.time;
		}

		break;
	}
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}
		if ( clientNum == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		CG_StartSound (NULL, clientNum, CHAN_ITEM, cgs.media.regenSound );
		break;
	case EV_POWERUP_ARMOR_REGEN:
		DEBUGNAME("EV_POWERUP_ARMOR_REGEN");
		if (cgs.cpma  ||  cgs.osp) {
			clientNum = CG_CheckClientEventCpma(clientNum, es);
		}
		if (clientNum == cg.snap->ps.clientNum) {
			cg.powerupActive = PW_ARMORREGEN;
			cg.powerupTime = cg.time;
		}
		CG_StartSound(NULL, clientNum, CHAN_ITEM, cgs.media.armorRegenSoundRegen);
		break;
	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		if (es->number >= MAX_CLIENTS) {
			if (1) {  //(!cgs.protocol == PROTOCOL_QL) {
				// can't get client number in ql
				//CG_Printf("^3FIXME event %d  %s  num %d  clientNum %d\n", event, eventName, es->number, es->clientNum);
				//CG_PrintEntityStatep(es);
			}
		}

		// don't play gib sound when using the kamikaze because it interferes
		// with the kamikaze sound, downside is that the gib sound will also
		// not be played when someone is gibbed while just carrying the kamikaze
		if (cgs.cpma  ||  cgs.osp) {
			//clientNum = CG_CheckClientEventCpma(clientNum, es);
			//cent = &cg_entities[clientNum];
		}

		if (cgs.gametype == GT_FREEZETAG) {
			Com_Printf("^5skipping extra gib....\n");
			break;
		}

		if (*EffectScripts.gibbed) {
			CG_FX_GibPlayer(cent);
			break;
		} else {
		  if ( !(es->eFlags & EF_KAMIKAZE) ) {
			if (!SC_Cvar_Get_Int("cl_useq3gibs")) {
				switch(rand() % 4) {
				case 0:
					CG_StartSound( NULL, clientNum, CHAN_BODY, cgs.media.electroGibSound1);
					break;
				case 1:
					CG_StartSound( NULL, clientNum, CHAN_BODY, cgs.media.electroGibSound2);
					break;
				case 2:
					CG_StartSound( NULL, clientNum, CHAN_BODY, cgs.media.electroGibSound3);
					break;
				case 3:
					CG_StartSound( NULL, clientNum, CHAN_BODY, cgs.media.electroGibSound4);
					break;
				default:
					break;
				}

			} else {
				// q3
				CG_StartSound( NULL, clientNum, CHAN_BODY, cgs.media.gibSound );
			}
		  }
		  CG_GibPlayer(cent);
		}

		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		if (es->number >= MAX_CLIENTS) {
			CG_Printf("^3FIXME event %d  %s  clientnum %d\n", event, eventName, es->number);
		}
		trap_S_StopLoopingSound( es->number );
		es->loopSound = 0;  //FIXME is this ok???
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam( cent );
		break;

	case EV_OVERTIME: {
		DEBUGNAME("EV_OVERTIME");
		//Com_Printf("overtime: %d %d %d %d %d\n", es->eventParm, es->number, es->otherEntityNum, es->time, es->time2);
		CG_Printf("overtime: %s seconds added\n", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_overtime"));
		//CG_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.klaxon2);
		//CG_AddBufferedSound(cgs.media.klaxon2);
		CG_CenterPrint(va("Overtime! %s seconds added", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_overtime")), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		CG_StartLocalSound( cgs.media.klaxon2, CHAN_LOCAL_SOUND );
		break;
	}
	case EV_GAMEOVER: {
		DEBUGNAME("EV_GAMEOVER");
		break;
	}
	case EV_THAW_PLAYER: {
		DEBUGNAME("EV_THAW_PLAYER");
		if (es->number >= MAX_CLIENTS) {
			if (cgs.protocol != PROTOCOL_QL) {
				// can't get client number in ql
				CG_Printf("^3FIXME event %d  %s  num %d  clientNum %d\n", event, eventName, es->number, es->clientNum);
				//Com_Printf("158  %s\n", cgs.clientinfo[es->number - 158].name);
				//CG_PrintEntityStatep(es);
			}
		}
		//Com_Printf("^5EV_THAW_PLAYER  num %d\n", es->number);
		if (*EffectScripts.thawed) {
			CG_FX_ThawPlayer(cent);
		} else {
			CG_ThawPlayer(cent);
		}
		break;
	}
	case EV_THAW_TICK: {
		DEBUGNAME("EV_THAW_TICK");
		if (es->number >= MAX_CLIENTS) {
			if (cgs.protocol != PROTOCOL_QL) {
				// can't get client number in ql
				CG_Printf("^3FIXME event %d  %s  num %d  clientNum %d\n", event, eventName, es->number, es->clientNum);
			}
		}
		//CG_StartLocalSound(cgs.media.wearOffSound, CHAN_LOCAL_SOUND);
		//CG_StartSound( NULL, es->number, CHAN_BODY, cgs.media.wearOffSound);
		CG_StartSound( NULL, es->number, CHAN_BODY, cgs.media.thawTick);
		break;
	}
	case EV_HEADSHOT: {
		DEBUGNAME("EV_HEADSHOT");
		// es->otherEntityNum == person who hit head shot
		if (*EffectScripts.headShot) {
			CG_ResetScriptVars();
			//VectorCopy(cent->lerpOrigin, ScriptVars.origin);
			VectorCopy(es->pos.trBase, ScriptVars.origin);
			ScriptVars.clientNum = es->otherEntityNum;
			CG_RunQ3mmeScript(EffectScripts.headShot, NULL);
		} else if (cg_headShots.integer) {
			CG_HeadShotPlum(es->pos.trBase);
		}
		break;
	}
	case EV_POI: {
		poiPic_t *p;

		DEBUGNAME("EV_POI");
		//Com_Printf("^3EV_POI  %d\n", es->eventParm);
		//CG_PrintEntityStatep(es);
		//FIXME more than this?
		// powerups  time in ms
		// generic1  team
		if (cg.numPoiPics >= MAX_POI_PICS) {
			break;
		}
		p = &cg.poiPics[cg.numPoiPics];
		VectorCopy(es->pos.trBase, p->origin);
		p->startTime = cg.time;
		p->length = es->powerups;
		p->team = es->generic1;
		cg.numPoiPics++;
		break;
	}
	case EV_RACE_START: {
		DEBUGNAME("EV_RACE_START");
		if (es->clientNum < 0  ||  es->clientNum >= MAX_CLIENTS) {
			CG_Printf("^3EV_RACE_START invalid client number %d\n", es->clientNum);
			break;
		}

		if (es->clientNum == ourClientNum) {
			CG_StartLocalSound(cgs.media.bellSound, CHAN_LOCAL_SOUND);
		}

		if (es->clientNum == cg.snap->ps.clientNum) {
			cg.predictedPlayerEntity.pe.raceStartTime = es->time;
			cg.predictedPlayerEntity.pe.raceCheckPointNum = 1;
			cg.predictedPlayerEntity.pe.raceCheckPointNextEnt = es->otherEntityNum2;
		} else {
			cg_entities[es->clientNum].pe.raceStartTime = es->time;
			cg_entities[es->clientNum].pe.raceCheckPointNum = 1;
			cg_entities[es->clientNum].pe.raceCheckPointNextEnt = es->otherEntityNum2;
		}
		break;
	}
	case EV_RACE_CHECKPOINT: {
		DEBUGNAME("EV_RACE_CHECKPOINT");
		if (es->clientNum < 0  ||  es->clientNum >= MAX_CLIENTS) {
			CG_Printf("^3EV_RACE_CHECKPOINT invalid client number %d\n", es->clientNum);
			break;
		}

		if (es->clientNum == ourClientNum) {
			int rem;

			CG_StartLocalSound(cgs.media.bellSound, CHAN_LOCAL_SOUND);
			//CG_CenterPrint("CheckPoint", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			//CG_CenterPrint(va("CheckPoint %d / %d\ntest", es->otherEntityNum, cgs.numberOfRaceCheckPoints), SCREEN_HEIGHT * 0.70, BIGCHAR_WIDTH );
			rem = cgs.numberOfRaceCheckPoints - es->otherEntityNum - 1;  // don't include start
			if (rem <= 3) {
				CG_CenterPrint(va("Checkpoint\n%d remaining", rem), SCREEN_HEIGHT * 0.70, BIGCHAR_WIDTH );
			} else {
				//CG_CenterPrint(va("CheckPoint %d / %d", es->otherEntityNum, cgs.numberOfRaceCheckPoints), SCREEN_HEIGHT * 0.70, BIGCHAR_WIDTH );
				CG_CenterPrint("Checkpoint", SCREEN_HEIGHT * 0.70, BIGCHAR_WIDTH );
			}
		}


		if (es->clientNum == cg.snap->ps.clientNum) {
			//cg.predictedPlayerEntity.pe.raceStartTime = es->time;
			cg.predictedPlayerEntity.pe.raceCheckPointNum = es->otherEntityNum + 1;
			cg.predictedPlayerEntity.pe.raceCheckPointNextEnt = es->otherEntityNum2;
		} else {
			//cg_entities[es->clientNum].pe.raceStartTime = es->time;
			cg_entities[es->clientNum].pe.raceCheckPointNum = es->otherEntityNum + 1;
			cg_entities[es->clientNum].pe.raceCheckPointNextEnt = es->otherEntityNum2;
		}
		break;
	}
	case EV_RACE_END: {
		DEBUGNAME("EV_RACE_END");
		//Com_Printf("^3FIXME EV_RACE_END:\n");
		if (es->clientNum < 0  ||  es->clientNum >= MAX_CLIENTS) {
			CG_Printf("^3EV_RACE_END invalid client number %d\n", es->clientNum);
			break;
		}

		if (es->clientNum == ourClientNum) {
			CG_StartLocalSound(cgs.media.klaxon1, CHAN_LOCAL_SOUND);
		}

		if (es->clientNum == cg.snap->ps.clientNum) {
			//cg.predictedPlayerEntity.pe.raceStartTime = es->time;
			cg.predictedPlayerEntity.pe.raceCheckPointNum = 0;
			cg.predictedPlayerEntity.pe.raceCheckPointNextEnt = 0;
			cg.predictedPlayerEntity.pe.raceEndTime = es->time;
		} else {
			//cg_entities[es->clientNum].pe.raceStartTime = es->time;
			cg_entities[es->clientNum].pe.raceCheckPointNum = 0;
			cg_entities[es->clientNum].pe.raceCheckPointNextEnt = 0;
			cg_entities[es->clientNum].pe.raceEndTime = es->time;
		}

		//CG_PrintEntityStatep(es);
		break;
	}

	case EV_AWARD: {
		qhandle_t shader = 0;
		sfxHandle_t sfx;
		clientInfo_t *ci;

		DEBUGNAME("EV_AWARD");

		// clientNum:  clientNum
		// modelindex2:  count
		// generic1:  award type

		if (es->clientNum < 0  ||  es->clientNum >= MAX_CLIENTS) {
			CG_Printf("^3WARNING EV_AWARD invalid client number %d\n", es->clientNum);
			break;
		}

		ci = &cgs.clientinfo[es->clientNum];

		// multi sound:  comboKill, rampage, revenge, midair

		switch (es->generic1) {
			/*FIXME

			// damage  -- not seen?
			// timing  -- not seen?

			*/

		case 0:  // combo kill*
			if (cg_rewardComboKill.integer) {
				shader = cgs.media.medalComboKill;
				sfx = cgs.media.comboKillRewardSound[rand() % NUM_REWARD_VARIATIONS];
			}
			break;
		case 1:  // rampage*
			if (cg_rewardRampage.integer) {
				shader = cgs.media.medalRampage;
				sfx = cgs.media.rampageRewardSound[rand() % NUM_REWARD_VARIATIONS];
			}
			break;
		case 2:  // mid air*
			if (cg_rewardMidAir.integer) {
				shader = cgs.media.medalMidAir;
				sfx = cgs.media.midAirRewardSound[rand() % NUM_REWARD_VARIATIONS];
			}
			break;
		case 3:  // revenge*
			if (cg_rewardRevenge.integer) {
				shader = cgs.media.medalRevenge;
				sfx = cgs.media.revengeRewardSound[rand() % NUM_REWARD_VARIATIONS];
			}
			break;
		case 4:  // perforated
			if (cg_rewardPerforated.integer) {
				shader = cgs.media.medalPerforated;
				sfx = cgs.media.perforatedRewardSound;
			}
			break;
		case 5:  // headshot
			if (cg_rewardHeadshot.integer) {
				shader = cgs.media.medalHeadshot;
				sfx = cgs.media.headshotRewardSound;
			}
			break;
		case 6:  // accuracy
			if (cg_rewardAccuracy.integer) {
				shader = cgs.media.medalAccuracy;
				sfx = cgs.media.accuracyRewardSound;
			}
			break;
		case 7:  // quad god
			if (cg_rewardQuadGod.integer) {
				shader = cgs.media.medalQuadGod;
				sfx = cgs.media.quadGodRewardSound;
			}
			break;
		case 8:  // first frag
			if (cg_rewardFirstFrag.integer) {
				shader = cgs.media.medalFirstFrag;
				sfx = cgs.media.firstFragRewardSound;
			}
			break;
		case 9:  // perfect
			if (cg_rewardPerfect.integer) {
				shader = cgs.media.medalPerfect;
				sfx = cgs.media.perfectRewardSound;
			}
			break;

		default:
			Com_Printf("^3FIXME EV_AWARD\n");
			CG_PrintEntityStatep(&cent->currentState);
			CG_Printf("^3unknown EV_AWARD index %d\n", es->generic1);
			shader = 0;
			break;
		}

		if (es->clientNum == ourClientNum) {
			if (shader) {
				CG_PushReward(sfx, shader, es->modelindex2);
			}
		} else {
			// not main client
			// debugging
			//CG_Printf("^2EV_AWARD for non ps (%d) client %d\n", cg.snap->ps.clientNum, es->clientNum);
			//CG_PrintEntityStatep(&cent->currentState);
		}

		if (shader) {
			ci->clientRewards.startTime = cg.time;
			ci->clientRewards.shader = shader;
			ci->clientRewards.sfx = sfx;
		}

		break;
	}

	case EV_INFECTED: {
		DEBUGNAME("EV_INFECTED");

		//FIXME not sure if any info is transmitted
		CG_StartLocalSound(cgs.media.infectedSound, CHAN_LOCAL_SOUND);
		cg.infectedSnapshotTime = cg.snap->serverTime;
		break;
	}

	case EV_NEW_HIGH_SCORE: {
		DEBUGNAME("EV_NEW_HIGH_SCORE");
		// new race high score
		// 2018-07-28 is this ever used?
	}

	default: {
		static int lastUnknownEvent = 0;

		DEBUGNAME("UNKNOWN");
		//CG_Error( "Unknown event: %i", event );
		if (event != lastUnknownEvent) {
			CG_Printf("Unknown event: %i\n", event);
			if (cg_debugEvents.integer > 1) {
				CG_PrintEntityStatep(&cent->currentState);
			}
			//lastUnknownEvent = event;
		}
		break;
	}
	}

}
#undef DEBUGNAME


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			//Com_Printf("%d %d already fired\n", cent - cg_entities, cent->currentState.eType - ET_EVENTS);
			return;	// already fired
		}
		// if this is a player event set the entity number of the client entity number
		if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
			cent->currentState.number = cent->currentState.otherEntityNum;  //FIXME damn
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	//FIXME wolfcam wtf?
	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	//Com_Printf("event ent %d\n", cent->currentState.number);
	CG_EntityEvent( cent, cent->lerpOrigin );
}

void CG_AddClientSidePredictableEvent (int event, int eventParam)
{
	if (cg.clientSideEventSequence >= MAX_PREDICTED_EVENTS) {
		CG_Printf("^3cg.clientSideEventSequence >= MAX_PREDICTED_EVENTS\n");
		return;
	}

	cg.clientSidePredictableEvents[cg.clientSideEventSequence] = event;
	cg.clientSidePredictableEventParams[cg.clientSideEventSequence] = eventParam;
	cg.clientSideEventSequence++;
}

#include "cg_local.h"

#include "cg_main.h"
#include "cg_syscalls.h"
#include "wolfcam_consolecmds.h"

#include "wolfcam_local.h"

// from "enemy territory"
static int BG_cleanName( const char *pszIn, char *pszOut, unsigned int dwMaxLength, qboolean fCRLF )
{
    const char *pInCopy = pszIn;
    const char *pszOutStart = pszOut;

    while( *pInCopy && ( pszOut - pszOutStart < dwMaxLength - 1 ) ) {
        if( *pInCopy == '^' )
            pInCopy += ((pInCopy[1] == 0) ? 1 : 2);
        else if( (*pInCopy < 32 && (!fCRLF || *pInCopy != '\n')) || (*pInCopy > 126))
            pInCopy++;
        else
            *pszOut++ = *pInCopy++;
    }

    *pszOut = 0;
    return( pszOut - pszOutStart );
}


void Wolfcam_List_Player_Models_f (void)
{
	int i;
	const clientInfo_t *ci;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		ci = &cgs.clientinfoOrig[i];
		if (!ci->infoValid)
			continue;
		Com_Printf("%d %s %s\n", i, ci->name, ci->modelName);
	}
}

/*
===============
Wolfcam_Players_f

print some info about players
===============
*/
void Wolfcam_Players_f (void)
{
    int i;
    const clientInfo_t *ci;
    char color;
	const char *clanTag;
	
    for (i = 0;  i < MAX_CLIENTS;  i++) {
        ci = &cgs.clientinfo[i];
        if (!ci->infoValid)
            continue;
        switch (ci->team) {
        case TEAM_RED:
            color = '1';
            break;
        case TEAM_BLUE:
            color = '4';
            break;
        case TEAM_SPECTATOR:
            color = '3';
            break;
        case TEAM_FREE:
            color = '7';
            break;
        default:
            color = '6';
        }
		clanTag = ci->clanTag;
		if (*clanTag) {
			Com_Printf ("^%c X^7  %02d: %s ^7%s", color, i, clanTag, ci->name);
		} else {
			Com_Printf ("^%c X^7  %02d: %s", color, i, ci->name);
		}

#if 0
		//FIXME testing utf8
		{
			const char *s = ci->name;
			Com_Printf("\nutf8 values: ");
			while (*s) {
				Com_Printf("[%d '%c'] ", s[0] & 255, s[0] & 255);
				s++;
			}
			Com_Printf("\n");
		}
#endif

		if (cgs.realProtocol >= 91) {
			if (ci->steamId[0] != '\0') {
				Com_Printf("     ^5steamId: %s^7", ci->steamId);
			}
		}

		if (i == cg.clientNum) {
			Com_Printf("    ^3[demo taker]");
		} else if (i == cg.snap->ps.clientNum) {
			Com_Printf("    ^6[demo taker following this pov]");
		}
		Com_Printf("\n");
    }
}

/*
===============
Wolfcam_Playersw_f

print some info about players
===============
*/
void Wolfcam_Playersw_f (void)
{
    int i;
    const clientInfo_t *ci;
    char color;
    char name[MAX_QPATH];

    for (i = 0;  i < MAX_CLIENTS;  i++) {
        ci = &cgs.clientinfo[i];
        if (!ci->infoValid)
            continue;
        switch (ci->team) {
        case TEAM_RED:
            color = '1';
            break;
        case TEAM_BLUE:
            color = '4';
            break;
        case TEAM_SPECTATOR:
            color = '3';
            break;
        case TEAM_FREE:
            color = '7';
            break;
        default:
            color = '6';
        }
        BG_cleanName (ci->name, name, sizeof(name), qfalse);
        Com_Printf ("^%c X^7  %02d: %s\n", color, i, name);
    }
}

void Wolfcam_Follow_f (void)
{
    int clientNum;
    char name[MAX_QPATH];

    if (trap_Argc() < 2) {
        Com_Printf ("currently following [%d]  ", wcg.selectedClientNum);
        if (wcg.followMode == WOLFCAM_FOLLOW_DEMO_TAKER) {
            Com_Printf ("^3<demo pov>\n");
		} else {
            BG_cleanName (cgs.clientinfo[wcg.selectedClientNum].name, name, sizeof(name), qfalse);
            Com_Printf ("%s\n", name);
			if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
				Com_Printf("mode: follow killer\n");
			} else if (wcg.followMode == WOLFCAM_FOLLOW_VICTIM) {
				Com_Printf("mode: follow victim\n");
			}
			//Com_Printf("\n");
        }
        Com_Printf ("^7use 'follow -1' to return to demo taker's pov\n");
        return;
    }

	if (!strcmp("victim", CG_Argv(1))) {
		wcg.followMode = WOLFCAM_FOLLOW_VICTIM;
		goto done;
	}
	if (!strcmp("killer", CG_Argv(1))) {
		wcg.followMode = WOLFCAM_FOLLOW_KILLER;
		goto done;
	}

    clientNum = atoi(CG_Argv(1));
    if (clientNum > MAX_CLIENTS  ||  clientNum < -1) {
        Com_Printf ("bad client number\n");
        return;
    }
    if (clientNum == -1) {
        wcg.selectedClientNum = -1;
        wcg.clientNum = -1;
        //cg.renderingThirdPerson = 0;
        wolfcam_following = qfalse;
		wcg.followMode = WOLFCAM_FOLLOW_DEMO_TAKER;
        //trap_Cvar_Set ("cg_thirdPerson", "0");  //FIXME wolfcam
		cg.freecam = qfalse;
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
			trap_SendConsoleCommand("exec spectator.cfg\n");
		} else if (cg.snap  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
			trap_SendConsoleCommand("exec spectator.cfg\n");
		} else {
			trap_SendConsoleCommand("exec ingame.cfg\n");
		}
        return;
    }
    if (!cgs.clientinfo[clientNum].infoValid) {
        Com_Printf ("invalid client\n");
        return;
    }

    wcg.selectedClientNum = clientNum;
    wcg.clientNum = clientNum;
	wcg.followMode = WOLFCAM_FOLLOW_SELECTED_PLAYER;

 done:
    wolfcam_following = qtrue;
	cg.freecam = qfalse;
	trap_SendConsoleCommand("exec follow.cfg\n");
}


void Wolfcam_Server_Info_f (void)
{
    const char *info;

    info = CG_ConfigString( CS_SERVERINFO );

    Com_Printf ("^4server config string: ^2%s\n", info);
}

// server totals
static int totalKills[WP_MAX_NUM_WEAPONS_ALL_PROTOCOLS];
static float totalWarp;
//static int totalNoWarp;
static int totalWarpSeverity;
static int totalClients;
static float avgWarp;
static int avgSeverity;

static void print_ind_weap_stats (const wweaponStats_t *w)
{
    if (w->kills < 1000)
        Com_Printf (" ");
    if (w->kills < 100)
        Com_Printf (" ");
    if (w->kills < 10)
        Com_Printf (" ");
    Com_Printf ("%d", w->kills);

    if (w->deaths < 1000)
        Com_Printf (" ");
    if (w->deaths < 100)
        Com_Printf (" ");
    if (w->deaths < 10)
        Com_Printf (" ");
    Com_Printf ("%d", w->deaths);

    Com_Printf (":       ");

    if (w->hits < 10000)
        Com_Printf (" ");
    if (w->hits < 1000)
        Com_Printf (" ");
    if (w->hits < 100)
        Com_Printf (" ");
    if (w->hits < 10)
        Com_Printf (" ");
    Com_Printf ("%d / ", w->hits);

    if (w->atts < 10000)
        Com_Printf (" ");
    if (w->atts < 1000)
        Com_Printf (" ");
    if (w->atts < 100)
        Com_Printf (" ");
    if (w->atts < 10)
        Com_Printf (" ");
    Com_Printf ("%d", w->atts);

    Com_Printf ("    ");

    if (w->atts  &&  w->hits)
        Com_Printf ("%0.2f", (float)(w->hits) / (float)(w->atts));
    else {
        //Com_Printf ("0.00");
    }

    Com_Printf ("\n");
}

static void wolfcam_reset_weapon_stats (int clientNum, qboolean oldclient)
{
	wclient_t *wc;
	wweaponStats_t *ws;
	int i;

	if (oldclient) {
		Com_Printf("FIXME reset weapon stats old client\n");
		return;
	}

	wc = &wclients[clientNum];
	wc->kills = 0;
	wc->deaths = 0;
	wc->suicides = 0;
	ws = wc->wstats;

	for (i = WP_NONE;  i < WP_NUM_WEAPONS;  i++) {
		memset(&ws[i], 0, sizeof(wweaponStats_t));
	}
}

static void wolfcam_calc_averages (void)
{
	int i, j;
	const woldclient_t *woc;
	const wclient_t *wc;
	const wweaponStats_t *ws;

	memset(totalKills, 0, sizeof(totalKills));
	totalWarp = 0;
	//totalNoWarp = 0;
	totalWarpSeverity = 0;
	totalClients = 0;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}
		wc = &wclients[i];
		if (!wc->validCount) {
			continue;
		}
		totalClients++;
		if (wc->nowarp > 0) {
			totalWarp += (float)wc->warp / (float)wc->nowarp;
		}
		//totalNoWarp += wc->nowarp;
		if (wc->warp) {
			totalWarpSeverity += wc->warpaccum / wc->warp;
		}
		for (j = 0;  j < WP_NUM_WEAPONS;  j++) {
			ws = &wc->wstats[j];
			totalKills[j] += ws->kills;
		}
	}

	for (i = 0;  i < wnumOldClients;  i++) {
		//wolfcam_print_weapon_stats (i, qtrue);

		woc = &woldclients[i];
		if (!woc->validCount) {
			continue;
		}
		totalClients++;
		if (woc->nowarp > 0) {
			totalWarp += (float)woc->warp / (float)woc->nowarp;
		}
		//totalNoWarp += woc->nowarp;
		if (woc->warp) {
			totalWarpSeverity += woc->warpaccum / woc->warp;
		}
		for (j = 0;  j < WP_NUM_WEAPONS;  j++) {
			ws = &woc->wstats[j];
			totalKills[j] += ws->kills;
		}
	}

	if (totalClients > 0) {
		avgWarp = totalWarp / (float)totalClients;
		avgSeverity = totalWarpSeverity / (float)totalClients;
	} else {
		avgWarp = 0;
		avgSeverity = 0;
	}
}

static void wolfcam_print_weapon_stats (int clientNum, qboolean oldclient)
{
    const char *name;
    int kills;
    int deaths;
    int suicides;
    int warp;
    int nowarp;
    int warpaccum;
	int noCommandCount;
	int noMoveCount;
	int validCount;
	int invalidCount;
	int serverPingAccum;
	int serverPingSamples;
	//int snapshotPingAccum;
	int snapshotPingSamples;
	const clientInfo_t *ci;
	//float fwarp;
	//int sev;

    const wweaponStats_t *w;
    const wweaponStats_t *ws;  //[WP_NUM_WEAPONS];

    if (oldclient) {
        const woldclient_t *wc = &woldclients[clientNum];

		ci = &woldclients[clientNum].clientinfo;
        name = woldclients[clientNum].clientinfo.name;
        kills = woldclients[clientNum].kills;
        deaths = woldclients[clientNum].deaths;
        suicides = woldclients[clientNum].suicides;
        warp = wc->warp;
        nowarp = wc->nowarp;
        warpaccum = wc->warpaccum;
		validCount = wc->validCount;
		invalidCount = wc->invalidCount;
		serverPingAccum = wc->serverPingAccum;
		serverPingSamples = wc->serverPingSamples;
		//snapshotPingAccum = wc->snapshotPingAccum;
		snapshotPingSamples = wc->snapshotPingSamples;
		noCommandCount = wc->noCommandCount;
		noMoveCount = wc->noMoveCount;

        ws = woldclients[clientNum].wstats;
    } else {
        const wclient_t *wc = &wclients[clientNum];

		ci = &cgs.clientinfo[clientNum];
        name = cgs.clientinfo[clientNum].name;
        kills = wclients[clientNum].kills;
        deaths = wclients[clientNum].deaths;
        suicides = wclients[clientNum].suicides;
        warp = wc->warp;
        nowarp = wc->nowarp;
        warpaccum = wc->warpaccum;
		validCount = wc->validCount;
		invalidCount = wc->invalidCount;
		serverPingAccum = wc->serverPingAccum;
		serverPingSamples = wc->serverPingSamples;
		//snapshotPingAccum = wc->snapshotPingAccum;
		snapshotPingSamples = wc->snapshotPingSamples;
		noCommandCount = wc->noCommandCount;
		noMoveCount = wc->noMoveCount;
        ws = wclients[clientNum].wstats;
    }

	Com_Printf("^1-------------------------------------------------------\n");
    Com_Printf ("^7Stats for %s^3:\n", name);

    Com_Printf("    ^7kills: %d  ^7deaths: %d  ^7suicides: %d\n", kills, deaths, suicides);
	Com_Printf("    in %d frames out of %d   ^3%0.2f\n", validCount, invalidCount + validCount, (invalidCount + validCount) > 0 ? (float)validCount / (float)(invalidCount + validCount) : -1);
	if (clientNum == cg.clientNum  ||  (clientNum == cg.snap->ps.clientNum  &&  cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)) {
		Com_Printf("    noCommand: (%d (^3%d^7) / %d)  warp: %d nowarp: %d ", noMoveCount, noCommandCount, snapshotPingSamples, warp, nowarp);
	} else {
		Com_Printf("    noMoveCount: (%d / %d)      warp: %d nowarp: %d ", noMoveCount, snapshotPingSamples, warp, nowarp);
	}

    if ((nowarp) > 0) {
		float f;

		f = (float)warp / (float)nowarp;
		if (f > avgWarp) {
			Com_Printf(" %.3f ^1+%.3f", f, f - avgWarp);
		} else if (f < avgWarp) {
			Com_Printf(" %.3f ^2-%.3f", f, avgWarp - f);
		} else {
			Com_Printf (" %.3f", (float)warp / (float)(nowarp));
		}
    } else {
        Com_Printf ("         ");
	}

    if (warp > 0) {
		int sev;

		sev = warpaccum / warp;
		if (sev > avgSeverity) {
			Com_Printf("  ^7%d ^1+%d\n", sev, sev - avgSeverity);
		} else if (sev < avgSeverity) {
			Com_Printf("  ^7%d ^2-%d\n", sev, avgSeverity - sev);
		} else {
			Com_Printf ("  ^7%d\n", warpaccum / warp);
		}
    } else {
        Com_Printf ("\n");
	}

    Com_Printf("    serverPing:    ^3%d    ^7%d\n", serverPingSamples ? serverPingAccum / serverPingSamples : -1, serverPingSamples);
	// not in ql
	//Com_Printf("    snapshotPing:  ^3%d    ^7%d\n", snapshotPingSamples ? snapshotPingAccum / snapshotPingSamples / 2 : -1, snapshotPingSamples);

	if (ci->knowSkillRating) {
		Com_Printf("    skillRating: ^5%d\n", ci->skillRating);
	}

    w = &ws[WP_GAUNTLET];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2gauntlet     ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_LIGHTNING];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2lightning gun");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_MACHINEGUN];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2machinegun   ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_HEAVY_MACHINEGUN];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2hmg          ");
        print_ind_weap_stats (w);
    }

    {
        wweaponStats_t wtmp;

    w = &ws[WP_SHOTGUN];
    memcpy (&wtmp, w, sizeof (wweaponStats_t));
    // shotgun kludge
    //wtmp.hits /= DEFAULT_SHOTGUN_COUNT;
    if (wtmp.kills  ||  wtmp.deaths  ||  wtmp.atts) {
        Com_Printf ("  ^2shotgun      ");
        print_ind_weap_stats (&wtmp);
    }
    }

    w = &ws[WP_ROCKET_LAUNCHER];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2rocket       ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_GRENADE_LAUNCHER];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2grenade      ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_PLASMAGUN];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2plasma gun   ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_RAILGUN];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2rail gun     ");
        print_ind_weap_stats (w);
    }

    w = &ws[WP_BFG];
    if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2bfg          ");
        print_ind_weap_stats (w);
    }

	w = &ws[WP_NAILGUN];
	if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2nail gun     ");
		print_ind_weap_stats(w);
	}

	w = &ws[WP_PROX_LAUNCHER];
	if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2prox launcher ");
		print_ind_weap_stats(w);
	}

	w = &ws[WP_CHAINGUN];
	if (w->kills  ||  w->deaths  ||  w->atts) {
        Com_Printf ("  ^2chaingun      ");
		print_ind_weap_stats(w);
	}
}

void Wolfcam_Weapon_Stats_f (void)
{
    int clientNum;

    if (trap_Argc() < 2) {
        if (wolfcam_following)
            clientNum = wcg.clientNum;
        else
            clientNum = cg.clientNum;
    } else {
        clientNum = atoi(CG_Argv(1));
    }

    if (clientNum == -1)
        clientNum = cg.clientNum;

    if (clientNum < 0  ||  clientNum > MAX_CLIENTS) {
        Com_Printf ("invalid client number\n");
        return;
    }

	wolfcam_calc_averages();
    wolfcam_print_weapon_stats(clientNum, qfalse);
}

void Wolfcam_Weapon_Statsall_f (void)
{
    int i;  //, j;
	//woldclient_t *woc;
	//wclient_t *wc;
	//wweaponStats_t *ws;
	//float f;

#if 0
	memset(totalKills, 0, sizeof(totalKills));
	totalWarp = 0;
	//totalNoWarp = 0;
	totalWarpSeverity = 0;
	totalClients = 0;
#endif

	wolfcam_calc_averages();

#if 0
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}
		totalClients++;
		wc = &wclients[i];
		if (wc->nowarp > 0) {
			totalWarp += (float)wc->warp / (float)wc->nowarp;
		}
		//totalNoWarp += wc->nowarp;
		if (wc->warp) {
			totalWarpSeverity += wc->warpaccum / wc->warp;
		}
		for (j = 0;  j < WP_NUM_WEAPONS;  j++) {
			ws = &wc->wstats[j];
			totalKills[j] += ws->kills;
		}
	}

	for (i = 0;  i < wnumOldClients;  i++) {
		//wolfcam_print_weapon_stats (i, qtrue);
		totalClients++;
		woc = &woldclients[i];
		if (woc->nowarp > 0) {
			totalWarp += (float)woc->warp / (float)woc->nowarp;
		}
		//totalNoWarp += woc->nowarp;
		if (woc->warp) {
			totalWarpSeverity += woc->warpaccum / woc->warp;
		}
		for (j = 0;  j < WP_NUM_WEAPONS;  j++) {
			ws = &woc->wstats[j];
			totalKills[j] += ws->kills;
		}
	}

	if (totalClients > 0) {
		avgWarp = totalWarp / (float)totalClients;
		avgSeverity = totalWarpSeverity / totalClients;
	} else {
		avgWarp = 0;
		avgSeverity = 0;
	}
#endif

	//////////////////////////
    for (i = 0;  i < MAX_CLIENTS;  i++) {
        if (cgs.clientinfo[i].infoValid) {
            wolfcam_print_weapon_stats (i, qfalse);
            Com_Printf ("\n");
        }
    }

    if (wnumOldClients) {
        Com_Printf ("disconnected clients:\n\n");
        for (i = 0;  i < wnumOldClients;  i++) {
            wolfcam_print_weapon_stats (i, qtrue);
            Com_Printf ("\n");
        }
    }

	Com_Printf("^1-------------------------------------------------------\n");
	Com_Printf("Server totals^3:\n");
#if 0
	if (totalNoWarp > 0) {
		f = (float)totalWarp / (float)totalNoWarp;
	} else {
		f = 0;
	}
	Com_Printf("   warp: %d nowarp: %d  %f  %d\n", totalWarp, totalNoWarp, avgWarp, avgSeverity);
#endif
	Com_Printf("   average warp: %f  average severity: %d\n", avgWarp, avgSeverity);
	//Com_Printf("   kills:\n");
	for (i = WP_GAUNTLET;  i < WP_NUM_WEAPONS;  i++) {
		Com_Printf("  ^2%d  %s\n", totalKills[i], weapNamesCasual[i]);
	}

	memset(totalKills, 0, sizeof(totalKills));
	totalWarp = 0;
	//totalNoWarp = 0;
	totalWarpSeverity = 0;
	totalClients = 0;
	avgWarp = -1;
	avgSeverity = -1;
}

void Wolfcam_Reset_Weapon_Stats_f (void)
{
    int i;

    for (i = 0;  i < MAX_CLIENTS;  i++) {
        if (cgs.clientinfo[i].infoValid) {
            wolfcam_reset_weapon_stats (i, qfalse);
            //Com_Printf ("\n");
        }
    }

	memset(woldclients, 0, sizeof(woldclients));
	wnumOldClients = 0;

#if 0
    if (wnumOldClients) {
        //Com_Printf ("disconnected clients:\n\n");
        for (i = 0;  i < wnumOldClients;  i++) {
            wolfcam_reset_weapon_stats (i, qtrue);
            Com_Printf ("\n");
        }
    }
#endif
}

// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

#include "cg_draw.h"
#include "cg_drawtools.h"
#include "cg_event.h"
#include "cg_main.h"
#include "cg_newdraw.h"  // QLWideScreen
#include "cg_players.h"
#include "cg_scoreboard.h"
#include "cg_syscalls.h"
#include "sc.h"

#include "wolfcam_local.h"

#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X			(SCOREBOARD_X+64)

#define SB_SCORELINE_X		112

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH) // width 6
#define SB_SCORE_X			(SB_SCORELINE_X + BIGCHAR_WIDTH) // width 6
#define SB_RATING_X			(SB_SCORELINE_X + 6 * BIGCHAR_WIDTH) // width 6
#define SB_PING_X			(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8) // width 5
#define SB_TIME_X			(SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8) // width 5
#define SB_NAME_X			(SB_SCORELINE_X + 22 * BIGCHAR_WIDTH) // width 15

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed


static void CG_DrawClientScore( int y, const score_t *score, const float *color, float fade, qboolean largeFormat ) {
	char	string[1024];
	vec3_t	headAngles;
	const clientInfo_t *ci;
	int iconx, headx;

#if 0
	Com_Printf("----  CG_DrawClientScore()  ------\n");
		Com_Printf("  client: %d\n", score->client);
		Com_Printf("  score: %d\n", score->score);
		Com_Printf("  ping:  %d\n", score->ping);
		Com_Printf("  time: %d\n", score->time);
		Com_Printf("  scoreFlags: %d\n", score->scoreFlags);
		Com_Printf("  powerUps:  %d\n", score->powerUps);
		Com_Printf("  accuracy:  %d\n", score->accuracy);
		//		Com_Printf("  ...\n");
		Com_Printf("  impressiveCount:  %d\n", score->impressiveCount);
		Com_Printf("  excellentCount:  %d\n", score->excellentCount);
		Com_Printf("  gauntletCount:  %d\n", score->gauntletCount);
		Com_Printf("  defendCount:  %d\n", score->defendCount);
		Com_Printf("  assistCount:  %d\n", score->assistCount);
		Com_Printf("  perfect:  %d\n", score->perfect);
		Com_Printf("  team:  %d\n", score->team);
#endif

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_RED, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_RED, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_BLUE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_BLUE, qfalse );
		}
	} else {
		if ( ci->botSkill > 0 && ci->botSkill <= 5 ) {
			if ( cg_drawIcons.integer ) {
				if( largeFormat ) {
					CG_DrawPic( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
				else {
					CG_DrawPic( iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
			}
		} else if ( ci->handicap < 100 ) {
			Com_sprintf( string, sizeof( string ), "%i", ci->handicap );
			if ( CG_IsDuelGame(cgs.gametype) )
				CG_DrawSmallStringColor( iconx, y - SMALLCHAR_HEIGHT/2, string, color );
			else
				CG_DrawSmallStringColor( iconx, y, string, color );
		}

		// draw the wins / losses
		if ( CG_IsDuelGame(cgs.gametype) ) {
			Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
			if( ci->handicap < 100 && !ci->botSkill ) {
				CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, string, color );
			}
			else {
				CG_DrawSmallStringColor( iconx, y, string, color );
			}
		}
	}

	// draw the face
	VectorClear( headAngles );
	headAngles[YAW] = 180;
	if( largeFormat ) {
		CG_DrawHead( headx, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE, 
					 score->client, headAngles, qtrue, qfalse, qtrue );
	}
	else {
		CG_DrawHead( headx, y, 16, 16, score->client, headAngles, qtrue, qfalse, qtrue );
	}

#ifdef MISSIONPACK
	// draw the team task
	if ( ci->teamTask != TEAMTASK_NONE ) {
		if ( ci->teamTask == TEAMTASK_OFFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.assaultShader );
		}
		else if ( ci->teamTask == TEAMTASK_DEFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.defendShader );
		}
	}
#endif
	// draw the score line
	if ( score->ping == -1 ) {
		Com_sprintf(string, sizeof(string),
			" connecting    %s", ci->name);
	//} else if ( ci->team == TEAM_SPECTATOR   &&  cg.numScores > 0) {
	} else if ( score->team == TEAM_SPECTATOR   &&  cg.numScores > 0) {
		Com_sprintf(string, sizeof(string),
			"^3%5i %4i %4i ^7%s", score->score, score->ping, score->time, ci->name);
	} else if (cg.numScores > 0) {
#if 1
		Com_sprintf(string, sizeof(string),
			"%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);
#endif
#if 0
		Com_sprintf(string, sizeof(string),
			"%5i %4i %4i %i %i %i %i %s %s", score->score, score->ping, score->time, score->alive, score->frags, score->deaths, score->accuracy, weapNames[score->bestWeapon], ci->name);
#endif
#if 0
		Com_sprintf(string, sizeof(string),
					"%5i %4i %4i %d %s %s", score->score, score->ping, score->time, score->accuracy, weapNames[score->bestWeapon], ci->name);
#endif
	} else if (cg.demoPlayback) {
		Com_sprintf(string, sizeof(string),
					"               %s", ci->name);
	}

	//FIXME wolfcam
	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
			 || (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) ) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect( SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2), y, 
			640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT+1, hcolor );
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
		CG_DrawBigStringColor( iconx, y, "READY", color );
	}
}

static void CG_DrawClientScoreCpmaMstatsa (int y, int player, float fade)
{
	char	string[1024];
	const duelScore_t *ds;


	if (player != 0  &&  player != 1) {
		Com_Printf("CG_DrawClientScoreCpmaMstatsa():  invalid player %d\n", player);
		return;
	}

	ds = &cg.duelScores[player];

	// draw the score line
	if (cg.duelForfeit  &&  player == 1) {
		Com_sprintf(string, sizeof(string),
					"    - %4i %4i %s", ds->ping, ds->time, ds->ci.name);

	} else {
		Com_sprintf(string, sizeof(string),
					"%5i %4i %4i %s", ds->score, ds->ping, ds->time, ds->ci.name);
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );
}

static void CG_DrawClientScoreQlForfeit (int y, int player, float fade)
{
	char	string[1024];
	const duelScore_t *ds;
	int forfeitPlayer;

	if (player != 0  &&  player != 1) {
		Com_Printf("CG_DrawClientScoreCpmaMstatsa():  invalid player %d\n", player);
		return;
	}

	// indexed at 1
	forfeitPlayer = cg.duelPlayerForfeit - 1;

	ds = &cg.duelScores[player];


	// draw the score line
	if (player == forfeitPlayer) {
		Com_sprintf(string, sizeof(string),
					"    - %4i %4i %s", ds->ping, ds->time, ds->ci.name);

	} else {
		Com_sprintf(string, sizeof(string),
					"%5i %4i %4i %s", ds->score, ds->ping, ds->time, ds->ci.name);
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight ) {
	int		i;
	const score_t	*score;
	float	color[4];
	int		count;
	const clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;

	// cpma mstatsa doesn't transmit client numbers
	if (cgs.cpma  &&  CG_CheckCpmaVersion(1, 50, "")  &&  CG_IsDuelGame(cgs.gametype)  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
		if (team == TEAM_FREE) {
			CG_DrawClientScoreCpmaMstatsa(y + lineHeight * count, 0, fade);
			count++;
			CG_DrawClientScoreCpmaMstatsa(y + lineHeight * count, 1, fade);
			count++;

			return count;
		} else {  // specs
			for (i = 0;  i < MAX_CLIENTS;  i++) {
				score_t sc;

				ci = &cgs.clientinfo[i];
				if (!ci->infoValid)
					continue;

				if (team != ci->team)
					continue;

				memset(&sc, 0, sizeof(sc));
				sc.client = i;
				sc.team = ci->team;

				CG_DrawClientScore( y + lineHeight * count, &sc, color, fade, lineHeight == SB_NORMAL_HEIGHT );
				count++;
			}

			return count;
		}
	}

	if (cgs.protocolClass == PROTOCOL_QL  &&  CG_IsDuelGame(cgs.gametype)  &&  cg.snap->ps.pm_type == PM_INTERMISSION  &&  cg.duelForfeit) {
		if (team == TEAM_FREE) {
			CG_DrawClientScoreQlForfeit(y + lineHeight * count, 0, fade);
			count++;
			CG_DrawClientScoreQlForfeit(y + lineHeight * count, 1, fade);
			count++;

			return count;
		} else {  // specs
			for (i = 0;  i < MAX_CLIENTS;  i++) {
				score_t sc;

				ci = &cgs.clientinfo[i];
				if (!ci->infoValid)
					continue;

				if (team != ci->team)
					continue;

				memset(&sc, 0, sizeof(sc));
				sc.client = i;
				sc.team = ci->team;

				CG_DrawClientScore( y + lineHeight * count, &sc, color, fade, lineHeight == SB_NORMAL_HEIGHT );
				count++;
			}

			return count;
		}
	}

	if (cg.numScores == 0) {
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			score_t sc;

			ci = &cgs.clientinfo[i];
			if (!ci->infoValid)
				continue;

			if (team != ci->team)
				continue;

			memset(&sc, 0, sizeof(sc));
			sc.client = i;
			sc.team = ci->team;

			CG_DrawClientScore( y + lineHeight * count, &sc, color, fade, lineHeight == SB_NORMAL_HEIGHT );
			count++;
		}
		return count;
	}

	count = 0;

	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		CG_DrawClientScore( y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );

		count++;
	}

	return count;
}

/*
=================
CG_DrawOldScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	const float	*fadeColor;
	const char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	QLWideScreen = WIDESCREEN_CENTER;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
		w = CG_DrawStrlen( s, &cgs.media.bigchar );
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
	}

	// current rank
	if (!CG_IsTeamGame(cgs.gametype)) {
		if (!wolfcam_following  ||  (wolfcam_following  &&  wcg.clientNum == cg.snap->ps.clientNum)) {
			if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
				s = va("%s place with %i",
					   CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
					   cg.snap->ps.persistant[PERS_SCORE] );
				w = CG_DrawStrlen( s, &cgs.media.bigchar );
				x = ( SCREEN_WIDTH - w ) / 2;
				y = 60;
				CG_DrawBigString( x, y, s, fade );
			}
		} else {  // wolfcam_following
			if (cgs.clientinfo[wcg.clientNum].team != TEAM_SPECTATOR) {
				if (CG_IsCpmaMvd()) {
					int rank;
					int i;

					rank = 1;
					for (i = 0;  i < MAX_CLIENTS;  i++) {
						if (!cgs.clientinfo[i].infoValid) {
							continue;
						}
						if (cgs.clientinfo[i].team == TEAM_SPECTATOR) {
							continue;
						}
						if (cgs.clientinfo[i].score > cgs.clientinfo[wcg.clientNum].score) {
							rank++;
						}
					}

					s = va("%s ^7place with %i", CG_PlaceString(rank), cgs.clientinfo[wcg.clientNum].score);

					w = CG_DrawStrlen( s, &cgs.media.bigchar );
					x = ( SCREEN_WIDTH - w ) / 2;
					y = 60;
					CG_DrawBigString( x, y, s, fade );
				} else {  // not cpma mvd
					// following someone who is ingame but not the main demo view

					if (CG_IsDuelGame(cgs.gametype)) {
						// we are following the other dueler
						if (cgs.scores1 == cgs.scores2) {
							s = va("%s ^7place with %i", CG_PlaceString(1), cgs.scores1);
						} else {
							if (cg.snap->ps.persistant[PERS_RANK] == 0) {
								// we are second
								s = va("%s ^7place with %i", CG_PlaceString(2), cgs.scores2);
							} else {
								// we are first
								s = va("%s ^7place with %i", CG_PlaceString(1), cgs.scores1);
							}
						}
						w = CG_DrawStrlen( s, &cgs.media.bigchar );
						x = ( SCREEN_WIDTH - w ) / 2;
						y = 60;
						CG_DrawBigString( x, y, s, fade );
					} else {
						// we don't have enough information
						// pass, don't draw ranking
					}
				}
			}
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
		}

		w = CG_DrawStrlen( s, &cgs.media.bigchar );
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 60;
		CG_DrawBigString( x, y, s, fade );
	}

	// scoreboard
	y = SB_HEADER;

	CG_DrawPic( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore );
	CG_DrawPic( SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing );
	CG_DrawPic( SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime );
	CG_DrawPic( SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName );

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if (1) {  //( cg.numScores > SB_MAXCLIENTS_NORMAL ) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	localClient = qfalse;

	if ( CG_IsTeamGame(cgs.gametype) ) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float		x;
	vec4_t		color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - CG_DrawStrlen( string, &cgs.media.giantchar ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawTourneyScoreboard( void ) {
	const char		*s;
	vec4_t			color;
	int				min, tens, ones;
	const clientInfo_t	*ci;
	int				y;
	int				i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if ( CG_IsTeamGame(cgs.gametype) ) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
		s = va("%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
		
		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
		s = va("%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
	} else {
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
			s = va("%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0, &cgs.media.giantchar );
			y += 64;
		}
	}
}

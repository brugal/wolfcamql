// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definitely
// be a valid snapshot this frame

#include "cg_local.h"

#include "cg_consolecmds.h"  // start camera
#include "cg_draw.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_marks.h"
#ifdef MISSIONPACK
  #include "cg_newdraw.h"  // CG_ShowResponseHead()
#endif
#include "cg_players.h"
#include "cg_servercmds.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "sc.h"
#include "wolfcam_servercmds.h"

//#include "../../ui/menudef.h" // bk001205 - for Q3_ui as well
#include "wolfcam_local.h"

static void CG_MapRestart (void);


typedef struct {
	const char *order;
	int taskNum;
} orderTask_t;

#ifdef MISSIONPACK
static const orderTask_t validOrders[] = {
	{ VOICECHAT_GETFLAG,						TEAMTASK_OFFENSE },
	{ VOICECHAT_OFFENSE,						TEAMTASK_OFFENSE },
	{ VOICECHAT_DEFEND,							TEAMTASK_DEFENSE },
	{ VOICECHAT_DEFENDFLAG,					TEAMTASK_DEFENSE },
	{ VOICECHAT_PATROL,							TEAMTASK_PATROL },
	{ VOICECHAT_CAMP,								TEAMTASK_CAMP },
	{ VOICECHAT_FOLLOWME,						TEAMTASK_FOLLOW },
	{ VOICECHAT_RETURNFLAG,					TEAMTASK_RETRIEVE },
	{ VOICECHAT_FOLLOWFLAGCARRIER,	TEAMTASK_ESCORT }
};

static const int numValidOrders = ARRAY_LEN(validOrders);

static int CG_ValidOrder(const char *p) {
	int i;
	for (i = 0; i < numValidOrders; i++) {
		if (Q_stricmp(p, validOrders[i].order) == 0) {
			return validOrders[i].taskNum;
		}
	}
	return -1;
}
#endif

static void CG_RoundStart (void)
{
	int i;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		wclients[i].aliveThisRound = qtrue;
	}

	trap_SendConsoleCommand("exec roundstart.cfg\n");
}

static void CG_RoundEnd (void)
{
	trap_SendConsoleCommand("exec roundend.cfg\n");
}


//FIXME hack for demos that might send stray premium scores commands to
//      non premium users
#define MAX_OLD_SCORE_TIME 500

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void ) {
	int		i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int SCSIZE;

	if (cg.snap->serverTime > (cg.duelScoresServerTime + MAX_OLD_SCORE_TIME)) {
		cg.duelScoresValid = qfalse;
	}
	if (cg.snap->serverTime > (cg.tdmScore.serverTime + MAX_OLD_SCORE_TIME)) {
		if (cg.tdmScore.valid) {
			memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));
			//Com_Printf("^1clearing tdm scores\n");
		}
		cg.tdmScore.valid = qfalse;
	}
	if (cg.snap->serverTime > (cg.ctfScore.serverTime + MAX_OLD_SCORE_TIME)) {
		if (cg.ctfScore.valid) {
			memset(&cg.ctfScore, 0, sizeof(cg.ctfScore));
		}
		cg.ctfScore.valid = qfalse;
	}

	cg.scoresValid = qtrue;
	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		Com_Printf("^3CG_ParseScores() cg.numScores (%d) > MAX_CLIENTS (%d)\n", cg.numScores, MAX_CLIENTS);
		cg.numScores = MAX_CLIENTS;
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cgs.clientinfo[i].scoreValid = qfalse;
		cg.clientHasScore[i] = qfalse;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	//memset( cg.scores, 0, sizeof( cg.scores ) );

	if (cgs.protocol == PROTOCOL_QL) {
		SCSIZE = 18;
	} else {
		SCSIZE = 14;
	}

	for ( i = 0 ; i < cg.numScores ; i++ ) {
		int clientNum;

		clientNum = atoi( CG_Argv( i * SCSIZE + 4 ) );
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores() invalid client number %d\n", clientNum);
		}
		cg.scores[i].client = clientNum;
		cg.scores[i].score = atoi( CG_Argv( i * SCSIZE + 5 ) );
		cg.scores[i].ping = atoi( CG_Argv( i * SCSIZE + 6 ) );
		cg.scores[i].time = atoi( CG_Argv( i * SCSIZE + 7 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * SCSIZE + 8 ) );
		cg.scores[i].powerups = atoi( CG_Argv( i * SCSIZE + 9 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * SCSIZE + 10));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * SCSIZE + 11));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * SCSIZE + 12));
		cg.scores[i].gauntletCount = atoi(CG_Argv(i * SCSIZE + 13));
		cg.scores[i].defendCount = atoi(CG_Argv(i * SCSIZE + 14));
		cg.scores[i].assistCount = atoi(CG_Argv(i * SCSIZE + 15));
		cg.scores[i].perfect = atoi(CG_Argv(i * SCSIZE + 16));
		cg.scores[i].captures = atoi(CG_Argv(i * SCSIZE + 17));

		if (cgs.protocol == PROTOCOL_QL) {
			cg.scores[i].alive = atoi(CG_Argv(i * SCSIZE + 18));
			cg.scores[i].frags = atoi(CG_Argv(i * SCSIZE + 19));
			cg.scores[i].deaths = atoi(CG_Argv(i * SCSIZE + 20));
			cg.scores[i].bestWeapon = atoi(CG_Argv(i * SCSIZE + 21));
		}

		//Com_Printf("score %d %s\n", i, cgs.clientinfo[cg.scores[i].client].name);
		//Com_Printf("sc %d (%d  %d)\n", i, cg.scores[i].scoreFlags, cg.scores[i].alive);
		//Com_Printf("sc %d (%d  %d %d  -- %d %d)\n", i, cg.scores[i].scoreFlags, cg.scores[i].perfect, cg.scores[i].captures, cg.scores[i].frags, cg.scores[i].deaths);

#if 0
		Com_Printf("%d  %d  %d  %d\n",
				   atoi(CG_Argv(i * SCSIZE + 18)),
				   atoi(CG_Argv(i * SCSIZE + 19)),
				   atoi(CG_Argv(i * SCSIZE + 20)),
				   atoi(CG_Argv(i * SCSIZE + 21)));

		Com_Printf("accuracy: %d\n", cg.scores[i].accuracy);
#endif

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			Com_Printf("FIXME score->client invalid: %d\n", cg.scores[i].client);
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;
		cgs.clientinfo[ cg.scores[i].client ].powerups = cg.scores[i].powerups;
		cgs.clientinfo[cg.scores[i].client].scoreIndexNum = i;

		cgs.clientinfo[cg.scores[i].client].scoreValid = qtrue;
		cg.clientHasScore[cg.scores[i].client] = qtrue;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

		//Com_Printf("^1sss  %s  score %d\n", cgs.clientinfo[cg.scores[i].client].name, cgs.clientinfo[cg.scores[i].client].score);

		if (cg.scores[i].team == TEAM_RED) {
			redCount++;
			redTotalPing += cg.scores[i].ping;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += cg.scores[i].ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	//Com_Printf("^5red ping %d  blue ping %d\n", cg.avgRedPing, cg.avgBluePing);

#if 1  //def MPACK
	CG_SetScoreSelection(NULL);
#endif

	if (CG_IsDuelGame(cgs.gametype)) {
		CG_SetDuelPlayers();
	}

	// check sizes
	if (CG_Argc() != (i * SCSIZE + 4)) {
		CG_Printf("^1CG_ParseScores() argc (%d) != %d\n", CG_Argc(), (i * SCSIZE + 4));
	}

//#undef SCSIZE
}
#undef MAX_OLD_SCORE_TIME

static void CG_ParseRRScores (void)
{
	int		i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int SCSIZE;

	// they forgot score->team ?

	cg.scoresValid = qtrue;
	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		Com_Printf("^3CG_ParseRRScores() numScores (%d) > MAX_CLIENTS (%d)\n", cg.numScores, MAX_CLIENTS);
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	//memset( cg.scores, 0, sizeof( cg.scores ) );

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cgs.clientinfo[i].scoreValid = qfalse;
		cg.clientHasScore[i] = qfalse;
	}

	SCSIZE = 19;

	for ( i = 0 ; i < cg.numScores ; i++ ) {
		int clientNum;

		clientNum = atoi( CG_Argv( i * SCSIZE + 4 ) );
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseRRScores() invalid client number %d\n", clientNum);
		}
		cg.scores[i].client = clientNum;
		cg.scores[i].score = atoi( CG_Argv( i * SCSIZE + 5 ) );
		// 6
		cg.scores[i].ping = atoi( CG_Argv( i * SCSIZE + 7 ) );
		cg.scores[i].time = atoi( CG_Argv( i * SCSIZE + 8 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * SCSIZE + 9 ) );
		cg.scores[i].powerups = atoi( CG_Argv( i * SCSIZE + 10 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * SCSIZE + 11));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * SCSIZE + 12));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * SCSIZE + 13));
		cg.scores[i].gauntletCount = atoi(CG_Argv(i * SCSIZE + 14));
		cg.scores[i].defendCount = atoi(CG_Argv(i * SCSIZE + 15));
		cg.scores[i].assistCount = atoi(CG_Argv(i * SCSIZE + 16));
		cg.scores[i].perfect = atoi(CG_Argv(i * SCSIZE + 17));
		cg.scores[i].captures = atoi(CG_Argv(i * SCSIZE + 18));

		cg.scores[i].alive = atoi(CG_Argv(i * SCSIZE + 19));
		cg.scores[i].frags = atoi(CG_Argv(i * SCSIZE + 20));
		cg.scores[i].deaths = atoi(CG_Argv(i * SCSIZE + 21));
		cg.scores[i].bestWeapon = atoi(CG_Argv(i * SCSIZE + 22));

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			Com_Printf("FIXME score->client invalid: %d\n", cg.scores[i].client);
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;

		cgs.clientinfo[cg.scores[i].client].scoreValid = qtrue;
		cg.clientHasScore[cg.scores[i].client] = qtrue;

		cgs.clientinfo[ cg.scores[i].client ].powerups = cg.scores[i].powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

		if (cg.scores[i].team == TEAM_RED) {
			redCount++;
			redTotalPing += cg.scores[i].ping;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += cg.scores[i].ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

#if 1  //def MPACK
	CG_SetScoreSelection(NULL);
#endif

	// check sizes
	if (CG_Argc() != (i * SCSIZE + 4)) {
		CG_Printf("^1CG_ParseRRScores() argc (%d) != %d\n", CG_Argc(), (i * SCSIZE + 4));
	}
}

static void CG_ParseCaScores (void)
{
	int i;
	int clientNum;
	tdmPlayerScore_t *s;

	//memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;

	i = 0;

	while (1) {
		if (!*CG_Argv(i + 1)) {
			break;
		}
		clientNum = atoi(CG_Argv(i + 1));
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseCaScores() invalid client number %d\n", clientNum);
			return;
		}
		s = &cg.tdmScore.playerScore[clientNum];
		s->valid = qtrue;

		s->team = atoi(CG_Argv(i + 2));
		s->subscriber = atoi(CG_Argv(i + 3));
		s->damageDone = atoi(CG_Argv(i + 4));
		s->accuracy = atoi(CG_Argv(i + 5));

		i += 5;
	}

	// check sizes
	if (CG_Argc() != (i + 1)) {
		CG_Printf("^1CG_ParseCaScores() argc (%d) != %d\n", CG_Argc(), (i + 1));
	}
}

static void CG_ParseTdmScores (void)
{
	int i;
	int clientNum;
	int team;

	//memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 1;

	cg.tdmScore.rra = atoi(CG_Argv(1));
	cg.tdmScore.rya = atoi(CG_Argv(2));
	cg.tdmScore.rga = atoi(CG_Argv(3));
	cg.tdmScore.rmh = atoi(CG_Argv(4));
	cg.tdmScore.rquad = atoi(CG_Argv(5));
	cg.tdmScore.rbs = atoi(CG_Argv(6));
	cg.tdmScore.rquadTime = atoi(CG_Argv(7));
	cg.tdmScore.rbsTime = atoi(CG_Argv(8));

	cg.tdmScore.bra = atoi(CG_Argv(9));
	cg.tdmScore.bya = atoi(CG_Argv(10));
	cg.tdmScore.bga = atoi(CG_Argv(11));
	cg.tdmScore.bmh = atoi(CG_Argv(12));
	cg.tdmScore.bquad = atoi(CG_Argv(13));
	cg.tdmScore.bbs = atoi(CG_Argv(14));
	cg.tdmScore.bquadTime = atoi(CG_Argv(15));
	cg.tdmScore.bbsTime = atoi(CG_Argv(16));

	i = 17;
	while (1) {
		if (!*CG_Argv(i)) {
			break;
		}
		clientNum = atoi(CG_Argv(i));
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseTdmScores() invalid client number %d\n", clientNum);
			return;
		}

		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.tdmScore.playerScore[clientNum].valid = qtrue;

			cg.tdmScore.playerScore[clientNum].team = atoi(CG_Argv(i + 1));
			cg.tdmScore.playerScore[clientNum].tks = atoi(CG_Argv(i + 2));
			cg.tdmScore.playerScore[clientNum].tkd = atoi(CG_Argv(i + 3));
			cg.tdmScore.playerScore[clientNum].damageDone = atoi(CG_Argv(i + 4));

			//Com_Printf("%d  (%d %d %d   -- %d)\n", clientNum, cg.tdmScore.playerScore[clientNum].team, cg.tdmScore.playerScore[clientNum].tkd, cg.tdmScore.playerScore[clientNum].tks, cg.tdmScore.playerScore[clientNum].damageDone);

		}

		i += 5;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseTdmScores() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseTdmScores2 (void)
{
	int i;
	int clientNum;
	int team;

	//memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 2;

	cg.tdmScore.rra = atoi(CG_Argv(1));
	cg.tdmScore.rya = atoi(CG_Argv(2));
	cg.tdmScore.rga = atoi(CG_Argv(3));
	cg.tdmScore.rmh = atoi(CG_Argv(4));
	cg.tdmScore.rquad = atoi(CG_Argv(5));
	cg.tdmScore.rbs = atoi(CG_Argv(6));
	cg.tdmScore.rregen = atoi(CG_Argv(7));
	cg.tdmScore.rhaste = atoi(CG_Argv(8));
	cg.tdmScore.rinvis = atoi(CG_Argv(9));

	cg.tdmScore.rquadTime = atoi(CG_Argv(10));
	cg.tdmScore.rbsTime = atoi(CG_Argv(11));
	cg.tdmScore.rregenTime = atoi(CG_Argv(12));
	cg.tdmScore.rhasteTime = atoi(CG_Argv(13));
	cg.tdmScore.rinvisTime = atoi(CG_Argv(14));

	cg.tdmScore.bra = atoi(CG_Argv(15));
	cg.tdmScore.bya = atoi(CG_Argv(16));
	cg.tdmScore.bga = atoi(CG_Argv(17));
	cg.tdmScore.bmh = atoi(CG_Argv(18));
	cg.tdmScore.bquad = atoi(CG_Argv(19));
	cg.tdmScore.bbs = atoi(CG_Argv(20));
	cg.tdmScore.bregen = atoi(CG_Argv(21));
	cg.tdmScore.bhaste = atoi(CG_Argv(22));
	cg.tdmScore.binvis = atoi(CG_Argv(23));

	cg.tdmScore.bquadTime = atoi(CG_Argv(24));
	cg.tdmScore.bbsTime = atoi(CG_Argv(25));
	cg.tdmScore.bregenTime = atoi(CG_Argv(26));
	cg.tdmScore.bhasteTime = atoi(CG_Argv(27));
	cg.tdmScore.binvisTime = atoi(CG_Argv(28));

	i = 29;

	while (1) {
		if (!*CG_Argv(i)) {
			break;
		}
		clientNum = atoi(CG_Argv(i));
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseTdmScores2() invalid client number %d\n", clientNum);
			return;
		}

		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.tdmScore.playerScore[clientNum].valid = qtrue;

			cg.tdmScore.playerScore[clientNum].team = atoi(CG_Argv(i + 1));
			cg.tdmScore.playerScore[clientNum].subscriber = atoi(CG_Argv(i + 2));
			cg.tdmScore.playerScore[clientNum].tks = atoi(CG_Argv(i + 3));
			cg.tdmScore.playerScore[clientNum].tkd = atoi(CG_Argv(i + 4));
			cg.tdmScore.playerScore[clientNum].damageDone = atoi(CG_Argv(i + 5));

		}

		i += 6;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseTdmScores2() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseCtfScores (void)
{
	int i;
	int clientNum;
	int team;

	//memset(&cg.ctfScore, 0, sizeof(cg.ctfScore));

	cg.ctfScore.valid = qtrue;
	cg.ctfScore.version = 0;

	cg.ctfScore.rra = atoi(CG_Argv(1));
	cg.ctfScore.rya = atoi(CG_Argv(2));
	cg.ctfScore.rga = atoi(CG_Argv(3));
	cg.ctfScore.rmh = atoi(CG_Argv(4));
	cg.ctfScore.rquad = atoi(CG_Argv(5));
	cg.ctfScore.rbs = atoi(CG_Argv(6));
	cg.ctfScore.rregen = atoi(CG_Argv(7));
	cg.ctfScore.rhaste = atoi(CG_Argv(8));
	cg.ctfScore.rinvis = atoi(CG_Argv(9));
	cg.ctfScore.rflag = atoi(CG_Argv(10));
	cg.ctfScore.rmedkit = atoi(CG_Argv(11));

	cg.ctfScore.rquadTime = atoi(CG_Argv(12));
	cg.ctfScore.rbsTime = atoi(CG_Argv(13));
	cg.ctfScore.rregenTime = atoi(CG_Argv(14));
	cg.ctfScore.rhasteTime = atoi(CG_Argv(15));
	cg.ctfScore.rinvisTime = atoi(CG_Argv(16));
	cg.ctfScore.rflagTime = atoi(CG_Argv(17));

	cg.ctfScore.bra = atoi(CG_Argv(18));
	cg.ctfScore.bya = atoi(CG_Argv(19));
	cg.ctfScore.bga = atoi(CG_Argv(20));
	cg.ctfScore.bmh = atoi(CG_Argv(21));
	cg.ctfScore.bquad = atoi(CG_Argv(22));
	cg.ctfScore.bbs = atoi(CG_Argv(23));
	cg.ctfScore.bregen = atoi(CG_Argv(24));
	cg.ctfScore.bhaste = atoi(CG_Argv(25));
	cg.ctfScore.binvis = atoi(CG_Argv(26));
	cg.ctfScore.bflag = atoi(CG_Argv(27));
	cg.ctfScore.bmedkit = atoi(CG_Argv(28));

	cg.ctfScore.bquadTime = atoi(CG_Argv(29));
	cg.ctfScore.bbsTime = atoi(CG_Argv(30));
	cg.ctfScore.bregenTime = atoi(CG_Argv(31));
	cg.ctfScore.bhasteTime = atoi(CG_Argv(32));
	cg.ctfScore.binvisTime = atoi(CG_Argv(33));
	cg.ctfScore.bflagTime = atoi(CG_Argv(34));

	i = 35;

	while (1) {
		if (!*CG_Argv(i)) {
			break;
		}
		clientNum = atoi(CG_Argv(i));
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseCtfScores() invalid client number %d\n", clientNum);
			return;
		}

		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.ctfScore.playerScore[clientNum].valid = qtrue;

			cg.ctfScore.playerScore[clientNum].team = atoi(CG_Argv(i + 1));
			cg.ctfScore.playerScore[clientNum].subscriber = atoi(CG_Argv(i + 2));
			//cg.ctfScore.playerScore[clientNum].tks = atoi(CG_Argv(i + 3));
			//cg.ctfScore.playerScore[clientNum].tkd = atoi(CG_Argv(i + 4));
			//cg.ctfScore.playerScore[clientNum].damageDone = atoi(CG_Argv(i + 5));
		}

		i += 3;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseCtfScores() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseCaStats (void)
{
	int clientNum;
	caStats_t *s;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^1CG_ParseCaStats() invalid client number %d\n", clientNum);
		return;
	}

	s = &cg.caStats[clientNum];
	s->valid = qtrue;
	s->clientNum = clientNum;
	s->damageDone = atoi(CG_Argv(2));
	s->damageReceived = atoi(CG_Argv(3));
	// 4  gauntlet accuracy, ql loops through all the weapons generically
	s->gauntKills = atoi(CG_Argv(5));
	s->mgAccuracy = atoi(CG_Argv(6));
	s->mgKills = atoi(CG_Argv(7));
	s->sgAccuracy = atoi(CG_Argv(8));
	s->sgKills = atoi(CG_Argv(9));
	s->glAccuracy = atoi(CG_Argv(10));
	s->glKills = atoi(CG_Argv(11));
	s->rlAccuracy = atoi(CG_Argv(12));
	s->rlKills = atoi(CG_Argv(13));
	s->lgAccuracy = atoi(CG_Argv(14));
	s->lgKills = atoi(CG_Argv(15));
	s->rgAccuracy = atoi(CG_Argv(16));
	s->rgKills = atoi(CG_Argv(17));
	s->pgAccuracy = atoi(CG_Argv(18));
	s->pgKills = atoi(CG_Argv(19));
	s->bfgAccuracy = atoi(CG_Argv(20));
	s->bfgKills = atoi(CG_Argv(21));
	s->grappleAccuracy = atoi(CG_Argv(22));
	s->grappleKills = atoi(CG_Argv(23));
	s->ngAccuracy = atoi(CG_Argv(24));
	s->ngKills = atoi(CG_Argv(25));
	s->plAccuracy = atoi(CG_Argv(26));
	s->plKills = atoi(CG_Argv(27));
	s->cgAccuracy = atoi(CG_Argv(28));
	s->cgKills = atoi(CG_Argv(29));
	s->hmgAccuracy = atoi(CG_Argv(30));
	s->hmgKills = atoi(CG_Argv(31));
}


static void CG_ParseTdmStats (void)
{
	//int i;
	int clientNum;
	tdmStats_t *ts;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^1CG_ParseTdmStats() invalid client number %d\n", clientNum);
		return;
	}

	//FIXME for freezetag not clientNum, it's index into cg.scores
	// no, always index into scores
	if (0) {  //(cgs.gametype == GT_FREEZETAG) {
		ts = &cg.tdmStats[cg.scores[clientNum].client];
	} else {  //FIXME double check
		ts = &cg.tdmStats[clientNum];
	}

	ts->valid = qtrue;
	// this isn't real clientNum, it's index into cg.scores
	ts->clientNum = clientNum;
	ts->selfKill = atoi(CG_Argv(2));
	ts->tks = atoi(CG_Argv(3));
	ts->tkd = atoi(CG_Argv(4));
	ts->damageDone = atoi(CG_Argv(5));
	ts->damageReceived = atoi(CG_Argv(6));
	ts->ra = atoi(CG_Argv(7));
	ts->ya = atoi(CG_Argv(8));
	ts->ga = atoi(CG_Argv(9));
	ts->mh = atoi(CG_Argv(10));
	ts->quad = atoi(CG_Argv(11));
	ts->bs = atoi(CG_Argv(12));

	//FIXME weapon stats
}

static void CG_ParseCtfStats (void)
{
	//int i;
	int clientNum;
	ctfStats_t *ts;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^1CG_ParseCtfStats() invalid client number %d\n", clientNum);
		return;
	}

	ts = &cg.ctfStats[clientNum];
	ts->valid = qtrue;
	ts->clientNum = clientNum;
	ts->selfKill = atoi(CG_Argv(2));
	ts->damageDone = atoi(CG_Argv(3));
	ts->damageReceived = atoi(CG_Argv(4));
	ts->ra = atoi(CG_Argv(5));
	ts->ya = atoi(CG_Argv(6));
	ts->ga = atoi(CG_Argv(7));
	ts->mh = atoi(CG_Argv(8));
	ts->quad = atoi(CG_Argv(9));
	ts->bs = atoi(CG_Argv(10));
	ts->regen = atoi(CG_Argv(11));
	ts->haste = atoi(CG_Argv(12));
	ts->invis = atoi(CG_Argv(13));
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo_91 (void)
{
	int		i;
	int j;
	int		clientNum;
	clientInfo_t *ci;
	const entityState_t *es;
	const playerState_t *ps;

	// handling in transition snapshot
	if (1) {
		return;
	}

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );

	if (numSortedTeamPlayers >= TEAM_MAXOVERLAY) {
		CG_Printf("^3team info91:  numSortedTeamPlayers >= TEAM_MAXOVERLAY\n");
		numSortedTeamPlayers = TEAM_MAXOVERLAY - 1;
	}

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		clientNum = atoi( CG_Argv( i + 2 ) );
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseTeamInfo_91() invalid client number %d\n", clientNum);
			return;
		}
		sortedTeamPlayers[i] = clientNum;
		//Com_Printf("%s\n", cgs.clientinfo[clientNum].name);
		ci = &cgs.clientinfo[clientNum];
		if (clientNum == cg.snap->ps.clientNum) {
			ps = &cg.snap->ps;
			ci->location = ps->location;
			ci->health = ps->stats[STAT_HEALTH];
			ci->armor = ps->stats[STAT_ARMOR];
			ci->curWeapon = ps->weapon;
			ci->powerups = 0;
			for (j = 0;  j < MAX_POWERUPS;  j++) {
				if (ps->powerups[j]) {
					ci->powerups |= 1 << j;
				}
			}
		} else {
			es = &cg_entities[clientNum].currentState;
			ci->location = es->location;
			ci->health = es->health;
			ci->armor = es->armor;
			ci->curWeapon = es->weapon;
			ci->powerups = es->powerups;
		}

		ci->hasTeamInfo = qtrue;
	}
}

static void CG_ParseTeamInfo( void ) {
	int		i;
	int		clientNum;
	//int healthOld;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );
	if (numSortedTeamPlayers >= TEAM_MAXOVERLAY) {
		CG_Printf("^3team info:  numSortedTeamPlayers >= TEAM_MAXOVERLAY\n");
		numSortedTeamPlayers = TEAM_MAXOVERLAY - 1;
	}

	//Com_Printf("CG_ParseTeamInfo numSortedTeamPlayers %d\n", numSortedTeamPlayers);

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		clientNum = atoi( CG_Argv( i * 6 + 2 ) );
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseTeamInfo() invalid client number %d\n", clientNum);
			return;
		}
		sortedTeamPlayers[i] = clientNum;
		//Com_Printf("%s\n", cgs.clientinfo[clientNum].name);

		cgs.clientinfo[ clientNum ].location = atoi( CG_Argv( i * 6 + 3 ) );

		//healthOld = cgs.clientinfo[clientNum].health;

		cgs.clientinfo[ clientNum ].health = atoi( CG_Argv( i * 6 + 4 ) );

#if 0
		if (healthOld > cgs.clientinfo[clientNum].health) {
			cgs.clientinfo[clientNum].hitTime = cg.ftime;
			Com_Printf("player %d hit...\n", clientNum);
		}
#endif

		cgs.clientinfo[ clientNum ].armor = atoi( CG_Argv( i * 6 + 5 ) );
		cgs.clientinfo[ clientNum ].curWeapon = atoi( CG_Argv( i * 6 + 6 ) );
		cgs.clientinfo[ clientNum ].powerups = atoi( CG_Argv( i * 6 + 7 ) );
		cgs.clientinfo[clientNum].hasTeamInfo = qtrue;
	}
}

/*
  1069 linux-i386 May 25 2016 20:02:14
  1069 win-x86 Jun  3 2016 16:09:50

  1068 linux-x64 Mar  4 2016 19:27:22
  1068 win-x86 Apr 28 2016 15:24:47

  1067 win-x86 Feb 22 2016 13:59:16
  1064 win-x86 Nov 10 2015 10:16:14
  1064 win-x86 Oct 30 2015 12:09:55

  beta testing:  \version\b1032 linux-i386 Jul 20 2015 05:19:10\
                 \version\b1032 win-x86 Jul 20 2015 00:19:38\

  Quake Live  0.1.0.1013 win-x86 Apr 28 2015 10:43:33

  Quake Live  0.1.0.934 win-x86 Aug 26 2014 18:06:25 (new dm90 protocol)

  QuakeLive  0.1.0.790 linux-i386 Jul 17 2013 16:28:51
  QuakeLive  0.1.0.785 linux-i386 Jul 15 2013 16:03:55
      * race game type,
      * team health/status icons
  QuakeLive  0.1.0.728 linux-i386 Feb 28 2013 13:20:49 (new scores)
  0.1.0.719 linux-i386 Feb 21 2013 11:37:37  (has new scores commands)

  0.1.0.578  linux-i386 May 16 2012 17:53:25
  0.1.0.577  linux-i386 May 11 2012 16:18:15
       * last standing vo includes team number
  0.1.0.571  QuakeLive linux-i386 Apr 17 2012 14:51:00
  0.1.0.495  QuakeLive  0.1.0.495 linux-i386 Dec 14 2011 16:08:54
       * new ctf scoreboard, orange color for weapon bar low ammo text
  0.1.0.491  QuakeLive  0.1.0.491 linux-i386 Oct 20 2011 17:01:17
  0.1.0.484  QuakeLive  0.1.0.484 linux-i386 Jul 19 2011 17:51:58  (hotfix)
  0.1.0.482  QuakeLive  0.1.0.482 linux-i386 Jul 18 2011 10:19:51
       * new tdm scoreboard
  0.1.0.459  QuakeLive  0.1.0.459 linux-i386 May 31 2011 17:57:10
  0.1.0.432  QuakeLive  0.1.0.432 linux-i386 Jan 31 2011 19:12:31
  0.1.0.420  QuakeLive  0.1.0.420 linux-i386 Dec 13 2010 15:31:41
  0.1.0.386  QuakeLive  0.1.0.386 linux-i386 Aug  5 2010 18:28:00
       * first non beta version I could play, there was a version before but
         couldn't log in
  0.1.0.351  QuakeLive  0.1.0.351 linux-i386 Jul  9 2010 23:05:36
       * last version I saw before the move out of beta
  0.1.0.309                                  Mar 8  2010 ??
  0.1.0.305  QuakeLive  0.1.0.305 linux-i386 Feb 25 2010 20:23:54
  0.1.0.303  QuakeLive  0.1.0.303 linux-i386 Feb 23 2010 19:24:17
       * new rail colors
  0.1.0.288  QuakeLive  0.1.0.288 linux-i386 Dec 19 2009 17:54:38
  0.1.0.284  QuakeLive  0.1.0.284 linux-i386 Dec  8 2009 23:28:39
       * impact sparks (rail registers as missile hit)

  0.1.0.277  QuakeLive  0.1.0.277 linux-i386 Nov 20 2009 21:30:30
  0.1.0.263
  0.1.0.258
  0.1.0.256  QuakeLive  0.1.0.256 linux-i386 Aug 10 2009 19:56:10
*/

#define BUFFER_LENGTH 1024
static void CG_ParseVersion (const char *info)
{
	const char *val;
	const char *p;
	char buffer[BUFFER_LENGTH];
	int bufferPos;
	int i;

	val = Info_ValueForKey(info, "version");
	//Com_Printf("version:  %s\n", val);
	// vote failed
	// quakelive  9

	cgs.isQuakeLiveBetaDemo = qfalse;

	if (cg.demoPlayback) {
		//FIXME other beta numbers
		if (!Q_stricmpn(val, "b1032", strlen("b1032"))) {
			cgs.isQuakeLiveDemo = qtrue;
			cgs.isQuakeLiveBetaDemo = qtrue;
			cgs.qlversion[0] = 1032;

			//Com_Printf("^6quake live demo recorded with beta %d\n", cgs.qlversion[0]);
			return;
		}

		// protocol 91 no longer has 'quake live' string in version
		if (!Q_stricmpn(val, "QuakeLive", strlen("QuakeLive"))  ||  !Q_stricmpn(val, "Quake Live", strlen("Quake Live"))  ||  cgs.realProtocol >= 91) {
			cgs.isQuakeLiveDemo = qtrue;
		} else {
			cgs.isQuakeLiveDemo = qfalse;
		}
	}

	//Com_Printf("^5ql demo:  %d\n", cgs.isQuakeLiveDemo);

	for (i = 0;  i < 4;  i++) {
		cgs.qlversion[i] = 0;
	}

	// advance to first num
	p = val;
	while (p[0] < '0'  ||  p[0] > '9') {
		if (p[0] == '\0') {
			//Com_Printf("^1couldn't detect quake live version number [initial digit]\n");
			return;
		}
		p++;
	}

	for (i = 0;  i < 4;  i++) {
		if (p[0] == '\0') {
			//Com_Printf("^1couldn't parse quake live version string..\n");
			return;
		}
		bufferPos = 0;
		while (p[0] >= '0'  &&  p[0] <= '9') {
			if (p[0] == '\0'  ||  bufferPos >= (BUFFER_LENGTH - 2)) {
				//Com_Printf("^1couldn't parse quake live version string...\n");
				return;
			}
			buffer[bufferPos] = p[0];
			bufferPos++;
			p++;
		}

		if (p[0] == '\0') {
			//Com_Printf("^1couldn't parse quake live version string....\n");
			return;
		}
		p++;  // skip '.'
		buffer[bufferPos] = '\0';
		cgs.qlversion[i] = atoi(buffer);
	}

	if (cgs.isQuakeLiveDemo) {
		if (cgs.qlversion[0] > 0  ||  cgs.qlversion[1] > 0  ||  cgs.qlversion[2] > 0  ||  cgs.qlversion[3] > 0) {
			//Com_Printf("^5demo recorded with QuakeLive version:  %d.%d.%d.%d\n", cgs.qlversion[0], cgs.qlversion[1], cgs.qlversion[2], cgs.qlversion[3]);
		}
	} else {
		//Com_Printf("^3quake live version wasn't parsed\n");
	}
}

#undef BUFFER_LENGTH


static void CG_LoadServerModelOverride (void)
{
	char modelStr[MAX_QPATH];
	char *skin;
	const char *dir;
	const char *s;
	int i;

	//Com_Printf("loading server model\n");

	memset(&cg.serverModel, 0, sizeof(cg.serverModel));

	if (*cgs.serverModelOverride) {
		Q_strncpyz(modelStr, cgs.serverModelOverride, sizeof(modelStr));
		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz(cg.serverModel.modelName, modelStr, sizeof(cg.serverModel.modelName));
		Q_strncpyz(cg.serverModel.skinName, skin, sizeof(cg.serverModel.skinName));

		Q_strncpyz(cg.serverModel.headModelName, cg.serverModel.modelName, sizeof(cg.serverModel.headModelName));
		Q_strncpyz(cg.serverModel.headSkinName, cg.serverModel.skinName, sizeof(cg.serverModel.headSkinName));

		CG_RegisterClientModelname(&cg.serverModel, cg.serverModel.modelName, cg.serverModel.skinName, cg.serverModel.headModelName, cg.serverModel.headSkinName, "", qtrue);

		// sounds
		dir = cg.serverModel.modelName;
		//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

		for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
			s = cg_customSoundNames[i];
			if (!s) {
				break;
			}
			cg.serverModel.sounds[i] = 0;

			// if the model didn't load use the sounds of the default model
			cg.serverModel.sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
		}
	}

	if (*cgs.serverHeadModelOverride) {
		char filename[MAX_QPATH * 2];

		Q_strncpyz(modelStr, cgs.serverHeadModelOverride, sizeof(modelStr));
		if ((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz(cg.serverModel.headModelName, modelStr, sizeof(cg.serverModel.headModelName));
		Q_strncpyz(cg.serverModel.headSkinName, skin, sizeof(cg.serverModel.headSkinName));

		//CG_RegisterClientModelname(&cg.serverModel, cg.serverModel.modelName, cg.serverModel.skinName, cg.serverModel.headModelName, cg.serverModel.headSkinName, "", qtrue);

		// just the head
		if (0) {  //(cg.serverModel.headModelName[0] == '*') {  //FIXME what is this again??
			Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", &cg.serverModel.headModelName[1], &cg.serverModel.headModelName[1]);
        }
        else {
			Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", cg.serverModel.headModelName);
        }

		cg.serverModel.headModel = trap_R_RegisterModel(filename);
		if (!cg.serverModel.headModel) {
			Com_Printf("^3CG_LoadServerModelOverride() couldn't load head model '%s'\n", filename);
			return;
		}

		if (CG_FindClientHeadFile(filename, sizeof(filename), &cg.serverModel, "", cg.serverModel.headModelName, cg.serverModel.headSkinName, "head", "skin", qtrue)) {
			cg.serverModel.headSkin = CG_RegisterSkinVertexLight(filename);
        }
        if (!cg.serverModel.headSkin) {
			Com_Printf("^3CG_LoadServerModelOverride() head skin load failure '%s'\n", filename);
        }
	}
}

void CG_LoadInfectedGameTypeModels (void)
{
	char *modelStr;
	char *skin;
	const char *dir;
	const char *s;
	int i;
	clientInfo_t *ci;

	ci = &cgs.bonesInfected;
	memset(ci, 0, sizeof(*ci));

	modelStr = "bones";
	skin = "default";

	Q_strncpyz(ci->modelName, modelStr, sizeof(ci->modelName));
	Q_strncpyz(ci->skinName, skin, sizeof(ci->skinName));

	Q_strncpyz(ci->headModelName, ci->modelName, sizeof(ci->headModelName));
	Q_strncpyz(ci->headSkinName, ci->skinName, sizeof(ci->headSkinName));

	CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, "", qtrue);

	// sounds
	dir = ci->modelName;
	//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
		s = cg_customSoundNames[i];
		if (!s) {
			break;
		}
		ci->sounds[i] = 0;

		// if the model didn't load use the sounds of the default model
		ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
	}

	ci = &cgs.urielInfected;
	memset(ci, 0, sizeof(*ci));

	modelStr = "uriel";
	skin = "default";

	Q_strncpyz(ci->modelName, modelStr, sizeof(ci->modelName));
	Q_strncpyz(ci->skinName, skin, sizeof(ci->skinName));

	Q_strncpyz(ci->headModelName, ci->modelName, sizeof(ci->headModelName));
	Q_strncpyz(ci->headSkinName, ci->skinName, sizeof(ci->headSkinName));

	CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, "", qtrue);

	// sounds
	dir = ci->modelName;
	//fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for (i = 0;  i < MAX_CUSTOM_SOUNDS;  i++) {
		s = cg_customSoundNames[i];
		if (!s) {
			break;
		}
		ci->sounds[i] = 0;

		// if the model didn't load use the sounds of the default model
		ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
	}
}

static void CG_SetScorePlace (void)
{
	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (*CG_ConfigString(CS_FIRSTPLACE)  ||  *CG_ConfigString(CS_SECONDPLACE)) {
		Q_strncpyz(cgs.firstPlace, CG_ConfigString(CS_FIRSTPLACE), sizeof(cgs.firstPlace));
		Q_strncpyz(cgs.secondPlace, CG_ConfigString(CS_SECONDPLACE), sizeof(cgs.secondPlace));
	}
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo (qboolean firstCall, qboolean seeking)
{
	const char	*info;
	const char *str;
	const char	*mapname;
	int val, i, numClients;
	int df;
	qboolean instaGib;
	const char *gametypeConfig = NULL;

	info = CG_ConfigString( CS_SERVERINFO );

	//FIXME PROTOCOL_Q3
	cgs.gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	if (cgs.protocol == PROTOCOL_Q3) {
		if (cgs.cpma) {
			// done in CG_CpmaParseGameState()
		} else {
			switch (cgs.gametype) {
			case 2:
				cgs.gametype = GT_SINGLE_PLAYER;
				break;
			case 4:
				cgs.gametype = GT_CTF;
				break;
			case 5:
				cgs.gametype = GT_1FCTF;
				break;
			case 6:
				cgs.gametype = GT_OBELISK;
				break;
			case 7:
				cgs.gametype = GT_HARVESTER;
			default:
				break;
			}
		}
	} else if (cgs.protocol == PROTOCOL_QL) {
		if (cgs.gametype == 2) {
			cgs.gametype = GT_RACE;
		}
	}

	//FIXME q3 and ql differences
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	df = cgs.dmflags;
	df &= ~(DF_NO_SELF_HEALTH_DAMAGE | DF_NO_SELF_ARMOR_DAMAGE | DF_NO_FALLING_DAMAGE | DF_NO_FOOTSTEPS);
	if (df) {
		Com_Printf("^1Unknown dmflags: ^7%d\n", df);
	}
	instaGib = atoi(Info_ValueForKey(info, "g_instagib"));
	trap_Cvar_Set("g_instagib", Info_ValueForKey(info, "g_instagib"));

	cgs.teamflags = atoi( Info_ValueForKey( info, "teamflags" ) );

	if (cgs.cpma) {
		CG_CpmaParseGameState(qtrue);
	} else {
		cgs.scorelimit = atoi( Info_ValueForKey( info, "scorelimit" ) );
		cgs.fraglimit = atoi( Info_ValueForKey( info, "fraglimit" ) );
		cgs.roundlimit = atoi( Info_ValueForKey( info, "roundlimit" ) );
		cgs.roundtimelimit = atoi(Info_ValueForKey(info, "roundtimelimit"));
		cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );
		cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
		cgs.realTimelimit = cgs.timelimit;

		if (cgs.protocol == PROTOCOL_QL) {
			str = CG_ConfigString(CS_ROUND_STATUS);
			cgs.roundNum = atoi(Info_ValueForKey(str, "round"));
			cgs.roundTurn = atoi(Info_ValueForKey(str, "turn"));
		}
	}

	trap_Cvar_Set("cg_gametype", va("%i", cgs.gametype));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));

	if (cgs.timelimit == 0) {
		if (cg_levelTimerDirection.integer == 4) {
			int s, e;

			s = trap_GetGameEndTime();
			e = trap_GetGameStartTime();
			if (s  &&  e) {
				cgs.timelimit = trap_GetGameEndTime() - trap_GetGameStartTime();
				cgs.timelimit /= (60 * 1000);
			} else {
				cgs.timelimit = cg_levelTimerDefaultTimeLimit.integer;
				if (cgs.timelimit <= 0) {
					cgs.timelimit = 60;
				}
			}
		} else {  // timerdirection 3
			cgs.timelimit = cg_levelTimerDefaultTimeLimit.integer;
			if (cgs.timelimit <= 0) {
				cgs.timelimit = 60;
			}
		}
	}
	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );

	if (cgs.cpma) {
		//CG_Printf("^3CG_ParseServerInfo\n");
		CG_CpmaParseScores(seeking);
	}

	if (cgs.cpma) {
		const char *server_gameplay;
		const char *server_promode;
		qboolean promodeDetected;

		//FIXME older demos used 'server_promode' and even older demos didn't
		// have this info in config strings.  They were server side variables.
		//
		// cpma changelogs:
		//
		//   Notes for version 1.22 (8 Feb 04)
		//   ...
		//   chg: server_promode renamed to server_gameplay (default "CPM")
		//
		//   Notes for version 1.1 (20 Mar 03)
		//   ...
		//   add: arQmode :P (server_promode 2, callvote promode 2)
		//
		//   Notes for version 0.99y1 (14 Nov 02)
		//   ...
		//   chg: server_promode tagged as serverinfo for ASE etc
		//
		// Mega health wear off settings could be different from server info
		// settings.  That info is server side.  Ex:
		//    /callvote simplemega 1
		//

		promodeDetected = qfalse;

		server_gameplay = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "server_gameplay");
		if (server_gameplay[0] == '\0') {
			//Com_Printf("^5no server_gameplay\n");
			server_promode = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "server_promode");
			if (server_promode[0] == '\0') {
				//Com_Printf("^5no server_promode\n");
				// bot server_gameplay and server_promode missing, default to promode
				promodeDetected = qtrue;
			} else {
				// checking server_promode
				if (!Q_stricmp("1", server_promode)  ||  !Q_stricmp("2", server_promode)) {
					promodeDetected = qtrue;
				}
			}
		} else {
			// checking server_gameplay
			if (!Q_stricmp("cpm", server_gameplay)  ||  !Q_stricmp("pmc", server_gameplay)) {
				promodeDetected = qtrue;
			}
		}

		if (promodeDetected) {
			cgs.cpm = qtrue;
			cgs.rocketSpeed = 900;
			trap_Cvar_Set("cg_armorTiered", "1");
			//Com_Printf("^5cpm gameplay\n");
		} else {
			cgs.cpm = qfalse;
			cgs.rocketSpeed = 800;
			trap_Cvar_Set("cg_armorTiered", "0");
			//Com_Printf("^5not cpm gameplay\n");
		}
	}

	//FIXME these shouldn't be in CG_ParseServerInfo()

	if (!cgs.cpma) {
		cgs.voteModified = qfalse;
		cgs.voteTime = atoi(CG_ConfigString(CS_VOTE_TIME));
		cgs.voteYes = atoi(CG_ConfigString(CS_VOTE_YES));
		cgs.voteNo = atoi(CG_ConfigString(CS_VOTE_NO));
		Q_strncpyz(cgs.voteString, CG_ConfigString(CS_VOTE_STRING), sizeof(cgs.voteString));

		//FIXME team votes
		if (cgs.protocol == PROTOCOL_QL) {
			//FIXME team vote not seen yet in ql
		} else {
			cgs.teamVoteTime[0] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_TIME + 0));
			cgs.teamVoteTime[1] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_TIME + 1));
			Q_strncpyz(cgs.teamVoteString[0], CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_STRING + 0), MAX_STRING_TOKENS);
			Q_strncpyz(cgs.teamVoteString[1], CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_STRING + 1), MAX_STRING_TOKENS);
			cgs.teamVoteYes[0] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_YES + 0));
			cgs.teamVoteYes[1] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_YES + 1));
			cgs.teamVoteNo[0] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_NO + 0));
			cgs.teamVoteNo[1] = atoi(CG_ConfigStringNoConvert(CSQ3_TEAMVOTE_NO + 1));
			cgs.teamVoteModified[0] = qfalse;
			cgs.teamVoteModified[1] = qfalse;
		}
	}

	if (cgs.protocol != PROTOCOL_QL) {
		if (firstCall) {
			Q_strncpyz(cgs.redTeamClanTag, "Red Team", sizeof(cgs.redTeamClanTag));
			Q_strncpyz(cgs.redTeamName, "Red Team", sizeof(cgs.redTeamName));
			trap_Cvar_Set("g_redTeam", cgs.redTeamName);

			Q_strncpyz(cgs.blueTeamClanTag, "Blue Team", sizeof(cgs.blueTeamClanTag));
			Q_strncpyz(cgs.blueTeamName, "Blue Team", sizeof(cgs.blueTeamName));
			trap_Cvar_Set("g_blueTeam", cgs.blueTeamName);
		}
		CG_BuildSpectatorString();
	}

	//FIXME don't check for cgs.gametype < GT_MAX_GAME_TYPE since cpma gametype
	// can be negative

	if ((firstCall  ||  cgs.instaGib != instaGib)  &&  (cgs.gametype >= 0  &&  cgs.gametype < GT_MAX_GAME_TYPE)) {
		cgs.instaGib = instaGib;
		if (cgs.instaGib) {
			gametypeConfig = va("i%s", gametypeConfigs[cgs.gametype]);
		} else {
			gametypeConfig = gametypeConfigs[cgs.gametype];
		}
		if (gametypeConfig) {
			trap_SendConsoleCommand(va("exec %s\n", gametypeConfig));
		} else {
			Com_Printf("FIXME unknown gametype: %d\n", cgs.gametype);
		}
	}


	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	//////////////////////////////////////////////////
	// from here on only PROTOCOL_QL

	CG_SetScorePlace();

	if (firstCall) {
		// not including ca or freezetag since the config strings are being
		// filled with random player tags

		// 2015-08-13 qcon protocol 91 demos don't have cs for team name

		if (cgs.realProtocol < 91  && (*CG_ConfigString(CS_RED_TEAM_CLAN_TAG)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF))) {
			Q_strncpyz(cgs.redTeamClanTag, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamClanTag));
		} else {
			Q_strncpyz(cgs.redTeamClanTag, "Red Team", sizeof(cgs.redTeamClanTag));
		}

		if (cgs.realProtocol < 91  &&  (*CG_ConfigString(CS_RED_TEAM_CLAN_NAME)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF))) {
			Q_strncpyz(cgs.redTeamName, CG_ConfigString(CS_RED_TEAM_CLAN_NAME), sizeof(cgs.redTeamName));
		} else {
			Q_strncpyz(cgs.redTeamName, "Red Team", sizeof(cgs.redTeamName));
		}
		trap_Cvar_Set("g_redTeam", cgs.redTeamName);

		if (cgs.realProtocol < 91  &&  (*CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF))) {
			Q_strncpyz(cgs.blueTeamClanTag, CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG), sizeof(cgs.blueTeamClanTag));
		} else {
			Q_strncpyz(cgs.blueTeamClanTag, "Blue Team", sizeof(cgs.blueTeamClanTag));
		}

		if (cgs.realProtocol < 91  && (*CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF))) {
			Q_strncpyz(cgs.blueTeamName, CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME), sizeof(cgs.blueTeamName));
		} else {
			Q_strncpyz(cgs.blueTeamName, "Blue Team", sizeof(cgs.blueTeamName));
		}
		trap_Cvar_Set("g_blueTeam", cgs.blueTeamName);
	}

	cgs.sv_fps = atoi(Info_ValueForKey(info, "sv_fps"));

	val = atoi(Info_ValueForKey(info, "sv_skillrating"));
	trap_Cvar_Set("sv_skillrating", va("%d", val));

	//FIXME breaks with seeking
	if (
		(cgs.protocol == PROTOCOL_QL  &&  cgs.realProtocol <= 90)  &&
		(cgs.lastConnectedDisconnectedPlayer > -1  &&  cgs.lastConnectedDisconnectedPlayerName[0]  &&  cgs.lastConnectedDisconnectedPlayerClientInfo  &&  cgs.needToCheckSkillRating)
		) {
		for (i = 0, numClients = 0;  i < MAX_CLIENTS;  i++) {
			if (cgs.clientinfo[i].infoValid) {
				numClients++;
			}
		}
		if (!cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].infoValid) {
			int skillRating;

			skillRating = abs((val * numClients - cgs.skillRating * (numClients + 1)));
			cgs.lastConnectedDisconnectedPlayerClientInfo->skillRating = skillRating;
			cgs.lastConnectedDisconnectedPlayerClientInfo->knowSkillRating = qtrue;
			//Com_Printf("^5last disconnected player had skill rating %d\n", abs((val * numClients - cgs.skillRating * (numClients + 1))));
			CG_Printf("%s ^5had skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, abs((val * numClients - cgs.skillRating * (numClients + 1))));
			if (cg_printSkillRating.integer) {
				CG_PrintToScreen("%s ^5had skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, abs((val * numClients - cgs.skillRating * (numClients + 1))));
			}
			cgs.needToCheckSkillRating = qfalse;
		} else {
			//int skillRating;

			//skillRating = abs(val * numClients - cgs.skillRating * (numClients - 1));
			//Com_Printf("^5last connected player has skill rating %d\n", abs(val * numClients - cgs.skillRating * (numClients - 1)));
			CG_Printf("%s ^5has skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, abs(val * numClients - cgs.skillRating * (numClients - 1)));
			if (cg_printSkillRating.integer) {
				CG_PrintToScreen("%s ^5has skill rating %d\n", cgs.lastConnectedDisconnectedPlayerName, abs(val * numClients - cgs.skillRating * (numClients - 1)));
			}
			cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].skillRating = abs(val * numClients - cgs.skillRating * (numClients - 1));
			cgs.clientinfo[cgs.lastConnectedDisconnectedPlayer].knowSkillRating = qtrue;
			cgs.needToCheckSkillRating = qfalse;
			CG_BuildSpectatorString();
		}
	} else {
	}

	cgs.skillRating = val;
	//Com_Printf("^3sv_skillRating %d\n", val);
	//FIXME in CG_SetConfigValues
	CG_ParseVersion(info);

	if (CG_CheckQlVersion(0, 1, 0, 495)) {
		const char *val;

		info = CG_ConfigString(CS_ARMOR_TIERED);
		val = Info_ValueForKey(info, "armor_tiered");
		if (*val) {
			cgs.armorTiered = atoi(val);
		} else {
			cgs.armorTiered = qfalse;
		}
		trap_Cvar_Set("cg_armorTiered", val);
	}


	if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
		info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS2);
	} else {
		info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS);
	}

	if (*info) {
		const char *model;
		const char *headModel;

		cgs.serverHaveCustomModelString = qtrue;
		cgs.serverAllowCustomHead = atoi(Info_ValueForKey(info, "g_allowCustomHeadModels"));
		cgs.serverModelScale = atof(Info_ValueForKey(info, "g_playerModelScale"));
		cgs.serverHeadModelScale = atof(Info_ValueForKey(info, "g_playerheadScale"));
		if (*Info_ValueForKey(info, "g_playerheadScaleOffset")) {
			cgs.serverHaveHeadScaleOffset = qtrue;
			cgs.serverHeadModelScaleOffset = atof(Info_ValueForKey(info, "g_playerheadScaleOffset"));
		} else {
			cgs.serverHaveHeadScaleOffset = qfalse;
		}

		model = Info_ValueForKey(info, "g_playerModelOverride");
		headModel = Info_ValueForKey(info, "g_playerheadmodelOverride");

		if (Q_stricmp(model, cgs.serverModelOverride)  ||  Q_stricmp(headModel, cgs.serverHeadModelOverride)) {
			Q_strncpyz(cgs.serverModelOverride, Info_ValueForKey(info, "g_playerModelOverride"), sizeof(cgs.serverModelOverride));
			Q_strncpyz(cgs.serverHeadModelOverride, Info_ValueForKey(info, "g_playerheadmodelOverride"), sizeof(cgs.serverHeadModelOverride));
			CG_LoadServerModelOverride();
		}
	} else {
		cgs.serverHaveCustomModelString = qfalse;
		cgs.serverAllowCustomHead = qfalse;
		cgs.serverModelScale = 1.0;
		cgs.serverHeadModelScale = 1.0;
		cgs.serverHeadModelScaleOffset = 1.0;
		cgs.serverHaveHeadScaleOffset = qfalse;
		cgs.serverModelOverride[0] = '\0';
		cgs.serverHeadModelOverride[0] = '\0';
	}

	//FIXME check for first call
	cgs.timeoutBeginTime = atoi(CG_ConfigString(CS_TIMEOUT_BEGIN_TIME));
	cgs.timeoutEndTime = atoi(CG_ConfigString(CS_TIMEOUT_END_TIME));
	cgs.redTeamTimeoutsLeft = atoi(CG_ConfigString(CS_RED_TEAM_TIMEOUTS_LEFT));
	cgs.blueTeamTimeoutsLeft = atoi(CG_ConfigString(CS_BLUE_TEAM_TIMEOUTS_LEFT));

	if (firstCall) {
		cgs.customServerSettings = atoi(CG_ConfigString(CS_CUSTOM_SERVER_SETTINGS));
	}
}

/*
==================
CG_ParseWarmup
==================
*/
void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;
	int i;

	if (cgs.cpma) {
		info = CG_ConfigStringNoConvert(CSCPMA_GAMESTATE);
		warmup = atoi(Info_ValueForKey(info, "tw"));
	} else if (cgs.protocol == PROTOCOL_QL) {
		info = CG_ConfigString( CS_WARMUP );
		warmup = atoi(Info_ValueForKey(info, "time"));
	} else {
		info = CG_ConfigString(CSQ3_WARMUP);
		warmup = atoi(info);
	}

	cg.warmupCount = -1;
	//Com_Printf("^1sssssssssssssssssssssss  %d\n", warmup);

	if ( warmup == 0 && cg.warmup ) {
		Com_Printf("reset match start\n");
		CG_ResetTimedItemPickupTimes();
        cg.itemPickupTime = 0;
        cg.itemPickupClockTime = 0;
		cg.vibrateCameraTime = 0;
		cg.vibrateCameraValue = 0;
		cg.vibrateCameraPhase = 0;

		cg.crosshairClientTime = -(cg_drawCrosshairNamesTime.integer + 2000);  //FIXME 2000, 1000 fixed number
		cg.crosshairClientNum = CROSSHAIR_CLIENT_INVALID;
		cg.powerupTime = 0;
		cg.attackerTime = 0;
		cg.voiceTime = 0;
		cg.rewardTime = 0;
		cg.soundTime = 0;
#ifdef MISSIONPACK
		cg.voiceChatTime = 0;
#endif
		cg.weaponSelectTime = 0;
		cg.weaponAnimationTime = 0;
		cg.damageTime = 0;
		cg.lastChatBeepTime = 0;
		cg.lastFragTime = 0;
		memset(&cg.obituaries, 0, sizeof(cg.obituaries));
		cg.obituaryIndex = 0;

		cg.lastTeamChatBeepTime = 0;
		cg.damageDoneTime = 0;

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			wclients[i].killCount = 0;
		}
	}

	//Com_Printf("parsewarmup %d  %d   %s\n", warmup, cg.warmup, info);
	cg.warmup = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
	const char *s;

	if (cgs.protocol == PROTOCOL_QL) {
		cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
		cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	} else if (!cgs.cpma) {
		cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
		cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	}

	if(cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
#if 1  //def MPACK
	else if( cgs.gametype == GT_1FCTF ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus = s[0] - '0';
	}
#endif

	if (cgs.protocol == PROTOCOL_QL) {
		if (cgs.realProtocol < 91) {
			cgs.dominationRedPoints = atoi(CG_ConfigStringNoConvert(CS_DOMINATION_RED_POINTS));
			cgs.dominationBluePoints = atoi(CG_ConfigStringNoConvert(CS_DOMINATION_BLUE_POINTS));
		} else {
			cgs.dominationRedPoints = atoi(CG_ConfigStringNoConvert(CS91_GENERIC_COUNT_RED));
			cgs.dominationBluePoints = atoi(CG_ConfigStringNoConvert(CS91_GENERIC_COUNT_BLUE));
		}
	} else {
		cgs.dominationRedPoints = 0;
		cgs.dominationBluePoints = 0;
	}

	//FIXME GT_1FCTF see cg_servercmds.c
	if (cgs.protocol == PROTOCOL_QL) {
		cgs.redPlayersLeft = atoi(CG_ConfigString(CS_RED_PLAYERS_LEFT));
		cgs.bluePlayersLeft = atoi(CG_ConfigString(CS_BLUE_PLAYERS_LEFT));
	} else {
		cgs.redPlayersLeft = 0;
		cgs.bluePlayersLeft = 0;
	}

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	//////////////////////////////////////////////////
	// from here on only PROTOCOL_QL

	cgs.numberOfRaceCheckPoints = atoi(CG_ConfigString(CS_NUMBER_OF_RACE_CHECKPOINTS));
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void) {
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString( CS_SHADERSTATE );
	while (o && *o) {
		n = strstr(o, "=");
		if (n && *n) {
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t) {
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o) {
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				//CG_Printf("@ RemapShader: %s   %s\n", originalShader, newShader);
				trap_R_RemapShader(originalShader, newShader, timeOffset, qfalse, qfalse);
			}
		} else {
			break;
		}
	}
}

void CG_PlayWinLossMusic (void)
{
	int rank;

	if (cg_winLossMusic.integer == 0) {
		return;
	}

	//FIXME check cpma and other q3 protocols
	if (cgs.cpma) {
		return;
	}

	rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
	//Com_Printf("rank: %d\n", rank);

	if (rank == 0) {
		trap_S_StartBackgroundTrack("music/win.ogg", "music/win.ogg");
	} else {
		trap_S_StartBackgroundTrack("music/loss.ogg", "music/loss.ogg");
	}
}

void CG_InterMissionHit (void)
{
	if (cg_buzzerSound.integer) {
		CG_StartLocalSound(cgs.media.buzzer, CHAN_LOCAL_SOUND);
	}

	CG_PlayWinLossMusic();


#if 0
	if (cgs.cpma) {
		CG_CpmaParseScores(qfalse);
		Com_Printf("red %d   blue %d\n", cgs.scores1, cgs.scores2);
		trap_SendConsoleCommand("configstrings\n");
	}
#endif

#if 0
	if (cg_audioAnnouncerWin.integer) {
		if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
			//if (cgs.scores1 == cgs.roundlimit) {
			if (cgs.scores1 > cgs.scores2) {
				CG_StartLocalSound(cgs.media.redWinsSound, CHAN_ANNOUNCER);
			} else {
				CG_StartLocalSound(cgs.media.blueWinsSound, CHAN_ANNOUNCER);
			}
		} else if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			if (cgs.scores1 > cgs.scores2) {
				CG_StartLocalSound(cgs.media.redWinsSound, CHAN_ANNOUNCER);
			} else {
				CG_StartLocalSound(cgs.media.blueWinsSound, CHAN_ANNOUNCER);
			}
		}
	}
#endif

	//Com_Printf("intermission hit\n");
}

static qboolean CG_IsModelConfigString (int num, int *modelNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_MODELS  &&  num < CSQ3_MODELS + MAX_MODELS) {
			*modelNum = num - CSQ3_MODELS;
			return qtrue;
		}
	} else {
		if (num >= CS_MODELS  &&  num < CS_SOUNDS) {
			*modelNum = num - CS_MODELS;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_IsSoundConfigString (int num, int *soundNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_SOUNDS  &&  num < CSQ3_SOUNDS + MAX_SOUNDS) {
			*soundNum = num - CSQ3_SOUNDS;
			return qtrue;
		}
	} else {
		if (num >= CS_MODELS  &&  num < CS_PLAYERS) {
			*soundNum = num - CS_SOUNDS;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_IsPlayerConfigString (int num, int *clientNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_PLAYERS  &&  num < CSQ3_PLAYERS + MAX_CLIENTS) {
			*clientNum = num - CSQ3_PLAYERS;
			return qtrue;
		}
	} else {
		if (num >= CS_PLAYERS  &&  num < CS_LOCATIONS) {
			*clientNum = num - CS_PLAYERS;
			return qtrue;
		}
	}

	return qfalse;
}

#if 0
static qboolean CG_IsLocationConfigString (int num, int *locationNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_LOCATIONS  &&  num < CSQ3_LOCATIONS + MAX_LOCATIONS) {
			*locationNum = num - CSQ3_LOCATIONS;
			return qtrue;
		}
	} else {
		if (num >= CS_LOCATIONS  &&  num < CS_PARTICLES) {
			*locationNum = num - CS_LOCATIONS;
			return qtrue;
		}
	}

	return qfalse;
}
#endif

static qboolean CG_IsTeamVoteTime (int num, int *newNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_TEAMVOTE_TIME  &&  num <= CSQ3_TEAMVOTE_TIME + 1) {
			*newNum = num - CSQ3_TEAMVOTE_TIME;
			return qtrue;
		}
	} else {
		if (num >= CS_TEAMVOTE_TIME  &&  num <= CS_TEAMVOTE_TIME + 1) {
			*newNum = num - CS_TEAMVOTE_TIME;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_IsTeamVoteYes (int num, int *newNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_TEAMVOTE_YES  &&  num <= CSQ3_TEAMVOTE_YES + 1) {
			*newNum = num - CSQ3_TEAMVOTE_YES;
			return qtrue;
		}
	} else {
		if (num >= CS_TEAMVOTE_YES  &&  num <= CS_TEAMVOTE_YES + 1) {
			*newNum = num - CS_TEAMVOTE_YES;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_IsTeamVoteNo (int num, int *newNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_TEAMVOTE_NO  &&  num <= CSQ3_TEAMVOTE_NO + 1) {
			*newNum = num - CSQ3_TEAMVOTE_NO;
			return qtrue;
		}
	} else {
		if (num >= CS_TEAMVOTE_NO  &&  num <= CS_TEAMVOTE_NO + 1) {
			*newNum = num - CS_TEAMVOTE_NO;
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_IsTeamVoteString (int num, int *newNum)
{
	if (cgs.protocol == PROTOCOL_Q3) {
		if (num >= CSQ3_TEAMVOTE_STRING  &&  num <= CSQ3_TEAMVOTE_STRING + 1) {
			*newNum = num - CSQ3_TEAMVOTE_STRING;
			return qtrue;
		}
	} else {
		if (num >= CS_TEAMVOTE_STRING  &&  num <= CS_TEAMVOTE_STRING + 1) {
			*newNum = num - CS_TEAMVOTE_STRING;
			return qtrue;
		}
	}

	return qfalse;
}


void CG_CpmaParseScores (qboolean seeking)
{
	const char *str;
	int i;

	str = CG_ConfigStringNoConvert(CSCPMA_SCORES);

	//CG_Printf("^5scores(%d): %s\n", seeking, str);

	if (CG_IsTeamGame(cgs.gametype)) {
		int roundTime;
		int lastRoundTime;
		cgs.scores1 = atoi(Info_ValueForKey(str, "sr"));
		cgs.scores2 = atoi(Info_ValueForKey(str, "sb"));
		cgs.bluePlayersLeft = atoi(Info_ValueForKey(str, "pb"));
		cgs.redPlayersLeft = atoi(Info_ValueForKey(str, "pr"));
		// tw round time

		// -1 before match starts
		// > 0 indicates when round begins
		// 0 when round starts  -- 2016-08-06 not always (match start) so this can't be used as the trigger for round starting.  Start of first round is triggered when gamestate warmup has ended
		//FIXME this isn't used for ctfs round status it is always based on game status changes
		lastRoundTime = cgs.roundBeginTime;
		roundTime = atoi(Info_ValueForKey(str, "tw"));
		//CG_Printf("^5%d round time(%d): %d\n", cg.time, seeking, roundTime);

		cgs.roundBeginTime = roundTime;
		if (roundTime > 0) {
			cgs.roundStarted = qfalse;
			if (lastRoundTime == 0) {
				if (!seeking) {
					CG_RoundEnd();
				}
			}
		} else if (roundTime == 0  &&  lastRoundTime > 0) {
			// check for lastRoundTime since first round start is triggered when warmup changes, not based on roundTime.  Other rounds are based on roundTime
			cgs.roundStarted = qtrue;
			cgs.countDownSoundPlayed = 1;  // trigger "FIGHT!" screen message
			if (!seeking) {
				CG_RoundStart();
			}
		}
	} else {  // duel or ffa
		cgs.scores1 = atoi(Info_ValueForKey(str, "sr"));
		cgs.scores2 = atoi(Info_ValueForKey(str, "sb"));
		if (cgs.scores1 < cgs.scores2) {
			i = cgs.scores1;
			cgs.scores1 = cgs.scores2;
			cgs.scores2 = i;
		}
	}
	//Com_Printf("^5FIXME red %d  blue %d\n", cgs.scores1, cgs.scores2);
}

void CG_CpmaParseGameState (qboolean initial)
{
	const char *str;
	int x, td, te, ts;

	//FIXME plays buzzer sound to mark intermission?

	str = CG_ConfigStringNoConvert(CSCPMA_GAMESTATE);

	//Com_Printf("%d ^2%s\n", cg.snap ? cg.snap->serverTime : 0, str);
	//CG_Printf("^2cs game state(%d): %s\n", initial, str);

	/*

first timeout at 1:04 or something:

1:14 Wait another 0.0s to issue timeout
1:15 10.rat ate 10.rat's rocket
1:15 10.rat blew himself up.
165994 sb\2\sr\5\pm\2\t\3\f\139395221\i\2147451900\tw\0\ts\89302\td\0\te\76692\to\5\tl\20\sl\0\h\100\a\0\wr\30\pu\90\vt\0\vs\\vf\0\va\0\p\4\cr\2\cb\2\nr\\nb\\
1:16 lastte 76692
196420 sb\2\sr\5\pm\2\t\3\f\139395221\i\2147451900\tw\0\ts\89302\td\30426\te\76692\to\5\tl\20\sl\0\h\100\a\0\wr\30\pu\90\vt\0\vs\\vf\0\va\0\p\4\cr\2\cb\2\nr\\nb\\
201403 sb\2\sr\5\pm\2\t\3\f\139395220\i\2147451900\tw\0\ts\89302\td\35409\te\0\to\5\tl\20\sl\0\h\100\a\0\wr\30\pu\90\vt\0\vs\\vf\0\va\0\p\4\cr\2\cb\2\nr\\nb\\
0:41 lastte 0

te + ts + td == serverTime

	*/

	/**********

		  sb\0\sr\0\pm\2\t\1\f\139395204\i\16745468\tw\63450\ts\0\td\0\te\0\to\2\tl\10\sl\0\h\100\a\0\wr\15\pu\90\vt\0\vs\\vf\0\va\0\p\2\cr\0\cb\0\nr\\nb\	\

		  pm\2\t\1\f\139395204\i\16745468\td\0\to\2\wr\15\pu\90\p\2\cr\0\cb\0\nr\\nb\\

		  // pm  promode?
		  // f flags?  changes with gameplay
		  // p changes with gameplay

		  sb  score blue
		  sr  score red
		  tw  warmup time
		  ts  level start time
		  td  time delay ?
		  tl  time limit
		  sl  score limit
		  te  timeout end?
		  vt  vote time
		  vf  vote for
		  va  vote against
		  vs  vote string
		  h   starting health
          a   starting armor
		  wr  weapon respawn?
		  pu  power up respawn?
		  pb  max blue players?
		  pr  max red players?

		  t   gametype

		**********/

		// older cpma demos don't set g_gametype in CS_SERVERINFO, always get it from here
		x = atoi(Info_ValueForKey(str, "t"));

		switch (x) {
		case 0:
			cgs.gametype = GT_FFA;
			break;
		case 1:
			cgs.gametype = GT_TOURNAMENT;
			break;
		case 2:
			//FIME this possible with cpma?
			cgs.gametype = GT_SINGLE_PLAYER;
			break;
		case 3:
			cgs.gametype = GT_TEAM;
			break;
		case 4:
			cgs.gametype = GT_CTF;
			break;
		case 5:
			cgs.gametype = GT_CA;
			break;
		case 6:
			cgs.gametype = GT_FREEZETAG;
			break;
		case 7:
			cgs.gametype = GT_CTFS;
			break;
		case 8:
			cgs.gametype = GT_NTF;
			break;
		case 9:
			cgs.gametype = GT_2V2;
			break;
		case -1:
			cgs.gametype = GT_HM;
			break;
		default:
			Com_Printf("^3unknown cpma game type: %d\n", x);
			cgs.gametype = GT_FFA;
			break;
		}


		cgs.fraglimit = atoi(Info_ValueForKey(str, "sl"));  // score limit
		//cgs.capturelimit = cgs.fraglimit;
		cgs.scorelimit = cgs.fraglimit;
		cgs.roundlimit = cgs.fraglimit;
		cgs.timelimit = atoi(Info_ValueForKey(str, "tl"));
		cgs.realTimelimit = cgs.timelimit;
		ts = atoi(Info_ValueForKey(str, "ts"));
		cgs.levelStartTime = ts;
		td = atoi(Info_ValueForKey(str, "td"));
		cgs.levelStartTime += td;

		x = atoi(Info_ValueForKey(str, "tw"));
		if (!x  &&  cg.warmup) {
			cg.warmup = x;  // to play sound
			if (!initial) {
				CG_MapRestart();  // hmmmmmm no? not sure  //FIXME seeking
				if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_CTFS) {
					CG_RoundStart();
				}
			}
		}
		cg.warmup = x;

		//Com_Printf("%f  level startTime %f\n", cg.ftime, (float) cgs.levelStartTime);

		cgs.redPlayersLeft = atoi(Info_ValueForKey(str, "pr"));
		cgs.bluePlayersLeft = atoi(Info_ValueForKey(str, "pb"));

		x = atoi(Info_ValueForKey(str, "vt"));
		if (x) {
			if (x != cgs.voteTime  &&  !initial) {
				if (cg_audioAnnouncerVote.integer) {
					CG_StartLocalSound(cgs.media.voteNow, CHAN_ANNOUNCER);
				}
			}
			cgs.voteModified = qtrue;
		}

		cgs.voteTime = x;
		cgs.voteYes = atoi(Info_ValueForKey(str, "vf"));
		cgs.voteNo = atoi(Info_ValueForKey(str, "va"));
		Q_strncpyz(cgs.voteString, Info_ValueForKey(str, "vs"), sizeof(cgs.voteString));

		te = atoi(Info_ValueForKey(str, "te"));

		if (te  &&  cgs.cpmaLastTe == 0) {  // timeout called
			cgs.timeoutEndTime = ts + td + te + (320 * 1000);
			cgs.timeoutBeginTime = ts + td + te;
			if (!initial) {
				CG_StartLocalSound(cgs.media.klaxon1, CHAN_LOCAL_SOUND);
				CG_ResetTimedItemPickupTimes();  //FIXME take out eventually
			}
		} else if (te  &&  td != cgs.cpmaLastTd) {  // timein called
			cgs.timeoutBeginTime = ts + td + te;
			// for cpma compat:
			if (!initial) {
				//CG_StartLocalSound(cgs.media.countPrepareSound, CHAN_LOCAL_SOUND);
			}
		} else if (!te  &&  cgs.cpmaLastTe) {  // timeout over
			cgs.timeoutEndTime = 0;
			cgs.timeoutBeginTime = 0;
			// for cpma compat:
			if (!initial) {
				//CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
			}
		}

		cgs.cpmaLastTe = te;
		cgs.cpmaLastTd = td;
}

static qboolean CG_CpmaCs (int num)
{
	//FIXME plays buzzer sound to mark intermission?

	if (num == CSCPMA_SCORES) {
		//CG_Printf("^3CG_CpmaCs\n");
		CG_CpmaParseScores(qfalse);
		// hack for overtime or end of buzzer scores that trigger
		// PM_INTERMISSION  but scores haven't updated
		//FIXME no, not here
		if (!cg.intermissionStarted  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
			trap_SendConsoleCommand("exec gameend.cfg\n");
			CG_InterMissionHit();
			cg.intermissionStarted = cg.time;
		}
	} else if (num == CSCPMA_GAMESTATE) {
		CG_CpmaParseGameState(qfalse);
	} else {
		const char *str;

		str = CG_ConfigStringNoConvert(num);
		CG_Printf("^3unknown cpma config string: \n'%s'\n", str);
		return qfalse;
	}

	return qtrue;
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void ) {
	const char	*str;
	int		num;
	int numOrig;
	char buf[MAX_STRING_CHARS];
	int i;
	//int teamNum;
	const char *info;
	qboolean newcs;
	int n;

	if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
		newcs = qtrue;
	} else {
		newcs = qfalse;
	}

	num = atoi( CG_Argv( 1 ) );
	numOrig = num;

	if (cg.configStringOverride[num]) {
		//Q_strncpyz(ov, CG_ConfigString(num), sizeof(ov));
	}
	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	for (i = 0;  i < MAX_CONFIGSTRINGS;  i++) {
		if (cg.configStringOverride[i]) {
			CG_ChangeConfigString(cg.configStringOurs[i], i);
		}
	}

	if (cg.configStringOverride[num]) {
		//Com_Printf("ignoring %d\n", num);
		return;
	}

	// look up the individual string that was modified
	str = CG_ConfigStringNoConvert(num);

	if (SC_Cvar_Get_Int("debug_configstring")) {
		CG_Printf("cgame: %d  %s\n", num, str);
	}

	Q_strncpyz(buf, str, sizeof(buf));

	if (cgs.protocol == PROTOCOL_Q3) {
		CG_ConfigStringIndexFromQ3(&num);
		if (SC_Cvar_Get_Int("debug_configstring")) {
			Com_Printf("%d  -->  %d\n", numOrig, num);
		}
	}

	/*
	if (cgs.realProtocol >= 91) {
		//FIXME bad hack for new protocol
		if (num >= 679)  {  // 679 == CS_MAP_CREATOR
			//num--;
		}
	}
	*/

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo(qfalse, qfalse);
	} else if (num == CS_SYSTEMINFO) {

	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1  &&  !cgs.cpma) {
		cgs.scores1 = atoi( str );
		if (cgs.protocol != PROTOCOL_QL) {
			cgs.firstPlace[0] = '\0';
			cgs.secondPlace[0] = '\0';
		}
	} else if ( num == CS_SCORES2  &&  !cgs.cpma) {
		cgs.scores2 = atoi( str );
		if (cgs.protocol != PROTOCOL_QL) {
			cgs.firstPlace[0] = '\0';
			cgs.secondPlace[0] = '\0';
		}
	} else if ( num == CS_LEVEL_START_TIME   &&  !cgs.cpma) {
		cgs.levelStartTime = atoi( str );
		//Com_Printf("^1level start time... %d\n", cgs.levelStartTime);
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
#if 1  //def MPACK
		if (*cgs.voteString  &&  cg_audioAnnouncerVote.integer) {
			CG_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
		}
#endif //MPACK
		//Com_Printf("vote string: %s\n", cgs.voteString);
	} else if (CG_IsTeamVoteTime(numOrig, &n)) {
		cgs.teamVoteTime[n] = atoi(str);
		cgs.teamVoteModified[n] = qtrue;
	} else if (CG_IsTeamVoteYes(numOrig, &n)) {
		cgs.teamVoteYes[n] = atoi(str);
		cgs.teamVoteModified[n] = qtrue;
	} else if (CG_IsTeamVoteNo(numOrig, &n)) {
		cgs.teamVoteNo[n] = atoi(str);
		cgs.teamVoteModified[n] = qtrue;
	} else if (CG_IsTeamVoteString(numOrig, &n)) {
		Q_strncpyz(cgs.teamVoteString[n], str, MAX_STRING_TOKENS);
#if 1 //def MPACK
		//FIXME wc and if you are not on the same team??
		if (*str  &&  cg_audioAnnouncerTeamVote.integer) {
			CG_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
		}
#endif
	} else if ( num == CS_INTERMISSION ) {
		if (cg.intermissionStarted == 0) {
			CG_InterMissionHit();
			trap_SendConsoleCommand("exec gameend.cfg\n");
		}

		//FIXME quakelive has qtrue for intermission
		if (cgs.protocol == PROTOCOL_QL) {
			if (*str) {
				cg.intermissionStarted = qtrue;
			} else {
				cg.intermissionStarted = qfalse;
			}
		} else {
			cg.intermissionStarted = atoi( str );
		}

#if 0
		if (cg.intermissionStarted) {
			CG_InterMissionHit();
		}

		Com_Printf("^1intermission %d\n", cg.intermissionStarted);
#endif
	} else if (CG_IsModelConfigString(numOrig, &n)) {
		//Com_Printf("model string\n");
		cgs.gameModels[n] = trap_R_RegisterModel( str );
#if 0
		if (cgs.gametype == GT_DOMINATION) {
			if (!Q_stricmp(str, "models/flag3/d_flag3.md3")) {
				cgs.dominationControlPointModel = n;
				Com_Printf("control point model %d\n", n);
			}
		}
#endif
	} else if (CG_IsSoundConfigString(numOrig, &n)) {
		if ( str[0] != '*' ) {	// player specific sounds don't register here
			//Com_Printf("^1FIXME ok wtf will the index value mean\n");
			//CG_Printf("server says to play sound %d (%d): %s\n", numOrig, n, str);
			//cgs.gameSounds[ num-CS_SOUNDS + 1] = trap_S_RegisterSound( str, qfalse );  //FIXME +1 check
			cgs.gameSounds[ n + 1] = trap_S_RegisterSound( str, qfalse );  //FIXME +1 check
		}
	} else if (CG_IsPlayerConfigString(numOrig, &n)) {
		//int start;

		//start = trap_Milliseconds();
		//Com_Printf("cs client %d\n", n);
		CG_NewClientInfo(n);
		//Com_Printf("^5client:  %d ms\n", trap_Milliseconds() - start);

		CG_BuildSpectatorString();
		//Com_Printf("build spec string\n");
	} else if ( num == CS_FLAGSTATUS ) {
		if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			//Com_Printf("CS_FLAGSTATUS: %s\n", str);
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
#if 1  //def MPACK
		else if( cgs.gametype == GT_1FCTF ) {
			cgs.flagStatus = str[0] - '0';
		}
#endif
	}
	else if ( num == CS_SHADERSTATE ) {
		CG_ShaderStateChanged();

	} else if (cgs.protocol == PROTOCOL_Q3) {
		if (cgs.cpma) {
			if (CG_CpmaCs(num)) {
				return;
			}
		}

		// the rest are quake live specific
		CG_Printf("%d unknown q3 config string modified  %d: %s\n", cg.time, num, str);
	} else if (num == CS_FIRSTPLACE) {
		Q_strncpyz(cgs.firstPlace, str, sizeof(cgs.firstPlace));
	} else if (num == CS_SECONDPLACE) {
		Q_strncpyz(cgs.secondPlace, str, sizeof(cgs.secondPlace));
	} else if (num == CS_RED_PLAYERS_LEFT) {
		cgs.redPlayersLeft = atoi(str);
	} else if (num == CS_BLUE_PLAYERS_LEFT) {
		cgs.bluePlayersLeft = atoi(str);
	} else if (num == CS_ROUND_STATUS  &&  (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER)) {
		//Com_Printf("^3%d %d CS_ROUND_STATUS  %s\n", cg.time, num, str);

		// Don't use CS_ROUND_STATUS to determine if a round is still active.
		// In Quake Live CS_ROUND_TIME is set to -1 when a round ends.  A few
		// seconds later CS_ROUND_STATUS will be updated with next round start
		// time.  When the round starts both CS_ROUND_STATUS and CS_ROUND_TIME
		// are updated.

		if (str[0] == '\0') {  // older ql demos dm_73
			cgs.roundStarted = qtrue;
			for (i = 0;  i < cg.numScores;  i++) {
				cg.scores[i].alive = qtrue;
			}
			for (i = 0;  i < MAX_CLIENTS;  i++) {
				memset(&wclients[i].perKillwstats, 0, sizeof(wclients[i].perKillwstats));
			}
			//Com_Printf("^2 ROUND STARTED...\n");
			cgs.thirtySecondWarningPlayed = qfalse;
		} else {
			// cgs.roundStarted = qfalse is set with CS_ROUND_TIME change
			cgs.roundBeginTime = atoi(Info_ValueForKey(str, "time"));
			cgs.roundNum = atoi(Info_ValueForKey(str, "round"));
			cgs.roundTurn = atoi(Info_ValueForKey(str, "turn"));
			//FIXME ctfs  'state' ?
		}
	} else if (num == CS_ROUND_TIME  &&  (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER)) {
		int val;

		val = atoi(str);
		//Com_Printf("^5%d CS_ROUND_TIME %d\n", cg.time, val);
		if (val > 0  &&  !cgs.roundStarted) {
			//CG_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );

			// hack, ql appears to have a bug where the fov values from the
			// previously viewed player are still being copied into playerstate
			if (cg_useDemoFov.integer == 2) {
				CG_ZoomUp_f();
			}
			cgs.roundStarted = qtrue;
			CG_RoundStart();
		} else if (val < 0) {
			//Com_Printf("ca time reset\n");
			cgs.roundStarted = qfalse;
			cgs.roundBeginTime = -999;
			cgs.countDownSoundPlayed = -999;
			CG_RoundEnd();
			//Com_Printf("round end\n");
		}
	} else if (num == CS_TIMEOUT_BEGIN_TIME) {
		cgs.timeoutBeginTime = atoi(str);
	} else if (num == CS_TIMEOUT_END_TIME) {
		cgs.timeoutEndTime = atoi(str);
	} else if (num == CS_RED_TEAM_TIMEOUTS_LEFT) {
		cgs.redTeamTimeoutsLeft = atoi(str);
	} else if (num == CS_BLUE_TEAM_TIMEOUTS_LEFT) {
		cgs.blueTeamTimeoutsLeft = atoi(str);
	} else if (num == CS91_NAME1STPLAYER  &&  cgs.realProtocol >= 91) {
	} else if (num == CS91_NAME2NDPLAYER  &&  cgs.realProtocol >= 91) {
	} else if (num == CS_FIRST_PLACE_CLIENT_NUM  &&  !newcs) {
	} else if (num == CS_FIRST_PLACE_CLIENT_NUM2  &&  newcs) {
	} else if (num == CS_SECOND_PLACE_CLIENT_NUM  &&  !newcs) {
	} else if (num == CS_SECOND_PLACE_CLIENT_NUM2  &&  newcs) {
	} else if (num == CS_FIRST_PLACE_SCORE  &&  !newcs) {
	} else if (num == CS_FIRST_PLACE_SCORE2  &&  newcs) {
	} else if (num == CS_SECOND_PLACE_SCORE  &&  !newcs) {
	} else if (num == CS_SECOND_PLACE_SCORE2  &&  newcs) {
	} else if (num == CS_MOST_DAMAGE_DEALT  &&  !newcs) {
	} else if (num == CS_MOST_ACCURATE  &&  !newcs) {
	} else if (num == CS_BEST_ITEM_CONTROL  &&  !newcs) {
	} else if (num == CS_MOST_DAMAGE_DEALT2  &&  newcs) {
	} else if (num == CS_MOST_ACCURATE2  &&  newcs) {
	} else if (num == CS_BEST_ITEM_CONTROL2  &&  newcs) {
	} else if (num == CS_MVP_OFFENSE  &&  cgs.realProtocol < 91) {
	} else if (num == CS_MVP_DEFENSE  &&  cgs.realProtocol < 91) {
	} else if (num == CS_MVP &&  cgs.realProtocol < 91) {
	} else if (cgs.realProtocol >= 91  &&  (num == CS91_MOST_ACCURATE_PLYR  ||  num == CS91_MOST_DAMAGEDEALT_PLYR  ||  num == CS91_BEST_ITEMCONTROL_PLYR  ||  num == CS91_MOST_VALUABLE_OFFENSIVE_PLYR  ||  num == CS91_MOST_VALUABLE_DEFENSIVE_PLYR  ||  num == CS91_MOST_VALUABLE_PLYR  ||  num == CS91_BEST_ITEMCONTROL_PLYR)) {
		// pass, handled in cg_newdraw.c
	} else if (num == CS_RED_TEAM_CLAN_NAME  &&  cgs.realProtocol < 91) {
		// 2015-08-13 qcon protocol 91 doesn't have clan or team names
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.redTeamName, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamName));
		}
		//CG_Printf("^5new red team name ^7'%s'\n", CG_ConfigString(CS_RED_TEAM_CLAN_NAME));
	} else if (num == CS_BLUE_TEAM_CLAN_NAME  && cgs.realProtocol < 91) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.blueTeamName, CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG), sizeof(cgs.blueTeamName));
		}
		//CG_Printf("^5new blue team name ^7'%s'\n", CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME));
	} else if (num == CS_RED_TEAM_CLAN_TAG  &&  cgs.realProtocol < 91) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.redTeamClanTag, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamClanTag));
		}
		//CG_Printf("^5new red clan tag ^7'%s'\n", CG_ConfigString(CS_RED_TEAM_CLAN_TAG));
	} else if (num == CS_BLUE_TEAM_CLAN_TAG  &&  cgs.realProtocol < 91) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.blueTeamClanTag, CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG), sizeof(cgs.blueTeamClanTag));
		}
		//CG_Printf("^5new blue clan tag ^7'%s'\n", CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG));
	//} else if (num == CS_PMOVE_SETTINGS) {

	} else if ((num == CS_CUSTOM_PLAYER_MODELS2  &&  newcs)  ||  (num == CS_CUSTOM_PLAYER_MODELS  &&  !newcs)) {
		const char *model;
		const char *headModel;

		if (newcs) {
			info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS2);
		} else {
			info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS);
		}

		if (*info) {
			cgs.serverHaveCustomModelString = qtrue;
		} else {
			cgs.serverHaveCustomModelString = qfalse;
		}

		cgs.serverAllowCustomHead = atoi(Info_ValueForKey(info, "g_allowCustomHeadModels"));
		cgs.serverModelScale = atof(Info_ValueForKey(info, "g_playerModelScale"));
		cgs.serverHeadModelScale = atof(Info_ValueForKey(info, "g_playerheadScale"));

		cgs.serverHeadModelScaleOffset = atof(Info_ValueForKey(info, "g_playerheadScaleOffset"));

		if (*Info_ValueForKey(info, "g_playerheadScaleOffset")) {
			cgs.serverHaveHeadScaleOffset = qtrue;
		} else {
			cgs.serverHaveHeadScaleOffset = qfalse;
		}

		model = Info_ValueForKey(info, "g_playerModelOverride");
		headModel = Info_ValueForKey(info, "g_playerheadmodelOverride");

		if (Q_stricmp(model, cgs.serverModelOverride)  ||  Q_stricmp(headModel, cgs.serverHeadModelOverride)) {
			Q_strncpyz(cgs.serverModelOverride, Info_ValueForKey(info, "g_playerModelOverride"), sizeof(cgs.serverModelOverride));
			Q_strncpyz(cgs.serverHeadModelOverride, Info_ValueForKey(info, "g_playerheadmodelOverride"), sizeof(cgs.serverHeadModelOverride));
			CG_LoadServerModelOverride();
		}

		//Com_Printf("^1server scale:  model %f  head %f  headoffset %f\n", cgs.serverModelScale, cgs.serverHeadModelScale, cgs.serverHeadModelScaleOffset);
	} else if ((num == CS_DOMINATION_RED_POINTS  &&  cgs.realProtocol < 91)  ||  (num == CS91_GENERIC_COUNT_RED  &&  cgs.realProtocol >= 91)) {
		cgs.dominationRedPoints = atoi(str);
	} else if ((num == CS_DOMINATION_BLUE_POINTS  &&  cgs.realProtocol < 91)  ||  (num == CS91_GENERIC_COUNT_BLUE  &&  cgs.realProtocol >= 91)) {
		cgs.dominationBluePoints = atoi(str);
	} else if (num == CS_ROUND_WINNERS) {
	} else if (num == CS_MAP_VOTE_INFO) {
	} else if (num == CS_MAP_VOTE_COUNT) {
	} else if (num == CS_DISABLE_MAP_VOTE) {
	} else if (num == CS_PMOVE_SETTINGS) {
	} else if (num == CS_WEAPON_SETTINGS2  &&  newcs) {
	} else if (num == CS_WEAPON_SETTINGS  &&  !newcs) {
	} else if (num == CS_CUSTOM_SERVER_SETTINGS) {
		cgs.customServerSettings = atoi(str);
	} else if (num == CS_ARMOR_TIERED  &&  newcs) {
		const char *val;

		val = Info_ValueForKey(str, "armor_tiered");
		if (*val) {
			cgs.armorTiered = atoi(val);
		} else {
			cgs.armorTiered = qfalse;
		}
		trap_Cvar_Set("cg_armorTiered", val);
	} else if (num == CS_NUMBER_OF_RACE_CHECKPOINTS) {
		cgs.numberOfRaceCheckPoints = atoi(str);
	} else {
		CG_Printf("%d unknown config string modified  %d: %s\n", cg.time, num, str);
	}
}


/*
=======================
CG_AddToTeamChat

=======================
*/
static void CG_AddToTeamChat( const char *str ) {
	int len;
	char *p;
    const char *ls;
	int lastcolor;
	int chatHeight;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0) {
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str) {
		if (len > TEAMCHAT_WIDTH - 1) {
			if (ls) {
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		//FIXME osp colors
		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ') {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if (cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	//int i;
	//int val;

	Com_Printf("cgame: map restart\n");

	if ( cg_showmiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles ();

	CG_ResetTimedItemPickupTimes();
	cg.itemPickupTime = 0;
	cg.itemPickupClockTime = 0;

	cg.vibrateCameraTime = 0;
	cg.vibrateCameraValue = 0;
	cg.vibrateCameraPhase = 0;

	cg.crosshairClientTime = -(cg_drawCrosshairNamesTime.integer + 2000);  //FIXME 2000, 1000 fixed number
	cg.powerupTime = 0;
	cg.attackerTime = 0;
	cg.voiceTime = 0;
	cg.rewardTime = 0;
	cg.rewardStack = 0;
	cg.soundTime = 0;
#ifdef MISSIONPACK
	cg.voiceChatTime = 0;
#endif
	cg.weaponSelectTime = 0;
	cg.weaponAnimationTime = 0;
	cg.damageTime = 0;
	cg.lastChatBeepTime = 0;
	cg.lastFragTime = 0;
	memset(&cg.obituaries, 0, sizeof(cg.obituaries));
	cg.obituaryIndex = 0;

	cg.lastTeamChatBeepTime = 0;
	cg.damageDoneTime = 0;

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;

	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;

	cgs.voteTime = 0;

	cg.mapRestart = qtrue;

	CG_StartMusic();

	trap_S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	CG_ResetTimedItemPickupTimes();

	//Com_Printf("map restart: %d\n", cg.time);

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmup == 0   &&  cgs.gametype != GT_FREEZETAG /* && CG_IsDuelGame(cgs.gametype) */) {
		if (cg_audioAnnouncerWarmup.integer) {
			if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_allowServerOverride.integer) {
				int ourClientNum;

				if (wolfcam_following) {
					ourClientNum = wcg.clientNum;
				} else {
					ourClientNum = cg.snap->ps.clientNum;
				}
				if (cgs.clientinfo[ourClientNum].team == TEAM_RED) {
					CG_StartLocalSound(cgs.media.countBiteSound, CHAN_ANNOUNCER);
				} else {
					CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
				}
			} else if (cgs.gametype == GT_RACE) {
				CG_StartLocalSound(cgs.media.countGoSound, CHAN_ANNOUNCER);
			} else {
				//CG_Printf("^2FIGHT sound servercmds\n");
				CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
			}
		}

		if (cgs.cpma) {
			int duelPlayer;
			int i;

			cgs.roundBeginTime = 0;
			cgs.roundStarted = qtrue;
			cgs.countDownSoundPlayed = 0;

			if (CG_IsDuelGame(cgs.gametype)) {
				duelPlayer = 1;
				for (i = 0;  i < MAX_CLIENTS;  i++) {
					if (!cgs.clientinfo[i].infoValid) {
						continue;
					}
					if (cgs.clientinfo[i].team == TEAM_FREE) {
						if (duelPlayer == 1) {
							Q_strncpyz(cgs.firstPlace, cgs.clientinfo[i].name, sizeof(cgs.firstPlace));
							//Com_Printf("map restart duel first: %s\n", cgs.firstPlace);
							duelPlayer++;
						} else if (duelPlayer == 2) {
							Q_strncpyz(cgs.secondPlace, cgs.clientinfo[i].name, sizeof(cgs.secondPlace));
							//Com_Printf("map restart duel second: %s\n", cgs.secondPlace);
							duelPlayer++;
						}
					}
				}
			} else {
				cgs.firstPlace[0] = '\0';
				cgs.secondPlace[0] = '\0';
			}
		}

		if (cg_drawFightMessage.integer) {
			if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_allowServerOverride.integer) {
				int ourClientNum;

				if (wolfcam_following) {
					ourClientNum = wcg.clientNum;
				} else {
					ourClientNum = cg.snap->ps.clientNum;
				}
				if (cgs.clientinfo[ourClientNum].team == TEAM_RED) {
					CG_CenterPrint("BITE!", 120, BIGCHAR_WIDTH);
				} else {
					CG_CenterPrint("FIGHT!", 120, BIGCHAR_WIDTH);
				}
			} else if (cgs.gametype == GT_RACE) {
				CG_CenterPrint("GO!", 120, BIGCHAR_WIDTH);
			} else if (cgs.gametype == GT_CTFS) {
				//FIXME not here
#if 0
				int ourTeam;

				if (wolfcam_following) {
					ourTeam = cgs.clientinfo[wcg.clientNum].team;
				} else {
					ourTeam = cgs.clientinfo[cg.snap->ps.clientNum].team;
				}
				if ((ourTeam == TEAM_RED  ||  ourTeam == TEAM_BLUE)  &&  cg_attackDefendVoiceStyle.integer == 1) {
					if ((cgs.roundTurn % 2 == 0  &&  ourTeam == TEAM_RED)  ||  (cgs.roundTurn % 2 != 0  &&  ourTeam == TEAM_BLUE)) {
						CG_CenterPrint("ATTACK THE FLAG!", 120, BIGCHAR_WIDTH);
					} else {
						CG_CenterPrint("DEFEND THE FLAG!", 120, BIGCHAR_WIDTH);
					}
				} else {
					CG_CenterPrint("FIGHT!", 120, BIGCHAR_WIDTH);
				}
#endif
			} else {
				//CG_Printf("^6FIGHT servercmds\n");
				CG_CenterPrint("FIGHT!", 120, BIGCHAR_WIDTH);
			}
		} else {  // no fight screen message
			//FIXME is this needed?
			CG_CenterPrint("", 120, BIGCHAR_WIDTH);
		}
	}
#ifdef MISSIONPACK
	if (cg_singlePlayerActive.integer) {
		trap_Cvar_Set("ui_matchStartTime", va("%i", cg.time));
		if (cg_recordSPDemo.integer && *cg_recordSPDemoName.string) {
			trap_SendConsoleCommand(va("set g_synchronousclients 1 ; record %s \n", cg_recordSPDemoName.string));
		}
	}
#endif
	//trap_Cvar_Set("cg_thirdPerson", "0");  //FIXME wolfcam
	if (cg.snap) {
		cg.matchRestartServerTime = cg.snap->serverTime;
	} else {
		cg.matchRestartServerTime = 0;
	}
	CG_ParseWarmup();
}

#ifdef MISSIONPACK

#define MAX_VOICEFILESIZE	16384
#define MAX_VOICEFILES		8
#define MAX_VOICECHATS		64
#define MAX_VOICESOUNDS		64
#define MAX_CHATSIZE		64
#define MAX_HEADMODELS		64

typedef struct voiceChat_s
{
	char id[64];
	int numSounds;
	sfxHandle_t sounds[MAX_VOICESOUNDS];
	char chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
} voiceChat_t;

typedef struct voiceChatList_s
{
	char name[64];
	int gender;
	int numVoiceChats;
	voiceChat_t voiceChats[MAX_VOICECHATS];
} voiceChatList_t;

typedef struct headModelVoiceChat_s
{
	char headmodel[64];
	int voiceChatNum;
} headModelVoiceChat_t;

voiceChatList_t voiceChatLists[MAX_VOICEFILES];
headModelVoiceChat_t headModelVoiceChat[MAX_HEADMODELS];

/*
=================
CG_ParseVoiceChats
=================
*/
static int CG_ParseVoiceChats( const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats ) {
	int	len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;
	voiceChat_t *voiceChats;
	qboolean compress;
	sfxHandle_t sound;

	compress = qtrue;
	if (cg_buildScript.integer) {
		compress = qfalse;
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "voice chat file not found: %s\n", filename ) );
		return qfalse;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap_Print( va( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i\n", filename, len, MAX_VOICEFILESIZE ) );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ptr = buf;
	p = &ptr;

	Com_sprintf(voiceChatList->name, sizeof(voiceChatList->name), "%s", filename);
	voiceChats = voiceChatList->voiceChats;
	for ( i = 0; i < maxVoiceChats; i++ ) {
		voiceChats[i].id[0] = 0;
	}
	token = COM_ParseExt(p, qtrue);
	if (!token[0]) {
		return qtrue;
	}
	if (!Q_stricmp(token, "female")) {
		voiceChatList->gender = GENDER_FEMALE;
	}
	else if (!Q_stricmp(token, "male")) {
		voiceChatList->gender = GENDER_MALE;
	}
	else if (!Q_stricmp(token, "neuter")) {
		voiceChatList->gender = GENDER_NEUTER;
	}
	else {
		trap_Print( va( S_COLOR_RED "expected gender not found in voice chat file: %s\n", filename ) );
		return qfalse;
	}

	voiceChatList->numVoiceChats = 0;
	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);
		if (!token[0]) {
			return qtrue;
		}
		Com_sprintf(voiceChats[voiceChatList->numVoiceChats].id, sizeof( voiceChats[voiceChatList->numVoiceChats].id ), "%s", token);
		token = COM_ParseExt(p, qtrue);
		if (Q_stricmp(token, "{")) {
			trap_Print( va( S_COLOR_RED "expected { found %s in voice chat file: %s\n", token, filename ) );
			return qfalse;
		}
		voiceChats[voiceChatList->numVoiceChats].numSounds = 0;
		while(1) {
			token = COM_ParseExt(p, qtrue);
			if (!token[0]) {
				return qtrue;
			}
			if (!Q_stricmp(token, "}"))
				break;
			sound = trap_S_RegisterSound( token, compress );
			voiceChats[voiceChatList->numVoiceChats].sounds[voiceChats[voiceChatList->numVoiceChats].numSounds] = sound;
			token = COM_ParseExt(p, qtrue);
			if (!token[0]) {
				return qtrue;
			}
			Com_sprintf(voiceChats[voiceChatList->numVoiceChats].chats[
							voiceChats[voiceChatList->numVoiceChats].numSounds], MAX_CHATSIZE, "%s", token);
			if (sound)
				voiceChats[voiceChatList->numVoiceChats].numSounds++;
			if (voiceChats[voiceChatList->numVoiceChats].numSounds >= MAX_VOICESOUNDS)
				break;
		}
		voiceChatList->numVoiceChats++;
		if (voiceChatList->numVoiceChats >= maxVoiceChats)
			return qtrue;
	}
	return qtrue;
}

/*
=================
CG_LoadVoiceChats
=================
*/
void CG_LoadVoiceChats( void ) {
	int size;

	size = trap_MemoryRemaining();
	CG_ParseVoiceChats( "scripts/female1.voice", &voiceChatLists[0], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/female2.voice", &voiceChatLists[1], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/female3.voice", &voiceChatLists[2], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male1.voice", &voiceChatLists[3], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male2.voice", &voiceChatLists[4], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male3.voice", &voiceChatLists[5], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male4.voice", &voiceChatLists[6], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male5.voice", &voiceChatLists[7], MAX_VOICECHATS );
	CG_Printf("voice chat memory size = %d\n", size - trap_MemoryRemaining());
}

/*
=================
CG_HeadModelVoiceChats
=================
*/
static int CG_HeadModelVoiceChats( const char *filename ) {
	int	len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	const char *token;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		//trap_Print( va( "voice chat file not found: %s\n", filename ) );
		return -1;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap_Print( va( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i\n", filename, len, MAX_VOICEFILESIZE ) );
		trap_FS_FCloseFile( f );
		return -1;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ptr = buf;
	p = &ptr;

	token = COM_ParseExt(p, qtrue);
	if ( !token[0] ) {
		return -1;
	}

	for ( i = 0; i < MAX_VOICEFILES; i++ ) {
		if ( !Q_stricmp(token, voiceChatLists[i].name) ) {
			return i;
		}
	}

	//FIXME: maybe try to load the .voice file which name is stored in token?

	return -1;
}


/*
=================
CG_GetVoiceChat
=================
*/
static int CG_GetVoiceChat( const voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, char **chat) {
	int i, rnd;

	for ( i = 0; i < voiceChatList->numVoiceChats; i++ ) {
		if ( !Q_stricmp( id, voiceChatList->voiceChats[i].id ) ) {
			rnd = random() * voiceChatList->voiceChats[i].numSounds;
			*snd = voiceChatList->voiceChats[i].sounds[rnd];
			*chat = (char *)voiceChatList->voiceChats[i].chats[rnd];
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
CG_VoiceChatListForClient
=================
*/
static voiceChatList_t *CG_VoiceChatListForClient( int clientNum ) {
	clientInfo_t *ci;
	int voiceChatNum, i, j, k, gender;
	char filename[MAX_QPATH], headModelName[MAX_QPATH];

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Printf("^1CG_VoiceChatListForClient() invalid client number %d\n", clientNum);
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( k = 0; k < 2; k++ ) {
		if ( k == 0 ) {
			if (ci->headModelName[0] == '*') {
				Com_sprintf( headModelName, sizeof(headModelName), "%s/%s", ci->headModelName+1, ci->headSkinName );
			}
			else {
				Com_sprintf( headModelName, sizeof(headModelName), "%s/%s", ci->headModelName, ci->headSkinName );
			}
		}
		else {
			if (ci->headModelName[0] == '*') {
				Com_sprintf( headModelName, sizeof(headModelName), "%s", ci->headModelName+1 );
			}
			else {
				Com_sprintf( headModelName, sizeof(headModelName), "%s", ci->headModelName );
			}
		}
		// find the voice file for the head model the client uses
		for ( i = 0; i < MAX_HEADMODELS; i++ ) {
			if (!Q_stricmp(headModelVoiceChat[i].headmodel, headModelName)) {
				break;
			}
		}
		if (i < MAX_HEADMODELS) {
			return &voiceChatLists[headModelVoiceChat[i].voiceChatNum];
		}
		// find a <headmodelname>.vc file
		for ( i = 0; i < MAX_HEADMODELS; i++ ) {
			if (!strlen(headModelVoiceChat[i].headmodel)) {
				Com_sprintf(filename, sizeof(filename), "scripts/%s.vc", headModelName);
				voiceChatNum = CG_HeadModelVoiceChats(filename);
				if (voiceChatNum == -1)
					break;
				Com_sprintf(headModelVoiceChat[i].headmodel, sizeof ( headModelVoiceChat[i].headmodel ),
							"%s", headModelName);
				headModelVoiceChat[i].voiceChatNum = voiceChatNum;
				return &voiceChatLists[headModelVoiceChat[i].voiceChatNum];
			}
		}
	}
	gender = ci->gender;
	for (k = 0; k < 2; k++) {
		// just pick the first with the right gender
		for ( i = 0; i < MAX_VOICEFILES; i++ ) {
			if (strlen(voiceChatLists[i].name)) {
				if (voiceChatLists[i].gender == gender) {
					// store this head model with voice chat for future reference
					for ( j = 0; j < MAX_HEADMODELS; j++ ) {
						if (!strlen(headModelVoiceChat[j].headmodel)) {
							Com_sprintf(headModelVoiceChat[j].headmodel, sizeof ( headModelVoiceChat[j].headmodel ),
									"%s", headModelName);
							headModelVoiceChat[j].voiceChatNum = i;
							break;
						}
					}
					return &voiceChatLists[i];
				}
			}
		}
		// fall back to male gender because we don't have neuter in the mission pack
		if (gender == GENDER_MALE)
			break;
		gender = GENDER_MALE;
	}
	// store this head model with voice chat for future reference
	for ( j = 0; j < MAX_HEADMODELS; j++ ) {
		if (!strlen(headModelVoiceChat[j].headmodel)) {
			Com_sprintf(headModelVoiceChat[j].headmodel, sizeof ( headModelVoiceChat[j].headmodel ),
					"%s", headModelName);
			headModelVoiceChat[j].voiceChatNum = 0;
			break;
		}
	}
	// just return the first voice chat list
	return &voiceChatLists[0];
}

#define MAX_VOICECHATBUFFER		32

typedef struct bufferedVoiceChat_s
{
	int clientNum;
	sfxHandle_t snd;
	int voiceOnly;
	char cmd[MAX_SAY_TEXT];
	char message[MAX_SAY_TEXT];
} bufferedVoiceChat_t;

bufferedVoiceChat_t voiceChatBuffer[MAX_VOICECHATBUFFER];

/*
=================
CG_PlayVoiceChat
=================
*/
static void CG_PlayVoiceChat( const bufferedVoiceChat_t *vchat ) {
	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	if ( !cg_noVoiceChats.integer ) {
		CG_StartLocalSound( vchat->snd, CHAN_VOICE);
		if (vchat->clientNum != cg.snap->ps.clientNum) {
			int orderTask = CG_ValidOrder(vchat->cmd);
			if (orderTask > 0) {
				cgs.acceptOrderTime = cg.time + 5000;
				Q_strncpyz(cgs.acceptVoice, vchat->cmd, sizeof(cgs.acceptVoice));
				cgs.acceptTask = orderTask;
				cgs.acceptLeader = vchat->clientNum;
			}
			// see if this was an order
			CG_ShowResponseHead();
		}
	}
	if (!vchat->voiceOnly && !cg_noVoiceText.integer) {
		CG_AddToTeamChat( vchat->message );
		CG_Printf( "%s\n", vchat->message );
	}
	voiceChatBuffer[cg.voiceChatBufferOut].snd = 0;
}

/*
=====================
CG_PlayBufferedVoieChats
=====================
*/
void CG_PlayBufferedVoiceChats( void ) {
	if ( cg.voiceChatTime < cg.time ) {
		if (cg.voiceChatBufferOut != cg.voiceChatBufferIn && voiceChatBuffer[cg.voiceChatBufferOut].snd) {
			//
			CG_PlayVoiceChat(&voiceChatBuffer[cg.voiceChatBufferOut]);
			//
			cg.voiceChatBufferOut = (cg.voiceChatBufferOut + 1) % MAX_VOICECHATBUFFER;
			cg.voiceChatTime = cg.time + 1000;
		}
	}
}

/*
=====================
CG_AddBufferedVoiceChat
=====================
*/
static void CG_AddBufferedVoiceChat( const bufferedVoiceChat_t *vchat ) {
	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	memcpy(&voiceChatBuffer[cg.voiceChatBufferIn], vchat, sizeof(bufferedVoiceChat_t));
	cg.voiceChatBufferIn = (cg.voiceChatBufferIn + 1) % MAX_VOICECHATBUFFER;
	if (cg.voiceChatBufferIn == cg.voiceChatBufferOut) {
		CG_PlayVoiceChat( &voiceChatBuffer[cg.voiceChatBufferOut] );
		cg.voiceChatBufferOut++;
	}
}

/*
=================
CG_VoiceChatLocal
=================
*/
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd ) {
	char *chat;
	voiceChatList_t *voiceChatList;
	clientInfo_t *ci;
	sfxHandle_t snd;
	bufferedVoiceChat_t vchat;

	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	if ( mode == SAY_ALL && CG_IsTeamGame(cgs.gametype) && cg_teamChatsOnly.integer ) {
		return;
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Printf("^1CG_VoiceChatLocal() invalid client number %d\n", clientNum);
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	cgs.currentVoiceClient = clientNum;

	voiceChatList = CG_VoiceChatListForClient( clientNum );

	if ( CG_GetVoiceChat( voiceChatList, cmd, &snd, &chat ) ) {
		vchat.clientNum = clientNum;
		vchat.snd = snd;
		vchat.voiceOnly = voiceOnly;
		Q_strncpyz(vchat.cmd, cmd, sizeof(vchat.cmd));
		if ( mode == SAY_TELL ) {
			Com_sprintf(vchat.message, sizeof(vchat.message), "[%s]: %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
		}
		else if ( mode == SAY_TEAM ) {
			Com_sprintf(vchat.message, sizeof(vchat.message), "(%s): %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
		}
		else {
			Com_sprintf(vchat.message, sizeof(vchat.message), "%s: %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
		}
		CG_AddBufferedVoiceChat(&vchat);
	}
}

/*
=================
CG_VoiceChat
=================
*/
static void CG_VoiceChat( int mode ) {
	const char *cmd;
	int clientNum, color;
	qboolean voiceOnly;

	voiceOnly = atoi(CG_Argv(1));
	clientNum = atoi(CG_Argv(2));
	color = atoi(CG_Argv(3));
	cmd = CG_Argv(4);

	if (cg_noTaunt.integer != 0) {
		if (!strcmp(cmd, VOICECHAT_KILLINSULT)  || !strcmp(cmd, VOICECHAT_TAUNT) || \
			!strcmp(cmd, VOICECHAT_DEATHINSULT) || !strcmp(cmd, VOICECHAT_KILLGAUNTLET) || \
			!strcmp(cmd, VOICECHAT_PRAISE)) {
			return;
		}
	}

	CG_VoiceChatLocal( mode, voiceOnly, clientNum, color, cmd );
}
#endif  // MPACK

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar( char *text ) {
	int i, l;
	int len;
	char orig[MAX_STRING_CHARS];

	if (cgs.protocol != PROTOCOL_QL) {
		return;
	}

	if (!text)
		return;

	len = strlen(text);
	if (len >= MAX_STRING_CHARS) {
		Com_Printf("chat string too long\n");
		text[0] = '\0';
		return;
	}

	if (len < 3)
		return;

	//Com_Printf("%s\n", text);
	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (text[i] == '\x19') {
			//Com_Printf("^2removed x19\n");
			continue;
		}
		text[l++] = text[i];
	}
	text[l] = '\0';

	// remove player numbers
	Q_strncpyz(orig, text, len);
	Q_strncpyz(text, orig + 3, len - 3);
}

static int CG_CpmaStringScore (char c)
{
	int x;

	if (c >= 'A'  &&  c <= 'Z') {
		x = 26 + c - 'A';
	} else if (c >= 'a'  &&  c <= 'z') {
		x = c - 'a';
	} else {
		//Com_Printf("^1CG_CpmaStringScore() invalid char '%c'\n", c);
		x = 0;
	}

	return x;
}

static qboolean CG_CpmaParseMstats (qboolean intermission)
{
	int n;
	int wflags;
	int powerupFlags;
	duelScore_t *ds;
	duelWeaponStats_t *ws;
	int clientNum;
	int i;
	int duelPlayer;
	int totalHits;
	int totalAtts;
	const char *s;
	wclient_t *wc;
	//int val;

	// none -- 26 args

	//FIXME saw it in a tdm demo as well
	if (!CG_IsDuelGame(cgs.gametype)) {
		return qfalse;
	}

	n = 1;
	clientNum = atoi(CG_Argv(n));  n++; // player num

	if (intermission) {
		// mstatsa
		// 0: winner
		// 1: loser

		if (clientNum != 0  &&  clientNum != 1) {
			CG_Printf("^1CG_CpmaParseMstats() invalid mstatsa client number %d\n", clientNum);
			return qtrue;
		}

		//FIXME try to get real client numbers?

		if (clientNum == 0) {  // winner
			duelPlayer = 0;
			cg.duelPlayer1 = 0;  //FIXME
		} else {
			duelPlayer = 1;
			cg.duelPlayer2 = 1;  //FIXME
		}

		wc = NULL;
	} else {
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_CpmaParseMstats() invalid client number %d\n", clientNum);
			return qtrue;
		}

		wc = &wclients[clientNum];

		//FIXME
		// with cpma >= 1.50 one player might have forfeited and they might have disconnected or moved to spec
		duelPlayer = -1;
		for (i = 0;  i < MAX_CLIENTS;  i++) {
			if (!cgs.clientinfo[i].infoValid  ||  cgs.clientinfo[i].team != TEAM_FREE) {
				continue;
			}

			duelPlayer++;

			if (i == clientNum) {
				break;
			}
		}

		if (duelPlayer < 0) {
			// disconnected or spec, make them the second player
			duelPlayer = 1;
		} else if (duelPlayer >= 1) {
			duelPlayer = 1;
		} else {
			// duelPlayer is 0
		}

		if (duelPlayer == 0) {
			cg.duelPlayer1 = clientNum;
		} else {
			cg.duelPlayer2 = clientNum;
		}
	}

	//Com_Printf("^5duel player %d %s\n", duelPlayer, cgs.clientinfo[clientNum].name);

	ds = &cg.duelScores[duelPlayer];
	memset(ds, 0, sizeof(duelScore_t));

	ds->ci = cgs.clientinfo[clientNum];

	if (intermission) {
		// client info set above is bs
		if (clientNum == 0) {  // winner
			Q_strncpyz(ds->ci.name, cg.cpmaDuelPlayerWinner, sizeof(ds->ci.name));
		} else {  // loser
			Q_strncpyz(ds->ci.name, cg.cpmaDuelPlayerLoser, sizeof(ds->ci.name));
		}
	}

	cg.duelScoresValid = qtrue;
	cg.duelScoresServerTime = cg.snap->serverTime;

	totalHits = 0;
	totalAtts = 0;

	wflags = atoi(CG_Argv(n));  n++;

	// rail           1000 0000     128
	// rocket         0010 0000
    // shotgun        0000 1000
    // lg             0100 0000
    // grenade        0001 0000
    // mg             0000 0100
    // plasma    0001 0000 0000
    // gaunt               0010

	if (wflags & 0x1) {
		ws = &ds->weaponStats[WP_NONE];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x2) {
		ws = &ds->weaponStats[WP_GAUNTLET];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x4) {
		ws = &ds->weaponStats[WP_MACHINEGUN];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x8) {
		ws = &ds->weaponStats[WP_SHOTGUN];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x10) {
		ws = &ds->weaponStats[WP_GRENADE_LAUNCHER];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x20) {
		ws = &ds->weaponStats[WP_ROCKET_LAUNCHER];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x40) {
		ws = &ds->weaponStats[WP_LIGHTNING];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x80) {
		ws = &ds->weaponStats[WP_RAILGUN];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x100) {
		ws = &ds->weaponStats[WP_PLASMAGUN];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}

	if (wflags & 0x200) {
		ws = &ds->weaponStats[WP_BFG];
		ws->hits = atoi(CG_Argv(n));  n++;
		ws->atts = atoi(CG_Argv(n));  n++;
		totalHits += ws->hits;
		totalAtts += ws->atts;
		if (ws->atts) {
			ws->accuracy = (100 * ws->hits) / ws->atts;
		} else {
			ws->accuracy = 0;
		}
		ws->kills = atoi(CG_Argv(n));  n++;
		ws->deaths = atoi(CG_Argv(n));  n++;
		ws->take = atoi(CG_Argv(n));  n++;
		ws->drop = atoi(CG_Argv(n));  n++;
	}
	if (wflags & 0x400) {
		Com_Printf("^3FIXME cpma stats wflags 0x400\n");
	}
	if (wflags & 0x800) {
		Com_Printf("^3FIXME cpma stats wflags 0x800\n");
	}

	ds->damage = atoi(CG_Argv(n));  n++;  // damage done
	atoi(CG_Argv(n));  n++;  // damage taken
	atoi(CG_Argv(n));  n++;  // total armor taken
	atoi(CG_Argv(n));  n++;  // total health taken
	ds->megaHealthPickups = atoi(CG_Argv(n));  n++;
	ds->redArmorPickups = atoi(CG_Argv(n));  n++;
	ds->yellowArmorPickups = atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;  // green armor?

	// skip powerup info
	powerupFlags = atoi(CG_Argv(n));  n++;
	for (i = 0;  i < 16;  i++) {
		if (powerupFlags & (1 << i)) {
			atoi(CG_Argv(n));  // pickups
			//Com_Printf("^2powerup pickups: %d\n", val);
			n++;

			atoi(CG_Argv(n));  // time
			//Com_Printf("^2powerup time: %d\n", val);
			n++;
		}
	}

	atoi(CG_Argv(n));  n++;  // 2?
	s = CG_Argv(n);  n++;  // abataiayaaaa
	//Com_Printf("^3%s\n", s);
	if (s[0] != '-') {
		ds->ping = CG_CpmaStringScore(s[2]) * 32 + CG_CpmaStringScore(s[3]);
	} else {
		ds->ping = 0;
	}

	// with mstatsa we don't know wc
	if (!intermission) {
		wc->serverPingAccum += ds->ping;
		wc->serverPingSamples++;
	}

	ds->score = atoi(CG_Argv(n));  n++;  // score

	// with mstatsa we don't have real clientNum
	if (!intermission) {
		cgs.clientinfo[clientNum].score = ds->score;
	}

	ds->kills = atoi(CG_Argv(n));  n++;  // frags?
	atoi(CG_Argv(n));  n++;  // deaths?
	atoi(CG_Argv(n));  n++;  // ?
	atoi(CG_Argv(n));  n++;  // ?
	atoi(CG_Argv(n));  n++;  // team damage
	atoi(CG_Argv(n));  n++;  // team kills
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;

	if (totalAtts) {
		ds->accuracy = (100 * totalHits) / totalAtts;
	} else {
		ds->accuracy = 0;
	}

	// with mstats we don't have real clientNum
	if (!intermission) {
		if (clientNum == cg.snap->ps.clientNum) {
			ds->awardExcellent = cg.snap->ps.persistant[PERS_EXCELLENT_COUNT];
			ds->awardImpressive = cg.snap->ps.persistant[PERS_IMPRESSIVE_COUNT];
			ds->awardHumiliation = cg.snap->ps.persistant[PERS_GAUNTLET_FRAG_COUNT];
		} else {
			ds->awardExcellent = -1;
			ds->awardImpressive = -1;
			ds->awardHumiliation = -1;
		}
	}

	// with mstatsa we don't have real clientNum
	if (!intermission) {
		// for old scoreboard
		for (i = 0;  i < cg.numScores;  i++) {
			if (cg.scores[i].client != clientNum) {
				continue;
			}

			cg.scores[i].ping = ds->ping;
			cg.scores[i].score = cgs.clientinfo[clientNum].score;
			break;
		}
	}

	return qtrue;
}

static qboolean CG_CpmaParseDmscores (void)
{
	int clientNum;

	// 1  first place client number
	// 2  second place client number

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^3CG_CpmaParseDmscores: invalid first place client number: %d\n", clientNum);
		return qtrue;
	}
	Q_strncpyz(cgs.firstPlace, cgs.clientinfo[clientNum].name, sizeof(cgs.firstPlace));

	clientNum = atoi(CG_Argv(2));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^3CG_CpmaParseDmscores: invalid second place client number: %d\n", clientNum);
		return qtrue;
	}
	Q_strncpyz(cgs.secondPlace, cgs.clientinfo[clientNum].name, sizeof(cgs.secondPlace));

	//Com_Printf("first place: %s\n", cgs.firstPlace);
	//Com_Printf("second place: %s\n", cgs.secondPlace);

	return qtrue;
}

static qboolean CG_CpmaMM2 (void)
{
	int clientNum;
	int location;
	const char *s;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		CG_Printf("^1CG_CpmaMM2() invalid client number %d\n", clientNum);
		return qtrue;
	}

	location = atoi(CG_Argv(2));
	s = va("(%s^7) (%s): ^5%s", cgs.clientinfo[clientNum].name, CG_ConfigString(CS_LOCATIONS + location), CG_Argv(3));
	CG_Printf("%s\n", s);
	if (cg_serverPrintToChat.integer) {  //FIXME treat as 'print' ?
		CG_PrintToScreen("%s", s);
	}

	return qtrue;
}

static qboolean CG_CpmaXscores (void)
{
	int numScores;
	int i;
	int n;
	int clientNum;
	clientInfo_t *ci;
	const char *s;
	//const char *s2;
	//int x;
	score_t *sc;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	memset(cg.scores, 0, sizeof(cg.scores));

	n = 1;
	numScores = atoi(CG_Argv(n));  n++;
	cg.numScores = numScores;

	for (i = 0;  i < numScores;  i++) {
		sc = &cg.scores[i];

		clientNum = atoi(CG_Argv(n));  n++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_CpmaXscores() invalid client number %d\n", clientNum);
			return qtrue;
		}
		sc->client = clientNum;

		ci = &cgs.clientinfo[clientNum];
		sc->score = atoi(CG_Argv(n));  n++;  // score
		ci->score = sc->score;

		s = CG_Argv(n);  n++;  // text
#if 0
		Com_Printf("xscore '%s^7' %s\n", ci->name, s);
		s2 = s;
		while (1) {
			qboolean gotIt = qtrue;
			if (!*s2) {
				break;
			}
			if (s2[0] >= 'A'  &&  s2[0] <= 'Z') {
				x = 26 + s2[0] - 'A';
			} else if (s2[0] >= 'a'  &&  s2[0] <= 'z') {
				x = s2[0] - 'a';
			} else {
				gotIt = qfalse;
			}

			if (gotIt) {
				Com_Printf("%d ", x);
			} else {
				Com_Printf("^1%c^7 ", s2[0]);
			}

			s2++;
		}
		Com_Printf("\n");
#endif

#if 0
		x = CG_CpmaStringScore(s[1]);
		if (x == 2) {
			sc->team = ci->team = TEAM_RED;
		} else if (x == 1) {
			sc->team = ci->team = TEAM_BLUE;
		} else {
			sc->team = ci->team = TEAM_SPECTATOR;
		}
#endif
		sc->team = ci->team;

		if (s[0] != '-') {
			sc->ping = CG_CpmaStringScore(s[2]) * 32 + CG_CpmaStringScore(s[3]);
		} else {
			sc->ping = 0;
		}

		sc->time = atoi(CG_Argv(n));  n++;  // time
		sc->powerups = atoi(CG_Argv(n));  n++;
		sc->net = atoi(CG_Argv(n));  n++;  // net
		sc->captures = sc->net;
		sc->defendCount = atoi(CG_Argv(n));  n++;

		if (sc->team == TEAM_RED) {
			redCount++;
			redTotalPing += sc->ping;
		} else if (sc->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += sc->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	return qtrue;
	//return qfalse;
}

static qboolean CG_CpmaParseDuelEndScores (void)
{
	int winMode;

	Q_strncpyz(cg.cpmaDuelPlayerWinner, CG_Argv(1), sizeof(cg.cpmaDuelPlayerWinner));
	Q_strncpyz(cg.cpmaDuelPlayerLoser, CG_Argv(2), sizeof(cg.cpmaDuelPlayerLoser));

	winMode = atoi(CG_Argv(3));

	if (winMode == 0) {
		cg.duelForfeit = qfalse;
	} else {
		// this precedes mstatsa / xstats2a so we don't know who cg.duelPlayer[12] are yet
		cg.duelForfeit = qtrue;
	}


	return qtrue;
}

static void CG_ParseCtfsRoundScores (void)
{
	int i;
	int n;

	cg.ctfsRoundScoreValid = qtrue;

	n = 1;

	//FIXME don't know if it's always 10
	for (i = 0;  i < 10;  i++) {
		cg.ctfsRedRoundScores[i] = atoi(CG_Argv(n));
		n++;
		cg.ctfsBlueRoundScores[i] = atoi(CG_Argv(n));
		n++;
	}

	cg.ctfsRedScore = atoi(CG_Argv(21));
	cg.ctfsBlueScore = atoi(CG_Argv(22));
}

static void CG_FilterOspTeamChatLocation (const char *text)
{
#if 0
	char *p;

	p = text;

	while (1) {
		if (!*p) {
			break;
		}
		Com_Printf("'%c'\n", p[0]);

		p++;
	}
#endif

	return;
}

static void CG_ParseScores_Ffa (void)
{
	int i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int SCSIZE;

	cg.scoresValid = qtrue;
	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		Com_Printf("^3CG_ParseScores_Ffa() cg.numScores (%d) > MAX_CLIENTS (%d)\n", cg.numScores, MAX_CLIENTS);
		cg.numScores = MAX_CLIENTS;
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cgs.clientinfo[i].scoreValid = qfalse;
		cg.clientHasScore[i] = qfalse;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	//memset( cg.scores, 0, sizeof( cg.scores ) );

	if (cgs.protocol == PROTOCOL_QL) {
		SCSIZE = 18;
	} else {
		SCSIZE = 14;
	}

	for ( i = 0 ; i < cg.numScores ; i++ ) {
		int clientNum;

		clientNum = atoi( CG_Argv( i * SCSIZE + 4 ) );
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Ffa() invalid client number %d\n", clientNum);
		}
		cg.scores[i].client = clientNum;
		cg.scores[i].score = atoi( CG_Argv( i * SCSIZE + 5 ) );
		cg.scores[i].ping = atoi( CG_Argv( i * SCSIZE + 6 ) );
		cg.scores[i].time = atoi( CG_Argv( i * SCSIZE + 7 ) );
		//cg.scores[i].scoreFlags = atoi( CG_Argv( i * SCSIZE + 8 ) );
		//cg.scores[i].powerups = atoi( CG_Argv( i * SCSIZE + 9 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * SCSIZE + 8));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * SCSIZE + 9));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * SCSIZE + 10));
		cg.scores[i].gauntletCount = atoi(CG_Argv(i * SCSIZE + 11));
		cg.scores[i].defendCount = atoi(CG_Argv(i * SCSIZE + 12));
		cg.scores[i].assistCount = atoi(CG_Argv(i * SCSIZE + 13));
		cg.scores[i].perfect = atoi(CG_Argv(i * SCSIZE + 14));
		cg.scores[i].captures = atoi(CG_Argv(i * SCSIZE + 15));

		cg.scores[i].alive = atoi(CG_Argv(i * SCSIZE + 16));
		cg.scores[i].frags = atoi(CG_Argv(i * SCSIZE + 17));
		cg.scores[i].deaths = atoi(CG_Argv(i * SCSIZE + 18));
		cg.scores[i].bestWeapon = atoi(CG_Argv(i * SCSIZE + 19));

		//FIXME check if 20 is powerups
		cg.scores[i].powerups = atoi(CG_Argv(i * SCSIZE + 20));
		cg.scores[i].damageDone = atoi(CG_Argv(i * SCSIZE + 21));

		//Com_Printf("score %d %s\n", i, cgs.clientinfo[cg.scores[i].client].name);
		//Com_Printf("sc %d (%d  %d)\n", i, cg.scores[i].scoreFlags, cg.scores[i].alive);
		//Com_Printf("sc %d (%d  %d %d  -- %d %d)\n", i, cg.scores[i].scoreFlags, cg.scores[i].perfect, cg.scores[i].captures, cg.scores[i].frags, cg.scores[i].deaths);

#if 0
		Com_Printf("%d  %d  %d  %d\n",
				   atoi(CG_Argv(i * SCSIZE + 18)),
				   atoi(CG_Argv(i * SCSIZE + 19)),
				   atoi(CG_Argv(i * SCSIZE + 20)),
				   atoi(CG_Argv(i * SCSIZE + 21)));

		Com_Printf("accuracy: %d\n", cg.scores[i].accuracy);
#endif

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			Com_Printf("^3FIXME CG_ParseScores_Ffa() score->client invalid: %d\n", cg.scores[i].client);
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;
		cgs.clientinfo[ cg.scores[i].client ].powerups = cg.scores[i].powerups;
		cgs.clientinfo[cg.scores[i].client].scoreIndexNum = i;

		cgs.clientinfo[cg.scores[i].client].scoreValid = qtrue;
		cg.clientHasScore[cg.scores[i].client] = qtrue;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

		//Com_Printf("^1sss  %s  score %d\n", cgs.clientinfo[cg.scores[i].client].name, cgs.clientinfo[cg.scores[i].client].score);

		if (cg.scores[i].team == TEAM_RED) {
			redCount++;
			redTotalPing += cg.scores[i].ping;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += cg.scores[i].ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	//Com_Printf("^5red ping %d  blue ping %d\n", cg.avgRedPing, cg.avgBluePing);

#if 1  //def MPACK
	CG_SetScoreSelection(NULL);
#endif

	if (CG_IsDuelGame(cgs.gametype)) {
		CG_SetDuelPlayers();
	}

	// check sizes
	if (CG_Argc() != (i * SCSIZE + 4)) {
		CG_Printf("^1CG_ParseScores_Ffa() argc (%d) != %d\n", CG_Argc(), (i * SCSIZE + 4));
	}

//#undef SCSIZE
}

static void CG_ParseScores_Duel (void)
{
	int i;
	int n;
	int j;
	int maxWeapons;

	cg.duelScoresValid = qtrue;
	cg.duelScoresServerTime = cg.snap->serverTime;

	cg.numDuelScores = atoi(CG_Argv(1));
	if (cg.numDuelScores > 2) {
		Com_Printf("^3FIXME CG_ParseScores_Duel scores_duel num %d (more than 2)\n", cg.numDuelScores);
	}

	n = 2;
	for (i = 0;  i < 2;  i++) {
		duelScore_t *ds;
		clientInfo_t *ci;

		ds = &cg.duelScores[i];

		if (!*CG_Argv(n)) {
			break;
		}

		ds->clientNum = atoi(CG_Argv(n));  n++;
		if (ds->clientNum < 0  ||  ds->clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Duel() invalid client number %d\n", ds->clientNum);
			return;
		}

		//FIXME 2017-06-03 ql/minqlx config string fix needed?

		ci = &cgs.clientinfo[ds->clientNum];

		if (ci->infoValid) {
			ds->ci = cgs.clientinfo[ds->clientNum];
		}

		ds->score = atoi(CG_Argv(n));  n++;
		//FIXME ughhh
		ci->score = ds->score;
		ds->ci.score = ds->score;

		ds->ping = atoi(CG_Argv(n));  n++;
		ds->time = atoi(CG_Argv(n));  n++;
		ds->kills = atoi(CG_Argv(n));  n++;
		ds->deaths = atoi(CG_Argv(n));  n++;
		ds->accuracy = atoi(CG_Argv(n));  n++;
		ds->bestWeapon = atoi(CG_Argv(n));  n++;
		ds->damage = atoi(CG_Argv(n));  n++;
		ds->awardImpressive = atoi(CG_Argv(n));  n++;
		ds->awardExcellent = atoi(CG_Argv(n));  n++;
		ds->awardHumiliation = atoi(CG_Argv(n));  n++;
		ds->perfect = atoi(CG_Argv(n));  n++;

		ds->redArmorPickups = atoi(CG_Argv(n));  n++;
		ds->redArmorTime = atof(CG_Argv(n));  n++;
		ds->yellowArmorPickups = atoi(CG_Argv(n));  n++;
		ds->yellowArmorTime = atof(CG_Argv(n));  n++;
		ds->greenArmorPickups = atoi(CG_Argv(n));  n++;
		ds->greenArmorTime = atof(CG_Argv(n));  n++;
		ds->megaHealthPickups = atoi(CG_Argv(n));  n++;
		ds->megaHealthTime = atof(CG_Argv(n));  n++;

		maxWeapons = WP_NUM_WEAPONS;

		for (j = 1;  j < maxWeapons;  j++) {
			duelWeaponStats_t *ws;

			ws = &ds->weaponStats[j];

			ws->hits = atoi(CG_Argv(n));  n++;
			ws->atts = atoi(CG_Argv(n));  n++;
			ws->accuracy = atoi(CG_Argv(n));  n++;
			ws->damage = atoi(CG_Argv(n));  n++;
			ws->kills = atoi(CG_Argv(n));  n++;
		}

		// for old scoreboard
		for (j = 0;  j < cg.numScores;  j++) {
			if (cg.scores[j].client != ds->clientNum) {
				continue;
			}

			//Com_Printf("^5score set for %d  ^3%d %d %d\n", j, ds->ping, ds->score, ds->time);
			cg.scores[j].ping = ds->ping;
			cg.scores[j].score = ds->score;
			cg.scores[j].time = ds->time;
			break;
		}

	}

	CG_SetDuelPlayers();

	// check sizes
	if (CG_Argc() != n) {
		CG_Printf("^1CG_ParseScores_Duel() argc (%d) != %d\n", CG_Argc(), n);
	}
}

static void CG_ParseScores_Tdm (void)
{
	int i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	//memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 3;

	cg.tdmScore.rra = atoi(CG_Argv(1));
	cg.tdmScore.rya = atoi(CG_Argv(2));
	cg.tdmScore.rga = atoi(CG_Argv(3));
	cg.tdmScore.rmh = atoi(CG_Argv(4));
	cg.tdmScore.rquad = atoi(CG_Argv(5));
	cg.tdmScore.rbs = atoi(CG_Argv(6));
	cg.tdmScore.rregen = atoi(CG_Argv(7));
	cg.tdmScore.rhaste = atoi(CG_Argv(8));
	cg.tdmScore.rinvis = atoi(CG_Argv(9));

	cg.tdmScore.rquadTime = atoi(CG_Argv(10));
	cg.tdmScore.rbsTime = atoi(CG_Argv(11));
	cg.tdmScore.rregenTime = atoi(CG_Argv(12));
	cg.tdmScore.rhasteTime = atoi(CG_Argv(13));
	cg.tdmScore.rinvisTime = atoi(CG_Argv(14));

	cg.tdmScore.bra = atoi(CG_Argv(15));
	cg.tdmScore.bya = atoi(CG_Argv(16));
	cg.tdmScore.bga = atoi(CG_Argv(17));
	cg.tdmScore.bmh = atoi(CG_Argv(18));
	cg.tdmScore.bquad = atoi(CG_Argv(19));
	cg.tdmScore.bbs = atoi(CG_Argv(20));
	cg.tdmScore.bregen = atoi(CG_Argv(21));
	cg.tdmScore.bhaste = atoi(CG_Argv(22));
	cg.tdmScore.binvis = atoi(CG_Argv(23));

	cg.tdmScore.bquadTime = atoi(CG_Argv(24));
	cg.tdmScore.bbsTime = atoi(CG_Argv(25));
	cg.tdmScore.bregenTime = atoi(CG_Argv(26));
	cg.tdmScore.bhasteTime = atoi(CG_Argv(27));
	cg.tdmScore.binvisTime = atoi(CG_Argv(28));

	cg.tdmScore.numPlayerScores = atoi(CG_Argv(29));
	//CG_Printf("^5num player scores %d\n", cg.tdmScore.numPlayerScores);

	i = 30;

	// can't use version check because some servers (sampler?) are running
	// old server binaries with new cgame dlls

	// 16
	//if (CG_CheckQlVersion(0, 1, 0, 728)) {
	if (CG_Argc() > (cg.tdmScore.numPlayerScores * 16 + 31)  ||  cgs.realProtocol >= 91) {
		cg.teamScores[0] = atoi(CG_Argv(i));  i++;
		cg.teamScores[1] = atoi(CG_Argv(i));  i++;
	}

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;

	memset(&cg.tdmScore.playerScore, 0, sizeof(cg.tdmScore.playerScore));

	// need cg.scores index for endgame stats
	cg.numScores = 0;
	cg.scoresValid = qtrue;

	while (1) {
		tdmPlayerScore_t *ts;
		int clientNum;
		int team;
		score_t *oldScore;

		if (!*CG_Argv(i)) {
			break;
		}

		oldScore = &cg.scores[cg.numScores];
		cg.numScores++;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Tdm() invalid client number %d\n", clientNum);
			return;
		}

		team = atoi(CG_Argv(i));  i++;

		// 2017-06-03 ql/minqlx bug not updating config strings so players appear with wrong team, hack to grab them from scores
		if (cg_useScoresUpdateTeam.integer) {
			if (!cgs.clientinfo[clientNum].override) {
				cgs.clientinfo[clientNum].team = team;
			}
		}

		//Com_Printf("client %d  team %d\n", clientNum, team);

		ts = &cg.tdmScore.playerScore[clientNum];

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			ts->valid = qtrue;
		} else {
			//FIXME maybe??
			ts->valid = qfalse;
		}

		ts->clientNum = clientNum;
		oldScore->client = clientNum;
		ts->team = team;
		oldScore->team = team;
		if (cgs.realProtocol < 91) {
			ts->subscriber = atoi(CG_Argv(i));  i++;
		}
		ts->score = atoi(CG_Argv(i));  i++;
		oldScore->score = ts->score;
		ts->ping = atoi(CG_Argv(i));  i++;
		oldScore->ping = ts->ping;
		ts->time = atoi(CG_Argv(i));  i++;
		oldScore->time = ts->time;
		ts->kills = atoi(CG_Argv(i));  i++;
		oldScore->frags = ts->kills;
		ts->deaths = atoi(CG_Argv(i));  i++;
		oldScore->deaths = ts->deaths;
		ts->accuracy = atoi(CG_Argv(i));  i++;
		oldScore->accuracy = ts->accuracy;
		ts->bestWeapon = atoi(CG_Argv(i));  i++;
		oldScore->bestWeapon = ts->bestWeapon;
		ts->awardImpressive = atoi(CG_Argv(i));  i++;
		oldScore->impressiveCount = ts->awardImpressive;
		ts->awardExcellent = atoi(CG_Argv(i));  i++;
		oldScore->excellentCount = ts->awardExcellent;
		ts->awardHumiliation = atoi(CG_Argv(i));  i++;
		oldScore->gauntletCount = ts->awardHumiliation;
		ts->tks = atoi(CG_Argv(i));  i++;
		ts->tkd = atoi(CG_Argv(i));  i++;
		ts->damageDone = atoi(CG_Argv(i));  i++;

		if (ts->team == TEAM_RED) {
			redCount++;
			redTotalPing += ts->ping;
		} else if (ts->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += ts->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseScores_Tdm() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseScores_Ca (void)
{
	int i;
	tdmPlayerScore_t *ts;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 3;

	cg.tdmScore.numPlayerScores = atoi(CG_Argv(1));

	memset(&cg.tdmScore.playerScore, 0, sizeof(cg.tdmScore.playerScore));

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;

	i = 2;

	// can't use version check because some servers (sampler?) are running
	// old server binaries with new cgame dlls

	// 17
	//if (CG_CheckQlVersion(0, 1, 0, 728)) {
	if (CG_Argc() > (cg.tdmScore.numPlayerScores * 17 + 3)  ||  cgs.realProtocol >= 91) {
		cg.teamScores[0] = atoi(CG_Argv(i));  i++;
		cg.teamScores[1] = atoi(CG_Argv(i));  i++;
	}

	// need score indexes for endgame stats
	cg.numScores = 0;
	cg.scoresValid = qtrue;

	while (1) {
		int clientNum;
		score_t *oldScore;

		if (!*CG_Argv(i)) {
			break;
		}

		oldScore = &cg.scores[cg.numScores];
		cg.numScores++;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Ca() invalid client number %d\n", clientNum);
			return;
		}

		ts = &cg.tdmScore.playerScore[clientNum];
		ts->valid = qtrue;
		ts->clientNum = clientNum;
		oldScore->client = clientNum;

		ts->team = atoi(CG_Argv(i));  i++;

		// 2017-06-03 ql/minqlx bug not updating config strings so players appear with wrong team, hack to grab them from scores
		if (cg_useScoresUpdateTeam.integer) {
			if (!cgs.clientinfo[clientNum].override) {
				cgs.clientinfo[clientNum].team = ts->team;
			}
		}

		oldScore->team = ts->team;

		if (cgs.realProtocol < 91) {
			ts->subscriber = atoi(CG_Argv(i));  i++;
		}
		ts->score = atoi(CG_Argv(i));  i++;
		oldScore->score = ts->score;
		ts->ping = atoi(CG_Argv(i));  i++;
		oldScore->ping = ts->ping;
		ts->time = atoi(CG_Argv(i));  i++;
		oldScore->time = ts->time;
		ts->kills = atoi(CG_Argv(i));  i++;
		oldScore->frags = ts->kills;
		ts->deaths = atoi(CG_Argv(i));  i++;
		oldScore->deaths = ts->deaths;
		ts->accuracy = atoi(CG_Argv(i));  i++;
		//oldScore->bestWeaponAccuracy = ts->accuracy;
		oldScore->accuracy = ts->accuracy;
		ts->bestWeapon = atoi(CG_Argv(i));  i++;
		oldScore->bestWeapon = ts->bestWeapon;
		ts->bestWeaponAccuracy = atoi(CG_Argv(i));  i++;
		oldScore->bestWeaponAccuracy = ts->bestWeaponAccuracy;
		ts->damageDone = atoi(CG_Argv(i));  i++;
		ts->awardImpressive = atoi(CG_Argv(i));  i++;
		oldScore->impressiveCount = ts->awardImpressive;
		ts->awardExcellent = atoi(CG_Argv(i));  i++;
		oldScore->excellentCount = ts->awardExcellent;
		ts->awardHumiliation = atoi(CG_Argv(i));  i++;
		oldScore->gauntletCount = ts->awardHumiliation;
		ts->perfect = atoi(CG_Argv(i));  i++;
		oldScore->perfect = ts->perfect;
		ts->alive = atoi(CG_Argv(i));  i++;
		oldScore->alive = ts->alive;

		if (ts->team == TEAM_RED) {
			redCount++;
			redTotalPing += ts->ping;
		} else if (ts->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += ts->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseScores_Ca() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseSmScores (void)
{
	int i;
	tdmPlayerScore_t *ts;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	//Com_Printf("^5CG_ParseSmScores()\n");
	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 3;

	cg.tdmScore.numPlayerScores = atoi(CG_Argv(1));

	memset(&cg.tdmScore.playerScore, 0, sizeof(cg.tdmScore.playerScore));

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;

	i = 2;

	cg.teamScores[0] = atoi(CG_Argv(i));  i++;
	cg.teamScores[1] = atoi(CG_Argv(i));  i++;

	// need score indexes for endgame stats
	cg.numScores = 0;
	cg.scoresValid = qtrue;

	while (1) {
		int clientNum;
		score_t *oldScore;

		if (!*CG_Argv(i)) {
			break;
		}

		oldScore = &cg.scores[cg.numScores];
		cg.numScores++;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseSmScores invalid client number %d\n", clientNum);
			return;
		}

		ts = &cg.tdmScore.playerScore[clientNum];
		ts->valid = qtrue;
		ts->clientNum = clientNum;
		oldScore->client = clientNum;

		//ts->team = atoi(CG_Argv(i));  i++;
		ts->team = cgs.clientinfo[clientNum].team;
		oldScore->team = ts->team;

		ts->score = atoi(CG_Argv(i));  i++;
		oldScore->score = ts->score;
		ts->ping = atoi(CG_Argv(i));  i++;
		oldScore->ping = ts->ping;
		ts->time = atoi(CG_Argv(i));  i++;
		oldScore->time = ts->time;

		// for these two have only seen 0 0  or  0 1 (1 0 ?)
		i++;  // unknown
		i++;  // unknown

		/*
		ts->accuracy = atoi(CG_Argv(i));  i++;
		//oldScore->bestWeaponAccuracy = ts->accuracy;
		oldScore->accuracy = ts->accuracy;
		ts->bestWeapon = atoi(CG_Argv(i));  i++;
		oldScore->bestWeapon = ts->bestWeapon;
		*/
		
		ts->kills = atoi(CG_Argv(i));  i++;
		oldScore->frags = ts->kills;
		ts->deaths = atoi(CG_Argv(i));  i++;
		oldScore->deaths = ts->deaths;


		/*
		ts->bestWeaponAccuracy = atoi(CG_Argv(i));  i++;
		oldScore->bestWeaponAccuracy = ts->bestWeaponAccuracy;
		ts->damageDone = atoi(CG_Argv(i));  i++;
		ts->awardImpressive = atoi(CG_Argv(i));  i++;
		oldScore->impressiveCount = ts->awardImpressive;
		ts->awardExcellent = atoi(CG_Argv(i));  i++;
		oldScore->excellentCount = ts->awardExcellent;
		ts->awardHumiliation = atoi(CG_Argv(i));  i++;
		oldScore->gauntletCount = ts->awardHumiliation;
		ts->perfect = atoi(CG_Argv(i));  i++;
		oldScore->perfect = ts->perfect;
		ts->alive = atoi(CG_Argv(i));  i++;
		oldScore->alive = ts->alive;
		*/
		if (ts->team == TEAM_RED) {
			redCount++;
			redTotalPing += ts->ping;
		} else if (ts->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += ts->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseSmScores()) argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseScores_Ctf (void)
{
	int i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	cg.ctfScore.valid = qtrue;
	cg.ctfScore.version = 1;

	cg.ctfScore.rra = atoi(CG_Argv(1));
	cg.ctfScore.rya = atoi(CG_Argv(2));
	cg.ctfScore.rga = atoi(CG_Argv(3));
	cg.ctfScore.rmh = atoi(CG_Argv(4));
	cg.ctfScore.rquad = atoi(CG_Argv(5));
	cg.ctfScore.rbs = atoi(CG_Argv(6));
	cg.ctfScore.rregen = atoi(CG_Argv(7));
	cg.ctfScore.rhaste = atoi(CG_Argv(8));
	cg.ctfScore.rinvis = atoi(CG_Argv(9));
	cg.ctfScore.rflag = atoi(CG_Argv(10));
	cg.ctfScore.rmedkit = atoi(CG_Argv(11));

	cg.ctfScore.rquadTime = atoi(CG_Argv(12));
	cg.ctfScore.rbsTime = atoi(CG_Argv(13));
	cg.ctfScore.rregenTime = atoi(CG_Argv(14));
	cg.ctfScore.rhasteTime = atoi(CG_Argv(15));
	cg.ctfScore.rinvisTime = atoi(CG_Argv(16));
	cg.ctfScore.rflagTime = atoi(CG_Argv(17));

	cg.ctfScore.bra = atoi(CG_Argv(18));
	cg.ctfScore.bya = atoi(CG_Argv(19));
	cg.ctfScore.bga = atoi(CG_Argv(20));
	cg.ctfScore.bmh = atoi(CG_Argv(21));
	cg.ctfScore.bquad = atoi(CG_Argv(22));
	cg.ctfScore.bbs = atoi(CG_Argv(23));
	cg.ctfScore.bregen = atoi(CG_Argv(24));
	cg.ctfScore.bhaste = atoi(CG_Argv(25));
	cg.ctfScore.binvis = atoi(CG_Argv(26));
	cg.ctfScore.bflag = atoi(CG_Argv(27));
	cg.ctfScore.bmedkit = atoi(CG_Argv(28));

	cg.ctfScore.bquadTime = atoi(CG_Argv(29));
	cg.ctfScore.bbsTime = atoi(CG_Argv(30));
	cg.ctfScore.bregenTime = atoi(CG_Argv(31));
	cg.ctfScore.bhasteTime = atoi(CG_Argv(32));
	cg.ctfScore.binvisTime = atoi(CG_Argv(33));
	cg.ctfScore.bflagTime = atoi(CG_Argv(34));

	i = 35;

	memset(&cg.ctfScore.playerScore, 0, sizeof(cg.ctfScore.playerScore));

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;

	cg.ctfScore.numPlayerScores = atoi(CG_Argv(i));  i++;

	// can't use version check because some servers (sampler?) are running
	// old server binaries with new cgame dlls

	// 19
	//if (CG_CheckQlVersion(0, 1, 0, 728)) {
	if (CG_Argc() > (cg.ctfScore.numPlayerScores * 19 + 36)  ||  cgs.realProtocol >= 91) {
		cg.teamScores[0] = atoi(CG_Argv(i));  i++;
		cg.teamScores[1] = atoi(CG_Argv(i));  i++;
	}

	// need score index for endgame stats
	cg.numScores = 0;
	cg.scoresValid = qtrue;

	while (1) {
		ctfPlayerScore_t *s;
		int clientNum;
		score_t *oldScore;

		if (!*CG_Argv(i)) {
			break;
		}

		oldScore = &cg.scores[cg.numScores];
		cg.numScores++;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Ctf() invalid client number %d\n", clientNum);
			return;
		}

		s = &cg.ctfScore.playerScore[clientNum];
		s->clientNum = clientNum;
		oldScore->client = clientNum;

		s->team = atoi(CG_Argv(i));  i++;
		oldScore->team = s->team;

		// 2017-06-03 ql/minqlx bug not updating config strings so players appear with wrong team, hack to grab them from scores
		if (cg_useScoresUpdateTeam.integer) {
			if (!cgs.clientinfo[clientNum].override) {
				cgs.clientinfo[clientNum].team = s->team;
			}
		}

		if (s->team == TEAM_RED  ||  s->team == TEAM_BLUE) {
			s->valid = qtrue;
		} else {
			s->valid = qfalse;  //FIXME  maybe??
		}

		if (cgs.realProtocol < 91) {
			s->subscriber = atoi(CG_Argv(i));  i++;
		}
		s->score = atoi(CG_Argv(i));  i++;
		oldScore->score = s->score;
		s->ping = atoi(CG_Argv(i));  i++;
		oldScore->ping = s->ping;
		s->time = atoi(CG_Argv(i));  i++;
		oldScore->time = s->time;
		s->kills = atoi(CG_Argv(i));  i++;
		oldScore->frags = s->kills;
		s->deaths = atoi(CG_Argv(i));  i++;
		oldScore->deaths = s->deaths;
		if (cgs.realProtocol < 91) {
			s->powerups = atoi(CG_Argv(i));  i++;
			oldScore->powerups = s->powerups;
		}
		s->accuracy = atoi(CG_Argv(i));  i++;
		oldScore->accuracy = s->accuracy;
		s->bestWeapon = atoi(CG_Argv(i));  i++;
		oldScore->bestWeapon = s->bestWeapon;
		s->awardImpressive = atoi(CG_Argv(i));  i++;
		oldScore->impressiveCount = s->awardImpressive;
		s->awardExcellent = atoi(CG_Argv(i));  i++;
		oldScore->excellentCount = s->awardExcellent;
		s->awardHumiliation = atoi(CG_Argv(i));  i++;
		oldScore->gauntletCount = s->awardHumiliation;
		s->awardDefend = atoi(CG_Argv(i));  i++;
		oldScore->defendCount = s->awardDefend;
		s->awardAssist = atoi(CG_Argv(i));  i++;
		oldScore->assistCount = s->awardAssist;
		s->captures = atoi(CG_Argv(i));  i++;
		oldScore->captures = s->captures;
		s->perfect = atoi(CG_Argv(i));  i++;
		oldScore->perfect = s->perfect;
		s->alive = atoi(CG_Argv(i));  i++;
		oldScore->alive = s->alive;

		if (s->team == TEAM_RED) {
			redCount++;
			redTotalPing += s->ping;
		} else if (s->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += s->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseScores_Ctf() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseScores_Rr (void)
{
	int i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int n;

	// they forgot score->team ?
	// 2017-06-03 ql/minqlx bug not updating config strings -- can't get team from here then

	cg.scoresValid = qtrue;
	cg.scoresVersion = 1;

	cg.numScores = atoi(CG_Argv(1));

	if (cg.numScores > MAX_CLIENTS) {
		Com_Printf("^3CG_ParseScores_Rr() numScores (%d) > MAX_CLIENTS (%d)\n", cg.numScores, MAX_CLIENTS);
		cg.numScores = MAX_CLIENTS;
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cgs.clientinfo[i].scoreValid = qfalse;
		cg.clientHasScore[i] = qfalse;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	//memset( cg.scores, 0, sizeof( cg.scores ) );

	i = 4;

	for (n = 0;  n < cg.numScores;  n++) {
		score_t *s;
		int clientNum;
		clientInfo_t *ci;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Rr() invalid client number %d\n", clientNum);
			return;
		}
		s = &cg.scores[n];
		ci = &cgs.clientinfo[clientNum];

		s->client = clientNum;
		s->team = ci->team;

		s->score = atoi(CG_Argv(i));  i++;
		ci->score = s->score;

		ci->scoreValid = qtrue;
		cg.clientHasScore[clientNum] = qtrue;

		s->roundScore = atoi(CG_Argv(i));  i++;
		s->ping = atoi(CG_Argv(i));  i++;
		s->time = atoi(CG_Argv(i));  i++;
		s->frags = atoi(CG_Argv(i));  i++;
		s->deaths = atoi(CG_Argv(i));  i++;

		if (cgs.realProtocol < 91) {
			s->powerups = atoi(CG_Argv(i));  i++;
			ci->powerups = s->powerups;
		}

		s->accuracy = atoi(CG_Argv(i));  i++;
		s->bestWeapon = atoi(CG_Argv(i));  i++;
		s->bestWeaponAccuracy = atoi(CG_Argv(i));  i++;
		s->impressiveCount = atoi(CG_Argv(i));  i++;
		s->excellentCount = atoi(CG_Argv(i));  i++;
		s->gauntletCount = atoi(CG_Argv(i));  i++;
		s->defendCount = atoi(CG_Argv(i));  i++;
		s->assistCount = atoi(CG_Argv(i));  i++;
		s->perfect = atoi(CG_Argv(i));  i++;
		s->captures = atoi(CG_Argv(i));  i++;
		s->alive = atoi(CG_Argv(i));  i++;

		if (cgs.realProtocol >= 91) {
			//FIXME check
			s->damageDone = atoi(CG_Argv(i));  i++;
		}
		if (s->team == TEAM_RED) {
			redCount++;
			redTotalPing += s->ping;
		} else if (s->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += s->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

#if 1  //def MPACK
	CG_SetScoreSelection(NULL);
#endif

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseScores_Rr() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseScores_Race (void)
{
	int i;
	int n;

	// they forgot score->team ?

	cg.scoresValid = qtrue;
	cg.scoresVersion = 1;

	cg.numScores = atoi(CG_Argv(1));

	if (cg.numScores > MAX_CLIENTS) {
		Com_Printf("^3CG_ParseScores_Race() numScores (%d) > MAX_CLIENTS (%d)\n", cg.numScores, MAX_CLIENTS);
		cg.numScores = MAX_CLIENTS;
	}

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		cgs.clientinfo[i].scoreValid = qfalse;
		cg.clientHasScore[i] = qfalse;
	}

	i = 2;

	//for (n = 0;  n < cg.numScores;  n++) {
	for (n = 0;  *CG_Argv(i);  n++) {
		score_t *s;
		int clientNum;
		clientInfo_t *ci;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Race() invalid client number %d\n", clientNum);
			return;
		}
		s = &cg.scores[n];
		ci = &cgs.clientinfo[clientNum];

		s->client = clientNum;
		s->team = ci->team;

		if (cgs.realProtocol < 91) {
			// team?

			//FIXME 2017-06-03 ql/minql config string team fix?
			i++;
		}

		s->score = atoi(CG_Argv(i));  i++;
		ci->score = s->score;

		ci->scoreValid = qtrue;
		cg.clientHasScore[clientNum] = qtrue;

		s->ping = atoi(CG_Argv(i));  i++;
		s->time = atoi(CG_Argv(i));  i++;
	}

#if 1  //def MPACK
	CG_SetScoreSelection(NULL);
#endif

	// check sizes
	if (CG_Argc() != i) {
		//FIXME quakelive sends spec scores as well and doesn't report them
		// in number of scores send, can ignore?
		//CG_Printf("^1CG_ParseScores_Race() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseScores_Ft (void)
{
	int i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;

	//memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;
	cg.tdmScore.version = 3;

	cg.tdmScore.rra = atoi(CG_Argv(1));
	cg.tdmScore.rya = atoi(CG_Argv(2));
	cg.tdmScore.rga = atoi(CG_Argv(3));
	cg.tdmScore.rmh = atoi(CG_Argv(4));
	cg.tdmScore.rquad = atoi(CG_Argv(5));
	cg.tdmScore.rbs = atoi(CG_Argv(6));
	cg.tdmScore.rregen = atoi(CG_Argv(7));
	cg.tdmScore.rhaste = atoi(CG_Argv(8));
	cg.tdmScore.rinvis = atoi(CG_Argv(9));

	cg.tdmScore.rquadTime = atoi(CG_Argv(10));
	cg.tdmScore.rbsTime = atoi(CG_Argv(11));
	cg.tdmScore.rregenTime = atoi(CG_Argv(12));
	cg.tdmScore.rhasteTime = atoi(CG_Argv(13));
	cg.tdmScore.rinvisTime = atoi(CG_Argv(14));

	cg.tdmScore.bra = atoi(CG_Argv(15));
	cg.tdmScore.bya = atoi(CG_Argv(16));
	cg.tdmScore.bga = atoi(CG_Argv(17));
	cg.tdmScore.bmh = atoi(CG_Argv(18));
	cg.tdmScore.bquad = atoi(CG_Argv(19));
	cg.tdmScore.bbs = atoi(CG_Argv(20));
	cg.tdmScore.bregen = atoi(CG_Argv(21));
	cg.tdmScore.bhaste = atoi(CG_Argv(22));
	cg.tdmScore.binvis = atoi(CG_Argv(23));

	cg.tdmScore.bquadTime = atoi(CG_Argv(24));
	cg.tdmScore.bbsTime = atoi(CG_Argv(25));
	cg.tdmScore.bregenTime = atoi(CG_Argv(26));
	cg.tdmScore.bhasteTime = atoi(CG_Argv(27));
	cg.tdmScore.binvisTime = atoi(CG_Argv(28));

	cg.tdmScore.numPlayerScores = atoi(CG_Argv(29));

	i = 30;

	// can't use version check because some servers (sampler?) are running
	// old server binaries with new cgame dlls

	// 16
	//if (CG_CheckQlVersion(0, 1, 0, 728)) {
	if (1) {  //(CG_Argc() > (cg.tdmScore.numPlayerScores * 16 + 31)) {
		cg.teamScores[0] = atoi(CG_Argv(i));  i++;
		cg.teamScores[1] = atoi(CG_Argv(i));  i++;
	}

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;

	memset(&cg.tdmScore.playerScore, 0, sizeof(cg.tdmScore.playerScore));

	cg.numScores = 0;

	// 19 per player?
	while (1) {
		tdmPlayerScore_t *ts;
		int clientNum;
		int team;
		score_t *oldScore;

		if (!*CG_Argv(i)) {
			break;
		}

		// end game scoreboard stats uses index into cg.scores instead of
		// client number
		oldScore = &cg.scores[cg.numScores];
		cg.numScores++;

		clientNum = atoi(CG_Argv(i));  i++;
		if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
			CG_Printf("^1CG_ParseScores_Ft() invalid client number %d\n", clientNum);
			return;
		}

		oldScore->client = clientNum;

		team = atoi(CG_Argv(i));  i++;

		//Com_Printf("client %d  team %d\n", clientNum, team);

		// 2017-06-03 ql/minqlx bug not updating config strings so players appear with wrong team, hack to grab them from scores
		if (cg_useScoresUpdateTeam.integer) {
			if (!cgs.clientinfo[clientNum].override) {
				cgs.clientinfo[clientNum].team = team;
			}
		}


		ts = &cg.tdmScore.playerScore[clientNum];

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			ts->valid = qtrue;
		} else {
			//FIXME maybe??
			ts->valid = qfalse;
		}

		ts->clientNum = clientNum;  // 32
		ts->team = team;  // 33
		oldScore->team = team;
		if (cgs.realProtocol < 91) {
			ts->subscriber = atoi(CG_Argv(i));  i++;  // 34  //FIXME no, or unused
		}
		ts->score = atoi(CG_Argv(i));  i++;  // 35
		oldScore->score = ts->score;
		ts->ping = atoi(CG_Argv(i));  i++;  // 36
		oldScore->ping = ts->ping;
		ts->time = atoi(CG_Argv(i));  i++;  // 37
		oldScore->time = ts->time;
		ts->kills = atoi(CG_Argv(i));  i++;  // 38
		oldScore->frags = ts->kills;
		ts->deaths = atoi(CG_Argv(i));  i++;  // 39
		oldScore->deaths = ts->deaths;
		ts->accuracy = atoi(CG_Argv(i));  i++;  // 40
		oldScore->accuracy = ts->accuracy;
		ts->bestWeapon = atoi(CG_Argv(i));  i++;  // 41
		oldScore->bestWeapon = ts->bestWeapon;
		ts->awardImpressive = atoi(CG_Argv(i));  i++;  // 42
		oldScore->impressiveCount = ts->awardImpressive;
		ts->awardExcellent = atoi(CG_Argv(i));  i++;  // 43
		oldScore->excellentCount = ts->awardExcellent;
		ts->awardHumiliation = atoi(CG_Argv(i));  i++;  // 44
		oldScore->gauntletCount = ts->awardHumiliation;
		ts->thaws = atoi(CG_Argv(i));  i++;  // 45
		ts->tkd = atoi(CG_Argv(i));  i++;  // 46

		//Com_Printf("cn %d  new1  %d  '%s'\n", clientNum, atoi(CG_Argv(i)), cgs.clientinfo[clientNum].name);
		i++;  //FIXME unknown?
		ts->damageDone = atoi(CG_Argv(i));  i++;  // 47, looks like 48

		ts->alive = atoi(CG_Argv(i));  i++;
		oldScore->alive = ts->alive;
		//Com_Printf("cn %d  new2  %d\n", clientNum, atoi(CG_Argv(i)));
		//i++;  //FIXME ?

		//FIXME one of the missing ones is sp->alive
		//FIXME the other is thaws

		if (ts->team == TEAM_RED) {
			redCount++;
			redTotalPing += ts->ping;
		} else if (ts->team == TEAM_BLUE) {
			blueCount++;
			blueTotalPing += ts->ping;
		}
	}

	if (redCount) {
		cg.avgRedPing = redTotalPing / redCount;
	} else {
		cg.avgRedPing = 0;
	}

	if (blueCount) {
		cg.avgBluePing = blueTotalPing / blueCount;
	} else {
		cg.avgBluePing = 0;
	}

	// check sizes
	if (CG_Argc() != i) {
		CG_Printf("^1CG_ParseScores_Ft() argc (%d) != %d\n", CG_Argc(), i);
	}
}

static void CG_ParseDScores (void)
{
	int i;
	int n;
	int j;
	int maxWeapons;
	//FIXME hack
	static int oldDuelPlayer1 = DUEL_PLAYER_INVALID;
	static int oldDuelPlayer2 = DUEL_PLAYER_INVALID;
	static qboolean oldScoreValid = qfalse;

	//oldDuelPlayer1 = cg.duelPlayer1;
	//oldDuelPlayer2 = cg.duelPlayer2;
	//oldScoreValid = cg.duelScoresValid;

	CG_SetDuelPlayers();
	cg.duelScoresValid = qtrue;
	cg.duelScoresServerTime = cg.snap->serverTime;

	//CG_Printf("dscores argc %d\n", CG_Argc());

	if (cg.duelPlayer1 == DUEL_PLAYER_INVALID  ||  cg.duelPlayer2 == DUEL_PLAYER_INVALID) {
		//CG_Printf("^5d1 %d  d2 %d  oldvalid %d\n", oldDuelPlayer1, oldDuelPlayer2, oldScoreValid);

		if ((cg.snap->ps.pm_type == PM_INTERMISSION  ||  !cg.warmup)  &&  oldScoreValid  &&  oldDuelPlayer1 != DUEL_PLAYER_INVALID  &&  oldDuelPlayer2 != DUEL_PLAYER_INVALID) {
			// one player quit match
			//Com_Printf("^6duel was forfeited\n");
			cg.duelPlayer1 = oldDuelPlayer1;
			cg.duelPlayer2 = oldDuelPlayer2;
			return;
		}
	}

	oldDuelPlayer1 = cg.duelPlayer1;
	oldDuelPlayer2 = cg.duelPlayer2;
	oldScoreValid = qtrue;

	n = 1;
	for (i = 0;  i < 2;  i++) {
		duelScore_t *ds;

		ds = &cg.duelScores[i];

#if 0
		if (!*CG_Argv(n)) {
			CG_Printf("^4duel forfeit\n");
		}
#endif

		ds->ping = atoi(CG_Argv(n));  n++;
		ds->kills = atoi(CG_Argv(n));  n++;
		ds->deaths = atoi(CG_Argv(n));  n++;
		ds->accuracy = atoi(CG_Argv(n));  n++;
		ds->damage = atoi(CG_Argv(n));  n++;
		ds->awardImpressive = atoi(CG_Argv(n));  n++;  // 6
		ds->awardExcellent = atoi(CG_Argv(n));  n++;  // 7
		ds->awardHumiliation = atoi(CG_Argv(n));  n++;  // 8
		//Com_Printf("%d  %s\n", i, CG_Argv(n));  n++;  // 9
		//Com_Printf("%d  %s\n", i, CG_Argv(n)); n++;  // 10
		n++;  // 9
		n++;  // 10
		ds->redArmorPickups = atoi(CG_Argv(n));  n++;
		ds->redArmorTime = atof(CG_Argv(n));  n++;
		ds->yellowArmorPickups = atoi(CG_Argv(n));  n++;
		ds->yellowArmorTime = atof(CG_Argv(n));  n++;
		ds->greenArmorPickups = atoi(CG_Argv(n));  n++;
		ds->greenArmorTime = atof(CG_Argv(n));  n++;
		ds->megaHealthPickups = atoi(CG_Argv(n));  n++;
		ds->megaHealthTime = atof(CG_Argv(n));  n++;

		maxWeapons = WP_NUM_WEAPONS;

		for (j = 1;  j < maxWeapons;  j++) {
			duelWeaponStats_t *ws;

			ws = &ds->weaponStats[j];

			ws->hits = atoi(CG_Argv(n));  n++;
			ws->atts = atoi(CG_Argv(n));  n++;
			ws->accuracy = atoi(CG_Argv(n));  n++;
			ws->damage = atoi(CG_Argv(n));  n++;
			ws->kills = atoi(CG_Argv(n));  n++;
		}
	}

	if (cg.duelPlayer1 != DUEL_PLAYER_INVALID) {
		cg.duelScores[0].clientNum = cg.duelPlayer1;
		if (cgs.clientinfo[cg.duelPlayer1].infoValid) {
			//Q_strncpyz(cg.duelScores[0].name, cgs.clientinfo[cg.duelPlayer1].name, sizeof(cg.duelScores[0].name));
			cg.duelScores[0].ci = cgs.clientinfo[cg.duelPlayer1];
		}
	}
	if (cg.duelPlayer2 != DUEL_PLAYER_INVALID) {
		cg.duelScores[1].clientNum = cg.duelPlayer2;
		if (cgs.clientinfo[cg.duelPlayer2].infoValid) {
			//Q_strncpyz(cg.duelScores[1].name, cgs.clientinfo[cg.duelPlayer2].name, sizeof(cg.duelScores[1].name));
			cg.duelScores[1].ci = cgs.clientinfo[cg.duelPlayer2];
		}
	}
}

static void CG_RaceInit (void)
{
	int i;
	playerEntity_t *pe;

	pe = &cg.predictedPlayerEntity.pe;

	pe->raceStartTime = 0;
	pe->raceCheckPointNum = 0;
	pe->raceCheckPointNextEnt = 0;
	pe->raceCheckPointTime = 0;
	pe->raceEndTime = 0;

	for (i = 0;  i < MAX_CLIENTS;  i++) {
		pe = &cg_entities[i].pe;

		pe->raceStartTime = 0;
		pe->raceCheckPointNum = 0;
		pe->raceCheckPointNextEnt = 0;
		pe->raceCheckPointTime = 0;
		pe->raceEndTime = 0;
	}
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand( void ) {
	const char	*cmd;
	char		text[MAX_SAY_TEXT];
	int i;
	char args[4][MAX_QPATH];

	cmd = CG_Argv(0);

	if (cg_debugServerCommands.integer == 1) {
		Com_Printf("^3command: '%s'\n", cmd);
		i = 1;
		while (1) {
			if (!*CG_Argv(i)) {
				break;
			}
			CG_Printf("argv[%d]: %s\n", i, CG_Argv(i));
			i++;
		}
	}

	cmd = CG_Argv(0);

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	// camera script
	if ( !strcmp( cmd, "startCam" ) ) {
		//CG_StartidCamera( CG_Argv(1), atoi(CG_Argv(2)) );
		return;
	}
	if ( !strcmp( cmd, "stopCam" ) ) {
		//cg.cameraMode = qfalse;
		return;
	}
	// end camera script

	if ( !strcmp( cmd, "cp" ) ) {
		//Com_Printf("^5---  '%s'\n", CG_Argv(1));
		if (cg_serverCenterPrint.integer) {
			CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		}
		if (cg_serverCenterPrintToChat.integer) {
			CG_PrintToScreen("%s", CG_Argv(1));
		}
		if (cg_serverCenterPrintToConsole.integer) {
			CG_Printf("%s", CG_Argv(1));
		}
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		if (cg_serverPrint.integer) {
			CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		}
		if (cg_serverPrintToChat.integer) {
			CG_PrintToScreen("%s", CG_Argv(1));
		}
		if (cg_serverPrintToConsole.integer) {
			CG_Printf("%s", CG_Argv(1));
		}

#if 1 //def MPACK
		cmd = CG_Argv(1);			// yes, this is obviously a hack, but so is the way we hear about
									// votes passing or failing

		if (!Q_stricmpn(cmd, "vote failed", 11)  &&  cg_audioAnnouncerVote.integer) {
			CG_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "team vote failed", 16)  &&  cg_audioAnnouncerTeamVote.integer) {
			CG_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "vote passed", 11)  &&  cg_audioAnnouncerVote.integer) {
			CG_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "team vote passed", 16)  &&  cg_audioAnnouncerTeamVote.integer) {
			CG_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		} else if (cgs.protocol == PROTOCOL_QL  &&  CG_IsDuelGame(cgs.gametype)  &&  !Q_stricmpn(cmd, "Game has been forfeited", 23)) {
			//FIXME bad hack
			Com_Printf("^5forfeit...\n");
			cg.duelForfeit = qtrue;
			if (!cgs.clientinfo[cg.duelPlayer1].infoValid  ||  cgs.clientinfo[cg.duelPlayer1].team  != TEAM_FREE) {
				cg.duelPlayerForfeit = 1;
			} else if (!cgs.clientinfo[cg.duelPlayer2].infoValid  ||  cgs.clientinfo[cg.duelPlayer2].team  != TEAM_FREE) {
				cg.duelPlayerForfeit = 2;
			} else {
				CG_Printf("^1unknown player forfeited\n");
			}
		}
#endif
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( CG_IsTeamGame(cgs.gametype) && cg_teamChatsOnly.integer ) {
			return;
		}

		if (cg_chatBeep.integer) {
			if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
				CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
				cg.lastChatBeepTime = cg.time;
			}
		}
		Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		CG_Printf( "%s\n", text );
		CG_PrintToScreen("%s", text);
		//Q_strncpyz(cg.lastChatString, text, sizeof(cg.lastChatString));
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		//FIXME wolfcam_following
		if (cg.snap->ps.clientNum != cg.clientNum) {
			// it's a spec demo, team is other specs
			if (cg_chatBeep.integer) {
				if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
					CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
					cg.lastChatBeepTime = cg.time;
				}
			}
		} else if (cg_teamChatBeep.integer) {
			if (cg.time - cg.lastTeamChatBeepTime >= (cg_teamChatBeepMaxTime.value * 1000)) {
				CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
				cg.lastTeamChatBeepTime = cg.time;
			}
		}
		Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		if (cgs.osp) {  //FIXME option and translate
			CG_FilterOspTeamChatLocation(text);
		}
		CG_AddToTeamChat( text );
		CG_Printf( "%s\n", text );
		CG_PrintToScreen("%s", text);
		//Q_strncpyz(cg.lastChatString, text, sizeof(cg.lastChatString));
		return;
	}

#ifdef MISSIONPACK
	if ( !strcmp( cmd, "vchat" ) ) {
		CG_VoiceChat( SAY_ALL );
		return;
	}

	if ( !strcmp( cmd, "vtchat" ) ) {
		CG_VoiceChat( SAY_TEAM );
		return;
	}

	if ( !strcmp( cmd, "vtell" ) ) {
		CG_VoiceChat( SAY_TELL );
		return;
	}
#endif

	if ( !strcmp( cmd, "scores" ) ) {
		//Com_Printf("^3---------------scores\n");
		//memset(cgs.newConnectedClient, 0, sizeof(cgs.newConnectedClient));
		CG_ParseScores();
		Wolfcam_ScoreData();
		CG_BuildSpectatorString();
		//Com_Printf("^3scores\n");
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		if (cgs.realProtocol >= 91) {
			CG_ParseTeamInfo_91();
		} else {
			CG_ParseTeamInfo();
		}
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	if ( Q_stricmp (cmd, "remapShader") == 0 ) {
		if (trap_Argc() == 4) {
			//CG_Printf("remapshader %s  %s\n", CG_Argv(1), CG_Argv(2));
			//trap_R_RemapShader(CG_Argv(1), CG_Argv(2), CG_Argv(3), qfalse);
			// don't use CG_Argv() like above (stacking multiple and passing to trap)
			for (i = 1;  i < 4;  i++) {
				Q_strncpyz(args[i - 1], CG_Argv(i), sizeof(args[i - 1]));
			}
			trap_R_RemapShader(args[0], args[1], args[2], qfalse, qfalse);
		}
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddefered" ) ) {	// FIXME: spelled wrong, but not changing for demo
		CG_Printf("servercmd: loaddefered\n");
		CG_LoadDeferredPlayers();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		cg.levelShot = qtrue;
		return;
	}

	if (cgs.cpma) {
		if (!Q_stricmp(cmd, "mstats")) {
			if (CG_CpmaParseMstats(qfalse)) {
				return;
			}
		}

		if (!Q_stricmp(cmd, "mstatsa")) {
			// during intermission, same as mstats
			if (CG_CpmaParseMstats(qtrue)) {
				return;
			}
		}

		if (!Q_stricmp(cmd, "dmscores")) {
			if (CG_CpmaParseDmscores()) {
				return;
			}
		}
		if (!Q_stricmp(cmd, "mm2")) {
			if (CG_CpmaMM2()) {
				return;
			}
		}
		if (!Q_stricmp(cmd, "xscores")) {
			if (CG_CpmaXscores()) {
				Wolfcam_ScoreData();
				return;
			}
		}

		// xstats2[a] same as mstats[a], sent at the end of the game
		if (!Q_stricmp(cmd, "xstats2")) {
			if (CG_CpmaParseMstats(qfalse)) {
				return;
			}
		}

		if (!Q_stricmp(cmd, "xstats2a")) {
			if (CG_CpmaParseMstats(qtrue)) {
				return;
			}
		}

		if (!Q_stricmp(cmd, "specs")) {
			// spec data with info indicating whether they are referee
			return;
		}

		if (!Q_stricmp(cmd, "duelendscores")) {
			if (CG_CpmaParseDuelEndScores()) {
				return;
			}
		}

		// ignored commands

		if (!Q_stricmp(cmd, "ss")) {
			// screenshot
			return;
		}

		if (!Q_stricmp(cmd, "stuff")) {
			// misc commands, stoprecord ...
			return;
		}
	}

	if (cgs.protocol == PROTOCOL_QL) {
		// crash tutorial
		if (!Q_stricmp(cmd, "playSound")) {
			if (!cg_ignoreServerPlaySound.integer) {
				trap_SendConsoleCommand(va("play %s\n", CG_Argv(1)));
				//CG_Printf("^5playsound: '%s'\n", CG_Argv(1));
			}
			return;
		}

		if ( !strcmp( cmd, "bchat" ) ) {  //FIXME bot chat ??, crash tutorial?
			if ( !cg_teamChatsOnly.integer ) {
				if (cg_chatBeep.integer) {
					if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
						CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
						cg.lastChatBeepTime = cg.time;
					}
				}
				Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
				//CG_RemoveChatEscapeChar( text );
				CG_Printf( "%s\n", text );
				CG_PrintToScreen("%s", text);
				//Q_strncpyz(cg.lastChatString, text, sizeof(cg.lastChatString));
			}
			return;
		}

		if (!Q_stricmp(cmd, "scores_duel")) {
			CG_ParseScores_Duel();
			return;
		}
		if (!Q_stricmp(cmd, "scores_tdm")) {
			CG_ParseScores_Tdm();
			return;
		}
		if (!Q_stricmp(cmd, "scores_ca")) {
			CG_ParseScores_Ca();
			return;
		}
		if (!Q_stricmp(cmd, "scores_ctf")) {
			CG_ParseScores_Ctf();
			return;
		}
		if (!Q_stricmp(cmd, "scores_rr")) {
			CG_ParseScores_Rr();
			return;
		}
		if (!Q_stricmp(cmd, "scores_race")) {
			CG_ParseScores_Race();
			return;
		}
		if (!Q_stricmp(cmd, "scores_ft")) {
			CG_ParseScores_Ft();
			return;
		}
		if (!Q_stricmp(cmd, "scores_ffa")) {
			CG_ParseScores_Ffa();
			return;
		}

		if (!Q_stricmp(cmd, "dscores")) {
			CG_ParseDScores();
			return;
		}

		if (!Q_stricmp(cmd, "cascores")) {
			CG_ParseCaScores();
			return;
		}

		if (!Q_stricmp(cmd, "castats")) {
			CG_ParseCaStats();
			return;
		}

		if (!Q_stricmp(cmd, "tdmscores")) {
			CG_ParseTdmScores();
			return;
		}

		if (!Q_stricmp(cmd, "tdmstats")) {
			//Com_Printf("tdmstats\n");
			CG_ParseTdmStats();
			return;
		}

		if (!Q_stricmp(cmd, "tdmscores2")) {
			//Com_Printf("^3tdmscores2\n");
			CG_ParseTdmScores2();
			return;
		}

		if (!Q_stricmp(cmd, "ctfscores")) {
			CG_ParseCtfScores();
			return;
		}

		if (!Q_stricmp(cmd, "ctfstats")) {
			CG_ParseCtfStats();
			return;
		}

		if ( !strcmp( cmd, "rrscores" ) ) {
			//Com_Printf("^3---------------rrscores\n");
			//memset(cgs.newConnectedClient, 0, sizeof(cgs.newConnectedClient));
			CG_ParseRRScores();
			Wolfcam_ScoreData();
			CG_BuildSpectatorString();
			return;
		}

		// 2015-07-06 renamed scores_ad
		if (!strcmp(cmd, "adscores")  ||  !strcmp(cmd, "scores_ad")) {
			CG_ParseCtfsRoundScores();
			return;
		}

		// 2016-08-05 might be a ql or minqlx bug, sometimes server starts sending this instead of ca scores
		if (!strcmp(cmd, "smscores")) {
			CG_ParseSmScores();
			return;
		}

		if (!Q_stricmp(cmd, "rcmd")) {
			CG_Printf("^3remote command '%s'\n", CG_ArgsFrom(1));
			return;
		}

		if (!strcmp(cmd, "acc")) {
			int i;
			int maxWeapons;

			cg.serverAccuracyStatsTime = cg.time;
			cg.serverAccuracyStatsClientNum = cg.snap->ps.clientNum;

			maxWeapons = WP_NUM_WEAPONS;

			//CG_Printf("acc: \n");
			for (i = WP_NONE;  i < maxWeapons;  i++) {
				//CG_Printf("  %s    %s\n", CG_Argv(i + 1), weapNames[i]);
				cg.serverAccuracyStats[i] = atoi(CG_Argv(i + 1));
			}
			return;
		}

		if (!strcmp(cmd, "pcp")) {  // pause center print??, player called pause?
			// timeout
			//CG_AddBufferedSound(cgs.media.klaxon1);
			CG_StartLocalSound( cgs.media.klaxon1, CHAN_LOCAL_SOUND );
			if (cg_serverCenterPrint.integer) {
				CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			}
			if (cg_serverCenterPrintToChat.integer) {
				CG_PrintToScreen("%s", CG_Argv(1));
			}
#if 1  //FIXME
			if (cg_serverCenterPrintToConsole.integer) {
				CG_Printf("%s", CG_Argv(1));
			}
#endif
			//CG_Printf("%s\n", CG_Argv(1));
			CG_ResetTimedItemPickupTimes();  //FIXME take out eventually
			return;
		}

		if (!strcmp(cmd, "race_init")) {
			CG_RaceInit();
			return;
		}
	}

	// unknown:

	CG_Printf( "Unknown client game command: %s\n", CG_Argv(0) );
	if (cg_debugServerCommands.integer == 2) {
		i = 1;
		while (1) {
			if (!*CG_Argv(i)) {
				break;
			}
			CG_Printf("argv[%d]: %s\n", i, CG_Argv(i));
			i++;
		}
	}
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}

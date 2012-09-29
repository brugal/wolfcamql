// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"
//#include "../../ui/menudef.h" // bk001205 - for Q3_ui as well
#include "wolfcam_local.h"

static void CG_CpmaParseScores (void);
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

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void ) {
	int		i;
	//int powerups;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int SCSIZE;

	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	memset( cg.scores, 0, sizeof( cg.scores ) );

	//#define SCSIZE 18  // was 14

	if (cgs.protocol == PROTOCOL_QL) {
		SCSIZE = 18;
	} else {
		SCSIZE = 14;
	}

	for ( i = 0 ; i < cg.numScores ; i++ ) {
		//int t1, t2, t3, t4;

		cg.scores[i].client = atoi( CG_Argv( i * SCSIZE + 4 ) );
		cg.scores[i].score = atoi( CG_Argv( i * SCSIZE + 5 ) );
		cg.scores[i].ping = atoi( CG_Argv( i * SCSIZE + 6 ) );
		cg.scores[i].time = atoi( CG_Argv( i * SCSIZE + 7 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * SCSIZE + 8 ) );
		cg.scores[i].powerups = atoi( CG_Argv( i * SCSIZE + 9 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * SCSIZE + 10));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * SCSIZE + 11));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * SCSIZE + 12));
		cg.scores[i].guantletCount = atoi(CG_Argv(i * SCSIZE + 13));
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

#if 1  //def MISSIONPACK
	CG_SetScoreSelection(NULL);
#endif
//#undef SCSIZE
}

static void CG_ParseRRScores (void)
{
	int		i;
	int redCount = 0;
	int blueCount = 0;
	int redTotalPing = 0;
	int blueTotalPing = 0;
	int SCSIZE;

	// they forgot score->team ?

	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	cg.avgRedPing = 0;
	cg.avgBluePing = 0;
	memset( cg.scores, 0, sizeof( cg.scores ) );


	SCSIZE = 19;

	for ( i = 0 ; i < cg.numScores ; i++ ) {
		//int t1, t2, t3, t4;

		cg.scores[i].client = atoi( CG_Argv( i * SCSIZE + 4 ) );
		cg.scores[i].score = atoi( CG_Argv( i * SCSIZE + 5 ) );
		// 6
		cg.scores[i].ping = atoi( CG_Argv( i * SCSIZE + 7 ) );
		cg.scores[i].time = atoi( CG_Argv( i * SCSIZE + 8 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * SCSIZE + 9 ) );
		cg.scores[i].powerups = atoi( CG_Argv( i * SCSIZE + 10 ) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * SCSIZE + 11));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * SCSIZE + 12));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * SCSIZE + 13));
		cg.scores[i].guantletCount = atoi(CG_Argv(i * SCSIZE + 14));
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

#if 1  //def MISSIONPACK
	CG_SetScoreSelection(NULL);
#endif
}

static void CG_ParseCaScores (void)
{
	int i;
	int n;
	tdmPlayerScore_t *s;

	memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

	cg.tdmScore.valid = qtrue;

	i = 0;

	while (1) {
		if (!*CG_Argv(i + 1)) {
			break;
		}
		n = atoi(CG_Argv(i + 1));
		s = &cg.tdmScore.playerScore[n];
		s->valid = qtrue;

		s->team = atoi(CG_Argv(i + 2));
		s->subscriber = atoi(CG_Argv(i + 3));
		s->damageDone = atoi(CG_Argv(i + 4));
		s->accuracy = atoi(CG_Argv(i + 5));

		i += 5;
	}
}

static void CG_ParseTdmScores (void)
{
	int i;
	int n;
	int team;

	memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

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
		n = atoi(CG_Argv(i));
		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.tdmScore.playerScore[n].valid = qtrue;

			cg.tdmScore.playerScore[n].team = atoi(CG_Argv(i + 1));
			cg.tdmScore.playerScore[n].tks = atoi(CG_Argv(i + 2));
			cg.tdmScore.playerScore[n].tkd = atoi(CG_Argv(i + 3));
			cg.tdmScore.playerScore[n].damageDone = atoi(CG_Argv(i + 4));

			//Com_Printf("%d  (%d %d %d   -- %d)\n", n, cg.tdmScore.playerScore[n].team, cg.tdmScore.playerScore[n].tkd, cg.tdmScore.playerScore[n].tks, cg.tdmScore.playerScore[n].damageDone);

		}

		i += 5;
	}
}

static void CG_ParseTdmScores2 (void)
{
	int i;
	int n;
	int team;

	memset(&cg.tdmScore, 0, sizeof(cg.tdmScore));

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
		n = atoi(CG_Argv(i));
		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.tdmScore.playerScore[n].valid = qtrue;

			cg.tdmScore.playerScore[n].team = atoi(CG_Argv(i + 1));
			cg.tdmScore.playerScore[n].subscriber = atoi(CG_Argv(i + 2));
			cg.tdmScore.playerScore[n].tks = atoi(CG_Argv(i + 3));
			cg.tdmScore.playerScore[n].tkd = atoi(CG_Argv(i + 4));
			cg.tdmScore.playerScore[n].damageDone = atoi(CG_Argv(i + 5));

			//Com_Printf("%d  (%d %d %d %d   -- %d)\n", n, cg.tdmScore.playerScore[n].u2, cg.tdmScore.playerScore[n].u3, cg.tdmScore.playerScore[n].u4, cg.tdmScore.playerScore[n].u5, cg.tdmScore.playerScore[n].damageDone);

			//Com_Printf("%d  %d  %d\n", n, cg.tdmScore.playerScore[n].damageDone, cg.tdmScore.playerScore[n].subscriber);
			//Com_Printf("%d  (%d %d %d %d  %d)\n", n, cg.tdmScore.playerScore[n].team, cg.tdmScore.playerScore[n].subscriber, cg.tdmScore.playerScore[n].tks, cg.tdmScore.playerScore[n].tkd, cg.tdmScore.playerScore[n].damageDone);
		}

		i += 6;
	}
}

static void CG_ParseCtfScores (void)
{
	int i;
	int n;
	int team;

	memset(&cg.ctfScore, 0, sizeof(cg.ctfScore));

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
		n = atoi(CG_Argv(i));
		team = atoi(CG_Argv(i + 1));

		if (team == TEAM_RED  ||  team == TEAM_BLUE) {
			cg.ctfScore.playerScore[n].valid = qtrue;

			cg.ctfScore.playerScore[n].team = atoi(CG_Argv(i + 1));
			cg.ctfScore.playerScore[n].subscriber = atoi(CG_Argv(i + 2));
			//cg.ctfScore.playerScore[n].tks = atoi(CG_Argv(i + 3));
			//cg.ctfScore.playerScore[n].tkd = atoi(CG_Argv(i + 4));
			//cg.ctfScore.playerScore[n].damageDone = atoi(CG_Argv(i + 5));

			//Com_Printf("%d  (%d %d %d %d   -- %d)\n", n, cg.ctfScore.playerScore[n].u2, cg.ctfScore.playerScore[n].u3, cg.ctfScore.playerScore[n].u4, cg.ctfScore.playerScore[n].u5, cg.ctfScore.playerScore[n].damageDone);

			//Com_Printf("%d  %d  %d\n", n, cg.ctfScore.playerScore[n].damageDone, cg.ctfScore.playerScore[n].subscriber);
			//Com_Printf("%d  (%d %d %d %d  %d)\n", n, cg.ctfScore.playerScore[n].team, cg.ctfScore.playerScore[n].subscriber, cg.ctfScore.playerScore[n].tks, cg.ctfScore.playerScore[n].tkd, cg.ctfScore.playerScore[n].damageDone);
		}

		i += 3;
	}
}

static void CG_ParseCaStats (void)
{
	int clientNum;
	caStats_t *s;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return;
	}

	s = &cg.caStats[clientNum];
	s->valid = qtrue;
	s->clientNum = clientNum;
	s->damageDone = atoi(CG_Argv(2));
	s->damageReceived = atoi(CG_Argv(3));
	// 4  --  ??  0  gauntAccuracy?
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

	// the rest ....  
}


static void CG_ParseTdmStats (void)
{
	//int i;
	int clientNum;
	tdmStats_t *ts;

	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0  ||  clientNum >= MAX_CLIENTS) {
		return;
	}

	ts = &cg.tdmStats[clientNum];
	ts->valid = qtrue;
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
static void CG_ParseTeamInfo( void ) {
	int		i;
	int		client;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );
	//Com_Printf("CG_ParseTeamInfo numSortedTeamPlayers %d\n", numSortedTeamPlayers);

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( CG_Argv( i * 6 + 2 ) );

		sortedTeamPlayers[i] = client;
		//Com_Printf("%s\n", cgs.clientinfo[client].name);

		cgs.clientinfo[ client ].location = atoi( CG_Argv( i * 6 + 3 ) );
		cgs.clientinfo[ client ].health = atoi( CG_Argv( i * 6 + 4 ) );
		cgs.clientinfo[ client ].armor = atoi( CG_Argv( i * 6 + 5 ) );
		cgs.clientinfo[ client ].curWeapon = atoi( CG_Argv( i * 6 + 6 ) );
		cgs.clientinfo[ client ].powerups = atoi( CG_Argv( i * 6 + 7 ) );
	}
}

/*
  0.1.0.578  linux-i386 May 16 2012 17:53:25
  0.1.0.577  linux-i386 May 11 2012 16:18:15
       * last standing vo includes team number
  0.1.0.571  QuakeLive linux-i386 Apr 17 2012 14:51:00
  0.1.0.495  QuakeLive  0.1.0.495 linux-i386 Dec 14 2011 16:08:54
       * new ctf scoreboard
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

void CG_ParseVersion (const char *info)
{
	char *val;
	char *p;
	char buffer[1024];
	int bufferPos;
	int i;

	val = Info_ValueForKey(info, "version");
	//Com_Printf("version:  %s\n", val);
	// vote failed
	// quakelive  9

	if (cg.demoPlayback) {
		if (!Q_stricmpn(val, "QuakeLive", strlen("QuakeLive"))) {
			cgs.isQuakeLiveDemo = qtrue;
		} else {
			cgs.isQuakeLiveDemo = qfalse;
		}
	}

	for (i = 0;  i < 4;  i++) {
		cgs.qlversion[i] = 0;
	}

	// advance to first num
	p = val;
	while (p[0] < '0'  ||  p[0] > '9') {
		if (p[0] == '\0') {
			return;
		}
		p++;
	}

	for (i = 0;  i < 4;  i++) {
		if (p[0] == '\0') {
			return;
		}
		bufferPos = 0;
		while (p[0] >= '0'  &&  p[0] <= '9') {
			if (p[0] == '\0'  ||  bufferPos >= 1022) {
				return;
			}
			buffer[bufferPos] = p[0];
			bufferPos++;
			p++;
		}

		if (p[0] == '\0') {
			return;
		}
		p++;  // skip '.'
		buffer[bufferPos] = '\0';
		cgs.qlversion[i] = atoi(buffer);
	}

	//Com_Printf("QuakeLive version:  %d  %d  %d  %d\n", cgs.qlversion[0], cgs.qlversion[1], cgs.qlversion[2], cgs.qlversion[3]);
}

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
			//cg.useTeamSkins = qtrue;
		} else {
			*skin++ = 0;
			//cg.useTeamSkins = qfalse;
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
			//cg.useTeamSkins = qtrue;
		} else {
			*skin++ = 0;
			//cg.useTeamSkins = qfalse;
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
void CG_ParseServerinfo (qboolean firstCall)
{
	const char	*info;
	char	*mapname;
	int val, i, numClients;
	int df;
	qboolean instaGib;
	const char *gametypeConfig = NULL;
	const char *cinfo;

	info = CG_ConfigString( CS_SERVERINFO );

	//FIXME PROTOCOL_Q3
	cgs.gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	if (cgs.protocol == PROTOCOL_Q3) {
		if (cgs.cpma) {
			switch (cgs.gametype) {
			case 4:
				cgs.gametype = GT_CTF;
				break;
			case 5:
				//cgs.gametype = GT_1FCTF;
				cgs.gametype = GT_CA;
				break;
			case 6:
				//cgs.gametype = GT_OBELISK;
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
				break;
			}
		} else {
			switch (cgs.gametype) {
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
	}

	trap_Cvar_Set("cg_gametype", va("%i", cgs.gametype));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));

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
		cinfo = CG_ConfigStringNoConvert(CSCPMA_GAMESTATE);
		cgs.fraglimit = atoi(Info_ValueForKey(cinfo, "sl"));
		cgs.timelimit = atoi(Info_ValueForKey(cinfo, "tl"));
		cgs.realTimelimit = cgs.timelimit;
		cgs.levelStartTime = atoi(Info_ValueForKey(cinfo, "ts"));
		cg.warmup = atoi(Info_ValueForKey(cinfo, "tw"));
	} else {
		cgs.scorelimit = atoi( Info_ValueForKey( info, "scorelimit" ) );
		cgs.fraglimit = atoi( Info_ValueForKey( info, "fraglimit" ) );
		cgs.roundlimit = atoi( Info_ValueForKey( info, "roundlimit" ) );
		cgs.roundtimelimit = atoi(Info_ValueForKey(info, "roundtimelimit"));
		cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );
		cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
		cgs.realTimelimit = cgs.timelimit;
	}

	if (cgs.timelimit == 0) {
#if 0
		if (cgs.gametype == GT_FFA) {
			cgs.timelimit = 15;
		} else if (cgs.gametype == GT_TOURNAMENT) {
			cgs.timelimit = 10;
		} else if (cgs.gametype == GT_TEAM) {
			cgs.timelimit = 20;
		} else if (cgs.gametype == GT_CA) {
			// great :(
			// 2010-08-07 new servers didn't set -- dumb ass it just means unlimited time
			cgs.timelimit = 20;
		} else if (cgs.gametype == GT_CTF) {
			cgs.timelimit = 20;
		} else if (cgs.gametype == GT_FREEZETAG) {
			cgs.timelimit = 15;
		}
#endif
		if (cg_levelTimerDirection.integer == 4) {
			int s, e;

			s = trap_GetGameEndTime();
			e = trap_GetGameStartTime();
			if (s  &&  e) {
				//cgs.timelimit = cg_levelTimerDefaultTimeLimit.integer;
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
		CG_CpmaParseScores();
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
		return;
	}

	CG_SetScorePlace();

	if (firstCall) {
		// not including ca or freezetag since the config strings are being
		// filled with random player tags
		if (*CG_ConfigString(CS_RED_TEAM_CLAN_TAG)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF)) {
			Q_strncpyz(cgs.redTeamClanTag, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamClanTag));
		} else {
			Q_strncpyz(cgs.redTeamClanTag, "Red Team", sizeof(cgs.redTeamClanTag));
		}

		if (*CG_ConfigString(CS_RED_TEAM_CLAN_NAME)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF)) {
			Q_strncpyz(cgs.redTeamName, CG_ConfigString(CS_RED_TEAM_CLAN_NAME), sizeof(cgs.redTeamName));
		} else {
			Q_strncpyz(cgs.redTeamName, "Red Team", sizeof(cgs.redTeamName));
		}
		trap_Cvar_Set("g_redTeam", cgs.redTeamName);

		if (*CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF)) {
			Q_strncpyz(cgs.blueTeamClanTag, CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG), sizeof(cgs.blueTeamClanTag));
		} else {
			Q_strncpyz(cgs.blueTeamClanTag, "Blue Team", sizeof(cgs.blueTeamClanTag));
		}

		if (*CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME)  &&  (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF)) {
			Q_strncpyz(cgs.blueTeamName, CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME), sizeof(cgs.blueTeamName));
		} else {
			Q_strncpyz(cgs.blueTeamName, "Blue Team", sizeof(cgs.blueTeamName));
		}
		trap_Cvar_Set("g_blueTeam", cgs.blueTeamName);
	}

	cgs.sv_fps = atoi(Info_ValueForKey(info, "sv_fps"));
	//Com_Printf("sv_fps %d\n", cgs.sv_fps);

	//Com_Printf("^5sv_skillRating %s\n", Info_ValueForKey(info, "sv_skillrating"));
	val = atoi(Info_ValueForKey(info, "sv_skillrating"));
	trap_Cvar_Set("sv_skillrating", va("%d", val));

	//FIXME breaks with seeking
	if (cgs.protocol == PROTOCOL_QL  &&  (cgs.lastConnectedDisconnectedPlayer > -1  &&  cgs.lastConnectedDisconnectedPlayerName[0]  &&  cgs.lastConnectedDisconnectedPlayerClientInfo  &&  cgs.needToCheckSkillRating)) {
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
		info = CG_ConfigString(CS_ARMOR_TIERED);
		trap_Cvar_Set("cg_armorTiered", Info_ValueForKey(info, "armor_tiered"));
	}


	if (CG_CheckQlVersion(0, 1, 0, 495)) {
		info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS2);
		//Com_Printf("info2:  %s\n", info);
	} else {
		info = CG_ConfigString(CS_CUSTOM_PLAYER_MODELS);
		//Com_Printf("info1:  %s\n", info);
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

	if (firstCall  ||  cgs.instaGib != instaGib) {
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
}

/*
==================
CG_ParseWarmup
==================
*/
void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;

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

	if ( warmup == 0 && cg.warmup ) {
		Com_Printf("reset match start\n");
		CG_ResetTimedItemPickupTimes();
        cg.itemPickupTime = 0;
        cg.itemPickupClockTime = 0;
		cg.vibrateCameraTime = 0;
		cg.vibrateCameraValue = 0;
		cg.vibrateCameraPhase = 0;

		cg.crosshairClientTime = 0;
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
		cg.lastObituary.time = 0;
		cg.lastTeamChatBeepTime = 0;
		cg.damageDoneTime = 0;
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
		cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
		cg.warmup = atoi(Info_ValueForKey(CG_ConfigString(CS_WARMUP), "time"));
	} else if (!cgs.cpma) {
		cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
		cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
		cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
		cg.warmup = atoi(CG_ConfigString(CS_WARMUP));
	}

	if(cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
#if 1  //def MISSIONPACK
	else if( cgs.gametype == GT_1FCTF ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus = s[0] - '0';
	}
#endif

	//Com_Printf("setconfigvalues() cg.warmup: %d   %d\n", cg.warmup, cg.time);
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

void CG_InterMissionHit (void)
{
	if (cg_buzzerSound.integer) {
		trap_S_StartLocalSound(cgs.media.buzzer, CHAN_LOCAL_SOUND);
	}

#if 0
	if (cgs.cpma) {
		CG_CpmaParseScores();
		Com_Printf("red %d   blue %d\n", cgs.scores1, cgs.scores2);
		trap_SendConsoleCommand("configstrings\n");
	}
#endif

#if 0
	if (cg_audioAnnouncerWin.integer) {
		if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
			//if (cgs.scores1 == cgs.roundlimit) {
			if (cgs.scores1 > cgs.scores2) {
				trap_S_StartLocalSound(cgs.media.redWinsSound, CHAN_ANNOUNCER);
			} else {
				trap_S_StartLocalSound(cgs.media.blueWinsSound, CHAN_ANNOUNCER);
			}
		} else if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			if (cgs.scores1 > cgs.scores2) {
				trap_S_StartLocalSound(cgs.media.redWinsSound, CHAN_ANNOUNCER);
			} else {
				trap_S_StartLocalSound(cgs.media.blueWinsSound, CHAN_ANNOUNCER);
			}
		}
	}
#endif

	//Com_Printf("intermission hit\n");
	if (cgs.gametype == GT_FFA  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endscore_menu_ffa");
	} else if (cgs.gametype == GT_TOURNAMENT  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endscore_menu_duel");
	} else if (cgs.gametype == GT_TEAM  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_tdm");
	} else if (cgs.gametype == GT_CA  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ca");
	} else if ((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS)  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ctf");
	} else if (cgs.gametype == GT_FREEZETAG  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ft");
	} else if (cgs.gametype == GT_1FCTF  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_1fctf");
	} else if (cgs.gametype == GT_HARVESTER  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_har");
	} else if (cgs.gametype == GT_DOMINATION  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_dom");
	} else if (cgs.gametype == GT_CTFS  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ad");
	} else if (cgs.gametype == GT_RED_ROVER  &&  !cg_scoreBoardOld.integer) {
		cg.menuScoreboard = Menus_FindByName("endscore_menu_ffa");
	} else {
		if ( cgs.gametype >= GT_TEAM ) {
			cg.menuScoreboard = Menus_FindByName("endteamscore_menu");
			if (!cg.menuScoreboard) {
				Com_Printf("couldn't find teamscore_menu\n");
			}
		} else {
			cg.menuScoreboard = Menus_FindByName("endscore_menu");
			if (!cg.menuScoreboard) {
				Com_Printf("couldn't find score_menu\n");
			}
		}
	}

	//Com_Printf("intermission menu: %p\n", cg.menuScoreboard);
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

static void CG_CpmaParseScores (void)
{
	const char *str;
	int i;
	clientInfo_t *ci;

	str = CG_ConfigStringNoConvert(CSCPMA_SCORES);

	if (cgs.gametype >= GT_TEAM) {
		cgs.scores1 = atoi(Info_ValueForKey(str, "sr"));
		cgs.scores2 = atoi(Info_ValueForKey(str, "sb"));
	} else {  // duel or ffa
		cgs.scores1 = atoi(Info_ValueForKey(str, "sr"));
		cgs.scores2 = atoi(Info_ValueForKey(str, "sb"));
		if (cgs.scores1 < cgs.scores2) {
			i = cgs.scores1;
			cgs.scores1 = cgs.scores2;
			cgs.scores2 = i;
		}
		if (cgs.gametype == GT_TOURNAMENT  &&  cg.nextSnap) {
			if (cgs.scores1 == cg.nextSnap->ps.persistant[PERS_SCORE]) {
				Q_strncpyz(cgs.firstPlace, cgs.clientinfo[cg.nextSnap->ps.clientNum].name, sizeof(cgs.firstPlace));
				for (i = 0;  i < MAX_CLIENTS;  i++) {
					ci = &cgs.clientinfo[i];
					if (!ci->infoValid) {
						continue;
					}
					if (i == cg.nextSnap->ps.clientNum) {
						continue;
					}
					if (ci->team != TEAM_FREE) {
						continue;
					}
					Q_strncpyz(cgs.secondPlace, ci->name, sizeof(cgs.secondPlace));
					//Com_Printf("^5we are first (%d):  %s\n", cg.nextSnap->ps.persistant[PERS_SCORE], ci->name);
					break;
				}
			} else if (cgs.scores2 == cg.nextSnap->ps.persistant[PERS_SCORE]) {
				Q_strncpyz(cgs.secondPlace, cgs.clientinfo[cg.nextSnap->ps.clientNum].name, sizeof(cgs.secondPlace));
				for (i = 0;  i < MAX_CLIENTS;  i++) {
					ci = &cgs.clientinfo[i];
					if (!ci->infoValid) {
						continue;
					}
					if (i == cg.nextSnap->ps.clientNum) {
						continue;
					}
					if (ci->team != TEAM_FREE) {
						continue;
					}
					Q_strncpyz(cgs.firstPlace, ci->name, sizeof(cgs.firstPlace));
					//Com_Printf("^5we are second (%d):  %s\n", cg.nextSnap->ps.persistant[PERS_SCORE], ci->name);
					break;
				}
			}
		}
	}
	//Com_Printf("^5FIXME red %d  blue %d\n", cgs.scores1, cgs.scores2);
}

static qboolean CG_CpmaCs (int num)
{
	const char *str;
	int x;

	str = CG_ConfigStringNoConvert(num);

	if (num == CSCPMA_SCORES) {
		CG_CpmaParseScores();
		// hack for overtime or end of buzzer scores that trigger
		// PM_INTERMISSION  but scores haven't updated
		if (!cg.intermissionStarted  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
			trap_SendConsoleCommand("exec gameend.cfg\n");
			CG_InterMissionHit();
			cg.intermissionStarted = cg.time;
		}
	} else if (num == 657) {  //CSCPMA_GAMESTATE) {  //FIXME wtf?
		//Com_Printf("^1%f  657  '%s'\n", cg.ftime, str);
		if (1) {  //(!*str) {
			str = CG_ConfigStringNoConvert(672);
		} else {
			Com_Printf("^2st valid '%s'\n", str);
		}
		cgs.fraglimit = atoi(Info_ValueForKey(str, "sl"));
		cgs.timelimit = atoi(Info_ValueForKey(str, "tl"));
		cgs.realTimelimit = cgs.timelimit;
		cgs.levelStartTime = atoi(Info_ValueForKey(str, "ts"));

		x = atoi(Info_ValueForKey(str, "tw"));
		if (!x  &&  cg.warmup) {
			cg.warmup = x;  // to play sound
			CG_MapRestart();
		}
		cg.warmup = x;

		//Com_Printf("%f  level startTime %f\n", cg.ftime, (float) cgs.levelStartTime);
		//} else if (num == CSCPMA_GAMESTATE) {
		//Com_Printf("^5gamestate\n");
		x = atoi(Info_ValueForKey(str, "vt"));
		if (x) {
			//Com_Printf("^3vote %d\n", x);
			if (x != cgs.voteTime) {
				if (cg_audioAnnouncerVote.integer) {
					trap_S_StartLocalSound(cgs.media.voteNow, CHAN_ANNOUNCER);
				}
			}
			cgs.voteTime = x;
			cgs.voteModified = qtrue;
			cgs.voteYes = atoi(Info_ValueForKey(str, "vf"));
			cgs.voteNo = atoi(Info_ValueForKey(str, "va"));
			Q_strncpyz(cgs.voteString, Info_ValueForKey(str, "vs"), sizeof(cgs.voteString));
		}
		x = atoi(Info_ValueForKey(str, "te"));
		if (x  &&  x != cgs.cpmaTimeoutTime) {
			cgs.timeoutEndTime = x + cg.snap->serverTime;
			cgs.timeoutBeginTime = cg.snap->serverTime;
			cgs.timeoutBeginCgTime = cg.time;
			cgs.cpmaTimeoutTime = x;
			trap_S_StartLocalSound(cgs.media.klaxon1, CHAN_LOCAL_SOUND);
		} else if (!x) {
			cgs.timeoutBeginTime = 0;
			cgs.timeoutBeginCgTime = 0;
			cgs.timeoutEndTime = 0;
		}
	} else {
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

	if (CG_CheckQlVersion(0, 1, 0, 495)) {
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

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo(qfalse);
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
			trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
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
		Q_strncpyz( cgs.teamVoteString[n], str, sizeof( cgs.teamVoteString ) );
#if 1 //def MISSIONPACK
		//FIXME wc and if you are not on the same team??
		if (*str  &&  cg_audioAnnouncerTeamVote.integer) {
			trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
		}
#endif
	} else if ( num == CS_INTERMISSION ) {
		if (cg.intermissionStarted == 0) {
			trap_SendConsoleCommand("exec gameend.cfg\n");
		}
		cg.intermissionStarted = atoi( str );
		CG_InterMissionHit();
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
		//Com_Printf("cs client %d\n", n);
		CG_NewClientInfo(n);
		CG_BuildSpectatorString();
		//Com_Printf("build spec string\n");
	} else if ( num == CS_FLAGSTATUS ) {
		if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			//Com_Printf("CS_FLAGSTATUS: %s\n", str);
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
#if 1  //def MISSIONPACK
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
		//Com_Printf("%d FIXME %d CS_ROUND_STATUS  %s\n", cg.time, num, str);
		if (str[0] == '\0') {
			cgs.roundStarted = qtrue;
			for (i = 0;  i < cg.numScores;  i++) {
				cg.scores[i].alive = qtrue;
			}
			for (i = 0;  i < MAX_CLIENTS;  i++) {
				memset(&wclients[i].perKillwstats, 0, sizeof(wclients[i].perKillwstats));
			}
			cgs.thirtySecondWarningPlayed = qfalse;
		} else {
			cgs.roundBeginTime = atoi(Info_ValueForKey(str, "time"));
			cgs.roundNum = atoi(Info_ValueForKey(str, "round"));
			cgs.roundTurn = atoi(Info_ValueForKey(str, "turn"));
			//FIXME ctfs  'state' ?
		}
	} else if (num == CS_ROUND_TIME  &&  (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER)) {
		int val;

#if 0
		cgs.redPlayersLeft = atoi(CG_ConfigString(CS_RED_PLAYERS_LEFT));
		cgs.bluePlayersLeft = atoi(CG_ConfigString(CS_BLUE_PLAYERS_LEFT));
		Com_Printf("^1red ^7%d   ^4blue ^7%d\n", cgs.redPlayersLeft, cgs.bluePlayersLeft);
#endif

		val = atoi(str);
		if (val > 0  &&  !cgs.roundStarted) {
			//trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
			cgs.roundStarted = qtrue;
			trap_SendConsoleCommand("exec roundstart.cfg\n");
		} else if (val < 0) {
			//Com_Printf("ca time reset\n");
			cgs.roundStarted = qfalse;
			cgs.roundBeginTime = -999;
			cgs.countDownSoundPlayed = -999;
#if 0
			if (cgs.gametype == GT_CA) {
				if (cgs.redPlayersLeft == 0) {
					if (cg_audioAnnouncerRound.integer) {
						trap_S_StartLocalSound(cgs.media.blueWinsRoundSound, CHAN_ANNOUNCER);
					}
					CG_CenterPrint("Blue Wins the Round", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				} else if (cgs.bluePlayersLeft == 0) {
					if (cg_audioAnnouncerRound.integer) {
						trap_S_StartLocalSound(cgs.media.redWinsRoundSound, CHAN_ANNOUNCER);
					}
					CG_CenterPrint("Red Wins the Round", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				} else {
					if (cg_audioAnnouncerRound.integer) {
						trap_S_StartLocalSound(cgs.media.roundDrawSound, CHAN_ANNOUNCER);
					}
					CG_CenterPrint("Round Draw", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}
			} else if (cgs.gametype == GT_FREEZETAG) {
				if (cgs.redPlayersLeft != 0  &&  cgs.bluePlayersLeft != 0) {
					if (cg_audioAnnouncerRound.integer) {
						trap_S_StartLocalSound(cgs.media.roundDrawSound, CHAN_ANNOUNCER);
					}
					CG_CenterPrint("Round Draw", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}
			}
#endif
			trap_SendConsoleCommand("exec roundend.cfg\n");
			//Com_Printf("round end\n");
		}
	} else if (num == CS_TIMEOUT_BEGIN_TIME) {
		cgs.timeoutBeginTime = atoi(str);
		cgs.timeoutBeginCgTime = cg.time;
	} else if (num == CS_TIMEOUT_END_TIME) {
		cgs.timeoutEndTime = atoi(str);
		//FIXME do shit
	} else if (num == CS_RED_TEAM_TIMEOUTS_LEFT) {
		cgs.redTeamTimeoutsLeft = atoi(str);
	} else if (num == CS_BLUE_TEAM_TIMEOUTS_LEFT) {
		cgs.blueTeamTimeoutsLeft = atoi(str);
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
	} else if (num == CS_MVP_OFFENSE) {
	} else if (num == CS_MVP_DEFENSE) {
	} else if (num == CS_MVP) {
	} else if (num == CS_RED_TEAM_CLAN_NAME) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.redTeamName, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamName));
		}
		//CG_Printf("^5new red team name ^7'%s'\n", CG_ConfigString(CS_RED_TEAM_CLAN_NAME));
	} else if (num == CS_BLUE_TEAM_CLAN_NAME) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.blueTeamName, CG_ConfigString(CS_BLUE_TEAM_CLAN_TAG), sizeof(cgs.blueTeamName));
		}
		//CG_Printf("^5new blue team name ^7'%s'\n", CG_ConfigString(CS_BLUE_TEAM_CLAN_NAME));
	} else if (num == CS_RED_TEAM_CLAN_TAG) {
		if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS) {
			Q_strncpyz(cgs.redTeamClanTag, CG_ConfigString(CS_RED_TEAM_CLAN_TAG), sizeof(cgs.redTeamClanTag));
		}
		//CG_Printf("^5new red clan tag ^7'%s'\n", CG_ConfigString(CS_RED_TEAM_CLAN_TAG));
	} else if (num == CS_BLUE_TEAM_CLAN_TAG) {
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
	} else if (num == CS_DOMINATION_RED_POINTS) {
		cgs.dominationRedPoints = atoi(str);
	} else if (num == CS_DOMINATION_BLUE_POINTS) {
		cgs.dominationBluePoints = atoi(str);
	} else if (num == CS_ROUND_WINNERS) {
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
	char *p, *ls;
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

	Com_Printf("reset map restart\n");

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

	cg.crosshairClientTime = 0;
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
	cg.lastObituary.time = 0;
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
	if ( cg.warmup == 0   &&  cgs.gametype != GT_FREEZETAG /* && cgs.gametype == GT_TOURNAMENT */) {
		if (cg_audioAnnouncerWarmup.integer) {
			trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
		}
		//CG_CenterPrint( "FIGHT!", 120, GIANTCHAR_WIDTH*2 );
		CG_CenterPrint("FIGHT!", 120, BIGCHAR_WIDTH);
	}
#ifdef MISSIONPACK
	if (cg_singlePlayerActive.integer) {
		trap_Cvar_Set("ui_matchStartTime", va("%i", cg.time));
		if (cg_recordSPDemo.integer && cg_recordSPDemoName.string && *cg_recordSPDemoName.string) {
			trap_SendConsoleCommand(va("set g_synchronousclients 1 ; record %s \n", cg_recordSPDemoName.string));
		}
	}
#endif
	//trap_Cvar_Set("cg_thirdPerson", "0");  //FIXME wolfcam
	cg.matchRestartServerTime = cg.snap->serverTime;
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
int CG_ParseVoiceChats( const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats ) {
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
	if (!token || token[0] == 0) {
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
		if (!token || token[0] == 0) {
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
			if (!token || token[0] == 0) {
				return qtrue;
			}
			if (!Q_stricmp(token, "}"))
				break;
			sound = trap_S_RegisterSound( token, compress );
			voiceChats[voiceChatList->numVoiceChats].sounds[voiceChats[voiceChatList->numVoiceChats].numSounds] = sound;
			token = COM_ParseExt(p, qtrue);
			if (!token || token[0] == 0) {
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
int CG_HeadModelVoiceChats( char *filename ) {
	int	len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;

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
	if (!token || token[0] == 0) {
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
int CG_GetVoiceChat( voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, char **chat) {
	int i, rnd;

	for ( i = 0; i < voiceChatList->numVoiceChats; i++ ) {
		if ( !Q_stricmp( id, voiceChatList->voiceChats[i].id ) ) {
			rnd = random() * voiceChatList->voiceChats[i].numSounds;
			*snd = voiceChatList->voiceChats[i].sounds[rnd];
			*chat = voiceChatList->voiceChats[i].chats[rnd];
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
voiceChatList_t *CG_VoiceChatListForClient( int clientNum ) {
	clientInfo_t *ci;
	int voiceChatNum, i, j, k, gender;
	char filename[MAX_QPATH], headModelName[MAX_QPATH];

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
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
void CG_PlayVoiceChat( bufferedVoiceChat_t *vchat ) {
#ifdef MISSIONPACK
	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	if ( !cg_noVoiceChats.integer ) {
		trap_S_StartLocalSound( vchat->snd, CHAN_VOICE);
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
#endif
}

/*
=====================
CG_PlayBufferedVoieChats
=====================
*/
void CG_PlayBufferedVoiceChats( void ) {
#ifdef MISSIONPACK
	if ( cg.voiceChatTime < cg.time ) {
		if (cg.voiceChatBufferOut != cg.voiceChatBufferIn && voiceChatBuffer[cg.voiceChatBufferOut].snd) {
			//
			CG_PlayVoiceChat(&voiceChatBuffer[cg.voiceChatBufferOut]);
			//
			cg.voiceChatBufferOut = (cg.voiceChatBufferOut + 1) % MAX_VOICECHATBUFFER;
			cg.voiceChatTime = cg.time + 1000;
		}
	}
#endif
}

/*
=====================
CG_AddBufferedVoiceChat
=====================
*/
void CG_AddBufferedVoiceChat( bufferedVoiceChat_t *vchat ) {
#ifdef MISSIONPACK
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
#endif
}

/*
=================
CG_VoiceChatLocal
=================
*/
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd ) {
#ifdef MISSIONPACK
	char *chat;
	voiceChatList_t *voiceChatList;
	clientInfo_t *ci;
	sfxHandle_t snd;
	bufferedVoiceChat_t vchat;

	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	cgs.currentVoiceClient = clientNum;

	voiceChatList = CG_VoiceChatListForClient( clientNum );

	if ( CG_GetVoiceChat( voiceChatList, cmd, &snd, &chat ) ) {
		//
		if ( mode == SAY_TEAM || !cg_teamChatsOnly.integer ) {
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
#endif
}

/*
=================
CG_VoiceChat
=================
*/
void CG_VoiceChat( int mode ) {
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
#endif

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
		if (text[i] == '\x19')
			continue;
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

static qboolean CG_CpmaParseMstats (void)
{
	int n;
	int wflags;
	duelScore_t *ds;
	duelWeaponStats_t *ws;
	int clientNum;
	int i;
	int duelPlayer;
	int totalHits;
	int totalAtts;
	const char *s;
	wclient_t *wc;

	// none -- 26 args

	//FIXME saw it in a tdm demo as well
	if (cgs.gametype != GT_TOURNAMENT) {
		return qfalse;
	}

	n = 1;
	clientNum = atoi(CG_Argv(n));  n++; // player num
	wc = &wclients[clientNum];

	//FIXME
	duelPlayer = 0;
	for (i = 0;  i < MAX_CLIENTS;  i++) {
		if (!cgs.clientinfo[i].infoValid  ||  cgs.clientinfo[i].team != TEAM_FREE) {
			continue;
		}
		if (i == clientNum) {
			break;
		}
		duelPlayer++;
	}

	if (duelPlayer > 1) {
		duelPlayer = 1;
	}

	if (duelPlayer == 0) {
		cg.duelPlayer1 = clientNum;
	} else {
		cg.duelPlayer2 = clientNum;
	}

	ds = &cg.duelScores[duelPlayer];

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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
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
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
		atoi(CG_Argv(n));  n++;
	}

	ds->damage = atoi(CG_Argv(n));  n++;  // damage done
	atoi(CG_Argv(n));  n++;  // damage taken
	atoi(CG_Argv(n));  n++;  // total armor taken
	atoi(CG_Argv(n));  n++;  // total health taken
	ds->megaHealthPickups = atoi(CG_Argv(n));  n++;
	ds->redArmorPickups = atoi(CG_Argv(n));  n++;
	ds->yellowArmorPickups = atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;  // 2
	s = CG_Argv(n);  n++;  // abataiayaaaa
	//Com_Printf("^3%s\n", s);
	if (s[0] != '-') {
		ds->ping = CG_CpmaStringScore(s[2]) * 32 + CG_CpmaStringScore(s[3]);
	} else {
		ds->ping = 0;
	}
	wc->serverPingAccum += ds->ping;
	wc->serverPingSamples++;

	cgs.clientinfo[clientNum].score = atoi(CG_Argv(n));  n++;  // score?
	ds->kills = atoi(CG_Argv(n));  n++;  // frags?
	atoi(CG_Argv(n));  n++;  // deaths?
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
	atoi(CG_Argv(n));  n++;
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

	return qtrue;
}

static qboolean CG_CpmaParseDmscores (void)
{

	// 1  first place client number
	// 2  second place client number

	return qtrue;
}

static qboolean CG_CpmaMM2 (void)
{
	int clientNum;
	int location;
	char *s;

	clientNum = atoi(CG_Argv(1));
	location = atoi(CG_Argv(2));
	s = va("(%s^7) (%s): ^5%s", cgs.clientinfo[clientNum].name, CG_ConfigString(CS_LOCATIONS + location), CG_Argv(3));
	CG_Printf("%s\n", s);
	CG_PrintToScreen("%s", s);

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
		sc->client = clientNum;

		ci = &cgs.clientinfo[clientNum];
		sc->score = atoi(CG_Argv(n));  n++;  // score

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

static void CG_FilterOspTeamChatLocation (char *text)
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

	if (SC_Cvar_Get_Int("cg_debugcommands")) {
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
		//CG_StartCamera( CG_Argv(1), atoi(CG_Argv(2)) );
		return;
	}
	if ( !strcmp( cmd, "stopCam" ) ) {
		//cg.cameraMode = qfalse;
		return;
	}
	// end camera script

	if ( !strcmp( cmd, "cp" ) ) {
		//Com_Printf("'%s'\n", CG_Argv(1));
		CG_CenterPrint( CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		CG_Printf( "%s", CG_Argv(1) );
		CG_PrintToScreen("%s", CG_Argv(1));

#if 1 //def MISSIONPACK
		cmd = CG_Argv(1);			// yes, this is obviously a hack, but so is the way we hear about
									// votes passing or failing

		if (!Q_stricmpn(cmd, "vote failed", 11)  &&  cg_audioAnnouncerVote.integer) {
			trap_S_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "team vote failed", 16)  &&  cg_audioAnnouncerTeamVote.integer) {
			trap_S_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "vote passed", 11)  &&  cg_audioAnnouncerVote.integer) {
			trap_S_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		} else if (!Q_stricmpn(cmd, "team vote passed", 16)  &&  cg_audioAnnouncerTeamVote.integer) {
			trap_S_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		}
#endif
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( !cg_teamChatsOnly.integer ) {
			if (cg_chatBeep.integer) {
				if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
					trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
					cg.lastChatBeepTime = cg.time;
				}
			}
			Q_strncpyz( text, CG_Argv(1), MAX_SAY_TEXT );
			CG_RemoveChatEscapeChar( text );
			CG_Printf( "%s\n", text );
			CG_PrintToScreen("%s", text);
			//Q_strncpyz(cg.lastChatString, text, sizeof(cg.lastChatString));
		}
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		//FIXME wolfcam_following
		if (cg.snap->ps.clientNum != cg.clientNum) {
			// it's a spec demo, team is other specs
			if (cg_chatBeep.integer) {
				if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
					trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
					cg.lastChatBeepTime = cg.time;
				}
			}
		} else if (cg_teamChatBeep.integer) {
			if (cg.time - cg.lastTeamChatBeepTime >= (cg_teamChatBeepMaxTime.value * 1000)) {
				trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
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

	if ( !strcmp( cmd, "bchat" ) ) {  //FIXME bot chat ??
		if ( !cg_teamChatsOnly.integer ) {
			if (cg_chatBeep.integer) {
				if (cg.time - cg.lastChatBeepTime >= (cg_chatBeepMaxTime.value * 1000)) {
					trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
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

	if ( !strcmp( cmd, "rrscores" ) ) {
		//Com_Printf("^3---------------rrscores\n");
		//memset(cgs.newConnectedClient, 0, sizeof(cgs.newConnectedClient));
		CG_ParseRRScores();
		Wolfcam_ScoreData();
		CG_BuildSpectatorString();
		return;
	}

	if (!strcmp(cmd, "adscores")) {
		CG_ParseCtfsRoundScores();
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
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
			if (CG_CpmaParseMstats()) {
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
	}

	// quake live
	if (!strcmp(cmd, "acc")) {
		int i;

		cg.serverAccuracyStatsTime = cg.time;
		cg.serverAccuracyStatsClientNum = cg.snap->ps.clientNum;

		//CG_Printf("acc: \n");
		for (i = WP_NONE;  i < WP_NUM_WEAPONS;  i++) {
			//CG_Printf("  %s    %s\n", CG_Argv(i + 1), weapNames[i]);
			cg.serverAccuracyStats[i] = atoi(CG_Argv(i + 1));
		}
		return;
	}

	if (!strcmp(cmd, "pcp")) {  // pause center print??
		// timeout
		//CG_AddBufferedSound(cgs.media.klaxon1);
		trap_S_StartLocalSound( cgs.media.klaxon1, CHAN_LOCAL_SOUND );
		CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		Com_Printf("%s\n", CG_Argv(1));
		return;
	}

	if (!Q_stricmp(cmd, "playSound")) {
		//Com_Printf("^3playsound:  '%s'\n", CG_Argv(1));
		//trap_S_StartLocalSound(CG_Argv(1));
		trap_SendConsoleCommand(va("play %s\n", CG_Argv(1)));
		return;
	}

	if (!Q_stricmp(cmd, "dscores")) {
		int n;
		int j;

		CG_SetDuelPlayers();

		n = 1;
		for (i = 0;  i < 2;  i++) {
			duelScore_t *ds;

			ds = &cg.duelScores[i];

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

			for (j = 1;  j < WP_NUM_WEAPONS;  j++) {
				duelWeaponStats_t *ws;

				ws = &ds->weaponStats[j];

				ws->hits = atoi(CG_Argv(n));  n++;
				ws->atts = atoi(CG_Argv(n));  n++;
				ws->accuracy = atoi(CG_Argv(n));  n++;
				ws->damage = atoi(CG_Argv(n));  n++;
				ws->kills = atoi(CG_Argv(n));  n++;
			}
		}
		//Com_Printf("dscores:  n %d\n", n);
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

	if (!Q_stricmp(cmd, "rcmd")) {
		CG_Printf("^3remote command '%s'\n", CG_ArgsFrom(1));
		return;
	}

	// unknown:
	//CG_Printf( "Unknown client game command: %s\n", cmd );
	CG_Printf( "Unknown client game command: %s\n", CG_Argv(0) );
	i = 1;
	while (1) {
		if (!*CG_Argv(i)) {
			break;
		}
		CG_Printf("argv[%d]: %s\n", i, CG_Argv(i));
		i++;
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

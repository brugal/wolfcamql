// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#include "cg_drawtools.h"
#include "cg_info.h"
#include "cg_main.h"
#include "cg_newdraw.h"  // QLWideScreen
#include "cg_players.h"
#include "cg_syscalls.h"

#include "wolfcam_local.h"
#include "sc.h"

#define MAX_LOADING_PLAYER_ICONS	128  //16
#define MAX_LOADING_ITEM_ICONS		26

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;

	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		if (!loadingPlayerIcons[n]) {
			continue;
		}
		x = 16 + n % 8 * 78;
		y = 324-40;
		//CG_DrawPic( x, y, 64, 64, loadingPlayerIcons[n] );
		CG_DrawPic( x, 480 - 32 - 32, 32, 32, loadingPlayerIcons[n] );
	}

	for( n = 0; n < loadingItemIconCount; n++ ) {
		if (!loadingItemIcons[n]) {
			continue;
		}
		y = 400-40;
		if( n >= 13 ) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		//CG_DrawPic( x, y, 32, 32, loadingItemIcons[n] );
		CG_DrawPic( x, 480 - 32, 32, 32, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );
	//Com_Printf("loadingstring '%s'\n", s);
	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	const gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}

	CG_LoadingString( item->pickup_name );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );

		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	//Q_CleanStr( personality );
	//Com_Printf("clean str %c%c  %s\n", personality[0], personality[1], personality);

	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ), qtrue );
	}

	CG_LoadingString( personality );
	//sleep(5);

#if 0
	// testing
	while (1) {
		const char *s;
		static int i = 0;

		i++;
		s = va("loading test %d", i);
		CG_LoadingString(s);
	}
#endif

}

void CG_LoadingFutureClient (const char *modelName)
{
	//const char		*info;
	char			*skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	//info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		//Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		Q_strncpyz(model, modelName, sizeof(model));
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );

		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	//Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_strncpyz(personality, modelName, sizeof(personality));
	//Q_CleanStr( personality );
	//Com_Printf("clean str %c%c  %s\n", personality[0], personality[1], personality);

	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ), qtrue );
	}

	CG_LoadingString( personality );


}

static char clockBuffer[MAX_STRING_CHARS];

static char *clockString (int timems)
{
	int days, hours, minutes;
	float seconds;
	int count;
	char *s;

	clockBuffer[0] = '\0';

	days = hours = minutes = 0;
	count = timems;


	days = count / (24 * 60 * 60 * 1000);
	count -= days * (24 * 60 * 60 * 1000);

	hours = count / (60 * 60 * 1000);
	count -= hours * (60 * 60 * 1000);

	minutes = count / (60 * 1000);
	count -= minutes * (60 * 1000);

	seconds = count / 1000.0f;

	if (days > 0) {
		s = va("%d day(s) %d:%02d:%02.2f", days, hours, minutes, seconds);
	} else if (hours > 0) {
		s = va("%d:%02d:%02.2f", hours, minutes, seconds);
	} else if (minutes > 0) {
		s = va("%d:%02.2f", minutes, seconds);
	} else {
		s = va("%02.2f", seconds);
	}

	Q_strncpyz(clockBuffer, s, sizeof(clockBuffer));

	return clockBuffer;
}

/*
====================
CG_DrawInformation
 
Draw all the status / pacifier stuff during level loading and ingame
====================
**/
void CG_DrawInformation (qboolean loading)
{
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	levelshot;
	qhandle_t	detail;
	char		buf[1024];
	//vec4_t color;
	int n;
	int lines;

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	if (loading) {
		QLWideScreen = 0;

		s = Info_ValueForKey( info, "mapname" );
		if (CG_FileExists(va("levelshots/%s.tga", s))  ||  CG_FileExists(va("levelshots/%s.jpg", s))) {
			levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );
		} else {
			levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
		}
		if ( !levelshot ) {
			levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
		}
		trap_R_SetColor( NULL );
		//trap_R_SetColor(colorBlue);
		//Vector4Set(color, 0, 0, 1, 1);
		//trap_R_SetColor(color);
		CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

		//FIXME new ql 2014-09-07 not found and spams "RE_RegisterShader(levelShotDetail) failed"
		// blend a detail texture over it
		detail = trap_R_RegisterShader( "wc/levelShotDetail" );
		if (detail) {
			trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail );
		}

		// draw the icons of things as they are loaded
		QLWideScreen = 2;
		CG_DrawLoadingIcons();

		// the first 150 rows are reserved for the client connection
		// screen to write into
		if ( cg.infoScreenText[0] ) {
			UI_DrawProportionalString3( 320, 128-32, va("Loading... %s", cg.infoScreenText),
										UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		} else {
			UI_DrawProportionalString3( 320, 128-32, "Awaiting snapshot...",
										UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		}
	}  // loading

	QLWideScreen = 1;
	//CG_DrawStringExt(2, 2, va("wolfcamql version %s Τη γλώσσα μου έδωσαν ελληνική test test", WOLFCAM_VERSION), colorYellow, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.smallchar);
	//CG_DrawStringExt(2, 2, va("wolfcamql version %s Τη γλώσσα μου έδωσαν ελληνική test test", WOLFCAM_VERSION), colorYellow, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.bigchar);
	CG_DrawStringExt(2, 2, va("wolfcamql version %s", WOLFCAM_VERSION), colorYellow, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.bigchar);
	QLWideScreen = 2;

	// draw info string information

	if (loading) {
		//y = 200 - 32;
		y = 160 - 32;
	} else {
		y = 20;
	}

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if (1) {  //( !atoi( buf ) ) {
		int timePlayed;

		if (cgs.cpma) {
			//Com_Printf("^11:  %s\n", CG_GetLocalTimeString());
		}

		timePlayed = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_levelStartTime"));
		if (timePlayed) {
			lines = UI_DrawProportionalString3(320, y, CG_GetLocalTimeString(), UI_CENTER | UI_SMALLFONT|UI_DROPSHADOW, colorYellow);
			y += PROP_HEIGHT * lines;
		}

		// server hostname
		lines = UI_DrawProportionalString3( 320, y, Info_ValueForKey(info, "sv_hostname"),
											UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT * lines;

		// ranked or mod
		if (cgs.protocol == PROTOCOL_QL  &&  cgs.realProtocol < 91) {
			value = atoi(Info_ValueForKey(info, "sv_ranked"));
			Com_sprintf(buf, sizeof(buf), "(%s server)", value ? "ranked" : "unranked");
		} else if (cgs.protocol == PROTOCOL_Q3) {  // mod:  baseq3, osp, cpma, etc..
			if (cgs.ospEncrypt) {
				Com_sprintf(buf, sizeof(buf), "osp ^6encrypted");
			} else if (cgs.osp) {
				Com_sprintf(buf, sizeof(buf), "osp");
			} else if (cgs.cpma) {
				Com_sprintf(buf, sizeof(buf), "cpma: %s", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "server_gameplay"));
			} else {
				Com_sprintf(buf, sizeof(buf), "%s", CG_ConfigString(CS_GAME_VERSION));
			}
		} else {
			// current ql, skip
			//FIXME check for maybe minix or something else?
			buf[0] = '\0';
		}

		if (buf[0] != '\0') {
			lines = UI_DrawProportionalString3( 320, y, buf,
											UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorYellow);
			y += PROP_HEIGHT * lines;
		}

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			lines = UI_DrawProportionalString3( 320, y, va("Pure Server protocol %d", SC_Cvar_Get_Int("real_protocol")),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorCyan );
			y += PROP_HEIGHT * lines;
		} else {
			lines = UI_DrawProportionalString3( 320, y, va("Impure Server protocol %d", SC_Cvar_Get_Int("real_protocol")),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorRed );
			y += PROP_HEIGHT * lines;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			lines = UI_DrawProportionalString3( 320, y, s,
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT * lines;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	//s = CG_ConfigString( CS_MESSAGE );
	s = va("^1(%s.bsp) ^7%s", Info_ValueForKey(info, "mapname"), CG_ConfigString(CS_MESSAGE));
	if ( s[0] ) {
		lines = UI_DrawProportionalString3( 320, y, s,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT * lines;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		lines = UI_DrawProportionalString3( 320, y, "CHEATS ARE ENABLED",
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorRed );
		y += PROP_HEIGHT * lines;
	}

	// game type
	if (cgs.gametype == GT_FFA) {
		s = "Free For All";
	} else if (cgs.gametype == GT_SINGLE_PLAYER) {
		s = "Single Player";
	} else if (cgs.gametype == GT_TOURNAMENT) {
		s = "Duel";
	} else if (cgs.gametype == GT_TEAM) {
		s = "Team Deathmatch";
	} else if (cgs.gametype == GT_CA) {
		s = "Clan Arena";
	} else if (cgs.gametype == GT_CTF) {
		s = "Capture The Flag";
	} else if (cgs.gametype == GT_1FCTF) {
		s = "One Flag CTF";
	} else if (cgs.gametype == GT_CTFS) {
		if (cgs.cpma) {
			s = "Capture Strike";
		} else {
			s = "Attack and Defend";
		}
	} else if (cgs.gametype == GT_OBELISK) {
		s = "Overload";
	} else if (cgs.gametype == GT_HARVESTER) {
		s = "Harvester";
	} else if (cgs.gametype == GT_FREEZETAG) {
		s = "Freeze Tag";
	} else if (cgs.gametype == GT_DOMINATION) {
		s = "Domination";
	} else if (cgs.gametype == GT_RED_ROVER) {
		if (cgs.customServerSettings & SERVER_SETTING_INFECTED) {
			s = "Red Rover (Infected)";
		} else {
			s = "Red Rover";
		}
	} else if (cgs.gametype == GT_NTF) {
		s = "Not Team Fortress";
	} else if (cgs.gametype == GT_2V2) {
		s = "Two vs Two";
	} else if (cgs.gametype == GT_HM) {
		s = "Hoonymode";
	} else if (cgs.gametype == GT_RACE) {
		s = "Race";
	} else {
		s = "Unknown Gametype";
	}

	if (cgs.protocol != PROTOCOL_QL) {
		lines = UI_DrawProportionalString3(320, y, s, UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorYellow);
		y += PROP_HEIGHT * lines;
	} else {
		n = atoi(Info_ValueForKey(info, "ruleset"));
		if (n == 2) {
			lines = UI_DrawProportionalString3(320, y, va("%s %s%s", s, " (pql)", atoi(Info_ValueForKey(info, "g_instagib")) ? " (instagib)" : ""), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorBlue);
		} else {
			if (CG_CheckQlVersion(0, 1, 0, 495)) {
				n = atoi(Info_ValueForKey(CG_ConfigString(CS_ARMOR_TIERED), "armor_tiered"));
			} else {
				n = 0;
			}
			lines = UI_DrawProportionalString3(320, y, va("%s%s%s", s, n == 1 ? " (tiered armor)" : "", atoi(Info_ValueForKey(info, "g_instagib")) ? " (instagib)" : ""), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, n == 1 ? colorGreen : atoi(Info_ValueForKey(info, "g_instagib")) ? colorMagenta : colorYellow);
		}
		y += PROP_HEIGHT * lines;

		if (atoi(Info_ValueForKey(info, "ruleset")) != 2) {
			if (*CG_ConfigString(CS_PMOVE_SETTINGS)) {
				if (atoi(Info_ValueForKey(CG_ConfigString(CS_PMOVE_SETTINGS), "pmove_WeaponRaiseTime")) == 0  &&  atoi(Info_ValueForKey(CG_ConfigString(CS_PMOVE_SETTINGS), "pmove_WeaponDropTime")) == 0) {
					lines = UI_DrawProportionalString3(320, y, "fast weapon switch", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorCyan);
					y += PROP_HEIGHT * lines;
				}
			}
		}
	}

	value = atoi( Info_ValueForKey( info, "scorelimit" ) );
	if ( value ) {
		lines = UI_DrawProportionalString3( 320, y, va( "scorelimit %i", value ),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT * lines;
	}

	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		lines = UI_DrawProportionalString3( 320, y, va( "timelimit %i", value ),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT * lines;
	}

	if (cgs.gametype < GT_CA ) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			lines = UI_DrawProportionalString3( 320, y, va( "fraglimit %i", value ),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT * lines;
		}
	}

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
		value = atoi( Info_ValueForKey( info, "roundlimit" ) );
		if ( value ) {
			lines = UI_DrawProportionalString3( 320, y, va( "roundlimit %i", value ),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT * lines;
		}
	}

	if (cgs.gametype >= GT_CTF  &&  cgs.gametype < GT_FREEZETAG) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			lines = UI_DrawProportionalString3( 320, y, va( "capturelimit %i", value ),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT * lines;
		}
	}

	if (cgs.protocol == PROTOCOL_QL) {
		const char *p;

		if (cgs.realProtocol < 91) {
			value = atoi(Info_ValueForKey(info, "sv_skillrating"));
			lines = UI_DrawProportionalString3(320, y, va( "skill rating %i", value), UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT * lines;
		}

		if (*Info_ValueForKey(info, "sv_location")) {
			lines = UI_DrawProportionalString3(320, y, va("location %s", Info_ValueForKey(info, "sv_location")), UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT * lines;
		}

		if (*cgs.serverModelOverride  ||  *cgs.serverHeadModelOverride) {
			lines = UI_DrawProportionalString3(320, y, "forced server models", UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorMdGrey);
			y += PROP_HEIGHT * lines;
		}

		p = SC_Cvar_Get_String("com_workshopids");
		// strlen() check since it is sometimes a blank space ' '
		if (p  && *p  &&  strlen(p) > 1) {
			lines = UI_DrawProportionalString3(320, y, va("workshops: %s", p), UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorMdGrey);
			y += PROP_HEIGHT * lines;
		}
	}  // end protocol ql

	if (cg.demoPlayback  &&  !loading  &&  cg.snap) {
		int startTime, endTime, currentTime;

		startTime = trap_GetFirstServerTime();
		endTime = trap_GetLastServerTime();
		currentTime = cg.snap->serverTime;

		trap_Cvar_VariableStringBuffer("cl_demoFileBaseName", buf, sizeof(buf));

		QLWideScreen = 1;
		CG_DrawStringExt(2, 480 - 64,
						 va("^3demo: ^7%s", buf),
						 colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.bigchar);

		CG_DrawStringExt(2, 480 - 32,
						 va("  ^5%s   ^6%02.2f%%", clockString(endTime - startTime), (float)(currentTime - startTime) / (float)(endTime - startTime) * 100.0f),
						 colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.bigchar);


		QLWideScreen = 2;

	}
}

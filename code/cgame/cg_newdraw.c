#include "cg_local.h"

#include "cg_consolecmds.h"  // CG_AdjustTimeForTimeouts
#include "cg_draw.h"
#include "cg_drawtools.h"
#include "cg_event.h"
#include "cg_newdraw.h"
#include "cg_main.h"
#include "cg_players.h"  // color from string
#include "cg_playerstate.h"
#include "cg_syscalls.h"
#include "cg_weapons.h"
#include "wolfcam_info.h"
#include "wolfcam_main.h"

#include "../ui/ui_shared.h"

#include "wolfcam_local.h"
#include "sc.h"
#include "../../ui/wcmenudef.h"


typedef struct {
	float x;
	float y;
	float w;
	float h;
	float text_x;
	float text_y;
	int ownerDraw;
	int ownerDrawFlags;
	int align;
	float special;
	float scale;
	vec4_t color;
	qhandle_t shader;
	int textStyle;
	const fontInfo_t *font;
} odArgs_t;

// set in CG_ParseTeamInfo

//static int sortedTeamPlayers[TEAM_MAXOVERLAY];
//static int numSortedTeamPlayers;
//int drawTeamOverlayModificationCount = -1;

//static char systemChat[256];
//static char teamChat1[256];
//static char teamChat2[256];

void CG_InitTeamChat(void) {
  memset(teamChat1, 0, sizeof(teamChat1));
  memset(teamChat2, 0, sizeof(teamChat2));
  memset(systemChat, 0, sizeof(systemChat));
}

void CG_SetPrintString(int type, const char *p) {
  if (type == SYSTEM_PRINT) {
    strcpy(systemChat, p);
  } else {
    strcpy(teamChat2, teamChat1);
    strcpy(teamChat1, p);
  }
}

void CG_CheckOrderPending(void) {
	if (wolfcam_following) {
		return;
	}

	if (cgs.gametype < GT_CTF  ||  cgs.gametype != GT_FREEZETAG) {
		return;
	}
	if (cgs.orderPending) {
		//clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
		const char *p1, *p2, *b;
		p1 = p2 = b = NULL;
		switch (cgs.currentOrder) {
			case TEAMTASK_OFFENSE:
				p1 = VOICECHAT_ONOFFENSE;
				p2 = VOICECHAT_OFFENSE;
				b = "+button7; wait; -button7";
			break;
			case TEAMTASK_DEFENSE:
				p1 = VOICECHAT_ONDEFENSE;
				p2 = VOICECHAT_DEFEND;
				b = "+button8; wait; -button8";
			break;
			case TEAMTASK_PATROL:
				p1 = VOICECHAT_ONPATROL;
				p2 = VOICECHAT_PATROL;
				b = "+button9; wait; -button9";
			break;
			case TEAMTASK_FOLLOW:
				p1 = VOICECHAT_ONFOLLOW;
				p2 = VOICECHAT_FOLLOWME;
				b = "+button10; wait; -button10";
			break;
			case TEAMTASK_CAMP:
				p1 = VOICECHAT_ONCAMPING;
				p2 = VOICECHAT_CAMP;
			break;
			case TEAMTASK_RETRIEVE:
				p1 = VOICECHAT_ONGETFLAG;
				p2 = VOICECHAT_RETURNFLAG;
			break;
			case TEAMTASK_ESCORT:
				p1 = VOICECHAT_ONFOLLOWCARRIER;
				p2 = VOICECHAT_FOLLOWFLAGCARRIER;
			break;
		}

		if (cg_currentSelectedPlayer.integer == numSortedTeamPlayers) {
			// to everyone
			trap_SendConsoleCommand(va("cmd vsay_team %s\n", p2));
		} else {
			// for the player self
			if (cg_currentSelectedPlayer.integer < 0  ||  cg_currentSelectedPlayer.integer > numSortedTeamPlayers) {
				Com_Printf("^3CG_CheckOrderPending: invalid selected player %d\n", cg_currentSelectedPlayer.integer);
			} else {
				if (sortedTeamPlayers[cg_currentSelectedPlayer.integer] == cg.snap->ps.clientNum && p1) {
					trap_SendConsoleCommand(va("teamtask %i\n", cgs.currentOrder));
					//trap_SendConsoleCommand(va("cmd say_team %s\n", p2));
					trap_SendConsoleCommand(va("cmd vsay_team %s\n", p1));
				} else if (p2) {
					//trap_SendConsoleCommand(va("cmd say_team %s, %s\n", ci->name,p));
					trap_SendConsoleCommand(va("cmd vtell %d %s\n", sortedTeamPlayers[cg_currentSelectedPlayer.integer], p2));
				}
			}
		}
		if (b) {
			trap_SendConsoleCommand(b);
		}
		cgs.orderPending = qfalse;
	}
}

static void CG_SetSelectedPlayerName(void) {
  if (cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		const clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
	  if (ci) {
			trap_Cvar_Set("cg_selectedPlayerName", ci->name);
			trap_Cvar_Set("cg_selectedPlayer", va("%d", sortedTeamPlayers[cg_currentSelectedPlayer.integer]));
			cgs.currentOrder = ci->teamTask;
	  }
	} else {
		trap_Cvar_Set("cg_selectedPlayerName", "Everyone");
	}
}
static int CG_GetSelectedPlayer(void) {
	if (cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer = 0;
	}
	return cg_currentSelectedPlayer.integer;
}

void CG_SelectNextPlayer(void) {
	CG_CheckOrderPending();
	if (cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer++;
	} else {
		cg_currentSelectedPlayer.integer = 0;
	}
	CG_SetSelectedPlayerName();
}

void CG_SelectPrevPlayer(void) {
	CG_CheckOrderPending();
	if (cg_currentSelectedPlayer.integer > 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer--;
	} else {
		cg_currentSelectedPlayer.integer = numSortedTeamPlayers;
	}
	CG_SetSelectedPlayerName();
}


static void CG_DrawPlayerArmorIcon( const rectDef_t *rect, qboolean draw2D ) {
	//centity_t	*cent;
	//playerState_t	*ps;
	vec3_t		angles;
	vec3_t		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	if (wolfcam_following) {
		return;
	}

	//cent = &cg_entities[cg.snap->ps.clientNum];
	//ps = &cg.snap->ps;

	if ( draw2D || ( !cg_draw3dIcons.integer && cg_drawIcons.integer) ) { // bk001206 - parentheses
		CG_DrawPic( rect->x, rect->y + rect->h/2 + 1, rect->w, rect->h, cgs.media.yellowArmorIcon );
  } else if (cg_draw3dIcons.integer) {
	  VectorClear( angles );
    origin[0] = 90;
  	origin[1] = 0;
  	origin[2] = -10;
  	angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;

    CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cgs.media.armorModel, 0, origin, angles );
  }

}

static void CG_DrawPlayerArmorValue (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align, const rectDef_t *menuRect)
{
	char	num[16];
	int value;
	//centity_t	*cent;
	const playerState_t	*ps;
	rectDef_t r;

	if (wolfcam_following) {
		//Com_Printf("wolfcam\n");
		//return;
		value = Wolfcam_PlayerArmor(wcg.clientNum);
		if (value < 0)
			return;
	} else {

		//cent = &cg_entities[cg.snap->ps.clientNum];
		ps = &cg.snap->ps;

		value = ps->stats[STAT_ARMOR];
	}

	memcpy(&r, rect, sizeof(rectDef_t));

	if (shader) {
    trap_R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
	  trap_R_SetColor( NULL );
	} else {
		float height;

		Com_sprintf (num, sizeof(num), "%i", value);
		//value = CG_Text_Width(num, scale, 0, font);
		//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);
		//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, colorBlack, num, 0, 0, textStyle, font);
		//r.y += r.h;  //FIXME 2015-10-23 why?
		height = CG_Text_Height(num, scale, 0, font);
		//r.y += height;
		//Com_Printf("^6height: %d  %f\n", height, rect->h);

		//r.y = menuRect->y;
		//r.h = menuRect->h;

		r.y = rect->y;
		r.h = rect->h;

		//r.y += r.h;
		r.y += height;

		//r.y -= (r.h - height) / 2;
		r.y += scale * 12.0f;
		CG_Text_Paint_Align(&r, scale, color, num, 0, 0, textStyle, font, align);
	}
}

#if 0  //ndef MISSIONPACK // bk001206
static const float healthColors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
  // bk0101016 - float const
  { 1.0f, 0.69f, 0.0f, 1.0f } ,		// normal
  { 1.0f, 0.2f, 0.2f, 1.0f },		// low health
  { 0.5f, 0.5f, 0.5f, 1.0f},		// weapon firing
  { 1.0f, 1.0f, 1.0f, 1.0f } };		// health > 100
#endif

static void CG_DrawPlayerAmmoIcon( const rectDef_t *rect, qboolean draw2D ) {
	//centity_t	*cent;
	//playerState_t	*ps;
	vec3_t		angles;
	vec3_t		origin;
	int weapon;

	if (wolfcam_following) {
		weapon = cg_entities[wcg.clientNum].currentState.weapon;
	} else {
		weapon = cg.predictedPlayerState.weapon;
	}

	//cent = &cg_entities[cg.snap->ps.clientNum];
	//ps = &cg.snap->ps;

	if ( draw2D || (!cg_draw3dIcons.integer && cg_drawIcons.integer) ) { // bk001206 - parentheses
	  qhandle_t	icon;
	  icon = cg_weapons[weapon].ammoIcon;
		if ( icon ) {
		  CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
		}
  } else if (cg_draw3dIcons.integer) {
  	if ( weapon && cg_weapons[weapon].ammoModel ) {
	    VectorClear( angles );
	  	origin[0] = 70;
  		origin[1] = 0;
  		origin[2] = 0;
  		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
  		CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cg_weapons[weapon].ammoModel, 0, origin, angles );
  	}
  }
}

static void CG_DrawPlayerAmmoValue (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align, const rectDef_t *menuRect)
{
	char	num[16];
	int value;
	const centity_t	*cent;
	const playerState_t	*ps;
	rectDef_t r;

	if (wolfcam_following) {
		return;
	}

	memcpy(&r, rect, sizeof(rectDef_t));

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	if ( cent->currentState.weapon  &&  cent->currentState.weapon != WP_GAUNTLET ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if (shader) {
				trap_R_SetColor( color );
				CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
				trap_R_SetColor( NULL );
			} else {
				float height;

				Com_sprintf (num, sizeof(num), "%i", value);
				//value = CG_Text_Width(num, scale, 0, font);
				//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);

				//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, colorBlack, num, 0, 0, textStyle, font);
				//r.y += r.h;  //FIXME 2015-10-23 wtf?
				height = CG_Text_Height(num, scale, 0, font);

				//r.y = menuRect->y;
				//r.h = menuRect->h;

				r.y = rect->y;
				r.h = rect->h;

				//r.y += r.h;
				r.y += height;
				//r.y += (scale * scale) * 12.0f;
				r.y += scale * 12.0f;
				//r.y -= (r.h - height) / 2;

				CG_Text_Paint_Align(&r, scale, color, num, 0, 0, textStyle, font, align);
			}
		} else {
			//CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.infiniteAmmo);
			//Com_Printf("rx %f ry %f rw %f rh %f scale %f \n", rect->x, rect->y, rect->w, rect->h, scale);
			CG_DrawPic(rect->x, rect->y - 22 / 4, 22, 22, cgs.media.infiniteAmmo);
		}
	}

}

static void CG_DrawPlayerHead (const rectDef_t *rect, qboolean draw2D, qboolean useDefaultTeamSkin)
{
	vec3_t		angles;
	float		size, stretch;
	float		frac;
	float		x = rect->x;
	int clientNum;

	if (!cg.snap) {
		return;
	}

	if (wolfcam_following) {
		clientNum = wcg.clientNum;
		return;  //FIXME  damage time
	} else {
		clientNum = cg.snap->ps.clientNum;
	}

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = rect->w * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - rect->w * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = rect->w * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;


	CG_DrawHead( x, rect->y, rect->w, rect->h, clientNum, angles, useDefaultTeamSkin );
}

static void CG_DrawSelectedPlayerHealth( const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font ) {
	const clientInfo_t *ci;
	float value;
	char num[16];

  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
  if (ci) {
		if (shader) {
			trap_R_SetColor( color );
			CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
			trap_R_SetColor( NULL );
		} else {
			Com_sprintf (num, sizeof(num), "%i", ci->health);
			value = CG_Text_Width(num, scale, 0, font);
			CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);
		}
	}
}

static void CG_DrawSelectedPlayerArmor( const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font ) {
	const clientInfo_t *ci;
	float value;
	char num[16];


  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
  if (ci) {
    if (ci->armor > 0) {
			if (shader) {
				trap_R_SetColor( color );
				CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
				trap_R_SetColor( NULL );
			} else {
				Com_sprintf (num, sizeof(num), "%i", ci->armor);
				value = CG_Text_Width(num, scale, 0, font);
				CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);
			}
		}
 	}
}

static qhandle_t CG_StatusHandle(int task) {
	qhandle_t h = cgs.media.assaultShader;
	switch (task) {
		case TEAMTASK_OFFENSE :
			h = cgs.media.assaultShader;
			break;
		case TEAMTASK_DEFENSE :
			h = cgs.media.defendShader;
			break;
		case TEAMTASK_PATROL :
			h = cgs.media.patrolShader;
			break;
		case TEAMTASK_FOLLOW :
			h = cgs.media.followShader;
			break;
		case TEAMTASK_CAMP :
			h = cgs.media.campShader;
			break;
		case TEAMTASK_RETRIEVE :
			h = cgs.media.retrieveShader;
			break;
		case TEAMTASK_ESCORT :
			h = cgs.media.escortShader;
			break;
		default :
			h = cgs.media.assaultShader;
			break;
	}
	return h;
}

static void CG_DrawSelectedPlayerStatus( const rectDef_t *rect ) {
	const clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];

	if (ci) {
		qhandle_t h;
		if (cgs.orderPending) {
			// blink the icon
			if ( cg.time > cgs.orderTime - 2500 && (cg.time >> 9 ) & 1 ) {
				return;
			}
			h = CG_StatusHandle(cgs.currentOrder);
		}	else {
			h = CG_StatusHandle(ci->teamTask);
		}
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h );
	}
}


static void CG_DrawPlayerStatus( const rectDef_t *rect ) {
	const clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];

	if (wolfcam_following) {
		ci = &cgs.clientinfo[wcg.clientNum];
	}

	if (ci) {
		qhandle_t h = CG_StatusHandle(ci->teamTask);
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h);
	}
}


static void CG_DrawSelectedPlayerName( const rectDef_t *rect, float scale, const vec4_t color, qboolean voice, int textStyle, const fontInfo_t *font) {
	const clientInfo_t *ci;
  ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);
  if (ci) {
	  if (ci->clanTag) {
		  CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, va("%s ^7%s", ci->clanTag, ci->name), 0, 0, textStyle, font);
	  } else {
		  CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, ci->name, 0, 0, textStyle, font);
	  }
  }
}

static void CG_DrawSelectedPlayerLocation( const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font ) {
	const clientInfo_t *ci;
  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
  if (ci) {
		const char *p = CG_ConfigString(CS_LOCATIONS + ci->location);
		if (!p || !*p) {
			p = "unknown";
		}
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle, font);
  }
}

static void CG_DrawPlayerLocation( const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font  ) {
	const clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];

	if (wolfcam_following) {
		ci = &cgs.clientinfo[wcg.clientNum];
	}

  if (ci) {
		const char *p = CG_ConfigString(CS_LOCATIONS + ci->location);
		if (!p || !*p) {
			p = "unknown";
		}
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle, font);
  }
}



static void CG_DrawSelectedPlayerWeapon( const rectDef_t *rect ) {
	const clientInfo_t *ci;

  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
  if (ci) {
	  if ( cg_weapons[ci->curWeapon].weaponIcon ) {
	    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_weapons[ci->curWeapon].weaponIcon );
		} else {
  	  CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader);
    }
  }
}

static void CG_DrawPlayerScore (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	int scores = cg.snap->ps.persistant[PERS_SCORE];

	if (wolfcam_following) {
		return;
	}

	if (shader) {
		trap_R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
		trap_R_SetColor( NULL );
	} else {
		if (cgs.gametype == GT_RACE) {
			//FIXME wolfcam_following

			// doesn't fit in PERS_SCORE ?
			scores = cgs.clientinfo[cg.snap->ps.clientNum].score;

			if (scores <= 0  ||  scores >= MAX_RACE_SCORE) {
				CG_Text_Paint_Align(rect, scale, color, va("-"), 0, 0, textStyle, font, align);
			} else {
				CG_Text_Paint_Align(rect, scale, color, va("%is", (int)(round(scores / 1000.0))), 0, 0, textStyle, font, align);
			}
        } else {
			CG_Text_Paint_Align(rect, scale, color, va("%i", scores), 0, 0, textStyle, font, align);
        }
	}
}

static void CG_DrawPlayerItem( const rectDef_t *rect, float scale, qboolean draw2D) {
	int		value;
  vec3_t origin, angles;

  if (wolfcam_following) {
	  return;
  }

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		//Com_Printf("^3register item vis %d\n", value);
		CG_RegisterItemVisuals( value );

		if (qtrue) {
		  CG_RegisterItemVisuals( value );
		  CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icon );
		  //Com_Printf("holdable %d\n", value);
		  //if (value == 34) {
		  if (bg_itemlist[value].giType == IT_POWERUP  &&  bg_itemlist[value].giTag == PW_FLIGHT) {
			  vec4_t color;
			  const char *s;
			  float cw;
			  fontInfo_t *font;
			  float tscale;

			  tscale = scale * 0.25;
			  font = &cgDC.Assets.textFont;

			  s = va("%d%%", (int)((float)cg.snap->ps.stats[STAT_POWERUP_REMAINING] / 16000.0 * 100.0));
			  cw = CG_Text_Width(s, tscale, 0, font);
			  Vector4Set(color, 1, 1, 1, 1);
			  CG_Text_Paint(rect->x + rect->w / 2 - cw / 2 /*+ rect->w*/, rect->y + rect->h, tscale, color, s, 0, 0, 0, font);
		  }
		} else {
 			VectorClear( angles );
			origin[0] = 90;
  		origin[1] = 0;
   		origin[2] = -10;
  		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
			CG_Draw3DModel(rect->x, rect->y, rect->w, rect->h, cg_items[ value ].models[0], 0, origin, angles );
		}
	}

}


static void CG_DrawSelectedPlayerPowerup( const rectDef_t *rect, qboolean draw2D ) {
	const clientInfo_t *ci;
  int j;
  float x, y;

  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
  if (ci) {
    x = rect->x;
    y = rect->y;

		for (j = 0; j < PW_NUM_POWERUPS; j++) {
			if (ci->powerups & (1 << j)) {
				gitem_t	*item;
				item = BG_FindItemForPowerup( j );
				if (item) {
				  CG_DrawPic( x, y, rect->w, rect->h, trap_R_RegisterShader( item->icon ) );
				  //Com_Printf("drawing powerup icon %s\n", item->icon);
					x += 3;
					y += 3;
          return;
				}
			}
		}

  }
}


static void CG_DrawSelectedPlayerHead( const rectDef_t *rect, qboolean draw2D, qboolean voice ) {
	clipHandle_t	cm;
	const clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs, angles;


  ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);

  if (ci) {
  	if ( cg_draw3dIcons.integer ) {
	  	cm = ci->headModel;
  		if ( !cm ) {
  			return;
	  	}

  		// offset the origin y and z to center the head
  		trap_R_ModelBounds( cm, mins, maxs );

	  	origin[2] = -0.5 * ( mins[2] + maxs[2] );
  		origin[1] = 0.5 * ( mins[1] + maxs[1] );

	  	// calculate distance so the head nearly fills the box
  		// assume heads are taller than wide
  		len = 0.7 * ( maxs[2] - mins[2] );
  		origin[0] = len / 0.268;	// len / tan( fov/2 )

  		// allow per-model tweaking
  		VectorAdd( origin, ci->headOffset, origin );

    	angles[PITCH] = 0;
    	angles[YAW] = 180;
    	angles[ROLL] = 0;

      CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, ci->headModel, ci->headSkin, origin, angles );
  	} else if ( cg_drawIcons.integer ) {
	  	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, ci->modelIcon );
	  	//CG_DrawPic( rect->x, rect->y, rect->w, rect->h, ci->modelIconReal );
  	}

  	// if they are deferred, draw a cross out
  	if ( ci->deferred ) {
  		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader );
  	}
  }

}


static void CG_DrawPlayerHealth (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align, const rectDef_t *menuRect)
{
	const playerState_t	*ps;
	int value;
	char	num[16];
	rectDef_t r;
	float height;

	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];

	if (wolfcam_following) {
		value = Wolfcam_PlayerHealth(wcg.clientNum);
		if (value <= INVALID_WOLFCAM_HEALTH) {
			return;
		}
	}

	memcpy(&r, rect, sizeof(rectDef_t));

	//CG_FillRect(rect->x, rect->y, rect->w, rect->h, colorYellow);
	
	if (shader) {
		trap_R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
		trap_R_SetColor( NULL );
	} else {
		Com_sprintf (num, sizeof(num), "%i", value);
		//value = CG_Text_Width(num, scale, 0, font);
		//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);
		//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, colorBlack, num, 0, 0, textStyle, font);
		//CG_Text_Paint_Align(rect, scale, color, num, 0, 0, textStyle, font, align);
		//r.y += r.h;  //FIXME 2015-10-23 why?

		//r.y = menuRect->y;
		//r.h = menuRect->h;

		r.y = rect->y;
		r.h = rect->h;

		height = CG_Text_Height(num, scale, 0, font);
		
		//r.y += r.h;
		r.y += height;


		//r.y -= (r.h - height) / 2;
		r.y += scale * 12.0f;
		CG_Text_Paint_Align(&r, scale, color, num, 0, 0, textStyle, font, align);
	}
}


static void CG_DrawRedScore (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	char num[16];
	if ( cgs.scores1 == SCORE_NOT_PRESENT ) {
		Com_sprintf (num, sizeof(num), "-");
	}
	else {
		Com_sprintf (num, sizeof(num), "%i", cgs.scores1);
	}

	CG_Text_Paint_Align(rect, scale, color, num, 0, 0, textStyle, font, align);
}

static void CG_DrawBlueScore (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	char num[16];

	if ( cgs.scores2 == SCORE_NOT_PRESENT ) {
		Com_sprintf (num, sizeof(num), "-");
	}
	else {
		Com_sprintf (num, sizeof(num), "%i", cgs.scores2);
	}
	CG_Text_Paint_Align(rect, scale, color, num, 0, 0, textStyle, font, align);
}

// FIXME: team name support
static void CG_DrawRedName(const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font, int align ) {
	//CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_redTeamName.string , 0, 0, textStyle, font);
	//Com_Printf("red team name\n");
	CG_Text_Paint_Align(rect, scale, color, cg_redTeamName.string , 0, 0, textStyle, font, align);
	//CG_Text_Paint_Align(rect, scale, color, "fuck you" , 0, 0, textStyle, font, align);
}

static void CG_DrawBlueName(const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font, int align ) {
	//CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_blueTeamName.string, 0, 0, textStyle, font);
	CG_Text_Paint_Align(rect, scale, color, cg_blueTeamName.string, 0, 0, textStyle, font, align);
}

static void CG_DrawBlueFlagName(const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font ) {
  int i;
  for ( i = 0 ; i < cgs.maxclients ; i++ ) {
	  if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1<< PW_BLUEFLAG )) {
		  CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle, font);
      return;
    }
  }
}

static void CG_DrawBlueFlagStatus(const rectDef_t *rect, qhandle_t shader) {
	if (cgs.gametype != GT_CTF  &&  cgs.gametype != GT_1FCTF  &&  cgs.gametype != GT_CTFS) {
		if (cgs.gametype == GT_HARVESTER) {
		  vec4_t color = {0, 0, 1, 1};
		  trap_R_SetColor(color);
	    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.blueCubeIcon );
		  trap_R_SetColor(NULL);
		}
		return;
	}
  if (shader) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
  } else {
	  const gitem_t *item = BG_FindItemForPowerup( PW_BLUEFLAG );
    if (item) {
		  vec4_t color = {0, 0, 1, 1};
		  trap_R_SetColor(color);
	    if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
		    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.blueflag] );
			} else {
		    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
		  trap_R_SetColor(NULL);
	  }
  }
}

static void CG_DrawBlueFlagHead(const rectDef_t *rect) {
  int i;
  for ( i = 0 ; i < cgs.maxclients ; i++ ) {
	  if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1<< PW_BLUEFLAG )) {
      vec3_t angles;
      VectorClear( angles );
 		  angles[YAW] = 180 + 20 * sin( cg.time / 650.0 );;
		  CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0,angles, qtrue );
      return;
    }
  }
}

static void CG_DrawRedFlagName(const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font ) {
  int i;
  for ( i = 0 ; i < cgs.maxclients ; i++ ) {
	  if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1<< PW_REDFLAG )) {
		  CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle, font);
      return;
    }
  }
}

static void CG_DrawRedFlagStatus(const rectDef_t *rect, qhandle_t shader) {
	if (cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF  &&  cgs.gametype != GT_CTFS) {
		if (cgs.gametype == GT_HARVESTER) {
		  vec4_t color = {1, 0, 0, 1};
		  trap_R_SetColor(color);
	    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.redCubeIcon );
		  trap_R_SetColor(NULL);
		}
		return;
	}
  if (shader) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
  } else {
	  const gitem_t *item = BG_FindItemForPowerup( PW_REDFLAG );
    if (item) {
		  vec4_t color = {1, 0, 0, 1};
		  trap_R_SetColor(color);
	    if( cgs.redflag >= 0 && cgs.redflag <= 2) {
		    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.redflag] );
			} else {
		    CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
		  trap_R_SetColor(NULL);
	  }
  }
}

static void CG_DrawRedFlagHead(const rectDef_t *rect) {
  int i;
  for ( i = 0 ; i < cgs.maxclients ; i++ ) {
	  if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1<< PW_REDFLAG )) {
      vec3_t angles;
      VectorClear( angles );
 		  angles[YAW] = 180 + 20 * sin( cg.time / 650.0 );;
		  CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0,angles, qtrue );
      return;
    }
  }
}

static void CG_HarvesterSkulls(const rectDef_t *rect, float scale, const vec4_t color, qboolean force2D, int textStyle, const fontInfo_t *font ) {
	char num[16];
	vec3_t origin, angles;
	qhandle_t handle;
	int value = cg.snap->ps.generic1 & 0x3f;
	float w;

	if (wolfcam_following) {
		return;
	}

	if (cgs.gametype != GT_HARVESTER) {
		return;
	}

	if( value > 99 ) {
		value = 99;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	w = CG_Text_Width(num, scale, 0, font);
	CG_Text_Paint(rect->x + (rect->w - w), rect->y + rect->h, scale, color, num, 0, 0, textStyle, font);

	if (cg_drawIcons.integer) {
		if (!force2D && cg_draw3dIcons.integer) {
			VectorClear(angles);
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
			if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeModel;
			} else {
				handle = cgs.media.blueCubeModel;
			}
			CG_Draw3DModel( rect->x, rect->y, 35, 35, handle, 0, origin, angles );
		} else {
			if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeIcon;
			} else {
				handle = cgs.media.blueCubeIcon;
			}
			CG_DrawPic( rect->x + 3, rect->y + 16, 20, 20, handle );
		}
	}
}

#define FLAG_OFFSET 9

static void CG_OneFlagStatus(const rectDef_t *rect) {
	int yoffset = 0;
	qboolean blueIsFirst;

	if (cgs.gametype != GT_1FCTF) {
		return;
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

		blueIsFirst = qfalse;

		if (CG_ScoresEqual(cgs.scores2, cgs.scores1)) {
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				blueIsFirst = qtrue;
			}
		} else if (cgs.scores2 > cgs.scores1) {
			blueIsFirst = qtrue;
		} else if (cgs.scores2 < cgs.scores1) {
			blueIsFirst = qfalse;
		} else {  // scores tied, so demo pov is first
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				blueIsFirst = qtrue;
			}
		}

		if (item) {
			if( cgs.flagStatus >= 0 && cgs.flagStatus <= 4 ) {
				vec4_t color = {1, 1, 1, 1};
				int index = 0;
				if (cgs.flagStatus == FLAG_TAKEN_RED) {
					color[1] = color[2] = 0;
					index = 1;
					if (blueIsFirst) {
						yoffset = FLAG_OFFSET;
					} else {
						yoffset = -FLAG_OFFSET;
					}
				} else if (cgs.flagStatus == FLAG_TAKEN_BLUE) {
					color[0] = color[1] = 0;
					index = 1;
					if (blueIsFirst) {
						yoffset = -FLAG_OFFSET;
					} else {
						yoffset = FLAG_OFFSET;
					}
				} else if (cgs.flagStatus == FLAG_DROPPED) {
					index = 2;
				}
			  trap_R_SetColor(color);
				CG_DrawPic( rect->x, rect->y + yoffset, rect->w, rect->h, cgs.media.flagShaders[index] );
			}
		}
	}
}

#undef FLAG_OFFSET

static void CG_DrawCTFPowerUp(const rectDef_t *rect) {
	int		value;

	if (cgs.protocol == PROTOCOL_Q3) {
		return;
	}

	if (wolfcam_following) {
		return;  //FIXME
	}

	if (cgs.gametype < GT_CTF  ||  cgs.gametype == GT_FREEZETAG) {
		return;
	}
	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		//Com_Printf("ctf powerup %d\n", value);
		if (cgs.protocol == PROTOCOL_QL) {
			//FIXME  this is absolutely fucked up, why is this happening?????

			if (value == 0) {
				value = 39;
			} else {
				value += 42;
			}
		}
		CG_RegisterItemVisuals( value );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icon );
	}
}



static void CG_DrawTeamColor(const rectDef_t *rect, const vec4_t color) {
	if (wolfcam_following) {
		CG_DrawTeamBackground(rect->x, rect->y, rect->w, rect->h, color[3], cgs.clientinfo[wcg.clientNum].team);
	} else {
		CG_DrawTeamBackground(rect->x, rect->y, rect->w, rect->h, color[3], cg.snap->ps.persistant[PERS_TEAM]);
	}
}

static void CG_DrawAreaPowerUp(const rectDef_t *rect, int align, float special, float scale, const vec4_t color, const fontInfo_t *font) {
	char num[16];
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	const playerState_t	*ps;
	int		t;
	const gitem_t	*item;
	float	f;
	rectDef_t r2;
	float *inc;
	int origX;
	int origY;

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_TOURNAMENT) {
		//return;
	}

	if (wolfcam_following) {
		return;  //FIXME
	}

	r2.x = rect->x;
	r2.y = rect->y;
	r2.w = rect->w;
	r2.h = rect->h;

	inc = (align == HUD_VERTICAL) ? &r2.y : &r2.x;

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0  ||  ps->eFlags & EF_DEAD) {
		//Com_Printf("returning\n");
		//return;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		//Com_Printf("powerup: %d\n", i);

		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t <= 0 || t >= 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if (item) {
			t = ps->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap_R_SetColor( NULL );
			} else {
				vec4_t	modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor( modulate );
			}


			//Com_Printf("drawing powerup %s  sorted[i] %d\n", item->icon, sorted[i]);
			if (sorted[i] != 0) {  // 2010-08-08 new spawn protection powerup
				CG_DrawPic( r2.x, r2.y, r2.w * .75, r2.h, trap_R_RegisterShader( item->icon ) );
				Com_sprintf (num, sizeof(num), "%i", sortedTime[i] / 1000);
				CG_Text_Paint(r2.x + (r2.w * .75) + 3 , r2.y + r2.h, scale, color, num, 0, 0, 0, font);
				origX = r2.x;
				origY = r2.y;
				*inc += r2.w + special;

				if (item->giTag == PW_QUAD  &&  cgs.protocol == PROTOCOL_QL  &&  cg_quadKillCounter.integer == 1  &&  ps->stats[STAT_QUAD_KILL_COUNT] > 0) {
					float sc;

					sc = 0.25;
					//Com_Printf("%d\n", ps->stats[STAT_QUAD_KILL_COUNT]);
					CG_DrawPic(origX + (r2.w * 0.75) + 3, (float)origY + (float)r2.w * 0.10, r2.w * sc, r2.h * sc, cgs.media.redCubeIcon);
					//CG_Text_Paint(origX + (r2.w * 0.75) + 3 + r2.w * sc, origY + r2.h, scale * 0.25, color, va("x %d", ps->stats[STAT_QUAD_KILL_COUNT]), 0, 0, 0, font);
					CG_Text_Paint(origX + (r2.w * 0.75) + 3 + r2.w * sc, origY + (float)r2.h * (sc + 0.10), scale * 0.33, color, va(" x %d", ps->stats[STAT_QUAD_KILL_COUNT]), 0, 0, 0, font);
				}
				if (item->giTag == PW_BATTLESUIT  &&  cgs.protocol == PROTOCOL_QL  &&  cg_battleSuitKillCounter.integer == 1  &&  ps->stats[STAT_BATTLE_SUIT_KILL_COUNT] > 0) {
					float sc;

					sc = 0.25;
					//Com_Printf("%d\n", ps->stats[STAT_QUAD_KILL_COUNT]);
					CG_DrawPic(origX + (r2.w * 0.75) + 3, (float)origY + (float)r2.w * 0.10, r2.w * sc, r2.h * sc, cgs.media.redCubeIcon);
					//CG_Text_Paint(origX + (r2.w * 0.75) + 3 + r2.w * sc, origY + r2.h, scale * 0.25, color, va("x %d", ps->stats[STAT_QUAD_KILL_COUNT]), 0, 0, 0, font);
					CG_Text_Paint(origX + (r2.w * 0.75) + 3 + r2.w * sc, origY + (float)r2.h * (sc + 0.10), scale * 0.33, color, va(" x %d", ps->stats[STAT_BATTLE_SUIT_KILL_COUNT]), 0, 0, 0, font);
				}
			}
		}

	}
	trap_R_SetColor( NULL );

}

static qboolean CG_HaveWeapon (int weapon)
{
	if (wolfcam_following) {
		if (cg_entities[wcg.clientNum].currentState.weapon == weapon) {
			return qtrue;
		} else {
			return qfalse;
		}
	} else {
		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << weapon)) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
}

//FIXME duplicate code in CG_DrawWeaponSelect()
static int CG_NumHeldWeapons (void)
{
	int count;
	int bits;
	int i;

	if (wolfcam_following) {
		bits = (1 << cg_entities[wcg.clientNum].currentState.weapon);
	} else {
		bits = cg.snap->ps.stats[STAT_WEAPONS];
	}

	count = 0;
	//FIXME i < 16
	for (i = 1;  i < 16;  i++) {
		if (bits & (1 << i)) {
			count++;
		}
	}

	return count;
}

static int CG_WeaponAmmo (int weapon)
{
	if (wolfcam_following) {
		return W_AMMO_UNKNOWN;  //FIXME hack
	}

	return cg.snap->ps.ammo[weapon];
}

float CG_GetValue(int ownerDraw) {
	const centity_t	*cent;
 	const clientInfo_t *ci;
	const playerState_t	*ps;
	int ourClientNum;
	const score_t *score;
	int i;
	int count;
	float f;

	if (!cg.snap) {
		return 0;
	}

  cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}
	ci = &cgs.clientinfo[ourClientNum];
	score = &cg.scores[ci->scoreIndexNum];

  switch (ownerDraw) {
  case CG_SELECTEDPLAYER_ARMOR:
    ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
    return ci->armor;
    break;
  case CG_SELECTEDPLAYER_HEALTH:
    ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
    return ci->health;
    break;
  case CG_PLAYER_ARMOR_VALUE:
	  if (wolfcam_following) {
		  int val;

		  val = Wolfcam_PlayerArmor(wcg.clientNum);
		  if (val < 0) {
			  return 0;
		  } else {
			  return val;
		  }
	  } else {
		  return ps->stats[STAT_ARMOR];
	  }
    break;
  case CG_PLAYER_AMMO_VALUE:
	  if (wolfcam_following) {
		  return -1;  //FIXME  see if any side effects
	  } else {
		if ( cent->currentState.weapon ) {
		  return ps->ammo[cent->currentState.weapon];
		}
	  }
    break;
  case CG_PLAYER_SCORE:
	  if (wolfcam_following) {
		  return -1;
	  } else {
		  return cg.snap->ps.persistant[PERS_SCORE];
	  }
    break;
  case CG_PLAYER_HEALTH:
	  if (wolfcam_following) {
		  int value;

		  value = Wolfcam_PlayerHealth(wcg.clientNum);
		  return value;
#if 0
		  value = wclients[wcg.clientNum].eventHealth;  //FIXME time, team info
		  if (value >= 9999) {
			  if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
				  return 0;
			  } else {
				  return -1;
			  }
		  }
		  return value;
#endif
	  } else {
		  return ps->stats[STAT_HEALTH];
	  }
    break;
  case CG_RED_SCORE:
		return cgs.scores1;
    break;
  case CG_BLUE_SCORE:
		return cgs.scores2;
    break;

	/////////////////// new from here

  case CG_SELECTEDPLAYER_WEAPON:
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  return ci->curWeapon;
	  break;

#if 0
  case CG_SELECTEDPLAYER_POWERUP:
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  return ci->health;
	  break;
#endif

	  ////CG_FLAGCARRIER_WEAPON
	  ////CG_FLAGCARRIER_POWERUP

  case CG_HARVESTER_SKULLS:
	  if (wolfcam_following) {
		  return 0;
	  } else {
		  if (cg.snap) {
			  return (cg.snap->ps.generic1 & 0x3f);
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_ONEFLAG_STATUS:
	  return cgs.flagStatus;
	  break;

	  ////CG_PLAYER_HASFLAG

  case CG_GAME_TYPE:
	  return cgs.gametype;
	  break;

  case CG_ACCURACY:
	  if (ci->scoreValid) {
		  return score->accuracy;
	  } else {
		  return 0;
	  }
	  break;

  case CG_ASSISTS:
	  if (ci->scoreValid) {
		  return score->assistCount;
	  } else {
		  return 0;
	  }
	  break;

  case CG_DEFEND:
	  if (ci->scoreValid) {
		  return score->defendCount;
	  } else {
		  return 0;
	  }
	  break;

  case CG_CAPTURES:
	  if (ci->scoreValid) {
		  return score->captures;
	  } else {
		  return 0;
	  }
	  break;

  case CG_EXCELLENT:
	  if (ci->scoreValid) {
		  return score->excellentCount;
	  } else {
		  return 0;
	  }
	  break;

  case CG_IMPRESSIVE:
	  if (ci->scoreValid) {
		  return score->impressiveCount;
	  } else {
		  return 0;
	  }
	  break;

  case CG_PERFECT:
	  if (ci->scoreValid) {
		  return score->perfect;
	  } else {
		  return 0;
	  }
	  break;

  case CG_GAUNTLET:
	  if (ci->scoreValid) {
		  return score->gauntletCount;
	  } else {
		  return 0;
	  }
	  break;

  case CG_CAPFRAGLIMIT: {
	  int limit;

	  if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_RED_ROVER) {
		  limit = cgs.roundlimit;
	  } else if (cgs.gametype == GT_TOURNAMENT  ||  cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_RACE) {
		  limit = cgs.timelimit;
	  } else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF) {
		  if (cgs.capturelimit) {
			  limit = cgs.capturelimit;
		  } else {
			  limit = cgs.timelimit;
		  }
	  } else if (cgs.gametype == GT_DOMINATION  ||  cgs.gametype == GT_CTFS) {
		  limit = cgs.scorelimit;
	  } else {
		  limit = cgs.fraglimit;
	  }

	  return limit;
	  break;
  }
	  ////CG_LEVELTIMER  ??

	  //FIXME ugggggggggg
	//CG_1ST_PLACE_SCORE
	//CG_2ND_PLACE_SCORE


  case CG_SELECTED_PLYR_ACCURACY:
#if 0
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (ci->scoreValid) {
		  return cg.scores[ci->scoreIndexNum].accuracy;
	  } else {
		  return 0;
	  }
#endif
	  return cg.scores[cg.selectedScore].accuracy;
	  break;

  case CG_PLAYER_COUNTS:
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  if (!cgs.clientinfo[i].infoValid) {
			  continue;
		  }
		  // ql includes the specs in player count
		  count++;
	  }
	  return count;
	  break;

  case CG_RED_PLAYER_COUNT:
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  if (cgs.clientinfo[i].team != TEAM_RED) {
			  continue;
		  }
		  count++;
	  }
	  return count;
	  break;

  case CG_BLUE_PLAYER_COUNT:
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  if (cgs.clientinfo[i].team != TEAM_BLUE) {
			  continue;
		  }
		  count++;
	  }
	  return count;
	  break;

  case CG_RED_CLAN_PLYRS:
	  return cgs.redPlayersLeft;
	  break;

  case CG_BLUE_CLAN_PLYRS:
	  return cgs.bluePlayersLeft;
	  break;

	  //FIXME check
	//CG_GAME_LIMIT

	////CG_ROUNDTIMER ??

  case CG_RED_TIMEOUT_COUNT:
	  f = 0;
	  if (cgs.protocol == PROTOCOL_QL) {
		  f = atof(CG_ConfigString(CS_RED_TEAM_TIMEOUTS_LEFT));
	  }
	  return f;
	  break;

  case CG_BLUE_TIMEOUT_COUNT:
	  f = 0;
	  if (cgs.protocol == PROTOCOL_QL) {
		  f = atof(CG_ConfigString(CS_BLUE_TEAM_TIMEOUTS_LEFT));
	  }
	  return f;
	  break;

  case CG_SPEEDOMETER:
	  //FIXME duplicate code
	  f = 0;
	  if (!wolfcam_following) {
		  f = cg.xyspeed;
	  } else {
		  if (wcg.clientNum != cg.snap->ps.clientNum) {
			  const entityState_t *es;

			  es = &cg_entities[wcg.clientNum].currentState;
			  f = sqrt (es->pos.trDelta[0] * es->pos.trDelta[0] + es->pos.trDelta[1] * es->pos.trDelta[1]);
		  } else {
			  const playerState_t *ps;

			  //ps = &cg.predictedPlayerState;
			  ps = &cg.snap->ps;
			  f = sqrt( ps->velocity[0] * ps->velocity[0] +
							ps->velocity[1] * ps->velocity[1] );
		  }
	  }
	  return f;
	  break;

  case CG_1ST_PLYR_SCORE:
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  return cg.duelScores[0].score;
	  } else {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
			  return cgs.clientinfo[cg.duelPlayer1].score;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_FRAGS:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].kills;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].frags;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_DEATHS:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].deaths;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].deaths;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_DMG:
	  return cg.duelScores[0].damage;
	  break;

  case CG_1ST_PLYR_TIME:
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  return cg.duelScores[0].time;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].time;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_PING:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].ping;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].ping;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_WINS:
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
		  return cgs.clientinfo[cg.duelPlayer1].wins;
	  } else {
		  return 0;
	  }
	  break;

  case CG_1ST_PLYR_ACC:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].accuracy;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].accuracy;
		  } else {
			  return 0;
		  }
	  }
	  break;

	  //FIXME
	//CG_1ST_PLYR_TIMEOUT_COUNT

  case CG_2ND_PLYR_SCORE:
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  return cg.duelScores[1].score;
	  } else {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  return cgs.clientinfo[cg.duelPlayer2].score;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_FRAGS:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].kills;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].frags;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_DEATHS:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].deaths;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].deaths;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_DMG:
	  return cg.duelScores[1].damage;
	  break;

  case CG_2ND_PLYR_TIME:
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  return cg.duelScores[1].time;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].time;
		  } else {
			  return 0;
		  }
	  }
	  break;
  case CG_2ND_PLYR_PING:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].ping;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].ping;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_WINS:
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
		  return cgs.clientinfo[cg.duelPlayer2].wins;
	  } else {
		  return 0;
	  }
	  break;

  case CG_2ND_PLYR_ACC:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].accuracy;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].accuracy;
		  } else {
			  return 0;
		  }
	  }
	  break;

	  //FIXME
	//CG_2ND_PLYR_TIMEOUT_COUNT

  case CG_RED_AVG_PING:
	  return cg.avgRedPing;
	  break;

  case CG_BLUE_AVG_PING:
	  return cg.avgBluePing;
	  break;

  case CG_VOTECOUNT1:
	  //FIXME cache
	  return atof(Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "0"));
	  break;

  case CG_VOTECOUNT2:
	  return atof(Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "1"));
	  break;

  case CG_VOTECOUNT3:
	  return atof(Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "2"));
	  break;

	//CG_VOTETIMER  ??
	//CG_LOCALTIME  ??

  case CG_1ST_PLYR_FRAGS_G:
	  return cg.duelScores[0].weaponStats[WP_GAUNTLET].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_MG:
	  return cg.duelScores[0].weaponStats[WP_MACHINEGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_HMG:
	  return cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_SG:
	  return cg.duelScores[0].weaponStats[WP_SHOTGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_GL:
	  return cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_RL:
	  return cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_LG:
	  return cg.duelScores[0].weaponStats[WP_LIGHTNING].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_RG:
	  return cg.duelScores[0].weaponStats[WP_RAILGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_PG:
	  return cg.duelScores[0].weaponStats[WP_PLASMAGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_BFG:
	  return cg.duelScores[0].weaponStats[WP_BFG].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_CG:
	  return cg.duelScores[0].weaponStats[WP_CHAINGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_NG:
	  return cg.duelScores[0].weaponStats[WP_NAILGUN].kills;
	  break;

  case CG_1ST_PLYR_FRAGS_PL:
	  return cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].kills;
	  break;

  case CG_1ST_PLYR_HITS_MG:
	  return cg.duelScores[0].weaponStats[WP_MACHINEGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_HMG:
	  return cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_SG:
	  return cg.duelScores[0].weaponStats[WP_SHOTGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_GL:
	  return cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].hits;
	  break;

  case CG_1ST_PLYR_HITS_RL:
	  return cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].hits;
	  break;

  case CG_1ST_PLYR_HITS_LG:
	  return cg.duelScores[0].weaponStats[WP_LIGHTNING].hits;
	  break;

  case CG_1ST_PLYR_HITS_RG:
	  return cg.duelScores[0].weaponStats[WP_RAILGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_PG:
	  return cg.duelScores[0].weaponStats[WP_PLASMAGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_BFG:
	  return cg.duelScores[0].weaponStats[WP_BFG].hits;
	  break;

  case CG_1ST_PLYR_HITS_CG:
	  return cg.duelScores[0].weaponStats[WP_CHAINGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_NG:
	  return cg.duelScores[0].weaponStats[WP_NAILGUN].hits;
	  break;

  case CG_1ST_PLYR_HITS_PL:
	  return cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].hits;
	  break;

  case CG_1ST_PLYR_SHOTS_MG:
	  return cg.duelScores[0].weaponStats[WP_MACHINEGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_HMG:
	  return cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_SG:
	  return cg.duelScores[0].weaponStats[WP_SHOTGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_GL:
	  return cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_RL:
	  return cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_LG:
	  return cg.duelScores[0].weaponStats[WP_LIGHTNING].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_RG:
	  return cg.duelScores[0].weaponStats[WP_RAILGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_PG:
	  return cg.duelScores[0].weaponStats[WP_PLASMAGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_BFG:
	  return cg.duelScores[0].weaponStats[WP_BFG].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_CG:
	  return cg.duelScores[0].weaponStats[WP_CHAINGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_NG:
	  return cg.duelScores[0].weaponStats[WP_NAILGUN].atts;
	  break;

  case CG_1ST_PLYR_SHOTS_PL:
	  return cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].atts;
	  break;

  case CG_1ST_PLYR_DMG_G:
	  return cg.duelScores[0].weaponStats[WP_GAUNTLET].damage;
	  break;

  case CG_1ST_PLYR_DMG_MG:
	  return cg.duelScores[0].weaponStats[WP_MACHINEGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_HMG:
	  return cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_SG:
	  return cg.duelScores[0].weaponStats[WP_SHOTGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_GL:
	  return cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].damage;
	  break;

  case CG_1ST_PLYR_DMG_RL:
	  return cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].damage;
	  break;

  case CG_1ST_PLYR_DMG_LG:
	  return cg.duelScores[0].weaponStats[WP_LIGHTNING].damage;
	  break;

  case CG_1ST_PLYR_DMG_RG:
	  return cg.duelScores[0].weaponStats[WP_RAILGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_PG:
	  return cg.duelScores[0].weaponStats[WP_PLASMAGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_BFG:
	  return cg.duelScores[0].weaponStats[WP_BFG].damage;
	  break;

  case CG_1ST_PLYR_DMG_CG:
	  return cg.duelScores[0].weaponStats[WP_CHAINGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_NG:
	  return cg.duelScores[0].weaponStats[WP_NAILGUN].damage;
	  break;

  case CG_1ST_PLYR_DMG_PL:
	  return cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].damage;
	  break;

  case CG_1ST_PLYR_ACC_MG:
	  return cg.duelScores[0].weaponStats[WP_MACHINEGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_HMG:
	  return cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_SG:
	  return cg.duelScores[0].weaponStats[WP_SHOTGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_GL:
	  return cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_RL:
	  return cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_LG:
	  return cg.duelScores[0].weaponStats[WP_LIGHTNING].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_RG:
	  return cg.duelScores[0].weaponStats[WP_RAILGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_PG:
	  return cg.duelScores[0].weaponStats[WP_PLASMAGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_BFG:
	  return cg.duelScores[0].weaponStats[WP_BFG].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_CG:
	  return cg.duelScores[0].weaponStats[WP_CHAINGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_NG:
	  return cg.duelScores[0].weaponStats[WP_NAILGUN].accuracy;
	  break;

  case CG_1ST_PLYR_ACC_PL:
	  return cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].accuracy;
	  break;

  case CG_1ST_PLYR_PICKUPS_RA:
	  return cg.duelScores[0].redArmorPickups;
	  break;

  case CG_1ST_PLYR_PICKUPS_YA:
	  return cg.duelScores[0].yellowArmorPickups;
	  break;

  case CG_1ST_PLYR_PICKUPS_GA:
	  return cg.duelScores[0].greenArmorPickups;
	  break;

  case CG_1ST_PLYR_PICKUPS_MH:
	  return cg.duelScores[0].megaHealthPickups;
	  break;

  case CG_1ST_PLYR_AVG_PICKUP_TIME_RA:
	  return cg.duelScores[0].redArmorTime;
	  break;

  case CG_1ST_PLYR_AVG_PICKUP_TIME_YA:
	  return cg.duelScores[0].yellowArmorTime;
	  break;

  case CG_1ST_PLYR_AVG_PICKUP_TIME_GA:
	  return cg.duelScores[0].greenArmorTime;
	  break;

  case CG_1ST_PLYR_AVG_PICKUP_TIME_MH:
	  return cg.duelScores[0].megaHealthTime;
	  break;

  // now player 2

  case CG_2ND_PLYR_FRAGS_G:
	  return cg.duelScores[1].weaponStats[WP_GAUNTLET].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_MG:
	  return cg.duelScores[1].weaponStats[WP_MACHINEGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_HMG:
	  return cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_SG:
	  return cg.duelScores[1].weaponStats[WP_SHOTGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_GL:
	  return cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_RL:
	  return cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_LG:
	  return cg.duelScores[1].weaponStats[WP_LIGHTNING].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_RG:
	  return cg.duelScores[1].weaponStats[WP_RAILGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_PG:
	  return cg.duelScores[1].weaponStats[WP_PLASMAGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_BFG:
	  return cg.duelScores[1].weaponStats[WP_BFG].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_CG:
	  return cg.duelScores[1].weaponStats[WP_CHAINGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_NG:
	  return cg.duelScores[1].weaponStats[WP_NAILGUN].kills;
	  break;

  case CG_2ND_PLYR_FRAGS_PL:
	  return cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].kills;
	  break;

  case CG_2ND_PLYR_HITS_MG:
	  return cg.duelScores[1].weaponStats[WP_MACHINEGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_HMG:
	  return cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_SG:
	  return cg.duelScores[1].weaponStats[WP_SHOTGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_GL:
	  return cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].hits;
	  break;

  case CG_2ND_PLYR_HITS_RL:
	  return cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].hits;
	  break;

  case CG_2ND_PLYR_HITS_LG:
	  return cg.duelScores[1].weaponStats[WP_LIGHTNING].hits;
	  break;

  case CG_2ND_PLYR_HITS_RG:
	  return cg.duelScores[1].weaponStats[WP_RAILGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_PG:
	  return cg.duelScores[1].weaponStats[WP_PLASMAGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_BFG:
	  return cg.duelScores[1].weaponStats[WP_BFG].hits;
	  break;

  case CG_2ND_PLYR_HITS_CG:
	  return cg.duelScores[1].weaponStats[WP_CHAINGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_NG:
	  return cg.duelScores[1].weaponStats[WP_NAILGUN].hits;
	  break;

  case CG_2ND_PLYR_HITS_PL:
	  return cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].hits;
	  break;

  case CG_2ND_PLYR_SHOTS_MG:
	  return cg.duelScores[1].weaponStats[WP_MACHINEGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_HMG:
	  return cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_SG:
	  return cg.duelScores[1].weaponStats[WP_SHOTGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_GL:
	  return cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_RL:
	  return cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_LG:
	  return cg.duelScores[1].weaponStats[WP_LIGHTNING].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_RG:
	  return cg.duelScores[1].weaponStats[WP_RAILGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_PG:
	  return cg.duelScores[1].weaponStats[WP_PLASMAGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_BFG:
	  return cg.duelScores[1].weaponStats[WP_BFG].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_CG:
	  return cg.duelScores[1].weaponStats[WP_CHAINGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_NG:
	  return cg.duelScores[1].weaponStats[WP_NAILGUN].atts;
	  break;

  case CG_2ND_PLYR_SHOTS_PL:
	  return cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].atts;
	  break;

  case CG_2ND_PLYR_DMG_G:
	  return cg.duelScores[1].weaponStats[WP_GAUNTLET].damage;
	  break;

  case CG_2ND_PLYR_DMG_MG:
	  return cg.duelScores[1].weaponStats[WP_MACHINEGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_HMG:
	  return cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_SG:
	  return cg.duelScores[1].weaponStats[WP_SHOTGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_GL:
	  return cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].damage;
	  break;

  case CG_2ND_PLYR_DMG_RL:
	  return cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].damage;
	  break;

  case CG_2ND_PLYR_DMG_LG:
	  return cg.duelScores[1].weaponStats[WP_LIGHTNING].damage;
	  break;

  case CG_2ND_PLYR_DMG_RG:
	  return cg.duelScores[1].weaponStats[WP_RAILGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_PG:
	  return cg.duelScores[1].weaponStats[WP_PLASMAGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_BFG:
	  return cg.duelScores[1].weaponStats[WP_BFG].damage;
	  break;

  case CG_2ND_PLYR_DMG_CG:
	  return cg.duelScores[1].weaponStats[WP_CHAINGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_NG:
	  return cg.duelScores[1].weaponStats[WP_NAILGUN].damage;
	  break;

  case CG_2ND_PLYR_DMG_PL:
	  return cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].damage;
	  break;

  case CG_2ND_PLYR_ACC_MG:
	  return cg.duelScores[1].weaponStats[WP_MACHINEGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_HMG:
	  return cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_SG:
	  return cg.duelScores[1].weaponStats[WP_SHOTGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_GL:
	  return cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_RL:
	  return cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_LG:
	  return cg.duelScores[1].weaponStats[WP_LIGHTNING].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_RG:
	  return cg.duelScores[1].weaponStats[WP_RAILGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_PG:
	  return cg.duelScores[1].weaponStats[WP_PLASMAGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_BFG:
	  return cg.duelScores[1].weaponStats[WP_BFG].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_CG:
	  return cg.duelScores[1].weaponStats[WP_CHAINGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_NG:
	  return cg.duelScores[1].weaponStats[WP_NAILGUN].accuracy;
	  break;

  case CG_2ND_PLYR_ACC_PL:
	  return cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].accuracy;
	  break;

  case CG_2ND_PLYR_PICKUPS_RA:
	  return cg.duelScores[1].redArmorPickups;
	  break;

  case CG_2ND_PLYR_PICKUPS_YA:
	  return cg.duelScores[1].yellowArmorPickups;
	  break;

  case CG_2ND_PLYR_PICKUPS_GA:
	  return cg.duelScores[1].greenArmorPickups;
	  break;

  case CG_2ND_PLYR_PICKUPS_MH:
	  return cg.duelScores[1].megaHealthPickups;
	  break;

  case CG_2ND_PLYR_AVG_PICKUP_TIME_RA:
	  return cg.duelScores[1].redArmorTime;
	  break;

  case CG_2ND_PLYR_AVG_PICKUP_TIME_YA:
	  return cg.duelScores[1].yellowArmorTime;
	  break;

  case CG_2ND_PLYR_AVG_PICKUP_TIME_GA:
	  return cg.duelScores[1].greenArmorTime;
	  break;

  case CG_2ND_PLYR_AVG_PICKUP_TIME_MH:
	  return cg.duelScores[1].megaHealthTime;
	  break;

  case CG_1ST_PLYR_EXCELLENT:
	  return cg.duelScores[0].awardExcellent;
	  break;

  case CG_1ST_PLYR_IMPRESSIVE:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].awardImpressive;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].impressiveCount;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_HUMILIATION:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[0].awardHumiliation;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].gauntletCount;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_EXCELLENT:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].awardExcellent;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].excellentCount;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_IMPRESSIVE:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].awardImpressive;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].impressiveCount;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_2ND_PLYR_HUMILIATION:
	  if (cg.duelScoresValid) {
		  return cg.duelScores[1].awardHumiliation;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  return cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].gauntletCount;
		  } else {
			  return 0;
		  }
	  }
	  break;

  case CG_1ST_PLYR_READY:
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
		  return cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << cg.duelPlayer1);
	  } else {
		  return 0;
	  }
	  break;

  case CG_2ND_PLYR_READY:
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
		  return cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << cg.duelPlayer2);
	  } else {
		  return 0;
	  }
	  break;

	  //FIXME
	  //#define CG_SELECTED_PLYR_SCORE 291
	  //#define CG_SELECTED_PLYR_DMG 292
	  //#define CG_SELECTED_PLYR_ACC 293

	  //#define CG_SELECTED_PLYR_PICKUPS_RA 296
	  //#define CG_SELECTED_PLYR_PICKUPS_YA 297
	  //#define CG_SELECTED_PLYR_PICKUPS_GA 298
	  //#define CG_SELECTED_PLYR_PICKUPS_MH 299

	  //#define CG_1ST_PLYR_PR 300  // no
	  //#define CG_2ND_PLYR_PR 301  // no
	  //#define CG_1ST_PLYR_TIER 302  // no
	  //#define CG_2ND_PLYR_TIER 303  // no


  case CG_RED_TEAM_PICKUPS_RA:
	  //FIXME return -1 for invalid tdmScore ?
	  return cg.tdmScore.rra;
	  break;

  case CG_RED_TEAM_PICKUPS_YA:
	  return cg.tdmScore.rya;
	  break;

  case CG_RED_TEAM_PICKUPS_GA:
	  return cg.tdmScore.rga;
	  break;

  case CG_RED_TEAM_PICKUPS_MH:
	  return cg.tdmScore.rmh;
	  break;

  case CG_RED_TEAM_PICKUPS_QUAD:
	  return cg.tdmScore.rquad;
	  break;

  case CG_RED_TEAM_PICKUPS_BS:
	  return cg.tdmScore.rbs;
	  break;

  case CG_BLUE_TEAM_PICKUPS_RA:
	  return cg.tdmScore.bra;
	  break;

  case CG_BLUE_TEAM_PICKUPS_YA:
	  return cg.tdmScore.bya;
	  break;

  case CG_BLUE_TEAM_PICKUPS_GA:
	  return cg.tdmScore.bga;
	  break;

  case CG_BLUE_TEAM_PICKUPS_MH:
	  return cg.tdmScore.bmh;
	  break;

  case CG_BLUE_TEAM_PICKUPS_QUAD:
	  return cg.tdmScore.bquad;
	  break;

  case CG_BLUE_TEAM_PICKUPS_BS:
	  return cg.tdmScore.bbs;
	  break;

  case CG_RED_TEAM_TIMEHELD_QUAD:
	  return cg.tdmScore.rquadTime;
	  break;

  case CG_RED_TEAM_TIMEHELD_BS:
	  return cg.tdmScore.rbsTime;
	  break;

  case CG_BLUE_TEAM_TIMEHELD_QUAD:
	  return cg.tdmScore.bquadTime;
	  break;

  case CG_BLUE_TEAM_TIMEHELD_BS:
	  return cg.tdmScore.bbsTime;
	  break;

  //FIXME
  //#define CG_RED_TEAM_PICKUPS_FLAG 321
  //#define CG_RED_TEAM_PICKUPS_MEDKIT 322
	  //#define CG_RED_TEAM_PICKUPS_REGEN 323
	  //#define CG_RED_TEAM_PICKUPS_HASTE 324
	  //#define CG_RED_TEAM_PICKUPS_INVIS 325
	  //#define CG_BLUE_TEAM_PICKUPS_FLAG 326
	  //#define CG_BLUE_TEAM_PICKUPS_MEDKIT 327
	  //#define CG_BLUE_TEAM_PICKUPS_REGEN 328
	  //#define CG_BLUE_TEAM_PICKUPS_HASTE 329
	  //#define CG_BLUE_TEAM_PICKUPS_INVIS 330
	  //#define CG_RED_TEAM_TIMEHELD_FLAG 331
	  //#define CG_RED_TEAM_TIMEHELD_REGEN 332
	  //#define CG_RED_TEAM_TIMEHELD_HASTE 333
	  //#define CG_RED_TEAM_TIMEHELD_INVIS 334
	  //#define CG_BLUE_TEAM_TIMEHELD_FLAG 335
	  //#define CG_BLUE_TEAM_TIMEHELD_REGEN 336
	  //#define CG_BLUE_TEAM_TIMEHELD_HASTE 337
	  //#define CG_BLUE_TEAM_TIMEHELD_INVIS 338

  case CG_RED_OWNED_FLAGS:
	  return cgs.dominationRedPoints;
	  break;

  case CG_BLUE_OWNED_FLAGS:
	  return cgs.dominationBluePoints;
	  break;

  case CG_TEAM_PLYR_COUNT: {
	  //FIXME duplicate code
	  int ourTeam;

	  if (wolfcam_following) {
		  ourTeam = cgs.clientinfo[wcg.clientNum].team;
	  } else {
		  ourTeam = cg.snap->ps.persistant[PERS_TEAM];
	  }

	  if (ourTeam == TEAM_BLUE) {
		  return cgs.bluePlayersLeft;
	  } else {
		  return cgs.redPlayersLeft;
	  }
	  break;
  }

  case CG_ENEMY_PLYR_COUNT: {
	  //FIXME duplicate code
	  int ourTeam;

	  if (wolfcam_following) {
		  ourTeam = cgs.clientinfo[wcg.clientNum].team;
	  } else {
		  ourTeam = cg.snap->ps.persistant[PERS_TEAM];
	  }

	  if (ourTeam == TEAM_BLUE) {
		  return cgs.redPlayersLeft;
	  } else {
		  return cgs.bluePlayersLeft;
	  }
	  break;
  }

//CG_ROUND ???

  ////  wolfcam ownerdraws

  case WCG_GAME_STATUS:
	  if (cg.warmup) {
		  return W_STATUS_WARMUP;
	  } else if (cg.snap  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
		  return W_STATUS_INTERMISSION;
	  } else if (cg.snap) {
		  return W_STATUS_PLAYING;
	  } else {
		  return W_STATUS_DISCONNECTED;
	  }
	  break;

  case WCG_WEAPON_SELECTED:
	  if (wolfcam_following) {
		  return cg_entities[wcg.clientNum].currentState.weapon;
	  } else {
		  if (!cg.demoPlayback) {
			  return cg.weaponSelect;
		  } else {
			  return cg.snap->ps.weapon;
		  }
	  }
	  break;

  case WCG_WEAPON_SELECT_TIME:
	  if (wolfcam_following) {
		  return wcg.weaponSelectTime;
	  } else {
		  return cg.weaponSelectTime;
	  }
	  break;

  case WCG_NUMBER_OF_HELD_WEAPONS:
	  return CG_NumHeldWeapons();
	  break;

  case WCG_WEAPON_HAVE_GAUNTLET:
	  return CG_HaveWeapon(WP_GAUNTLET);
	  break;

  case WCG_WEAPON_HAVE_MACHINEGUN:
	  return CG_HaveWeapon(WP_MACHINEGUN);
	  break;

  case WCG_WEAPON_HAVE_SHOTGUN:
	  return CG_HaveWeapon(WP_SHOTGUN);
	  break;

  case WCG_WEAPON_HAVE_GRENADE_LAUNCHER:
	  return CG_HaveWeapon(WP_GRENADE_LAUNCHER);
	  break;

  case WCG_WEAPON_HAVE_ROCKET_LAUNCHER:
	  return CG_HaveWeapon(WP_ROCKET_LAUNCHER);
	  break;

  case WCG_WEAPON_HAVE_LIGHTNING:
	  return CG_HaveWeapon(WP_LIGHTNING);
	  break;

  case WCG_WEAPON_HAVE_RAILGUN:
	  return CG_HaveWeapon(WP_RAILGUN);
	  break;

  case WCG_WEAPON_HAVE_PLASMAGUN:
	  return CG_HaveWeapon(WP_PLASMAGUN);
	  break;

  case WCG_WEAPON_HAVE_BFG:
	  return CG_HaveWeapon(WP_BFG);
	  break;

  case WCG_WEAPON_HAVE_GRAPPLING_HOOK:
	  return CG_HaveWeapon(WP_GRAPPLING_HOOK);
	  break;

  case WCG_WEAPON_HAVE_NAILGUN:
	  return CG_HaveWeapon(WP_NAILGUN);
	  break;

  case WCG_WEAPON_HAVE_PROX_LAUNCHER:
	  return CG_HaveWeapon(WP_PROX_LAUNCHER);
	  break;

  case WCG_WEAPON_HAVE_CHAINGUN:
	  return CG_HaveWeapon(WP_CHAINGUN);
	  break;

  case WCG_WEAPON_HAVE_HEAVY_MACHINEGUN:
	  return CG_HaveWeapon(WP_HEAVY_MACHINEGUN);
	  break;

  case WCG_WEAPON_AMMO_GAUTNLET:
	  return CG_WeaponAmmo(WP_GAUNTLET);
	  break;

  case WCG_WEAPON_AMMO_MACHINEGUN:
	  return CG_WeaponAmmo(WP_MACHINEGUN);
	  break;

  case WCG_WEAPON_AMMO_SHOTGUN:
	  return CG_WeaponAmmo(WP_SHOTGUN);
	  break;

  case WCG_WEAPON_AMMO_GRENADE_LAUNCHER:
	  return CG_WeaponAmmo(WP_GRENADE_LAUNCHER);
	  break;

  case WCG_WEAPON_AMMO_ROCKET_LAUNCHER:
	  return CG_WeaponAmmo(WP_ROCKET_LAUNCHER);
	  break;

  case WCG_WEAPON_AMMO_LIGHTNING:
	  return CG_WeaponAmmo(WP_LIGHTNING);
	  break;

  case WCG_WEAPON_AMMO_RAILGUN:
	  return CG_WeaponAmmo(WP_RAILGUN);
	  break;

  case WCG_WEAPON_AMMO_PLASMAGUN:
	  return CG_WeaponAmmo(WP_PLASMAGUN);
	  break;

  case WCG_WEAPON_AMMO_BFG:
	  return CG_WeaponAmmo(WP_BFG);
	  break;

  case WCG_WEAPON_AMMO_GRAPPLING_HOOK:
	  return CG_WeaponAmmo(WP_GRAPPLING_HOOK);
	  break;

  case WCG_WEAPON_AMMO_NAILGUN:
	  return CG_WeaponAmmo(WP_NAILGUN);
	  break;

  case WCG_WEAPON_AMMO_PROX_LAUNCHER:
	  return CG_WeaponAmmo(WP_PROX_LAUNCHER);
	  break;

  case WCG_WEAPON_AMMO_CHAINGUN:
	  return CG_WeaponAmmo(WP_CHAINGUN);
	  break;

  case WCG_WEAPON_AMMO_HEAVY_MACHINEGUN:
	  return CG_WeaponAmmo(WP_HEAVY_MACHINEGUN);
	  break;

  case WCG_KILL_COUNT: {
	  if (wolfcam_following) {
		  return wclients[wcg.clientNum].killCount;
	  } else {
		  return wclients[cg.snap->ps.clientNum].killCount;
	  }
  }

  case WCG_PLAYER_KEY_PRESS_FORWARD:
	  return cg.playerKeyPressForward;

  case WCG_PLAYER_KEY_PRESS_BACK:
	  return cg.playerKeyPressBack;

  case WCG_PLAYER_KEY_PRESS_RIGHT:
	  return cg.playerKeyPressRight;

  case WCG_PLAYER_KEY_PRESS_LEFT:
	  return cg.playerKeyPressLeft;

  case WCG_PLAYER_KEY_PRESS_FIRE:
	  return cg.playerKeyPressFire;

  case WCG_PLAYER_KEY_PRESS_CROUCH:
	  return cg.playerKeyPressCrouch;

  case WCG_PLAYER_KEY_PRESS_JUMP:
	  return cg.playerKeyPressJump;

  default:
	  Com_Printf("CG_GetValue() unknown ownerDraw %d\n", ownerDraw);
    break;
  }
	return -1;
}

qboolean CG_OtherTeamHasFlag(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_CTFS) {
		int team = cg.snap->ps.persistant[PERS_TEAM];

		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		}

		if (cgs.gametype == GT_1FCTF) {
			if (team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_BLUE) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_RED) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if (team == TEAM_RED && cgs.redflag == FLAG_TAKEN) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.blueflag == FLAG_TAKEN) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
	return qfalse;
}

qboolean CG_YourTeamHasFlag(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_CTFS) {
		int team = cg.snap->ps.persistant[PERS_TEAM];

		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		}

		if (cgs.gametype == GT_1FCTF) {
			if (team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_RED) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_BLUE) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if (team == TEAM_RED && cgs.blueflag == FLAG_TAKEN) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.redflag == FLAG_TAKEN) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
	return qfalse;
}

static qboolean CG_OwnerDrawVisible2 (int flags)
{
	int w;
	int clientNum;
	int team;
	int cs;

	if (flags & CG_SHOW_IF_PLYR1) {
		if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
			cs = CS_FIRST_PLACE_CLIENT_NUM2;
		} else {
			cs = CS_FIRST_PLACE_CLIENT_NUM;
		}

		if (atoi(CG_ConfigString(cs)) == cg.snap->ps.clientNum) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR2) {
		if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
			cs = CS_SECOND_PLACE_CLIENT_NUM2;
		} else {
			cs = CS_SECOND_PLACE_CLIENT_NUM;
		}

		if (atoi(CG_ConfigString(cs)) == cg.snap->ps.clientNum) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_G_FIRED) {
		w = WP_GAUNTLET;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_MG_FIRED) {
		w = WP_MACHINEGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_SG_FIRED) {
		w = WP_SHOTGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_HMG_FIRED) {
		w = WP_HEAVY_MACHINEGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_GL_FIRED) {
		w = WP_GRENADE_LAUNCHER;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_RL_FIRED) {
		w = WP_ROCKET_LAUNCHER;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_LG_FIRED) {
		w = WP_LIGHTNING;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_RG_FIRED) {
		w = WP_RAILGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PG_FIRED) {
		w = WP_PLASMAGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_BFG_FIRED) {
		w = WP_BFG;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_CG_FIRED) {
		w = WP_CHAINGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_NG_FIRED) {
		w = WP_NAILGUN;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PL_FIRED) {
		w = WP_PROX_LAUNCHER;
		if (cg.duelScores[0].weaponStats[w].atts  ||  cg.duelScores[1].weaponStats[w].atts) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_RED_OR_SPEC) {
		clientNum = cg.snap->ps.clientNum;  //FIXME wolfcam
		team = cgs.clientinfo[clientNum].team;
		if (team == TEAM_RED  ||  team == TEAM_SPECTATOR) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_BLUE_OR_SPEC) {
		clientNum = cg.snap->ps.clientNum;  //FIXME wolfcam
		team = cgs.clientinfo[clientNum].team;
		if (team == TEAM_BLUE  ||  team == TEAM_SPECTATOR) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_RED_NO_SPEC) {
		clientNum = cg.snap->ps.clientNum;  //FIXME wolfcam
		team = cgs.clientinfo[clientNum].team;
		if (team == TEAM_RED) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_BLUE_NO_SPEC) {
		clientNum = cg.snap->ps.clientNum;  //FIXME wolfcam
		team = cgs.clientinfo[clientNum].team;
		if (team == TEAM_BLUE) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_1ST_PLYR_FOLLOWED) {
		if (cgs.realProtocol >= 91) {
			if (atoi(CG_ConfigString(CS91_CLIENTNUM1STPLAYER)) == cg.snap->ps.clientNum) {
				return qtrue;
			} else {
				return qfalse;
			}
		}

		// other protocols

		if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
			cs = CS_FIRST_PLACE_CLIENT_NUM2;
		} else {
			cs = CS_FIRST_PLACE_CLIENT_NUM;
		}

		if (atoi(CG_ConfigString(cs)) == cg.snap->ps.clientNum) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_2ND_PLYR_FOLLOWED) {
		if (cgs.realProtocol >= 91) {
			if (atoi(CG_ConfigString(CS91_CLIENTNUM2NDPLAYER)) == cg.snap->ps.clientNum) {
				return qtrue;
			} else {
				return qfalse;
			}
		}

		// other protocols
		if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
			cs = CS_SECOND_PLACE_CLIENT_NUM2;
		} else {
			cs = CS_SECOND_PLACE_CLIENT_NUM;
		}

		if (atoi(CG_ConfigString(cs)) == cg.snap->ps.clientNum) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	//FIXME ac other ones
	switch (flags) {

	default:
		Com_Printf("^3fixme ownerdrawflag2 0x%08x\n", flags);
		break;
	}

	return qfalse;
}

// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean CG_OwnerDrawVisible (int flags, int flags2)
{
	int team;

	if (flags2) {
		return CG_OwnerDrawVisible2(flags2);
	}

	//FIXME incompatible flags??  say a check for team and not team games, don't understand why some things fall through

	if (flags & CG_SHOW_TEAMINFO) {
		return (cg_currentSelectedPlayer.integer == numSortedTeamPlayers);
	}

	if (flags & CG_SHOW_NOTEAMINFO) {
		return !(cg_currentSelectedPlayer.integer == numSortedTeamPlayers);
	}

	if (flags & CG_SHOW_OTHERTEAMHASFLAG) {
		return CG_OtherTeamHasFlag();
	}

	if (flags & CG_SHOW_YOURTEAMHASENEMYFLAG) {
		return CG_YourTeamHasFlag();
	}

	if (flags & (CG_SHOW_BLUE_TEAM_HAS_REDFLAG | CG_SHOW_RED_TEAM_HAS_BLUEFLAG)) {
		if (cgs.gametype == GT_1FCTF) {
			if (flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG  &&  cgs.flagStatus == FLAG_TAKEN_BLUE) {
				return qtrue;
			} else if (flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG  &&  cgs.flagStatus == FLAG_TAKEN_RED) {
				return qtrue;
			}
		} else {
			if (flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG  &&  cgs.redflag == FLAG_TAKEN) {
				return qtrue;
			} else if (flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG  &&  cgs.blueflag == FLAG_TAKEN) {
				return qtrue;
			}
		}
		return qfalse;
	}

	if (flags & CG_SHOW_ANYTEAMGAME) {
		if (CG_IsTeamGame(cgs.gametype)) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_ANYNONTEAMGAME) {
		if (!CG_IsTeamGame(cgs.gametype)) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_HARVESTER) {
		if( cgs.gametype == GT_HARVESTER ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_ONEFLAG) {
		if( cgs.gametype == GT_1FCTF ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_CTF) {
		if( cgs.gametype == GT_CTF) {  // ||  cgs.gametype == GT_CTFS ) {  //FIXME maybe not ctfs
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_OBELISK) {
		if( cgs.gametype == GT_OBELISK ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_HEALTHCRITICAL) {
		if (wolfcam_following) {
			//return qfalse;  //FIXME
			if (0) { //(wclients[wcg.clientNum].eventHealth < 25) {
				return qtrue;
			} else {
				return qfalse;
			}
		}

		if (cg.snap->ps.stats[STAT_HEALTH] < 25) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

#if 0
	if (flags & CG_SHOW_HEALTHOK) {
		if (wolfcam_following) {
			//return qtrue;  //FIXME
			if (1) { //(wclients[wcg.clientNum].eventHealth >= 25) {
				return qtrue;
			} else {
				return qfalse;
			}
		}

		if (cg.snap->ps.stats[STAT_HEALTH] >= 25) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
#endif

#if 1
	if (flags & CG_SHOW_DOMINATION) {
		if (cgs.gametype == GT_DOMINATION) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

#endif

#if 0
	if (flags & CG_SHOW_SINGLEPLAYER) {
		if( cgs.gametype == GT_SINGLE_PLAYER ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
#endif

	if (flags & CG_SHOW_CLAN_ARENA) {
		if( cgs.gametype == GT_CA ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_TOURNAMENT) {
		if( cgs.gametype == GT_TOURNAMENT ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	//if (flags & CG_SHOW_DURINGINCOMINGVOICE) {
	//}

	if (flags & CG_SHOW_IF_PLAYER_HAS_FLAG) {
		if (wolfcam_following) {
			return qfalse;  //FIXME
		}

		if (cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_RED) {
		if (wolfcam_following) {
			if (cgs.clientinfo[wcg.clientNum].team == TEAM_RED) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_ON_BLUE) {
		if (wolfcam_following) {
			if (cgs.clientinfo[wcg.clientNum].team == TEAM_BLUE) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE) {
		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		} else {
			team = cg.snap->ps.persistant[PERS_TEAM];
		}

		if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
			if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
				return qfalse;
			} else if (cgs.scores1 < cgs.scores2) {
				if (team == TEAM_RED) {
					return qtrue;
				} else {
					return qfalse;
				}
			} else {  // cgs.scores2 < cgs.scores1
				if (team == TEAM_BLUE) {
					return qtrue;
				} else {
					return qfalse;
				}
			}
		}

		if ((cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG) != 0) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_RED_IS_FIRST_PLACE) {
		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		} else {
			team = cg.snap->ps.persistant[PERS_TEAM];
		}

		if (cgs.gametype == GT_RED_ROVER) {
			//return qfalse;
		}

		if (CG_ScoresEqual(cgs.scores1, cgs.scores2)  &&  team == TEAM_RED) {
			return qtrue;
		} else if (cgs.scores1 > cgs.scores2) {  //FIXME maybe just '>' ?
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_BLUE_IS_FIRST_PLACE) {
		//if (cgs.scores2 >= cgs.scores1) {  //FIXME maybe just '>' ?
		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		} else {
			team = cg.snap->ps.persistant[PERS_TEAM];
		}

		if (cgs.gametype == GT_RED_ROVER) {
			//return qfalse;
		}

		if (CG_ScoresEqual(cgs.scores2, cgs.scores1)  &&  team == TEAM_BLUE) {
			return qtrue;
		} else if (cgs.scores2 > cgs.scores1) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_NOTICE_PRESENT) {
		//FIXME what is this maybe friend is online or some shit
		if (0) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_PLYR_IS_FIRST_PLACE) {
		if (wolfcam_following) {
			team = cgs.clientinfo[wcg.clientNum].team;
		} else {
			team = cg.snap->ps.persistant[PERS_TEAM];
		}

		if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
			if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
				return qtrue;
			} else if (cgs.scores1 < cgs.scores2) {
				if (team == TEAM_RED) {
					return qfalse;
				} else {
					return qtrue;
				}
			} else {  // cgs.scores2 < cgs.scores1
				if (team == TEAM_BLUE) {
					return qfalse;
				} else {
					return qtrue;
				}
			}
		}

		if ((cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG) == 0) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_MSG_PRESENT) {  //FIXME don't know  -- friend bullshit
		if (0) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_CHAT_VISIBLE) {
		if (cg.numChatLinesVisible) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_NOT_WARMUP) {
		if (!cg.warmup) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_IF_WARMUP) {
		if (cg.warmup) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_INTERMISSION) {
		if (cg.snap->ps.pm_type == PM_INTERMISSION) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_NOTINTERMISSION) {
		if (cg.snap->ps.pm_type != PM_INTERMISSION) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_PLAYERS_REMAINING) {
		//Com_Printf("wtf\n");
		if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_RED_ROVER  ||  cgs.gametype == GT_CTFS) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	//FIXME ac other ones
	switch (flags) {

	default:
		Com_Printf("^3fixme ownerdrawflag 0x%08x\n", flags);
		break;
	}

	return qfalse;
}



static void CG_DrawPlayerHasFlag(const rectDef_t *rect, qboolean force2D) {
	int adj = (force2D) ? 0 : 2;

	if (wolfcam_following) {
		return;  //FIXME
	}

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
  	CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_RED, force2D);
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
  	CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_BLUE, force2D);
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
  	CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_FREE, force2D);
	}
}

static void CG_DrawAreaSystemChat(const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, systemChat, 0, 0, 0, font);
}

static void CG_DrawAreaTeamChat(const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color,teamChat1, 0, 0, 0, font);
}

static void CG_DrawAreaChat(const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, teamChat2, 0, 0, 0, font);
}

const char *CG_GetKillerText(void) {
	const char *s = "";

	if (wolfcam_following) {
		return s;  //FIXME
	}

	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
	}
	return s;
}


static void CG_DrawKiller (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	float w, h;
	float tw;
	const char *s;

	// fragged by ... line

	if (wolfcam_following) {
		return;  //FIXME
	}

	if ( cg.killerName[0] ) {
		//int x = rect->x + rect->w / 2;

		//CG_Text_Pic_Paint(rect->x, rect->y, scale, newColor, extString, 0, 0, cg_drawFragMessageStyle.integer, font, th, cg_obituaryIconScale.value);

		s = CG_GetKillerText();
		w = CG_Text_Width(s, scale, 0, font);
		h = CG_Text_Height(s, scale, 0, font);

		w += CG_Text_Width(" ", scale, 0, font);
		tw = w;
		w += h * 1.5;

		if (align == ITEM_ALIGN_CENTER) {
			//CG_Text_Paint(x - w / 2, rect->y + rect->h, scale, color, s, 0, 0, textStyle, font);
			//  - h2 - (picHeight - h2) / 2
			//CG_DrawPic(x + w / 2 + CG_Text_Width(" ", scale, 0, font), rect->y + rect->h - h - (h * 1.5 - h) / 2, h * 1.5, h * 1.5, cg.killerWeaponIcon);
			CG_Text_Paint(rect->x - w / 2, rect->y, scale, color, s, 0, 0, textStyle, font);
			CG_DrawPic(rect->x - w / 2 + tw, rect->y + rect->h - h - (h * 1.5 - h) / 2, h * 1.5, h * 1.5, cg.killerWeaponIcon);
		} else if (align == ITEM_ALIGN_RIGHT) {
			CG_Text_Paint(rect->x + rect->y - w, rect->y + rect->h, scale, color, s, 0, 0, textStyle, font);
			CG_DrawPic(rect->x - (h * 1.5), rect->y + rect->h - h - (h * 1.5 - h) / 2, h * 1.5, h * 1.5, cg.killerWeaponIcon);
		} else if (align == ITEM_ALIGN_LEFT) {
			CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, s, 0, 0, textStyle, font);
			CG_DrawPic(rect->x + tw, rect->y + rect->h - h - (h * 1.5 - h) / 2, h * 1.5, h * 1.5, cg.killerWeaponIcon);
		} else {
			Com_Printf("^3CG_DrawKiller() unknown alignment value\n");
		}
	}
}


static void CG_DrawCapFragLimit (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align, const rectDef_t *menuRect) {
	int limit;

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_RED_ROVER) {
		limit = cgs.roundlimit;
	} else if (cgs.gametype == GT_TOURNAMENT  ||  cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_RACE) {
		limit = cgs.timelimit;
	} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF) {
		if (cgs.capturelimit) {
			limit = cgs.capturelimit;
		} else {
			limit = cgs.timelimit;
		}
	} else if (cgs.gametype == GT_DOMINATION  ||  cgs.gametype == GT_CTFS) {
		limit = cgs.scorelimit;
	} else {
		limit = cgs.fraglimit;
	}

	//FIXME menurect offset like ammo, armor, and health?
	//CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", limit),0, 0, textStyle, font);
	CG_Text_Paint_Align(rect, scale, color, va("%2i", limit), 0, 0, textStyle, font, align);
}

static void CG_Draw1stPlace (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	int scores;

	if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
		scores = cgs.scores1;
	} else if (cgs.scores1 > cgs.scores2) {
		scores = cgs.scores1;
	} else {
		scores = cgs.scores2;
	}

	if (cgs.gametype == GT_RACE) {
		if (scores < 0  ||  scores >= MAX_RACE_SCORE) {
			CG_Text_Paint_Align(rect, scale, color, va("-"), 0, 0, textStyle, font, align);
		} else {
			CG_Text_Paint_Align(rect, scale, color, va("%is", (int)(round(scores / 1000.0))), 0, 0, textStyle, font, align);
		}
	} else {
		CG_Text_Paint_Align(rect, scale, color, va("%i", scores), 0, 0, textStyle, font, align);
	}
}

static void CG_Draw2ndPlace (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	int scores;

	//FIXME non team games and show player's score???

	if (!CG_IsTeamGame(cgs.gametype)  ||  cgs.gametype == GT_RED_ROVER) {
		if (CG_NumPlayers() < 2) {
			return;
		}
	}

	if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
		scores = cgs.scores2;
	} else if (cgs.scores1 < cgs.scores2) {
		scores = cgs.scores1;
	} else {
		scores = cgs.scores2;
	}

	if (cgs.gametype == GT_RACE) {
		if (scores < 0  ||  scores >= MAX_RACE_SCORE) {
			CG_Text_Paint_Align(rect, scale, color, va("-"), 0, 0, textStyle, font, align);
		} else {
			CG_Text_Paint_Align(rect, scale, color, va("%is", (int)(round(scores / 1000.0))), 0, 0, textStyle, font, align);
		}
	} else {
		CG_Text_Paint_Align(rect, scale, color, va("%i", scores), 0, 0, textStyle, font, align);
	}
}

const char *CG_GetGameStatusText (vec4_t color)
{
	const char *s = "";

	if (!CG_IsTeamGame(cgs.gametype)) {
		//FIXME wolfcam
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			s = va("%s ^7place with %i",CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),cg.snap->ps.persistant[PERS_SCORE] );
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("^1Red ^7leads ^4Blue, ^7%i to %i", cg.teamScores[0], cg.teamScores[1] );
			Vector4Set(color, 1, 1, 1, 1);  // quakelive ignoring color
		} else {
			s = va("^4Blue ^7leads ^1Red, ^7%i to %i", cg.teamScores[1], cg.teamScores[0] );
			Vector4Set(color, 1, 1, 1, 1);  // qaukelive ignoring color
		}
	}
	return s;
}

static void CG_DrawGameStatus (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font, int align)
{
	const char *s;
	vec4_t statusColor;
	float w;
	rectDef_t newRect;

	newRect = *rect;
	s = CG_GetGameStatusText(statusColor);
	//CG_Text_Paint(rect->x, rect->y + rect->h, scale, colorWhite, s, 0, 0, textStyle, font);
	newRect.y += rect->h;
	// quake live ignoring color
	//CG_Text_Paint_Align(rect, scale, colorWhite, s, 0, 0, textStyle, font, align);
	CG_Text_Paint_Align(&newRect, scale, statusColor, s, 0, 0, textStyle, font, align);
	//CG_Text_Paint_Align(rect, scale, color, s, 0, 0, textStyle, font, align);
	if (cgs.gametype != GT_TOURNAMENT  &&  cgs.gametype != GT_TEAM) {
		w = CG_Text_Width(s, scale, 0, font);
		newRect.x += w;
		CG_Text_Paint_Align(&newRect, scale, colorWhite, va("       %s", CG_GetLocalTimeString()), 0, 0, textStyle, font, align);
	}
}

const char *CG_GameTypeString(void) {
	if ( cgs.gametype == GT_FFA ) {
		return "Free For All";
	} else if ( cgs.gametype == GT_TEAM ) {
		return "Team Deathmatch";
	} else if ( cgs.gametype == GT_CA ) {
		return "Clan Arena";
	} else if ( cgs.gametype == GT_CTF ) {
		return "Capture the Flag";
	} else if ( cgs.gametype == GT_1FCTF ) {
		return "One Flag CTF";
	} else if (cgs.gametype == GT_CTFS) {
		if (cgs.cpma) {
			return "Capture Strike";
		} else {
			return "Attack and Defend";
		}
	} else if ( cgs.gametype == GT_OBELISK ) {
		return "Overload";
	} else if ( cgs.gametype == GT_HARVESTER ) {
		return "Harvester";
	} else if (cgs.gametype == GT_FREEZETAG) {
		return "Freeze Tag";
	} else if (cgs.gametype == GT_DOMINATION) {
		return "Domination";
	} else if (cgs.gametype == GT_RED_ROVER) {
		return "Red Rover";
	}

	return "";
}

#if 0
static void CG_DrawGameType(const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font ) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, CG_GameTypeString(), 0, 0, textStyle, font);
}
#endif

int CG_Text_Length (const char *s)
{
	int count = 0;

	while (*s) {
		if (Q_IsColorString(s)) {
			if (cgs.osp) {
				if (s[1] == 'x'  ||  s[1] == 'X') {
					s += 8;
				} else {
					s += 2;
				}
			} else {
				s += 2;
			}
		} else {
			int bytes;
			qboolean error;

			Q_GetCpFromUtf8(s, &bytes, &error);
			s += (bytes - 1);

			count++;
			s++;
		}
	}

	return count;
}

//FIXME duplicate code CG_Text_Paint()
//FIXME style
void CG_Text_Paint_Limit (float *maxX, float x, float y, float scale, const vec4_t color, const char* text, float adjust, int limit, const fontInfo_t *font)
{
	int len, count;
	vec4_t newColor;
	glyphInfo_t glyph;
	float xscale, yscale;

	xscale = 1.0;
	yscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
		yscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
		yscale = 3.0;
	}

	if (text) {
	    const char *s = text;
	    float max = *maxX;
		float useScale;

		//Com_Printf("CG_Text_Paint_Limit %d  %s\n", limit, text);

		if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
			if (scale <= cg_smallFont.value) {
				font = &cgDC.Assets.smallFont;
			} else if (scale > cg_bigFont.value) {
				font = &cgDC.Assets.bigFont;
			}
		}
		useScale = scale * font->glyphScale;
		font = CG_ScaleFont(font, &scale, &useScale);

		trap_R_SetColor( color );
		len = CG_Text_Length(text);  //strlen(text);  //FIXME hmm
		//len = strlen(text);
		//Com_Printf("length %d %s\n", len, text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			int cp;
			int bytes;
			qboolean error;

			bytes = 0;
			cp = Q_GetCpFromUtf8(s, &bytes, &error);

			trap_R_GetGlyphInfo(font, cp, &glyph);

			if ( Q_IsColorString( s ) ) {
				if (cgs.cpma) {
					CG_CpmaColorFromString(s + 1, newColor);
				} else if (cgs.osp) {
					CG_OspColorFromString(s + 1, newColor);
				} else {
					memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				}
				//memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				//FIXME double check, did it for spec list in ql scoreboard
				newColor[3] = color[3];
				if (s[1] == '7') {
					VectorCopy(color, newColor);
				}
				trap_R_SetColor( newColor );
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
						//count += 8;
					} else {
						s += 2;
						//count += 8;
					}
				} else {
					s += 2;
					//count += 2;
				}
				continue;
			} else {
				float yadj = useScale * glyph.top;
				float xadj = useScale * glyph.left;
#if 0
				//if (CG_Text_Width(s, useScale, 1, font) + x > max) {
				if (CG_Text_Width(s, scale, 1, font) + x > max) {
					//Com_Printf("maxx %f  %s\n", max, text);
					*maxX = 0;
					break;
				}
#endif
				//FIXME not really, shouldn't use xSkip, should be based on actual width drawn
				if (x + (glyph.xSkip * useScale * xscale) + adjust > max) {
					*maxX = 0;
					break;
				}
				//Com_Printf("print x %f\n", x);
				CG_Text_PaintCharScale(
									   x + xadj,
									   y - yadj,
									   glyph.imageWidth,
									   glyph.imageHeight,
									   useScale * xscale,
									   useScale * yscale,
									   glyph.s,
									   glyph.t,
									   glyph.s2,
									   glyph.t2,
									   glyph.glyph);
				x += (glyph.xSkip * useScale * xscale) + adjust;
				*maxX = x;
				count++;
				s++;
				s += (bytes - 1);  // utf8
			}
		}
		//Com_Printf("count %d  len %d\n", count, len);
		trap_R_SetColor( NULL );
	}

}

void CG_Text_Paint_Limit_Bottom(float *maxX, float x, float y, float scale, const vec4_t color, const char* text, float adjust, int limit, const fontInfo_t *font)
{
	float th;

#if 0
	//FIXME same for CG_Text_Paint_Bottom()
	if (!Q_stricmp(font->name, "q3tiny")) {
		th = 0;
	} else if (!Q_stricmp(font->name, "q3small")) {
		th = 0;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		th = 0;
	} else {
		th = CG_Text_Height("1IPUTY", scale, limit, font);  //FIXME fontInfo should store max height  point size
	}
#endif

	th = CG_Text_Height("1IPUTY", scale, limit, font);  //FIXME fontInfo should store max height  point size
	CG_Text_Paint_Limit(maxX, x, y + th, scale, color, text, adjust, limit, font);
}

void CG_Text_Paint_Align (const rectDef_t *rect, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, int align)
{
	float w;

	if (align == ITEM_ALIGN_LEFT) {
		CG_Text_Paint(rect->x, rect->y, scale, color, text, adjust, limit, style, fontOrig);
		return;
	}

	w = CG_Text_Width(text, scale, limit, fontOrig);
	if (align == ITEM_ALIGN_CENTER) {
		//CG_Text_Paint(rect->x + rect->w / 2 - w / 2, rect->y, scale, color, text, adjust, limit, style, fontOrig);
		CG_Text_Paint(rect->x - w / 2, rect->y, scale, color, text, adjust, limit, style, fontOrig);
		return;
	} else if (align == ITEM_ALIGN_RIGHT) {
		//CG_Text_Paint(rect->x + rect->w - w, rect->y, scale, color, text, adjust, limit, style, fontOrig);
		CG_Text_Paint(rect->x - w, rect->y, scale, color, text, adjust, limit, style, fontOrig);
		return;
	}

	Com_Printf("CG_Text_Paint_Align() unknown align option: %d\n", align);
}

#define PIC_WIDTH 12

static void CG_DrawNewTeamInfo(const rectDef_t *rect, float text_x, float text_y, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	int xx;
	float y;
	int i, j, count;
	float len;
	const char *p;
	vec4_t		hcolor;
	float pwidth, lwidth, maxx, leftOver;
	const clientInfo_t *ci;
	gitem_t	*item;
	qhandle_t h;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			len = CG_Text_Width( ci->name, scale, 0, font);
			if (len > pwidth)
				pwidth = len;
		}
	}

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_Text_Width(p, scale, 0, font);
			if (len > lwidth)
				lwidth = len;
		}
	}

	y = rect->y;

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			xx = rect->x + 1;
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, trap_R_RegisterShader( item->icon ) );
						xx += PIC_WIDTH;
					}
				}
			}

			// FIXME: max of 3 powerups shown properly
			xx = rect->x + (PIC_WIDTH * 3) + 2;

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );
			trap_R_SetColor(hcolor);
			CG_DrawPic( xx, y + 1, PIC_WIDTH - 2, PIC_WIDTH - 2, cgs.media.heartShader );

			//Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
			//CG_Text_Paint(xx, y + text_y, scale, hcolor, st, 0, 0, &cgDC.Assets.textFont); 

			// draw weapon icon
			xx += PIC_WIDTH + 1;

// weapon used is not that useful, use the space for task
#if 0
			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cgs.media.deferShader );
			}
#endif

			trap_R_SetColor(NULL);
			if (cgs.orderPending) {
				// blink the icon
				if ( cg.time > cgs.orderTime - 2500 && (cg.time >> 9 ) & 1 ) {
					h = 0;
				} else {
					h = CG_StatusHandle(cgs.currentOrder);
				}
			}	else {
				h = CG_StatusHandle(ci->teamTask);
			}

			if (h) {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, h);
			}

			xx += PIC_WIDTH + 1;

			leftOver = rect->w - xx;
			maxx = xx + leftOver / 3;



			CG_Text_Paint_Limit(&maxx, xx, y + text_y, scale, color, ci->name, 0, 0, font);

			p = CG_ConfigString(CS_LOCATIONS + ci->location);
			if (!p || !*p) {
				p = "unknown";
			}

			xx += leftOver / 3 + 2;
			maxx = rect->w - 4;

			CG_Text_Paint_Limit(&maxx, xx, y + text_y, scale, color, p, 0, 0, font);
			y += text_y + 2;
			if ( y + text_y + 2 > rect->y + rect->h ) {
				break;
			}

		}
	}
}


static void CG_DrawTeamSpectators(const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	float maxX;
	float sw;
	int numUtf8Bytes;
	qboolean error;

	if (!cg.spectatorLen) {
		return;
	}

	sw = CG_Text_Width(cg.spectatorList, scale, 0, font);

	if (cg.spectatorWidth == -1) {
		// reset
		cg.spectatorWidth = sw;
		cg.spectatorPaintX = rect->x + 1;
		cg.spectatorPaintX2 = -1;
		cg.spectatorOffset = 0;
		cg.spectatorOffsetWidth = 0;
	}

	if (sw < (int)rect->w) {
		if (!cg_scoreBoardSpectatorScroll.integer) {
			maxX = rect->x + rect->w - 2;
			CG_Text_Paint_Limit(&maxX, rect->x + 1, rect->y + rect->h - 3, scale, color, cg.spectatorList, 0, 0, font);
			return;
		}
	}

	if (cg.spectatorOffset >= strlen(cg.spectatorList) ) {
		cg.spectatorOffset = 0;
		if (cg.spectatorPaintX2 > -1) {
			cg.spectatorPaintX = cg.spectatorPaintX2;
		} else {
			cg.spectatorPaintX = rect->x + rect->w - 1;
		}
		cg.spectatorPaintX2 = -1;
	}

	if (cg.time > cg.spectatorTime) {
		cg.spectatorTime = cg.time + 10;
		if (cg.spectatorPaintX <= rect->x + 2) {
			if (cg.spectatorOffset < strlen(cg.spectatorList)) {
				if (cg.spectatorOffsetWidth <= 0) {
					while (1) {
						if (cg.spectatorList[cg.spectatorOffset] == '^') {
							cg.spectatorListCurrentColor = cg.spectatorList[cg.spectatorOffset + 1];
							if (cg.spectatorOffset + 2 < strlen(cg.spectatorList)) {
								cg.spectatorOffset += 2;
							} else {
								//cg.spectatorOffset++;
								//FIXME check overflow
								Q_GetCpFromUtf8(&cg.spectatorList[cg.spectatorOffset], &numUtf8Bytes, &error);
								//FIXME check overflow
								cg.spectatorOffset += numUtf8Bytes;
							}
						} else {
							break;
						}
					}
					cg.spectatorOffsetWidth = CG_Text_Width(&cg.spectatorList[cg.spectatorOffset], scale, 1, font);
					cg.spectatorPaintX += cg.spectatorOffsetWidth;
					//cg.spectatorOffset++;
					//FIXME check overflow
					Q_GetCpFromUtf8(&cg.spectatorList[cg.spectatorOffset], &numUtf8Bytes, &error);
					//FIXME check overflow
					cg.spectatorOffset += numUtf8Bytes;

				} else {
					cg.spectatorOffsetWidth--;
					cg.spectatorPaintX--;
					cg.spectatorPaintX2--;
				}
			} else {
				cg.spectatorOffset = 0;
				if (cg.spectatorPaintX2 >= 0) {
					cg.spectatorPaintX = cg.spectatorPaintX2;
				} else {
					cg.spectatorPaintX = rect->x + rect->w - 2;
				}
				cg.spectatorPaintX2 = -1;
			}
		} else {
			cg.spectatorPaintX--;
			if (cg.spectatorPaintX2 >= 0) {
				cg.spectatorPaintX2--;
			}
		}
	}

	maxX = rect->x + rect->w - 2;
	CG_Text_Paint_Limit(&maxX, cg.spectatorPaintX, rect->y + rect->h - 3, scale, color, va("^%c%s", cg.spectatorListCurrentColor, &cg.spectatorList[cg.spectatorOffset]), 0, 0, font);
	// offsetting and fit to the end of spec list, so add start of list

	if (cg.spectatorPaintX2 >= 0) {
		float maxX2 = rect->x + rect->w - 2;
		float newWidth;

		newWidth = CG_Text_Width(&cg.spectatorList[cg.spectatorOffset], scale, 0, font);
		if (cg.spectatorPaintX + newWidth > cg.spectatorPaintX2) {
			cg.spectatorPaintX2 = cg.spectatorPaintX + newWidth;
		}
		CG_Text_Paint_Limit(&maxX2, cg.spectatorPaintX2, rect->y + rect->h - 3, scale, color, cg.spectatorList, 0, cg.spectatorOffset, font);
	}
	if (cg.spectatorOffset && maxX > 0) {
		// if we have an offset ( we are skipping the first part of the string ) and we fit the string
		if (cg.spectatorPaintX2 <= -1) {
			cg.spectatorPaintX2 = rect->x + rect->w - 2;
		}
	} else {
		cg.spectatorPaintX2 = -1;
	}
}

static void CG_DrawMedal(int ownerDraw, const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, const fontInfo_t *font) {
	const score_t *score = &cg.scores[cg.selectedScore];
	float value = 0;
	//int tw;
	float th;
	const char *text = NULL;
	vec4_t colorNew;

	Vector4Copy(color, colorNew);
	colorNew[3] = 0.25;

	switch (ownerDraw) {
		case CG_ACCURACY:
			value = score->accuracy;
			break;
		case CG_ASSISTS:
			value = score->assistCount;
			break;
		case CG_DEFEND:
			value = score->defendCount;
			break;
		case CG_EXCELLENT:
			value = score->excellentCount;
			break;
		case CG_IMPRESSIVE:
			value = score->impressiveCount;
			break;
		case CG_PERFECT:
			value = score->perfect;
			break;
		case CG_GAUNTLET:
			value = score->gauntletCount;
			break;
		case CG_CAPTURES:
			value = score->captures;
			break;
	}

	if (value > 0) {
		if (ownerDraw != CG_PERFECT) {
			if (ownerDraw == CG_ACCURACY) {
				text = va("%i%%", (int)value);
				if (value > 50) {
					colorNew[3] = 1.0;
				}
			} else {
				text = va("%i", (int)value);
				colorNew[3] = 1.0;
			}
		} else {
			if (value) {
				colorNew[3] = 1.0;
			}
			text = "Wow";
		}
	}

	trap_R_SetColor(colorNew);
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );

	if (text) {
		th = CG_Text_Height(text, scale, 0, font);

		colorNew[3] = 1.0;
		trap_R_SetColor(color);
		//CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h + 10 , scale, color, text, 0, 0, 0, font);
		//CG_Text_Paint_Bottom(rect->x + ((float)rect->w * 1.5), rect->y , scale, color, text, 0, 0, 0, font);
		CG_Text_Paint(rect->x + ((float)rect->w * 1.5), rect->y + rect->h / 2 + th / 2, scale, color, text, 0, 0, 0, font);
	}
	trap_R_SetColor(NULL);

}

static void CG_SetTeamColor (void)
{
	vec4_t color;
	int clientNum;

	if (wolfcam_following) {
		clientNum = wcg.clientNum;
	} else {
		clientNum = cg.snap->ps.clientNum;
	}

	if (cgs.clientinfo[clientNum].team == TEAM_RED) {
		//VectorSet(color, 1, 0, 0);
		//VectorCopy(colorRed, color);
		SC_Vec3ColorFromCvar(color, &cg_hudRedTeamColor);
	} else if (cgs.clientinfo[clientNum].team == TEAM_BLUE) {
		//VectorSet(color, 0, 0, 1);
		//VectorCopy(colorBlue, color);
		SC_Vec3ColorFromCvar(color, &cg_hudBlueTeamColor);
	} else {
		//VectorCopy(colorYellow, color);
		//VectorSet(color, 0.95, 0.865, 0);
		//VectorSet(color, 0.95, 0.86, 0.122);
		SC_Vec3ColorFromCvar(color, &cg_hudNoTeamColor);
	}

	color[3] = 1.0;

	trap_R_SetColor(color);

	//Com_Printf("set team color\n");
}

static void CG_OspCalcPlacements (void)
{
	const clientInfo_t *ci;
	int i;

	if (cgs.gametype != GT_TOURNAMENT) {
		return;
	}

	if (*cgs.firstPlace  &&  *cgs.secondPlace) {
		return;
	}

	//FIXME non duel games

	if (CG_ScoresEqual(cgs.scores1, cg.snap->ps.persistant[PERS_SCORE])) {
		//FIXME should be storing clientNum
		Q_strncpyz(cgs.firstPlace, cgs.clientinfo[cg.snap->ps.clientNum].name, sizeof(cgs.firstPlace));

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			ci = &cgs.clientinfo[i];
			if (!ci->infoValid) {
				continue;
			}
			if (i == cg.snap->ps.clientNum) {
				continue;
			}
			if (ci->team != TEAM_FREE) {
				continue;
			}
			Q_strncpyz(cgs.secondPlace, ci->name, sizeof(cgs.secondPlace));
			break;
		}
	} else if (CG_ScoresEqual(cgs.scores2, cg.snap->ps.persistant[PERS_SCORE])) {
		Q_strncpyz(cgs.secondPlace, cgs.clientinfo[cg.snap->ps.clientNum].name, sizeof(cgs.secondPlace));

		for (i = 0;  i < MAX_CLIENTS;  i++) {
			ci = &cgs.clientinfo[i];
			if (!ci->infoValid) {
				continue;
			}
			if (i == cg.snap->ps.clientNum) {
				continue;
			}
			if (ci->team != TEAM_FREE) {
				continue;
			}
			Q_strncpyz(cgs.firstPlace, ci->name, sizeof(cgs.firstPlace));
			break;
		}
	} else {
		Com_Printf("^1wtf...\n");
	}

}

static void CG_Draw1stPlaceScore (const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font)
{
	char *s;
	int spacing;
	int teamSpacing;
	int score;
	int team;
	int maxNameWidth;
	char *endOfName;
	qboolean nameTruncated;
	char scoreString[128];
	const char *tmpString;
	int scoreStringLength;

	if (cgs.protocol == PROTOCOL_Q3  &&  !cgs.cpma) {
		CG_OspCalcPlacements();
	}

	if (wolfcam_following) {
		team = cgs.clientinfo[wcg.clientNum].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

	if (cgs.cpma) {
		if (CG_IsTeamGame(cgs.gametype)) {

		} else {

		}
	}

	if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
		if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
			//FIXME free cam
			if (team == TEAM_RED) {
				if (*cg_hudForceRedTeamClanTag.string) {
					s = cg_hudForceRedTeamClanTag.string;
				} else {
					s = va("%s", cgs.redTeamClanTag);
				}
			} else {
				if (*cg_hudForceBlueTeamClanTag.string) {
					s = cg_hudForceBlueTeamClanTag.string;
				} else {
					s = va("%s", cgs.blueTeamClanTag);
				}
			}
			score = cgs.scores1;
		} else if (cgs.scores1 > cgs.scores2) {
			if (*cg_hudForceRedTeamClanTag.string) {
				s = cg_hudForceRedTeamClanTag.string;
			} else {
				s = va("%s", cgs.redTeamClanTag);
			}
			score = cgs.scores1;
		} else if (cgs.scores1 < cgs.scores2) {
			if (*cg_hudForceBlueTeamClanTag.string) {
				s = cg_hudForceBlueTeamClanTag.string;
			} else {
				s = va("%s", cgs.blueTeamClanTag);
			}
			score = cgs.scores2;
		} else {
			CG_Printf("^3FIXME CG_Draw1stPlaceScore() warning shouldn't happen 1\n");
			score = -100;  // gcc warning
			s = va("FIXME");
		}

		teamSpacing = rect->h / 2;
	} else {
		//if (cgs.scores1 == cg.snap->ps.persistant[PERS_SCORE]) {  //FIXME wolfcam
		if (CG_ScoresEqual(cgs.scores1, cg.snap->ps.persistant[PERS_SCORE])) {
			const char *clanTag;

			clanTag = cgs.clientinfo[cg.snap->ps.clientNum].clanTag;
			if (*clanTag) {
				s = va("1. ^7%s ^7%s", clanTag, cgs.clientinfo[cg.snap->ps.clientNum].name);
			} else {
				s = va("1. ^7%s", cgs.clientinfo[cg.snap->ps.clientNum].name);
			}
		} else {
			s = va("1. %s", cgs.firstPlace);  //CG_ConfigString(CS_FIRSTPLACE));
		}
		teamSpacing = 0;
		score = cgs.scores1;
	}

	//FIXME "..." if not enough space
	//spacing = CG_Text_Width("0", scale, 0, font);
	spacing = 0;

	if (score == SCORE_NOT_PRESENT) {
		s = "1.";
	}

	if (cgs.gametype == GT_RACE) {
		tmpString = CG_GetTimeString(score);
		Com_sprintf(scoreString, sizeof(scoreString), "%s", tmpString);
	} else {
		if (score == SCORE_NOT_PRESENT) {
			scoreString[0] = '\0';
		} else {
			Com_sprintf(scoreString, sizeof(scoreString), "%d", score);
		}
	}
	scoreStringLength = CG_Text_Width(scoreString, scale, 0, font);

	//FIXME stupid
	//FIXME use '...' like ql
	//maxNameWidth = rect->w - spacing - 1 - CG_Text_Width(scoreString, scale, 0, font);
	maxNameWidth = rect->w - spacing - scoreStringLength;
	endOfName = s + strlen(s);
	nameTruncated = qfalse;
	while (maxNameWidth > 0  &&  (CG_Text_Width(s, scale, 0, font) + teamSpacing) > maxNameWidth) {
		endOfName--;
		endOfName[0] = '\0';
		nameTruncated = qtrue;
	}
	if (nameTruncated  &&  strlen(s) > 3) {
 		endOfName += 3;  // room for color code
		endOfName[0] = '\0';

		endOfName--;
		endOfName[0] = '.';
		endOfName--;
		endOfName[0] = '.';
		endOfName--;
		endOfName[0] = '.';

		endOfName--;
		endOfName[0] = '7';
		endOfName--;
		endOfName[0] = '^';

		// last character could be ^
		endOfName--;
		endOfName[0] = ' ';
	}

	//FIXME spacing for scores
	CG_Text_Paint(rect->x + teamSpacing, rect->y, scale, color, s, 0, rect->w - teamSpacing - spacing - 6, textStyle, font);

	//CG_Text_Paint(rect->x + rect->w - spacing - CG_Text_Width(scoreString, scale, 0, font), rect->y, scale, color, scoreString, 0, 0, textStyle, font);
	CG_Text_Paint(rect->x + rect->w - scoreStringLength, rect->y, scale, color, scoreString, 0, 0, textStyle, font);
}

//FIXME duplicate with CG_Draw1stPlaceScore

static void CG_Draw2ndPlaceScore (const rectDef_t *rect, float scale, const vec4_t color, int textStyle, const fontInfo_t *font)
{
	char *s = NULL;
	int rank;
	int score;
	int spacing;
	int teamSpacing;
	int team;
	const char *sname;
	const char *ourName;
	int snameLen;
	int ourNameLen;
	int maxNameWidth;
	char *endOfName;
	qboolean nameTruncated;
	char scoreString[128];
	const char *tmpString;
	int scoreStringLength;

	if (cgs.protocol == PROTOCOL_Q3  &&  !cgs.cpma) {
		CG_OspCalcPlacements();
	}

	if (!CG_IsTeamGame(cgs.gametype)  ||  cgs.gametype == GT_RED_ROVER) {
		if (CG_NumPlayers() < 2) {
			return;
		}
	}

	if (wolfcam_following) {
		team = cgs.clientinfo[wcg.clientNum].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

	//FIXME store from config string
	rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
	rank++;

	if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
		if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
			rank = 1;
		} else {
			rank = 2;
		}
		teamSpacing = rect->h / 2;

		if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
			if (team == TEAM_RED) {
				if (*cg_hudForceBlueTeamClanTag.string) {
					s = cg_hudForceBlueTeamClanTag.string;
				} else {
					s = va("%s", cgs.blueTeamClanTag);
				}
			} else {
				if (*cg_hudForceRedTeamClanTag.string) {
					s = cg_hudForceRedTeamClanTag.string;
				} else {
					s = va("%s", cgs.redTeamClanTag);
				}
			}
			score = cgs.scores1;
		} else if (cgs.scores1 < cgs.scores2) {
			if (*cg_hudForceRedTeamClanTag.string) {
				s = cg_hudForceRedTeamClanTag.string;
			} else {
				s = va("%s", cgs.redTeamClanTag);
			}
			score = cgs.scores1;
		} else if (cgs.scores1 > cgs.scores2) {
			if (*cg_hudForceBlueTeamClanTag.string) {
				s = cg_hudForceBlueTeamClanTag.string;
			} else {
				s = va("%s", cgs.blueTeamClanTag);
			}
			score = cgs.scores2;
		} else {
			CG_Printf("^3FIXME CG_Draw2ndPlaceScore() warning shouldn't happen 1\n");
			score = -100;  // gcc warning
		}

		//score = cgs.scores2;
		//} else if (cgs.scores1 == cg.snap->ps.persistant[PERS_SCORE]) {
	} else if (CG_ScoresEqual(cgs.scores1, cg.snap->ps.persistant[PERS_SCORE])) {
		//FIXME wolfcam
		if (CG_ScoresEqual(cgs.scores2, cgs.scores1)) {
			qboolean secondPlaceIsUs = qfalse;

			//sname = CG_ConfigString(CS_SECONDPLACE);
			sname = cgs.secondPlace;  //CG_ConfigString(CS_SECONDPLACE);
			ourName = Info_ValueForKey(CG_ConfigString(CS_PLAYERS + cg.snap->ps.clientNum), "n");
			snameLen = strlen(sname);
			ourNameLen = strlen(ourName);

			if (snameLen >= ourNameLen) {
				sname = sname + snameLen - ourNameLen;

				if (!Q_stricmp(sname, ourName)) {
					secondPlaceIsUs = qtrue;
				}
			}
			if (snameLen > ourNameLen) {
				// check for something like 'blah'  and  'sssblah'
				if ((sname - 1)[0] != ' ') {
					secondPlaceIsUs = qfalse;
				}
			}

			//if (!Q_stricmp(CG_ConfigString(CS_SECONDPLACE), cgs.clientinfo[cg.snap->ps.clientNum].name)) {
			//if (!Q_stricmp(CG_ConfigString(CS_SECONDPLACE), Info_ValueForKey(CG_ConfigString(CS_PLAYERS + cg.snap->ps.clientNum), "n"))) {
			if (secondPlaceIsUs) {
				s = va("%d. %s", 1, cgs.firstPlace);  //CG_ConfigString(CS_FIRSTPLACE));
			} else {
				s = va("%d. %s", 1, cgs.secondPlace);  //CG_ConfigString(CS_SECONDPLACE));
			}
		} else {
			//s = va("%d. %s   %d", cgs.scores2 == cgs.scores1 ? 1 : 2, CG_ConfigString(CS_SECONDPLACE), cgs.scores2);
			//s = va("%d. %s", cgs.scores2 == cgs.scores1 ? 1 : 2, CG_ConfigString(CS_SECONDPLACE));
			//s = va("%d. %s", cgs.scores2 == cgs.scores1 ? 1 : 2, cgs.secondPlace);
			s = va("%d. %s", CG_ScoresEqual(cgs.scores2, cgs.scores1) ? 1 : 2, cgs.secondPlace);
		}
		score = cgs.scores2;
		teamSpacing = 0;
	} else {
		const char *clanTag;

		// we go into second place
		//FIXME placement num
		clanTag = cgs.clientinfo[cg.snap->ps.clientNum].clanTag;
		if (*clanTag) {
			s = va("%d. ^7%s ^7%s", rank, clanTag, cgs.clientinfo[cg.snap->ps.clientNum].name);
		} else {
			s = va("%d. ^7%s", rank, cgs.clientinfo[cg.snap->ps.clientNum].name);
		}

		score = cg.snap->ps.persistant[PERS_SCORE];
		teamSpacing = 0;
	}

	//if (cgs.scores2 == SCORE_NOT_PRESENT) {
	if (score == SCORE_NOT_PRESENT) {
		s = "2.";
	}

	if (cgs.gametype == GT_RACE) {
		tmpString = CG_GetTimeString(score);
		Com_sprintf(scoreString, sizeof(scoreString), "%s", tmpString);
	} else {
		if (score == SCORE_NOT_PRESENT) {
			scoreString[0] = '\0';
		} else {
			Com_sprintf(scoreString, sizeof(scoreString), "%d", score);
		}
	}
	scoreStringLength = CG_Text_Width(scoreString, scale, 0, font);

	//spacing = CG_Text_Width("0", scale, 0, font);
	spacing = 0;

	//FIXME ... if not enough space
	//FIXME size of score   -6

	//FIXME stupid
	//FIXME use '...' like ql
	//maxNameWidth = rect->w - spacing - 1 - CG_Text_Width(scoreString, scale, 0, font);
	maxNameWidth = rect->w - spacing - scoreStringLength;
	endOfName = s + strlen(s);
	nameTruncated = qfalse;
	while (maxNameWidth > 0  &&  (CG_Text_Width(s, scale, 0, font) + teamSpacing) > maxNameWidth) {
		endOfName--;
		endOfName[0] = '\0';
		nameTruncated = qtrue;
	}
	if (nameTruncated  &&  strlen(s) > 3) {
 		endOfName += 3;  // room for color code
		endOfName[0] = '\0';

		endOfName--;
		endOfName[0] = '.';
		endOfName--;
		endOfName[0] = '.';
		endOfName--;
		endOfName[0] = '.';

		endOfName--;
		endOfName[0] = '7';
		endOfName--;
		endOfName[0] = '^';

		// last character could be ^
		endOfName--;
		endOfName[0] = ' ';
	}

	//CG_Text_Paint(rect->x + teamSpacing, rect->y, scale, color, s, 0, rect->h - teamSpacing - spacing - 6, textStyle, font);
	CG_Text_Paint(rect->x + teamSpacing, rect->y, scale, color, s, 0, rect->w - teamSpacing - spacing - 6, textStyle, font);

	//CG_Text_Paint(rect->x + rect->w - spacing - CG_Text_Width(scoreString, scale, 0, font), rect->y, scale, color, scoreString, 0, 0, textStyle, font);
	CG_Text_Paint(rect->x + rect->w - scoreStringLength, rect->y, scale, color, scoreString, 0, 0, textStyle, font);
}



static void CG_DrawObit (const rectDef_t *rect, float scale, const vec4_t color, qhandle_t shader, int textStyle, const fontInfo_t *font)
{
	const char *text;
	const floatint_t *extString;
	char linebuffer[1024];
	int numIcons;
	float th;
	char *lb;
	const floatint_t *es;
	vec4_t newColor;
	int t;
	const obituary_t *obituary;
	float yOffset;
	int i;
	int stack;

	stack = cg_obituaryStack.integer;
	if (stack < 0) {
		stack = 0;
	}
	if (stack > MAX_OBITUARIES) {
		stack = MAX_OBITUARIES;
	}

	yOffset = 0;

#if 0
	// testing
	{
		vec4_t rcolor = { 1.0f, 1.0f, 0.0f, 1.0f };
		CG_DrawRect(rect->x, rect->y, rect->w, rect->h, 1.0f, rcolor);
	}
#endif

	for (i = -(stack - 1);  i <= 0;  i++) {
		if ((cg.obituaryIndex + i - 1) < 0) {
			continue;
		}
		obituary = &cg.obituaries[(cg.obituaryIndex + i - 1) % MAX_OBITUARIES];

		if (obituary->time == 0) {
			//Com_Printf("%d  invalid time 0:  %d\n", cg.obituaryIndex, i);
			continue;
		}
		if (cg.time - obituary->time > cg_obituaryTime.integer) {
			//Com_Printf("%d  time passed:  %d  %d  cg.time: %d\n", cg.obituaryIndex, i, obituary->time, cg.time);
			continue;
		}

		//Com_Printf("%d  drawing obit  %d  %d\n", cg.obituaryIndex, i, obituary->time);
		//Com_Printf("%d\n", (cg.obituaryIndex + i - 1) % MAX_OBITUARIES);

		//Com_Printf("about to create frag string...\n");
		extString = CG_CreateFragString(qfalse, i);
		//CG_PrintPicString(extString);

		while (1) {
			floatint_t *ts;
			qboolean haveNewLineAmount = qfalse;
			float newLineAmount = 0;

			if (extString[0].i == 0) {
				break;
			}

			// get a line
			ts = tmpExtString;

			while (extString[0].i) {
				if (extString[0].i >= 0  &&  extString[0].i <= 255) {
					*ts = *extString;
					if (extString[0].i == 0) {
						break;
					}
					extString++;
					ts++;
					continue;
				}

				if (extString[0].i == TEXT_PIC_PAINT_NEWLINE) {
					extString++;
					newLineAmount = extString[0].f;
					extString++;
					haveNewLineAmount = qtrue;
					ts[0].i = 0;
					break;
				}

				// command (2 ints)
				*ts = *extString;
				extString++;
				ts++;
				*ts = *extString;
				extString++;
				ts++;
			}
			ts[0].i = 0;

			lb = linebuffer;
			es = tmpExtString;  //extString;
			numIcons = 0;
			while (es[0].i) {
				if (es[0].i >= 0  &&  es[0].i <= 255) {
					*lb = (char)es[0].i;
					lb++;
					es++;
					continue;
				}
				if (es[0].i == TEXT_PIC_PAINT_ICON) {
					numIcons++;
				}
				//lb++;
				es += 2;
			}
			*lb = '\0';
			text = linebuffer;

			Vector4Copy(color, newColor);
			t = cg.time - obituary->time;

			if (t > (cg_obituaryTime.integer - cg_obituaryFadeTime.integer)) {
				float fade;

				t -= (cg_obituaryTime.integer - cg_obituaryFadeTime.integer);
				fade = (float)(cg_obituaryFadeTime.integer - t) / (float)(cg_obituaryFadeTime.integer);
				newColor[3] *= fade;
			}

			//tw = CG_Text_Width(text, scale, 0, font);
			th = CG_Text_Height(text, scale, 0, font);

			//tw += ((float)th * cg_obituaryIconScale.value) * (float)numIcons;

			//tw = CG_Text_Pic_Width(extString, scale, cg_obituaryIconScale.value, /*limit*/ 0, th, font);

			//Com_Printf("^2string: ");
			//CG_PrintPicString(tmpExtString);

			CG_Text_Pic_Paint(rect->x, rect->y - yOffset, scale, newColor, tmpExtString, 0, 0, cg_drawFragMessageStyle.integer, font, th, cg_obituaryIconScale.value);

			if (haveNewLineAmount) {
				yOffset -= newLineAmount;
			} else {
				if (cg_obituaryIconScale.value > 1.0f) {
					yOffset -= (th * cg_obituaryIconScale.value);
				} else {
					yOffset -= th;
					//yOffset -= (th * 1.2f * 3.0);
				}
				yOffset -= 2.0f;
			}

		}  // while (1)
	}
}

/*
===================
CG_DrawWeaponBar
===================
*/

void CG_DrawWeaponBar( void ) {
	int		i;
	int		count;
	float x, y;
	vec4_t color;
	const weaponInfo_t *wi;
	int ammo;
	vec4_t textColor;
	qboolean haveWeapon;
	int weapons;
	int elementWidth;
	int currentWeapon;
	const fontInfo_t *font;
	int maxWeapons;
	int ammoWarning;
	int style;
	int ammoOffset;

	if (cg_weaponBar.integer == 4  ||  cg_weaponBar.integer == 5) {
		CG_DrawWeaponSelect();
		return;
	}
	if (cg_weaponBar.integer == 0) {
		return;
	}

	if (*cg_weaponBarFont.string) {
		font = &cgs.media.weaponBarFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_weaponBarWideScreen.integer;
	
	// don't display if dead
	if (wolfcam_following) {
		if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
			return;
		}
		currentWeapon = cg_entities[wcg.clientNum].currentState.weapon;
	} else {
		if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
			return;
		}
		currentWeapon = cg.snap->ps.weapon;
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	trap_R_SetColor( color );

	// count the number of weapons owned
	if (wolfcam_following) {
		weapons = (1 << cg_entities[wcg.clientNum].currentState.weapon);
	} else {
		weapons = cg.snap->ps.stats[ STAT_WEAPONS ];
	}

	maxWeapons = WP_NUM_WEAPONS;
	if (cgs.protocol == PROTOCOL_Q3) {
		maxWeapons = WP_GRAPPLING_HOOK;
	}

	//Com_Printf("weapons: %d 0x%x\n", weapons, weapons);

	count = 0;

	for (i = 1;  i < maxWeapons;  i++) {
		if (!cg_weapons[i].registered)
			continue;
		if (i == WP_GAUNTLET)
			continue;

		count++;
	}

	elementWidth = 54;
	x = 320 - count * elementWidth / 2;

	y = 410;

	if (cg_weaponBar.integer == 1) {
		y = 90;
		x = 0;
	}

	if (cg_weaponBar.integer == 2) {
		y = 90;
		x = 640 - 50 - 4 - 4;
	}

	if (cg_weaponBarX.string[0] != '\0') {
		x = cg_weaponBarX.value;
	}
	if (cg_weaponBarY.string[0] != '\0') {
		y = cg_weaponBarY.value;
	}

	for ( i = 2 ; i < maxWeapons; i++ ) {
		wi = &cg_weapons[i];
		if (!wi->registered) {
			continue;
		}

		if ( ( weapons & ( 1 << i ) ) ) {
			haveWeapon = qtrue;
		} else {
			haveWeapon = qfalse;
		}

		if (cg_drawFullWeaponBar.integer == 0  && !haveWeapon) {
			continue;
		}

		// draw selection marker
		if (i == currentWeapon) {
			qhandle_t weaplit;

			weaplit = cgs.media.weaplit;

			color[0] = 1;
			color[1] = 1;
			color[2] = 1;
			color[3] = 0.8;

			trap_R_SetColor( color );

			if (cg_weaponBar.integer == 1) {
				CG_DrawPic(x - 2, y - 3, 50, 20, weaplit);
			} else if (cg_weaponBar.integer == 2) {
				CG_DrawPic(x + 8, y - 3, 50, 20, weaplit);
			} else if (cg_weaponBar.integer == 3) {
				CG_DrawPic(x - 4, y - 3, 50, 20, weaplit);
			}
			trap_R_SetColor(colorWhite);
		}

		// draw weapon icon
		if (cg_weaponBar.integer == 1) {
			CG_DrawPic( x, y, 16, 16, wi->weaponIcon );
		} else if (cg_weaponBar.integer == 2) {
			CG_DrawPic(x + 40, y, 16, 16, wi->weaponIcon);
		} else if (cg_weaponBar.integer == 3) {
			CG_DrawPic(x + 0, y + 0, 16, 16, wi->weaponIcon);
		}

		if (!haveWeapon) {
			goto finishLoop;
		}

		if (wolfcam_following) {
			goto finishLoop;
		}

		ammo = cg.snap->ps.ammo[i];
		Vector4Copy(colorWhite, textColor);

		ammoOffset = 0;
		style = cg_lowAmmoWarningStyle.integer;
		if (style == 0) {
			style = 2;
		} else if (style == 1) {
			ammoOffset = 1;
		}

		if (cg_lowAmmoWeaponBarWarning.integer == 1) {
			ammoWarning = CG_GetAmmoWarning(i, style, ammoOffset);
			if (ammoWarning == AMMO_WARNING_EMPTY) {
				Vector4Copy(colorRed, textColor);
			}
		} else if (cg_lowAmmoWeaponBarWarning.integer == 2) {
			ammoWarning = CG_GetAmmoWarning(i, style, ammoOffset);
			if (ammoWarning == AMMO_WARNING_EMPTY) {
				Vector4Copy(colorRed, textColor);
			} else if (ammoWarning == AMMO_WARNING_LOW) {
				Vector4Set(textColor, 0.996, 0.78125, 0.348, 1);
			}
		}

		// bleh...
		if (cg_lowAmmoWarningStyle.integer == 0  &&  i != currentWeapon) {
			Vector4Copy(colorWhite, textColor);
		}

		if (ammo < 0) {
			trap_R_SetColor(colorWhite);
			CG_DrawPic(x + 18, y , 16, 16, cgs.media.infiniteAmmo);
		} else if (cg_weaponBar.integer == 1  ||  cg_weaponBar.integer == 3) {
			CG_Text_Paint(x + 16, y + 12, 0.25, textColor, va(" %d",  ammo), 0, 0, 0, font);
		} else if (cg_weaponBar.integer == 2) {
			float textWidth;
			const char *s;

			s = va(" %d", ammo);
			textWidth = CG_Text_Width(s, 0.25, 0, font);
			CG_Text_Paint(x + 38 - textWidth, y + 12, 0.25, textColor, s, 0, 0, 0, font);
		}

	finishLoop:
		if (cg_weaponBar.integer < 3) {
			y += 22;
		} else if (cg_weaponBar.integer == 3) {
			x += elementWidth;
		}
	}
}

static void CG_DrawAreaNewChat (const odArgs_t *args)
{
	int i;
	int n;
	int ctime;
	float height;
	float scale;
	int count;
	int lines;
	int firstLine = -1;  // silence gcc warning
	//static int lastDrawActiveFrameCount = -1;

	//FIXME hack to avoid overdrawing chat area, happens with wolfcam_follow, need to check why
	// 2016-09-28 with wolfcam_follow this gets called twice and it will have different widescreen values -- 2016-09-29  no, menu simply being drawn twice with wolfcam_follow
	/*
	if (lastDrawActiveFrameCount == cg.drawActiveFrameCount) {
		return;
	}
	*/
	scale = args->scale;
	//Com_Printf("areanewchat scale %f\n", scale);

	//Com_Printf("areanewchat %d  widescreen:%d\n", cg.time, QLWideScreen);

	//scale = 0.25;  //FIXME  ??
	ctime = cg_chatTime.integer;
	if (cg.forceDrawChat) {
		lines = cg_chatHistoryLength.integer;
	} else {
		lines = cg_chatLines.integer;
	}

	if (lines >= MAX_CHAT_LINES) {
		lines = MAX_CHAT_LINES - 1;
	}

	//Com_Printf("areanewchat font: %p\n", args->font);

	//FIXME text max height
	height = CG_Text_Height("T", scale, 0, args->font);

	//height = 20;

	count = 0;
	for (i = 0;  i < lines;  i++) {
		n = cg.chatAreaStringsIndex - 1 - i;
		if (n < 0) {
			break;
		}
		n = n % MAX_CHAT_LINES;
		//FIXME hack to always show chat with scoreboard
		if (!cg.scoreBoardShowing) {
			if (!cg.forceDrawChat  &&  cg.time - cg.chatAreaStringsTime[n] > ctime) {
				break;
			}
		} else {

			//FIXME bah  ... quakelive
			//if ((args->y + (args->h) - (height + 4) * count) <= (args->y - args->h)) {
			//if (((height + 4) * count) >= args->h) {
			//if (((height + 4) * count) >= (args->h / 2)) {
			if (count > 2) {
				break;
			}
		}

		// don't wrap around
		if (i == 0) {
			firstLine = n;
		} else if (n == firstLine) {
			break;
		}

		//Com_Printf("text style: %d\n", args->textStyle);

		//FIXME height based on scale
		CG_Text_Paint(args->x, args->y + (args->h) - (height + 4) * count, scale, args->color, cg.chatAreaStrings[n], 0, 0, args->textStyle, args->font);

		// testing
#if 0
		{
			vec4_t color;

			//Com_Printf("acfcount %d\n", cg.drawActiveFrameCount);

			if (lastDrawActiveFrameCount == cg.drawActiveFrameCount) {
				Vector4Copy(colorRed, color);
			} else {
				Vector4Copy(colorYellow, color);
			}
			CG_Text_Paint(200, 400, 0.22, color, "test", 0, 0, 1, &cgs.media.attackerFont);
		}
#endif

		count++;
	}

	cg.numChatLinesVisible = count;

	//lastDrawActiveFrameCount = cg.drawActiveFrameCount;
}

#if 0  // broken
static int CG_NumOverTimes (void)
{
	  int numOverTimes = 0;
	  int overTimeAmount = 0;
	  int overTimeOffset = 0;
	  int timePlayed;

	  timePlayed = cg.time - cgs.levelStartTime;
	  //Com_Printf("timeplayed %d   cgs.timelimit %d\n", timePlayed, cgs.timelimit);
	  if (timePlayed > (cgs.timelimit * 60 * 1000)) {
		  overTimeAmount = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_overtime")) * 1000;
		  if (overTimeAmount > 0) {
			  numOverTimes = (timePlayed - (cgs.timelimit * 60 * 1000)) / overTimeAmount;
			  overTimeOffset = (timePlayed - (cgs.timelimit * 60 * 1000)) % overTimeAmount;
			  numOverTimes++;
		  } else {  //FIXME is this right
			  numOverTimes = 1;
			  overTimeOffset = 0;
			  numOverTimes = 0;
		  }
	  }

	  return numOverTimes;
}
#endif

int CG_GetCurrentTimeWithDirection (void)
{
	  int numOverTimes = 0;
	  int overTimeAmount = 0;
	  int overTimeOffset = 0;
	  int timePlayed;
	  int msec;
	  int cgtime;  // cg.time override
	  int gameStartTime;

	  //return cg.time - cgs.levelStartTime;

	  //gameStartTime = trap_GetGameStartTime();
	  gameStartTime = cgs.levelStartTime;

	  if (cg.warmup) {
		  msec = cg.time - gameStartTime;
		  cg.timerGoesUp = qtrue;
		  return msec;
	  }

	  if (CG_GameTimeout()) {
		  //Com_Printf("cgs.timeoutEndTime %d\n", cgs.timeoutEndTime - cg.time);
		  timePlayed = cgs.timeoutBeginTime - gameStartTime;
		  cgtime = cgs.timeoutBeginTime;
	  } else {
		  timePlayed = cg.time - gameStartTime;
		  //Com_Printf("timeplayed %d\n", timePlayed);
		  cgtime = cg.time;
	  }

		  //Com_Printf("timeplayed %d   cgs.timelimit %d\n", timePlayed, cgs.timelimit);

	  if (cgs.protocol == PROTOCOL_Q3) {

	  } else if ((cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG)  &&  cgs.realTimelimit) {
		  if (timePlayed > (cgs.timelimit * 60 * 1000)) {
			  numOverTimes = 1;
			  overTimeOffset = timePlayed - (cgs.timelimit * 60 * 1000);
			  overTimeAmount = cgs.timelimit;
		  } else {
			  numOverTimes = 0;
			  overTimeOffset = 0;
		  }
	  } else if (!cg.warmup) {
		  //FIXME take out, use CG_NumOverTimes(args);
		  if (timePlayed > (cgs.timelimit * 60 * 1000)) {
			  //Com_Printf("timeplayed %d  >  timelimit %d\n", timePlayed, cgs.timelimit * 60 * 1000);
			  overTimeAmount = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_overtime")) * 1000;
			  if (overTimeAmount) {
				  numOverTimes = (timePlayed - (cgs.timelimit * 60 * 1000)) / overTimeAmount;
			  } else {
				  numOverTimes = 0;
			  }
			  if (overTimeAmount) {
				  overTimeOffset = (timePlayed - (cgs.timelimit * 60 * 1000)) % overTimeAmount;
			  } else {
				  overTimeOffset = 0;
			  }
			  numOverTimes++;
		  }
	  }

	  //FIXME not here, what if this function isn't called
	  cgs.numOverTimes = numOverTimes;

    //
    if (cg_levelTimerDirection.integer == 0) {
		if (numOverTimes) {
			msec = overTimeOffset;
		} else {
			msec = cgtime - gameStartTime;
		}
		cg.timerGoesUp = qtrue;
	} else if (cg_levelTimerDirection.integer == 1) {
		if (numOverTimes) {
			msec = overTimeOffset;
			cg.timerGoesUp = qtrue;
		} else {
			msec = (gameStartTime + (cgs.timelimit * 60 * 1000)) - cgtime;
			cg.timerGoesUp = qfalse;
		}

		if (cgs.realTimelimit == 0) {
			if (numOverTimes) {
				msec = overTimeOffset;
			} else {
				msec = cgtime - gameStartTime;
			}
			cg.timerGoesUp = qtrue;
		}
	} else if (cg_levelTimerDirection.integer == 2) {
		msec = cgtime - gameStartTime;
		cg.timerGoesUp = qtrue;
	} else if (cg_levelTimerDirection.integer == 3) {
		if (numOverTimes) {
			//Com_Printf("yes overtime\n");
			msec = overTimeAmount - overTimeOffset;
			//Com_Printf("m %d\n", msec);
			if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
				//msec += (numOverTimes * cg_levelTimerDefaultTimeLimit.integer * 60 * 1000);
				msec = -overTimeOffset;
				while (msec < 0) {
					if (cg_levelTimerDefaultTimeLimit.integer > 0) {
						msec += cg_levelTimerDefaultTimeLimit.integer * 60 * 1000;
					} else {
						msec += 60 * 60 * 1000;
					}
				}
				//Com_Printf("x %d\n", msec);
			}
		} else {
			msec = (gameStartTime + (cgs.timelimit * 60 * 1000)) - cgtime;
		}
		cg.timerGoesUp = qfalse;
	} else if (cg_levelTimerDirection.integer == 4) {
		msec = trap_GetGameEndTime() - cgtime;
		if (msec < 0) {  // defaults to dir 3 behavior
			if (numOverTimes) {
				msec = overTimeAmount - overTimeOffset;
				if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
					msec = -overTimeOffset;
					while (msec < 0) {
						if (cg_levelTimerDefaultTimeLimit.integer > 0) {
							msec += cg_levelTimerDefaultTimeLimit.integer * 60 * 1000;
						} else {
							msec += 60 * 60 * 1000;
						}
					}
				}
			} else {
				msec = (gameStartTime + (cgs.timelimit * 60 * 1000)) - cgtime;
			}
		}
		cg.timerGoesUp = qfalse;
	} else {
		msec = cgtime - gameStartTime;
		cg.timerGoesUp = qtrue;
	}

    //Com_Printf("msec: %d\n", msec);
    return msec;
}

const char *CG_GetLevelTimerString (void)
{
	static char buffer[MAX_STRING_CHARS];
	const char *s;
	int hours, mins, seconds;
	int msec;

	msec = CG_GetCurrentTimeWithDirection();

	//Com_Printf("msec %d\n", msec);

	hours = (msec / 1000) / 3600;
	msec -= (hours * 3600 * 1000);

	mins = (msec / 1000) / 60;
	msec -= (mins * 60 * 1000);

	seconds = (msec / 1000);

	if (cg.warmup  &&  (cg_warmupTime.integer == 0  ||  cg_warmupTime.integer == 2)) {
		hours = mins = seconds = 0;
	}

	if (cg.warmup  &&  (cg_warmupTime.integer == 1  ||  cg_warmupTime.integer == 2)) {
		if (hours) {
			s = va("%i:%02i:%02i (warmup)", hours, mins, seconds);
		} else {
			s = va("%i:%02i (warmup)", mins, seconds);
		}
	} else {
		if (hours) {
			s = va("%i:%02i:%02i", hours, mins, seconds);
		} else {
			s = va("%i:%02i", mins, seconds);
		}
	}
	Q_strncpyz(buffer, s, sizeof(buffer));

	return buffer;
}

//FIXME only works with level timer up
double CG_GetServerTimeFromClockString (const char *timeString)
{
	qboolean wantWarmup;
	int slen;
	int colonCount;
	int i;
	double t;

	wantWarmup = qfalse;
	if (!Q_stricmp(timeString, "now")) {
		if (!cg.snap) {
			Com_Printf("CG_GetServerTimeFromClockString()  demo/play hasn't started yet\n");
			return -1;
		}
		//return cg.snap->serverTime;
		return cg.ftime;
	} else {
		if (timeString[0] == 'w'  ||  timeString[0] == 'W') {
			if (!cg.demoHasWarmup) {
				Com_Printf("CG_GetServerTimeFromClockString() demo doesn't have warmup\n");
				return -1;
			}
			wantWarmup = qtrue;
			timeString++;
		}
	}

	slen = strlen(timeString);
	colonCount = 0;
	for (i = 0;  i < slen;  i++) {
		if (timeString[i] == ':') {
			colonCount++;
		}
	}

	if (colonCount == 0) {
		// server time
		return atof(timeString);
	}

	t = Q_ParseClockTime(timeString);

	// no timeouts during warm up
	if (wantWarmup) {
		return (double)cg.warmupTimeStart + t;
	}

	if (trap_GetGameStartTime() < 0) {
		Com_Printf("CG_GetServerTimeFromClockString() demo doesn't contain a game\n");
		return -1;
	}

	t = CG_AdjustTimeForTimeouts(t, qtrue);
	//Com_Printf("%d  %d  %f\n", cgs.levelStartTime, trap_GetGameStartTime(), t);

	return (double)trap_GetGameStartTime() + t;
}

#if 0
int CG_GetGameStartTime (void)
{
	//FIXME trap_GetGameStartTime() should do this instead
	if (cg.demoHasWarmup) {
		return trap_GetGameStartTime();
	} else {
		return cgs.levelStartTime;
	}
}
#endif

static void CG_DrawTeamMapPickupsTdm (const rectDef_t *rect, float scale, int style, const fontInfo_t *font, int team)
{
	float x;
	float y;
	const tdmScore_t *sc;
	int ra, ya, ga, regen, haste, invis, mega, quad, bs, quadTime, bsTime, regenTime, hasteTime, invisTime;
	float w, h;
	float regScale;
	float spacing;
	const char *s;
	float textWidth;
	//rectDef_t textRect;
	float xspace;
	int diffArmorHealthCount;

	sc = &cg.tdmScore;

	if (team == TEAM_RED) {
		ra = sc->rra;
		ya = sc->rya;
		ga = sc->rga;
		mega = sc->rmh;
		quad = sc->rquad;
		bs = sc->rbs;
		regen = sc->rregen;
		haste = sc->rhaste;
		invis = sc->rinvis;

		quadTime = sc->rquadTime;
		bsTime = sc->rbsTime;
		regenTime = sc->rregenTime;
		hasteTime = sc->rhasteTime;
		invisTime = sc->rinvisTime;
	} else if (team == TEAM_BLUE) {
		ra = sc->bra;
		ya = sc->bya;
		ga = sc->bga;
		mega = sc->bmh;
		quad = sc->bquad;
		bs = sc->bbs;
		regen = sc->bregen;
		haste = sc->bhaste;
		invis = sc->binvis;

		quadTime = sc->bquadTime;
		bsTime = sc->bbsTime;
		regenTime = sc->bregenTime;
		hasteTime = sc->bhasteTime;
		invisTime = sc->binvisTime;
	} else {
		Com_Printf("^3CG_DrawTeamMapPickupsTdm()  invalid team number %d\n", team);
		return;
	}

	// testing
	//ra = ya = ga = mega = quad = bs = regen = haste = invis = 2;

	// scale 0.16
	w = 32;
	h = 32;

	regScale = 0.256;  //0.4;  //0.32;  // 0.25

	w = 32.0 * (scale / regScale);
	h = 32.0 * (scale / regScale);

	x = rect->x;

	w = rect->h;
	h = rect->h;

	spacing = h * 1.7;

	//Com_Printf("scale %f\n", scale);

	//memcpy(&textRect, rect, sizeof(textRect));

	//Com_Printf("h: %f\n", rect->h);

	y = rect->y;

	xspace = 0;  //CG_Text_Width("0", scale, 0, font) * 0.5;

	//scale *= 0.9;

	diffArmorHealthCount = 0;

	if (ra) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconra);
		s = va("%d", ra);
		textWidth = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		CG_Text_Paint(x + h - xspace, y + h, scale, colorWhite, s, 0, 0, style, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (ya) {

		//textRect.x += spacing;
		CG_DrawPic(x, rect->y, w, h, cgs.media.pickup_iconya);
		s = va("%d", ya);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (ga) {

		CG_DrawPic(x, rect->y, w, h, cgs.media.pickup_iconga);
		s = va("%d", ga);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (mega) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconmh);
		s = va("%d", mega);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}

	// ql  :{
	if (diffArmorHealthCount < 4) {
		x = rect->x + spacing * 3;
	}

	////

	//y = rect->y;

	s = va("0:00");
	y -= CG_Text_Height(s, scale, 0, font) * 1.5;


	//textHeight = CG_Text_Height(s, scale, 0, font);

	if (quad) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconquad);
		s = va("%d", quad);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", quadTime / 60, quadTime % 60);
		//textWidth = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint(x, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		//CG_Text_Paint(x + (h * 1.1) - textWidth, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (bs) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconbs);
		s = va("%d", bs);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", bsTime / 60, bsTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (regen) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconregen);
		s = va("%d", regen);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", regenTime / 60, regenTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (haste) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconhaste);
		s = va("%d", haste);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", hasteTime / 60, hasteTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (invis) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconinvis);
		s = va("%d", invis);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", invisTime / 60, invisTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
}

static void CG_DrawTeamMapPickupsCtf (const rectDef_t *rect, float scale, int style, const fontInfo_t *font, int team)
{
	float x;
	float y;
	const ctfScore_t *sc;
	int ra, ya, ga, regen, haste, invis, mega, medkit, flag, quad, bs, quadTime, bsTime, regenTime, hasteTime, invisTime, flagTime;
	float w, h;
	float regScale;
	float spacing;
	const char *s;
	float textWidth;
	//rectDef_t textRect;
	float xspace;
	int diffArmorHealthCount;

	sc = &cg.ctfScore;

	if (team == TEAM_RED) {
		ra = sc->rra;
		ya = sc->rya;
		ga = sc->rga;
		mega = sc->rmh;
		quad = sc->rquad;
		bs = sc->rbs;
		regen = sc->rregen;
		haste = sc->rhaste;
		invis = sc->rinvis;
		medkit = sc->rmedkit;
		flag = sc->rflag;

		quadTime = sc->rquadTime;
		bsTime = sc->rbsTime;
		regenTime = sc->rregenTime;
		hasteTime = sc->rhasteTime;
		invisTime = sc->rinvisTime;
		flagTime = sc->rflagTime;
	} else if (team == TEAM_BLUE) {
		ra = sc->bra;
		ya = sc->bya;
		ga = sc->bga;
		mega = sc->bmh;
		quad = sc->bquad;
		bs = sc->bbs;
		regen = sc->bregen;
		haste = sc->bhaste;
		invis = sc->binvis;
		medkit = sc->bmedkit;
		flag = sc->bflag;

		quadTime = sc->bquadTime;
		bsTime = sc->bbsTime;
		regenTime = sc->bregenTime;
		hasteTime = sc->bhasteTime;
		invisTime = sc->binvisTime;
		flagTime = sc->bflagTime;
	} else {
		Com_Printf("^3CG_DrawTeamMapPickupsCtf()  invalid team number %d\n", team);
		return;
	}

	// testing
	//ra = ya = ga = mega = quad = bs = regen = haste = invis = 2;

	// scale 0.16
	w = 32;
	h = 32;

	regScale = 0.256;  //0.4;  //0.32;  // 0.25

	w = 32.0 * (scale / regScale);
	h = 32.0 * (scale / regScale);

	x = rect->x;

	w = rect->h;
	h = rect->h;

	spacing = h * 1.7;

	//Com_Printf("scale %f\n", scale);

	//memcpy(&textRect, rect, sizeof(textRect));

	//Com_Printf("h: %f\n", rect->h);

	y = rect->y;

	xspace = 0;  //CG_Text_Width("0", scale, 0, font) * 0.5;

	//scale *= 0.9;

	diffArmorHealthCount = 0;

	if (ra) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconra);
		s = va("%d", ra);
		textWidth = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		CG_Text_Paint(x + h - xspace, y + h, scale, colorWhite, s, 0, 0, style, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (ya) {
		//textRect.x += spacing;
		CG_DrawPic(x, rect->y, w, h, cgs.media.pickup_iconya);
		s = va("%d", ya);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (ga) {

		CG_DrawPic(x, rect->y, w, h, cgs.media.pickup_iconga);
		s = va("%d", ga);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (mega) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconmh);
		s = va("%d", mega);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}
	if (medkit) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconmedkit);
		s = va("%d", medkit);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		diffArmorHealthCount++;
		x += spacing;
	}

	// ql  :{
	if (diffArmorHealthCount < 4) {
		x = rect->x + spacing * 3;
	}

	////

	//y = rect->y;

	s = va("0:00");
	y -= CG_Text_Height(s, scale, 0, font) * 1.5;


	//textHeight = CG_Text_Height(s, scale, 0, font);

	if (flag) {

		if (cgs.gametype == GT_1FCTF) {
			CG_DrawPic(x, y, w, h, cgs.media.pickup_iconneutralflag);
		} else if (team == TEAM_RED) {
			CG_DrawPic(x, y, w, h, cgs.media.pickup_iconblueflag);
		} else {
			CG_DrawPic(x, y, w, h, cgs.media.pickup_iconredflag);
		}
		s = va("%d", flag);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", flagTime / 60, flagTime % 60);
		//textWidth = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint(x, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		//CG_Text_Paint(x + (h * 1.1) - textWidth, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (quad) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconquad);
		s = va("%d", quad);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", quadTime / 60, quadTime % 60);
		//textWidth = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint(x, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		//CG_Text_Paint(x + (h * 1.1) - textWidth, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (bs) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconbs);
		s = va("%d", bs);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", bsTime / 60, bsTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (regen) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconregen);
		s = va("%d", regen);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", regenTime / 60, regenTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (haste) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconhaste);
		s = va("%d", haste);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", hasteTime / 60, hasteTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
	if (invis) {

		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconinvis);
		s = va("%d", invis);
		textWidth = 0;  //CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x + h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		s = va("%d:%02d", invisTime / 60, invisTime % 60);
		CG_Text_Paint(x + h * 0.1, rect->y + h, scale, colorWhite, s, 0, 0, 0, font);
		x += spacing;
	}
}

static void CG_DrawTeamMapPickups (const rectDef_t *rect, float scale, int style, const fontInfo_t *font, int team)
{
	if (cgs.gametype == GT_DOMINATION) {
		return;
	}

	if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_FREEZETAG) {
		CG_DrawTeamMapPickupsTdm(rect, scale, style, font, team);
	} else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_HARVESTER) {
		CG_DrawTeamMapPickupsCtf(rect, scale, style, font, team);
	} else {
		Com_Printf("^3CG_DrawTeamMapPickups() unsupported game type: %d\n", cgs.gametype);
	}
}

static void CG_Draw1stPlayerPickups (const rectDef_t *rect, float scale, int style, const fontInfo_t *font)
{
	const duelScore_t *ds;
	float x, y, w, h;
	const char *s;

	ds = &cg.duelScores[0];

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	if (ds->redArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconra);
		s = va("%d", ds->redArmorPickups);
		CG_Text_Paint(x + h, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->redArmorTime);
			CG_Text_Paint(x + h * 2, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->yellowArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconya);
		s = va("%d", ds->yellowArmorPickups);
		CG_Text_Paint(x + h, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->yellowArmorTime);
			CG_Text_Paint(x + h * 2, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->greenArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconga);
		s = va("%d", ds->greenArmorPickups);
		CG_Text_Paint(x + h, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->greenArmorTime);
			CG_Text_Paint(x + h * 2, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->megaHealthPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconmh);
		s = va("%d", ds->megaHealthPickups);
		CG_Text_Paint(x + h, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->megaHealthTime);
			CG_Text_Paint(x + h * 2, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
}

static void CG_Draw2ndPlayerPickups (const rectDef_t *rect, float scale, int style, const fontInfo_t *font)
{
	const duelScore_t *ds;
	float x, y, w, h;
	const char *s;
	float textWidth;

	ds = &cg.duelScores[1];

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	if (ds->redArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconra);
		s = va("%d", ds->redArmorPickups);
		textWidth = CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->redArmorTime);
			textWidth = CG_Text_Width(s, scale, 0, font);
			CG_Text_Paint(x - h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->yellowArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconya);
		s = va("%d", ds->yellowArmorPickups);
		textWidth = CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->yellowArmorTime);
			textWidth = CG_Text_Width(s, scale, 0, font);
			CG_Text_Paint(x - h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->greenArmorPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconga);
		s = va("%d", ds->greenArmorPickups);
		textWidth = CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->greenArmorTime);
			textWidth = CG_Text_Width(s, scale, 0, font);
			CG_Text_Paint(x - h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
	if (ds->megaHealthPickups) {
		CG_DrawPic(x, y, w, h, cgs.media.pickup_iconmh);
		s = va("%d", ds->megaHealthPickups);
		textWidth = CG_Text_Width(s, scale, 0, font);
		CG_Text_Paint(x - textWidth, y + h, scale, colorWhite, s, 0, 0, style, font);
		if (cgs.protocol == PROTOCOL_QL) {
			s = va("%.2f", ds->megaHealthTime);
			textWidth = CG_Text_Width(s, scale, 0, font);
			CG_Text_Paint(x - h - textWidth, y + h, scale, colorWhite, s, 0, 0, 0, font);
		}
		y += w;
	}
}

static void CG_SetArmorColor (void)
{
	if (wolfcam_following) {
		return;
	}

	switch (cg.snap->ps.stats[STAT_ARMOR_TIER]) {
	case 2:  // red
		trap_R_SetColor(colorRed);
		break;
	case 1:  // yellow
		trap_R_SetColor(colorYellow);
		break;
	default:
	case 0:  // green
		trap_R_SetColor(colorGreen);
		break;
	}
}

//FIXME hack
int MenuWidescreen = 0;
int QLWideScreen = 0;
rectDef_t MenuRect;

//
void CG_OwnerDraw (float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int ownerDrawFlags2, int align, float special, float scale, const vec4_t color, qhandle_t shader, int textStyle, int fontIndex, int menuWidescreen, int itemWidescreen, rectDef_t menuRect)
{
	rectDef_t rect;
	int ival;
	float s1, t1, s2, t2;
	odArgs_t args;
	int debug;
	const fontInfo_t *font;
	const char *s;
	int mins;
	int secs;
	const clientInfo_t *ci;
	int clientNum;
	vec4_t newColor;


	//FIXME debugging
	if (cg_wideScreen.integer == 6) {
		return;
	}

  if ( cg_drawStatus.integer == 0 ) {
		return;
  }

	//if (ownerDrawFlags != 0 && !CG_OwnerDrawVisible(ownerDrawFlags)) {
	//	return;
	//}

  if (fontIndex <= 0) {
	  font = &cgDC.Assets.textFont;
  } else {
	  font = &cgDC.Assets.extraFonts[fontIndex];
  }

  args.x = x;
  args.y = y;
  args.w = w;
  args.h = h;
  args.text_x = text_x;
  args.text_y = text_y;


  args.ownerDraw = ownerDraw;
  if (ownerDrawFlags) {
	  args.ownerDrawFlags = ownerDrawFlags;
  } else {
	  args.ownerDrawFlags = ownerDrawFlags2;
  }

  args.align = align;
  args.special = special;
  args.scale = scale;
  //args.color = color;
  VectorCopy(color, args.color);
  args.color[3] = color[3];
  args.shader = shader;
  args.textStyle = textStyle;
  args.font = font;

  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;

  MenuWidescreen = menuWidescreen;
  QLWideScreen = itemWidescreen;
  MenuRect = menuRect;
  
  //FIXME debugging widescreen
  //args.color[0] = 255;
  //args.color[1] = 255;
  //args.color[2] = 255;

  if (menuWidescreen) {
	  //args.color[0] = 0;
	  //rect.x += 300;
	  //x += 300;

	  if (!itemWidescreen) {
		  Com_Printf("^5... item %d  menu %d\n", itemWidescreen, menuWidescreen);
	  }
  }

  if (itemWidescreen) {
	  //args.color[1] = 0;
  }
  
  //Com_Printf("^3menu widescreen %d  item widescreen %d\n", menuWidescreen, itemWidescreen);

  if (menuWidescreen  ||  itemWidescreen) {
	  //Com_Printf("^3menu widescreen %d  item widescreen %d\n", menuWidescreen, itemWidescreen);
	  //FIXME testing widescren

	  int widescreenValue = menuWidescreen;
	  if (widescreenValue == 0) {
		  widescreenValue = itemWidescreen;
	  }

	  //rect.x += 300;
	  //x += 300;
  }



  {
	  int mn, mx;

	  mn = 0;
	  mx = 0;

	  if (ownerDraw < mn  ||  ownerDraw > mx) {
		  //return;
	  }
  }

  debug = SC_Cvar_Get_Int("cg_debugOwnerDraw");

  if (debug > 1) {
	  //CG_Text_Paint(rect.x, rect.y, scale, color, va("xxx %d  xxx", ownerDraw), 0, 0, textStyle, &cgDC.Assets.textFont);
	  CG_Text_Paint_Align(&rect, scale, color, va("xxx %d  xxx", ownerDraw), 0, 0, textStyle, &cgDC.Assets.textFont, align);
  }

  if (debug > 2) {
	  MenuWidescreen = 0;
	  QLWideScreen = 0;
	  return;
  }

  switch (ownerDraw) {
  case CG_PLAYER_ARMOR_ICON:  // 1
    CG_DrawPlayerArmorIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
    break;
  case CG_PLAYER_ARMOR_ICON2D:
    CG_DrawPlayerArmorIcon(&rect, qtrue);
    break;
  case CG_PLAYER_ARMOR_VALUE: // 2
	  CG_DrawPlayerArmorValue(&rect, scale, color, shader, textStyle, font, align, &menuRect);
    break;
  case CG_PLAYER_AMMO_ICON:  // 5
    CG_DrawPlayerAmmoIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
    break;
  case CG_PLAYER_AMMO_ICON2D:
    CG_DrawPlayerAmmoIcon(&rect, qtrue);
    break;
  case CG_PLAYER_AMMO_VALUE:  // 6
	  CG_DrawPlayerAmmoValue(&rect, scale, color, shader, textStyle, font, align, &menuRect);
    break;
  case CG_SELECTEDPLAYER_HEAD:  // 7
    CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse);
    break;
  case CG_VOICE_HEAD:
    CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue);
    break;
  case CG_VOICE_NAME:
	  CG_DrawSelectedPlayerName(&rect, scale, color, qtrue, textStyle, font);
    break;
  case CG_SELECTEDPLAYER_STATUS:  // 10
    CG_DrawSelectedPlayerStatus(&rect);
    break;
  case CG_SELECTEDPLAYER_ARMOR:
	  CG_DrawSelectedPlayerArmor(&rect, scale, color, shader, textStyle, font);
    break;
  case CG_SELECTEDPLAYER_HEALTH:
	  CG_DrawSelectedPlayerHealth(&rect, scale, color, shader, textStyle, font);
    break;
  case CG_SELECTEDPLAYER_NAME:  // 8
	  CG_DrawSelectedPlayerName(&rect, scale, color, qfalse, textStyle, font);
    break;
  case CG_SELECTEDPLAYER_LOCATION:  // 9
	  CG_DrawSelectedPlayerLocation(&rect, scale, color, textStyle, font);
    break;
  case CG_SELECTEDPLAYER_WEAPON:
    CG_DrawSelectedPlayerWeapon(&rect);
    break;
  case CG_SELECTEDPLAYER_POWERUP:
    CG_DrawSelectedPlayerPowerup(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
    break;
  case CG_PLAYER_HEAD:  // 3
	  CG_DrawPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue);
	  break;
  case WCG_REAL_PLAYER_HEAD:
	  CG_DrawPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse);
	  break;
  case CG_PLAYER_ITEM:
    CG_DrawPlayerItem(&rect, scale, ownerDrawFlags & CG_SHOW_2DONLY);
    break;
  case CG_PLAYER_SCORE:
	  CG_DrawPlayerScore(&rect, scale, color, shader, textStyle, font, align);
    break;
  case CG_PLAYER_HEALTH:  // 4
	  CG_DrawPlayerHealth(&rect, scale, color, shader, textStyle, font, align, &menuRect);
    break;
  case CG_RED_SCORE:
	  CG_DrawRedScore(&rect, scale, color, shader, textStyle, font, align);
    break;
  case CG_BLUE_SCORE:
	  CG_DrawBlueScore(&rect, scale, color, shader, textStyle, font, align);
    break;
  case CG_RED_NAME:
	  CG_DrawRedName(&rect, scale, color, textStyle, font, align);
    break;
  case CG_BLUE_NAME:
	  CG_DrawBlueName(&rect, scale, color, textStyle, font, align);
    break;
  case CG_BLUE_FLAGHEAD:
    CG_DrawBlueFlagHead(&rect);
    break;
  case CG_BLUE_FLAGSTATUS:
    CG_DrawBlueFlagStatus(&rect, shader);
    break;
  case CG_BLUE_FLAGNAME:
	  CG_DrawBlueFlagName(&rect, scale, color, textStyle, font);
    break;
  case CG_RED_FLAGHEAD:
    CG_DrawRedFlagHead(&rect);
    break;
  case CG_RED_FLAGSTATUS:
    CG_DrawRedFlagStatus(&rect, shader);
    break;
  case CG_RED_FLAGNAME:
	  CG_DrawRedFlagName(&rect, scale, color, textStyle, font);
    break;

	/*
#define CG_FLAGCARRIER_HEAD 13
#define CG_FLAGCARRIER_NAME 14
#define CG_FLAGCARRIER_LOCATION 15
#define CG_FLAGCARRIER_STATUS 16
#define CG_FLAGCARRIER_WEAPON 17
#define CG_FLAGCARRIER_POWERUP 18
	*/

  case CG_HARVESTER_SKULLS:
	  //Com_Printf("text_x %f  text_y %f\n", text_x, text_y);
	  CG_HarvesterSkulls(&rect, scale, color, qfalse, textStyle, font);
    break;
  case CG_HARVESTER_SKULLS2D:
	  CG_HarvesterSkulls(&rect, scale, color, qtrue, textStyle, font);
    break;
  case CG_ONEFLAG_STATUS:
    CG_OneFlagStatus(&rect);
    break;
  case CG_PLAYER_LOCATION:
	  CG_DrawPlayerLocation(&rect, scale, color, textStyle, font);
    break;
  case CG_TEAM_COLOR:
	  //Com_Printf("CG_TEAM_COLOR\n");
    CG_DrawTeamColor(&rect, color);
    break;
  case CG_CTF_POWERUP:
    CG_DrawCTFPowerUp(&rect);
    break;
  case CG_AREA_POWERUP:
	  CG_DrawAreaPowerUp(&rect, align, special, scale, color, font);
    break;
  case CG_PLAYER_STATUS:
    CG_DrawPlayerStatus(&rect);
    break;
  case CG_PLAYER_HASFLAG:
    CG_DrawPlayerHasFlag(&rect, qfalse);
    break;
  case CG_PLAYER_HASFLAG2D:
    CG_DrawPlayerHasFlag(&rect, qtrue);
    break;
  case CG_AREA_SYSTEMCHAT:
	  CG_DrawAreaSystemChat(&rect, scale, color, shader, font);
    break;
  case CG_AREA_TEAMCHAT:
	  CG_DrawAreaTeamChat(&rect, scale, color, shader, font);
    break;
  case CG_AREA_CHAT:
	  CG_DrawAreaChat(&rect, scale, color, shader, font);
    break;
  case CG_GAME_TYPE:
	  //CG_DrawGameType(&rect, scale, color, shader, textStyle, font);
	  //CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, CG_GameTypeString(), 0, 0, textStyle, font);
	  CG_Text_Paint_Align(&rect, scale, color, CG_GameTypeString(), 0, 0, textStyle, font, align);
	  break;
  case CG_GAME_STATUS:
	  CG_DrawGameStatus(&rect, scale, color, shader, textStyle, font, align);
	  break;
  case CG_KILLER:
	  CG_DrawKiller(&rect, scale, color, shader, textStyle, font, align);
	  break;
	case CG_ACCURACY:
	case CG_ASSISTS:
	case CG_DEFEND:
	case CG_EXCELLENT:
	case CG_IMPRESSIVE:
	case CG_PERFECT:
	case CG_GAUNTLET:
	case CG_CAPTURES:
		CG_DrawMedal(ownerDraw, &rect, scale, color, shader, font);
		break;
  case CG_SPECTATORS:
	  CG_DrawTeamSpectators(&rect, scale, color, shader, font);
		break;
  case CG_TEAMINFO:
		if (cg_currentSelectedPlayer.integer == numSortedTeamPlayers) {
			CG_DrawNewTeamInfo(&rect, text_x, text_y, scale, color, shader, font);
		}
		break;
  case CG_CAPFRAGLIMIT:
	  CG_DrawCapFragLimit(&rect, scale, color, shader, textStyle, font, align, &menuRect);
		break;
  case CG_1STPLACE:
	  CG_Draw1stPlace(&rect, scale, color, shader, textStyle, font, align);
		break;
  case CG_2NDPLACE:  // 68
	  CG_Draw2ndPlace(&rect, scale, color, shader, textStyle, font, align);
		break;

		////////////////////////////////////////////////////

		//CG_FULLTEAMINFO 70  // some weird heart thing

  case CG_LEVELTIMER:  { // 71
#if 0
	  const char *s;
	  int hours, mins, seconds;
	  int msec;

	  msec = CG_GetCurrentTimeWithDirection();

	  hours = (msec / 1000) / 3600;
	  msec -= (hours * 3600 * 1000);

	  mins = (msec / 1000) / 60;
	  msec -= (mins * 60 * 1000);

	  seconds = (msec / 1000);

	  if (cg.warmup  &&  !cg_warmupTime.integer) {
		  hours = mins = seconds = 0;
	  }

	  if (cg.warmup  &&  cg_warmupTime.integer > 0) {
		  if (hours) {
			  s = va("%i:%02i:%02i (warmup)", hours, mins, seconds);
		  } else {
			  s = va("%i:%02i (warmup)", mins, seconds);
		  }
	  } else {
		  if (hours) {
			  s = va("%i:%02i:%02i", hours, mins, seconds);
		  } else {
			  s = va("%i:%02i", mins, seconds);
		  }
	  }
#endif
	  CG_Text_Paint(rect.x, rect.y, scale, color, CG_GetLevelTimerString(), 0, 0, textStyle, font);
	  break;
  }
	  // CG_PLAYER_SKILL 72  // doesn't appear to be used

  case CG_PLAYER_OBIT:  // 73
	  CG_DrawObit(&rect, scale, color, shader, textStyle, font);
	  break;
  case CG_PLAYER_HEALTH_BAR_100:  // 74
	  ival = cg.snap->ps.stats[STAT_HEALTH];

	  if (wolfcam_following) {
		  //break;  //FIXME
		  //ival = wclients[wcg.clientNum].eventHealth;  //FIXME time, team info
		  ival = Wolfcam_PlayerHealth(wcg.clientNum);
		  if (ival <= INVALID_WOLFCAM_HEALTH) {
			  break;
		  }
	  }

	  if (ival > 100) {
		  ival = 100;
	  }
	  if (ival < 0) {
		  ival = 0;
	  }

	  CG_SetTeamColor();
	  //CG_DrawPic(rect.x, rect.y, rect.w * (ival / 100.0), rect.h, shader);

	  //CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
	  s1 = 0;
	  t1 = 0;
	  s2 = (ival / 100.0);
	  t2 = 1;
	  //trap_R_DrawStretchPic( rect.x, rect.y, rect.w * (ival / 100.0), rect.h, s1, t1, s2, t2, shader );
	  CG_DrawStretchPic( rect.x, rect.y, rect.w * (ival / 100.0), rect.h, s1, t1, s2, t2, shader );
	  break;
  case CG_PLAYER_HEALTH_BAR_200:  // 75
	  ival = cg.snap->ps.stats[STAT_HEALTH];

	  if (wolfcam_following) {
		  //break;  //FIXME
		  //ival = wclients[wcg.clientNum].eventHealth;  //FIXME time, team info
		  ival = Wolfcam_PlayerHealth(wcg.clientNum);
		  if (ival <= INVALID_WOLFCAM_HEALTH) {
			  break;
		  }
	  }

	  ival -= 100;
	  if (ival > 100) {
		  ival = 100;
	  }
	  if (ival < 0) {
		  ival = 0;
	  }
	  CG_SetTeamColor();
	  //CG_DrawPic(rect.x, rect.y, rect.w, rect.h * 0.5, shader);
	  CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
	  s1 = 0;
	  t1 = 1.0 - (ival / 100.0);  //0.5;
	  s2 = 1;  //0.5;
	  t2 = 1;
	  trap_R_DrawStretchPic( rect.x, rect.y + rect.h * (1 - ival / 100.0), rect.w, rect.h * (ival / 100.0), s1, t1, s2, t2, shader );
	  break;
  case CG_PLAYER_ARMOR_BAR_100:  // 76
	  ival = cg.snap->ps.stats[STAT_ARMOR];

	  if (wolfcam_following) {
		  //break;  //FIXME team info
		  ival = Wolfcam_PlayerArmor(wcg.clientNum);
		  if (ival < 0) {
			  break;
		  }
	  }

	  if (ival > 100) {
		  ival = 100;
	  }
	  if (ival < 0) {
		  ival = 0;
	  }
	  CG_SetTeamColor();
	  //CG_DrawPic(rect.x, rect.y, rect.w * (ival / 100.0), rect.h, shader);
	  CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
	  s1 = 1.0 - (ival / 100.0);
	  t1 = 0;
	  s2 = 1;  //(ival / 100.0);
	  t2 = 1;
	  trap_R_DrawStretchPic( rect.x + rect.w * (1 - ival / 100.0), rect.y, rect.w * (ival / 100.0), rect.h, s1, t1, s2, t2, shader );
	  break;
  case CG_PLAYER_ARMOR_BAR_200:  // 77
	  ival = cg.snap->ps.stats[STAT_ARMOR];

	  if (wolfcam_following) {
		  //break;  //FIXME team info
		  ival = Wolfcam_PlayerArmor(wcg.clientNum);
		  if (ival < 0) {
			  break;
		  }
	  }

	  ival -= 100;
	  if (ival > 100) {
		  ival = 100;
	  }
	  if (ival < 0) {
		  ival = 0;
	  }
	  CG_SetTeamColor();
	  //CG_DrawPic(rect.x, rect.y, rect.w, rect.h * (ival / 100.0), shader);
	  CG_AdjustFrom640( &rect.x, &rect.y, &rect.w, &rect.h );
	  s1 = 0;
	  t1 = 1.0 - (ival / 100.0);  //0.5;
	  s2 = 1;  //0.5;
	  t2 = 1;
	  trap_R_DrawStretchPic( rect.x, rect.y + rect.h * (1 - ival / 100.0), rect.w, rect.h * (ival / 100.0), s1, t1, s2, t2, shader );
	  break;
  case CG_AREA_NEW_CHAT:  // 78
	  // hack for new ql which uses notosans-regular in area chat but doesn't set anything in the menu file
	  if (fontIndex <= 0) {
		  // not &cgDC.Assets.textFont
		  args.font = &cg.notosansFont;
	  }

	  CG_DrawAreaNewChat(&args);
	  break;
  case CG_TEAM_COLORIZED:  // 79
	  //Com_Printf("CG_TEAM_COLORIZED\n");
	  CG_SetTeamColor();  //FIXME redrover
	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;
  case CG_1ST_PLACE_SCORE:  // 80
	  CG_Draw1stPlaceScore(&rect, scale, color, textStyle, font);
	  break;
  case CG_2ND_PLACE_SCORE:  // 81
	  CG_Draw2ndPlaceScore(&rect, scale, color, textStyle, font);
	  break;
  case CG_GAME_TYPE_ICON:  { // 82
	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.gametypeIcon[cgs.gametype]);
	  break;
  }
	  // CG_1STPLACE_PLYR_MODEL 83

  case CG_MATCH_WINNER:  {  // 84
	  vec4_t ourColor = { 1, 1, 1, 1 };

	  if (!CG_IsTeamGame(cgs.gametype)  ||  cgs.gametype == GT_RED_ROVER) {
		  float w;

		  //w = CG_Text_Width(CG_ConfigString(CS_FIRSTPLACE), scale, 0, font);
		  w = CG_Text_Width(cgs.firstPlace, scale, 0, font);
		  //CG_Text_Paint_Align(&rect, scale, color, va("%s", CG_ConfigString(CS_FIRSTPLACE)), 0, 0, textStyle, font, align);
		  CG_Text_Paint_Align(&rect, scale, color, va("%s", cgs.firstPlace), 0, 0, textStyle, font, align);
		  rect.x += w;
		  ourColor[3] = color[3];
		  if (cgs.protocol == PROTOCOL_QL  &&  cgs.gametype == GT_TOURNAMENT  &&  cg.duelForfeit) {
			  CG_Text_Paint_Align(&rect, scale, ourColor, " WINS by forfeit", 0, 0, textStyle, font, align);
		  } else {
			  CG_Text_Paint_Align(&rect, scale, ourColor, " WINS", 0, 0, textStyle, font, align);
		  }
	  } else {
		  //FIXME color getting set?
		  //Vector4Copy(color, ourColor);
		  ourColor[3] = color[3];
		  if (CG_ScoresEqual(cgs.scores1, cgs.scores2)) {
			  CG_Text_Paint_Align(&rect, scale, ourColor, va("^4Blue Team ^7WINS by a score of %d to %d", cgs.scores2, cgs.scores1), 0, 0, textStyle, font, align);
		  } else if (cgs.scores1 > cgs.scores2) {
			  CG_Text_Paint_Align(&rect, scale, ourColor, va("^1Red Team ^7WINS by a score of %d to %d", cgs.scores1, cgs.scores2), 0, 0, textStyle, font, align);
		  } else {
			  //CG_Text_Paint_Align(&rect, scale, color, "Blue wins", 0, 0, textStyle, font, align);
			  CG_Text_Paint_Align(&rect, scale, ourColor, va("^4Blue Team ^7WINS by a score of %d to %d", cgs.scores2, cgs.scores1), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  }
  case CG_MATCH_END_CONDITION:  // 85
	  //FIXME GT_CTFS
	  if (cgs.gametype == GT_FFA  ||  cgs.gametype == GT_TOURNAMENT  ||  cgs.gametype == GT_TEAM) {
		  s = "Highest score within the time limit";  // not really true for ffa, but used in ql
	  } else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CA) {
		  s = "First to reach the round limit";
	  } else if (cgs.gametype == GT_FREEZETAG) {
		  s = "Highest score within the time limit";
	  } else if (cgs.gametype == GT_RED_ROVER  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_OBELISK  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS) {
		  s = "Highest score at the end of the game";
	  } else if (cgs.gametype == GT_RACE) {
		  s = "Fastest race time within the time limit";
	  } else {
		  s = "";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  case CG_PLYR_END_GAME_SCORE:  // 86
	  // chopping off color code like ql
	  if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
		  int i;
		  const score_t *sc;
		  const ctfPlayerScore_t *cs;
		  int assists = 0;
		  int captures = 0;
		  int defends = 0;

		  if (cg.ctfScore.valid  &&  cg.ctfScore.version >= 1) {
			  for (i = 0;  i < cg.ctfScore.numPlayerScores;  i++) {
				  cs = &cg.ctfScore.playerScore[i];
				  if (cs->clientNum == cg.snap->ps.clientNum) {
					  assists = cs->awardAssist;
					  captures = cs->captures;
					  defends = cs->awardDefend;
					  break;
				  }
			  }
		  } else {
			  for (i = 0;  i < cg.numScores;  i++) {
				  sc = &cg.scores[i];
				  if (sc->client == cg.snap->ps.clientNum) {
					  assists = sc->assistCount;
					  captures = sc->captures;
					  defends = sc->defendCount;
					  break;
				  }
			  }
		  }

		  s = CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1);
		  if (s[0] == '^') {
			  s += 2;
		  }
		  if (cgs.gametype == GT_FREEZETAG) {
			  if (assists) {
				  CG_Text_Paint_Align(&rect, scale, color, va("You had %d assist%s", assists, assists == 1 ? "." : "s."), 0, 0, textStyle, font, align);
			  } else {
				  CG_Text_Paint_Align(&rect, scale, color, va("You finished with a score of %d.", cg.snap->ps.persistant[PERS_SCORE]), 0, 0, textStyle, font, align);
			  }
		  } else if (cgs.gametype == GT_CA) {  //FIXME in quakelive red rover too
			  CG_Text_Paint_Align(&rect, scale, color, va("You finished with a score of %d.", cg.snap->ps.persistant[PERS_SCORE]), 0, 0, textStyle, font, align);
		  } else if (cgs.gametype == GT_RACE) {
			  CG_Text_Paint_Align(&rect, scale, color, va("You finished %s with a time of %s", s, CG_GetTimeString(cg.snap->ps.persistant[PERS_SCORE])), 0, 0, textStyle, font, align);
		  } else if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_OBELISK  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS) {  //FIXME OBELISK like quakelive -- even if wrong
			  if (captures) {
				  if (cgs.gametype == GT_HARVESTER) {
					  CG_Text_Paint_Align(&rect, scale, color, va("You captured %d skull%s", captures, captures == 1 ? "." : "s."), 0, 0, textStyle, font, align);
				  } else {
					  CG_Text_Paint_Align(&rect, scale, color, va("You had %d flag capture%s", captures, captures == 1 ? "." : "s."), 0, 0, textStyle, font, align);
				  }
			  } else if (assists) {
				  CG_Text_Paint_Align(&rect, scale, color, va("You had %d assist%s", assists, assists == 1 ? "." : "s."), 0, 0, textStyle, font, align);
			  } else if (defends) {
				  CG_Text_Paint_Align(&rect, scale, color, va("You had %d defend%s", defends, defends == 1 ? "." : "s."), 0, 0, textStyle, font, align);
			  } else {
				  CG_Text_Paint_Align(&rect, scale, color, va("You finished with a score of %d.", cg.snap->ps.persistant[PERS_SCORE]), 0, 0, textStyle, font, align);
			  }
		  }	else {
			  CG_Text_Paint_Align(&rect, scale, color, va("You finished %s with a score of %d.", s, cg.snap->ps.persistant[PERS_SCORE]), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_MAP_NAME: { //  87
	  if (cg.scoreBoardShowing  &&  cgs.protocol == PROTOCOL_QL  &&  cgs.realProtocol < 91) {  // show skill rating
		  //FIXME hack don't do this here
		  CG_Text_Paint(rect.x, rect.y, scale, color, va("%s  ^2skr: %d", CG_ConfigString(CS_MESSAGE), atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO),"sv_skillrating"))), 0, 0, textStyle, font);
	  } else {
		  CG_Text_Paint(rect.x, rect.y, scale, color, CG_ConfigString(CS_MESSAGE), 0, 0, textStyle, font);
	  }
	  break;
  }
  case CG_PLYR_BEST_WEAPON_NAME: {  // 88
	  int w;

	  //FIXME shouldn't this be selected player?
	  //CG_Text_Paint(rect.x, rect.y, scale, color, weapNamesCasual[cg.scores[cg.snap->ps.clientNum].bestWeapon], 0, 0, textStyle, font);

	  if (cg.selectedScore < 0  ||  cg.selectedScore > MAX_CLIENTS) {
		  Com_Printf("^1CG_PLYR_BEST_WEAPON_NAME invalid client number %d\n", cg.selectedScore);
		  break;
	  }

	  w = cg.scores[cg.selectedScore].bestWeapon;
	  if (w < 0  ||  w > MAX_WEAPONS) {
		  Com_Printf("^1CG_PLYR_BEST_WEAPON_NAME invalid weapon number %d\n", w);
		  break;
	  }
	  CG_Text_Paint(rect.x, rect.y, scale, color, weapNamesCasual[w], 0, 0, textStyle, font);
	  //CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cg_weapons[cg.scores[cg.snap->ps.clientNum].bestWeapon].weaponIcon);
	  break;
  }
  case CG_SELECTED_PLYR_TEAM_COLOR:  {  // 89  //FIXME selectedScore
	  int team;

	  //team = cgs.clientinfo[cg.selectedScore].team;

	  if (wolfcam_following) {
		  team = cgs.clientinfo[wcg.clientNum].team;
	  } else {
		  team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	  }

	  if (team == TEAM_RED) {
		  trap_R_SetColor(colorRed);
	  } else if (team == TEAM_BLUE) {
		  trap_R_SetColor(colorBlue);
	  } else {
		  trap_R_SetColor(colorYellow);
	  }

	  if (team == TEAM_RED  ||  team == TEAM_BLUE) {
		  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  }
	  break;
  }
  case CG_SELECTED_PLYR_ACCURACY: { // 90  //FIXME selectedScore
	  //CG_Text_Paint(rect.x, rect.y, scale, color, va("%d%%", cg.scores[cg.selectedScore].accuracy), 0, 0, textStyle, font);
	  CG_Text_Paint(rect.x, rect.y, scale, color, va("%d%%", cg.scores[cg.selectedScore].accuracy), 0, 0, textStyle, font);
	  break;
  }
  case CG_PLAYER_COUNTS:  {  // 91
	  int i;
	  int count;
	  const clientInfo_t *ci;

	  //FIXME use cg.scores ?
	  //FIXME don't do it every time
	  //FIXME assuming this is only used for non team games
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  ci = &cgs.clientinfo[i];
		  if (!ci->infoValid) {
			  continue;
		  }
		  if (ci->team == TEAM_SPECTATOR) {
			  // ql includes the specs
			  //continue;
		  }
		  count++;
	  }

	  //CG_Text_Paint_Align(rect.x, rect.y, scale, color, va("%d/%s Players", count, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_maxclients")), 0, 0, textStyle, font, align);
	  CG_Text_Paint_Align(&rect, scale, color, va("%d/%s Players", count, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_maxclients")), 0, 0, textStyle, font, align);
	  break;
  }
  case CG_RED_PLAYER_COUNT:  {  // 92
	  int i;
	  int count;
	  const clientInfo_t *ci;
	  const char *teamSizeStr;
	  int teamSize;

	  //FIXME use cg.scores?
	  //FIXME don't do it every time
	  //FIXME assuming this is only used for team games
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  ci = &cgs.clientinfo[i];
		  if (!ci->infoValid) {
			  continue;
		  }
		  if (ci->team != TEAM_RED) {
			  continue;
		  }
		  count++;
	  }

	  //FIXME store teamsize in cgs.
	  teamSizeStr = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "teamsize");
	  teamSize = 0;
	  if (*teamSizeStr) {
		  teamSize = atoi(teamSizeStr);
	  }

	  if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_CA  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_DOMINATION) {

		  if (teamSize > 0) {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("(%d/%d)", count, teamSize), 0, 0, textStyle, font);
		  } else {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("(%d)", count), 0, 0, textStyle, font);
		  }
	  } else {
		  if (teamSize > 0) {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("RED - %d/%d PLAYERS", count, teamSize), 0, 0, textStyle, font);
		  } else {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("RED - %d PLAYERS", count), 0, 0, textStyle, font);
		  }
	  }
	  break;
  }
  case CG_BLUE_PLAYER_COUNT:  {  // 93
	  int i;
	  int count;
	  const clientInfo_t *ci;
	  const char *teamSizeStr;
	  int teamSize;

	  //FIXME use cg.scores?
	  //FIXME don't do it every time
	  //FIXME assuming this is only used for team games
	  count = 0;
	  for (i = 0;  i < MAX_CLIENTS;  i++) {
		  ci = &cgs.clientinfo[i];
		  if (!ci->infoValid) {
			  continue;
		  }
		  if (ci->team != TEAM_BLUE) {
			  continue;
		  }
		  count++;
	  }

	  //FIXME store teamsize in cgs.
	  teamSizeStr = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "teamsize");
	  teamSize = 0;
	  if (*teamSizeStr) {
		  teamSize = atoi(teamSizeStr);
	  }

	  if (cgs.gametype == GT_TEAM  ||  cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_CA  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_DOMINATION) {
		  if (teamSize > 0) {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("(%d/%d)", count, teamSize), 0, 0, textStyle, font);
		  } else {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("(%d)", count), 0, 0, textStyle, font);
		  }
	  } else {
		  if (teamSize > 0) {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("BLUE - %d/%d PLAYERS", count, teamSize), 0, 0, textStyle, font);
		  } else {
			  CG_Text_Paint(rect.x, rect.y, scale, color, va("BLUE - %d PLAYERS", count), 0, 0, textStyle, font);
		  }
	  }
	  break;
  }
  case CG_FOLLOW_PLAYER_NAME:  // 94
	  if (wolfcam_following) {
	      CG_Text_Paint(rect.x, rect.y, scale, color, va("%s", cgs.clientinfo[wcg.clientNum].name), 0, 0, textStyle, font);
	  } else {
	      CG_Text_Paint(rect.x, rect.y, scale, color, va("%s", cgs.clientinfo[cg.snap->ps.clientNum].name), 0, 0, textStyle, font);
	  }
		  break;
  case CG_RED_CLAN_PLYRS:  // 95
	  CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", cgs.redPlayersLeft), 0, 0, textStyle, font);
		  break;
  case CG_BLUE_CLAN_PLYRS:  // 96
	  CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", cgs.bluePlayersLeft), 0, 0, textStyle, font);
		  break;
  case CG_GAME_LIMIT:  // 97
	  CG_Text_Paint_Align(&rect, scale, color, va("Roundlimit: %s", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "roundlimit")), 0, 0, textStyle, font, align);
	  break;
  case CG_ROUNDTIMER: {  // 98
	  // 30 second warning counter
	  if (cgs.gametype != GT_CA  &&  cgs.gametype != GT_FREEZETAG  &&  cgs.gametype != GT_CTFS  &&  cgs.gametype != GT_RED_ROVER) {
		  break;  //FIXME you sure it's only ca?
	  }

	  ival = cg.time - atoi(CG_ConfigString(CS_ROUND_TIME));
	  if (cgs.roundStarted  &&  cgs.roundlimit  &&  cgs.roundtimelimit - (ival / 1000) <= 30) {
		  //FIXME color
		  if (cgs.roundtimelimit - (ival / 1000) >= 0) {
			  CG_Text_Paint(rect.x, rect.y, scale, colorRed, va("%d", cgs.roundtimelimit - (ival / 1000)), 0, 0, textStyle, font);
		  }
	  }
	  break;
  }

  case CG_RED_TIMEOUT_COUNT:  // 100
	  if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, va("TO: %s", CG_ConfigString(CS_RED_TEAM_TIMEOUTS_LEFT)), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TIMEOUT_COUNT:  // 101
	  if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, va("TO: %s", CG_ConfigString(CS_BLUE_TEAM_TIMEOUTS_LEFT)), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_1ST_PLACE_SCORE_EX:  // 102
	  //CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", cgs.scores1), 0, 0, textStyle, &cgDC.Assets.textFont);
	  CG_Draw1stPlaceScore(&rect, scale, color, textStyle, font);
	  break;
  case CG_2ND_PLACE_SCORE_EX:  // 103
	  //CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", cgs.scores2), 0, 0, textStyle, &cgDC.Assets.textFont);
	  CG_Draw2ndPlaceScore(&rect, scale, color, textStyle, font);
	  break;
  case CG_FOLLOW_PLAYER_NAME_EX:  // 104
	  if (wolfcam_following) {
		  CG_Text_Paint(rect.x, rect.y, scale, color, va("%s", cgs.clientinfo[wcg.clientNum].name), 0, 0, textStyle, font);
	  } else {
		  CG_Text_Paint(rect.x, rect.y, scale, color, va("%s", cgs.clientinfo[cg.snap->ps.clientNum].name), 0, 0, textStyle, font);
	  }
	  break;

  case CG_SPEEDOMETER:  { // 105
	  if (!wolfcam_following) {
		  CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", (int)cg.xyspeed), 0, 0, textStyle, font);
	  } else {
		  int speed;

		  if (wcg.clientNum != cg.snap->ps.clientNum) {
			  const entityState_t *es;

			  es = &cg_entities[wcg.clientNum].currentState;
			  speed = sqrt (es->pos.trDelta[0] * es->pos.trDelta[0] + es->pos.trDelta[1] * es->pos.trDelta[1]);
		  } else {
			  const playerState_t *ps;

			  //ps = &cg.predictedPlayerState;
			  ps = &cg.snap->ps;
			  speed = sqrt( ps->velocity[0] * ps->velocity[0] +
							ps->velocity[1] * ps->velocity[1] );
		  }
		  CG_Text_Paint(rect.x, rect.y, scale, color, va("%d", speed), 0, 0, textStyle, font);
	  }
	  break;
  }
	  //+#define CG_WP_VERTICAL 106
	  //+#define CG_ACC_VERTICAL 107

	  ////////////////////////////////////////////////////////////////
	  // 2010-12-14 new ql gillz scoreboard
	  //


	  //#define CG_1STPLACE_PLYR_MODEL_ACTIVE 108

  case CG_1ST_PLYR: {
	  const clientInfo_t *ci;
	  const char *s;

	  if (!cg.duelScoresValid) {
		  if (!CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
			  break;
		  } else {
			  ci = &cgs.clientinfo[cg.duelPlayer1];
		  }
		  // Com_Printf("^3invalid duel scores......\n");
	  } else {
		  ci = &cg.duelScores[0].ci;
	  }

	  if (*ci->clanTag) {
		  s = va("%s ^7%s", ci->clanTag, ci->name);
	  } else {
		  s = ci->name;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  }
  case CG_1ST_PLYR_SCORE:
	  if (cgs.protocol == PROTOCOL_QL  &&  cg.duelForfeit  &&  cg.duelPlayerForfeit == 1) {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
		  break;
	  }
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].score), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.clientinfo[cg.duelPlayer1].score), 0, 0, textStyle, font, align);
		  } else {
			  break;
		  }
	  }
	  break;
  case CG_1ST_PLYR_FRAGS:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].kills), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].frags), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_1ST_PLYR_DEATHS:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].deaths), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].deaths), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_1ST_PLYR_DMG:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].damage), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_1ST_PLYR_TIME:  // not used?
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].time), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].time), 0, 0, textStyle, font, align);
		  }
	  }
	  break;

  case CG_1ST_PLYR_PING: {
	  int colorNum;
	  int ping;

	  if (cg.duelScoresValid) {
		  ping = cg.duelScores[0].ping;
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  ping = cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].ping;
		  } else {
			  break;
		  }
	  }

	  if (ping < 41) {
		  colorNum = 2;
	  } else if (ping < 81) {
		  colorNum = 3;
	  } else {
		  colorNum = 1;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("^%d%d", colorNum, ping), 0, 0, textStyle, font, align);
	  break;
  }
  case CG_1ST_PLYR_WINS:  // 116
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d/%d", cgs.clientinfo[cg.duelPlayer1].wins, cgs.clientinfo[cg.duelPlayer1].losses), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_1ST_PLYR_ACC:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d%%", cg.duelScores[0].accuracy), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d%%", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].accuracy), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_1ST_PLYR_FLAG: {  // 118
	  const clientInfo_t *ci;

	  if (cg.duelScoresValid) {
		  ci = &cg.duelScores[0].ci;
	  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
		  ci = &cgs.clientinfo[cg.duelPlayer1];
	  } else {
		  break;
	  }

	  if (ci->countryFlag) {
		  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ci->countryFlag);
	  }
	  break;
  }
  case CG_1ST_PLYR_FULLCLAN: { // 119
	  const clientInfo_t *ci;

	  if (cg.duelScoresValid) {
		  ci = &cg.duelScores[0].ci;
	  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
		  ci = &cgs.clientinfo[cg.duelPlayer1];
	  } else {
		  break;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, ci->fullClanName, 0, 0, textStyle, font, align);
	  break;
  }
	  //#define CG_1ST_PLYR_TIMEOUT_COUNT 120  // not used?

  case CG_2ND_PLYR: {
	  const clientInfo_t *ci;
	  const char *s;

	  if (!cg.duelScoresValid) {
		  if (!CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  break;
		  } else {
			  ci = &cgs.clientinfo[cg.duelPlayer2];
		  }
	  } else {
		  ci = &cg.duelScores[1].ci;
	  }

	  if (*ci->clanTag) {
		  s = va("%s ^7%s", ci->clanTag, ci->name);
	  } else {
		  s = ci->name;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  }
  case CG_2ND_PLYR_SCORE:
	  if (cgs.protocol == PROTOCOL_QL  &&  cg.duelForfeit  &&  cg.duelPlayerForfeit == 2) {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
		  break;
	  }
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].score), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.clientinfo[cg.duelPlayer2].score), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_FRAGS:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].kills), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].frags), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_DEATHS:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].deaths), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].deaths), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_DMG:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].damage), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_2ND_PLYR_TIME:  // 126  not used?
	  if (CG_CheckQlVersion(0, 1, 0, 719)  &&  cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].time), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].time), 0, 0, textStyle, font, align);
		  }
	  }

  case CG_2ND_PLYR_PING: {
	  int colorNum;
	  int ping;

	  if (cg.duelScoresValid) {
		  ping = cg.duelScores[1].ping;
	  } else if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
		  ping = cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].ping;
	  } else {
		  break;
	  }

	  if (ping < 41) {
		  colorNum = 2;
	  } else if (ping < 81) {
		  colorNum = 3;
	  } else {
		  colorNum = 1;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("^%d%d", colorNum, ping), 0, 0, textStyle, font, align);
	  break;
  }
  case CG_2ND_PLYR_WINS:  // 128
	  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d/%d", cgs.clientinfo[cg.duelPlayer2].wins, cgs.clientinfo[cg.duelPlayer2].losses), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_2ND_PLYR_ACC:
	  if (cg.duelScoresValid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d%%", cg.duelScores[1].accuracy), 0, 0, textStyle, font, align);
	  } else {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d%%", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].accuracy), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_FLAG: {  // 130
	  const clientInfo_t *ci;

	  if (cg.duelScoresValid) {
		  ci = &cg.duelScores[1].ci;
	  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
		  ci = &cgs.clientinfo[cg.duelPlayer2];
	  } else {
		  break;
	  }

	  if (ci->countryFlag) {
		  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ci->countryFlag);
	  }
	  break;
  }
  case CG_2ND_PLYR_FULLCLAN: {  // 131
	  const clientInfo_t *ci;

	  if (cg.duelScoresValid) {
		  ci = &cg.duelScores[1].ci;
	  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
		  ci = &cgs.clientinfo[cg.duelPlayer2];
	  } else {
		  break;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, ci->fullClanName, 0, 0, textStyle, font, align);
	  break;
  }

	  //#define CG_2ND_PLYR_TIMEOUT_COUNT 132

  case CG_RED_AVG_PING: {
	  int colorNum;

	  if (cg.avgRedPing < 41) {
		  colorNum = 2;
	  } else if (cg.avgRedPing < 81) {
		  colorNum = 3;
	  } else {
		  colorNum = 1;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("^%d(%d)", colorNum, cg.avgRedPing), 0, 0, textStyle, font, align);
	  break;
  }
  case CG_BLUE_AVG_PING: {
	  int colorNum;

	  if (cg.avgBluePing < 41) {
		  colorNum = 2;
	  } else if (cg.avgBluePing < 81) {
		  colorNum = 3;
	  } else {
		  colorNum = 1;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("^%d(%d)", colorNum, cg.avgBluePing), 0, 0, textStyle, font, align);
	  break;
  }

	  //#define CG_VOTEMAP1 99  // never used?
	  //#define CG_VOTEMAP2 135  // never used?
	  //#define CG_VOTEMAP3 136  // never used?

  case CG_VOTESHOT1: {
	  qhandle_t shader = 0;
	  const char *info;

	  info = CG_ConfigString(CS_MAP_VOTE_INFO);
	  if (*info) {
		  //shader = trap_R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "map_0")));
		  if (CG_FileExists(va("levelshots/preview/%s", Info_ValueForKey(info, "map_0")))) {
			  shader = trap_R_RegisterShaderNoMip(va("levelshots/preview/%s", Info_ValueForKey(info, "map_0")));
		  }
	  }
	  if (!shader) {
		  shader = trap_R_RegisterShaderNoMip("levelshots/preview/default");
	  }

	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;
  }
  case CG_VOTESHOT2: {
	  qhandle_t shader = 0;
	  const char *info;

	  info = CG_ConfigString(CS_MAP_VOTE_INFO);
	  if (*info) {
		  //shader = trap_R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "map_1")));
		  if (CG_FileExists(va("levelshots/preview/%s", Info_ValueForKey(info, "map_1")))) {
			  shader = trap_R_RegisterShaderNoMip(va("levelshots/preview/%s", Info_ValueForKey(info, "map_1")));
			  //shader = trap_R_RegisterShaderNoMip(va("levelshots/preview/overlay"));
		  }
	  }
	  if (!shader) {
		  shader = trap_R_RegisterShaderNoMip("levelshots/preview/default");
	  }

	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;
  }
  case CG_VOTESHOT3: {
	  qhandle_t shader = 0;
	  const char *info;

	  info = CG_ConfigString(CS_MAP_VOTE_INFO);
	  if (*info) {
		  //shader = trap_R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "map_2")));
		  if (CG_FileExists(va("levelshots/preview/%s", Info_ValueForKey(info, "map_2")))) {
			  shader = trap_R_RegisterShaderNoMip(va("levelshots/preview/%s", Info_ValueForKey(info, "map_2")));
		  }
	  }
	  if (!shader) {
		  shader = trap_R_RegisterShaderNoMip("levelshots/preview/default");
	  }

	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;
  }

  case CG_VOTENAME1:
	  CG_Text_Paint_Align(&rect, scale, color, Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_INFO), "title_0"), 0, 0, textStyle, font, align);
	  break;
  case CG_VOTENAME2:
	  CG_Text_Paint_Align(&rect, scale, color, Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_INFO), "title_1"), 0, 0, textStyle, font, align);
	  break;
  case CG_VOTENAME3:
	  CG_Text_Paint_Align(&rect, scale, color, Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_INFO), "title_2"), 0, 0, textStyle, font, align);
	  break;

  case CG_VOTECOUNT1:
	  CG_Text_Paint_Align(&rect, scale, color, va("Votes: %s", Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "0")), 0, 0, textStyle, font, align);
	  break;
  case CG_VOTECOUNT2:
	  CG_Text_Paint_Align(&rect, scale, color, va("Votes: %s", Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "1")), 0, 0, textStyle, font, align);
	  break;
  case CG_VOTECOUNT3:
	  CG_Text_Paint_Align(&rect, scale, color, va("Votes: %s", Info_ValueForKey(CG_ConfigString(CS_MAP_VOTE_COUNT), "2")), 0, 0, textStyle, font, align);
	  break;

  case CG_VOTETIMER: {
	  int t;

	  //FIXME 20 sec fixed???
	  t = (atoi(CG_ConfigString(CS_VOTE_TIME)) + 20000 - cg.time) / 1000;

	  if (t > 0) {
		  CG_Text_Paint_Align(&rect, scale, color, va("Voting ends in %d seconds.", t), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "Voting has ended.", 0, 0, textStyle, font, align);
	  }
	  break;
  }

  case CG_BEST_ITEMCONTROL_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
		  //CG_Text_Paint_Align(&rect, scale, colorWhite, CG_ConfigString(CS_BEST_ITEM_CONTROL2), 0, 0, textStyle, font, align);
		  if (cgs.realProtocol >= 91) {
			  int n;

			  n = atoi(CG_ConfigString(CS_BEST_ITEM_CONTROL2));
			  if (n >= 0  &&  n <= MAX_CLIENTS) {
				  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
				  //FIXME steam avatar is shown
			  } else {
				  Com_Printf("^3CG_BEST_ITEMCONTROL_PLYR invalid player number %d\n", n);
			  }

		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_BEST_ITEM_CONTROL2), 0, 0, textStyle, font, align);
		  }
	  } else if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_BEST_ITEM_CONTROL), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_MOST_ACCURATE_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
		  if (cgs.realProtocol >= 91) {
			  int n;

			  n = atoi(CG_ConfigString(CS_MOST_ACCURATE2));
			  if (n >= 0  &&  n <= MAX_CLIENTS) {
				  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
				  //FIXME steam avatar is shown
			  } else {
				  Com_Printf("^3CG_MOST_ACCURATE_PLYR invalid player number %d\n", n);
			  }
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MOST_ACCURATE2), 0, 0, textStyle, font, align);
		  }
	  } else if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MOST_ACCURATE), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_MOST_DAMAGEDEALT_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (CG_CheckQlVersion(0, 1, 0, 495)  ||  cgs.realProtocol >= 91) {
		  if (cgs.realProtocol >= 91) {
			  int n;

			  n = atoi(CG_ConfigString(CS_MOST_DAMAGE_DEALT2));
			  if (n >= 0  &&  n <= MAX_CLIENTS) {
				  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
				  //FIXME steam avatar is shown
			  } else {
				  Com_Printf("^3CG_MOST_DAMAGEDEALT_PLYR invalid player number %d\n", n);
			  }
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MOST_DAMAGE_DEALT2), 0, 0, textStyle, font, align);
		  }
	  } else if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MOST_DAMAGE_DEALT), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_LOCALTIME: {
#if 0
	  qtime_t now;
	  const char *nowString;
	  int tm;
	  int offset;
	  qboolean timeAm = qfalse;

	  offset = cg.time - cgs.levelStartTime;
	  tm = atoi(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_levelStartTime"));
	  if (tm  &&  cg.demoPlayback  &&  cg_localTime.integer == 0) {
		  if (offset > 0) {
			  tm += (offset / 1000);
		  }
		  trap_RealTime(&now, qfalse, tm);
	  } else {
		  trap_RealTime(&now, qtrue, 0);
	  }
	  if (cg_localTimeStyle.integer == 0) {
		  nowString = va("%02d:%02d (%s %d, %d)", now.tm_hour, now.tm_min, MonthAbbrev[now.tm_mon], now.tm_mday, 1900 + now.tm_year);
	  } else {
		  if (now.tm_hour < 12) {
			  timeAm = qtrue;
			  if (now.tm_hour < 1) {
				  now.tm_hour = 12;
			  }
		  } else if (now.tm_hour >= 12) {
			  timeAm = qfalse;
			  if (now.tm_hour >= 13) {
				  now.tm_hour -= 12;
			  }
		  }
		  nowString = va("%d:%02d%s (%s %d, %d)", now.tm_hour, now.tm_min, timeAm ? "am" : "pm", MonthAbbrev[now.tm_mon], now.tm_mday, 1900 + now.tm_year);
	  }
#endif
	  CG_Text_Paint_Align(&rect, scale, color, CG_GetLocalTimeString(), 0, 0, textStyle, font, align);
	  break;
  }

  case CG_MATCH_DETAILS: {  // 151
	  const char *detail;
	  char mapname[MAX_QPATH];

	  if (cg.warmup) {
		  detail = "MATCH WARMUP";
	  } else {
		  if (cg.snap->ps.pm_type == PM_INTERMISSION) {
			  detail = "MATCH SUMMARY";
		  } else {
			  detail = "MATCH IN PROGRESS";
		  }
	  }

#define DETAIL_LIMIT 53

	  //FIXME or not:  using sv_location instead of gametype

	  if (cgs.protocol == PROTOCOL_QL) {
		  s = va("%s - %s %s - %s", detail, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_hostname"), Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_location"), CG_ConfigString(CS_MESSAGE));
		  CG_LimitText((char *)s, DETAIL_LIMIT);
		  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  } else {
		  //CG_Text_Paint_Align(&rect, scale, color, va("%s - %s %s", detail, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_hostname"), Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_location")), 0, 0, textStyle, font, align);
		  //FIXME store this
		  mapname[0] = '\0';
		  Q_strncpyz(mapname, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "mapname"), sizeof(mapname));
		  s = va("%s - %s ^7- %s", detail, Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_hostname"), mapname);
		  CG_LimitText((char *)s, DETAIL_LIMIT);
		  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  }

#undef DETAIL_LIMIT

	  break;
  }
  case CG_1ST_PLYR_FRAGS_G:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GAUNTLET].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_MACHINEGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].kills), 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_FRAGS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_SHOTGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_LIGHTNING].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_RAILGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PLASMAGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_BFG].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_CHAINGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_NAILGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_FRAGS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_MACHINEGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].hits), 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_HITS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_SHOTGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_LIGHTNING].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_RAILGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PLASMAGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_BFG].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_CHAINGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_NAILGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_HITS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_MACHINEGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].atts), 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_SHOTS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_SHOTGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_LIGHTNING].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_RAILGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PLASMAGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_BFG].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_CHAINGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_NAILGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_SHOTS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_G:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GAUNTLET].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_MACHINEGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_HEAVY_MACHINEGUN].damage), 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_DMG_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_SHOTGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_GRENADE_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_ROCKET_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_LIGHTNING].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_RAILGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PLASMAGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_BFG].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_CHAINGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_NAILGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_DMG_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].weaponStats[WP_PROX_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_ACC_MG:
	  ival = WP_MACHINEGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_ACC_HMG:
	  ival = WP_HEAVY_MACHINEGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_SG:
	  ival = WP_SHOTGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_GL:
	  ival = WP_GRENADE_LAUNCHER;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_RL:
	  ival = WP_ROCKET_LAUNCHER;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_LG:
	  ival = WP_LIGHTNING;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_RG:
	  ival = WP_RAILGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_PG:
	  ival = WP_PLASMAGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_BFG:
	  ival = WP_BFG;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_CG:
	  ival = WP_CHAINGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_NG:
	  ival = WP_NAILGUN;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_ACC_PL:
	  ival = WP_PROX_LAUNCHER;
	  if (cg.duelScores[0].weaponStats[ival].accuracy >= cg.duelScores[1].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[0].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_1ST_PLYR_PICKUPS_RA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].redArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_PICKUPS_YA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].yellowArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_PICKUPS_GA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].greenArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_PICKUPS_MH:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].megaHealthPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_AVG_PICKUP_TIME_RA:
	  if (cg.duelScores[0].redArmorPickups) {
		  s = va("%.2f", cg.duelScores[0].redArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_AVG_PICKUP_TIME_YA:
	  if (cg.duelScores[0].yellowArmorPickups) {
		  s = va("%.2f", cg.duelScores[0].yellowArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  case CG_1ST_PLYR_AVG_PICKUP_TIME_GA:
	  if (cg.duelScores[0].greenArmorPickups) {
		  s = va("%.2f", cg.duelScores[0].greenArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);

	  break;
  case CG_1ST_PLYR_AVG_PICKUP_TIME_MH:
	  if (cg.duelScores[0].megaHealthPickups) {
		  s = va("%.2f", cg.duelScores[0].megaHealthTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;


	  ///////////////////////////////////////////////////////////////////

	  //////////////////////////////////////////////////////


  case CG_2ND_PLYR_FRAGS_G:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GAUNTLET].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_MACHINEGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].kills), 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_FRAGS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_SHOTGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_LIGHTNING].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_RAILGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PLASMAGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_BFG].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_CHAINGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_NAILGUN].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_FRAGS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].kills), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_MACHINEGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].hits), 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_HITS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_SHOTGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_LIGHTNING].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_RAILGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PLASMAGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_BFG].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_CHAINGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_NAILGUN].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_HITS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].hits), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_MACHINEGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].atts), 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_SHOTS_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_SHOTGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_LIGHTNING].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_RAILGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PLASMAGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_BFG].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_CHAINGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_NAILGUN].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_SHOTS_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].atts), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_G:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GAUNTLET].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_MG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_MACHINEGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_HMG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_HEAVY_MACHINEGUN].damage), 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_DMG_SG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_SHOTGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_GL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_GRENADE_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_RL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_ROCKET_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_LG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_LIGHTNING].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_RG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_RAILGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_PG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PLASMAGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_BFG].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_CG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_CHAINGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_NG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_NAILGUN].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_DMG_PL:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].weaponStats[WP_PROX_LAUNCHER].damage), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_ACC_MG:
	  ival = WP_MACHINEGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_ACC_HMG:
	  ival = WP_HEAVY_MACHINEGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_SG:
	  ival = WP_SHOTGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_GL:
	  ival = WP_GRENADE_LAUNCHER;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_RL:
	  ival = WP_ROCKET_LAUNCHER;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_LG:
	  ival = WP_LIGHTNING;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_RG:
	  ival = WP_RAILGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_PG:
	  ival = WP_PLASMAGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_BFG:
	  ival = WP_BFG;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_CG:
	  ival = WP_CHAINGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_NG:
	  ival = WP_NAILGUN;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_ACC_PL:
	  ival = WP_PROX_LAUNCHER;
	  if (cg.duelScores[1].weaponStats[ival].accuracy >= cg.duelScores[0].weaponStats[ival].accuracy) {
		  Vector4Set(newColor, 1, 1, 1, 1);
		  s = va("^7%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  } else {
		  Vector4Copy(color, newColor);
		  s = va("%d%%", cg.duelScores[1].weaponStats[ival].accuracy);
	  }
	  CG_Text_Paint_Align(&rect, scale, newColor, s, 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_PICKUPS_RA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].redArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_PICKUPS_YA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].yellowArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_PICKUPS_GA:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].greenArmorPickups), 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_PICKUPS_MH:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].megaHealthPickups), 0, 0, textStyle, font, align);
	  break;

  case CG_2ND_PLYR_AVG_PICKUP_TIME_RA:
	  if (cg.duelScores[1].redArmorPickups) {
		  s = va("%.2f", cg.duelScores[1].redArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_AVG_PICKUP_TIME_YA:
	  if (cg.duelScores[1].yellowArmorPickups) {
		  s = va("%.2f", cg.duelScores[1].yellowArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;
  case CG_2ND_PLYR_AVG_PICKUP_TIME_GA:
	  if (cg.duelScores[1].greenArmorPickups) {
		  s = va("%.2f", cg.duelScores[1].greenArmorTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);

	  break;
  case CG_2ND_PLYR_AVG_PICKUP_TIME_MH:
	  if (cg.duelScores[1].megaHealthPickups) {
		  s = va("%.2f", cg.duelScores[1].megaHealthTime);
	  } else {
		  s = "-";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;

	  /////////////////////////////////////////


  case CG_1ST_PLYR_EXCELLENT:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[0].awardExcellent == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].awardExcellent), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].excellentCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_1ST_PLYR_IMPRESSIVE:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[0].awardImpressive == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].awardImpressive), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].impressiveCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_1ST_PLYR_HUMILIATION:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[0].awardHumiliation == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].awardHumiliation), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer1)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer1].scoreIndexNum].gauntletCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;

  case CG_2ND_PLYR_EXCELLENT:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[1].awardExcellent == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].awardExcellent), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].excellentCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_IMPRESSIVE:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[1].awardImpressive == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].awardImpressive), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].impressiveCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;
  case CG_2ND_PLYR_HUMILIATION:
	  if (cg.duelScoresValid) {
		  if (cgs.cpma  &&  cg.duelScores[1].awardHumiliation == -1) {
			  // pass
		  } else {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].awardHumiliation), 0, 0, textStyle, font, align);
		  }
	  } else if (!cgs.cpma  &&  cg.scoresValid) {
		  if (CG_DuelPlayerScoreValid(cg.duelPlayer2)) {
			  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.scores[cgs.clientinfo[cg.duelPlayer2].scoreIndexNum].gauntletCount), 0, 0, textStyle, font, align);
		  }
	  }
	  break;

  case CG_1ST_PLYR_READY:  // 288
	  if (cg.warmup) {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer1)  &&  cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << cg.duelPlayer1)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_ready");
		  } else {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_notready");
		  }
	  } else {
		  if (!CG_DuelPlayerInfoValid(cg.duelPlayer1)  &&  !CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_tied");
		  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer1)  &&  !CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_leads");
		  } else if (!CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_trails");
		  } else if (cgs.clientinfo[cg.duelPlayer1].score > cgs.clientinfo[cg.duelPlayer2].score) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_leads");
		  } else if (cgs.clientinfo[cg.duelPlayer1].score < cgs.clientinfo[cg.duelPlayer2].score) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_trails");
		  } else {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/1st_plyr_tied");
		  }
	  }

	  //CG_Text_Paint_Align(&rect, scale, color, "test1", 0, 0, textStyle, font, align);
	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;

  case CG_2ND_PLYR_READY:  // 289
	  if (cg.warmup) {
		  if (CG_DuelPlayerInfoValid(cg.duelPlayer2)  &&  cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << cg.duelPlayer2)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_ready");
		  } else {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_notready");
		  }
	  } else {
		  if (!CG_DuelPlayerInfoValid(cg.duelPlayer1)  &&  !CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_tied");
		  } else if (CG_DuelPlayerInfoValid(cg.duelPlayer2)  &&  !CG_DuelPlayerInfoValid(cg.duelPlayer1)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_leads");
		  } else if (!CG_DuelPlayerInfoValid(cg.duelPlayer2)) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_trails");
		  } else if (cgs.clientinfo[cg.duelPlayer2].score > cgs.clientinfo[cg.duelPlayer1].score) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_leads");
		  } else if (cgs.clientinfo[cg.duelPlayer2].score < cgs.clientinfo[cg.duelPlayer1].score) {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_trails");
		  } else {
			  shader = trap_R_RegisterShaderNoMip("ui/assets/score/2nd_plyr_tied");
		  }
	  }

	  //CG_Text_Paint_Align(&rect, scale, color, "test2", 0, 0, textStyle, font, align);
	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;

  // CG_SELECTED_PLYR_*  just duel?

  case CG_SELECTED_PLYR_NAME:
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (ci->clanTag) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%s ^7%s", ci->clanTag, ci->name), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, ci->name, 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_SELECTED_PLYR_SCORE:
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", ci->score), 0, 0, textStyle, font, align);
	  break;

  case CG_SELECTED_PLYR_DMG:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].damage), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].damage), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_SELECTED_PLYR_ACC:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].accuracy), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].accuracy), 0, 0, textStyle, font, align);
	  }
	  break;


  case CG_SELECTED_PLYR_FLAG:
	  ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (ci->countryFlag) {
		  // CG_Text_Paint_Align(&rect, scale, color, va("%s ^7%s", ci->clanTag, ci->name), 0, 0, textStyle, font, align);
		  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ci->countryFlag);
	  }
	  break;

  case CG_SELECTED_PLYR_FULLCLAN:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, cg.duelScores[0].ci.fullClanName, 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, cg.duelScores[1].ci.fullClanName, 0, 0, textStyle, font, align);
	  }
	  break;


  case CG_SELECTED_PLYR_PICKUPS_RA:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].redArmorPickups), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].redArmorPickups), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_SELECTED_PLYR_PICKUPS_YA:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].yellowArmorPickups), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].yellowArmorPickups), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_SELECTED_PLYR_PICKUPS_GA:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].greenArmorPickups), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].greenArmorPickups), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_SELECTED_PLYR_PICKUPS_MH:
	  clientNum = sortedTeamPlayers[CG_GetSelectedPlayer()];
	  if (clientNum == cg.duelPlayer1) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[0].megaHealthPickups), 0, 0, textStyle, font, align);
	  } else if (clientNum == cg.duelPlayer2) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.duelScores[1].megaHealthPickups), 0, 0, textStyle, font, align);
	  }
	  break;

	  //CG_1ST_PLYR_PR 300  // not used?
	  //CG_2ND_PLYR_PR 301  // not used?
	  //CG_1ST_PLYR_TIER 302  // not used?
	  //CG_2ND_PLYR_TIER 303  // not used?

  case CG_SERVER_OWNER: {
	  const char *ownerName = NULL;

	  ownerName = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "sv_owner");
	  if (!ownerName  ||  !*ownerName) {
		  ownerName = "Quake Live";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, va("%s", ownerName), 0, 0, textStyle, font, align);
	  break;
  }

  case CG_RED_TEAM_PICKUPS_RA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rra), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_PICKUPS_YA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rya), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_PICKUPS_GA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rga), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_PICKUPS_MH:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rmh), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_PICKUPS_QUAD:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rquad), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_PICKUPS_BS:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.rbs), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_BLUE_TEAM_PICKUPS_RA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bra), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_PICKUPS_YA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bya), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_PICKUPS_GA:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bga), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_PICKUPS_MH:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bmh), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_PICKUPS_QUAD:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bquad), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_PICKUPS_BS:
	  if (cg.tdmScore.valid) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.tdmScore.bbs), 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_RED_TEAM_TIMEHELD_QUAD:
	  if (!cg.tdmScore.valid) {
		  break;
	  }
	  if (cg.tdmScore.rquad) {
		  mins = cg.tdmScore.rquadTime / 60;
		  secs = cg.tdmScore.rquadTime % 60;
		  CG_Text_Paint_Align(&rect, scale, color, va("%d:%02d", mins, secs), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_TEAM_TIMEHELD_BS:
	  if (!cg.tdmScore.valid) {
		  break;
	  }
	  if (cg.tdmScore.rbs) {
		  mins = cg.tdmScore.rbsTime / 60;
		  secs = cg.tdmScore.rbsTime % 60;
		  CG_Text_Paint_Align(&rect, scale, color, va("%d:%02d", mins, secs), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_TIMEHELD_QUAD:
	  if (!cg.tdmScore.valid) {
		  break;
	  }
	  if (cg.tdmScore.bquad) {
		  mins = cg.tdmScore.bquadTime / 60;
		  secs = cg.tdmScore.bquadTime % 60;
		  CG_Text_Paint_Align(&rect, scale, color, va("%d:%02d", mins, secs), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_BLUE_TEAM_TIMEHELD_BS:
	  if (!cg.tdmScore.valid) {
		  break;
	  }
	  if (cg.tdmScore.bbs) {
		  mins = cg.tdmScore.bbsTime / 60;
		  secs = cg.tdmScore.bbsTime % 60;
		  CG_Text_Paint_Align(&rect, scale, color, va("%d:%02d", mins, secs), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "-", 0, 0, textStyle, font, align);
	  }
	  break;

  case CG_ARMORTIERED_COLORIZED:
	  CG_SetArmorColor();
	  CG_DrawPic(rect.x, rect.y, rect.w, rect.h, shader);
	  break;
  case CG_RED_TEAM_MAP_PICKUPS:  // 340
	  CG_DrawTeamMapPickups(&rect, scale, textStyle, font, TEAM_RED);
	  break;
  case CG_BLUE_TEAM_MAP_PICKUPS:
	  CG_DrawTeamMapPickups(&rect, scale, textStyle, font, TEAM_BLUE);
	  break;
  case CG_1ST_PLYR_PICKUPS:
	  CG_Draw1stPlayerPickups(&rect, scale, textStyle, font);
	  break;
  case CG_2ND_PLYR_PICKUPS:
	  CG_Draw2ndPlayerPickups(&rect, scale, textStyle, font);
	  break;
  case CG_MOST_VALUABLE_OFFENSIVE_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (cgs.realProtocol >= 91) {
		  int n;

		  n = atoi(CG_ConfigString(CS_MVP_OFFENSE));
		  if (n >= 0  &&  n <= MAX_CLIENTS) {
			  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
			  //FIXME steam avatar is shown
		  } else {
			  Com_Printf("^3CG_MOST_VALUABLE_OFFENSIVE_PLYR invalid player number %d\n", n);
		  }
	  } else if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MVP_OFFENSE), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_MOST_VALUABLE_DEFENSIVE_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (cgs.realProtocol >= 91) {
		  int n;

		  n = atoi(CG_ConfigString(CS_MVP_DEFENSE));
		  if (n >= 0  &&  n <= MAX_CLIENTS) {
			  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
			  //FIXME steam avatar is shown
		  } else {
			  Com_Printf("^3CG_MOST_VALUABLE_DEFENSIVE_PLYR invalid player number %d\n", n);
		  }
	  } else if (cgs.protocol == PROTOCOL_QL) {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MVP_DEFENSE), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_MOST_VALUABLE_PLYR:
	  // 2015-11-04 current menu files only have room for avatar
	  if (qtrue) {
		  break;
	  }

	  if (cgs.realProtocol >= 91) {
		  int n;

		  n = atoi(CG_ConfigString(CS_MVP));
		  if (n >= 0  &&  n <= MAX_CLIENTS) {
			  //CG_Text_Paint_Align(&rect, scale, color, cgs.clientinfo[n].name, 0, 0, textStyle, font, align);
			  //FIXME steam avatar is shown
		  } else {
			  Com_Printf("^3CG_MOST_VALUABLE_PLYR invalid player number %d\n", n);
		  }
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, CG_ConfigString(CS_MVP), 0, 0, textStyle, font, align);
	  }
	  break;
  case CG_RED_OWNED_FLAGS:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.dominationRedPoints), 0, 0, textStyle, font, align);
	  break;
  case CG_BLUE_OWNED_FLAGS:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.dominationBluePoints), 0, 0, textStyle, font, align);
	  break;
  case CG_ROUND: {
	  if (cg.warmup) {
		  CG_Text_Paint_Align(&rect, scale, color, "Warmup", 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, va("Round %d", cgs.roundNum), 0, 0, textStyle, font, align);
	  }
  }
	  break;
  case CG_TEAM_PLYR_COUNT: {
	  int ourTeam;

	  if (wolfcam_following) {
		  ourTeam = cgs.clientinfo[wcg.clientNum].team;
	  } else {
		  ourTeam = cg.snap->ps.persistant[PERS_TEAM];
	  }

	  if (ourTeam == TEAM_BLUE) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.bluePlayersLeft), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.redPlayersLeft), 0, 0, textStyle, font, align);
	  }
	  break;
  }
  case CG_ENEMY_PLYR_COUNT: {
	  int ourTeam;

	  if (wolfcam_following) {
		  ourTeam = cgs.clientinfo[wcg.clientNum].team;
	  } else {
		  ourTeam = cg.snap->ps.persistant[PERS_TEAM];
	  }

	  if (ourTeam == TEAM_BLUE) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.redPlayersLeft), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", cgs.bluePlayersLeft), 0, 0, textStyle, font, align);
	  }
	  break;
  }
  case UI_ADVERT:
	  // ql advertisment
	  break;

	  // UI_CROSSHAIR_COLOR 258  // only in config menu?
	  // UI_NEXTMAP 259
	  // UI_VOTESTRING 260
	  // UI_SERVER_SETTINGS 580

  case UI_KEYBINDSTATUS:  // 542
	  if (Display_KeyBindPending()) {
		  s = "Waiting for new key... Press ESCAPE to cancel";
	  } else {
		  s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
	  }
	  CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, textStyle, font, align);
	  break;

  // wolfcam ownerdraws

  case CG_RACE_STATUS: {
	  int nextCheckPointEnt;

	  if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
		  break;
	  }

	  if (wolfcam_following) {
		  //ourClientNum = wcg.clientNum;
		  nextCheckPointEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
	  } else {
		  // ourClientNum = cg.snap->ps.clientNum;
		  nextCheckPointEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
	  }

	  if (nextCheckPointEnt > 0) {
		  CG_Text_Paint_Align(&rect, scale, color, "^3CURRENT RUN", 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "^3LAST TIME", 0, 0, textStyle, font, align);
	  }

	  break;
  }

  case CG_RACE_TIMES: {
	  int nextCheckPointEnt;
	  int start;
	  int end;
	  const char *timeString;

	  if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
		  break;
	  }

	  if (wolfcam_following) {
		  //ourClientNum = wcg.clientNum;
		  nextCheckPointEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
		  start = cg_entities[wcg.clientNum].pe.raceStartTime;
		  end = cg_entities[wcg.clientNum].pe.raceEndTime;
	  } else {
		  // ourClientNum = cg.snap->ps.clientNum;
		  nextCheckPointEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
		  start = cg.predictedPlayerEntity.pe.raceStartTime;
		  end = cg.predictedPlayerEntity.pe.raceEndTime;
	  }

	  if (nextCheckPointEnt > 0) {
		  timeString = CG_GetTimeString(cg.time - start);
		  CG_Text_Paint_Align(&rect, scale, color, va("%s", timeString), 0, 0, textStyle, font, align);
	  } else {
		  timeString = CG_GetTimeString(end);
		  //Com_Printf("tstring %d  start %d  end %d\n", end - start, start, end);
		  CG_Text_Paint_Align(&rect, scale, color, va("%s", timeString), 0, 0, textStyle, font, align);
	  }

	  break;
  }

  case CG_OVERTIME: {
	  if (!cg.warmup  &&  cgs.numOverTimes  &&  !(cg.snap->ps.pm_type == PM_INTERMISSION)) {
		  CG_Text_Paint_Align(&rect, scale, color, "Overtime", 0, 0, textStyle, font, align);
	  }
	  break;
  }

  case CG_PLAYER_HASKEY: {
	  if (cgs.protocol != PROTOCOL_QL) {
		  break;
	  }

	  if (wolfcam_following  &&  wcg.clientNum != cg.snap->ps.clientNum) {
		  // pass
	  } else {
		  qhandle_t shader;
		  int spacing = 0;


		  //FIXME other keys besides gold, don't know what it will look like

		  if (cg.snap->ps.stats[STAT_MAP_KEYS] & 0x1) {
			  shader = cgs.media.silverKeyIcon;
			  CG_DrawPic(rect.x + spacing, rect.y, rect.w, rect.h, shader);
			  spacing += 10;
		  }

		  if (cg.snap->ps.stats[STAT_MAP_KEYS] & 0x2) {
			  shader = cgs.media.goldKeyIcon;
			  CG_DrawPic(rect.x + spacing, rect.y, rect.w, rect.h, shader);
			  spacing += 10;
		  }

		  if (cg.snap->ps.stats[STAT_MAP_KEYS] & 0x4) {
			  shader = cgs.media.masterKeyIcon;
			  CG_DrawPic(rect.x + spacing, rect.y, rect.w, rect.h, shader);
			  spacing += 10;
		  }
	  }

	  break;
  }

  case CG_1ST_PLYR_AVATAR:
	  //FIXME
	  break;

  case CG_2ND_PLYR_AVATAR:
	  //FIXME
	  break;

  case CG_MATCH_STATE:
	  //FIXME like CG_MATCH_STATUS?
	  //Com_Printf("^3FIXME CG_MATCH_STATE\n");
	  //CG_Text_Paint_Align(&rect, scale, color, "^2CG_MATCH_STATE", 0, 0, textStyle, font, align);
	  break;

  case WCG_GAME_STATUS:
	  if (cg.warmup) {
		  CG_Text_Paint_Align(&rect, scale, color, "Warmup", 0, 0, textStyle, font, align);
	  } else if (cg.snap  &&  cg.snap->ps.pm_type == PM_INTERMISSION) {
		  CG_Text_Paint_Align(&rect, scale, color, "Intermission", 0, 0, textStyle, font, align);
	  } else if (cg.snap) {
		  // playing
		  CG_Text_Paint_Align(&rect, scale, color, "Game Active", 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, "Not Connected", 0, 0, textStyle, font, align);
	  }
	  break;

  case WCG_WEAPON_SELECTED:  //FIXME duplicate code
	  if (wolfcam_following) {
		   ival = cg_entities[wcg.clientNum].currentState.weapon;
	  } else {
		  if (!cg.demoPlayback) {
			  ival = cg.weaponSelect;
		  } else {
			  ival = cg.snap->ps.weapon;
		  }
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("%d", ival), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_SELECT_TIME:
	  if (wolfcam_following) {
		  ival = wcg.weaponSelectTime;
	  } else {
		  ival = cg.weaponSelectTime;
	  }

	  CG_Text_Paint_Align(&rect, scale, color, va("%d", ival), 0, 0, textStyle, font, align);
	  break;

  case WCG_NUMBER_OF_HELD_WEAPONS:
	  ival = CG_NumHeldWeapons();
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", ival), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_GAUNTLET:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_GAUNTLET)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_MACHINEGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_MACHINEGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_SHOTGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_SHOTGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_GRENADE_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_GRENADE_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_ROCKET_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_ROCKET_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_LIGHTNING:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_LIGHTNING)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_RAILGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_RAILGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_PLASMAGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_PLASMAGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_BFG)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_GRAPPLING_HOOK:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_GRAPPLING_HOOK)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_NAILGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_NAILGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_PROX_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_PROX_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_HAVE_CHAINGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_HaveWeapon(WP_CHAINGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_GAUTNLET:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_GAUNTLET)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_MACHINEGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_MACHINEGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_SHOTGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_SHOTGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_GRENADE_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_GRENADE_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_ROCKET_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_ROCKET_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_LIGHTNING:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_LIGHTNING)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_RAILGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_RAILGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_PLASMAGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_PLASMAGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_BFG:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_BFG)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_GRAPPLING_HOOK:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_GRAPPLING_HOOK)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_NAILGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_NAILGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_PROX_LAUNCHER:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_PROX_LAUNCHER)), 0, 0, textStyle, font, align);
	  break;

  case WCG_WEAPON_AMMO_CHAINGUN:
	  CG_Text_Paint_Align(&rect, scale, color, va("%d", CG_WeaponAmmo(WP_CHAINGUN)), 0, 0, textStyle, font, align);
	  break;

  case WCG_KILL_COUNT: {
	  if (wolfcam_following) {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", wclients[wcg.clientNum].killCount), 0, 0, textStyle, font, align);
	  } else {
		  CG_Text_Paint_Align(&rect, scale, color, va("%d", wclients[cg.snap->ps.clientNum].killCount), 0, 0, textStyle, font, align);
	  }
  }

  default:
	  if (debug > 0) {
		  //Com_Printf("CG_OwnerDraw() unknown ownerDraw %d\n", ownerDraw);
		  //CG_Text_Paint(rect.x, rect.y + rect.h, scale, color, va("xxx %d  xxx", ownerDraw), 0, 0, textStyle, &cgDC.Assets.textFont);
		  CG_Text_Paint_Align(&rect, scale, color, va("^3xxx %d  xxx", ownerDraw), 0, 0, textStyle, &cgDC.Assets.textFont, align);
		  Com_Printf("FIXME CG_OwnerDraw()  %d\n", ownerDraw);
	  }
    break;
  }

  MenuWidescreen = 0;
  QLWideScreen = 0;
}

void CG_MouseEvent(int x, int y) {
	int n;

    switch (cgs.eventHandling) {
    case CGAME_EVENT_DEMO:
        //FIXME wolfcam
		cg.mousex += x;
		cg.mousey += y;
		//Com_Printf("mouse event demo\n");
        //return;
		break;
    default:
		//Com_Printf("mouse default\n");
        CG_EventHandling(CGAME_EVENT_NONE);
        trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
        break;
    }

	if ( (cg.predictedPlayerState.pm_type == PM_NORMAL || cg.predictedPlayerState.pm_type == PM_SPECTATOR) && cg.showScores == qfalse) {
		if (!cg.demoPlayback) {
			trap_Key_SetCatcher(0);
			return;
		}
	}

	if (cg.mouseSeeking) {
		double scale;

		if (cg.realTime - cg.lastMouseSeekTime < cg_mouseSeekPollInterval.integer) {
			return;
		}

		if (x > 0  &&  cg.mousex < 0) {
			cg.mousex = 0;
		} else if (x < 0  &&  cg.mousex > 0) {
			cg.mousex = 0;
		}

		scale = 0.0002;
		scale *= cg_mouseSeekScale.value;
		if (cg_mouseSeekUseTimescale.integer == 1) {
			scale *= cg_timescale.value;
		} else if (cg_mouseSeekUseTimescale.integer == 2  &&  cg_timescale.value < 1.0) {
			scale *= cg_timescale.value;
		}

		if ((cg.mousex > 0  &&  scale > 0.0)  ||  (cg.mousex < 0  &&  scale < 0.0)) {
			trap_SendConsoleCommand(va("fastforward %f\n", (double)cg.mousex * scale));
			//Com_Printf("ff %f\n", (double)cg.mousex * scale);
		} else if ((cg.mousex < 0  &&  scale > 0.0)  ||  (cg.mousex > 0  &&  scale < 0.0)) {
			cg.mousex = -cg.mousex;
			trap_SendConsoleCommand(va("rewind %f\n", (double)cg.mousex * scale));
		}
		cg.mousex = 0;
		cg.mousey = 0;
		cg.lastMouseSeekTime = cg.realTime;
		return;
	}

	if (cg.freecam) {
		return;
	}

	//FIXME hack
	if (!cg.scoreBoardShowing  &&  !cg.testMenu) {
		return;
	}

	cgs.cursorX += x;
	if (cgs.cursorX < 0)
		cgs.cursorX = 0;
	else if (cgs.cursorX > 640)
		cgs.cursorX = 640;

	cgs.cursorY += y;
	if (cgs.cursorY < 0)
		cgs.cursorY = 0;
	else if (cgs.cursorY > 480)
		cgs.cursorY = 480;

	n = Display_CursorType(cgs.cursorX, cgs.cursorY);
	cgs.activeCursor = 0;
	if (n == CURSOR_ARROW) {
		cgs.activeCursor = cgs.media.selectCursor;
	} else if (n == CURSOR_SIZER) {
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	//cgs.activeCursor = cgs.media.selectCursor;

  if (cgs.capturedItem) {
	  Display_MouseMove(cgs.capturedItem, x, y);
  } else {
	  Display_MouseMove(NULL, cgs.cursorX, cgs.cursorY);
  }

  //Com_Printf("x %d   y %d\n", cgs.cursorX, cgs.cursorY);
}

/*
==================
CG_HideTeamMenus
==================

*/
static void CG_HideTeamMenu( void ) {
  Menus_CloseByName("teamMenu");
  Menus_CloseByName("getMenu");
}

#if 0  // unused

/*
==================
CG_ShowTeamMenus
==================

*/
static void CG_ShowTeamMenu( void ) {
  Menus_OpenByName("teamMenu");
}

#endif

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int type) {
    CG_Printf ("^3CG_EventHandling type: %d\n", type);

	cgs.eventHandling = type;

	if (cg.demoPlayback  &&  type == CGAME_EVENT_NONE) {
		type = CGAME_EVENT_DEMO;
	}

	if (type == CGAME_EVENT_NONE) {
		if (cg.demoPlayback) {
			//type = CGAME_EVENT_DEMO;
		}
		CG_HideTeamMenu();
	} else if (type == CGAME_EVENT_TEAMMENU) {
		//CG_ShowTeamMenu();
	} else if (type == CGAME_EVENT_SCOREBOARD) {
		//Com_Printf("event scoreboard\n");
	}

	cgs.eventHandling = type;
	if (type == CGAME_EVENT_NONE) {
		trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
	} else {
		Com_Printf("CG_EventHandling type: %d  not handled\n", type);
	}

}

void CG_KeyEvent (int key, qboolean down)
{
    //CG_Printf ("CG_KeyEvent: %d %d %d\n", key, down, cgs.eventHandling);

    switch (cgs.eventHandling) {
    case CGAME_EVENT_DEMO:
        Wolfcam_DemoKeyClick (key, down);
		//return;  //FIXME wolfcam fucks up scoreboard (constant select sound)
		break;
    default:
        //CG_EventHandling(CGAME_EVENT_NONE);
        //trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
        break;
    }

	if (!cg.demoPlayback) {
		switch (key) {
		case K_F1:
		case K_F2:
		case K_F3:
		case K_F4:
		case K_F5:
		case K_F6:
		case K_F7:
		case K_F8:
		case K_F9:
		case K_F10:
		case K_F11:
		case K_F12:
		case K_F13:
		case K_F14:
		case K_F15:
			Wolfcam_DemoKeyClick(key, down);
			return;
		default:
			break;
		}
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL || (cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse)) {
		if (!cg.demoPlayback) {
			CG_EventHandling(CGAME_EVENT_NONE);
			trap_Key_SetCatcher(0);
			return;
		}
	}

  //if (key == trap_Key_GetKey("teamMenu") || !Display_CaptureItem(cgs.cursorX, cgs.cursorY)) {
    // if we see this then we should always be visible
  //  CG_EventHandling(CGAME_EVENT_NONE);
  //  trap_Key_SetCatcher(0);
  //}

	//FIXME hack
	if (!cg.scoreBoardShowing  &&  !cg.testMenu) {
		return;
	}

	//FIXME and more hacks
	if (key != K_MOUSE1  &&  key != K_MOUSE2) {
		return;
	}


	Display_HandleKey(key, down, cgs.cursorX, cgs.cursorY);
	//Com_Printf("key: 0x%x  down: %d  K_MOUSE2 == 0x%x\n", key, down, K_MOUSE2);

	if (cgs.capturedItem) {
		cgs.capturedItem = NULL;
	} else {
		if (key == K_MOUSE2 && down) {
			cgs.capturedItem = Display_CaptureItem(cgs.cursorX, cgs.cursorY);
		}
	}
}

#if 0  // unused

static int CG_ClientNumFromName(const char *p) {
  int i;
  for (i = 0; i < cgs.maxclients; i++) {
    if (cgs.clientinfo[i].infoValid && Q_stricmp(cgs.clientinfo[i].name, p) == 0) {
      return i;
    }
  }
  return -1;
}

#endif

void CG_ShowResponseHead(void) {
  Menus_OpenByName("voiceMenu");
	trap_Cvar_Set("cl_conXOffset", "72");
	cg.voiceTime = cg.time;
}

void CG_RunMenuScript(char **args) {
}


void CG_GetTeamColor(vec4_t *color) {
	int team;

	//Com_Printf("^2  :: getteamcolor\n");

	if (wolfcam_following) {
		team = cgs.clientinfo[wcg.clientNum].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

  if (team == TEAM_RED) {
#if 0
    (*color)[0] = 1.0f;
    (*color)[3] = 0.25f;
    (*color)[1] = (*color)[2] = 0.0f;
#endif
	SC_Vec3ColorFromCvar(*color, &cg_hudRedTeamColor);
  } else if (team == TEAM_BLUE) {
#if 0
    (*color)[0] = (*color)[1] = 0.0f;
    (*color)[2] = 1.0f;
    (*color)[3] = 0.25f;
#endif
	SC_Vec3ColorFromCvar(*color, &cg_hudBlueTeamColor);
  } else {
#if 0
    (*color)[0] = (*color)[2] = 0.0f;
    (*color)[1] = 0.17f;
    (*color)[3] = 0.25f;
#endif
	SC_Vec3ColorFromCvar(*color, &cg_hudNoTeamColor);
  }

  (*color)[3] = 1.0;
  //Com_Printf("CG_GetTeamColor()  %f %f %f %f\n", *color[0], *color[1], *color[2], *color[3]);
}

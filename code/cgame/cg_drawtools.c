// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"
#include "../qcommon/q_shared.h"
#include "cg_drawtools.h"
#include "cg_draw.h"
#include "cg_newdraw.h"
#include "cg_players.h"  // color from string
#include "cg_syscalls.h"

// unused
/*
static void debug_rect (float x, float y, float width, float height, qboolean adjust, float r, float g, float b, float alpha)
{
	vec4_t color;

	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = alpha;

	trap_R_SetColor(color);

	if (adjust) {
		CG_AdjustFrom640(&x, &y, &width, &height);
	}
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}
*/

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float aspect;
	qboolean square = qfalse;

	float origWidth = *w;
	float origX = *x;
	
	if (*w == *h) {
		square = qtrue;
	}

#if 0
	// adjust for wide screens
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		*x += 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * 640 / 480 ) );
	}
#endif

	// ql widescreen
	if (cg_wideScreen.integer == 5) {
		if (MenuWidescreen != QLWideScreen  &&  QLWideScreen != 0) {
			//Com_Printf("^3m %d  i %d\n", MenuWidescreen, QLWideScreen);
		}

		if (1) {  //(MenuWidescreen  ||  QLWideScreen) {
			//FIXME duplicate code
			float width43;
			float diff;
			float newXScale;
			rectDef_t menuRect;
			float newX = 0;
			
			menuRect = MenuRect;
			menuRect.x *= cgs.screenXScale;
			menuRect.y *= cgs.screenYScale;
			menuRect.w *= cgs.screenXScale;
			menuRect.h *= cgs.screenYScale;

			width43 = 4.0 * (cgs.glconfig.vidHeight / 3.0);
			diff = (float)cgs.glconfig.vidWidth - width43;

			newXScale = width43 / 640.0;


			//*x *= newXScale;



						//FIXME
			if (QLWideScreen == 0) {
				//FIXME

				if (MenuWidescreen) {
					Com_Printf("^3FIXME ql QLWideScreen 0  menu %d\n", MenuWidescreen);
				}
				//debug_rect(menuRect.x, menuRect.y, menuRect.w, menuRect.h, qfalse, 1, 0, 0, 0.1);
				*x *= cgs.screenXScale;
				*y *= cgs.screenYScale;
				*w *= cgs.screenXScale;
				*h *= cgs.screenYScale;

			} else if (QLWideScreen == 1) {
				//debug_rect(menuRect.x, menuRect.y, menuRect.w, menuRect.h, qfalse, 0, 0, 1, 0.1);
				//*x *= cgs.screenXScale;
				//*x = menuRect.x;

				//*x = 0;  //FIXME testing
				
				*y *= cgs.screenYScale;
				*w *= newXScale;
				*h *= cgs.screenYScale;


				*x *= newXScale;
			} else if (QLWideScreen == 2) {
				rectDef_t itemRect;

				//debug_rect(menuRect.x, menuRect.y, menuRect.w, menuRect.h, qfalse, 0, 1, 0, 0.1);

				*y *= cgs.screenYScale;
				*w *= newXScale;
				*h *= cgs.screenYScale;


				itemRect.x = *x - MenuRect.x;
				//FIXME
				//*x = 0;
				//*x = 40;

				//*x = (menuRect.x + (menuRect.w / 2) - (*w / 2));
				
				//*x += itemRect.x * cgs.screenXScale / 2;
				//*x += itemRect.x * newXScale / 2;
				//*x += itemRect.x;
				//*x = 0;
				//*x = menuRect.x;
				//*y += 30;
				//*x *= cgs.screenXScale;

				//diff = (origWidth * cgs.screenXScale) - (origWidth * newXScale);
				////*x *= cgs.screenXScale;

				/* this is it */
 				*x *= newXScale;
				*x += diff / 2;
				
				//*x += diff / 2;
				//*x += diff;

				if (origX <= 320) {
					//*x += diff;
					
				} else {
					//*x -= diff;
				}
				
			} else if (QLWideScreen == 3) {
				//debug_rect(menuRect.x, menuRect.y, menuRect.w, menuRect.h, qfalse, 1, 0.5, 0.8, 0.1);

				*y *= cgs.screenYScale;
				*w *= newXScale;
				*h *= cgs.screenYScale;


				//*x += (diff / 2.0);
				//*x = 6666;
				//*x = (menuRect.x + menuRect.w) - *w;

				//*x = 0;  //FIXME testing
				
				*x *= newXScale;
				*x += diff;

			} else {
				*x *= cgs.screenXScale;
				*y *= cgs.screenYScale;
				*w *= cgs.screenXScale;
				*h *= cgs.screenYScale;

				Com_Printf("^3invalid widescreen value: %d\n", QLWideScreen);
			}


		} else {
			*x *= cgs.screenXScale;
			*y *= cgs.screenYScale;
			*w *= cgs.screenXScale;
			*h *= cgs.screenYScale;
		}

		return;
	}

#if 0
	if (cg.scoreBoardShowing) {
		*x *= cgs.screenXScale;
		*y *= cgs.screenYScale;
		*w *= cgs.screenXScale;
		*h *= cgs.screenYScale;
		return;
	}
#endif

	// scale for screen sizes

	if (cg.scoreBoardShowing) {
		//Com_Printf("scoreboard\n");
		if (cg_wideScreenScoreBoardHack.integer == 1) {
			// don't stretch and center on screen
			float width43;
			float diff;
			float newXScale;

			width43 = 4.0 * (cgs.glconfig.vidHeight / 3.0);
			diff = (float)cgs.glconfig.vidWidth - width43;

			newXScale = width43 / 640.0;

			//*x += (diff / 2.0);

			*x *= newXScale;
			*y *= cgs.screenYScale;
			*w *= newXScale;
			*h *= cgs.screenYScale;

			*x += (diff / 2.0);

			return;

		} else if (cg_wideScreenScoreBoardHack.integer == 2) {
			// just 4:3
			*x *= cgs.screenXScale;
			*y *= cgs.screenYScale;
			*w *= cgs.screenXScale;
			*h *= cgs.screenYScale;
			return;
		}
	}

	if (cg_wideScreen.integer == 0  ||  cg_wideScreen.integer == 3  ||  cg_wideScreen.integer == 4) {
		*x *= cgs.screenXScale;
		*y *= cgs.screenYScale;
		*w *= cgs.screenXScale;
		*h *= cgs.screenYScale;

		if (cg_wideScreen.integer == 4) {
			aspect = (float)cgs.glconfig.vidHeight / (float)cgs.glconfig.vidWidth;
			*w -= ((480.0 / 640.0) - aspect) * *w;
			if (square) {
				//Com_Printf("w %f  h %f\n", *w, *h);
			}
		}
	} else if (cg_wideScreen.integer == 1) {
		// don't adjust values
	} else if (cg_wideScreen.integer == 2) {
		*x *= cgs.screenXScale;
		*y *= cgs.screenYScale;
		*w *= cgs.screenXScale;
		*h *= cgs.screenXScale;
	}
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	//CG_Printf("fill rect %f %f %f %f\n", color[0], color[1], color[2], color[3]);

	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  CG_DrawTopBottom(x, y, width, height, size);
  CG_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}



/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void CG_DrawStretchPic (float x, float y, float width, float height, float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, hShader );
}


/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
static void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	trap_R_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cgs.media.charsetShader );
}



/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
					   qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars, const fontInfo_t *font ) {
	vec4_t		color;
	const char	*s;
	int			xx;
	int			cnt;

	//FIXME test
	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = 1.0;

	if (cg_testQlFont.integer) {
		float scale;
		float pt;
		const fontInfo_t *subFont;

		//CG_Text_Paint_Ext(x, y, 0.25, 0.25, color, string, 0, 0, 0, &cgs.media.qlfont24);
		//CG_Text_Paint(x, y, 0.25, 0.25, (float *)color, string, 0, 0, 0, &cgs.media.qlfont24);
		// x, y, scale, color, string, float adjust, limit, style, fontinfo
		//CG_Text_Paint(x, y + (24.0f * 0.25f) + 2, 0.25, color, string, 0, 0, 0, &cgs.media.qlfont24);
		// giant 32, big 16, small 8, tiny 8
		scale = 0.25;  // 0.25
		pt = 16.0;  //24.0;
		subFont = &cgs.media.qlfont16;  //&cgs.media.qlfont24;
		if (charWidth != charHeight) {
			//Com_Printf("FIXME charWidth != charHeight\n");
		}
		//CG_Text_Paint(x, y + (24.0f * 0.25f) + 2, scale, color, string, 0, 0, 0, &cgs.media.qlfont24);
		if (font == &cgs.media.tinychar) {
			scale *= 0.5;
		} else if (font == &cgs.media.bigchar) {
			//scale *= 2.0;
		} else if (font == &cgs.media.giantchar) {
			scale *= 2.0;
		}

		CG_Text_Paint(x, y + (pt * scale) + 2, scale, color, string, 0, 0, 0, subFont);
		return;
	}

	if (font != &cgs.media.bigchar  &&  font != &cgs.media.smallchar  &&  font != &cgs.media.tinychar  &&  font != &cgs.media.giantchar) {
		//FIXME
		CG_Text_Paint(x, y + (24.0f * 0.25f) + 2, 0.25, color, string, 0, 0, 0, &cgs.media.qlfont24);
		return;
	}

	if (maxChars <= 0)
		maxChars = 32767; // do them all!

	// draw the drop shadow
	if (shadow) {
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		trap_R_SetColor( color );
		s = string;
		xx = x;
		cnt = 0;
		while ( *s && cnt < maxChars) {
			if ( Q_IsColorString( s ) ) {
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
					} else {
						s += 2;
					}
				} else {
					s += 2;
				}
				continue;
			}
			CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
			cnt++;
			xx += charWidth;
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	trap_R_SetColor( setColor );
	while ( *s && cnt < maxChars) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				if (cgs.cpma) {
					CG_CpmaColorFromString(s + 1, color);
				} else if (cgs.osp) {
					CG_OspColorFromString(s + 1, color);
				} else {
					memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				}
				//memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			if (cgs.osp) {
				if (s[1] == 'x'  ||  s[1] == 'X') {
					s += 8;
				} else {
					s += 2;
				}
			} else {
				s += 2;
			}
			continue;
		}
		CG_DrawChar( xx, y, charWidth, charHeight, *s );
		xx += charWidth;
		cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, &cgs.media.bigchar );
}

void CG_DrawBigStringColor( int x, int y, const char *s, const vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, &cgs.media.bigchar );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.smallchar );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, const vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, &cgs.media.smallchar );
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str, const fontInfo_t *font ) {
	const char *s = str;
	int count = 0;
	int w;

	if (cg_testQlFont.integer) {
		//CG_Text_Width(const char *text, float scale, int limit, const fontInfo_t *font)
		return CG_Text_Width(str, 0.25f, 0, &cgs.media.qlfont24) / BIGCHAR_WIDTH;
	}

	if (font != &cgs.media.bigchar  &&  font != &cgs.media.smallchar  &&  font != &cgs.media.tinychar  &&  font != &cgs.media.giantchar) {
		//FIXME  scale, limit,
		return CG_Text_Width(str, 0.25f, 0, font);
	}

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
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
			count++;
			s++;
		}
	}

	if (font == &cgs.media.giantchar) {
		w = GIANTCHAR_WIDTH;
	} else if (font == &cgs.media.bigchar) {
		w = BIGCHAR_WIDTH;
	} else if (font == &cgs.media.smallchar) {
		w = SMALLCHAR_WIDTH;
	} else if (font == &cgs.media.tinychar) {
		w = TINYCHAR_WIDTH;
	} else {
		w = 1;
	}

	return count * w;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 && 
		cg.refdef.width == w && cg.refdef.height == h ) {
		return;		// full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}

float *CG_FadeColorRealTime (int startMsec, int totalMsec)
{
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.realTime - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}

void CG_FadeColorVec4 (vec4_t color,  int startMsec, int totalMsec, int fadeTimeMsec)
{
	int			t;

	if ( startMsec == 0 ) {
		//return NULL;
		Vector4Set(color, 0, 0, 0, 0);
		return;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		//return NULL;
		Vector4Set(color, 0, 0, 0, 0);
		return;
	}

	// fade out
	if ( totalMsec - t < fadeTimeMsec ) {
		color[3] = ( totalMsec - t ) * 1.0/fadeTimeMsec;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;
}


/*
================
CG_TeamColor
================
*/
float *CG_TeamColor( int team ) {
	static vec4_t	red = {1, 0.2f, 0.2f, 1};
	static vec4_t	blue = {0.2f, 0.2f, 1, 1};
	static vec4_t	other = {1, 1, 1, 1};
	static vec4_t	spectator = {0.7f, 0.7f, 0.7f, 1};

	switch ( team ) {
	case TEAM_RED:
		return red;
	case TEAM_BLUE:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	default:
		return other;
	}
}



/*
=================
CG_GetColorForHealth
=================
*/
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor ) {
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear( hcolor );	// black
		hcolor[3] = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count ) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) / 33.0;
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) / 30.0;
	}
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForHealth( vec4_t hcolor ) {

	CG_GetColorForHealth( cg.snap->ps.stats[STAT_HEALTH], 
		cg.snap->ps.stats[STAT_ARMOR], hcolor );
}




// bk001205 - code below duplicated in q3_ui/ui-atoms.c
// bk001205 - FIXME: does this belong in ui_shared.c?
// bk001205 - FIXME: HARD_LINKED flags not visible here
#ifndef Q3_STATIC // bk001205 - q_shared defines not visible here 
/*
=================
UI_DrawProportionalString2
=================
*/
static int	propMap[128][3] = {
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, PROP_SPACE_WIDTH},		// SPACE
{11, 122, 7},	// !
{154, 181, 14},	// "
{55, 122, 17},	// #
{79, 122, 18},	// $
{101, 122, 23},	// %
{153, 122, 18},	// &
{9, 93, 7},		// '
{207, 122, 8},	// (
{230, 122, 9},	// )
{177, 122, 18},	// *
{30, 152, 18},	// +
{85, 181, 7},	// ,
{34, 93, 11},	// -
{110, 181, 6},	// .
{130, 152, 14},	// /

{22, 64, 17},	// 0
{41, 64, 12},	// 1
{58, 64, 17},	// 2
{78, 64, 18},	// 3
{98, 64, 19},	// 4
{120, 64, 18},	// 5
{141, 64, 18},	// 6
{204, 64, 16},	// 7
{162, 64, 17},	// 8
{182, 64, 18},	// 9
{59, 181, 7},	// :
{35,181, 7},	// ;
{203, 152, 14},	// <
{56, 93, 14},	// =
{228, 152, 14},	// >
{177, 181, 18},	// ?

{28, 122, 22},	// @
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{60, 152, 7},	// [
{106, 151, 13},	// '\'
{83, 152, 7},	// ]
{128, 122, 17},	// ^
{4, 152, 21},	// _

{134, 181, 5},	// '
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{153, 152, 13},	// {
{11, 181, 5},	// |
{180, 152, 13},	// }
{79, 93, 17},	// ~
{0, 0, -1}		// DEL
};

#if 0  // unused
static int propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36
#endif

#if 0  // unused
/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, const vec4_t color, qboolean forceColor )
{
	const char* s;
	unsigned char	ch; // bk001204 : array subscript
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );

	//ax = x * cgs.screenXScale;
	//ay = y * cgs.screenYScale;
	ax = x;
	ay = y;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			//ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* cgs.screenXScale;
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH);
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;

			/*
			aw = (float)propMapB[ch][2] * cgs.screenXScale;
			ah = (float)PROPB_HEIGHT * cgs.screenYScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetPropB );

			ax += (aw + (float)PROPB_GAP_WIDTH * cgs.screenXScale);
			*/
			aw = (float)propMapB[ch][2];
			ah = (float)PROPB_HEIGHT;
			//trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetPropB );
			CG_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, cgs.media.charsetPropB);

			ax += aw + (float)PROPB_GAP_WIDTH;

		}
		s++;
	}

	trap_R_SetColor( NULL );
}

#endif


#if 0  // unused
static void UI_DrawBannerString( int x, int y, const char* str, int style, const vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor, qtrue );
	}

	UI_DrawBannerString2( x, y, str, color, qfalse );
}

#endif

static int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;

	s = str;
	width = 0;
	while ( *s ) {
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
			continue;
		}
		ch = *s & 127;
		charWidth = propMap[ch][2];
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	return width;
}

static void UI_DrawProportionalString2( int x, int y, const char* str, const vec4_t color, qboolean forceColor, float sizeScale, qhandle_t charset )
{
	const char* s;
	unsigned char	ch; // bk001204 - unsigned
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	vec4_t newColor;

	// draw the colored text
	trap_R_SetColor( color );

	/*
	ax = x * cgs.screenXScale;
	ay = y * cgs.screenYScale;
	*/
	ax = x;
	ay = y;
	
	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if (Q_IsColorString(s)) {
			if (forceColor) {
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
					} else {
						s += 2;
					}
				} else {
					s += 2;
				}
				continue;
			}
				if (cgs.cpma) {
					CG_CpmaColorFromString(s + 1, newColor);
				} else if (cgs.osp) {
					CG_OspColorFromString(s + 1, newColor);
				} else {
					memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				}
				//memcpy(newColor, g_color_table[ColorIndex(*(s+1))], sizeof(newColor));
			newColor[3] = color[3];
			if (s[1] == '7') {
				VectorCopy(color, newColor);
			}
			trap_R_SetColor(newColor);
			if (cgs.osp) {
				if (s[1] == 'x'  ||  s[1] == 'X') {
					s += 8;
				} else {
					s += 2;
				}
			} else {
				s += 2;
			}
			continue;
		} else if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * cgs.screenXScale * sizeScale;
		} else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			/*
			aw = (float)propMap[ch][2] * cgs.screenXScale * sizeScale;
			ah = (float)PROP_HEIGHT * cgs.screenYScale * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
			*/
			aw = (float)propMap[ch][2] * sizeScale;
			ah = (float)PROP_HEIGHT * sizeScale;
			CG_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
		} else {
			aw = 0;
		}

		//ax += (aw + (float)PROP_GAP_WIDTH * cgs.screenXScale * sizeScale);
		ax += (aw + (float)PROP_GAP_WIDTH * sizeScale);
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
static float UI_ProportionalSizeScale( int style ) {
	if(  style & UI_SMALLFONT ) {
		return 0.75;
	}

	return 1.00;
}


/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, const vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, qtrue, sizeScale, cgs.media.charsetProp );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, qtrue, sizeScale, cgs.media.charsetProp );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, color, qtrue, sizeScale, cgs.media.charsetProp );

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin( cg.time / PULSE_DIVISOR );
		UI_DrawProportionalString2( x, y, str, drawcolor, qtrue, sizeScale, cgs.media.charsetPropGlow );
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, qfalse, sizeScale, cgs.media.charsetProp );
}
#endif // Q3STATIC

//FIXME hack for info screen
int UI_DrawProportionalString3 (int x, int y, const char* str, int style, const vec4_t color)
{
	char buffer[MAX_STRING_CHARS];
	int count;
	int realCount;
	const char *p;
	char *b;
	int lines;

	p = str;
	b = buffer;
	count = 0;
	realCount = 0;
	lines = 1;
	while (1) {
		*b = *p;

		if (*p == '^') {
			if (*(p + 1) == '\0') {
				*b = '\0';
				break;
			}
			if (realCount >= MAX_STRING_CHARS) {
				*b = '\0';
				break;
			}
			b++;
			p++;
			*b = *p;
			b++;
			p++;

			realCount += 2;
			continue;
		}

		count++;
		realCount++;

		if (realCount >= (MAX_STRING_CHARS - 1)) {
			*b = '\0';
			break;
		}

		if (*p == '\0') {
			break;
		}

		if (count >= 40) {
			*(b + 1) = '\0';
			UI_DrawProportionalString (x, y, buffer, style, color);
			y += PROP_HEIGHT;
			b = buffer;
			p++;
			count = 0;
			realCount = 0;
			lines++;
			continue;
		}

		b++;
		p++;
	}

	//UI_DrawProportionalString (x, y, str, style, color);
	UI_DrawProportionalString(x, y, buffer, style, color);

	return lines;
}

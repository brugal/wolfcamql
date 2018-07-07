#include "cg_draw.h"
#include "cg_drawdc.h"
#include "cg_drawtools.h"
#include "cg_local.h"  // cgDC
#include "cg_newdraw.h"
#include "cg_syscalls.h"

#define wsset() QLWideScreen = widescreen; MenuWidescreen = widescreen; MenuRect = menuRect
#define wsoff() QLWideScreen = WIDESCREEN_NONE;  MenuWidescreen = WIDESCREEN_NONE

void CG_DrawHandlePicDc (float x, float y, float w, float h, qhandle_t asset, int widescreen, rectDef_t menuRect)
{
    wsset();
	CG_DrawPic(x, y, w, h, asset);
    wsoff();
}

void CG_DrawStretchPicDc (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, int widescreen, rectDef_t menuRect)
{
    wsset();
	CG_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
    wsoff();
}

void CG_DrawTextDc (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int fontIndex, int widescreen, rectDef_t menuRect)
{
    wsset();
    //FIXME widescreen
    CG_Text_Paint_old(x, y, scale, color, text, adjust, limit, style, fontIndex);
    wsoff();
}

float CG_TextWidthDc (const char *text, float scale, int limit, int fontIndex, int widescreen, rectDef_t menuRect)
{
    float r;

    wsset();
    //FIXME widescreen
    r = CG_Text_Width_old(text, scale, limit, fontIndex);
    wsoff();

    return r;
}

float CG_TextHeightDc (const char *text, float scale, int limit, int fontIndex, int widescreen, rectDef_t menuRect)
{
    float r;

    wsset();
    //FIXME widescreen
    r = CG_Text_Height_old(text, scale, limit, fontIndex);
    wsoff();

    return r;
}

void CG_FillRectDc (float x, float y, float w, float h, const vec4_t color, int widescreen, rectDef_t menuRect)
{
    wsset();
    CG_FillRect(x, y, w, h, color);
    wsoff();
}

void CG_DrawRectDc (float x, float y, float w, float h, float size, const vec4_t color, int widescreen, rectDef_t menuRect)
{
    wsset();
    CG_DrawRect(x, y, w, h, size, color);
    wsoff();
}

void CG_DrawSidesDc (float x, float y, float w, float h, float size, int widescreen, rectDef_t menuRect)
{
    wsset();
    CG_DrawSides(x, y, w, h, size);
    wsoff();
}

void CG_DrawTopBottomDc (float x, float y, float w, float h, float size, int widescreen, rectDef_t menuRect)
{
    wsset();
    CG_DrawTopBottom(x, y, w, h, size);
    wsoff();
}

static void CG_Text_PaintWithCursor (float x, float y, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int fontIndex)
{
	 const fontInfo_t *font;

	 if (fontIndex <= 0) {
		 font = &cgDC.Assets.textFont;
	 } else {
		 font = &cgDC.Assets.extraFonts[fontIndex];
	 }

	 CG_Text_Paint(x, y, scale, color, text, 0, limit, style, font);

	 Com_Printf("FIXME cursor (%s)\n", text);
	 //FIXME cursor
}

void CG_DrawTextWithCursorDc (float x, float y, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int fontIndex, int widescreen, rectDef_t menuRect)
{
    wsset();
    //FIXME widescreen
    CG_Text_PaintWithCursor(x, y, scale, color, text, cursorPos, cursor, limit, style, fontIndex);
    wsoff();
}

static float CG_OwnerDrawWidth (int ownerDraw, float scale, int fontIndex)
{
	 const fontInfo_t *font;
	 const char *s;

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
	case UI_KEYBINDSTATUS:
		if (Display_KeyBindPending()) {
			s = "Waiting for new key... Press ESCAPE to cancel";
		} else {
			s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
		}
		return CG_Text_Width(s, scale, 0, font);
		break;
	default:
		Com_Printf("CG_OwnerDrawWidth() unknown ownerDraw %d\n", ownerDraw);
		break;
	}
	return 0;
}

//FIXME is it even used?
float CG_OwnerDrawWidthDc (int ownerDraw, float scale, int fontIndex, int widescreen, rectDef_t menuRect)
{
    float r;

    wsset();
    //FIXME widescreen
    r = CG_OwnerDrawWidth(ownerDraw, scale, fontIndex);
    wsoff();

    return r;
}

int CG_PlayCinematicDc (const char *name, float x, float y, float w, float h, int widescreen, rectDef_t menuRect)
{
    //FIXME widescreen
    return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

void CG_StopCinematicDc (int handle)
{
    trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic (int handle, float x, float y, float w, float h)
{
    trap_CIN_SetExtents(handle, x, y, w, h);
    trap_CIN_DrawCinematic(handle);
}

void CG_DrawCinematicDc (int handle, float x, float y, float w, float h, int widescreen, rectDef_t menuRect)
{
    wsset();
    //FIXME widescreen
    CG_DrawCinematic(handle, x, y, w, h);
    wsoff();
}

void CG_RunCinematicFrameDc (int handle)
{
    trap_CIN_RunCinematic(handle);
}

void CG_PauseDc (qboolean pause)
{
	trap_Cvar_Set("cl_freezeDemo", pause ? "1" : "0");
}

void CG_ExecuteTextDc (int exec_when, const char *text)
{
	Com_Printf("^3FIXME ui execute text:  %d  '%s'\n", exec_when, text);
	//trap_Cmd_ExecuteText(exec_when, text);
}

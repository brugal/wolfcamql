#ifndef cg_newdraw_h_included
#define cg_newdraw_h_included

#include "../qcommon/q_shared.h"
#include "../ui/ui_shared.h"

extern int MenuWidescreen;
extern int QLWideScreen;
extern rectDef_t MenuRect;

void CG_InitTeamChat(void);
void CG_SetPrintString(int type, const char *p);
void CG_CheckOrderPending(void);
void CG_SelectNextPlayer(void);
void CG_SelectPrevPlayer(void);
float CG_GetValue(int ownerDraw);
qboolean CG_OtherTeamHasFlag(void);
qboolean CG_YourTeamHasFlag(void);
qboolean CG_OwnerDrawVisible(int flags, int flags2);
const char *CG_GetKillerText(void);
const char *CG_GetGameStatusText (vec4_t color);
const char *CG_GameTypeString(void);

int CG_Text_Length (const char *s);
void CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, const vec4_t color, const char* text, float adjust, int limit, const fontInfo_t *font);
void CG_Text_Paint_Limit_Bottom(float *maxX, float x, float y, float scale, const vec4_t color, const char* text, float adjust, int limit, const fontInfo_t *font);
void CG_Text_Paint_Align (const rectDef_t *rect, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, int align);

void CG_DrawWeaponBar (void);
int CG_GetCurrentTimeWithDirection (int *numberOfOvertimes);
const char *CG_GetLevelTimerString (void);
double CG_GetServerTimeFromClockString (const char *timeString);

void CG_OwnerDraw (float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int ownerDrawFlags2, int align, float special, float scale, const vec4_t color, qhandle_t shader, int textStyle, int fontIndex, int menuWidescreen, int itemWidescreen, rectDef_t menuRect);

void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_KeyEvent(int key, qboolean down);
void CG_ShowResponseHead(void);
void CG_RunMenuScript(char **args);
void CG_GetTeamColor(vec4_t *color);


#endif  // cg_newdraw_h_included

#ifndef cg_drawdc_h_included
#define cg_drawdc_h_included

#include "../qcommon/q_shared.h"
//#include "cg_public.h"
#include "../ui/ui_shared.h"  // rectDef_t

void CG_DrawHandlePicDc (float x, float y, float w, float h, qhandle_t asset, int widescreen, rectDef_t menuRect);
void CG_DrawStretchPicDc (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, int widescreen, rectDef_t menuRect);
void CG_DrawTextDc (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int fontIndex, int widescreen, rectDef_t menuRect);
int CG_TextWidthDc (const char *text, float scale, int limit, int fontIndex, int widescreen, rectDef_t menuRect);
int CG_TextHeightDc (const char *text, float scale, int limit, int fontIndex, int widescreen, rectDef_t menuRect);
void CG_FillRectDc (float x, float y, float w, float h, const vec4_t color, int widescreen, rectDef_t menuRect);
void CG_DrawRectDc (float x, float y, float w, float h, float size, const vec4_t color, int widescreen, rectDef_t menuRect);
void CG_DrawSidesDc (float x, float y, float w, float h, float size, int widescreen, rectDef_t menuRect);
void CG_DrawTopBottomDc (float x, float y, float w, float h, float size, int widescreen, rectDef_t menuRect);
void CG_DrawTextWithCursorDc (float x, float y, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int fontIndex, int widescreen, rectDef_t menuRect);
int CG_OwnerDrawWidthDc (int ownerDraw, float scale, int fontIndex, int widescreen, rectDef_t menuRect);

int CG_PlayCinematicDc (const char *name, float x, float y, float w, float h, int widescreen, rectDef_t menuRect);
void CG_StopCinematicDc (int handle);
void CG_DrawCinematicDc (int handle, float x, float y, float w, float h, int widescreen, rectDef_t menuRect);
void CG_RunCinematicFrameDc (int handle);

#endif  // cg_drawdc_h_included

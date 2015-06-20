#ifndef cg_draw_h_included
#define cg_draw_h_included

#include "../game/bg_public.h"
#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"  // stereoFrame_t
#include "cg_public.h"

extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

int CG_Text_Width(const char *text, float scale, int limit, const fontInfo_t *font);
int CG_Text_Width_old(const char *text, float scale, int limit, int fontIndex);
int CG_Text_Height(const char *text, float scale, int limit, const fontInfo_t *font);
int CG_Text_Height_old(const char *text, float scale, int limit, int fontIndex);

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_Text_PaintCharScale (float x, float y, float width, float height, float xscale, float yscale, float s, float t, float s2, float t2, qhandle_t hShader);

void CG_Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *font);
void CG_Text_Pic_Paint (float x, float y, float scale, const vec4_t color, const int *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, int textHeight, float iconScale);

//void CG_CreateNameSprite (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, qhandle_t h, int imageWidth, int imageHeight);
void CG_CreateNameSprite (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, qhandle_t h);

void CG_Text_Paint_Bottom (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *font);
void CG_Text_Paint_old(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int fontIndex);

void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, const vec3_t origin, const vec3_t angles );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, const vec3_t headAngles, qboolean useDefaultTeamSkin );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );

#define LAG_SAMPLES		128

typedef struct {
	int             frameSamples[LAG_SAMPLES];
	int             frameCount;
	int             snapshotFlags[LAG_SAMPLES];
	int             snapshotSamples[LAG_SAMPLES];
	int             snapshotCount;
} lagometer_t;

extern lagometer_t  lagometer;

void CG_ResetLagometer (void);
void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_LagometerMarkNoMove (void);

void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_CenterPrintFragMessage (const char *str, int y, int charWidth);
int *CG_CreateFragString (qboolean lastFrag);
void CG_CreateNewCrosshairs (void);

void CG_Fade( int a, int time, int duration );
void CG_DrawFlashFade( void );

void CG_DrawActive( stereoFrame_t stereoView );

#endif  // cg_draw_h_included

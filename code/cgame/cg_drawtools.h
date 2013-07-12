#ifndef cg_drawtools_h_included
#define cg_drawtools_h_included

void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawStretchPic (float x, float y, float width, float height, float s1, float t1, float s2, float t2, qhandle_t hShader);

void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
					   qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars, const fontInfo_t *font );


//void CG_DrawString( float x, float y, const char *string, float charWidth, float charHeight, const float *modulate );


void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, const vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, const vec4_t color );

int CG_DrawStrlen( const char *str, const fontInfo_t *font );

void CG_TileClear( void );

float	*CG_FadeColor( int startMsec, int totalMsec );
float *CG_FadeColorRealTime (int startMsec, int totalMsec);
void  CG_FadeColorVec4 (vec4_t color, int startMsec, int totalMsec, int fadeTimeMsec);

float *CG_TeamColor( int team );

void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );
void CG_ColorForHealth( vec4_t hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, const vec4_t color );
int UI_DrawProportionalString3 (int x, int y, const char* str, int style, const vec4_t color);

#endif  // cg_drawtools_h_included

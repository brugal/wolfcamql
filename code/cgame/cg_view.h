#ifndef cg_view_h_included
#define cg_view_h_included

void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);

void CG_CalcVrect (void);

void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );

double CG_CalcZoom (double fov_x);
void CG_AdjustedFov (float fov_x, float *new_fov_x, float *new_fov_y);

void CG_AddBufferedSound( sfxHandle_t sfx);
float CG_CalcSpline (int step, float tension);

void CG_AdjustOriginToAvoidSolid (vec3_t origin, const centity_t *cent);

void CG_DrawActiveFrame (int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, qboolean videoRecording, int ioverf, qboolean draw);

#endif  // cg_view_h_included

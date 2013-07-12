#ifndef wolfcam_view_h_included
#define wolfcam_view_h_included

void CG_GetModelName (const char *s, char *model);
void CG_GetModelAndSkinName (const char *s, char *model, char *skin);
void Wolfcam_LoadModels (void);
int Wolfcam_OffsetThirdPersonView (void);
int Wolfcam_OffsetFirstPersonView (void);
int Wolfcam_CalcFov (void);

#endif  // wolfcam_view_h_included

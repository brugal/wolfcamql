#ifndef wolfcam_weapons_h_included
#define wolfcam_weapons_h_included

qboolean Wolfcam_CalcMuzzlePoint (int entityNum, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, qboolean useLerp);
void Wolfcam_AddViewWeapon (void);

#endif  // wolfcam_weapons_h_included

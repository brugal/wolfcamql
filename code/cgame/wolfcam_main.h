#ifndef wolfcam_main_h_included
#define wolfcam_main_h_included

void Wolfcam_AddBox (const vec3_t origin, int x, int y, int z, int red, int green, int blue);
void Wolfcam_AddBoundingBox( const centity_t *cent );
int Wolfcam_PlayerHealth (int clientNum, qboolean usePainValue);
int Wolfcam_PlayerArmor (int clientNum);

#endif  // wolfcam_main_h_included

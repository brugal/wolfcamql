#ifndef cg_sound_h_included
#define cg_sound_h_included

#include "../qcommon/q_shared.h"

void CG_StartSound (const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);

void CG_StartLocalSound (sfxHandle_t sfx, int channelNum);

void CG_AddLoopingSound (int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);

void CG_AddRealLoopingSound (int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);

#endif  // cg_sound_h_included

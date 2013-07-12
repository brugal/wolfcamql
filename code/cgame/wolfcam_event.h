#ifndef wolfcam_event_h_included
#define wolfcam_event_h_included

int find_best_target (int attackerClientNum, qboolean useLerp, vec_t *distance, vec3_t offsets);
void wolfcam_log_event (const centity_t *cent, const vec3_t position);
void Wolfcam_LogMissileHit (int weapon, const vec3_t origin, const vec3_t dir, int entityNum);

#endif  // wolfcam_event_h_included

#ifndef cg_playerstate_h_included
#define cg_playerstate_h_included

void CG_PrintPlayerState (void);
int CG_GetAmmoWarning (int weapon, int style, int ammoOffset);
//void CG_DamageFeedback( int yawByte, int pitchByte, int damage );
void CG_Respawn( void );
void CG_CheckChangedPredictableEvents( const playerState_t *ps );
void CG_TransitionPlayerState( const playerState_t *ps, playerState_t *ops );
void CG_PushReward (sfxHandle_t sfx, qhandle_t shader, int rewardCount);

#endif  // cg_playerstate_h_included

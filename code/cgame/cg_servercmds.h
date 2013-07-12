#ifndef cg_servercmds_h_included
#define cg_servercmds_h_included

void CG_LoadInfectedGameTypeModels (void);
void CG_ParseServerinfo (qboolean firstCall);
void CG_ParseWarmup (void);
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);
void CG_InterMissionHit (void);

#ifdef MISSIONPACK
void CG_LoadVoiceChats( void );
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );
#endif

void CG_ExecuteNewServerCommands( int latestSequence );

void CG_CpmaParseScores (void);
void CG_CpmaParseGameState (qboolean initial);

#endif  // cg_servercmds_h_included

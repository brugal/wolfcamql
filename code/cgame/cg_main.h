#ifndef cg_main_h_included
#define cg_main_h_included

extern const char *gametypeConfigs[];

void CG_UpdateCvars (void);
void CG_CheckFontUpdates (void);
int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_AddChatLine (const char *line);

void QDECL CG_PrintToScreen( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL CG_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL CG_Error( const char *msg, ... ) __attribute__ ((noreturn, format (printf, 1, 2)));

const char *CG_Argv( int arg );
int CG_Argc (void);
const char *CG_ArgsFrom (int arg);

void CG_BuildSpectatorString(void);
qboolean CG_ConfigStringIndexToQ3 (int *index);
qboolean CG_ConfigStringIndexFromQ3 (int *index);
const char *CG_ConfigString( int index );
const char *CG_ConfigStringNoConvert (int index);

void CG_StartMusic( void );

void CG_ParseMenu (const char *menuFile);
void CG_LoadMenus(const char *menuFile);
void CG_SetScoreSelection (menuDef_t *menu);
void CG_LimitText (char *s, int len);

float CG_Cvar_Get (const char *cvar);
void CG_ResetTimedItemPickupTimes (void);
void CG_CreateScoresFromClientInfo (void);
void CG_LoadDefaultMenus (void);
void CG_LoadHudFile (const char *menuFile);
void CG_SetDuelPlayers (void);
void CG_ForceModelChange (void);

#endif  // cg_main_h_included

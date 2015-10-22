#ifndef cg_consolecmds_h_included
#define cg_consolecmds_h_included

#include "../qcommon/q_shared.h"

qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

int CG_AdjustTimeForTimeouts (int timeLength, qboolean forward);

void CG_ErrorPopup (const char *s);
void CG_EchoPopup (const char *s, int x, int y);

void CG_CleanUpFieldNumber (void);
void CG_ChangeConfigString (const char *buffer, int index);

void CG_StartidCamera (const char *name, qboolean startBlack);


#endif  // cg_consolecmds_h_included

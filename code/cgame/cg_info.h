#ifndef cg_info_h_included
#define cg_info_h_included

void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_LoadingFutureClient (const char *modelName);
void CG_DrawInformation (qboolean loading);

#endif  // cg_info_h_included

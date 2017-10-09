#ifndef cg_syscalls_h_included
#define cg_syscalls_h_included

#include "../renderer/tr_types.h"  // refEntity_t
#include "cg_camera.h"  // cameraPoint_t
#include "cg_public.h"  // snapshot_t, itemPickup_t, timeOut_t

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *fmt );

// abort the game
void		trap_Error( const char *fmt ) __attribute__ ((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
qboolean trap_Cvar_Exists (const char *var_name);

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );
void trap_SendConsoleCommandNow(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );
void    trap_RemoveCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void    trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void    trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
										 const vec3_t mins, const vec3_t maxs,
										 clipHandle_t model, int brushmask,
										 const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points,
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void	trap_S_StopBackgroundTrack( void );
void trap_S_PrintSfxFilename (sfxHandle_t sfx);

int trap_RealTime (qtime_t *qtime, qboolean now, int convertTime);

void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderLightMap( const char *name, int lightmap );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );
void trap_R_AddRefEntityPtrToScene (refEntity_t *re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int lightmap );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void    trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );

int			trap_R_LightForPoint( const vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd, const refdef_t *fd2 );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
					   float frac, const char *tagName );
void trap_R_RemapShader (const char *oldShader, const char *newShader, const char *timeOffset, qboolean keepLightmap, qboolean userSet);
void trap_R_ClearRemappedShader (const char *shaderName);
qboolean  trap_R_inPVS( const vec3_t p1, const vec3_t p2 );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned separately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean	trap_PeekSnapshot (int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale );

// aids for VM testing
void		testPrintInt( const char *string, int i );
void		testPrintFloat( const char *string, float f );

int			trap_MemoryRemaining( void );
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );
void		trap_Key_GetBinding(int keynum, char *buffer);
void		trap_Get_Advertisements(int *num, float *verts, char shaders[][MAX_QPATH]);
void		trap_R_DrawConsoleLines (void);
int			trap_GetLastExecutedServerCommand (void);
qboolean trap_GetNextKiller (int us, int serverTime, int *killer, int *foundServerTime, qboolean onlyOtherClient);
qboolean trap_GetNextVictim (int us, int serverTime, int *victim, int *foundServerTime, qboolean onlyOtherClient);
void trap_ReplaceShaderImage (qhandle_t h, const ubyte *data, int width, int height);
qhandle_t trap_RegisterShaderFromData (const char *name, const ubyte *data, int width, int height, qboolean mipmap, qboolean allowPicmip, int wrapClampMode, int lightmapIndex);
void trap_GetShaderImageDimensions (qhandle_t h, int *width, int *height);
void trap_GetShaderImageData (qhandle_t h, ubyte *data);
void trap_CalcSpline (int step, float tension, float *out);
qhandle_t trap_R_GetSingleShader (void);


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

void trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles, float *fov);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

int trap_PC_LoadSource( const char *filename );
int trap_PC_FreeSource( int handle );
int trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int trap_PC_AddGlobalDefine( char *define );
void trap_SetPathLines (int *numCameraPoints, cameraPoint_t *cameraPoints, int *numSplinePoints, vec3_t *splinePoints, const vec4_t color);
int trap_GetGameStartTime (void);
int trap_GetGameEndTime (void);
int trap_GetFirstServerTime (void);
int trap_GetLastServerTime (void);
void trap_AddAt (int serverTime, const char *clockTime, const char *command);
int trap_GetLegsAnimStartTime (int entityNum);
int trap_GetTorsoAnimStartTime (int entityNum);
void trap_autoWriteConfig (qboolean write);
int trap_GetItemPickupNumber (int pickupTime);
int trap_GetItemPickup (int pickupNumber, itemPickup_t *ip);
void trap_Get_Demo_Timeouts (int *numTimeouts, timeOut_t *timeOuts);
int trap_GetNumPlayerInfos (void);
void trap_GetExtraPlayerInfo (int num, char *modelName);
void trap_GetRealMapName (char *name, char *realName, size_t szRealName);

// ui
void trap_Key_SetOverstrikeMode (qboolean state);
qboolean trap_Key_GetOverstrikeMode (void);
void trap_Key_SetBinding (int keynum, const char *binding);
void trap_Key_GetBindingBuf (int keynum, char *buf, int buflen);
void trap_Key_KeynumToStringBuf (int keynum, char *buf, int buflen);
qboolean trap_R_GetGlyphInfo (const fontInfo_t *fontInfo, int charValue, glyphInfo_t *glyphOut);
qboolean trap_R_GetFontInfo (int fontId, fontInfo_t *font);
void trap_GetRoundStartTimes (int *numRoundStarts, int *roundStarts);
qboolean trap_GetTeamSwitchTime (int clientNum, int startTime, int *teamSwitchTime);
#endif  // cg_syscalls_h_included

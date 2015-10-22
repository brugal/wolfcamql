// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

#include "cg_local.h"

#include "cg_syscalls.h"



#ifdef CGAME_HARD_LINKED
#define syscall cgame_syscall
extern int cgame_syscall (int arg, ...);

#else
static intptr_t (QDECL *syscall)( intptr_t arg, ... ) = (intptr_t (QDECL *)( intptr_t, ...))-1;


Q_EXPORT void dllEntry( intptr_t (QDECL  *syscallptr)( intptr_t arg,... ) ) {
	syscall = syscallptr;
}
#endif


static int PASSFLOAT( float x ) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void	trap_Print( const char *fmt ) {
	syscall( CG_PRINT, fmt );
}

void __attribute__ ((noreturn)) trap_Error( const char *fmt ) {
	syscall( CG_ERROR, fmt );
	exit(1);  // silence gcc warning
}

int		trap_Milliseconds( void ) {
	return syscall( CG_MILLISECONDS );
}

void	trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	syscall( CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags );
}

void	trap_Cvar_Update( vmCvar_t *vmCvar ) {
	syscall( CG_CVAR_UPDATE, vmCvar );
}

void	trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( CG_CVAR_SET, var_name, value );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

qboolean trap_Cvar_Exists (const char *var_name)
{
	return syscall(CG_CVAR_EXISTS, var_name);
}

int		trap_Argc( void ) {
	return syscall( CG_ARGC );
}

void	trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( CG_ARGV, n, buffer, bufferLength );
}

void	trap_Args( char *buffer, int bufferLength ) {
	syscall( CG_ARGS, buffer, bufferLength );
}

int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( CG_FS_FOPENFILE, qpath, f, mode );
}

void	trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_READ, buffer, len, f );
}

void	trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_WRITE, buffer, len, f );
}

void	trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( CG_FS_FCLOSEFILE, f );
}

int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return syscall( CG_FS_SEEK, f, offset, origin );
}

void	trap_SendConsoleCommand( const char *text ) {
	syscall( CG_SENDCONSOLECOMMAND, text );
}

void	trap_SendConsoleCommandNow( const char *text ) {
	syscall( CG_SENDCONSOLECOMMANDNOW, text );
}

void	trap_AddCommand( const char *cmdName ) {
	syscall( CG_ADDCOMMAND, cmdName );
}

void	trap_RemoveCommand( const char *cmdName ) {
	syscall( CG_REMOVECOMMAND, cmdName );
}

void	trap_SendClientCommand( const char *s ) {
	syscall( CG_SENDCLIENTCOMMAND, s );
}

void	trap_UpdateScreen( void ) {
	syscall( CG_UPDATESCREEN );
}

void	trap_CM_LoadMap( const char *mapname ) {
	syscall( CG_CM_LOADMAP, mapname );
}

int		trap_CM_NumInlineModels( void ) {
	return syscall( CG_CM_NUMINLINEMODELS );
}

clipHandle_t trap_CM_InlineModel( int index ) {
	return syscall( CG_CM_INLINEMODEL, index );
}

clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {
	return syscall( CG_CM_TEMPBOXMODEL, mins, maxs );
}

clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs ) {
	return syscall( CG_CM_TEMPCAPSULEMODEL, mins, maxs );
}

int		trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return syscall( CG_CM_POINTCONTENTS, p, model );
}

int		trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return syscall( CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles );
}

void	trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	syscall( CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask );
}

void	trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	syscall( CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask );
}

void	trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

void	trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

int		trap_CM_MarkFragments( int numPoints, const vec3_t *points,
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer ) {
	return syscall( CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void	trap_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	syscall( CG_S_STARTSOUND, origin, entityNum, entchannel, sfx );
}

void	trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( CG_S_STARTLOCALSOUND, sfx, channelNum );
}

void	trap_S_ClearLoopingSounds( qboolean killall ) {
	syscall( CG_S_CLEARLOOPINGSOUNDS, killall );
}

void	trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	syscall( CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx );
}

void	trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	syscall( CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, sfx );
}

void	trap_S_StopLoopingSound( int entityNum ) {
	syscall( CG_S_STOPLOOPINGSOUND, entityNum );
}

void	trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	syscall( CG_S_UPDATEENTITYPOSITION, entityNum, origin );
}

void	trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	syscall( CG_S_RESPATIALIZE, entityNum, origin, axis, inwater );
}

sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return syscall( CG_S_REGISTERSOUND, sample, compressed );
}

void	trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	syscall( CG_S_STARTBACKGROUNDTRACK, intro, loop );
}

void trap_S_PrintSfxFilename (sfxHandle_t sfx)
{
	syscall(CG_S_PRINTSFXFILENAME, sfx);
}

void	trap_R_LoadWorldMap( const char *mapname ) {
	syscall( CG_R_LOADWORLDMAP, mapname );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return syscall( CG_R_REGISTERMODEL, name );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return syscall( CG_R_REGISTERSKIN, name );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	return syscall( CG_R_REGISTERSHADER, name );
}

qhandle_t trap_R_RegisterShaderLightMap( const char *name, int lightmap ) {
	return syscall( CG_R_REGISTERSHADERLIGHTMAP, name, lightmap );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return syscall( CG_R_REGISTERSHADERNOMIP, name );
}

void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) {
	syscall(CG_R_REGISTERFONT, fontName, pointSize, font );
}

void	trap_R_ClearScene( void ) {
	syscall( CG_R_CLEARSCENE );
}

void	trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( CG_R_ADDREFENTITYTOSCENE, re );
}

void trap_R_AddRefEntityPtrToScene (refEntity_t *re)
{
	syscall(CG_R_ADDREFENTITYPTRTOSCENE, re);
}

void	trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int lightmap ) {
	syscall( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts, lightmap );
}

void	trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num ) {
	syscall( CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, num );
}

int		trap_R_LightForPoint( const vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	return syscall( CG_R_LIGHTFORPOINT, point, ambientLight, directedLight, lightDir );
}

void	trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	//refEntity_t re;

	syscall( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );

#if 0  // testing..  take out

	if (SC_Cvar_Get_Int("r_flares") > 1) {
		return;
	}

	memset(&re, 0, sizeof(re));

	VectorCopy(org, re.origin);
	re.shaderTime = cg.time / 1000.0f;
	re.reType = RT_SPRITE;
	re.rotation = 0;
	re.radius = 40;
	re.customShader = trap_R_RegisterShader("flareShader");
#if 0
	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff;
#endif
	re.shaderRGBA[0] = 255.0 * r;
	re.shaderRGBA[1] = 255.0 * g;
	re.shaderRGBA[2] = 255.0 * b;
	re.shaderRGBA[3] = 100;

	CG_AddRefEntity(&re);
#endif
}

void	trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( CG_R_ADDADDITIVELIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );
}

void	trap_R_RenderScene( const refdef_t *fd ) {
	syscall( CG_R_RENDERSCENE, fd );
}

void	trap_R_SetColor( const float *rgba ) {
	syscall( CG_R_SETCOLOR, rgba );
}

void	trap_R_DrawStretchPic( float x, float y, float w, float h,
							   float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( CG_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader );
}

void	trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( CG_R_MODELBOUNDS, model, mins, maxs );
}

int		trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
					   float frac, const char *tagName ) {
	return syscall( CG_R_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName );
}

void trap_R_RemapShader (const char *oldShader, const char *newShader, const char *timeOffset, qboolean keepLightmap, qboolean userSet) {
	syscall(CG_R_REMAP_SHADER, oldShader, newShader, timeOffset, keepLightmap, userSet);
}

void trap_R_ClearRemappedShader (const char *shaderName)
{
	syscall(CG_R_CLEAR_REMAPPED_SHADER, shaderName);
}

void		trap_GetGlconfig( glconfig_t *glconfig ) {
	syscall( CG_GETGLCONFIG, glconfig );
}

void		trap_GetGameState( gameState_t *gamestate ) {
	syscall( CG_GETGAMESTATE, gamestate );
}

void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	syscall( CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime );
}

qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return syscall( CG_GETSNAPSHOT, snapshotNumber, snapshot );
}

qboolean	trap_PeekSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return syscall( CG_PEEKSNAPSHOT, snapshotNumber, snapshot );
}

qboolean	trap_GetServerCommand( int serverCommandNumber ) {
	return syscall( CG_GETSERVERCOMMAND, serverCommandNumber );
}

int			trap_GetCurrentCmdNumber( void ) {
	return syscall( CG_GETCURRENTCMDNUMBER );
}

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return syscall( CG_GETUSERCMD, cmdNumber, ucmd );
}

void		trap_SetUserCmdValue( int stateValue, float sensitivityScale ) {
	syscall( CG_SETUSERCMDVALUE, stateValue, PASSFLOAT(sensitivityScale) );
}

void		testPrintInt( const char *string, int i ) {
	syscall( CG_TESTPRINTINT, string, i );
}

void		testPrintFloat( const char *string, float f ) {
	syscall( CG_TESTPRINTFLOAT, string, PASSFLOAT(f) );
}

int trap_MemoryRemaining( void ) {
	return syscall( CG_MEMORY_REMAINING );
}

qboolean trap_Key_IsDown( int keynum ) {
	return syscall( CG_KEY_ISDOWN, keynum );
}

int trap_Key_GetCatcher( void ) {
	return syscall( CG_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( CG_KEY_SETCATCHER, catcher );
}

int trap_Key_GetKey( const char *binding ) {
	return syscall( CG_KEY_GETKEY, binding );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( CG_PC_ADD_GLOBAL_DEFINE, define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( CG_PC_LOAD_SOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( CG_PC_FREE_SOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( CG_PC_READ_TOKEN, handle, pc_token );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

void	trap_S_StopBackgroundTrack( void ) {
	syscall( CG_S_STOPBACKGROUNDTRACK );
}

int trap_RealTime (qtime_t *qtime, qboolean now, int convertTime) {
	return syscall(CG_REAL_TIME, qtime, now, convertTime);
}

void trap_SnapVector( float *v ) {
	syscall( CG_SNAPVECTOR, v );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits) {
  return syscall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic(int handle) {
  return syscall(CG_CIN_STOPCINEMATIC, handle);
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic (int handle) {
  return syscall(CG_CIN_RUNCINEMATIC, handle);
}


// draws the current frame
void trap_CIN_DrawCinematic (int handle) {
  syscall(CG_CIN_DRAWCINEMATIC, handle);
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  syscall(CG_CIN_SETEXTENTS, handle, x, y, w, h);
}


qboolean trap_loadCamera( const char *name ) {
	return syscall( CG_LOADCAMERA, name );
}

void trap_startCamera(int time) {
	syscall(CG_STARTCAMERA, time);
}

qboolean trap_getCameraInfo( int time, vec3_t *origin, vec3_t *angles, float *fov) {
	return syscall( CG_GETCAMERAINFO, time, origin, angles, fov );
}


qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return syscall( CG_GET_ENTITY_TOKEN, buffer, bufferSize );
}

qboolean trap_R_inPVS( const vec3_t p1, const vec3_t p2 ) {
	return syscall( CG_R_INPVS, p1, p2 );
}

void trap_Get_Advertisements (int *num, float *verts, char shaders[][MAX_QPATH]) {
	syscall(CG_GET_ADVERTISEMENTS, num, verts, shaders);
}

void trap_R_DrawConsoleLines (void)
{
	syscall(CG_DRAW_CONSOLE_LINES);
}

void trap_Key_GetBinding (int key, char *buffer)
{
	syscall(CG_KEY_GETBINDING, key, buffer);
}

int trap_GetLastExecutedServerCommand (void)
{
	return syscall(CG_GETLASTEXECUTEDSERVERCOMMAND);
}

qboolean trap_GetNextKiller (int us, int serverTime, int *killer, int *foundServerTime, qboolean onlyOtherClient)
{
	return syscall(CG_GETNEXTKILLER, us, serverTime, killer, foundServerTime, onlyOtherClient);
}

qboolean trap_GetNextVictim (int us, int serverTime, int *victim, int *foundServerTime, qboolean onlyOtherClient)
{
	return syscall(CG_GETNEXTVICTIM, us, serverTime, victim, foundServerTime, onlyOtherClient);
}

void trap_ReplaceShaderImage (qhandle_t h, const ubyte *data, int width, int height)
{
	syscall(CG_REPLACESHADERIMAGE, h, data, width, height);
}

qhandle_t trap_RegisterShaderFromData (const char *name, const ubyte *data, int width, int height, qboolean mipmap, qboolean allowPicmip, int wrapClampMode, int lightmapIndex)
{
	return syscall(CG_REGISTERSHADERFROMDATA, name, data, width, height, mipmap, allowPicmip, wrapClampMode, lightmapIndex);
}

void trap_GetShaderImageDimensions (qhandle_t h, int *width, int *height)
{
	syscall(CG_GETSHADERIMAGEDIMENSIONS, h, width, height);
}

void trap_GetShaderImageData (qhandle_t h, ubyte *data)
{
	syscall(CG_GETSHADERIMAGEDATA, h, data);
}

void trap_CalcSpline (int step, float tension, float *out)
{
	syscall(CG_CALCSPLINE, step, PASSFLOAT(tension), out);
}

void trap_SetPathLines (int *numCameraPoints, cameraPoint_t *cameraPoints, int *numSplinePoints, vec3_t *splinePoints, const vec4_t color)
{
	syscall(CG_SETPATHLINES, numCameraPoints, cameraPoints, numSplinePoints, splinePoints, color);
}

int trap_GetGameStartTime (void)
{
	return syscall(CG_GETGAMESTARTTIME);
}

int trap_GetGameEndTime (void)
{
	return syscall(CG_GETGAMEENDTIME);
}

int trap_GetFirstServerTime (void)
{
	return syscall(CG_GETFIRSTSERVERTIME);
}

#if 0
void trap_AddAt (int serverTime, const char *clockTime, const char *command)
{
	syscall(CG_ADDAT, serverTime, clockTime, command);
}
#endif

int trap_GetLegsAnimStartTime (int entityNum)
{
	return syscall(CG_GETLEGSANIMSTARTTIME, entityNum);
}

int trap_GetTorsoAnimStartTime (int entityNum)
{
	return syscall(CG_GETTORSOANIMSTARTTIME, entityNum);
}

void trap_autoWriteConfig (qboolean write)
{
	syscall(CG_AUTOWRITECONFIG, write);
}

int trap_GetItemPickupNumber (int pickupTime)
{
	return syscall(CG_GETITEMPICKUPNUMBER, pickupTime);
}

int trap_GetItemPickup (int pickupNumber, itemPickup_t *ip)
{
	return syscall(CG_GETITEMPICKUP, pickupNumber, ip);
}

qhandle_t trap_R_GetSingleShader (void)
{
	return syscall(CG_R_GETSINGLESHADER);
}

void trap_Get_Demo_Timeouts (int *numTimeouts, timeOut_t *timeOuts)
{
	syscall(CG_GET_DEMO_TIMEOUTS, numTimeouts, timeOuts);
}

int trap_GetNumPlayerInfos (void)
{
	return syscall(CG_GET_NUM_PLAYER_INFO);
}

void trap_GetExtraPlayerInfo (int num, char *modelName)
{
	syscall(CG_GET_EXTRA_PLAYER_INFO, num, modelName);
}

void trap_GetRealMapName (char *name, char *realName, size_t szRealName)
{
	syscall(CG_GET_REAL_MAP_NAME, name, realName, szRealName);
}

// ui
qboolean trap_Key_GetOverstrikeMode (void)
{
	return syscall(CG_KEY_GETOVERSTRIKEMODE);
}

void trap_Key_SetOverstrikeMode (qboolean state)
{
	syscall(CG_KEY_SETOVERSTRIKEMODE, state);
}

void trap_Key_SetBinding (int keynum, const char *binding)
{
	syscall(CG_KEY_SETBINDING, keynum, binding);
}

void trap_Key_GetBindingBuf (int keynum, char *buf, int buflen)
{
	syscall(CG_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

void trap_Key_KeynumToStringBuf (int keynum, char *buf, int buflen)
{
	syscall(CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

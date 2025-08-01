/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"
#include "../cgame/cg_camera.h"
#include "../client/cl_avi.h"

#ifdef USE_LOCAL_HEADERS
  #include "SDL_opengl.h"
#else
  #include <SDL_opengl.h>
#endif

#define	REF_API_VERSION		601  // wolfcam don't know the point of this, but bumping anyway

//
// these are the functions exported by the refresh module
//
typedef struct {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void	(*Shutdown)( qboolean destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void	(*BeginRegistration)( glconfig_t *config );
	void (*GetGlConfig)(glconfig_t *config);
	qhandle_t (*RegisterModel)( const char *name );
	void (*GetModelName)( qhandle_t index, char *name, int sz );
	qhandle_t (*RegisterSkin)( const char *name );
	qhandle_t (*RegisterShader)( const char *name );
	qhandle_t (*RegisterShaderLightMap)( const char *name, int lightmap );
	qhandle_t (*RegisterShaderNoMip)( const char *name );
	void	(*LoadWorld)( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void	(*SetWorldVisData)( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void	(*EndRegistration)( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void	(*ClearScene)( void );
	void	(*AddRefEntityToScene)( const refEntity_t *re );
	void	(*AddRefEntityPtrToScene)(refEntity_t *re);
	void (*SetPathLines)(int *numCameraPoints, cameraPoint_t *cameraPoints, int *numSplinePoints, vec3_t *splinePoints, const vec4_t color);
	void	(*AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num, int lightmap );
	int		(*LightForPoint)( const vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
	void	(*AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void	(*AddAdditiveLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void	(*RenderScene)( const refdef_t *fd );

	void	(*SetColor)( const float *rgba );	// NULL = 1,1,1,1
	void	(*DrawStretchPic) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void	(*UploadCinematic) (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

	void	(*BeginFrame)(stereoFrame_t stereoFrame, qboolean recordingVideo);

	// if the pointers are not NULL, timing info will be returned
	void	(*EndFrame)( int *frontEndMsec, int *backEndMsec );


	int		(*MarkFragments)( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int		(*LerpTag)( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, 
					 float frac, const char *tagName );
	void	(*ModelBounds)( qhandle_t model, vec3_t mins, vec3_t maxs );

#ifdef __USEA3D
	void    (*A3D_RenderGeometry) (void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus);
#endif
	void	(*RegisterFont)(const char *fontName, int pointSize, fontInfo_t *font);
	qboolean (*GetGlyphInfo) (fontInfo_t *fontInfo, int charValue, glyphInfo_t *glyphOut);
	qboolean (*GetFontInfo) (int fontId, fontInfo_t *font);
	void (*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime, qboolean keepLightmap, qboolean userSet);
	void (*ClearRemappedShader)(const char *shaderName);
	qboolean (*GetEntityToken)( char *buffer, int size );
	qboolean (*inPVS)( const vec3_t p1, const vec3_t p2 );

	void (*TakeVideoFrame)(aviFileData_t *afd, int h, int w, byte* captureBuffer, byte *encodeBuffer, qboolean motionJpeg, qboolean avi, qboolean tga, qboolean jpg, qboolean png, int picCount, char *givenFileName);

	void (*BeginHud)(void);
	void (*UpdateDof)(float viewFocus, float viewRadius);

	void (*Get_Advertisements)(int *num, float *verts, char shaders[][MAX_QPATH]);
	void (*ReplaceShaderImage)(qhandle_t h, const ubyte *data, int width, int height);

	qhandle_t (*RegisterShaderFromData) (const char *name, ubyte *data, int width, int height, qboolean mipmap, qboolean allowPicmip, int wrapClampMode, int lightmapIndex);
	void (*GetShaderImageDimensions) (qhandle_t h, int *width, int *height);
	void (*GetShaderImageData) (qhandle_t h, ubyte *data);
	qhandle_t (*GetSingleShader) (void);
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct {
	// print message on the local console
	void	(QDECL *Printf)( int printLevel, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

	// abort the game
	void	(QDECL *Error)( int errorLevel, const char *fmt, ...) __attribute__ ((noreturn, format (printf, 2, 3)));

	// RealMilliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int		(*ScaledMilliseconds)( void );
	int		(*RealMilliseconds) (void);

	// stack based memory allocation for per-level things that
	// won't be freed
#ifdef HUNK_DEBUG
	void	*(*Hunk_AllocDebug)( int size, ha_pref pref, char *label, char *file, int line );
#else
	void	*(*Hunk_Alloc)( int size, ha_pref pref );
#endif
	void	*(*Hunk_AllocateTempMemory)( int size );
	void	(*Hunk_FreeTempMemory)( void *block );

	// dynamic memory allocator for things that need to be freed
	void	*(*Malloc)( int bytes );
	void	(*Free)( void *buf );

	cvar_t	*(*Cvar_Get)( const char *name, const char *value, int flags );
	void	(*Cvar_Set)( const char *name, const char *value );
	void	(*Cvar_SetValue) (const char *name, float value);
	void	(*Cvar_ForceReset) (const char *var_name);
	cvar_t	*(*Cvar_FindVar) (const char *var_name);
	void	(*Cvar_CheckRange)( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral );
	int		(*Cvar_VariableIntegerValue) (const char *var_name);
	float	(*Cvar_VariableValue) (const char *var_name);
	void	(*Cvar_VariableStringBuffer) (const char *var_name, char *buffer, int bufsize);
	void	(*Cvar_SetDescription)( cvar_t *cv, const char *description );

	void	(*Cmd_AddCommand)( const char *name, void(*cmd)(void) );
	void	(*Cmd_RemoveCommand)( const char *name );

	int		(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);

	void	(*Cmd_ExecuteText) (int exec_when, const char *text);

	byte	*(*CM_ClusterPVS)(int cluster);

	// visualization for debugging collision detection
	void	(*CM_DrawDebugSurface)( void (*drawPoly)(int color, int numPoints, float *points) );

	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existence
	int		(*FS_FileIsInPAK)( const char *name, int *pCheckSum );
	long	(*FS_ReadFile)( const char *name, void **buf );
	void	(*FS_FreeFile)( void *buf );
	char **	(*FS_ListFiles)( const char *name, const char *extension, int *numfilesfound );
	void	(*FS_FreeFileList)( char **filelist );
	void	(*FS_WriteFile)( const char *qpath, const void *buffer, int size );
	int		(*FS_Write) (const void *buffer, int len, fileHandle_t f);
	qboolean (*FS_FileExists)( const char *file );
	const char	*(*FS_FindSystemFile) (const char *file);
	void	(*FS_FCloseFile) (fileHandle_t f);
	fileHandle_t	(*FS_FOpenFileWrite) (const char *qpath );

	// cinematic stuff
	void	(*CIN_UploadCinematic)(int handle);
	int		(*CIN_PlayCinematic)( const char *arg0, int xpos, int ypos, int width, int height, int bits);
	e_status (*CIN_RunCinematic) (int handle);

	void	(*CL_WriteAVIVideoFrame)(aviFileData_t *afd, const byte *buffer, int size);

	// input event handling
	void	(*IN_Init)( void *windowData );
	void    (*IN_Shutdown)( void );
	void    (*IN_Restart)( void );

	// math
	long    (*ftol)(float f);

	// system stuff
	void    (*Sys_SetEnv)( const char *name, const char *value );
	void    (*Sys_GLimpSafeInit)( void );
	void    (*Sys_GLimpInit)( void );
	qboolean (*Sys_LowPhysicalMemory)( void );

	// video recording stuff
	qboolean *SplitVideo;

	aviFileData_t *afdMain;
	aviFileData_t *afdLeft;
	aviFileData_t *afdRight;

	aviFileData_t *afdDepth;
	aviFileData_t *afdDepthLeft;
	aviFileData_t *afdDepthRight;

	GLfloat **Video_DepthBuffer;
	byte **ExtraVideoBuffer;

	// misc
	mapNames_t *MapNames;

	qboolean sse2_supported;
} refimport_t;


// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
#ifdef USE_RENDERER_DLOPEN
typedef	refexport_t* (QDECL *GetRefAPI_t) (int apiVersion, refimport_t *rimp);
#else
refexport_t*GetRefAPI( int apiVersion, refimport_t *rimp );
#endif

#endif	// __TR_PUBLIC_H

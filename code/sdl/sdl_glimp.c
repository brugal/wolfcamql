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

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#ifdef SMP
#	ifdef USE_LOCAL_HEADERS
#		include "SDL_thread.h"
#	else
#		include <SDL_thread.h>
#	endif
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderer/tr_local.h"
#include "../sys/sys_local.h"
#include "wolfcamql_icon.h"

#define PrintPtrAddr(ptr) do { ri.Printf(PRINT_ALL, "ptr# %p\n", ptr); } while (0);

/* Just hack it for now. */
#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
typedef CGLContextObj QGLContext;
#define GLimp_GetCurrentContext() CGLGetCurrentContext()
#define GLimp_SetCurrentContext(ctx) CGLSetCurrentContext(ctx)
#else
typedef void *QGLContext;
#define GLimp_GetCurrentContext() (NULL)
#define GLimp_SetCurrentContext(ctx)
#endif

static QGLContext opengl_context;

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

static SDL_Surface *screen = NULL;
static const SDL_VideoInfo *videoInfo = NULL;

static int lastMode = -999;
static int lastFullscreen = -999;
static int lastBpp = -999;

static const char *ExtensionString = NULL;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

cvar_t *r_visibleWindowWidth;
cvar_t *r_visibleWindowHeight;
//cvar_t *r_visibleWindowPixelAspect;
cvar_t *r_fboStencil;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
void (APIENTRYP qglMultiTexCoord2iARB) (GLenum target, GLint s, GLint t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

// glsl
GLhandleARB (APIENTRYP qglCreateShaderObjectARB) (GLenum shaderType);
void (APIENTRYP qglShaderSourceARB) (GLhandleARB shader, int numOfStrings, const char **strings, int *lenOfStrings);
void (APIENTRYP qglCompileShaderARB) (GLhandleARB shader);
GLhandleARB (APIENTRYP qglCreateProgramObjectARB) (void);
void (APIENTRYP qglAttachObjectARB) (GLhandleARB program, GLhandleARB shader);
void (APIENTRYP qglLinkProgramARB) (GLhandleARB program);
void (APIENTRYP qglUseProgramObjectARB) (GLhandleARB prog);
void (APIENTRYP qglGetObjectParameterivARB) (GLhandleARB object, GLenum type, int *param);
void (APIENTRYP qglGetInfoLogARB) (GLhandleARB object, int maxLen, int *len, char *log);

void (APIENTRYP qglDetachObjectARB) (GLhandleARB program, GLhandleARB shader);
void (APIENTRYP qglDeleteObjectARB) (GLhandleARB id);

GLint (APIENTRYP qglGetUniformLocationARB) (GLhandleARB program, const char *name);
void (APIENTRYP qglUniform1fARB) (GLint location, GLfloat v0);
void (APIENTRYP qglUniform1iARB) (GLint location, GLint v0);

// frame and render buffer
void (APIENTRYP qglGenFramebuffersEXT) (GLsizei n, GLuint *framebuffers);
void (APIENTRYP qglDeleteFramebuffersEXT) (GLsizei n, const GLuint *framebuffers);
GLvoid (APIENTRYP qglBindFramebufferEXT) (GLenum target, GLuint framebuffer);
GLvoid (APIENTRYP qglFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLenum (APIENTRYP qglCheckFramebufferStatusEXT) (GLenum target);

void (APIENTRYP qglGenRenderbuffersEXT) (GLsizei n, GLuint *renderbuffers);
void (APIENTRYP qglDeleteRenderbuffersEXT) (GLsizei n, const GLuint *renderbuffers);
void (APIENTRYP qglBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
GLvoid (APIENTRYP qglFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLvoid (APIENTRYP qglRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid (APIENTRYP qglRenderbufferStorageMultisampleEXT) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid (APIENTRYP qglBlitFramebufferEXT) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	IN_Shutdown();

	ri.Printf(PRINT_ALL, "GLimp_Shutdown: mode %d lastMode %d\n", r_mode->integer, lastMode);
	//FIXME vid_restart is causing stuttering playback
	// fucking hack
#if 0
	if (r_mode->integer != lastMode  ||  r_fullscreen->integer != lastFullscreen) {
		ri.Printf(PRINT_ALL, "change, shutting down video\n");
		//SDL_QuitSubSystem( SDL_INIT_VIDEO );
	}
#endif
	//SDL_QuitSubSystem( SDL_INIT_TIMER );
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	//SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
	//SDL_QuitSubSystem(SDL_INIT_EVENTTHREAD);
	//SDL_Quit();
	screen = NULL;

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
	ExtensionString = NULL;
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize(void)
{
	SDL_WM_IconifyWindow();
}

/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( char *comment )
{
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = *(SDL_Rect **)a;
	SDL_Rect *modeB = *(SDL_Rect **)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs( aspectA - displayAspect );
	float aspectDiffB = fabs( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}


/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes(void)
{
	char buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect **modes;
	int numModes;
	int i;

	modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );

	if( !modes )
	{
		ri.Printf( PRINT_WARNING, "Can't get list of available modes\n" );
		return;
	}

	if( modes == (SDL_Rect **)-1 )
	{
		ri.Printf( PRINT_ALL, "Display supports any resolution\n" );
		return; // can set any resolution
	}

	for( numModes = 0; modes[ numModes ]; numModes++ );

	if( numModes > 1 )
		qsort( modes, numModes, sizeof( SDL_Rect* ), GLimp_CompareModes );

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ]->w, modes[ i ]->h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			ri.Printf( PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[i]->w, modes[i]->h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri.Printf( PRINT_ALL, "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	const char*   glstring;
	int sdlcolorbits;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int samples;
	int i = 0;
	SDL_Surface *vidscreen = NULL;
	Uint32 flags = SDL_OPENGL;
	int sugBpp;
	int vidWidth;
	int vidHeight;
	GLint maxViewPortWidthAndHeight[2];

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");
	//r_mode->modified = qfalse;

	if ( r_allowResize->integer )
		flags |= SDL_RESIZABLE;

	if( videoInfo == NULL )
	{
		static SDL_VideoInfo sVideoInfo;
		static SDL_PixelFormat sPixelFormat;

		videoInfo = SDL_GetVideoInfo( );

		// Take a copy of the videoInfo
		Com_Memcpy( &sPixelFormat, videoInfo->vfmt, sizeof( SDL_PixelFormat ) );
		sPixelFormat.palette = NULL; // Should already be the case
		Com_Memcpy( &sVideoInfo, videoInfo, sizeof( SDL_VideoInfo ) );
		sVideoInfo.vfmt = &sPixelFormat;
		videoInfo = &sVideoInfo;

		if( videoInfo->current_h > 0 )
		{
			// Guess the display aspect ratio through the desktop resolution
			// by assuming (relatively safely) that it is set at or close to
			// the display's native aspect ratio
			displayAspect = (float)videoInfo->current_w / (float)videoInfo->current_h;

			ri.Printf( PRINT_ALL, "Estimated display aspect: %.3f\n", displayAspect );
		}
		else
		{
			ri.Printf( PRINT_ALL,
					"Cannot estimate display aspect, assuming 1.333\n" );
		}
	}

	ri.Printf (PRINT_ALL, "...setting mode %d:", mode );

	if (mode == -2) {
		glConfig.vidWidth = videoInfo->current_w;
		glConfig.vidHeight = videoInfo->current_h;
		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}

	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	if (fullscreen)
	{
		flags |= SDL_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if (noborder)
			flags |= SDL_NOFRAME;

		glConfig.isFullscreen = qfalse;
	}

#if 0
	if (noborder) {
		flags |= SDL_NOFRAME;
	}
#endif

	colorbits = r_colorbits->value;
	if ((!colorbits) || (colorbits >= 32))
		colorbits = 24;

	if (!r_depthbits->value)
		depthbits = 24;
	else
		depthbits = r_depthbits->value;
	stencilbits = r_stencilbits->value;
	samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorbits == 24)
						colorbits = 16;
					break;
				case 1 :
					if (depthbits == 24)
						depthbits = 16;
					else if (depthbits == 16)
						depthbits = 8;
				case 3 :
					if (stencilbits == 24)
						stencilbits = 16;
					else if (stencilbits == 16)
						stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ((i % 4) == 3)
		{ // reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		sdlcolorbits = 4;
		if (tcolorbits == 24)
			sdlcolorbits = 8;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (sdlcolorbits == 4)
			sdlcolorbits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, sdlcolorbits );
		//SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, sdlcolorbits );

		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

		if(r_stereoEnabled->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

#if 0 // See http://bugzilla.icculus.org/show_bug.cgi?id=3526
		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
		{
			if( SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 ) < 0 )
			{
				ri.Printf( PRINT_ALL, "Unable to guarantee accelerated "
						"visual with libSDL < 1.2.10\n" );
			}
		}
#endif

		if( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval->integer ) < 0 )
			ri.Printf( PRINT_ALL, "r_swapInterval requires libSDL >= 1.2.10\n" );

#ifdef USE_ICON
		{
			SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(
					(void *)CLIENT_WINDOW_ICON.pixel_data,
					CLIENT_WINDOW_ICON.width,
					CLIENT_WINDOW_ICON.height,
					CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
					CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
					0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
					0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
					);

			SDL_WM_SetIcon( icon, NULL );
			SDL_FreeSurface( icon );
		}
#endif

		SDL_WM_SetCaption(CLIENT_WINDOW_TITLE, CLIENT_WINDOW_MIN_TITLE);
		SDL_ShowCursor(0);

		//colorbits = sdlcolorbits;

		ri.Printf(PRINT_ALL, "about to set mode %d %d  bpp %d\n", glConfig.vidWidth, glConfig.vidHeight, tcolorbits);
		sugBpp = SDL_VideoModeOK(glConfig.vidWidth, glConfig.vidHeight, tcolorbits, SDL_HWSURFACE);
		if (sugBpp != tcolorbits) {
			ri.Printf(PRINT_ALL, "sdl says no, to use %d instead of %d\n", sugBpp, tcolorbits);
			//colorbits = sugBpp;
			//r_mode->modified = qfalse;
		}

		if (r_useFbo  &&  (*r_visibleWindowWidth->string  &&  *r_visibleWindowHeight->string  &&  r_visibleWindowWidth->integer > 0  &&  r_visibleWindowHeight->integer > 0)) {
			vidWidth = r_visibleWindowWidth->integer;
			vidHeight = r_visibleWindowHeight->integer;
			glConfig.visibleWindowWidth = vidWidth;
			glConfig.visibleWindowHeight = vidHeight;
		} else {
			vidWidth = glConfig.vidWidth;
			vidHeight = glConfig.vidHeight;
			glConfig.visibleWindowWidth = vidWidth;
			glConfig.visibleWindowHeight = vidHeight;
		}

		if (!(vidscreen = SDL_SetVideoMode(vidWidth, vidHeight, tcolorbits, flags)))
		{
			ri.Printf(PRINT_ALL, "^1SDL_SetVideoMode failed: %s\n", SDL_GetError());
			continue;
		}

		opengl_context = GLimp_GetCurrentContext();

		qglGetIntegerv(GL_MAX_VIEWPORT_DIMS, &maxViewPortWidthAndHeight[0]);
		glConfig.maxViewPortWidth = maxViewPortWidthAndHeight[0];
		glConfig.maxViewPortHeight = maxViewPortWidthAndHeight[1];
		//Com_Printf("max viewport: %d x %d\n", glConfig.maxViewPortWidth, glConfig.maxViewPortHeight);
#if 0  // pointless
		if (vidWidth > glConfig.maxViewPortWidth  ||  vidHeight > glConfig.maxViewPortHeight) {
			ri.Printf(PRINT_ALL, "^1invalid width or height for window (%d x %d), maximum is (%d x %d)\n", vidWidth, vidHeight, glConfig.maxViewPortWidth, glConfig.maxViewPortHeight);
			return RSERR_INVALID_MODE;
		}
#endif

		ri.Printf( PRINT_ALL, "^2Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
				sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		//glConfig.stencilBits = 8;
		//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		break;
	}

	GLimp_DetectAvailableModes();

	if (!vidscreen)
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	screen = vidscreen;

	glstring = (char *) qglGetString (GL_RENDERER);
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	rserr_t err;
	//static qboolean sdlinit = qfalse;

	if (!SDL_WasInit(SDL_INIT_VIDEO))   // &&  !sdlinit)
	{
		char driverName[ 64 ];

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n",
					SDL_GetError());
			//FIXME maybe not
			lastMode = -999;
			lastFullscreen = -999;
			lastBpp = -999;
			return qfalse;
		}
		//sdlinit = qtrue;

		SDL_VideoDriverName( driverName, sizeof( driverName ) - 1 );
		ri.Printf( PRINT_ALL, "SDL using driver \"%s\"\n", driverName );
		Cvar_Set( "r_sdlDriver", driverName );
	}

	if (fullscreen && Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = GLimp_SetMode(mode, fullscreen, noborder);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			lastMode = -999;
			lastFullscreen = -999;
			lastBpp = -999;
			return qfalse;
		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
			lastMode = -999;
			lastFullscreen = -999;
			lastBpp = -999;
			return qfalse;
		default:
			break;
	}

	lastMode = mode;
	lastFullscreen = fullscreen;
	//lastBpp = colorbits;

	return qtrue;
}

static qboolean GLimp_HaveExtension (const char *ext)
{
	const char *ptr;

	if (ExtensionString == NULL) {
		Com_Printf("^1GLimp_HaveExtension:  ExtensionString == NULL\n");
		return qfalse;
	}
	if (ext == NULL) {
		Com_Printf("^1GLimp_HaveExtension:  ext == NULL\n");
		return qfalse;
	}

	ptr = Q_stristr(ExtensionString, ext);
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}


/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( void )
{
	//GLint n;

	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( GLimp_HaveExtension( "GL_ARB_texture_compression" ) &&
	     GLimp_HaveExtension( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		if ( GLimp_HaveExtension( "GL_S3_s3tc" ) )
		{
			if ( r_ext_compressed_textures->value )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
		}
	}


	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GLimp_HaveExtension( "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglMultiTexCoord2iARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( GLimp_HaveExtension( "GL_ARB_multitexture" ) )
	{
		if ( r_ext_multitexture->value )
		{
			qglMultiTexCoord2fARB = SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
			qglMultiTexCoord2iARB = SDL_GL_GetProcAddress( "glMultiTexCoord2iARB" );
			qglActiveTextureARB = SDL_GL_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				GLint glint = 0;
				qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
				glConfig.numTextureUnits = (int) glint;
				if ( glConfig.numTextureUnits > 1 )
				{
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglMultiTexCoord2iARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	if ( GLimp_HaveExtension( "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT)
			{
				ri.Error (ERR_FATAL, "bad getprocaddress for compiled vertex array");
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	textureFilterAnisotropic = qfalse;
	if ( GLimp_HaveExtension( "GL_EXT_texture_filter_anisotropic" ) )
	{
		if ( r_ext_texture_filter_anisotropic->integer ) {
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&maxAnisotropy );
			if ( maxAnisotropy <= 0 ) {
				ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				maxAnisotropy = 0;
			}
			else
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", maxAnisotropy );
				textureFilterAnisotropic = qtrue;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}

	qglCreateShaderObjectARB = NULL;
	qglShaderSourceARB = NULL;
	qglCompileShaderARB = NULL;
	qglCreateProgramObjectARB = NULL;
	qglAttachObjectARB = NULL;
	qglLinkProgramARB = NULL;
	qglUseProgramObjectARB = NULL;
	qglGetObjectParameterivARB = NULL;
	qglGetInfoLogARB = NULL;
	qglGetObjectParameterivARB = NULL;
	qglDetachObjectARB = NULL;
	qglDeleteObjectARB = NULL;
	qglGetUniformLocationARB = NULL;
	qglUniform1fARB = NULL;
	qglUniform1iARB = NULL;

	//if (GLimp_HaveExtension("GL_ARB_shader_objects")) {
	if (1) {  //if (GLimp_HaveExtension("GL_ARB_fragment_shader")  &&  GLimp_HaveExtension("GL_ARB_vertex_shader")  &&  (GLimp_HaveExtension("GL_ARB_texture_rectangle")  ||  GLimp_HaveExtension("GL_EXT_texture_rectangle")  ||  GLimp_HaveExtension("GL_NV_texture_rectangle"))) {
		if (r_enablePostProcess->integer) {
			glConfig.glsl = qtrue;

			qglCreateShaderObjectARB = (GLhandleARB (APIENTRY *)(GLenum)) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
			qglShaderSourceARB = (void (APIENTRY *)(GLhandleARB, int, const char **, int *)) SDL_GL_GetProcAddress("glShaderSourceARB");
			qglCompileShaderARB = (void (APIENTRY *)(GLhandleARB)) SDL_GL_GetProcAddress("glCompileShaderARB");
			qglCreateProgramObjectARB = (GLhandleARB (APIENTRY *)(void)) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
			qglAttachObjectARB = (void (APIENTRY *)(GLhandleARB, GLhandleARB)) SDL_GL_GetProcAddress("glAttachObjectARB");
			qglLinkProgramARB = (void (APIENTRY *)(GLhandleARB)) SDL_GL_GetProcAddress("glLinkProgramARB");
			qglUseProgramObjectARB = (void (APIENTRY *)(GLhandleARB)) SDL_GL_GetProcAddress("glUseProgramObjectARB");
			qglGetObjectParameterivARB = (void (APIENTRY *)(GLhandleARB, GLenum, int *)) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
			qglGetInfoLogARB = (void (APIENTRY *)(GLhandleARB, int, int *, char *)) SDL_GL_GetProcAddress("glGetInfoLogARB");

			qglDetachObjectARB = (void (APIENTRY *)(GLhandleARB, GLhandleARB)) SDL_GL_GetProcAddress("glDetachObjectARB");
			qglDeleteObjectARB = (void (APIENTRY *)(GLhandleARB)) SDL_GL_GetProcAddress("glDeleteObjectARB");

			qglGetUniformLocationARB = (GLint (APIENTRY *)(GLhandleARB, const char *)) SDL_GL_GetProcAddress("glGetUniformLocationARB");
			qglUniform1fARB = (void (APIENTRY *)(GLint, GLfloat)) SDL_GL_GetProcAddress("glUniform1fARB");
			qglUniform1iARB = (void (APIENTRY *)(GLint, GLint)) SDL_GL_GetProcAddress("glUniform1iARB");

			if (!qglCreateShaderObjectARB  ||  !qglShaderSourceARB  ||  !qglCompileShaderARB  ||  !qglCreateProgramObjectARB  ||  !qglAttachObjectARB  ||  !qglLinkProgramARB  ||  !qglUseProgramObjectARB  ||  !qglGetObjectParameterivARB  || !qglGetInfoLogARB  ||  !qglGetObjectParameterivARB  ||  !qglDetachObjectARB  ||  !qglDeleteObjectARB  ||  !qglGetUniformLocationARB  ||  !qglUniform1fARB  ||  !qglUniform1iARB) {
				glConfig.glsl = qfalse;
				ri.Printf(PRINT_ALL, "^1...error: ignoring fragment shaders some proc addresses not found\n");
				ri.Printf(PRINT_ALL, "qglCreateShaderObjectARB %p\n", qglCreateShaderObjectARB);
				ri.Printf(PRINT_ALL, "qglShaderSourceARB %p\n", qglShaderSourceARB);
				ri.Printf(PRINT_ALL, "qglCompileShaderARB %p\n", qglCompileShaderARB);
				ri.Printf(PRINT_ALL, "qglCreateProgramObjectARB %p\n", qglCreateProgramObjectARB);
				ri.Printf(PRINT_ALL, "qglAttachObjectARB %p\n", qglAttachObjectARB);
				ri.Printf(PRINT_ALL, "qglLinkProgramARB %p\n", qglLinkProgramARB);
				ri.Printf(PRINT_ALL, "qglUseProgramObjectARB %p\n", qglUseProgramObjectARB);
				ri.Printf(PRINT_ALL, "qglGetObjectParameterivARB %p\n", qglGetObjectParameterivARB);
				ri.Printf(PRINT_ALL, "qglGetInfoLogARB %p\n", qglGetInfoLogARB);
				ri.Printf(PRINT_ALL, "qglGetObjectParameterivARB %p\n", qglGetObjectParameterivARB);
				ri.Printf(PRINT_ALL, "qglDetachObjectARB %p\n", qglDetachObjectARB);
				ri.Printf(PRINT_ALL, "qglDeleteObjectARB %p\n", qglDeleteObjectARB);
				ri.Printf(PRINT_ALL, "qglGetUniformLocationARB %p\n", qglGetUniformLocationARB);
				ri.Printf(PRINT_ALL, "qglUniform1fARB %p\n", qglUniform1fARB);
				ri.Printf(PRINT_ALL, "qglUniform1iARB %p\n", qglUniform1iARB);
			} else {
				ri.Printf(PRINT_ALL, "...using fragment shaders\n");
			}
		} else {
			glConfig.glsl = qfalse;
			ri.Printf(PRINT_ALL, "...ignoring fragment shaders\n");
		}
	} else {
		glConfig.glsl = qfalse;
		qglUniform1fARB = NULL;

		ri.Printf(PRINT_ALL, "...no fragment shader support\n");
	}

	if (GLimp_HaveExtension("GL_EXT_framebuffer_object")) {
		glConfig.fbo = qtrue;

		//qglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &n);
		//Com_Printf("^2max renderbuffer size %d\n", n);
		qglGenFramebuffersEXT = SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		qglDeleteFramebuffersEXT = SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		qglBindFramebufferEXT = SDL_GL_GetProcAddress("glBindFramebufferEXT");
		qglFramebufferTexture2DEXT = SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
		qglCheckFramebufferStatusEXT = SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");

		qglGenRenderbuffersEXT = SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
		qglDeleteRenderbuffersEXT = SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
		qglBindRenderbufferEXT = SDL_GL_GetProcAddress("glBindRenderbufferEXT");
		qglFramebufferRenderbufferEXT = SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
		qglRenderbufferStorageEXT = SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
		if (!qglGenFramebuffersEXT  ||  !qglDeleteFramebuffersEXT  ||  !qglBindFramebufferEXT  ||  !qglFramebufferTexture2DEXT  ||  !qglCheckFramebufferStatusEXT  ||  !qglGenRenderbuffersEXT  ||  !qglDeleteRenderbuffersEXT  ||  !qglBindRenderbufferEXT  ||  !qglFramebufferRenderbufferEXT  ||  !qglRenderbufferStorageEXT) {
			glConfig.fbo = qfalse;
			ri.Printf(PRINT_ALL, "^1...error: some framebuffer object proc addresses not found\n");
			PrintPtrAddr(qglGenFramebuffersEXT);
			PrintPtrAddr(qglDeleteFramebuffersEXT);
			PrintPtrAddr(qglBindFramebufferEXT);
			PrintPtrAddr(qglFramebufferTexture2DEXT);
			PrintPtrAddr(qglCheckFramebufferStatusEXT);
			PrintPtrAddr(qglGenRenderbuffersEXT);
			PrintPtrAddr(qglDeleteRenderbuffersEXT);
			PrintPtrAddr(qglBindRenderbufferEXT);
			PrintPtrAddr(qglFramebufferRenderbufferEXT);
			PrintPtrAddr(qglRenderbufferStorageEXT);
		} else {
			ri.Printf(PRINT_ALL, "...using GL_EXT_framebuffer_object\n");
		}

		if (r_fboStencil->integer) {
			if (GLimp_HaveExtension("GL_EXT_packed_depth_stencil")) {
				glConfig.fboStencil = qtrue;
				ri.Printf(PRINT_ALL, "...using GL_EXT_packed_depth_stencil\n");
			} else {
				glConfig.fboStencil = qfalse;
				ri.Printf(PRINT_ALL, "...ignoring GL_EXT_packed_depth_stencil\n");
			}
		} else {
			glConfig.fboStencil = qfalse;
		}
		if (GLimp_HaveExtension("GL_EXT_framebuffer_multisample")  &&  GLimp_HaveExtension("GL_EXT_framebuffer_blit")) {
			glConfig.fboMultiSample = qtrue;
			qglRenderbufferStorageMultisampleEXT = SDL_GL_GetProcAddress("glRenderbufferStorageMultisampleEXT");
			if (!qglRenderbufferStorageMultisampleEXT) {
				glConfig.fboMultiSample = qfalse;
				ri.Printf(PRINT_ALL, "^1error glRenderbufferStorageMultisampleEXT not found\n");
			} else {
				ri.Printf(PRINT_ALL, "...using GL_EXT_framebuffer_multisample\n");
			}
			qglBlitFramebufferEXT = SDL_GL_GetProcAddress("glBlitFramebufferEXT");
			if (!qglBlitFramebufferEXT) {
				glConfig.fboMultiSample = qfalse;
				ri.Printf(PRINT_ALL, "^1error glBlitFramebufferEXT not found\n");
			} else {
				ri.Printf(PRINT_ALL, "...using GL_EXT_framebuffer_blit\n");
			}
		} else {
			glConfig.fboMultiSample = qfalse;
			ri.Printf(PRINT_ALL, "...GL_EXT_framebuffer_multisample not found\n");
		}
	} else {
		glConfig.fbo = qfalse;
		ri.Printf(PRINT_ALL, "...no framebuffer object support\n");
	}
}

#define R_MODE_FALLBACK 3 // 640 * 480

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( void )
{
	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

#if defined( _WIN32)
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "directx", CVAR_ROM );
#elif defined(__linux__)
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "x11", CVAR_ROM );
#elif defined(__APPLE__)
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "Quartz", CVAR_ROM );
#else
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
#endif

	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_visibleWindowWidth = ri.Cvar_Get("r_visibleWindowWidth", "", CVAR_ARCHIVE | CVAR_LATCH);
	r_visibleWindowHeight = ri.Cvar_Get("r_visibleWindowHeight", "", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboStencil = ri.Cvar_Get("r_fboStencil", "1", CVAR_ARCHIVE | CVAR_LATCH);

	Sys_SetEnv( "SDL_VIDEO_CENTERED", r_centerWindow->integer ? "1" : "" );

	Sys_GLimpInit( );

	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer))
		goto success;

	// Try again, this time in a platform specific "safe mode"
	ri.Printf(PRINT_ALL, "^1GLimp_StartDriverAndSetMode() failed, reverting to safe values\n");
	GLimp_Shutdown();
	Cvar_ForceReset("r_mode");
	Cvar_ForceReset("r_fullscreen");
	Cvar_ForceReset("r_colorbits");
	Cvar_ForceReset("r_depthbits");
	Cvar_ForceReset("r_stencilbits");
	Cvar_ForceReset("r_ext_multisample");
	Cvar_ForceReset("r_stereoEnabled");
	Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse))
		goto success;

	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n",
				r_mode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, r_fullscreen->integer, qfalse))
			goto success;
	}

	// Nothing worked, give up
	ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );

success:
	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	if ( -1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = 1;

	if ( 1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = 0;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );

	if (sscanf(glConfig.version_string, "%d.%d", &glConfig.openGLMajorVersion, &glConfig.openGLMinorVersion) != 2) {
		glConfig.openGLMajorVersion = glConfig.openGLMinorVersion = 0;
	}

	ExtensionString = (const char *)qglGetString(GL_EXTENSIONS);
	Q_strncpyz( glConfig.extensions_string, ExtensionString, sizeof( glConfig.extensions_string ) );

	// initialize extensions
	GLimp_InitExtensions( );

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init( );
}


static void check_mode (void)
{
	//return;
	//ri.Printf(PRINT_ALL, "r_mode modified\n");
#if 0
	if (r_mode->modified) {
		//SDL_Init(SDL_INIT_VIDEO);
		ri.Printf(PRINT_ALL, "change mode %d %d %d\n", r_mode->integer, r_fullscreen->integer, r_noborder->integer);

		SDL_Init(SDL_INIT_VIDEO);
		videoInfo = NULL;
		// check that it gets saved in config
		if (!GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer)) {

		//if (0) {  //(GLimp_SetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer) != RSERR_OK) {
			ri.Printf(PRINT_ALL, "couldn't change mode '%s'\n", SDL_GetError());
		} else {
			//videoInfo = NULL;
			//GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer 
				//, r_noborder->integer);
		}

		r_mode->modified = qfalse;
	}
#endif
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapBuffers();
	}

	check_mode();

	if( r_fullscreen->modified )
	{
		qboolean    fullscreen;
		qboolean    needToToggle = qtrue;
		qboolean    sdlToggled = qfalse;
		SDL_Surface *s = SDL_GetVideoSurface( );

		if( s )
		{
			// Find out the current state
			fullscreen = !!( s->flags & SDL_FULLSCREEN );

			if( r_fullscreen->integer && Cvar_VariableIntegerValue( "in_nograb" ) )
			{
				ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
				ri.Cvar_Set( "r_fullscreen", "0" );
				r_fullscreen->modified = qfalse;
			}

			// Is the state we want different from the current state?
			needToToggle = !!r_fullscreen->integer != fullscreen;

			if( needToToggle )
				sdlToggled = SDL_WM_ToggleFullScreen( s );
		}

		if( needToToggle )
		{
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
				Cbuf_AddText( "vid_restart\n" );

			IN_Restart( );
		}

		r_fullscreen->modified = qfalse;
	}
}



#ifdef SMP
/*
===========================================================

SMP acceleration

===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 * thread-safe OpenGL libraries, and it looks like the original Linux
 * code counted on each thread claiming the GL context with glXMakeCurrent(),
 * which you can't currently do in SDL. We'll just have to hope for the best.
 */

static SDL_mutex *smpMutex = NULL;
static SDL_cond *renderCommandsEvent = NULL;
static SDL_cond *renderCompletedEvent = NULL;
static void (*glimpRenderThread)( void ) = NULL;
static SDL_Thread *renderThread = NULL;

/*
===============
GLimp_ShutdownRenderThread
===============
*/
static void GLimp_ShutdownRenderThread(void)
{
	if (smpMutex != NULL)
	{
		SDL_DestroyMutex(smpMutex);
		smpMutex = NULL;
	}

	if (renderCommandsEvent != NULL)
	{
		SDL_DestroyCond(renderCommandsEvent);
		renderCommandsEvent = NULL;
	}

	if (renderCompletedEvent != NULL)
	{
		SDL_DestroyCond(renderCompletedEvent);
		renderCompletedEvent = NULL;
	}

	glimpRenderThread = NULL;
}

/*
===============
GLimp_RenderThreadWrapper
===============
*/
static int GLimp_RenderThreadWrapper( void *arg )
{
	Com_Printf(PRINT_ALL,  "Render thread starting\n" );

	glimpRenderThread();

	GLimp_SetCurrentContext(NULL);

	Com_Printf( "Render thread terminating\n" );

	return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	static qboolean warned = qfalse;
	if (!warned)
	{
		Com_Printf("WARNING: You enable r_smp at your own risk!\n");
		warned = qtrue;
	}

#ifndef MACOS_X
	return qfalse;  /* better safe than sorry for now. */
#endif

	if (renderThread != NULL)  /* hopefully just a zombie at this point... */
	{
		Com_Printf("Already a render thread? Trying to clean it up...\n");
		SDL_WaitThread(renderThread, NULL);
		renderThread = NULL;
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();
	if (smpMutex == NULL)
	{
		Com_Printf( "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCommandsEvent = SDL_CreateCond();
	if (renderCommandsEvent == NULL)
	{
		Com_Printf( "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCompletedEvent = SDL_CreateCond();
	if (renderCompletedEvent == NULL)
	{
		Com_Printf( "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	glimpRenderThread = function;
	renderThread = SDL_CreateThread(GLimp_RenderThreadWrapper, NULL);
	if ( renderThread == NULL )
	{
		ri.Printf( PRINT_ALL, "SDL_CreateThread() returned %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	else
	{
		// tma 01/09/07: don't think this is necessary anyway?
		//
		// !!! FIXME: No detach API available in SDL!
		//ret = pthread_detach( renderThread );
		//if ( ret ) {
		//ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		//}
	}

	return qtrue;
}

static volatile void    *smpData = NULL;
static volatile qboolean smpDataReady;

/*
===============
GLimp_RendererSleep
===============
*/
void *GLimp_RendererSleep( void )
{
	void  *data = NULL;

	GLimp_SetCurrentContext(NULL);

	SDL_LockMutex(smpMutex);
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal(renderCompletedEvent);

		while ( !smpDataReady )
			SDL_CondWait(renderCommandsEvent, smpMutex);

		data = (void *)smpData;
	}
	SDL_UnlockMutex(smpMutex);

	GLimp_SetCurrentContext(opengl_context);

	return data;
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep( void )
{
	SDL_LockMutex(smpMutex);
	{
		while ( smpData )
			SDL_CondWait(renderCompletedEvent, smpMutex);
	}
	SDL_UnlockMutex(smpMutex);

	GLimp_SetCurrentContext(opengl_context);
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer( void *data )
{
	GLimp_SetCurrentContext(NULL);

	SDL_LockMutex(smpMutex);
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal(renderCommandsEvent);
	}
	SDL_UnlockMutex(smpMutex);
}

#else

// No SMP - stubs
void GLimp_RenderThreadWrapper( void *arg )
{
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
	return qfalse;
}

void *GLimp_RendererSleep( void )
{
	return NULL;
}

void GLimp_FrontEndSleep( void )
{
}

void GLimp_WakeRenderer( void *data )
{
}

#endif

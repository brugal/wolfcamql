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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "../renderercommon/tr_mme.h"
#include "../client/cl_avi.h"

glconfig_t  glConfig;
qboolean    textureFilterAnisotropic = qfalse;
int         maxAnisotropy = 0;
float       displayAspect = 0.0f;
qboolean    haveClampToEdge = qfalse;

glstate_t	glState;

static void GfxInfo_f( void );

#ifdef USE_RENDERER_DLOPEN
cvar_t	*com_altivec;
#endif

cvar_t	*r_flareSize;
cvar_t	*r_flareFade;
cvar_t	*r_flareCoeff;

cvar_t	*r_railWidth;
cvar_t	*r_railCoreWidth;
cvar_t	*r_railSegmentLength;

cvar_t	*r_ignoreFastPath;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;
cvar_t	*r_zproj;
cvar_t	*r_stereoSeparation;

cvar_t	*r_skipBackEnd;

cvar_t	*r_stereoEnabled;
cvar_t	*r_anaglyphMode;
//cvar_t *r_anaglyphModeFullRgb;
cvar_t	*r_anaglyph2d;

cvar_t	*r_greyscale;
cvar_t  *r_greyscaleValue;
cvar_t  *r_mapGreyScale;
cvar_t	*r_picmipGreyScale;
cvar_t	*r_picmipGreyScaleValue;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t *r_fastSkyColor;
cvar_t *r_drawSkyFloor;
cvar_t *r_cloudHeight;
cvar_t *r_cloudHeightOrig;
cvar_t *r_forceSky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_max_anisotropy;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t	*r_drawBuffer;
cvar_t	*r_lightmap;
cvar_t *r_lightmapColor;
cvar_t *r_darknessThreshold;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t *r_singleShaderName;
cvar_t	*r_roundImagesDown;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t *r_ignoreShaderNoMipMaps;
cvar_t *r_ignoreShaderNoPicMip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t *r_clearColor;
cvar_t	*r_swapInterval;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t *r_polygonFill;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen;
cvar_t  *r_noborder;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;
cvar_t	*r_customPixelAspect;

cvar_t	*r_overBrightBits;
cvar_t *r_overBrightBitsValue;
cvar_t	*r_mapOverBrightBits;
cvar_t *r_mapOverBrightBitsValue;
cvar_t	*r_mapOverBrightBitsCap;
//cvar_t *r_mapColor;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;
cvar_t *r_debugFonts;
cvar_t *r_defaultQlFontFallbacks;
cvar_t *r_defaultMSFontFallbacks;
cvar_t *r_defaultSystemFontFallbacks;
cvar_t *r_defaultUnifontFallbacks;

cvar_t	*r_marksOnTriangleMeshes;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

cvar_t *r_jpegCompressionQuality;
cvar_t *r_pngZlibCompression;
cvar_t *r_forceMap;

cvar_t *r_enablePostProcess;
cvar_t *r_enableColorCorrect;
cvar_t *r_contrast;
cvar_t *r_enableBloom;

//FIXME find out exactly what these three do
cvar_t *r_BloomBlurScale;
cvar_t *r_BloomBlurFalloff;
cvar_t *r_BloomBlurRadius;

cvar_t *r_BloomPasses;
cvar_t *r_BloomSceneIntensity;
cvar_t *r_BloomSceneSaturation;
cvar_t *r_BloomIntensity;
cvar_t *r_BloomSaturation;
cvar_t *r_BloomBrightThreshold;

cvar_t *r_BloomTextureScale;
cvar_t *r_BloomDebug;

cvar_t *r_portalBobbing;
cvar_t *r_useFbo;
cvar_t *r_fboAntiAlias;
cvar_t *r_teleporterFlash;
cvar_t *r_debugMarkSurface;
cvar_t *r_ignoreNoMarks;
cvar_t *r_fog;
cvar_t *r_ignoreEntityMergable;
cvar_t *r_debugScaledImages;
cvar_t *r_scaleImagesPowerOfTwo;
cvar_t *r_screenMapTextureSize;
cvar_t *r_weather;

static void R_DeleteQLGlslShadersAndPrograms (void);

static void InitFrameBufferAndRenderBuffer (void)
{
	GLenum target;
	GLenum err;
	GLenum status;
	int width;
	int height;
	GLint maxSamples;
	int samples = 0;

	tr.usingFinalFrameBufferObject = qfalse;
	tr.usingMultiSample = qfalse;
	tr.usingFboStencil = qfalse;

	if (!r_useFbo->integer  ||  !glConfig.fbo) {
		return;
	}

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

#if 0  // pointless
	if (glConfig.maxTextureSize > 0  &&  (width  > glConfig.maxTextureSize  ||  height > glConfig.maxTextureSize)) {
		ri.Printf(PRINT_ALL, "^1can't create framebuffer object, selected fbo width (%d) or height (%d) is greater than the video card's max texture size (%d)\n", width, height, glConfig.maxTextureSize);
		return;
	}
	//ri.Printf(PRINT_ALL, "max texture size: %d\n", glConfig.maxTextureSize);
#endif

	GL_SelectTextureUnit(0);

	target = GL_TEXTURE_RECTANGLE_ARB;

	qglDisable(GL_TEXTURE_2D);
	qglEnable(target);

	// create framebuffer object
	qglGenFramebuffersEXT(1, &tr.frameBuffer);
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBuffer);

	// create scene texture
	qglGenTextures(1, &tr.sceneTexture);
	qglBindTexture(target, tr.sceneTexture);
	qglTexImage2D(target, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	err = qglGetError();
	if (err != GL_NO_ERROR) {
		ri.Printf(PRINT_ALL, "^1opengl error creating offscreen render texture:  0x%x\n", err);
	}
	qglTexParameteri(target, GL_TEXTURE_WRAP_S, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_WRAP_T, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, tr.sceneTexture, 0);

	// create depth texture
	if (glConfig.fboStencil) {
		tr.usingFboStencil = qtrue;
		qglGenTextures(1, &tr.depthTexture);
		qglBindTexture(target, tr.depthTexture);
		qglTexImage2D(target, 0, GL_DEPTH24_STENCIL8_EXT, width, height, 0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, NULL);
		err = qglGetError();
		if (err != GL_NO_ERROR) {
			ri.Printf(PRINT_ALL, "^1opengl error creating offscreen depth/stencil texture:  0x%x\n", err);
		}
		qglTexParameteri(target, GL_TEXTURE_WRAP_S, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
		qglTexParameteri(target, GL_TEXTURE_WRAP_T, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
		qglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, target, tr.depthTexture, 0);
        qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, target, tr.depthTexture, 0);
	} else {
		qglGenRenderbuffersEXT(1, &tr.depthBuffer);
		qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, tr.depthBuffer);
		qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, tr.depthBuffer);
	}

	status = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
		tr.usingFinalFrameBufferObject = qtrue;
	} else {
		ri.Printf(PRINT_ALL, "^1%s  framebuffer error: 0x%x\n", __FUNCTION__, status);
		switch(status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			ri.Printf(PRINT_ALL, "^1attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			ri.Printf(PRINT_ALL, "^1missing attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			ri.Printf(PRINT_ALL, "^1dimensions\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			ri.Printf(PRINT_ALL, "^1formats\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			ri.Printf(PRINT_ALL, "^1draw buffer\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			ri.Printf(PRINT_ALL, "^1read buffer\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			ri.Printf(PRINT_ALL, "^1unsupported\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
			ri.Printf(PRINT_ALL, "^1incomplete multisample buffer\n");
			break;
		default:
			ri.Printf(PRINT_ALL, "^1unknown error\n");
		}

		tr.usingFinalFrameBufferObject = qfalse;
	}


	err = qglGetError();
	if (err != GL_NO_ERROR) {
		ri.Printf(PRINT_ALL, "^1opengl error end of framebuffer init:  0x%x\n", err);
	}

	if (!glConfig.fboMultiSample  ||  !tr.usingFinalFrameBufferObject) {
		qglDisable(target);
		qglEnable(GL_TEXTURE_2D);
		return;
	}

	qglGetIntegerv(GL_MAX_SAMPLES_EXT, &maxSamples);
	samples = r_fboAntiAlias->integer;
	if (samples > maxSamples) {
		samples = maxSamples;
	}
	if (samples < 0) {
		samples = 0;
	}

	if (samples <= 0) {
		qglDisable(target);
		qglEnable(GL_TEXTURE_2D);
		return;
	}

	////////////////////////////////////////////

	qglGenFramebuffersEXT(1, &tr.frameBufferMultiSample);
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBufferMultiSample);

	qglGenRenderbuffersEXT(1, &tr.renderBufferMultiSample);
	qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, tr.renderBufferMultiSample);

	ri.Printf(PRINT_ALL, "anti-alias samples: %d\n", samples);
	qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_RGB8, width, height);
	qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, tr.renderBufferMultiSample);

	err = qglGetError();
	if (err != GL_NO_ERROR) {
		ri.Printf(PRINT_ALL, "^1opengl error creating framebuffer:  0x%x\n", err);
	}

	qglGenRenderbuffersEXT(1, &tr.depthBufferMultiSample);
	qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, tr.depthBufferMultiSample);
	if (glConfig.fboStencil) {
		qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_DEPTH24_STENCIL8_EXT, width, height);
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, tr.depthBufferMultiSample);
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, tr.depthBufferMultiSample);
	} else {
		qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_DEPTH_COMPONENT24, width, height);
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, tr.depthBufferMultiSample);
	}

	err = qglGetError();
	if (err) {
		switch(err) {
		case GL_INVALID_VALUE:
			ri.Printf(PRINT_ALL, "^1opengl error creating multisample depth render buffer: (invalid values for width and/or height)\n");
			break;
		case GL_OUT_OF_MEMORY:
			ri.Printf(PRINT_ALL, "^1opengl error creating multisample depth render buffer: (gl out of memory)\n");
			break;
		default:
			ri.Printf(PRINT_ALL, "^1opengl error creating multisample depth render buffer:  0x%x\n", err);
		}
	}

	status = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
		tr.usingFinalFrameBufferObject = qtrue;
		tr.usingMultiSample = qtrue;
	} else {
		ri.Printf(PRINT_ALL, "^1%s  multisample framebuffer error: 0x%x\n", __FUNCTION__, status);
		switch(status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			ri.Printf(PRINT_ALL, "^1attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			ri.Printf(PRINT_ALL, "^1missing attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			ri.Printf(PRINT_ALL, "^1dimensions\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			ri.Printf(PRINT_ALL, "^1formats\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			ri.Printf(PRINT_ALL, "^1draw buffer\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			ri.Printf(PRINT_ALL, "^1read buffer\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			ri.Printf(PRINT_ALL, "^1unsupported\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
			ri.Printf(PRINT_ALL, "^1incomplete multisample buffer\n");
			break;
		default:
			ri.Printf(PRINT_ALL, "^1unknown error\n");
		}

		tr.usingFinalFrameBufferObject = qfalse;
		tr.usingMultiSample = qfalse;
	}

	qglDisable(target);
	qglEnable(GL_TEXTURE_2D);

	err = qglGetError();
	if (err != GL_NO_ERROR) {
		ri.Printf(PRINT_ALL, "^1opengl error end of multisample framebuffer init:  0x%x\n", err);
	}
}

static void printGlslLog (GLhandleARB obj)
{
    int infoLogLength = 0;
    char infoLog[1024];
	int len;

	qglGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infoLogLength);
    if (infoLogLength > 0) {
		qglGetInfoLogARB(obj, 1024, &len, infoLog);
        ri.Printf(PRINT_ALL, "%s\n", infoLog);
	}
}

static char ShaderExtensions[] = "#version 110\n#extension GL_ARB_texture_rectangle : enable";

static void R_InitFragmentShader (const char *filename, GLhandleARB *fragmentShader, GLhandleARB *program, GLhandleARB vertexShader)
{
	int len;
	int slen;
	void *shaderSource;
	char *text;

	if (!glConfig.qlGlsl) {
		return;
	}

	ri.Printf(PRINT_ALL, "^5%s ->\n", filename);
	*fragmentShader = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	len = ri.FS_ReadFile(filename, &shaderSource);
	if (len <= 0) {
		ri.Printf(PRINT_ALL, "^1couldn't find file\n");
		R_DeleteQLGlslShadersAndPrograms();
		glConfig.qlGlsl = qfalse;
		return;
	}
	slen = strlen(ShaderExtensions);
	text = (char *)malloc(len + slen + 3);
	if (!text) {
		ri.Printf(PRINT_ALL, "R_InitFragmentShader() couldn't allocate memory for glsl shader file\n");
		ri.FS_FreeFile(shaderSource);
		R_DeleteQLGlslShadersAndPrograms();
		glConfig.qlGlsl = qfalse;
		return;
	}
	Com_sprintf(text, len + slen + 3, "%s\n%s\n", ShaderExtensions, (char *)shaderSource);
	//qglShaderSourceARB(*fragmentShader, 1, (const char **)&shaderSource, NULL);
	qglShaderSourceARB(*fragmentShader, 1, (const char **)&text, NULL);
	qglCompileShaderARB(*fragmentShader);
	printGlslLog(*fragmentShader);
	ri.FS_FreeFile(shaderSource);
	free(text);

	*program = qglCreateProgramObjectARB();
	qglAttachObjectARB(*program, vertexShader);
	qglAttachObjectARB(*program, *fragmentShader);
	qglLinkProgramARB(*program);
	printGlslLog(*program);
	//ri.Printf(PRINT_ALL, "\n");
}

static void InitQLGlslShadersAndPrograms (void)
{
	void *shaderSource;
	int len;
	GLenum target;
	float bloomTextureScale;

	if (!r_enablePostProcess->integer  ||  !glConfig.qlGlsl) {
		return;
	}

	GL_SelectTextureUnit(0);
	qglDisable(GL_TEXTURE_2D);
	qglEnable(GL_TEXTURE_RECTANGLE_ARB);

	bloomTextureScale = r_BloomTextureScale->value;
	if (bloomTextureScale < 0.01) {
		bloomTextureScale = 0.01;
	}
	if (bloomTextureScale > 1) {
		bloomTextureScale = 1;
	}
	target = GL_TEXTURE_RECTANGLE_ARB;
	tr.bloomWidth = glConfig.vidWidth * bloomTextureScale;
	tr.bloomHeight = glConfig.vidHeight * bloomTextureScale;
	qglGenTextures(1, &tr.bloomTexture);
	qglBindTexture(target, tr.bloomTexture);
	qglTexImage2D(target, 0, GL_RGBA8, tr.bloomWidth, tr.bloomHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	qglTexParameteri(target, GL_TEXTURE_WRAP_S, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_WRAP_T, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	target = GL_TEXTURE_RECTANGLE_ARB;
	qglGenTextures(1, &tr.backBufferTexture);
	qglBindTexture(target, tr.backBufferTexture);

	qglTexImage2D(target, 0, GL_RGB8, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	qglTexParameteri(target, GL_TEXTURE_WRAP_S, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_WRAP_T, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	qglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	qglDisable(GL_TEXTURE_RECTANGLE_ARB);
	qglEnable(GL_TEXTURE_2D);

	GL_SelectTextureUnit(0);

	ri.Printf(PRINT_ALL, "^5scripts/posteffect.vs ->\n");
	len = ri.FS_ReadFile("scripts/posteffect.vs", &shaderSource);

	if (len > 0) {
		char *text;
		int slen;

		slen = strlen(ShaderExtensions);
		text = (char *)malloc(len + slen + 3);
		if (!text) {
			ri.Printf(PRINT_ALL, "InitQLGlslShadersAndPrograms() couldn't allocate memory for posteffect.vs\n");
			ri.FS_FreeFile(shaderSource);
			R_DeleteQLGlslShadersAndPrograms();
			glConfig.qlGlsl = qfalse;
			return;
		}

		Com_sprintf(text, len + slen + 3, "%s\n%s\n", ShaderExtensions, (char *)shaderSource);
		tr.mainVs = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(tr.mainVs, 1, (const char **)&text, NULL);
		qglCompileShaderARB(tr.mainVs);
		printGlslLog(tr.mainVs);
		ri.FS_FreeFile(shaderSource);
		free(text);
	} else {
		ri.Printf(PRINT_ALL, "^1file not found\n");
		R_DeleteQLGlslShadersAndPrograms();
		glConfig.qlGlsl = qfalse;
	}

	R_InitFragmentShader("scripts/colorcorrect.fs", &tr.colorCorrectFs, &tr.colorCorrectSp, tr.mainVs);
	R_InitFragmentShader("scripts/blurhoriz.fs", &tr.blurHorizFs, &tr.blurHorizSp, tr.mainVs);
	R_InitFragmentShader("scripts/blurvertical.fs", &tr.blurVerticalFs, &tr.blurVerticalSp, tr.mainVs);
	R_InitFragmentShader("scripts/brightpass.fs", &tr.brightPassFs, &tr.brightPassSp, tr.mainVs);
	R_InitFragmentShader("scripts/combine.fs", &tr.combineFs, &tr.combineSp, tr.mainVs);
	R_InitFragmentShader("scripts/downsample1.fs", &tr.downSample1Fs, &tr.downSample1Sp, tr.mainVs);
}

static void R_DeleteQLGlslShadersAndPrograms (void)
{
	if (!r_enablePostProcess->integer  ||  !glConfig.qlGlsl) {
		return;
	}

	// detach

	if (tr.blurHorizSp) {
		if (tr.blurHorizFs) {
			qglDetachObjectARB(tr.blurHorizSp, tr.blurHorizFs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.blurHorizSp, tr.mainVs);
		}
	}

	if (tr.blurVerticalSp) {
		if (tr.blurVerticalFs) {
			qglDetachObjectARB(tr.blurVerticalSp, tr.blurVerticalFs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.blurVerticalSp, tr.mainVs);
		}
	}

	if (tr.brightPassSp) {
		if (tr.brightPassFs) {
			qglDetachObjectARB(tr.brightPassSp, tr.brightPassFs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.brightPassSp, tr.mainVs);
		}
	}

	if (tr.downSample1Sp) {
		if (tr.downSample1Fs) {
			qglDetachObjectARB(tr.downSample1Sp, tr.downSample1Fs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.downSample1Sp, tr.mainVs);
		}
	}

	if (tr.combineSp) {
		if (tr.combineFs) {
			qglDetachObjectARB(tr.combineSp, tr.combineFs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.combineSp, tr.mainVs);
		}
	}

	if (tr.colorCorrectSp) {
		if (tr.colorCorrectFs) {
			qglDetachObjectARB(tr.colorCorrectSp, tr.colorCorrectFs);
		}
		if (tr.mainVs) {
			qglDetachObjectARB(tr.colorCorrectSp, tr.mainVs);
		}
	}

	// delete

	if (tr.blurHorizSp) {
		qglDeleteObjectARB(tr.blurHorizSp);
	}
	if (tr.blurHorizFs) {
		qglDeleteObjectARB(tr.blurHorizFs);
	}

	if (tr.blurVerticalSp) {
		qglDeleteObjectARB(tr.blurVerticalSp);
	}
	if (tr.blurVerticalFs) {
		qglDeleteObjectARB(tr.blurVerticalFs);
	}

	if (tr.brightPassSp) {
		qglDeleteObjectARB(tr.brightPassSp);
	}
	if (tr.brightPassFs) {
		qglDeleteObjectARB(tr.brightPassFs);
	}

	if (tr.downSample1Sp) {
		qglDeleteObjectARB(tr.downSample1Sp);
	}
	if (tr.downSample1Fs) {
		qglDeleteObjectARB(tr.downSample1Fs);
	}

	if (tr.combineSp) {
		qglDeleteObjectARB(tr.combineSp);
	}
	if (tr.combineFs) {
		qglDeleteObjectARB(tr.combineFs);
	}

	if (tr.colorCorrectSp) {
		qglDeleteObjectARB(tr.colorCorrectSp);
	}
	if (tr.colorCorrectFs) {
		qglDeleteObjectARB(tr.colorCorrectFs);
	}

	if (tr.mainVs) {
		qglDeleteObjectARB(tr.mainVs);
	}

	// reset

	tr.blurHorizSp = 0;
	tr.blurHorizFs = 0;
	tr.blurVerticalSp = 0;
	tr.blurVerticalFs = 0;
	tr.brightPassSp = 0;
	tr.brightPassFs = 0;
	tr.downSample1Sp = 0;
	tr.downSample1Fs = 0;
	tr.combineSp = 0;
	tr.combineFs = 0;
	tr.colorCorrectSp = 0;
	tr.colorCorrectFs = 0;
	tr.mainVs = 0;
}


static void R_DeleteFramebufferObject (void)
{
	if (!tr.usingFinalFrameBufferObject) {
		return;
	}

	if (!r_useFbo->integer  ||  !glConfig.fbo) {
		return;
	}

#if 0
	if (tr.frameBuffer) {
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBuffer);
	}
#endif
	if (tr.depthTexture) {
		qglDeleteTextures(1, &tr.depthTexture);
		tr.depthTexture = 0;
	}
	if (tr.depthBuffer) {
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
		qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		qglDeleteRenderbuffersEXT(1, &tr.depthBuffer);
		tr.depthBuffer = 0;
	}
	if (tr.depthBufferMultiSample) {
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
		qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		qglDeleteRenderbuffersEXT(1, &tr.depthBufferMultiSample);
		tr.depthBufferMultiSample = 0;
	}
	if (tr.renderBufferMultiSample) {
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, 0);
		qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		qglDeleteRenderbuffersEXT(1, &tr.renderBufferMultiSample);
		tr.renderBufferMultiSample = 0;
	}

	if (tr.frameBuffer) {
		// use default framebuffer
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		qglDeleteFramebuffersEXT(1, &tr.frameBuffer);
		tr.frameBuffer = 0;
	}
	if (tr.frameBufferMultiSample) {
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		qglDeleteFramebuffersEXT(1, &tr.frameBufferMultiSample);
		tr.frameBufferMultiSample = 0;
	}

	if (tr.sceneTexture) {
		qglDeleteTextures(1, &tr.sceneTexture);
		tr.sceneTexture = 0;
	}
}

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/

static void InitOpenGL( void )
{
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 )
	{
		GLint		temp;

		GLimp_Init( qtrue );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 )
		{
			glConfig.maxTextureSize = 0;
		}
	}

	InitFrameBufferAndRenderBuffer();
	InitQLGlslShadersAndPrograms();

	// set default state
	GL_SetDefaultState();

	// screwing around
	//qglEnable(GL_LIGHTING);
	//qglEnable(GL_LIGHT0);
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
	int		err;
	char	s[64];
	//static qboolean outOfMemory = qfalse;

	err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}

#if 0
	if (err == GL_OUT_OF_MEMORY) {
		ri.Printf(PRINT_ALL, "^1gl out of memory: window or framebuffer possibly too big\n");
		ri.Printf(PRINT_ALL, "^1setting r_mode 4, r_fullscreen 0, r_useFbo 0\n");
		ri.Cvar_Set("r_mode", "4");
		ri.Cvar_Set("r_fullscreen", "0");
		ri.Cvar_Set("r_useFbo", "0");

		if (outOfMemory) {
			Cbuf_ExecuteText(EXEC_NOW, "quit\n");
		} else {
			Cbuf_ExecuteText(EXEC_NOW, "vid_restart\n");
		}

		outOfMemory = qtrue;
		return;
	}
#endif

	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch( err ) {
		case GL_INVALID_ENUM:
			strcpy( s, "GL_INVALID_ENUM" );
			break;
		case GL_INVALID_VALUE:
			strcpy( s, "GL_INVALID_VALUE" );
			break;
		case GL_INVALID_OPERATION:
			strcpy( s, "GL_INVALID_OPERATION" );
			break;
		case GL_STACK_OVERFLOW:
			strcpy( s, "GL_STACK_OVERFLOW" );
			break;
		case GL_STACK_UNDERFLOW:
			strcpy( s, "GL_STACK_UNDERFLOW" );
			break;
		case GL_OUT_OF_MEMORY:
			strcpy( s, "GL_OUT_OF_MEMORY" );
			break;
		default:
			Com_sprintf( s, sizeof(s), "%i", err);
			break;
	}

	ri.Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480",		640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480 (wide)",856,	480,	1 }
};
static int      s_numVidModes = ARRAY_LEN( r_vidModes );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode ) {
	vidmode_t	*vm;
	float			pixelAspect;

	if ( mode < -2 ) {
		return qfalse;
	}
	if (mode == -2) {
		return qtrue;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	} else {
		vm = &r_vidModes[mode];

		*width  = vm->width;
		*height = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / ( *height * pixelAspect );

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	ri.Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

three commands: "screenshot", "screenshotJPEG", and "screenshotPNG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg, etc.) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/* 
================== 
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
================== 
*/  

#if 0  // unused in wolfcamql, alignment is set with qglPixelstorei(GL_PACK_ALIGNMENT, 1)
byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;
	
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
	
	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);
	
	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);
	
	bufstart = PADP((intptr_t) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);
	
	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;
	
	return buffer;
}
#endif

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;
	int			i, c, temp;

	buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3+18);

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	if (tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	} else {
		//qglReadBuffer(GL_FRONT);
	}

	qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_BACK);
	}

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for (i=18 ; i<c ; i+=3) {
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	ri.FS_WriteFile( fileName, buffer, c );
	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/
void RB_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;

	buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight * 3);

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_FRONT);
	}

	qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer );

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_BACK);
	}

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	ri.FS_WriteFile( fileName, buffer, 1 );		// create path
	RE_SaveJPG(fileName, r_jpegCompressionQuality->integer, glConfig.vidWidth, glConfig.vidHeight, buffer, 0);

	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotPNG
==================
*/
void RB_TakeScreenshotPNG (int x, int y, int width, int height, char *fileName)
{
	byte		*buffer;

	buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*4);

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_FRONT);
	}

	qglReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_BACK);
	}

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 4 );
	}

	ri.FS_WriteFile( fileName, buffer, 1 );		// create path
	//RE_SaveJPG(fileName, r_jpegCompressionQuality->integer, glConfig.vidWidth, glConfig.vidHeight, buffer, 0);
	SavePNG(fileName, buffer, glConfig.vidWidth, glConfig.vidHeight, 4);
	ri.Hunk_FreeTempMemory( buffer );
}


/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
	const screenshotCommand_t	*cmd;

	cmd = (const screenshotCommand_t *)data;

	if (cmd->type == SCREENSHOT_JPEG) {
		RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	} else if (cmd->type == SCREENSHOT_PNG) {
		RB_TakeScreenshotPNG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	} else {  // SCREENSHOT_TGA
		RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	}

	return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, int type ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->type = type;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d );
}

/*
==================
R_ScreenshotFilenameJPEG
==================
*/
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
		, a, b, c, d );
}

//FIXME duplicate code screenshot filename jpeg
/*
==================
R_ScreenshotFilenamePNG
==================
*/
void R_ScreenshotFilenamePNG (int lastNumber, char *fileName)
{
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.png" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.png"
		, a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	Com_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	source = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = ri.Hunk_AllocateTempMemory( 128 * 128*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_FRONT);
	}

	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source );

	if (!tr.usingFinalFrameBufferObject) {
		//qglReadBuffer(GL_BACK);
	}

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f (void) {
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber >= 9999 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SCREENSHOT_TGA );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

void R_ScreenShotJPEG_f (void)
{
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SCREENSHOT_JPEG );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

//FIXME duplicate code with jpeg screenshot
void R_ScreenShotPNG_f (void)
{
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.png", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenamePNG( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SCREENSHOT_PNG );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

//============================================================================

// RB_TakeVideoFrameCmd
#include "../renderercommon/inc_tr_init.c"


//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth( 1.0f );
	qglDepthRange(0, 1);

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTextureUnit( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTextureUnit( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

	/* ioquake3 remove:  2013-10-08-01  Revert "5097 - Menu corrupted on start (on IRIX)"

Calling glClear( any bits ) in GL_SetDefaultState makes connect screen black using the proprietary NVidia driver on Windows and GNU/Linux.

This reverts commit 9257327.

    /////////////

	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	qglClearDepth( 1.0 );
	qglDrawBuffer( GL_FRONT );
	qglClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ACCUM_BUFFER_BIT|GL_STENCIL_BUFFER_BIT );

	qglDrawBuffer( GL_BACK );
	qglClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ACCUM_BUFFER_BIT|GL_STENCIL_BUFFER_BIT );
    */

	// for screenshots and screen width not divisible by 4
	qglPixelStorei(GL_PACK_ALIGNMENT, 1);

	// for updating screen rectangle with width not divisible by 4
	qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
static void R_PrintLongString(const char *string) {
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0)
	{
		Q_strncpyz(buffer, p, sizeof (buffer) );
		ri.Printf( PRINT_ALL, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void )
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: " );
	// glConfig.extensions_string is a limited length so get the full list directly
	if ( qglGetStringi )
	{
		GLint numExtensions;
		int i;

		qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		for ( i = 0; i < numExtensions; i++ )
		{
			ri.Printf( PRINT_ALL, "%s ", qglGetStringi( GL_EXTENSIONS, i ) );
		}
	}
	else
	{
		R_PrintLongString( (char *) qglGetString( GL_EXTENSIONS ) );
	}
	ri.Printf( PRINT_ALL, "\n" );
	ri.Printf( PRINT_ALL, "\n" );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );

	//printf("%d  '%s'\n", strlen(glConfig.extensions_string), glConfig.extensions_string);

	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else
	{
		ri.Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			ri.Printf( PRINT_ALL, "none\n" );
		} else if ( primitives == 2 ) {
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
	{
		ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( glConfig.hardwareType == GLHW_RAGEPRO )
	{
		ri.Printf( PRINT_ALL, "HACK: ragePro approximations\n" );
	}
	if ( glConfig.hardwareType == GLHW_RIVA128 )
	{
		ri.Printf( PRINT_ALL, "HACK: riva128 approximations\n" );
	}
	if ( r_finish->integer ) {
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

void R_CreateColorSkins_f (void)
{
	R_CreatePlayerColorSkinImages(qtrue);
}

static void R_PrintViewParms_f (void)
{
	viewParms_t *p;
	int i;

	p = &tr.viewParms;

	ri.Printf(PRINT_ALL, "isPortal: %d\n", p->isPortal);
	ri.Printf(PRINT_ALL, "viewport (x, y, w, h):  %d %d %d %d\n", p->viewportX, p->viewportY, p->viewportWidth, p->viewportHeight);
	ri.Printf(PRINT_ALL, "fov x y:  %f %f\n", p->fovX, p->fovY);
	ri.Printf(PRINT_ALL, "projectionMatrix:\n");
	for (i = 0;  i < 16;  i++) {
		ri.Printf(PRINT_ALL, "%f ", p->projectionMatrix[i]);
	}
	ri.Printf(PRINT_ALL, "\n");

	ri.Printf(PRINT_ALL, "visBounds: (%f %f %f)  (%f %f %f)\n", p->visBounds[0][0], p->visBounds[0][1], p->visBounds[0][2], p->visBounds[1][0], p->visBounds[1][1], p->visBounds[1][2]);
	ri.Printf(PRINT_ALL, "zFar %f\n", p->zFar);
}

static void R_RemapLastTwoShaders_f (void)
{
	if (!*tr.markSurfaceNames[0]  &&  !*tr.markSurfaceNames[1]) {
		ri.Printf(PRINT_ALL, "couldn't remap:  need two surfaces\n");
		return;
	}

	R_RemapShader(tr.markSurfaceNames[1], tr.markSurfaceNames[0], "0", qtrue, qtrue);
}

/*
===============
R_Register
===============
*/
void R_Register( void )
{
	#ifdef USE_RENDERER_DLOPEN
	com_altivec = ri.Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
	#endif

	//
	// latched and archived variables
	//
	r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = ri.Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic",
			"0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_max_anisotropy = ri.Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );

	r_picmip = ri.Cvar_Get ("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignoreShaderNoMipMaps = ri.Cvar_Get ("r_ignoreShaderNoMipMaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreShaderNoPicMip = ri.Cvar_Get ("r_ignoreShaderNoPicMip", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_roundImagesDown = ri.Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri.Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	ri.Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_detailTextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multisample = ri.Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri.Cvar_CheckRange( r_ext_multisample, 0, 4, qtrue );
	r_overBrightBits = ri.Cvar_Get ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_overBrightBitsValue = ri.Cvar_Get("r_overBrightBitsValue", "1.0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri.Cvar_Get( "r_mode", "11", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE );
	r_noborder = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri.Cvar_Get( "r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = ri.Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri.Cvar_Get( "r_uiFullScreen", "0", 0);
	r_subdivisions = ri.Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereoEnabled = ri.Cvar_Get( "r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreFastPath = ri.Cvar_Get( "r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_greyscale = ri.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange(r_greyscale, 0, 1, qfalse);
	r_greyscaleValue = ri.Cvar_Get("r_greyscaleValue", "1.0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mapGreyScale = ri.Cvar_Get("r_mapGreyScale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_picmipGreyScale = ri.Cvar_Get("r_picmipGreyScale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_picmipGreyScaleValue = ri.Cvar_Get("r_picmipGreyScaleValue", "0.5", CVAR_ARCHIVE | CVAR_LATCH);

	//
	// temporary latched variables that can only change over a restart
	//
	r_displayRefresh = ri.Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH );
	ri.Cvar_CheckRange( r_displayRefresh, 0, 200, qtrue );

	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_mapOverBrightBits = ri.Cvar_Get ("r_mapOverBrightBits", "2", CVAR_LATCH | CVAR_ARCHIVE );
	r_mapOverBrightBitsValue = ri.Cvar_Get("r_mapOverBrightBitsValue", "1.0", CVAR_LATCH | CVAR_ARCHIVE);
	r_mapOverBrightBitsCap = ri.Cvar_Get("r_mapOverBrightBitsCap", "255", CVAR_LATCH | CVAR_ARCHIVE );
	//r_mapColor = ri.Cvar_Get("r_mapColor", "", CVAR_LATCH | CVAR_ARCHIVE);

	r_intensity = ri.Cvar_Get ("r_intensity", "1", CVAR_LATCH | CVAR_ARCHIVE );
	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_ARCHIVE);
	r_singleShaderName = ri.Cvar_Get("r_singleShaderName", "", CVAR_ARCHIVE);

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri.Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE);
	r_lodbias = ri.Cvar_Get( "r_lodbias", "-2", CVAR_ARCHIVE );
	r_flares = ri.Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	//ri.Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_zproj = ri.Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
	r_stereoSeparation = ri.Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
	r_ignoreGLErrors = ri.Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_fastSkyColor = ri.Cvar_Get("r_fastSkyColor", "", CVAR_ARCHIVE);
	r_drawSkyFloor = ri.Cvar_Get("r_drawSkyFloor", "1", CVAR_ARCHIVE);
	r_cloudHeight = ri.Cvar_Get("r_cloudHeight", "", CVAR_ARCHIVE);
	r_forceSky = ri.Cvar_Get("r_forceSky", "", CVAR_LATCH | CVAR_ARCHIVE);
	r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = ri.Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
	r_finish = ri.Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = ri.Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_swapInterval = ri.Cvar_Get( "r_swapInterval", "0",
					CVAR_ARCHIVE | CVAR_LATCH );
	r_gamma = ri.Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_facePlaneCull = ri.Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_railWidth = ri.Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = ri.Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE );
	r_railSegmentLength = ri.Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

	r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_ambientScale = ri.Cvar_Get( "r_ambientScale", "10", CVAR_ARCHIVE );
	r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_ARCHIVE );

	r_anaglyphMode = ri.Cvar_Get("r_anaglyphMode", "0", CVAR_ARCHIVE);
	//r_anaglyphModeFullRgb = ri.Cvar_Get("r_anaglyphModeFullRgb", "0", CVAR_ARCHIVE);
	r_anaglyph2d = ri.Cvar_Get("r_anaglyph2d", "0", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri.Cvar_Get( "r_saveFontData", "0", 0 );
	r_debugFonts = ri.Cvar_Get("r_debugFonts", "0", CVAR_ARCHIVE);
	r_defaultQlFontFallbacks = ri.Cvar_Get("r_defaultQlFontFallbacks", "1", CVAR_ARCHIVE);
	r_defaultMSFontFallbacks = ri.Cvar_Get("r_defaultMSFontFallbacks", "1", CVAR_ARCHIVE);
	r_defaultSystemFontFallbacks = ri.Cvar_Get("r_defaultSystemFontFallbacks", "1", CVAR_ARCHIVE);
	r_defaultUnifontFallbacks = ri.Cvar_Get("r_defaultUnifontFallbacks", "1", CVAR_ARCHIVE);

	r_nocurves = ri.Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	//r_lightmap = ri.Cvar_Get ("r_lightmap", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_lightmap = ri.Cvar_Get ("r_lightmap", "0", CVAR_ARCHIVE );
	r_lightmapColor = ri.Cvar_Get("r_lightmapColor", "", CVAR_ARCHIVE | CVAR_LATCH);
	r_darknessThreshold = ri.Cvar_Get("r_darknessThreshold", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_portalOnly = ri.Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = ri.Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri.Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);
	r_flareCoeff = ri.Cvar_Get ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

	r_skipBackEnd = ri.Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri.Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = ri.Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri.Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri.Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri.Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri.Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = ri.Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = ri.Cvar_Get ("r_clear", "0", CVAR_ARCHIVE);
	r_clearColor = ri.Cvar_Get("r_clearColor", "", CVAR_ARCHIVE);
	r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_polygonFill = ri.Cvar_Get("r_polygonFill", "1", CVAR_CHEAT);
	r_drawBuffer = ri.Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", 0 );

	r_marksOnTriangleMeshes = ri.Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

	r_maxpolys = ri.Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);
	r_jpegCompressionQuality = ri.Cvar_Get("r_jpegCompressionQuality", "90", CVAR_ARCHIVE);
	r_pngZlibCompression = ri.Cvar_Get("r_pngZlibCompression", "1", CVAR_ARCHIVE);
	r_forceMap = ri.Cvar_Get("r_forceMap", "", CVAR_ARCHIVE);
	r_enablePostProcess = ri.Cvar_Get("r_enablePostProcess", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_enableColorCorrect = ri.Cvar_Get("r_enableColorCorrect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_contrast = ri.Cvar_Get("r_contrast", "1.0", CVAR_ARCHIVE);
	r_enableBloom = ri.Cvar_Get("r_enableBloom", "0", CVAR_ARCHIVE);
	r_BloomBlurScale = ri.Cvar_Get("r_BloomBlurScale", "1.0", CVAR_ARCHIVE);
	r_BloomBlurFalloff = ri.Cvar_Get("r_BloomBlurFalloff", "0.75", CVAR_ARCHIVE);
	r_BloomBlurRadius = ri.Cvar_Get("r_BloomBlurRadius", "5", CVAR_ARCHIVE);
	r_BloomPasses = ri.Cvar_Get("r_BloomPasses", "1", CVAR_ARCHIVE);
	r_BloomSceneIntensity = ri.Cvar_Get("r_BloomSceneIntensity", "1.000", CVAR_ARCHIVE);
	r_BloomSceneSaturation = ri.Cvar_Get("r_BloomSceneSaturation", "1.000", CVAR_ARCHIVE);
	r_BloomIntensity = ri.Cvar_Get("r_BloomIntensity", "0.750", CVAR_ARCHIVE);
	r_BloomSaturation = ri.Cvar_Get("r_BloomSaturation", "0.800", CVAR_ARCHIVE);
	r_BloomBrightThreshold = ri.Cvar_Get("r_BloomBrightThreshold", "0.125", CVAR_ARCHIVE);
	r_BloomTextureScale = ri.Cvar_Get("r_BloomTextureScale", "0.5", CVAR_LATCH | CVAR_ARCHIVE);
	r_BloomDebug = ri.Cvar_Get("r_BloomDebug", "0", CVAR_ARCHIVE);
	r_portalBobbing = ri.Cvar_Get("r_portalBobbing", "1", CVAR_ARCHIVE);
	r_useFbo = ri.Cvar_Get("r_useFbo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboAntiAlias = ri.Cvar_Get("r_fboAntiAlias", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_teleporterFlash = ri.Cvar_Get("r_teleporterFlash", "1", CVAR_ARCHIVE);
	r_debugMarkSurface = ri.Cvar_Get("r_debugMarkSurface", "0", CVAR_ARCHIVE);
	r_ignoreNoMarks = ri.Cvar_Get("r_ignoreNoMarks", "0", CVAR_ARCHIVE);
	r_fog = ri.Cvar_Get("r_fog", "1", CVAR_ARCHIVE);
	r_ignoreEntityMergable = ri.Cvar_Get("r_ignoreEntityMergable", "2", CVAR_ARCHIVE);
	r_debugScaledImages = ri.Cvar_Get("r_debugScaledImages", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_scaleImagesPowerOfTwo = ri.Cvar_Get("r_scaleImagesPowerOfTwo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_screenMapTextureSize = ri.Cvar_Get("r_screenMapTextureSize", "128", CVAR_ARCHIVE | CVAR_LATCH);
	r_weather = ri.Cvar_Get("r_weather", "1", CVAR_ARCHIVE | CVAR_LATCH);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "fontlist", R_FontList_f);
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "screenshotPNG", R_ScreenShotPNG_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );
	ri.Cmd_AddCommand( "createcolorskins", R_CreateColorSkins_f );
	ri.Cmd_AddCommand( "printviewparms", R_PrintViewParms_f );
	ri.Cmd_AddCommand( "remaplasttwoshaders", R_RemapLastTwoShaders_f );
	ri.Cmd_AddCommand( "listremappedshaders", R_ListRemappedShaders_f );
	ri.Cmd_AddCommand( "clearallremappedshaders", R_ClearAllRemappedShaders_f );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {
	int	err;
	int i;
	byte *ptr;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
	Com_Memset( &tess, 0, sizeof( tess ) );

	// why ??
#if 0
	if(sizeof(glconfig_t) != 11332)
		ri.Error( ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %u != 11332", (unsigned int) sizeof(glconfig_t));
#endif

	backEnd.entity2D.ePtr = &backEnd.entity2D.ent;
	tr.worldEntity.ePtr = &tr.worldEntity.ent;

//	Swap_Init();

	if ( (intptr_t)tess.xyz & 15 ) {
		ri.Printf(PRINT_ALL,  "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	Com_Memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = ri.Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);
	R_InitNextFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	R_MME_Init();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	// print info
	GfxInfo_f();

	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "skinlist" );
	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "modelist" );
	ri.Cmd_RemoveCommand( "fontlist" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "screenshotJPEG" );
	ri.Cmd_RemoveCommand( "screenshotPNG" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "minimize" );
	ri.Cmd_RemoveCommand( "createcolorskins" );
	ri.Cmd_RemoveCommand( "printviewparms" );
	ri.Cmd_RemoveCommand( "remaplasttwoshaders" );
	ri.Cmd_RemoveCommand( "listremappedshaders" );
	ri.Cmd_RemoveCommand( "clearallremappedshaders" );


	if ( tr.registered ) {
		R_IssuePendingRenderCommands();
		R_DeleteQLGlslShadersAndPrograms();
		R_DeleteFramebufferObject();
		R_DeleteTextures();
	}

	R_MME_Shutdown();
	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		Com_Memset( &glConfig, 0, sizeof( glConfig ) );
		textureFilterAnisotropic = qfalse;
		maxAnisotropy = 0;
		displayAspect = 0.0f;
		haveClampToEdge = qfalse;

		Com_Memset( &glState, 0, sizeof( glState ) );
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_IssuePendingRenderCommands();
	if (!ri.Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *rimp ) {
#else
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp ) {
#endif

	static refexport_t	re;

	ri = *rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.GetGlConfig = RE_GetGlConfig;
	re.RegisterModel = RE_RegisterModel;
	re.GetModelName = R_GetModelName;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderLightMap = RE_RegisterShaderLightMap;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddRefEntityPtrToScene = RE_AddRefEntityPtrToScene;
	re.SetPathLines = RE_SetPathLines;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.GetGlyphInfo = RE_GetGlyphInfo;
	re.GetFontInfo = RE_GetFontInfo;
	re.RemapShader = R_RemapShader;
	re.ClearRemappedShader = R_ClearRemappedShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;
	re.Get_Advertisements = RE_Get_Advertisements;
	re.ReplaceShaderImage = RE_ReplaceShaderImage;
	re.RegisterShaderFromData = RE_RegisterShaderFromData;
	re.GetShaderImageDimensions = RE_GetShaderImageDimensions;
	re.GetShaderImageData = RE_GetShaderImageData;
	re.GetSingleShader = RE_GetSingleShader;
	re.BeginHud = RE_BeginHud;
	re.UpdateDof = RE_UpdateDof;

	return &re;
}

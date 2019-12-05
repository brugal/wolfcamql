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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_common.h"
#include "tr_mme.h"

// qglReadPixels
#define GLE(ret, name, ...) extern name##proc * qgl##name;
QGL_1_1_PROCS;
//QGL_1_1_FIXED_FUNCTION_PROCS;
//QGL_DESKTOP_1_1_PROCS;
//QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
//QGL_3_0_PROCS;
#undef GLE

//FIXME
extern cvar_t *r_znear;
void R_GammaCorrect (byte *buffer, int bufSize);

shotData_t shotDataMain;
shotData_t shotDataLeft;

//Data to contain the blurring factors
static passData_t passDataMain;
static passData_t passDataLeft;

// MME cvars
cvar_t	*mme_blurFrames;
cvar_t	*mme_blurType;
cvar_t	*mme_blurOverlap;
cvar_t	*mme_blurJitter;
cvar_t	*mme_blurStrength;

cvar_t	*mme_dofFrames;
cvar_t	*mme_dofRadius;
cvar_t  *mme_dofVisualize;

cvar_t  *mme_cpuSSE2;

cvar_t	*mme_depthFocus;
cvar_t	*mme_depthRange;

//cvar_t	*mme_saveStencil;
cvar_t	*mme_saveDepth;

static qboolean R_MME_MakeBlurBlock( mmeBlurBlock_t *block, int size, mmeBlurControl_t* control, shotData_t *shotData ) {
	memset( block, 0, sizeof( *block ) );
	size = (size + 15) & ~15;
	block->count = size / sizeof ( __m64 );
	block->control = control;

	//ri.Printf(PRINT_ALL, "size: %d  workUsed:%d\n", size, shotData->workUsed);
	if ( control->totalFrames ) {
		//Allow for floating point buffer with sse
		block->accum = (__m64 *)(shotData->workAlign + shotData->workUsed);
		shotData->workUsed += size * 4;
		if ( shotData->workUsed > shotData->workSize ) {
			ri.Printf(PRINT_ALL, "^1%d: Failed to allocate %d bytes (blur) from the mme work buffer  (workSize: %d)  blur\n", shotData == &shotDataMain, shotData->workUsed, shotData->workSize);
			return qfalse;
		}
	}
	if ( control->overlapFrames ) {
		block->overlap = (__m64 *)(shotData->workAlign + shotData->workUsed);
		shotData->workUsed += control->overlapFrames * size;
		if ( shotData->workUsed > shotData->workSize ) {
			ri.Printf(PRINT_ALL, "^1%d: Failed to allocate %d bytes (overlap) from the mme work buffer  (workSize: %d)  overlap\n", shotData == &shotDataMain, shotData->workUsed, shotData->workSize);
			return qfalse;
		}
	}

	return qtrue;
}

//#define MME_STRING( s ) # s

void R_MME_CheckCvars (qboolean init, shotData_t *shotData)
{
	int pixelCount, blurTotal, passTotal;
	//mmeBlurControl_t* blurControl = &blurData.control;
	mmeBlurControl_t* passControl;
	passData_t *passData;

	if (shotData == &shotDataMain) {
		passControl = &passDataMain.control;
		passData = &passDataMain;
	} else if (shotData == &shotDataLeft) {
		passControl = &passDataLeft.control;
		passData = &passDataLeft;
	} else {
		ri.Error(ERR_DROP, "%s unknown shotData structure", __FUNCTION__);
	}

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;

	if (mme_blurType->modified) {
		// can't just reset the specified *shotData since mme_blurType->modfied needs to be set to qfalse at this point (to avoid looping)
		shotDataMain.forceReset = qtrue;
		shotDataLeft.forceReset = qtrue;
		mme_blurType->modified = qfalse;

		//FIXME use something like shotData->lastSetBlurType
	}

	if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0" );
	}

	blurTotal = mme_blurFrames->integer + mme_blurOverlap->integer ;
	// blurTotal > BLURMAX:256 will overflow internal buffers
	if (blurTotal > BLURMAX) {
		ri.Printf(PRINT_ALL, "^1blurFrames + blurOverlap (%d + %d) have to be less than or equal to %d, resetting\n", mme_blurFrames->integer, mme_blurOverlap->integer, BLURMAX);
		ri.Cvar_Set( "mme_blurFrames", "0" );
		ri.Cvar_Set( "mme_blurOverlap", "0" );
		blurTotal = 0;
	}

	if (mme_dofFrames->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_dofFrames", va( "%d", BLURMAX) );
	} else if (mme_dofFrames->integer < 0 ) {
		ri.Cvar_Set( "mme_dofFrames", "0");
	}

	passTotal = mme_dofFrames->integer;

	if (!init  &&  (mme_blurFrames->integer != shotData->lastSetBlurFrames  ||  mme_blurOverlap->integer != shotData->lastSetOverlapFrames  ||  mme_dofFrames->integer != shotData->lastSetDofFrames)) {
		R_MME_InitMemory(qfalse, shotData);
	}

	if (!init  &&  ((shotData->forceReset  ||  passTotal != passControl->totalFrames  ||  blurTotal != shotData->control.totalFrames || pixelCount != shotData->pixelCount || shotData->control.overlapFrames != mme_blurOverlap->integer)
					&& !shotData->allocFailed)) {

		shotData->forceReset = qfalse;

		blurCreate(&shotData->control, mme_blurType->string, blurTotal);

		shotData->control.totalFrames = blurTotal;
		shotData->control.totalIndex = 0;
		shotData->control.overlapFrames = mme_blurOverlap->integer;
		shotData->control.overlapIndex = 0;

		if (blurTotal > 0) {
			if (!R_MME_MakeBlurBlock(&shotData->shot, pixelCount * 3, &shotData->control, shotData)) {
				ri.Printf(PRINT_ALL, "^1blur block creation failed\n");
				shotData->allocFailed = qtrue;
				goto alloc_Skip;
			}
		}

#if 0  //FIXME implement
		R_MME_MakeBlurBlock( &blurData.stencil, pixelCount * 1, blurControl );
		R_MME_MakeBlurBlock( &blurData.depth, pixelCount * 1, blurControl );
#endif

		R_MME_JitterTable( shotData->jitter[0], blurTotal );

		//Multi pass data
		blurCreate( passControl, "median", passTotal );
		passControl->totalFrames = passTotal;
		passControl->totalIndex = 0;
		passControl->overlapFrames = 0;
		passControl->overlapIndex = 0;

		if (passTotal > 0) {
			if (!R_MME_MakeBlurBlock( &passData->dof, pixelCount * 3, passControl, shotData )) {
				ri.Printf(PRINT_ALL, "^1blur pass creation failed\n");
				shotData->allocFailed = qtrue;
				goto alloc_Skip;
			}
		}

		R_MME_JitterTable( passData->jitter[0], passTotal );

		//ri.Printf(PRINT_ALL, "^6shotData reset\n");
	}

alloc_Skip:
	shotData->pixelCount = pixelCount;
}


qboolean R_MME_JitterOrigin( float *x, float *y, qboolean useMain ) {
	mmeBlurControl_t *passControl;
	shotData_t *shotData;
	passData_t *passData;
	*x = 0;
	*y = 0;

	if (useMain) {
		passControl = &passDataMain.control;
		shotData = &shotDataMain;
		passData = &passDataMain;
	} else {
		passControl = &passDataLeft.control;
		shotData = &shotDataLeft;
		passData = &passDataLeft;
	}

	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = shotData->dofFocus;
		float radius = shotData->dofRadius;
		R_MME_ClampDof(&focus, &radius);
		scale = radius * R_MME_FocusScale(focus);
		*x = scale * passData->jitter[i][0];
		*y = -scale * passData->jitter[i][1];

		return qtrue;
	}
	return qfalse;
}

void R_MME_JitterView( float *pixels, float *eyes, qboolean useMain ) {
	mmeBlurControl_t *blurControl;
	mmeBlurControl_t *passControl;
	shotData_t *shotData;
	passData_t *passData;

	if (useMain) {
		shotData = &shotDataMain;
		blurControl = &shotDataMain.control;
		passControl = &passDataMain.control;
		passData = &passDataMain;
	} else {
		shotData = &shotDataLeft;
		blurControl = &shotDataLeft.control;
		passControl = &passDataLeft.control;
		passData = &passDataLeft;
	}

	if ( blurControl->totalFrames ) {
		int i = blurControl->totalIndex;

		pixels[0] = mme_blurJitter->value * shotData->jitter[i][0];
		pixels[1] = mme_blurJitter->value * shotData->jitter[i][1];
	}

	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = shotData->dofFocus;
		float radius = shotData->dofRadius;

		R_MME_ClampDof(&focus, &radius);
		scale = r_znear->value / focus;
		scale *= radius * R_MME_FocusScale(focus);;
		eyes[0] = scale * passData->jitter[i][0];
		eyes[1] = scale * passData->jitter[i][1];
	}
}

int R_MME_MultiPassNext (qboolean useMain)
{
	mmeBlurControl_t *control;
	shotData_t *shotData;
	passData_t *passData;
	byte* outAlloc;
	__m64 *outAlign;
	//int i;

	if (useMain) {
		control = &passDataMain.control;
		shotData = &shotDataMain;
		passData = &passDataMain;
	} else {
		control = &passDataLeft.control;
		shotData = &shotDataLeft;
		passData = &passDataLeft;
	}

	if ( !control->totalFrames )
		return 0;

	if (shotData->allocFailed) {
		return 0;
	}

	outAlloc = ri.Hunk_AllocateTempMemory( shotData->pixelCount * 3 + 16);
	outAlign = (__m64 *)((((intptr_t)(outAlloc)) + 15) & ~15);

	// don't call GLimp_EndFrame()

	//qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_BGR, GL_UNSIGNED_BYTE, outAlign);
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, outAlign);

	// testing
#if 0
	for (i = 0;  i < glConfig.vidWidth * glConfig.vidHeight * 3;  i++) {
		((byte *)outAlign)[i] += i;
	}
#endif

	R_MME_BlurAccumAdd( &passData->dof, outAlign );

	ri.Hunk_FreeTempMemory( outAlloc );
	if ( ++(control->totalIndex) < control->totalFrames ) {
		int nextIndex = control->totalIndex;

		if ( ++(nextIndex) >= control->totalFrames ) {
			// pass
		}
		return 1;
	}
	control->totalIndex = 0;
	R_MME_BlurAccumShift( &passData->dof );
	return 0;
}

#if 0
void R_MME_GetMultiShot (byte * target, int width, int height, int glMode)
{
	if ( !passDataMain.control.totalFrames ) {
		//R_MME_GetShot( target, shotData.main.type );
		qglReadPixels(0, 0, width, height, glMode, GL_UNSIGNED_BYTE, target);
	} else {
		//Com_Memcpy( target, passDataMain.dof.accum, mainData.pixelCount * 3 );
		Com_Memcpy( target, passDataMain.dof.accum, shotDataMain.pixelCount * 3 );
	}
}
#endif

byte *R_MME_GetPassData (qboolean useMain)
{
	if (useMain) {
		return (byte *)passDataMain.dof.accum;
	} else {
		return (byte *)passDataLeft.dof.accum;
	}
}

void R_MME_Shutdown(void) {
	R_MME_FreeMemory(&shotDataMain);
	R_MME_FreeMemory(&shotDataLeft);
}

void R_MME_Init(void) {
	// MME cvars
	mme_blurFrames = ri.Cvar_Get ( "mme_blurFrames", "0", CVAR_ARCHIVE );
	mme_blurOverlap = ri.Cvar_Get ("mme_blurOverlap", "0", CVAR_ARCHIVE );
	mme_blurType = ri.Cvar_Get ( "mme_blurType", "gaussian", CVAR_ARCHIVE );
	mme_blurJitter = ri.Cvar_Get ( "mme_blurJitter", "1", CVAR_ARCHIVE );
	mme_blurStrength = ri.Cvar_Get ( "mme_blurStrength", "0", CVAR_ARCHIVE );

	mme_dofFrames = ri.Cvar_Get ( "mme_dofFrames", "0", CVAR_ARCHIVE );
	mme_dofRadius = ri.Cvar_Get ( "mme_dofRadius", "2", CVAR_ARCHIVE );
	mme_dofVisualize = ri.Cvar_Get ( "mme_dofVisualize", "0", CVAR_TEMP );

	mme_cpuSSE2 = ri.Cvar_Get ( "mme_cpuSSE2", "0", CVAR_ARCHIVE );

	mme_depthRange = ri.Cvar_Get ( "mme_depthRange", "2000", CVAR_ARCHIVE );
	mme_depthFocus = ri.Cvar_Get ( "mme_depthFocus", "0", CVAR_ARCHIVE );
	//mme_saveStencil = ri.Cvar_Get ( "mme_saveStencil", "0", CVAR_ARCHIVE );
	mme_saveDepth = ri.Cvar_Get ( "mme_saveDepth", "0", CVAR_ARCHIVE );

	Com_Memset(&shotDataMain, 0, sizeof(shotDataMain));
	Com_Memset(&shotDataLeft, 0, sizeof(shotDataLeft));

	R_MME_CheckCvars(qtrue, &shotDataMain);
	R_MME_InitMemory(qtrue, &shotDataMain);

	R_MME_CheckCvars(qtrue, &shotDataLeft);
	R_MME_InitMemory(qtrue, &shotDataLeft);

	// avoid crash if mme_dofVisualize set when renderer starts
	if (mme_dofVisualize->integer) {
		shotDataMain.forceReset = qtrue;
	}

#if 0
	//FIXME testing
	shotDataMain.dofFocus = 1024;
	shotDataMain.dofRadius = 5;

	//FIXME testing
	shotDataLeft.dofFocus = 1024;
	shotDataLeft.dofRadius = 5;
#endif
}

void R_MME_InitMemory (qboolean verbose, shotData_t *shotData)
{
	int m;
	int bl, ovr;
	int dofFrames;

	m = 1;

	bl = ovr = 0;
	if (mme_blurFrames->integer > 0) {
		bl = 1;
	}
	if (mme_blurOverlap->integer > 0) {
		ovr = mme_blurOverlap->integer;
	}
	m += (bl + ovr);

	dofFrames = 0;
	if (mme_dofFrames->integer > 0) {
		dofFrames = mme_dofFrames->integer;
	}

	// R_MME_MakeBlurBlock() allocates 4*size (size is pixelcount * (3 or 1)), also increase if depth or stencil blur used
	if ((bl + ovr) > 0) {
		m += 4;
	}
	if (dofFrames > 0) {
		m += 4;
	}

	//FIXME too much extra added
	shotData->workSize = (glConfig.vidWidth * glConfig.vidHeight * 4 * m) + (1 * 1024 * 1024);

	if (!shotData->workAlloc) {
		shotData->workAlloc = calloc(shotData->workSize + 16, 1);
		//ri.Printf(PRINT_ALL, "^5calloc() %f\n", (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
	} else {
		// 2018-11-28 don't use realloc() since it can cause premature failure in Win32 x86 (presumably because the new and old memory are still allocated until the old one is copied over)
		free(shotData->workAlloc);
		shotData->workAlloc = malloc(shotData->workSize + 16);
		//ri.Printf(PRINT_ALL, "^5realloc() %f\n", (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
	}
	if (!shotData->workAlloc) {
		Com_Error(ERR_DROP, "%d: Failed to allocate %f megabytes for mme work buffer", shotData == &shotDataMain, (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
		return;
	}
	if (verbose  ||  ri.Cvar_VariableIntegerValue("mme_debugmemory")) {
		ri.Printf(PRINT_ALL, "%d: %f megabytes for mme work buffer\n", shotData == &shotDataMain, (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
	}
	shotData->workAlign = (char *)(((intptr_t)shotData->workAlloc + 15) & ~15);
	shotData->workUsed = 0;

	shotData->lastSetBlurFrames = mme_blurFrames->integer;
	shotData->lastSetOverlapFrames = mme_blurOverlap->integer;
	shotData->lastSetDofFrames = mme_dofFrames->integer;
}

void R_MME_FreeMemory (shotData_t *shotData)
{
	if (shotData->workAlloc) {
		//ri.Printf(PRINT_ALL, "^5freeing shotData->workAlloc\n");
		free(shotData->workAlloc);
		shotData->workAlloc = NULL;
	}
}

void RE_UpdateDof (float viewFocus, float viewRadius)
{
	shotDataMain.dofFocus = viewFocus;
	shotDataMain.dofRadius = viewRadius;

	shotDataLeft.dofFocus = viewFocus;
	shotDataLeft.dofRadius = viewRadius;
}

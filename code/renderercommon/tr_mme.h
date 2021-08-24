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

#ifndef tr_mme_h_included
#define tr_mme_h_included

#include "tr_common.h"
#include <xmmintrin.h>

#define BLURMAX 256
//#define PASSMAX 256

typedef struct {
//      int             pixelCount;
        int             totalFrames;
        int             totalIndex;
        int             overlapFrames;
        int             overlapIndex;

        short   MMX[BLURMAX];
        short   SSE[BLURMAX];
        float   Float[BLURMAX];
} mmeBlurControl_t;

typedef struct {
        __m64   *accum;
        __m64   *overlap;
        int             count;
        mmeBlurControl_t *control;
} mmeBlurBlock_t;

typedef struct {
	mmeBlurControl_t control;
	// mmeBlurBlock_t shot, depth, stencil;
	mmeBlurBlock_t shot, depth;

	float dofFocus, dofRadius;

	int		pixelCount;
	qboolean allocFailed;
	qboolean forceReset;

	char *workAlloc;
	char *workAlign;
	int workSize, workUsed;

	int lastSetBlurFrames;
	int lastSetOverlapFrames;
	int lastSetDofFrames;

	float jitter[BLURMAX][2];
} shotData_t;

typedef struct {
        mmeBlurControl_t control;
        mmeBlurBlock_t dof;
        float   jitter[BLURMAX][2];
} passData_t;

extern shotData_t shotDataMain;
extern shotData_t shotDataLeft;

extern cvar_t *mme_aviFormat;
extern cvar_t *mme_depthFocus;
extern cvar_t *mme_depthRange;
extern cvar_t *mme_saveDepth;

extern cvar_t *mme_blurJitter;
extern cvar_t *mme_blurStrength;

extern cvar_t *mme_dofFrames;
extern cvar_t *mme_dofRadius;
extern cvar_t *mme_dofVisualize;

extern cvar_t *mme_cpuSSE2;

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const __m64 *add );
void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index );
void R_MME_BlurAccumShift( mmeBlurBlock_t *block );
void blurCreate( mmeBlurControl_t* control, const char* type, int frames );
void R_MME_JitterTable(float *jitarr, int num);
void R_MME_JitterView( float *pixels, float* eyes, qboolean useMain );
qboolean R_MME_JitterOrigin( float *x, float *y, qboolean useMain );
float R_MME_FocusScale (float focus);
void R_MME_ClampDof (float *focus, float *radius);
int R_MME_MultiPassNext (qboolean useMain);

//void R_MME_GetMultiShot (byte * target, int width, int height, int glMode);
byte *R_MME_GetPassData (qboolean useMain);

void R_MME_GetDepth( byte *output );

void MME_AccumClearSSE( void *w, const void *r, short int mul, int count );
void MME_AccumAddSSE( void* w, const void* r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );

void R_MME_CheckCvars (qboolean init, shotData_t *shotData);
void R_MME_InitMemory (qboolean verbose, shotData_t *shotData);
void R_MME_FreeMemory (shotData_t *shotData);
void R_MME_Shutdown (void);

ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block ) {
        mmeBlurControl_t* control = block->control;
        int index = control->overlapIndex % control->overlapFrames;
        return (byte *)( block->overlap + block->count * index );
}

#endif  // tr_mme_h_included

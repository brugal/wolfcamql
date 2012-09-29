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
#include "tr_local.h"
#include "tr_mme.h"

shotData_t shotDataMain;
shotData_t shotDataLeft;

// MME cvars
cvar_t	*mme_blurFrames;
cvar_t	*mme_blurType;
cvar_t	*mme_blurOverlap;

cvar_t	*mme_depthFocus;
cvar_t	*mme_depthRange;

//cvar_t	*mme_saveStencil;
cvar_t	*mme_saveDepth;

void accumCreateMultiply (shotData_t *shotData)
{
	float	blurBase[256];
	int		blurEntries[256];
	float	blurHalf = 0.5f * (shotData->blurTotal - 1 );
	float	strength = 128;
	float	bestStrength = 0;
	int		passes, bestTotal = 0;
	int		i;

	if (blurHalf <= 0)
		return;

	if (!Q_stricmp(mme_blurType->string, "gaussian")) {
		for (i = 0; i < shotData->blurTotal; i++) {
			double xVal = ((i - blurHalf) / blurHalf) * 3;
			double expVal = exp( - (xVal * xVal) / 2);
			double sqrtVal = 1.0f / sqrt( 2 * M_PI);
			blurBase[i] = sqrtVal * expVal;
		}
	} else if (!Q_stricmp(mme_blurType->string, "triangle")) {
		for (i = 0; i < shotData->blurTotal; i++) {
			if ( i <= blurHalf )
				blurBase[i] = 1 + i;
			else
				blurBase[i] = 1 + (shotData->blurTotal - 1 - i);
		}
	} else {
		int mulDelta = (256 << 10) / shotData->blurTotal;
		int mulIndex = 0;
		for (i = 0; i < shotData->blurTotal; i++) {
			int lastMul = mulIndex & ~((1 << 10) -1);
			mulIndex += mulDelta;
			blurBase[i] = (mulIndex - lastMul) >> 10;
		}
	}
	/* Check for best 256 match */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < shotData->blurTotal; i++)
			total += strength * blurBase[i];
		if (total > 256) {
			strength *= (256.0 / total);
		} else if (total < 256) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (256.0 / total);
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < shotData->blurTotal; i++) {
		blurEntries[i] = bestStrength * blurBase[i];
	}
	for (i = 0; i < shotData->blurTotal; i++) {
		int a = blurEntries[i];
		shotData->blurMultiply[i] = _mm_set_pi16( a, a, a, a);
	}
	_mm_empty();
}

void accumClearMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count ) {
	 __m64 readVal, zeroVal, work0, work1, multiply;
	 int i;
	 multiply = *multp;
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count / 2 ; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = work0;
		 writer[1] = work1;
		 writer += 2;
	 }
	 _mm_empty();
}

void accumAddMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count ) {
	 __m64 readVal, zeroVal, work0, work1, multiply;
	 int i;
	 multiply = *multp;
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count / 2 ; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = _mm_add_pi16( writer[0], work0 );
		 writer[1] = _mm_add_pi16( writer[1], work1 );
		 writer += 2;
	 }
	 _mm_empty();
}

void accumShift( const __m64 *reader,  __m64 *writer, int count ) {
	__m64 work0, work1, work2, work3;
	int i;
	/* Handle 4 pixels at once */
	for (i = count /4;i>0;i--) {
		work0 = _mm_srli_pi16 (reader[0], 8);
		work1 = _mm_srli_pi16 (reader[1], 8);
		work2 = _mm_srli_pi16 (reader[2], 8);
		work3 = _mm_srli_pi16 (reader[3], 8);
//		_mm_stream_pi( writer + 0, _mm_packs_pu16( work0, work1 ));
//		_mm_stream_pi( writer + 1, _mm_packs_pu16( work2, work3 ));
		writer[0] = _mm_packs_pu16( work0, work1 );
		writer[1] = _mm_packs_pu16( work2, work3 );
		writer+=2;
		reader+=4;
	}
	_mm_empty();
}

//#define MME_STRING( s ) # s

void R_MME_CheckCvars (qboolean init, shotData_t *shotData)
{
	int pixelCount, newBlur;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;
	if (mme_blurType->modified) {
		mme_blurType->modified = qfalse;
		accumCreateMultiply(shotData);
	}

	if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (!init  &&  mme_blurFrames->integer > shotData->lastSetBlurFrames) {
		R_MME_InitMemory(qfalse, shotData);
	}

	if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0");
	}

	if (!init  &&  mme_blurOverlap->integer > shotData->lastSetOverlapFrames) {
		R_MME_InitMemory(qfalse, shotData);
	}

	newBlur = mme_blurFrames->integer + mme_blurOverlap->integer ;

	if (!init  &&  ((newBlur != shotData->blurTotal || pixelCount != shotData->pixelCount || shotData->overlapTotal != mme_blurOverlap->integer)
					&& !shotData->allocFailed)) {
		shotData->workUsed = 0;

		shotData->blurTotal = newBlur;
		if ( newBlur ) {
			shotData->accumAlign = (__m64 *)(shotData->workAlign + shotData->workUsed);
			shotData->workUsed += pixelCount * sizeof( __m64 );
			shotData->workUsed = (shotData->workUsed + 7) & ~7;
			if ( shotData->workUsed > shotData->workSize) {
				ri.Printf(PRINT_ALL, "^1%d: Failed to allocate %d bytes from the mme work buffer  (workSize: %d)  blur\n", shotData == &shotDataMain, shotData->workUsed, shotData->workSize);
				shotData->allocFailed = qtrue;
				goto alloc_Skip;
			}
			accumCreateMultiply(shotData);
		}
		shotData->overlapTotal = mme_blurOverlap->integer;
		if ( shotData->overlapTotal ) {
			shotData->overlapAlign = (__m64 *)(shotData->workAlign + shotData->workUsed);
			shotData->workUsed += shotData->overlapTotal * pixelCount * 4;
			shotData->workUsed = (shotData->workUsed + 7) & ~7;
			if ( shotData->workUsed > shotData->workSize) {
				ri.Printf(PRINT_ALL, "^1%d: Failed to allocate %d bytes from the mme work buffer  (workSize: %d)  overlap\n", shotData == &shotDataMain, shotData->workUsed, shotData->workSize);
				shotData->allocFailed = qtrue;
				goto alloc_Skip;
			}
		}
		shotData->overlapIndex = 0;
		shotData->blurIndex = 0;
	}
alloc_Skip:
	shotData->pixelCount = pixelCount;
}

void R_MME_Shutdown(void) {
	R_MME_FreeMemory(&shotDataMain);
	R_MME_FreeMemory(&shotDataLeft);
}

void R_MME_Init(void) {
	// MME cvars
	mme_blurFrames = ri.Cvar_Get ( "mme_blurFrames", "0", CVAR_ARCHIVE );
	mme_blurOverlap = ri.Cvar_Get ("mme_blurOverlap", "0", CVAR_ARCHIVE );
	mme_blurType = ri.Cvar_Get ( "mme_blurType", "median", CVAR_ARCHIVE );

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
}

void R_MME_InitMemory (qboolean verbose, shotData_t *shotData)
{
	int m;
	int bl, ovr;

	m = 1;

	bl = ovr = 0;
	if (mme_blurFrames->integer > 0) {
		bl = mme_blurFrames->integer;
	}
	if (mme_blurOverlap->integer > 0) {
		ovr = mme_blurOverlap->integer;
	}
	m += (bl + ovr);

	shotData->workSize = glConfig.vidWidth * glConfig.vidHeight * 4 * m + 4 * 1024 * 1024;

	if (!shotData->workAlloc) {
		shotData->workAlloc = calloc(shotData->workSize + 16, 1);
	} else {
		shotData->workAlloc = realloc(shotData->workAlloc, shotData->workSize + 16);
	}
	if (!shotData->workAlloc) {
		Com_Error(ERR_DROP, "%d: Failed to allocate %f megabytes for mme work buffer", shotData == &shotDataMain, (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
		return;
	}
	if (verbose) {
		Com_Printf("%d: %f megabytes for mme work buffer\n", shotData == &shotDataMain, (float)(shotData->workSize + 16) / 1024.0 / 1024.0);
	}
	shotData->workAlign = (char *)(((int)shotData->workAlloc + 15) & ~15);
	shotData->lastSetBlurFrames = mme_blurFrames->integer;
	shotData->lastSetOverlapFrames = mme_blurOverlap->integer;
}

void R_MME_FreeMemory (shotData_t *shotData)
{
	if (shotData->workAlloc) {
		free(shotData->workAlloc);
		shotData->workAlloc = NULL;
	}
}

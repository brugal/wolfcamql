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

typedef struct {
	int		pixelCount;
	__m64	*accumAlign;
	__m64	*overlapAlign;
	int		overlapTotal, overlapIndex;
	int		blurTotal, blurIndex;
	__m64	blurMultiply[256];

	qboolean allocFailed;

	char *workAlloc;
	char *workAlign;
	int workSize, workUsed;

	int lastSetBlurFrames;
	int lastSetOverlapFrames;

} shotData_t;

extern shotData_t shotDataMain;
extern shotData_t shotDataLeft;

extern cvar_t *mme_aviFormat;
extern cvar_t *mme_depthFocus;
extern cvar_t *mme_depthRange;
extern cvar_t *mme_saveDepth;

void accumCreateMultiply (shotData_t *shotData);
void accumClearMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count );
void accumAddMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count );
void accumShift( const __m64 *reader,  __m64 *writer, int count );

void R_MME_CheckCvars (qboolean init, shotData_t *shotData);
void R_MME_InitMemory (qboolean verbose, shotData_t *shotData);
void R_MME_FreeMemory (shotData_t *shotData);
void R_MME_Shutdown (void);

#endif  // tr_mme_h_included

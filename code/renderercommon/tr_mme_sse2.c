/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

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

#include "tr_mme.h"
#include <xmmintrin.h>
#include <emmintrin.h>


void MME_AccumClearSSE( void *w, const void *r, short int mul, int count ) {
	const __m128i * reader = (const __m128i *) r;
	__m128i *writer = (__m128i *) w;
	int i; 
	 __m128i multiply =  _mm_set1_epi16( mul );
	 __m128i zeroVal = _mm_setzero_si128();

	 for (i = count / 2; i>0 ; i--) {
		 __m128i readVal = *reader++;
		 __m128i workBase, workLo, workHi;

// 		 readVal = _mm_set1_epi8( 1 );
		 workBase = _mm_unpacklo_epi8( readVal, zeroVal );
		 workLo = _mm_mullo_epi16( workBase, multiply );
		 workHi = _mm_mulhi_epi16( workBase, multiply );
		 writer[0] = _mm_unpacklo_epi16( workLo, workHi );
		 writer[1] = _mm_unpackhi_epi16( workLo, workHi );
		 
		 workBase = _mm_unpackhi_epi8( readVal, zeroVal );
		 workLo = _mm_mullo_epi16( workBase, multiply );
		 workHi = _mm_mulhi_epi16( workBase, multiply );

		 writer[2] = _mm_unpacklo_epi16( workLo, workHi );
		 writer[3] = _mm_unpackhi_epi16( workLo, workHi );
		 writer += 4;
	 }
}


void MME_AccumAddSSE( void* w, const void* r, short int mul, int count ) {
	int i; 
	const __m128i * reader = (const __m128i *) r;
	__m128i *writer = (__m128i *) w;

	 __m128i multiply =  _mm_set1_epi16( mul );
	 __m128i zeroVal = _mm_setzero_si128();

	 for (i = count / 2; i>0 ; i--) {
		 __m128i readVal = *reader++;
		 __m128i workBase, workLo, workHi;

		 //Prefetch some data 
		 _mm_prefetch( (const char*)(reader + 2), _MM_HINT_NTA );
		 _mm_prefetch( (const char*)(writer + 8), _MM_HINT_T0 );

		 workBase = _mm_unpacklo_epi8( readVal, zeroVal );
		 workLo = _mm_mullo_epi16( workBase, multiply );
		 workHi = _mm_mulhi_epi16( workBase, multiply );
		 writer[0] = _mm_add_epi32( writer[0], _mm_unpacklo_epi16( workLo, workHi ) );
		 writer[1] = _mm_add_epi32( writer[1], _mm_unpackhi_epi16( workLo, workHi ) );
		 
		 workBase = _mm_unpackhi_epi8( readVal, zeroVal );
		 workLo = _mm_mullo_epi16( workBase, multiply );
		 workHi = _mm_mulhi_epi16( workBase, multiply );

		 writer[2] = _mm_add_epi32( writer[2], _mm_unpacklo_epi16( workLo, workHi ) );
		 writer[3] = _mm_add_epi32( writer[3], _mm_unpackhi_epi16( workLo, workHi ) );
		 writer += 4;
	 }
}



void MME_AccumShiftSSE( const void *r, void *w, int count ) {
	const __m128i * reader = (const __m128i *) r;
	__m128i *writer = (__m128i *) w;
	int i;
	__m128i work0, work1, work2, work3;
	/* Handle 4 at once */
	for (i = count/4;i>0;i--) {
#define shift 15
		 _mm_prefetch( (const char*)(reader + 16), _MM_HINT_T0 );

		work0 = _mm_packs_epi32( _mm_srli_epi32 (reader[0], shift ), _mm_srli_epi32 (reader[1], shift ) );
		work1 = _mm_packs_epi32( _mm_srli_epi32 (reader[2], shift ), _mm_srli_epi32 (reader[3], shift ) );
		work2 = _mm_packs_epi32( _mm_srli_epi32 (reader[4], shift ), _mm_srli_epi32 (reader[5], shift ) );
		work3 = _mm_packs_epi32( _mm_srli_epi32 (reader[6], shift ), _mm_srli_epi32 (reader[7], shift ) );
		reader += 8;

		writer[0] = _mm_packus_epi16( work0, work1 );
		writer[1] = _mm_packus_epi16( work2, work3 );
		writer += 2;
	}
}

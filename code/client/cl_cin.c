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

/*****************************************************************************
 * name:		cl_cin.c
 *
 * desc:		video and cinematic playback
 *
 * $Archive: /MissionPack/code/client/cl_cin.c $
 *
 * cl_glconfig.hwtype trtypes 3dfx/ragepro need 256x256
 *
 *****************************************************************************/

#include "client.h"
#include "snd_local.h"

#define MAXSIZE				8
#define MINSIZE				4

#define DEFAULT_CIN_WIDTH	512
#define DEFAULT_CIN_HEIGHT	512

#define ROQ_QUAD			0x1000
#define ROQ_QUAD_INFO		0x1001
#define ROQ_CODEBOOK		0x1002
#define ROQ_QUAD_VQ			0x1011
#define ROQ_QUAD_JPEG		0x1012
#define ROQ_QUAD_HANG		0x1013
#define ROQ_PACKET			0x1030
#define ZA_SOUND_MONO		0x1020
#define ZA_SOUND_STEREO		0x1021

#define MAX_VIDEO_HANDLES	16


static void RoQ_init( void );

/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

static	long				ROQ_YY_tab[256];
static	long				ROQ_UB_tab[256];
static	long				ROQ_UG_tab[256];
static	long				ROQ_VG_tab[256];
static	long				ROQ_VR_tab[256];
static qboolean YUVTablesInitialized = qfalse;

static	unsigned short		vq2[256*16*4];
static	unsigned short		vq4[256*64*4];
static	unsigned short		vq8[256*256*4];


#define MAX_FRAME_SIZE 65536

typedef struct {
	byte				linbuf[DEFAULT_CIN_WIDTH*DEFAULT_CIN_HEIGHT*4*2];
	byte				file[MAX_FRAME_SIZE + 8];   // plus header
	short				sqrTable[256];

	int		mcomp[256];
	byte				*qStatus[2][32768];

	long				oldXOff, oldYOff, oldysize, oldxsize;

} cinematics_t;

typedef struct {
	char				fileName[MAX_OSPATH];
	int					CIN_WIDTH, CIN_HEIGHT;
	int					xpos, ypos, width, height;
	qboolean			looping, holdAtEnd, dirty, alterGameState, silent, shader;
	fileHandle_t		iFile;
	e_status			status;
	double		startTime;
	double		lastTime;
	double playCallStartTime;  // time when the play call was issued
	long				tfps;
	long				RoQPlayed;
	long				ROQSize;
	unsigned int		RoQFrameSize;
	long				onQuad;
	long				numQuads;
	long				totalQuads;
	qboolean			testParse;
	long				samplesPerLine;
	unsigned int		roq_id;
	long				screenDelta;

	void ( *VQ0)(byte *status, void *qdata );
	void ( *VQ1)(byte *status, void *qdata );
	void ( *VQNormal)(byte *status, void *qdata );
	void ( *VQBuffer)(byte *status, void *qdata );

	long				samplesPerPixel;				// defaults to 2
	byte*				gray;
	unsigned int		xsize, ysize, maxsize, minsize;

	qboolean			half, smootheddouble, inMemory;
	long				normalBuffer0;
	long				roq_flags;
	long				roqF0;
	long				roqF1;
	long				t[2];
	long				roqFPS;
	int					playonwalls;
	byte*				buf;
	long				drawX, drawY;
} cin_cache;

static cinematics_t		cin[MAX_VIDEO_HANDLES];
static cin_cache		cinTable[MAX_VIDEO_HANDLES];
static int				currentHandle = -1;
static int				CL_handle = -1;

extern int				s_soundtime;		// sample PAIRS


void CIN_CloseAllVideos(void) {
	int		i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if (cinTable[i].fileName[0] != 0 ) {
			CIN_StopCinematic(i);
		}
	}
}


static int CIN_HandleForVideo(void) {
	int		i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( cinTable[i].fileName[0] == 0 ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
	return -1;
}


extern int CL_ScaledMilliseconds(void);

//-----------------------------------------------------------------------------
// RllSetupTable
//
// Allocates and initializes the square table.
//
// Parameters:	None
//
// Returns:		Nothing
//-----------------------------------------------------------------------------
static void RllSetupTable( void )
{
	int z;

	for (z=0;z<128;z++) {
		cin[currentHandle].sqrTable[z] = (short)(z*z);
		cin[currentHandle].sqrTable[z+128] = (short)(-cin[currentHandle].sqrTable[z]);
	}
}


#if 0  // unused

//-----------------------------------------------------------------------------
// RllDecodeMonoToMono
//
// Decode mono source data into a mono buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= # of shorts of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
static long RllDecodeMonoToMono (const unsigned char *from, short *to, unsigned int size, char signedOutput , unsigned short flag)
{
	unsigned int z;
	int prev;
	
	if (signedOutput)	
		prev =  flag - 0x8000;
	else 
		prev = flag;

	for (z=0;z<size;z++) {
		prev = to[z] = (short)(prev + cin[currentHandle].sqrTable[from[z]]); 
	}
	return size;	//*sizeof(short));
}

#endif

//-----------------------------------------------------------------------------
// RllDecodeMonoToStereo
//
// Decode mono source data into a stereo buffer. Output is 4 times the number
// of bytes in the input.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/4 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
static long RllDecodeMonoToStereo (const unsigned char *from, short *to, unsigned int size, char signedOutput, unsigned short flag)
{
	unsigned int z;
	int prev;
	
	if (signedOutput)	
		prev =  flag - 0x8000;
	else 
		prev = flag;

	for (z = 0; z < size; z++) {
		prev = (short)(prev + cin[currentHandle].sqrTable[from[z]]);
		to[z*2+0] = to[z*2+1] = (short)(prev);
	}
	
	return size;	// * 2 * sizeof(short));
}


//-----------------------------------------------------------------------------
// RllDecodeStereoToStereo
//
// Decode stereo source data into a stereo buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/2 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
static long RllDecodeStereoToStereo (const unsigned char *from,short *to,unsigned int size,char signedOutput, unsigned short flag)
{
	unsigned int z;
	const unsigned char *zz = from;
	int	prevL, prevR;

	if (signedOutput) {
		prevL = (flag & 0xff00) - 0x8000;
		prevR = ((flag & 0x00ff) << 8) - 0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = (flag & 0x00ff) << 8;
	}

	for (z=0;z<size;z+=2) {
                prevL = (short)(prevL + cin[currentHandle].sqrTable[*zz++]); 
                prevR = (short)(prevR + cin[currentHandle].sqrTable[*zz++]);
                to[z+0] = (short)(prevL);
                to[z+1] = (short)(prevR);
	}
	
	return (size>>1);	//*sizeof(short));
}


#if 0  // unused
//-----------------------------------------------------------------------------
// RllDecodeStereoToMono
//
// Decode stereo source data into a mono buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
static long RllDecodeStereoToMono(const unsigned char *from,short *to,unsigned int size,char signedOutput, unsigned short flag)
{
	unsigned int z;
	int prevL,prevR;
	
	if (signedOutput) {
		prevL = (flag & 0xff00) - 0x8000;
		prevR = ((flag & 0x00ff) << 8) -0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = (flag & 0x00ff) << 8;
	}

	for (z=0;z<size;z+=1) {
		prevL= prevL + cin[currentHandle].sqrTable[from[z*2]];
		prevR = prevR + cin[currentHandle].sqrTable[from[z*2+1]];
		to[z] = (short)((prevL + prevR)/2);
	}

	return size;
}

#endif

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void move8_32( const byte *src, byte *dst, int spl )
{
	int i;

	for(i = 0; i < 8; ++i)
	{
		memcpy(dst, src, 32);
		src += spl;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void move4_32( const byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 4; ++i)
	{
		memcpy(dst, src, 16);
		src += spl;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void blit8_32( const byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 8; ++i)
	{
		memcpy(dst, src, 32);
		src += 32;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/
static void blit4_32( const byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 4; ++i)
	{
		memmove(dst, src, 16);
		src += 16;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void blit2_32( const byte *src, byte *dst, int spl  )
{
	memcpy(dst, src, 8);
	memcpy(dst+spl, src+8, 8);
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void blitVQQuad32fs( byte **status, const unsigned char *data )
{
unsigned short	newd, celdata, code;
unsigned int	index, i;
int		spl;

	newd	= 0;
	celdata = 0;
	index	= 0;
	
        spl = cinTable[currentHandle].samplesPerLine;
        
	do {
		if (!newd) { 
			newd = 7;
			celdata = data[0] + data[1]*256;
			data += 2;
		} else {
			newd--;
		}

		code = (unsigned short)(celdata&0xc000); 
		celdata <<= 2;
		
		switch (code) {
			case	0x8000:													// vq code
				blit8_32( (byte *)&vq8[(*data)*128], status[index], spl );
				data++;
				index += 5;
				break;
			case	0xc000:													// drop
				index++;													// skip 8x8
				for(i=0;i<4;i++) {
					if (!newd) { 
						newd = 7;
						celdata = data[0] + data[1]*256;
						data += 2;
					} else {
						newd--;
					}
						
					code = (unsigned short)(celdata&0xc000); celdata <<= 2; 

					switch (code) {											// code in top two bits of code
						case	0x8000:										// 4x4 vq code
							blit4_32( (byte *)&vq4[(*data)*32], status[index], spl );
							data++;
							break;
						case	0xc000:										// 2x2 vq code
							blit2_32( (byte *)&vq2[(*data)*8], status[index], spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+8, spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+spl*2, spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+spl*2+8, spl );
							data++;
							break;
						case	0x4000:										// motion compensation
							move4_32( status[index] + cin[currentHandle].mcomp[(*data)], status[index], spl );
							data++;
							break;
					}
					index++;
				}
				break;
			case	0x4000:													// motion compensation
				move8_32( status[index] + cin[currentHandle].mcomp[(*data)], status[index], spl );
				data++;
				index += 5;
				break;
			case	0x0000:
				index += 5;
				break;
		}
	} while ( status[index] != NULL );
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void ROQ_GenYUVTables( void )
{
	float t_ub,t_vr,t_ug,t_vg;
	long i;

	if (YUVTablesInitialized) {
		return;
	}

	t_ub = (1.77200f/2.0f) * (float)(1<<6) + 0.5f;
	t_vr = (1.40200f/2.0f) * (float)(1<<6) + 0.5f;
	t_ug = (0.34414f/2.0f) * (float)(1<<6) + 0.5f;
	t_vg = (0.71414f/2.0f) * (float)(1<<6) + 0.5f;
	for(i=0;i<256;i++) {
		float x = (float)(2 * i - 255);

		ROQ_UB_tab[i] = (long)( ( t_ub * x) + (1<<5));
		ROQ_VR_tab[i] = (long)( ( t_vr * x) + (1<<5));
		ROQ_UG_tab[i] = (long)( (-t_ug * x)		 );
		ROQ_VG_tab[i] = (long)( (-t_vg * x) + (1<<5));
		ROQ_YY_tab[i] = (long)( (i << 6) | (i >> 2) );
	}

	YUVTablesInitialized = qtrue;
}

#define VQ2TO4(a,b,c,d) { \
    	*c++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*c++ = a[1];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*c++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*c++ = b[1];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	a += 2; b += 2; }
 
#define VQ2TO2(a,b,c,d) { \
	*c++ = *a;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*c++ = *b;	\
	*d++ = *b;	\
	*d++ = *b;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*d++ = *b;	\
	*d++ = *b;	\
	a++; b++; }

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static unsigned short yuv_to_rgb( long y, long u, long v )
{ 
	long r,g,b,YY = (long)(ROQ_YY_tab[(y)]);

	r = (YY + ROQ_VR_tab[v]) >> 9;
	g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 8;
	b = (YY + ROQ_UB_tab[u]) >> 9;
	
	if (r<0) r = 0;
	if (g<0) g = 0;
	if (b<0) b = 0;
	if (r > 31) r = 31;
	if (g > 63) g = 63;
	if (b > 31) b = 31;

	return (unsigned short)((r<<11)+(g<<5)+(b));
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/
static unsigned int yuv_to_rgb24( long y, long u, long v )
{ 
	long r,g,b,YY = (long)(ROQ_YY_tab[(y)]);

	r = (YY + ROQ_VR_tab[v]) >> 6;
	g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 6;
	b = (YY + ROQ_UB_tab[u]) >> 6;
	
	if (r<0) r = 0;
	if (g<0) g = 0;
	if (b<0) b = 0;
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	
	return LittleLong ((r)|(g<<8)|(b<<16)|(255<<24));
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void decodeCodeBook( const byte *input, unsigned short roq_flags )
{
	long	i, j, two, four;
	unsigned short	*aptr, *bptr, *cptr, *dptr;
	long	y0,y1,y2,y3,cr,cb;
	byte	*bbptr, *baptr, *bcptr, *bdptr;
	union {
		unsigned int *i;
		unsigned short *s;
	} iaptr, ibptr, icptr, idptr;

	if (!roq_flags) {
		two = four = 256;
	} else {
		two  = roq_flags>>8;
		if (!two) two = 256;
		four = roq_flags&0xff;
	}

	four *= 2;

	bptr = (unsigned short *)vq2;

	if (!cinTable[currentHandle].half) {
		if (!cinTable[currentHandle].smootheddouble) {
//
// normal height
//
			if (cinTable[currentHandle].samplesPerPixel==2) {
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)vq4;
				dptr = (unsigned short *)vq8;
		
				for(i=0;i<four;i++) {
					aptr = (unsigned short *)vq2 + (*input++)*4;
					bptr = (unsigned short *)vq2 + (*input++)*4;
					for(j=0;j<2;j++)
						VQ2TO4(aptr,bptr,cptr,dptr);
				}
			} else if (cinTable[currentHandle].samplesPerPixel==4) {
				ibptr.s = bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;
	
				for(i=0;i<four;i++) {
					iaptr.s = vq2;
					iaptr.i += (*input++)*4;
					ibptr.s = vq2;
					ibptr.i += (*input++)*4;
					for(j=0;j<2;j++) 
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
				}
			} else if (cinTable[currentHandle].samplesPerPixel==1) {
				bbptr = (byte *)bptr;
				for(i=0;i<two;i++) {
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input]; input +=3;
				}

				bcptr = (byte *)vq4;
				bdptr = (byte *)vq8;
	
				for(i=0;i<four;i++) {
					baptr = (byte *)vq2 + (*input++)*4;
					bbptr = (byte *)vq2 + (*input++)*4;
					for(j=0;j<2;j++) 
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
				}
			}
		} else {
//
// double height, smoothed
//
			if (cinTable[currentHandle].samplesPerPixel==2) {
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( ((y0*3)+y2)/4, cr, cb );
					*bptr++ = yuv_to_rgb( ((y1*3)+y3)/4, cr, cb );
					*bptr++ = yuv_to_rgb( (y0+(y2*3))/4, cr, cb );
					*bptr++ = yuv_to_rgb( (y1+(y3*3))/4, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)vq4;
				dptr = (unsigned short *)vq8;
		
				for(i=0;i<four;i++) {
					aptr = (unsigned short *)vq2 + (*input++)*8;
					bptr = (unsigned short *)vq2 + (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(aptr,bptr,cptr,dptr);
						VQ2TO4(aptr,bptr,cptr,dptr);
					}
				}
			} else if (cinTable[currentHandle].samplesPerPixel==4) {
				ibptr.s = bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ((y0*3)+y2)/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ((y1*3)+y3)/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( (y0+(y2*3))/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( (y1+(y3*3))/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;
	
				for(i=0;i<four;i++) {
					iaptr.s = vq2;
					iaptr.i += (*input++)*8;
					ibptr.s = vq2;
					ibptr.i += (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
					}
				}
			} else if (cinTable[currentHandle].samplesPerPixel==1) {
				bbptr = (byte *)bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input; input+= 3;
					*bbptr++ = cinTable[currentHandle].gray[y0];
					*bbptr++ = cinTable[currentHandle].gray[y1];
					*bbptr++ = cinTable[currentHandle].gray[((y0*3)+y2)/4];
					*bbptr++ = cinTable[currentHandle].gray[((y1*3)+y3)/4];
					*bbptr++ = cinTable[currentHandle].gray[(y0+(y2*3))/4];
					*bbptr++ = cinTable[currentHandle].gray[(y1+(y3*3))/4];						
					*bbptr++ = cinTable[currentHandle].gray[y2];
					*bbptr++ = cinTable[currentHandle].gray[y3];
				}

				bcptr = (byte *)vq4;
				bdptr = (byte *)vq8;
	
				for(i=0;i<four;i++) {
					baptr = (byte *)vq2 + (*input++)*8;
					bbptr = (byte *)vq2 + (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
					}
				}
			}			
		}
	} else {
//
// 1/4 screen
//
		if (cinTable[currentHandle].samplesPerPixel==2) {
			for(i=0;i<two;i++) {
				y0 = (long)*input; input+=2;
				y2 = (long)*input; input+=2;
				cr = (long)*input++;
				cb = (long)*input++;
				*bptr++ = yuv_to_rgb( y0, cr, cb );
				*bptr++ = yuv_to_rgb( y2, cr, cb );
			}

			cptr = (unsigned short *)vq4;
			dptr = (unsigned short *)vq8;
	
			for(i=0;i<four;i++) {
				aptr = (unsigned short *)vq2 + (*input++)*2;
				bptr = (unsigned short *)vq2 + (*input++)*2;
				for(j=0;j<2;j++) { 
					VQ2TO2(aptr,bptr,cptr,dptr);
				}
			}
		} else if (cinTable[currentHandle].samplesPerPixel == 1) {
			bbptr = (byte *)bptr;
				
			for(i=0;i<two;i++) {
				*bbptr++ = cinTable[currentHandle].gray[*input]; input+=2;
				*bbptr++ = cinTable[currentHandle].gray[*input]; input+=4;
			}

			bcptr = (byte *)vq4;
			bdptr = (byte *)vq8;
	
			for(i=0;i<four;i++) {
				baptr = (byte *)vq2 + (*input++)*2;
				bbptr = (byte *)vq2 + (*input++)*2;
				for(j=0;j<2;j++) { 
					VQ2TO2(baptr,bbptr,bcptr,bdptr);
				}
			}			
		} else if (cinTable[currentHandle].samplesPerPixel == 4) {
			ibptr.s = bptr;
			for(i=0;i<two;i++) {
				y0 = (long)*input; input+=2;
				y2 = (long)*input; input+=2;
				cr = (long)*input++;
				cb = (long)*input++;
				*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr.s = vq4;
			idptr.s = vq8;
	
			for(i=0;i<four;i++) {
				iaptr.s = vq2;
				iaptr.i += (*input++)*2;
				ibptr.s = vq2 + (*input++)*2;
				ibptr.i += (*input++)*2;
				for(j=0;j<2;j++) { 
					VQ2TO2(iaptr.i,ibptr.i,icptr.i,idptr.i);
				}
			}
		}
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void recurseQuad( long startX, long startY, long quadSize, long xOff, long yOff )
{
	byte *scroff;
	long bigx, bigy, lowx, lowy, useY;
	long offset;

	offset = cinTable[currentHandle].screenDelta;
	
	lowx = lowy = 0;
	bigx = cinTable[currentHandle].xsize;
	bigy = cinTable[currentHandle].ysize;

	if (bigx > cinTable[currentHandle].CIN_WIDTH) bigx = cinTable[currentHandle].CIN_WIDTH;
	if (bigy > cinTable[currentHandle].CIN_HEIGHT) bigy = cinTable[currentHandle].CIN_HEIGHT;

	if ( (startX >= lowx) && (startX+quadSize) <= (bigx) && (startY+quadSize) <= (bigy) && (startY >= lowy) && quadSize <= MAXSIZE) {
		useY = startY;
		scroff = cin[currentHandle].linbuf + (useY+((cinTable[currentHandle].CIN_HEIGHT-bigy)>>1)+yOff)*(cinTable[currentHandle].samplesPerLine) + (((startX+xOff))*cinTable[currentHandle].samplesPerPixel);

		cin[currentHandle].qStatus[0][cinTable[currentHandle].onQuad  ] = scroff;
		cin[currentHandle].qStatus[1][cinTable[currentHandle].onQuad++] = scroff+offset;
	}

	if ( quadSize != MINSIZE ) {
		quadSize >>= 1;
		recurseQuad( startX,		  startY		  , quadSize, xOff, yOff );
		recurseQuad( startX+quadSize, startY		  , quadSize, xOff, yOff );
		recurseQuad( startX,		  startY+quadSize , quadSize, xOff, yOff );
		recurseQuad( startX+quadSize, startY+quadSize , quadSize, xOff, yOff );
	}
}


/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void setupQuad( long xOff, long yOff )
{
	long numQuadCels, i,x,y;
	byte *temp;

	if (xOff == cin[currentHandle].oldXOff && yOff == cin[currentHandle].oldYOff && cinTable[currentHandle].ysize == cin[currentHandle].oldysize && cinTable[currentHandle].xsize == cin[currentHandle].oldxsize) {
		return;
	}

	cin[currentHandle].oldXOff = xOff;
	cin[currentHandle].oldYOff = yOff;
	cin[currentHandle].oldysize = cinTable[currentHandle].ysize;
	cin[currentHandle].oldxsize = cinTable[currentHandle].xsize;

	numQuadCels  = (cinTable[currentHandle].xsize*cinTable[currentHandle].ysize) / (16);
	numQuadCels += numQuadCels/4;
	numQuadCels += 64;							  // for overflow

	cinTable[currentHandle].onQuad = 0;

	for(y=0;y<(long)cinTable[currentHandle].ysize;y+=16) 
		for(x=0;x<(long)cinTable[currentHandle].xsize;x+=16) 
			recurseQuad( x, y, 16, xOff, yOff );

	temp = NULL;

	for(i=(numQuadCels-64);i<numQuadCels;i++) {
		cin[currentHandle].qStatus[0][i] = temp;			  // eoq
		cin[currentHandle].qStatus[1][i] = temp;			  // eoq
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void readQuadInfo( const byte *qData )
{
	if (currentHandle < 0) return;

	cinTable[currentHandle].xsize    = qData[0]+qData[1]*256;
	cinTable[currentHandle].ysize    = qData[2]+qData[3]*256;
	cinTable[currentHandle].maxsize  = qData[4]+qData[5]*256;
	cinTable[currentHandle].minsize  = qData[6]+qData[7]*256;
	
	cinTable[currentHandle].CIN_HEIGHT = cinTable[currentHandle].ysize;
	cinTable[currentHandle].CIN_WIDTH  = cinTable[currentHandle].xsize;

	cinTable[currentHandle].samplesPerLine = cinTable[currentHandle].CIN_WIDTH*cinTable[currentHandle].samplesPerPixel;
	cinTable[currentHandle].screenDelta = cinTable[currentHandle].CIN_HEIGHT*cinTable[currentHandle].samplesPerLine;

	cinTable[currentHandle].half = qfalse;
	cinTable[currentHandle].smootheddouble = qfalse;
	
	cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
	cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;

	cinTable[currentHandle].t[0] = cinTable[currentHandle].screenDelta;
	cinTable[currentHandle].t[1] = -cinTable[currentHandle].screenDelta;

        cinTable[currentHandle].drawX = cinTable[currentHandle].CIN_WIDTH;
        cinTable[currentHandle].drawY = cinTable[currentHandle].CIN_HEIGHT;
        
	// rage pro is very slow at 512 wide textures, voodoo can't do it at all
	if ( cls.glconfig.hardwareType == GLHW_RAGEPRO || cls.glconfig.maxTextureSize <= 256) {
                if (cinTable[currentHandle].drawX>256) {
                        cinTable[currentHandle].drawX = 256;
                }
                if (cinTable[currentHandle].drawY>256) {
                        cinTable[currentHandle].drawY = 256;
                }
		if (cinTable[currentHandle].CIN_WIDTH != 256 || cinTable[currentHandle].CIN_HEIGHT != 256) {
			Com_Printf("HACK: approxmimating cinematic for Rage Pro or Voodoo\n");
		}
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQPrepMcomp( long xoff, long yoff ) 
{
	long i, j, x, y, temp, temp2;

	i=cinTable[currentHandle].samplesPerLine; j=cinTable[currentHandle].samplesPerPixel;
	if ( cinTable[currentHandle].xsize == (cinTable[currentHandle].ysize*4) && !cinTable[currentHandle].half ) { j = j+j; i = i+i; }
	
	for(y=0;y<16;y++) {
		temp2 = (y+yoff-8)*i;
		for(x=0;x<16;x++) {
			temp = (x+xoff-8)*j;
			cin[currentHandle].mcomp[(x*16)+y] = cinTable[currentHandle].normalBuffer0-(temp2+temp);
		}
	}
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void initRoQ( void ) 
{
	if (currentHandle < 0) return;

	cinTable[currentHandle].VQNormal = (void (*)(byte *, void *))blitVQQuad32fs;
	cinTable[currentHandle].VQBuffer = (void (*)(byte *, void *))blitVQQuad32fs;
	cinTable[currentHandle].samplesPerPixel = 4;
	ROQ_GenYUVTables();
	RllSetupTable();
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/
/*
static byte* RoQFetchInterlaced( byte *source ) {
	int x, *src, *dst;

	if (currentHandle < 0) return NULL;

	src = (int *)source;
	dst = (int *)cinTable[currentHandle].buf2;

	for(x=0;x<256*256;x++) {
		*dst = *src;
		dst++; src += 2;
	}
	return cinTable[currentHandle].buf2;
}
*/
static void RoQReset( void ) {
	
	if (currentHandle < 0) return;

	FS_FCloseFile( cinTable[currentHandle].iFile );
	FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);
	// let the background thread start reading ahead
	FS_Read (cin[currentHandle].file, 16, cinTable[currentHandle].iFile);
	RoQ_init();
	cinTable[currentHandle].status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQInterrupt(void)
{
	byte				*framedata;
        short		sbuf[32768];
        int		ssize;
		qboolean testParse;

	if (currentHandle < 0) return;

	// need additional check here since this could be set in RoQ_init()
	//FIXME also check cinTable[currentHandle].RoQFRameSize == 0 ?
	if (cinTable[currentHandle].RoQFrameSize > MAX_FRAME_SIZE) {
		Com_Printf("^1%s: invalid RoQFrameSize: %d\n", __FUNCTION__, cinTable[currentHandle].RoQFrameSize);
		cinTable[currentHandle].status = FMV_EOF;
	}

	FS_Read( cin[currentHandle].file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) { 
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return; 
	}

	framedata = cin[currentHandle].file;
	testParse = cinTable[currentHandle].testParse;
//
// new frame is ready
//
redump:
	switch(cinTable[currentHandle].roq_id) 
	{
		case	ROQ_QUAD_VQ:
			if (testParse) {
				cinTable[currentHandle].numQuads++;
				cinTable[currentHandle].totalQuads++;
				cinTable[currentHandle].dirty = qtrue;
				break;
			}

			if ((cinTable[currentHandle].numQuads&1)) {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[1];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ1( (byte *)cin[currentHandle].qStatus[1], framedata);
				cinTable[currentHandle].buf = 	cin[currentHandle].linbuf + cinTable[currentHandle].screenDelta;
			} else {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[0];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ0( (byte *)cin[currentHandle].qStatus[0], framedata );
				cinTable[currentHandle].buf = 	cin[currentHandle].linbuf;
			}
			if (cinTable[currentHandle].numQuads == 0) {		// first frame
				Com_Memcpy(cin[currentHandle].linbuf+cinTable[currentHandle].screenDelta, cin[currentHandle].linbuf, cinTable[currentHandle].samplesPerLine*cinTable[currentHandle].ysize);
			}

			cinTable[currentHandle].numQuads++;
			cinTable[currentHandle].dirty = qtrue;
			break;
		case	ROQ_CODEBOOK:
			if (testParse) {
				break;
			}

			decodeCodeBook( framedata, (unsigned short)cinTable[currentHandle].roq_flags );
			break;
		case	ZA_SOUND_MONO:
			if (testParse) {
				break;
			}

			if (!cinTable[currentHandle].silent) {
				if (cinTable[currentHandle].numQuads == -1) {
					S_Update();
					s_rawend[0] = s_soundtime;
				}
				ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
				S_RawSamples(0, ssize, 22050, 2, 1, (byte *)sbuf, 1.0f, -1);
			}
			break;
		case	ZA_SOUND_STEREO:
			if (testParse) {
				break;
			}

			if (!cinTable[currentHandle].silent) {
				if (cinTable[currentHandle].numQuads == -1) {
					S_Update();
					s_rawend[0] = s_soundtime;
				}
				ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
				S_RawSamples(0, ssize, 22050, 2, 2, (byte *)sbuf, 1.0f, -1);
			}
			break;
		case	ROQ_QUAD_INFO:
			if (cinTable[currentHandle].numQuads == -1) {
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
				cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds();
			}
			if (cinTable[currentHandle].numQuads != 1) cinTable[currentHandle].numQuads = 0;
			break;
		case	ROQ_PACKET:
			cinTable[currentHandle].inMemory = cinTable[currentHandle].roq_flags;
			cinTable[currentHandle].RoQFrameSize = 0;           // for header
			break;
		case	ROQ_QUAD_HANG:
			cinTable[currentHandle].RoQFrameSize = 0;
			break;
		case	ROQ_QUAD_JPEG:
			break;
		default:
			cinTable[currentHandle].status = FMV_EOF;
			break;
	}	
//
// read in next frame data
//
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) { 
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return; 
	}
	
	framedata		 += cinTable[currentHandle].RoQFrameSize;
	cinTable[currentHandle].roq_id		 = framedata[0] + framedata[1]*256;
	cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3]*256 + framedata[4]*65536;
	cinTable[currentHandle].roq_flags	 = framedata[6] + framedata[7]*256;
	cinTable[currentHandle].roqF0		 = (signed char)framedata[7];
	cinTable[currentHandle].roqF1		 = (signed char)framedata[6];

	//FIXME also check if cinTable[currentHandle].RoQFrameSize == 0 ?
	if (cinTable[currentHandle].RoQFrameSize > MAX_FRAME_SIZE || cinTable[currentHandle].roq_id==0x1084) {
		Com_DPrintf("roq_size>%d||roq_id==0x1084\n", MAX_FRAME_SIZE);
		cinTable[currentHandle].status = FMV_EOF;
		if (cinTable[currentHandle].looping) {
			RoQReset();
		}
		return;
	}
	if (cinTable[currentHandle].inMemory && (cinTable[currentHandle].status != FMV_EOF)) { cinTable[currentHandle].inMemory--; framedata += 8; goto redump; }
//
// one more frame hits the dust
//
//	assert(cinTable[currentHandle].RoQFrameSize <= MAX_FRAME_SIZE);
//	r = FS_Read( cin[currentHandle].file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	cinTable[currentHandle].RoQPlayed	+= cinTable[currentHandle].RoQFrameSize+8;
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQ_init( void )
{
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds();

	cinTable[currentHandle].RoQPlayed = 24;

/*	get frame rate */	
	cinTable[currentHandle].roqFPS	 = cin[currentHandle].file[ 6] + cin[currentHandle].file[ 7]*256;
	
	if (!cinTable[currentHandle].roqFPS) cinTable[currentHandle].roqFPS = 30;

	cinTable[currentHandle].numQuads = -1;

	cinTable[currentHandle].roq_id		= cin[currentHandle].file[ 8] + cin[currentHandle].file[ 9]*256;
	cinTable[currentHandle].RoQFrameSize	= cin[currentHandle].file[10] + cin[currentHandle].file[11]*256 + cin[currentHandle].file[12]*65536;
	cinTable[currentHandle].roq_flags	= cin[currentHandle].file[14] + cin[currentHandle].file[15]*256;

	if (cinTable[currentHandle].RoQFrameSize > MAX_FRAME_SIZE || !cinTable[currentHandle].RoQFrameSize) {
		Com_Printf("^3%s: invalid RoQFrameSize: %d\n", __FUNCTION__, cinTable[currentHandle].RoQFrameSize);
		return;
	}

}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQShutdown( void ) {
	const char *s;

	if (!cinTable[currentHandle].buf) {
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf("finished cinematic\n");
	cinTable[currentHandle].status = FMV_IDLE;

	if (cinTable[currentHandle].iFile) {
		FS_FCloseFile( cinTable[currentHandle].iFile );
		cinTable[currentHandle].iFile = 0;
	}

	if (cinTable[currentHandle].alterGameState) {
		clc.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString( "nextmap" );
		if ( s[0] ) {
			Cbuf_ExecuteText( EXEC_APPEND, va("%s\n", s) );
			Cvar_Set( "nextmap", "" );
		}
		CL_handle = -1;
	}
	cinTable[currentHandle].fileName[0] = 0;
	currentHandle = -1;
}

/*
==================
CIN_StopCinematic
==================
*/
e_status CIN_StopCinematic(int handle) {
	
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;
	currentHandle = handle;

	Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);

	if (!cinTable[currentHandle].buf) {
		return FMV_EOF;
	}

	if (cinTable[currentHandle].alterGameState) {
		if ( clc.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}
	cinTable[currentHandle].status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}

/*
==================
CIN_RunCinematic

Fetch and decompress the pending frame
==================
*/


e_status CIN_RunCinematic (int handle)
{
	int	start = 0;
	int     thisTime = 0;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;

	if (cinTable[handle].playonwalls < -1)
	{
		return cinTable[handle].status;
	}

	currentHandle = handle;

	if (cinTable[currentHandle].alterGameState) {
		if ( clc.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}

	if (cinTable[currentHandle].status == FMV_IDLE) {
		return cinTable[currentHandle].status;
	}

	if (cl_freezeDemo->integer) {
		return FMV_IDLE;
	}

	thisTime = CL_ScaledMilliseconds();

	if (thisTime < cinTable[currentHandle].lastTime) {
		// shouldn't happen with cls.scaledtime
		Com_Printf("^3%s: thisTime (%d) < lastTime (%f)\n", __FUNCTION__, thisTime, cinTable[currentHandle].lastTime);
	}

	// check for game hitch?
	if (cinTable[currentHandle].shader && ( (fabs(thisTime - cinTable[currentHandle].lastTime))>100.0 )) {

		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}

	//FIXME hardcoded to 30fps
	cinTable[currentHandle].tfps = ((((thisTime) - cinTable[currentHandle].startTime)*3)/100);

#if 0  // debugging
	if (cinTable[currentHandle].tfps > cinTable[currentHandle].totalQuads) {
		Com_Printf("^5%s:  greater total wanted time %ld > %ld  :  startTime:%f  timeNow:%d\n", __FUNCTION__, cinTable[currentHandle].tfps, cinTable[currentHandle].totalQuads, cinTable[currentHandle].startTime, CL_ScaledMilliseconds());
	}
#endif

	start = cinTable[currentHandle].startTime;
	while(  (cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads)
		&& (cinTable[currentHandle].status == FMV_PLAY) )
	{
		RoQInterrupt();
		if (start != cinTable[currentHandle].startTime) {
		  cinTable[currentHandle].tfps = ((((thisTime) - cinTable[currentHandle].startTime)*3)/100);
			start = cinTable[currentHandle].startTime;
		}
	}

	cinTable[currentHandle].lastTime = thisTime;

	if (cinTable[currentHandle].status == FMV_LOOPED) {
		cinTable[currentHandle].status = FMV_PLAY;
	}

	if (cinTable[currentHandle].status == FMV_EOF) {
	  if (cinTable[currentHandle].looping) {
		RoQReset();
	  } else {
		RoQShutdown();
		return FMV_EOF;
	  }
	}

	return cinTable[currentHandle].status;
}

/*
==================
CL_PlayCinematic

==================
*/
int CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits ) {
	unsigned short RoQID;
	char	name[MAX_OSPATH];
	int		i;

	if (strstr(arg, "/") == NULL && strstr(arg, "\\") == NULL) {
		Com_sprintf (name, sizeof(name), "video/%s", arg);
	} else {
		Com_sprintf (name, sizeof(name), "%s", arg);
	}

	if (!(systemBits & CIN_system)) {
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
			if (!strcmp(cinTable[i].fileName, name) ) {
				return i;
			}
		}
	}

	Com_DPrintf("CIN_PlayCinematic( %s )\n", arg);

	currentHandle = CIN_HandleForVideo();

	if (currentHandle == -1) {
		Com_Printf("^3CIN_PlayCinematic: no available handles\n");
		return -1;
	}

	Com_Memset(&cin[currentHandle], 0, sizeof(cinematics_t));

	strcpy(cinTable[currentHandle].fileName, name);

	cinTable[currentHandle].ROQSize = 0;
	cinTable[currentHandle].ROQSize = FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);

	if (cinTable[currentHandle].ROQSize<=0) {
		Com_DPrintf("play(%s), ROQSize<=0\n", arg);
		cinTable[currentHandle].fileName[0] = 0;
		return -1;
	}

	CIN_SetExtents(currentHandle, x, y, w, h);
	CIN_SetLooping(currentHandle, (systemBits & CIN_loop)!=0);

	cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	cinTable[currentHandle].CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
	cinTable[currentHandle].holdAtEnd = (systemBits & CIN_hold) != 0;
	cinTable[currentHandle].alterGameState = (systemBits & CIN_system) != 0;
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].silent = (systemBits & CIN_silent) != 0;
	cinTable[currentHandle].shader = (systemBits & CIN_shader) != 0;

	if (cinTable[currentHandle].alterGameState) {
		// close the menu
		if ( uivm ) {
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
		}
	} else {
		cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
	}

	initRoQ();

	FS_Read (cin[currentHandle].file, 16, cinTable[currentHandle].iFile);

	RoQID = (unsigned short)(cin[currentHandle].file[0]) + (unsigned short)(cin[currentHandle].file[1])*256;
	if (RoQID == 0x1084)
	{
		RoQ_init();
//		FS_Read (cin[currentHandle].file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

		cinTable[currentHandle].status = FMV_PLAY;
		Com_DPrintf("trFMV::play(), playing %s\n", arg);

		if (cinTable[currentHandle].alterGameState) {
			clc.state = CA_CINEMATIC;
		}

		if (cinTable[currentHandle].alterGameState) {
			Con_Close();
		}

		if (!cinTable[currentHandle].silent) {
			s_rawend[0] = s_soundtime;
		}

		// test parse to get total quads
		{
			qboolean loopingOrig;

			loopingOrig = cinTable[currentHandle].looping;
			cinTable[currentHandle].looping = qfalse;  // to allow FMV_EOF status
			cinTable[currentHandle].testParse = qtrue;
			cinTable[currentHandle].totalQuads = 0;

			// video can end with FMV_EOF or FMV_IDLE (holdAtEnd option)
			while (cinTable[currentHandle].status == FMV_PLAY) {
				// this is incrementing totalQuads
				RoQInterrupt();
			}

			RoQReset();
			cinTable[currentHandle].testParse = qfalse;
			cinTable[currentHandle].looping = loopingOrig;
			cinTable[currentHandle].playCallStartTime = CL_ScaledMilliseconds();
			cinTable[currentHandle].status = FMV_PLAY;

			//Com_Printf("^5cinematic total quads: %ld\n", cinTable[currentHandle].totalQuads);
		}
		return currentHandle;
	}
	Com_DPrintf("trFMV::play(), invalid RoQ ID\n");

	RoQShutdown();
	return -1;
}

// milliseconds from current position
void CIN_SeekCinematic (double offset)
{
	long int wantQuadCount;
	qboolean silentOrig;
	long int currentQuads;
	int currentHandleOrig;
	// debugging
	//int startTimeOrig;
	int i;
	double totalWantedTime;
	double totalVideoTimeLength;

	currentHandleOrig = currentHandle;

	for (i = 0;  i < MAX_VIDEO_HANDLES;  i++) {
		currentHandle = i;

		if (cinTable[currentHandle].fileName[0] == 0) {
			continue;
		}

		if (cinTable[currentHandle].totalQuads < 2) {
			// avoid infinite loop with empty roq and not enough precision with low number of quads
			continue;
		}

		if (cinTable[currentHandle].shader  &&  cl_cinematicIgnoreSeek->integer) {
			continue;
		}

		totalWantedTime = (double)cinTable[currentHandle].lastTime - (double)cinTable[currentHandle].startTime + offset;

		totalVideoTimeLength = (double)cinTable[currentHandle].totalQuads * (1000.0 / 30.0);

		if (totalWantedTime < 0.0) {
			if (!cinTable[currentHandle].looping) {
				// ignore seek before start time
				continue;
			}

			// loop it
			while (totalWantedTime < 0.0) {
				totalWantedTime += totalVideoTimeLength;
			}
		} else if (totalWantedTime > totalVideoTimeLength) {
			if (!cinTable[currentHandle].looping) {
				// ignore seek after end time
				continue;
			}

			// loop it
			while (totalWantedTime > totalVideoTimeLength) {
				totalWantedTime -= totalVideoTimeLength;
			}
		}

		wantQuadCount = totalWantedTime * 30.0 / 1000.0;

		// note:  numQuads can be -1 at beginning of cinematic
		currentQuads = cinTable[currentHandle].numQuads;
		if (currentQuads < 0) {
			currentQuads = 0;
		}

		// debugging
		//startTimeOrig = cinTable[currentHandle].startTime;

		if (wantQuadCount > currentQuads) {  // fast forward
			if (Cvar_VariableIntegerValue("debug_seek")) {
				Com_Printf("cinematic fast forward %ld -> %ld\n", cinTable[currentHandle].numQuads, wantQuadCount);
			}

			silentOrig = cinTable[currentHandle].silent;
			cinTable[currentHandle].silent = qtrue;

			while (wantQuadCount > cinTable[currentHandle].numQuads  &&  cinTable[currentHandle].status == FMV_PLAY) {
				RoQInterrupt();
			}

			cinTable[currentHandle].silent = silentOrig;
		} else if (wantQuadCount < currentQuads) {  // rewind
			if (Cvar_VariableIntegerValue("debug_seek")) {
				Com_Printf("cinematic rewind %ld -> %ld\n", cinTable[currentHandle].numQuads, wantQuadCount);
			}

			silentOrig = cinTable[currentHandle].silent;

			RoQReset();

			cinTable[currentHandle].silent = qtrue;
			cinTable[currentHandle].status = FMV_PLAY;

			while (wantQuadCount > cinTable[currentHandle].numQuads  &&  cinTable[currentHandle].status == FMV_PLAY) {
				RoQInterrupt();
			}

			cinTable[currentHandle].silent = silentOrig;
		} else {
			// quad counts the same, still need to update time
		}

		cinTable[currentHandle].startTime = (double)CL_ScaledMilliseconds() - totalWantedTime;
		cinTable[currentHandle].lastTime = (double)CL_ScaledMilliseconds();
		//Com_Printf("^5setting time  startTimeOrig:%f  startTime:%f  now:%d\n", startTimeOrig, cinTable[currentHandle].startTime, CL_ScaledMilliseconds());
	}

	currentHandle = currentHandleOrig;
}

void CIN_SetExtents (int handle, int x, int y, int w, int h) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].dirty = qtrue;
}

void CIN_SetLooping(int handle, qboolean loop) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].looping = loop;
}

/*
==================
CIN_ResampleCinematic

Resample cinematic to 256x256 and store in buf2
==================
*/
void CIN_ResampleCinematic(int handle, int *buf2) {
	int ix, iy, *buf3, xm, ym, ll;
	byte	*buf;

	buf = cinTable[handle].buf;

	xm = cinTable[handle].CIN_WIDTH/256;
	ym = cinTable[handle].CIN_HEIGHT/256;
	ll = 8;
	if (cinTable[handle].CIN_WIDTH==512) {
		ll = 9;
	}

	buf3 = (int*)buf;
	if (xm==2 && ym==2) {
		byte *bc2, *bc3;
		int	ic, iiy;

		bc2 = (byte *)buf2;
		bc3 = (byte *)buf3;
		for (iy = 0; iy<256; iy++) {
			iiy = iy<<12;
			for (ix = 0; ix<2048; ix+=8) {
				for(ic = ix;ic<(ix+4);ic++) {
					*bc2=(bc3[iiy+ic]+bc3[iiy+4+ic]+bc3[iiy+2048+ic]+bc3[iiy+2048+4+ic])>>2;
					bc2++;
				}
			}
		}
	} else if (xm==2 && ym==1) {
		byte *bc2, *bc3;
		int	ic, iiy;

		bc2 = (byte *)buf2;
		bc3 = (byte *)buf3;
		for (iy = 0; iy<256; iy++) {
			iiy = iy<<11;
			for (ix = 0; ix<2048; ix+=8) {
				for(ic = ix;ic<(ix+4);ic++) {
					*bc2=(bc3[iiy+ic]+bc3[iiy+4+ic])>>1;
					bc2++;
				}
			}
		}
	} else {
		for (iy = 0; iy<256; iy++) {
			for (ix = 0; ix<256; ix++) {
					buf2[(iy<<8)+ix] = buf3[((iy*ym)<<ll) + (ix*xm)];
			}
		}
	}
}

/*
==================
CIN_DrawCinematic

==================
*/
void CIN_DrawCinematic (int handle) {
	float	x, y, w, h;
	byte	*buf;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;

	if (!cinTable[handle].buf) {
		return;
	}

	x = cinTable[handle].xpos;
	y = cinTable[handle].ypos;
	w = cinTable[handle].width;
	h = cinTable[handle].height;
	buf = cinTable[handle].buf;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	if (cinTable[handle].dirty && (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY)) {
		int *buf2;

		buf2 = Hunk_AllocateTempMemory( 256*256*4 );

		CIN_ResampleCinematic(handle, buf2);

		re.DrawStretchRaw( x, y, w, h, 256, 256, (byte *)buf2, handle, qtrue);
		cinTable[handle].dirty = qfalse;
		Hunk_FreeTempMemory(buf2);
		return;
	}

	re.DrawStretchRaw( x, y, w, h, cinTable[handle].drawX, cinTable[handle].drawY, buf, handle, cinTable[handle].dirty);
	cinTable[handle].dirty = qfalse;
}

void CL_PlayCinematic_f(void) {
	char	*arg, *s;
	int bits = CIN_system;

	Com_DPrintf("CL_PlayCinematic_f\n");
	if (clc.state == CA_CINEMATIC) {
		SCR_StopCinematic();
	}

	arg = Cmd_Argv( 1 );
	s = Cmd_Argv(2);

	if ((s && s[0] == '1') || Q_stricmp(arg,"demoend.roq")==0 || Q_stricmp(arg,"end.roq")==0) {
		bits |= CIN_hold;
	}
	if (s && s[0] == '2') {
		bits |= CIN_loop;
	}

	S_StopAllSounds ();

	CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );
	if (CL_handle >= 0) {
		// don't loop this like quake3 since CL_ScaledMilliseconds() will not advance until CL_Frame() is called again
		SCR_RunCinematic();
	}
}

void CL_RestartCinematic_f (void)
{
	int i;
	int currentHandleOrig;
	char *arg;
	int startNum;
	int endNum;

	if (Cmd_Argc() < 2) {
		Com_Printf("cinematic_restart [all | <cinematic number>]\n");
		return;
	}

	startNum = 0;
	endNum = MAX_VIDEO_HANDLES;

	arg = Cmd_Argv(1);
	if (!Q_stricmpn(arg, "all", 3)) {
		// pass
	} else {
		startNum = atoi(arg);
		if (startNum < 0  ||  startNum >= MAX_VIDEO_HANDLES) {
			Com_Printf("invalid cinematic number\n");
			return;
		}
	}

	currentHandleOrig = currentHandle;

	for (i = startNum;  i < endNum;  i++) {
		if (cinTable[i].fileName[0] != 0) {
			currentHandle = i;
			cinTable[currentHandle].playCallStartTime = CL_ScaledMilliseconds();
			RoQReset();
		}
	}

	currentHandle = currentHandleOrig;
}

void CL_ListCinematic_f (void)
{
	int i;

	for (i = 0;  i < MAX_VIDEO_HANDLES;  i++) {
		if (cinTable[i].fileName[0] != 0) {
			const cin_cache *c = &cinTable[i];

			Com_Printf("%02d: '%s' status:%d file:%d totalFrames:%ld frameCount:%ld startTime:%f lastTime:%f playStartTime:%f\n", i, c->fileName, c->status, c->iFile, c->totalQuads, c->numQuads, c->startTime, c->lastTime, c->playCallStartTime);
		}
	}
}

void SCR_DrawCinematic (void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_DrawCinematic(CL_handle);
	}
}

void SCR_RunCinematic (void)
{
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_RunCinematic(CL_handle);
	}
}

void SCR_StopCinematic(void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_StopCinematic(CL_handle);
		S_StopAllSounds ();
		CL_handle = -1;
	}
}

void CIN_UploadCinematic(int handle) {
	if (handle >= 0 && handle < MAX_VIDEO_HANDLES) {
		if (!cinTable[handle].buf) {
			return;
		}
		if (cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty) {
			if (cinTable[handle].playonwalls == 0) {
				cinTable[handle].playonwalls = -1;
			} else {
				if (cinTable[handle].playonwalls == -1) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].dirty = qfalse;
				}
			}
		}

		// Resample the video if needed
		if (cinTable[handle].dirty && (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY)) {
			int *buf2;

			buf2 = Hunk_AllocateTempMemory( 256*256*4 );

			CIN_ResampleCinematic(handle, buf2);

			re.UploadCinematic( cinTable[handle].CIN_WIDTH, cinTable[handle].CIN_HEIGHT, 256, 256, (byte *)buf2, handle, qtrue);
			cinTable[handle].dirty = qfalse;
			Hunk_FreeTempMemory(buf2);
		} else {
			// Upload video at normal resolution
			re.UploadCinematic( cinTable[handle].CIN_WIDTH, cinTable[handle].CIN_HEIGHT, cinTable[handle].drawX, cinTable[handle].drawY,
								cinTable[handle].buf, handle, cinTable[handle].dirty);
			cinTable[handle].dirty = qfalse;
		}

		if (cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1) {
			cinTable[handle].playonwalls--;
		} else if (cl_inGameVideo->integer != 0 && cinTable[handle].playonwalls != 1) {
			cinTable[handle].playonwalls = 1;
		}
	}
}


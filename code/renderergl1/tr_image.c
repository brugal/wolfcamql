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
// tr_image.c
#include "tr_local.h"

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];
static byte			 s_lightmapintensitytable[256];
static unsigned char s_lightmapgammatable[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE		1024
static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	if (r_enablePostProcess->integer  &&  r_enableColorCorrect->integer  &&  glConfig.glsl) {
		return;
	}

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

void R_GammaCorrectLightMap (byte *buffer, int bufSize)
{
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_lightmapgammatable[buffer[i]];
	}
}

typedef struct {
	char *name;
	int	minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int		i;
	image_t	*glt;

	for ( i=0 ; i< 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	// hack to prevent trilinear from being set on voodoo,
	// because their driver freaks...
	if ( i == 5 && glConfig.hardwareType == GLHW_3DFX_2D3D ) {
		ri.Printf( PRINT_ALL, "Refusing to set trilinear on a voodoo.\n" );
		i = 3;
	}


	if ( i == 6 ) {
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;
	//gl_filter_min = gl_filter_max = GL_LINEAR_MIPMAP_LINEAR;  // testing

	// change all the existing mipmap texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		if ( glt->flags & IMGFLAG_MIPMAP ) {
			GL_Bind (glt);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int i;
	int estTotalSize = 0;

	ri.Printf(PRINT_ALL, "\n      -w-- -h-- type  -size- --name-------\n");

	for ( i = 0 ; i < tr.numImages ; i++ )
	{
		image_t *image = tr.images[i];
		char *format = "???? ";
		char *sizeSuffix;
		int estSize;
		int displaySize;

		estSize = image->uploadHeight * image->uploadWidth;

		switch(image->internalFormat)
		{
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
				format = "sDXT1";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
				format = "sDXT5";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
				format = "sBPTC";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
				format = "LATC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				format = "DXT1 ";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				format = "DXT5 ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
				format = "BPTC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_RGB4_S3TC:
				format = "S3TC ";
				// same as DXT1?
				estSize /= 2;
				break;
			case GL_RGBA4:
			case GL_RGBA8:
			case GL_RGBA:
				format = "RGBA ";
				// 4 bytes per pixel
				estSize *= 4;
				break;
			case GL_LUMINANCE8:
			case GL_LUMINANCE16:
			case GL_LUMINANCE:
				format = "L    ";
				// 1 byte per pixel?
				break;
			case GL_RGB5:
			case GL_RGB8:
			case GL_RGB:
				format = "RGB  ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_LUMINANCE8_ALPHA8:
			case GL_LUMINANCE16_ALPHA16:
			case GL_LUMINANCE_ALPHA:
				format = "LA   ";
				// 2 bytes per pixel?
				estSize *= 2;
				break;
			case GL_SRGB_EXT:
			case GL_SRGB8_EXT:
				format = "sRGB ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_SRGB_ALPHA_EXT:
			case GL_SRGB8_ALPHA8_EXT:
				format = "sRGBA";
				// 4 bytes per pixel?
				estSize *= 4;
				break;
			case GL_SLUMINANCE_EXT:
			case GL_SLUMINANCE8_EXT:
				format = "sL   ";
				// 1 byte per pixel?
				break;
			case GL_SLUMINANCE_ALPHA_EXT:
			case GL_SLUMINANCE8_ALPHA8_EXT:
				format = "sLA  ";
				// 2 byte per pixel?
				estSize *= 2;
				break;
		}

		// mipmap adds about 50%
		if (image->flags & IMGFLAG_MIPMAP)
			estSize += estSize / 2;

		sizeSuffix = "b ";
		displaySize = estSize;

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "kb";
		}

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "Mb";
		}

		if (displaySize > 1024)
		{
			displaySize /= 1024;
			sizeSuffix = "Gb";
		}

		ri.Printf(PRINT_ALL, "%4i: %4ix%4i %s %4i%s %s\n", i, image->uploadWidth, image->uploadHeight, format, displaySize, sizeSuffix, image->imgName);
		estTotalSize += estSize;
	}

	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " approx %i bytes\n", estTotalSize);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function 
before or after.
================
*/
extern image_t *CurrentBoundImage;
//static unsigned p1[4096];
//static unsigned p2[4096];

static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,  
							int outwidth, int outheight ) {
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[2048], p2[2048];
	byte		*pix1, *pix2, *pix3, *pix4;

	if (outwidth > 2048) {
		ri.Error(ERR_DROP, "ResampleTexture(%d): max width (%d > 2048)  '%s'", CurrentBoundImage->texnum, outwidth, CurrentBoundImage->imgName);
	}
	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth) {
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		for (j=0 ; j<outwidth ; j++) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = (byte *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap (byte *in, int width, int height) {
	int		i, j;
	byte	*out;
	int		row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};

/*
===============
Upload32

===============
*/
void R_Upload32( unsigned *data, 
						  int width, int height, 
						  qboolean mipmap, 
						  qboolean picmip, 
							qboolean lightMap,
						  qboolean allowCompression,
						  int *format, 
						  int *pUploadWidth, int *pUploadHeight )
{
	int			samples;
	unsigned	*scaledBuffer = NULL;
	unsigned	*resampledBuffer = NULL;
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	GLenum		internalFormat = GL_RGB;
	float		rMax = 0, gMax = 0, bMax = 0;
	qboolean powerOfTwoScale;

	powerOfTwoScale = qtrue;

	// allow non power of two textures for hardware that supposedly supports it
	//FIXME also check major < 2 and arb extension?  -- no, would still have
	// limited support

	/*
	 * http://aras-p.info/blog/2012/10/17/non-power-of-two-textures/
	 *
	 * "Then the question of course is, how to detect DX10+ level GPU in
	 * OpenGL? If you’re using OpenGL 3+, then you are on DX10+ GPU. In
	 * earlier GL versions, you’d have to use some heuristics. For example, if
	 * you have ARB_fragment_program and
	 * GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB is less than 4096 is a pretty
	 * good indicator of pre-DX10 hardware, on Mac OS X at least. Likewise,
	 * you could query MAX_TEXTURE_SIZE, lower than 8192 is a good indicator
	 * for pre-DX10."
	 */

	if (glConfig.openGLMajorVersion >= 3  &&  r_scaleImagesPowerOfTwo->integer == 0) {
		powerOfTwoScale = qfalse;
	}

	// allow non power of two textures even if hardware doesn't support it
	if (r_scaleImagesPowerOfTwo->integer == -1) {
		powerOfTwoScale = qfalse;
	}

	//
	// convert to exact power of 2 sizes
	//
	if (powerOfTwoScale) {
		for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
			;
		for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
			;
		if ( r_roundImagesDown->integer && scaled_width > width )
			scaled_width >>= 1;
		if ( r_roundImagesDown->integer && scaled_height > height )
			scaled_height >>= 1;
	} else {
		scaled_width = width;
		scaled_height = height;
	}



	if ( scaled_width != width || scaled_height != height ) {
		int sw, sh;

		sw = scaled_width;
		sh = scaled_height;

		//FIXME 2048  change in ResampleTexture as well and get from glconfig
		while (scaled_width > 2048) {
			scaled_width >>= 1;
		}
		while (scaled_height > 2048) {
			scaled_height >>= 1;
		}

		if (sw != scaled_width  ||  sh != scaled_height) {
			//ri.Printf(PRINT_ALL, "^3'%s' had to be rounded down\n", CurrentBoundImage->imgName);
		}
		resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		ResampleTexture (data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;

		if (r_debugScaledImages->integer  &&  (scaled_width != width  ||  scaled_height != height)) {
			int i;
			unsigned char *sd = (unsigned char *)data;

			for (i = 0;  i < (scaled_width * scaled_height * 4);  i += 4) {
				sd[i + 0] = 255;
				sd[i + 1] = 255;
				sd[i + 2] = 0;
			}
		}

		width = scaled_width;
		height = scaled_height;

	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize
		|| scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((byte *)data);
	samples = 3;

	if( r_greyscale->integer )
	{
		for ( i = 0; i < c; i++ )
		{
			byte luma = LUMA(scan[i*4], scan[i*4 + 1], scan[i*4 + 2]);
			scan[i*4] = luma;
			scan[i*4 + 1] = luma;
			scan[i*4 + 2] = luma;
		}
	}
	else if( r_greyscale->value )
	{
		for ( i = 0; i < c; i++ )
		{
			float luma = LUMA(scan[i*4], scan[i*4 + 1], scan[i*4 + 2]);
			scan[i*4] = LERP(scan[i*4], luma, r_greyscale->value);
			scan[i*4 + 1] = LERP(scan[i*4 + 1], luma, r_greyscale->value);
			scan[i*4 + 2] = LERP(scan[i*4 + 2], luma, r_greyscale->value);
		}
	}

	if(lightMap)
	{
		if(r_greyscale->integer == 1  ||  (r_greyscale->integer == 2  &&  !lightMap))
			internalFormat = GL_LUMINANCE;
		else
			internalFormat = GL_RGB;
	}
	else
	{
		for ( i = 0; i < c; i++ )
		{
			if ( scan[i*4+0] > rMax )
			{
				rMax = scan[i*4+0];
			}
			if ( scan[i*4+1] > gMax )
			{
				gMax = scan[i*4+1];
			}
			if ( scan[i*4+2] > bMax )
			{
				bMax = scan[i*4+2];
			}
			if ( scan[i*4 + 3] != 255 ) 
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if ( samples == 3 )
		{
			if (r_greyscale->integer == 1  ||  (r_greyscale->integer == 2  &&  !lightMap))  //(r_greyscale->integer)
			{
				if(r_texturebits->integer == 16)
					internalFormat = GL_LUMINANCE8;
				else if(r_texturebits->integer == 32)
					internalFormat = GL_LUMINANCE16;
				else
					internalFormat = GL_LUMINANCE;
			}
			else
			{
				if ( allowCompression && glConfig.textureCompression == TC_S3TC_ARB )
				{
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				}
				else if ( allowCompression && glConfig.textureCompression == TC_S3TC )
				{
					internalFormat = GL_RGB4_S3TC;
				}
				else if ( r_texturebits->integer == 16 )
				{
					internalFormat = GL_RGB5;
				}
				else if ( r_texturebits->integer == 32 )
				{
					internalFormat = GL_RGB8;
				}
				else
				{
					internalFormat = GL_RGB;
				}
			}
		}
		else if ( samples == 4 )
		{
			if (r_greyscale->integer == 1  ||  (r_greyscale->integer == 2  &&  !lightMap))  //(r_greyscale->integer)
			{
				if(r_texturebits->integer == 16)
					internalFormat = GL_LUMINANCE8_ALPHA8;
				else if(r_texturebits->integer == 32)
					internalFormat = GL_LUMINANCE16_ALPHA16;
				else
					internalFormat = GL_LUMINANCE_ALPHA;
			}
			else
			{
				if ( r_texturebits->integer == 16 )
				{
					internalFormat = GL_RGBA4;
				}
				else if ( r_texturebits->integer == 32 )
				{
					internalFormat = GL_RGBA8;
				}
				else
				{
					internalFormat = GL_RGBA;
				}
			}
		}
	}

	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && 
		( scaled_height == height ) ) {
		if (!mipmap)
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		Com_Memcpy (scaledBuffer, data, width*height*4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		Com_Memcpy( scaledBuffer, data, width * height * 4 );
	}

	R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap( (byte *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( (byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:

	if (mipmap)
	{
		if ( textureFilterAnisotropic )
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
					(GLint)Com_Clamp( 1, maxAnisotropy, r_ext_max_anisotropy->integer ) );

		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		if ( textureFilterAnisotropic )
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );

		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		ri.Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		ri.Hunk_FreeTempMemory( resampledBuffer );
}


extern qboolean LoadingWorld;

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImage( const char *name, byte *pic, int width, int height,
						imgType_t type, imgFlags_t flags, int internalFormat ) {
	image_t		*image;
	qboolean	isLightmap = qfalse;
	long		hash;
	int			glWrapClampMode;

	//mipmap = qfalse;  //test
	//ri.Printf(PRINT_ALL, "R_CreateImage (mipmap:%d, allowpicmip:%d): %s\n", mipmap, allowPicmip, name);

	if (strlen(name) >= MAX_QPATH ) {
		ri.Error (ERR_DROP, "R_CreateImage: \"%s\" is too long", name);
	}
	if ( !strncmp( name, "*lightmap", 9 ) ) {
		isLightmap = qtrue;
	}

	if ( tr.numImages >= MAX_DRAWIMAGES ) {
		ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES (%d) hit", MAX_DRAWIMAGES);
	}

	image = tr.images[tr.numImages] = ri.Hunk_Alloc( sizeof( image_t ), h_low );
	// can't hardcode anymore since additional textures are being used
	// with glGenTextures()
	//image->texnum = 1024 + tr.numImages;

	qglGenTextures(1, &image->texnum);
	tr.numImages++;

	image->type = type;
	image->flags = flags;

	strcpy (image->imgName, name);

	image->width = width;
	image->height = height;
	if (flags & IMGFLAG_CLAMPTOEDGE)
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	else
		glWrapClampMode = GL_REPEAT;

	// lightmaps are always allocated on TMU 1
	if ( qglActiveTextureARB && isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	if ( qglActiveTextureARB ) {
		GL_SelectTextureUnit( image->TMU );
	}

	GL_Bind(image);

	if (*r_lightmapColor->string  &&  isLightmap) {
		int x, y;
		byte *r, *g, *b;
		//byte *a;
		byte *xpic;
		int width;
		int height;
		int tr, tg, tb;
		int v;

		v = r_lightmapColor->integer;
		tr = (v & 0xff0000) / 0x010000;
		tg = (v & 0x00ff00) / 0x000100;
		tb = (v & 0x0000ff) / 0x000001;

		xpic = (byte *)pic;
		width = image->width;
		height = image->height;

		for (y = 0;  y < height;  y++) {
			for (x = 0;  x < width;  x++) {
				int avg;

				r = &xpic[(width * 4) * y   + x * 4   + 0];
				g = &xpic[(width * 4) * y   + x * 4   + 1];
				b = &xpic[(width * 4) * y   + x * 4   + 2];
				//a = &xpic[(width * 4) * y   + x * 4   + 3];
				//*g = 0;
				//*b = 0;
				avg = ((*r + *g + *b) / 3) * r_greyscaleValue->value;
				*r = (byte)((float)avg * (float)tr / 255.0);
				*g = (byte)((float)avg * (float)tg / 255.0);
				*b = (byte)((float)avg * (float)tb / 255.0);
			}
		}

	}

	//FIXME testing grayscale
	if ((flags & IMGFLAG_PICMIP)  &&  r_picmipGreyScale->integer) { //(1) {  //(allowPicmip) {
		int x, y;
		byte *r, *g, *b;
		//byte *a;
		byte *xpic;
		int width;
		int height;

		xpic = (byte *)pic;
		width = image->width;
		height = image->height;

		for (y = 0;  y < height;  y++) {
			for (x = 0;  x < width;  x++) {
				int avg;

				r = &xpic[(width * 4) * y   + x * 4   + 0];
				g = &xpic[(width * 4) * y   + x * 4   + 1];
				b = &xpic[(width * 4) * y   + x * 4   + 2];
				//a = &xpic[(width * 4) * y   + x * 4   + 3];
				//*g = 0;
				//*b = 0;
				avg = ((*r + *g + *b) / 3) * r_picmipGreyScaleValue->value;
				*r = (byte)avg;
				*g = (byte)avg;
				*b = (byte)avg;
			}
		}
	}

	if (r_greyscale->integer == 1  ||  (r_greyscale->integer == 2  &&  !isLightmap))  {  //(r_greyscale->integer) { //(1) {  //(allowPicmip) {
		int x, y;
		byte *r, *g, *b;
		//byte *a;
		byte *xpic;
		int width;
		int height;

		xpic = (byte *)pic;
		width = image->width;
		height = image->height;

		for (y = 0;  y < height;  y++) {
			for (x = 0;  x < width;  x++) {
				int avg;

				r = &xpic[(width * 4) * y   + x * 4   + 0];
				g = &xpic[(width * 4) * y   + x * 4   + 1];
				b = &xpic[(width * 4) * y   + x * 4   + 2];
				//a = &xpic[(width * 4) * y   + x * 4   + 3];
				//*g = 0;
				//*b = 0;
				avg = ((*r + *g + *b) / 3) * r_greyscaleValue->value;
				*r = (byte)avg;
				*g = (byte)avg;
				*b = (byte)avg;
			}
		}
	}

#if 0
	// testing
	if (LoadingWorld) {  //  &&  !isLightmap) {
		R_GammaCorrectLightMap((byte *)pic, image->width * image->height * 4);
	}
#endif

	R_Upload32( (unsigned *)pic, image->width, image->height, 
				image->flags & IMGFLAG_MIPMAP,
				image->flags & IMGFLAG_PICMIP,
								isLightmap,
				!(image->flags & IMGFLAG_NO_COMPRESSION),
								&image->internalFormat,
								&image->uploadWidth,
								&image->uploadHeight );

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	glState.currenttextures[glState.currenttmu] = 0;
	qglBindTexture( GL_TEXTURE_2D, 0 );

	if ( image->TMU == 1 ) {
		GL_SelectTextureUnit( 0 );
	}

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}

//===================================================================

typedef struct
{
	char *ext;
	void (*ImageLoader)( const char *, unsigned char **, int *, int * );
} imageExtToLoaderMap_t;


// qll testing
#if 0
/* Fake image loader to redirect some textures */
/* Need improvements to prevent circular references */
void R_LoadImage( const char *name, byte **pic, int *width, int *height );
void R_LoadLink ( const char *name, byte **pic, int *width, int *height)
{
	char *buffer, *newname;
	int len;

	*pic = NULL;
	*width = 0;
	*height = 0;
	len = ri.FS_ReadFile(name, &buffer);
	if (len < 0) {
		ri.Printf(PRINT_ALL, "failed load link: %s\n", name);
		return;
	}
	newname = (char *)Z_Malloc(len + 1);
	Com_Memcpy(newname, buffer, len);
	newname[len] = 0;
	ri.Printf(PRINT_ALL, "Redirect: %s => %s\n", name, newname);
	R_LoadImage(newname, pic, width, height);
	Z_Free(newname);
}
#endif

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[ ] =
{
	//{ "link", R_LoadLink },
	{ "jpg",  R_LoadJPG },
	{ "jpeg", R_LoadJPG },
	{ "png",  R_LoadPNG },
	{ "tga",  R_LoadTGA },
	{ "pcx",  R_LoadPCX },
	{ "bmp",  R_LoadBMP }
};

static int numImageLoaders = ARRAY_LEN( imageLoaders );

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *name, byte **pic, int *width, int *height )
{
	qboolean orgNameFailed = qfalse;
	int orgLoader = -1;
	int i;
	char localName[ MAX_QPATH ];
	const char *ext;
	char *altName;

	*pic = NULL;
	*width = 0;
	*height = 0;

	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
			{
				// Load
				imageLoaders[ i ].ImageLoader( localName, pic, width, height );
				break;
			}
		}

		// A loader was found
		if( i < numImageLoaders )
		{
			if( *pic == NULL )
			{
				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				orgNameFailed = qtrue;
				orgLoader = i;
				COM_StripExtension( name, localName, MAX_QPATH );
			}
			else
			{
				// Something loaded
				return;
			}
		}
	}

	// Try and find a suitable match using all
	// the image formats supported
	for( i = 0; i < numImageLoaders; i++ )
	{
		if (i == orgLoader)
			continue;

		altName = va( "%s.%s", localName, imageLoaders[ i ].ext );

		// Load
		imageLoaders[ i ].ImageLoader( altName, pic, width, height );

		if( *pic )
		{
			if( orgNameFailed )
			{
				ri.Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",
						name, altName );
			}

			break;
		}
	}
}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags )
{
	image_t	*image;
	int		width, height;
	byte	*pic;
	long	hash;

	if (!name) {
		return NULL;
	}

	hash = generateHashValue(name);

	//
	// see if the image is already loaded
	//
	for (image=hashTable[hash]; image; image=image->next) {
		if ( !strcmp( name, image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( strcmp( name, "*white" ) ) {
				if ( image->flags != flags ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags );
				}
			}
			return image;
		}
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height );
	if ( pic == NULL ) {
		return NULL;
	}

	//ri.Printf(PRINT_ALL, "R_FindImageFile  %s\n", name);

#if 0
	//FIXME testing grayscale
	if (1) {  //(allowPicmip) {
		int x, y;
		byte *r, *g, *b, *a;

		for (y = 0;  y < height;  y++) {
			for (x = 0;  x < width;  x++) {
				int avg;

				r = &pic[(width * 4) * y   + x * 4   + 0];
				g = &pic[(width * 4) * y   + x * 4   + 1];
				b = &pic[(width * 4) * y   + x * 4   + 2];
				a = &pic[(width * 4) * y   + x * 4   + 3];
				//*g = 0;
				//*b = 0;
				avg = (*r + *g + *b) / 3;
				*r = (byte)avg;
				*g = (byte)avg;
				*b = (byte)avg;
			}
		}
	}
#endif

	image = R_CreateImage( ( char * ) name, pic, width, height, type, flags, 0 );

	ri.Free( pic );
	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) {
	int		x,y;
	byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int		b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x=0 ; x<DLIGHT_SIZE ; x++) {
		for (y=0 ; y<DLIGHT_SIZE ; y++) {
			float	d;

			d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
				( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
			b = 4000 / d;
			if (b > 255) {
				b = 255;
			} else if ( b < 75 ) {
				b = 0;
			}
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = b;
			data[y][x][3] = 255;			
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	d;

	data = ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
	ri.Hunk_FreeTempMemory( data );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, 0);
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int		x,y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++) {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *)data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0);


	for(x=0;x<32;x++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE, 0);
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		i, j;
	float	g;
	int		inf;
	int		shift;
	float mul;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	mul = r_overBrightBitsValue->value;
	if (mul < 0.0) {
		mul = 0.0;
	}

	if ( !glConfig.deviceSupportsGamma ) {
		// why disable?  still needed for screenshots and video
		//tr.overbrightBits = 0;		// need hardware gamma for overbright
		//ri.Printf(PRINT_ALL, "^1no hw gamma no overbrightbits\n");
	}

	//tr.overbrightBits = 0;

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen )
	{
		//tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( (1 << tr.overbrightBits) * mul );
	tr.identityLightByte = 255 * tr.identityLight;


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 5.0f ) {
		ri.Cvar_Set( "r_gamma", "5.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		inf *= mul;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma )
	{
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

void R_SetLightMapColorMappings (void)
{
	int		i, j;
	float	g;
	int		inf;
	int		shift;
	float mul;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	mul = r_overBrightBitsValue->value;
	if (mul < 0.0) {
		mul = 0.0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	//tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	//tr.identityLightByte = 255 * tr.identityLight;
	//tr.overbrightBits = 1;


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 5.0f ) {
		ri.Cvar_Set( "r_gamma", "5.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		inf *= mul;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_lightmapgammatable[i] = inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_lightmapintensitytable[i] = j;
	}
}


static char *Skin_Images[][3] = {
	{ "cache/models/players/mynx/%s_s%s", NULL, NULL },
	{ "cache/models/players/klesk/%s%s", NULL, NULL },
	{ "cache/models/players/klesk/%s_h%s", NULL, NULL },
	{ "cache/models/players/bitterman/h_%s%s", NULL, NULL },
	{ "cache/models/players/bitterman/%s%s", NULL, NULL },
	{ "cache/models/players/grunt/%s%s", NULL, NULL },
	{ "cache/models/players/grunt/%s_h%s", NULL, NULL },
	{ "cache/models/players/uriel/%s_w%s", NULL, NULL },
	{ "cache/models/players/uriel/%s%s", NULL, NULL },
	{ "cache/models/players/lucy/%s_h2%s", NULL, NULL },
	{ "cache/models/players/lucy/%s%s", NULL, NULL },
	{ "cache/models/players/lucy/%s_h%s", NULL, NULL },
	{ "cache/models/players/xaero/%s%s", NULL, NULL },
	{ "cache/models/players/xaero/%s_h%s", NULL, NULL },
	{ "cache/models/players/tankjr/%s%s", NULL, NULL },
	{ "cache/models/players/biker/%s%s", NULL, NULL },
	{ "cache/models/players/biker/%s_h%s", NULL, NULL },
	{ "cache/models/players/doom/%s%s", NULL, NULL },
	{ "cache/models/players/ranger/%s%s", NULL, NULL },
	{ "cache/models/players/razor/h_%s%s", NULL, NULL },
	{ "cache/models/players/razor/%s%s", NULL, NULL },
	{ "cache/models/players/slash/%s%s", NULL, NULL },
	{ "cache/models/players/slash/%s_h%s", NULL, NULL },
	{ "cache/models/players/visor/%s%s", NULL, NULL },
	{ "cache/models/players/major/%s_h2%s", NULL, NULL },
	{ "cache/models/players/major/%s%s", NULL, NULL },
	{ "cache/models/players/major/%s_h%s", NULL, NULL },
	{ "cache/models/players/sorlag/%s_t%s", NULL, NULL },
	{ "cache/models/players/sorlag/%s%s", NULL, NULL },
	//	{ "cache/models/players/orbb/orbb_light_%s%s", NULL, NULL },
	//{ "cache/models/players/orbb/%s%s", NULL, NULL },
	//{ "cache/models/players/orbb/orbb_tail_%s%s", NULL, NULL },
	//{ "cache/models/players/orbb/%s_h%s", NULL, NULL },
	{ "cache/models/players/james/%sshad2%s", NULL, NULL },
	{ "cache/models/players/james/%sshad%s", NULL, NULL },
	{ "cache/models/players/james/%s%s", NULL, NULL },
	{ "cache/models/players/james/james_%s%s", "models/players/james/james_red.png", "models/players/james/james_blu.png" },
	{ "cache/models/players/james/james_e_%s%s", "models/players/james/james_e_red.png", "models/players/james/james_e_blu.png" },

	{ "cache/models/players/hunter/%s%s", NULL, NULL },
	{ "cache/models/players/hunter/%s_h%s", NULL, NULL },
	{ "cache/models/players/bones/%s%s", NULL, NULL },
	{ "cache/models/players/keel/%s%s", NULL, NULL },
	{ "cache/models/players/keel/%s_h%s", NULL, NULL },
	{ "cache/models/players/anarki/%s_g%s", NULL, NULL },
	{ "cache/models/players/anarki/%s%s", NULL, NULL },
	{ "cache/models/players/crash/%s_t%s", NULL, NULL },
	{ "cache/models/players/crash/%s%s", NULL, NULL },
	{ "cache/models/players/sarge/%s2%s", NULL, NULL },
	{ "cache/models/players/sarge/%s%s", NULL, NULL },

	{ "cache/models/players/janet/%s%s", NULL, NULL },
	{ "cache/models/players/janet/callisto2_%s%s", "models/players/janet/callisto2_r.jpg", "models/players/janet/callisto2_b.jpg" },
	{ "cache/models/players/janet/callisto_c%s%s", "models/players/janet/callisto_r.jpg", "models/players/janet/callisto_b.jpg" },
	{ "cache/models/players/janet/callisto_%s%s", "models/players/janet/callisto_red.jpg", "models/players/janet/callisto_blu.jpg" },


	{ "cache/models/players/orbb/%s%s", NULL, NULL },
	{ "cache/models/players/orbb/%s_h%s", NULL, NULL },
	{ "cache/models/players/orbb/orbb_light_%s%s", "models/players/orbb/orbb_light.tga", "models/players/orbb/orbb_light_blue.tga" },
	{ "cache/models/players/orbb/orbb_tail_%s%s", "models/players/orbb/orbb_tail.tga", "models/players/orbb/orbb_tail_blue.tga" },

	{ "cache/models/players/santa/%s%s", "models/players/santa/santa.png", NULL },
	{ "cache/models/players/santa/%s_h%s", "models/players/santa/santa_h.png", NULL },

	{ NULL, NULL , NULL },
};

//FIXME alloc, 
static ubyte ibuf[256 * 256 * 4 + 18];
static cvar_t *r_colorSkinsFuzz;
static cvar_t *r_colorSkinsIntensity;

void R_CreatePlayerColorSkinImages (qboolean force)
{
	int i, j, k;
	int rw, rh;
	int bw, bh;
	int c;
	byte *rpic;
	byte *bpic;
	byte tgaHeader[18];
	float intensity;
	int fuzz;
	GLuint texture;
	//int width, height;

	r_colorSkinsFuzz = ri.Cvar_Get("r_colorSkinsFuzz", "20", CVAR_ARCHIVE);
	r_colorSkinsIntensity = ri.Cvar_Get("r_colorSkinsIntensity", "1.0", CVAR_ARCHIVE);
	fuzz = r_colorSkinsFuzz->integer;
	intensity = r_colorSkinsIntensity->value;

	for (i = 0;  Skin_Images[i][0];  i++) {
		if (!force  &&  ri.FS_FileExists(va(Skin_Images[i][0], "color", ".tga"))) {
			continue;
		}

		// get red
		if (Skin_Images[i][1]) {
			R_LoadImage(Skin_Images[i][1], &rpic, &rw, &rh);
		} else {
			R_LoadImage(va(Skin_Images[i][0] + strlen("cache/"), "red", ""), &rpic, &rw, &rh);
		}

		if (rw == 0  ||  rh == 0  ||  rpic == NULL) {
			ri.Printf(PRINT_ALL, "failed to open %s\n", va(Skin_Images[i][0] + strlen("cache/"), "red", ""));
			continue;
		}

		if (Skin_Images[i][2]) {
			R_LoadImage(Skin_Images[i][2], &bpic, &bw, &bh);
		} else {
			R_LoadImage(va(Skin_Images[i][0] + strlen("cache/"), "blue", ""), &bpic, &bw, &bh);
		}

		if (bw == 0  ||  bh == 0  ||  bpic == NULL) {
			ri.Printf(PRINT_ALL, "failed to open %s\n", va(Skin_Images[i][0] + strlen("cache/"), "blue", ""));
			ri.Free(rpic);
			continue;
		}

		if (rw != bw  ||  rh != bh) {
			ri.Printf(PRINT_ALL, "wtf  rw %d bw %d rh %d bh %d\n", rw, bw, rh, bh);
			goto finish;
		}

		// update screen
		GL_SelectTextureUnit(0);
		GL_State(GLS_DEPTHTEST_DISABLE);
        qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
        qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);

        qglMatrixMode(GL_PROJECTION);
        qglLoadIdentity();
		qglOrtho(-1, 1, -1, 1, -1, 1);

        qglMatrixMode(GL_MODELVIEW);
		qglLoadIdentity();
		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_BLEND);
        qglDisable(GL_STENCIL_TEST);

		qglClear(GL_COLOR_BUFFER_BIT);
		qglClear(GL_DEPTH_BUFFER_BIT);
		qglClearColor(0, 0, 0, 1);
		qglGenTextures(1, &texture);
        qglBindTexture(GL_TEXTURE_2D, texture);
        qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		if (r_texturebits->integer == 16) {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, rw, rh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rpic);
		} else {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rw, rh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rpic);
		}

        qglBindTexture(GL_TEXTURE_2D, texture);
		qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		//width = rw;
		//height = rh;

		qglBegin(GL_QUADS);

		glTexCoord2d(0.0, 0.0);  glVertex2d(0.0, 0.0);
		glTexCoord2d(1.0, 0.0);  glVertex2d(1.0, 0.0);
		glTexCoord2d(1.0, 1.0);  glVertex2d(1.0, 1.0);
		glTexCoord2d(0.0, 1.0);  glVertex2d(0.0, 1.0);

        qglEnd();

		qglFinish();
		GLimp_EndFrame();
		qglDeleteTextures(1, &texture);

		memset(tgaHeader, 0, 18);
		tgaHeader[2] = 2;
		tgaHeader[12] = rw & 255;
		tgaHeader[13] = rw >> 8;
		tgaHeader[14] = rh & 255;
		tgaHeader[15] = rh >> 8;
		tgaHeader[16] = 32;        // pixel size
		tgaHeader[17] = 0;  //0x20;  tga support kind of broken
		memcpy(ibuf, tgaHeader, 18);

		for (j = 0;  j < rw * rh * 4;  j += 4) {
			//int cm;
			//qboolean diff = qfalse;
			//byte alpha = 0;
			byte r0, r1, r2, b0, b1, b2;

			r0 = rpic[j + 0];
			r1 = rpic[j + 1];
			r2 = rpic[j + 2];
			b0 = bpic[j + 0];
			b1 = bpic[j + 1];
			b2 = bpic[j + 2];

			//cm = abs(r0 - b0) + abs(r1 - b1) + abs(r2 - b2);
			//if (cm) {  // > fuzz) {
			if (abs(r0 - b0) > fuzz  ||  abs(r1 - b1) > fuzz  ||  abs(r2 - b2) > fuzz) {
				//diff = qtrue;
				//alpha = 255;
			} else {
				bpic[j + 3] = 0;
				continue;
			}

			// blue matches with red
			c = (float)b2 * intensity;  // 1.5;
			if (c > 255) {
				c = 255;
			}

			bpic[j + 0] = c;
			bpic[j + 1] = c;
			bpic[j + 2] = c;
#if 0
			if (cm < 0) {
				alpha = 0;
			}
#endif
			//ibuf[j+18 + 3] = alpha;
			//bpic[j + 3] = alpha;
		}

		// swap rgb bgr
		for (j = 0;  j < bw * bh * 4;  j += 4) {
			byte tmp = bpic[j];
			bpic[j] = bpic[j + 2];
			bpic[j + 2] = tmp;
		}

		//FIXME fuck why??
		// switch from topdown
		j = 0;
		for (k = (bh - 1) * bw * 4;  k >= 0;  k -= (bw * 4), j += (bw * 4)) {
			memcpy(ibuf + j + 18, bpic + k, bw * 4);
		}

		ri.FS_WriteFile(va(Skin_Images[i][0], "color", ".tga"), ibuf, 18 + bw * bh * 4);
		ri.Printf(PRINT_ALL, "created: ");
		ri.Printf(PRINT_ALL, "%s", va(Skin_Images[i][0], "color", ".tga"));
		ri.Printf(PRINT_ALL, "\n");

finish:
		ri.Free(rpic);
		ri.Free(bpic);
	}
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	Com_Memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	R_SetColorMappings();
	R_SetLightMapColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
	R_CreatePlayerColorSkinImages(qfalse);  //FIXME qtrue
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int		i;

	for ( i=0; i<tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	Com_Memset( tr.images, 0, sizeof( tr.images ) );

	tr.numImages = 0;

	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	qglDeleteTextures(1, &tr.backBufferTexture);
	qglDeleteTextures(1, &tr.bloomTexture);

	if ( qglActiveTextureARB ) {
		GL_SelectTextureUnit( 1 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
		GL_SelectTextureUnit( 0 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
	} else {
		qglBindTexture( GL_TEXTURE_2D, 0 );
	}
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	union {
		char *c;
		void *v;
	} text;
	char		*text_p;
	char		*token;
	char		surfName[MAX_QPATH];
	char iname[MAX_QPATH];
	char *p;

	if ( !name || !name[0] ) {
		ri.Printf(PRINT_ALL,  "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf(PRINT_ALL,  "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}
	Q_strncpyz(iname, name, sizeof(iname));
	p = iname;
	while (*p) {
		int c = *p;
		if (c >= 'A'  &&  c <= 'Z') {
			c += 'a' - 'A';
		}
		*p = (char)c;
		p++;
	}

	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, iname ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			//ri.Printf(PRINT_ALL, "%s already loaded\n", name);
			return hSkin;
		}
	}

	//ri.Printf(PRINT_ALL, "^4going to allocate new skin %s\n", name);
	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, iname, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	R_IssuePendingRenderCommands();

	// If not a .skin file, load as a single shader
	if ( strcmp( iname + strlen( iname ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->surfaces[0] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		skin->surfaces[0]->shader = R_FindShader( iname, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( iname, &text.v );
	if ( !text.c ) {
		return 0;
	}

	text_p = text.c;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		if ( skin->numSurfaces >= MD3_MAX_SURFACES ) {
			ri.Printf( PRINT_WARNING, "WARNING: Ignoring surfaces in '%s', the max is %d surfaces!\n", name, MD3_MAX_SURFACES );
			break;
		}

		surf = skin->surfaces[ skin->numSurfaces ] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text.v );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}


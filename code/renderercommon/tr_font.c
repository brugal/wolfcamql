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
// tr_font.c
// 
//
// The font system uses FreeType 2.x to render TrueType fonts for use within the game.
// As of this writing ( Nov, 2000 ) Team Arena uses these fonts for all of the ui and 
// about 90% of the cgame presentation. A few areas of the CGAME were left uses the old 
// fonts since the code is shared with standard Q3A.
//
// If you include this font rendering code in a commercial product you MUST include the
// following somewhere with your product, see www.freetype.org for specifics or changes.
// The Freetype code also uses some hinting techniques that MIGHT infringe on patents 
// held by apple so be aware of that also.
//
// As of Q3A 1.25+ and Team Arena, we are shipping the game with the font rendering code
// disabled. This removes any potential patent issues and it keeps us from having to 
// distribute an actual TrueTrype font which is 1. expensive to do and 2. seems to require
// an act of god to accomplish. 
//
// What we did was pre-render the fonts using FreeType ( which is why we leave the FreeType
// credit in the credits ) and then saved off the glyph data and then hand touched up the 
// font bitmaps so they scale a bit better in GL.
//
// There are limitations in the way fonts are saved and reloaded in that it is based on 
// point size and not name. So if you pre-render Helvetica in 18 point and Impact in 18 point
// you will end up with a single 18 point data file and image set. Typically you will want to 
// choose 3 sizes to best approximate the scaling you will be doing in the ui scripting system
// 
// In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team Arena, we
// use three or four scales, most of them exactly equaling the specific rendered size. We 
// rendered three sizes in Team Arena, 12, 16, and 20. 
//
// To generate new font data you need to go through the following steps.
// 1. delete the fontImage_x_xx.tga files and fontImage_xx.dat files from the fonts path.
// 2. in a ui script, specificy a font, smallFont, and bigFont keyword with font name and 
//    point size. the original TrueType fonts must exist in fonts at this point.
// 3. run the game, you should see things normally.
// 4. Exit the game and there will be three dat files and at least three tga files. The 
//    tga's are in FSIZExFSIZE pages so if it takes three images to render a 24 point font you 
//    will end up with fontImage_0_24.tga through fontImage_2_24.tga
// 5. In future runs of the game, the system looks for these images and data files when a
//    specific point sized font is rendered and loads them for use. 
// 6. Because of the original beta nature of the FreeType code you will probably want to hand
//    touch the font bitmaps.
// 
// Currently a define in the project turns on or off the FreeType code which is currently 
// defined out. To pre-render new fonts you need enable the define ( BUILD_FREETYPE ) and 
// uncheck the exclude from build check box in the FreeType2 area of the Renderer project. 

//#include "tr_local.h"
#include "tr_common.h"
#include "../qcommon/qcommon.h"

#ifdef BUILD_FREETYPE

//#ifdef USE_LOCAL_HEADERS
//  #include "../freetype-2.12.1/include/ft2build.h"
//#else
//  #include <ft2build.h>
//#endif

#include <ft2build.h>

#include FT_FREETYPE_H
//#include FT_ERRORS_H
//#include FT_SYSTEM_H
//#include FT_IMAGE_H
#include FT_OUTLINE_H

#define _FLOOR(x)  ((x) & -64)
#define _CEIL(x)   (((x)+63) & -64)
#define _TRUNC(x)  ((x) >> 6)


static FT_Library ftLibrary = NULL;

typedef struct fallbackFonts_s {
	FT_Face face;
	void *faceData;
	char name[MAX_QPATH];

	struct fallbackFonts_s *next;
} fallbackFonts_t;

static fallbackFonts_t *fallbackFonts = NULL;

#endif

#define FSIZE 256

static int registeredFontCount = 0;
static fontInfo_t registeredFont[MAX_FONTS];

#ifdef BUILD_FREETYPE

// if successfull caller must free() buffer
static int ReadFileData (const char *filename, void **buffer)
{
	int len;
	void *data;
	void *newData;

	len = ri.FS_ReadFile(filename, &data);
	if (len <= 0) {
		return len;
	}

	if (buffer != NULL) {
		newData = malloc(len);
		if (!newData) {
			ri.Printf(PRINT_ALL, "^1ReadFile couldn't allocate memory for '%s'\n", filename);
			return -1;
		}

		Com_Memcpy(newData, data, len);
		*buffer = newData;
		ri.FS_FreeFile(data);
	}

	return len;
}

static void R_GetGlyphInfo(FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch) {

  *left  = _FLOOR( glyph->metrics.horiBearingX );
  *right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
  *width = _TRUNC(*right - *left);

  *top    = _CEIL( glyph->metrics.horiBearingY );
  *bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
  *height = _TRUNC( *top - *bottom );
  *pitch  = ( qtrue ? (*width+3) & -4 : (*width+7) >> 3 );
}


static FT_Bitmap *R_RenderGlyph(FT_GlyphSlot glyph, glyphInfo_t* glyphOut) {

  FT_Bitmap  *bit2;
  int left, right, width, top, bottom, height, pitch, size;

  R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

  if ( glyph->format == ft_glyph_format_outline ) {
    size   = pitch*height;

    bit2 = ri.Malloc(sizeof(FT_Bitmap));
	if (bit2 == NULL) {
		ri.Printf(PRINT_ALL, "%s: (outline) couldn't allocate memory for bitmap\n", __FUNCTION__);
		return NULL;
	}

    bit2->width      = width;
    bit2->rows       = height;
    bit2->pitch      = pitch;
    bit2->pixel_mode = ft_pixel_mode_grays;
    //bit2->pixel_mode = ft_pixel_mode_mono;

	//ri.Printf(PRINT_ALL, "width %d  height %d  pitch %d\n", width, height, pitch);
    bit2->buffer     = ri.Malloc(pitch*height);

	if (bit2->buffer == NULL) {
		ri.Printf(PRINT_ALL, "^1%s:  couldn't allocate memory for bitmap:o buffer (%d bytes)\n", __FUNCTION__, pitch * height);
		ri.Free(bit2);
		return NULL;
	}

    bit2->num_grays = 256;

    Com_Memset( bit2->buffer, 0, size );

    FT_Outline_Translate( &glyph->outline, -left, -bottom );

    FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

    glyphOut->height = height;
    glyphOut->pitch = pitch;
    glyphOut->top = glyph->metrics.horiBearingY >> 6;
    glyphOut->bottom = bottom;
	glyphOut->left = glyph->metrics.horiBearingX >> 6;

    return bit2;
  } else {
    //ri.Printf(PRINT_WARNING, "R_RenderGlyph:  non-outline fonts are not supported\n");

	//FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
	FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

    bit2 = ri.Malloc(sizeof(FT_Bitmap));
	if (bit2 == NULL) {
		ri.Printf(PRINT_ALL, "%s: (bitmap) couldn't allocate memory for bitmap\n", __FUNCTION__);
		return NULL;
	}

	*bit2 = glyph->bitmap;
	bit2->buffer = ri.Malloc(glyph->bitmap.pitch * glyph->bitmap.rows);
	if (bit2->buffer == NULL) {
		ri.Printf(PRINT_ALL, "^1%s:  couldn't allocate memory for bitmap:b buffer (%d bytes)\n", __FUNCTION__, pitch * height);
		ri.Free(bit2);
		return NULL;
	}

	memcpy(bit2->buffer, glyph->bitmap.buffer, glyph->bitmap.pitch * glyph->bitmap.rows);

	// testing
#if 0
	{
		int i;
		int j;

		ri.Printf(PRINT_ALL, "^2  font glyph  %d x %d   %d x %d::::\n", glyph->bitmap.pitch, glyph->bitmap.rows, height, width);

		for (i = 0;  i < glyph->bitmap.rows;  i++) {
			for (j = 0;  j < glyph->bitmap.pitch;  j++) {
				char c;
				c = bit2->buffer[i * glyph->bitmap.pitch + j];
				//bit2->buffer[i] = 255;
				ri.Printf(PRINT_ALL, "%c", c > 0 ? '*' : ' ');
			}
			ri.Printf(PRINT_ALL, "\n");
		}
	}
#endif

	glyphOut->height = glyph->bitmap.rows;
	glyphOut->pitch =  glyph->bitmap.pitch;
	glyphOut->top = glyph->metrics.horiBearingY >> 6;
	glyphOut->bottom = bottom;
	glyphOut->left = glyph->metrics.horiBearingX >> 6;

	return bit2;
  }

  return NULL;
}

static void WriteTGA (char *filename, byte *data, int width, int height) {
	byte	*buffer;
	int		i, c;
	int             row;
	unsigned char  *flip;
	unsigned char  *src, *dst;

	buffer = ri.Malloc(width*height*4 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width&(FSIZE - 1);
	buffer[13] = width>>8;
	buffer[14] = height&(FSIZE - 1);
	buffer[15] = height>>8;
	buffer[16] = 32;	// pixel size

	// swap rgb to bgr
	c = 18 + width * height * 4;
	for (i=18 ; i<c ; i+=4)
	{
		buffer[i] = data[i-18+2];		// blue
		buffer[i+1] = data[i-18+1];		// green
		buffer[i+2] = data[i-18+0];		// red
		buffer[i+3] = data[i-18+3];		// alpha
	}

	// flip upside down
	flip = (unsigned char *)ri.Malloc(width*4);
	for(row = 0; row < height/2; row++)
	{
		src = buffer + 18 + row * 4 * width;
		dst = buffer + 18 + (height - row - 1) * 4 * width;

		Com_Memcpy(flip, src, width*4);
		Com_Memcpy(src, dst, width*4);
		Com_Memcpy(dst, flip, width*4);
	}
	ri.Free(flip);

	ri.FS_WriteFile(filename, buffer, c);

	//f = fopen (filename, "wb");
	//fwrite (buffer, 1, c, f);
	//fclose (f);

	ri.Free (buffer);
}

static glyphInfo_t *RE_ConstructGlyphInfo(unsigned char *imageOut, int imageOutSize, int *xOut, int *yOut, int *maxHeight, FT_Face face, const unsigned long c, qboolean calcHeight) {
  int i;
  static glyphInfo_t glyph;
  unsigned char *src, *dst;
  float scaled_width, scaled_height;
  FT_Bitmap *bitmap = NULL;
  FT_Error error;

  Com_Memset(&glyph, 0, sizeof(glyphInfo_t));
  // make sure everything is here
  if (face != NULL) {
	  error = FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT);
	//FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO);
	  //FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING);
	  //FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT);

	  if (error != 0) {
		  ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo:  couldn't load glyph %ld\n", c);
		  return &glyph;
	  }
    bitmap = R_RenderGlyph(face->glyph, &glyph);

	glyph.xSkip = face->glyph->metrics.horiAdvance >> 6;

    if (!bitmap) {
		ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo: no bitmap for %lu\n", c);
      return &glyph;
    }

    if (glyph.height > *maxHeight) {
		//ri.Printf(PRINT_ALL, "maxheight changed from %d  to  %d\n", *maxHeight, glyph.height);
      *maxHeight = glyph.height;
    }

    if (calcHeight) {
      ri.Free(bitmap->buffer);
      ri.Free(bitmap);
      return &glyph;
    }

/*
    // need to convert to power of 2 sizes so we do not get
    // any scaling from the gl upload
  	for (scaled_width = 1 ; scaled_width < glyph.pitch ; scaled_width<<=1)
	  	;
  	for (scaled_height = 1 ; scaled_height < glyph.height ; scaled_height<<=1)
	  	;
*/

    scaled_width = glyph.pitch;
    scaled_height = glyph.height;

    // we need to make sure we fit
    if (*xOut + scaled_width + 1 >= (FSIZE - 1)) {
        *xOut = 0;
        *yOut += *maxHeight + 1;
		//ri.Printf(PRINT_ALL, "new row\n");
      }
    //FIXME hack since something is calculating wrong heights
    //} else if (*yOut + *maxHeight + 1 >= (FSIZE - 1)) {

	if (*yOut + *maxHeight + 1 >= (FSIZE - 1)) {
		//ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo:  wrong height %d for char %d\n", *yOut, (unsigned int)c);

		*yOut = -1;
		*xOut = -1;
		ri.Free(bitmap->buffer);
		ri.Free(bitmap);
		return &glyph;
    }

    src = bitmap->buffer;
    dst = imageOut + (*yOut * FSIZE) + *xOut;
	if (dst - imageOut >= imageOutSize) {
		ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo: initial overflow imageOut %lu >= %d\n", (long unsigned)(dst - imageOut), imageOutSize);
		*yOut = -1;
		*xOut = -1;

		ri.Free(bitmap->buffer);
		ri.Free(bitmap);
		return &glyph;
	}

	if (bitmap->pixel_mode == ft_pixel_mode_mono) {
		for (i = 0; i < glyph.height; i++) {
			int j;
			unsigned char *_src = src;
			unsigned char *_dst = dst;
			unsigned char mask = 0x80;
			unsigned char val = *_src;

			//ri.Printf(PRINT_ALL, "^2MONO glyph ...\n");
			for (j = 0; j < glyph.pitch; j++) {
				if (mask == 0x80) {
					val = *_src++;
				}
				if (val & mask) {
					*_dst = 0xff;
				}
				mask >>= 1;

				if ( mask == 0 ) {
					mask = 0x80;
				}
				_dst++;
			}

			src += glyph.pitch;
			dst += FSIZE;
			if (dst - imageOut >= imageOutSize) {
				ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo: pixel mode mono overflow imageOut %lu >= %d\n", (long unsigned)(dst - imageOut), imageOutSize);
				*yOut = -1;
				*xOut = -1;

				ri.Free(bitmap->buffer);
				ri.Free(bitmap);
				return &glyph;
			}
		}
	} else {  // pixel mode != mono
	    for (i = 0; i < glyph.height; i++) {
		    Com_Memcpy(dst, src, glyph.pitch);
			src += glyph.pitch;
			dst += FSIZE;
			if (dst - imageOut >= imageOutSize) {
				ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo: pixel mode != mono overflow imageOut %lu >= %d\n", (long unsigned)(dst - imageOut), imageOutSize);
				*yOut = -1;
				*xOut = -1;

				ri.Free(bitmap->buffer);
				ri.Free(bitmap);
				return &glyph;
			}
	    }
	}

    // we now have an 8 bit per pixel grey scale bitmap
    // that is width wide and pf->ftSize->metrics.y_ppem tall

    glyph.imageHeight = scaled_height;
    glyph.imageWidth = scaled_width;
    glyph.s = (float)*xOut / FSIZE;
    glyph.t = (float)*yOut / FSIZE;
    glyph.s2 = glyph.s + (float)scaled_width / FSIZE;
    glyph.t2 = glyph.t + (float)scaled_height / FSIZE;

    *xOut += scaled_width + 1;
  } else {  // face == NULL
	  ri.Printf(PRINT_ALL, "^1RE_ConstructGlyphInfo:  face == NULL\n");
	  return &glyph;
  }

//ri.Printf(PRINT_ALL, "new width and height:  %d  %d\n", glyph.imageWidth, glyph.imageHeight);

  ri.Free(bitmap->buffer);
  ri.Free(bitmap);

  return &glyph;
}
#endif

static int fdOffset;
static byte	*fdFile;

static int readInt( void ) {
	int i = ((unsigned int)fdFile[fdOffset] | ((unsigned int)fdFile[fdOffset+1]<<8) | ((unsigned int)fdFile[fdOffset+2]<<16) | ((unsigned int)fdFile[fdOffset+3]<<24));
	fdOffset += 4;

	//ri.Printf(PRINT_ALL, "int %d\n", i);
	return i;
}

typedef union {
	byte	fred[4];
	float	ffred;
} poor;

static float readFloat( void ) {
	poor	me;
#if defined Q3_BIG_ENDIAN
	me.fred[0] = fdFile[fdOffset+3];
	me.fred[1] = fdFile[fdOffset+2];
	me.fred[2] = fdFile[fdOffset+1];
	me.fred[3] = fdFile[fdOffset+0];
#elif defined Q3_LITTLE_ENDIAN
	me.fred[0] = fdFile[fdOffset+0];
	me.fred[1] = fdFile[fdOffset+1];
	me.fred[2] = fdFile[fdOffset+2];
	me.fred[3] = fdFile[fdOffset+3];
#else
  #error "shouldn't happen"
    me.fred[0] = me.fred[1] = me.fred[2] = me.fred[3] = 0;
#endif
	fdOffset += 4;

	//ri.Printf(PRINT_ALL, "float  %f\n", me.ffred);

	return me.ffred;
}

static void LoadQ3Font (const char *fontName, int pointSize, fontInfo_t *font)
{
	int i;

	//R_IssuePendingRenderCommands();

	// see if it was loaded already
	for (i = 0; i < registeredFontCount; i++) {
		if (Q_stricmp(fontName, registeredFont[i].name) == 0) {
			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			return;
		}
	}

	for (i = 0;  i < GLYPHS_PER_FONT;  i++) {
		font->baseGlyphs[i].height = 16;
		font->baseGlyphs[i].top = 16;  //FIXME
		font->baseGlyphs[i].bottom          = 0;  //FIXME
		font->baseGlyphs[i].pitch           = 16;
		font->baseGlyphs[i].xSkip           = 16;
		font->baseGlyphs[i].imageWidth      = 16;
		font->baseGlyphs[i].imageHeight     = 16;
		font->baseGlyphs[i].left = 0;
		font->baseGlyphs[i].s                       = (float)(i % 16) / 16.0;
		font->baseGlyphs[i].t                       = (float)(i / 16) / 16.0;
		font->baseGlyphs[i].s2                      = (float)((i % 16) + 1) / 16.0;
		font->baseGlyphs[i].t2                      = (float)(i / 16 + 1) / 16.0;

		Q_strncpyz(font->baseGlyphs[i].shaderName, "gfx/2d/bigchars", sizeof(font->baseGlyphs[i].shaderName));
		font->baseGlyphs[i].glyph           = RE_RegisterShaderNoMip(font->baseGlyphs[i].shaderName);
		if (!font->baseGlyphs[i].glyph) {
			// wolfcam not setup correctly, no access to ql paks
			font->baseGlyphs[i].glyph = RE_RegisterShaderNoMip("gfx/wc/openarenachars");
		}
	}

	Q_strncpyz(font->name, fontName, sizeof(font->name));
	Q_strncpyz(font->registerName, fontName, sizeof(font->registerName));
	font->glyphScale = 2.5;  //2.2;  //2.4;  //2.5;  //2.0;  //48.0 / 16.0;

	font->fontId = registeredFontCount;
	font->faceData = NULL;
	font->q3Font = qtrue;
	font->bitmapFont = qtrue;
	font->pointSize = 16;
	font->fontFace = NULL;
	font->extraGlyphs = NULL;
	// adding font
	Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));
	//ri.Printf(PRINT_ALL, "%s registered as %s\n", fontName, font->name);
}

qboolean RE_GetFontInfo (int fontId, fontInfo_t *font)
{
	int i;

	if (font == NULL) {
		ri.Printf(PRINT_WARNING, "%s: font == NULL\n", __FUNCTION__);
		return qfalse;
	}

	for (i = 0;  i < registeredFontCount;  i++) {
		if (registeredFont[i].fontId == fontId) {
			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			return qtrue;
		}
	}

	ri.Printf(PRINT_WARNING, "%s: couldn't find font for id %d\n", __FUNCTION__, fontId);

	return qfalse;
}

#define FONT_OUT_BUFFER_SIZE (1024 * 1024 + 1)

void RE_RegisterFont (const char *fontName, int pointSize, fontInfo_t *font)
{
#ifdef BUILD_FREETYPE
	FT_Face face;
	int j, k, xOut, yOut, lastStart, imageNumber;
	int scaledSize, newSize, maxHeight, left;
	unsigned char *out, *imageBuff;
	glyphInfo_t *glyph;
	image_t *image;
	qhandle_t h;
	float max;
	float dpi = 72;
	float glyphScale;
#endif
	void *faceData;
	qboolean gotFont = qfalse;
	int i, len;
	char name[1024];
	char baseName[1024];
	qboolean syncRenderThread = qfalse;
	int checksum;

	if (!font) {
		ri.Printf(PRINT_ALL, "^1RE_RegisterFont: font == NULL\n");
		return;
	}

	if (!fontName) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: called with empty name\n");
		font->name[0] = '\0';
		return;
	}

	//ri.Printf(PRINT_ALL, "register font %d  ps %d font name: '%s'\n", registeredFontCount, pointSize, fontName);
	
	// testing default font
	//fontName = "/home/acano/unifont/uni.ttf";

	//if (!Q_stricmp(fontName, "q3font")) {
	if (!Q_stricmp(fontName, "q3tiny")  ||  !Q_stricmp(fontName, "q3small")  ||  !Q_stricmp(fontName, "q3big")  ||  !Q_stricmp(fontName, "q3giant")) {
		LoadQ3Font(fontName, pointSize, font);
		return;
	}

	Q_strncpyz(name, fontName, sizeof(name));
	COM_StripExtension(COM_SkipPath(name), baseName, sizeof(baseName));
	//ri.Printf(PRINT_ALL, "%s\n", baseName);

	if (strlen(fontName) > 4) {
		if (!Q_stricmpn(fontName + strlen(fontName) - 4, ".ttf", 4)) {
			font->qlDefaultFont = qfalse;  //FIXME 2016-03-08 still true?
			goto try_ttf;
		}
	}

    //ri.Printf(PRINT_ALL, "RE_RegisterFont %s  %d\n", fontName, pointSize);

	if (pointSize <= 0) {
		ri.Printf(PRINT_WARNING, "%s: invalid point size %d for '%s'\n", __FUNCTION__, pointSize, fontName);
		pointSize = 12;
	}

#if 0
	if (pointSize > 512) {
		ri.Printf(PRINT_WARNING, "%s: invalid point size %d for '%s'\n", __FUNCTION__, pointSize, fontName);
		pointSize = 512;
	}
#endif

	//R_IssuePendingRenderCommands();
	syncRenderThread = qtrue;

	if (registeredFontCount >= MAX_FONTS) {
		ri.Printf(PRINT_WARNING, "RE_RegisterFont: Too many fonts registered already.\n");
		font->name[0] = '\0';
		return;
	}

	font->qlDefaultFont = qfalse;

	Com_sprintf(name, sizeof(name), "fonts/fontImage_%i.dat", pointSize);
	for (i = 0; i < registeredFontCount; i++) {
		if (Q_stricmp(name, registeredFont[i].name) == 0) {
			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			//ri.Printf(PRINT_ALL, "found font returning '%s'\n", name);
			return;
		}
	}

	//ri.Printf(PRINT_ALL, "trying file... '%s'\n", name);
	
	len = ReadFileData(name, NULL);
	//ri.Printf(PRINT_ALL, "len: %d\n", len);
	//FIXME stupid shit, glyph->left integer
	//if (len == sizeof(fontInfo_t) - GLYPHS_PER_FONT * sizeof(int)) {

	//FIXME what if a ttf file is this length?

	if (len == 20548) {
		ReadFileData(name, &faceData);
		fdOffset = 0;
		fdFile = faceData;
		for(i=0; i<GLYPHS_PER_FONT; i++) {
			//ri.Printf(PRINT_ALL, "%s  %d\n", name, i);

			font->baseGlyphs[i].height		= readInt();
			font->baseGlyphs[i].top			= readInt();
			font->baseGlyphs[i].bottom		= readInt();
			font->baseGlyphs[i].pitch		= readInt();
			font->baseGlyphs[i].xSkip		= readInt();
			font->baseGlyphs[i].imageWidth	= readInt();
			font->baseGlyphs[i].imageHeight = readInt();
			font->baseGlyphs[i].s			= readFloat();
			font->baseGlyphs[i].t			= readFloat();
			font->baseGlyphs[i].s2			= readFloat();
			font->baseGlyphs[i].t2			= readFloat();
			font->baseGlyphs[i].glyph		= readInt();
			font->baseGlyphs[i].left = 0;  // q3 issue: not written to font data file
			Q_strncpyz(font->baseGlyphs[i].shaderName, (const char *)&fdFile[fdOffset], sizeof(font->baseGlyphs[i].shaderName));
			fdOffset += sizeof(font->baseGlyphs[i].shaderName);
		}
		font->glyphScale = readFloat();
		Com_Memcpy(font->name, &fdFile[fdOffset], MAX_QPATH);

//		Com_Memcpy(font, faceData, sizeof(fontInfo_t));
		Q_strncpyz(font->name, name, sizeof(font->name));
		Q_strncpyz(font->registerName, fontName, sizeof(font->registerName));

		for (i = GLYPH_START; i < GLYPH_END; i++) {
			//ri.Printf(PRINT_ALL, "^1register glyph '%s'\n", font->baseGlyphs[i].shaderName);
			font->baseGlyphs[i].glyph = RE_RegisterShaderNoMip(font->baseGlyphs[i].shaderName);
		}
		font->fontId = registeredFontCount;
		font->faceData = NULL;
		font->fontFace = NULL;
		font->q3Font = qfalse;
		font->bitmapFont = qtrue;
		font->pointSize = pointSize;
		font->extraGlyphs = NULL;
		// adding font
		Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));
		//ri.Printf(PRINT_ALL, "%s %d registered as %s scale %f\n", fontName, pointSize, font->name, font->glyphScale);
		free(faceData);
		return;
	} else if (len < 0) {
		// couldn't open file, try ttf
	} else {
		ri.Printf(PRINT_WARNING, "WARNING RE_RegisterFont: wrong font dat size for '%s':  %d, expected %d\n", name, len, (int)sizeof(fontInfo_t));
		return;
	}

 try_ttf:

	font->qlDefaultFont = qfalse;
	//ri.Printf(PRINT_ALL, "trying ttf\n");

	// make sure the render thread is stopped
	if (!syncRenderThread) {
		//R_IssuePendingRenderCommands();
		syncRenderThread = qtrue;
	}

	if (pointSize <= 0) {
		ri.Printf(PRINT_WARNING, "%s: invalid point size %d for '%s'\n", __FUNCTION__, pointSize, fontName);
		//Crash();
		pointSize = 12;
	}

#if 0
	if (pointSize > 512) {
		ri.Printf(PRINT_WARNING, "%s: invalid point size %d for '%s'\n", __FUNCTION__, pointSize, fontName);
		pointSize = 512;
	}
#endif

	Q_strncpyz(name, va("fonts2/%s_%i.dat", baseName, pointSize), sizeof(name));
	for (i = 0; i < registeredFontCount; i++) {
		if (Q_stricmp(name, registeredFont[i].name) == 0) {
			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			//ri.Printf(PRINT_ALL, "found font returning: %s\n", name);
			return;
		}
	}


#ifndef BUILD_FREETYPE
    ri.Printf(PRINT_ALL, "RE_RegisterFont: FreeType code not available\n");
#else
	if (ftLibrary == NULL) {
		ri.Printf(PRINT_ALL, "^1RE_RegisterFont: FreeType not initialized.\n");
		font->name[0] = '\0';
		return;
	}

	// check if in q3 pack
	gotFont = qfalse;

	if (ri.FS_FileIsInPAK(fontName, &checksum) == 1) {
		len = ReadFileData(fontName, &faceData);
		if (len <= 0) {
			ri.Printf(PRINT_ALL, "^1RE_RegisterFont:  couldn't load '%s' from pk3\n", fontName);
			gotFont = qfalse;
		} else {
			// got it
			gotFont = qtrue;
			// allocate on the stack first in case we fail
			if (FT_New_Memory_Face( ftLibrary, faceData, len, 0, &face )) {
				ri.Printf(PRINT_ALL, "^1RE_RegisterFont: FreeType2, unable to allocate new face for '%s' from pak file\n", fontName);
				font->name[0] = '\0';
				free(faceData);  //FIXME missed this?
				return;
			}
		}
	} else {  // try file system
		faceData = NULL;
		len = -1;
		if (FT_New_Face(ftLibrary, ri.FS_FindSystemFile(fontName), 0, &face)) {
			//ri.Printf(PRINT_WARNING, "RE_RegisterFont:  unable to load '%s' from system\n", fontName);
			gotFont = qfalse;
		} else {
			gotFont = qtrue;
		}
	}

	//FIXME hack for new ql testing pk3s, generic names like 'smallchar' etc..
	if (!gotFont) {
		//FIXME cvar for this
		const char *defaultFontFile = DEFAULT_FONT;

		//ri.Printf(PRINT_ALL, "trying default font...\n");
		// try system file first
		faceData = NULL;
		len = -1;
		if (FT_New_Face(ftLibrary, ri.FS_FindSystemFile(defaultFontFile), 0, &face)) {
			// nope, try pk3 file
			//FIXME keeping these memory copies for every default font
			len = ReadFileData(defaultFontFile, &faceData);
			if (len <= 0) {
				ri.Printf(PRINT_ALL, "^1RE_RegisterFont:  couldn't open default font '%s' from pak file\n", defaultFontFile);
				font->name[0] = '\0';
				return;
			}
			if (FT_New_Memory_Face(ftLibrary, faceData, len, 0, &face)) {
				ri.Printf(PRINT_ALL, "^1RE_RegisterFont:  couldn't allocate new face for default font '%s' from file system\n", defaultFontFile);
				free(faceData);
				font->name[0] = '\0';
				return;
			}
		}

		// got it, as either file or memory

		font->qlDefaultFont = qtrue;
		//ri.Printf(PRINT_ALL, "default ql font\n");
	}

	// have a valid font

	// << 6:  freetype lib uses 1/64 of pixel as base size
	if (FT_Set_Char_Size(face, pointSize << 6, pointSize << 6, dpi, dpi)) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: FreeType2, Unable to set face char size for '%s'.\n", fontName);
		font->name[0] = '\0';
		if (faceData != NULL) {
			free(faceData);
		}
		return;
	}

	//*font = &registeredFonts[registeredFontCount++];

	// make a FSIZExFSIZE image buffer, once it is full, register it, clean it and keep going
	// until all glyphs are rendered

	out = ri.Malloc(FONT_OUT_BUFFER_SIZE);
	if (out == NULL) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: ri.Malloc failure during output image creation for '%s'.\n", fontName);
		font->name[0] = '\0';
		if (faceData != NULL) {
			free(faceData);
		}
		return;
	}
	Com_Memset(out, 0, FONT_OUT_BUFFER_SIZE);

	maxHeight = 0;

	//ri.Printf(PRINT_ALL, "%s ...\n", fontName);
	//FIXME what is the point of this, how is maxHeight used?
	for (i = GLYPH_START; i < GLYPH_END; i++) {
		//ri.Printf(PRINT_ALL, "glyph %d\n", i);
		glyph = RE_ConstructGlyphInfo(out, FONT_OUT_BUFFER_SIZE, &xOut, &yOut, &maxHeight, face, (unsigned char)i, qtrue);
		if (glyph == NULL) {
			ri.Printf(PRINT_ALL, "^1%s: couldn't check glyph height for %d\n", __FUNCTION__, i);
			break;
		}
	}

	xOut = 0;
	yOut = 0;
	i = GLYPH_START;
	lastStart = i;
	imageNumber = 0;

	while ( i <= GLYPH_END ) {

		glyph = RE_ConstructGlyphInfo(out, FONT_OUT_BUFFER_SIZE, &xOut, &yOut, &maxHeight, face, (unsigned char)i, qfalse);
		if (glyph == NULL) {
			ri.Printf(PRINT_ALL, "^1%s: couldn't construct glyph for %d\n", __FUNCTION__, i);
			break;
		}

		if (xOut == -1 || yOut == -1 || i == GLYPH_END)  {
			// ran out of room
			// we need to create an image from the bitmap, set all the handles in the glyphs to this point
			//

			//FIXME what if GLYPH_END doesn't fit?
			//ri.Printf(PRINT_ALL, "ran out of room %d\n", i);
			if (i >= 32  &&  i <= 127) {
				//ri.Printf(PRINT_ALL, "boundry letter: %c\n", (char)i);
			}
			if (xOut == -1) {
				//ri.Printf(PRINT_ALL, "xOut == -1\n");
			}
			if (yOut == -1) {
				//ri.Printf(PRINT_ALL, "yOut == -1\n");
			}
			if (i == GLYPH_END) {
				//ri.Printf(PRINT_ALL, "i == GLYPH_END\n");
				if (xOut == -1  ||  yOut == -1) {
					ri.Printf(PRINT_ALL, "FIXME last glyph being skipped for '%s'\n", fontName);
				}
			}

			scaledSize = FSIZE*FSIZE;
			newSize = scaledSize * 4;
			imageBuff = ri.Malloc(newSize);
			left = 0;
			max = 0;

			for ( k = 0; k < (scaledSize) ; k++ ) {
				if (max < out[k]) {
					max = out[k];
				}
			}

			if (max > 0) {
				max = (256 - 1)/max;
			}

			for ( k = 0; k < (scaledSize) ; k++ ) {
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;

				imageBuff[left++] = ((float)out[k] * max);
				//imageBuff[left++] = out[k];
				//imageBuff[left++] = 255;
				//imageBuff[left++] = out[k] * 2;
			}

			Com_sprintf (name, sizeof(name), "fonts2/%s_%i_%i.tga", baseName, imageNumber++, pointSize);
			if (r_saveFontData->integer) {
				WriteTGA(name, imageBuff, FSIZE, FSIZE);
				//FIXME FONT_DIMENSIONS defined
			}

			//Com_sprintf (name, sizeof(name), "fonts/fontImage_%i_%i", imageNumber++, pointSize);
			//image = R_CreateImage(name, imageBuff, FSIZE, FSIZE, qfalse, qfalse, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
			image = R_CreateImage(name, imageBuff, FSIZE, FSIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
			h = RE_RegisterShaderFromImage(name, LIGHTMAP_2D, image, qfalse);
			for (j = lastStart; j < i; j++) {
				font->baseGlyphs[j].glyph = h;
				Q_strncpyz(font->baseGlyphs[j].shaderName, name, sizeof(font->baseGlyphs[j].shaderName));
			}
			lastStart = i;
			Com_Memset(out, 0, FONT_OUT_BUFFER_SIZE - 1);
			xOut = 0;
			yOut = 0;
			ri.Free(imageBuff);
			//i++; // no
			if (i == GLYPH_END) {
				break;
			}
		} else {
			Com_Memcpy(&font->baseGlyphs[i], glyph, sizeof(glyphInfo_t));
			i++;
		}
	}

	// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
	glyphScale = 72.0f / dpi;

	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
	glyphScale *= 48.0f / pointSize;

	registeredFont[registeredFontCount].glyphScale = glyphScale;
	font->glyphScale = 48.0 / (float)pointSize;  //glyphScale * 1;

	// hack scaling to match quake live
	if (!Q_stricmp(baseName, "notosans-regular")) {
		font->glyphScale *= 0.8;
	} else if (!Q_stricmp(baseName, "droidsansmono")) {
		font->glyphScale *= 0.8;
	}

	Q_strncpyz(font->name, va("fonts2/%s_%i.dat", baseName, pointSize), sizeof(font->name));
	Q_strncpyz(font->registerName, fontName, sizeof(font->name));

	font->fontId = registeredFontCount;
	font->fontFace = malloc(sizeof(FT_Face));
	if (font->fontFace == NULL) {
		ri.Printf(PRINT_ALL, "^1RE_RegisterFont couldn't allocate memory for font face\n");
		if (faceData != NULL) {
			free(faceData);
		}
		ri.Free(out);
		font->name[0] = '\0';
		return;
	}
	memcpy(font->fontFace, &face, sizeof(FT_Face));

	font->faceData = faceData;
	font->q3Font = qfalse;
	font->bitmapFont = qfalse;  // this implies a freetype font
	font->pointSize = pointSize;
	font->extraGlyphs = NULL;
	// adding font
	Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));

	if (r_saveFontData->integer) {
		// q3 issue: this doesn't work if font->glyph->left is in the font info structure

		// 2016-02-24 at this point fontInfo_t has been changed so this file will not work with the .dat reading code, this is just for debugging
		ri.FS_WriteFile(va("fonts2/%s_%i.wcdat", baseName, pointSize), font, sizeof(fontInfo_t));
	}

	ri.Free(out);

	if (r_debugFonts->integer >= 2) {
		Q_strncpyz(name, fontName, sizeof(name));

		ri.Printf(PRINT_ALL, "%s font registered\n", COM_SkipPath(name));
		ri.Printf(PRINT_ALL, "%s %d  registered as %s  scale %f\n", fontName, pointSize, font->name, font->glyphScale);
	}
#endif
}

qboolean RE_GetGlyphInfo (fontInfo_t *fontInfo, int charValue, glyphInfo_t *glyphOut)
{
	int i, k;
	fontInfo_t *realFont;
	extraGlyphInfo_t *g, *bg;
	glyphInfo_t *glyph;
	FT_Face face;
	unsigned char *out, *imageBuff;
	int xOut, yOut, maxHeight;
	image_t *image;
	qhandle_t h;
	char *fontShaderName;
	float max;
	int left;

	if (fontInfo == NULL) {
		ri.Printf(PRINT_WARNING, "RE_GetGlyphInfo: font info is NULL\n");
		return qfalse;
	}
	if (charValue < 0) {
		ri.Printf(PRINT_WARNING, "RE_GetGlyphInfo: invalid character number %d\n", charValue);
		return qfalse;
	}

	realFont = NULL;
	for (i = 0;  i < registeredFontCount;  i++) {
		if (registeredFont[i].fontId == fontInfo->fontId) {
			realFont = &registeredFont[i];
			break;
		}
	}

	if (realFont == NULL) {
		ri.Printf(PRINT_WARNING, "RE_GetGlyphInfo:  couldn't font id %d\n", fontInfo->fontId);
		return qfalse;
	}

	// q3/ql bitmap fonts don't have the correct glyphs for ascii above 0x7f
	if (charValue <= 0x7e  &&  realFont->bitmapFont) {  // 0x7f is DEL, skip just in case
		*glyphOut = realFont->baseGlyphs[charValue];
		return qtrue;
	} else if (charValue < 255  &&  !realFont->bitmapFont) {  // these are automatically loaded when a font is first registered
		*glyphOut = realFont->baseGlyphs[charValue];
		return qtrue;
	}
	// unicode char
	g = realFont->extraGlyphs;
	while (g != NULL) {
		if (g->charValue == charValue) {
			//FIXME use glyphInfo_t in extraglyhs
			//glyphOut->charValue = g->charValue;
			glyphOut->height = g->height;
			glyphOut->top = g->top;
			glyphOut->bottom = g->bottom;
			glyphOut->pitch = g->pitch;
			glyphOut->xSkip = g->xSkip;
			glyphOut->left = g->left;
			glyphOut->imageWidth = g->imageWidth;
			glyphOut->imageHeight = g->imageHeight;
			glyphOut->s = g->s;
			glyphOut->t = g->t;
			glyphOut->s2 = g->s2;
			glyphOut->t2 = g->t2;
			glyphOut->glyph = g->glyph;
			Q_strncpyz(glyphOut->shaderName, g->shaderName, sizeof(glyphOut->shaderName));
			return qtrue;
		}
		g = g->next;
	}

	// allocate new glyph

	g = malloc(sizeof(extraGlyphInfo_t));
	if (g == NULL) {
		ri.Printf(PRINT_ALL, "^1RE_GetGlyphInfo could't get memory for new glyph\n");
		return qfalse;
	}

	Com_Memset(g, 0, sizeof(extraGlyphInfo_t));


	g->charValue = charValue;

	//R_IssuePendingRenderCommands();

	out = ri.Malloc(FONT_OUT_BUFFER_SIZE);
	if (out == NULL) {
		ri.Printf(PRINT_ALL, "^1RE_GetGlyphInfo ri.Malloc failure during output image creation\n");
		free(g);
		return qfalse;
	}
	Com_Memset(out, 0, FONT_OUT_BUFFER_SIZE);

	//FIXME multiple glyphs per image

	if (realFont->bitmapFont) {
		if (fallbackFonts) {
			face = fallbackFonts->face;
		} else {
			// not even a fallback font, this can happen if installation is
			// incorrect and not even the supplied files are found
			ri.Printf(PRINT_ALL, "^11RE_GetGlyphInfo fallback not found\n");
			ri.Free(out);
			free(g);
			return qfalse;
		}
	} else {
		face = *(FT_Face *)realFont->fontFace;
	}

	if (FT_Get_Char_Index(face, charValue) == 0) {
		fallbackFonts_t *fb;
		qboolean gotFallback = qfalse;

		if (r_debugFonts->integer >= 2) {
			ri.Printf(PRINT_ALL, "^5checking fallback fonts for %d 0x%x\n", charValue, charValue);
		}

		// check fallback fonts
		fb = fallbackFonts;
		while (fb) {
			if (r_debugFonts->integer >= 2) {
				ri.Printf(PRINT_ALL, "^5checking '%s'\n", fb->name);
			}
			if (FT_Get_Char_Index(fb->face, charValue) != 0) {
				// got it
				face = fb->face;
				if (r_debugFonts->integer >= 2) {
					ri.Printf(PRINT_ALL, "^5got fallback glyph %d 0x%x in %s\n", charValue, charValue, fb->name);
				}
				gotFallback = qtrue;
				break;
			}
			fb = fb->next;
		}

		if (!gotFallback) {
			if (r_debugFonts->integer >= 1) {
				ri.Printf(PRINT_WARNING, "couldn't get fallback glyph for %d 0x%x\n", charValue, charValue);
			}
			if (realFont->bitmapFont) {
				// use first ttf fallback (notosans..) to get tofu
				if (fallbackFonts != NULL) {
					face = fallbackFonts->face;
				} else {
					const glyphInfo_t *glyph;

					ri.Printf(PRINT_WARNING, "RE_GetGlyphInfo no font fallbacks defined, using stub for %c 0x%x\n", charValue, charValue);
					//*glyphOut = realFont->baseGlyphs[0];
					//*g = realFont->baseGlyphs[0];
					glyph = &realFont->baseGlyphs['*'];

					//FIXME duplicate code
					g->height = glyph->height;
					g->top = glyph->top;
					g->bottom = glyph->bottom;
					g->pitch = glyph->pitch;
					g->xSkip = glyph->xSkip;
					g->left = glyph->left;
					g->imageWidth = glyph->imageWidth;
					g->imageHeight = glyph->imageHeight;
					g->s = glyph->s;
					g->t = glyph->t;
					g->s2 = glyph->s2;
					g->t2 = glyph->t2;
					g->glyph = glyph->glyph;
					g->next = NULL;

					goto addnewglyphtolist;
				}
			}
		}
	}

	if (FT_Set_Char_Size(face, realFont->pointSize << 6, realFont->pointSize << 6, 72, 72)) {
	  ri.Printf(PRINT_ALL, "^1RE_RegisterFont: FreeType2, Unable to set face char size for extra glyph.\n");
	}

	xOut = 0;
	yOut = 0;
	maxHeight = 0;

	glyph = RE_ConstructGlyphInfo(out, FONT_OUT_BUFFER_SIZE, &xOut, &yOut, &maxHeight, face, (unsigned long)charValue, qfalse);

	// don't use font filename since shader max name length is MAX_QPATH
	fontShaderName = va("font-extra-glyph-%d-%d", realFont->fontId, charValue);

	//FIXME testing
	//for (i = 0;  i < 1024 * 1024;  i++) {
		//out[i] = i % 255;
		//out[i] = 255;
	//}

	imageBuff = ri.Malloc(FSIZE * FSIZE * 4);
	if (imageBuff == NULL) {
		//FIXME
		ri.Printf(PRINT_ALL, "^1RE_GetGlyphInfo ri.Malloc failure during output imageBuff creation\n");
		ri.Free(out);
		free(g);
		return qfalse;

	}

	left = 0;
	max = 0;
	//FIXME should be font out buffer size
	for ( k = 0; k < (FSIZE * FSIZE / 4) ; k++ ) {
		if (max < out[k]) {
			max = out[k];
		}
	}

	if (max > 0) {
		max = (256 - 1)/max;
	}

	for ( k = 0; k < (FSIZE * FSIZE / 4) ; k++ ) {
		imageBuff[left++] = 255;
		imageBuff[left++] = 255;
		imageBuff[left++] = 255;

		imageBuff[left++] = ((float)out[k] * max);
		//imageBuff[left++] = out[k];
	}

	//image = R_CreateImage(fontShaderName, imageBuff, FSIZE, FSIZE, qfalse, qfalse, haveClampToEdge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
	image = R_CreateImage(fontShaderName, imageBuff, FSIZE, FSIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
	h = RE_RegisterShaderFromImage(fontShaderName, LIGHTMAP_2D, image, qfalse);

	ri.Free(out);
	ri.Free(imageBuff);

	g->height = glyph->height;
	g->top = glyph->top;
	g->bottom = glyph->bottom;
	g->left = glyph->left;
	g->pitch = glyph->pitch;
	if (realFont->q3Font) {
		g->xSkip = glyph->xSkip;
		// hack for diactrical marks having small xSkip
		if (g->xSkip > 1) {
			//FIXME this assumes the q3Font is monospaced, doesn't have to be (see prop and propB in cg_drawtools.c
			//g->xSkip = 16;
		}
	} else {
		g->xSkip = glyph->xSkip;
	}
	g->imageWidth = glyph->imageWidth;
	g->imageHeight = glyph->imageHeight;
	g->s = glyph->s;
	g->t = glyph->t;
	g->s2 = glyph->s2;
	g->t2 = glyph->t2;
	g->glyph = h;
	g->next = NULL;

 addnewglyphtolist:

	if (realFont->extraGlyphs == NULL) {
		realFont->extraGlyphs = g;
	} else {
		bg = realFont->extraGlyphs;
		while (bg) {
			if (bg->next == NULL) {
				bg->next = g;
				break;
			}
			bg = bg->next;
		}
	}

	glyphOut->height = g->height;
	glyphOut->top = g->top;
	glyphOut->bottom = g->bottom;
	glyphOut->pitch = g->pitch;
	glyphOut->xSkip = g->xSkip;
	glyphOut->left = g->left;
	glyphOut->imageWidth = g->imageWidth;
	glyphOut->imageHeight = g->imageHeight;
	glyphOut->s = g->s;
	glyphOut->t = g->t;
	glyphOut->s2 = g->s2;
	glyphOut->t2 = g->t2;
	glyphOut->glyph = g->glyph;
	Q_strncpyz(glyphOut->shaderName, g->shaderName, sizeof(glyphOut->shaderName));

	if (r_debugFonts->integer >= 3) {
		ri.Printf(PRINT_ALL, "^6created glyph for %lu '%c' %s  %d %d %d %d %d %d %d %f %f %f %f\n", (long unsigned int)charValue, charValue, realFont->name,
			  g->height, g->top, g->bottom, g->pitch, g->xSkip, g->imageWidth, g->imageHeight, g->s, g->t, g->s2, g->t2);
	}

	return qtrue;
}

static void addFontFallback (const char *fileName)
{
	fallbackFonts_t *fb;
	void *faceData;
	int checksum;
	int len;

	if (fileName == NULL  ||  fileName[0] == '\0') {
		ri.Printf(PRINT_ALL, "^1addFontFallback: invalid file name\n");
		return;
	}

	//ri.Printf(PRINT_ALL, "addFontFallback:  loading fallback font '%s'\n", fileName);

	fb = malloc(sizeof(fallbackFonts_t));
	if (fb == NULL) {
		ri.Printf(PRINT_ALL, "^1addFontFallback:  couldn't allocate memory for font fallback '%s'\n", fileName);
		return;
	}

	Com_Memset(fb, 0, sizeof(fallbackFonts_t));

	//FIXME duplicate code
	if (ri.FS_FileIsInPAK(fileName, &checksum) == 1) {
		//ri.Printf(PRINT_ALL, "^2'%s' is in pak\n", fileName);
		len = ReadFileData(fileName, &faceData);
		if (len <= 0) {
			ri.Printf(PRINT_ALL, "^1addFontFallback:  couldn't load fallback font '%s' from pak file\n", fileName);
			free(fb);
			return;
		}
		fb->faceData = faceData;
		// in memory since it's in a pk3
		if (FT_New_Memory_Face(ftLibrary, fb->faceData, len, 0, &fb->face)) {
			ri.Printf(PRINT_ALL, "^1addFontFallback:  couldn't allocate new face for fallback font '%s' from pak file\n", fileName);
				free(fb->faceData);
				free(fb);
				return;
		}
	} else {
		// try file system
		fb->faceData = NULL;
		if (FT_New_Face(ftLibrary, ri.FS_FindSystemFile(fileName), 0, &fb->face)) {
			ri.Printf(PRINT_ALL, "^1addFontFallback:  couldn't allocate new face for fallback font '%s' from file system\n", fileName);
				free(fb);
				return;
		}


	}

	// got it
	Q_strncpyz(fb->name, fileName, sizeof(fb->name));

	ri.Printf(PRINT_ALL, "using fallbackfont: '%s'\n", fileName);

	if (fallbackFonts == NULL) {
		fallbackFonts = fb;
	} else {
		fallbackFonts_t *p;

		p = fallbackFonts;
		while (p) {
			if (p->next == NULL) {
				p->next = fb;
				break;
			}
			p = p->next;
		}
	}
}

void R_InitFreeType (void) {
#ifdef BUILD_FREETYPE
	int i;
	const char *fontName;

	if (FT_Init_FreeType( &ftLibrary )) {
		ri.Printf(PRINT_ALL, "^1R_InitFreeType: Unable to initialize FreeType.\n");
		return;
	}

	/////////////////////////////////////////

	// add fallback fonts
	//
	// for ql Windows also add : l_10646.ttf  segoeui.ttf  arialuni.ttf
	//

	if (r_defaultQlFontFallbacks->integer) {
		ri.Printf(PRINT_ALL, "^5trying default ql font fallbacks\n");

		addFontFallback(DEFAULT_SANS_FONT);
		addFontFallback("fonts/droidsansfallbackfull.ttf");
	}

	// cvar fallbacks
	i = 0;
	while (1) {
		const char *cvarName;
		const cvar_t *cvar;
		const char *fileName;

		i++;
		cvarName = va("r_fallbackFont%d", i);

		//FIXME archive?
		cvar = ri.Cvar_Get(cvarName, "", CVAR_TEMP);
		fileName = cvar->string;
		if (fileName == NULL  ||  fileName[0] == '\0') {
			// all done
			break;
		}

		ri.Printf(PRINT_ALL, "R_InitFreeType:  trying to load fallback font %d : %s\n", i, fileName);


		addFontFallback(fileName);
	}

	if (r_defaultMSFontFallbacks->integer) {
		ri.Printf(PRINT_ALL, "^5trying default MS font fallbacks\n");

		fontName = "fonts/l_10646.ttf";
		if (ri.FS_FileExists(fontName)) {
			addFontFallback(fontName);
		}
		fontName = "fonts/segoeui.ttf";
		if (ri.FS_FileExists(fontName)) {
			addFontFallback(fontName);
		}
		fontName = "fonts/arialuni.ttf";
		if (ri.FS_FileExists(fontName)) {
			addFontFallback(fontName);
		}
	}

	if (r_defaultSystemFontFallbacks->integer) {
		ri.Printf(PRINT_ALL, "^5trying default system font fallbacks\n");

#ifdef _WIN32
		addFontFallback("C:\\Windows\\Fonts\\l_10646.ttf");
		addFontFallback("C:\\Windows\\Fonts\\segoeui.ttf");
		addFontFallback("C:\\Windows\\Fonts\\arialuni.ttf");
#endif

#endif
	}

	if (r_defaultUnifontFallbacks->integer) {
		ri.Printf(PRINT_ALL, "^5trying Unifont fallbacks\n");
		addFontFallback("wcfonts/unifont_csur-8.0.01.ttf");
		addFontFallback("wcfonts/unifont_upper_csur-8.0.01.ttf");
	}

	ri.Printf(PRINT_ALL, "^5R_InitFreeType completed\n");
	registeredFontCount = 0;
}


void R_DoneFreeType (void)
{
#ifdef BUILD_FREETYPE
	int i;
	fallbackFonts_t *fb;

	for (i = 0;  i < registeredFontCount;  i++) {
		fontInfo_t *f;
		extraGlyphInfo_t *g;

		f = &registeredFont[i];
		g = f->extraGlyphs;
		while (g != NULL) {
			extraGlyphInfo_t *gnext;

			gnext = g->next;
			free(g);
			g = gnext;
		}
		if (f->faceData != NULL) {
			free(f->faceData);
		}
		if (!f->bitmapFont) {
			FT_Done_Face(*(FT_Face *)f->fontFace);
		}
		free(f->fontFace);
	}

	fb = fallbackFonts;
	while (fb) {
		fallbackFonts_t *fborig;

		fborig = fb;

		if (fb->faceData != NULL) {
			free(fb->faceData);
		}
		FT_Done_Face(fb->face);

		fb = fb->next;
		free(fborig);
	}
	fallbackFonts = NULL;

	if (ftLibrary) {
		FT_Done_FreeType( ftLibrary );
		ftLibrary = NULL;
	}
#endif

	ri.Printf(PRINT_ALL, "^5R_DoneFreeType\n");
	registeredFontCount = 0;
}

void R_FontList_f (void)
{
	int i;
	const fallbackFonts_t *f;

	for (i = 0;  i < registeredFontCount;  i++) {
		const fontInfo_t *fi;
		const extraGlyphInfo_t *eg;
		int count;

		fi = &registeredFont[i];

		count = 0;
		eg = fi->extraGlyphs;
		while (eg) {
			count++;
			eg = eg->next;
		}

		ri.Printf(PRINT_ALL, "%d '%s' '%s' %d  extra glyphs: %d\n", i + 1, fi->name, fi->registerName, fi->pointSize, count);
	}

	f = fallbackFonts;
	while (f) {
		ri.Printf(PRINT_ALL, "fallback: '%s'\n", f->name);
		f = f->next;
	}
}

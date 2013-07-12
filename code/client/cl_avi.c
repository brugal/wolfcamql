/*
===========================================================================
Copyright (C) 2005-2006 Tim Angus

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

#include "client.h"
#include "snd_local.h"
#include "cl_avi.h"

#include <errno.h>

/////////////////////////////

#define INDEX_FILENAME_EXT ".stdindex.dat"
#define INDEX_VIDEO_FILENAME_EXT ".vindex.dat"
#define INDEX_AUDIO_FILENAME_EXT ".aindex.dat"


#ifndef MACOS_X
#ifndef ftello
#define ftello ftello64
#endif

#ifndef fseeko
#define fseeko fseeko64
#endif
#endif

//static int PcmBytesInBuffer = 0;

static void CL_NewRiff (aviFileData_t *afd);
static void CL_PadEndOfFile (const aviFileData_t *afd);
static void CL_PadEndOfFileExt (const aviFileData_t *afd, int pad);


//static aviFileData_t afd;

#define MAX_AVI_BUFFER (1024 * 24)

static byte buffer[ MAX_AVI_BUFFER ];
static byte buffer2[8];
static int  bufIndex = 0;

/*
===============
SafeFS_Write
===============
*/
static ID_INLINE void SafeFS_Write( const void *buffer, int len, fileHandle_t f )
{
    int r;

    r = FS_Write(buffer, len, f);
    if (r < len) {
        Com_Error(ERR_DROP, "SafeFS_Write()  failed to write to file %d < %d  f:%d", r, len, f);
    }

    FS_Flush(f);
}

/*
===============
WRITE_STRING
===============
*/
static ID_INLINE void WRITE_STRING( const char *s )
{
    if (bufIndex + strlen(s) >= MAX_AVI_BUFFER) {
        Com_Printf("WRITE_STRING %d + %d > MAX_AVI_BUFFER (%d)\n", bufIndex, (int)strlen(s), MAX_AVI_BUFFER);
        return;
    }

    Com_Memcpy( &buffer[ bufIndex ], s, strlen( s ) );
    bufIndex += strlen( s );
    //Com_Printf("WRITE_STRING %d\n", bufIndex);
}

static ID_INLINE void fwriteString (const char *s, fileHandle_t f)
{
    SafeFS_Write(s, strlen(s), f);
}

/*
===============
WRITE_4BYTES
===============
*/
static ID_INLINE void WRITE_4BYTES( int x )
{
    if (bufIndex + 4 >= MAX_AVI_BUFFER) {
        Com_Printf("WRITE_4BYTES %d + %d > MAX_AVI_BUFFER %d\n", bufIndex, 4, MAX_AVI_BUFFER);
        return;
    }

    buffer[ bufIndex + 0 ] = (byte)( ( x >>  0 ) & 0xFF );
    buffer[ bufIndex + 1 ] = (byte)( ( x >>  8 ) & 0xFF );
    buffer[ bufIndex + 2 ] = (byte)( ( x >> 16 ) & 0xFF );
    buffer[ bufIndex + 3 ] = (byte)( ( x >> 24 ) & 0xFF );
    bufIndex += 4;
}

static ID_INLINE void fwrite4 (int x, fileHandle_t f)
{
  buffer2[ 0 ] = (byte)( ( x >>  0 ) & 0xFF );
  buffer2[ 1 ] = (byte)( ( x >>  8 ) & 0xFF );
  buffer2[ 2 ] = (byte)( ( x >> 16 ) & 0xFF );
  buffer2[ 3 ] = (byte)( ( x >> 24 ) & 0xFF );
  SafeFS_Write(buffer2, 4, f);
}

/*
===============
WRITE_8BYTES
===============
*/
static ID_INLINE void WRITE_8BYTES( int64_t x )
{
    if (bufIndex + 8 >= MAX_AVI_BUFFER) {
        Com_Printf("WRITE_8BYTES %d + %d > MAX_AVI_BUFFER %d\n", bufIndex, 8, MAX_AVI_BUFFER);
        return;
    }

    buffer[ bufIndex + 0 ] = (byte)( ( x >>  0 ) & 0xFF );
    buffer[ bufIndex + 1 ] = (byte)( ( x >>  8 ) & 0xFF );
    buffer[ bufIndex + 2 ] = (byte)( ( x >> 16 ) & 0xFF );
    buffer[ bufIndex + 3 ] = (byte)( ( x >> 24 ) & 0xFF );

    buffer[ bufIndex + 4 ] = (byte)( ( x >> 32 ) & 0xFF );
    buffer[ bufIndex + 5 ] = (byte)( ( x >> 40 ) & 0xFF );
    buffer[ bufIndex + 6 ] = (byte)( ( x >> 48 ) & 0xFF );
    buffer[ bufIndex + 7 ] = (byte)( ( x >> 56 ) & 0xFF );

    bufIndex += 4;
}

static ID_INLINE void fwrite8 (int64_t x, fileHandle_t f)
{
  buffer2[  0 ] = (byte)( ( x >>  0 ) & 0xFF );
  buffer2[  1 ] = (byte)( ( x >>  8 ) & 0xFF );
  buffer2[  2 ] = (byte)( ( x >> 16 ) & 0xFF );
  buffer2[  3 ] = (byte)( ( x >> 24 ) & 0xFF );

  buffer2[  4 ] = (byte)( ( x >> 32 ) & 0xFF );
  buffer2[  5 ] = (byte)( ( x >> 40 ) & 0xFF );
  buffer2[  6 ] = (byte)( ( x >> 48 ) & 0xFF );
  buffer2[  7 ] = (byte)( ( x >> 56 ) & 0xFF );
  SafeFS_Write(buffer2, 8, f);
}

/*
===============
WRITE_2BYTES
===============
*/
static ID_INLINE void WRITE_2BYTES( int x )
{
    if (bufIndex + 2 >= MAX_AVI_BUFFER) {
        Com_Printf("WRITE_2BYTES %d + %d > MAX_AVI_BUFFER %d\n", bufIndex, 2, MAX_AVI_BUFFER);
        return;
    }

    buffer[ bufIndex + 0 ] = (byte)( ( x >>  0 ) & 0xFF );
    buffer[ bufIndex + 1 ] = (byte)( ( x >>  8 ) & 0xFF );
    bufIndex += 2;
}

static ID_INLINE void fwrite2 (int x, fileHandle_t f)
{
  buffer2[  0 ] = (byte)( ( x >>  0 ) & 0xFF );
  buffer2[  1 ] = (byte)( ( x >>  8 ) & 0xFF );
  SafeFS_Write(buffer2, 2, f);
}

/*
===============
WRITE_1BYTES
===============
*/
static ID_INLINE void WRITE_1BYTES( int x )
{
    if (bufIndex + 1 >= MAX_AVI_BUFFER) {
        Com_Printf("WRITE_1BYTES %d + %d > MAX_AVI_BUFFER %d\n", bufIndex, 1, MAX_AVI_BUFFER);
        return;
    }

    buffer[ bufIndex ] = x;
    bufIndex += 1;
}

static ID_INLINE void fwrite1 (int x, fileHandle_t f)
{
    buffer2[0] = x;
    SafeFS_Write(buffer2, 1, f);
}

/*
===============
START_CHUNK
===============
*/
static ID_INLINE void START_CHUNK (aviFileData_t *afd, const char *s)
{
  if( afd->chunkStackTop == MAX_RIFF_CHUNKS )
  {
    Com_Error( ERR_DROP, "ERROR: Top of chunkstack breached" );
  }

  afd->chunkStack[ afd->chunkStackTop ] = bufIndex;
  afd->chunkStackTop++;
  WRITE_STRING( s );
  WRITE_4BYTES( 0 );
}

/*
===============
END_CHUNK
===============
*/
static ID_INLINE void END_CHUNK (aviFileData_t *afd)
{
  int endIndex = bufIndex;

  if( afd->chunkStackTop <= 0 )
  {
    Com_Error( ERR_DROP, "ERROR: Bottom of chunkstack breached" );
  }

  afd->chunkStackTop--;
  bufIndex = afd->chunkStack[ afd->chunkStackTop ];
  bufIndex += 4;
  WRITE_4BYTES( endIndex - bufIndex - 4 );
  bufIndex = endIndex;
  bufIndex = PAD( bufIndex, 2 );
}

/*
===============
CL_CreateAVIHeader
===============
*/
static void CL_CreateAVIHeader (aviFileData_t *afd)
{
  bufIndex = 0;
  afd->chunkStackTop = 0;
  int i;

  afd->newRiffSizePos = 4;
  START_CHUNK(afd, "RIFF");
  {  // riff
    WRITE_STRING( "AVI " );
    {
      START_CHUNK(afd, "LIST");
      {  // hdrl avi list
        WRITE_STRING( "hdrl" );
        WRITE_STRING( "avih" );
        WRITE_4BYTES( 56 );                     //"avih" "chunk" size
        WRITE_4BYTES( afd->framePeriod );        //dwMicroSecPerFrame
        WRITE_4BYTES( afd->maxRecordSize *
            afd->frameRate );                    //dwMaxBytesPerSec
        WRITE_4BYTES( 0 );                      //dwReserved1
        if (0) {  //(afd->useOpenDml) {
            WRITE_4BYTES(0x00000100);  // AVIF_ISINTERLEAVED
        } else {
            WRITE_4BYTES( 0x110 );                  //dwFlags bits HAS_INDEX and IS_INTERLEAVED
        }
        afd->mainHeaderNumVideoFramesHeaderOffset = bufIndex;
        WRITE_4BYTES( afd->numVideoFrames );     //dwTotalFrames
        WRITE_4BYTES( 0 );                      //dwInitialFrame

        if( afd->audio )                         //dwStreams
          WRITE_4BYTES( 2 );
        else
          WRITE_4BYTES( 1 );

        afd->mainHeaderMaxRecordSizeHeaderOffset = bufIndex;
        WRITE_4BYTES( afd->maxRecordSize );      //dwSuggestedBufferSize
        WRITE_4BYTES( afd->width );              //dwWidth
        WRITE_4BYTES( afd->height );             //dwHeight
        WRITE_4BYTES( 0 );                      //dwReserved[ 0 ]
        WRITE_4BYTES( 0 );                      //dwReserved[ 1 ]
        WRITE_4BYTES( 0 );                      //dwReserved[ 2 ]
        WRITE_4BYTES( 0 );                      //dwReserved[ 3 ]

        START_CHUNK(afd, "LIST");
        {  // video list
          WRITE_STRING( "strl" );
          WRITE_STRING( "strh" );
          WRITE_4BYTES( 56 );                   //"strh" "chunk" size
          WRITE_STRING( "vids" );

          if (afd->codec == CODEC_MJPEG) {
              WRITE_STRING("MJPG");
          } else if (afd->codec == CODEC_HUFFYUV) {
              WRITE_STRING("HFYU");
          } else {
              WRITE_4BYTES(0);                  // BI_RGB
          }

          WRITE_4BYTES( 0 );                    //dwFlags
          WRITE_4BYTES( 0 );                    //dwPriority
          WRITE_4BYTES( 0 );                    //dwInitialFrame

          WRITE_4BYTES( 1 );                    //dwTimescale
          WRITE_4BYTES( afd->frameRate );        //dwDataRate
          WRITE_4BYTES( 0 );                    //dwStartTime
          afd->numVideoFramesHeaderOffset = bufIndex;
          WRITE_4BYTES( afd->numVideoFrames );   //dwDataLength

          afd->videoHeaderMaxRecordSizeHeaderOffset = bufIndex;
          WRITE_4BYTES( afd->maxRecordSize );    //dwSuggestedBufferSize
          WRITE_4BYTES( -1 );                   //dwQuality
          WRITE_4BYTES( 0 );                    //dwSampleSize
          WRITE_2BYTES( 0 );                    //rcFrame
          WRITE_2BYTES( 0 );                    //rcFrame
          WRITE_2BYTES( afd->width );            //rcFrame
          WRITE_2BYTES( afd->height );           //rcFrame

          WRITE_STRING( "strf" );

          if (afd->codec == CODEC_HUFFYUV) {
              WRITE_4BYTES(40 + afd->AC->extradata_size);  //"strf" "chunk" size
              WRITE_4BYTES(40 + afd->AC->extradata_size);  //biSize
          } else {
              WRITE_4BYTES(40);                   //"strf" "chunk" size
              WRITE_4BYTES(40);                   //biSize
          }
          WRITE_4BYTES( afd->width );            //biWidth
          WRITE_4BYTES( afd->height );           //biHeight
          WRITE_2BYTES( 1 );                    //biPlanes
          WRITE_2BYTES( 24 );                   //biBitCount

          // biCompression:
          if (afd->codec == CODEC_MJPEG) {
              WRITE_STRING("MJPG");
              WRITE_4BYTES(afd->width * afd->height);      //biSizeImage
          } else if (afd->codec == CODEC_HUFFYUV) {
              WRITE_STRING("HFYU");
              WRITE_4BYTES(afd->width * afd->height * 3);  //biSizeImage
          } else {
              WRITE_4BYTES(0);                             // BI_RGB
              WRITE_4BYTES(afd->width * afd->height * 3);  //biSizeImage
          }

          WRITE_4BYTES( 0 );                    //biXPelsPetMeter
          WRITE_4BYTES( 0 );                    //biYPelsPetMeter
          WRITE_4BYTES( 0 );                    //biClrUsed
          WRITE_4BYTES( 0 );                    //biClrImportant

          if (afd->codec == CODEC_HUFFYUV) {
              for (i = 0;  i < afd->AC->extradata_size;  i++) {
                  WRITE_1BYTES(afd->AC->extradata[i]);
              }
          }

          if (afd->useOpenDml) {
              START_CHUNK(afd, "indx");
              WRITE_2BYTES(4);  // wLongsPerEntry  size of each entry in aIndex array, must be 4
              WRITE_1BYTES(0);  // must be 0 or AVI_INDEX_2FIELD
              WRITE_1BYTES(0);  // must be  0 == AVI_INDEX_OF_INDEXES

              afd->odmlVideonEntriesInUseHeaderOffset = bufIndex;
              WRITE_4BYTES(afd->numVIndexEntries);  // nEntriesInUse  index of first unused member in aIndex array

              WRITE_STRING("00dc");  //FIXME chunkid   is this right even for uncompressed?  others seem to do it

              WRITE_4BYTES(0);  // dwReserved[0]  meaning differs for each index  must be 0
              WRITE_4BYTES(0);  // dwReserved[1]  meaning differs for each index  must be 0
              WRITE_4BYTES(0);  // dwReserved[2]  meaning differs for each index  must be 0
              afd->odmlVideoIndexToIndexesHeaderOffset = bufIndex;
              //FIXME  other programs are doing (* 2)
              for (i = 0;  i < MAX_OPENDML_INDEX_ENTRIES * 2;  i++) {
                  aviSuperIndex_t *as;

                  as = &afd->vIndex[i];
                  WRITE_8BYTES(as->offset);  // absolute file ofset, offset 0 is unused entry
                  WRITE_4BYTES(as->size);  // size of index chunk at this offset
                  WRITE_4BYTES(as->duration);  //FIXME time span in stream ticks
              }
              END_CHUNK(afd);
          }
        }
        END_CHUNK(afd);


        if( afd->audio )
        {
          START_CHUNK(afd, "LIST");
          {  // audio list
            WRITE_STRING( "strl" );
            WRITE_STRING( "strh" );
            WRITE_4BYTES( 56 );                 //"strh" "chunk" size
            WRITE_STRING( "auds" );
            WRITE_4BYTES( 0 );                  //FCC
            WRITE_4BYTES( 0 );                  //dwFlags
            WRITE_4BYTES( 0 );                  //dwPriority
            WRITE_4BYTES( 0 );                  //dwInitialFrame

            WRITE_4BYTES( afd->a.sampleSize );   //dwTimescale
            WRITE_4BYTES( afd->a.sampleSize *
                afd->a.rate );                   //dwDataRate
            WRITE_4BYTES( 0 );                  //dwStartTime
            afd->numAudioFramesHeaderOffset = bufIndex;
            //WRITE_4BYTES( afd->a.totalBytes / afd->a.sampleSize );             //dwDataLength
            WRITE_4BYTES(afd->audioDataLength);  // dwDataLength

            WRITE_4BYTES( 0 );                  //dwSuggestedBufferSize
            WRITE_4BYTES( -1 );                 //dwQuality
            WRITE_4BYTES( afd->a.sampleSize );   //dwSampleSize
            WRITE_2BYTES( 0 );                  //rcFrame
            WRITE_2BYTES( 0 );                  //rcFrame
            WRITE_2BYTES( 0 );                  //rcFrame
            WRITE_2BYTES( 0 );                  //rcFrame

            WRITE_STRING( "strf" );
            WRITE_4BYTES( 18 );                 //"strf" "chunk" size
            WRITE_2BYTES( afd->a.format );       //wFormatTag
            WRITE_2BYTES( afd->a.channels );     //nChannels
            WRITE_4BYTES( afd->a.rate );         //nSamplesPerSec
            WRITE_4BYTES( afd->a.sampleSize *
                afd->a.rate );                   //nAvgBytesPerSec
            WRITE_2BYTES( afd->a.sampleSize );   //nBlockAlign
            WRITE_2BYTES( afd->a.bits );         //wBitsPerSample
            WRITE_2BYTES( 0 );                  //cbSize

            if (afd->useOpenDml) {
                START_CHUNK(afd, "indx");
                WRITE_2BYTES(4);  // wLongsPerEntry  size of each entry in aIndex array, must be 4
                WRITE_1BYTES(0);  // must be 0 or AVI_INDEX_2FIELD
                WRITE_1BYTES(0);  // must be  0 == AVI_INDEX_OF_INDEXES

                afd->odmlAudionEntriesInUseHeaderOffset = bufIndex;
                WRITE_4BYTES(afd->numAIndexEntries);  // nEntriesInUse  index of first unused member in aIndex array
                WRITE_STRING("01wb");  // dwChunkId
                WRITE_4BYTES(0);  // dwReserved[0]  meaning differs for each index  must be 0
                WRITE_4BYTES(0);  // dwReserved[1]  meaning differs for each index  must be 0
                WRITE_4BYTES(0);  // dwReserved[2]  meaning differs for each index  must be 0
                afd->odmlAudioIndexToIndexesHeaderOffset = bufIndex;
                //FIXME other programs using (* 2)
                for (i = 0;  i < MAX_OPENDML_INDEX_ENTRIES * 2;  i++) {
                    aviSuperIndex_t *as;

                    as = &afd->aIndex[i];
                    WRITE_8BYTES(as->offset);  // absolute file offset, offset 0 is unused entry
                    WRITE_4BYTES(as->size);  // size of index chunk at this offset
                    WRITE_4BYTES(as->duration);  //FIXME time span in stream ticks
                }
                END_CHUNK(afd);
            }
          }
          END_CHUNK(afd);
        }

        if (afd->useOpenDml) {
            START_CHUNK(afd, "LIST");
            WRITE_STRING("odml");
            START_CHUNK(afd, "dmlh");
            afd->odmlDmlhHeaderOffset = bufIndex;
            WRITE_4BYTES(afd->odmlNumVideoFrames);  // filled in later
            //FIXME no clue what this is for
            for (i = 0;  i < 248;  i+= 4) {
                WRITE_4BYTES(0);
            }
            END_CHUNK(afd);
            END_CHUNK(afd);
        }

        START_CHUNK(afd, "LIST"); {
            WRITE_STRING("INFO");
            START_CHUNK(afd, "ISFT"); {
                //WRITE_STRING("Software");
                WRITE_STRING(va("wolfcamql version %s", WOLFCAM_VERSION));
            }
            END_CHUNK(afd);
        }
        END_CHUNK(afd);

      }
      END_CHUNK(afd);

      afd->moviOffset1 = bufIndex;
      afd->moviOffset = bufIndex;
      afd->moviDataOffset = bufIndex;

      afd->newMoviListSizePos = bufIndex + 4;
      START_CHUNK(afd, "LIST");
      {
        WRITE_STRING( "movi" );
        //afd->moviDataOffset = bufIndex;  //FIXME maybe not for super index, will need to skip idx1 ??
      }
    }
  }
}

static void CL_InitIndexes (const aviFileData_t *afd)
{
  // 0
  fwriteString("00ix", afd->idxVF);
    //fwriteString("ix00", afd->idxVF);
  // 4
  fwrite4(0, afd->idxVF);  // size filled in later
  // 8
  fwrite2(2, afd->idxVF);  // wLongsPerEntry
  // 10
  fwrite1(0, afd->idxVF);  // bIndexSubType,  0: frame index
  // 11
  fwrite1(1, afd->idxVF);  // bIndexType, 1: index of chunks AVI_INDEX_OF_CHUNKS
  // 12
  fwrite4(0, afd->idxVF);  // nEntriesInUse
  // 16
  fwriteString("00dc", afd->idxVF);  // dwChunkId
  //fwrite4(0, afd->idxVF);  // chunk ID, don't know
  // 20
  fwrite8(0, afd->idxVF);  // qwBaseOffset -- movi list
  // 28
  fwrite4(0, afd->idxVF);  // dwReserved[3]  must be 0
  // 32


  fwriteString("01ix", afd->idxAF);
  //fwriteString("ix01", afd->idxAF);
  fwrite4(0, afd->idxAF);  // size filled in later
  fwrite2(2, afd->idxAF);  // wLongsPerEntry
  fwrite1(0, afd->idxAF);  // bIndexSubType,  0: frame index
  fwrite1(1, afd->idxAF);  // bIndexType, 1: index of chunks
  fwrite4(0, afd->idxAF);  // nEntriesInUse
  fwriteString("01wb", afd->idxAF);  // dwChunkId
  fwrite8(0, afd->idxAF);  // qwBaseOffset -- movi list
  fwrite4(0, afd->idxAF);  // dwReserved[3]  must be 0
}

extern int sys_timeBase;

/*
===============
CL_OpenAVIForWriting

Creates an AVI file and gets it into a state where
writing the actual data can begin
===============
*/

// us:  called internally if odml isn't used and a series of avi files is
// written for one recording

qboolean CL_OpenAVIForWriting (aviFileData_t *afd, const char *fileName, qboolean us, qboolean avi, qboolean noSoundAvi, qboolean wav, qboolean tga, qboolean jpg, qboolean depth, qboolean split, qboolean left)
{
    byte *cBuffer, *eBuffer;
    int size;
    fileHandle_t wf;
    int vcount;
    int pcount;
    char sbuf[MAX_QPATH];
    //int i;
    int startTime;

    if (afd->recording) {
        Com_Printf("^1CL_OpenAVIForWriting() already recording\n");
        return qfalse;
    }

  if (!us) {
      afd->PcmBytesInBuffer = 0;  // audio static buffer
  }

  //Com_Printf("^2CL_OpenAVIForWriting(%s)\n", fileName);

#ifdef _FILE_OFFSET_BITS
  //Com_Printf("file offset bits: %d\n", _FILE_OFFSET_BITS);
#endif

  cBuffer = afd->cBuffer;
  eBuffer = afd->eBuffer;
  wf = afd->wavFile;
  vcount = afd->vidFileCount;
  pcount = afd->picCount;
  startTime = afd->startTime;
  Q_strncpyz(sbuf, afd->givenFileName, MAX_QPATH);

  Com_Memset(afd, 0, sizeof(aviFileData_t));

  if (us) {
      afd->cBuffer = cBuffer;
      afd->eBuffer = eBuffer;
      afd->wavFile = wf;
      afd->vidFileCount = vcount + 1;
      afd->picCount = pcount;
      afd->startTime = startTime;
      Q_strncpyz(afd->givenFileName, sbuf, MAX_QPATH);
  } else {
      afd->startTime = cls.realtime;
  }

  afd->avi = avi;
  afd->wav = wav;
  afd->tga = tga;
  afd->jpg = jpg;
  afd->noSoundAvi = noSoundAvi;
  if (noSoundAvi) {
      afd->avi = qtrue;
  }
  afd->depth = depth;
  afd->split = split;
  afd->left = left;

  // Don't start if a framerate has not been chosen
  if( cl_aviFrameRate->integer <= 0 ) {
      Com_Printf( S_COLOR_RED "cl_aviFrameRate must be >= 1\n" );
      return qfalse;
  }

  //Q_strncpyz( afd->fileName, fileName, MAX_QPATH );


  if (!us) {
      if (!*fileName) {
          //Com_sprintf(afd->givenFileName, MAX_QPATH, "%d", Sys_Milliseconds() + sys_timeBase);
          Com_sprintf(afd->givenFileName, MAX_QPATH, "%d", (unsigned int)time(NULL));
      } else {
          Q_strncpyz( afd->givenFileName, fileName, MAX_QPATH );
      }
      if (depth) {
          afd->depth = qtrue;
          Q_strcat(afd->givenFileName, sizeof(afd->givenFileName), "-depth");
      }
      if (split) {
          afd->split = qtrue;
          if (left) {
              afd->left = qtrue;
              Q_strcat(afd->givenFileName, sizeof(afd->givenFileName), "-left");
          } else {
              afd->left = qfalse;
              Q_strcat(afd->givenFileName, sizeof(afd->givenFileName), "-right");
          }
      }
      //Com_Printf("^3record: %s\n", afd->givenFileName);
  } else {
      //Q_strncpyz(afd->givenFileName, afd->fileName, MAX_QPATH);
  }

  Com_sprintf(afd->fileName, MAX_QPATH, "videos/%s-%04d.%s", afd->givenFileName, afd->vidFileCount, cl_aviExtension->string);
  //Com_Printf("%s\n", afd->fileName);

  if( ( afd->f = FS_FOpenFileWrite( afd->fileName ) ) <= 0 ) {
      Com_Printf("CL_OpenAVIForWriting()  couldn't open video file\n");
      return qfalse;
  }

  if( ( afd->idxF = FS_FOpenFileWrite(va("%s%s", afd->fileName, INDEX_FILENAME_EXT))) <= 0 )
  {
      Com_Printf("CL_OpenAVIForWriting() couldn't open standard index file '%s'\n", va("%s%s", afd->fileName, INDEX_FILENAME_EXT));
      FS_FCloseFile( afd->f );
      return qfalse;
  }
  if( ( afd->idxVF = FS_FOpenFileWrite(va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT))) <= 0)
  {
      Com_Printf("CL_OpenAVIForWriting() couldn't open video index file\n");
      FS_FCloseFile( afd->f );
      FS_FCloseFile(afd->idxF);
      return qfalse;
  }
  if( ( afd->idxAF = FS_FOpenFileWrite(va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT))) <= 0)
  {
      Com_Printf("CL_OpenAVIForWriting() couldn't open audio index file\n");
      FS_FCloseFile( afd->f );
      FS_FCloseFile(afd->idxF);
      FS_FCloseFile(afd->idxVF);
      return qfalse;
  }


  //Com_Printf("getting stream handle\n");
  afd->file = FS_FileForHandle(afd->f);
  //Com_Printf("file %p  f:%d\n", afd->file, afd->f);
  afd->frameRate = cl_aviFrameRate->integer;
  afd->framePeriod = (int)( 1000000.0f / afd->frameRate );
  afd->width = cls.glconfig.vidWidth;
  afd->height = cls.glconfig.vidHeight;

  //if (cl_aviUseOpenDml->integer) {
  if (cl_aviAllowLargeFiles->integer) {
      afd->useOpenDml = qtrue;
      //Com_Printf("opendml large avi support\n");
  } else {
      afd->useOpenDml = qfalse;
      //Com_Printf("regular avi (size limit)\n");
  }

  if (!Q_stricmp(cl_aviCodec->string, "mjpeg")) {
      afd->codec = CODEC_MJPEG;
  } else if (!Q_stricmp(cl_aviCodec->string, "huffyuv")) {
      afd->codec = CODEC_HUFFYUV;
  } else {
      afd->codec = CODEC_UNCOMPRESSED;
  }

  if (!us) {
      afd->cBuffer = malloc(afd->width * afd->height * 4 + 18);
      afd->eBuffer = malloc(afd->width * afd->height * 4 + 18);
  }

  afd->a.rate = dma.speed;
  afd->a.format = WAV_FORMAT_PCM;
  afd->a.channels = dma.channels;
  afd->a.bits = dma.samplebits;
  afd->a.sampleSize = ( afd->a.bits / 8 ) * afd->a.channels;

#if 0
  if( afd->a.rate % afd->frameRate )
  {
    int suggestRate = afd->frameRate;

    while( ( afd->a.rate % suggestRate ) && suggestRate >= 1 )
      suggestRate--;

    if (!afd->noSoundAvi) {
        Com_Printf( S_COLOR_YELLOW "WARNING: cl_aviFrameRate is not a divisor of the audio rate, suggest %d\n", suggestRate );
    }
  }
#endif

  if( !Cvar_VariableIntegerValue( "s_initsound" ) ) {
    afd->audio = qfalse;
  } else if( Q_stricmp( Cvar_VariableString( "s_backend" ), "OpenAL" ) ) {
      if( afd->a.bits != 16 || afd->a.channels != 2 ) {
        Com_Printf( S_COLOR_YELLOW "WARNING: Audio format of %d bit/%d channels not supported", afd->a.bits, afd->a.channels );
        afd->audio = qfalse;
      } else {
        afd->audio = qtrue;
      }
  } else {
    afd->audio = qfalse;
    Com_Printf( S_COLOR_YELLOW "WARNING: Audio capture is not supported "
        "with OpenAL. Set s_useOpenAL to 0 for audio capture\n" );
  }

  if (afd->noSoundAvi) {
      afd->audio = qfalse;
  }

  //FIXME testing odml
  if (Cvar_VariableIntegerValue("testodml")) {
      afd->newRiffOrCloseFileSize = (1024 * 1024) * Cvar_VariableIntegerValue("testodml");
  } else {
      afd->newRiffOrCloseFileSize = (1024 * 1024) * 950;  //950;  //(1024 * 1024) * 20;  //(1024 * 1024 * 1024);  //FIXME  base it on sample sizes subtracted from defined value
  }

  if (!us  &&  afd->codec == CODEC_HUFFYUV) {
      afd->AC = calloc(1, sizeof(AVCodecContext));
      if (!afd->AC) {
          Com_Error(ERR_DROP, "%s couldn't allocate memory for codec", __FUNCTION__);
      }
      afd->AC->priv_data = calloc(1, sizeof(HYuvContext));
      //afd->AC->priv_data = calloc(1, 1024 * 1024);  //FIXME size and free
      if (!afd->AC->priv_data) {
          Com_Error(ERR_DROP, "%s couldn't allocate memory for codec private data", __FUNCTION__);
      }
      afd->AC->width = afd->width;  //800;
      afd->AC->height = afd->height;  //600;
      huffyuv_encode_init(afd->AC);
  }

  // This doesn't write a real header, but allocates the
  // correct amount of space at the beginning of the file
  CL_CreateAVIHeader(afd);

  SafeFS_Write( buffer, bufIndex, afd->f );
  afd->riffSize = bufIndex;

  bufIndex = 0;
  START_CHUNK(afd, "idx1");
  SafeFS_Write( buffer, bufIndex, afd->idxF );

  CL_InitIndexes(afd);

  afd->moviSize = 4; // For the "movi"
  afd->fileOpen = qtrue;

  afd->riffCount = 1;

  // testing

  if (!us  &&  wav  &&  afd == &afdMain) {
      //Com_sprintf(sbuf, MAX_QPATH, "videos/%s.wav", afd->givenFileName);
      Com_sprintf(sbuf, MAX_QPATH, "videos/%s.wav", afd->givenFileName);
      afd->wavFile = FS_FOpenFileWrite(sbuf);
      if (!afd->wavFile) {
          Com_Printf("couldn't open wav file\n");
      }

      size = 0x7fffffff;  // bogus size

      fwriteString("RIFF", afd->wavFile);
      fwrite4(size, afd->wavFile);  // bogus size
      fwriteString("WAVE", afd->wavFile);
      fwriteString("fmt ", afd->wavFile);
      fwrite4(16, afd->wavFile);  // format len
      fwrite2(afd->a.format, afd->wavFile);  // WAVE_FORMAT_PCM
      fwrite2(2, afd->wavFile);  // wChannels
      fwrite4(afd->a.rate, afd->wavFile);  // sample rate
      fwrite4((2 * afd->a.rate * (16 >> 3)), afd->wavFile);  // dwAvgBytesPerSec
      fwrite2((2 * (16 >> 3)), afd->wavFile);  // wBlockAlign
      fwrite2(16, afd->wavFile);  // bits per sample
      fwriteString("data", afd->wavFile);
      fwrite4(size - 44, afd->wavFile);
  }

  afd->recording = qtrue;
  return qtrue;
}

/*
===============
CL_CheckRiffSize
===============
*/
static qboolean CL_CheckRiffSize (aviFileData_t *afd, int bytesToAdd)
{
    int64_t newRiffSize;

    newRiffSize =
        afd->riffSize +                // Current riff size
        bytesToAdd +                  // What we want to add
        ( afd->numIndices * 16 ) +     // The index
        4;                            // The index size

  if (afd->useOpenDml) {
      //newFileSize = afd->fileSize + bytesToAdd;
      fseeko(afd->file, 0, SEEK_END);
      newRiffSize = ftello(afd->file) - ((afd->riffCount - 1) * (int64_t)afd->newRiffOrCloseFileSize) + bytesToAdd;
  }

  // I assume all the operating systems
  // we target can handle a 2Gb file
  if( newRiffSize > afd->newRiffOrCloseFileSize )
  {

      if (!afd->useOpenDml) {
          // Close the current file...
          CL_CloseAVI(afd, qtrue);

          // ...And open a new one
          //CL_OpenAVIForWriting( va( "%s_", afd->fileName ), qtrue );
          CL_OpenAVIForWriting(afd, afd->givenFileName, qtrue, afd->avi, afd->noSoundAvi, afd->wav, afd->tga, afd->jpg, afd->depth, afd->split, afd->left);
          return qtrue;
      } else {
          //FIXME
          //Com_Printf("^3max size\n");
          CL_NewRiff(afd);
          return qtrue;
          //return qfalse;
      }
  }

  return qfalse;
}

/*
===============
CL_WriteAVIVideoFrame
===============
*/
static void CL_WriteAVIVideoFrameReal (aviFileData_t *afd, const byte *imageBuffer, int size)
{
  int   chunkOffset = afd->riffSize - afd->moviOffset - 8;
  int   chunkSize = 8 + size;
  int   paddingSize = PAD( size, 2 ) - size;
  byte  padding[ 4 ] = { 0 };
  int64_t currentFileSize;

  if( !afd->fileOpen ) {
      Com_Printf("^1CL_WriteAVIVideoFrame() file not open\n");
      return;
  }

  if (!afd->avi) {
      return;
  }

  // Chunk header + contents + padding
  if (CL_CheckRiffSize(afd, 8 + size + 2)) {
      //Com_Printf("video frame would exceed size\n");
      //return;  //FIXME why would you return??
      //return;

  }

  chunkOffset = afd->riffSize - afd->moviOffset - 8;

  fseeko(afd->file, 0, SEEK_END);

  currentFileSize = ftello(afd->file);

  bufIndex = 0;
  WRITE_STRING( "00dc" );
  WRITE_4BYTES( size );

  SafeFS_Write( buffer, 8, afd->f );
  SafeFS_Write( imageBuffer, size, afd->f );
  SafeFS_Write( padding, paddingSize, afd->f );
  //CL_PadEndOfFile(afd);

  afd->riffSize += ( chunkSize + paddingSize );
  //afd->odmlFileSize += (chunkSize + paddingSize);

  afd->odmlNumVideoFrames++;  // total

  if (afd->riffCount == 1) {
      afd->numVideoFrames++;  // riff 1
      afd->moviSize += ( chunkSize + paddingSize );
  }

  if( size > afd->maxRecordSize)  //  &&  afd->riffCount == 1)
    afd->maxRecordSize = size;

  if (size != afd->maxRecordSize  &&  afd->codec == CODEC_UNCOMPRESSED) {
      Com_Printf("^1size != afd->maxRecordSize  %d  %d\n", size, afd->maxRecordSize);
  }

  if (afd->riffCount == 1) {
      // Index
      bufIndex = 0;
      WRITE_STRING( "00dc" );           //dwIdentifier
      WRITE_4BYTES( 0x00000010 );       //dwFlags (all frames are KeyFrames)
      WRITE_4BYTES( chunkOffset );      //dwOffset
      WRITE_4BYTES( size );             //dwLength
      SafeFS_Write( buffer, 16, afd->idxF );

      afd->numIndices++;
  }

  //fwrite4(newChunkOffset, afd->idxVF);  //FIXME + 8
  fwrite4((int)((currentFileSize + 8) - afd->moviDataOffset), afd->idxVF);
  //fwrite4((size + paddingSize) | 0x80000000L, afd->idxVF);  // 0x80000000 == key frame
  //fwrite4((size ) | 0x80000000L, afd->idxVF);  // 0x80000000 == key frame
  fwrite4((size ), afd->idxVF);  // 0x80000000 == key frame

  afd->numVIndices++;
}

//FIXME
static byte *EncodeBuffer = NULL;
static int EncodeBufferSize = 0;

void CL_WriteAVIVideoFrame (aviFileData_t *afd, const byte *imageBuffer, int size)
{
    int newSize;
    byte *newBuffer;
    AVFrame VlcFrame;
    int bufSize;

    //FIXME
    bufSize = afd->width * afd->height * 4 * 2;
    if (!EncodeBuffer) {
        EncodeBuffer = malloc(bufSize);
        if (!EncodeBuffer) {
            Com_Error(ERR_DROP, "%s couldn't allocate encode buffer", __FUNCTION__);
        }
        EncodeBufferSize = bufSize;
    }

    if (bufSize > EncodeBufferSize) {
        EncodeBuffer = realloc(EncodeBuffer, bufSize);
        if (!EncodeBuffer) {
            Com_Error(ERR_DROP, "%s couldn't reallocate encode buffer", __FUNCTION__);
        }
        EncodeBufferSize = bufSize;
    }

    if (afd->codec == CODEC_HUFFYUV) {
        int i;
        byte *p;

        p = (byte *)imageBuffer;

        //FIXME change codec so you don't have to swap bgr
        for (i = 0;  i < size;  i += 3) {
            int r, b;

            r = p[i + 0];
            b = p[i + 2];

            p[i + 0] = b;
            p[i + 2] = r;
        }

        VlcFrame.data[0] = (uint8_t *)imageBuffer;
        VlcFrame.linesize[0] = afd->width * 3;

        newSize = huffyuv_encode_frame(afd->AC, EncodeBuffer, EncodeBufferSize, &VlcFrame);
        newBuffer = EncodeBuffer;
        //Com_Printf("size  %d  ->  %d\n", size, newSize);
    } else {
        newSize = size;
        newBuffer = (byte *)imageBuffer;
    }

    CL_WriteAVIVideoFrameReal(afd, newBuffer, newSize);
}

/*
===============
CL_WriteAVIAudioFrame
===============
*/


void CL_WriteAVIAudioFrame (aviFileData_t *afd, const byte *pcmBuffer, int size)
{
  //static byte pcmCaptureBuffer[ PCM_BUFFER_SIZE ] = { 0 };
  int bytesInBuffer;

  bytesInBuffer = afd->PcmBytesInBuffer;

  if( !afd->audio  &&  !afd->noSoundAvi)
    return;

  if (!afd->wav  &&  !afd->avi) {
      return;
  }

  if( !afd->fileOpen ) {
      Com_Printf("^1CL_WriteAVIAudioFrame() file not open\n");
      return;
  }

  //Com_Printf("bytesInBuffer %d\n", bytesInBuffer);

  if( bytesInBuffer + size > PCM_BUFFER_SIZE )
  {
    Com_Printf( S_COLOR_YELLOW
                "WARNING: Audio capture buffer overflow -- truncating  size: %d  bytesInBuffer %d\n", size, bytesInBuffer );
    size = PCM_BUFFER_SIZE - bytesInBuffer;
  }

  Com_Memcpy( &afd->pcmCaptureBuffer[ bytesInBuffer ], pcmBuffer, size );
  bytesInBuffer += size;

  if (bytesInBuffer >= (int)ceil( (float)afd->a.rate / (float)afd->frameRate ) * afd->a.sampleSize) {

      if (afd->wav) {
          SafeFS_Write(afd->pcmCaptureBuffer, bytesInBuffer, afd->wavFile);
          if (!afd->avi  ||  afd->noSoundAvi) {
              afd->PcmBytesInBuffer = 0;
              //Com_Printf("zero\n");
              return;
          }
      }

      if (!afd->avi  ||  afd->noSoundAvi) {
          //Com_Printf("no avi\n");
          goto done;
      }
      // Chunk header + contents + padding
      if (CL_CheckRiffSize(afd, 8 + bytesInBuffer + size + 2)) {
          //Com_Printf("audio frame would exceed size\n");
      }
  }

  // Only write if we have a frame's worth of audio
  if (bytesInBuffer >= (int)ceil( (float)afd->a.rate / (float)afd->frameRate ) * afd->a.sampleSize) {
    int   chunkOffset = afd->riffSize - afd->moviOffset - 8;
    int   chunkSize = 8 + bytesInBuffer;
    int   paddingSize = PAD( bytesInBuffer, 2 ) - bytesInBuffer;
    byte  padding[ 4 ] = { 0 };
    int64_t currentFileSize;


    fseeko(afd->file, 0, SEEK_END);
    currentFileSize = ftello(afd->file);

    bufIndex = 0;
    WRITE_STRING( "01wb" );
    WRITE_4BYTES( bytesInBuffer );

    SafeFS_Write( buffer, 8, afd->f );
    SafeFS_Write( afd->pcmCaptureBuffer, bytesInBuffer, afd->f );
    SafeFS_Write( padding, paddingSize, afd->f );
    //CL_PadEndOfFile(afd);

    afd->riffSize += ( chunkSize + paddingSize );

    afd->moviSize += ( chunkSize + paddingSize );
    afd->a.totalBytes += bytesInBuffer;

    if (afd->riffCount == 1) {
        afd->numAudioFrames++;

        // Index
        bufIndex = 0;
        WRITE_STRING( "01wb" );           //dwIdentifier
        WRITE_4BYTES( 0 );                //dwFlags
        WRITE_4BYTES( chunkOffset );      //dwOffset
        WRITE_4BYTES( bytesInBuffer );    //dwLength
        SafeFS_Write( buffer, 16, afd->idxF );

        afd->numIndices++;
    }

    //fwrite4(newChunkOffset, afd->idxAF);  //FIXME + 8
    fwrite4((currentFileSize + 8) - afd->moviDataOffset, afd->idxAF);
    //fwrite4(bytesInBuffer | 0x80000000L, afd->idxAF);  // 0x80000000 == key frame
    fwrite4(bytesInBuffer, afd->idxAF);

    afd->numAIndices++;
    afd->audioSize += bytesInBuffer;
    afd->odmlNumAudioFrames++;

    bytesInBuffer = 0;
  }

 done:
  afd->PcmBytesInBuffer = bytesInBuffer;
}

/*
===============
CL_TakeVideoFrame
===============
*/
void CL_TakeVideoFrame (aviFileData_t *afd)
{
    //FIXME afd->fileOpen should now be afd->recording
    if( !afd->fileOpen ) {
        Com_Printf("^1CL_TakeVideoFrame() file not open\n");
        return;
    }

    if (!afd->avi  &&  !afd->tga  &&  !afd->jpg) {
        return;
    }

    re.TakeVideoFrame(afd, afd->width, afd->height, afd->cBuffer, afd->eBuffer, afd->codec == CODEC_MJPEG, afd->avi, afd->tga, afd->jpg, afd->picCount, afd->givenFileName);

    afd->picCount++;
}

static void CL_PadEndOfFile (const aviFileData_t *afd)
{
    int paddingSize;
    int i;
    int64_t pos;

    fseeko(afd->file, 0, SEEK_END);
    pos = ftello(afd->file);

    paddingSize = ((PAD( pos, 2 ))) - pos;
    fseeko(afd->file, 0, SEEK_END);
    for (i = 0;  i < paddingSize;  i++) {
        fwrite1(0, afd->f);
    }
    if (paddingSize) {
        //Com_Printf("pad %d\n", paddingSize);
    }
}

static void CL_PadEndOfFileExt (const aviFileData_t *afd, int pad)
{
    int paddingSize;
    int i;
    int64_t pos;

    fseeko(afd->file, 0, SEEK_END);
    pos = ftello(afd->file);

    paddingSize = PAD(pos, pad) - pos;
    if (paddingSize < 12) {
        for (i = 0;  i < paddingSize;  i++) {
            fwrite1(0, afd->f);
        }
    } else {
        fwriteString("JUNK", afd->f);
        paddingSize -= 4;
        fwrite4(paddingSize - 4, afd->f);
        paddingSize -= 4;
        memset(buffer, 0, paddingSize);
        SafeFS_Write(buffer, paddingSize, afd->f);
    }

    //Com_Printf("pad extended %d\n", paddingSize);
    //FIXME junk list, but what if paddingSize < "JUNK"
}

static void CL_WriteIndexes (aviFileData_t *afd)
{
  int indexRemainder;
  int indexSize;  // = afd->numIndices * 16;
  const char *idxFileName;
  const char *idxVFileName;
  const char *idxAFileName;
  int64_t pos;
  int64_t indexPos;
  int r;

  if( !afd->fileOpen ) {
      Com_Printf("^1CL_WriteIndexes() file not open\n");
      return;
  }

  //Com_Printf("CL_WriteIndexes %lld  %lld frames\n", ftello(afd->file), afd->odmlNumVideoFrames);

  // we are still in a movi list

  if (afd->useOpenDml) {
      /////////////////////    video index    //////////////////////////

      pos = FS_FTell(afd->idxVF);
      FS_Seek(afd->idxVF, 4, FS_SEEK_SET);
      fwrite4(24 + afd->numVIndices * 8, afd->idxVF);
      FS_Seek(afd->idxVF, 12, FS_SEEK_SET);
      fwrite4(afd->numVIndices, afd->idxVF);  // nEntriesInUse
      FS_Seek(afd->idxVF, 20, FS_SEEK_SET);  //FIXME check qwBaseOffset
      fwrite8(afd->moviDataOffset, afd->idxVF);
      FS_FCloseFile(afd->idxVF);

      idxVFileName = va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT);
      if ((indexSize = FS_FOpenFileRead(idxVFileName, &afd->idxVF, qtrue)) <= 0) {
          Com_Printf("CL_WriteIndexes()  couldn't open video index file\n");
          FS_FCloseFile(afd->f);
          afd->fileOpen = qfalse;
          afd->file = NULL;
          return;
      }
      fseeko(afd->file, 0, SEEK_END);
      indexPos = ftello(afd->file);
      indexRemainder = indexSize;

      // Append index to end of avi file
      while( indexRemainder > MAX_AVI_BUFFER ) {
          FS_Read( buffer, MAX_AVI_BUFFER, afd->idxVF );
          SafeFS_Write( buffer, MAX_AVI_BUFFER, afd->f );
          afd->riffSize += MAX_AVI_BUFFER;
          indexRemainder -= MAX_AVI_BUFFER;
      }
      FS_Read( buffer, indexRemainder, afd->idxVF );
      SafeFS_Write( buffer, indexRemainder, afd->f );
      afd->riffSize += indexRemainder;
      FS_FCloseFile( afd->idxVF );
      CL_PadEndOfFile(afd);  // ??ix are chunks

      afd->numVIndexEntries++;
      pos = ftello(afd->file);
      //pos = ftell64(afd->file);
      if (pos < 0) {
          int e = errno;
          Com_Printf("couldn't get position: %s\n", strerror(e));
      }
      fseeko(afd->file, afd->odmlVideonEntriesInUseHeaderOffset, SEEK_SET);

      fwrite4(afd->numVIndexEntries, afd->f);
      fseeko(afd->file, afd->odmlVideoIndexToIndexesHeaderOffset + (afd->numVIndexEntries - 1) * sizeof(aviSuperIndex_t), SEEK_SET);
      fwrite8(indexPos, afd->f);
      fwrite4(indexSize, afd->f);
      fwrite4(afd->numVIndices, afd->f);  //FIXME duration don't know if it is right
      //fwrite4(0, afd->f);

      //Com_Printf("^2%d [%d] %lld  (%d)  video\n", afd->riffCount, afd->numVIndexEntries, indexPos, indexSize);
      FS_HomeRemove(va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT));
  } else {
      FS_FCloseFile(afd->idxVF);
      FS_HomeRemove(va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT));
  }

  if (afd->useOpenDml  &&  afd->audio) {
      //////////////////////////     audio index    //////////////////////
      //r = fseeko(afd->file, pos, SEEK_SET);
      r = fseeko(afd->file, 0, SEEK_END);
      if (r < 0) {
          int e = errno;
          Com_Printf("%s\n", strerror(e));
      }

      //Com_Printf("seeking to %lld\n", pos);

      pos = FS_FTell(afd->idxAF);
      FS_Seek(afd->idxAF, 4, FS_SEEK_SET);
      fwrite4(24 + afd->numAIndices * 8, afd->idxAF);
      FS_Seek(afd->idxAF, 12, FS_SEEK_SET);
      fwrite4(afd->numAIndices, afd->idxAF);  // nEntriesInUse
      FS_Seek(afd->idxAF, 20, FS_SEEK_SET);  //FIXME check qwBaseOffset
      fwrite8(afd->moviDataOffset, afd->idxAF);
      FS_FCloseFile(afd->idxAF);

      idxAFileName = va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT);
      if ((indexSize = FS_FOpenFileRead(idxAFileName, &afd->idxAF, qtrue)) <= 0) {
          Com_Printf("CL_WriteIndexes()  couldn't open audio index file\n");
          FS_FCloseFile(afd->f);
          afd->fileOpen = qfalse;
          afd->file = NULL;
          return;
      }
      fseeko(afd->file, 0, SEEK_END);
      indexPos = ftello(afd->file);
      //Com_Printf("indexpos %lld\n", indexPos);
      indexRemainder = indexSize;
      // Append index to end of avi file
      while( indexRemainder > MAX_AVI_BUFFER ) {
          FS_Read( buffer, MAX_AVI_BUFFER, afd->idxAF );
          SafeFS_Write( buffer, MAX_AVI_BUFFER, afd->f );
          afd->riffSize += MAX_AVI_BUFFER;
          indexRemainder -= MAX_AVI_BUFFER;
      }
      FS_Read( buffer, indexRemainder, afd->idxAF );
      SafeFS_Write( buffer, indexRemainder, afd->f );
      afd->riffSize += indexRemainder;
      FS_FCloseFile( afd->idxAF );
      CL_PadEndOfFile(afd);  // ??ix are chunks

      afd->numAIndexEntries++;
      pos = ftello(afd->file);
      fseeko(afd->file, afd->odmlAudionEntriesInUseHeaderOffset, SEEK_SET);

      fwrite4(afd->numAIndexEntries, afd->f);
      fseeko(afd->file, afd->odmlAudioIndexToIndexesHeaderOffset + (afd->numAIndexEntries - 1) * sizeof(aviSuperIndex_t), SEEK_SET);
      fwrite8(indexPos, afd->f);
      fwrite4(indexSize, afd->f);
      //fwrite4(afd->numAIndices, afd->f);  //FIXME duration assuming these are "ticks"
      fwrite4(afd->audioSize / 4  /* sample size */, afd->f);
      //fwrite4(0, afd->f);

      //Com_Printf("^2%d [%d] %lld  (%d)  audio\n", afd->riffCount, afd->numAIndexEntries, indexPos, indexSize);

      FS_HomeRemove(va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT));
      fseeko(afd->file, 0, SEEK_END);
  } else {
      FS_FCloseFile(afd->idxAF);
      FS_HomeRemove(va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT));
  }

  if (afd->riffCount == 1) {  //(!afd->useOpenDml  &&  afd->riffCount == 1) {
      // first close the first movi list chunk
      fseeko(afd->file, 0, SEEK_END);
      pos = ftello(afd->file);
      fseeko(afd->file, afd->moviOffset1 + 4, SEEK_SET);
      fwrite4(pos - (afd->moviOffset1 + 4 + 4), afd->f);
      CL_PadEndOfFile(afd);

      indexSize = afd->numIndices * 16;
      FS_Seek( afd->idxF, 4, FS_SEEK_SET );
      bufIndex = 0;
      WRITE_4BYTES( indexSize );
      SafeFS_Write( buffer, bufIndex, afd->idxF );
      FS_FCloseFile( afd->idxF );

      // Write index

      // Open the temp index file
      idxFileName = va("%s%s", afd->fileName, INDEX_FILENAME_EXT);
      //Com_Printf("afd->fileName: '%s'\n", afd->fileName);
      //Com_Printf("idxFilename: '%s'\n", idxFileName);
      if( ( indexSize = FS_FOpenFileRead( idxFileName,
                                          &afd->idxF, qtrue ) ) <= 0 )
          {
              Com_Printf("CL_WriteIndexes()  couldn't open standard index file '%s'  indexSize: %d\n", va("%s%s", afd->fileName, INDEX_FILENAME_EXT), indexSize);
              FS_FCloseFile( afd->f );
              afd->fileOpen = qfalse;
              afd->file = NULL;
              return;
          }
      //Com_Printf("idx size %d\n", indexSize);
      indexRemainder = indexSize;

      // Append index to end of avi file
      while( indexRemainder > MAX_AVI_BUFFER )
          {
              FS_Read( buffer, MAX_AVI_BUFFER, afd->idxF );
              SafeFS_Write( buffer, MAX_AVI_BUFFER, afd->f );
              afd->riffSize += MAX_AVI_BUFFER;
              indexRemainder -= MAX_AVI_BUFFER;
          }
      FS_Read( buffer, indexRemainder, afd->idxF );

      SafeFS_Write( buffer, indexRemainder, afd->f );
      afd->riffSize += indexRemainder;
      FS_FCloseFile( afd->idxF );
      CL_PadEndOfFile(afd);  // idx is chunk

      //  start
      // 0 "RIFF"
      // 4 riff size
      // 8  "AVI "
      // 12  "LIST"
      // 16  list size

      CL_PadEndOfFileExt(afd, 2048);

#if 0  // no :(
      // now close avi list
      fseeko(afd->file, 0, SEEK_END);
      pos = ftell(afd->file);
      fseeko(afd->file, 16, SEEK_SET);
      fwrite4(pos - 20, afd->f);
      //CL_PadEndOfFileExt(2048);
#endif

      // now close first riff
      fseeko(afd->file, 0, SEEK_END);
      pos = ftell(afd->file);
      fseeko(afd->file, 4, SEEK_SET);
      fwrite4(pos - 8, afd->f);
      CL_PadEndOfFile(afd);

      //afd->fileSize1 = afd->fileSize;
      //afd->moviSize1 = afd->moviSize;
      //Com_Printf("riff1 afd->fileSize1 %d  afd->moviSize1 %d\n", afd->fileSize1, afd->moviSize1);
      //Com_Printf("close riff 1\n");

      if (afd->audio) {
          afd->audioDataLength = afd->a.totalBytes / afd->a.sampleSize;  //FIXME check if only for std riff 1 index
      }
      FS_HomeRemove(va("%s%s", afd->fileName, INDEX_FILENAME_EXT));
  }
}

static void CL_CloseRiff (const aviFileData_t *afd)
{
    int64_t sz;
    int64_t pos;

    // riff 1 closed in WriteIndexes()
    if (afd->riffCount > 1) {
    //if (afd->useOpenDml) {  //(afd->riffCount > 1) {
        //Com_Printf("close riff %d\n", afd->riffCount);

        fseeko(afd->file, 0, SEEK_END);
        pos = ftello(afd->file);

        //sz = (pos - afd->newMoviListSizePos);  // - 4;
        sz = pos - (afd->newMoviListSizePos + 4);
        //Com_Printf("    movi  %llx -> %llx    size %llx\n", afd->newMoviListSizePos, pos, sz);
        fseeko(afd->file, afd->newMoviListSizePos, SEEK_SET);
        fwrite4(sz, afd->f);

        CL_PadEndOfFileExt(afd, 2048);


        fseeko(afd->file, 0, SEEK_END);
        pos = ftello(afd->file);
        //sz = (pos - afd->newRiffSizePos);  // - 4;
        sz = pos - (afd->newRiffSizePos + 4);
        //Com_Printf("    riff  %llx -> %llx    size %llx\n", afd->newRiffSizePos, pos, sz);
        fseeko(afd->file, afd->newRiffSizePos, SEEK_SET);
        fwrite4(sz, afd->f);
        CL_PadEndOfFile(afd);

        fseeko(afd->file, 0, SEEK_END);
    }
}

/*
===============
CL_CloseAVI

Closes the AVI file and writes an index chunk
===============
*/
qboolean CL_CloseAVI (aviFileData_t *afd, qboolean us)
{
    //int r;
    int pos;
    //char sbuf[MAX_QPATH];

#if 0
    if( !afd->fileOpen ) {
        Com_Printf("^1CL_CloseAVI() file not open\n");
        return qfalse;
    }
#endif

    if (!afd->recording) {
        //Com_Printf("CL_CloseAVI() not recording\n");
        return qfalse;
    }

  //FIXME need to flush audio maybe

  CL_WriteIndexes(afd);
  CL_CloseRiff(afd);
  //CL_WriteIndexes(afd);

  // finalize header

  //FS_Seek(afd->f, 4, FS_SEEK_SET);  // "RIFF" size
  //fwrite4(afd->fileSize1 - 8, afd->f);

  FS_Seek(afd->f, afd->mainHeaderNumVideoFramesHeaderOffset, FS_SEEK_SET);
  fwrite4(afd->numVideoFrames, afd->f);

  FS_Seek(afd->f, afd->mainHeaderMaxRecordSizeHeaderOffset, FS_SEEK_SET);
  fwrite4(afd->maxRecordSize, afd->f);

  FS_Seek(afd->f, afd->numVideoFramesHeaderOffset, FS_SEEK_SET);
  fwrite4(afd->numVideoFrames, afd->f);

  FS_Seek(afd->f, afd->videoHeaderMaxRecordSizeHeaderOffset, FS_SEEK_SET);
  fwrite4(afd->maxRecordSize, afd->f);

  if (afd->audio) {
      FS_Seek(afd->f, afd->numAudioFramesHeaderOffset, FS_SEEK_SET);
      fwrite4(afd->audioDataLength, afd->f);
  }


  //FS_Seek(afd->f, afd->moviOffset1 + 4, FS_SEEK_SET);  // Skip "LIST"
  //fwrite4(afd->moviSize1, afd->f);


  if (afd->useOpenDml) {
      //fseeko(afd->file, afd->mainHeaderNumVideoFramesHeaderOffset, SEEK_SET);
      //fwrite4(afd->odmlNumVideoFrames, afd->f);

      //fseeko(afd->file, afd->numVideoFramesHeaderOffset, SEEK_SET);
      //fwrite4(afd->odmlNumVideoFrames, afd->f);

      //FIXME  wrong, whatever
      //fseeko(afd->file, afd->numAudioFramesHeaderOffset, SEEK_SET);
      //fwrite4(afd->odmlNumAudioFrames, afd->f);

      FS_Seek(afd->f, afd->odmlDmlhHeaderOffset, FS_SEEK_SET);
      fwrite4(afd->odmlNumVideoFrames, afd->f);
      //Com_Printf("odml:  video %lld  audio  %lld  total %lld\n", afd->odmlNumVideoFrames, afd->odmlNumAudioFrames, afd->odmlNumVideoFrames + afd->odmlNumAudioFrames);
  } else {
      //Com_Printf( "Wrote %d:%d (%d) frames to %s\n", afd->numVideoFrames, afd->numAudioFrames, afd->numVideoFrames + afd->numAudioFrames, afd->fileName );
  }

#if 0
  //FIXME testing
  fseeko(afd->file, 0, SEEK_END);

  fwriteString("JUNK", afd->f);
  fwrite4(16, afd->f);
  fwrite8(0xdeadbeaf, afd->f);
  fwrite8(0xdeadbeaf, afd->f);
#endif

  if (!us) {
      free(afd->cBuffer);
      free(afd->eBuffer);

      if (afd->codec == CODEC_HUFFYUV) {
          huffyuv_encode_end(afd->AC);
          free(afd->AC->priv_data);
          free(afd->AC);
      }
  }

  FS_FCloseFile( afd->f );
  if (!afd->avi) {
      FS_HomeRemove(afd->fileName);
  }
  //FS_FCloseFile(afd->idxF);
  FS_FCloseFile(afd->idxVF);
  FS_FCloseFile(afd->idxAF);
  //FS_HomeRemove(va("%s%s", afd->fileName, INDEX_FILENAME_EXT));
  FS_HomeRemove(va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT));
  FS_HomeRemove(va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT));
  afd->fileOpen = qfalse;
  afd->file = NULL;
  afd->recording = qfalse;

  if (!us) {
      afd->PcmBytesInBuffer = 0;
  }

  //Com_Printf("^2%s closed\n", afd->fileName);

  if (!us) {
      if (afd->wav  &&  afd == &afdMain) {
          pos = FS_FTell(afd->wavFile);
          FS_Seek(afd->wavFile, 4, FS_SEEK_SET);
          fwrite4(pos - 8, afd->wavFile);
          FS_Seek(afd->wavFile, 40, FS_SEEK_SET);
          fwrite4(pos - 44, afd->wavFile);

          FS_FCloseFile(afd->wavFile);
#if 0
          if (!afd->wav) {
              Com_sprintf(sbuf, MAX_QPATH, "videos/%s.wav", afd->givenFileName);
              FS_HomeRemove(sbuf);
          }
#endif
      }
      //Com_Printf("video time: %d -> %d  total %d (%f)\n", afd->startTime, cl.snap.serverTime, cl.snap.serverTime - afd->startTime, (float)(cl.snap.serverTime - afd->startTime) / 1000.0);
      //FIXME wrong -- can rewind while recording
      //Com_Printf("video time: %d -> %d  total %d (%f)\n", afd->startTime, cls.realtime, cls.realtime - afd->startTime, (float)(cls.realtime - afd->startTime) / 1000.0);
  }

  return qtrue;
}

static void CL_NewRiff (aviFileData_t *afd)
{
  //int64_t pos;
  //int64_t newRiffStart;

  if( !afd->fileOpen ) {
      Com_Printf("^1CL_NewRiff() file not open\n");
      return;
  }

  //pos = ftello(afd->file);


  CL_WriteIndexes(afd);
  CL_CloseRiff(afd);
  //CL_WriteIndexes(afd);

  //fseeko(afd->file, pos, SEEK_SET);


  if (afd->riffCount >= MAX_OPENDML_INDEX_ENTRIES * 2) {
      //Com_Printf("MAX_OPENDML_INDEX_ENTRIES * 2 starting new file\n");
      CL_CloseAVI(afd, qtrue);
      //CL_OpenAVIForWriting(va("%s_", afd->fileName), qtrue);
      CL_OpenAVIForWriting(afd, afd->givenFileName, qtrue, afd->avi, afd->noSoundAvi, afd->wav, afd->tga, afd->jpg, afd->depth, afd->split, afd->left);
      return;
  }

  fseeko(afd->file, 0, SEEK_END);

  fwriteString("RIFF", afd->f);
  afd->newRiffSizePos = ftello(afd->file);
  fwrite4(0x1, afd->f);
  fwriteString("AVIX", afd->f);
  afd->moviDataOffset = ftello(afd->file);
  fwriteString("LIST", afd->f);
  afd->newMoviListSizePos = ftello(afd->file);
  fwrite4(0x1, afd->f);
  fwriteString("movi", afd->f);
  //afd->moviDataOffset = ftello(afd->file);

  afd->riffSize = 24;  //FIXME
  afd->moviSize = 4;  //FIXME
  afd->moviOffset = 0;
  //afd->numIndices = 0;  // std
  //Com_Printf( "New Riff(%d) Wrote %d:%d frames to %s\n", afd->riffCount, afd->numVIndices, afd->numAIndices, afd->fileName );
  //afd->numVideoFrames = 0;  // std
  //afd->numAudioFrames = 0;  // std
  memset(afd->chunkStack, 0, sizeof(afd->chunkStack));
  afd->chunkStackTop = 0;

  afd->numVIndices = 0;
  afd->numAIndices = 0;
  afd->audioSize = 0;

  afd->riffCount++;

  if( ( afd->idxVF = FS_FOpenFileWrite(va("%s%s", afd->fileName, INDEX_VIDEO_FILENAME_EXT))) <= 0)
  {
      Com_Printf("CL_NewRiff() couldn't open video index file\n");
      FS_FCloseFile( afd->f );
      afd->fileOpen = qfalse;
      afd->file = NULL;
      return;
  }

  if( ( afd->idxAF = FS_FOpenFileWrite(va("%s%s", afd->fileName, INDEX_AUDIO_FILENAME_EXT))) <= 0)
  {
      Com_Printf("CL_NewRiff() couldn't open audio index file\n");
      FS_FCloseFile( afd->f );
      afd->fileOpen = qfalse;
      afd->file = NULL;
      return;
  }

  CL_InitIndexes(afd);
}

/*
===============
CL_VideoRecording
===============
*/
qboolean CL_VideoRecording (const aviFileData_t *afd)
{
    return afd->recording;
}

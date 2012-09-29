#ifndef cl_avi_h_included
#define cl_avi_h_included

#include "../qcommon/q_shared.h"

#define MAX_RIFF_CHUNKS 16

#define MAX_OPENDML_INDEX_ENTRIES 256  //testing 256  //FIXME everyone seems to have 256, maybe max riff size?

#define PCM_BUFFER_SIZE 44100

typedef struct aviSuperIndex_s {
    int64_t offset;  // absolute file offset
    int size;  // size of index chunk
    int duration;  // time span in stream ticks
} aviSuperIndex_t;

typedef struct audioFormat_s
{
  int rate;
  int format;
  int channels;
  int bits;

  int sampleSize;
  int totalBytes;
} audioFormat_t;

typedef struct aviFileData_s
{
    qboolean recording;
    qboolean avi;
    qboolean wav;
    qboolean tga;
    qboolean jpg;
    qboolean noSoundAvi;
    qboolean depth;
    qboolean split;
    qboolean left;

    int startTime;

    int picCount;
    int vidFileCount;

  qboolean      fileOpen;
  fileHandle_t  f;
    FILE *file;
  char          fileName[ MAX_QPATH ];
    char givenFileName[MAX_QPATH];

  int64_t           riffSize;

    //byte vidFourcc[4];
    //int fileSize1;
  int64_t           moviOffset;
    int64_t moviOffset1;
    int64_t moviDataOffset;
  int           moviSize;
    //int moviSize1;

  fileHandle_t  idxF;
    fileHandle_t idxVF;
    fileHandle_t idxAF;
    int numVIndices;
    int numAIndices;
    int64_t audioSize;
  int64_t           numIndices;

  int           frameRate;
  int           framePeriod;
  int           width, height;
    int           numVideoFrames;  // just for first riff
    int64_t     numVideoFramesHeaderOffset;

  int           maxRecordSize;
  qboolean      motionJpeg;

  qboolean      audio;
  audioFormat_t a;
    int           numAudioFrames;  // just for first riff
    int64_t numAudioFramesHeaderOffset;
    int audioDataLength;

    int64_t mainHeaderNumVideoFramesHeaderOffset;
    int64_t mainHeaderMaxRecordSizeHeaderOffset;
    int64_t videoHeaderMaxRecordSizeHeaderOffset;

  int           chunkStack[ MAX_RIFF_CHUNKS ];
  int           chunkStackTop;

  byte          *cBuffer, *eBuffer;

    qboolean useOpenDml;

    int64_t odmlVideonEntriesInUseHeaderOffset;
    int64_t odmlVideoIndexToIndexesHeaderOffset;
    int64_t odmlAudionEntriesInUseHeaderOffset;
    int64_t odmlAudioIndexToIndexesHeaderOffset;

    int odmlDmlhHeaderOffset;

    int64_t newRiffOrCloseFileSize;
    int riffCount;
    fileHandle_t indxF;
    int64_t odmlNumVideoFrames;  // real total
    int64_t odmlNumAudioFrames;
    //int64_t odmlFileSize;  // real file size
    aviSuperIndex_t vIndex[MAX_OPENDML_INDEX_ENTRIES * 2];
    aviSuperIndex_t aIndex[MAX_OPENDML_INDEX_ENTRIES * 2];
    int numVIndexEntries;
    int numAIndexEntries;
    int64_t newRiffSizePos;
    int64_t newMoviListSizePos;

    // testing
    fileHandle_t wavFile;

    int PcmBytesInBuffer;
    byte pcmCaptureBuffer[PCM_BUFFER_SIZE];

} aviFileData_t;

extern aviFileData_t afdMain;
extern aviFileData_t afdLeft;
extern aviFileData_t afdRight;

extern aviFileData_t afdDepth;
extern aviFileData_t afdDepthLeft;
extern aviFileData_t afdDepthRight;

qboolean CL_VideoRecording (aviFileData_t *afd);

#endif  // cl_avi_h_included

#ifndef ffmpegcompat_h_included
#define ffmpegcompat_h_included

#include "../qcommon/q_shared.h"  // Com_Printf
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#define AV_LOG_ERROR 16
#define CODEC_FLAG_PASS1 0x0200
#define CODEC_FLAG_PASS2 0x0400
#define CODEC_FLAG2_NO_OUTPUT 0x00000004
#define CODEC_FLAG_INTERLACED_ME 0x20000000

#define av_cold

#define AV_PIX_FMT_YUV420P 0
#define AV_PIX_FMT_RGB24 2
#define AV_PIX_FMT_YUV422P 4
#define AV_PIX_FMT_RGB32 1000  //FIXME bleh

#define AV_CODEC_ID_HUFFYUV 1

#define AV_NUM_DATA_POINTERS 8
#define VLC_BITS 11

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define av_assert0 assert
#define av_assert1 assert

#define av_log(context, error_level, format, args...)  Com_Printf("^3" format, ## args)
#define av_mallocz(x) calloc(x, 1)
#define av_malloc(x) malloc(x)
#define av_freep(x) free(*x)
#define AVERROR(x) (x)
//#define emms_c  //FIXME

typedef struct {

} VLC;

typedef struct {
    uint8_t *data[AV_NUM_DATA_POINTERS];
    int linesize[AV_NUM_DATA_POINTERS];
} AVFrame;

typedef struct {

} GetBitContext;

typedef struct {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
    int size_in_bits;
} PutBitContext;

typedef struct {
    void (*diff_bytes) (uint8_t *dst, const uint8_t *src1, const uint8_t *src2,
                        int w);
} DSPContext;

typedef struct {
    int id;
} AVCodec;

typedef struct {

} AVPacket;

typedef struct {
    void *priv_data;
    uint8_t *extradata;
    int extradata_size;

    int width;
    int height;
    int bits_per_coded_sample;
    int prediction_method;
    int context_model;
    int flags;
    int flags2;

    AVCodec *codec;
    AVPacket *pkt;

    char *stats_out;
    char *stats_in;

    AVFrame *coded_frame;
    int pix_fmt;

} AVCodecContext;

typedef enum Predictor{
    LEFT= 0,
    PLANE,
    MEDIAN,
} Predictor;

typedef struct HYuvContext{
    AVCodecContext *avctx;
    Predictor predictor;
    GetBitContext gb;
    PutBitContext pb;
    int interlaced;
    int decorrelate;
    int bitstream_bpp;
    int version;
    int yuy2;                               //use yuy2 instead of 422P
    int bgr32;                              //use bgr32 instead of bgr24
    int width, height;
    int flags;
    int context;
    int picture_number;
    int last_slice_end;
    uint8_t *temp[3];
    uint64_t stats[3][256];
    uint8_t len[3][256];
    uint32_t bits[3][256];
    uint32_t pix_bgr_map[1<<VLC_BITS];
    //VLC vlc[6];                             //Y,U,V,YY,YU,YV
    AVFrame picture;
    uint8_t *bitstream_buffer;
    unsigned int bitstream_buffer_size;
    //DSPContext dsp;
} HYuvContext;


//void ff_dsputil_init(DSPContext* c, AVCodecContext *avctx);

int huffyuv_encode_end (AVCodecContext *avctx);
int huffyuv_encode_init (AVCodecContext *avctx);
int huffyuv_encode_frame (AVCodecContext *avctx, unsigned char *buf, int buf_size, void *data);

#endif  // ffmpegcompat_h_included

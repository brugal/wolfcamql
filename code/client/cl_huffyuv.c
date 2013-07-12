/* from ffmpeg */

#include "ffmpegcompat.h"

#if 0  //HAVE_BIGENDIAN
#define B 3
#define G 2
#define R 1
#define A 0
#else
#define B 0
#define G 1
#define R 2
#define A 3
#endif


static inline void init_put_bits(PutBitContext *s, uint8_t *buffer, int buffer_size)
{
    if(buffer_size < 0) {
        buffer_size = 0;
        buffer = NULL;
    }

    s->size_in_bits= 8*buffer_size;
    s->buf = buffer;
    s->buf_end = s->buf + buffer_size;
    s->buf_ptr = s->buf;
    s->bit_left=32;
    s->bit_buf=0;
}

static inline int put_bits_count(PutBitContext *s)
{
    return (s->buf_ptr - s->buf) * 8 + 32 - s->bit_left;
}


//FIXME
//#define BITSTREAM_WRITER_LE 1

/**
 * Pad the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s)
{
#ifndef BITSTREAM_WRITER_LE
    if (s->bit_left < 32)
        s->bit_buf<<= s->bit_left;
#endif
    while (s->bit_left < 32) {
        /* XXX: should test end of buffer */
#ifdef BITSTREAM_WRITER_LE
        *s->buf_ptr++=s->bit_buf;
        s->bit_buf>>=8;
#else
        *s->buf_ptr++=s->bit_buf >> 24;
        s->bit_buf<<=8;
#endif
        s->bit_left+=8;
    }
    s->bit_left=32;
    s->bit_buf=0;
}

//FIXME not sure
#ifndef AV_WL32
#   define AV_WL32(p, d) do {                   \
    ((uint8_t*)(p))[0] = (d);               \
    ((uint8_t*)(p))[1] = (d)>>8;            \
    ((uint8_t*)(p))[2] = (d)>>16;           \
    ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_WB32
#   define AV_WB32(p, darg) do {                \
    unsigned d = (darg);                    \
    ((uint8_t*)(p))[3] = (d);               \
    ((uint8_t*)(p))[2] = (d)>>8;            \
    ((uint8_t*)(p))[1] = (d)>>16;           \
    ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

/**
 * Write up to 31 bits into a bitstream.
 * Use put_bits32 to write 32 bits.
 */
static inline void put_bits(PutBitContext *s, int n, unsigned int value)
{
    unsigned int bit_buf;
    int bit_left;

    //    printf("put_bits=%d %x\n", n, value);
    assert(n <= 31 && value < (1U << n));

    bit_buf = s->bit_buf;
    bit_left = s->bit_left;
    //    printf("n=%d value=%x cnt=%d buf=%x\n", n, value, bit_cnt, bit_buf);
    /* XXX: optimize */
#ifdef BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
        AV_WL32(s->buf_ptr, bit_buf);
        s->buf_ptr+=4;
        bit_buf = (bit_left==32)?0:value >> bit_left;
        bit_left+=32;
    }
    bit_left-=n;
#else
    if (n < bit_left) {
        bit_buf = (bit_buf<<n) | value;
        bit_left-=n;
    } else {
        bit_buf<<=bit_left;
        bit_buf |= value >> (n - bit_left);
        AV_WB32(s->buf_ptr, bit_buf);
        //printf("bitbuf = %08x\n", bit_buf);
        s->buf_ptr+=4;
        bit_left+=32 - n;
        bit_buf = value;
    }
#endif

    s->bit_buf = bit_buf;
    s->bit_left = bit_left;
}

#define AV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define AV_BSWAP32C(x) (AV_BSWAP16C(x) << 16 | AV_BSWAP16C((x) >> 16))
#define av_bswap32 AV_BSWAP32C

static void bswap_buf(uint32_t *dst, const uint32_t *src, int w){
    int i;

    for(i=0; i+8<=w; i+=8){
        dst[i+0]= av_bswap32(src[i+0]);
        dst[i+1]= av_bswap32(src[i+1]);
        dst[i+2]= av_bswap32(src[i+2]);
        dst[i+3]= av_bswap32(src[i+3]);
        dst[i+4]= av_bswap32(src[i+4]);
        dst[i+5]= av_bswap32(src[i+5]);
        dst[i+6]= av_bswap32(src[i+6]);
        dst[i+7]= av_bswap32(src[i+7]);
    }
    for(;i<w; i++){
        dst[i+0]= av_bswap32(src[i+0]);
    }
}

#define pb_7f (~0UL/255 * 0x7f)
#define pb_80 (~0UL/255 * 0x80)

static void diff_bytes (uint8_t *dst, uint8_t *src1, uint8_t *src2, int w){
    long i;
#if 0  //!HAVE_FAST_UNALIGNED
    if((long)src2 & (sizeof(long)-1)){
        for(i=0; i+7<w; i+=8){
            dst[i+0] = src1[i+0]-src2[i+0];
            dst[i+1] = src1[i+1]-src2[i+1];
            dst[i+2] = src1[i+2]-src2[i+2];
            dst[i+3] = src1[i+3]-src2[i+3];
            dst[i+4] = src1[i+4]-src2[i+4];
            dst[i+5] = src1[i+5]-src2[i+5];
            dst[i+6] = src1[i+6]-src2[i+6];
            dst[i+7] = src1[i+7]-src2[i+7];
        }
    }else
#endif
        for(i=0; i<=w-sizeof(long); i+=sizeof(long)){
            long a = *(long*)(src1+i);
            long b = *(long*)(src2+i);
            *(long*)(dst+i) = ((a|pb_80) - (b&pb_7f)) ^ ((a^b^pb_80)&pb_80);
        }
    for(; i<w; i++)
        dst[i+0] = src1[i+0]-src2[i+0];
}

////////////////////////////////////

#if 0
static inline void sub_left_prediction_bgr32(HYuvContext *s, uint8_t *dst, uint8_t *src, int w, int *red, int *green, int *blue){
    int i;
    int r,g,b;
    r= *red;
    g= *green;
    b= *blue;
    for(i=0; i<FFMIN(w,4); i++){
        const int rt= src[i*4+R];
        const int gt= src[i*4+G];
        const int bt= src[i*4+B];
        dst[i*4+R]= rt - r;
        dst[i*4+G]= gt - g;
        dst[i*4+B]= bt - b;
        r = rt;
        g = gt;
        b = bt;
    }
    //s->dsp.diff_bytes(dst+16, src+16, src+12, w*4-16);
    diff_bytes(dst+16, src+16, src+12, w*4-16);
    *red=   src[(w-1)*4+R];
    *green= src[(w-1)*4+G];
    *blue=  src[(w-1)*4+B];
}

#endif

static inline void sub_left_prediction_rgb24(const HYuvContext *s, uint8_t *dst, const uint8_t *src, int w, int *red, int *green, int *blue){
    int i;
    int r,g,b;
    r = *red;
    g = *green;
    b = *blue;
    for (i = 0; i < FFMIN(w,16); i++) {
        const int rt = src[i*3 + 0];
        const int gt = src[i*3 + 1];
        const int bt = src[i*3 + 2];
        dst[i*3 + 0] = rt - r;
        dst[i*3 + 1] = gt - g;
        dst[i*3 + 2] = bt - b;
        r = rt;
        g = gt;
        b = bt;
    }

    //s->dsp.diff_bytes(dst + 48, src + 48, src + 48 - 3, w*3 - 48);
    diff_bytes(dst + 48, (uint8_t *)src + 48, (uint8_t *)src + 48 - 3, w*3 - 48);

    *red   = src[(w - 1)*3 + 0];
    *green = src[(w - 1)*3 + 1];
    *blue  = src[(w - 1)*3 + 2];
}

static int generate_bits_table(uint32_t *dst, const uint8_t *len_table){
    int len, index;
    uint32_t bits=0;

    for(len=32; len>0; len--){
        for(index=0; index<256; index++){
            if(len_table[index]==len)
                dst[index]= bits++;
        }
        if(bits & 1){
            //av_log(NULL, AV_LOG_ERROR, "Error generating huffman table\n");
            Com_Error(ERR_DROP, "generate_bits_table() error generating huffman table");
            return -1;
        }
        bits >>= 1;
    }
    return 0;
}

typedef struct {
    uint64_t val;
    int name;
} HeapElem;

static void heap_sift(HeapElem *h, int root, int size)
{
    while(root*2+1 < size) {
        int child = root*2+1;
        if(child < size-1 && h[child].val > h[child+1].val)
            child++;
        if(h[root].val > h[child].val) {
            FFSWAP(HeapElem, h[root], h[child]);
            root = child;
        } else
            break;
    }
}

static void generate_len_table(uint8_t *dst, const uint64_t *stats){
    HeapElem h[256];
    int up[2*256];
    int len[2*256];
    int offset, i, next;
    int size = 256;

    for(offset=1; ; offset<<=1){
        for(i=0; i<size; i++){
            h[i].name = i;
            h[i].val = (stats[i] << 8) + offset;
        }
        for(i=size/2-1; i>=0; i--)
            heap_sift(h, i, size);

        for(next=size; next<size*2-1; next++){
            // merge the two smallest entries, and put it back in the heap
            uint64_t min1v = h[0].val;
            up[h[0].name] = next;
            h[0].val = INT64_MAX;
            heap_sift(h, 0, size);
            up[h[0].name] = next;
            h[0].name = next;
            h[0].val += min1v;
            heap_sift(h, 0, size);
        }

        len[2*size-2] = 0;
        for(i=2*size-3; i>=size; i--)
            len[i] = len[up[i]] + 1;
        for(i=0; i<size; i++) {
            dst[i] = len[up[i]] + 1;
            if(dst[i] >= 32) break;
        }
        if(i==size) break;
    }
}


static av_cold void alloc_temp(HYuvContext *s){
    int i;

    if(s->bitstream_bpp<24){
        for(i=0; i<3; i++){
            s->temp[i]= av_malloc(s->width + 16);
        }
    }else{
        s->temp[0]= av_mallocz(4*s->width + 16);
    }
}

static av_cold int common_init(AVCodecContext *avctx){
    HYuvContext *s = avctx->priv_data;

    s->avctx= avctx;
    s->flags= avctx->flags;

    //dsputil_init(&s->dsp, avctx);

    s->width= avctx->width;
    s->height= avctx->height;
    assert(s->width>0 && s->height>0);

    return 0;
}


static int store_table(HYuvContext *s, const uint8_t *len, uint8_t *buf){
    int i;
    int index= 0;

    for(i=0; i<256;){
        int val= len[i];
        int repeat=0;

        for(; i<256 && len[i]==val && repeat<255; i++)
            repeat++;

        assert(val < 32 && val >0 && repeat<256 && repeat>0);
        if(repeat>7){
            buf[index++]= val;
            buf[index++]= repeat;
        }else{
            buf[index++]= val | (repeat<<5);
        }
    }

    return index;
}

int huffyuv_encode_init (AVCodecContext *avctx)
{
    HYuvContext *s = avctx->priv_data;
    int i, j;

    common_init(avctx);

    avctx->extradata= av_mallocz(1024*30); // 256*3+4 == 772
    avctx->stats_out= av_mallocz(1024*30); // 21*256*3(%llu ) + 3(\n) + 1(0) = 16132
    s->version=2;

    //avctx->coded_frame= &s->picture;

    s->bitstream_bpp = 24;

    avctx->bits_per_coded_sample= s->bitstream_bpp;
    s->decorrelate= s->bitstream_bpp >= 24;
    s->predictor= 0;  //avctx->prediction_method;
    s->interlaced= avctx->flags&CODEC_FLAG_INTERLACED_ME ? 1 : 0;
    if(avctx->context_model==1){
        s->context= avctx->context_model;
        if(s->flags & (CODEC_FLAG_PASS1|CODEC_FLAG_PASS2)){
            av_log(avctx, AV_LOG_ERROR, "context=1 is not compatible with 2 pass huffyuv encoding\n");
            return -1;
        }
    }else s->context= 0;

    if (1) {  //(avctx->codec->id==CODEC_ID_HUFFYUV){
        if (0) {  //(avctx->pix_fmt==PIX_FMT_YUV420P){
            av_log(avctx, AV_LOG_ERROR, "Error: YV12 is not supported by huffyuv; use vcodec=ffvhuff or format=422p\n");
            return -1;
        }
        if(avctx->context_model){
            av_log(avctx, AV_LOG_ERROR, "Error: per-frame huffman tables are not supported by huffyuv; use vcodec=ffvhuff\n");
            return -1;
        }
        if(s->interlaced != ( s->height > 288 )) {
            //av_log(avctx, AV_LOG_INFO, "using huffyuv 2.2.0 or newer interlacing flag\n");
        }
    }

    if(s->bitstream_bpp>=24 && s->predictor==MEDIAN){
        av_log(avctx, AV_LOG_ERROR, "Error: RGB is incompatible with median predictor\n");
        return -1;
    }

    ((uint8_t*)avctx->extradata)[0]= s->predictor | (s->decorrelate << 6);
    ((uint8_t*)avctx->extradata)[1]= s->bitstream_bpp;
    ((uint8_t*)avctx->extradata)[2]= s->interlaced ? 0x10 : 0x20;
    if(s->context)
        ((uint8_t*)avctx->extradata)[2]|= 0x40;
    ((uint8_t*)avctx->extradata)[3]= 0;
    s->avctx->extradata_size= 4;

    if(avctx->stats_in){
        char *p= avctx->stats_in;

        for(i=0; i<3; i++)
            for(j=0; j<256; j++)
                s->stats[i][j]= 1;

        for(;;){
            for(i=0; i<3; i++){
                char *next;

                for(j=0; j<256; j++){
                    s->stats[i][j]+= strtol(p, &next, 0);
                    if(next==p) return -1;
                    p=next;
                }
            }
            if(p[0]==0 || p[1]==0 || p[2]==0) break;
        }
    }else{
        for(i=0; i<3; i++)
            for(j=0; j<256; j++){
                int d= FFMIN(j, 256-j);

                s->stats[i][j]= 100000000/(d+1);
            }
    }

    for(i=0; i<3; i++){
        generate_len_table(s->len[i], s->stats[i]);

        if(generate_bits_table(s->bits[i], s->len[i])<0){
            return -1;
        }

        s->avctx->extradata_size+=
        store_table(s, s->len[i], &((uint8_t*)s->avctx->extradata)[s->avctx->extradata_size]);
    }

    if(s->context){
        for(i=0; i<3; i++){
            int pels = s->width*s->height / (i?40:10);
            for(j=0; j<256; j++){
                int d= FFMIN(j, 256-j);
                s->stats[i][j]= pels/(d+1);
            }
        }
    }else{
        for(i=0; i<3; i++)
            for(j=0; j<256; j++)
                s->stats[i][j]= 0;
    }

//    printf("pred:%d bpp:%d hbpp:%d il:%d\n", s->predictor, s->bitstream_bpp, avctx->bits_per_coded_sample, s->interlaced);

    alloc_temp(s);

    s->picture_number=0;

    return 0;
}

#if 0
static int encode_bgr_bitstream(HYuvContext *s, int count){
    int i;

    if(s->pb.buf_end - s->pb.buf - (put_bits_count(&s->pb)>>3) < 3*4*count){
        //av_log(s->avctx, AV_LOG_ERROR, "encoded frame too large\n");
        Com_Printf("^1encode_bgr_bitstream() encoded frame too large\n");
        return -1;
    }

#define LOAD3\
            int g= s->temp[0][4*i+G];\
            int b= (s->temp[0][4*i+B] - g) & 0xff;\
            int r= (s->temp[0][4*i+R] - g) & 0xff;
#define STAT3\
            s->stats[0][b]++;\
            s->stats[1][g]++;\
            s->stats[2][r]++;
#define WRITE3\
            put_bits(&s->pb, s->len[1][g], s->bits[1][g]);\
            put_bits(&s->pb, s->len[0][b], s->bits[0][b]);\
            put_bits(&s->pb, s->len[2][r], s->bits[2][r]);

    if (0) {  //((s->flags&CODEC_FLAG_PASS1) && (s->avctx->flags2&CODEC_FLAG2_NO_OUTPUT)){
        for(i=0; i<count; i++){
            LOAD3;
            STAT3;
        }
    }else if (0) {  //(s->context || (s->flags&CODEC_FLAG_PASS1)){
        for(i=0; i<count; i++){
            LOAD3;
            STAT3;
            WRITE3;
        }
    }else{
        for(i=0; i<count; i++){
            LOAD3;
            WRITE3;
        }
    }
    return 0;
}
#endif

static inline int encode_bgra_bitstream(HYuvContext *s, int count, int planes)
{
    int i;

    if (s->pb.buf_end - s->pb.buf - (put_bits_count(&s->pb)>>3) < 4*planes*count) {
        av_log(s->avctx, AV_LOG_ERROR, "encoded frame too large\n");
        return -1;
    }

#define LOAD3\
            int g =  s->temp[0][planes==3 ? 3*i + 1 : 4*i + G];\
            int b = (s->temp[0][planes==3 ? 3*i + 2 : 4*i + B] - g) & 0xff;\
            int r = (s->temp[0][planes==3 ? 3*i + 0 : 4*i + R] - g) & 0xff;\
            int a =  s->temp[0][planes*i + A];
#define STAT3\
            s->stats[0][b]++;\
            s->stats[1][g]++;\
            s->stats[2][r]++;\
            if(planes==4) s->stats[2][a]++;
#define WRITE3\
            put_bits(&s->pb, s->len[1][g], s->bits[1][g]);\
            put_bits(&s->pb, s->len[0][b], s->bits[0][b]);\
            put_bits(&s->pb, s->len[2][r], s->bits[2][r]);\
            if(planes==4) put_bits(&s->pb, s->len[2][a], s->bits[2][a]);

    if ((s->flags & CODEC_FLAG_PASS1) &&
        (s->avctx->flags2 & CODEC_FLAG2_NO_OUTPUT)) {
        for (i = 0; i < count; i++) {
            LOAD3;
            STAT3;
        }
    } else if (s->context || (s->flags & CODEC_FLAG_PASS1)) {
        for (i = 0; i < count; i++) {
            LOAD3;
            STAT3;
            WRITE3;
        }
    } else {
        for (i = 0; i < count; i++) {
            LOAD3;
            WRITE3;
        }
    }
    return 0;
}

static int common_end(HYuvContext *s){
    int i;

    if (s->bitstream_bpp < 24) {
        for (i = 0;  i < 3;  i++) {
            av_freep(&s->temp[i]);
        }
    } else {
        av_freep(&s->temp[0]);
    }

    return 0;
}


int huffyuv_encode_frame (AVCodecContext *avctx, unsigned char *buf, int buf_size, void *data)
{
    HYuvContext *s;  // = avctx->priv_data;
    AVFrame *pict;  // = data;
    //const int width;  //= s->width;
    //const int height;  //= s->height;
    //const int fake_ystride;  //= s->interlaced ? pict->linesize[0]*2  : pict->linesize[0];
    int width;  //= s->width;
    //int height;  //= s->height;
    //int fake_ystride;  //= s->interlaced ? pict->linesize[0]*2  : pict->linesize[0];
    //AVFrame const *p;  //= &s->picture;
    AVFrame *p;  //= &s->picture;
    int size;


    s = avctx->priv_data;
    pict = data;
    width= s->width;
    //width2= s->width>>1;
    //height= s->height;
    //fake_ystride= s->interlaced ? pict->linesize[0]*2  : pict->linesize[0];
    //fake_ustride= s->interlaced ? pict->linesize[1]*2  : pict->linesize[1];
    //fake_vstride= s->interlaced ? pict->linesize[2]*2  : pict->linesize[2];
    p = &s->picture;
    size = 0;

    *p = *pict;
    //p->pict_type= FF_I_TYPE;
    //p->key_frame= 1;

    init_put_bits(&s->pb, buf+size, buf_size-size);

    if (0) {  //(avctx->pix_fmt == PIX_FMT_YUV422P || avctx->pix_fmt == PIX_FMT_YUV420P){

    } else if (1) {  // if(avctx->pix_fmt == PIX_FMT_RGB32){
        //x// uint8_t *data = p->data[0] + (height-1)*p->linesize[0];
        uint8_t *data = p->data[0];  // + (height-1)*p->linesize[0];
        //x// const int stride = -p->linesize[0];
        const int stride = p->linesize[0];
        //const int fake_stride = -fake_ystride;
        int y;
        int leftr, leftg, leftb;

        put_bits(&s->pb, 8, leftr= data[R]);
        put_bits(&s->pb, 8, leftg= data[G]);
        put_bits(&s->pb, 8, leftb= data[B]);
        put_bits(&s->pb, 8, 0);

        //Com_Printf("width %d  height %d  linesize %d\n", width, height, p->linesize[0]);
        //sub_left_prediction_bgr32(s, s->temp[0], data+4, width-1, &leftr, &leftg, &leftb);
        sub_left_prediction_rgb24(s, s->temp[0], data+3, width-1, &leftr, &leftg, &leftb);
        //encode_bgr_bitstream(s, width-1);
        encode_bgra_bitstream(s, width-1, 3);

        for(y=1; y<s->height; y++){
            uint8_t *dst = data + y*stride;
            if (0) {  //(s->predictor == PLANE && s->interlaced < y){
            }else{
                //sub_left_prediction_bgr32(s, s->temp[0], dst, width, &leftr, &leftg, &leftb);
                sub_left_prediction_rgb24(s, s->temp[0], dst, width, &leftr, &leftg, &leftb);
            }
            //encode_bgr_bitstream(s, width);
            encode_bgra_bitstream(s, width, 3);
        }
    }else{
        //av_log(avctx, AV_LOG_ERROR, "Format not supported!\n");
    }
    //emms_c();

    size+= (put_bits_count(&s->pb)+31)/8;
    put_bits(&s->pb, 16, 0);
    put_bits(&s->pb, 15, 0);
    size/= 4;

    if (0) {  //((s->flags&CODEC_FLAG_PASS1) && (s->picture_number&31)==0){

    } else {
        //avctx->stats_out[0] = '\0';
    }

    if (1) {  //(!(s->avctx->flags2 & CODEC_FLAG2_NO_OUTPUT)){
        flush_put_bits(&s->pb);
        //s->dsp.bswap_buf((uint32_t*)buf, (uint32_t*)buf, size);
        bswap_buf((uint32_t*)buf, (uint32_t*)buf, size);
    }

    s->picture_number++;

    return size*4;
}

int huffyuv_encode_end (AVCodecContext *avctx)
{
    HYuvContext *s = avctx->priv_data;

    common_end(s);

    av_freep(&avctx->extradata);
    av_freep(&avctx->stats_out);

    return 0;
}


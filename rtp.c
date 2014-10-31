#include <stdio.h>
#include <string.h>

#include "rtsp.h"
#include "rtp.h"
#include "utils.h"
#include "rtspType.h"

#define SHORT_INT_MAX (32767)
static uint32_t UnpackRtpSingle_NAL(char *buf, uint32_t size, char *framebuf);
static uint32_t UnpackRtpSTAP_A_NAL(char *buf, uint32_t size, char *frembuf);
static uint32_t UnpackRtpFU_A_NAL(char *buf, uint32_t size, char *framebuf);

static char h264prefix[3] = {0x00, 0x00, 0x01};

int32_t CheckRtpSequence(char *buf, void* args)
{
    RtspSession *sess = (RtspSession *)(args);
    /* Sequence number */
    uint32_t seq = ((unsigned char)buf[2])*256 + (unsigned char)buf[3];

    if ((SHORT_INT_MAX == sess->rtpseq) && (0x00 == seq)){
        sess->rtpseq = seq;
    }else if (0x01 < (seq - sess->rtpseq)){
        return False;
    }else{
        sess->rtpseq = seq;
    }
    return True;
}

int32_t CheckRtpHeaderMarker(char *buf, uint32_t size)
{
    int32_t marker = CHECK_BIT(buf[0x01], 8);

    if (0x01 == marker)
        return True;
    return False;
}

uint32_t GetRtpHeaderLength(char *buf, uint32_t size)
{
    uint32_t offset = 0;

    /* next byte */
    offset++;

    /* next byte */
    offset++;

    /* Sequence number */
    offset += 2;

    /* time stamp */
    offset += 4;

    /* ssrc / source identifier */
    offset += 4;

    return offset;
}

#if 1
unsigned int UnpackRtpNAL(char *buf, uint32_t size, char *framebuf, uint32_t framelen)
{
    if (((0x00 == buf[0]) && (0x00 == buf[1]) && (0x01 == buf[2])) || \
        ((0x00 == buf[0]) && (0x00 == buf[1]) && (0x00 == buf[2]) && (0x01 == buf[3]))){
        memcpy((void *)framebuf, (const void*)buf, size);
        return size;
    }
    /*
     * NAL, first byte header
     *
     *   +---------------+
     *   |0|1|2|3|4|5|6|7|
     *   +-+-+-+-+-+-+-+-+
     *   |F|NRI|  Type   |
     *   +---------------+
     */
    NALU_HEADER nalu = *(NALU_HEADER *)(&buf[0]);
    int32_t nal_forbidden_zero = nalu.F;//CHECK_BIT(buf[0], 7);
    int32_t nal_nri  = nalu.NRI;//(buf[0] & 0x60) >> 5;
    int32_t nal_type = nalu.TYPE;//(buf[0] & 0x1F);
#if 0
    printf(" NAL Header  %02x, %02x, %02x\n", buf[0], buf[1], buf[2]);
    printf("         Forbidden zero: %i\n", nal_forbidden_zero);
    printf("         NRI           : %i\n", nal_nri);
    printf("         Type          : %i\n", nal_type);
#endif

    /* Single NAL unit packet */
    if (nal_type >= NAL_TYPE_SINGLE_NAL_MIN &&
        nal_type <= NAL_TYPE_SINGLE_NAL_MAX) {
        return UnpackRtpSingle_NAL(buf, size, framebuf);
    }else if (nal_type == NAL_TYPE_STAP_A) {
        return UnpackRtpSTAP_A_NAL(buf, size, framebuf);
    }else if (nal_type == NAL_TYPE_FU_A) {
        return UnpackRtpFU_A_NAL(buf, size, framebuf);
    }
    return 0x00;
}

static uint32_t UnpackRtpSingle_NAL(char *buf, uint32_t size, char* framebuf)
{
    uint32_t len = sizeof(h264prefix);
    memcpy((void *)framebuf, (const void*)(&h264prefix), len);
    memcpy((void *)(framebuf+len), (const void*)(buf), size);
    return size+len;
}
/*
 * Agregation packet - STAP-A
 * ------
 * http://www.ietf.org/rfc/rfc3984.txt
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          RTP Header                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         NALU 1 Data                           |
 * :                                                               :
 * +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |               | NALU 2 Size                   | NALU 2 HDR    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         NALU 2 Data                           |
 * :                                                               :
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
static uint32_t UnpackRtpSTAP_A_NAL(char *buf, uint32_t size, char *framebuf)
{
    uint32_t len = sizeof(h264prefix);
    char *pos = framebuf;
    char *q = NULL;
    unsigned int nalu_size;
    uint32_t paysize = size;

    q = buf+1;
    int32_t nidx = 0;
    while (nidx < (paysize-1)) {
        memcpy((void *)pos, (const void*)(&h264prefix), len);
        pos += len;
        /* get NALU size */
        nalu_size = ((unsigned char)q[nidx]<<8)|((unsigned char)q[nidx+1]);
        nidx += 2;
        memcpy((void *)pos, (const void*)(&nalu_size), 1);
        pos += 1;
        if (nalu_size == 0) {
            nidx++;
            continue;
        }
        memcpy((void *)pos, (const void*)(q+nidx), nalu_size);
        nidx += nalu_size;
        pos  += nalu_size;
    }

    return (pos-framebuf);
}


static uint32_t UnpackRtpFU_A_NAL(char *buf, uint32_t size, char *framebuf)
{
    FU_HEADER fu_hdr = *(FU_HEADER *)(&buf[1]);
    unsigned  char  head1  =  buf[0];
    unsigned  char  head2  =  buf[1];
    unsigned  char  fua  = (head1&0xe0)|(head2& 0x1f ); // FU_A nal
    uint32_t len = sizeof(h264prefix);
    char *pos = framebuf;

    if (0x01 == fu_hdr.S) {
        /* 加上一字节的NALU头 */
        memcpy((void *)pos, (const void*)(&h264prefix), len);
        pos += len;
        memcpy((void *)pos, (const void*)(&fua), sizeof(fua));
        pos += 1;
        memcpy((void *)pos, (const void*)(buf+3), size -3);
        pos += size-3;
    }else if (0x01 == fu_hdr.E){
        /* 分片NAL单元结束 */
        memcpy((void *)pos, (const void*)(buf+2), size-2);
        pos += size-2;
    }else{
        /* 分片NAL单元中间数据 */
        memcpy((void *)pos, (const void*)(buf+2), size-2);
        pos += size-2;
    }

    return (pos-framebuf);
}

#endif

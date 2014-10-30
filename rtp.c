#include <string.h>

#include "rtsp.h"
#include "rtp.h"
#include "utils.h"
#include "rtspType.h"

#define SHORT_INT_MAX (32767)
int32_t CheckRtpSequence(char *buf, void* args)
{
    RtspSession *sess = (RtspSession *)(args);
    /* Sequence number */
    uint32_t seq = ((unsigned char)buf[2])*256 + (unsigned char)buf[3];

#if 0
    if ((SHORT_INT_MAX == sess->rtpseq) && (0x00 == seq)){
        sess->rtpseq = seq;
    }else if (0x01 < (seq - sess->rtpseq)){
        return False;
    }else{
        sess->rtpseq = seq;
    }
#endif
    return True;
}

uint32_t GetRtpHeaderLength(char *buf, uint32_t size)
{
    uint32_t offset = 0;
    RtpHeader rtphdr;

    rtphdr.version = buf[offset] >> 6;
    rtphdr.padding = CHECK_BIT(buf[offset], 5);
    rtphdr.extension = CHECK_BIT(buf[offset], 4);
    rtphdr.cc = buf[offset] & 0xFF;

    /* next byte */
    offset++;

    rtphdr.marker = CHECK_BIT(buf[offset], 8);
    rtphdr.pt     = buf[offset] & 0x7f;

    /* next byte */
    offset++;

    /* Sequence number */
    rtphdr.seq = buf[offset] * 256 + buf[offset + 1];
    offset += 2;

    /* time stamp */
    rtphdr.ts = \
        (buf[offset    ] << 24) |
        (buf[offset + 1] << 16) |
        (buf[offset + 2] <<  8) |
        (buf[offset + 3]);
    offset += 4;

    /* ssrc / source identifier */
    rtphdr.ssrc = \
        (buf[offset    ] << 24) |
        (buf[offset + 1] << 16) |
        (buf[offset + 2] <<  8) |
        (buf[offset + 3]);
    offset += 4;

    return offset;
}

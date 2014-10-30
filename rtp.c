#include <string.h>

#include "rtp.h"
#include "utils.h"


uint32_t GetRtpHeaderLength(char *buf, uint32_t size)
{
    uint32_t offset = 0;
    RtpHeader rtp_h;

    rtp_h.version = buf[offset] >> 6;
    rtp_h.padding = CHECK_BIT(buf[offset], 5);
    rtp_h.extension = CHECK_BIT(buf[offset], 4);
    rtp_h.cc = buf[offset] & 0xFF;

    /* next byte */
    offset++;

    rtp_h.marker = CHECK_BIT(buf[offset], 8);
    rtp_h.pt     = buf[offset] & 0x7f;

    /* next byte */
    offset++;

    /* Sequence number */
    rtp_h.seq = buf[offset] * 256 + buf[offset + 1];
    offset += 2;

    /* time stamp */
    rtp_h.ts = \
        (buf[offset    ] << 24) |
        (buf[offset + 1] << 16) |
        (buf[offset + 2] <<  8) |
        (buf[offset + 3]);
    offset += 4;

    /* ssrc / source identifier */
    rtp_h.ssrc = \
        (buf[offset    ] << 24) |
        (buf[offset + 1] << 16) |
        (buf[offset + 2] <<  8) |
        (buf[offset + 3]);
    offset += 4;

    return offset;
}

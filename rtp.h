#ifndef _RTP_H_
#define _RTP_H_

#include <time.h>
#include "rtspType.h"
#define MAX_BUF_SIZE  (65536 * 10)

typedef struct RTP_OVER_TCP{
    unsigned char magic;
    unsigned char ch;
    unsigned char len[2];
}RtpOverTcp;

typedef struct rtp_header {
    uint32_t version:2;     /* protocol version */
    uint32_t padding:1;     /* padding flag */
    uint32_t extension:1;   /* header extension flag */
    uint32_t cc:4;          /* CSRC count */
    uint32_t marker:1;      /* marker bit */
    uint32_t pt:7;          /* payload type */
    uint32_t seq:16;            /* sequence number */
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
    uint32_t csrc[1];           /* optional CSRC list */
}RtpHeader;

#define RTP_TCP_MAGIC   (0x24)
#define RTP_FREQ    90000
#define RTP_SPROP   "sprop-parameter-sets="

/* Enumeration of H.264 NAL unit types */
enum {
    NAL_TYPE_UNDEFINED = 0,
    NAL_TYPE_SINGLE_NAL_MIN	= 1,
    NAL_TYPE_SINGLE_NAL_MAX	= 23,
    NAL_TYPE_STAP_A		= 24,
    NAL_TYPE_FU_A		= 28,
};

uint32_t GetRtpHeaderLength(char *buf, uint32_t size);
int32_t CheckRtpSequence(char *buf, void* args);


#endif

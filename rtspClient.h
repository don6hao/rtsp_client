#ifndef _RTSP_Client_H_
#define _RTSP_Client_H_

#include <types.h>
#include "rtspType.h"

typedef struct RTSP_VIDEO_INFO{
}VideoInfo;

typdef struct RTSP_SDP_INFO{
}SdpInfo;

typedef struct RTSP_CLIENT_SESSION{
    uint8_t  ip[16];
    uint32_t port;
    int32_t sockfd;
    uint32_t cseq;

}RtspClientSession;


void ParseUrl(char *url, RtspClientSession *sess);
int RtspEventLoop(RtspClientSession *sess);

#endif

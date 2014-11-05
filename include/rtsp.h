#ifndef _RTSP_H_
#define _RTSP_H_

#include "rtspType.h"
#include "rtp.h"
#include "rtcp.h"

typedef enum{
    RTSP_START,
    RTSP_OPTIONS,
    RTSP_DESCRIBE,
    RTSP_SETUP,
    RTSP_PLAY,
    RTSP_PAUSE,
    RTSP_GET_PARAMETER,
    RTSP_SET_PARAMETER,
    RTSP_REDIRECT,
    RTSP_TEARDOWN,
    RTSP_QUIT
}EN_RTSP_STATUS;

typedef enum{
    RTP_AVP_TCP,
    RTP_AVP_UDP
}EN_TRANSPORTS;

typedef struct Public_Table{
    char cmd[16];
    uint32_t key;
}PublicTbl;

typedef struct RTP_TCP{
    char  start;          /* interleaved start */
    char  end;            /* interleaved end   */
    char  reserve[3];
}RtpTcp;

typedef struct RTP_UDP{
    uint32_t cport_from;    /* client_port from */
    uint32_t cport_to;      /* client port to   */
    uint32_t sport_from;    /* server port from */
    uint32_t sport_to;      /* server port to   */
    char unicast;
    char mode;
    char reserve[2];
}RtpUdp;

typedef struct VIDEO_MEDIA{
    char control[128];
}VideoMedia;

typedef struct AUDIO_MEDIA{
    char control[128];
}AudioMedia;

typedef struct RTSPSESSION{
    uint32_t port;
    int32_t sockfd;
    int32_t cseq;
    uint32_t timeout;

    uint32_t packetization; /* Packetization mode from SDP data */
    union{
        RtpUdp    udp;
        RtpTcp    tcp;
    }transport;
    PublicTbl     *pubtbl;
    RtpSession    rtpsess;
    AudioMedia    amedia;
    VideoMedia    vmedia;
    BufferControl buffctrl;
    char  sessid[32];
    char  url[128];
    char  username[128];
    char  password[128];
    char  ip[16];
    char  trans;      /* RTP/AVP/UDP or RTP/AVP/TCP */
    char  status;
    char  reserve[2];
}RtspSession;

int32_t RtspOptionsCommand(RtspSession *sess);
int32_t RtspDescribeCommand(RtspSession *sess);
int32_t RtspSetupCommand(RtspSession *sess);
int32_t RtspPlayCommand(RtspSession *sess);
int32_t RtspTeardownCommand(RtspSession *sess);
int32_t RtspStatusMachine(RtspSession *sess);

#endif

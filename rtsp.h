#ifndef _RTSP_H_
#define _RTSP_H_

#define VERSION           "0.1"
#define PROTOCOL_PREFIX   "rtsp://"
#define RTSP_PORT         554
#define RTSP_CLIENT_PORT  9500
#define RTSP_RESPONSE     "RTSP/1.0 "
#define CMD_OPTIONS       "OPTIONS rtsp://%s:%d RTSP/1.0\r\nCSeq: %i\r\n\r\n"
#define CMD_DESCRIBE      "DESCRIBE %s RTSP/1.0\r\nCSeq: %i\r\nAccept: application/sdp\r\n\r\n"
#define CMD_SETUP         "SETUP %s RTSP/1.0\r\nCSeq: %i\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n"
#define CMD_PLAY          "PLAY %s RTSP/1.0\r\nCSeq: %i\r\nSession: %s\r\nRange: npt=0.00-\r\n\r\n"

#define SETUP_SESSION      "Session: "
#define SETUP_TRNS_CLIENT  "client_port="
#define SETUP_TRNS_SERVER  "server_port="

#define SDP_M_VIDEO        "m=video "
#define SDP_M_AUDIO        "m=audio "
#define SDP_A_RTPMAP       "a=rtpmap:"
#define SDP_A_FMTP         "a=fmtp:"
#define SDP_A_CONTROL      "a=control:"

typedef struct RTP_TCP{
    char  start;          /* interleaved start */
    char  end;            /* interleaved end   */
    char  reserve[3];
}RtpTcp;

typedef struct RTP_UDP{
    uint32_t ssrc;
    uint32_t cport_from;    /* client_port from */
    uint32_t cport_to;      /* client port to   */
    uint32_t sport_from;    /* server port from */
    uint32_t sport_to;      /* server port to   */
    char unicast;
    char mode;
    char reserve[2];
}RtpUdp;

typedef struct RTSPSESSION{
    uint32_t port;
    int32_t sockfd;
    int32_t cseq;

    uint32_t packetization; /* Packetization mode from SDP data */
    union{
        RtpUdp    udp;
        RtpTcp    tcp;
    }transport;
    char  sessid[16];
    char  url[128];
    char  ip[16];
    char  trans;      /* RTP/AVP/UDP or RTP/AVP/TCP */
    char  reserve[3];
}RtspSession;

int32_t RtspOptionsMsg(RtspSession *sess);
int32_t RtspDescribeMsg(RtspSession *sess);
int32_t RtspSetupMsg(RtspSession *sess);
int32_t RtspPlayMsg(RtspSession *sess);

#endif

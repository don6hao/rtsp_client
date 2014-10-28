#ifndef _RTSP_H_
#define _RTSP_H_

#define VERSION           "0.1"
#define PROTOCOL_PREFIX   "rtsp://"
#define RTSP_PORT         554
#define RTSP_CLIENT_PORT  9500
#define RTSP_RESPONSE     "RTSP/1.0 "
#define CMD_OPTIONS       "OPTIONS rtsp://%s:%lu RTSP/1.0\r\nCSeq: %i\r\n\r\n"
#define CMD_DESCRIBE      "DESCRIBE %s RTSP/1.0\r\nCSeq: %i\r\nAccept: application/sdp\r\n\r\n"
#define CMD_SETUP         "SETUP %s/trackID=1 RTSP/1.0\r\nCSeq: %i\r\nTransport: RTP/AVP/TCP;interleaved=0-1;\r\n\r\n"
#define CMD_PLAY          "PLAY %s RTSP/1.0\r\nCSeq: %i\r\nSession: %lu\r\nRange: npt=0.00-\r\n\r\n"

#define SETUP_SESSION      "Session: "
#define SETUP_TRNS_CLIENT  "client_port="
#define SETUP_TRNS_SERVER  "server_port="

int RtspOptionsMsg(RtspClientSession *sess);

#endif

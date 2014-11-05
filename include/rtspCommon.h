#ifndef _RTSP_COMMON_H_
#define  _RTSP_COMMON_H_

#include "rtspType.h"
#include "rtsp.h"

#define CLIENT_PORT_FLAG 0x0A
#define SERVER_PORT_FLAG 0x0B

int32_t ParseTimeout(char *buf, uint32_t size, RtspSession *sess);
int32_t ParseUdpPort(char *buf, uint32_t size, RtspSession *sess);
int32_t ParseInterleaved(char *buf, uint32_t num, RtspSession *sess);
int32_t ParseSessionID(char *buf, uint32_t size, RtspSession *sess);
int32_t RtspResponseStatus(char *response);
uint32_t GetSDPLength(char *buf, uint32_t size);
int32_t ParseSdpProto(char *buf, uint32_t size, RtspSession *sess);
void GetSdpVideoTransport(char *buf, uint32_t size, RtspSession *sess);
void GetSdpVideoAcontrol(char *buf, uint32_t size, RtspSession *sess);
void RtspIncreaseCseq(RtspSession *sess);

#endif

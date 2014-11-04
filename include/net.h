#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>
#include "rtspType.h"

int32_t RtspSocketCork(int32_t fd, int32_t state);
int32_t RtspSocketNonblock(int32_t sockfd);
int32_t RtspUdpConnect(char *ip, uint32_t port);
int32_t RtspTcpConnect(char *ip, uint32_t port);
int32_t RtspTcpSendData(int32_t fd, char *buf, uint32_t size);
int32_t RtspTcpReceiveData(int32_t fd, char *buf, uint32_t size);
void RtspCloseScokfd(int32_t sockfd);
int32_t RtspRecvUdpRtpData(int32_t fd, char *buf, uint32_t size);
int32_t RtspCreateUdpServer(char *ip, uint32_t port);
int32_t RtspSendUdpRtpData(int32_t fd, char *buf, uint32_t size, char *ip, uint32_t port);

#endif

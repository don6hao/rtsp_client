#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>
#include "rtspType.h"

int32_t RtspSocketCork(int32_t fd, int32_t state);
int32_t RtspSocketNonblock(int32_t sockfd);
int32_t RtspUdpConnect(int8_t *ip, uint32_t port);
int32_t RtspTcpConnect(int8_t *ip, uint32_t port);
int32_t RtspTcpSendMsg(int32_t fd, int8_t *buf, uint32_t size);
int32_t RtspTcpRcvMsg(int32_t fd, int8_t *buf, uint32_t size);
void RtspCloseScokfd(int32_t sockfd);

#endif

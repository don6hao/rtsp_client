#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>
#include <netinet/in.h>
#include "rtspType.h"

int32_t SocketCork(int32_t fd, int32_t state);
int32_t SocketNonblock(int32_t sockfd);
int32_t UdpConnect(struct sockaddr_in *addr, char *ip, uint32_t port, uint32_t sockfd);
int32_t TcpConnect(char *ip, uint32_t port);
int32_t TcpSendData(int32_t fd, char *buf, uint32_t size);
int32_t TcpReceiveData(int32_t fd, char *buf, uint32_t size);
void    CloseScokfd(int32_t sockfd);
int32_t CreateUdpServer(char *ip, uint32_t port);
int32_t UdpSendData(int32_t fd, char *buf, uint32_t size, struct sockaddr_in *remote);
int32_t UdpReceiveData(int32_t fd, char *buf, uint32_t size, struct sockaddr_in *addr);

#endif

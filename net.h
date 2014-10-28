/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <stdint.h>

#ifndef NETWORK_H
#define NETWORK_H

int RtspTCPConnect(char *host, unsigned long port);
int RtspUDPConnect(char *host, unsigned long port);
int RtspSocketNonblock(int sockfd);
int RtspSocketCork(int fd, int state);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "net.h"

int32_t RtspTcpConnect(char *ip, uint32_t port)
{
    int32_t res;
    int32_t sock_fd;
    struct sockaddr_in *remote;

    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd <= 0) {
	    printf("Error: could not create socket\n");
	    return -1;
    }

    remote = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    remote->sin_family = AF_INET;
    res = inet_pton(AF_INET, (const char*)ip, (void *) (&(remote->sin_addr.s_addr)));

    if (res < 0) {
	    fprintf(stderr, "Error: Can't set remote->sin_addr.s_addr\n");
	    free(remote);
	    return -1;
    }
    else if (res == 0) {
	    fprintf(stderr, "Error: Invalid address '%s'\n", ip);
	    free(remote);
	    return -1;
    }

    remote->sin_port = htons(port);
    if (connect(sock_fd,
                (struct sockaddr *) remote, sizeof(struct sockaddr)) == -1) {
        close(sock_fd);
        fprintf(stderr, "Error connecting to %s:%d\n", ip, port);
        free(remote);
        return -1;
    }

    free(remote);
    return sock_fd;
}

int32_t RtspCreateUdpServer(char *ip, uint32_t port)
{
    int32_t sockfd;
    struct sockaddr_in addr;
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0){
        perror ("socket");
        return -1;
    }
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(INADDR_ANY) ;
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0){
        perror("connect");
        return -1;
    }

    return sockfd;
}

/* Connect to a UDP socket server and returns the file descriptor */
int32_t RtspUdpConnect(char *ip, uint32_t port)
{
    int32_t res;
    int32_t sock_fd;
    struct sockaddr_in *remote;

    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd <= 0) {
	    fprintf(stderr, "Error: could not create socket\n");
	    return -1;
    }

    remote = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    remote->sin_family = AF_INET;
    res = inet_pton(AF_INET, (const char*)ip, (void *) (&(remote->sin_addr.s_addr)));

    if (res < 0) {
	    fprintf(stderr, "Error: Can't set remote->sin_addr.s_addr\n");
	    free(remote);
	    return -1;
    }
    else if (res == 0) {
	    fprintf(stderr, "Error: Invalid address '%s'\n", ip);
	    free(remote);
	    return -1;
    }

    remote->sin_port = htons(port);
    if (connect(sock_fd,
                (struct sockaddr *) remote, sizeof(struct sockaddr)) == -1) {
        close(sock_fd);
        fprintf(stderr, "Error connecting to %s:%d\n", ip, port);
        free(remote);
        return -1;
    }

    free(remote);
    return sock_fd;
}

int32_t RtspSocketNonblock(int32_t sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }

   return 0;
}

int32_t RtspSocketCork(int32_t fd, int32_t state)
{
    return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
}

int32_t RtspTcpSendMsg(int32_t fd, char *buf, uint32_t size)
{
    int32_t num = 0x00;
    num = send(fd, buf, size, 0);

    return num;
}


int32_t RtspTcpRcvMsg(int32_t fd, char *buf, uint32_t size)
{
    int32_t num = recv(fd, buf, size-1, 0);
    return num;
}


void RtspCloseScokfd(int32_t sockfd)
{
    close(sockfd);
    return;
}


int32_t RtspRecvUdpRtpData(int32_t fd, char *buf, uint32_t size)
{
    int32_t num = 0x00;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    memset(buf,0x00, size);
    num = recvfrom(fd, buf, size, 0, (struct sockaddr *)&addr, &addr_len);
    /*显示client端的网络地址*/
    printf("receive from %s\n" , inet_ntoa( addr.sin_addr));

    return num;
}

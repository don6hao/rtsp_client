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

int RtspTcpConnect(char *host, unsigned long port)
{
    int res;
    int sock_fd;
    struct sockaddr_in *remote;

    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd <= 0) {
	    printf("Error: could not create socket\n");
	    return -1;
    }

    remote = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    remote->sin_family = AF_INET;
    res = inet_pton(AF_INET, host, (void *) (&(remote->sin_addr.s_addr)));

    if (res < 0) {
	    fprintf(stderr, "Error: Can't set remote->sin_addr.s_addr\n");
	    free(remote);
	    return -1;
    }
    else if (res == 0) {
	    fprintf(stderr, "Error: Invalid address '%s'\n", host);
	    free(remote);
	    return -1;
    }

    remote->sin_port = htons(port);
    if (connect(sock_fd,
                (struct sockaddr *) remote, sizeof(struct sockaddr)) == -1) {
        close(sock_fd);
        fprintf(stderr, "Error connecting to %s:%lu\n", host, port);
        free(remote);
        return -1;
    }

    free(remote);
    return sock_fd;
}


/* Connect to a UDP socket server and returns the file descriptor */
int RtspUdpConnect(char *host, unsigned long port)
{
    int res;
    int sock_fd;
    struct sockaddr_in *remote;

    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd <= 0) {
	    fprintf(stderr, "Error: could not create socket\n");
	    return -1;
    }

    remote = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    remote->sin_family = AF_INET;
    res = inet_pton(AF_INET, host, (void *) (&(remote->sin_addr.s_addr)));

    if (res < 0) {
	    fprintf(stderr, "Error: Can't set remote->sin_addr.s_addr\n");
	    free(remote);
	    return -1;
    }
    else if (res == 0) {
	    fprintf(stderr, "Error: Invalid address '%s'\n", host);
	    free(remote);
	    return -1;
    }

    remote->sin_port = htons(port);
    if (connect(sock_fd,
                (struct sockaddr *) remote, sizeof(struct sockaddr)) == -1) {
        close(sock_fd);
        fprintf(stderr, "Error connecting to %s:%lu\n", host, port);
        free(remote);
        return -1;
    }

    free(remote);
    return sock_fd;
}

int RtspSocketNonblock(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }

   return 0;
}

int RtspSocketCork(int fd, int state)
{
    return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
}

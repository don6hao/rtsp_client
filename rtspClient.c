#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtspType.h"
#include "rtspClient.h"
#include "net.h"
#include "tpool.h"

uint32_t ParseUrl(char *url, RtspClientSession *cses)
{
    uint32_t offset = sizeof(PROTOCOL_PREFIX) - 1;
    char *pos = NULL, buf[8];
    uint32_t len = 0x00;

    RtspSession *sess = &cses->sess;
    //get host
    pos = (char *)strchr((const char *)(url+offset), ':');
    if (NULL == pos){
        pos = (char *)strchr((const char *)(url+offset), '/');
        if (NULL == pos)    return False;
    }
    len  = (pos-url)-offset;
    strncpy((char *)sess->ip, (const char *)(url+offset), len);
    sess->ip[len] = '\0';


    //get port
    pos = (char *)strchr((const char *)(url+offset), ':');
    if (NULL != pos){
        len = pos-url+1;
        pos = (char *)strchr((const char *)(url+len), '/');
        if (NULL == pos)    return False;

        uint32_t size = (pos-url)-len;
        if (size > sizeof(buf)){
            fprintf(stderr, "Error: Invalid port\n");
            return False;
        }
        pos = url+len; //指向:符号
        strncpy((char *)buf, (const char *)pos, size);
        sess->port = atol((const char *)buf); //Note测试port的范围大小
    }

    strncpy((char *)sess->url, (const char *)url, \
            strlen((const char *)url) > sizeof(sess->url) ? \
            sizeof(sess->url) : strlen((const char *)url));
#ifdef RTSP_DEBUG
    printf("Host|Port : %s:%d\n", sess->ip, sess->port);
#endif

    return True;
}

void* RtspHandleUdpConnect(void* args)
{
    RtspClientSession *csess = (RtspClientSession *)(args);
    RtspSession *sess = &csess->sess;

    int32_t rtpfd = RtspCreateUdpServer(sess->ip, sess->transport.udp.cport_from);
    /*int32_t rtcpfd = RtspUdpConnect(sess->ip, sess->transport.udp.sport_to);*/
    int32_t num = 0x00, size = 4096;
    char    buf[size];
#ifdef RTSP_DEBUG
    printf("rtp fd : %d\n", rtpfd);
    printf("ip, port : %s, %d\n", sess->ip, sess->transport.udp.cport_from);
#endif
    do{
        num = RtspRecvUdpRtpData(rtpfd, buf, size-1);
        memset(buf, 0x00, size);
        printf("recv data length : %d\n", num);
    }while(num > 0);

    return NULL;
}

void* RtspEventLoop(void* args)
{
    RtspClientSession *csess = (RtspClientSession *)(args);
    RtspSession *sess = &csess->sess;
    int32_t fd = RtspTcpConnect(sess->ip, sess->port);
    if (fd <= 0x00){
        fprintf(stderr, "Error: RtspConnect.\n");
        return NULL;
    }

    pthread_t transid = 0x00;
    sess->sockfd = fd;
    do{
        if (False == RtspStatusMachine(sess))
            break;
        if (RTSP_PLAY == sess->status){
            if ((0x00 == transid) && (RTP_AVP_UDP == sess->trans)){
                transid = RtspCreateThread(RtspHandleUdpConnect, (void *)sess);
                if (transid <= 0x00){
                    fprintf(stderr, "RtspCreateThread Error!\n");
                    sleep(10);
                    continue;
                }
            }
        }
    }while(1);

    return NULL;
}

RtspClientSession* InitRtspClientSession()
{
    RtspClientSession *cses = (RtspClientSession *)calloc(1, sizeof(RtspClientSession));

    if (NULL == cses)
        return NULL;

    RtspSession *sess = &cses->sess;
    sess->trans  = RTP_AVP_UDP;
    sess->status = RTSP_START;
    return cses;
}

void DeleteRtspClientSession(RtspClientSession *cses)
{
    if (NULL == cses)
        return;

    RtspSession *sess = &cses->sess;
    RtspCloseScokfd(sess->sockfd);
    free(sess);
    sess = NULL;
    return;
}

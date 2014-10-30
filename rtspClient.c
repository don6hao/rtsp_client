#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtspType.h"
#include "rtspClient.h"
#include "net.h"
#include "tpool.h"
#include "utils.h"
#include "rtp.h"

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

void* RtspHandleTcpConnect(void* args)
{
    RtspClientSession *csess = (RtspClientSession *)(args);
    RtspSession *sess = &csess->sess;

    int32_t sockfd = sess->sockfd;
    int32_t num = 0x00, size = 1920*1080;
    int32_t rtpch = sess->transport.tcp.start;
    int32_t rtcpch = sess->transport.tcp.end;
    char    buf[size], *pos = buf;
    uint32_t length;
    RtpOverTcp rot;
#ifdef SAVE_FILE_DEBUG
    FILE    *fp = fopen("1.264", "w+");
    if (NULL == fp){
        fprintf(stderr, "fopen error!\n");
        return NULL;
    }
#endif

    do{
        num = RtspTcpRcvMsg(sockfd, (char *)&rot, sizeof(RtpOverTcp));
        if (num <= 0x00){
            fprintf(stderr, "recv error or connection closed!\n");
            break;
        }

        if (RTP_TCP_MAGIC == rot.magic){
            rot.len[1] &= 0xFF;
            length = GET_16(&rot.len[0]);
            int32_t size = length;
            do{
                num = RtspTcpRcvMsg(sockfd, pos, size);
                if (num <= 0x00){
                    fprintf(stderr, "recv error or connection closed!\n");
                    break;
                }
                size -= num;
            }while(size > 0x00);
            if (rtcpch == rot.ch){
                /* RTCP Protocl */
            }else if (rtpch == rot.ch){
                /* RTP Protocl */
                length = GetRtpHeaderLength(pos, length);
                /*if (False == CheckRtpSequence(pos, (void *)sess))*/
                    /*continue;*/
#ifdef SAVE_FILE_DEBUG
                fwrite(pos+length, num-length, 1, fp);
                fflush(fp);
#endif
            }
        }
    }while(1);

#ifdef SAVE_FILE_DEBUG
    fclose(fp);
#endif
    printf("RtspHandleTcpConnect Quit!\n");
    return NULL;
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

    printf("RtspHandleUdpConnect Quit!\n");
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

#if 0
    pthread_t transid = 0x00;
#endif
    sess->sockfd = fd;
    do{
        if ((False == RtspStatusMachine(sess)) || \
                (RTSP_QUIT == sess->status))
            break;
        if (RTSP_PLAY == sess->status){
#if 0
            if (0x00 == transid){
                if (RTP_AVP_UDP == sess->trans){
                    transid = RtspCreateThread(RtspHandleUdpConnect, (void *)sess);
                    if (transid <= 0x00){
                        fprintf(stderr, "RtspCreateThread Error!\n");
                        continue;
                    }
                }else if (RTP_AVP_TCP == sess->trans){
                    RtspHandleTcpConnect((void *)sess);
                }
            }
#else
            RtspHandleTcpConnect((void *)sess);
#endif
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
    sess->transport.tcp.start = 0x00;
    sess->transport.tcp.end = 0x01;
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "rtspType.h"
#include "rtspClient.h"
#include "net.h"
#include "tpool.h"
#include "utils.h"
#include "rtp.h"
#include "rtcp.h"

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
    int32_t num = 0x00, size = 1500;
    int32_t rtpch = sess->transport.tcp.start;
    int32_t rtcpch = sess->transport.tcp.end;
    char    buf[size], *pos = buf, framebuf[1920*1080];
    uint32_t length, framelen = 0x00;
    RtpOverTcp rot;
#ifdef SAVE_FILE_DEBUG
    FILE    *fp = fopen("1.264", "w+");
    if (NULL == fp){
        fprintf(stderr, "fopen error!\n");
        return NULL;
    }
#endif

    memset(framebuf, 0x00, sizeof(framebuf));
    do{
        pos = buf;
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
                pos  += num;
            }while(size > 0x00);
            if (rtcpch == rot.ch){
                /* RTCP Protocl */
            }else if (rtpch == rot.ch){
                /* RTP Protocl */
                length = sizeof(RtpHeader);
                num = UnpackRtpNAL(buf+length, num-length, framebuf+framelen, framelen);
                framelen += num;
                if (True == CheckRtpHeaderMarker(buf, length))
                {
#ifdef SAVE_FILE_DEBUG
                    printf("frame length :%d\n", framelen);
                    fwrite(framebuf, framelen, 1, fp);
                    fflush(fp);
#endif
                    exit(0);
                    framelen = 0x00;
                }
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
    int32_t rtcpfd = RtspCreateUdpServer(sess->ip, sess->transport.udp.cport_to);
    int32_t num = 0x00, size = 4096;
    char    buf[size], framebuf[1920*1080];
    uint32_t length, framelen = 0x00;
#ifdef RTSP_DEBUG
    printf("-------  %d, %d ---------\n", sess->transport.udp.sport_from, sess->transport.udp.sport_to);
    printf("rtp fd : %d, %d\n", rtpfd, rtcpfd);
    printf("ip, port : %s, %d\n", sess->ip, sess->transport.udp.cport_from);
#endif
#ifdef SAVE_FILE_DEBUG
    FILE    *fp = fopen("1.264", "w+");
    if (NULL == fp){
        fprintf(stderr, "fopen error!\n");
        return NULL;
    }
#endif

    struct timeval timeout;
    timeout.tv_sec=1;
    timeout.tv_usec=0;
    fd_set readfd;
    fd_set writefd;
    do{
        FD_ZERO(&readfd);
        FD_ZERO(&writefd);
        FD_SET(rtpfd, &readfd);
        FD_SET(rtcpfd, &readfd);
        FD_SET(rtcpfd, &writefd);
        int32_t ret = select(rtcpfd+1, &readfd, &writefd, NULL, &timeout);
        if (-1 == ret){
            fprintf(stderr, "select error!\n");
            break;
        }
        if (FD_ISSET(rtpfd, &readfd)){
            num = RtspRecvUdpRtpData(rtpfd, buf, size);
            if (num < 0x00){
                fprintf(stderr, "recv error or connection closed!\n");
                break;
            }
            ParseRtp(buf, num, &sess->rtpsess);
            length = sizeof(RtpHeader);
            num = UnpackRtpNAL(buf+length, num-length, framebuf+framelen, framelen);
            framelen += num;
            if (True == CheckRtpHeaderMarker(buf, length))
            {
#ifdef SAVE_FILE_DEBUG
                fwrite(framebuf, framelen, 1, fp);
                fflush(fp);
#endif
                framelen = 0x00;
            }
        }
        if (FD_ISSET(rtcpfd, &readfd)){
            num = RtspRecvUdpRtpData(rtcpfd, buf, size);
            if (num < 0x00){
                fprintf(stderr, "recv error or connection closed!\n");
                break;
            }
            uint32_t ret = ParseRtcp(buf, num);
            if (RTCP_BYE == ret){
                printf("Receive RTCP BYE!\n");
                break;
            }else if (RTCP_SR == ret){
                printf("Receive RTCP Sender Report!\n");
                char tmp[512];
                uint32_t len = RtcpReceiveReport(tmp, sizeof(tmp), &sess->rtpsess);
                RtspSendUdpRtpData(rtcpfd, tmp, len, sess->ip, sess->transport.udp.sport_to);
            }
        }
    }while(1);

#ifdef SAVE_FILE_DEBUG
    fclose(fp);
#endif
    close(rtpfd);
    close(rtcpfd);
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

#if 1
    pthread_t transid = 0x00;
#endif
    sess->sockfd = fd;
    do{
        if ((False == RtspStatusMachine(sess)) || \
                (RTSP_QUIT == sess->status))
            break;

        if (RTSP_PLAY == sess->status){
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


int32_t ParseRtspUrl(char *url, RtspSession *sess)
{
	do {
        // Parse the URL as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<stream-name>]"
        uint32_t offset = strlen(PROTOCOL_PREFIX);
        uint32_t i;
        char *from = url+offset;
        char *colonPasswordStart = NULL;
        char *p = NULL;

        for (p = from; *p != '\0' && *p != '/'; ++p)
        {
            if (*p == ':' && colonPasswordStart == NULL){
                colonPasswordStart = p;
            }else if (*p == '@'){
                if (colonPasswordStart == NULL) colonPasswordStart = p;
                char *usernameStart = from;
                uint32_t usernameLen = colonPasswordStart - usernameStart;

                for (i = 0; i < usernameLen; ++i)
                    sess->username[i] = usernameStart[i];
                sess->username[usernameLen] = '\0';

                char *passwordStart = colonPasswordStart;
                if (passwordStart < p) ++passwordStart; // skip over the ':'

                uint32_t passwordLen = p - passwordStart;
                uint32_t j = 0x00;
                for (; j < passwordLen; ++j)
                    sess->password[j] = passwordStart[j];
                sess->password[passwordLen] = '\0';
                from = p + 1; // skip over the '@'
                break;
            }
        }
        printf("sess username : %s\n", sess->username);
        printf("sess pssword : %s\n", sess->password);

        // Next, parse <server-address-or-name>
        char *to = sess->ip;
        for (i = 0; i < sizeof(sess->ip)+1; ++i)
        {
            if (*from == '\0' || *from == ':' || *from == '/')
            {
                *to = '\0';
                break;
            }
            *to++ = *from++;
        }
        if (i == sizeof(sess->ip)+1) {
            fprintf(stderr, "URL is too long");
            break;
        }

        printf("sess ip : %s\n", sess->ip);
        char nextChar = *from++;
        char tmp[8];
        to = tmp;
        if (nextChar == ':') {
            for (i = 0; i < sizeof(tmp); ++i)
            {
                if (*from == '/')
                {
                    *to = '\0';
                    break;
                }
                *to++ = *from++;
            }
            sess->port = atol((const char *)tmp);
        }
        printf("sess port : %d\n", sess->port);
    }while(0);

    return True;
}


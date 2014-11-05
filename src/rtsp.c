#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "rtspType.h"
#include "utils.h"
#include "rtsp.h"
#include "rtspCommon.h"
#include "rtspResponse.h"
#include "net.h"

static int32_t RtspSendOptionsCommand(RtspSession *sess)
{
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;

    memset(buf, '\0', size);
    int32_t num = snprintf(buf, size, CMD_OPTIONS, sess->ip, sess->port, sess->cseq);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }

    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

    return True;
}

int32_t RtspOptionsCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    char *buf = sess->buffctrl.buffer;
    int32_t sock = sess->sockfd;
#ifdef RTSP_DEBUG
    printf("++++++++++++++++++  OPTIONS: command  +++++++++++++++++++++\n");
#endif
    if (False == RtspSendOptionsCommand(sess))
        return False;
#ifdef RTSP_DEBUG
    printf("OPTIONS Request: %s\n", buf);
#endif
    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        printf("Error: Server did not respond properly, closing...");
        return False;
    }

#ifdef RTSP_DEBUG
    printf("\nOptions Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;
    RtspIncreaseCseq(sess);
    return True;
}

static int32_t RtspSendDescribeCommand(RtspSession *sess, char *buf, uint32_t size)
{
    int32_t sock = sess->sockfd;
    memset(buf, '\0', size);
    int32_t num = snprintf(buf, size, CMD_DESCRIBE, sess->url, sess->cseq);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }

    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

    return True;
}

int32_t RtspDescribeCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    char *buf = sess->buffctrl.buffer;
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("++++++++++++++++++++++  DESCRIBE: command  +++++++++++++++++++++++++++\n");
#endif

    if (False == RtspSendDescribeCommand(sess, buf, size))
        return False;

#ifdef RTSP_DEBUG
    printf("DESCRIBE Request: %s\n", buf);
#endif

    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        printf("Error: Server did not respond properly, closing...");
        return False;
    }

#ifdef RTSP_DEBUG
    printf("\nDescribe Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;
    ParseSdpProto(buf, num, sess);
    RtspIncreaseCseq(sess);

    return True;
}

static int32_t RtspSendSetupCommand(RtspSession *sess)
{
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t num = 0x00;
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;
    char url[256];

    memset(buf, '\0', size);
    memset(url, '\0', sizeof(url));
    if (NULL == strstr(sess->vmedia.control, PROTOCOL_PREFIX)){
        int32_t len = strlen(sess->url);
        strncpy(url, sess->url, len);
        url[len] = '/';
        url[len+1] = '\0';
    }
    strncat(url, sess->vmedia.control, strlen(sess->vmedia.control));
#ifdef RTSP_DEBUG
    printf("SETUP URL: %s\n", url);
#endif
    if (RTP_AVP_TCP == sess->trans){
        num = snprintf(buf, size, CMD_TCP_SETUP, url, sess->cseq);
    }else if (RTP_AVP_UDP == sess->trans){
        num = snprintf(buf, size, CMD_UDP_SETUP, url, sess->cseq, 10000, 10001);
    }
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }
    return True;
}

int32_t RtspSetupCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    char *buf = sess->buffctrl.buffer;
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("++++++++++++++++++++  SETUP: command  +++++++++++++++++++++++++\n");
#endif


    if (False == RtspSendSetupCommand(sess))
        return False;

#ifdef RTSP_DEBUG
    printf("SETUP Request: %s\n", buf);
#endif
    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

#ifdef RTSP_DEBUG
    printf("SETUP Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;

    if (RTP_AVP_UDP == sess->trans){
        ParseUdpPort(buf, num, sess);
    }else{
        ParseInterleaved(buf, num, sess);
    }
    ParseSessionID(buf, num, sess);
    sess->packetization = 1;
    RtspIncreaseCseq(sess);
    return True;
}

static int32_t RtspSendPlayCommand(RtspSession *sess)
{
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;

    memset(buf, '\0', size);
    int32_t num = snprintf(buf, size, CMD_PLAY, sess->url, sess->cseq, sess->sessid);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

    return True;
}

int32_t RtspPlayCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;

#ifdef RTSP_DEBUG
    printf("+++++++++++++++++++  PLAY: command  ++++++++++++++++++++++++++\n");
#endif
    if (False == RtspSendPlayCommand(sess))
        return False;

#ifdef RTSP_DEBUG
    printf("PLAY Request: %s\n", buf);
#endif

    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

#ifdef RTSP_DEBUG
    printf("PLAY Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;

    RtspIncreaseCseq(sess);
    return True;
}

static int32_t RtspSendGetParameterCommand(RtspSession *sess)
{
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;
    memset(buf, '\0', size);
    int32_t num = snprintf(buf, size, CMD_GET_PARAMETER, sess->url, sess->cseq, sess->sessid);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

    return True;
}

int32_t RtspGetParameterCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;

#ifdef RTSP_DEBUG
    printf("+++++++++++++++++++  Get Parameter: command  ++++++++++++++++++++++++++\n");
#endif
    if (False == RtspSendGetParameterCommand(sess))
        return False;

#ifdef RTSP_DEBUG
    printf("GET_PARAMETER Request: %s\n", buf);
#endif
    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }


#ifdef RTSP_DEBUG
    printf("GET PARAMETER Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;

    RtspIncreaseCseq(sess);
    return True;
}


static int32_t RtspSendTeardownCommand(RtspSession *sess)
{
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;
    memset(buf, '\0', size);
    int32_t num = snprintf(buf, size, CMD_TEARDOWN, sess->url, sess->cseq, sess->sessid);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = TcpSendData(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

    return True;
}

int32_t RtspTeardownCommand(RtspSession *sess)
{
    int32_t num;
    int32_t size = sizeof(sess->buffctrl.buffer);
    int32_t sock = sess->sockfd;
    char *buf = sess->buffctrl.buffer;

#ifdef RTSP_DEBUG
    printf("++++++++++++++++ TEARDOWN: command ++++++++++++++++++++++++++++\n");
#endif
    if (False == RtspSendTeardownCommand(sess))
        return False;

#ifdef RTSP_DEBUG
    printf("TEARDOWN Request: %s\n", buf);
#endif

    memset(buf, '\0', size);
    num = RtspReceiveResponse(sock, &sess->buffctrl);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

#ifdef RTSP_DEBUG
    printf("TEARDOWN Reply: %s\n", buf);
#endif
    if (False == RtspCheckResponseStatus(buf))
        return False;
    RtspIncreaseCseq(sess);
    return True;
}

int32_t RtspStatusMachine(RtspSession *sess)
{
    struct timeval now, playnow;
    do{
        switch(sess->status){
            case RTSP_START:
                if (False == RtspOptionsCommand(sess)){
                    fprintf(stderr, "Error: RtspOptionsCommand.\n");
                    return False;
                }
                sess->status = RTSP_OPTIONS;
                break;
            case RTSP_OPTIONS:
                if (False == RtspDescribeCommand(sess)){
                    fprintf(stderr, "Error: RtspDescribeCommand.\n");
                    return False;
                }
                sess->status = RTSP_DESCRIBE;
                break;
            case RTSP_DESCRIBE:
                if (False == RtspSetupCommand(sess)){
                    fprintf(stderr, "Error: RtspSetupCommand.\n");
                    return False;
                }
                sess->status = RTSP_SETUP;
                break;
            case RTSP_SETUP:
                if (False == RtspPlayCommand(sess)){
                    fprintf(stderr, "Error: RtspPlayCommand.\n");
                    return False;
                }
                sess->status = RTSP_PLAY;
                break;
            case RTSP_PLAY:
                RtspGetParameterCommand(sess);
                gettimeofday(&playnow, NULL);
                sess->status = RTSP_GET_PARAMETER;
                break;
            case RTSP_GET_PARAMETER:
                gettimeofday(&now, NULL);
                if ((now.tv_sec - playnow.tv_sec) > sess->timeout-5){
                    RtspGetParameterCommand(sess);
                    playnow = now;
                }
                break;
            case RTSP_TEARDOWN:
                if (False == RtspTeardownCommand(sess)){
                    fprintf(stderr, "Error: RtspTeardownCommand.\n");
                    return False;
                }
                sess->status = RTSP_QUIT;
                break;
            default:
                printf("unkown status!\n");
                break;
        }
        usleep(100000);
    }while(0);

    return True;
}

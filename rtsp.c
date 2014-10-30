#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "rtspType.h"
#include "utils.h"
#include "rtsp.h"
#include "net.h"


static int32_t ParseUdpPort(char *buf, uint32_t size, RtspSession *sess);
inline static void RtspIncreaseCseq(RtspSession *sess);
inline static void RtspIncreaseCseq(RtspSession *sess)
{
    sess->cseq++;
    return;
}

int32_t RtspResponseStatus(char *response, char **error)
{
    int32_t size = 256, err_size = 0x00;
    int32_t offset = sizeof(RTSP_RESPONSE) - 1;
    char buf[8], *sep = NULL, *eol = NULL;
    *error = NULL;

    if (strncmp((const char*)response, (const char*)RTSP_RESPONSE, offset) != 0) {
        *error = calloc(1, size);
        snprintf((char *)*error, size, "Invalid RTSP response format");
        return -1;
    }

    sep = strchr((const char *)response+offset, ' ');
    if (!sep) {
        *error = calloc(1, size);
        snprintf((char *)*error, size, "Invalid RTSP response format");
        return -1;
    }

    memset(buf, '\0', sizeof(buf));
    strncpy((char *)buf, (const char *)(response+offset), sep-response-offset);

    eol = strchr(response, '\r');
    err_size = (eol - response) - offset - 1 - strlen(buf);
    *error = calloc(1, err_size + 1);
    if (NULL == *error){
        fprintf(stderr, "%s: Error calloc\n", __func__);
        return -1;
    }
    strncpy(*error, response + offset + 1 + strlen(buf), err_size);

    return atoi(buf);
}


int32_t RtspOptionsMsg(RtspSession *sess)
{
    int32_t num;
    int32_t ret = True;
    int32_t status;
    int32_t size = 4096;
    char *err = NULL;
    char buf[size];
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("OPTIONS: command\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = snprintf(buf, size, CMD_OPTIONS, sess->ip, sess->port, sess->cseq);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = RtspTcpSendMsg(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }
#ifdef RTSP_DEBUG
    printf("OPTIONS: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = RtspTcpRcvMsg(sock, buf, size-1);
    if (num <= 0) {
        printf("Error: Server did not respond properly, closing...");
        return False;
    }

    status = RtspResponseStatus(buf, &err);
    if (status == 200){
        printf("OPTIONS: response status %i (%i bytes)\n", status, num);
    }
    else{
        printf("OPTIONS: response status %i: %s\n", status, err);
        ret = False;
    }
    if (NULL != err)
        free(err);
    RtspIncreaseCseq(sess);

    return ret;
}

static void GetSdpVideoAcontrol(char *buf, uint32_t size, RtspSession *sess)
{
    char *ptr = (char *)memmem((const void*)buf, size,
            (const void*)SDP_M_VIDEO, strlen(SDP_M_VIDEO)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: m=video not found!\n");
        return;
    }

    ptr = (char *)memmem((const void*)ptr, size,
            (const void*)SDP_A_CONTROL, strlen(SDP_A_CONTROL)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: a=control not found!\n");
        return;
    }

    char *endptr = (char *)memmem((const void*)ptr, size,
            (const void*)"\r\n", strlen("\r\n")-1);
    if (NULL == endptr){
        fprintf(stderr, "Error: %s not found!\n", "\r\n");
        return;
    }
    ptr += strlen(SDP_A_CONTROL);
    if ('*' == *ptr){
        /* a=control:* */
        printf("a=control:*\n");
        return;
    }else{
        /* a=control:rtsp://ip:port/track1  or a=control : TrackID=1*/
        memcpy((void *)sess->vmedia.control, (const void*)(ptr), (endptr-ptr));
        sess->vmedia.control[endptr-ptr] = '\0';
    }

    return;
}

static void GetSdpVideoTransport(char *buf, uint32_t size, RtspSession *sess)
{
    char *ptr = (char *)memmem((const void*)buf, size,
            (const void*)SDP_M_VIDEO, strlen(SDP_M_VIDEO)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: m=video not found!\n");
        return;
    }

    ptr = (char *)memmem((const void*)ptr, size,
            (const void*)UDP_TRANSPORT, strlen(UDP_TRANSPORT)-1);
    if (NULL != ptr){
        sess->trans = RTP_AVP_UDP;
    }else{
        ptr = (char *)memmem((const void*)ptr, size,
                (const void*)TCP_TRANSPORT, strlen(TCP_TRANSPORT)-1);
        if (NULL != ptr)
            sess->trans = RTP_AVP_TCP;
    }

    return;
}

static int32_t ParseSdpProto(char *buf, uint32_t size, RtspSession *sess)
{
    GetSdpVideoTransport(buf, size, sess);
    GetSdpVideoAcontrol(buf, size, sess);
#ifdef RTSP_DEBUG
    printf("video control: %s\n", sess->vmedia.control);
#endif
    return True;
}

int32_t RtspDescribeMsg(RtspSession *sess)
{
    int32_t num;
    int32_t ret = True;
    int32_t status;
    int32_t size = 4096;
    char *err;
    char buf[size];
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("DESCRIBE: command\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = snprintf(buf, size, CMD_DESCRIBE, sess->url, sess->cseq);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }

    num = RtspTcpSendMsg(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

#ifdef RTSP_DEBUG
    printf("DESCRIBE: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = RtspTcpRcvMsg(sock, buf, size-1);
    if (num <= 0) {
        printf("Error: Server did not respond properly, closing...");
        return False;
    }


    status = RtspResponseStatus(buf, &err);
    if (status == 200) {
        printf("DESCRIBE: response status %i (%i bytes)\n", status, num);
    }
    else {
        printf("DESCRIBE: response status %i: %s\n", status, err);
        ret = False;
    }
    if (NULL != err)
        free(err);

    ParseSdpProto(buf, num, sess);
    RtspIncreaseCseq(sess);

    return ret;
}

static int32_t ParseTransport(char *buf, uint32_t size, RtspSession *sess)
{
    char *p = strstr(buf, "Transport: ");
    if (!p) {
        printf("SETUP: Error, Transport header not found\n");
        return False;
    }

    char *sep = strchr(p, ';');
    if (NULL == sep)
        return False;
    return True;
}

static int32_t ParseUdpPort(char *buf, uint32_t size, RtspSession *sess)
{
    /* Session ID */
    char *p = strstr(buf, SETUP_CPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_CPORT);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    char *sep = strchr((const char *)p, '-');
    if (NULL == sep){
        printf("SETUP: ; not found\n");
        return False;
    }
    char tmp[8] = {0x00};
    strncpy(tmp, p+1, sep-p-1);
    sess->transport.udp.cport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));


    p = strchr((const char *)sep, ';');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    strncpy(tmp, sep+1, p-sep-1);
    sess->transport.udp.cport_to = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));


    p = strstr(buf, SETUP_SPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_SPORT);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    sep = strchr((const char *)p, '-');
    if (NULL == sep){
        printf("SETUP: - not found\n");
        return False;
    }
    strncpy(tmp, p+1, sep-p-1);
    sess->transport.udp.sport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));


    p = strstr((const char *)sep, "\r\n");
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    strncpy(tmp, sep+1, p-sep-1);
    sess->transport.udp.sport_to = atol(tmp);
#ifdef RTSP_DEBUG
    printf("client port from %d to %d\n", \
            sess->transport.udp.cport_from, \
            sess->transport.udp.cport_to);
    printf("server port from %d to %d\n", \
            sess->transport.udp.sport_from, \
            sess->transport.udp.sport_to);
#endif
    return True;
}

static int32_t ParseSessionID(char *buf, uint32_t size, RtspSession *sess)
{
    /* Session ID */
    char *p = strstr(buf, SETUP_SESSION);
    if (!p) {
        printf("SETUP: Session header not found\n");
        return False;
    }
    char *sep = strchr((const char *)p, ';');
    char *sep2 = strstr((const char *)p, "\r\n");
    if ((NULL == sep) || (sep-sep2 > 0x00))
        sep = sep2;
    memset(sess->sessid, '\0', sizeof(sess->sessid));
    strncpy(sess->sessid, p+sizeof(SETUP_SESSION)-1, sep-p-sizeof(SETUP_SESSION)+1);
#ifdef RTSP_DEBUG
    printf("sessid : %s\n", sess->sessid);
#endif
    return True;
}

int32_t RtspSetupMsg(RtspSession *sess)
{
    int32_t num, ret = True, status;
    int32_t size = 4096;
    char *err;
    char buf[size];
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("SETUP: command\n");
#endif

    memset(buf, '\0', sizeof(buf));
    char url[256];
    memset(url, '\0', sizeof(url));
    if (NULL == strstr(sess->vmedia.control, PROTOCOL_PREFIX)){
        strncpy(url, sess->url, strlen(sess->url));
    }
    strncat(url, sess->vmedia.control, strlen(sess->vmedia.control));
#ifdef RTSP_DEBUG
    printf("SETUP URL: %s\n", url);
#endif
    if (RTP_AVP_TCP == sess->trans){
        num = snprintf(buf, size, CMD_TCP_SETUP, url, sess->cseq);
    }else if (RTP_AVP_UDP == sess->trans){
        num = snprintf(buf, size, CMD_TCP_SETUP, url, sess->cseq);
        /*num = snprintf(buf, size, CMD_UDP_SETUP, sess->url, sess->cseq, 10000, 10001);*/
    }
#ifdef RTSP_DEBUG
    printf("setup cmd : %s\n", buf);
#endif
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = RtspTcpSendMsg(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

#ifdef RTSP_DEBUG
    printf("SETUP: request sent\n");
#endif
    memset(buf, '\0', sizeof(buf));
    num = RtspTcpRcvMsg(sock, buf, size-1);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

    status = RtspResponseStatus(buf, &err);
    if (status == 200) {
        printf("SETUP: response status %i (%i bytes)\n", status, num);
    }
    else {
        printf("SETUP: response status %i: %s\n", status, err);
        return False;
    }

    if (RTP_AVP_UDP == sess->trans){
        ParseUdpPort(buf, num, sess);
    }else{
    }
    ParseSessionID(buf, num, sess);
    sess->packetization = 1;
    RtspIncreaseCseq(sess);
    return ret;
}

int32_t RtspPlayMsg(RtspSession *sess)
{
    int32_t num, ret = True, status, size=4096;
    char  *err, buf[size];
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("PLAY: command\n");
#endif
    memset(buf, '\0', sizeof(buf));
    printf("url : %s\n", sess->url);
    num = snprintf(buf, size, CMD_PLAY, sess->url, sess->cseq, sess->sessid);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = RtspTcpSendMsg(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

#ifdef RTSP_DEBUG
    printf("PLAY: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = RtspTcpRcvMsg(sock, buf, size-1);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

    status = RtspResponseStatus(buf, &err);
    if (status == 200) {
        printf("PLAY: response status %i (%i bytes)\n", status, num);
    }
    else {
        fprintf(stderr, "PLAY: response status %i: %s\n", status, err);
        ret = False;
    }

    RtspIncreaseCseq(sess);
    return ret;
}


int32_t RtspTeardownMsg(RtspSession *sess)
{
    int32_t num, ret = True, status, size=4096;
    char  *err, buf[size];
    int32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("TEARDOWN: command\n");
#endif
    memset(buf, '\0', sizeof(buf));
    num = snprintf(buf, size, CMD_TEARDOWN, sess->url, sess->cseq, sess->sessid);
    if (num < 0x00){
        fprintf(stderr, "%s : snprintf error!\n", __func__);
        return False;
    }
    num = RtspTcpSendMsg(sock, buf, (uint32_t)num);
    if (num < 0){
        fprintf(stderr, "%s : Send Error\n", __func__);
        return False;
    }

#ifdef RTSP_DEBUG
    printf("TEARDOWN: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    num = RtspTcpRcvMsg(sock, buf, size-1);
    if (num <= 0) {
        fprintf(stderr, "Error: Server did not respond properly, closing...");
        return False;
    }

    status = RtspResponseStatus(buf, &err);
    if (status == 200) {
        printf("TEARDOWN: response status %i (%i bytes)\n", status, num);
    }
    else {
        fprintf(stderr, "TEARDOWN: response status %i: %s\n", status, err);
        ret = False;
    }

    RtspIncreaseCseq(sess);
    return ret;
}

int32_t RtspStatusMachine(RtspSession *sess)
{
    do{
        switch(sess->status){
            case RTSP_START:
                if (False == RtspOptionsMsg(sess)){
                    fprintf(stderr, "Error: RtspOptionsMsg.\n");
                    return False;
                }
                sess->status = RTSP_OPTIONS;
                break;
            case RTSP_OPTIONS:
                if (False == RtspDescribeMsg(sess)){
                    fprintf(stderr, "Error: RtspDescribeMsg.\n");
                    return False;
                }
                sess->status = RTSP_DESCRIBE;
                break;
            case RTSP_DESCRIBE:
                if (False == RtspSetupMsg(sess)){
                    fprintf(stderr, "Error: RtspSetupMsg.\n");
                    return False;
                }
                sess->status = RTSP_SETUP;
                break;
            case RTSP_SETUP:
                if (False == RtspPlayMsg(sess)){
                    fprintf(stderr, "Error: RtspPlayMsg.\n");
                    return False;
                }
                sess->status = RTSP_PLAY;
                break;
            case RTSP_TEARDOWN:
                if (False == RtspTeardownMsg(sess)){
                    fprintf(stderr, "Error: RtspTeardownMsg.\n");
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

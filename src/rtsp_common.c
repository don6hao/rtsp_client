#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "rtsp_common.h"

static CmdTbl gcmdtbl[]={{"OPTIONS", 0},
                        {"DESCRIBE", 2},
                        {"SETUP", 4},
                        {"PLAY", 8},
                        {"PAUSE", 16},
                        {"GET_PARAMETER", 32},
                        {"SET_PARAMETER", 64},
                        {"REDIRECT", 128},
                        {"TEARDOWN", 256},
                        {"", -1}};

static uint32_t GetCmdTblKey(char *cmd)
{
    int32_t size = sizeof(gcmdtbl)/sizeof(CmdTbl);
    uint32_t i = 0x00;

    for (; i < size; i++){
        if (strncmp(gcmdtbl[i].cmd, cmd, strlen(gcmdtbl[i].cmd)) == 0){
            return gcmdtbl[i].key;
        }
    }
    return 0x00;
}

int32_t RtspCommandIsSupported(int32_t key, RtspSession *sess)
{
#ifdef RTSP_DEBUG
    printf("cmd stats : %d, key : %d\n", sess->cmdstats, key);
#endif
    if ((0x01 == (sess->cmdstats&0x01)) || (0x01 == (key&0x01)))
        return False;

    if ((key & sess->cmdstats) > 0x01)
        return True;
    return False;
}

void ParseOptionsPublic(char *buf, uint32_t size, RtspSession *sess)
{
    char *p = strstr(buf, OPTIONS_PUBLIC);
    if (NULL == p) {
        printf("SETUP: %s not found\n", SETUP_CPORT);
        return;
    }
    p += strlen(OPTIONS_PUBLIC);
    char *ptr = p;
    char tmp[32] = {0x00};
    do{
        memset(tmp, 0x00, sizeof(tmp));
        if (*ptr == ','){
            strncpy(tmp, p, ptr-p);
            tmp[ptr-p]='\0';
            p = ptr+1;
            sess->cmdstats += GetCmdTblKey(tmp);
        }else if (*ptr == '\r'){
            strncpy(tmp, p, ptr-p);
            tmp[ptr-p]='\0';
            break;
        }
        ptr++;
    }while(1);
    sess->cmdstats += GetCmdTblKey(tmp);
#ifdef RTSP_DEBUG
    printf("cmd stats : %d\n", sess->cmdstats);
#endif
    return;
}


static void GetClientPort(char *buf, uint32_t size, RtspSession *sess)
{
    char *p = strstr(buf, SETUP_CPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_CPORT);
        return;
    }
    p += strlen(SETUP_CPORT);

    char *ptr = p;
    do{
        if (*ptr == '-'){
            break;
        }
        ptr++;
    }while(1);

    char tmp[8] = {0x00};
    strncpy(tmp, p, ptr-p);
    sess->transport.udp.cport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));
    ptr++;

    p = ptr;
    do{
        if (*ptr == ';' || *ptr == '\r'){
            break;
        }
        ptr++;
    }while(1);
    strncpy(tmp, p, ptr-p);
    sess->transport.udp.cport_to = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));

    return;
}

static void GetServerPort(char *buf, uint32_t size, RtspSession *sess)
{
    char tmp[8] = {0x00};
    memset(tmp, 0x00, sizeof(tmp));
    char *p = strstr(buf, SETUP_SPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_SPORT);
        return;
    }
    p += strlen(SETUP_SPORT);
    char *ptr = p;
    do{
        if (*ptr == '-'){
            break;
        }
        ptr++;
    }while(1);
    strncpy(tmp, p, ptr-p);
    sess->transport.udp.sport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));
    ptr++;

    p=ptr;
    do{
        if (*ptr == ';' || *ptr == '\r'){
            break;
        }
        ptr++;
    }while(1);
    strncpy(tmp, p, ptr-p);
    sess->transport.udp.sport_to = atol(tmp);

    return;
}

int32_t ParseUdpPort(char *buf, uint32_t size, RtspSession *sess)
{
    GetClientPort(buf, size, sess);
    GetServerPort(buf, size, sess);

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

int32_t ParseTimeout(char *buf, uint32_t size, RtspSession *sess)
{
    char *p = strstr(buf, TIME_OUT);
    if (!p) {
        printf("GET_PARAMETER: %s not found\n", TIME_OUT);
        return False;
    }
    p += strlen(TIME_OUT);
    char *ptr = p;
    do{
        if (*ptr == ';' || *ptr == '\r'){
            break;
        }
        ptr++;
    }while(1);

    char tmp[8] = {0x00};
    strncpy(tmp, p, ptr-p);
    sess->timeout = atol(tmp);
#ifdef RTSP_DEBUG
    printf("timeout : %d\n", sess->timeout);
#endif
    return True;
}

int32_t ParseSessionID(char *buf, uint32_t size, RtspSession *sess)
{
    /* Session ID */
    char *ptr = strstr(buf, SETUP_SESSION);
    if (!ptr) {
        printf("SETUP: %s not found\n", SETUP_SESSION);
        return False;
    }
    ptr += strlen(SETUP_SESSION);
    char *p = ptr;
    do{
        if (*p == ';' || *p == '\r'){
            break;
        }
        p++;
    }while(1);

    memset(sess->sessid, '\0', sizeof(sess->sessid));
    memcpy((void *)sess->sessid, (const void *)ptr, p-ptr);
#ifdef RTSP_DEBUG
    printf("sessid : %s\n", sess->sessid);
#endif
    return True;
}


int32_t ParseInterleaved(char *buf, uint32_t num, RtspSession *sess)
{
    char *p = strstr(buf, TCP_INTERLEAVED);
    if (!p) {
        printf("SETUP: %s not found\n", TCP_INTERLEAVED);
        return False;
    }

    p += strlen(TCP_INTERLEAVED);
    char *ptr = p;
    do{
        if (*ptr == '-'){
            break;
        }
        ptr++;
    }while(1);

    char tmp[8] = {0x00};
    strncpy(tmp, p, ptr-p);
    sess->transport.tcp.start = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));
    ptr++;

    p = ptr;
    do{
        if (*ptr == ';' || *ptr == '\r'){
            break;
        }
        ptr++;
    }while(1);
    strncpy(tmp, p, ptr-p);
    sess->transport.udp.cport_to = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));

#ifdef RTSP_DEBUG
    printf("tcp interleaved from %d to %d\n", \
            sess->transport.tcp.start, \
            sess->transport.tcp.end);
#endif
    return True;
}


void RtspIncreaseCseq(RtspSession *sess)
{
    sess->cseq++;
    return;
}

void GetSdpVideoAcontrol(char *buf, uint32_t size, RtspSession *sess)
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

void GetSdpVideoTransport(char *buf, uint32_t size, RtspSession *sess)
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

int32_t ParseSdpProto(char *buf, uint32_t size, RtspSession *sess)
{
    GetSdpVideoTransport(buf, size, sess);
    GetSdpVideoAcontrol(buf, size, sess);
#ifdef RTSP_DEBUG
    printf("video control: %s\n", sess->vmedia.control);
#endif
    return True;
}



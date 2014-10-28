#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rtspClient.h"
#include "net.h"

uint32_t ParseUrl(int8_t *url, RtspClientSession *cses)
{
    uint32_t offset = sizeof(PROTOCOL_PREFIX) - 1;
    int8_t *pos = NULL, buf[8];
    uint32_t len = 0x00;

    RtspSession *sess = &cses->sess;
    //get host
    pos = strchr(url+offset, ':');
    if (NULL == pos){
        pos = strchr(url+offset, '/');
        if (NULL == pos)    return False;
    }
    len  = (pos-url)-offset;
    strncpy(sess->ip, url+offset, len);
    sess->ip[len] = '\0';


    //get port
    pos = strchr(url+offset, ':');
    if (NULL != pos){
        len = pos-url+1;
        pos = strchr(url+len, '/');
        if (NULL == pos)    return False;

        uint32_t size = (pos-url)-len;
        if (size > sizeof(buf)False){
            fprintf(stderr, "Error: Invalid port\n");
            return False;
        }
        pos = url+len; //指向:符号
        strncpy(buf, pos, size);
        sess->port = atol(buf); //Note测试port的范围大小
    }

    strncpy(sess->url, url, strlen(url) > sizeof(sess->url) ? sizeof(sess->url) : strlen(url));
#ifdef RTSP_DEBUG
    printf("Host|Port : %s:%d\n", sess->host, sess->port);
#endif

    return True;
}


int32_t RtspEventLoop(RtspClientSession *csess)
{
    RtspSession *sess = &csess->sess;
    int32_t fd = RtspTcpConnect(sess->ip, sess->port);
    if (fd <= 0x00){
        fprintf(stderr, "Error: RtspConnect.\n");
        return False;
    }

    sess->sockfd = fd;
    int32_t ret = RtspOptionsMsg(sess);
    if (False == ret){
        fprintf(stderr, "Error: RtspOptionsMsg.\n");
        return False;
    }

    ret = RtspDescribeMsg(sess);
    if (False == ret){
        fprintf(stderr, "Error: RtspDescribeMsg.\n");
        return False;
    }

    if (False == RtspSetupMsg(sess)){
        fprintf(stderr, "Error: RtspSetupMsg.\n");
        return False;
    }


    if (False == RtspPlayMsg(sess)){
        fprintf(stderr, "Error: RtspPlayMsg.\n");
        return False;
    }

    return True;
}

RtspClientSession* InitRtspClientSession()
{
    RtspClientSession *sess = (RtspClientSession *)calloc(1, sizeof(RtspClientSession));

    if (NULL == sess)
        return NULL;

    return sess;
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

#include "rtspClient.h"

void ParseUrl(int8_t *url, RtspClientSession *sess)
{
    uint32_t offset = sizeof(PROTOCOL+PREFIX) - 1;
    int8_t *pos = NULL;
    uint32_t len = 0x00;

    //get host
    pos = strchr(url+offset, ':');
    if (NULL == pos){
        pos = strchr(url+offset, '/');
        if (NULL == pos)    return False;
    }
    len  = (pos-url)-offset;
    strncpy(sess->host, url+offset, len);
    sess->host[len] = '\0';


    //get port
    pos = strchr(url+offset, ':');
    if (NULL != pos){
        len = pos-url+1;
        pos = strchr(url+len, '/');
        if (NULL == pos)    return False;

        uint32_t size = (pos-url)-len;
        if (size > sizeof(buf)-1){
            fprintf(stderr, "Error: Invalid port\n");
            return False;
        }
        pos = url+len; //指向:符号
        strncpy(buf, pos, size);
        sess->port = atol(buf); //Note测试port的范围大小
    }

#ifdef RTSP_DEBUG
    printf("Host|Port : %s:%d\n", sess->host, sess->port);
#endif

    return True;
}


int32_t RtspEventLoop(RtspClientSession *sess)
{
    int32_t fd = RtspTcpConnect(sess->url, sess->port);
    if (fd <= 0x00){
        fprintf(stderr, "Error: RtspConnect.\n");
        return -1;
    }

    sess->sockfd = fd;
    int32_t ret = RtspOptionsMsg(sess);
    if (0x00 != ret){
        fprintf(stderr, "Error: RtspOptionsMsg.\n");
        return -1;
    }

    ret = RtspDescribeMsg(sess);
    if (0x00 != ret){
        fprintf(stderr, "Error: RtspDescribeMsg.\n");
        return -1;
    }

    RtspSetupMsg();
    RtspPlayMsg();

    return 0x00;
}


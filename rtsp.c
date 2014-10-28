#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rtspClient.h"
#include "utils.h"
#include "rtsp.h"


inline static void RtspIncCseq(RtspClientSession *sess);
inline static void RtspIncCseq(RtspClientSession *sess)
{
    sess->cseq++;
    return;
}

int32_t RtspResponseStatus(char *response, char **error)
{
    int32_t size = 256, err_size = 0x00;
    int32_t offset = sizeof(RTSP_RESPONSE) - 1;
    int8_t buf[8], *sep = NULL, *eol = NULL;
    *error = NULL;

    if (strncmp(response, RTSP_RESPONSE, offset) != 0) {
        *error = malloc(size);
        snprintf(*error, size, "Invalid RTSP response format");
        return -1;
    }

    sep = strchr(response + offset, ' ');
    if (!sep) {
        *error = malloc(size);
        snprintf(*error, size, "Invalid RTSP response format");
        return -1;
    }

    memset(buf, '\0', sizeof(buf));
    strncpy(buf, response + offset, sep - response - offset);

    eol = strchr(response, '\r');
    err_size = (eol - response) - offset - 1 - strlen(buf);
    *error = calloc(1, err_size + 1);
    if (NULL == *error){
        fprintf(stderr, "%s: Error calloc\n", __func__);
        return = -1;
    }
    strncpy(*error, response + offset + 1 + strlen(buf), err_size);

    return atoi(buf);
}


int32_t RtspOptionsMsg(RtspClientSession *sess)
{
    int32_t n;
    int32_t ret = 0;
    int32_t status;
    int32_t size = 4096;
    int8_t *err = NULL;
    int8_t buf[size];
    uint32_t sock = sess->sockfd;

#ifdef RTSP_DEBUG
    printf("OPTIONS: command\n");
#endif

    memset(buf, '\0', sizeof(buf));
    n = snprintf(buf, size, CMD_OPTIONS, \
            sess->ip, sess->port, sess->cseq);
#ifdef RTSP_DEBUG
    DEBUG_REQ(buf);
#endif
    n = send(sock, buf, n, 0);
#ifdef RTSP_DEBUG
    printf("OPTIONS: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    n = recv(sock, buf, size - 1, 0);
    if (n <= 0) {
        printf("Error: Server did not respond properly, closing...");
        close(sock);
        return -1;
    }

    status = RtspResponseStatus(buf, &err);
    if (status == 200){
        printf("OPTIONS: response status %i (%i bytes)\n", status, n);
    }
    else{
        printf("OPTIONS: response status %i: %s\n", status, err);
        if (NULL != err)
            free(err);
        ret = -1;
    }
#ifdef RTSP_DEBUG
    DEBUG_RES(buf);
#endif
    RtspIncCseq(sess->cseq);

    return ret;
}

static int32_t ParseSDP(int8_t *buf, uint32_t size)
{
    int8_t p = strstr(buf, "\r\n\r\n");
    if (!p) {
        return -1;
    }

    /* Create buffer for DSP */
    dsp = calloc(1, n + 1);
    memset(dsp, '\0', n + 1);
    strcpy(dsp, p + 4);

    /* sprop-parameter-sets key */
    p = strstr(dsp, RTP_SPROP);
    if (!p) {
        return -1;
    }

    end = strchr(p, '\r');
    if (!end) {
        return -1;
    }

    int32_t prop_size = (end - p) - sizeof(RTP_SPROP) + 1;
    *sprop = malloc(prop_size + 1);
    memcpy(*sprop, p + sizeof(RTP_SPROP) - 1, prop_size);

    return;
}

int32_t RtspDescribeMsg(int sock, char *stream, char **sprop)
{
    int32_t n;
    int32_t ret = 0;
    int32_t status;
    int32_t size = 4096;
    int8_t *p, *end;
    int8_t *err;
    int8_t buf[size];

#ifdef RTSP_DEBUG
    RTSP_INFO("DESCRIBE: command\n");
#endif

    memset(buf, '\0', sizeof(buf));
    n = snprintf(buf, size, CMD_DESCRIBE, stream, rtsp_cseq);

#ifdef RTSP_DEBUG
    DEBUG_REQ(buf);
#endif
    n = send(sock, buf, n, 0);

#ifdef RTSP_DEBUG
    RTSP_INFO("DESCRIBE: request sent\n");
#endif

    memset(buf, '\0', sizeof(buf));
    n = recv(sock, buf, size - 1, 0);
    if (n <= 0) {
        printf("Error: Server did not respond properly, closing...");
        close(sock);
        return -1;
    }

    status = rtsp_response_status(buf, &err);
    if (status == 200) {
        RTSP_INFO("DESCRIBE: response status %i (%i bytes)\n", status, n);
    }
    else {
        RTSP_INFO("DESCRIBE: response status %i: %s\n", status, err);
        ret = -1;
    }

    ParseSDP(buf, n);
#ifdef RTSP_DEBUG
    DEBUG_RES("%s\n", buf);
#endif
    RtspIncCseq(sess->cseq);

    return ret;
}




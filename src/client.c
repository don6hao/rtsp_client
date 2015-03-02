#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>

#include "rtsp_type.h"
#include "rtsp_client.h"
#include "tpool.h"

static void help(int status);
static void help(int status)
{
    printf("Usage: rpsp_client -u rtsp://url\n\n");
    printf("  -u, --url=rtsp://      rtsp address\n");
	printf("  -h, --help             print this help\n");
	printf("\n");
	exit(status);
}

static int32_t quitflag = 0x00;
static void signal_handler(int signo)
{
    printf("catch signal NO. %d\n", signo);
    quitflag = 0x01;
    return;
}

static void signal_init()
{
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGQUIT, signal_handler);
    return;
}

int32_t main(int argc, char **argv)
{
    signal_init();

    int32_t opt;
    char *url = NULL;
    static const struct option long_opts[] = {
                            { "url", required_argument, NULL, 'u'},
                            { "help", no_argument, NULL, 'h'},
                            { NULL, 0, NULL, 0 }
                        };

    while ((opt = getopt_long(argc, argv, "u:h",
                       long_opts, NULL)) != -1) {
        switch (opt) {
            case 'u':
                if (NULL == (url  = strdup(optarg))){
                    fprintf(stderr, "Error : Url Address Equal Null.\n");
                    return 0x00;
                }
                break;
            case 'h':
                help(EXIT_SUCCESS);
                break;
            default:
                break;
        }
    }

    RtspClientSession *cses = InitRtspClientSession();
    if ((NULL == cses) || (False == ParseUrl(url, cses))){
        fprintf(stderr, "Error : Invalid Url Address.\n");
        return 0x00;
    }

    pthread_t rtspid = RtspCreateThread(RtspEventLoop, (void *)cses);
    if (rtspid < 0x00){
        fprintf(stderr, "RtspCreateThread Error!\n");
        return 0x00;
    }


    do{
        if (0x01 == quitflag){
            SetRtspClientSessionQuit(cses);
            TrykillThread(rtspid);
            break;
        }
        sleep(1);
    }while(1);

    printf("RTSP Event Loop stopped, waiting 5 seconds...\n");
    DeleteRtspClientSession(cses);
    return 0x00;
}


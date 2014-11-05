#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include "tpool.h"
#include "rtspType.h"

int32_t RtspCreateThread(FUNC func, void* args)
{
    pthread_t id;
    if (0x00 != pthread_create(&id, NULL, func, args)){
        fprintf(stderr, "pthread_create error!\n");
        return -1;
    }
    return id;
}

int32_t TrykillThread(pthread_t id)
{
    int32_t ret =  0x00;
    do{
        ret = pthread_kill(id, 0x00);
        if (ESRCH == ret){
            break;
        }
    }while(1);

    return True;
}



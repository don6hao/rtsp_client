#ifndef _TPOOL_H
#define _TPOOL_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef void *(*FUNC)(void *);

int32_t RtspCreateThread(FUNC func, void *args);
int32_t TrykillThread(pthread_t id);
#endif

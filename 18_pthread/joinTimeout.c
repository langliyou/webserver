#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#define OUTTIME 2

void * sonThread(void * arg)
{
    sleep(5);
    printf("%ld, get arg: %s\n", pthread_self(), (char*) arg);
    static char* rv = "exit normally";
    pthread_exit((void *)rv);
}

void * joinTimeOut(void *arg)
{
    pthread_t tid = *((pthread_t *)arg);
    struct timespec ts;
    int ret;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) 
    {
        perror("clock_gettime");
        pthread_cancel(tid);
        pthread_exit(NULL);
    }

    ts.tv_sec += OUTTIME;

    void *retval;
    ret = pthread_timedjoin_np(tid, &retval, &ts);
    printf("ret: %d", ret);
    if(ret == ETIMEDOUT)
    {
        ret = pthread_cancel(tid);
        if(ret == 0)
        {
            printf("%ld timeout of %d, so I(%ld) cancel it.\n", tid, OUTTIME, pthread_self());
        }
        else
        {
            perror("pthread_cancel");
        }
    }
    else 
    {
        printf("child exit, retval: %s", (char *)retval);
    }
    pthread_exit(NULL);
}
int main()
{
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, sonThread, (void *)"a great day");
    if(ret != 0)
    {
        strerror(ret);
        return 1;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t tid2;
    ret = pthread_create(&tid2, &attr, joinTimeOut, (void *)tid);
    if(ret != 0)
    {
        strerror(ret);
        return 1;
    }
    // ret = pthread_detach(tid);
    // void *retval;
    // ret = pthread_join(tid2, &retval);
    // printf("child exit, retval: %s\n", (char *)retval);
    pthread_attr_destroy(&attr);
    printf("%ld, main pthread_exit\n", pthread_self());
    pthread_exit(NULL);
    return 0;
}
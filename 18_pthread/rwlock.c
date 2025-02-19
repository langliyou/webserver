#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <c++/13/bits/ranges_util.h>
//场景: 8个线程同时操作一个全局变量; 3个不定时写、5个不定时读
#define RDNUM 5
#define WRNUM 3
int sharedInt = 10;
pthread_rwlock_t rwlock;

void *readSth(void *arg)
{
    pthread_t pid = pthread_self();
    int flag = 1;
    while(flag)
    {
        pthread_rwlock_rdlock(&rwlock);
        printf("%ld reading sharedInt: %d\n", pid, sharedInt);
        if(sharedInt == 0)
        {
            flag = 0;
        }
        pthread_rwlock_unlock(&rwlock);
        sleep(2);
    }
    pthread_exit(NULL);
}
void *writeSth(void *arg)
{
    pthread_t pid = pthread_self();
    int flag = 1;
    while(flag)
    {
        pthread_rwlock_wrlock(&rwlock);
        printf("%ld writing sharedInt: %d\n", pid, sharedInt);
        if(sharedInt == 0)
        {
            flag = 0;
        }
        else
        {
            --sharedInt;
        }
        pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
    pthread_exit(NULL);
}
int main()
{
    pthread_t tid_Rd[RDNUM], tid_Wr[WRNUM];
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//设置线程分离
    pthread_rwlockattr_t rwAttr;
    pthread_rwlockattr_init(&rwAttr);
    // pthread_rwlockattr_setkind_np(&rwAttr, PTHREAD_RWLOCK_PREFER_READER_NP);//设置读优先
    pthread_rwlockattr_setkind_np(&rwAttr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);//设置读写公平

    pthread_rwlock_init(&rwlock, &rwAttr);
    for(int i = 0; i < RDNUM; ++i)
    {
        int ret = pthread_create(&tid_Rd[i], NULL, readSth, NULL);
    }
    for(int i = 0; i < WRNUM; ++i)
    {
        int ret = pthread_create(&tid_Wr[i], NULL, writeSth, NULL);
    }
    // pthread_attr_destroy(&attr);
    for(int i = 0; i < RDNUM; ++i)
    {
        int ret = pthread_join(tid_Rd[i], NULL);
    }
    for(int i = 0; i < WRNUM; ++i)
    {
        int ret = pthread_join(tid_Wr[i], NULL);
    }
    pthread_rwlock_destroy(&rwlock);
    pthread_rwlockattr_destroy(&rwAttr);
    printf("%ld, main pthread_exit\n", pthread_self());
    pthread_exit(NULL);
    return 0;
}
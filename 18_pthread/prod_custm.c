#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <c++/13/bits/ranges_util.h>
// #include <./node.h>
//场景: 10个线程同时操作一个链表; 5个不定时生产、5个不定时消费; 10个就满了 阻塞生产
#define CUSNUM 5
#define PRONUM 5
#define MAXCNT 5

// Node* dummy;
pthread_mutex_t cusMtx;
pthread_mutex_t proMtx;
pthread_cond_t cusCond;
pthread_cond_t proCond;
int cnt = 0;

void *customer(void *arg)
{
    pthread_t tid = pthread_self();
    int i = 10;
    while(i--)
    {
        pthread_mutex_lock(&cusMtx);
        while(cnt == 0)//取决于编译器的实现, 这里最好用while避免过期条件
        {
            pthread_cond_wait(&cusCond, &cusMtx);
        }
        printf("%ld get one, now cnt = %d\n", tid, --cnt);
        pthread_mutex_unlock(&cusMtx);
        pthread_cond_signal(&proCond);
        //消费结点
        printf("%ld consuming...\n", tid);
         
    }
    printf("customer thread %ld exit.\n", tid);
    pthread_exit(NULL);
}
void *producer(void *arg)
{
    pthread_t tid = pthread_self();
    int i = 10;
    while(i--)
    {
        printf("%ld producing...\n", tid);
        pthread_mutex_lock(&proMtx);
        while(cnt == MAXCNT)//取决于编译器的实现, 这里最好用while避免过期条件
        {
            pthread_cond_wait(&proCond, &proMtx);
        }
        printf("%ld put one, now cnt = %d\n", tid, ++cnt);
        pthread_mutex_unlock(&proMtx);
        pthread_cond_signal(&cusCond);

    }
    printf("producer thread %ld exit.\n", tid);
    pthread_exit(NULL);
}
int main()
{
    pthread_t tid_cus[CUSNUM], tid_pro[PRONUM];
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//设置线程分离
    pthread_mutex_init(&cusMtx, NULL);
    pthread_cond_init(&cusCond, NULL);
    pthread_mutex_init(&proMtx, NULL);
    pthread_cond_init(&proCond, NULL);
    for(int i = 0; i < CUSNUM; ++i)
    {
        int ret = pthread_create(&tid_cus[i], NULL, customer, NULL);
    }
    for(int i = 0; i < PRONUM; ++i)
    {
        int ret = pthread_create(&tid_pro[i], NULL, producer, NULL);
    }
    // pthread_attr_destroy(&attr);
    for(int i = 0; i < CUSNUM; ++i)
    {
        int ret = pthread_join(tid_cus[i], NULL);
    }
    for(int i = 0; i < PRONUM; ++i)
    {
        int ret = pthread_join(tid_pro[i], NULL);
    }
    pthread_mutex_destroy(&cusMtx);
    pthread_cond_destroy(&cusCond);
    pthread_mutex_destroy(&proMtx);
    pthread_cond_destroy(&proCond);
    printf("%ld, main pthread_exit\n", pthread_self());
    pthread_exit(NULL);
    return 0;
}
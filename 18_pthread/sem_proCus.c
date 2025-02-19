#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <c++/13/bits/ranges_util.h>
#include <semaphore.h>
// #include <./node.h>
//场景: 10个线程同时操作一个链表; 5个不定时生产、5个不定时消费; 10个就满了 阻塞生产
#define CUSNUM 5
#define PRONUM 5
#define MAXCNT 5

// Node* dummy;
sem_t sem_vacCnt;
sem_t sem_srcCnt;
pthread_mutex_t mtx_nodeList;//进入临界区, 实际操作资源的时候要上锁 这里用printf模拟

void *customer(void *arg)
{
    pthread_t tid = pthread_self();
    int i = 10;
    while(i--)
    {
        sem_wait(&sem_srcCnt);
        pthread_mutex_lock(&mtx_nodeList);
        int cnt;
        sem_getvalue(&sem_srcCnt, &cnt);
        printf("%ld get one, now cnt = %d\n", tid, cnt);
        pthread_mutex_unlock(&mtx_nodeList);
        sem_post(&sem_vacCnt);
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
        //生产结点
        printf("%ld producing...\n", tid);
        sem_wait(&sem_vacCnt);
        pthread_mutex_lock(&mtx_nodeList);
        int vacancyCnt;
        sem_getvalue(&sem_vacCnt, &vacancyCnt);
        printf("%ld put one, now cnt = %d\n", tid, MAXCNT - vacancyCnt);
        pthread_mutex_unlock(&mtx_nodeList);
        sem_post(&sem_srcCnt);
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
    pthread_mutex_init(&mtx_nodeList, NULL);
    sem_init(&sem_vacCnt, 0, MAXCNT);
    sem_init(&sem_srcCnt, 0, 0);
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
    sem_destroy(&sem_vacCnt);
    sem_destroy(&sem_srcCnt);
    pthread_mutex_destroy(&mtx_nodeList);
    printf("%ld, main pthread_exit\n", pthread_self());
    pthread_exit(NULL);
    return 0;
}
#pragma once

#include <pthread.h>
#include <semaphore.h>

class Mutex
{
public:
    Mutex();
    bool lock();
    bool unlock();
    ~Mutex();
    pthread_mutex_t* get();
private:
    pthread_mutex_t m_mutex;
};

class Cond
{
public:
    Cond();
    bool wait(pthread_mutex_t* mutex);
    bool timedwait(pthread_mutex_t *mutex, const struct timespec *abstime);
    bool signal(pthread_mutex_t* mutex);
    bool broadcast(pthread_mutex_t* mutex);
    ~Cond();
private:
    pthread_cond_t m_cond;
};

class Sem
{
public:
    Sem(unsigned int value);
    Sem(int pshared, unsigned int value);
    ~Sem();
    bool wait();
    bool timedwait(const struct timespec * abs_timeout);
    bool post();
    int getvalue();
private:
    sem_t m_sem;
};
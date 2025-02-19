#include "concurrent.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>

Mutex::Mutex()
{
    pthread_mutex_init(&m_mutex, NULL);
}

bool Mutex::lock()
{
    int ret;
    if((ret = pthread_mutex_lock(&m_mutex)) != 0)
    {
        if(ret == EDEADLK)
        {
            fprintf(stderr, "pthread_mutex_lock: EDEADLK--the mutex is already locked by the calling thread.\n");
        }
        return false;
    }
    return true;
}

bool Mutex::unlock()
{
    int ret;
    if((ret = pthread_mutex_unlock(&m_mutex)) != 0)
    {
        if(ret == EPERM)
        {
            fprintf(stderr, "pthread_mutex_unlock: EPERM-- the calling thread does not own the mutex.\n");
        }
        return false;
    }
    return true;
}

pthread_mutex_t* Mutex::get()
{
    return &m_mutex;
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mutex);
}

//----------------------------------------------------------
Cond::Cond()
{
    pthread_cond_init(&m_cond, NULL);
}

bool Cond::wait(pthread_mutex_t* mutex)
{
    return pthread_cond_wait(&m_cond, mutex) == 0;
}

bool Cond::timedwait(pthread_mutex_t *mutex, const struct timespec *abstime)
{
    int ret;
    if((ret = pthread_cond_timedwait(&m_cond, mutex, abstime)) != 0)
    {
        if(ret == ETIMEDOUT)
        {
            fprintf(stderr, "pthread_cond_timedwait: ETIMEDOUT-- the condition variable was not signaled until the timeout specified by abstime.\n");
        }
        else if(ret == EINTR)
        {
            fprintf(stderr, "pthread_cond_timedwait was interrupted by a signal.\n");
        }
        return false;
    }
    return true;
}

bool Cond::signal(pthread_mutex_t* mutex)
{
    return pthread_cond_signal(&m_cond) == 0;
}

bool Cond::broadcast(pthread_mutex_t* mutex)
{
    return pthread_cond_broadcast(&m_cond) == 0;
}

Cond::~Cond()
{
    pthread_cond_destroy(&m_cond);
}
//----------------------------------------------------------

Sem::Sem(unsigned int value)
{
    Sem(0, value);
}

Sem::Sem(int pshared, unsigned int value)
{
    sem_init(&m_sem, pshared, value);
}

Sem::~Sem()
{
    if(sem_destroy(&m_sem) != 0)
    {
        perror("sem_destroy");
    }
}

bool Sem::wait()
{
    int ret;
    if((ret = sem_wait(&m_sem) != 0))
    {
        // if(ret == EINTR)
        // {
        //     fprintf(stderr, "sem_wait: EINTR--The call was interrupted by a signal handler.\n");
        // }
        // else if(ret == EINVAL)
        // {
        //     fprintf(stderr, "sem_wait: EINVAL--sem is not a valid semaphore.\n");
        // }
        perror("sem_timedwait");
        return false;
    }
    return true;
}

bool Sem::timedwait(const struct timespec * abs_timeout)
{
    int ret;
    if((ret = sem_timedwait(&m_sem, abs_timeout)) != 0)
    {
        // if(ret == EINTR)
        // {
        //     fprintf(stderr, "sem_wait: EINTR--The call was interrupted by a signal handler.\n");
        // }
        // else if(ret == EINVAL)
        // {
        //     fprintf(stderr, "sem_wait: EINVAL--The value of abs_timeout.tv_nsecs is less than 0, or greater than or equal to 1000 million.\n");
        // }
        // else if(ret == ETIMEDOUT)
        // {
        //     fprintf(stderr, "sem_wait: ETIMEDOUT--The call timed out before the semaphore could be locked.\n");
        // }
        perror("sem_timedwait");
        return false;
    }
    return true;
}

bool Sem::post()
{
    int ret;
    if((ret = sem_post(&m_sem)) != 0)
    {
        perror("sem_getvalue");
        return false;
    }
    return true;
}

int Sem::getvalue()
{
    int value;
    int ret;
    if((ret = sem_getvalue(&m_sem, &value))!= 0)
    {
        perror("sem_getvalue");
        return -1;
    }
    return value;
}

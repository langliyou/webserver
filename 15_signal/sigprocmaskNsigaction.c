#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

#define INTV 1
void handler(int num)
{
    printf("capture signal %d\n", num);
}
int main()
{
    //一开始阻塞SIGALRM
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
    //设置handler
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);//清空 
    int retSig = sigaction(SIGALRM, &act, NULL);
    if(retSig == 0)
    {
        printf("set sigaction ok.\n");
    }
    else if(retSig == -1)
    {
        perror("sigaction");
        return 1;
    }


    struct itimerval new_val;
    new_val.it_interval.tv_sec = INTV;
    new_val.it_interval.tv_usec = 0;
    new_val.it_value.tv_sec = 2;
    new_val.it_value.tv_usec = 0;
    int ret = setitimer(ITIMER_REAL, &new_val, NULL);
    if(ret == 0)
    {
        printf("setitimer ok.\n");
    }
    else if(ret == -1)
    {
        perror("setitimer\n");
        return 1;
    }
    while(1)
    {
        ret = getchar();
        if(ret == '0')
        {
            sigset_t uset;
            sigemptyset(&uset);
            sigaddset(&uset, SIGALRM);
            sigprocmask(SIG_UNBLOCK, &uset, NULL);
        }
        else if(ret == '1')
        {
            return 0;
        }
    }
    return 0;
}
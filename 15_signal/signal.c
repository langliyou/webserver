#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
void alarmHandler(int num)
{
    printf("capture signal %d\n", num);
}
int main()
{
    signal(SIGALRM, alarmHandler);
    struct itimerval new_val;
    new_val.it_interval.tv_sec = 2;
    new_val.it_interval.tv_usec = 0;
    new_val.it_value.tv_sec = 3;
    new_val.it_value.tv_usec = 0;
    int ret = setitimer(ITIMER_REAL, &new_val, NULL);
    if(ret == 0)
    {
        printf("setitimer ok.\n");
    }
    else if(ret == -1)
    {
        perror("setitimer");
        return 1;
    }
    getchar();
    return 0;
}
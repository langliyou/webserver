#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
void timeInfo(int num)
{
    time_t t = time(NULL);
    struct tm * loc = localtime(&t);
    char * str = asctime(loc);
    int fd = open("timeInfo.txt", O_RDWR | O_APPEND | O_CREAT, 0664);
    if(fd == -1)
    {
        perror("open");
        return;
    }
    write(fd, str, strlen(str));
    close(fd);
}

int main() 
{    
    int pid = fork();
    if(pid > 0) return 0;
    int sid = setsid();
    umask(022);
    int retDir = chdir("/home/lly/Documents/");
    if(retDir == -1)
    {
        perror("retDir");
        return 1;
    }
    int nullFd = open("/dev/null", O_RDWR);
    dup2(nullFd, 0);
    dup2(nullFd, 1);
    dup2(nullFd, 2);
    close(nullFd);

    //设置定时器 打印时间信息
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = timeInfo;
    sigemptyset(&act.sa_mask);//清空 
    sigaction(SIGALRM, &act, NULL);
    struct itimerval new_val;
    new_val.it_interval.tv_sec = 2;
    new_val.it_interval.tv_usec = 0;
    new_val.it_value.tv_sec = 1;
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
    while(1)
    {
        sleep(60);
    }
    return 0;
}
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

#define INTV 1
void waitChildren(int num)
{
    while(1)
    {
        int ret = waitpid(-1, NULL, WNOHANG);
        if(ret == 0) break;
        if(ret == -1)
        {
            perror("waitpid");
            return ;
        }
        printf("waitpid %d\n", ret);
    }
}
int main()
{
    // //一开始阻塞SIGALRM
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, NULL);
    int pid;
    for(int i = 0; i < 10; ++i)
    {
        pid = fork();
        if(pid == 0) break;
    }
    if(pid == -1)
    {
        perror("fork");
        return 1;
    }
    if(pid == 0)
    {
        printf("child %d\n", getpid());
        sleep(2);
        return 0;
    }
    //设置handler
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = waitChildren;
    sigemptyset(&act.sa_mask);//清空 
    int retSig = sigaction(SIGCHLD, &act, NULL);
    if(retSig == 0)
    {
        printf("set sigaction ok.\n");
    }
    else if(retSig == -1)
    {
        perror("sigaction");
        return 1;
    }
     //解除阻塞SIGALRM
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    while(1)
    {
        printf("do my business...\n");
        sleep(1);
    }
    return 0;
}
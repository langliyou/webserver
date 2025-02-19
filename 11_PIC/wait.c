#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
int NUM = 5;
int main()
{
    pid_t pid;
    int i;
    for(i = 0; i < NUM; ++i)
    {
        pid = fork();
        if(pid == -1)
        {
            perror("fork");
            return 1;
        }
        else if(pid == 0) break;
    }
    if(pid == 0)
    {
        pid_t childId = getpid();
        pid_t parentId = getppid();
        sleep(1);
        printf("i= %d, child process, pid= %d, ppid= %d\n", i, childId, parentId);
        fflush(stdout);
        exit(0);
    }
    else
    {
        pid_t pid = getpid();
        pid_t ppid = getppid();
        int status;
        pid_t ret;
        do {
            // cpid = waitpid(-1, &status, 0);
            ret = waitpid(-1, &status, WNOHANG);
            if(ret == 0)
            {
                printf("  do my business...\n");
                sleep(1);
                continue;
            }
            printf("parent process, pid = %d, ppid= %d, cpid = %d\n", pid, ppid, ret);
            fflush(stdout);
            if (WIFEXITED(status)) {
                printf("exited, status=%d\n", WEXITSTATUS(status)); 
                fflush(stdout);
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
                fflush(stdout);
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n", WSTOPSIG(status));
                fflush(stdout);
            } else if (WIFCONTINUED(status)) {
                printf("continued\n");
                fflush(stdout);
            }
        } while (ret != -1);
        exit(0);
    }
    return 0;
}
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if(pid == 0)
    {
        pid_t childId = getpid();
        for(int i =0; i < 3; ++i)
        {
            printf("i= %d, child process, pid= %d\n", i, childId);
            sleep(1);
        }
        exit(0);
    }
    pid_t parentId = getpid();
    int status;
    printf("parent process, pid= %d\n", parentId);
    sleep(2);
    kill(pid, SIGINT);
    int ret = waitpid(pid, &status, WNOHANG);
    if (ret == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status)) {
        printf("%d exited, status=%d\n", pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("%d killed by signal %d\n", pid, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("%d stopped by signal %d\n", pid,  WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
        printf("%d continued\n", pid );
    }

    return 0;
}
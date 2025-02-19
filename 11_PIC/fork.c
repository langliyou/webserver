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
        // execlp("ls", "ls", "-l", NULL);
        // perror("execlp");
        // return 1;
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
    int i = 0;
    do {
        printf("i= %d, parent process, pid= %d\n", i, parentId);
        ++i;
        int ret = waitpid(pid, &status, WNOHANG);
        if (ret == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("exited, status=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d\n", WSTOPSIG(status));
        } else if (WIFCONTINUED(status)) {
            printf("continued\n");
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return 0;
}
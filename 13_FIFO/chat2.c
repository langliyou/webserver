#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define BUF_SIZE 256
int main()
{
    if(access("fifo1", F_OK) == -1)
    {
        printf("create FIFO1...\n");
        if(mkfifo("fifo1", 0775) == -1)
        {
            perror("mkfifo");
        }
    }
    if(access("fifo2", F_OK) == -1)
    {
        printf("create FIFO2...\n");
        if(mkfifo("fifo2", 0775) == -1)
        {
            perror("mkfifo");
        }
    }
    int fdr = open("fifo1", O_RDONLY);
    if(fdr == -1)
    {
        perror("open");
    }
    int fdw = open("fifo2", O_WRONLY);
    if(fdw == -1)
    {
        perror("open");
    }
    char buf[BUF_SIZE];
    int wCnt = 0;
    int rCnt = 0;
    int ret;
    while(1)
    {
        memset(buf, 0, BUF_SIZE);
        ret = read(fdr, buf, BUF_SIZE);
        if(ret == -1)
        {
            perror("read");
            break;
        }
        if(ret == 0)
        {
            printf("write terminal quit so i quit.\n");
            break;
        }
        printf("recv %d: %s", rCnt++, buf);

        printf("please input: \n");
        memset(buf, 0, BUF_SIZE);
        fgets(buf, sizeof(buf), stdin);
        ret = write(fdw, buf, sizeof(buf));
        if(ret == -1)
        {
            perror("write");
            break;
        }
        printf("wrt %d: %s\n", wCnt++, buf);
    }
    close(fdr);
    close(fdw);
    if(ret == -1) return 1;
    return 0;
}
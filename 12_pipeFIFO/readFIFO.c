#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main()
{
    int fd = open("fifo1", O_RDONLY);
    if(fd == -1)
    {
        perror("open");
    }
    int ret;
    char buf[1024];
    int i = 0;
    while((ret = read(fd, buf, 1024)) != -1)
    {
        if(ret == 0)
        {
            printf("write terminal end so i quit.\n");
            break;
        }
        printf("recv %d: %s", i++, buf);
        sleep(0);
    }
    if(ret == -1) perror("read");

    return 0;
}
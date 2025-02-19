#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main()
{
    if(access("fifo1", F_OK) == -1)
    {
        printf("create FIFO...\n");
        if(mkfifo("fifo1", 0775) == -1)
        {
            perror("mkfifo");
        }
    }
    int fd = open("fifo1", O_WRONLY);
    if(fd == -1)
    {
        perror("open");
    }
    char buf[1024];
    int i = 0;
    int ret;
    char* str = "im long for tokyo-fuji rock festival, please v me 50000 and send me to Japan.\n";
    int strLen = strlen(str);
    do 
    {
        sprintf(buf, "write %d: %s", i++, str);
        //sprintf会在构造的字符串末尾追加\0, 所以strlen会得到正确的长度
        sleep(1);
        printf("%s", buf);
    }
    while((ret = write(fd, str, strLen)) != -1);
    if(ret == -1) perror("write");

    return 0;
}
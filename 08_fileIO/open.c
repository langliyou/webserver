#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    // int fd = open("./hello.txt", O_RDONLY);
    int fd = open("./hello1.txt", O_RDONLY | O_CREAT, 0774);
    if(fd == -1)
    {
        perror("open");
        return -1;
    }
    printf("file opened\n");
    close(fd);
    return 0;
}
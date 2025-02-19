#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int rF = open("./english.txt", O_RDONLY);
    if(rF == -1)
    {
        perror("read file open");
        return -1;
    }
    int wF = open("./english_copy.txt", O_WRONLY | O_TRUNC | O_CREAT, 0664);
    if(wF == -1)
    {
        perror("write file open");
        return -1;
    }
    char buf[1024];
    int len = 0;
    while((len = read(rF, buf, sizeof(buf))) > 0)
    {
        if(write(wF, buf, len) == -1)
        {
            perror("write file failed");
        }
    }
    // printf("sizeof(buf)= %zu.\n", sizeof(buf));
    printf("write succeeded.\n");
    close(rF);
    close(wF);
    return 0;
}
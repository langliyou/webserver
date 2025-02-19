#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <string.h>

int main()
{
    int fd = open("sharedFile.txt", O_RDWR);
    if(fd == -1)
    {
        perror("open");
        return 1;
    }
    size_t fileLen = lseek(fd, 0, SEEK_END);
    void* ptr = mmap(NULL, fileLen, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    pid_t pid = fork();
    int ret = 0;
    if(pid == -1)
    {
        perror("fork");
        return 1;
    }
    if(pid == 0)
    {
        char buf[] = "gezan gezan gezan gezan gezan, love and peace; no war.\n";
        strcpy((char *)ptr, buf);
    }
    else
    {
        wait(NULL);
        char buf[256] = {0};
        char* p = strcpy(buf, (char*)ptr);
        printf("%s\n", buf);
    }
    munmap(ptr, fileLen);
    return ret;
}
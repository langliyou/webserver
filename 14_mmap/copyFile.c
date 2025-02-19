#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <string.h>

int main()
{
    //打开读 映射
    int fdr = open("1.txt", O_RDONLY);
    if(fdr == -1)
    {
        perror("open");
        return 1;
    }
    int fileLen = lseek(fdr, 0, SEEK_END);
    void* ptrR = mmap(NULL, fileLen, PROT_READ, MAP_PRIVATE, fdr, 0);
    if(ptrR == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    //打开写 映射
    int fdw = open("1_copy.txt", O_RDWR | O_CREAT, 0664);
    if(fdw == -1)
    {
        perror("open");
        return 1;
    }
    if (ftruncate(fdw, fileLen) == -1) {
        perror("ftruncate");
        return 1;
    }
    void* ptrW = mmap(NULL, fileLen, PROT_READ | PROT_WRITE, MAP_SHARED, fdw, 0);
    if(ptrW == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    //在内存中拷贝数据
    memcpy(ptrW, ptrR, fileLen);
    //释放映射, FILO
    munmap(ptrW, fileLen);
    munmap(ptrR, fileLen);
    return 0;
}
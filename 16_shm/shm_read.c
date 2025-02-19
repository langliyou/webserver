#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>

 int main()
 {
    int id = shmget(100, 2048, IPC_CREAT | 0664);
    if(id == -1)
    {
        perror("shmget");
        return 1;
    }
    void * addr = shmat(id, NULL, SHM_RDONLY);
    printf("%s\n", (char *)addr);
    getchar();
    shmdt(addr);
    
    return 0;
 }
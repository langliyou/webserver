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
    void * addr = shmat(id, NULL, 0);
    strcpy(addr, "lololololololololololololol.");
    shmdt(addr);
    getchar();
    int ret = shmctl(id, IPC_RMID, NULL);
    if(ret == -1)
    {
        perror("shmctl");
        return 1;
    }
    if(ret == 0)
    {
        printf("delete shared memory.\n");
    }
    return 0;
 }
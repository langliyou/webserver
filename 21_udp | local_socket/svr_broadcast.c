#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define FD_SIZE 128
#define MAX_EVENTS 10

//socket->bind->listen->accept->recv read write send->close
int main()
{
    int lsnFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(lsnFd == -1) perror("listen socket");
    
    int opt = 1;
    int optret = setsockopt(lsnFd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));//设置广播
    if(optret == 0)
    {
        printf("Broadcast set successfully.\n");
    }
    else if(optret == -1)
    {
        perror("setsockopt");
        return 1;
    }
    
    struct sockaddr_in destAddr;
    socklen_t addrLen = sizeof(destAddr);
    destAddr.sin_port = htons(9999);
    destAddr.sin_family = AF_INET;
    int ptonRet = inet_pton(AF_INET, "192.168.101.255", &destAddr.sin_addr.s_addr);
    switch (ptonRet)
    {
    case 0:
        printf("src invalid.\n");
        break;
    case 1:
        printf("inet_pton ok.\n");
        break;
    default:
        perror("inet_pton");
        break;    
    }

    char readBuf[256];
    char msgBuf[256];
    int i = 0;
    while(1)
    {
        int len = sprintf(msgBuf, "svr msg %d", i++);
        int sendRet = sendto(lsnFd, msgBuf, len, 
                0, (struct sockaddr *)&destAddr, sizeof(destAddr));//发送到广播地址:9999 === 发送给所有端口为9999的主机
        if(sendRet == -1)
        {
            perror("sendto");
            break;
        }
        sleep(1);
    }

    close(lsnFd);
    return 0;
}
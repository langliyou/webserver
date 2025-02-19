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
    struct sockaddr_in clientAddr;
    clientAddr.sin_port = htons(9999);
    clientAddr.sin_family = AF_INET;
    // clientAddr.sin_addr.s_addr = INADDR_ANY;
    int ptonRet = inet_pton(AF_INET, "192.168.101.129", &clientAddr.sin_addr.s_addr);
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
    int opt = 1;
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if(bind(lsnFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1)
    {
        close(lsnFd);
        perror("bind");
        return 1;
    }
    


    struct sockaddr_in connAddr;
    socklen_t addrLen = sizeof(connAddr);
    char readBuf[256];
    char msgBuf[256];
    int i = 0;
    while(1)
    {
        int readRet = recvfrom(lsnFd, readBuf, sizeof(readBuf) - 1, 
                            0, (struct sockaddr *)&connAddr, &addrLen);//会阻塞
        if(readRet == -1)
        {
            perror("recvfrom");
            break;
        }
        else if(readRet == 0)
        {
            printf("client quits.\n");
            break;
        }
        else
        {
            readBuf[readRet] = '\0';
            if(!inet_ntop(AF_INET, &connAddr.sin_addr.s_addr, msgBuf, sizeof(msgBuf)))
            {
                perror("inet_pton");
                break;    
            }
            uint16_t port = ntohs(connAddr.sin_port);
            printf("recv %s from %s: %d\n", readBuf, msgBuf, port);
        }

        int len = sprintf(msgBuf, "svr msg %d", i++);
        int sendRet = sendto(lsnFd, msgBuf, len, 
                0, (struct sockaddr *)&connAddr, sizeof(connAddr));//发送给本地服务端
        if(sendRet == -1)
        {
            perror("sendto");
            break;
        }
    }

    close(lsnFd);
    return 0;
}
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>


//socket->connect->recv read write send->close
int main()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1) perror("socket");
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;
    
    //设置端口复用, 从而可以有多个client被广播
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    //加入到多播组
    struct ip_mreq multiGroup;
    int ptonRet = inet_pton(AF_INET, "239.0.0.10", &multiGroup.imr_multiaddr);
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
    multiGroup.imr_interface.s_addr = INADDR_ANY;
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multiGroup, sizeof(multiGroup));
    //需要指定自己的端口, 不然广播不到自己
    if(bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        close(fd);
        perror("bind");
        return 1;
    }

    char readBuf[256];
    char msgBuf[256];
    int i = 0;
    
    while(1)
    {
        int readRet = recvfrom(fd, readBuf, sizeof(readBuf) - 1, 0, NULL, NULL);
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
            printf("%s\n", readBuf);
        }
    }

    close(fd);
    return 0;
}
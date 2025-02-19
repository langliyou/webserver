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
#include <sys/types.h>
#include <ifaddrs.h>

#define BUF_SIZE 1024
#define FD_SIZE 128
#define MAX_EVENTS 10

//socket->bind->listen->accept->recv read write send->close
int main()
{
    int lsnFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(lsnFd == -1) perror("listen socket");
    
     // 获取本地网络接口列表  
    struct ifaddrs *ifAddrStruct = NULL;  
    struct ifaddrs *ifa = NULL;  
    struct sockaddr_in *inetAddr;  
  
    if (getifaddrs(&ifAddrStruct) == -1) {  
        perror("getifaddrs");  
        close(lsnFd);  
        return 1;  
    }  
  
    // 假设我们选择第一个非回环（loopback）的IPv4接口  
    struct in_addr multicastInterfaceAddr;  
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {  
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {  
            inetAddr = (struct sockaddr_in *)ifa->ifa_addr;  
            multicastInterfaceAddr = inetAddr->sin_addr;  
            break;  
        }  
    }  
  
    if (ifa == NULL) {  
        fprintf(stderr, "No suitable interface found for multicast.\n");  
        freeifaddrs(ifAddrStruct);  
        close(lsnFd);  
        return 1;  
    }  
  
    freeifaddrs(ifAddrStruct); // 释放 getifaddrs 分配的内存  

    //设置多播组
    // struct in_addr multiAddr;
    // int ptonRet = inet_pton(AF_INET, "239.0.0.10", &multiAddr.s_addr);
    // switch (ptonRet)
    // {
    // case 0:
    //     printf("src invalid.\n");
    //     break;
    // case 1:
    //     printf("inet_pton ok.\n");
    //     break;
    // default:
    //     perror("inet_pton");
    //     break;    
    // }
    int optret = setsockopt(lsnFd, IPPROTO_IP, IP_MULTICAST_IF, &multicastInterfaceAddr, sizeof(multicastInterfaceAddr));//设置多播
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
    int inet_ptonRet = inet_pton(AF_INET, "239.0.0.10", &destAddr.sin_addr.s_addr);
    switch (inet_ptonRet)
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
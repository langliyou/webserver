#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h>

//socket->bind->listen->accept->recv read write send->close
int main()
{
    int lsnFd = socket(AF_INET, SOCK_STREAM, 0);
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
    

    if(bind(lsnFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) perror("bind");
    if(listen(lsnFd, 8) == -1) perror("bind");
    struct sockaddr_in connAddr;
    socklen_t len = sizeof(connAddr); 
    int connFd = accept(lsnFd, (struct sockaddr *)&connAddr, &len);
    if(connFd == -1) perror("accept");

    char buf[16];
    inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, buf, sizeof(buf));//一定要转换再打印
    printf("client IP address = %s, port = %d\n", buf, connAddr.sin_port);
    char readBuf[256];
    char msgBuf[256];
    while(1)
    {
        int readRet = read(connFd, readBuf, sizeof(readBuf));
        if(readRet == -1)
        {
            perror("read");
            break;
        }
        else if(readRet == 0)
        {
            printf("client quits.\n");
            break;
        }
        else
        {
            printf("%s\n", readBuf);
        }
        fgets(msgBuf, sizeof(msgBuf), stdin);
        write(connFd, msgBuf, sizeof(msgBuf));
    }
    close(connFd);
    close(lsnFd);
    return 0;
}
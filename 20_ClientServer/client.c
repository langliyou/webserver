#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h>

//socket->connect->recv read write send->close
int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) perror("connect socket");
    // unsigned char buf[16];
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    int ptonRet = inet_pton(AF_INET, "192.168.101.129", &saddr.sin_addr.s_addr);
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
    // printf("pton: %d.%d.%d.%d\n", *buf, *(buf + 1), *(buf + 2), *(buf + 3));
    
    char readBuf[256];
    char msgBuf[256];
    if(connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) perror("connect");
    int i = 0;
    while(1)
    {
        // fgets(msgBuf, sizeof(msgBuf), stdin);
        int len = sprintf(msgBuf, "msg %d", i++);
        write(fd, msgBuf, len);
        int readRet = read(fd, readBuf, sizeof(readBuf) - 1);
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
            readBuf[readRet] = '\0';
            printf("%s\n", readBuf);
        }
        sleep(1);
    }

    close(fd);
    return 0;
}
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>

#define SOCK_FILENAME "./sock"

//socket->connect->recv read write send->close
int main()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1) perror("connect socket");
    struct sockaddr_un saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, SOCK_FILENAME, sizeof(saddr.sun_path) - 1);
    if(connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) perror("connect");
    char readBuf[256];
    char msgBuf[256];
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
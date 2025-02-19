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
// - 问题1: 如何回收子进程
//     - 必须使用sigaction, 处理信号SIGCHLD, 否则监听进程阻塞在accept, 无法回收子进程
//     - 信号处理必须一开始阻塞（避免子进程比监听进程先退出而未接收到信号）， 然后解除阻塞一次 
// - 问题2: read在客户端退出后会返回-1, read: connection reset by peer
//     - 原因: Ctrl C终止, 还未关闭连接
// - 问题3: 子进程可能会阻塞在write
// - 问题4: ctrl C终止, 相当于发送了FIN
void * handleConn(void *arg)
{
    int connFd = *(int *)arg;
    char readBuf[256];
    char msgBuf[256];
    while(1)
    {
        int readret = read(connFd, readBuf, sizeof(readBuf) - 1);
        if(readret == -1)
        {
            perror("read");
            break;
        }
        else if(readret == 0)
        {
            printf("client quits.\n");
            break;
        }
        else
        {
            readBuf[readret] = '\0';
            printf("%s\n", readBuf);
        }
        fgets(msgBuf, sizeof(msgBuf), stdin);
        write(connFd, msgBuf, sizeof(msgBuf));
    }
    close(connFd);
    pthread_exit(NULL);
}

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
    int opt = 1;
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if(bind(lsnFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) perror("bind");
    if(listen(lsnFd, 8) == -1) perror("bind");
    
    
    struct sockaddr_in connAddr;
    while(1)
    {
        socklen_t len = sizeof(connAddr); 
        int connFd = accept(lsnFd, (struct sockaddr *)&connAddr, &len);
        if(connFd == -1)
        {
            if(errno == EINTR)
            {
                //回收进程时系统调用会打断accept导致其返回-1, 此时只能简单忽略
                continue;
            }
            perror("accept");
            exit(1);
        }
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, handleConn, (void *)&connFd);
        pthread_detach(tid);
        if(ret != 0)
        {
            strerror(ret);
        }
        else
        {
            char buf[16];
            inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, buf, sizeof(buf));//一定要转换再打印
            printf("client IP address = %s, port = %d, pid = %ld\n", buf, connAddr.sin_port, tid);
        }
    }
    close(lsnFd);
    return 0;
}
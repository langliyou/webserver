#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <stdio.h>
#include <unistd.h> 
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
// - 问题1: 如何回收子进程
//     - 必须使用sigaction, 处理信号SIGCHLD, 否则监听进程阻塞在accept, 无法回收子进程
//     - 信号处理必须一开始阻塞（避免子进程比监听进程先退出而未接收到信号）， 然后解除阻塞一次 
// - 问题2: read在客户端退出后会返回-1, read: connection reset by peer
//     - 原因: Ctrl C终止, 还未关闭连接
// - 问题3: 子进程可能会阻塞在write
void waitChildren(int num)
{
    while(1)
    {
        int ret = waitpid(-1, NULL, WNOHANG);
        if(ret == 0) break;
        if(ret == -1)
        {
            printf("no child process.\n");
            break;//继续监听, 等待新的连接
        }
        printf("recycle child %d\n", ret);
    }
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
    
    if(bind(lsnFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) perror("bind");
    if(listen(lsnFd, 8) == -1) perror("bind");
    
    // 一开始阻塞SIGALRM
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, NULL);
    
    struct sockaddr_in connAddr;
    int bBlockSig = 0;
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
        int ret = fork();
        if(ret == -1)
        {
            perror("fork");
            exit(1);
        }
        else if(ret == 0)
        {
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
            exit(0);
        }
        else
        {
            char buf[16];
            inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, buf, sizeof(buf));//一定要转换再打印
            printf("client IP address = %s, port = %d, pid = %d\n", buf, connAddr.sin_port, ret);
            if(bBlockSig == 0)
            {
                bBlockSig = 1;
                //设置handler
                struct sigaction act;
                act.sa_flags = 0;
                act.sa_handler = waitChildren;
                sigemptyset(&act.sa_mask);//清空 
                int retSig = sigaction(SIGCHLD, &act, NULL);
                if(retSig == 0)
                {
                    printf("set sigaction ok.\n");
                }
                else if(retSig == -1)
                {
                    perror("sigaction");
                    exit(1);
                }
                //解除阻塞SIGALRM
                sigemptyset(&set);
                sigaddset(&set, SIGCHLD);
                sigprocmask(SIG_UNBLOCK, &set, NULL);   
            }
        }
    }
    close(lsnFd);
    return 0;
}
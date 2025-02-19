// #include "concurrent.h"
#include <signal.h>
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
#include "/usr/include/c++/13/cerrno"
#include <vector>
#include <string>

#include "threadPool.hpp"
#include "http_conn.h"

#define MAX_FD_NUM 65535 // 客户端最大连接数量
#define EVENT_NUM 10000


using std::vector;

// 封装了错误处理的函数
void Inet_pton(int af, const char * src, void * dst)
{
    int ptonRet = inet_pton(af, src, dst);
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
}
void addSigAction(int signum, void (*handler)(int))
{
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);//清空 
    int retSig = sigaction(signum, &act, NULL);
    if(retSig == 0)
    {
        printf("set sigaction ok.\n");
    }
    else if(retSig == -1)
    {
        perror("sigaction");
    }
}


int main(int argc, char* argv[])
{
    if(argc <= 1)// 字符串数组只有默认的name, 没传入参数
    {
        printf( "usage: %s port_number\n", basename(argv[0]));
        return 1;
    }
    addSigAction(SIGPIPE, SIG_IGN);// 忽略pipe信号？
    int port = std::stoi(argv[1]);

    // 初始化线程池
    ThreadPool tp;
    tp.start(4);

    // epoll准备工作
    int lsnFd = socket(AF_INET, SOCK_STREAM, 0);
    if(lsnFd == -1) perror("listen socket");
    struct sockaddr_in clientAddr;
    clientAddr.sin_port = port;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY; // 允许任意ip连接
    // Inet_pton(AF_INET, "192.168.101.129", &clientAddr.sin_addr.s_addr);
        // 设置端口复用
    int opt = 1;
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(lsnFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if(bind(lsnFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1)
    {
        close(lsnFd);
        perror("bind");
        return 1;
    }
    if(listen(lsnFd, 8) == -1)
    {
        close(lsnFd);
        perror("bind");
        return 1;
    }
     
    struct epoll_event events[EVENT_NUM];// 作为传出参数
    vector<HttpConn> connObjs(MAX_FD_NUM);
    int connFd, nfds, epollFd;
    int msgCnt = 0;

    epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (epollFd == -1) {
        perror("epoll_create1");
        close(epollFd);
        close(lsnFd);
        exit(EXIT_FAILURE);
    }
    addFd(lsnFd);// 一开始只epoll监听
    HttpConn::m_epollFd = epollFd;
    printf("%d listening...\n", lsnFd);
    for (;;) 
    {
        nfds = epoll_wait(epollFd, events, EVENT_NUM, -1);
        if (nfds == -1 && errno != EINTR) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < nfds; ++i) 
        {
            int curFd = events[i].data.fd;
            if (curFd == lsnFd) 
            {
                struct sockaddr_in connAddr;
                socklen_t len = sizeof(connAddr); 
                connFd = accept(lsnFd, (struct sockaddr *)&connAddr, &len);
                if (connFd == -1) 
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                // setnonblocking(connFd);
                if(HttpConn::m_conn_cnt >= MAX_FD_NUM)
                {
                    // 向客户端发送报文: 服务器正忙
                    close(connFd);
                    continue;
                }
                connObjs[connFd].init(connFd, connAddr); // connFd递增, 用其作为数组的索引
            } 
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) // 读取异常或错误, 则关闭连接
            {
                int curFd = events[i].data.fd;
                connObjs[curFd].closeConn();
            }
            else if(events[i].events & EPOLLIN)
            {
                // 一次性读出所有数据, 成功则交给线程池的线程处理
                connObjs[curFd].readAll();
                //...
                // 失败则关闭连接
                //...
            }
            else if(events[i].events & EPOLLOUT)
            {
                // 一次性读出所有数据, 成功则交给线程池的线程处理
                if(connObjs[curFd].writeAll())
                {
                    connObjs[curFd].closeConn();
                }
            }
        }
    }   
    close(epollFd);
    close(lsnFd);
    return 0;
}
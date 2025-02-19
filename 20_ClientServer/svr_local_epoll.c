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
#include <sys/un.h>

// - 问题1: 如何回收子进程
//     - 必须使用sigaction, 处理信号SIGCHLD, 否则监听进程阻塞在accept, 无法回收子进程
//     - 信号处理必须一开始阻塞（避免子进程比监听进程先退出而未接收到信号）， 然后解除阻塞一次 
// - 问题2: read在客户端退出后会返回-1, read: connection reset by peer
//     - 原因: Ctrl C终止, 还未关闭连接
// - 问题3: 子进程可能会阻塞在write
// - 问题4: ctrl C终止, 相当于发送了FIN
#define BUF_SIZE 1024
#define FD_SIZE 128
#define MAX_EVENTS 10
#define SOCK_FILENAME "./sock"

//socket->bind->listen->accept->recv read write send->close
int main()
{
    int lsnFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(lsnFd == -1) perror("listen socket");


    //删除已存在的文件
    if (unlink(SOCK_FILENAME) == -1 && errno != ENOENT) //不是文件不存在的错误
    {  
        perror("unlink");  
        exit(EXIT_FAILURE);  
    }
    struct sockaddr_un sockName;
    memset(&sockName, 0, sizeof(sockName));
    sockName.sun_family = AF_UNIX;
    strncpy(sockName.sun_path, SOCK_FILENAME, sizeof(sockName.sun_path) - 1);//拷贝并自动加上结束符
    if(bind(lsnFd, (struct sockaddr *)&sockName, sizeof(sockName)) == -1)
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
    
     
    struct epoll_event ev;//临时设置用
    struct epoll_event events[MAX_EVENTS];//作为传出参数
    int connFd, nfds, epollFd;
    int msgCnt = 0;


    epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (epollFd == -1) {
        perror("epoll_create1");
        close(epollFd);
        close(lsnFd);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = lsnFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, lsnFd, &ev) == -1) {
        perror("epoll_ctl: lsnFd");
        close(epollFd);
        close(lsnFd);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < nfds; ++i) 
        {
            if (events[i].data.fd == lsnFd) 
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
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;//可读=》不可读；可写=》不可写才会在epoll_wait内返回
                ev.data.fd = connFd;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, connFd, &ev) == -1)
                {
                    perror("epoll_ctl: connFd");
                    exit(EXIT_FAILURE);
                }
            } 
            else 
            {
                int curFd = events[i].data.fd;
                if(events[i].events & EPOLLIN)
                {
                    char buf[BUF_SIZE] = {0};
                    int recvRet = recv(curFd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
                    if (recvRet == -1) 
                    {  
                        if (errno == ECONNRESET) {  
                            // 连接被对方重置  
                            fprintf(stderr, "Read error: Connection reset by peer\n");  
                            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, curFd, &ev) == -1)
                            {
                                perror("epoll_ctl: curFd");
                                exit(EXIT_FAILURE);
                            }
                            close(curFd); // 关闭套接字  
                            continue;
                        } 
                        else if(errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            printf("no data to read now\n");//设置非阻塞 当没有数据可读需要处理这个错误
                        }
                        else
                        {  
                            // 处理其他类型的错误...  
                            perror("Read error");  
                        }  
                    } 
                    else if (recvRet == 0) 
                    {  
                        printf("recvRet = 0\n");
                        // 对方正常关闭连接  
                        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, curFd, &ev) == -1)
                        {
                            perror("epoll_ctl: curFd");
                            exit(EXIT_FAILURE);
                        }
                        close(curFd); // 关闭套接字  
                        continue;
                    } 
                    else 
                    {  
                        // 正常读取数据...  
                        buf[recvRet] = '\0'; // 确保字符串以 null 结尾  
                        // 处理读取到的数据...  
                        printf("from fd %d, recv %s\n", curFd, buf);
                    }
                }
                // while(1)
                //     {
                //         char buf[RD_BUF_SIZE] = {0};
                //         int recvRet = recv(curFd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
                //         if (recvRet == -1) 
                //         {  
                //             if (errno == ECONNRESET) {  
                //                 // 连接被对方重置  
                //                 fprintf(stderr, "Read error: Connection reset by peer\n");  
                //                 if (epoll_ctl(epollFd, EPOLL_CTL_DEL, curFd, &ev) == -1)
                //                 {
                //                     perror("epoll_ctl: curFd");
                //                     exit(EXIT_FAILURE);
                //                 }
                //                 close(curFd); // 关闭套接字  
                //                 isValid = 0;
                //                 break;
                //             } 
                //             else if(errno == EAGAIN || errno == EWOULDBLOCK)
                //             {
                //                 printf("no data to read now\n");//设置非阻塞 当没有数据可读需要处理这个错误
                //                 break;
                //             }
                //             else
                //             {  
                //                 // 处理其他类型的错误...  
                //                 perror("Read error");  
                //                 break;
                //             }  
                //         } 
                //         else if (recvRet == 0) 
                //         {  
                //             printf("recvRet = 0\n");
                //             // 对方正常关闭连接  
                //             if (epoll_ctl(epollFd, EPOLL_CTL_DEL, curFd, &ev) == -1)
                //             {
                //                 perror("epoll_ctl: curFd");
                //                 exit(EXIT_FAILURE);
                //             }
                //             close(curFd); // 关闭套接字  
                //             isValid = 0;
                //             break;
                //         } 
                //         else 
                //         {  
                //             // 正常读取数据...  
                //             buf[recvRet] = '\0'; // 确保字符串以 null 结尾  
                //             // 处理读取到的数据...  
                //             printf("from fd %d, recv %s\n", curFd, buf);
                //         }
                //     }
                // }
                if(events[i].events & EPOLLOUT)
                {
                    char msg[256] = {0};
                    int len = snprintf(msg, sizeof(msg), "sending msg %d", msgCnt++);
                    int writeret = send(curFd, msg, len, MSG_DONTWAIT);
                    if (writeret == -1) 
                    {  
                        perror("Write error"); 
                        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, curFd, &ev) == -1)
                        {
                            perror("epoll_ctl: curFd");
                            exit(EXIT_FAILURE);
                        }
                        close(curFd); // 关闭套接字  
                    } 
                    // else if (writeret < sizeof(msg)) 
                    // {  
                    //     printf("write not fully\n");
                    // } 
                }
            }
        }
    }
    close(lsnFd);
    unlink(SOCK_FILENAME);//删除套接字文件
    return 0;
}
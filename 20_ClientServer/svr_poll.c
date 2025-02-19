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
#include <poll.h>
// - 问题1: 如何回收子进程
//     - 必须使用sigaction, 处理信号SIGCHLD, 否则监听进程阻塞在accept, 无法回收子进程
//     - 信号处理必须一开始阻塞（避免子进程比监听进程先退出而未接收到信号）， 然后解除阻塞一次 
// - 问题2: read在客户端退出后会返回-1, read: connection reset by peer
//     - 原因: Ctrl C终止, 还未关闭连接
// - 问题3: 子进程可能会阻塞在write
// - 问题4: ctrl C终止, 相当于发送了FIN
#define BUF_SIZE 1024
#define FD_SIZE 128

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
    
    
    struct pollfd fds[FD_SIZE];
    for(int i = 1; i < FD_SIZE; ++i)
    {
        fds[i].fd = -1;
    }
    fds[0].fd = lsnFd;
    fds[0].events = POLLIN | POLLOUT;
    int nfds = 1;//现在维护的是结构体数组的有效数量, 但不直接和文件描述符相同
    int msgCnt = 0;
    while(1)
    {
        int ready = poll(fds, nfds, -1);//nfds限制个数, 但是传入一整个结构体数组; 阻塞, 或设置超时
        if(ready == -1)
        {
            perror("poll");
            close(lsnFd);
            exit(1);
        }
        // printf("%d fds changed status.\n", ready);
        // 监听fd发生变化, 说明有新的连接进来
        if(fds[0].revents & POLLIN)
        {
            struct sockaddr_in connAddr;
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
                close(connFd);
                exit(1);
            }
            int bNfdsEnough = 0;
            for(int i = 1; i < nfds; ++i)
            {
                if(fds[i].fd == -1)
                {
                    fds[i].fd = connFd;
                    fds[i].events = POLLIN | POLLOUT;
                    bNfdsEnough = 1;
                    break;
                }
            }
            if(bNfdsEnough == 0)
            {
                fds[nfds].fd = connFd;
                fds[nfds].events = POLLIN | POLLOUT;
                ++nfds;
            }
            char buf[16] = {0};
            inet_ntop(AF_INET, &connAddr.sin_addr.s_addr, buf, sizeof(buf));//一定要转换再打印
            uint16_t port = ntohs(connAddr.sin_port);
            printf("connected client IP address = %s, port = %d\n", buf, port);
            continue;//下次循环再处理读写
        }
        // 判断其他是否可读可写
        for(int i = 1; i < nfds; ++i)
        {
            if(fds[i].revents & POLLIN)
            {
                char buf[BUF_SIZE] = {0};
                int readret = read(fds[i].fd, buf, sizeof(buf) - 1);
                if (readret == -1) 
                {  
                    if (errno == ECONNRESET) {  
                        // 连接被对方重置  
                        fprintf(stderr, "Read error: Connection reset by peer\n");  
                        close(fds[i].fd); // 关闭套接字  
                        fds[i].fd = -1;
                        continue;
                    } 
                    else 
                    {  
                        // 处理其他类型的错误...  
                        perror("Read error");  
                    }  
                } 
                else if (readret == 0) 
                {  
                    // 对方正常关闭连接  
                    close(fds[i].fd); // 关闭套接字  
                    fds[i].fd = -1;
                    continue;
                } 
                else 
                {  
                    // 正常读取数据...  
                    buf[readret] = '\0'; // 确保字符串以 null 结尾  
                    // 处理读取到的数据...  
                    printf("from fd %d, recv %s\n", i, buf);
                }
            }
            if(fds[i].revents & POLLOUT)
            {
                char msg[256] = {0};
                int len = snprintf(msg, sizeof(msg), "sending msg %d", msgCnt++);
                int writeret = write(fds[i].fd, msg, len);
                if (writeret == -1) 
                {  
                    perror("Write error"); 
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
                else if (writeret < sizeof(msg)) 
                {  
                    printf("write not fully\n");
                } 
            }
        }
    }
    close(lsnFd);
    return 0;
}
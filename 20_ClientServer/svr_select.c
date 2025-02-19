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
#include <sys/select.h>
// - 问题1: 如何回收子进程
//     - 必须使用sigaction, 处理信号SIGCHLD, 否则监听进程阻塞在accept, 无法回收子进程
//     - 信号处理必须一开始阻塞（避免子进程比监听进程先退出而未接收到信号）， 然后解除阻塞一次 
// - 问题2: read在客户端退出后会返回-1, read: connection reset by peer
//     - 原因: Ctrl C终止, 还未关闭连接
// - 问题3: 子进程可能会阻塞在write
// - 问题4: ctrl C终止, 相当于发送了FIN
#define BUF_SIZE 1024


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
    
    
    fd_set readFds;
    fd_set writeFds;
    fd_set preRdFds;
    fd_set preWtFds;
    FD_ZERO(&preRdFds);
    FD_ZERO(&preWtFds);
    FD_SET(lsnFd, &preRdFds);
    int maxFd = lsnFd;
    while(1)
    {
        readFds = preRdFds;
        writeFds = preWtFds;
        int ready = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);//阻塞, 或设置超时
        if(ready == -1)
        {
            perror("select");
            close(lsnFd);
            exit(1);
        }
        // printf("%d fds changed status.\n", ready);
        // 监听fd发生变化, 说明有新的连接进来
        if(FD_ISSET(lsnFd, &readFds))
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
            FD_SET(connFd, &preRdFds);//加入读集合
            FD_SET(connFd, &preWtFds);//加入写集合
            maxFd = maxFd < connFd? connFd: maxFd;
            char buf[16];
            inet_ntop(AF_INET, &connAddr.sin_addr.s_addr, buf, sizeof(buf));//一定要转换再打印
            uint16_t port = ntohs(connAddr.sin_port);
            printf("connected client IP address = %s, port = %d", buf, port);
        }
        // 判断其他是否可读可写
        for(int i = lsnFd + 1; i <= maxFd; ++i)
        {
            if(FD_ISSET(i, &readFds))
            {
                char buf[BUF_SIZE] = {0};
                int readret = read(i, buf, sizeof(buf) - 1);
                if (readret == -1) 
                {  
                    if (errno == ECONNRESET) {  
                        // 连接被对方重置  
                        fprintf(stderr, "Read error: Connection reset by peer\n");  
                        close(i); // 关闭套接字  
                        FD_CLR(i, &preRdFds);
                        FD_CLR(i, &preWtFds);
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
                    close(i); // 关闭套接字  
                    FD_CLR(i, &preRdFds);
                    FD_CLR(i, &preWtFds);
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
            if(FD_ISSET(i, &writeFds))
            {
                char* msg = "sending sth.";
                int writeret = write(i, msg, sizeof(msg));
                if (writeret == -1) 
                {  
                    perror("Write error"); 
                    close(i);
                    FD_CLR(i, &preRdFds);
                    FD_CLR(i, &preWtFds);
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
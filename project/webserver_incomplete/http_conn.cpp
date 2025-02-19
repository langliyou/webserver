#include "http_conn.h"
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
#include <fcntl.h>
#include <string.h>


int HttpConn::m_epollFd = -1;
int HttpConn::m_conn_cnt = 0;

void setNonBlock(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}
void addFd(int fd)
{
    if(HttpConn::m_epollFd == -1) return;
    struct epoll_event ev;
    // 设置读写异常事件, 不在读写返回值时处理, 统一用epoll处理; 设置读、写事件触发一次, 防止多线程同时进入, 触发后需要重新设置
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    ev.data.fd = fd;
    if(ET_STATUS)
    {
        ev.events |= EPOLLET;
        setNonBlock(fd);
    }

    if (epoll_ctl(HttpConn::m_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: lsnFd");
        close(HttpConn::m_epollFd);
        close(fd);
        exit(EXIT_FAILURE);
    }
}
void removeFd(int fd)
{
    if(HttpConn::m_epollFd == -1) return;
    if (epoll_ctl(HttpConn::m_epollFd, EPOLL_CTL_DEL, fd, 0) == -1)
    {
        perror("epoll_ctl: curFd");
        exit(EXIT_FAILURE);
    }
}
void modifyFd(int fd, int newEv)
{
    if(HttpConn::m_epollFd == -1) return;
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = newEv | EPOLLONESHOT | EPOLLRDHUP;
     if(ET_STATUS)
    {
        ev.events |= EPOLLET;
        setNonBlock(fd);
    }
    epoll_ctl( HttpConn::m_epollFd, EPOLL_CTL_MOD, fd, &ev );
}
HttpConn::HttpConn(): 
    m_connFd(-1),
    m_read_idx(0)
{
    memset(m_readBuf, 0, READ_BUF_SIZE);
}
void HttpConn::init(int sockFd, const sockaddr_in& addr)
{
    m_connFd = sockFd;
    m_clientAddr = addr;

    // 设置端口复用
    int opt = 1;
    setsockopt(m_connFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    ++m_conn_cnt;
    addFd(m_connFd);
}
void HttpConn::closeConn()
{
    if(m_connFd == -1)
    {
        return;
    }
    --m_conn_cnt;
    removeFd(m_connFd);
    m_connFd = -1;
}
bool HttpConn::readAll()
{
    if(m_read_idx >= READ_BUF_SIZE)
    {
        return false;
    }
    int readBytes = 0;
    while(true)
    {
        readBytes = recv(m_connFd, m_readBuf + m_read_idx, READ_BUF_SIZE - m_read_idx, 0); 
        if(readBytes == -1)
        {
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // 没有数据可读
                break;
            }
            else if(readBytes == 0)
            {
                // 对方关闭连接
                return false;
            }
        }
        m_read_idx += readBytes;
    }
    printf("read %d bytes from %d fd.\n", readBytes, m_connFd);
    return true;
}
bool HttpConn::writeAll()
{
    return true;
}

void HttpConn::process()
{
    // 解析HTTP请求
    HTTP_CODE readRet = processRead();
    if ( readRet == NO_REQUEST ) {
        modifyFd(m_connFd, EPOLLIN);
        return;
    }
    
    // 生成响应
    bool writeRet = processWrite( readRet );
    if ( !writeRet ) {
        closeConn();
    }
    modifyFd(m_connFd, EPOLLOUT);
}
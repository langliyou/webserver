#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "locker.h"
// #include "threadpool.h"
#include "threadPool.h"
#include "HTTP_CONN.h"

#define FD_NUM 65536   // 最大的文件描述符个数
#define EVENT_NUM 10000  // 监听的最大的事件数量

// 添加文件描述符
extern void addfd( int epollfd, int fd, bool one_shot );
extern void removefd( int epollfd, int fd );

// 添加信号捕捉
void addsig(int sig, void( handler )(int)){
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

int main( int argc, char* argv[] ) {
    
    if( argc <= 1 ) {
        printf( "usage: %s port_number\n", basename(argv[0]));
        return 1;
    }

    // 获取端口号
    int port = atoi( argv[1] );

    // 对SIGPIE信号进行处理
    addsig( SIGPIPE, SIG_IGN );
    
    // 创建和初始化线程池
    ThreadPool* pool = NULL;
    try {
        pool = new ThreadPool;
        pool->setPoolMode(PoolMode::CASHED_MODE);
	    pool->start(4);
    } catch( ... ) {
        return 1;
    }

    // 创建一个数组 用于保存所有的客户端信息
    HTTP_CONN* users = new HTTP_CONN[ FD_NUM ];


    // 创建监听套接字
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );

    int ret = 0;
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons( port );

    // 端口复用
    int reuse = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );

    // 监听
    ret = listen( listenfd, 5 );

    // 创建epoll对象，和事件数组，添加
    epoll_event events[ EVENT_NUM ];
    int epollfd = epoll_create( 5 );
    // listenfd添加到epoll对象中
    addfd( epollfd, listenfd, false );// 不能注册EPOLLONESHOT, 否则只能连接一个客户端
    HTTP_CONN::m_epollfd = epollfd;

    while(true) {
        
        int number = epoll_wait( epollfd, events, EVENT_NUM, -1 );// 回调, 返回的0~number - 1的所有event都是已就绪的
        
        if ( ( number < 0 ) && ( errno != EINTR ) ) {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; i++ ) {
            
            int sockfd = events[i].data.fd;
            
            if( sockfd == listenfd ) {
                // 有客户端连接进来
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                
                if ( connfd < 0 ) {
                    printf( "errno is: %d\n", errno );
                    continue;
                } 

                if( HTTP_CONN::m_user_count >= FD_NUM ) {
                    // 目前连接数满了
                    // 给客户端写一个信息： 服务器正忙
                    close(connfd);
                    continue;
                }
                // 将新的客户的数据初始化， 放入数组中
                users[connfd].init( connfd, client_address);
            
            } else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ) {
                // 对方主动断开、异常断开或错误等事件
                users[sockfd].close_conn();

            } else if(events[i].events & EPOLLIN) {
                // 一次性把全部数据读完
                if(users[sockfd].read()) {
                    auto boundTask = std::bind(&HTTP_CONN::process, &users[sockfd]);
                    pool->submitTask(boundTask);
                    // pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }

            }  else if( events[i].events & EPOLLOUT ) {
                // 一次性把全部数据写完
                if( !users[sockfd].write() ) {
                    users[sockfd].close_conn();
                }

            }
        }
    }
    
    close( epollfd );
    close( listenfd );
    delete [] users;
    delete pool;
    return 0;
}
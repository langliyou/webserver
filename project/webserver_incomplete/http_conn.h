#ifndef HttpConn_H
#define HttpConn_H

#include "/usr/include/netinet/in.h"
#define ET_STATUS false
#define READ_BUF_SIZE 1048

void addFd(int fd);
void removeFd(int fd);
void modifyFd(int fd, int newEv);

// 负责封装和连接相关的操作和数据
class HttpConn
{
public:
    static int m_epollFd;
    static int m_conn_cnt;
    // HTTP请求方法，这里只支持GET
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    /*
        解析客户端请求时，主状态机的状态
        CHECK_STATE_REQUESTLINE:当前正在分析请求行
        CHECK_STATE_HEADER:当前正在分析头部字段
        CHECK_STATE_CONTENT:当前正在解析请求体
    */
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    
    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    
public:
    HttpConn();
    void init(int sockFd, const sockaddr_in& addr);
    void closeConn();
    bool readAll();
    bool writeAll();
    void process();// 由线程池工作线程调用
private:
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE parseRet);
    LINE_STATUS parseLine();
    LINE_STATUS parseTopLine();
    LINE_STATUS parseHeader();
    LINE_STATUS parseContent();

private:
    char m_readBuf[READ_BUF_SIZE];
    sockaddr_in m_clientAddr;
    int m_connFd;
    int m_read_idx;// 目前读缓冲区已经使用的大小, 下一次从这里继续往后填充
};
#endif
// http 连接类

#ifndef _HTTPSCONN_H_
#define _HTTPSCONN_H_

#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include "../sql_connection_pool/sql_connection_pool.h"
// #include "../timer/timer.h"

// #include "../pthreadpool/pthreadpool.h"
using json = nlohmann::json;


class Http_conn {
public:
    // 此处定义为静态成员原因：1、节省空间，每个对象都要使用，2、避免冲突，类的数据声明不能使用该类其他的数据
    // static const char * root = "../root/";
    static const int WRITE_BUF_SIZE = 1024 ;
    static const int READ_BUFFER_SIZE = 2048;
    // 请求方法
    enum REQ {
        GET = 0,
        POST,
        PUT,
        DELETE,
        PATCH,
        HEAD,
        OPTIONS,
        TRACE
    };
    
    enum HTTP_CODE {
        NO_REQUEST = 0, // 请求不完整，需要继续读取请求报文数据
        GET_REQUEST, // 获得了完整的HTTP请求
        BAD_REQUEST, // HTTP请求报文有语法错误
        INTERNAL_ERROR,  // 服务器内部错误，该结果在主状态机逻辑switch的default下，一般不会触发
        NO_RESOURCE,  // 资源不存在
        FILE_REQUEST, // 文件请求
        FORBIDDEN_REQUEST, // 服务端拒绝服务
        CLOSED_CONNECTION // 断开连接
    };

    enum LINE_STATE {
        LINE_OK = 0,
        LINE_BAD,
        LINE_EMPTY
    };
    
     
private:
    int m_socket;  
    int m_epollfd; // epoll
    bool m_TRIGMode; // 触发模式

    char *m_buf=NULL; // 用于保存数据包
    char *m_req_method; // 请求办法
    char *m_url; // 请求访问的地址
    char *m_version; // http版本
    char *m_header; // 请求头
    char *m_post_payload; // post请求数据
    int m_buf_size; 
    
    json m_head_json; // 请求头数据
    json m_payload_json; // post负载数据

    char write_buf[WRITE_BUF_SIZE]; // 写入的数据包
    char m_path[512]; // 要发送的文件地址
    int wpos; // 当前的位置

    MYSQL *m_mysql; // 数据库
    char *user;
    char *password;

    // struct sockaddr_in addr;
    bool http_connect= false; // http连接
    
    bool m_linger; // 持续链接标志
    long m_resmessage_len; // 返回负载长度
    
    struct iovec m_iv[2];   
    int m_state; // 记录这个是读还是写 

    json to_json(char *buf); // 将字符串转成json
    // 解析
    LINE_STATE parse_line(); // 解析请求行
    HTTP_CODE parse_header();// 解析请求头
    HTTP_CODE parse_post_body(); // 解析post的负载
    HTTP_CODE Parse(); // 解析数据包
    REQ get_method() const; // 获取请求方法
    
    void get_resource(); // 获取资源目录 
    
    // 回复
    bool generate_head(int status, char* addr); //生成响音数据包
    bool add_request(char *format, ...);
    bool add_line(int stats,const char *title); // 加入状态行
    bool add_headers(long len ,char *ss); // 加入头
    bool add_content_type(char *ss); // 添加Content-Type
    bool add_content_length(int len); // 添加content_length
    bool add_linger(); // 是否持续链接
    bool add_blank_line(); // 添加空行
    bool add_message(char *message); // 加入请求报文
    char *to_kind(char *kind);  // 文件类型


    // 读数据包
    void read_event();
    void write_event();

    // 数据库
    bool verification_user();
    bool register_user();

public:
    Http_conn(){};
    ~Http_conn(){};
    void Init(); // 用于将连接数据清空
    void Init(int socket,int epollfd,bool TRIGMode,struct sockaddr_in *addr);
    bool con_mysql(Connection_pool *mysql_pool);
    void close_mysql(Connection_pool *mysql_pool);
    void close_conn();
    // char *Get_url();
    // int Get_epollfd() const;
    bool recv_message(); // 读socket缓冲区的数据包
    void process(); // 执行任务（解析数据包，生成回复数据包，发送数据包）
    void change_state(int state);
    MYSQL *re_mysql();
    // bool re_TRIGMode(); // 返回是否边缘触发


};

#endif
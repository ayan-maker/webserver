#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include <stdio.h>
#include <unordered_map>
#include "../Block_queue/Block_queue.h"
#include "../http_conn/http_conn.h"
#include "../LOG/log.h"
#include "../pthreadpool/pthreadpool.h"
#include "../timer/timer.h"
#include "../sql_connection_pool/sql_connection_pool.h"
#include <list>

class Webserver {
private:

    char LOG_FILE[1024]; // log file
    enum {MAX_NUM = 100};
    int m_listensk; // 监听socket
    int m_port;
    int m_epollfd;
    int m_sub_epollfd;

    // 线程池池
    int m_MAX_PTH;
    int m_max_requests;
    int m_state;
    
    // 连接池
    char m_ip[20];
    char m_user[20];
    char m_password[20];
    char m_database[20];
    int m_MAX_CONN;
    int m_close_log;
    int m_pipfd[2];
    bool m_TRIGMode; // 边缘触发
    int m_out_time;

    Timer_list m_time_list;
    
    Locker m_lock;
    Connection_pool *m_mysql_pool;
    PthreadPool<Http_conn> *m_pthread_pool;
    Log *m_web_log;
    Http_conn * m_http_gourp; // 先开辟
    std::list<Http_conn*> m_free_conn; // 用于保存当前空闲的连接
    std::unordered_map<int, Http_conn*> m_map;
    std::unordered_map<int, Timeout_timer*> m_timer_map;
    

    static void *Listen(void *); // 监听
    static void *deal_event(void*); // 用于处理事件
    // void Process_request(); // 处理请求
    void pthread_pool(); // 线程池开辟空间
    void mysql_pool(); // 创建连接池
    bool log_file(); // 创建日志
    void init_listen(); // 初始化监听
    void Reactor_one(); // 单Reactor多线程模型
    void main_reactor(); // 主reactor
    void sub_reactor(); // 从reactor
    void read_event(int fd); // 读事件
    void del_http(int fd); // 断开连接
    void write_event(int fd);
    void time_event();
public:
    Webserver();
    ~Webserver();
    int relistensk() const;
    bool add_request(Http_conn *conn, int stats);
    void init_server(int MAX_PTH,int max_requests, int state, char * ip,char* user,char * password,char *datename,int port,int max_conn,int close_log,char *logfile);
    void set_out_time(int);
};


#endif
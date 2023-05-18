#ifndef _TIMEER_H_
#define _TIMEER_H_

#include <stdio.h>
#include <netinet/in.h> // sockaddr_in
#include <functional>
#include <vector>
class Timeout_timer;

// struct Client_data {
//     // sockaddr_in address; // sock 通信的表示 IPv4 地址和端口号的结构体
//     int sockfd; // socket 文件描述符
//     Timeout_timer *Timer; // 超时定时器 未定义无法判断内存大小所以使用指针
// };  
// 定时器类 
class Timeout_timer {
public:
    time_t expire; // 设置超时时间 正常访问网页的时间 + 允许停留的时间（固定）表示到达该时间就会被停止
    Timeout_timer *pre;
    Timeout_timer *next;
    int sockfd;
public:
    Timeout_timer():pre(NULL), next(NULL){}
};

// 定时器列表以升序排列
class Timer_list {
private:
    Timeout_timer *head;
    Timeout_timer *tail;
    
    void add_timer(Timeout_timer *timer,Timeout_timer *head);

public:
    static int u_pipfd;
    Timer_list();
    ~Timer_list();
    // 加入定时任务timer
    void add_timer(Timeout_timer *timer);
    // 从队列中删除定时器timer
    void del_timer(Timeout_timer *timer);
    // 查看是否有任务超时，超时就将他剔除队列
    std::vector<int> tick(); //
    // 调整timer在队列中的顺序
    void adjust_list(Timeout_timer *timer); 
    // 发送信号给通道
    static void send_sig(int fd);
};

// class Complete_Timer {
// public:
//     // 通道文件描述符
//     static int u_pipefd;
//     // 定时器链表
//     Timer_list m_timer_list;
//     // cpu epoll事件文件描述符
//     static int u_epollfd;
//     // 超时时间
//     int TIMEOUT;
//     void (*sig_func)(int);

// public:
//     Complete_Timer();
//     ~Complete_Timer();

//     // 初始化函数
//     void Init(int out);
//     // 设置非阻塞
//     int Set_noblock(int fd);
//     // 将定时器事件和读事件注册到内核注册表中
//     void add_EPOLL(int epollfd,int fd,int one_shot, int TRIGMode);
//     // 信号处理事件
//     void sig_handler(int sig);
//     // 设置需要捕捉信号
//     void add_sig(int sig ,void(*handler)(int),bool restart= true);
//     // 定时处理函数 定时发送SIGALRM信号给
//     void time_handle();
//     // 输出错误信息
//     void show_error(int confd, char * info);

//     void del_func(int fd); // 将fd从epoll 中删除
// };


#endif
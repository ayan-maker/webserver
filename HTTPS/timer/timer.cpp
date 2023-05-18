#include <stdio.h>
#include "timer.h"
#include <time.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../webserver/webserver.h"

int Timer_list::u_pipfd = 0;

Timer_list::Timer_list() {
    head =NULL;
    tail = NULL;
}

Timer_list::~Timer_list() {
    while (head) {
        Timeout_timer *temp;
        temp = head;
        head = temp->next;
        delete temp;
    }
    tail = NULL;
}

void Timer_list::add_timer(Timeout_timer *timer) {
    if(!timer) {
        return;
    }
    if(head == NULL) {
        head = timer;
        tail = timer;
        return;
    }
    if(timer->expire < head->expire) {
        timer->next = head;
        head->pre = timer;
        head = timer;
        return;
    }
    if(timer->expire > tail->expire) {
        tail->next = timer;
        timer->pre = tail;
        tail = timer;
        return;
    }
    add_timer(timer,head);
}

void Timer_list::del_timer(Timeout_timer *timer) {
    if(!timer) {
        return;
    }
    if(head == NULL) {
        return;
    }
    if((head == tail) && (timer == head)) {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if(timer == head) {
        head = head->next;
        head->pre = NULL;
        delete timer;
        return;
    }
    if(head == tail) {
        tail = tail->pre;
        tail->next = NULL;
        delete timer;
        return;
    }

    timer->next->pre = timer->pre;
    timer->pre->next = timer->next;
    delete timer;
}

void Timer_list::add_timer(Timeout_timer *timer,Timeout_timer *head) {
    Timeout_timer *p = head;
    while(p){
        if(p->expire > timer->expire) {
            p->pre->next = timer;
            timer->pre = p->pre;
            timer->next = p;
            p->pre = timer;
            break; 
        }
        else {
            p=p->next;
        }
    }
}

void Timer_list::adjust_list(Timeout_timer *timer) {

    del_timer(timer);
    add_timer(timer);
}
// 从队伍中将超时的任务剔除
std::vector<int> Timer_list::tick() {
    std::vector<int> ans;
    if(!head) {
        return ans;
    }

    time_t now = time(NULL);
    Timeout_timer *p = head;
    while (p) {
        if(p->expire < now) {
            Timeout_timer *q=p->next;
            ans.push_back(p->sockfd);
            // func(p->user_data->sockfd);
            // delete p;
            p=q;
        }
        else {
            break;
        }
    }
    return ans;
}

void Timer_list::send_sig(int sig) {
    int saved_errno = errno;
    char s[50];
    memset(s,0,sizeof(s));
    sprintf(s,"%d",sig);
    // printf("pip : %d\n",u_pipfd);
    int a=write(u_pipfd,s,strlen(s));
    printf("pip send %d\n",a);
    if(a<=0) {
        printf("write sig error%s\n",strerror(errno));
    }
    errno = saved_errno;
    // alarm(10);
}
// // 静态变量外部定义
// int Complete_Timer::u_pipefd = 0;
// int Complete_Timer::u_epollfd= 0;

// void Complete_Timer::Init(int out) {
//     TIMEOUT = out;

// }

// int Complete_Timer::Set_noblock(int fd) {
//     // int fcntl(int fd, int cmd, ... /* arg */ );可以用来改变文件描述符的状态或属性
//     // 1、获取文件标志位
//     int flag = fcntl(fd, F_GETFL, 0);
//     if(flag == -1){
//         return -1;
//     }
//     // 2、设置标志位
//     return fcntl(fd, F_SETFL, flag | O_NONBLOCK); 
// }

// void Complete_Timer::add_EPOLL(int epollfd,int fd,int one_shot, int TRIGMode) {
//     struct epoll_event ev;
//     ev.data.fd = fd;
//     // EPOLLIN 可以读  ， EPOLLRDHUP对端已经关闭连接就挂起
//     if(TRIGMode == 1 ){  // 设置为边缘触发 EPOLLET 
//         ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
//     }
//     else {
//         ev.events = EPOLLIN | EPOLLRDHUP;
//     }
//     if(one_shot == 1){
//         // // 表示一个文件描述符（通常是一个套接字）在触发一个 EPOLLIN 或 EPOLLOUT 事件后，将自动从 epoll 监听队列中移除，以避免被重复触发
//         ev.events |= EPOLLONESHOT; 
//     }
//     epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
//     Set_noblock(fd);
// }

// // 通过通道发送信号给主程序
// void Complete_Timer::sig_handler(int sig) {
//     // 为了保证可重复性，保存原来的errno
//     int sig_error = errno;
//     char *s;
//     sprintf(s,"%d",sig);
//     send(u_pipefd,s,sizeof(s),0);
//     errno = sig_error;

// }

// void Complete_Timer::add_sig(int sig ,void(*handler)(int),bool restart) {
//     // 1、声明一个sigaction结构体
//     struct sigaction Sig;
//     memset(&Sig, 0, sizeof(Sig));
//     // 2、初始化结构体
//     Sig.sa_handler = handler;
//     if(restart) {
//         Sig.sa_flags = SA_RESTART;
//     }
//     // sigemptyset(&Sig.sa_mask);// 将一个信号集合设置为空集。
//     sigfillset(&Sig.sa_mask); // 函数将所有的信号都放在sa_mask集合中 可以屏蔽所有信号
//     // 3、调用函数设置，设置信号对那个信号进行处理
//     // 用于在程序中添加断言（assertion）检查,它的作用是在程序运行时判断一个表达式是否为真，如果表达式的值为假，就会触发一个错误，终止程序运行
//     assert(sigaction(sig, &Sig, NULL) == 0); 

//     // sigset_t set; // 设置信号集
//     // sigemptyset(&set); 
//     // sigaddset(&set,sig);
//     // 
//     // signal(sig,handler);直接设置
// }

// void Complete_Timer::time_handle() {
//     m_timer_list.tick();

//     alarm(TIMEOUT);
// }

// void Complete_Timer::show_error(int confd, char * info) {
//     // 将错误信息发送到文件中
//     send(confd, info, strlen(info),0);
//     // 关闭连接
//     close(confd);
// }

// void Complete_Timer::del_func(int fd) {
//     epoll_ctl(u_epollfd,EPOLL_CTL_DEL,fd,NULL);

// }
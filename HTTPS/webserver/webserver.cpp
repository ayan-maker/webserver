#include <stdio.h>
#include "webserver.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <exception>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h> // IP装换
#include <fcntl.h>
#include <errno.h>


void addfd(int epollfd,int socketfd,bool TRIGMode,bool oneshot) {
    epoll_event events;
    memset(&events,0,sizeof(events));
    events.data.fd = socketfd;
    events.events = EPOLLIN |  EPOLLRDHUP;
    if(TRIGMode) { // 边缘触发
        events.events |= EPOLLET;
    }
    if(oneshot) {
        events.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,socketfd,&events);
}

void movedf(int epollfd,int socketfd) {
    epoll_ctl(epollfd,EPOLL_CTL_DEL,socketfd,0);
    close(socketfd);
}

// 设置非阻塞方式
int set_nolock(int fd) {
    int flat = fcntl(fd, F_GETFL, 0);
    if(flat == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flat | O_NONBLOCK);
    
}

Webserver::Webserver(){
}

Webserver::~Webserver() {
    if(m_listensk) {
        close(m_listensk);
        m_listensk=0;
    }
    m_mysql_pool->Destroypool();
    delete m_pthread_pool;
    delete [] m_http_gourp;
    m_free_conn.clear();
    close(m_pipfd[0]);
    close(m_pipfd[1]);
    LOG_INFO("Webserver closed");
    
}

void Webserver::init_server(int MAX_PTH,int max_requests, int state, char * ip,char* user,char * password,char *datename,int port,int max_conn,int close_log ,char *logfile) {
    m_port = port;
    strcpy(m_ip,ip);
    strcpy(m_user,user);
    strcpy(m_password,password);
    strcpy(m_database,datename);
    m_MAX_PTH = MAX_PTH;
    m_max_requests = max_requests;
    m_state = state;
    m_MAX_CONN = max_conn;
    m_close_log = close_log;
    m_out_time = 10;
    m_epollfd = epoll_create(1);
    m_sub_epollfd = epoll_create(1);
    strcpy(LOG_FILE,logfile);
    strcat(LOG_FILE,"/logs");


    // 设置信号处理函数
    sigset_t set;
    sigfillset(&set); // 将所有的信号都放进信号集合中
    sigdelset(&set,SIGINT); // 将SIGINT信号从信号集中删除
    sigdelset(&set,SIGALRM); // 将定时器的信号放进信号集中
    sigprocmask(SIG_SETMASK,&set,NULL); // 将信号集的所有信号屏蔽
    struct sigaction sa;
    void (*sig_func)(int) = Timer_list::send_sig;
    sa.sa_handler = (__sighandler_t)sig_func;
    // sa.sa_handler = (__sighandler_t)send_sig;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGALRM,&sa,NULL)==-1) {
        printf("sigaction failed %s\n",strerror(errno));
        exit(1);
    }

    // 初始化定时器
    
    // m_timer.Init(10);
    // m_timer.u_epollfd = m_sub_epollfd;
    // m_timer.u_pipefd = m_pipfd[1];


    if(!close_log) {
        if(!log_file()) {
            printf("log failed\n");
        }
    }
    
    // 初始化HTTP连接先开辟在使用
    m_http_gourp = new Http_conn[m_max_requests];
    for(int i=0; i<m_max_requests;++i) {
        // m_http_gourp[i].Init();
        m_free_conn.push_back(&m_http_gourp[i]);
    }

    // 开辟通道为定时器任务
    if(pipe(m_pipfd) == -1) {
        printf("can't open pip %s\n",strerror(errno));
        exit(1);
    }

    Timer_list::u_pipfd = m_pipfd[1];
    // 将pip通道添加到epoll中进行管理
    epoll_event events;
    memset(&events,0,sizeof(events));
    events.data.fd = m_pipfd[0];
    events.events = EPOLLIN;
    epoll_ctl(m_sub_epollfd,EPOLL_CTL_ADD,m_pipfd[0],&events);
    if(set_nolock(m_pipfd[0])==-1 || set_nolock(m_pipfd[1])==-1) {
        printf("set_nolock failed %s\n",strerror(errno));
    }
    // m_http_list = new BlockQueue<Http_conn*>(m_max_requests);
    // for(int i = 0; i < m_max_requests; i++) {
    //     m_http_list->push(&m_http_gourp[i]);
    // }
    
    init_listen();
    // pthread_detach(main_pthread); // 将线程和主线程分离
}

void Webserver::init_listen() {
    // 自定义ip地址
    // in_addr_t ip_uint =0;
    // inet_pton(AF_INET,m_ip,&ip_uint); // 将ip字符串转成32位无符号整数
    // printf("%d\n",ip_uint);
    // 初始化监听socket
    // 1、创建socket
    m_listensk = socket(AF_INET, SOCK_STREAM, 0);
    // 2、绑定socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; // ip协议版本
    addr.sin_port = htons(m_port); // 通信端口，将主机字节序转换成网络字节序
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 将主机地址转变成网络字节序
    int flag = 1;
    setsockopt(m_listensk, SOL_SOCKET, SO_REUSEADDR,&flag,sizeof(flag)); // 设置socket允许重用本地地址端口
    // 如果 linger 选项被启用，那么在关闭 socket 时，如果还有数据没有发送完成，
    // 操作系统将会等待 l_linger 秒钟，然后强制关闭 socket，这样可以确保数据被发送完整
    struct linger tmp = {1,1};
    setsockopt(m_listensk,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp)); 
    // 2、绑定ip地址
    if(bind(m_listensk,(struct sockaddr*)&addr,sizeof(addr)) != 0) {
        // ip 和 socket 绑定失败
        close(m_listensk);
        m_listensk =0;
        printf("error %s",strerror(errno));
        LOG_ERROR("ip bind failed");
        exit(1);
        // throw std::exception();
    }
    // LOG_INFO("ip bind succeeded");
    // 3、把socke设置为监听模式 标记作用
    if(listen(m_listensk,128)!=0) {
        close(m_listensk);
        m_listensk =0;
        LOG_ERROR("socket set listen failed");
    }

    /*
    监听socket也会有缓冲区，只不过它的缓冲区和普通的连接socket的缓冲区不同。
    监听socket的缓冲区用于存储已经连接但还没有被accept处理的连接请求。
    只有执行了accept函数之后监听缓冲区的数据才会被拿走，所以这里要使用边缘触发，
    如果使用水平触发在accept之前就会持续的触发
    */
    epoll_event events;
    memset(&events,0,sizeof(events));
    events.data.fd = m_listensk;
    events.events = EPOLLIN |  EPOLLET;
    epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_listensk,&events);
    set_nolock(m_listensk); // 设置为非阻塞
    

    // 启动监听线程
    pthread_t main_pthread;
    pthread_t sub_pthread;
    
    pthread_create(&main_pthread,NULL,&Listen,this);
    usleep(10);
    pthread_create(&sub_pthread,NULL,&deal_event,this);
    mysql_pool();
    pthread_pool();
    alarm(m_out_time);
    pthread_join(main_pthread,NULL);
    pthread_join(sub_pthread,NULL);
}

// 单Reactor多线程模型
void Webserver::Reactor_one() {
    int infd = 0;
    struct epoll_event epevent[MAX_NUM];
    
    while(1) {
        memset(&epevent,0,sizeof(epevent));
        // 用来排除epoll_wait()函数收到中断信号的干扰 
        while((infd = epoll_wait(m_epollfd,epevent,MAX_NUM,50000)) == -1 && errno == EINTR); // 等待事件发生，返回的是事件发生的个数
        
        if(infd <0) {
            LOG_ERROR("eopll wait failed");
            // throw std::exception();
            break;
        }
        if(infd == 0) {
            LOG_ERROR("eopll wait timeout");
            break;
        }
        for(int i=0;i<infd;++i) { // 遍历所有连接事件
            int event_fd = epevent[i].data.fd;
            // 要确定是listensk的事件 以及事件是不是读时间 也就是连接事件
            if(event_fd == m_listensk && (epevent[i].events & EPOLLIN)) {
                // 开始进行连接
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                memset(&addr, 0, sizeof(addr));
                int clientsk = accept(m_listensk,(sockaddr*)&addr,&addrlen);
                
                
                if(clientsk == -1) {
                    printf("Accept ERROR %s\n",strerror(errno)); // 打印错误信息
                    LOG_ERROR("socket accept error");
                    continue; // 继续下一个
                }
                addfd(m_epollfd,clientsk,false,false);
                set_nolock(clientsk); // 将socket设置为非阻塞io
                // if(m_http_list->isempty()) {
                //     continue;
                // }
                // m_lock.lock();
                // Http_conn *task;
                // m_http_list->re_front(task);
                // m_http_gourp[clientsk % m_max_requests].Init(clientsk,m_epollfd,false,NULL);
                
                // m_lock.unlock();
                Http_conn *task = m_free_conn.front();
                m_free_conn.pop_front();
                task->Init(clientsk,m_epollfd,true,&addr);
                m_map.insert(std::make_pair(clientsk,task));
                LOG_INFO("task connected");
            } 
            else if(epevent[i].events & (EPOLLRDHUP)) {// 远程断开连接,EPOLLHUP | EPOLLERR表示连接出现异常
                // m_http_gourp[epevent[i].data.fd % m_max_requests]
                del_http(event_fd);
                // movedf(m_epollfd,event_fd);
            }
            else if(epevent[i].events & EPOLLIN) { // 有数据发过来
                // char pp[2000];
                // memset(pp, 0, sizeof(pp));
                // recv(event_fd, pp, sizeof(pp),0);
                // printf("socket %d , %s",event_fd,pp);
                read_event(event_fd);
            }
            else if(epevent[i].events & EPOLLOUT) { // 写入数据
                write_event(event_fd);
            }
        }
    }
}


void Webserver::main_reactor() {

    int indx = 0;
    struct epoll_event epevent[MAX_NUM];
    
    while(1) {
        memset(&epevent,0,sizeof(epevent));
        // 用来排除epoll_wait()函数收到中断信号的干扰 
        while((indx = epoll_wait(m_epollfd,epevent,MAX_NUM,50000)) == -1 && errno == EINTR); // 等待事件发生，返回的是事件发生的个数
        
        if(indx <0) {
            LOG_ERROR("eopll wait failed");
            // throw std::exception();
            break;
        }
        if(indx == 0) {
            LOG_ERROR("eopll wait timeout");
            break;
        }
        for(int i=0;i<indx;++i) { // 遍历所有连接事件
            int event_fd = epevent[i].data.fd;
            // 要确定是listensk的事件 以及事件是不是读时间 也就是连接事件
            if(event_fd == m_listensk && (epevent[i].events & EPOLLIN)) {
                // 开始进行连接
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                memset(&addr, 0, sizeof(addr));
                int clientsk = accept(m_listensk,(sockaddr*)&addr,&addrlen);
                
                
                if(clientsk == -1) {
                    printf("Accept ERROR %s\n",strerror(errno)); // 打印错误信息
                    LOG_ERROR("socket accept error");
                    continue; // 继续下一个
                }
                addfd(m_sub_epollfd,clientsk,false,true);
                set_nolock(clientsk); // 将socket设置为非阻塞io
                m_lock.lock();
                Http_conn *task = m_free_conn.front();
                m_free_conn.pop_front();
                task->Init(clientsk,m_sub_epollfd,true,&addr);
                m_map.insert(std::make_pair(clientsk,task));
                // 生成定时器
                Timeout_timer * newtime = new Timeout_timer;
                newtime->sockfd = clientsk;
                time_t now = time(NULL);
                newtime->expire = now + m_out_time;
                m_time_list.add_timer(newtime);
                m_timer_map.insert(std::make_pair(clientsk,newtime));
                m_lock.unlock();
                // 将新的socket传给从线程
                // write(m_pipfd[1],&clientsk,4);
                LOG_INFO("task connected");
            } 
        }
    }
}

void Webserver::sub_reactor() {

    struct epoll_event epevent[MAX_NUM];
    while (1) {
        memset(&epevent, 0, sizeof(epevent));
        int indx = 0;
        while((indx = epoll_wait(m_sub_epollfd,epevent,MAX_NUM,50000)) == -1 && errno == EINTR); 
        if(indx==-1) {
            printf("epoll_wait error: %s\n",strerror(errno));
            exit(1);
        }
        if(indx==0) {
            printf("epoll_wait out time\n");
            exit(1);
        }
        for(int i=0;i<indx;i++) {
            int sk = epevent[i].data.fd;
            if(epevent[i].events & EPOLLRDHUP) {
                del_http(sk);
            }
            else if(sk == m_pipfd[0] && (epevent[i].events & EPOLLIN)) {
                // 计时器任务
                time_event();
            }
            else if(epevent[i].events & EPOLLOUT) {
                write_event(sk);
            }
            else if(epevent[i].events & EPOLLIN) {
                read_event(sk);
            }

        }
    }
}

void *Webserver::Listen(void *arg) {
    
    Webserver *web = (Webserver*)arg;
    // 信号处理 处理SIGINT信号其他全部屏蔽
    

    // epoll监听socket
    // 1、创建一个socket
    web->main_reactor();
    // struct epoll_event event;
    // int listensk = web->relistensk();
    // event.events = EPOLLIN | EPOLLRDHUP;
    // event.data.fd = listensk;
    // epoll_ctl(epollfd,EPOLL_CTL_ADD,listensk, &event); // 将监听sk放进
    
}

void *Webserver::deal_event(void *arg) {
     Webserver *web = (Webserver*)arg;
    web->sub_reactor();
}

// 事件
void Webserver::read_event(int fd) {
    m_lock.lock();
    Http_conn *task =m_map[fd];
    Timeout_timer * timer = m_timer_map[fd];
    assert(timer && task);
    time_t now = time(NULL);
    timer->expire=now + m_out_time;
    m_time_list.adjust_list(timer);
    if(!m_pthread_pool->Append(task,1)) {
        del_http(fd);
    }
    m_lock.unlock();
}

void Webserver::del_http(int fd) {
    m_lock.lock();
    Http_conn *task =m_map[fd];
    Timeout_timer * timer = m_timer_map[fd];
    assert(task && timer);
    task->close_mysql(m_mysql_pool);
    task->close_conn();
    task->Init();
    m_map.erase(fd);
    m_free_conn.push_back(task);
    m_time_list.del_timer(timer);
    m_timer_map.erase(fd);
    m_lock.unlock();
    movedf(m_sub_epollfd,fd);
}

void Webserver::write_event(int fd) {
    m_lock.lock();
    Http_conn *task =m_map[fd];
    assert(task);
    m_pthread_pool->Append(task,0);
    
    m_lock.unlock();
}

void Webserver::time_event() { 
    char buf[1024];
    memset(buf,0,1024);
    read(m_pipfd[0],buf,1024);
    std::vector<int> Out = m_time_list.tick();
    int n = Out.size();
    if(n>0) {
        for(int i=0; i<n; i++) {
            del_http(Out[i]);
        }
    }
    printf("time out cheak !\n");
    // alarm(m_out_time);
}


bool Webserver::add_request(Http_conn *conn,int stats) {
    return m_pthread_pool->Append(conn,stats);
}

int Webserver::relistensk() const {
    return m_listensk;
}

// 初始化相关模块

// 线程池开辟空间
void Webserver::pthread_pool() { 
    m_pthread_pool = new PthreadPool<Http_conn>(m_MAX_PTH,m_max_requests, m_state,m_mysql_pool);
    LOG_INFO("pthread pool initialized");

}

// 创建连接池
void Webserver::mysql_pool() {
    m_mysql_pool = Connection_pool::GetInstance();
    m_mysql_pool->Init( "localhost",m_user,m_password,m_database,3306,m_MAX_CONN,m_close_log);
    LOG_INFO("mysql connection pool initialized");

} 

// 单例模式初始化日志
bool Webserver::log_file() {
    m_web_log = Log::get_instance();
    
    if(!m_web_log->init(LOG_FILE,m_close_log)){
        return  false;
    }
    LOG_INFO("log init successful");
    return true;
}


void Webserver::set_out_time(int out_time) {
    m_out_time = out_time;
}

#include "log.h"
#include <pthread.h>
#include <cstring>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>

std::vector<int> ad;

// 异步将缓存中的值写入到文件中
void *Log::async_write_log() {
    std::string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (m_log_queue->pop(single_log)){
        m_lock.lock();
        fprintf(m_fp,single_log.c_str());
        m_lock.unlock();
    }   
}

Log::Log() {
    m_count = 0;
    m_is_async = false;
}

Log::~Log() {
    if(m_fp != NULL) {
        fclose(m_fp);
    }
}

bool Log::init(const char *file_name,int close_log ,int log_buf_size, int split_lines, int max_queue_size) {
    // 如果max_queue_size设置了，则表示为异步的
    if(max_queue_size >=1) {
        m_log_queue = new BlockQueue<std::string>(max_queue_size);
        m_is_async = true;
        pthread_t pt;
        //  创建一个线程异步写日志，flush_log_thread是回调函数
        pthread_create(&pt, NULL,flush_log_thread,NULL);
    }
    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    max_split_lines = split_lines;
    // 初始化日志缓冲区
    m_buf =  new char[m_log_buf_size];
    memset(m_buf,'\0',m_log_buf_size);
    // 提取时间
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    m_today = t->tm_mday;
    // 文件名和路径
    char log_full_name[128]={0};
    memcpy(log_full_name,file_name,strlen(file_name));
    char *s=strrchr(log_full_name,'/');
    if(s!=NULL) { // 给了log存放的位置
        strcpy(log_name,s+1);
        memcpy(dir_name,log_full_name,s-log_full_name+1);
        snprintf(log_full_name,255,"%s%d_%02d_%02d_%s",dir_name,t->tm_year,t->tm_mon,t->tm_mday,log_name);
    }
    else {
        snprintf(log_full_name,255,"%d_%02d_%02d_%s",t->tm_yday,t->tm_mon,t->tm_mday,file_name);
    }
    
    m_fp = fopen(log_full_name,"a");
    if(m_fp== NULL) {
        printf("can't open log file %s",strerror(errno));
        return false;
    }
    return true;
}
// 将日志文件写入m_buf中 放在缓存中或者是直接写入文档中
void Log::write_log(int level, const char *format, ...) {
    auto now = time(NULL);
    struct tm *t = localtime(&now);
    char s[16]={0};
    switch (level) {
        case 0:
            strcpy(s,"[debug]:");
            break;
        case 1:
            strcpy(s,"[info]:");
            break;
        case 2:
            strcpy(s,"[warn]:");
            break;
        case 3:
            strcpy(s,"[error]:");
            break;
        default:
            strcpy(s,"[info]:");
            break;
    }
    // 开始写log
    m_lock.lock();
    ++m_count;
    // 记录每天的log 不同的放在不同的log中
    if(m_today != t->tm_mday || m_count % max_split_lines == 0) {
        char tail[16]={0};
        char new_log[255]={0};
        fflush(m_fp); // 刷新输出流的缓冲区
        fclose(m_fp); 
        snprintf (tail, 16, "%d_%02d_%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday);
        // 更换新的一天
        if(m_today != t->tm_mday) { 
            snprintf(new_log,255,"%s%s%S",dir_name,tail,log_name);
            m_today = t->tm_mday;
            m_count = 0;
        }
        else { // 文件写满了
            snprintf(new_log,255,"%s%s%S.%lld",dir_name,tail,log_name,m_count/max_split_lines); 
        }   
        m_fp=fopen(new_log,"a");
    }
    m_lock.unlock();

    va_list valist;
    va_start(valist,format);

    char log_str[1024];
    memset(log_str,0,1024);
    m_lock.lock();
    int n = snprintf(m_buf,48,"%d-%02d-%02d %02d:%02d:%02d %s",t->tm_year,t->tm_mon,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,s);
    int m = vsnprintf(m_buf+n,m_log_buf_size,format,valist);
    m_buf[m+n]='\n';
    m_buf[m+n+1]='\0';
    memcpy(log_str,m_buf,strlen(m_buf));

    m_lock.unlock();
    // 异步日志
    if(m_is_async && !m_log_queue->isfull()) {
        m_log_queue->push(log_str);
    }// 同步
    else {
        m_lock.lock();
        fprintf(m_fp,log_str,sizeof(log_str));
        m_lock.unlock();
    }

    va_end(valist);
}
// 将缓存的内存写到文件中
void Log::flush() {
    m_lock.lock();
    fflush(m_fp);
    m_lock.unlock();
}
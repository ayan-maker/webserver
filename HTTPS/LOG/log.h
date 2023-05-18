#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include "../Block_queue/Block_queue.h"
#include <string>
#include "../LOCK/lock.h"
#include <stdarg.h>

class Log {
private:
    char dir_name[128]; // 路径名
    char log_name[128]; // log文件名
    int max_split_lines; // 日志最大行数
    int m_log_buf_size; // 日志缓冲区大小
    long long m_count; // 日志行数记录
    int m_today; // 记录当天是那一天
    FILE *m_fp; // 打开log文件的指针
    char *m_buf; 
    BlockQueue<std::string> *m_log_queue; // 阻塞队列
    bool m_is_async;  // 是否同步标志位
    Locker m_lock; // 锁
    int m_close_log; // 关闭日志

private:
    Log();
    virtual ~Log(); // 虚函数
    void *async_write_log();

public: 
    static Log *get_instance() { // 单例模式
        static Log instance;
        return &instance;
    }
    // 刷新log
    static void *flush_log_thread (void *args) {
        Log::get_instance()->async_write_log();
    }

    bool init(const char *file_name,int close_log ,int log_buf_size = 8192, int split_lines = 500000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);
};

// 知识点：##VA_ARGS 可以用在宏定义中的参数列表中，用于将可变参数列表转换成一个参数，并且在可变参数列表为空时可以避免额外的逗号
#define LOG_DEBUG(format,...) if(0 == m_close_log) {Log::get_instance()->write_log(0,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format,...) if(0 == m_close_log) {Log::get_instance()->write_log(1,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format,...) if(0 == m_close_log) {Log::get_instance()->write_log(2,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format,...) if(0 == m_close_log) {Log::get_instance()->write_log(3,format,##__VA_ARGS__); Log::get_instance()->flush();}


#endif
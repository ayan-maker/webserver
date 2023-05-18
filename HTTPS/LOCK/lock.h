#ifndef _LOCK_H_
#define _LOCK_H_

#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<exception> // 异常

// 封装sem信号量函数
class Sem{
    private:
        sem_t sem;

    public:
        Sem();
        Sem(int num);
        ~Sem();
        bool wait();
        bool post();
};

// 线程互斥锁
class Locker {
    private:
        pthread_mutex_t mutex;

    public:
        Locker();
        ~Locker();

        bool lock();
        bool unlock();

        pthread_mutex_t * get();
};

/*
* 条件变量（Condition Variable），它是 POSIX 线程库中用于等待和通知线程的机制，用于线程间的同步
* 只有满足条件变量才可以解锁或者是上锁
* 有以下需要注意的：
* 1、等待线程在等待条件变量前必须获得相应的互斥锁，以保证条件检查和等待操作是原子的，防止其他线程修改共享数据导致的竞争情况。
* 2、发送线程在发送条件变量通知前必须获得相应的互斥锁，以保证等待线程能够正确检查条件。
* 3、等待线程在收到条件变量通知后应该重新检查条件是否满足，因为可能出现假唤醒情况（即条件变量信号被意外地发送）。
*/
class Cond {
private:
    pthread_cond_t cond;

public:
    Cond();
    ~Cond();
    // 等待线程和变量
    bool wait(pthread_mutex_t *mutex);
    // 设置了超时的等待
    bool wait_time(pthread_mutex_t *mutex,timespec *time);
    // 通知一个等待在条件变量
    bool signal();
    // 通知所有等待的变量
    bool broadcast();
};

#endif
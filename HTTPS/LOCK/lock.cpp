#include "lock.h"

Sem::Sem() {
    try {
        if(sem_init(&sem,0,0)!=0) {
            throw std::exception();
        }
    }
    catch(std::exception& ex) {
            fprintf(stderr,"ERROR: %s",ex.what());
    }    
}

 Sem::Sem(int num) {
        try {
            if(sem_init(&sem,0,num)!=0) {
                throw std::exception();
            }
        }
        catch(std::exception& ex) {
             fprintf(stderr,"ERROR: %s",ex.what());
        }
            
}
Sem::~Sem() {
    sem_destroy(&sem);
}

bool Sem::wait() {
    return sem_wait(&sem) == 0; // 等待信号量，信号量为0时便是没有信号信号量需要等待也就是阻塞
}


bool Sem::post() {
        return sem_post(&sem) == 0; // 释放信号量 信号量加一 ，并唤醒其他的被阻塞的程序,成功返回0 否则返回失败
}

Locker::Locker() {
    try {
        if(pthread_mutex_init(&mutex,NULL) != 0) {
            throw std::exception();
        }
    }
    catch(std::exception & ex) {
        fprintf(stderr,"ERROR: %s",ex.what());
    }
}
Locker::~Locker() {
    pthread_mutex_destroy(&mutex);
}

bool Locker::lock() {
    return pthread_mutex_lock(&mutex) == 0; // 尝试获取互斥锁
}

bool Locker::unlock() {
    return pthread_mutex_unlock(&mutex) == 0; // 解除互斥锁
}

pthread_mutex_t * Locker::get() {
    return &mutex;
}

Cond::Cond() {
    try {
        if(pthread_cond_init(&cond,NULL)!=0) {
            throw std::exception();
        }
    }
    catch(std::exception &e) {
        fprintf(stderr,"ERROR: %s",e.what());
    }
}
Cond::~Cond() {
    pthread_cond_destroy(&cond);
}
// 等待线程和变量
bool Cond::wait(pthread_mutex_t *mutex) { 
    return pthread_cond_wait(&cond,mutex) == 0;
}
// 设置了超时的等待
bool Cond::wait_time(pthread_mutex_t *mutex,timespec *time) { 
    return pthread_cond_timedwait(&cond,mutex,time)==0;
}
// 通知一个等待在条件变量
bool Cond::signal() { 
    return pthread_cond_signal(&cond) == 0;
}
// 通知所有等待的变量
bool Cond::broadcast() {
    return pthread_cond_broadcast(&cond) == 0;
}
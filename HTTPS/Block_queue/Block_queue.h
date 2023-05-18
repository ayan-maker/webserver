/**********************************************************
* 用于处理阻塞队列的问题 
* 循环队列
************************************************************/

#ifndef _BLOCK_QUEUE_H_
#define _BLOCK_QUEUE_H_

#include <stdio.h>
#include <sys/time.h>
#include "../LOCK/lock.h"
#include <stdlib.h>

template <typename T>
class BlockQueue {
private:
    Locker m_lock;
    Cond m_cond;

    T *queue;
    int max_size; // 队列最大的数量
    int size;   // 当前的数量
    int front; 
    int back;

public:
    BlockQueue(int max) {
        if(max <= 0) {
            exit(-1);
        }
        max_size = max;
        queue = new T[max_size];
        size = 0;
        front = -1;
        back = -1;
    }

    ~BlockQueue() {
        m_lock.lock();
        if(queue != NULL) {
            delete [] queue;
        }
        m_lock.unlock();
    }

    inline void clear() {
        m_lock.lock();
        size = 0;
        front = -1;
        back = -1;
        m_lock.unlock();
    }

    inline bool isfull() {
        m_lock.lock();
        if(size == max_size) {
            return true;
        }
        m_lock.unlock();
        return false;
    }

    inline bool isempty() {
        m_lock.lock();
        if(size ==  0) {
            return true;
        }
        m_lock.unlock();
        return false;
    }

    inline bool re_front(T &val) {
        m_lock.lock();
        if(front == -1) {
            return false;
        }
        val = queue[front];
        m_lock.unlock();
    }

    inline bool re_back(T &val) {
        m_lock.lock();
        if(back == -1) {
            return false;
        }
        val = queue[back];
        m_lock.unlock();
        return true;
    }

    inline int re_size() {
        m_lock.lock();
        int re = size;
        m_lock.unlock();
        return re;
    }

    inline int re_max_size() {
        m_lock.lock();
        int re = max_size;
        m_lock.unlock();
        return re;
    }

    inline bool push(T val) {
        m_lock.lock();
        if(size == max_size){
            m_cond.broadcast();
            m_lock.unlock();
            return false;
        }
        ++size;
        back = (back + 1) % max_size;
        queue[back] = val;
        m_cond.broadcast();
        m_lock.unlock();
        return true;
    }
    // pop时，如果队列中没有元素，要等待条件变量
    inline bool pop(T &val) {
        m_lock.lock();
        if(size <= 0) {
            if(!m_cond.wait(m_lock.get())) { // 等待条件变量发生变化，也就是队伍中出现新的元素
                m_lock.unlock();
                return false; 
            }
        }
        --size;
        val=queue[front];
        front = (front + 1) % max_size;
        m_lock.unlock();
        return true;
    }

    inline bool pop (T &val,int ms_timeout) {
        struct timespec t = {0,0};
        struct timeval now= {0,0};
        gettimeofday(&now,NULL);
        m_lock.lock();
        if(size <= 0) {
            t.tv_sec = now.tv_sec+ms_timeout/1000;
            t.tv_nsec = (now.tv_usec%1000) * 1000;
            if(!m_cond.wait_time(m_lock.get(),&t)) { // 等待条件变量发生变化，也就是队伍中出现新的元素，加入了超时的设置
                m_lock.unlock();
                return false;
            }
        }
        // 再次确认一下是否队列中有元素
        if(size <= 0) {
            m_lock.unlock();
            return false;
        }
        --size;
        val=queue[front];
        front = (front + 1) % max_size;
        m_lock.unlock();
        return true;
    }
};



#endif
#include "Block_queue.h"
#include "../http_conn/http_conn.h"
#include <string>

template <typename T>
BlockQueue<T>::BlockQueue(int max) {
    if(max <= 0) {
        exit(-1);
    }
    max_size = max;
    queue = new T[max_size];
    size = 0;
    front = -1;
    back = -1;
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
    m_lock.lock();
    if(queue != NULL) {
        delete [] queue;
    }
    m_lock.unlock();
}

template <typename T>
void BlockQueue<T>::clear() {
    m_lock.lock();
    size = 0;
    front = -1;
    back = -1;
    m_lock.unlock();
}

template <typename T>
bool BlockQueue<T>::isfull(){
    m_lock.lock();
    if(size == max_size) {
        return true;
    }
    m_lock.unlock();
    return false;
}

template <typename T>
bool BlockQueue<T>::isempty(){
    m_lock.lock();
    if(size ==  0) {
        return true;
    }
    m_lock.unlock();
    return false;
}

template <typename T>
bool BlockQueue<T>::re_front(T &val){
    m_lock.lock();
    if(front == -1) {
        return false;
    }
    val = queue[front];
    m_lock.unlock();

    return true;
}

template <typename T>
bool BlockQueue<T>::re_back(T &val){
    m_lock.lock();
    if(back == -1) {
        return false;
    }
    val = queue[back];
    m_lock.unlock();
    return true;
}

template <typename T>
int BlockQueue<T>::re_size(){
    m_lock.lock();
    int re = size;
    m_lock.unlock();
    return re;
}

template <typename T>
int BlockQueue<T>::re_max_size(){
    m_lock.lock();
    int re = max_size;
    m_lock.unlock();
    return re;
}

template <typename T>
bool BlockQueue<T>::push(T val) {
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
template <typename T>
bool BlockQueue<T>::pop(T &val) {
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

template <typename T>
bool BlockQueue<T>::pop (T &val,int ms_timeout) {
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

template class BlockQueue<std::string>;
template class BlockQueue<Http_conn*>;
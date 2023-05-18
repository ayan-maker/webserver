#include "pthreadpool.h"

template<typename T>
PthreadPool<T>::PthreadPool(int MAX_PTH,int requets_num ,int state, Connection_pool *pool) {
    
    m_MAX_PTH = MAX_PTH;
    m_max_requests = requets_num;
    m_state = state;
    m_pool = pool;
    // m_timer.Init();
    try{
        if(m_max_requests <=0 || m_MAX_PTH <=0){
            printf("创建线程失败\n");
            throw std::exception();
        }
        pth_list = new pthread_t[m_MAX_PTH];
        if(pth_list == NULL) {
            delete[] pth_list;
            printf("创建线程失败\n");
            throw std::exception();
        }
        for(std::size_t i=0 ; i<m_MAX_PTH ; i++) {
            if(pthread_create(&pth_list[i],NULL,worker,this) !=0) {
                
                delete[] pth_list; // 出现失败一定要删除堆栈
                printf("创建线程失败\n");
                throw std::exception();
            }
            if(pthread_detach(pth_list[i])){ // 用于将子线程和主线程分离，线程分离后不用其他线程等待结束，线程结束后释放资源不用pthread_join收回
                delete[] pth_list;
                printf("创建线程失败\n");
                throw std::exception();
            }
        }

    }
    catch(std::exception &ex) {
        printf("创建线程失败\n");
        fprintf(stderr, ex.what());
    }
    
}

template<typename T>
PthreadPool<T>::~PthreadPool() {
    delete[] pth_list;
}

// 添加任务到任务队列
template <typename T>
bool PthreadPool<T>::Append(T *request, int state) {
    m_lock.lock();
    if(m_work_list.size() >= m_max_requests){
        // delete request;
        m_lock.unlock();
        return false;
    }
    request->change_state(state);
    m_work_list.push_back(request);
    m_lock.unlock();
    m_sem.post(); // 队列中资源增加了加入队列并提醒线程处理任务
    return true;
}

template<typename T>
void *PthreadPool<T>::worker(void *arg) {
    PthreadPool *ph = (PthreadPool *)arg;
    ph->run();
    return ph;
}

template<typename T>
void PthreadPool<T>::run() {
    
    while(true){
        m_sem.wait(); // 等待队伍资源 队列中有元素了就下一步
        m_lock.lock(); // 锁住类的相关资源

        if(m_work_list.size() <=0){
            m_lock.unlock();
            continue;
        }
        T *request = m_work_list.front();
        m_work_list.pop_front();
        m_lock.unlock(); // 这里已经可以解锁了，因为后面的操作和任务队列没有关系了
        if(!request) {
            continue;
        }
        if(!request->re_mysql()) {
            request->con_mysql(m_pool);
        }
        request->process();
    }
    
}

template class PthreadPool<Http_conn>;
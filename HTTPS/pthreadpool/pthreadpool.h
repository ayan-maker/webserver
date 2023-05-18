#ifndef _PTHREADPOLL_H_
#define _PTHREADPOLL_H_

#include <pthread.h>
#include <stdio.h>
#include <list>
#include "../LOCK/lock.h"
// #include "../sql_connection_pool/sql_connection_pool.h"
#include <exception>
#include <stddef.h>
#include "../http_conn/http_conn.h"
#include "../timer/timer.h"

template <typename T>
class PthreadPool {
private:
    int m_MAX_PTH; // 最大的线程连接数
    int m_max_requests;  // 最大的请求数量
    int m_state; // 模式转换，同步/异步
    std::list<T *> m_work_list; // 保存任务的队列
    pthread_t * pth_list; // 线程队列
    Connection_pool *m_pool;
    Locker m_lock;  // 资源锁
    Sem m_sem; 
    // Complete_Timer m_timer;
    // Connection_pool * m_sql_conn_pool; // 数据库连接池
    

    // 任务函数，不断地从队伍中取出任务给线程工作
    inline void run() {
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

public:
    PthreadPool(int MAX_PTH,int requets_num, int state,Connection_pool *pool) {
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
    ~PthreadPool() {
        delete[] pth_list;
    } 
    
    // 将任务加入到任务队列中
    inline bool Append(T * request, int state) {
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
    
    inline static void *worker(void *arg) {
        PthreadPool *ph = (PthreadPool *)arg;
        ph->run();
        return ph;
    }
};


#endif
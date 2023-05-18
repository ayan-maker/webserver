#include "sql_connection_pool.h"
// #include "../lock.h"
// #include "../LOG/log.h"
// #include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// Connection_pool::pool = new Connection_pool;

Connection_pool::Connection_pool() {
    m_CONN =0;
    m_FREE_CONN =0;
}

Connection_pool* Connection_pool::GetInstance() {
    static Connection_pool pool;
    return &pool;
}

// void Connection_pool::DestroyInstance(Connection_pool *pool){
//     if(pool){
//         delete pool;
//         pool = NULL;
//     }
// }


void Connection_pool::Init(std::string url, std::string user, std::string password,std::string database_name,int Port, int MAX_CONN, int close_log) {
    m_url = url;
    m_user = user;
    m_passwd = password;
    m_database_name = database_name;
    m_close_log = close_log;
    m_port = Port;
    
    // 给所有的用户分配一个数据库资源
    for(int i=0;i<MAX_CONN;i++) {
        // 1、首先定义一个MYSQL并初始化MYSQL
        MYSQL *conn = mysql_init(NULL);
        
        if(conn==NULL){
            LOG_ERROR("MYSQL ERROR");
            
            exit(1);
        }
        printf("%p",conn);
        // 3、连接数据库
        conn = mysql_real_connect(conn,m_url.c_str(),m_user.c_str(),m_passwd.c_str()\
                                    ,m_database_name.c_str(),m_port,NULL,0);
        // conn = mysql_real_connect(conn,"localhost","root","456789","webserver",);
        if(conn == NULL) {
            LOG_ERROR("MYSQL ERROR");
            printf("msyql 问题 %s\n",strerror(errno));
            exit(1);
        }
        free_mysql_list.push_back(conn);
        mysql_list.push_back(conn);
        m_FREE_CONN++;
    }
    m_MAX_CONN = m_FREE_CONN;
    // 定义一个信息量用于锁住连接池不超出最大的数量
    reserve = Sem(m_FREE_CONN);
}

// 当出现请求时，从连接池中返回一个可用的连接
MYSQL * Connection_pool::Getconnection() {
    MYSQL *conn = NULL;
    // 如果连接池没有空闲的连接
    if(m_FREE_CONN <=0){
        return NULL;
    }
    // 首先等待信号量，等到信号量不为0 ,表示出现了空闲的连接
    reserve.wait();

    m_lock.lock();

    conn = free_mysql_list.front();
    free_mysql_list.pop_front();

    --m_FREE_CONN;
    ++m_CONN;

    m_lock.unlock();
    return conn;
}
// 释放连接
bool Connection_pool::Realseconnection(MYSQL *conn) {
    if(conn == NULL) {
        return false;
    }
    m_lock.lock();
    ++m_FREE_CONN;
    --m_CONN;
    free_mysql_list.push_back(conn);
    m_lock.unlock();
    reserve.post();
    return true;
}

// 得到当前的连接数量
int Connection_pool::Getfreeconn() {
    return m_FREE_CONN;  
}

void Connection_pool::Destroypool() {
    m_lock.lock();
    for(auto it = mysql_list.begin();it != mysql_list.end(); it++) {
        mysql_close(*it);
    }
    mysql_list.clear();
    free_mysql_list.clear();
    m_CONN =0;
    m_FREE_CONN =0;
    m_lock.unlock();
}

Connection_pool::~Connection_pool() {
    Destroypool();
}



//---------------------------Connection_RAII-------------------------------------
// 从池中获取一个连接
Connection_RAII::Connection_RAII(MYSQL *sql , Connection_pool *pool) {
    sql = pool->Getconnection();
    conRAII = sql;
    con_pool = pool;
}
// 将连接从池中删除
Connection_RAII::~Connection_RAII() {
    con_pool->Realseconnection(conRAII);
}
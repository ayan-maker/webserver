#ifndef _SQL_CON_POOL_
#define _SQL_CON_POOL_
// #include "../lock.h"
#include "../LOG/log.h"
#include <stdio.h>
#include <mysql/mysql.h>
#include <list>
#include <string>
#include <vector>

// Connection_pool::pool = new Connection_pool;

class Connection_pool {
private:
    int m_MAX_CONN; // 连接的最大个数
    int m_CONN; // 当前的连接数
    int m_FREE_CONN; // 当前空闲的连接数
    Locker m_lock;
    std::list<MYSQL *> free_mysql_list; // 连接池,这里存放的是空闲的连接
    std::vector<MYSQL *> mysql_list; // 记录所有的链接
    Sem reserve; // 信号量

    Connection_pool();
    ~Connection_pool();
    static Connection_pool *pool; // 唯一的实例对象指针

public: 
    std::string m_url; // 主机地址
    int m_port; //主机端口
    std::string m_user; // 用户名
    std::string m_passwd; // 用户密码
    std::string m_database_name; // 数据库名称
    int m_close_log; // 标志是否开启日志

   

    MYSQL * Getconnection(); // 获得数据库连接
    bool Realseconnection(MYSQL *); // 断开连接
    void Destroypool(); // 摧毁连接池
    int Getfreeconn(); // 获取连接还有多少剩余连接

    static Connection_pool * GetInstance(); // 单例运行

    // static void DestroyInstance(Connection_pool *pool); // 单例模式释放

    void Init(std::string ip, std::string user, std::string password,std::string database_name,int Port, int MAX_CONN, int close_log);
};

class Connection_RAII {
private:
    MYSQL *conRAII;
    Connection_pool *con_pool;

public:
    Connection_RAII(MYSQL *sql , Connection_pool *pool);
    ~Connection_RAII();
};

#endif
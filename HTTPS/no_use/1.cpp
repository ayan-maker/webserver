#include <string.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <errno.h>

using namespace std;

template<typename T>
class Stack {
public:
    Stack(int aa,int bb):a(aa),b(bb) {}
    void show() const { std::cout<<a<<b<<std::endl;}
    void push(const T& value) { data_.push_back(value); }
    void pop() { data_.pop_back(); }
    T& top() { return data_.back(); }
    bool empty() const { return data_.empty(); }
private:
    std::vector<T> data_;
    int a;
    int b;
};

int main () {
    Stack<int> stack(10,10);
    stack.show();
    FILE *f = fopen("./1.txt", "w");
    if(!f) {
        printf("Error open file\n");

    }
    fwrite("hello",10,1,f);
    MYSQL *conn = new MYSQL;
    conn = mysql_init(conn);
    
    if(conn==NULL){
        printf("Error creating connection\n");
        delete conn;
        exit(1);
    }
    // 3、连接数据库
    conn = mysql_real_connect(conn,"localhost","root","456789"\
                                ,"users",3306,NULL,0);
    if(conn == NULL) {
        printf("connection failed %s\n",strerror(errno));
        
    }
    printf("%s",&conn);
}
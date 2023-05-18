#include <stdio.h>

#include "http_conn.h"
#include <exception>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/epoll.h> 
#include <errno.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

using json = nlohmann::json;



void modfd(int epollfd,int socketfd,int ev,bool TRIGMode) {
    epoll_event events;
    memset(&events,0,sizeof(events));
    events.data.fd = socketfd;
    if(ev == 0) { // 读
        events.events = EPOLLIN;
    }
    else if(ev ==1){
        events.events = EPOLLOUT;
    }
    if(TRIGMode) { // 边缘触发
        events.events = events.events| EPOLLRDHUP | EPOLLET |EPOLLONESHOT ;
    }
    else {
        events.events = events.events| EPOLLRDHUP | EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_MOD,socketfd,&events);
}



const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";


void Http_conn::Init(int socket,int epollfd,bool TRIGMode, struct sockaddr_in *addr) {
    m_socket = socket;
    m_epollfd = epollfd;
    m_TRIGMode = TRIGMode;
    // addfd(m_epollfd,m_socket,TRIGMode);
    // m_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); // 参数分别为IPv4 stream套接字类型 协议
    memset(write_buf,'\0',sizeof(write_buf));
    
    wpos = 0;
    m_linger = true;
    m_resmessage_len = 0;
    // m_sql_name = sql_name;
    // m_sql_password = sql_password;
    // m_sql_name = sql_name;
    m_mysql = NULL; 
    
    if(!m_buf){
        m_buf = new char[READ_BUFFER_SIZE];
        m_buf_size = READ_BUFFER_SIZE;
    }
    memset(m_buf,'\0',m_buf_size);
    
}

void Http_conn::Init() {
    m_socket = 0;
    m_epollfd = 0;
    m_TRIGMode = false;
    m_req_method = NULL;
    m_url = NULL;
    m_version = NULL;
    m_header = NULL;
    m_post_payload = NULL;
    memset(write_buf,'\0',sizeof(write_buf));
    memset(m_buf,'\0',m_buf_size);
    memset(m_path,0,sizeof(m_path));
    wpos = 0;
    m_mysql = NULL;
    user = NULL;
    password = NULL;
    m_linger = true;
    m_resmessage_len = 0;
    m_mysql = NULL; 
    m_iv[0].iov_base = NULL;
    m_iv[0].iov_len = 0;
    m_iv[1].iov_base = NULL;
    m_iv[1].iov_len = 0;
}

bool Http_conn::con_mysql(Connection_pool *mysql_pool) {
    m_mysql = mysql_pool->Getconnection();
    // return m_mysql;
    if(!m_mysql) {
        return false;
    }
    return true;
}

void Http_conn::close_mysql(Connection_pool *mysql_pool){
    mysql_pool->Realseconnection(m_mysql);
    m_mysql = NULL;
}

void Http_conn::close_conn() {
    // epoll_ctl(m_epollfd,EPOLL_CTL_DEL,m_socket,0);
    close(m_socket);
    delete [] m_buf;
}


json Http_conn::to_json(char *buf) {
    json ans;
    // char *p = m_post_payload;
    std::string head;
    std::string body;
    bool front = true;
    char *p=buf;
    while (*p) {
        if(*p == '&' && !front) {
            ans[head] = body;
            head.clear();
            body.clear();
            front = true;
        }
        else if(*p == '=' && front) {
            front = false;
        }
        else if((*p!= '=') && (*p!= '&')){
            if(front) {
                head+=*p;
            }
            else {
                body+= *p;
            }
        }
        ++p;
    }
    return ans;
} 



// 循环读取数据知道断开连接
bool Http_conn::recv_message () {
    int i=0;
    if(m_TRIGMode) { // 边缘触发，一次可以读完使用
        int index= recv(m_socket,m_buf,READ_BUFFER_SIZE,0);
        if(index <= 0) {
            return false;
        }
    }
    else { // 水平触发，用于读取大量的数据，需要一部分一部分的读取
        int index =0;
        int buf_size = READ_BUFFER_SIZE/2;
        while(true) {
            index = recv(m_socket,m_buf+i,buf_size,0);
            if(index <= 0) {
                
                printf("Error %s",strerror(errno));
                return false; // 错误
            }
            else if(index < buf_size) { // 说明没有读完了
                return true;
            }
            else{
                i+=index;
                if(i==READ_BUFFER_SIZE) {  // 扩充缓冲区
                    char * tmp = new char[READ_BUFFER_SIZE*2];
                    memcpy(tmp,m_buf,READ_BUFFER_SIZE);
                    m_buf_size = READ_BUFFER_SIZE*2;
                    delete [] m_buf;
                    m_buf = tmp;
                }
                else if(i==(READ_BUFFER_SIZE*2)) { // 拒绝服务
                    return false;
                }
            } 
        }
    }
    return true;
}

void Http_conn::read_event() {
    int stats; // 状态码
    
    // 解析数据包
    HTTP_CODE parse_res = Parse();
    if(parse_res != GET_REQUEST) {
        stats = 400;
    }
    else { // 开始针对数据包的请求工作
        REQ method = get_method();
        get_resource(); // 资源目录
        struct stat st;
        if(stat(m_path,&st)==-1) {
            stats = 404;
        }
        else {
            m_resmessage_len = st.st_size;
            stats = 200;
        }
    }
    // 生成响应数据包
    char* fkind = strrchr(m_path,'.');
    char *kind = new char[50];
    strcpy(kind,to_kind(fkind+1));
    if(!generate_head(stats,kind)) {
        std::cerr<< "Error 无法生成数据包"<<std::endl;
        return;
    }
}

void Http_conn::write_event() {
    // 写头部文件
    m_iv[0].iov_base = write_buf;
    m_iv[0].iov_len = strlen(write_buf);
    FILE *file = fopen(m_path,"rb");
    assert(file); // 使用断言 出现问问题弹出错误直接挂掉
    // int size = lseek(file,0,SEEK_END);
    // 查看文件的属性
    struct stat file_stat;
    stat(m_path,&file_stat);
    int size = file_stat.st_size;
    char *file_date= (char *)mmap(NULL,size,PROT_READ,MAP_PRIVATE,fileno(file),0);
    if(file_date == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        exit(1);
    }
    m_iv[1].iov_base = file_date;
    m_iv[1].iov_len = size;
    while(1) {
        int ret = writev(m_socket,m_iv,2);
        printf("write %d byte\n",ret);
        if(ret == -1) {
            if (errno==EAGAIN) {
                continue;
            }
            else  if(errno==EPIPE) { // Broken pipe 问题 主要是出现在数据没法完对面就断开了连接
                printf("Broken pipe %s\n",strerror(errno));
                break;
            }
            else {
                printf("write error: %s\n",strerror(errno));
                exit(1);
            }
        }
        if(ret == 0 ) {
            break;
        }
        if(ret < m_iv[0].iov_len) {
            m_iv[0].iov_base = write_buf+ret;
            m_iv[0].iov_len = strlen(write_buf)-ret;
        }
        else if(ret >= m_iv[0].iov_len) {
            int a= ret - m_iv[0].iov_len;
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_iv[1].iov_base + a;
            m_iv[1].iov_len = m_iv[1].iov_len - a;
        }
    }
    
    memset(write_buf,0,sizeof(write_buf));
    wpos=0;
    memset(m_buf,0,m_buf_size);
}

void Http_conn::process() {
    // 读事件
    if(m_state==1) {
        recv_message();
        read_event();
        modfd(m_epollfd,m_socket,m_state,false); 
    }
    // 写事件
    else if(m_state==0) {
       write_event();
       modfd(m_epollfd,m_socket,m_state,false); 
    }
}


bool Http_conn::generate_head(int status, char* kind) {
    int len =0;
    switch(status) {
        case 200: 
            add_line(status,ok_200_title);
            add_headers(m_resmessage_len,kind);
            break;
        case 400:
            add_line(status,error_400_title);
            len = strlen(error_400_form);
            add_headers(len,NULL); 
            // std::fread(&write_buf[wpos],len+1,1,file);
            break;
        case 403:  
            add_line(status,error_403_title);
            len = strlen(error_403_form);
            add_headers(len,NULL); 
            // std::fread(&write_buf[wpos],len+1,1,file);
            break;
        case 404:
            add_line(status,error_404_title);
            len = strlen(error_404_form);
            add_headers(len,NULL); 
            // std::fread(&write_buf[wpos],len+1,1,file);
            break;
        case 500:
            add_line(status,error_500_title);
            len = strlen(error_500_form);
            add_headers(len,NULL); 
            // std::fread(&write_buf[wpos],len+1,1,file);
            break;
    }
    return true;
}

// 解析数据包
Http_conn::LINE_STATE Http_conn::parse_line() {
    int max= strlen(m_buf);
    if(!m_buf) {
        return LINE_EMPTY;
    }
    char *pos1 = strchr(m_buf, ' ');
    if(!pos1 || (pos1-m_buf)>max){
        return LINE_BAD;
    }
    *pos1 = '\0';
    ++pos1;
    m_req_method = m_buf;
    char *pos2 = strchr(pos1, ' ');
    if(!pos2 || (pos2-m_buf)>max) {
        return LINE_BAD;
    } 
    *pos2 = '\0';
    ++pos2;
    m_url = pos1;
    char *pos3 = strchr(pos2, '\r');
    if(!pos3 || (pos3-m_buf)>max) {
        return LINE_BAD;
    }
    *pos3 = '\0';
    ++ pos3;
    m_version = pos2;
    if(*pos3 != '\n') {
        return LINE_BAD;
    }
    m_header = pos3+1;
    return LINE_OK;

}

Http_conn::HTTP_CODE Http_conn::parse_header() {
    if(strlen(m_header) <=0) {
        return NO_REQUEST;
    }
    int max = strlen(m_header);
    char *pos = strrchr(m_header, '\r');
    if(!pos || (pos-m_header) > max) {
        return BAD_REQUEST;
    }
    *pos = '\0';
    pos+=2;
    if(pos){
        m_post_payload = pos;
    }
    char *p = m_header;
    std::string head,value;
    bool front=true;
    while(*p) {
        if(*p =='\r') {
            m_head_json[head] = value;
            front = true;
            head.clear();
            value.clear();
            p+=2;
        }
        else if(*p == ':' && front) {
            front = false;
            p+=2;
        }
        else {
            if(front) {
                head+=*p;
            }
            else {
                value += *p;
            }
            ++p;
        }
    }
    return GET_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::parse_post_body() {
    char *tag1 = strchr(m_post_payload,'=');
    char *tag2 = strchr(m_post_payload,'&');
    if(!tag1) {
        return GET_REQUEST;
    }
    user = tag1+1;
    *tag2 = '\0';
    tag1 = strchr(tag2+1,'=');
    password = tag1+1;
    return GET_REQUEST;
}

Http_conn::HTTP_CODE Http_conn::Parse() {
    LINE_STATE line_state = parse_line();
    switch (line_state) {
        case LINE_BAD:
            return BAD_REQUEST;
        
        case LINE_EMPTY:
            return NO_REQUEST;
        
        case LINE_OK:
        {
            HTTP_CODE head_stats = parse_header();
            if(head_stats != GET_REQUEST) {
                return head_stats;
            }  
            if(strcmp(m_req_method,"POST")==0) {
                return parse_post_body();
            }
            return GET_REQUEST;
        }  
        default:
            return BAD_REQUEST;
    }
}

// 生成数据头
bool Http_conn::add_request(char *format, ...) {
    if(wpos > WRITE_BUF_SIZE) {
        return false;
    }
    va_list valist; // 
    va_start(valist,format); // 将format 的后面的参数放进valist中
    int len = vsnprintf(write_buf + wpos,WRITE_BUF_SIZE - wpos, format, valist);
    if(len >= WRITE_BUF_SIZE -1-wpos) {
        va_end(valist);
        return false;
    }
    wpos += len;
    va_end(valist);
    return true;
}

bool Http_conn::add_line(int stats, const char *title) {
    return add_request("%s %d %s\r\n","HTTP/1.1",stats,title);
}

bool Http_conn::add_headers(long len,char *ss) {
    if(ss) {
        return add_content_length(len) && add_content_type(ss) && add_linger() && add_blank_line();
    }
    else {
        return add_content_length(len) && add_linger() && add_blank_line();
    }
}

bool Http_conn::add_content_length(int len) {
    return add_request("Content-Length: %d\r\n",len);
}

bool Http_conn::add_content_type(char *ss) {
    return add_request("Content-Type: %s\r\n",ss);
}

bool Http_conn::add_linger() {
    return add_request("Connection: %s\r\n", (m_linger == true)? "keep-alive" : "close");
}

bool Http_conn::add_blank_line() {
    return add_request("\r\n");
}

bool Http_conn::add_message(char *message) {
    add_request("%s",message);
}



Http_conn::REQ Http_conn::get_method() const {
    if(strcmp(m_req_method,"GET")==0) return GET;
    if(strcmp(m_req_method,"POST")==0) return POST;
    if(strcmp(m_req_method,"PUT")==0) return PUT;
    if(strcmp(m_req_method,"DELETE")==0) return DELETE;
    if(strcmp(m_req_method,"PATCH")==0) return PATCH;
    if(strcmp(m_req_method,"HEAD")==0) return HEAD;
    if(strcmp(m_req_method,"OPTIONS")==0) return OPTIONS;
    if(strcmp(m_req_method,"TRACE")==0) return TRACE;
}

void Http_conn::get_resource() {
    // 当m_url=\时表示打开首页
    if(strlen(m_url) == 1) {
        strcpy(m_path,"judge.html");
    }
    else if(strlen(m_url) == 2) {
        // action 0 表示注册
        if(strcmp(m_url,"/0")==0) {
            strcpy(m_path,"register.html");
        }
        else if(strcmp(m_url,"/1")==0) {// action 1 表示登录
            strcpy(m_path,"log.html");
        }
        else if(strcmp(m_url,"/5")==0) { // 查看图片
            strcpy(m_path,"picture.html");
        }
        else if(strcmp(m_url,"/6")==0) { // 查看视频
            strcpy(m_path,"video.html");
        }
        else if(strcmp(m_url,"/7")==0) { // 关注我
            strcpy(m_path,"fans.html");
        }
    }
    else if(strlen(m_url) == 12) {
        if(strcmp(m_url,"/2CGISQL.cgi")==0) { // 登录
            // m_payload_json = to_json(m_post_payload); // 
            if(verification_user()) { // 登录成功
                strcpy(m_path,"welcome.html");
            }
            else{ // 登录失败
                strcpy(m_path,"register.html");
            }
        }
        else if(strcmp(m_url,"/3CGISQL.cgi")==0) {// 注册
            
            if(register_user()) { // 注册成功
                strcpy(m_path,"log.html");
            }
            else {
               strcpy(m_path,"registerError.html");
            }
        }
    }
    else {
        strcpy(m_path,m_url+1); 
    }   
}

// 数据库操作
bool Http_conn::verification_user() {
    char query[50];
    memset(query, 0, sizeof(query));
    
    sprintf(query, "select password from users where username = \'%s\'", user);
    // 运行mysql语句 
    if(mysql_query(m_mysql,query)!= 0) { 
        return false;
    }
    // 返回最近一次执行查询的结果集中的列数，只适用于select查询语句
    if(mysql_field_count(m_mysql) == 0) {
        printf("没有该用户！\n");
        return false;
    }
    // 保存mysql数据结果，是一个表
    MYSQL_RES *results = mysql_store_result(m_mysql);
    // 表头表的结构
    // MYSQL_FIELD *field = mysql_fetch_fields(results);
    // 表内容 获取一行数据让后下一次再调用会到下一行
    MYSQL_ROW row = mysql_fetch_row(results);
    // 遍历表格
    // while(row = mysql_fetch_row(results)) {
    //     for(int i = 0;i<mysql_num_fields(results);i++) {
    //         printf("%s %s \n",field[i].name,row[i]);
    //     }
    // }
    if(strcmp(row[0],password)==0) {
        mysql_free_result(results);
        return true;
    }
    // 释放表内存
    mysql_free_result(results);
    return false;
    
}

bool Http_conn::register_user() {
    char query[100];
    memset(query, 0, sizeof(query));
    // 1、检验用户名是否已经被注册
    sprintf(query, "select username from users where username = \'%s\'", user);
    // if(!m_mysql) {
    //     printf("mysql is NULL\n");
    // }
    if(mysql_query(m_mysql,query)!= 0) {
        printf("Error: mysql select\n");
        return false;
    }
    MYSQL_RES *results = mysql_store_result(m_mysql);
    // 该用户名已经被注册过
    if(mysql_num_rows(results)) {
        printf("重复注册\n");
        mysql_free_result(results);
        return false;
    }
    // 注册
    memset(query, 0, sizeof(query));
    sprintf(query, "insert into users values (\'%s\',\'%s\')", user,password);
    if(mysql_query(m_mysql,query)!= 0) {
        printf("Error: mysql select\n");
        return false;
    }
    if(mysql_affected_rows(m_mysql)<=0) {
        printf("Error: mysql insert error\n");
        return false;
    }
    return true;
}


void Http_conn::change_state(int state) {
    m_state = state;
}

MYSQL *Http_conn::re_mysql() {
    return m_mysql;
}

char* Http_conn::to_kind(char *kind) {
    if(strcasecmp(kind,"html")==0) {
        return "text/html";
    }
    else if(strcasecmp(kind,"gif")==0) {
        return "image/gif";
    }
    else if(strcasecmp(kind,"ico")==0) {
        return "image/x-icon";
    }
    else if(strcasecmp(kind,"jpg")==0) {
        return "application/x-jpg";
    }
    else if(strcasecmp(kind,"mp4")==0) {
        return "video/mpeg4";
    }
    else if(strcasecmp(kind,"mp3")==0) {
        return "audio/mp3";
    }
}
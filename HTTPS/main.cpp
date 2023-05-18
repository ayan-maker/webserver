#include <stdio.h>
#include "./webserver/webserver.h"
#include <unistd.h>

int main(char argc, char **argv) {
    printf("webserver!\n");
    // config
    int port = 5555;
    char user[20] = "root";
    char password[20] = "456789";
    char datename[20] = "webserver";
    char ip[20] = "127.0.0.1";
    int max_conn =10;
    int close_log = 0;
    // 线程池
    int max_pth = 5;
    int max_requests = 20;
    int state = 1;
    
    // int out_time = 10;

    char path[1024]; // 工作路径
    char log_path[1024]; // log地址
    memset(path, 0, sizeof(path));
    memset(log_path, 0, sizeof(log_path));
    if(argc>1) {
        port = atoi(argv[1]); // 字符串转成整形
        memcpy(path,argv[2],sizeof(argv[2]));
        chdir(path);
    }
    else {
        getcwd(log_path,1024);
        strcpy(path,"/home/hy/root");
        if(chdir(path)!=0) {
            printf("%s\n",path);
            printf("change directory failed for %s\n", strerror(errno));
        }
    }
    
    Webserver Server;
    Server.init_server(max_pth,max_requests,state,ip,user,password,datename,port,max_conn,close_log,log_path);
    // Server.set_out_time(out_time);
}
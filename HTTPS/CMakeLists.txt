cmake_minimum_required(VERSION 3.6)

# 设置c++ 版本
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 表示是否支持c++ 指定版本的更低版本

# 项目名称
project(HTTP_server)

# 调试
add_definitions("-Wall -g")

# 用于设置编译器编译C++代码时的额外编译选项 类似于g++ xx.cpp -o xx -g
# set(CMAKE_CXX_FLAGS -lpthread -L/usr/lib64/mysql -lmysqlclient) 
# 设置连接选项
set(CMAKE_EXE_LINKER_FLAGS  "-lpthread -L/usr/lib64/mysql -lmysqlclient") # -lpthread只链接pthread库 -pthread 可以加入编译参数

link_directories(/usr/lib64/mysql)

include_directories("/usr/include/mysql"
                    "/home/hy/c++/json-develop/include"  
                    "${PROJECT_SOURCE_DIR}/webserver"                  
                    )

# 指明本项目存在一个子目录 这样子目录下的cmake就会被编译 
# 如果存在依赖关系需要设置顺序
add_subdirectory(timer)
add_subdirectory(LOCK)
# add_subdirectory(Block_queue)
add_subdirectory(LOG)   
add_subdirectory(sql_connection_pool)  
# add_subdirectory(pthreadpool)                
add_subdirectory(http_conn)            
add_subdirectory(webserver)          
                
                

#set(SRC_LIST main.cpp ./webserver/webserver.cpp ./pthreadpool/pthreadpool.cpp  ./http_conn/http_conn.cpp  ./sql_connection_pool/sql_connection_pool.cpp ./LOG/log.cpp ./timer/timer.cpp)
set(SRC_LIST main.cpp)
# 用于在构建时根据源文件的内容生成一个新的文件
# configure_file("${PROJECT_SOURCE_DIR}/config.h.in"
#                 "${PROJECT_SOURCE_DIR}/config.h"
#                 )

# set(PROJECT_BINARY_DIR ./build/)

# # 设置一个选择，并将USE_WEBSERVER设置成 ON
# option(USE_WEBSERVER "Use webserver" ON)

# if(USE_WEBSERVER)
#     # 用于向编译器添加头文件搜索路径
#     include_directories("${PROJECT_SOURCE_DIR}/webserver") 
#     # 指明本项目存在一个子目录 这样子目录下的cmake就会被编译
#     add_subdirectory(webserver)
#     # 将库添加到EXTRA_LIBS中方便后面不在单独制定库
#     set(EXTRA_LIBS ${EXTRA_LIBS} webserver)
# endif(USE_WEBSERVER)



#生成可执行文件
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}   webserver)


# 链接
# target_link_libraries(${PROJECT_NAME} PUBLIC ${EXTRA_LIBS})

# # 将库添加到头目录中
# target_include_directories(${PROJECT_NAME} PUBLIC 
#                             ${PROJECT_BINARY_DIR}
#                             ${PROJECT_SOURCE_DIR}/HttpServer
#                             )
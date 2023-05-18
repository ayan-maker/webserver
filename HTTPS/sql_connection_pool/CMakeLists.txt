cmake_minimum_required(VERSION 3.6)

aux_source_directory(.  DIR_LIB_SRCS)

add_library(sql_conn  ${DIR_LIB_SRCS})
target_link_libraries(sql_conn PRIVATE log)
target_link_libraries(sql_conn PUBLIC mysqlclient)
# 添加include 文件路径
target_include_directories(sql_conn PUBLIC /usr/include/mysql)
# 添加ku的文件路径
# target_link_directories(sql_conn PUBLIC /usr/lib64/mysql)
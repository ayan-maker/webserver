
# 将文件夹中的源文件放在DIR_LIB_ERCS中
aux_source_directory(. DIR_LIB_SRCS)

# 生成链接库 动态链接库
add_library(webserver  ${DIR_LIB_SRCS})
# 连接静态库 顺序时a依赖b b放在a后面
target_link_libraries(webserver PRIVATE  http_conn timer)

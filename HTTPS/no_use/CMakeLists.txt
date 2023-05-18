aux_source_directory(.  DIR_LIB_SRCS)

add_library(pthreadpool  ${DIR_LIB_SRCS})
target_link_libraries(pthreadpool PRIVATE http_conn sql_conn lock timer)
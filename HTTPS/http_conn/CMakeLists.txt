aux_source_directory(.  DIR_LIB_SRCS)

add_library(http_conn  ${DIR_LIB_SRCS})
target_link_libraries(http_conn PRIVATE sql_conn )
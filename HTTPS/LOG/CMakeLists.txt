aux_source_directory(.  DIR_LIB_SRCS)

add_library(log  ${DIR_LIB_SRCS})

target_link_libraries(log  lock)
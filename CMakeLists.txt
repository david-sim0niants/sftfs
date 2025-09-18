cmake_minimum_required(VERSION 3.13)
project(sftfs C)

set(TARGET_NAME sftfs)
add_executable(${TARGET_NAME} src/main.c)
target_compile_definitions(${TARGET_NAME} PRIVATE -D_FILE_OFFSET_BITS=64)
target_link_libraries(${TARGET_NAME} PRIVATE -lfuse -lssh)
target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Werror=format)

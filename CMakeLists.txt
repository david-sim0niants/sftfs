cmake_minimum_required(VERSION 3.13)
project(sftfs C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(ROOT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_subdirectory(src)

set(TARGET_NAME sftfs)
add_executable(${TARGET_NAME} src/main.c)
target_link_libraries(${TARGET_NAME} PRIVATE sftfs_core sftfs_common sftfs_endp sftfs_ssh)

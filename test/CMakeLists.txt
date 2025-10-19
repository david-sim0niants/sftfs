set(TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(ut)

set(TARGET_NAME ut_main)
add_library(${TARGET_NAME} STATIC ut_main.c)
target_include_directories(${TARGET_NAME} PUBLIC ${TEST_INCLUDE_DIR})

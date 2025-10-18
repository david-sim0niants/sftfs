set(TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(ut)

set(TARGET_NAME run_ut)
add_executable(${TARGET_NAME} ut_main.c)
target_link_libraries(${TARGET_NAME} PRIVATE ut_suites)

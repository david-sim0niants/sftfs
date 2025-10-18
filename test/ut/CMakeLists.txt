set(TARGET_NAME ut_suites)
add_library(${TARGET_NAME} STATIC htable.c)
target_include_directories(${TARGET_NAME} PUBLIC ${TEST_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE sftfs_common sftfs_abs)

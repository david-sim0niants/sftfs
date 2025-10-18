#pragma once

#include <stddef.h>

struct ut_test_suite_result {
    int tests_passed;
    int tests_failed;
};

struct ut_test_suite {
    const char *name;
    void (*run)(struct ut_test_suite_result *result);
};

void ut_register_test_suite(struct ut_test_suite *suite);

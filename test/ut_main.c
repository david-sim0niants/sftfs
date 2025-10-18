#include "ut.h"

#include <stdio.h>
#include <stdlib.h>

static struct ut_test_suite *suites;
static size_t nr_suites;

void ut_register_test_suite(struct ut_test_suite *suite)
{
    suites = realloc(suites, (nr_suites + 1) * sizeof(struct ut_test_suite));
    suites[nr_suites] = *suite;
    ++nr_suites;
}

int main()
{
    for (size_t i = 0; i < nr_suites; ++i) {
        struct ut_test_suite_result result;
        suites[i].run(&result);

        int tests_run = result.tests_passed + result.tests_failed;

        printf("PASSED %d/%d\nFAILED %d/%d\n",
                result.tests_passed, tests_run, result.tests_failed, tests_run);
    }
}

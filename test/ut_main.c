#include "ut.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *xrealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (NULL == ptr) {
        ut_report("OUT OF MEMORY\n");
        abort();
    }
    return ptr;
}

struct ut_test ut_root;

void ut_register_tests(struct ut_test *tests, size_t nr_tests)
{
    const size_t nr_all_tests = ut_root.nr_subtests + nr_tests;
    ut_root.subtests = xrealloc(ut_root.subtests, nr_all_tests * sizeof(struct ut_test));
    memcpy(ut_root.subtests + ut_root.nr_subtests, tests, nr_tests * sizeof(struct ut_test));
    ut_root.nr_subtests = nr_all_tests;
}

static void ut_unregister_tests(void)
{
    free(ut_root.subtests);
}

const char *prefix_match = "";
char *curr_name = NULL;
size_t curr_name_len = 0;

void ut_report(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("%s: ", curr_name);
    vprintf(fmt, args);
    va_end(args);
}

static void push_test_name(const char *name)
{
    if (! name)
        return;

    const size_t name_len = strlen(name);
    curr_name = xrealloc(curr_name, curr_name_len + name_len + 1);
    memcpy(curr_name + curr_name_len, name, name_len + 1);
    curr_name_len += name_len;
}

static void pop_test_name(const char *name)
{
    if (! name)
        return;

    const size_t name_len = strlen(name);
    assert(name_len <= curr_name_len);
    curr_name_len -= name_len;
    curr_name = realloc(curr_name, curr_name_len + 1);
    curr_name[curr_name_len] = '\0';
}

static void ut_say_passed(void)
{
    ut_report("%sPASSED%s", ut_color(UT_COLOR_GREEN), ut_color(UT_COLOR_DEFAULT));
}

static void ut_say_failed(void)
{
    ut_report("%sFAILED%s", ut_color(UT_COLOR_RED), ut_color(UT_COLOR_DEFAULT));
}

static int ut_run(struct ut_test *test)
{
    size_t tests_passed = 0;
    size_t all_tests = test->nr_subtests;

    if (test->runner) {
        if (test->runner() == UT_PASS)
            ++tests_passed;
        ++all_tests;
    }

    for (size_t i = 0; i < test->nr_subtests; ++i) {
        struct ut_test *subtest = &test->subtests[i];

        push_test_name(subtest->name);
        ut_report("%sRUN%s\n", ut_color(UT_COLOR_GREEN), ut_color(UT_COLOR_DEFAULT));

        if (ut_run(&test->subtests[i]) == UT_PASS) {
            ut_say_passed();
            ++tests_passed;
        } else {
            ut_say_failed();
        }
        printf("\n");

        pop_test_name(subtest->name);
    }

    if (tests_passed < all_tests) {
        ut_report("tests passed: %zu/%zu\n", tests_passed, all_tests);
        return UT_FAIL;
    } else {
        return UT_PASS;
    }
}

static const char *default_color = "";
static const char *red_color = "";
static const char *green_color = "";

void ut_colorize(void)
{
    default_color = "\033[0m";
    red_color = "\033[31m";
    green_color = "\033[32m";
}

void ut_uncolorize(void)
{
    default_color = "";
    red_color = "";
    green_color = "";
}

const char *ut_color(ut_color_t color)
{
    switch (color) {
        case UT_COLOR_DEFAULT:
            return default_color;
        case UT_COLOR_RED:
            return red_color;
        case UT_COLOR_GREEN:
            return green_color;
    }
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
        prefix_match = argv[1];
    if (isatty(fileno(stderr)))
        ut_colorize();
    else
        ut_uncolorize();
    atexit(ut_unregister_tests);
    return ut_run(&ut_root);
}

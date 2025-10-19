#pragma once

#include <stddef.h>

struct ut_test {
    const char *name;
    int (*runner)(void);
    struct ut_test *subtests;
    size_t nr_subtests;
};

enum {
    UT_PASS = 0,
    UT_FAIL = 1,
};

#define UT_LEAF_TEST(ut_name, ut_runner) (struct ut_test){.name = ut_name, .runner = ut_runner}

void ut_register_tests(struct ut_test *tests, size_t nr_tests);
void ut_report(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

typedef enum {
    UT_COLOR_DEFAULT,
    UT_COLOR_RED,
    UT_COLOR_GREEN
} ut_color_t;

void ut_colorize(void);
void ut_uncolorize(void);
const char *ut_color(ut_color_t color);

#define UT_CONCAT_IMPL(x, y) x##y
#define UT_CONCAT(x, y) UT_CONCAT_IMPL(x, y)
#define UT_ON_INIT void __attribute__((constructor)) UT_CONCAT(ut_on_init, __LINE__)(void)

#define UT_BEGIN int ut_rc = UT_PASS, prev_ut_rc = UT_PASS; (void)ut_rc; (void)prev_ut_rc;

#define UT_FINAL ut_final

#define UT_END return (ut_rc)

#define UT_FAILING (ut_rc != UT_PASS)
#define UT_PASSING (ut_rc == UT_PASS)
#define UT_JUST_FAILED (ut_rc == UT_FAIL && prev_ut_rc == UT_PASS)

#define UT_EXPECT(...) do { \
    if ((prev_ut_rc = ut_rc, ut_rc = (__VA_ARGS__) ? UT_PASS : UT_FAIL)) {\
        if (prev_ut_rc != UT_PASS && ut_rc == UT_PASS) \
            ut_rc = prev_ut_rc; \
        ut_report("%sExpectation failed%s at %s:%d %s\n", \
                ut_color(UT_COLOR_RED), ut_color(UT_COLOR_DEFAULT), \
                __FILE__, __LINE__, #__VA_ARGS__); \
    } \
} while (0)

#define UT_ASSERT(...) do { \
    if ((prev_ut_rc = ut_rc, ut_rc = (__VA_ARGS__) ? UT_PASS : UT_FAIL)) {\
        ut_report("%sAssertion failed%s at %s:%d %s\n", \
                ut_color(UT_COLOR_RED), ut_color(UT_COLOR_DEFAULT), \
                __FILE__, __LINE__, #__VA_ARGS__); \
        goto ut_final; \
    } \
    if (prev_ut_rc != UT_PASS && ut_rc == UT_PASS) \
        ut_rc = prev_ut_rc; \
} while (0)

#define UT_HARD_ASSERT(...) do { \
    if (!(__VA_ARGS__)) {\
        ut_report("%sHard assertion failed%s at %s:%d %s\n", \
                ut_color(UT_COLOR_RED), ut_color(UT_COLOR_DEFAULT), \
                __FILE__, __LINE__, #__VA_ARGS__); \
        abort(); \
    } \
} while (0)

#include <dc_util/filesystem.h>
#include "tests.h"
#include "util.h"
#include "input.h"

static void test_run_shell(const char *in, const char *expected_out, const char *expected_err);

Describe(shell);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(shell)
{
    dc_posix_env_init(&environ, NULL);
    dc_error_init(&error, NULL);
}

AfterEach(shell)
{
    dc_error_reset(&error);
}

Ensure(shell, run_shell)
{
    char *dir;
    char str[1024];

    dir = dc_get_working_dir(&environ, &error);

    setenv("PS1", ">>>>", true);
    sprintf(str, "[%s] >>>>", dir);
    test_run_shell("exit\n", str, "");

    unsetenv("PS1");
    sprintf(str, "[%s] $ ", dir);
    test_run_shell("exit\n", str, "");

    sprintf(str, "[%s] $ 0\n[/] $ ", dir);
    test_run_shell("cd /\nexit\n", str, "");
    free(dir);
}

static void test_run_shell(const char *in, const char *expected_out, const char *expected_err)
{
    char *in_buf;
    char out_buf[1024];
    char err_buf[1024];
    FILE *in_file;
    FILE *out_file;
    FILE *err_file;
    int ret_val;

    memset(out_buf, 0, sizeof(out_buf));
    memset(err_buf, 0, sizeof(out_buf));
    in_buf = strdup(in);
    in_file = fmemopen(in_buf, strlen(in_buf) + 1, "r");
    out_file = fmemopen(out_buf, sizeof(out_buf), "w");
    err_file = fmemopen(err_buf, sizeof(err_buf), "w");
    ret_val = run_shell(&environ, &error, in_file, out_file, err_file);
    assert_that(ret_val, is_equal_to(0));
    fflush(out_file);
    assert_that(out_buf, is_equal_to_string(expected_out));
    fflush(out_file);
    assert_that(err_buf, is_equal_to_string(expected_err));
    fclose(in_file);
    fclose(out_file);
    fclose(err_file);
    free(in_buf);
}

TestSuite *shell_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, shell, run_shell);

    return suite;
}

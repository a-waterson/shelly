#include <dc_util/strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "execute.h"
#include "tests.h"

static void test_execute(const char *cmd, size_t argc, char **argv, char **path,
                         bool check_exit_code, int expected_exit_code,
                         const char *out_file_name, const char *err_file_name);
static void check_redirection(const char *file_name);

Describe(execute);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(execute) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(execute) { dc_error_reset(&error); }

Ensure(execute, execute) {
  char **path;
  char **argv;
  char template[16];

  path = dc_strs_to_array(&environ, &error, 3, "/bin", "/usr/bin", NULL);

  argv = dc_strs_to_array(&environ, &error, 2, NULL, NULL);
  test_execute("pwd", 1, argv, path, true, 0, NULL, NULL);

  argv = dc_strs_to_array(&environ, &error, 2, NULL, NULL);
  strcpy(template, "/tmp/fileXXXXXX");
  test_execute("ls", 1, argv, path, true, 0, template, NULL);

  argv = dc_strs_to_array(&environ, &error, 3, NULL,
                          "asdasdasdfddfgsdfgasderdfdsf", NULL);
  strcpy(template, "/tmp/fileXXXXXX");
  test_execute("ls", 2, argv, path, false, ENOENT, NULL, template);

  dc_strs_destroy_array(&environ, 3, path);
  free(path);

  path = dc_strs_to_array(&environ, &error, 1, NULL);

  argv = dc_strs_to_array(&environ, &error, 2, NULL, NULL);
  test_execute("ls", 1, argv, path, true, 127, NULL, NULL);

  dc_strs_destroy_array(&environ, 1, path);
  free(path);
  path = dc_strs_to_array(&environ, &error, 2, "/", NULL);

  argv = dc_strs_to_array(&environ, &error, 2, NULL, NULL);
  test_execute("ls", 1, argv, path, true, 127, NULL, NULL);

  dc_strs_destroy_array(&environ, 2, path);
  free(path);
}

static void test_execute(const char *cmd, size_t argc, char **argv, char **path,
                         bool check_exit_code, int expected_exit_code,
                         const char *out_file_name, const char *err_file_name) {
  struct command command;

  memset(&command, 0, sizeof(struct command));
  command.command = strdup(cmd);
  command.argc = argc;
  command.argv = argv;

  if (out_file_name) {
    command.stdout_file = strdup(out_file_name);
  }

  if (err_file_name) {
    command.stderr_file = strdup(err_file_name);
  }

  execute(&environ, &error, &command, path);

  if (check_exit_code) {
    assert_that(command.exit_code, is_equal_to(expected_exit_code));
  }

  check_redirection(out_file_name);
  check_redirection(err_file_name);

  destroy_command(&environ, &command);
  dc_error_reset(&error);
}

static void check_redirection(const char *file_name) {
  if (file_name) {
    struct stat statbuf;
    int status;

    status = stat(file_name, &statbuf);
    assert_that(status, is_equal_to(0));
    assert_that(statbuf.st_size, is_greater_than(0));
    unlink(file_name);
  }
}

TestSuite *execute_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, execute, execute);

  return suite;
}

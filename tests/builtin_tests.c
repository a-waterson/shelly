#include <dc_util/filesystem.h>
#include <dc_util/path.h>
#include <dc_util/strings.h>
#include <unistd.h>

#include "builtins.h"
#include "tests.h"
#include "util.h"

static void test_builtin_cd(const char *line, const char *cmd, size_t argc,
                            char **argv, const char *expected_dir,
                            const char *expected_message);

Describe(builtin);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(builtin) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(builtin) { dc_error_reset(&error); }

Ensure(builtin, builtin_cd) {
  char **argv;
  char *path;
  char template[16];
  char message[1024];

  argv = dc_strs_to_array(&environ, &error, 3, NULL, "/", NULL);
  test_builtin_cd("cd /\n", "cd", 2, argv, "/", NULL);

  dc_expand_path(&environ, &error, &path, "~");
  argv = dc_strs_to_array(&environ, &error, 3, NULL, path, NULL);
  test_builtin_cd("cd ~\n", "cd", 2, argv, path, NULL);
  free(path);

  dc_expand_path(&environ, &error, &path, "~/");
  // remove the trailing space
  path[strlen(path) - 1] = '\0';
  argv = dc_strs_to_array(&environ, &error, 3, NULL, path, NULL);
  test_builtin_cd("cd ~\n", "cd", 2, argv, path, NULL);
  free(path);

  chdir("/");
  dc_expand_path(&environ, &error, &path, "~");
  argv = dc_strs_to_array(&environ, &error, 2, NULL, NULL);
  test_builtin_cd("cd\n", "cd", 1, argv, path, NULL);
  free(path);

  chdir("/tmp");
  argv = dc_strs_to_array(&environ, &error, 3, NULL, "/dev/null", NULL);
  test_builtin_cd("cd /dev/null\n", "cd", 2, argv, "/tmp",
                  "/dev/null: is not a directory\n");
  dc_error_reset(&error);

  strcpy(template, "/tmp/fileXXXXXX");
  mkdtemp(template);
  rmdir(template);
  sprintf(message, "%s: does not exist\n", template);
  argv = dc_strs_to_array(&environ, &error, 3, NULL, template, NULL);
  test_builtin_cd("cd fixme\n", "cd", 2, argv, "/tmp", message);
}

static void test_builtin_cd(const char *line, const char *cmd, size_t argc,
                            char **argv, const char *expected_dir,
                            const char *expected_message) {
  struct command command;
  char message[1024];
  FILE *stderr_file;
  char *working_dir;

  memset(&command, 0, sizeof(struct command));
  command.line = strdup(line);
  command.command = strdup(cmd);
  command.argc = argc;
  command.argv = argv;
  memset(message, 0, sizeof(message));
  stderr_file = fmemopen(message, sizeof(message), "w");
  builtin_cd(&environ, &error, &command, stderr_file);

  if (dc_error_has_no_error(&error)) {
    // TODO: wny does this hang if chdir failed?
    working_dir = dc_get_working_dir(&environ, &error);
    assert_that(working_dir, is_equal_to_string(expected_dir));
    assert_that(command.exit_code, is_equal_to(0));
    free(working_dir);
  } else {
    fflush(stderr_file);
    assert_that(message, is_equal_to_string(expected_message));
    assert_that(command.exit_code, is_equal_to(1));
  }

  fclose(stderr_file);
  destroy_command(&environ, &command);
}

TestSuite *builtin_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, builtin, builtin_cd);

  return suite;
}

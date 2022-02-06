#include <dc_util/path.h>
#include <dc_util/strings.h>

#include "shell_impl.h"
#include "tests.h"
#include "util.h"

static void test_parse_command(
    const char *expected_line, const char *expected_command,
    size_t expected_argc, char **expected_argv, const char *expected_stdin_file,
    const char *expected_stdout_file, bool expected_stdout_overwrite,
    const char *expected_stderr_file, bool expected_stderr_overwrite);
static void expand_path(const char *expected_file, char **expanded_file);
static void test_destroy_command(const char *expected_line);

Describe(command);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(command) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(command) { dc_error_reset(&error); }

Ensure(command, parse_command) {
  char **argv;

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("hello", "hello", 1, argv, NULL, NULL, false, NULL, false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out 2>err.txt", "./a.out", 1, argv, NULL, NULL, false,
                     "err.txt", false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("/usr/bin/ls > out.txt", "/usr/bin/ls", 1, argv, NULL,
                     "out.txt", false, NULL, false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out < in.txt", "./a.out", 1, argv, "in.txt", NULL,
                     false, NULL, false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out < in.txt > out.txt", "./a.out", 1, argv, "in.txt",
                     "out.txt", false, NULL, false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out > out.txt 2>    err.txt", "./a.out", 1, argv,
                     NULL, "out.txt", false, "err.txt", false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out < in.txt > out.txt 2>err.txt", "./a.out", 1, argv,
                     "in.txt", "out.txt", false, "err.txt", false);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out < in.txt >> out.txt 2>>err.txt", "./a.out", 1,
                     argv, "in.txt", "out.txt", true, "err.txt", true);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 1, NULL);
  test_parse_command("./a.out < ~/abc/in.txt >> ~/out.txt 2>>~/err.txt",
                     "./a.out", 1, argv, "~/abc/in.txt", "~/out.txt", true,
                     "~/err.txt", true);
  dc_strs_destroy_array(&environ, 1, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 4, NULL, "b", "c", NULL);
  test_parse_command("a b c", "a", 3, argv, NULL, NULL, false, NULL, false);
  dc_strs_destroy_array(&environ, 4, argv);
  free(argv);

  argv = dc_strs_to_array(&environ, &error, 5, NULL, "hello", "evil", "world",
                          NULL);
  test_parse_command("foo hello evil world", "foo", 4, argv, NULL, NULL, false,
                     NULL, false);
  dc_strs_destroy_array(&environ, 5, argv);
  free(argv);

  /*
  argv = dc_strs_to_array(&environ, &error, 5, NULL, "/User/ds/hello", "evil",
  "world", NULL); test_parse_command("foo ~/hello ~/def/evil \"world rocks\"",
                     "foo",
                     4,
                     argv,
                     NULL,
                     NULL,
                     false,
                     NULL,
                     false);
  */
}

static void test_parse_command(
    const char *expected_line, const char *expected_command,
    size_t expected_argc, char **expected_argv, const char *expected_stdin_file,
    const char *expected_stdout_file, bool expected_stdout_overwrite,
    const char *expected_stderr_file, bool expected_stderr_overwrite) {
  struct state state;
  char *expanded_stdin_file;
  char *expanded_stdout_file;
  char *expanded_stderr_file;

  expand_path(expected_stdin_file, &expanded_stdin_file);
  expand_path(expected_stdout_file, &expanded_stdout_file);
  expand_path(expected_stderr_file, &expanded_stderr_file);
  state.stdin = NULL;
  state.stdout = NULL;
  state.stderr = NULL;
  init_state(&environ, &error, &state);
  state.command = calloc(1, sizeof(struct command));
  state.command->line = strdup(expected_line);
  parse_command(&environ, &error, &state, state.command);
  assert_that(state.command->line, is_equal_to_string(expected_line));
  assert_that(state.command->command, is_equal_to_string(expected_command));
  assert_that(state.command->argc, is_equal_to(expected_argc));

  assert_that(state.command->argv[0], is_null);
  assert_that(state.command->argv[expected_argc], is_null);

  for (size_t i = 1; i < expected_argc; i++) {
    assert_that(state.command->argv[i], is_equal_to_string(expected_argv[i]));
  }

  assert_that(state.command->stdin_file,
              is_equal_to_string(expanded_stdin_file));
  assert_that(state.command->stdout_file,
              is_equal_to_string(expanded_stdout_file));
  assert_that(state.command->stdout_overwrite,
              is_equal_to(expected_stdout_overwrite));
  assert_that(state.command->stderr_file,
              is_equal_to_string(expanded_stderr_file));
  assert_that(state.command->stderr_overwrite,
              is_equal_to(expected_stderr_overwrite));
  assert_that(state.command->exit_code, is_equal_to(0));
  free(expanded_stdin_file);
  free(expanded_stdout_file);
  free(expanded_stderr_file);
  destroy_state(&environ, &error, &state);
}

static void expand_path(const char *expected_file, char **expanded_file) {
  if (expected_file == NULL) {
    *expanded_file = NULL;
  } else {
    dc_expand_path(&environ, &error, expanded_file, expected_file);

    if (dc_error_has_error(&error)) {
      fail_test(expected_file);
    }
  }
}

Ensure(command, destroy_command) {
  test_destroy_command("ls");
  test_destroy_command("ls -al");
  test_destroy_command("ls -al < in > out 2> err");
}

static void test_destroy_command(const char *expected_line) {
  struct state state;

  state.stdin = NULL;
  state.stdout = NULL;
  state.stderr = NULL;
  init_state(&environ, &error, &state);
  state.command = calloc(1, sizeof(struct command));
  state.command->line = strdup(expected_line);
  parse_command(&environ, &error, &state, state.command);
  destroy_command(&environ, state.command);
  assert_that(state.command->line, is_null);
  assert_that(state.command->command, is_null);
  assert_that(state.command->argv, is_null);
  assert_that(state.command->stdin_file, is_null);
  assert_that(state.command->stdout_file, is_null);
  assert_that(state.command->stderr_file, is_null);
  destroy_state(&environ, &error, &state);
}

TestSuite *command_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, command, parse_command);
  add_test_with_context(suite, command, destroy_command);

  return suite;
}

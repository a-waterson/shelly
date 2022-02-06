#include <dc_posix/dc_stdlib.h>
#include <dc_util/filesystem.h>
#include <unistd.h>

#include "shell_impl.h"
#include "tests.h"
static void test_execute_command(const char *command, int expected_next_state, const char *expected_exit_code, const char *expected_error_message);
static void test_handle_error(const char *current_line, bool is_fatal, int expected_error_code, const char *message, const char *expected_error_message, int expected_next_state);
static void test_parse_commands(const char *command, const char *expected_command, size_t expected_argc);
static void test_reset_state(const char *expected_prompt, bool initial_fatal);
static void test_init_state(const char *expected_prompt, FILE *in, FILE *out,
                            FILE *err);
static void test_destroy_state(bool initial_fatal);
static void test_read_commands(const char *command,
                               const char *expected_command,
                               int expected_return);
static void test_separate_commands(const char *command,
                                   const char *expected_command,
                                   int expected_return);
static void test_parse_commands(const char *command,
                                const char *expected_command,
                                size_t expected_argc);
Describe(shell_impl);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(shell_impl) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(shell_impl) { dc_error_reset(&error); }

Ensure(shell_impl, init_state) {
  unsetenv("PS1");
  test_init_state("$ ", stdin, stdout, stderr);

  setenv("PS1", "X", true);
  test_init_state("X", stdin, stdout, stderr);
}
Ensure(shell_impl, destroy_state) {
  test_destroy_state(true);
  test_destroy_state(false);
}
Ensure(shell_impl, reset_state) {
  unsetenv("PS1");
  test_reset_state("$ ", false);

  setenv("PS1", "$$  ", true);
  test_reset_state("$$  ", false);

  unsetenv("PS1");
  test_reset_state("$ ", true);

  setenv("PS1", "!>", true);
  test_reset_state("!>", true);
}
Ensure(shell_impl, read_commands) {
  test_read_commands("hello", "hello", SEPARATE_COMMANDS);
  test_read_commands("hello\n", "hello", SEPARATE_COMMANDS);
  test_read_commands("\n", "", RESET_STATE);
}
Ensure(shell_impl, separate_commands) {

  test_separate_commands("./a.out", "./a.out", SEPARATE_COMMANDS);
  test_separate_commands("cd ~\n", "cd ~", SEPARATE_COMMANDS);
  test_separate_commands("\n", "", RESET_STATE);

}
Ensure(shell_impl, parse_commands) {

  test_parse_commands("hello\n", "hello", 1);
  test_parse_commands("./a.out a b c\n", "./a.out", 4);
}
Ensure(shell_impl, execute_commands) {
  char *current_working_dir;

  test_execute_command("exit", EXIT, "", "");

  test_execute_command("cd /", RESET_STATE, "0\n", "");
  current_working_dir = dc_get_working_dir(&environ, &error);
  assert_that(current_working_dir, is_equal_to_string("/"));
  free(current_working_dir);

  test_execute_command("cd /dev/null", RESET_STATE, "1\n", "/dev/null: is not a directory\n");
  current_working_dir = dc_get_working_dir(&environ, &error);
  assert_that(current_working_dir, is_equal_to_string("/"));
  free(current_working_dir);

  test_execute_command("ls", RESET_STATE, "0\n", "");
}
Ensure(shell_impl, do_exit) {

  struct state state;
  int next_state;

  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));
  state.current_line_length = 10;

  next_state = do_exit(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(DESTROY_STATE));
  assert_that(state.current_line_length, is_equal_to(0));

  destroy_state(&environ, &error, &state);
}
Ensure(shell_impl, handle_error) {

  test_handle_error(NULL, true, 4, "foo", "internal error (4) foo\n", DESTROY_STATE);
  test_handle_error("ls", false, 6, "bar car", "internal error (6) bar car: \"ls\"\n", RESET_STATE);
  test_handle_error("ls -l", false, 897, "bar car", "internal error (897) bar car: \"ls -l\"\n", RESET_STATE);
}

TestSuite *shell_impl_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, shell_impl, init_state);
  add_test_with_context(suite, shell_impl, destroy_state);
  add_test_with_context(suite, shell_impl, reset_state);
  add_test_with_context(suite, shell_impl, read_commands);
  add_test_with_context(suite, shell_impl, separate_commands);
  add_test_with_context(suite, shell_impl, parse_commands);
  add_test_with_context(suite, shell_impl, execute_commands);
  add_test_with_context(suite, shell_impl, do_exit);
  add_test_with_context(suite, shell_impl, handle_error);
  return suite;
}
static void test_reset_state(const char *expected_prompt, bool initial_fatal) {
  struct state state;
  int next_state;
  long line_length;

  state.stdin = stdin;
  state.stdout = stdout;
  state.stderr = stderr;
  line_length = sysconf(_SC_ARG_MAX);
  assert_that_expression(line_length >= 0);
  init_state(&environ, &error, &state);
  state.fatal_error = initial_fatal;
  next_state = reset_state(&environ, &error, &state);
  assert_that(next_state, is_equal_to(READ_COMMANDS));
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(state.stdin, is_equal_to(stdin));
  assert_that(state.stdout, is_equal_to(stdout));
  assert_that(state.stderr, is_equal_to(stderr));
  assert_that(state.in_redirect_regex, is_not_null);
  assert_that(state.out_redirect_regex, is_not_null);
  assert_that(state.err_redirect_regex, is_not_null);
  assert_that(state.prompt, is_equal_to_string(expected_prompt));
  assert_that(state.path, is_not_null);
  assert_that(state.max_line_length, is_equal_to(line_length));
  assert_that(state.current_line, is_null);
  assert_that(state.current_line_length, is_equal_to(0));
  assert_that(state.command, is_null);
  destroy_state(&environ, &error, &state);
}
static void test_destroy_state(bool initial_fatal) {
  struct state state;
  int next_state;
  long line_length;

  line_length = sysconf(_SC_ARG_MAX);
  assert_that_expression(line_length >= 0);
  init_state(&environ, &error, &state);
  state.fatal_error = initial_fatal;
  next_state = destroy_state(&environ, &error, &state);

  assert_that(next_state, is_equal_to(DC_FSM_EXIT));
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(state.in_redirect_regex, is_null);
  assert_that(state.out_redirect_regex, is_null);
  assert_that(state.err_redirect_regex, is_null);
  assert_that(state.path, is_null);
  assert_that(state.prompt, is_null);

  assert_that(state.max_line_length, is_equal_to(0));
  assert_that(state.current_line, is_null);
  assert_that(state.current_line_length, is_equal_to(0));
  assert_that(state.command, is_null);
}
static void test_init_state(const char *expected_prompt, FILE *in, FILE *out,
                            FILE *err) {
  struct state state;
  int next_state;
  long line_length;

  state.stdin = in;
  state.stdout = out;
  state.stderr = err;
  line_length = sysconf(_SC_ARG_MAX);
  assert_that_expression(line_length >= 0);
  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));
  assert_that(state.stdin, is_equal_to(in));
  assert_that(state.stdout, is_equal_to(out));
  assert_that(state.stderr, is_equal_to(err));
  assert_that(state.in_redirect_regex, is_not_null);
  assert_that(state.out_redirect_regex, is_not_null);
  assert_that(state.err_redirect_regex, is_not_null);
  assert_that(state.path, is_not_null);
  assert_that(state.prompt, is_equal_to_string(expected_prompt));
  assert_that(state.max_line_length, is_equal_to(line_length));
  assert_that(state.current_line, is_null);
  assert_that(state.current_line_length, is_equal_to(0));
  assert_that(state.command, is_null);
  destroy_state(&environ, &error, &state);
}
static void test_read_commands(const char *command,
                               const char *expected_command,
                               int expected_return) {
  char *in_buf;
  char out_buf[1024];
  FILE *in;
  FILE *out;
  struct state state;
  int next_state;
  char *cwd;
  char *prompt;

  in_buf = strdup(command);
  in = fmemopen(in_buf, strlen(in_buf) + 1, "r");
  out = fmemopen(out_buf, sizeof(out_buf), "w");
  state.stdin = in;
  state.stdout = out;
  state.stderr = stderr;
  unsetenv("PS1");
  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));
  next_state = read_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(expected_return));
  assert_false(state.fatal_error);
  cwd = dc_get_working_dir(&environ, &error);
  // [current working directory] state.prompt
  prompt = malloc(1 + strlen(cwd) + 1 + 2 + strlen(state.prompt) + 1);
  sprintf(prompt, "[%s] %s", cwd, state.prompt);

  fflush(out);
  assert_that(out_buf, is_equal_to_string(prompt));
  assert_that(state.current_line, is_equal_to_string(expected_command));
  assert_that(state.current_line_length, is_equal_to(strlen(expected_command)));
  destroy_state(&environ, &error, &state);
  free(cwd);
  free(prompt);
  fclose(in);
  fclose(out);
  free(in_buf);
}

static void test_separate_commands(const char *command, const char *expected_command, int expected_return)
{
  char *in_buf;
  char out_buf[1024];
  FILE *in;
  FILE *out;
  struct state state;
  int next_state;

  in_buf = strdup(command);
  in = fmemopen(in_buf, strlen(in_buf) + 1, "r");
  out = fmemopen(out_buf, sizeof(out_buf), "w");
  state.stdin = in;
  state.stdout = out;
  state.stderr = stderr;
  unsetenv("PS1");

  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));

  next_state = read_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(expected_return));
  assert_false(state.fatal_error);
  assert_that(state.current_line, is_equal_to_string(expected_command));
  assert_that(state.current_line_length, is_equal_to(strlen(expected_command)));

  if(expected_return == RESET_STATE)
  {
    fclose(in);
    fclose(out);
    free(in_buf);
    destroy_state(&environ, &error, &state);
    return;
  }

  next_state = separate_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(PARSE_COMMANDS));
  assert_false(state.fatal_error);
  assert_that(state.command, is_not_null);
  assert_that(state.command->line, is_equal_to_string(state.current_line));
  assert_that(state.command->line, is_not_equal_to(state.current_line));
  assert_that(state.command->command, is_null);
  assert_that(state.command->argc, is_equal_to(0));
  assert_that(state.command->argv, is_null);
  assert_that(state.command->stdin_file, is_null);
  assert_that(state.command->stdout_file, is_null);
  assert_false(state.command->stdout_overwrite);
  assert_that(state.command->stderr_file, is_null);
  assert_false(state.command->stderr_overwrite);
  assert_that(state.command->exit_code, is_equal_to(0));
  fclose(in);
  fclose(out);
  free(in_buf);
  destroy_state(&environ, &error, &state);
}
static void test_parse_commands(const char *command, const char *expected_command, size_t expected_argc)
{
  char *in_buf;
  char out_buf[1024];
  FILE *in;
  FILE *out;
  struct state state;
  int next_state;

  in_buf = strdup(command);
  in = fmemopen(in_buf, strlen(in_buf) + 1, "r");
  out = fmemopen(out_buf, sizeof(out_buf), "w");
  state.stdin = in;
  state.stdout = out;
  state.stderr = stderr;
  unsetenv("PS1");

  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));

  next_state = read_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(SEPARATE_COMMANDS));
  assert_false(state.fatal_error);

  next_state = separate_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(PARSE_COMMANDS));

  next_state = parse_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(EXECUTE_COMMANDS));

  assert_that(state.command->command, is_equal_to_string(expected_command));
  assert_that(state.command->argc, is_equal_to(expected_argc));

  destroy_state(&environ, &error, &state);
  free(in_buf);
  fclose(in);
  fclose(out);
}
static void test_execute_command(const char *command, int expected_next_state, const char *expected_exit_code, const char *expected_error_message)
{
  char *in_buf;
  char out_buf[1024];
  char err_buf[1024];
  FILE *in;
  FILE *out;
  FILE *err;
  struct state state;
  int next_state;

  in_buf = strdup(command);
  memset(out_buf, 0, sizeof(out_buf));
  memset(err_buf, 0, sizeof(err_buf));
  in = fmemopen(in_buf, strlen(in_buf) + 1, "r");
  out = fmemopen(out_buf, sizeof(out_buf), "w");
  err = fmemopen(err_buf, sizeof(out_buf), "w");
  state.stdin = in;
  state.stdout = out;
  state.stderr = err;
  unsetenv("PS1");

  next_state = init_state(&environ, &error, &state);
  assert_false(dc_error_has_error(&error));
  assert_false(state.fatal_error);
  assert_that(next_state, is_equal_to(READ_COMMANDS));

  next_state = read_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(SEPARATE_COMMANDS));
  assert_false(state.fatal_error);

  next_state = separate_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(PARSE_COMMANDS));

  next_state = parse_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(EXECUTE_COMMANDS));

  fclose(out);
  out_buf[0] = '\0';
  out = fmemopen(out_buf, sizeof(out_buf), "w");
  state.stdout = out;

  next_state = execute_commands(&environ, &error, &state);
  assert_that(next_state, is_equal_to(expected_next_state));
  fflush(out);
  assert_that(out_buf, is_equal_to_string(expected_exit_code));
  fflush(err);
  assert_that(err_buf, is_equal_to_string(expected_error_message));

  destroy_state(&environ, &error, &state);
  fclose(in);
  fclose(out);
  fclose(err);
  free(in_buf);
}
static void test_handle_error(const char *current_line, bool is_fatal, int expected_error_code, const char *message, const char *expected_error_message, int expected_next_state)
{
  char out_buf[1024];
  char err_buf[1024];
  FILE *out_file;
  FILE *err_file;
  struct state state;
  int next_state;
  struct dc_error err;

  memset(out_buf, 0, sizeof(out_buf));
  memset(err_buf, 0, sizeof(err_buf));
  out_file = fmemopen(out_buf, sizeof(out_buf), "w");
  err_file = fmemopen(err_buf, sizeof(err_buf), "w");
  state.stdout = out_file;
  state.stderr = err_file;
  init_state(&environ, &error, &state);
  dc_error_init(&err, NULL);
  err.err_code = expected_error_code;
  err.message = strdup(message);

  if(current_line != NULL)
  {
    state.current_line = strdup(current_line);
    state.current_line_length = strlen(state.current_line);
  }

  state.fatal_error = is_fatal;
  next_state = handle_error(&environ, &err, &state);
  assert_that(next_state, is_equal_to(expected_next_state));
  fflush(out_file);
  assert_that(out_buf, is_equal_to_string(""));
  fflush(err_file);
  assert_that(err_buf, is_equal_to_string(expected_error_message));
  fclose(out_file);
  fclose(err_file);
  destroy_state(&environ, &error, &state);
  dc_error_reset(&err);
}
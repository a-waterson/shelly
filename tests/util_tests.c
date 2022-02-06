#include <dc_util/strings.h>

#include "command.h"
#include "state.h"
#include "tests.h"
#include "util.h"

static void check_state_reset(const struct dc_error *error,
                              const struct state *state, FILE *in, FILE *out,
                              FILE *err);
static void test_parse_path(const char *path_str, char **dirs);

Describe(util);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(util) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(util) { dc_error_reset(&error); }

Ensure(util, get_prompt) {
  const char *prompt;

  unsetenv("PS1");
  prompt = get_prompt(&environ, &error);
  assert_that(prompt, is_equal_to_string("$ "));

  setenv("PS1", "ABC", true);
  prompt = get_prompt(&environ, &error);
  assert_that(prompt, is_equal_to_string("ABC"));
}

Ensure(util, get_path) {
  static const char *paths[] = {
      "",           ".",          "abc", "abc:def",
      "/usr/bin:.", ".:/usr/bin", ":",   "/usr/bin:/bin:/usr/local/bin",
      NULL,
  };
  char *path;

  unsetenv("PATH");
  path = getenv("PATH");
  assert_that(path, is_null);

  for (int i = 0; paths[i]; i++) {
    setenv("PATH", paths[i], true);
    path = getenv("PATH");
    assert_that(path, is_equal_to_string(paths[i]));
  }
}

Ensure(util, parse_path) {
  test_parse_path("", dc_strs_to_array(&environ, &error, 1, NULL));
  test_parse_path("a", dc_strs_to_array(&environ, &error, 2, "a", NULL));
  test_parse_path("a:b", dc_strs_to_array(&environ, &error, 3, "a", "b", NULL));
  test_parse_path("a:bcde:f", dc_strs_to_array(&environ, &error, 4, "a", "bcde",
                                               "f", NULL));
  test_parse_path("a::b",
                  dc_strs_to_array(&environ, &error, 3, "a", "b", NULL));
}

static void test_parse_path(const char *path_str, char **dirs) {
  char **path_dirs;
  size_t i;

  path_dirs = parse_path(&environ, &error, path_str);

  for (i = 0; dirs[i] && path_dirs[i]; i++) {
    assert_that(path_dirs[i], is_equal_to_string(dirs[i]));
  }

  assert_that(dirs[i], is_null);
  assert_that(path_dirs[i], is_null);
}

Ensure(util, do_reset_state) {
  struct state state;

  state.stdin = stdin;
  state.stdout = stdout;
  state.stderr = stderr;
  state.in_redirect_regex = NULL;
  state.out_redirect_regex = NULL;
  state.err_redirect_regex = NULL;
  state.path = NULL;
  state.prompt = NULL;
  state.max_line_length = 0;
  state.current_line = NULL;
  state.current_line_length = 0;
  state.command = NULL;
  state.fatal_error = false;

  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);

  state.current_line = strdup("");
  state.current_line_length = strlen(state.current_line);
  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);

  state.current_line = strdup("ls");
  state.current_line_length = strlen(state.current_line);
  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);

  state.current_line = strdup("ls");
  state.current_line_length = strlen(state.current_line);
  state.command = calloc(1, sizeof(struct command));
  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);

  DC_ERROR_RAISE_ERRNO(&error, E2BIG);
  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);

  state.fatal_error = true;
  do_reset_state(&environ, &error, &state);
  check_state_reset(&error, &state, stdin, stdout, stderr);
}

static void check_state_reset(const struct dc_error *error,
                              const struct state *state, FILE *in, FILE *out,
                              FILE *err) {
  assert_false(state->fatal_error);
  assert_that(state->current_line, is_null);
  assert_that(state->current_line_length, is_equal_to(0));
  assert_that(state->command, is_null);
  assert_that(state->stdin, is_equal_to(in));
  assert_that(state->stdout, is_equal_to(out));
  assert_that(state->stderr, is_equal_to(err));

  assert_that(error->message, is_null);
  assert_that(error->file_name, is_null);
  assert_that(error->function_name, is_null);
  assert_that(error->line_number, is_equal_to(0));
  assert_that(error->type, is_equal_to(0));
  assert_that(error->type, is_equal_to(0));
  assert_that(error->reporter, is_null);
  assert_that(error->err_code, is_equal_to(0));
}

Ensure(util, state_to_string) {
  struct state state;
  char *str;

  state.in_redirect_regex = NULL;
  state.out_redirect_regex = NULL;
  state.err_redirect_regex = NULL;
  state.path = NULL;
  state.prompt = NULL;
  state.max_line_length = 0;
  state.current_line = NULL;
  state.current_line_length = 0;
  state.command = NULL;
  state.fatal_error = false;

  state.fatal_error = false;
  str = state_to_string(&environ, &error, &state);
  assert_that(str, is_equal_to_string("current_line = NULL, fatal_error = 0"));
  free(str);

  state.fatal_error = true;
  str = state_to_string(&environ, &error, &state);
  assert_that(str, is_equal_to_string("current_line = NULL, fatal_error = 1"));
  free(str);

  state.current_line = strdup("");
  state.fatal_error = false;
  str = state_to_string(&environ, &error, &state);
  assert_that(str, is_equal_to_string("current_line = \"\", fatal_error = 0"));
  free(str);
  free(state.current_line);

  state.current_line = strdup("hello");
  state.fatal_error = false;
  str = state_to_string(&environ, &error, &state);
  assert_that(str,
              is_equal_to_string("current_line = \"hello\", fatal_error = 0"));
  free(str);
  free(state.current_line);

  state.current_line = strdup("world");
  state.fatal_error = true;
  str = state_to_string(&environ, &error, &state);
  assert_that(str,
              is_equal_to_string("current_line = \"world\", fatal_error = 1"));
  free(str);
  free(state.current_line);
}

TestSuite *util_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, util, get_prompt);
  add_test_with_context(suite, util, get_path);
  add_test_with_context(suite, util, parse_path);
  add_test_with_context(suite, util, do_reset_state);
  add_test_with_context(suite, util, state_to_string);

  return suite;
}

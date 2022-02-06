#include "input.h"
#include "tests.h"
#include "util.h"

static void test_read_command_line(const char *data, ...);

Describe(input);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(input) {
  dc_posix_env_init(&environ, NULL);
  dc_error_init(&error, NULL);
}

AfterEach(input) { dc_error_reset(&error); }

Ensure(input, read_command_line) {
  test_read_command_line("hello\n", "hello", NULL);
  test_read_command_line(" evil \n", "evil", NULL);
  test_read_command_line(" \t\f\vabc def  \t\f\v\n", "abc def", NULL);
  // test_read_command_line("four\nthree\n", "four", "three", NULL);
  test_read_command_line("./a.out hello < in.txt > out.txt 2>err.txt\n",
                         "./a.out hello < in.txt > out.txt 2>err.txt", NULL);
}

static void test_read_command_line(const char *data, ...) {
  FILE *strstream;
  va_list strings;
  size_t buf_size;
  char *str;
  char *expected_line;

  buf_size = strlen(data) + 1;
  str = strdup(data);
  strstream = fmemopen(str, buf_size, "r");

  va_start(strings, data);

  do {
    char *line;
    size_t line_size;

    line_size = buf_size;
    line = read_command_line(&environ, &error, strstream, &line_size);
    expected_line = va_arg(strings, char *);

    if (expected_line == NULL) {
      assert_that(line, is_equal_to_string(""));
      assert_that(line_size, is_equal_to(0));
    } else {
      assert_that(line, is_equal_to_string(expected_line));
      assert_that(line_size, is_equal_to(strlen(line)));
    }

    free(line);
  } while (expected_line);

  va_end(strings);

  fclose(strstream);
  free(str);
}

TestSuite *input_tests(void) {
  TestSuite *suite;

  suite = create_test_suite();
  add_test_with_context(suite, input, read_command_line);

  return suite;
}

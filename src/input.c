#include "input.h"
#include "dc_util/strings.h"
#include <stdio.h>
#include <string.h>

char *read_command_line(const struct dc_posix_env *env, struct dc_error *err, FILE *stream, size_t *line_size)
{
    char *line;
    line = NULL;
    //    fgets(line, (int)*line_size, stream);
    getline(&line, line_size, stream);
    dc_str_trim(env, line);
    *line_size = strlen(line);
    return line;
}
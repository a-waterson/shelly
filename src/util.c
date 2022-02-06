#include "util.h"
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <stdlib.h>
#include <string.h>
char *get_prompt(const struct dc_posix_env *env, struct dc_error *err)
{
    const char *env_PS1;
    env_PS1 = dc_getenv(env, "PS1");
    if(env_PS1 == NULL)
        env_PS1 = "$ ";
    return env_PS1;
}

char *get_path(const struct dc_posix_env *env, struct dc_error *err)
{
    const char *path;
    path = dc_getenv(env, "PATH");
    return path;
}
size_t count(const char *str, int c)
{
    size_t num;
    num = 0;
    for(const char *tmp = str; *tmp; tmp++)
    {
        if(*tmp == c)
        {
            num++;
        }
    }
    return num;
}
char **parse_path(const struct dc_posix_env *env, struct dc_error *err, const char *path_str)
{
    char  *str;
    char  *state;
    char  *token;
    char **list;
    size_t num;
    size_t i;

    str   = strdup(path_str);
    state = str;
    num   = count(str, ':') + 1;
    list  = malloc((num + 1) * sizeof(char *));
    i     = 0;

    while((token = strtok_r(state, ":", &state)) != NULL)
    {
        list[i] = strdup(token);
        i++;
    };

    list[i] = NULL;
    free(str);
    return list;
}

void do_reset_state(const struct dc_posix_env *env, struct dc_error *err, struct state *state)
{
    dc_free(env, state->current_line, sizeof(state->current_line));

    state->current_line = NULL;

    dc_free(env, state->command, sizeof(state->command));
    
    state->command             = NULL;

    state->current_line_length = 0;

    dc_free(env, err->message, sizeof(err->message));

    state->fatal_error = false;
    err->message       = NULL;
    err->file_name     = NULL;
    err->function_name = NULL;
    err->line_number   = NULL;
    err->line_number   = 0;
    err->type          = 0;
    err->err_code      = 0;
}

void display_state(const struct dc_posix_env *env, const struct state *state, FILE *stream)
{
}
char *state_to_string(const struct dc_posix_env *env, struct dc_error *err, const struct state *state)
{
    size_t len;
    char  *line;

    if(state->current_line == NULL)
    {
        len = strlen("current_line = NULL");
    }
    else
    {
        len = strlen("current_line = \"\"");
        len += state->current_line_length;
    }

    len += strlen(", fatal_error = ");
    // +1 for 0 or 1 for the fatal_error and +1 for the null byte
    line = malloc(len + 1 + 1);

    if(state->current_line == NULL)
    {
        sprintf(line, "current_line = NULL, fatal_error = %d", state->fatal_error);
    }
    else
    {
        sprintf(line, "current_line = \"%s\", fatal_error = %d", state->current_line, state->fatal_error);
    }

    return line;
}

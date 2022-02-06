#include "command.h"
#include "shell.h"
#include <dc_util/filesystem.h>
#include <dc_util/path.h>
#include <dc_util/strings.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
bool get_error_redirect(const struct dc_posix_env *env,
                        struct command            *command,
                        char                      *input,
                        regmatch_t                *match_err,
                        int                        lastchar,
                        char                     **file);
void expand_directory(const struct dc_posix_env *env, struct dc_error *err, char **expanded, char *source);
void expand_command(const struct dc_posix_env *env,
                    struct dc_error           *err,
                    char                     **expanded,
                    char                      *source,
                    struct command            *command);
/*
If any out-of-memory errors occur,
set state.fatal_error

# NOTE assuming C, use a ‘\0’ before the start of each redirect as they are found

Find the stderr redirect via the state.err_redirect_regex
if regex matches
    if >> is present set state.command.stderr_overwrite to true:
    expand the file to change ~ to the users home dir
    set state.command.stderr_file to the expanded file

Find the stdout redirect via the state.out_redirect_regex
if regex matches
    if >> is present set state.command.stderr_overwrite to true
    expand the file to change ~ to the users home dir
    set state.command.stdout_file to the expanded file

Find the stdin redirect via the state.err_redirect_regex
if regex matches
expand the file to change ~ to the users home dir
    set state.command.stdin_file to the expanded file

# NOTE: this assumes C
use wordexp() to separate the rest of the command line
set state.command.argc to we_wordc

# NOTE + 2 is for argv[0] will become the program name, argv[n] is always NULL
set state.command.argv to an array of we_wordc size + 2

for each word in we_wordv, starting at 1
    copy the we_wordv[i] = state.command.argv[i]

copy we_word[0] to state.command.command

return EXECUTE_COMMANDS
*/
void parse_command(const struct dc_posix_env *env, struct dc_error *err, struct state *state, struct command *command)
{
    int        matched_err;
    int        matched_out;
    int        matched_in;
    char      *input;
    int        max_args = 24;
    char      *in_expanded[max_args];
    char      *out_expanded[max_args];
    char      *err_expanded[max_args];
    char      *command_expanded[max_args];
    regmatch_t match_err;
    regmatch_t match_out;
    regmatch_t match_in;

    input       = strdup(command->line);

    matched_err = regexec(state->err_redirect_regex, input, 1, &match_err, 0);
    if(matched_err == 0)
    {
        command->stderr_overwrite = get_error_redirect(env, command, input, &match_err, '>', &(command->stderr_file));
        dc_expand_path(env, err, err_expanded, command->stderr_file);
        command->stderr_file = err_expanded[0];
    }
    matched_out = regexec(state->out_redirect_regex, input, 1, &match_out, 0);
    if(matched_out == 0)
    {
        command->stdout_overwrite = get_error_redirect(env, command, input, &match_out, '>', &(command->stdout_file));
        dc_expand_path(env, err, out_expanded, command->stdout_file);
        command->stdout_file = out_expanded[0];
    }
    matched_in = regexec(state->in_redirect_regex, input, 1, &match_in, 0);
    if(matched_in == 0)
    {
        get_error_redirect(env, command, input, &match_in, '<', &(command->stdin_file));
        dc_expand_path(env, err, in_expanded, command->stdin_file);
        command->stdin_file = in_expanded[0];
    }
    command->argv = malloc(sizeof command->argv);
    expand_command(env, err, command_expanded, input, command);

    command->argv[0]             = NULL;
    command->argv[command->argc] = NULL;
    free(input);
}
bool get_error_redirect(const struct dc_posix_env *env,
                        struct command            *command,
                        char                      *input,
                        regmatch_t                *match_err,
                        int                        lastchar,
                        char                     **file)
{
    char    *str;
    regoff_t length;
    ssize_t  index;
    size_t   occurrences;
    bool     retval;

    retval = false;
    length = (*match_err).rm_eo - (*match_err).rm_so;
    str    = malloc(length + 1);
    strncpy(str, &input[(*match_err).rm_so], length);
    str[length]                   = '\0';
    index                         = dc_str_find_last(env, str, lastchar);
    *file                         = strdup(&str[index + 1]);
    input[strlen(input) - length] = '\0';
    occurrences                   = dc_str_find_all(env, str, lastchar);
    if(occurrences > 1)
        retval = true;

    free(str);
    return retval;
}
void expand_command(const struct dc_posix_env *env,
                    struct dc_error           *err,
                    char                     **expanded,
                    char                      *source,
                    struct command            *command)
{
    wordexp_t exp;
    int       status;

    status = wordexp(source, &exp, 0);

    if(status == 0)
    {
        command->argc    = exp.we_wordc;
        command->command = strdup(exp.we_wordv[0]);

        for(size_t i = 1; i < exp.we_wordc; i++)
        {
            command->argv[i] = strdup(exp.we_wordv[i]);
        }
        wordfree(&exp);
    }
    else
    {
        fprintf(stderr, "error");
    }
}

void destroy_command(const struct dc_posix_env *env, struct command *command)
{
    free(command->command);
    command->command = NULL;
    free(command->line);
    command->line = NULL;
    for(size_t i = 1; i < command->argc; ++i)
    {
        free(command->argv[i]);
        command->argv[i] = NULL;
    }
    free(command->argv);
    command->argv = NULL;
    free(command->stdin_file);
    command->stdin_file = NULL;
    free(command->stdout_file);
    command->stdout_file = NULL;
    free(command->stderr_file);
    command->stderr_file = NULL;
}

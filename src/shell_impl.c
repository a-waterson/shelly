#include "shell_impl.h"
#include "builtins.h"
#include "input.h"
#include "util.h"
#include <dc_posix/dc_regex.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_util/filesystem.h>
#include <unistd.h>
int init_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;

    state = arg;
    int   next_state;
    long  line_length;
    char *big_path;

    //    state->stdin              = stdin;
    //    state->stdout             = stdout;
    big_path                  = get_path(env, err);
    state->in_redirect_regex  = malloc(sizeof(regex_t));
    state->err_redirect_regex = malloc(sizeof(regex_t));
    state->out_redirect_regex = malloc(sizeof(regex_t));
    dc_regcomp(env, err, state->err_redirect_regex, "[ \t\f\v]2>[>]?.*", REG_EXTENDED);
    dc_regcomp(env, err, state->out_redirect_regex, "[ \t\f\v][1^2]?>[>]?.*", REG_EXTENDED);
    dc_regcomp(env, err, state->in_redirect_regex, "[ \t\f\v]<.*", REG_EXTENDED);

    line_length                = sysconf(_SC_ARG_MAX);

    state->max_line_length     = line_length;
    state->prompt              = get_prompt(env, err);
    state->path                = parse_path(env, err, big_path);
    state->current_line        = NULL;
    state->command             = NULL;

    state->fatal_error         = false;
    state->current_line_length = 0;
    next_state                 = READ_COMMANDS;
    return next_state;
};

int destroy_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    int           next_state;

    do_reset_state(env, err, arg);
    state                     = arg;
    state->in_redirect_regex  = NULL;
    state->out_redirect_regex = NULL;
    state->err_redirect_regex = NULL;
    state->path               = NULL;
    state->prompt             = NULL;
    state->max_line_length    = 0;
    next_state                = DC_FSM_EXIT;
    return next_state;
}

int reset_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    do_reset_state(env, err, arg);
    return READ_COMMANDS;
}

int read_commands(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    char         *cwd;
    char         *cmd;
    int           ret_val;

    state           = arg;
    size_t max_size = state->max_line_length;
    cwd             = dc_get_working_dir(env, err);
    fprintf(state->stdout, "[%s] %s", cwd, state->prompt);
    cmd                        = read_command_line(env, err, state->stdin, &max_size);
    state->current_line        = dc_strdup(env, err, cmd);
    state->current_line_length = dc_strlen(env, cmd);

    if(dc_strcmp(env, cmd, ""))
        ret_val = SEPARATE_COMMANDS;
    else
        ret_val = RESET_STATE;

    return ret_val;
}
//If any errors occur
//        set state.fatal_error to true and return ERROR
//
//        Set state.command to a new command object
//            zero out the state.command object
//
//# NOTE: this needs to change to handle non-simple commands
//                Copy the state.current_line to the state.command.line
//                    Set all other fields to zero, NULL, or false
//
//              return PARSE_COMMANDS

int separate_commands(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state   *state;
    struct command *command;
    int             ret_val;
    command        = calloc(1, sizeof(struct command));
    state          = arg;

    command->line  = dc_strdup(env, err, state->current_line);

    ret_val        = PARSE_COMMANDS;

    state->command = command;
    return ret_val;
}

int parse_commands(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    int retval;
    state = arg;
    parse_command(env, err, state, state->command);
    if(dc_error_has_error(err))
        retval = ERROR;
    else retval = EXECUTE_COMMANDS;

    return retval;
}

int execute_commands(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    state = arg;

    if(!(dc_strcmp(env, state->command->command, "cd")))
    {
        builtin_cd(env, err, state->command, state->stderr);
    }
    else if(!(dc_strcmp(env, state->command->command, "exit")))
    {
        return EXIT;
    }
    else
    {
        execute(env, err, state->command, state->path);
    }
    fprintf(state->stdout, "%d\n", state->command->exit_code);
    if(state->fatal_error)
        return ERROR;
    return RESET_STATE;
}

int do_exit(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    state = arg;
    do_reset_state(env, err, state);
    return DESTROY_STATE;
}
int handle_error(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
}
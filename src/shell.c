#include "shell.h"
#include "shell_impl.h"
#include <stdlib.h>

int run_shell(const struct dc_posix_env *env, struct dc_error *error, FILE *in, FILE *out, FILE *err)
{
    int                             ret_val;
    struct dc_fsm_info             *fsm_info;
    struct state                    state;
    static struct dc_fsm_transition transitions[] = {{DC_FSM_INIT, INIT_STATE, init_state},
                                                     {INIT_STATE, READ_COMMANDS, read_commands},
                                                     {INIT_STATE, ERROR, handle_error},
                                                     {READ_COMMANDS, RESET_STATE, reset_state},
                                                     {READ_COMMANDS, SEPARATE_COMMANDS, separate_commands},
                                                     {READ_COMMANDS, ERROR, handle_error},
                                                     {SEPARATE_COMMANDS, PARSE_COMMANDS, parse_commands},
                                                     {SEPARATE_COMMANDS, ERROR, handle_error},
                                                     {PARSE_COMMANDS, EXECUTE_COMMANDS, execute_commands},
                                                     {PARSE_COMMANDS, ERROR, handle_error},
                                                     {EXECUTE_COMMANDS, RESET_STATE, reset_state},
                                                     {EXECUTE_COMMANDS, EXIT, do_exit},
                                                     {EXECUTE_COMMANDS, ERROR, handle_error},
                                                     {RESET_STATE, READ_COMMANDS, read_commands},
                                                     {EXIT, DESTROY_STATE, destroy_state},
                                                     {ERROR, RESET_STATE, reset_state},
                                                     {ERROR, DESTROY_STATE, destroy_state},
                                                     {DESTROY_STATE, DC_FSM_EXIT, NULL}};
    fsm_info                                      = dc_fsm_info_create(env, err, "dc_shell");

    if(dc_error_has_no_error(error))
    {
        ret_val = EXIT_SUCCESS;
        int from_state;
        int to_state;
        state.stdin  = stdin;
        state.stdout = stdout;
        state.stderr = stderr;
        ret_val      = dc_fsm_run(env, error, fsm_info, &from_state, &to_state, &state, transitions);
        dc_fsm_info_destroy(env, &fsm_info);
    }
    else
    {
        ret_val = 1;
    }

    return ret_val;
}
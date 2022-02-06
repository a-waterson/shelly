#include "builtins.h"
#include <dc_posix/dc_unistd.h>
#include <dc_util/path.h>
#include <string.h>
#include <unistd.h>
/*
 * if chdir has an error
Print error message where message is
EACCESS: <dir>: message
ELOOP: <dir>: message
ENAMETOOLONG: <dir>: message
ENOENT: <dir>: does not exist
ENOTDIR: <dir>: is not a directory
set command.exit_code to 1
otherwise
set command.exit_code to 0

 */
void builtin_cd(const struct dc_posix_env *env, struct dc_error *err, struct command *command, FILE *errstream)
{
    if(command->argc == 1)
    {
        char *expanded_path;
        dc_expand_path(env, err, &expanded_path, "~");
        dc_chdir(env, err, expanded_path);
        if(dc_error_has_error(err))
        {
            printf("here");
        }

        command->exit_code = 0;
    }
    else
    {
        dc_chdir(env, err, command->argv[1]);
    }

    if(dc_error_has_error(err))
    {
        command->exit_code = 1;
        switch(err->errno_code)
        {
            case EACCES:
                fprintf(errstream, "%s: %s\n", command->argv[1], err->message);
                break;
            case ELOOP:
                fprintf(errstream, "%s: %s\n", command->argv[1], err->message);
                break;
            case ENAMETOOLONG:
                fprintf(errstream, "%s: %s\n", command->argv[1], err->message);
                break;
            case ENOENT:
                fprintf(errstream, "%s: does not exist\n", command->argv[1]);
                break;
            case ENOTDIR:
                fprintf(errstream, "%s: is not a directory\n", command->argv[1]);
                break;
        }
    } else {
        command->exit_code = 0;
    }

}

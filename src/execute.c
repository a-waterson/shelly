#include "execute.h"
#include "command.h"
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_unistd.h>
#include <dc_util/strings.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void execute(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    int pid;
    int status;
    int err_status;
    int waitpid_status;
    pid = dc_fork(env, err);
    if(pid == 0)
    {
        redirect(env, err, command);
        if(dc_error_has_error(err))
        {
            exit(126);
        }

        do_run(env, err, command, path);
        err_status = handle_run_error(err);
        if(err_status == 126)
        {
            fprintf(stderr, "command: %s not found\n", command->line);
        }
        dc_exit(env, err_status);
    }
    else
    {
        waitpid_status = waitpid(pid, &status, 0);

        if(WIFEXITED(waitpid_status))
        {
            command->exit_code = WEXITSTATUS(waitpid_status);
        }
    }
}
void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command)
{
    int          fd;
    unsigned int flags;
    if(command->stdin_file)
    {
        fd = dc_open(env, err, command->stdin_file, O_RDONLY, NULL);
        dc_dup2(env, err, fd, STDIN_FILENO);
        dc_close(env, err, fd);
    }
    if(command->stdout_file)
    {
        flags = command->stdout_overwrite ? O_TRUNC : O_APPEND;
        fd    = dc_open(env, err, command->stdout_file, O_WRONLY | flags | O_CREAT, NULL);
        dc_dup2(env, err, fd, STDOUT_FILENO);
        dc_close(env, err, fd);
    }
    if(command->stderr_file)
    {
        flags = command->stderr_file ? O_TRUNC : O_APPEND;
        fd    = dc_open(env, err, command->stdout_file, O_WRONLY | flags | O_CREAT, NULL);
        dc_dup2(env, err, fd, STDERR_FILENO);
        dc_close(env, err, fd);
    }
}

int do_run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    char cmd[512];
    if(dc_str_find_all(env, command->command, '/') > 0)
    {
        command->argv[0] = command->command;
        dc_execv(env, err, command->argv[0], command->argv);
    }
    else
    {
        if(!path[0])
        {
            DC_ERROR_RAISE_ERRNO(err, ENOENT);
        }
        else
        {
            while(*path)
            {
                sprintf(cmd, "%s/%s", *path, command->command);
                command->argv[0] = cmd;
                dc_execv(env, err, command->argv[0], command->argv);
                if(dc_error_has_error(err))
                    if(err->errno_code != ENOENT)
                        break;
                memset(cmd, 0, sizeof(cmd));
                path++;
            }
        }
    }
}

int handle_run_error(struct dc_error *err)
{
    if(dc_error_is_errno(err, E2BIG))
        return 0;
    else if(dc_error_is_errno(err, EACCES))
        return 1;
    else if(dc_error_is_errno(err, EINVAL))
        return 2;
    else if(dc_error_is_errno(err, ELOOP))
        return 3;
    else if(dc_error_is_errno(err, ENAMETOOLONG))
        return 4;
    else if(dc_error_is_errno(err, ENOENT))
        return 126;
    else if(dc_error_is_errno(err, ENOTDIR))
        return 5;
    else if(dc_error_is_errno(err, ENOEXEC))
        return 6;
    else if(dc_error_is_errno(err, ENOMEM))
        return 7;
    else if(dc_error_is_errno(err, ETXTBSY))
        return 8;
    else
        return 124;
}

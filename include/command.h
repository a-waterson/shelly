#ifndef DC_SHELL_COMMAND_H
#define DC_SHELL_COMMAND_H

/*
 * This file is part of dc_shell.
 *
 *  dc_shell is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dc_shell.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dc_posix/dc_posix_env.h>

#include "state.h"

/*! \struct command
    \brief The commands to enter, currently there is only one.

    The state passed around to the FSM functions.
*/
struct command {
  char *line;            /**< the current command line */
  char *command;         /**< the progran/builtin to run */
  size_t argc;           /**< the number of arguments to the command */
  char **argv;           /**< the arguments to the command */
  char *stdin_file;      /**< the file to redirect stdin from */
  char *stdout_file;     /**< the file to redirect stdout to */
  bool stdout_overwrite; /**< append or overwrite the stdout file (true =
                            overwrite) */
  char *stderr_file;     /**< the file to redirect strderr to */
  bool stderr_overwrite; /**< append or overwrite the strerr file (true =
                            overwrite) */
  int exit_code;         /**< the exit code from the program/builtin */
};

/**
 * Parse the command. Take the command->line and use it to fill in all of the
 * fields.
 *
 * @param env the posix environment.
 * @param err the error object.
 * @param state the current state, to set the fatal_error and access the command
 * line and regex for redirection.
 * @param command the command to parse.
 */
void parse_command(const struct dc_posix_env *env, struct dc_error *err,
                   struct state *state, struct command *command);
/**
 *
 * @param env
 * @param command
 */
void destroy_command(const struct dc_posix_env *env, struct command *command);
#endif  // DC_SHELL_COMMAND_H

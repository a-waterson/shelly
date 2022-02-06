#ifndef DC_SHELL_BUILTINS_H
#define DC_SHELL_BUILTINS_H

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

#include "execute.h"

/**
 * Change the working directory.
 * ~ is converted to the users home directory.
 * - no arguments is converted to the users home directory.
 * The command->exit_code is set to 0 on success or error->errno_code on
 * failure.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param command the command information
 */
void builtin_cd(const struct dc_posix_env *env, struct dc_error *err,
                struct command *command, FILE *errstream);

#endif  // DC_SHELL_BUILTINS_H

#ifndef DC_SHELL_SHELL_H
#define DC_SHELL_SHELL_H

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

#include <dc_fsm/fsm.h>
#include <dc_posix/dc_posix_env.h>

/*! \enum state
    \brief The possible FSM states.

    The state that the FSM can be in.
*/

enum shell_states {
  INIT_STATE = DC_FSM_USER_START,
  /**< the initial state */  //  2
  READ_COMMANDS,
  /**< accept user input */  //  3
  SEPARATE_COMMANDS,
  /**< separate the commands */  //  4
  PARSE_COMMANDS,
  /**< parse the commands */  //  5
  EXECUTE_COMMANDS,
  /**< execute the commands */  //  6
  EXIT,
  /**< exit the shell */  //  7
  RESET_STATE,
  /**< reset the state */  //  8
  ERROR,
  /**< handle errors */  //  9
  DESTROY_STATE,
  /**< destroy the state */  // 10
};

/**
 * Run the shell FSM.
 *
 * @param env the posix environment.
 * @param error the error object
 * @param err stderr
 * @param in    stdin
 * @param out stdout
 * @return the exit code from the shell.
 */

int run_shell(const struct dc_posix_env* env, struct dc_error* error, FILE* in,
              FILE* out, FILE* err);

#endif  // DC_SHELL_SHELL_H

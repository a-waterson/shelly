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

#include "shell.h"
#include "shell_impl.h"
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <getopt.h>

struct application_settings
{
    struct dc_opt_settings  opts;
    struct dc_setting_bool *verbose;
};

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);

static int
destroy_settings(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings **psettings);

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);

int        main(int argc, char *argv[])
{
    dc_posix_tracer             tracer;
    dc_error_reporter           reporter;
    struct dc_posix_env         env;
    struct dc_error             err;
    struct dc_application_info *info;
    int                         ret_val;

    tracer   = NULL;
    // tracer   = dc_posix_default_tracer;
    reporter = NULL;
    // reporter = dc_error_default_error_reporter;
    dc_posix_env_init(&env, tracer);
    dc_error_init(&err, reporter);
    info    = dc_application_info_create(&env, &err, "dcshell");
    ret_val = dc_application_run(&env,
                                 &err,
                                 info,
                                 create_settings,
                                 destroy_settings,
                                 run,
                                 dc_default_create_lifecycle,
                                 dc_default_destroy_lifecycle,
                                 "~/.dcshell.conf",
                                 argc,
                                 argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    static bool                  default_verbose = false;
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->verbose                 = dc_setting_bool_create(env, err);

    struct options opts[]             = {
        {(struct dc_setting *)settings->opts.parent.config_path,
         dc_options_set_path,
         "config",
         required_argument,
         'c',
         "CONFIG",
         dc_string_from_string,
         NULL,
         dc_string_from_config,
         NULL},
        {(struct dc_setting *)settings->verbose,
         dc_options_set_bool,
         "verbose",
         no_argument,
         'v',
         "VERBOSE",
         dc_flag_from_string,
         "verbose",
         dc_flag_from_config,
         &default_verbose},
    };

    // note the trick here - we use calloc and add 1 to ensure the last line is
    // all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size  = sizeof(struct options);
    settings->opts.opts       = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags      = "c:v:";
    settings->opts.env_prefix = "DC_SHELL_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env               *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings         **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_bool_destroy(env, &app_settings->verbose);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

static int run(const struct dc_posix_env                              *env,
               struct dc_error                                        *err,
               __attribute__((unused)) struct dc_application_settings *settings)
{
    int ret_val;

    DC_TRACE(env);
    ret_val = run_shell(env, err, stdin, stdout, stderr);

    return ret_val;
}

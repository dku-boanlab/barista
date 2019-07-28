/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup cli Command-Line Interface
 * \brief Functions to manage CLI commands
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "cli.h"

#include "component.h"
#include "event.h"

#include "application.h"
#include "app_event.h"

#include "storage.h"

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

/////////////////////////////////////////////////////////////////////

/** \brief The secret file of the CLI interface */
#define SECRET_CLI_FILE "secret/cli_password.txt"

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
ctx_t *cli_ctx;

/** \brief The pointer of the CLI context */
struct cli_def *cli;

/////////////////////////////////////////////////////////////////////

/** \brief The SIGINT handler definition */
void (*sigint_func)(int);

/** \brief Function to handle the SIGINT signal */
void sigint_handler(int sig)
{
    application_stop(NULL);
    component_stop(NULL);

    component_deactivate(NULL, "log");

    destroy_storage(cli_ctx);

    destroy_app_event(cli_ctx);
    destroy_event(cli_ctx);

    waitsec(1, 0);

    cli_done(cli);

    PRINTF("Barista is terminated!\n");

    exit(0);
}

/** \brief The SIGKILL handler definition */
void (*sigkill_func)(int);

/** \brief Function to handle the SIGKILL signal */
void sigkill_handler(int sig)
{
    application_stop(NULL);
    component_stop(NULL);

    component_deactivate(NULL, "log");

    destroy_storage(cli_ctx);

    destroy_app_event(cli_ctx);
    destroy_event(cli_ctx);

    waitsec(1, 0);

    cli_done(cli);

    PRINTF("Barista is terminated!\n");

    exit(0);
}

/** \brief The SIGTERM handler definition */
void (*sigterm_func)(int);

 /** \brief Function to handle the SIGTERM signal */
void sigterm_handler(int sig)
{
    application_stop(NULL);
    component_stop(NULL);

    component_deactivate(NULL, "log");

    destroy_storage(cli_ctx);

    destroy_app_event(cli_ctx);
    destroy_event(cli_ctx);

    waitsec(1, 0);

    cli_done(cli);

    PRINTF("Barista is terminated!\n");

    exit(0);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to load operator-defined policy file
 * \param cli CLI context
 */
int load_odp(struct cli_def *cli)
{
    int num_odps = 0;

    FILE *fp = fopen(DEFAULT_ODP_FILE, "r");
    if (fp != NULL) {
        char buf[__CONF_STR_LEN] = {0};

        if (cli)
            cli_print(cli, "Operator-defined policy file: %s", DEFAULT_ODP_FILE);
        else
            PRINTF("Operator-defined policy file: %s\n", DEFAULT_ODP_FILE);

        while (fgets(buf, __CONF_STR_LEN-1, fp) != NULL) {
            if (buf[0] == '#') continue;

            int cnt = 0;
            char *args[__CONF_ARGC + 1] = {0};

            str2args(buf, &cnt, args, __CONF_ARGC);

            if (cnt != 3) continue;

            if (strcmp(args[0], "component") == 0) {
                component_add_policy(cli, args[1], args[2]);
            } else if (strcmp(args[0], "application") == 0) {
                application_add_policy(cli, args[1], args[2]);
            }

            num_odps++;
        }

        fclose(fp);
    }

    if (!num_odps)
        return -1;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to load configuration files
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_load(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    // load components
    if (component_load(cli, cli_ctx)) {
        cli_print(cli, "Failed to load component configurations");
        return CLI_ERROR;
    }

    // load applications
    if (application_load(cli, cli_ctx)) {
        cli_print(cli, "Failed to load application configurations");
        return CLI_ERROR;
    }

    // autostart
    if (component_activate(cli, "log")) {
        cli_print(cli, "Failed to activate the log component");
        return CLI_ERROR;
    }

    // automatically load operator-defined policies
    if (load_odp(cli)) {
        cli_print(cli, "No operator-defined policy");
    }

    return CLI_OK;
}

/**
 * \brief Function to start the Barista NOS
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_start(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (component_start(cli)) {
        cli_print(cli, "Failed to start components");
        return CLI_ERROR;
    }

    if (application_start(cli)) {
        cli_print(cli, "Failed to start applications");
        return CLI_ERROR;
    }

    cli_print(cli, "Barista is excuted!");

    return CLI_OK;
}

/**
 * \brief Function to stop the Barista NOS
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_stop(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (component_stop(cli)) {
        cli_print(cli, "Failed to stop components");
        return CLI_ERROR;
    }

    if (application_stop(cli)) {
        cli_print(cli, "Failed to stop applications");
        return CLI_ERROR;
    }

    cli_print(cli, "Barista is stopped!");

    return CLI_OK;
}

/**
 * \brief Function to enable a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_enable_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (component_enable(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to enable an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_enable_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (application_enable(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to disable a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_disable_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (component_disable(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to disable an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_disable_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (application_disable(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to activate an enabled component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_activate_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (component_activate(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to activate an enabled application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_activate_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (application_activate(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to deactivate an activated component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_deactivate_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (component_deactivate(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to deactivate an activated application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_set_deactivate_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (application_deactivate(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to add a new operator-defined policy in a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_add_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 2) {
        if (component_add_policy(cli, argv[0], argv[1]))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to add a new operator-defined policy in an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_add_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 2) {
        if (application_add_policy(cli, argv[0], argv[1]))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to delete an operator-defined policy from a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_del_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 2) {
        if (component_del_policy(cli, argv[0], atoi(argv[1])))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to delete an operator-defined policy from an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_del_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 2) {
        if (application_del_policy(cli, argv[0], atoi(argv[1])))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to print an event
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_show_event(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (event_show(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to print an app event
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_show_app_event(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (app_event_show(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to print the configuration of a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_show_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (component_show(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to print the configuration of an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_show_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int i;
    for (i=0; i<argc; i++) {
        if (application_show(cli, argv[i]) != 0) {
            cli_print(cli, "%s is not valid", argv[i]);
            return CLI_ERROR;
        }
    }
    return CLI_OK;
}

/**
 * \brief Function to print the operator-defined policies applied to a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_show_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 1) {
        if (component_show_policy(cli, argv[0]))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to print the operator-defined policies applied to an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_policy_show_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc == 1) {
        if (application_show_policy(cli, argv[0]))
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to print all events
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_list_events(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    event_list(cli);

    return CLI_OK;
}

/**
 * \brief Function to print all app events
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_list_app_events(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    app_event_list(cli);

    return CLI_OK;
}

/**
 * \brief Function to print all components
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_list_components(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    component_list(cli);

    return CLI_OK;
}

/**
 * \brief Function to print all applications
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_list_applications(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    application_list(cli);

    return CLI_OK;
}

/**
 * \brief Function to deliver a command to a component
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_component(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc > 0) {
        if (component_cli(cli, &argv[0]) < 0)
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to deliver a command to an application
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_application(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc > 0) {
        if (application_cli(cli, &argv[0]) < 0)
            return CLI_ERROR;
        else
            return CLI_OK;
    }
    return CLI_ERROR;
}

/**
 * \brief Function to print logs
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_log(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "log %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

/**
 * \brief Function to terminate the Barista NOS
 * \param cli CLI context
 * \param command Command
 * \param argv Arguments
 * \param argc The number of arguments
 */
static int cli_exit(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    component_stop(cli);
    application_stop(cli);

    component_deactivate(cli, "log");

    cli_print(cli, "Barista is terminated!");

    destroy_storage(cli_ctx);

    destroy_app_event(cli_ctx);
    destroy_event(cli_ctx);

    waitsec(1, 0);

    cli_done(cli);

    exit(0);

    return CLI_OK;
}

/**
 * \brief Function to check the login information
 * \param username User ID
 * \param password Password
 */
static int check_auth(const char *username, const char *password)
{
    FILE *fp = fopen(SECRET_CLI_FILE, "r");
    if (fp != NULL) {
        char buf[__CONF_STR_LEN] = {0};

        while (fgets(buf, __CONF_STR_LEN-1, fp) != NULL) {
            if (buf[0] == '#') continue;

            char userid[__CONF_WORD_LEN];
            char userpw[__CONF_WORD_LEN];

            sscanf(buf, "%s %s", userid, userpw);

            if (strcasecmp(username, userid) == 0 && strcasecmp(password, userpw) == 0)
                return CLI_OK;
        }

        return CLI_ERROR;
    }

    return CLI_ERROR;
}

/**
 * \brief Function to handle an empty input
 * \param cli CLI context
 */
static int regular_callback(struct cli_def *cli)
{
    return CLI_OK;
}

/**
 * \brief Function to check the privileged password
 * \param password Privileged password
 */
static int check_enable(const char *password)
{
    FILE *fp = fopen(SECRET_CLI_FILE, "r");
    if (fp != NULL) {
        char buf[__CONF_STR_LEN] = {0};

        while (fgets(buf, __CONF_STR_LEN-1, fp) != NULL) {
            if (buf[0] == '#') continue;

            char userid[__CONF_WORD_LEN];
            char userpw[__CONF_WORD_LEN];

            sscanf(buf, "%s %s", userid, userpw);

            if (strcasecmp("admin", userid) == 0 && strcasecmp(password, userpw) == 0)
                return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

/**
 * \brief Function to handle CLI timeout
 * \param cli CLI context
 */
static int idle_timeout(struct cli_def *cli)
{
    return CLI_QUIT;
}

/** \brief The structure to manage CLI connections */
struct arg_struct {
    struct cli_def *cli;
    int acc_sock;
};

/**
 * \brief Function to accept CLI connections
 * \param arguments Arguments
 */
static void *connection_handler(void *arguments)
{
    struct arg_struct *args = (struct arg_struct *)arguments;
    struct cli_def *cli = args->cli;
    int acc_sock = args->acc_sock;

    cli_loop(cli, acc_sock);

    return NULL;
}

/**
 * \brief Function to provide the Barista CLI
 * \param ctx The context of the Barista NOS
 */
int start_cli(ctx_t *ctx)
{
    cli_ctx = ctx;

    struct cli_command *c, *cc;

    signal(SIGCHLD, SIG_IGN);

    cli = cli_init();
    cli_set_banner(cli, "Barista Console v2");
    cli_set_hostname(cli, "Barista");
    cli_telnet_protocol(cli, 1);
    cli_regular(cli, regular_callback);
    cli_regular_interval(cli, 5);
    cli_set_idle_timeout_callback(cli, 300, idle_timeout);

    cli_register_command(cli, NULL, "load", cli_load, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Load configurations");

    cli_register_command(cli, NULL, "start", cli_start, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Start the Barista NOS");
    cli_register_command(cli, NULL, "stop", cli_stop, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Stop the Barista NOS");

    cc = cli_register_command(cli, NULL, "enable", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_enable_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Enable components");
    cli_register_command(cli, cc, "application", cli_set_enable_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Enable applications");

    cc = cli_register_command(cli, NULL, "disable", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_disable_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Disable components");
    cli_register_command(cli, cc, "application", cli_set_disable_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Disable applications");

    cc = cli_register_command(cli, NULL, "activate", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_activate_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Activate components");
    cli_register_command(cli, cc, "application", cli_set_activate_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Activate applications");

    cc = cli_register_command(cli, NULL, "deactivate", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_deactivate_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Deactivate components");
    cli_register_command(cli, cc, "application", cli_set_deactivate_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Deactivate applications");

    c = cli_register_command(cli, NULL, "odp", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

    cc = cli_register_command(cli, c, "add", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_policy_add_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name] [Policy...], Add a policy to a component");
    cli_register_command(cli, cc, "application", cli_policy_add_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name] [Policy...], Add a policy to an application");

    cc = cli_register_command(cli, c, "del", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_policy_del_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name] [Policy#], Delete a policy from a component");
    cli_register_command(cli, cc, "application", cli_policy_del_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], [Policy#], Delete a policy from an application");

    cc = cli_register_command(cli, c, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_policy_show_component, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Component Name], Show all policies of a component");
    cli_register_command(cli, cc, "application", cli_policy_show_application, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Application Name], Show all policies of an application");

    c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "event", cli_show_event, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Event Name], Show the components mapped to an event");
    cli_register_command(cli, c, "app_event", cli_show_app_event, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[App_event Name], Show the applications mapped to an app event");
    cli_register_command(cli, c, "component", cli_show_component, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Component Name], Show the configuration of a component");
    cli_register_command(cli, c, "application", cli_show_application, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Application Name], Show the configuration of an application");

    c = cli_register_command(cli, NULL, "list", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "events", cli_list_events, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all events for components");
    cli_register_command(cli, c, "app_events", cli_list_app_events, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all events for applications");
    cli_register_command(cli, c, "components", cli_list_components, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all components");
    cli_register_command(cli, c, "applications", cli_list_applications, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all applications");

    cli_register_command(cli, NULL, "component", cli_component, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Component Name] [Arguments...], Run a command in a component");
    cli_register_command(cli, NULL, "application", cli_application, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Application Name] [Arguments...], Run a command in an application");

    cli_register_command(cli, NULL, "log", cli_log, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[# of lines], Print out log messages (default: 20)");
    cli_register_command(cli, NULL, "exit", cli_exit, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Terminate the Barista NOS");

    cli_set_auth_callback(cli, check_auth);
    cli_set_enable_callback(cli, check_enable);

    int cli_sock, acc_sock;
    struct sockaddr_in addr;
    int num_conn = __CLI_MAX_CONNECTIONS;
    int on = 1;

    if ((cli_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(cli_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(__CLI_HOST);
    addr.sin_port = htons(__CLI_PORT);

    if (bind(cli_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(cli_sock, num_conn) < 0) {
        perror("listen");
        return -1;
    }

    if (ctx->autostart) {
        // load components
        if (component_load(NULL, cli_ctx)) {
            PRINTF("Failed to load component configurations\n");
            return -1;
        }

        // load applications
        if (application_load(NULL, cli_ctx)) {
            PRINTF("Failed to load application configurations\n");
            return -1;
        }

        // activate log
        if (component_activate(NULL, "log")) {
            PRINTF("Failed to activate the log component\n");
            return -1;
        }

        waitsec(1, 0);

        // start components
        if (component_start(cli)) {
            PRINTF("Failed to start components\n");
            return -1;
        }

        // start applications
        if (application_start(cli)) {
            PRINTF("Failed to start applications\n");
            return -1;
        }

        // automatically load operator-defined policies
        if (load_odp(NULL)) {
            PRINTF("No operator-defined policy\n");
            return -1;
        }
    }

    sigint_func = signal(SIGINT, sigint_handler);
    sigkill_func = signal(SIGKILL, sigkill_handler);
    sigterm_func = signal(SIGTERM, sigterm_handler);

    PRINTF("Listening on port %d for the Barista CLI\n", __CLI_PORT);

    while ((acc_sock = accept(cli_sock, NULL, 0))) {
        socklen_t len = sizeof(addr);
        if (getpeername(acc_sock, (struct sockaddr *)&addr, &len) >= 0) {
            PRINTF("Accepted connection from %s\n", inet_ntoa(addr.sin_addr));
        }

        struct arg_struct args;
        args.cli = cli;
        args.acc_sock = acc_sock;

        pthread_t thread;
        if (pthread_create(&thread, NULL, connection_handler, (void *)&args) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    if (acc_sock < 0) {
        PERROR("accept");
        return -1;
    }

    cli_done(cli);

    return 0;
}

/**
 * @}
 *
 * @}
 */

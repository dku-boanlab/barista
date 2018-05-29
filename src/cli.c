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

#include <signal.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

/////////////////////////////////////////////////////////////////////

#include "cli_password.h"

/////////////////////////////////////////////////////////////////////

/** \brief The pointer of the context structure */
ctx_t *cli_ctx;

/** \brief The pointer of the CLI context */
struct cli_def *cli;

/////////////////////////////////////////////////////////////////////

/** \brief The SIGINT handler definition */
void (*sigint_func)(int);

/** \brief Function to handle the SIGINT signal */
void sigint_handler(int sig)
{
    PRINTF("Got a SIGINT signal\n");

    component_stop(NULL);

    waitsec(0, 1000 * 1000);

    application_stop(NULL);

    waitsec(0, 1000 * 1000);

    component_deactivate(NULL, "log");

    cli_done(cli);

    exit(0);
}

/** \brief The SIGKILL handler definition */
void (*sigkill_func)(int);

/** \brief Function to handle the SIGKILL signal */
void sigkill_handler(int sig)
{
    PRINTF("Got a SIGKILL signal\n");

    component_stop(NULL);

    waitsec(0, 1000 * 1000);

    application_stop(NULL);

    waitsec(0, 1000 * 1000);

    component_deactivate(NULL, "log");

    cli_done(cli);

    exit(0);
}

/** \brief The SIGTERM handler definition */
void (*sigterm_func)(int);

 /** \brief Function to handle the SIGTERM signal */
void sigterm_handler(int sig)
{
    PRINTF("Got a SIGTERM signal\n");

    component_stop(NULL);

    waitsec(0, 1000 * 1000);

    application_stop(NULL);

    waitsec(0, 1000 * 1000);

    component_deactivate(NULL, "log");

    cli_done(cli);

    exit(0);
}

/////////////////////////////////////////////////////////////////////

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
    FILE *fp = fopen(DEFAULT_ODP_FILE, "r");
    if (fp != NULL) {
        char buf[__CONF_STR_LEN] = {0};

        cli_print(cli, "Operator-defined policy file: %s", DEFAULT_ODP_FILE);

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
        }

        fclose(fp);
    } else {
        cli_print(cli, "No operator-defined policy");
    }

    return CLI_OK;
}

static int cli_start(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (component_start(cli)) {
        cli_print(cli, "Failed to start components");
        return CLI_ERROR;
    }

    waitsec(0, 1000 * 1000);

    if (application_start(cli)) {
        cli_print(cli, "Failed to start applications");
        return CLI_ERROR;
    }

    cli_print(cli, "\nBarista is excuted!");

    return CLI_OK;
}

static int cli_stop(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (component_stop(cli)) {
        cli_print(cli, "Failed to stop components");
        return CLI_ERROR;
    }

    waitsec(0, 1000 * 1000);

    if (application_stop(cli)) {
        cli_print(cli, "Failed to stop applications");
        return CLI_ERROR;
    }

    cli_print(cli, "\nBarista is terminated!");

    return CLI_OK;
}

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

static int cli_show_switch(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "switch_mgmt show %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_show_host_switch(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "host_mgmt show switch %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_show_host_ip(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "host_mgmt show ip %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_show_host_mac(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "host_mgmt show mac %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_show_link(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "topo_mgmt show %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_show_flow_switch(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "flow_mgmt show %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_list_events(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    event_list(cli);

    return CLI_OK;
}

static int cli_list_app_events(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    app_event_list(cli);

    return CLI_OK;
}

static int cli_list_components(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    component_list(cli);

    return CLI_OK;
}

static int cli_list_applications(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    application_list(cli);

    return CLI_OK;
}

static int cli_list_switches(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    sprintf(cmd, "switch_mgmt list switches");

    int cnt = 0;
    char *args[__CONF_ARGC + 1] = {0};

    str2args(cmd, &cnt, args, __CONF_ARGC);

    component_cli(cli, &args[0]);

    return CLI_OK;
}

static int cli_list_hosts(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    sprintf(cmd, "host_mgmt list hosts");

    int cnt = 0;
    char *args[__CONF_ARGC + 1] = {0};

    str2args(cmd, &cnt, args, __CONF_ARGC);

    component_cli(cli, &args[0]);

    return CLI_OK;
}

static int cli_list_links(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    sprintf(cmd, "topo_mgmt list links");

    int cnt = 0;
    char *args[__CONF_ARGC + 1] = {0};

    str2args(cmd, &cnt, args, __CONF_ARGC);

    component_cli(cli, &args[0]);

    return CLI_OK;
}

static int cli_list_flows(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    sprintf(cmd, "flow_mgmt list flows");

    int cnt = 0;
    char *args[__CONF_ARGC + 1] = {0};

    str2args(cmd, &cnt, args, __CONF_ARGC);

    component_cli(cli, &args[0]);

    return CLI_OK;
}

static int cli_stat_resource(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "resource_mgmt stat %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

static int cli_stat_traffic(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    char cmd[__CONF_WORD_LEN] = {0};

    if (argc == 1) {
        sprintf(cmd, "traffic_mgmt stat %s", argv[0]);

        int cnt = 0;
        char *args[__CONF_ARGC + 1] = {0};

        str2args(cmd, &cnt, args, __CONF_ARGC);

        component_cli(cli, &args[0]);
    }

    return CLI_OK;
}

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

static int cli_log(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    int line = 20;

    if (argc == 1) {
        line = atoi(argv[0]);
    } else if (argc > 1) {
        return CLI_OK;
    }

    FILE *log = fopen("../log/message.log", "r");
    if (log == NULL) {
        PERROR("fopen");
        return CLI_OK;
    }

    fseek(log, 0, SEEK_END);

    int cnt = 0;
    long int pos = ftell(log);
    while (pos) {
        fseek(log, --pos, SEEK_SET);
        if (fgetc(log) == '\n') {
            if (cnt++ == line)
                break;
        }
    }

    char buf[__CONF_STR_LEN] = {0};
    while (fgets(buf, sizeof(buf), log) != NULL) {
        buf[strlen(buf)-1] = '\0';
        cli_print(cli, "%s", buf);
    }

    fclose(log);

    return CLI_OK;
}

static int cli_exit(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    component_stop(cli);

    waitsec(0, 1000 * 1000);

    application_stop(cli);

    waitsec(0, 1000 * 1000);

    cli_print(cli, " ");

    component_deactivate(cli, "log");

    cli_print(cli, "\nReady to be terminated");

    return CLI_OK;
}

static int check_auth(const char *username, const char *password)
{
    if (strcasecmp(username, USERID) != 0)
        return CLI_ERROR;
    if (strcasecmp(password, USERPW) != 0)
        return CLI_ERROR;
    return CLI_OK;
}

static int regular_callback(struct cli_def *cli)
{
    return CLI_OK;
}

static int check_enable(const char *password)
{
    return !strcasecmp(password, ADMINPW);
}

static int idle_timeout(struct cli_def *cli)
{
    return CLI_QUIT;
}

struct arg_struct {
    struct cli_def *cli;
    int acc_sock;
};

static void *connection_handler(void *arguments)
{
    struct arg_struct *args = (struct arg_struct *)arguments;
    struct cli_def *cli = args->cli;
    int acc_sock = args->acc_sock;

    cli_loop(cli, acc_sock);

    return NULL;
}

/** \brief Function to provide the Barista CLI */
int start_cli(ctx_t *ctx)
{
    cli_ctx = ctx;

    struct cli_command *c, *cc;
    int cli_sock, acc_sock;
    struct sockaddr_in addr;
    int on = 1;

    signal(SIGCHLD, SIG_IGN);

    cli = cli_init();
    cli_set_banner(cli, "Barista Console");
    cli_set_hostname(cli, "Barista");
    cli_telnet_protocol(cli, 1);
    cli_regular(cli, regular_callback);
    cli_regular_interval(cli, 5);
    cli_set_idle_timeout_callback(cli, 600, idle_timeout);

    cli_register_command(cli, NULL, "load", cli_load, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Load configurations");

    cli_register_command(cli, NULL, "start", cli_start, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Start the Barista NOS");
    cli_register_command(cli, NULL, "stop", cli_stop, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Stop the Barista NOS");

    c = cli_register_command(cli, NULL, "set", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);

    cc = cli_register_command(cli, c, "enable", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_enable_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Enable components");
    cli_register_command(cli, cc, "application", cli_set_enable_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Enable applications");

    cc = cli_register_command(cli, c, "disable", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_disable_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Disable components");
    cli_register_command(cli, cc, "application", cli_set_disable_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Disable applications");

    cc = cli_register_command(cli, c, "activate", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "component", cli_set_activate_component, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Component Name], Activate components");
    cli_register_command(cli, cc, "application", cli_set_activate_application, PRIVILEGE_PRIVILEGED, MODE_EXEC, "[Application Name], Activate applications");

    cc = cli_register_command(cli, c, "deactivate", NULL, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
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

    cli_register_command(cli, c, "switch", cli_show_switch, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Datapath ID], Show the information of a switch");

    cc = cli_register_command(cli, c, "host", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, cc, "switch", cli_show_host_switch, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Datapath ID], Show all hosts in a switch");
    cli_register_command(cli, cc, "ip", cli_show_host_ip, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[IP address], Show a host that has a specific IP address");
    cli_register_command(cli, cc, "mac", cli_show_host_mac, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[MAC address], Show a host that has a specific MAC address");

    cli_register_command(cli, c, "link", cli_show_link, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Datapath ID], Show all links of a switch");
    cli_register_command(cli, c, "flow", cli_show_flow_switch, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Datapath ID], Show flow rules in a switch");

    c = cli_register_command(cli, NULL, "list", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "events", cli_list_events, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all events for components");
    cli_register_command(cli, c, "app_events", cli_list_app_events, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all events for applications");
    cli_register_command(cli, c, "components", cli_list_components, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all components");
    cli_register_command(cli, c, "applications", cli_list_applications, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all applications");

    cli_register_command(cli, c, "switches", cli_list_switches, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all switches");
    cli_register_command(cli, c, "hosts", cli_list_hosts, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all hosts");
    cli_register_command(cli, c, "links", cli_list_links, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all links");
    cli_register_command(cli, c, "flows", cli_list_flows, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "List up all flow rules");

    c = cli_register_command(cli, NULL, "stat", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "resource", cli_stat_resource, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Time(sec)], Show the resource usage for the last n seconds");
    cli_register_command(cli, c, "traffic", cli_stat_traffic, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Time(sec)], Show the traffic statistics for the last n seconds");

    cli_register_command(cli, NULL, "component", cli_component, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Component Name] [Arguments...], Run a command in a component");
    cli_register_command(cli, NULL, "application", cli_application, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[Application Name] [Arguments...], Run a command in an application");

    cli_register_command(cli, NULL, "log", cli_log, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "[# of lines], Print out log messages");
    cli_register_command(cli, NULL, "exit", cli_exit, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Terminate the Barista NOS");

    cli_set_auth_callback(cli, check_auth);
    cli_set_enable_callback(cli, check_enable);

    if ((cli_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(cli_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(__CLI_PORT);

    if (bind(cli_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(cli_sock, 10) < 0) {
        perror("listen");
        return -1;
    }

    if (ctx->autostart) {
        component_load(NULL, cli_ctx);
        application_load(NULL, cli_ctx);

        waitsec(0, 1000 * 1000);

        component_activate(NULL, "log");

        waitsec(0, 1000 * 1000);

        component_start(NULL);

        waitsec(0, 1000 * 1000);

        application_start(NULL);

        // automatically load operator-defined policies
        FILE *fp = fopen(DEFAULT_ODP_FILE, "r");
        if (fp != NULL) {
            char buf[__CONF_STR_LEN] = {0};

            PRINTF("Operator-defined policy file: %s\n", DEFAULT_ODP_FILE);

            while (fgets(buf, __CONF_STR_LEN-1, fp) != NULL) {
                if (buf[0] == '#') continue;

                int cnt = 0;
                char *args[__CONF_ARGC + 1] = {0};

                str2args(buf, &cnt, args, __CONF_ARGC);

                if (cnt != 3) continue;

                if (strcmp(args[0], "component") == 0) {
                    component_add_policy(NULL, args[1], args[2]);
                } else if (strcmp(args[0], "application") == 0) {
                    application_add_policy(NULL, args[1], args[2]);
                }
            }

            fclose(fp);
        } else {
            PRINTF("No operator-defined policy\n");
        }

        if (system("touch /tmp/barista.lock") != 0) {
            cli_print(cli, "Failed to create /tmp/barista.lock\n");
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

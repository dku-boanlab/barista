/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \defgroup framework Base Framework
 * @{
 * \defgroup base Base Framework
 * \brief Function to execute the Barista NOS
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "common.h"
#include "context.h"
#include "cli.h"

#include "event.h"
#include "app_event.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <spawn.h>
#include <sys/ioctl.h>

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
ctx_t ctx;

/////////////////////////////////////////////////////////////////////

/** \brief The default component configuration file */
char conf_file[__CONF_WORD_LEN] = COMPNT_DEFAULT_CONFIG_FILE;

/** \brief The default application configuration file */
char app_conf_file[__CONF_WORD_LEN] = APP_DEFAULT_CONFIG_FILE;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to daemonize the Barista NOS
 * \return None
 */
static void daemonize(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        PERROR("fork");
        exit(-1);
    } else if (pid != 0) {
        exit(0);
    }

    if (setsid() == -1) {
        PERROR("setsid");
    }

    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid == -1) {
        PERROR("fork");
        exit(-1);
    } else if (pid != 0) {
        exit(0);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (open("/dev/null", O_RDONLY) == -1) {
        PERROR("open");
        exit(0);
    }
    if (open("/dev/null", O_WRONLY) == -1) {
        PERROR("open");
        exit(0);
    }
    if (open("/dev/null", O_RDWR) == -1) {
        PERROR("open");
        exit(0);
    }
} 

/////////////////////////////////////////////////////////////////////

/** \brief Function to print the Barista logo */
static void print_logo(void)
{
    char tiny_log[] = {
    " Barista\n"
    };

    char small_log[] = {
    "    *****+.                     +@@\n" \
    "   +@@**%@@+                    +#+          +@@\n" \
    "   @@*   @@+   +%%#.+#. .#+.%@  **   =%@%# .#@@@#+  :#%%=:#=\n" \
    "  .@@#**@@=   @@%+#@@@  *@@@*# +@@  @@+.:* =%@@*#: %@@+*@@@:\n" \
    "  *@@**#@@%  @@#   %@#  @@%    @@*  @@%=    *@@   *@@   =@@\n" \
    "  @@*   =@@..@@.  :@@: .@@    .@@.   =%@@=  @@+   @@+   @@*\n" \
    " .@@=.:+@@* :@@+.#@@@  *@@    *@@  *.  @@= .@@+:. @@%.+@@@.\n" \
    " =@@@@@%*.   +@@@*+@+  @@=    %@+ .%@@@%:   %@@@  :@@@#:@%\n"
    };

    char big_logo[] = {
    "                                              .@@@%\n" \
    "      @@@@@@@@@%+                             =@@@.              =***\n" \
    "     .@@@+:::*@@@%                                               @@@*\n" \
    "     #@@#    =@@@:   .#@@@@@%@@@+   @@@=@@@@. @@@#   *%@@@@@%.#@@@@@@@@+  .#@@@@@%%@@+\n" \
    "    .@@@@%%@@@@+    +@@@+..+@@@@   :@@@@#..  =@@@.  @@@= ..#+ ..#@@#...  +@@@+..+@@@@\n" \
    "    +@@@%@@@@@@%+  =@@@.    #@@+   %@@@.     #@@#  .@@@%*.     .@@@:    =@@@.    *@@+\n" \
    "    @@@*     %@@@. @@@+    =@@@.  .@@@=     :@@@:    =#@@@@#   *@@%    .@@@+    :@@@.\n" \
    "   =@@@     +@@@* :@@@+   *@@@%   *@@@      *@@@   :    +@@@  .@@@+    .@@@+   *@@@%\n" \
    "   @@@@@@@@@@@@:   *@@@@@@@@@@:  .@@@+     .@@@=  @@@@@@@@@:  .@@@@@@=  *@@@@@@@@@@:\n" \
    "   *********:        ***+  ***   :***      :***    =*****       +***:     ***+  ***\n"
    };

    struct winsize w;

    ioctl(0, TIOCGWINSZ, &w);

    if (w.ws_col >= 128)
        PRINTF("\n%s\n", big_logo);
    else if (w.ws_col >= 64)
        PRINTF("\n%s\n", small_log);
    else
        PRINTF("\n%s\n", tiny_log);
}

/** \brief Function to print the Barista usage */
static void print_usage(char *name)
{
    PRINTF("Usage:\n");
    PRINTF("\t%s -c [config file] -a [app config file] -r {auto | base | daemon}\n", name);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to start the Barista NOS
 * \param argc The number of arguments
 * \param argv Arguments
 */
int main(int argc, char **argv)
{
    int opt;
    int autostart = FALSE;
    int daemon = FALSE;

    print_logo();

    if (argc >= 2) {
        while ((opt = getopt(argc, argv, "c:a:r:")) != -1) {
            switch (opt) {
            case 'c':
                memset(conf_file, 0, __CONF_WORD_LEN);
                strcpy(conf_file, optarg);
                break;
            case 'a':
                memset(app_conf_file, 0, __CONF_WORD_LEN);
                strcpy(app_conf_file, optarg);
                break;
            case 'r':
                if (strcmp(optarg, "auto") == 0) {
                    autostart = TRUE;
                } else if (strcmp(optarg, "base") == 0) {
                    memset(conf_file, 0, __CONF_WORD_LEN);
                    strcpy(conf_file, COMPNT_BASE_CONFIG_FILE);

                    memset(app_conf_file, 0, __CONF_WORD_LEN);
                    strcpy(app_conf_file, APP_BASE_CONFIG_FILE);
                } else if (strcmp(optarg, "daemon") == 0) {
                    daemon = TRUE;
                } else {
                    print_usage(argv[0]);
                    return -1;
                }
                break;
            default:
                print_usage(argv[0]);
                return -1;
            }
        }
    }

    // debug messages
    DEBUG("Structures in type.h\n");
    DEBUG("raw_msg_t: %lu\n", sizeof(raw_msg_t));
    DEBUG("msg_t: %lu\n", sizeof(msg_t));
    DEBUG("switch_t: %lu\n", sizeof(switch_t));
    DEBUG("port_t: %lu\n", sizeof(port_t));
    DEBUG("host_t: %lu\n", sizeof(host_t));
    DEBUG("pktin_t: %lu\n", sizeof(pktin_t));
    DEBUG("action_t: %lu\n", sizeof(action_t));
    DEBUG("pktout_t: %lu\n", sizeof(pktout_t));
    DEBUG("flow_t: %lu\n", sizeof(flow_t));
    DEBUG("traffic_t: %lu\n", sizeof(traffic_t));
    DEBUG("resource_t: %lu\n", sizeof(resource_t));
    DEBUG("odp_t: %lu\n", sizeof(odp_t));
    DEBUG("meta_event_t: %lu\n", sizeof(meta_event_t));

    // kill external packet I/O engine
    int pid;
    char *argb[] = {"pkill", "-9", "barista_sb", NULL};
    posix_spawn(&pid, "/usr/bin/pkill", NULL, NULL, argb, NULL);

    // kill resource monitor
    char *argr[] = {"pkill", "-9", "barista_rsm", NULL};
    posix_spawn(&pid, "/usr/bin/pkill", NULL, NULL, argr, NULL);

    if (daemon == TRUE)
        daemonize();

    // initialize context
    ctx_init(&ctx);

    // set autostart
    if (autostart == TRUE)
        ctx.autostart = TRUE;

    // store the conf file in the context
    strcpy(ctx.conf_file, conf_file);

    // store the app conf file in the context
    strcpy(ctx.app_conf_file, app_conf_file);

    // initialize event handler
    event_init(&ctx);

    // initialize app event handler
    app_event_init(&ctx);

    // execute cli
    start_cli(&ctx);

    return 0;
}

/**
 * @}
 *
 * @}
 */

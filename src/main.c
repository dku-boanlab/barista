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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
ctx_t ctx;

/////////////////////////////////////////////////////////////////////

/** \brief The default component configuration file */
char conf_file[__CONF_WORD_LEN] = DEFAULT_COMPNT_CONFIG_FILE;

/** \brief The default application configuration file */
char app_conf_file[__CONF_WORD_LEN] = DEFAULT_APP_CONFIG_FILE;

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

/** \brief Function to print out the Barista logo */
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

/** \brief Function to print out the Barista usage */
static void print_usage(char *name)
{
    PRINTF("Usage:\n");
    PRINTF("\t%s\n", name);
    PRINTF("\t\t-c [config file]\n");
    PRINTF("\t\t-a [app config file]\n");
    PRINTF("\t\t-b (no value, run with base components)\n");
    PRINTF("\t\t-r (no value, run in auto-start mode)\n");
    PRINTF("\t\t-d (no value, run in daemon mode)\n");
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
        while ((opt = getopt(argc, argv, "c:a:brd")) != -1) {
            switch (opt) {
            case 'c':
                memset(conf_file, 0, __CONF_WORD_LEN);
                strcpy(conf_file, optarg);
                break;
            case 'a':
                memset(app_conf_file, 0, __CONF_WORD_LEN);
                strcpy(app_conf_file, optarg);
                break;
            case 'b':
                memset(conf_file, 0, __CONF_WORD_LEN);
                strcpy(conf_file, BASE_COMPNT_CONFIG_FILE);

                memset(app_conf_file, 0, __CONF_WORD_LEN);
                strcpy(app_conf_file, BASE_APP_CONFIG_FILE);
                break;
            case 'r':
                autostart = TRUE;
                break;
            case 'd':
                daemon = TRUE;
                break;
            default:
                print_usage(argv[0]);
                return -1;
            }
        }
    }

    // initialize context
    ctx_init(&ctx);

    // daemonize the Barista NOS
    if (daemon == TRUE)
        daemonize();

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

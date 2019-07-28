/*
 * Copyright 2015-2019 NSSLab, KAIST
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
#include "storage.h"
#include "cli.h"

#include "event.h"
#include "app_event.h"

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
ctx_t ctx;

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

/**
 * \brief Function to print the Barista logo
 * \return None
 */
static void print_logo(void)
{
    return;

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

/**
 * \brief Function to print the Barista usage
 * \param name Program name
 */
static void print_usage(char *name)
{
    PRINTF("Usage:\n");
    PRINTF("\t%s\n", name);
    PRINTF("\t\t-c [component config file]\n");
    PRINTF("\t\t-a [application config file]\n");
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

    print_logo();

    // initialize context
    init_ctx(&ctx);

    if (argc >= 2) {
        while ((opt = getopt(argc, argv, "c:a:brd")) != -1) {
            switch (opt) {
            case 'c':
                // component config file
                strcpy(ctx.conf_file, optarg);
                break;
            case 'a':
                // application config file
                strcpy(ctx.app_conf_file, optarg);
                break;
            case 'b':
                // base component config file
                strcpy(ctx.conf_file, BASE_COMPNT_CONFIG_FILE);

                // base application config file
                strcpy(ctx.app_conf_file, BASE_APP_CONFIG_FILE);
                break;
            case 'r':
                // set autostart
                ctx.autostart = TRUE;
                break;
            case 'd':
                // daemonize the Barista NOS
                daemonize();
                break;
            default:
                print_usage(argv[0]);
                return -1;
            }
        }
    }

    // default component config file
    if (ctx.conf_file[0] == '\0')
        strcpy(ctx.conf_file, DEFAULT_COMPNT_CONFIG_FILE);

    // default application config file
    if (ctx.app_conf_file[0] == '\0')
        strcpy(ctx.app_conf_file, DEFAULT_APP_CONFIG_FILE);

    // initialize storage
    init_storage(&ctx);

    // initialize event handler
    init_event(&ctx);

    // initialize app event handler
    init_app_event(&ctx);

    // execute cli
    start_cli(&ctx);

    return 0;
}

/**
 * @}
 *
 * @}
 */

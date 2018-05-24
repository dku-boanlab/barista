/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

/** \brief The macro for the main function of an application */
#define DECLARE_MAIN_FUNC(name) \
    extern int name(int *activated, int argc, char **argv)
/** \brief The macro for the handler function of an application */
#define DECLARE_HANDLER_FUNC(name) \
    extern int name(const app_event_t *av, app_event_out_t *av_out)
/** \brief The macro for the cleanup function of an application */
#define DECLARE_CLEANUP_FUNC(name) \
    extern int name(int *activated)
/** \brief The macro for the CLI function of an application */
#define DECLARE_CLI_FUNC(name) \
    extern int name(cli_t *cli, char **args)

DECLARE_MAIN_FUNC(l2_shortest_ext_main);
DECLARE_HANDLER_FUNC(l2_shortest_ext_handler);
DECLARE_CLEANUP_FUNC(l2_shortest_ext_cleanup);
DECLARE_CLI_FUNC(l2_shortest_ext_cli);

/** \brief The function pointer of an application */
struct _app_func_t {
    char *name; /**< Application name */
    app_main_f main; /**< The main function pointer */
    app_handler_f handler; /**< The handler function pointer */
    app_cleanup_f cleanup; /**< The cleanup function pointer */
    app_cli_f cli; /**< The CLI function pointer */
} g_applications[] = {
    {"l2_shortest_ext", l2_shortest_ext_main, l2_shortest_ext_handler, l2_shortest_ext_cleanup, l2_shortest_ext_cli},
}; /**< The list of function pointers */

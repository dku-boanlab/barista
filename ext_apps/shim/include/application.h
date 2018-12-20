/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_shim External Application Handler
 * @{
 * \ingroup app_load External Application Loader
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

/** \brief The main function pointer of an application */
typedef int (* app_main_f)(int *activated, int argc, char **argv);
/** \brief The handler function pointer of an application */
typedef int (* app_handler_f)(const app_event_t *ev, app_event_out_t *ev_out);
/** \brief The cleanup function pointer of an application */
typedef int (* app_cleanup_f)(int *activated);
/** \brief The cli function pointer of an application */
typedef int (* app_cli_f)(cli_t *cli, char **args);

/** \brief The type of an application */
enum {
    APP_GENERAL,
    APP_AUTO,
};

/** \brief The structure of an application */
struct _app_t {
    uint32_t app_id; /**< Application ID */

    char name[__CONF_WORD_LEN]; /**< Application name */
    char args[__CONF_WORD_LEN]; /**< Unparsed arguments */

    int argc; /**< The number of arguments */
    char *argv[__CONF_ARGC + 1]; /**< Arguments */

    int type; /**< Type */
    int activated; /**< Activation */

    app_main_f main; /**< The main function pointer */
    app_handler_f handler; /**< The handler function pointer */
    app_cleanup_f cleanup; /**< The cleanup function pointer */
    app_cli_f cli; /**< The CLI function pointer */
};

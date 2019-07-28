/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

/** \brief The main function pointer of a component */
typedef int (* compnt_main_f)(int *activated, int argc, char **argv);
/** \brief The handler function pointer of a component */
typedef int (* compnt_handler_f)(const event_t *ev, event_out_t *ev_out);
/** \brief The cleanup function pointer of a component */
typedef int (* compnt_cleanup_f)(int *activated);
/** \brief The cli function pointer of a component */
typedef int (* compnt_cli_f)(cli_t *cli, char **args);

/** \brief The type of a component */
enum {
    COMPNT_GENERAL,
    COMPNT_AUTO,
};

/** \brief The structure of a component */
struct _compnt_t {
    uint32_t component_id; /**< Component ID */

    char name[__CONF_WORD_LEN]; /**< Component name */
    char args[__CONF_WORD_LEN]; /**< Unparsed arguments */

    int argc; /**< The number of arguments */
    char *argv[__CONF_ARGC + 1]; /**< Arguments */

    int type; /**< Type */
    int activated; /**< Activation */

    compnt_main_f main; /**< The main function pointer */
    compnt_handler_f handler; /**< The handler function pointer */
    compnt_cleanup_f cleanup; /**< The cleanup function pointer */
    compnt_cli_f cli; /**< The CLI function pointer */
};


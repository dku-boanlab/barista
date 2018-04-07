/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "context.h"
#include "hash.h"
#include "str.h"

#include <jansson.h>
#include <arpa/inet.h>

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

/** \brief The site of an application */
enum {
    APP_INTERNAL,
    APP_EXTERNAL,
};

/** \brief The status of an application */
enum {
    APP_DISABLED,
    APP_ENABLED,
};

/** \brief The role of an application */
enum {
    APP_BASE,
    APP_NETWORK,
    APP_MANAGEMENT,
    APP_SECURITY,
    APP_ADMIN,
};

/** \brief The permission of an application */
enum {
    APP_READ = 4,
    APP_WRITE = 2,
    APP_EXECUTE = 1,
    APP_MAX_PERM = 3
};

/** \brief The structure of an application */
struct _app_t {
    int id; /**< internal ID */
    uint32_t app_id; /**< Application ID */

    char name[__CONF_WORD_LEN]; /**< Application name */
    char args[__CONF_WORD_LEN]; /**< Original argument string */

    int argc; /**< The number of arguments */
    char *argv[__CONF_ARGC + 1]; /**< Arguments */

    int type; /**< Type */
    int site; /**< Site */
    int role; /**< Role */
    int perm; /**< Permission */
    int status; /**< Status */
    int activated; /**< Activation */

    void *push_ctx; /**< Context to push app events */
    void *push_sock; /**< Socket to push app events */

    void *req_ctx; /**< Context to request app events */
    void *req_sock; /**< Socket to request app events */

    app_main_f main; /**< The main function pointer */
    app_handler_f handler; /**< The handler function pointer */
    app_cleanup_f cleanup; /**< The cleanup function pointer */
    app_cli_f cli; /**< The CLI function pointer */

    int in_num; /**< The number of inbound app events */
    int in_list[__MAX_APP_EVENTS]; /**< Inbound app events */

    int out_num; /**< The number of outbound app events */
    int out_list[__MAX_APP_EVENTS]; /**< Outbound app events */

    int num_policies; /**< The number of policies */
    odp_t odp[__MAX_POLICIES]; /**< The list of operator-defined policies */

    uint64_t num_app_events[__MAX_APP_EVENTS]; /**< The number of triggered times */
    double time_events[__MAX_APP_EVENTS]; /** The cumulative latencies for each event */
};

// function for the base framework
int application_load(cli_t *, ctx_t *);
int application_start(cli_t *);
int application_stop(cli_t *);

// functions for cli
int app_event_show(cli_t *, char *);
int app_event_list(cli_t *);

int application_enable(cli_t *, char *);
int application_disable(cli_t *, char *);
int application_activate(cli_t *, char *);
int application_deactivate(cli_t *, char *);
int application_add_policy(cli_t *, char *, char *);
int application_del_policy(cli_t *, char *, int);
int application_show_policy(cli_t *, char *);
int application_show(cli_t *, char *);
int application_list(cli_t *);
int application_cli(cli_t *, char **);

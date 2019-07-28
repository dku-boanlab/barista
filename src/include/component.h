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

/** \brief The site of a component */
enum {
    COMPNT_INTERNAL,
    COMPNT_EXTERNAL,
};

/** \brief The status of a component */
enum {
    COMPNT_DISABLED,
    COMPNT_ENABLED,
};

/** \brief The role of a component */
enum {
    COMPNT_BASE,
    COMPNT_NETWORK,
    COMPNT_MANAGEMENT,
    COMPNT_SECURITY,
    COMPNT_SECURITY_V2,
    COMPNT_ADMIN,
};

/** \brief The permission of a component */
enum {
    COMPNT_READ = 4,
    COMPNT_WRITE = 2,
    COMPNT_EXECUTE = 1
};

/** \brief The structure of a component */
struct _compnt_t {
    int id; /**< Internal ID */
    uint32_t component_id; /**< Component ID */

    char name[__CONF_WORD_LEN]; /**< Component name */
    char args[__CONF_WORD_LEN]; /**< Unparsed arguments */

    int argc; /**< The number of arguments */
    char *argv[__CONF_ARGC + 1]; /**< Arguments */

    int type; /**< Type */
    int site; /**< Site */
    int role; /**< Role */
    int perm; /**< Permission */
    int status; /**< Status */
    int priority; /**< Priority */
    int activated; /**< Activation */

    void *push_ctx; /**< Push context */
    char push_addr[__CONF_WORD_LEN]; /**< Push address */

    void *req_ctx; /**< Request context */
    char req_addr[__CONF_WORD_LEN]; /**< Request address */

    compnt_main_f main; /**< The main function pointer */
    compnt_handler_f handler; /**< The handler function pointer */
    compnt_cleanup_f cleanup; /**< The cleanup function pointer */
    compnt_cli_f cli; /**< The CLI function pointer */

    int in_num; /**< The number of inbound events */
    int in_list[__MAX_EVENTS]; /**< Inbound events */
    int in_perm[__MAX_EVENTS]; /**< Inbound event permissions */

    int out_num; /**< The number of outbound events */
    int out_list[__MAX_EVENTS]; /**< Outbound events */

    int num_policies; /**< The number of policies */
    odp_t odp[__MAX_POLICIES]; /**< The list of operator-defined policies */

    uint64_t num_events[__MAX_EVENTS]; /**< The number of triggered times */
};

// function for the base framework
int component_load(cli_t *, ctx_t *);
int component_start(cli_t *);
int component_stop(cli_t *);

// functions for cli
int event_show(cli_t *, char *);
int event_list(cli_t *);

int component_enable(cli_t *, char *);
int component_disable(cli_t *, char *);
int component_activate(cli_t *, char *);
int component_deactivate(cli_t *, char *);
int component_add_policy(cli_t *, char *, char *);
int component_del_policy(cli_t *, char *, int);
int component_show_policy(cli_t *, char *);
int component_show(cli_t *, char *);
int component_list(cli_t *);
int component_cli(cli_t *, char **);

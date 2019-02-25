/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

/** \brief The context structure of the Barista NOS */
struct _ctx_t {
    int autostart; /**< Autostart */

    // component
    int compnt_on; /**< Component flag */
    char conf_file[__CONF_WORD_LEN]; /**< Component configuration file */

    int num_compnts; /**< The number of components */
    compnt_t **compnt_list; /**< Component list */

    int *ev_num; /**< The number of events */
    compnt_t ***ev_list; /**< Component chains for each event */

#ifdef __ENABLE_META_EVENTS
    int num_events[__MAX_EVENTS]; /**< Counters for each event */
    meta_event_t meta_event[__MAX_META_EVENTS]; /**< Meta events */
#endif /* __ENABLE_META_EVENTS */

    // event handler
    int ev_on; /**< The running flag of the event handler */

    // application
    int app_on; /**< Application flag */
    char app_conf_file[__CONF_WORD_LEN]; /**< Application configuration file */

    int num_apps; /**< The number of applications */
    app_t **app_list; /**< Application list */

    int *av_num; /**< The number of app events */
    app_t ***av_list; /**< Application chains for each app event */

#ifdef __ENABLE_META_EVENTS
    int num_app_events[__MAX_APP_EVENTS]; /**< Counters for each app event */
    meta_event_t meta_app_event[__MAX_META_EVENTS]; /**< Meta app events */
#endif /* __ENABLE_META_EVENTS */

    // app event handler
    int av_on; /**< The running flag of the application event handler */
};

int ctx_init(ctx_t *ctx);


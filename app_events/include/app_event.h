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
#include "app_event_id.h"
#include "app_event_list.h"

/** \brief The structure of an app event (read-only) */
typedef struct _app_event_t {
    // header
    const uint32_t id; /**< Application ID */
    const uint16_t type; /**< App event type */
    const uint16_t length; /**< Data length */
    const uint32_t checksum; /**< Checksum */

    // body
    union {
        const switch_t   *sw; /**< The pointer of a switch */
        const port_t     *port; /**< The pointer of a port */
        const host_t     *host; /**< The pointer of a host */
        const flow_t     *flow; /**< The pointer of a flow */

        const pktin_t    *pktin; /**< The pointer of a pktin */
        const pktout_t   *pktout; /**< The pointer of a pktout */

        const char       *log; /**< The pointer of a log */
    };
} app_event_t;

/** \brief The structure of an app event (read-write) */
typedef struct _app_event_out_t {
    // header
    uint32_t id; /**< Application ID */
    uint16_t type; /**< App event type */
    uint16_t length; /**< Data length */
    uint32_t checksum; /**< Checksum */

    // body
    union {
        switch_t         *sw_data; /**< The pointer of a switch */
        port_t           *port_data; /**< The pointer of a port */
        host_t           *host_data; /**< The pointer of a host */
        flow_t           *flow_data; /**< The pointer of a flow */

        uint8_t          *data; /**< The pointer of data */
    };
} app_event_out_t;

int app_event_init(ctx_t *ctx);
int destroy_av_workers(ctx_t *ctx);

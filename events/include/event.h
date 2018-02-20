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
#include "event_id.h"
#include "event_list.h"

/** \brief The structure of an event (read-only) */
typedef struct _event_t {
    // header
    uint32_t id; /**< Component ID */
    uint16_t type; /**< Event type */
    uint16_t length; /**< The length of data */
    uint16_t prev; /**< The event before the previous event */
    uint16_t last; /**< The previous event */
    uint32_t checksum; /**< Checksum */

    // body
    union {
        const raw_msg_t  *msg; /**< The pointer of a message */

        const switch_t   *sw; /**< The pointer of a switch */
        const port_t     *port; /**< The pointer of a port */
        const host_t     *host; /**< The pointer of a host */
        const flow_t     *flow; /**< The pointer of a flow */

        const pktin_t    *pktin; /**< The pointer of a pktin */
        const pktout_t   *pktout; /**< The pointer of a pktout */

        const traffic_t  *traffic; /**< The pointer of a traffic stat */
        const resource_t *resource; /**< The pointer of a resource stat */

        const char       *log; /**< The pointer of a log */
    };
} event_t;

/** \brief The structure of an event (read-write) */
typedef struct _event_out_t {
    // header
    uint32_t id; /**< Component ID */
    uint16_t type; /**< Event type */
    uint16_t length; /**< The length of data */
    uint16_t prev; /**< The event before the previous event */
    uint16_t last; /**< The previous event */
    uint32_t checksum; /**< Checksum */

    // body
    union {
        switch_t         *sw_data; /**< The pointer of a switch */
        port_t           *port_data; /**< The pointer of a port */
        host_t           *host_data; /**< The pointer of a host */
        flow_t           *flow_data; /**< The pointer of a flow */
    };
} event_out_t;

int event_init(ctx_t *ctx);

/** \brief The cluster ID updated by cluster */
int cluster_id;


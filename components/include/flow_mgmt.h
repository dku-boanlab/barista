/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"

/////////////////////////////////////////////////////////////////////

#include "flow_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a flow table */
typedef struct _flow_table_t {
    flow_t *head; /**< The head pointer */
    flow_t *tail; /**< The tail pointer */

    pthread_spinlock_t lock; /**< The lock for management */
} flow_table_t;

/** \brief Flow tables */
flow_table_t *flow_table;

/** \brief Key for table lookup */
#define FLOW_KEY(a) (a->dpid % __MAX_NUM_SWITCHES)

/** \brief The time (second) to update flow states (e.g., statistics) */
#define FLOW_MGMT_UPDATE_TIME 5

/////////////////////////////////////////////////////////////////////

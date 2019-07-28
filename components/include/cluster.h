/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"
#include "database.h"
#include "base64.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for cluster */
database_t cluster_db;

/////////////////////////////////////////////////////////////////////

/** \brief The size of an encoded event */
#define __CLUSTER_EVENT_SIZE 2048

/** \brief The structure of a cluster event */
typedef struct _cluster_event_t {
    uint32_t id; /**< Component ID */
    uint32_t type; /**< Event type */
    uint32_t length; /**< Event size */
    char data[__CLUSTER_EVENT_SIZE]; /**< Encoded event */
} cluster_event_t;

/** \brief The number of events */
int num_cluster_events;

/** \brief The last event number retrieved from database */
uint64_t event_num;

/** \brief Event queue */
cluster_event_t *cluster_events;

/** \brief The lock for event update */
pthread_spinlock_t cluster_lock;

/////////////////////////////////////////////////////////////////////

/** \brief The batch size of events */
#define __CLUSTER_BATCH_SIZE 1024

/** \brief The update time (second) to a database */
#define __CLUSTER_UPDATE_TIME 1

/////////////////////////////////////////////////////////////////////

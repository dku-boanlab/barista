/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 * \author Yeonkeun Kim <yeonk@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"
#include "database.h"
#include "lldp.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for topo_mgmt */
database_t topo_mgmt_db;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a topology */
typedef struct _topo_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t remote; /**< Remote switch */
    port_t link[__MAX_NUM_PORTS]; /**< Links */
} topo_t;

/** \brief Network topology */
topo_t *topo;

/** \brief Lock for topology update */
pthread_rwlock_t topo_lock;

/////////////////////////////////////////////////////////////////////

/** \brief Link discovery period */
#define __TOPO_MGMT_REQUEST_TIME 10

/////////////////////////////////////////////////////////////////////

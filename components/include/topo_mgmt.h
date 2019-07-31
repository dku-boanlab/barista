/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/** \brief The database information for topo_mgmt */
db_info_t topo_mgmt_info;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a topology */
typedef struct _topo_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t remote; /**< Remote switch */
    port_t link[__MAX_NUM_PORTS]; /**< Links */
} topo_t;

/** \brief Network topology */
topo_t *topo;

/** \brief Locks for switches */
pthread_spinlock_t topo_lock[__MAX_NUM_SWITCHES];

/////////////////////////////////////////////////////////////////////

/** \brief Link discovery period */
#define __TOPO_MGMT_REQUEST_TIME 10

/////////////////////////////////////////////////////////////////////

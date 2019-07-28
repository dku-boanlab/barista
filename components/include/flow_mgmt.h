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

/////////////////////////////////////////////////////////////////////

#include "flow_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief The number of flows */
int num_flows;

/** \brief Flow tables */
flow_table_t *flow_table;

/** \brief Key for table lookup */
#define FLOW_KEY(a) (a->dpid % __DEFAULT_TABLE_SIZE)

/** \brief The request time (second) for flow statistics collection */
#define FLOW_MGMT_REQUEST_TIME 5

/////////////////////////////////////////////////////////////////////

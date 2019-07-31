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
#include "database.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database information for channel_mgmt */
db_info_t channel_mgmt_info;

/////////////////////////////////////////////////////////////////////

/** \brief Traffic statistics */
traffic_t traffic;

/** \brief Lock to update statistics */
pthread_spinlock_t tr_lock;

/////////////////////////////////////////////////////////////////////

/** \brief The monitoring time (second) per traffic statistics */
#define __CHANNEL_MGMT_MONITOR_TIME 10

/////////////////////////////////////////////////////////////////////

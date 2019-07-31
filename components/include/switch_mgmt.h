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

/** \brief The database information for switch_mgmt */
db_info_t switch_mgmt_info;

/////////////////////////////////////////////////////////////////////

/** \brief Switch table */
switch_t *switch_table;

/** \brief Locks for switches */
pthread_spinlock_t sw_lock[__MAX_NUM_SWITCHES];

/////////////////////////////////////////////////////////////////////

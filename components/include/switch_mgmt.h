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

/** \brief The database connector for switch_mgmt */
database_t switch_mgmt_db;

/////////////////////////////////////////////////////////////////////

/** \brief Switch table */
switch_t *switch_table;

/** \brief Lock for switch update */
pthread_rwlock_t sw_lock;

/////////////////////////////////////////////////////////////////////

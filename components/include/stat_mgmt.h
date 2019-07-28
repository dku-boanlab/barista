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

/** \brief Switch list */
uint64_t *switch_list;

/** \brief Lock for list updates */
pthread_rwlock_t stat_lock;

/////////////////////////////////////////////////////////////////////

/** \brief The request time (second) for statistics collection */
#define __STAT_MGMT_REQUEST_TIME 5

/////////////////////////////////////////////////////////////////////

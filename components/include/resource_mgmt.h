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

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for resource_mgmt */
database_t resource_mgmt_db;

/////////////////////////////////////////////////////////////////////

/** \brief Resource usage */
resource_t resource;

/** \brief Command to get CPU and memory usages */
char cmd[] = "ps -p $(pgrep -x barista) -o %cpu,%mem 2> /dev/null | tail -n 1";

/////////////////////////////////////////////////////////////////////

/** \brief The monitoring time (second) per resource usage */
#define __RESOURCE_MGMT_MONITOR_TIME 10

/////////////////////////////////////////////////////////////////////

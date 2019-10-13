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

/** \brief The database information for resource_mgmt */
db_info_t resource_mgmt_info;

/////////////////////////////////////////////////////////////////////

/** \brief Resource usage */
resource_t resource;

/** \brief Command to get CPU and memory usages */
//char cmd[] = "ps -p $(pgrep -x barista) -o %cpu,%mem 2> /dev/null | tail -n 1";
char cmd[] = "top -b -n 1 | grep `pgrep -x barista` | awk '{print $9 \" \" $10}' 2> /dev/null";

/////////////////////////////////////////////////////////////////////

/** \brief The monitoring time (second) per resource usage */
#define __RESOURCE_MGMT_MONITOR_TIME 10

/////////////////////////////////////////////////////////////////////

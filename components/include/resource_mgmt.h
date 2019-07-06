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
#include <signal.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#define RESOURCE_MGMT_MONITOR_TIME 1
#define RESOURCE_MGMT_HISTORY 86400
#define DEFAULT_RESOURCE_FILE "../log/resource.log"

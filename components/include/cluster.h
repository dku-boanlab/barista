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
#include "base64.h"
#include "mac2int.h"

#ifdef __ENABLE_CLUSTER

#include <mysql.h>

#endif /* __ENABLE_CLUSTER */

#define CLUSTER_UPDATE_TIME_NS (1000*1000)
#define CLUSTER_DELIMITER 2000

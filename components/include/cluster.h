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

#include <arpa/inet.h>

#ifdef __ENABLE_CLUSTER

#include <mysql.h>

#define CLUSTER_MYID "barista"
#define CLUSTER_MYPW "barista"

#endif /* __ENABLE_CLUSTER */

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
#include "hash.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database information for host_mgmt */
db_info_t host_mgmt_info;

/////////////////////////////////////////////////////////////////////

/** \brief The number of host entries */
#define NUM_HOST_ENTRIES 8192

/** \brief The structure of a host hash key */
typedef struct _host_key_t {
    uint32_t ip; /**< IP address */
    uint32_t pad; /**< Pad */
    uint64_t mac; /**< MAC address */
} host_key_t;

/** \brief The cache of recently added hosts */
host_t *host_cache;

/** \brief The locks for host entries */
pthread_spinlock_t host_lock[NUM_HOST_ENTRIES];

/////////////////////////////////////////////////////////////////////

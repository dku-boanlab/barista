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
#include "hash.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for host_mgmt */
database_t host_mgmt_db;

/////////////////////////////////////////////////////////////////////

/** \brief The number of hostentries */
#define NUM_HOST_ENTRIES 4096

/** \brief The structure of a host hash key */
typedef struct _host_key_t {
    uint32_t ip; /**< IP address */
    uint32_t pad; /**< Pad */
    uint64_t mac; /**< MAC address */
} host_key_t;

/** \brief The cache of recently added hosts */
host_t *host_cache;

/////////////////////////////////////////////////////////////////////

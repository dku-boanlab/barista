/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "app_event.h"
#include "database.h"
#include "hash.h"

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for L2 Learning */
database_t l2_learning_db;

/////////////////////////////////////////////////////////////////////

/** \brief The number of MAC entries */
#define NUM_MAC_ENTRIES 4096

/** \brief The structure of a MAC entry */
typedef struct _mac_entry_t {
    uint64_t dpid; /**< Datapath ID */
    uint16_t port; /**< Port */
    uint32_t ip; /**< IP address */
    uint64_t mac; /**< MAC address */
} mac_entry_t;

/** \brief The structure of a MAC hash key */
typedef struct _mac_key_t {
    uint64_t dpid; /**< Datapath ID */
    uint64_t mac; /**< MAC address */
} mac_key_t;

/** \brief The cache of recently added hosts */
mac_entry_t *mac_cache;

/////////////////////////////////////////////////////////////////////

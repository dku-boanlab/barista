/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/** \brief The database information for L2 Learning */
db_info_t l2_learning_info;

/////////////////////////////////////////////////////////////////////

/** \brief The number of MAC entries */
#define NUM_MAC_ENTRIES 8192

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

/** \brief The locks for MAC entries */
pthread_spinlock_t mac_lock[NUM_MAC_ENTRIES];

/////////////////////////////////////////////////////////////////////

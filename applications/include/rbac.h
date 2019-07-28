/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "application.h"
#include "app_event.h"
#include "hash.h"
#include "str.h"

/////////////////////////////////////////////////////////////////////

/** \brief The nubmer of app roles */
#define __NUM_APP_ROLES 5

/** \brief The structure to map application IDs and roles */
typedef struct _app_to_role_t {
    uint32_t id; /**< Application ID */
    uint32_t role; /**< Application role */
    char name[__CONF_WORD_LEN]; /**< Application name */
} app_to_role_t;

/** \brief The number of applications */
int app_to_role_cnt;

/** \brief The mapping table of applications and their roles */
app_to_role_t *app_to_role;

/////////////////////////////////////////////////////////////////////

/** \brief The structure to map app roles and events */
typedef struct _role_to_event_t {
    char name[__CONF_WORD_LEN]; /**< Role */
    uint8_t event[__MAX_APP_EVENTS]; /**< Events grouped by the role */
} role_to_event_t;

/** \brief The mapping table of roles and their events */
role_to_event_t *role_to_event;

/////////////////////////////////////////////////////////////////////

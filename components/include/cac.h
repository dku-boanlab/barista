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
#include "hash.h"
#include "str.h"

/////////////////////////////////////////////////////////////////////

/** \brief The structure to map components and their events */
typedef struct _compnt_to_event_t {
    uint32_t id; /**< Component ID */
    char name[__CONF_WORD_LEN]; /**< Component name */

    int in_num; /**< The number of inbound events */
    int in_list[__MAX_EVENTS]; /**< The list of inbound events */

    int out_num; /**< The number of outbound events */
    int out_list[__MAX_EVENTS]; /**< The list of outbound events */
} compnt_to_event_t;

/** \brief The number of components */
int compnt_to_event_cnt;

/** \brief Component list */
compnt_to_event_t *compnt_to_event;

/////////////////////////////////////////////////////////////////////

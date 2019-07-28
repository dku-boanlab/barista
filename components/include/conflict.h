/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Hyeonseong Jo <hsjjo@kaist.ac.kr>
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"

/////////////////////////////////////////////////////////////////////

#include "arr_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief The number of rules */
int num_rules;

/** \brief Rule tables */
rule_table_t *rule_table;

/** \brief The size of a match list */
#define __CONFLICT_MATCH_LIST_SIZE 20

/////////////////////////////////////////////////////////////////////

/** \brief Key for table lookup */
#define RULE_KEY(a) (a->dpid % __DEFAULT_TABLE_SIZE)

/////////////////////////////////////////////////////////////////////

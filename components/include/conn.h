/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 * \author Yeonkeun Kim <yeonk@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"

/////////////////////////////////////////////////////////////////////

#include "openflow10.h"

/////////////////////////////////////////////////////////////////////

/** \brief The structure to keep the remaining part of a message */
typedef struct _buffer_t {
    int need; /**< The bytes that it needs to read */
    int done; /**< The bytes that it has */
    uint8_t head[4]; /**< Partial header */
    uint8_t *data; /**< Full message */
} buffer_t;

/** \brief Buffers for all possible sockets */
buffer_t *buffer;

/////////////////////////////////////////////////////////////////////

#include "epoll_env.h"

/////////////////////////////////////////////////////////////////////

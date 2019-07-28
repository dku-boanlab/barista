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

/////////////////////////////////////////////////////////////////////

/** \brief The database connector for logging */
database_t log_db;

/////////////////////////////////////////////////////////////////////

/** \brief The number of log messages */
int num_msgs;

/** \brief Log message queue */
char **msgs;

/** \brief The lock for log update */
pthread_spinlock_t log_lock;

/////////////////////////////////////////////////////////////////////

/** \brief The batch size of log messages */
#define __LOG_BATCH_SIZE 1024

/** \brief The update time (second) to a file and a database */
#define __LOG_UPDATE_TIME 1

/** \brief The default log file */
#define __DEFAULT_LOG_FILE "log/message.log"

/////////////////////////////////////////////////////////////////////

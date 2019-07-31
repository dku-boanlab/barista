/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "str.h"

/////////////////////////////////////////////////////////////////////

/** \brief The structure of database information */
typedef struct _db_info_t {
    char userid[__CONF_WORD_LEN];
    char userpw[__CONF_WORD_LEN];
    char database[__CONF_WORD_LEN];
} db_info_t;

/////////////////////////////////////////////////////////////////////

int get_database_info(db_info_t *info, char *database);

int reset_table(db_info_t *info, char *table, int all);
int insert_data(db_info_t *info, char *table, char *columns, char *values);
int update_data(db_info_t *info, char *table, char *changes, char *conditions);
int delete_data(db_info_t *info, char *table, char *conditions);
int select_data(db_info_t *info, database_t *db, char *table, char *columns, char *conditions, int all);

int init_database(db_info_t *info, database_t *db);
int destroy_database(database_t *db);
int execute_query(database_t *db, char *query);
query_result_t *get_query_result(database_t *db);
query_row_t fetch_query_row(query_result_t *result);
int release_query_result(query_result_t *result);

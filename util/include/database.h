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

int init_database(database_t *db, char *database);
int destroy_database(database_t *db);

int reset_table(database_t *db, char *table, int all);
int insert_data(database_t *db, char *table, char *columns, char *values);
int insert_long_data(database_t *db, char *table, char *columns, char *values);
int update_data(database_t *db, char *table, char *changes, char *conditions);
int delete_data(database_t *db, char *table, char *conditions);
int select_data(database_t *db, char *table, char *columns, char *conditions, int all);

int execute_query(database_t *db, char *query);
query_result_t *get_query_result(database_t *db);
query_row_t fetch_query_row(query_result_t *result);
int release_query_result(query_result_t *result);

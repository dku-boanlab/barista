/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup database Database Functions
 * \brief Functions to access a database
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "database.h"

/////////////////////////////////////////////////////////////////////

/** \brief The secret file of the database */
#define SECRET_DB_FILE "secret/db_password.txt"

/////////////////////////////////////////////////////////////////////

/** \brief The hostname of the current machine */
static char hostname[__CONF_SHORT_LEN];

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to get the information of a target database
 * \param info Database information
 * \param database Target database
 */
int get_database_info(db_info_t *info, char *database)
{
    if (hostname[0] == '\0')
        gethostname(hostname, __CONF_SHORT_LEN);

    int found = FALSE;

    FILE *fp = fopen(SECRET_DB_FILE, "r");
    if (fp != NULL) {
        char buf[__CONF_STR_LEN] = {0};

        while (fgets(buf, __CONF_STR_LEN-1, fp) != NULL) {
            if (buf[0] == '#') continue;

            sscanf(buf, "%s %s %s", info->userid, info->userpw, info->database);

            if (strcmp(database, info->database) == 0) {
                found = TRUE;
                fclose(fp);
                break;
            }

            memset(info->userid, 0, __CONF_WORD_LEN);
            memset(info->userpw, 0, __CONF_WORD_LEN);
            memset(info->database, 0, __CONF_WORD_LEN);
        }

        if (!found) {
            PRINTF("%s does not exist\n", database);
            return -1;
        }
    } else {
        PRINTF("Failed to read %s\n", SECRET_DB_FILE);
        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to remove all rows in a table
 * \param info Database information
 * \param table Target table
 * \param all The flag to delete all data for other instances too
 */
int reset_table(db_info_t *info, char *table, int all)
{
    database_t db;

    mysql_init(&db);

    if (mysql_real_connect(&db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        mysql_close(&db);
        return -1;
    }

    char query[__CONF_STR_LEN];
    if (all)
        sprintf(query, "delete from %s", table);
    else
        sprintf(query, "delete from %s where INSTANCE = '%s'", table, hostname);

    if (mysql_query(&db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    mysql_close(&db);

    return 0;
}

/**
 * \brief Function to insert data in a table
 * \param info Database information
 * \param table Table
 * \param columns Columns (A, B)
 * \param values Values (A, B)
 */
int insert_data(db_info_t *info, char *table, char *columns, char *values)
{
    database_t db;

    mysql_init(&db);

    if (mysql_real_connect(&db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        mysql_close(&db);
        return -1;
    }

    char query[__CONF_STR_LEN];
    sprintf(query, "insert into %s (%s, INSTANCE) values (%s, '%s')", table, columns, values, hostname);

    if (mysql_query(&db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    mysql_close(&db);

    return 0;
}

/**
 * \brief Function to update data in a table
 * \param info Database information
 * \param table Table
 * \param changes Changes (A=B, C=D)
 * \param conditions Conditions (A=B and C=D)
 */
int update_data(db_info_t *info, char *table, char *changes, char *conditions)
{
    database_t db;

    mysql_init(&db);

    if (mysql_real_connect(&db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        mysql_close(&db);
        return -1;
    }

    char query[__CONF_STR_LEN];
    sprintf(query, "update %s set %s where %s and INSTANCE = '%s'", table, changes, conditions, hostname);

    if (mysql_query(&db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    mysql_close(&db);

    return 0;
}

/**
 * \brief Function to delete data in a table
 * \param info Database information
 * \param table Table
 * \param conditions Conditions (A=B and C=D)
 */
int delete_data(db_info_t *info, char *table, char *conditions)
{
    database_t db;

    mysql_init(&db);

    if (mysql_real_connect(&db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        mysql_close(&db);
        return -1;
    }

    char query[__CONF_STR_LEN];
    sprintf(query, "delete from %s where %s and INSTANCE = '%s'", table, conditions, hostname);

    if (mysql_query(&db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(&db), mysql_sqlstate(&db), mysql_error(&db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    mysql_close(&db);

    return 0;
}

/**
 * \brief Function to get data in a table
 * \param info Database information
 * \param db Database connector
 * \param table Table
 * \param conditions Conditions (A=B and C=D)
 * \param all The flag to look up all data for other instances too
 */
int select_data(db_info_t *info, database_t *db, char *table, char *columns, char *conditions, int all)
{
    mysql_init(db);

    if (mysql_real_connect(db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        mysql_close(db);
        return -1;
    }

    char query[__CONF_STR_LEN];
    if (conditions && all)
        sprintf(query, "select %s from %s where %s", columns, table, conditions);
    else if (conditions && !all)
        sprintf(query, "select %s from %s where %s and INSTANCE = '%s'", columns, table, conditions, hostname);
    else if (!conditions && all)
        sprintf(query, "select %s from %s", columns, table);
    else // !conditions && !all
        sprintf(query, "select %s from %s where INSTANCE = '%s'", columns, table, hostname);

    if (mysql_query(db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize a database connector
 * \param info Database information
 * \param db Database connector
 */
int init_database(db_info_t *info, database_t *db)
{
    mysql_init(db);

    if (mysql_real_connect(db, __DB_HOST, info->userid, info->userpw, info->database, __DB_PORT, (char *)NULL, 0) == NULL) {
        PERROR("mysql_real_connect");
        PRINTF("Error %u (%s): %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        mysql_close(db);
        return -1;
    }

    return 0;
}

/**
 * \brief Function to close a database connector
 * \param db Database connector
 */
int destroy_database(database_t *db)
{
    mysql_close(db);

    return 0;
}

/**
 * \brief Function to execute a query
 * \param db Database connector
 * \param query Query
 */
int execute_query(database_t *db, char *query)
{
    if (mysql_query(db, query) != 0) {
        PERROR("mysql_query");
        PRINTF("Error %u (%s): %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        PRINTF("Query: %s\n", query);
        return -1;
    }

    return 0;
}

/**
 * \brief Function to get a query result
 * \param db Database connector
 * \return Query result
 */
query_result_t *get_query_result(database_t *db)
{
    query_result_t *result = mysql_store_result(db);

    if (result == NULL)
        return NULL;

    return result;
}

/**
 * \brief Function to get a row in a query result
 * \param result Query result
 * \return A row of the query result
 */
query_row_t fetch_query_row(query_result_t *result)
{
    return mysql_fetch_row(result);
}

/**
 * \brief Function to release a query result
 * \param result Query result
 */
int release_query_result(query_result_t *result)
{
    if (!result)
        return -1;

    mysql_free_result(result);

    return 0;
}

/**
 * @}
 *
 * @}
 */

/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup resource_mgmt Resource Management
 * \brief (Management) resource management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "resource_mgmt.h"

/** \brief Resource management ID */
#define RSM_ID 683559233

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to get CPU and memory usages
 * \param rs Structure to store CPU and memory usages
 */
static int monitor_resources(resource_t *rs)
{
    resource_t rs_temp = {0.0, 0.0};

    FILE *pp = popen(cmd, "r");
    if (pp != NULL) {
        char temp[__CONF_WORD_LEN] = {0};

        if (fgets(temp, __CONF_WORD_LEN-1, pp) != NULL) {
            sscanf(temp, "%lf %lf", &rs_temp.cpu, &rs_temp.mem);
        }

        pclose(pp);
    }

    rs->cpu += rs_temp.cpu;
    rs->mem += rs_temp.mem;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int resource_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(RSM_ID, "Init - Resource management");

    int num_procs = get_nprocs();

    if (get_database_info(&resource_mgmt_info, "barista_mgmt")) {
        LOG_ERROR(RSM_ID, "Failed to get the information of a resource_mgmt database");
        return -1;
    }

    reset_table(&resource_mgmt_info, "resource_mgmt", FALSE);

    memset(&resource, 0, sizeof(resource_t));

    activate();

    while (*activated) {
        int i;
        for (i=0; i<__RESOURCE_MGMT_MONITOR_TIME; i++) {
            if (*activated == FALSE) break;
            else waitsec(1, 0);
            monitor_resources(&resource);
        }

        resource_t rs;

        rs.cpu = resource.cpu / num_procs;
        rs.mem = resource.mem;

        rs.cpu /= __RESOURCE_MGMT_MONITOR_TIME;
        rs.mem /= __RESOURCE_MGMT_MONITOR_TIME;

        char values[__CONF_STR_LEN];
        sprintf(values, "%lf, %lf", rs.cpu, rs.mem);

        if (insert_data(&resource_mgmt_info, "resource_mgmt", "CPU, MEM", values)) {
            LOG_ERROR(RSM_ID, "insert_data() failed");
        }

        ev_rs_update_usage(RSM_ID, &rs);

        resource.cpu = 0.0;
        resource.mem = 0.0;
    }

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int resource_mgmt_cleanup(int *activated)
{
    LOG_INFO(RSM_ID, "Clean up - Resource management");

    deactivate();

    return 0;
}

/**
 * \brief Function to summarize the resource history for the last n seconds
 * \param cli The pointer of the Barista CLI
 * \param seconds Time range to query
 */
static int resource_stat_summary(cli_t *cli, char *seconds)
{
    database_t resource_mgmt_db;

    int sec = atoi(seconds) / __RESOURCE_MGMT_MONITOR_TIME;

    int cnt = 0;
    resource_t rs = {0.0, 0.0};

    char query[__CONF_STR_LEN];
    sprintf(query, "select CPU, MEM from resource_mgmt order by id desc limit %d", sec);

    if (init_database(&resource_mgmt_info, &resource_mgmt_db)) {
        cli_print(cli, "Failed to connect a resource_mgmt database");
        destroy_database(&resource_mgmt_db);
        return -1;
    }

    if (execute_query(&resource_mgmt_db, query)) {
        cli_print(cli, "Failed to read resource usages");
        destroy_database(&resource_mgmt_db);
        return -1;
    }

    query_result_t *result = get_query_result(&resource_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        rs.cpu += atof(row[0]);
        rs.mem += atof(row[1]);

        cnt++;
    }

    release_query_result(result);

    destroy_database(&resource_mgmt_db);

    rs.cpu /= cnt;
    rs.mem /= cnt;

    cli_print(cli, "< Resource Usages for the last %d seconds >", sec * __RESOURCE_MGMT_MONITOR_TIME);
    cli_print(cli, "  CPU: %.2f %%, MEM: %.2f %%", rs.cpu, rs.mem);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int resource_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "stat") == 0 && args[1] != NULL && args[2] == NULL) {
        resource_stat_summary(cli, args[1]);
        return 0;
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  resource_mgmt stat [N second(s)]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int resource_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

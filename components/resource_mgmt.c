/*
 * Copyright 2015-2018 NSSLab, KAIST
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

/** \brief The pointer to record resource usages in a history array */
uint32_t rs_history_ptr;

/** \brief The history array of resource usages */
resource_t *rs_history;

/** \brief The number of CPU cores */
int num_proc;

/////////////////////////////////////////////////////////////////////

/** \brief The default resource log file */
char resource_file[__CONF_WORD_LEN] = DEFAULT_RESOURCE_FILE;

/////////////////////////////////////////////////////////////////////

static int monitor_resources(resource_t *rs)
{
    char cmd[__CONF_WORD_LEN] = {0};
    sprintf(cmd, "ps -p $(pgrep -x barista) -o %%cpu,%%mem 2> /dev/null | tail -n 1");

    FILE *pp = popen(cmd, "r");
    if (pp != NULL) {
        char temp[__CONF_WORD_LEN] = {0};

        if (fgets(temp, __CONF_WORD_LEN-1, pp) != NULL) {
            sscanf(temp, "%lf %lf", &rs->cpu, &rs->mem);
        }

        pclose(pp);
    }

    rs->cpu /= num_proc; // normalization (under 100%)

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

    rs_history_ptr = 0;

    rs_history = (resource_t *)MALLOC(sizeof(resource_t) * RESOURCE_MGMT_HISTORY);
    if (rs_history == NULL) {
        PERROR("malloc");
        return -1;
    }

    memset(rs_history, 0, sizeof(resource_t) * RESOURCE_MGMT_HISTORY);

    num_proc = get_nprocs();

    LOG_INFO(RSM_ID, "Resource usage output: %s", resource_file);
    LOG_INFO(RSM_ID, "Resource monitoring period: %d sec", RESOURCE_MGMT_MONITOR_TIME);

    activate();

    while (*activated) {
        resource_t rs = {0};

        time_t timer;
        struct tm *tm_info;
        char now[__CONF_WORD_LEN] = {0};
        char buf[__CONF_WORD_LEN] = {0};

        time(&timer);
        tm_info = localtime(&timer);
        strftime(now, __CONF_WORD_LEN-1, "%Y:%m:%d %H:%M:%S", tm_info);

        monitor_resources(&rs);

        sprintf(buf, "%s - CPU %6.2lf MEM %6.2lf", now, rs.cpu, rs.mem);

        memmove(&rs_history[rs_history_ptr++ % RESOURCE_MGMT_HISTORY], &rs, sizeof(resource_t));

        FILE *fp = fopen(resource_file, "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", buf);
            fclose(fp);
        }

        ev_rs_update_usage(RSM_ID, &rs);

        waitsec(RESOURCE_MGMT_MONITOR_TIME, 0);
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

    FREE(rs_history);

    return 0;
}

/**
 * \brief Function to summarize the resource history for the last n seconds
 * \param cli The CLI pointer
 * \param seconds Time period to query
 */
static int resource_stat_summary(cli_t *cli, char *seconds)
{
    int sec = atoi(seconds);
    if (sec > RESOURCE_MGMT_HISTORY) sec = RESOURCE_MGMT_HISTORY;
    else if (sec > rs_history_ptr) sec = rs_history_ptr;

    int ptr = rs_history_ptr - sec;
    if (ptr < 0) ptr = RESOURCE_MGMT_HISTORY + ptr;

    resource_t rs = {0.0};

    int i;
    for (i=0; i<sec; i++) {
        rs.cpu += rs_history[ptr % RESOURCE_MGMT_HISTORY].cpu;
        rs.mem += rs_history[ptr % RESOURCE_MGMT_HISTORY].mem;

        ptr++;
    }

    rs.cpu /= sec;
    rs.mem /= sec;

    cli_print(cli, "<Resource Statistics for %d seconds>", sec);
    cli_print(cli, "  CPU: %.2f %%, MEM: %.2f %%", rs.cpu, rs.mem);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int resource_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "stat") == 0 && args[1] != NULL) {
        resource_stat_summary(cli, args[1]);
        return 0;
    }

    cli_print(cli, "<Available Commands>");
    cli_print(cli, "  resource_mgmt stat [n secs]");

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

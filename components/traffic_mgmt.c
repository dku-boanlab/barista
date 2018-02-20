/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup traffic_mgmt Control Traffic Management
 * \brief (Management) control traffic management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "traffic_mgmt.h"

/** \brief Control traffic management ID */
#define TRAFFIC_MGMT_ID 2206472807

/////////////////////////////////////////////////////////////////////

/** \brief Traffic statistics */
traffic_t traffic;

/** \brief The pointer to record traffic statistics in a history array */
uint32_t tr_history_ptr;

/** \brief The history array of traffic statistics */
traffic_t *tr_history;

/** \brief The lock for statistics management */
pthread_spinlock_t lock;

/** \brief The default traffic log file */
char traffic_file[__CONF_WORD_LEN] = DEFAULT_TRAFFIC_FILE;

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int traffic_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(TRAFFIC_MGMT_ID, "Init - Control traffic management");

    memset(&traffic, 0, sizeof(traffic_t));

    tr_history_ptr = 0;

    tr_history = (traffic_t *)MALLOC(sizeof(traffic_t) * TRAFFIC_MGMT_HISTORY);
    if (tr_history == NULL) {
        PERROR("malloc");
        return -1;
    }

    memset(tr_history, 0, sizeof(traffic_t) * TRAFFIC_MGMT_HISTORY);

    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

    LOG_INFO(TRAFFIC_MGMT_ID, "Traffic usage output: %s", traffic_file);
    LOG_INFO(TRAFFIC_MGMT_ID, "Traffic monitoring period: %d sec", TRAFFIC_MGMT_MONITOR_TIME);

    activate();

    while (*activated) {
        time_t timer;
        struct tm *tm_info;
        char now[__CONF_WORD_LEN] = {0}, buf[__CONF_STR_LEN] = {0};

        time(&timer);
        tm_info = localtime(&timer);
        strftime(now, __CONF_WORD_LEN-1, "%Y:%m:%d %H:%M:%S", tm_info);

        traffic_t tr;

        pthread_spin_lock(&lock);

        memmove(&tr_history[tr_history_ptr++ % TRAFFIC_MGMT_HISTORY], &traffic, sizeof(traffic_t));
        memmove(&tr, &traffic, sizeof(traffic_t));

        memset(&traffic, 0, sizeof(traffic_t));

        pthread_spin_unlock(&lock);

        snprintf(buf, __CONF_STR_LEN-1, 
                "%s - In %lu msgs %lu bytes / Out %lu msgs %lu bytes",
                now, tr.in_pkt_cnt, tr.in_byte_cnt, tr.out_pkt_cnt, tr.out_byte_cnt);

        FILE *fp = fopen(traffic_file, "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", buf);
            fclose(fp);
            fp = NULL;
        }

        ev_tr_update_stats(TRAFFIC_MGMT_ID, &tr);

        waitsec(TRAFFIC_MGMT_MONITOR_TIME, 0);
    }

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int traffic_mgmt_cleanup(int *activated)
{
    LOG_INFO(TRAFFIC_MGMT_ID, "Clean up - Control traffic management");

    deactivate();

    pthread_spin_destroy(&lock);

    FREE(tr_history);

    return 0;
}

/**
 * \brief Function to summarize the traffic history for the last n seconds
 * \param cli The CLI pointer
 * \param seconds Time period to query
 */
static int traffic_stat_summary(cli_t *cli, char *seconds)
{
    int sec = atoi(seconds);
    if (sec > TRAFFIC_MGMT_HISTORY) sec = TRAFFIC_MGMT_HISTORY;
    else if (tr_history_ptr < TRAFFIC_MGMT_HISTORY && tr_history_ptr < sec) sec = tr_history_ptr;

    int ptr = tr_history_ptr - sec;
    if (ptr < 0) ptr = TRAFFIC_MGMT_HISTORY + ptr;

    traffic_t tr = {0};

    int i;
    for (i=0; i<sec; i++) {
        tr.in_pkt_cnt += tr_history[ptr % TRAFFIC_MGMT_HISTORY].in_pkt_cnt;
        tr.in_byte_cnt += tr_history[ptr % TRAFFIC_MGMT_HISTORY].in_byte_cnt;

        tr.out_pkt_cnt += tr_history[ptr % TRAFFIC_MGMT_HISTORY].out_pkt_cnt;
        tr.out_byte_cnt += tr_history[ptr % TRAFFIC_MGMT_HISTORY].out_byte_cnt;

        ptr++;
    }

    double in_pkt_cnt = tr.in_pkt_cnt * 1.0 / sec;
    double in_byte_cnt = tr.in_byte_cnt * 1.0 / sec;

    double out_pkt_cnt = tr.out_pkt_cnt * 1.0 / sec;
    double out_byte_cnt = tr.out_byte_cnt * 1.0 / sec;

    cli_print(cli, "<Traffic Statistics for %d seconds>", sec);
    cli_print(cli, "  - Cumulative values");
    cli_print(cli, "    Inbound packet count : %lu packets", tr.in_pkt_cnt);
    cli_print(cli, "    Inbound byte count   : %lu bytes", tr.in_byte_cnt);
    cli_print(cli, "    Outbound packet count: %lu packets", tr.out_pkt_cnt);
    cli_print(cli, "    Outbound byte count  : %lu bytes", tr.out_byte_cnt);
    cli_print(cli, "  - Average values");
    cli_print(cli, "    Inbound packet count : %.2f packets", in_pkt_cnt);
    cli_print(cli, "    Inbound byte count   : %.2f bytes", in_byte_cnt);
    cli_print(cli, "    Outbound packet count: %.2f packets", out_pkt_cnt);
    cli_print(cli, "    Outbound byte count  : %.2f bytes", out_byte_cnt);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int traffic_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "stat") == 0 && args[1] != NULL && args[2] == NULL) {
        traffic_stat_summary(cli, args[1]);
        return 0;
    }

    PRINTF("<Available Commands>\n");
    PRINTF("  traffic_mgmt stat [n secs]\n");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int traffic_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_IN:
        PRINT_EV("EV_OFP_MSG_IN\n");
        {
            pthread_spin_lock(&lock);

            traffic.in_pkt_cnt++;
            traffic.in_byte_cnt += ev->msg->length;

            pthread_spin_unlock(&lock);
        }
        break;
    case EV_OFP_MSG_OUT:
        PRINT_EV("EV_OFP_MSG_OUT\n");
        {
            pthread_spin_lock(&lock);

            traffic.out_pkt_cnt++;
            traffic.out_byte_cnt += ev->msg->length;

            pthread_spin_unlock(&lock);
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */

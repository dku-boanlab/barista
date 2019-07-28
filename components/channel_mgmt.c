/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup channel_mgmt Control Channel Management
 * \brief (Management) control channel management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "channel_mgmt.h"

/** \brief Control channel management ID */
#define CHANNEL_MGMT_ID 476604139

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int channel_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CHANNEL_MGMT_ID, "Init - Control channel management");

    if (init_database(&channel_mgmt_db, "barista_mgmt")) {
        LOG_ERROR(CHANNEL_MGMT_ID, "Failed to connect a channel_mgmt database");
        return -1;
    } else {
        LOG_INFO(CHANNEL_MGMT_ID, "Connected to a channel_mgmt database");
    }

    reset_table(&channel_mgmt_db, "channel_mgmt", FALSE);

    memset(&traffic, 0, sizeof(traffic_t));

    pthread_spin_init(&tr_lock, PTHREAD_PROCESS_PRIVATE);

    activate();

    while (*activated) {
        traffic_t tr;

        pthread_spin_lock(&tr_lock);

        memmove(&tr, &traffic, sizeof(traffic_t));
        memset(&traffic, 0, sizeof(traffic_t));

        pthread_spin_unlock(&tr_lock);

        char values[__CONF_STR_LEN];
        sprintf(values, "%lu, %lu, %lu, %lu", tr.in_pkt_cnt, tr.in_byte_cnt, tr.out_pkt_cnt, tr.out_byte_cnt);

        if (insert_data(&channel_mgmt_db, "channel_mgmt", "IN_PKT_CNT, IN_BYTE_CNT, OUT_PKT_CNT, OUT_BYTE_CNT", values)) {
            LOG_ERROR(CHANNEL_MGMT_ID, "insert_data() failed");
        }

        ev_tr_update_stats(CHANNEL_MGMT_ID, &tr);

        int i;
        for (i=0; i<__CHANNEL_MGMT_MONITOR_TIME; i++) {
            if (*activated == FALSE) break;
            else waitsec(1, 0);
        }
    }

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int channel_mgmt_cleanup(int *activated)
{
    LOG_INFO(CHANNEL_MGMT_ID, "Clean up - Control channel management");

    deactivate();

    pthread_spin_destroy(&tr_lock);

    if (destroy_database(&channel_mgmt_db)) {
        LOG_INFO(CHANNEL_MGMT_ID, "Failed to disconnect a channel_mgmt database");
        return -1;
    } else {
        LOG_INFO(CHANNEL_MGMT_ID, "Disconnected from a channel_mgmt database");
    }

    return 0;
}

/**
 * \brief Function to summarize the traffic history for the last n seconds
 * \param cli The pointer of the Barista CLI
 * \param seconds The time range to query
 */
static int traffic_stat_summary(cli_t *cli, char *seconds)
{
    int sec = atoi(seconds) / __CHANNEL_MGMT_MONITOR_TIME;

    int cnt = 0;
    traffic_t tr = {0};

    char query[__CONF_STR_LEN];
    sprintf(query, "select IN_PKT_CNT, IN_BYTE_CNT, OUT_PKT_CNT, OUT_BYTE_CNT from channel_mgmt order by id desc limit %d", sec);

    if (execute_query(&channel_mgmt_db, query)) {
        cli_print(cli, "Failed to read the statistics of the control channel");
        return -1;
    }

    query_result_t *result = get_query_result(&channel_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        tr.in_pkt_cnt += strtoull(row[0], NULL, 0);
        tr.in_byte_cnt += strtoull(row[1], NULL, 0);
        tr.out_pkt_cnt += strtoull(row[2], NULL, 0);
        tr.out_byte_cnt += strtoull(row[3], NULL, 0);

        cnt++;
    }

    release_query_result(result);
    
    double in_pkt_cnt = tr.in_pkt_cnt * 1.0 / sec;
    double in_byte_cnt = tr.in_byte_cnt * 1.0 / sec;

    double out_pkt_cnt = tr.out_pkt_cnt * 1.0 / sec;
    double out_byte_cnt = tr.out_byte_cnt * 1.0 / sec;

    cli_print(cli, "< Traffic Statistics for %d seconds >", sec * __CHANNEL_MGMT_MONITOR_TIME);
    cli_print(cli, "  - Cumulative values");
    cli_print(cli, "    Inbound packet count : %lu packets", tr.in_pkt_cnt);
    cli_print(cli, "    Inbound byte count   : %lu bytes", tr.in_byte_cnt);
    cli_print(cli, "    Outbound packet count: %lu packets", tr.out_pkt_cnt);
    cli_print(cli, "    Outbound byte count  : %lu bytes", tr.out_byte_cnt);
    cli_print(cli, "  - Average values (per second)");
    cli_print(cli, "    Inbound packet count : %.2f packets", in_pkt_cnt);
    cli_print(cli, "    Inbound byte count   : %.2f bytes", in_byte_cnt);
    cli_print(cli, "    Outbound packet count: %.2f packets", out_pkt_cnt);
    cli_print(cli, "    Outbound byte count  : %.2f bytes", out_byte_cnt);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int channel_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "stat") == 0 && args[1] != NULL && args[2] == NULL) {
        traffic_stat_summary(cli, args[1]);
        return 0;
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  channel_mgmt stat [N minute(s)]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int channel_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_IN:
        PRINT_EV("EV_OFP_MSG_IN\n");
        {
            const msg_t *msg = ev->msg;

            pthread_spin_lock(&tr_lock);

            traffic.in_pkt_cnt++;
            traffic.in_byte_cnt += msg->length;

            pthread_spin_unlock(&tr_lock);
        }
        break;
    case EV_OFP_MSG_OUT:
        PRINT_EV("EV_OFP_MSG_OUT\n");
        {
            const msg_t *msg = ev->msg;

            pthread_spin_lock(&tr_lock);

            traffic.out_pkt_cnt++;
            traffic.out_byte_cnt += msg->length;

            pthread_spin_unlock(&tr_lock);
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

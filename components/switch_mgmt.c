/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup switch_mgmt Switch Management
 * \brief (Management) switch management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "switch_mgmt.h"

/** \brief Switch management ID */
#define SWITCH_MGMT_ID 893048714

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int switch_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(SWITCH_MGMT_ID, "Init - Switch management");

    if (init_database(&switch_mgmt_db, "barista_mgmt")) {
        LOG_ERROR(SWITCH_MGMT_ID, "Failed to connect to a switch_mgmt database");
        return -1;
    } else {
        LOG_INFO(SWITCH_MGMT_ID, "Connected to a switch_mgmt database");
    }

    reset_table(&switch_mgmt_db, "switch_mgmt", FALSE);

    switch_table = (switch_t *)CALLOC(__MAX_NUM_SWITCHES, sizeof(switch_t));
    if (switch_table == NULL) {
        LOG_ERROR(SWITCH_MGMT_ID, "calloc() failed");
        return -1;
    }

    pthread_rwlock_init(&sw_lock, NULL);

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int switch_mgmt_cleanup(int *activated)
{
    LOG_INFO(SWITCH_MGMT_ID, "Clean up - Switch management");

    deactivate();

    pthread_rwlock_destroy(&sw_lock);
    FREE(switch_table);

    if (destroy_database(&switch_mgmt_db)) {
        LOG_ERROR(SWITCH_MGMT_ID, "Failed to disconnect a switch_mgmt database");
        return -1;
    } else {
        LOG_INFO(SWITCH_MGMT_ID, "Disconnected from a switch_mgmt database");
    }

    return 0;
}

/**
 * \brief Function to print all switches
 * \param cli The pointer of the Barista CLI
 */
static int switch_listup(cli_t *cli)
{
    int cnt = 0;

    cli_print(cli, "< Switch List >");

    if (select_data(&switch_mgmt_db, "switch_mgmt", 
        "DPID, MFR_DESC, HW_DESC, SW_DESC, SERIAL_NUM, DP_DESC, PKT_COUNT, BYTE_COUNT, FLOW_COUNT", NULL, TRUE)) {
        cli_print(cli, "Failed to read the list of switches");
        return -1;
    }

    query_result_t *result = get_query_result(&switch_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        cli_print(cli, "  Switch #%d", ++cnt);
        cli_print(cli, "    Datapath ID: %s", row[0]);
        cli_print(cli, "    Manufacturer: %s", row[1]);
        cli_print(cli, "    Hardware: %s", row[2]);
        cli_print(cli, "    Software: %s", row[3]);
        cli_print(cli, "    Serial number: %s", row[4]);
        cli_print(cli, "    Datapath: %s", row[5]);
        cli_print(cli, "    Statistics:");
        cli_print(cli, "      - # of packets: %s", row[6]);
        cli_print(cli, "      - # of bytes: %s", row[7]);
        cli_print(cli, "      - # of flows: %s", row[8]);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No connected switch");

    return 0;
}

/**
 * \brief Function to print a switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int switch_showup(cli_t *cli, char *dpid_str)
{
    int cnt = 0;
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "< Switch Information >");

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "DPID = %lu", dpid);

    if (select_data(&switch_mgmt_db, "switch_mgmt", 
        "DPID, MFR_DESC, HW_DESC, SW_DESC, SERIAL_NUM, DP_DESC, PKT_COUNT, BYTE_COUNT, FLOW_COUNT", conditions, TRUE)) {
        cli_print(cli, "Failed to read the list of switches");
        return -1;
    }

    query_result_t *result = get_query_result(&switch_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        cli_print(cli, "  Switch #%d", ++cnt);
        cli_print(cli, "    Datapath ID: %s", row[0]);
        cli_print(cli, "    Manufacturer: %s", row[1]);
        cli_print(cli, "    Hardware: %s", row[2]);
        cli_print(cli, "    Software: %s", row[3]);
        cli_print(cli, "    Serial number: %s", row[4]);
        cli_print(cli, "    Datapath: %s", row[5]);
        cli_print(cli, "    Statistics:");
        cli_print(cli, "      - # of packets: %s", row[6]);
        cli_print(cli, "      - # of bytes: %s", row[7]);
        cli_print(cli, "      - # of flows: %s", row[8]);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No switch whose datapath ID is %lu", dpid);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int switch_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0 && args[1] != NULL && strcmp(args[1], "switches") == 0 && args[2] == NULL) {
        switch_listup(cli);
        return 0;
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0 && args[1] != NULL && args[2] == NULL) {
        switch_showup(cli, args[1]);
        return 0;
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  switch_mgmt list switches");
    cli_print(cli, "  switch_mgmt show [datapath ID]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int switch_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_SW_NEW_CONN:
        PRINT_EV("EV_SW_NEW_CONN\n");
        {
            const switch_t *sw = ev->sw;

            LOG_DEBUG(SWITCH_MGMT_ID, "Accepted (FD=%d)", sw->conn.fd);
        }
        break;
    case EV_SW_ESTABLISHED_CONN:
        PRINT_EV("EV_SW_ESTABLISHED_CONN\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_wrlock(&sw_lock);

            int idx = sw->dpid % __MAX_NUM_SWITCHES;
            do {
                switch_t *curr = &switch_table[idx];

                if (curr->dpid == sw->dpid) {
                    LOG_WARN(SWITCH_MGMT_ID, "%lu already exists", sw->dpid);
                    break;
                } else if (curr->dpid == 0) {
                    curr->dpid = sw->dpid;
                    curr->conn.fd = sw->conn.fd;
                    curr->conn.xid = sw->conn.xid;

                    pthread_rwlock_unlock(&sw_lock);

                    switch_t out = {0};
                    out.dpid = sw->dpid;
                    ev_sw_connected(SWITCH_MGMT_ID, &out);

                    char values[__CONF_STR_LEN];
                    sprintf(values, "%lu, 0, 0, 0", sw->dpid);

                    if (insert_data(&switch_mgmt_db, "switch_mgmt", "DPID, PKT_COUNT, BYTE_COUNT, FLOW_COUNT", values)) {
                        LOG_ERROR(SWITCH_MGMT_ID, "insert_data() failed");
                    }

                    LOG_INFO(SWITCH_MGMT_ID, "Connected (FD=%d, DPID=%lu)", sw->conn.fd, sw->dpid);

                    break;
                }

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != sw->dpid % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_SW_EXPIRED_CONN:
        PRINT_EV("EV_SW_EXPIRED_CONN\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_wrlock(&sw_lock);

            int i, deleted = FALSE;
            for (i=0; i<__MAX_NUM_SWITCHES; i++) {
                if (switch_table[i].conn.fd == sw->conn.fd) {
                    switch_t out = {0};
                    out.dpid = switch_table[i].dpid;
                    ev_sw_disconnected(SWITCH_MGMT_ID, &out);

                    char conditions[__CONF_STR_LEN];
                    sprintf(conditions, "DPID = %lu", switch_table[i].dpid);

                    if (delete_data(&switch_mgmt_db, "switch_mgmt", conditions)) {
                        LOG_ERROR(SWITCH_MGMT_ID, "delete_data() failed");
                    }

                    memset(&switch_table[i], 0, sizeof(switch_t));

                    LOG_INFO(SWITCH_MGMT_ID, "Disconnected (FD=%d, DPID=%lu)", sw->conn.fd, out.dpid);

                    deleted = TRUE;

                    break;
                }
            }

            pthread_rwlock_unlock(&sw_lock);

            if (!deleted)
                LOG_DEBUG(SWITCH_MGMT_ID, "Closed (FD=%d)", sw->conn.fd);
        }
        break;
    case EV_SW_CONNECTED:
        PRINT_EV("EV_SW_CONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == FALSE) break;

            LOG_INFO(SWITCH_MGMT_ID, "Connected (DPID=%lu)", sw->dpid);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == FALSE) break;

            LOG_INFO(SWITCH_MGMT_ID, "Disconnected (DPID=%lu)", sw->dpid);
        }
        break;
    case EV_SW_UPDATE_DESC:
        PRINT_EV("EV_SW_UPDATE_DESC\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_wrlock(&sw_lock);

            int idx = sw->dpid % __MAX_NUM_SWITCHES;
            do {
                switch_t *curr = &switch_table[idx];

                if (curr->dpid == sw->dpid) {
                    strncpy(curr->desc.mfr_desc, sw->desc.mfr_desc, 256);
                    strncpy(curr->desc.hw_desc, sw->desc.hw_desc, 256);
                    strncpy(curr->desc.sw_desc, sw->desc.sw_desc, 256);
                    strncpy(curr->desc.serial_num, sw->desc.serial_num, 32);
                    strncpy(curr->desc.dp_desc, sw->desc.dp_desc, 256);

                    char changes[__CONF_STR_LEN];
                    sprintf(changes, "MFR_DESC = '%s', HW_DESC = '%s', SW_DESC = '%s', SERIAL_NUM = '%s', DP_DESC = '%s'",
                            sw->desc.mfr_desc, sw->desc.hw_desc, sw->desc.sw_desc, sw->desc.serial_num, sw->desc.dp_desc);

                    char conditions[__CONF_STR_LEN];
                    sprintf(conditions, "DPID = %lu", sw->dpid);

                    if (update_data(&switch_mgmt_db, "switch_mgmt", changes, conditions)) {
                        LOG_ERROR(SWITCH_MGMT_ID, "update_data() failed");
                    }

                    break;
                }

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != sw->dpid % __MAX_NUM_SWITCHES);

            pthread_rwlock_unlock(&sw_lock);
        }
        break;
    case EV_DP_AGGREGATE_STATS:
        PRINT_EV("EV_DP_AGGREGATE_STATS\n");
        {
            const flow_t *flow = ev->flow;

            pthread_rwlock_wrlock(&sw_lock);

            int idx = flow->dpid % __MAX_NUM_SWITCHES;
            do {
                switch_t *curr = &switch_table[idx];

                if (curr->dpid == flow->dpid) {
                    curr->stat.pkt_count = flow->stat.pkt_count;
                    curr->stat.byte_count = flow->stat.byte_count;
                    curr->stat.flow_count = flow->stat.flow_count;

                    char changes[__CONF_STR_LEN];
                    sprintf(changes, "PKT_COUNT = %lu, BYTE_COUNT = %lu, FLOW_COUNT = %u",
                            curr->stat.pkt_count, curr->stat.byte_count, curr->stat.flow_count);

                    char conditions[__CONF_STR_LEN];
                    sprintf(conditions, "DPID = %lu", flow->dpid);

                    if (update_data(&switch_mgmt_db, "switch_mgmt", changes, conditions)) {
                        LOG_ERROR(SWITCH_MGMT_ID, "update_data() failed");
                    }

                    break;
                }

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != flow->dpid % __MAX_NUM_SWITCHES);

            pthread_rwlock_unlock(&sw_lock);
        }
        break;
    case EV_SW_GET_DPID:
        PRINT_EV("EV_SW_GET_DPID\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&sw_lock);

            int i;
            for (i=0; i<__MAX_NUM_SWITCHES; i++) {
                if (switch_table[i].conn.fd == sw->conn.fd) {
                    sw->dpid = switch_table[i].dpid;
                    break;
                }
            }

            pthread_rwlock_unlock(&sw_lock);
        }
        break;
    case EV_SW_GET_FD:
        PRINT_EV("EV_SW_GET_FD\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&sw_lock);

            int idx = sw->dpid % __MAX_NUM_SWITCHES;
            do {
                switch_t *curr = &switch_table[idx];

                if (curr->dpid == sw->dpid) {
                    sw->conn.fd = curr->conn.fd;
                    break;
                }

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != sw->dpid % __MAX_NUM_SWITCHES);

            pthread_rwlock_unlock(&sw_lock);
        }
        break;
    case EV_SW_GET_XID:
        PRINT_EV("EV_SW_GET_XID\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&sw_lock);

            if (sw->dpid) {
                int idx = sw->dpid % __MAX_NUM_SWITCHES;
                do {
                    switch_t *curr = &switch_table[idx];

                    if (curr->dpid == sw->dpid) {
                        sw->conn.xid = curr->conn.xid++;
                        break;
                    }

                    idx = (idx + 1) % __MAX_NUM_SWITCHES;
                } while (idx != sw->dpid % __MAX_NUM_SWITCHES);
            } else if (sw->conn.fd) {
                int i;
                for (i=0; i<__MAX_NUM_SWITCHES; i++) {
                    if (switch_table[i].conn.fd == sw->conn.fd) {
                        sw->conn.xid = switch_table[i].conn.xid++;
                        break;
                    }
                }
            }

            pthread_rwlock_unlock(&sw_lock);
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

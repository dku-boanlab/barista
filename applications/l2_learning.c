/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup l2_learning L2 Learning
 * \brief (Network) L2 learning
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "l2_learning.h"

/** \brief L2 learning ID */
#define L2_LEARNING_ID 153576300

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to conduct an output action into the data plane
 * \param pktin Pktin message
 * \param port Port
 */
static int send_packet(const pktin_t *pktin, uint16_t port)
{
    pktout_t out = {0};

    PKTOUT_INIT(out, pktin);

    out.num_actions = 1;
    out.action[0].type = ACTION_OUTPUT;
    out.action[0].port = port;

    av_dp_send_packet(L2_LEARNING_ID, &out);

    return 0;
}

/**
 * \brief Function to insert a flow rule into the data plane
 * \param pktin Pktin message
 * \param port Port
 */
static int insert_flow(const pktin_t *pktin, uint16_t port)
{
    flow_t out = {0};

    FLOW_INIT(out, pktin);

    out.meta.idle_timeout = DEFAULT_IDLE_TIMEOUT;
    out.meta.hard_timeout = DEFAULT_HARD_TIMEOUT;
    out.meta.priority = DEFAULT_PRIORITY;

    out.num_actions = 1;
    out.action[0].type = ACTION_OUTPUT;
    out.action[0].port = port;

    av_dp_insert_flow(L2_LEARNING_ID, &out);

    return 0;
}

/////////////////////////////////////////////////////////////////////

int get_mac_entry(mac_entry_t *entry)
{
    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "DPID = %lu and MAC = %lu", entry->dpid, entry->mac);

    if (select_data(&l2_learning_db, "forwarding_table", "PORT, IP", conditions, TRUE)) return -1;

    query_result_t *result = get_query_result(&l2_learning_db);
    query_row_t row;

    if ((row = fetch_query_row(result)) != NULL) {
        entry->port = strtoul(row[0], NULL, 0);
        entry->ip = strtoul(row[1], NULL, 0);

        release_query_result(result);

        return 0;
    }

    release_query_result(result);

    return -1;
}

int insert_mac_entry(mac_entry_t *entry)
{
    char values[__CONF_STR_LEN];
    sprintf(values, "%lu, %u, %lu, %u", entry->dpid, entry->port, entry->mac, entry->ip);

    if (insert_data(&l2_learning_db, "forwarding_table", "DPID, PORT, MAC, IP", values)) return -1;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle pktin messages
 * \param pktin Pktin message
 */
static int l2_learning(const pktin_t *pktin)
{
    mac_key_t mkey;

    // source check

    mkey.dpid = pktin->dpid;
    mkey.mac = mac2int(pktin->pkt_info.src_mac);

    uint32_t key = hash_func((uint32_t *)&mkey, 4) % NUM_MAC_ENTRIES;

    if (mac_cache[key].dpid != mkey.dpid && mac_cache[key].mac != mkey.mac) { // not in cache
        mac_entry_t src;

        src.dpid = mkey.dpid;
        src.mac = mkey.mac;

        if (get_mac_entry(&src)) { // not found in database
            mac_cache[key].dpid = mkey.dpid;
            mac_cache[key].port = pktin->port;
            mac_cache[key].ip = pktin->pkt_info.src_ip;
            mac_cache[key].mac = mkey.mac;

            insert_mac_entry(&mac_cache[key]);
        } else { // found in database
            mac_cache[key].dpid = mkey.dpid;
            mac_cache[key].port = src.port;
            mac_cache[key].ip = src.ip;
            mac_cache[key].mac = mkey.mac;
        }
    }

    // destination check

    mkey.mac = mac2int(pktin->pkt_info.dst_mac);

    if (mkey.mac == __BROADCAST_MAC) { // broadcast
        send_packet(pktin, PORT_FLOOD);
        return 0;
    }

    key = hash_func((uint32_t *)&mkey, 4) % NUM_MAC_ENTRIES;

    if (mac_cache[key].dpid != mkey.dpid && mac_cache[key].mac != mkey.mac) { // not in cache
        mac_entry_t dest;

        dest.dpid = mkey.dpid;
        dest.mac = mkey.mac;

        if (get_mac_entry(&dest)) { // not found in database
            send_packet(pktin, PORT_FLOOD);
            return 0;
        } else {
            mac_cache[key].dpid = mkey.dpid;
            mac_cache[key].port = dest.port;
            mac_cache[key].ip = dest.ip;
            mac_cache[key].mac = mkey.mac;
        }
    }

    // forwarding

    if (pktin->pkt_info.proto & PROTO_IPV4) { // IPv4
        insert_flow(pktin, mac_cache[key].port);
    } else { // Otherwise
        send_packet(pktin, mac_cache[key].port);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int l2_learning_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(L2_LEARNING_ID, "Init - L2 learning");

    if (init_database(&l2_learning_db, "l2_learning")) {
        ALOG_ERROR(L2_LEARNING_ID, "Failed to connect a l2_learning database");
        return -1;
    } else {
        ALOG_INFO(L2_LEARNING_ID, "Connected to a l2_learning database");
    }

    reset_table(&l2_learning_db, "forwarding_table", FALSE);

    mac_cache = (mac_entry_t *)CALLOC(NUM_MAC_ENTRIES, sizeof(mac_entry_t));
    if (mac_cache == NULL) {
        ALOG_ERROR(L2_LEARNING_ID, "calloc() failed");
        return -1;
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int l2_learning_cleanup(int *activated)
{
    ALOG_INFO(L2_LEARNING_ID, "Clean up - L2 learning");

    deactivate();

    FREE(mac_cache);

    if (destroy_database(&l2_learning_db)) {
        ALOG_ERROR(L2_LEARNING_ID, "Failed to disconnect a l2_learning database");
        return -1;
    } else {
        ALOG_INFO(L2_LEARNING_ID, "Disconnected from a l2_learning database");
    }

    return 0;
}

/**
 * \brief Function to list up all MAC tables
 * \param cli The pointer of the Barista CLI
 */
static int list_all_entries(cli_t *cli)
{
    cli_print(cli, "< MAC Tables >");

    if (select_data(&l2_learning_db, "forwarding_table", "DPID, PORT, IP, MAC", NULL, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&l2_learning_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint32_t ip = strtoul(row[2], NULL, 0);
        uint64_t mac = strtoull(row[3], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%4d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to show the MAC table for a specific switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int show_entry_switch(cli_t *cli, char *dpid_str)
{
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "< MAC Table for Switch [%lu] >", dpid);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "DPID = %lu", dpid);

    if (select_data(&l2_learning_db, "forwarding_table", "PORT, IP, MAC", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&l2_learning_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint32_t port = strtoul(row[0], NULL, 0);
        uint32_t ip = strtoul(row[1], NULL, 0);
        uint64_t mac = strtoull(row[2], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%4d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to find a MAC entry that contains a specific MAC address
 * \param cli The pointer of the Barista CLI
 * \param macaddr MAC address
 */
static int show_entry_mac(cli_t *cli, const char *macaddr)
{
    uint8_t mac[ETH_ALEN];
    str2mac(macaddr, mac);

    uint64_t macval = mac2int(mac);

    cli_print(cli, "< MAC Entry [%s] >", macaddr);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "MAC = %lu", macval);

    if (select_data(&l2_learning_db, "forwarding_table", "DPID, PORT, IP", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&l2_learning_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint32_t ip = strtoul(row[2], NULL, 0);

        cli_print(cli, "  Host #%4d - DPID: %lu, IP: %s, MAC: %s, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr, port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to find a MAC entry that contains a specific IP address
 * \param cli The pointer of the Barista CLI
 * \param ipaddr IP address
 */
static int show_entry_ip(cli_t *cli, const char *ipaddr)
{
    uint32_t ip = ip_addr_int(ipaddr);

    cli_print(cli, "< MAC Entry [%s] >", ipaddr);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "IP = %u", ip);

    if (select_data(&l2_learning_db, "forwarding_table", "DPID, PORT, MAC", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&l2_learning_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint64_t mac = strtoull(row[2], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%4d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ipaddr, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int l2_learning_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0) {
        if (args[1] != NULL && strcmp(args[1], "tables") == 0 && args[2] == NULL) {
            list_all_entries(cli);
            return 0;
        }
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0) {
        if (args[1] != NULL && strcmp(args[1], "switch") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_switch(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "mac") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_mac(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "ip") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_ip(cli, args[2]);
            return 0;
        }
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  l2_learning list tables");
    cli_print(cli, "  l2_learning show switch [DPID]");
    cli_print(cli, "  l2_learning show mac [MAC address]");
    cli_print(cli, "  l2_learning show ip [IP address]");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int l2_learning_handler(const app_event_t *av, app_event_out_t *av_out)
{
    switch (av->type) {
    case AV_DP_RECEIVE_PACKET:
        PRINT_EV("AV_DP_RECEIVE_PACKET\n");
        {
            const pktin_t *pktin = av->pktin;

            l2_learning(pktin);
        }
        break;
    case AV_DP_PORT_ADDED:
        PRINT_EV("AV_DP_PORT_ADDED\n");
        {
            const port_t *port = av->port;

            if (port->remote == FALSE) {
                char conditions[__CONF_STR_LEN];
                sprintf(conditions, "DPID = %lu and PORT = %u", port->dpid, port->port);

                delete_data(&l2_learning_db, "forwarding_table", conditions);
            }

            int i;
            for (i=0; i<NUM_MAC_ENTRIES; i++) {
                if (mac_cache[i].dpid == port->dpid && mac_cache[i].port == port->port) {
                    memset(&mac_cache[i], 0, sizeof(mac_entry_t));
                    break;
                }
            }
        }
        break;
    case AV_DP_PORT_DELETED:
        PRINT_EV("AV_DP_PORT_DELETED\n");
        {
            const port_t *port = av->port;

            if (port->remote == FALSE) {
                char conditions[__CONF_STR_LEN];
                sprintf(conditions, "DPID = %lu and PORT = %u", port->dpid, port->port);

                delete_data(&l2_learning_db, "forwarding_table", conditions);
            }

            int i;
            for (i=0; i<NUM_MAC_ENTRIES; i++) {
                if (mac_cache[i].dpid == port->dpid && mac_cache[i].port == port->port) {
                    memset(&mac_cache[i], 0, sizeof(mac_entry_t));
                    break;
                }
            }
        }
        break;
    case AV_SW_CONNECTED:
        PRINT_EV("AV_SW_CONNECTED\n");
        {
            const switch_t *sw = av->sw;

            if (sw->remote == FALSE) {
                char conditions[__CONF_STR_LEN];
                sprintf(conditions, "DPID = %lu", sw->dpid);

                delete_data(&l2_learning_db, "forwarding_table", conditions);
            }

            int i;
            for (i=0; i<NUM_MAC_ENTRIES; i++) {
                if (mac_cache[i].dpid == sw->dpid) {
                    memset(&mac_cache[i], 0, sizeof(mac_entry_t));
                }
            }
        }
        break;
    case AV_SW_DISCONNECTED:
        PRINT_EV("AV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = av->sw;

            if (sw->remote == FALSE) {
                char conditions[__CONF_STR_LEN];
                sprintf(conditions, "DPID = %lu", sw->dpid);

                delete_data(&l2_learning_db, "forwarding_table", conditions);
            }

            int i;
            for (i=0; i<NUM_MAC_ENTRIES; i++) {
                if (mac_cache[i].dpid == sw->dpid) {
                    memset(&mac_cache[i], 0, sizeof(mac_entry_t));
                }
            }
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

/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup host_mgmt Host Management
 * \brief (Management) host management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "host_mgmt.h"

/** \brief Host management ID */
#define HOST_MGMT_ID 367856965

/////////////////////////////////////////////////////////////////////

int get_host_entry(host_t *entry)
{
    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "IP = %u and MAC = %lu", entry->ip, entry->mac);

    if (select_data(&host_mgmt_db, "host_mgmt", "DPID, PORT", conditions, TRUE)) return -1;

    query_result_t *result = get_query_result(&host_mgmt_db);
    query_row_t row;

    if ((row = fetch_query_row(result)) != NULL) {
        entry->dpid = strtoul(row[0], NULL, 0);
        entry->port = strtoul(row[1], NULL, 0);

        release_query_result(result);

        return 0;
    }

    release_query_result(result);

    return -1;
}

int insert_host_entry(host_t *entry)
{
    char values[__CONF_STR_LEN];
    sprintf(values, "%lu, %u, %lu, %u", entry->dpid, entry->port, entry->mac, entry->ip);

    if (insert_data(&host_mgmt_db, "host_mgmt", "DPID, PORT, MAC, IP", values)) return -1;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to add a new host
 * \param pktin PACKET_IN message
 */
static int add_new_host(const pktin_t *pktin)
{
    host_key_t hkey = {0};

    hkey.ip = pktin->pkt_info.src_ip;
    hkey.mac = mac2int(pktin->pkt_info.src_mac);

    uint32_t key = hash_func((uint32_t *)&hkey, 4) % NUM_HOST_ENTRIES;

    if (host_cache[key].ip != hkey.ip && host_cache[key].mac != hkey.mac) {
        host_t src;

        src.ip = hkey.ip;
        src.mac = hkey.mac;

        memset(&host_cache[key], 0, sizeof(host_t));

        if (get_host_entry(&src)) { // new
            host_cache[key].dpid = pktin->dpid;
            host_cache[key].port = pktin->port;
            host_cache[key].ip = hkey.ip;
            host_cache[key].mac = hkey.mac;

            insert_host_entry(&host_cache[key]);

            ev_host_added(HOST_MGMT_ID, &host_cache[key]);

            LOG_INFO(HOST_MGMT_ID, "Detected a new device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                     pktin->dpid, ip_addr_str(pktin->pkt_info.src_ip),
                     pktin->pkt_info.src_mac[0], pktin->pkt_info.src_mac[1], pktin->pkt_info.src_mac[2], 
                     pktin->pkt_info.src_mac[3], pktin->pkt_info.src_mac[4], pktin->pkt_info.src_mac[5], pktin->port);

            return 0;
        }

        host_cache[key].dpid = src.dpid;
        host_cache[key].port = src.port;
        host_cache[key].ip = hkey.ip;
        host_cache[key].mac = hkey.mac;

        return 0;
    } else if (host_cache[key].ip != hkey.ip && host_cache[key].mac == hkey.mac) {
        uint8_t m[ETH_ALEN];
        int2mac(hkey.mac, m);

        LOG_WARN(HOST_MGMT_ID, "Different IP address (MAC: %02x:%02x:%02x:%02x:%02x:%02x, old IP: %s, new IP: %s)",
                 m[0], m[1], m[2], m[3], m[4], m[5], ip_addr_str(host_cache[key].ip), ip_addr_str(hkey.ip));

        return -1;
    } else if (host_cache[key].ip == hkey.ip && host_cache[key].mac != hkey.mac) {
        uint8_t o[ETH_ALEN], i[ETH_ALEN];
        int2mac(host_cache[key].mac, o);
        int2mac(hkey.mac, i);

        LOG_WARN(HOST_MGMT_ID, 
                 "Different MAC address (IP: %s, old MAC: %02x:%02x:%02x:%02x:%02x:%02x, new MAC: %02x:%02x:%02x:%02x:%02x:%02x)",
                 ip_addr_str(hkey.ip), o[0], o[1], o[2], o[3], o[4], o[5], i[0], i[1], i[2], i[3], i[4], i[5]);

        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int host_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(HOST_MGMT_ID, "Init - Host management");

    if (init_database(&host_mgmt_db, "barista_mgmt")) {
        LOG_ERROR(HOST_MGMT_ID, "Failed to connect a host_mgmt database");
        return -1;
    } else {
        LOG_INFO(HOST_MGMT_ID, "Connected to a host_mgmt database");
    }

    reset_table(&host_mgmt_db, "host_mgmt", FALSE);

    host_cache = (host_t *)CALLOC(NUM_HOST_ENTRIES, sizeof(host_t));
    if (host_cache == NULL) {
        LOG_ERROR(HOST_MGMT_ID, "calloc() failed");
        return -1;
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int host_mgmt_cleanup(int *activated)
{
    LOG_INFO(HOST_MGMT_ID, "Clean up - Host management");

    deactivate();

    FREE(host_cache);

    if (destroy_database(&host_mgmt_db)) {
        LOG_ERROR(HOST_MGMT_ID, "Failed to disconnect a host_mgmt database");
        return -1;
    } else {
        LOG_INFO(HOST_MGMT_ID, "Disconnected from a host_mgmt database");
    }

    return 0;
}

/**
 * \brief Function to print all hosts
 * \param cli The pointer of the Barista CLI
 */
static int host_listup(cli_t *cli)
{
    cli_print(cli, "< Host List >");

    if (select_data(&host_mgmt_db, "host_mgmt", "DPID, PORT, IP, MAC", NULL, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&host_mgmt_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint32_t ip = strtoul(row[2], NULL, 0);
        uint64_t mac = strtoull(row[3], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief Function to find hosts in the switch corresponding to a datapath ID
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int host_showup_switch(cli_t *cli, const char *dpid_str)
{
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "< Hosts connected to Switch [%lu] >", dpid);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "DPID = %lu", dpid);

    if (select_data(&host_mgmt_db, "host_mgmt", "PORT, IP, MAC", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&host_mgmt_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint32_t port = strtoul(row[0], NULL, 0);
        uint32_t ip = strtoul(row[1], NULL, 0);
        uint64_t mac = strtoull(row[2], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief Function to find a host that has a specific IP address
 * \param cli The pointer of the Barista CLI
 * \param ipaddr IP address
 */
static int host_showup_ip(cli_t *cli, const char *ipaddr)
{
    uint32_t ip = ip_addr_int(ipaddr);

    cli_print(cli, "< Host [%s] >", ipaddr);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "IP = %u", ip);

    if (select_data(&host_mgmt_db, "host_mgmt", "DPID, PORT, MAC", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&host_mgmt_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint64_t mac = strtoull(row[2], NULL, 0);

        uint8_t macaddr[ETH_ALEN];
        int2mac(mac, macaddr);

        cli_print(cli, "  Host #%d - DPID: %lu, IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                  ++cnt, dpid, ipaddr, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief Function to find a host that has a specific MAC address
 * \param cli The pointer of the Barista CLI
 * \param macaddr MAC address
 */
static int host_showup_mac(cli_t *cli, const char *macaddr)
{
    uint8_t mac[ETH_ALEN];
    str2mac(macaddr, mac);

    uint64_t macval = mac2int(mac);

    cli_print(cli, "< Host [%s] >", macaddr);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "MAC = %lu", macval);

    if (select_data(&host_mgmt_db, "host_mgmt", "DPID, PORT, IP", conditions, TRUE)) {
        cli_print(cli, "  Failed to execute a query");
        return -1;
    }

    query_result_t *result = get_query_result(&host_mgmt_db);
    query_row_t row;
    int cnt = 0;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t dpid = strtoull(row[0], NULL, 0);
        uint32_t port = strtoul(row[1], NULL, 0);
        uint32_t ip = strtoul(row[2], NULL, 0);

        cli_print(cli, "  Host #%d - DPID: %lu, IP: %s, MAC: %s, Port: %u",
                  ++cnt, dpid, ip_addr_str(ip), macaddr, port);
    }

    release_query_result(result);

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int host_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0 && args[1] != NULL && strcmp(args[1], "hosts") == 0 && args[2] == NULL) {
        host_listup(cli);
        return 0;
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0) {
        if (args[1] != NULL && strcmp(args[1], "switch") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_switch(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "ip") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_ip(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "mac") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_mac(cli, args[2]);
            return 0;
        }
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  host_mgmt list hosts");
    cli_print(cli, "  host_mgmt show switch [DPID]");
    cli_print(cli, "  host_mgmt show ip [IP address]");
    cli_print(cli, "  host_mgmt show mac [MAC address]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int host_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_DP_RECEIVE_PACKET:
        PRINT_EV("EV_DP_RECEIVE_PACKET\n");
        {
            const pktin_t *pktin = ev->pktin;

            if (add_new_host(pktin))
                return -1;
        }
        break;
    case EV_DP_PORT_DELETED:
        PRINT_EV("EV_DP_PORT_DELETED\n");
        {
            const port_t *port = ev->port;

            char conditions[__CONF_STR_LEN];
            sprintf(conditions, "DPID = %lu and PORT = %u", port->dpid, port->port);

            if (select_data(&host_mgmt_db, "host_mgmt", "IP, MAC", conditions, FALSE)) return -1;

            query_result_t *result = get_query_result(&host_mgmt_db);
            query_row_t row;

            while ((row = fetch_query_row(result)) != NULL) {
                host_t out = {0};

                out.dpid = port->dpid;
                out.port = port->port;

                out.ip = strtoul(row[0], NULL, 0);
                out.mac = strtoull(row[1], NULL, 0);

                ev_host_deleted(HOST_MGMT_ID, &out);

                uint8_t macaddr[6];
                int2mac(out.mac, macaddr);

                LOG_INFO(HOST_MGMT_ID, "Deleted a device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                         out.dpid, ip_addr_str(out.ip),
                         macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], out.port);
            }

            release_query_result(result);

            delete_data(&host_mgmt_db, "host_mgmt", conditions);

            int i;
            for (i=0; i<NUM_HOST_ENTRIES; i++) {
                if (host_cache[i].dpid == port->dpid && host_cache[i].port == port->port) {
                    memset(&host_cache[i], 0, sizeof(host_t));
                    break;
                }
            }
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            char conditions[__CONF_STR_LEN];
            sprintf(conditions, "DPID = %lu", sw->dpid);

            if (select_data(&host_mgmt_db, "host_mgmt", "PORT, IP, MAC", conditions, FALSE)) return -1;

            query_result_t *result = get_query_result(&host_mgmt_db);
            query_row_t row;

            while ((row = fetch_query_row(result)) != NULL) {
                host_t out = {0};

                out.dpid = sw->dpid;
                out.port = strtoul(row[0], NULL, 0);

                out.ip = strtoul(row[1], NULL, 0);
                out.mac = strtoull(row[2], NULL, 0);

                ev_host_deleted(HOST_MGMT_ID, &out);

                uint8_t macaddr[6];
                int2mac(out.mac, macaddr);

                LOG_INFO(HOST_MGMT_ID, "Deleted a device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                         out.dpid, ip_addr_str(out.ip),
                         macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], out.port);
            }

            release_query_result(result);

            delete_data(&host_mgmt_db, "host_mgmt", conditions);

            int i;
            for (i=0; i<NUM_HOST_ENTRIES; i++) {
                if (host_cache[i].dpid == sw->dpid) {
                    memset(&host_cache[i], 0, sizeof(host_t));
                }
            }
        }
        break;
    case EV_HOST_ADDED:
        PRINT_EV("EV_HOST_ADDED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == FALSE) break;

            uint8_t macaddr[6];
            int2mac(host->mac, macaddr);

            LOG_INFO(HOST_MGMT_ID, "Detected a new device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                     host->dpid, ip_addr_str(host->ip), 
                     macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], host->port);

        }
        break;
    case EV_HOST_DELETED:
        PRINT_EV("EV_HOST_DELETED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == FALSE) break;

            uint8_t macaddr[6];
            int2mac(host->mac, macaddr);

            LOG_INFO(HOST_MGMT_ID, "Deleted a device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                     host->dpid, ip_addr_str(host->ip), 
                     macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], host->port);
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

/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup topo_mgmt Topology Management
 * \brief (Management) topology management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 * \author Yeonkeun Kim <yeonk@kaist.ac.kr>
 */

#include "topo_mgmt.h"

/** \brief Topology management ID */
#define TOPO_MGMT_ID 3034593885

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send a LLDP packet using a pktout message
 * \param port Port
 */
static int send_lldp(port_t *port)
{
    pktout_t pktout = {0};

    pktout.dpid = port->dpid;
    pktout.port = -1;

    pktout.xid = 0;
    pktout.buffer_id = -1;

    pktout.total_len = 46;

    // LLDP
    lldp_chassis_id *chassis;  // mandatory
    lldp_port_id *portid;      // mandatory
    lldp_ttl *ttl;             // mandatory
    lldp_system_desc *desc;    // optional
    lldp_eol *eol;             // mandatory

    uint8_t multi[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};
    uint8_t mac[ETH_ALEN] = { *((uint8_t *)&port->dpid + 5),
                              *((uint8_t *)&port->dpid + 4),
                              *((uint8_t *)&port->dpid + 3),
                              *((uint8_t *)&port->dpid + 2),
                              *((uint8_t *)&port->dpid + 1),
                              *((uint8_t *)&port->dpid + 0) };

    struct ether_header *eth_header = (struct ether_header *)pktout.data; // 14

    memmove(eth_header->ether_shost, mac, ETH_ALEN);
    memmove(eth_header->ether_dhost, multi, ETH_ALEN);

    eth_header->ether_type = htons(ETHERTYPE_LLDP); // 32

    // lldp_chassis_id tlv
    chassis = (lldp_chassis_id *)(eth_header + 1);
    chassis->hdr.type = LLDP_TLVT_CHASSISID;
    chassis->hdr.length = 7;
    chassis->subtype = LLDP_CHASSISID_SUBTYPE_MACADDR;

    memmove(chassis->data, &port->info.hw_addr, ETH_ALEN);

    // lldp_port_id tlv
    portid = (lldp_port_id *)((uint8_t *)chassis + chassis->hdr.length + 2);
    portid->hdr.type = LLDP_TLVT_PORTID;
    portid->hdr.length = 5;
    portid->subtype = LLDP_PORTID_SUBTYPE_COMPONENT;

    uint32_t port_data = htonl(port->port);
    memmove(portid->data, &port_data, 4);

    // lldp_time_to_live tlv
    ttl = (lldp_ttl *)((uint8_t *)portid + portid->hdr.length + 2);
    ttl->hdr.type = LLDP_TLVT_TTL;
    ttl->hdr.length = 2;
    ttl->ttl = htons(DEFAULT_TTL);

    // lldp_port_description tlv
    desc = (lldp_system_desc *)((uint8_t *)ttl + ttl->hdr.length + 2);
    desc->hdr.type = LLDP_TLVT_SYSTEM_DESC;
    desc->hdr.length = 8;

    uint64_t net_dpid = htonll(port->dpid);
    memmove(desc->data, &net_dpid, 8);

    // lldp_end_of_lldpdu tlv
    eol = (lldp_eol *)((uint8_t *)desc + desc->hdr.length + 2);
    eol->hdr.type = LLDP_TLVT_EOL;
    eol->hdr.length = 0;

    // flip bytes
    chassis->hdr.raw = htons(chassis->hdr.raw);
    portid->hdr.raw = htons(portid->hdr.raw);
    ttl->hdr.raw = htons(ttl->hdr.raw);
    desc->hdr.raw = htons(desc->hdr.raw);
    eol->hdr.raw = htons(eol->hdr.raw);

    // output
    pktout.num_actions = 1;
    pktout.action[0].type = ACTION_OUTPUT;
    pktout.action[0].port = port->port;

    ev_dp_send_packet(TOPO_MGMT_ID, &pktout);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to insert a new link
 * \param src_dpid Source datapath ID
 * \param src_port Source port
 * \param dst_dpid Destination datapath ID
 * \param dst_port Destination port
 */
static int insert_link(uint64_t src_dpid, uint16_t src_port, uint64_t dst_dpid, uint16_t dst_port)
{
    int idx = src_dpid % __MAX_NUM_SWITCHES;
    do {
        pthread_spin_lock(&topo_lock[idx]);

        if (topo[idx].dpid == src_dpid) {
            int i;
            for (i=0; i<__MAX_NUM_PORTS; i++) {
                if (topo[idx].link[i].port == src_port) {
                    port_link_t *link = &topo[idx].link[i].link;

                   if (!link->dpid && !link->port) {
                        link->dpid = dst_dpid;
                        link->port = dst_port;

                        pthread_spin_unlock(&topo_lock[idx]);

                        char values[__CONF_STR_LEN];
                        sprintf(values, "%lu, %u, %lu, %u, 0, 0, 0, 0", src_dpid, src_port, dst_dpid, dst_port);
                        if (insert_data(&topo_mgmt_info, "topo_mgmt",
                            "SRC_DPID, SRC_PORT, DST_DPID, DST_PORT, RX_PACKETS, RX_BYTES, TX_PACKETS, TX_BYTES", values)) {
                            LOG_ERROR(TOPO_MGMT_ID, "insert_data() failed");
                            return 0;
                        }

                        port_t out = {0};

                        out.dpid = src_dpid;
                        out.port = src_port;
                        out.link.dpid = dst_dpid;
                        out.link.port = dst_port;

                        ev_link_added(TOPO_MGMT_ID, &out);

                        return 1;
                    } else if (link->dpid == dst_dpid && link->port == dst_port) {
                        pthread_spin_unlock(&topo_lock[idx]);

                        return 0;
                    } else {
                        pthread_spin_unlock(&topo_lock[idx]);

                        LOG_WARN(TOPO_MGMT_ID, "Inconsistent link {(%lu, %u) -> (%lu, %u)} -> {(%lu, %u) -> (%lu, %u)}\n",
                                 src_dpid, src_port, link->dpid, link->port, src_dpid, src_port, dst_dpid, dst_port);

                        return 0;
                    }

                    break;
                }
            }

            break;
        }

        pthread_spin_unlock(&topo_lock[idx]);

        idx = (idx + 1) % __MAX_NUM_SWITCHES;
    } while (idx != src_dpid % __MAX_NUM_SWITCHES);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int topo_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(TOPO_MGMT_ID, "Init - Topology management");

    if (get_database_info(&topo_mgmt_info, "barista_mgmt")) {
        LOG_ERROR(TOPO_MGMT_ID, "Failed to get the information of a topo_mgmt database");
        return -1;
    }

    reset_table(&topo_mgmt_info, "topo_mgmt", FALSE);

    topo = (topo_t *)CALLOC(__MAX_NUM_SWITCHES, sizeof(topo_t));
    if (topo == NULL) {
        LOG_ERROR(TOPO_MGMT_ID, "calloc() failed");
        return -1;
    }

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        pthread_spin_init(&topo_lock[i], PTHREAD_PROCESS_PRIVATE);
    }

    activate();

    while (*activated) {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            pthread_spin_lock(&topo_lock[i]);
            if (topo[i].dpid) {
                int j;
                for (j=0; j<__MAX_NUM_PORTS; j++) {
                    if (topo[i].link[j].port) {
                        send_lldp(&topo[i].link[j]);
                    }
                }
            }
            pthread_spin_unlock(&topo_lock[i]);
        }

        for (i=0; i<__TOPO_MGMT_REQUEST_TIME; i++) {
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
int topo_mgmt_cleanup(int *activated)
{
    LOG_INFO(TOPO_MGMT_ID, "Clean up - Topology management");

    deactivate();

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        pthread_spin_destroy(&topo_lock[i]);
    }

    FREE(topo);

    return 0;
}

/**
 * \brief Function to list up all detected links
 * \param cli The pointer of the Barista CLI
 */
static int topo_listup(cli_t *cli)
{
    database_t topo_mgmt_db;

    int cnt = 0;

    cli_print(cli, "< Link List >");

    if (select_data(&topo_mgmt_info, &topo_mgmt_db, "topo_mgmt", "SRC_DPID, SRC_PORT, DST_DPID, DST_PORT, RX_PACKETS, RX_BYTES, TX_PACKETS, TX_BYTES", 
        NULL, TRUE)) {
        cli_print(cli, "Failed to read the list of links");
        return -1;
    }

    query_result_t *result = get_query_result(&topo_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t src_dpid = strtoull(row[0], NULL, 0);
        uint32_t src_port = strtoul(row[1], NULL, 0);
        uint64_t dst_dpid = strtoull(row[2], NULL, 0);
        uint32_t dst_port = strtoul(row[3], NULL, 0);
        uint64_t rx_packets = strtoull(row[4], NULL, 0);
        uint64_t rx_bytes = strtoull(row[5], NULL, 0);
        uint64_t tx_packets = strtoull(row[6], NULL, 0);
        uint64_t tx_bytes = strtoull(row[7], NULL, 0);

        cli_print(cli, "  Link #%d: {(%lu, %u) -> (%lu, %u)}, rx_pkts: %lu, rx_bytes: %lu, tx_pkts: %lu, tx_bytes: %lu", 
                     ++cnt, src_dpid, src_port, dst_dpid, dst_port, rx_packets, rx_bytes, tx_packets, tx_bytes);
    }

    release_query_result(result);

    destroy_database(&topo_mgmt_db);

    return 0;
}

/**
 * \brief Function to print all links connected to a switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_string Datapath ID
 */
static int topo_showup(cli_t *cli, char *dpid_string)
{
    database_t topo_mgmt_db;

    int cnt = 0;
    uint64_t dpid = strtoull(dpid_string, NULL, 0);

    cli_print(cli, "< Link List [%s] >", dpid_string);

    char conditions[__CONF_STR_LEN];
    sprintf(conditions, "SRC_DPID = %lu", dpid);

    if (select_data(&topo_mgmt_info, &topo_mgmt_db, "topo_mgmt", "SRC_DPID, SRC_PORT, DST_DPID, DST_PORT, RX_PACKETS, RX_BYTES, TX_PACKETS, TX_BYTES", 
        conditions, TRUE)) {
        cli_print(cli, "Failed to read the list of links");
        return -1;
    }

    query_result_t *result = get_query_result(&topo_mgmt_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        uint64_t src_dpid = strtoull(row[0], NULL, 0);
        uint32_t src_port = strtoul(row[1], NULL, 0);
        uint64_t dst_dpid = strtoull(row[2], NULL, 0);
        uint32_t dst_port = strtoul(row[3], NULL, 0);
        uint64_t rx_packets = strtoull(row[4], NULL, 0);
        uint64_t rx_bytes = strtoull(row[5], NULL, 0);
        uint64_t tx_packets = strtoull(row[6], NULL, 0);
        uint64_t tx_bytes = strtoull(row[7], NULL, 0);

        cli_print(cli, "  Link #%d: {(%lu, %u) -> (%lu, %u)}, rx_pkts: %lu, rx_bytes: %lu, tx_pkts: %lu, tx_bytes: %lu",
                     ++cnt, src_dpid, src_port, dst_dpid, dst_port, rx_packets, rx_bytes, tx_packets, tx_bytes);
    }

    release_query_result(result);

    destroy_database(&topo_mgmt_db);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int topo_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0 && args[1] != NULL && strcmp(args[1], "links") == 0 && args[2] == NULL) {
        topo_listup(cli);
        return 0;
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0 && args[1] != NULL && args[2] == NULL) {
        topo_showup(cli, args[1]);
        return 0;
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  topo_mgmt list links");
    cli_print(cli, "  topo_mgmt show [datapath ID]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int topo_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_DP_RECEIVE_PACKET:
        PRINT_EV("EV_DP_RECEIVE_PACKET\n");
        {
            const pktin_t *pktin = ev->pktin;
            struct ether_header *eth_header = (struct ether_header *)pktin->data;

            if (pktin->pkt_info.proto & PROTO_LLDP) {
                uint64_t src_dpid = 0, dst_dpid = pktin->dpid;
                uint32_t src_port = 0, dst_port = pktin->port;

                struct tlv_header *tlv = (struct tlv_header *)(eth_header + 1);

                tlv->raw = ntohs(tlv->raw);

                while (tlv->type != LLDP_TLVT_EOL) {
                    if (tlv->type == LLDP_TLVT_PORTID) {
                        lldp_port_id *port = (lldp_port_id *)tlv;
                        if (port->subtype == LLDP_PORTID_SUBTYPE_COMPONENT) {
                            memmove(&src_port, port->data, 4);
                            src_port = ntohl(src_port);
                        }
                    } else if (tlv->type == LLDP_TLVT_SYSTEM_DESC) {
                        lldp_system_desc *desc = (lldp_system_desc *)tlv;
                        memmove(&src_dpid, desc->data, 8);
                        src_dpid = ntohll(src_dpid);
                    }

                    uint32_t length = tlv->length;
                    tlv->raw = htons(tlv->raw);

                    // move to next tlv
                    tlv = (struct tlv_header *)((uint8_t *)tlv + sizeof(struct tlv_header) + length);
                    tlv->raw = ntohs(tlv->raw);
                }

                tlv->raw = htons(tlv->raw);

                if (insert_link(src_dpid, src_port, dst_dpid, dst_port))
                    LOG_INFO(TOPO_MGMT_ID, "Detected a new link {(%lu, %u) -> (%lu, %u)}", src_dpid, src_port, dst_dpid, dst_port);

                return -1; // cut off the event chain
            }
        }
        break;
    case EV_SW_CONNECTED:
        PRINT_EV("EV_SW_CONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            int idx = sw->dpid % __MAX_NUM_SWITCHES;
            do {
                pthread_spin_lock(&topo_lock[idx]);

                if (topo[idx].dpid == 0) {
                    topo[idx].dpid = sw->dpid;
                    pthread_spin_unlock(&topo_lock[idx]);
                    break;
                }

                pthread_spin_unlock(&topo_lock[idx]);

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != sw->dpid % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            int idx = sw->dpid % __MAX_NUM_SWITCHES;
            do {
                pthread_spin_lock(&topo_lock[idx]);

                if (topo[idx].dpid == sw->dpid) {
                    int i;
                    for (i=0; i<__MAX_NUM_PORTS; i++) {
                        port_t *pt = &topo[idx].link[i];

                        if (pt->port && pt->link.port) {
                            LOG_INFO(TOPO_MGMT_ID, "Deleted a link {(%lu, %u) -> (%lu, %u)}", 
                                     topo[i].dpid, pt->port, pt->link.dpid, pt->link.port);

                            port_t out = {0};

                            out.dpid = pt->dpid;
                            out.port = pt->port;
                            out.link.dpid = pt->link.dpid;
                            out.link.port = pt->link.port;

                            ev_link_deleted(TOPO_MGMT_ID, &out);

                            memset(link, 0, sizeof(port_t));
                        }
                    }

                    topo[idx].dpid = 0;
                    topo[idx].remote = FALSE;

                    pthread_spin_unlock(&topo_lock[idx]);

                    char conditions[__CONF_STR_LEN];
                    sprintf(conditions, "SRC_DPID = %lu", sw->dpid);

                    if (delete_data(&topo_mgmt_info, "topo_mgmt", conditions)) {
                        LOG_ERROR(TOPO_MGMT_ID, "delete_data() failed");
                        break;
                    }

                    break;
                }

                pthread_spin_unlock(&topo_lock[idx]);

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != sw->dpid % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_DP_PORT_ADDED:
        PRINT_EV("EV_DP_PORT_ADDED\n");
        {
            const port_t *port = ev->port;

            if (port->remote == TRUE) break;
            else if (port->port > __MAX_NUM_PORTS) break;

            int idx = port->dpid % __MAX_NUM_SWITCHES;
            do {
                pthread_spin_lock(&topo_lock[idx]);

                if (topo[idx].dpid == port->dpid) {
                    int i;
                    for (i=0; i<__MAX_NUM_PORTS; i++) {
                        if (!topo[idx].link[i].port) {
                            topo[idx].link[i].dpid = port->dpid;
                            topo[idx].link[i].port = port->port;

                            memmove(&topo[idx].link[i].info.hw_addr, port->info.hw_addr, ETH_ALEN);

                            break;
                        }
                    }

                    pthread_spin_unlock(&topo_lock[idx]);

                    break;
                }

                pthread_spin_unlock(&topo_lock[idx]);

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != port->dpid % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_DP_PORT_DELETED:
        PRINT_EV("EV_DP_PORT_DELETED\n");
        {
            const port_t *port = ev->port;

            if (port->remote == TRUE) break;
            else if (port->port > __MAX_NUM_PORTS) break;

            int idx = port->dpid % __MAX_NUM_SWITCHES;
            do {
                pthread_spin_lock(&topo_lock[idx]);

                if (topo[idx].dpid == port->dpid) {
                    int i;
                    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
                        port_link_t *link = &topo[idx].link[i].link;

                        if (topo[idx].link[i].port != port->port) continue;

                        if (link->port) {
                            LOG_INFO(TOPO_MGMT_ID, "Deleted a link {(%lu, %u) -> (%lu, %u)}",
                                     port->dpid, port->port, link->dpid, link->port);

                            port_t out = {0};

                            out.dpid = port->dpid;
                            out.port = port->port;
                            out.link.dpid = link->dpid;
                            out.link.port = link->port;

                            ev_link_deleted(TOPO_MGMT_ID, &out);

                            char conditions[__CONF_STR_LEN];
                            sprintf(conditions, "SRC_DPID = %lu and SRC_PORT = %u and DST_DPID = %lu and DST_PORT = %u",
                                    out.dpid, out.port, out.link.dpid, out.link.port);

                            if (delete_data(&topo_mgmt_info, "topo_mgmt", conditions)) {
                                LOG_ERROR(TOPO_MGMT_ID, "delete_data() failed");
                            }

                            memset(&topo[idx].link[i], 0, sizeof(port_t));

                            break;
                        }
                    }

                    pthread_spin_unlock(&topo_lock[idx]);

                    break;
                }

                pthread_spin_unlock(&topo_lock[idx]);

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != port->dpid % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_DP_PORT_STATS:
        PRINT_EV("EV_DP_PORT_STATS\n");
        {
            const port_t *port = ev->port;

            if (port->remote == TRUE) break;

            int idx = port->dpid % __MAX_NUM_SWITCHES;
            do {
                pthread_spin_lock(&topo_lock[idx]);

                if (topo[idx].dpid == port->dpid) {
                    int i;
                    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
                        port_stat_t *stat = &topo[idx].link[i].stat;

                        if (topo[idx].link[i].port != port->port) continue;

                        stat->rx_packets = port->stat.rx_packets - stat->old_rx_packets;
                        stat->rx_bytes = port->stat.rx_bytes - stat->old_rx_bytes;
                        stat->tx_packets = port->stat.tx_packets - stat->old_tx_packets;
                        stat->tx_bytes = port->stat.tx_bytes - stat->old_tx_bytes;

                        stat->old_rx_packets = port->stat.rx_packets;
                        stat->old_rx_bytes = port->stat.rx_bytes;
                        stat->old_tx_packets = port->stat.tx_packets;
                        stat->old_tx_bytes = port->stat.tx_bytes;

                        char changes[__CONF_STR_LEN];
                        sprintf(changes, "RX_PACKETS = %lu, RX_BYTES = %lu, TX_PACKETS = %lu, TX_BYTES = %lu",
                                stat->rx_packets, stat->rx_bytes, stat->tx_packets, stat->tx_bytes);

                        char conditions[__CONF_STR_LEN];
                        sprintf(conditions, "SRC_DPID = %lu and SRC_PORT = %u and DST_DPID = %lu and DST_PORT = %u",
                                port->dpid, port->port, topo[idx].link[i].link.dpid, topo[idx].link[i].link.port);

                        if (update_data(&topo_mgmt_info, "topo_mgmt", changes, conditions)) {
                            LOG_ERROR(TOPO_MGMT_ID, "update_data() failed");
                        }

                        break;
                    }

                    pthread_spin_unlock(&topo_lock[idx]);

                    break;
                }

                pthread_spin_unlock(&topo_lock[idx]);

                idx = (idx + 1) % __MAX_NUM_SWITCHES;
            } while (idx != port->port % __MAX_NUM_SWITCHES);
        }
        break;
    case EV_LINK_ADDED:
        PRINT_EV("EV_LINK_ADDED\n");
        {
            const port_t *link = ev->port;

            if (link->remote == FALSE) break;

            LOG_INFO(TOPO_MGMT_ID, "Detected a new link {(%lu, %u) -> (%lu, %u)}", 
                     link->dpid, link->port, link->link.dpid, link->link.port);
        }
        break;
    case EV_LINK_DELETED:
        PRINT_EV("EV_LINK_DELETED\n");
        {
            const port_t *link = ev->port;

            if (link->remote == FALSE) break;

            LOG_INFO(TOPO_MGMT_ID, "Deleted a link {(%lu, %u) -> (%lu, %u)}",
                     link->dpid, link->port, link->link.dpid, link->link.port);
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

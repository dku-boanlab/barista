/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup ofp10 OpenFlow 1.0 Engine
 * \brief (Base) OpenFlow 1.0 engine
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "ofp10.h"

/** \brief OpenFlow engine ID */
#define OFP_ID 2846342287

/////////////////////////////////////////////////////////////////////

/** \brief The structure of Ethernet header with VLAN fields */
struct ether_vlan_header {
    uint8_t ether_dhost[ETH_ALEN];
    uint8_t ether_short[ETH_ALEN];
    uint16_t tpid;
    uint16_t tci;
    uint16_t ether_type;
} __attribute__((packed));

/** \brief The structure of ARP header */
struct arphdr {
    __be16        ar_hrd;
    __be16        ar_pro;
    unsigned char ar_hln;
    unsigned char ar_pln;
    __be16        ar_op;

    uint8_t arp_sha[ETH_ALEN];
    uint8_t arp_spa[4];
    uint8_t arp_tha[ETH_ALEN];
    uint8_t arp_tpa[4];
};

#define DHCP_SVR_PORT 67
#define DHCP_CLI_PORT 68

/////////////////////////////////////////////////////////////////////

static uint32_t get_xid(uint32_t fd)
{
    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);
    return sw.xid;
}

static uint64_t get_dpid(uint32_t fd)
{
    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_dpid(OFP_ID, &sw);
    return sw.dpid;
}

static uint32_t get_fd(uint64_t dpid)
{
    switch_t sw = {0};
    sw.dpid = dpid;
    ev_sw_get_fd(OFP_ID, &sw);
    return sw.fd;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to generate HELLO messages
 * \param msg HELLO message
 */
static int ofp10_hello(const raw_msg_t *msg)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_HELLO;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = of_input->xid;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to handle ERROR messages
 * \param msg ERROR message
 */
static int ofp10_error(const raw_msg_t *msg)
{
    struct ofp_error_msg *err = (struct ofp_error_msg *)msg->data;

    switch (ntohs(err->type)) {
    case OFPET_HELLO_FAILED:
        switch (ntohs(err->code)) {
        case OFPHFC_INCOMPATIBLE:
            LOG_ERROR(OFP_ID, "OFPET_HELLO_FAILED - OFPHFC_INCOMPATIBLE");
            break;
        case OFPHFC_EPERM:
            LOG_ERROR(OFP_ID, "OFPET_HELLO_FAILED - OFPHFC_EPERM");
            break;
        }
        break;
    case OFPET_BAD_REQUEST:
        switch (ntohs(err->code)) {
        case OFPBRC_BAD_VERSION:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_VERSION");
            break;
        case OFPBRC_BAD_TYPE:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_TYPE");
            break;
        case OFPBRC_BAD_STAT:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_STAT");
            break;
        case OFPBRC_BAD_VENDOR:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_VENDOR");
            break;
        case OFPBRC_BAD_SUBTYPE:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_SUBTYPE");
            break;
        case OFPBRC_EPERM:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_EPERM");
            break;
        case OFPBRC_BAD_LEN:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BAD_LEN");
            break;
        case OFPBRC_BUFFER_EMPTY:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BUFFER_EMPTY");
            break;
        case OFPBRC_BUFFER_UNKNOWN:
            LOG_ERROR(OFP_ID, "OFPET_BAD_REQUEST - OFPBRC_BUFFER_UNKNOWN");
            break;
        }
        break;
    case OFPET_BAD_ACTION:
        switch (ntohs(err->code)) {
        case OFPBAC_BAD_TYPE:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_TYPE");
            break;
        case OFPBAC_BAD_LEN:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_LEN");
            break;
        case OFPBAC_BAD_VENDOR:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_VENDOR");
            break;
        case OFPBAC_BAD_VENDOR_TYPE:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_VENDOR_TYPE");
            break;
        case OFPBAC_BAD_OUT_PORT:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_OUT_PORT");
            break;
        case OFPBAC_BAD_ARGUMENT:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_ARGUMENT");
            break;
        case OFPBAC_EPERM:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_EPERM");
            break;
        case OFPBAC_TOO_MANY:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_TOO_MANY");
            break;
        case OFPBAC_BAD_QUEUE:
            LOG_ERROR(OFP_ID, "OFPET_BAD_ACTION - OFPBAC_BAD_QUEUE");
            break;
        }
        break;
    case OFPET_FLOW_MOD_FAILED:
        switch (ntohs(err->code)) {
        case OFPFMFC_ALL_TABLES_FULL:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_ALL_TABLES_FULL");
            break;
        case OFPFMFC_OVERLAP:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_OVERLAP");
            break;
        case OFPFMFC_EPERM:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_EPERM");
            break;
        case OFPFMFC_BAD_EMERG_TIMEOUT:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_BAD_EMERG_TIMEOUT");
            break;
        case OFPFMFC_BAD_COMMAND:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_BAD_COMMAND");
            break;
        case OFPFMFC_UNSUPPORTED:
            LOG_ERROR(OFP_ID, "OFPET_FLOW_MOD_FAILED - OFPFMFC_UNSUPPORTED");
            break;
        }
        break;
    case OFPET_PORT_MOD_FAILED:
        switch (ntohs(err->code)) {
        case OFPPMFC_BAD_PORT:
            LOG_ERROR(OFP_ID, "OFPET_PORT_MOD_FAILED - OFPPMFC_BAD_PORT");
            break;
        case OFPPMFC_BAD_HW_ADDR:
            LOG_ERROR(OFP_ID, "OFPET_PORT_MOD_FAILED - OFPPMFC_BAD_HW_ADDR");
            break;
        }
        break;
    case OFPET_QUEUE_OP_FAILED:
        switch (ntohs(err->code)) {
        case OFPQOFC_BAD_PORT:
            LOG_ERROR(OFP_ID, "OFPET_QUEUE_OP_FAILED - OFPQOFC_BAD_PORT");
            break;
        case OFPQOFC_BAD_QUEUE:
            LOG_ERROR(OFP_ID, "OFPET_QUEUE_OP_FAILED - OFPQOFC_BAD_QUEUE");
            break;
        case OFPQOFC_EPERM:
            LOG_ERROR(OFP_ID, "OFPET_QUEUE_OP_FAILED - OFPQOFC_EPERM");
            break;
        }
        break;
    }

    return 0;
}

/**
 * \brief Function to handle ECHO messages
 * \param msg ECHO message
 */
static int ofp10_echo_reply(const raw_msg_t *msg)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_ECHO_REPLY;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = of_input->xid;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to generate FEATURES_REQUEST messages
 * \param msg OF message
 */
static int ofp10_features_request(const raw_msg_t *msg)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_FEATURES_REQUEST;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = ntohl(htonl(of_input->xid)+1);

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process FEATURES_REPLY messages (only for sw_mgmt)
 * \param msg FEATURE_REPLY message
 */
static int ofp10_features_reply(const raw_msg_t *msg)
{
    struct ofp_switch_features *reply = (struct ofp_switch_features *)msg->data;

    switch_t sw = {0};

    sw.dpid = ntohll(reply->datapath_id);

    sw.fd = msg->fd;
    sw.xid = ntohl(reply->header.xid) + 1;

    sw.n_buffers = ntohl(reply->n_buffers);
    sw.n_tables = reply->n_tables;

    sw.capabilities = ntohl(reply->capabilities);
    sw.actions = ntohl(reply->actions);

    ev_sw_update_config(OFP_ID, &sw);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process PACKET_IN messages
 * \param msg PACKET_IN message
 */
static int ofp10_packet_in(const raw_msg_t *msg)
{
    struct ofp_packet_in *in = (struct ofp_packet_in *)msg->data;

    pktin_t pktin = {0};

    pktin.dpid = get_dpid(msg->fd);
    pktin.port = ntohs(in->in_port);

    pktin.xid = ntohl(in->header.xid);
    pktin.buffer_id = ntohl(in->buffer_id);

    pktin.reason = in->reason;

    pktin.total_len = ntohs(in->total_len);
    uint8_t *data = (uint8_t *)msg->data + sizeof(struct ofp_packet_in) - 2;
    memmove(pktin.data, data, MIN(pktin.total_len, __MAX_PKT_SIZE));

    // packet pre-processing

    struct ether_header *eth_header = (struct ether_header *)data;
    uint16_t ether_type = ntohs(eth_header->ether_type);

    int base = sizeof(struct ether_header);

    if (ether_type == 0x8100) { // VLAN
        struct ether_vlan_header *eth_vlan_header = (struct ether_vlan_header *)data;

        base = sizeof(struct ether_vlan_header);
        ether_type = ntohs(eth_vlan_header->ether_type);

        pktin.proto |= PROTO_VLAN;

        pktin.vlan_id = ntohs(eth_vlan_header->tci) & 0xfff;
        pktin.vlan_pcp = ntohs(eth_vlan_header->tci) >> 13;
    }

    memmove(pktin.src_mac, eth_header->ether_shost, ETH_ALEN);
    memmove(pktin.dst_mac, eth_header->ether_dhost, ETH_ALEN);

    if (ether_type == 0x0800) { // IPv4
        struct iphdr *ip_header = (struct iphdr *)(data + base);
        int header_len = (ip_header->ihl * 4);

        pktin.proto |= PROTO_IPV4;
        pktin.ip_tos = ip_header->tos;

        pktin.src_ip = ntohl(ip_header->saddr);
        pktin.dst_ip = ntohl(ip_header->daddr);

        if (ip_header->protocol == IPPROTO_ICMP) {
            struct icmphdr *icmp_header = (struct icmphdr *)((uint8_t *)ip_header + header_len);

            pktin.proto |= PROTO_ICMP;

            pktin.type = ntohs(icmp_header->type);
            pktin.code = ntohs(icmp_header->code);
        } else if (ip_header->protocol == IPPROTO_TCP) {
            struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + header_len);

            pktin.proto |= PROTO_TCP;

            pktin.src_port = ntohs(tcp_header->source);
            pktin.dst_port = ntohs(tcp_header->dest);
        } else if (ip_header->protocol == IPPROTO_UDP) {
            struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + header_len);

            pktin.proto |= PROTO_UDP;

            pktin.src_port = ntohs(tcp_header->source);
            pktin.dst_port = ntohs(tcp_header->dest);

            if (pktin.dst_port == DHCP_SVR_PORT || pktin.dst_port == DHCP_CLI_PORT) {
                pktin.proto |= PROTO_DHCP;
            }
        } else { // IPv4
            // no pktin.src_port, pktin.dst_port
        }
    } else if (ether_type == 0x0806) { // ARP
        struct arphdr *arp_header = (struct arphdr *)(data + base);

        uint32_t *src_ip = (uint32_t *)arp_header->arp_spa;
        uint32_t *dst_ip = (uint32_t *)arp_header->arp_tpa;

        pktin.proto |= PROTO_ARP;

        pktin.src_ip = ntohl(*src_ip);
        pktin.dst_ip = ntohl(*dst_ip);

        pktin.opcode = ntohs(arp_header->ar_op);

        // no pktin.dst_port
    } else if (ether_type == 0x88cc) { // LLDP
        pktin.proto |= PROTO_LLDP;

        // no pktin.src_ip, pktin.dst_ip
        // no pktin.src_port, pktin.dst_port
    } else {
        pktin.proto |= PROTO_UNKNOWN;

        return -1;
    }

    ev_dp_receive_packet(OFP_ID, &pktin);

    return 0;
}

/**
 * \brief Function to process FLOW_REMOVED messages
 * \param msg FLOW_REMOVED message
 */
static int ofp10_flow_removed(const raw_msg_t *msg)
{
    struct ofp_flow_removed *removed = (struct ofp_flow_removed *)msg->data;

    flow_t flow = {0};

    flow.dpid = get_dpid(msg->fd);
    flow.port = ntohs(removed->match.in_port);

    flow.xid = ntohl(removed->header.xid);
    flow.cookie = ntohll(removed->cookie);
    flow.priority = ntohs(removed->priority);

    flow.idle_timeout = ntohs(removed->idle_timeout);

    flow.reason = removed->reason;

    // begin of match

    flow.wildcards = ntohl(removed->match.wildcards);

    if (removed->match.dl_vlan) {
        flow.proto |= PROTO_VLAN;

        flow.vlan_id = ntohs(removed->match.dl_vlan);
        flow.vlan_pcp = removed->match.dl_vlan_pcp;
    }

    switch (ntohs(removed->match.dl_type)) {
    case 0x0800: // IPv4
        flow.proto |= PROTO_IPV4;
        flow.ip_tos = removed->match.nw_tos;

        switch (removed->match.nw_proto) {
        case IPPROTO_TCP:
            flow.proto |= PROTO_TCP;
            break;
        case IPPROTO_UDP:
            flow.proto |= PROTO_UDP;

            if (ntohs(removed->match.tp_dst) == DHCP_SVR_PORT || ntohs(removed->match.tp_dst) == DHCP_CLI_PORT) {
                flow.proto |= PROTO_DHCP;
            }

            break;
        case IPPROTO_ICMP:
            flow.proto |= PROTO_ICMP;
            break;
        default:
            break;
        }

        break;
    case 0x0806: // ARP
        flow.proto |= PROTO_ARP;
        break;
    case 0x08cc: // LLDP
        flow.proto |= PROTO_LLDP;
        break;
    default: // Unknown
        flow.proto |= PROTO_UNKNOWN;
        break;
    }

    memmove(flow.src_mac, removed->match.dl_src, ETH_ALEN);
    memmove(flow.dst_mac, removed->match.dl_dst, ETH_ALEN);

    flow.src_ip = ntohl(removed->match.nw_src);
    flow.dst_ip = ntohl(removed->match.nw_dst);

    flow.src_port = ntohs(removed->match.tp_src);
    flow.dst_port = ntohs(removed->match.tp_dst);

    // end of match

    flow.duration_sec = ntohl(removed->duration_sec);
    flow.duration_nsec = ntohl(removed->duration_nsec);

    flow.pkt_count = ntohll(removed->packet_count);
    flow.byte_count = ntohll(removed->byte_count);

    switch (removed->reason) {
    case OFPRR_IDLE_TIMEOUT:
    case OFPRR_HARD_TIMEOUT:
        ev_dp_flow_expired(OFP_ID, &flow);
        break;
    case OFPRR_DELETE:
        ev_dp_flow_deleted(OFP_ID, &flow);
        break;
    default:
        break;
    }

    return 0;
}

/**
 * \brief Function to process PORT_STATUS messages
 * \param msg PORT_STATUS message
 */
static int ofp10_port_status(const raw_msg_t *msg)
{
    struct ofp_port_status *status = (struct ofp_port_status *)msg->data;
    struct ofp_phy_port *desc = (struct ofp_phy_port *)&status->desc;

    port_t port = {0};

    port.dpid = get_dpid(msg->fd);
    port.port = ntohs(desc->port_no);

    memmove(port.hw_addr, desc->hw_addr, ETH_ALEN);

    port.config = ntohl(desc->config);
    port.state = ntohl(desc->state);

    port.curr = ntohl(desc->curr);
    port.advertised = ntohl(desc->advertised);
    port.supported = ntohl(desc->supported);
    port.peer = ntohl(desc->peer);

    switch (status->reason) {
    case OFPPR_ADD:
        ev_dp_port_added(OFP_ID, &port);
        break;
    case OFPPR_MODIFY:
        if (port.config & OFPPC_PORT_DOWN || port.state & OFPPS_LINK_DOWN) {
            ev_dp_port_deleted(OFP_ID, &port);
        } else {
            ev_dp_port_modified(OFP_ID, &port);
        }
        break;
    case OFPPR_DELETE:
        ev_dp_port_deleted(OFP_ID, &port);
        break;
    default:
        break;
    }

    return 0;
}

/**
 * \brief Function to process STATS_REPLY messages
 * \param msg STATS_REPLY message
 */
static int ofp10_stats_reply(const raw_msg_t *msg)
{
    struct ofp_stats_reply *reply = (struct ofp_stats_reply *)msg->data;

    switch (ntohs(reply->type)) {
    case OFPST_DESC: // only for sw_mgmt
        {
            struct ofp_desc_stats *desc = (struct ofp_desc_stats *)reply->body;

            switch_t sw = {0};
            sw.fd = msg->fd;
            ev_sw_get_dpid(OFP_ID, &sw);

            strncpy(sw.mfr_desc, desc->mfr_desc, 256);
            strncpy(sw.hw_desc, desc->hw_desc, 256);
            strncpy(sw.sw_desc, desc->sw_desc, 256);
            strncpy(sw.serial_num, desc->serial_num, 32);
            strncpy(sw.dp_desc, desc->dp_desc, 256);

            ev_sw_update_desc(OFP_ID, &sw);
        }
        break;
    case OFPST_FLOW:
        {
            struct ofp_flow_stats *stats = (struct ofp_flow_stats *)reply->body;

            flow_t flow = {0};

            flow.dpid = get_dpid(msg->fd);
            flow.port = ntohs(stats->match.in_port);

            flow.cookie = ntohll(stats->cookie);
            flow.priority = ntohs(stats->priority);

            flow.idle_timeout = ntohs(stats->idle_timeout);
            flow.hard_timeout = ntohs(stats->hard_timeout);

            // begin of match

            flow.wildcards = ntohl(stats->match.wildcards);

            if (stats->match.dl_vlan) {
                flow.proto |= PROTO_VLAN;
                flow.vlan_id = ntohs(stats->match.dl_vlan);
                flow.vlan_pcp = stats->match.dl_vlan_pcp;
            }

            switch (ntohs(stats->match.dl_type)) {
            case 0x0800: // IPv4
                flow.proto |= PROTO_IPV4;
                flow.ip_tos = stats->match.nw_tos;

                switch (stats->match.nw_proto) {
                case IPPROTO_TCP:
                    flow.proto |= PROTO_TCP;
                    break;
                case IPPROTO_UDP:
                    flow.proto |= PROTO_UDP;

                    if (ntohs(stats->match.tp_dst) == DHCP_SVR_PORT || ntohs(stats->match.tp_dst) == DHCP_CLI_PORT) {
                        flow.proto |= PROTO_DHCP;
                    }

                    break;
                case IPPROTO_ICMP:
                    flow.proto |= PROTO_ICMP;
                    break;
                default:
                    break;
                }

                break;
            case 0x0806: // ARP
                flow.proto |= PROTO_ARP;
                break;
            case 0x08cc: // LLDP
                flow.proto |= PROTO_LLDP;
                break;
            default: // Unknown
                flow.proto |= PROTO_UNKNOWN;
                break;
            }

            memmove(flow.src_mac, stats->match.dl_src, ETH_ALEN);
            memmove(flow.dst_mac, stats->match.dl_dst, ETH_ALEN);

            flow.src_ip = ntohl(stats->match.nw_src);
            flow.dst_ip = ntohl(stats->match.nw_dst);

            flow.src_port = ntohs(stats->match.tp_src);
            flow.dst_port = ntohs(stats->match.tp_dst);

            // end of match

            flow.duration_sec = ntohl(stats->duration_sec);
            flow.duration_nsec = ntohl(stats->duration_nsec);

            flow.pkt_count = ntohll(stats->packet_count);
            flow.byte_count = ntohll(stats->byte_count);

            ev_dp_flow_stats(OFP_ID, &flow);
        }
        break;
    case OFPST_AGGREGATE:
        {
            struct ofp_aggregate_stats_reply *stats = (struct ofp_aggregate_stats_reply *)reply->body;

            flow_t flow = {0};

            flow.dpid = get_dpid(msg->fd);

            flow.pkt_count = ntohll(stats->packet_count);
            flow.byte_count = ntohll(stats->byte_count);
            flow.flow_count = ntohl(stats->flow_count);

            ev_dp_aggregate_stats(OFP_ID, &flow);
        }
        break;
    case OFPST_PORT:
        {
            struct ofp_port_stats *stats = (struct ofp_port_stats *)reply->body;

            int size = ntohs(reply->header.length) - sizeof(struct ofp_stats_reply);
            int entries = size / sizeof(struct ofp_port_stats);
            uint64_t dpid = get_dpid(msg->fd);

            int i;
            for (i=0; i<entries; i++) {
                if (ntohs(stats[i].port_no) > __MAX_NUM_PORTS)
                    continue;

                port_t port = {0};

                port.dpid = dpid;
                port.port = ntohs(stats[i].port_no);

                port.rx_packets = ntohll(stats[i].rx_packets);
                port.rx_bytes = ntohll(stats[i].rx_bytes);
                port.tx_packets = ntohll(stats[i].tx_packets);
                port.tx_bytes = ntohll(stats[i].tx_bytes);

                ev_dp_port_stats(OFP_ID, &port);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

static int ofp10_build_actions(int num_actions, const action_t *action, uint8_t *pkt, int *size)
{
    int i;
    for (i=0; i<num_actions; i++) {
        switch (action[i].type) {
        case ACTION_DISCARD:
            {
                // nothing to do
            }
            break;
        case ACTION_OUTPUT:
            {
                struct ofp_action_output *output = (struct ofp_action_output *)(pkt + *size);

                output->type = htons(OFPAT_OUTPUT);
                output->len = htons(sizeof(struct ofp_action_output));
                output->port = htons(action[i].port);

                output->max_len = 0;

                *size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_SET_VLAN_VID:
            {
                struct ofp_action_vlan_vid *vid = (struct ofp_action_vlan_vid *)(pkt + *size);

                vid->type = htons(OFPAT_SET_VLAN_VID);
                vid->len = htons(sizeof(struct ofp_action_vlan_vid));
                vid->vlan_vid = htons(action[i].vlan_id);

                *size += sizeof(struct ofp_action_vlan_vid);
            }
            break;
        case ACTION_SET_VLAN_PCP:
            {
                struct ofp_action_vlan_pcp *pcp = (struct ofp_action_vlan_pcp *)(pkt + *size);

                pcp->type = htons(OFPAT_SET_VLAN_PCP);
                pcp->len = htons(sizeof(struct ofp_action_vlan_pcp));
                pcp->vlan_pcp = action[i].vlan_pcp;

                *size += sizeof(struct ofp_action_vlan_pcp);
            }
            break;
        case ACTION_STRIP_VLAN:
            {
                struct ofp_action_header *act = (struct ofp_action_header *)(pkt + *size);

                act->type = htons(OFPAT_STRIP_VLAN);
                act->len = htons(sizeof(struct ofp_action_header));

                *size += sizeof(struct ofp_action_header);
            }
            break;
        case ACTION_SET_SRC_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + *size);

                mac->type = htons(OFPAT_SET_DL_SRC);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, action[i].mac_addr, ETH_ALEN);

                *size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_DST_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + *size);

                mac->type = htons(OFPAT_SET_DL_DST);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, action[i].mac_addr, ETH_ALEN);

                *size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_SRC_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + *size);

                ip->type = htons(OFPAT_SET_NW_SRC);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = action[i].ip_addr;

                *size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_DST_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + *size);

                ip->type = htons(OFPAT_SET_NW_DST);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = action[i].ip_addr;

                *size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_IP_TOS:
            {
                struct ofp_action_nw_tos *tos = (struct ofp_action_nw_tos *)(pkt + *size);

                tos->type = htons(OFPAT_SET_NW_TOS);
                tos->len = htons(sizeof(struct ofp_action_nw_tos));
                tos->nw_tos = action[i].ip_tos;

                *size += sizeof(struct ofp_action_nw_tos);
            }
            break;
        case ACTION_SET_SRC_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + *size);

                port->type = htons(OFPAT_SET_TP_SRC);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                port->tp_port = htons(action[i].port);

                *size += sizeof(struct ofp_action_tp_port);
            }
            break;
        case ACTION_SET_DST_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + *size);

                port->type = htons(OFPAT_SET_TP_DST);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                port->tp_port = htons(action[i].port);

                *size += sizeof(struct ofp_action_tp_port);
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

/**
 * \brief Function to process PACKET_OUT messages
 * \param pktout PKTOUT message
 */
static int ofp10_packet_out(const pktout_t *pktout)
{
    uint8_t pkt[__MAX_PKT_SIZE];
    struct ofp_packet_out *out = (struct ofp_packet_out *)pkt;

    int size = sizeof(struct ofp_packet_out);

    out->header.version = OFP_VERSION;
    out->header.type = OFPT_PACKET_OUT;
    out->header.xid = htonl(pktout->xid);

    out->buffer_id = htonl(pktout->buffer_id);
    out->in_port = htons(pktout->port);

    ofp10_build_actions(pktout->num_actions, pktout->action, pkt, &size);

    out->actions_len = htons(size - sizeof(struct ofp_packet_out));

    if (pktout->buffer_id == (uint32_t)-1 && pktout->total_len > 0) {
        memmove(pkt + size, pktout->data, MIN(pktout->total_len, __MAX_PKT_SIZE - size));
        size += MIN(pktout->total_len, __MAX_PKT_SIZE - size);
    }

    out->header.length = htons(size);

    raw_msg_t msg;

    msg.fd = get_fd(pktout->dpid);
    msg.length = size;
    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);
    
    return 0;
}

/**
 * \brief Function to process FLOW_MOD messages
 * \param flow FLOW message
 * \param command Command
 */
static int ofp10_flow_mod(const flow_t *flow, int command)
{
    uint8_t pkt[__MAX_PKT_SIZE];
    struct ofp_flow_mod *mod = (struct ofp_flow_mod *)pkt;

    int size = sizeof(struct ofp_flow_mod);

    mod->header.version = OFP_VERSION;
    mod->header.type = OFPT_FLOW_MOD;
    mod->header.xid = htonl(flow->xid);

    mod->buffer_id = htonl(flow->buffer_id);
    mod->cookie = htonl(flow->cookie);

    if (command == FLOW_ADD)
        mod->command = htons(OFPFC_ADD);
    else if (command == FLOW_MODIFY)
        mod->command = htons(OFPFC_MODIFY);
    else if (command == FLOW_DELETE)
        mod->command = htons(OFPFC_DELETE);

    mod->idle_timeout = htons(flow->idle_timeout);
    mod->hard_timeout = htons(flow->hard_timeout);

    if (flow->priority == 0)
        mod->priority = htons(0x8000); // default priority
    else
        mod->priority = htons(flow->priority);

    mod->out_port = htons(OFPP_NONE);
    mod->flags = htons(flow->flags);

    // begin of match

    mod->match.wildcards = htonl(flow->wildcards);
    mod->match.in_port = htons(flow->port);

    memmove(mod->match.dl_src, flow->src_mac, ETH_ALEN);
    memmove(mod->match.dl_dst, flow->dst_mac, ETH_ALEN);

    if (flow->vlan_id) {
        mod->match.dl_vlan = htons(flow->vlan_id);
        mod->match.dl_vlan_pcp = flow->vlan_pcp;
    } else {
        mod->match.dl_vlan = htons(OFP_VLAN_NONE);
    }

    if (flow->proto & PROTO_LLDP) { // LLDP
        mod->match.dl_type = htons(0x88cc);
    } else if (flow->proto & PROTO_ARP) { // ARP
        mod->match.dl_type = htons(0x0806);
        mod->match.nw_proto = htons(flow->opcode);

        mod->match.nw_src = htonl(flow->src_ip);
        mod->match.nw_dst = htonl(flow->dst_ip);
    } else if (flow->proto & (PROTO_IPV4 | PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // IPv4
        mod->match.dl_type = htons(0x0800);
        mod->match.nw_tos = flow->ip_tos;

        mod->match.nw_src = htonl(flow->src_ip);
        mod->match.nw_dst = htonl(flow->dst_ip);

        if (flow->proto & (PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // TCP,UDP,ICMP
            if (flow->proto & PROTO_TCP)
                mod->match.nw_proto = IPPROTO_TCP;
            else if (flow->proto & PROTO_UDP || flow->proto & PROTO_DHCP)
                mod->match.nw_proto = IPPROTO_UDP;
            else if (flow->proto & PROTO_ICMP)
                mod->match.nw_proto = IPPROTO_ICMP;

            mod->match.tp_src = htons(flow->src_port);
            mod->match.tp_dst = htons(flow->dst_port);
        }
    }

    // end of match

    ofp10_build_actions(flow->num_actions, flow->action, pkt, &size);

    mod->header.length = htons(size);

    raw_msg_t msg;

    msg.fd = get_fd(flow->dpid);
    msg.length = size;
    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);

    return 0;
}

/**
 * \brief Function to generate STATS_REQUEST (desc) messages
 * \param fd Socket
 */
static int ofp10_stats_desc_request(uint32_t fd)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = htons(sizeof(struct ofp_stats_request));
    request->header.xid = htonl(get_xid(fd));

    request->type = htons(OFPST_DESC);
    request->flags = htons(0);

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (flow) messages
 * \param flow FLOW message
 */
static int ofp10_flow_stats(const flow_t *flow)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    uint32_t fd = get_fd(flow->dpid);
    uint32_t xid = get_xid(fd);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(xid);

    request->type = htons(OFPST_FLOW);

    struct ofp_flow_stats_request *stat = (struct ofp_flow_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    // begin of match

    stat->match.wildcards = htonl(flow->wildcards);
    stat->match.in_port = htons(flow->port);

    memmove(stat->match.dl_src, flow->src_mac, ETH_ALEN);
    memmove(stat->match.dl_dst, flow->dst_mac, ETH_ALEN);

    stat->match.dl_vlan = htons(flow->vlan_id);
    stat->match.dl_vlan_pcp = flow->vlan_pcp;

    if (flow->proto & PROTO_LLDP) { // LLDP
        stat->match.dl_type = htons(0x88cc);
    } else if (flow->proto & PROTO_ARP) { // ARP
        stat->match.dl_type = htons(0x0806);
        stat->match.nw_proto = htons(flow->opcode);

        stat->match.nw_src = htonl(flow->src_ip);
        stat->match.nw_dst = htonl(flow->dst_ip);
    } else if (flow->proto & (PROTO_IPV4 | PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // IPv4
        stat->match.dl_type = htons(0x0800);
        stat->match.nw_tos = flow->ip_tos;

        stat->match.nw_src = htonl(flow->src_ip);
        stat->match.nw_dst = htonl(flow->dst_ip);

        if (flow->proto & (PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // TCP,UDP,ICMP
            if (flow->proto & PROTO_TCP)
                stat->match.nw_proto = IPPROTO_TCP;
            else if (flow->proto & PROTO_UDP || flow->proto & PROTO_DHCP)
                stat->match.nw_proto = IPPROTO_UDP;
            else if (flow->proto & PROTO_ICMP)
                stat->match.nw_proto = IPPROTO_ICMP;

            stat->match.tp_src = htons(flow->src_port);
            stat->match.tp_dst = htons(flow->dst_port);
        }
    }

    // end of match

    stat->table_id = 0xff;
    stat->out_port = htons(OFPP_NONE);

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (aggregate) messages
 * \param flow FLOW message
 */
static int ofp10_aggregate_stats(const flow_t *flow)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    uint32_t fd = get_fd(flow->dpid);
    uint32_t xid = get_xid(fd);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(xid);

    request->type = htons(OFPST_AGGREGATE);

    struct ofp_aggregate_stats_request *stat = (struct ofp_aggregate_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    // begin of match

    stat->match.wildcards = htonl(OFPFW_ALL);

    // end of match

    stat->table_id = 0xff;
    stat->out_port = htons(OFPP_NONE);

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (port) messages
 * \param port PORT message
 */
static int ofp10_port_stats(const port_t *port)
{
    raw_msg_t out;
    uint8_t pkt[__MAX_PKT_SIZE];

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_port_stats_request);

    uint32_t fd = get_fd(port->dpid);
    uint32_t xid = get_xid(fd);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(xid);

    request->type = htons(OFPST_PORT);

    struct ofp_port_stats_request *stat = (struct ofp_port_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    if (port->port == PORT_NONE)
        stat->port_no = htons(OFPP_NONE);
    else {
        stat->port_no = htons(port->port);
    }

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief OpenFlow 1.0 engine
 * \param msg OpenFlow message
 */
static int ofp10_engine(const raw_msg_t *msg)
{
    struct ofp_header *ofph = (struct ofp_header *)msg->data;

    switch (ofph->type) {
    case OFPT_HELLO:
        DEBUG("OFPT_HELLO\n");
        ofp10_hello(msg);
        ofp10_features_request(msg);
        break;
    case OFPT_ERROR:
        DEBUG("OFPT_ERROR\n");
        ofp10_error(msg);
        break;
    case OFPT_ECHO_REQUEST:
        DEBUG("OFPT_ECHO_REQUEST\n");
        ofp10_echo_reply(msg);
        break;
    case OFPT_ECHO_REPLY:
        DEBUG("OFPT_ECHO_REPLY\n");
        // nothing
        break;
    case OFPT_VENDOR:
        DEBUG("OFPT_VENDOR\n");
        // no implementation
        break;
    case OFPT_FEATURES_REQUEST:
        DEBUG("OFPT_FEATURES_REQUEST\n");
        // no way
        break;
    case OFPT_FEATURES_REPLY:
        DEBUG("OFPT_FEATURES_REPLY\n");
        ofp10_features_reply(msg);
        ofp10_stats_desc_request(msg->fd);
        break;
    case OFPT_GET_CONFIG_REQUEST:
        DEBUG("OFPT_GET_CONFIG_REQUEST\n");
        // no way (and no implementation)
        break;
    case OFPT_GET_CONFIG_REPLY:
        DEBUG("OFPT_GET_CONFIG_REPLY\n");
        // nothing (and no implementation)
        break;
    case OFPT_SET_CONFIG:
        DEBUG("OFPT_SET_CONFIG\n");
        // no way (and no implementation)
        break;
    case OFPT_PACKET_IN:
        DEBUG("OFPT_PACKET_IN\n");
        ofp10_packet_in(msg);
        break;
    case OFPT_FLOW_REMOVED:
        DEBUG("OFPT_FLOW_REMOVED\n");
        ofp10_flow_removed(msg);
        break;
    case OFPT_PORT_STATUS:
        DEBUG("OFPT_PORT_STATUS\n");
        ofp10_port_status(msg);
        break;
    case OFPT_PACKET_OUT:
        DEBUG("OFPT_PACKET_OUT\n");
        // no way
        break;
    case OFPT_FLOW_MOD:
        DEBUG("OFPT_FLOW_MOD\n");
        // no way
        break;
    case OFPT_PORT_MOD:
        DEBUG("OFPT_PORT_MOD\n");
        // no way
        break;
    case OFPT_STATS_REQUEST:
        DEBUG("OFPT_STATS_REQUEST\n");
        // no way
        break;
    case OFPT_STATS_REPLY:
        DEBUG("OFPT_STATS_REPLY\n");
        ofp10_stats_reply(msg);
        break;
    case OFPT_BARRIER_REQUEST:
        DEBUG("OFPT_BARRIER_REQUEST\n");
        // no way
        break;
    case OFPT_BARRIER_REPLY:
        DEBUG("OFPT_BARRIER_REPLY\n");
        // nothing (and no implementation)
        break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REQUEST\n");
        // no way (and no implementation)
        break;
    case OFPT_QUEUE_GET_CONFIG_REPLY:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REPLY\n");
        // nothing (and no implementation)
        break;
    default:
        DEBUG("UNKNOWN\n");
        break;
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
int ofp10_main(int *activated, int argc, char **argv)
{
    LOG_INFO(OFP_ID, "Init - OpenFlow 1.0 engine");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int ofp10_cleanup(int *activated)
{
    LOG_INFO(OFP_ID, "Clean up - OpenFlow 1.0 engine");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int ofp10_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int ofp10_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_IN:
        PRINT_EV("EV_OFP_MSG_IN\n");
        {
            const raw_msg_t *msg = ev->raw_msg;
            return ofp10_engine(msg);
        }
        break;
    case EV_DP_SEND_PACKET:
        PRINT_EV("EV_DP_SEND_PACKET\n");
        {
            const pktout_t *pktout = ev->pktout;
            return ofp10_packet_out(pktout);
        }
        break;
    case EV_DP_INSERT_FLOW:
        PRINT_EV("EV_DP_INSERT_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            return ofp10_flow_mod(flow, FLOW_ADD);
        }
        break;
    case EV_DP_MODIFY_FLOW:
        PRINT_EV("EV_DP_MODIFY_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            return ofp10_flow_mod(flow, FLOW_MODIFY);
        }
        break;
    case EV_DP_DELETE_FLOW:
        PRINT_EV("EV_DP_DELETE_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            return ofp10_flow_mod(flow, FLOW_DELETE);
        }
        break;
    case EV_DP_REQUEST_FLOW_STATS:
        PRINT_EV("EV_DP_REQUEST_FLOW_STATS\n");
        {
            const flow_t *flow = ev->flow;
            return ofp10_flow_stats(flow);
        }
        break;
    case EV_DP_REQUEST_AGGREGATE_STATS:
        PRINT_EV("EV_DP_REQUEST_AGGREGATE_STATS\n");
        {
            const flow_t *flow = ev->flow;
            return ofp10_aggregate_stats(flow);
        }
        break;
    case EV_DP_REQUEST_PORT_STATS:
        PRINT_EV("EV_DP_REQUEST_PORT_STATS\n");
        {
            const port_t *port = ev->port;
            return ofp10_port_stats(port);
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

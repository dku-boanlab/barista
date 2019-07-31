/*
 * Copyright 2015-2019 NSSLab, KAIST
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

static uint32_t get_xid(uint64_t dpid)
{
    switch_t sw = {0};
    sw.dpid = dpid;
    ev_sw_get_xid(OFP_ID, &sw);
    return sw.conn.xid;
}

static uint32_t get_xid_w_fd(uint32_t fd)
{
    switch_t sw = {0};
    sw.conn.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);
    return sw.conn.xid;
}

static uint64_t get_dpid(uint32_t fd)
{
    switch_t sw = {0};
    sw.conn.fd = fd;
    ev_sw_get_dpid(OFP_ID, &sw);
    return sw.dpid;
}

static uint32_t get_fd(uint64_t dpid)
{
    switch_t sw = {0};
    sw.dpid = dpid;
    ev_sw_get_fd(OFP_ID, &sw);
    return sw.conn.fd;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to generate HELLO messages
 * \param msg HELLO message
 */
static int ofp10_hello(const msg_t *msg)
{
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_HELLO;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = of_input->xid; // no change, as it is

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to handle ERROR messages
 * \param msg ERROR message
 */
static int ofp10_error(const msg_t *msg)
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
static int ofp10_echo_reply(const msg_t *msg)
{
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_ECHO_REPLY;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = of_input->xid; // no change, as it is

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to generate FEATURES_REQUEST messages
 * \param msg OF message
 */
static int ofp10_features_request(const msg_t *msg)
{
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);
    out.data = pkt;

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header *of_output = (struct ofp_header *)pkt;

    of_output->version = OFP_VERSION;
    of_output->type = OFPT_FEATURES_REQUEST;
    of_output->length = htons(sizeof(struct ofp_header));
    of_output->xid = htonl(ntohl(of_input->xid) + 1); // right after hello

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process FEATURES_REPLY messages (only for sw_mgmt)
 * \param msg FEATURE_REPLY message
 */
static int ofp10_features_reply(const msg_t *msg)
{
    struct ofp_switch_features *reply = (struct ofp_switch_features *)msg->data;

    switch_t sw = {0};

    sw.dpid = ntohll(reply->datapath_id);

    sw.conn.fd = msg->fd;
    sw.conn.xid = ntohl(reply->header.xid) + 1;

    ev_sw_established_conn(OFP_ID, &sw);

    int num_ports = (ntohs(reply->header.length) - sizeof(struct ofp_switch_features)) / sizeof(struct ofp_phy_port);
    struct ofp_phy_port *ports = reply->ports;

    int i;
    for (i=0; i<num_ports; i++) {
        port_t port = {0};

        port.dpid = sw.dpid;
        port.port = ntohs(ports[i].port_no);

        strcpy(port.info.name, ports[i].name);
        memmove(port.info.hw_addr, ports[i].hw_addr, ETH_ALEN);

        port.info.config = ntohl(ports[i].config);
        port.info.state = ntohl(ports[i].state);
        port.info.curr = ntohl(ports[i].curr);
        port.info.advertized = ntohl(ports[i].advertised);
        port.info.supported = ntohl(ports[i].supported);
        port.info.peer = ntohl(ports[i].peer);

        ev_dp_port_added(OFP_ID, &port);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process PACKET_IN messages
 * \param msg PACKET_IN message
 */
static int ofp10_packet_in(const msg_t *msg)
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

    memmove(pktin.pkt_info.src_mac, eth_header->ether_shost, ETH_ALEN);
    memmove(pktin.pkt_info.dst_mac, eth_header->ether_dhost, ETH_ALEN);

    if (ether_type == 0x8100) { // VLAN
        struct ether_vlan_header *eth_vlan_header = (struct ether_vlan_header *)data;

        base = sizeof(struct ether_vlan_header);
        ether_type = ntohs(eth_vlan_header->ether_type);

        pktin.pkt_info.proto |= PROTO_VLAN;

        pktin.pkt_info.vlan_id = ntohs(eth_vlan_header->tci) & 0xfff;
        pktin.pkt_info.vlan_pcp = ntohs(eth_vlan_header->tci) >> 13;
    }

    if (ether_type == 0x0800) { // IPv4
        struct iphdr *ip_header = (struct iphdr *)(data + base);
        int header_len = (ip_header->ihl * 4);

        pktin.pkt_info.proto |= PROTO_IPV4;
        pktin.pkt_info.ip_tos = ip_header->tos;

        pktin.pkt_info.src_ip = ntohl(ip_header->saddr);
        pktin.pkt_info.dst_ip = ntohl(ip_header->daddr);

        if (ip_header->protocol == IPPROTO_ICMP) {
            struct icmphdr *icmp_header = (struct icmphdr *)((uint8_t *)ip_header + header_len);

            pktin.pkt_info.proto |= PROTO_ICMP;

            pktin.pkt_info.icmp_type = icmp_header->type;
            pktin.pkt_info.icmp_code = icmp_header->code;
        } else if (ip_header->protocol == IPPROTO_TCP) {
            struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + header_len);

            pktin.pkt_info.proto |= PROTO_TCP;

            pktin.pkt_info.src_port = ntohs(tcp_header->source);
            pktin.pkt_info.dst_port = ntohs(tcp_header->dest);
        } else if (ip_header->protocol == IPPROTO_UDP) {
            struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + header_len);

            pktin.pkt_info.proto |= PROTO_UDP;

            pktin.pkt_info.src_port = ntohs(tcp_header->source);
            pktin.pkt_info.dst_port = ntohs(tcp_header->dest);

            if ((pktin.pkt_info.src_port == 67 && pktin.pkt_info.dst_port == 68) ||
                (pktin.pkt_info.src_port == 68 && pktin.pkt_info.dst_port == 67)) {
                pktin.pkt_info.proto |= PROTO_DHCP;
            }
        }
    } else if (ether_type == 0x0806) { // ARP
        struct arphdr *arp_header = (struct arphdr *)(data + base);

        uint32_t *src_ip = (uint32_t *)arp_header->arp_spa;
        uint32_t *dst_ip = (uint32_t *)arp_header->arp_tpa;

        pktin.pkt_info.proto |= PROTO_ARP;

        pktin.pkt_info.src_ip = ntohl(*src_ip);
        pktin.pkt_info.dst_ip = ntohl(*dst_ip);

        pktin.pkt_info.opcode = ntohs(arp_header->ar_op);
    } else if (ether_type == 0x88cc) { // LLDP
        pktin.pkt_info.proto |= PROTO_LLDP;
    } else {
        pktin.pkt_info.proto |= PROTO_UNKNOWN;
    }

    ev_dp_receive_packet(OFP_ID, &pktin);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to get the packet information from match fields
 * \param info Packet structure
 * \param match OFP match structure
 */
static int ofp10_get_match_fields(pkt_info_t *info, struct ofp_match *match)
{
    if (match->dl_vlan) {
        info->proto |= PROTO_VLAN;

        info->vlan_id = ntohs(match->dl_vlan);
        info->vlan_pcp = match->dl_vlan_pcp;
    }

    switch (ntohs(match->dl_type)) {
    case 0x0800: // IPv4
        info->proto |= PROTO_IPV4;
        info->ip_tos = match->nw_tos;

        switch (match->nw_proto) {
        case IPPROTO_TCP:
            info->proto |= PROTO_TCP;
            break;
        case IPPROTO_UDP:
            info->proto |= PROTO_UDP;

            if ((ntohs(match->tp_src) == 67 && ntohs(match->tp_dst) == 68) || 
                (ntohs(match->tp_src) == 68 && ntohs(match->tp_dst) == 67)) {
                info->proto |= PROTO_DHCP;
            }

            break;
        case IPPROTO_ICMP:
            info->proto |= PROTO_ICMP;
            break;
        default:
            break;
        }

        break;
    case 0x0806: // ARP
        info->proto |= PROTO_ARP;
        break;
    case 0x08cc: // LLDP
        info->proto |= PROTO_LLDP;
        break;
    default: // Unknown
        info->proto |= PROTO_UNKNOWN;
        break;
    }

    memmove(info->src_mac, match->dl_src, ETH_ALEN);
    memmove(info->dst_mac, match->dl_dst, ETH_ALEN);

    info->src_ip = ntohl(match->nw_src);
    info->dst_ip = ntohl(match->nw_dst);

    info->src_port = ntohs(match->tp_src);
    info->dst_port = ntohs(match->tp_dst);

    info->wildcards = ntohl(match->wildcards);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process FLOW_REMOVED messages
 * \param msg FLOW_REMOVED message
 */
static int ofp10_flow_removed(const msg_t *msg)
{
    struct ofp_flow_removed *removed = (struct ofp_flow_removed *)msg->data;

    flow_t flow = {0};

    flow.dpid = get_dpid(msg->fd);
    flow.port = ntohs(removed->match.in_port);

    flow.info.xid = ntohl(removed->header.xid);

    flow.meta.cookie = ntohll(removed->cookie);
    flow.meta.idle_timeout = ntohs(removed->idle_timeout);
    flow.meta.priority = ntohs(removed->priority);
    flow.meta.reason = removed->reason;

    ofp10_get_match_fields(&flow.pkt_info, &removed->match);

    flow.stat.duration_sec = ntohl(removed->duration_sec);
    flow.stat.duration_nsec = ntohl(removed->duration_nsec);

    flow.stat.pkt_count = ntohll(removed->packet_count);
    flow.stat.byte_count = ntohll(removed->byte_count);

    switch (flow.meta.reason) {
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
static int ofp10_port_status(const msg_t *msg)
{
    struct ofp_port_status *status = (struct ofp_port_status *)msg->data;
    struct ofp_phy_port *desc = (struct ofp_phy_port *)&status->desc;

    port_t port = {0};

    port.dpid = get_dpid(msg->fd);
    port.port = ntohs(desc->port_no);

    strcpy(port.info.name, desc->name);
    memmove(port.info.hw_addr, desc->hw_addr, ETH_ALEN);

    port.info.config = ntohl(desc->config);
    port.info.state = ntohl(desc->state);
    port.info.curr = ntohl(desc->curr);
    port.info.advertized = ntohl(desc->advertised);
    port.info.supported = ntohl(desc->supported);
    port.info.peer = ntohl(desc->peer);

    switch (status->reason) {
    case OFPPR_ADD:
        ev_dp_port_added(OFP_ID, &port);
        break;
    case OFPPR_MODIFY:
        if (port.info.config & OFPPC_PORT_DOWN || port.info.state & OFPPS_LINK_DOWN)
            ev_dp_port_deleted(OFP_ID, &port);
        else
            ev_dp_port_modified(OFP_ID, &port);
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
static int ofp10_stats_reply(const msg_t *msg)
{
    struct ofp_stats_reply *reply = (struct ofp_stats_reply *)msg->data;

    switch (ntohs(reply->type)) {
    case OFPST_DESC: // only for sw_mgmt
        {
            struct ofp_desc_stats *desc = (struct ofp_desc_stats *)reply->body;

            switch_t sw = {0};

            sw.dpid = get_dpid(msg->fd);

            strncpy(sw.desc.mfr_desc, desc->mfr_desc, 256);
            strncpy(sw.desc.hw_desc, desc->hw_desc, 256);
            strncpy(sw.desc.sw_desc, desc->sw_desc, 256);
            strncpy(sw.desc.serial_num, desc->serial_num, 32);
            strncpy(sw.desc.dp_desc, desc->dp_desc, 256);

            ev_sw_update_desc(OFP_ID, &sw);
        }
        break;
    case OFPST_FLOW:
        {
            struct ofp_flow_stats *stats = (struct ofp_flow_stats *)reply->body;

            flow_t flow = {0};

            flow.dpid = get_dpid(msg->fd);
            flow.port = ntohs(stats->match.in_port);

            flow.meta.cookie = ntohll(stats->cookie);
            flow.meta.idle_timeout = ntohs(stats->idle_timeout);
            flow.meta.hard_timeout = ntohs(stats->hard_timeout);
            flow.meta.priority = ntohs(stats->priority);

            ofp10_get_match_fields(&flow.pkt_info, &stats->match);

            flow.stat.duration_sec = ntohl(stats->duration_sec);
            flow.stat.duration_nsec = ntohl(stats->duration_nsec);

            flow.stat.pkt_count = ntohll(stats->packet_count);
            flow.stat.byte_count = ntohll(stats->byte_count);

            ev_dp_flow_stats(OFP_ID, &flow);
        }
        break;
    case OFPST_AGGREGATE:
        {
            struct ofp_aggregate_stats_reply *stats = (struct ofp_aggregate_stats_reply *)reply->body;

            flow_t flow = {0};

            flow.dpid = get_dpid(msg->fd);

            flow.stat.pkt_count = ntohll(stats->packet_count);
            flow.stat.byte_count = ntohll(stats->byte_count);
            flow.stat.flow_count = ntohl(stats->flow_count);

            ev_dp_aggregate_stats(OFP_ID, &flow);
        }
        break;
    case OFPST_TABLE:
        {
            // no implementation
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
                if (ntohs(stats[i].port_no) > __MAX_NUM_PORTS) continue;

                port_t port = {0};

                port.dpid = dpid;
                port.port = ntohs(stats[i].port_no);

                port.stat.rx_packets = ntohll(stats[i].rx_packets);
                port.stat.rx_bytes = ntohll(stats[i].rx_bytes);
                port.stat.tx_packets = ntohll(stats[i].tx_packets);
                port.stat.tx_bytes = ntohll(stats[i].tx_bytes);

                ev_dp_port_stats(OFP_ID, &port);
            }
        }
        break;
    case OFPST_QUEUE:
        {
            // no implementation
        }
        break;
    default:
        break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to build a set of actions
 * \param num_actions The number of actions
 * \param action Action list
 * \param pkt Packet pointer
 * \return The total size of actions
 */
static int ofp10_build_actions(int num_actions, const action_t *action, uint8_t *pkt)
{
    int size = 0;

    int i;
    for (i=0; i<num_actions; i++) {
        switch (action[i].type) {
        case ACTION_DROP:
            {
                // nothing to do
            }
            break;
        case ACTION_OUTPUT:
            {
                struct ofp_action_output *output = (struct ofp_action_output *)(pkt + size);

                output->type = htons(OFPAT_OUTPUT);
                output->len = htons(sizeof(struct ofp_action_output));
                output->port = htons(action[i].port);
                output->max_len = 0;

                size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_SET_VLAN_VID:
            {
                struct ofp_action_vlan_vid *vid = (struct ofp_action_vlan_vid *)(pkt + size);

                vid->type = htons(OFPAT_SET_VLAN_VID);
                vid->len = htons(sizeof(struct ofp_action_vlan_vid));
                vid->vlan_vid = htons(action[i].vlan_id);

                size += sizeof(struct ofp_action_vlan_vid);
            }
            break;
        case ACTION_SET_VLAN_PCP:
            {
                struct ofp_action_vlan_pcp *pcp = (struct ofp_action_vlan_pcp *)(pkt + size);

                pcp->type = htons(OFPAT_SET_VLAN_PCP);
                pcp->len = htons(sizeof(struct ofp_action_vlan_pcp));
                pcp->vlan_pcp = action[i].vlan_pcp;

                size += sizeof(struct ofp_action_vlan_pcp);
            }
            break;
        case ACTION_STRIP_VLAN:
            {
                struct ofp_action_header *act = (struct ofp_action_header *)(pkt + size);

                act->type = htons(OFPAT_STRIP_VLAN);
                act->len = htons(sizeof(struct ofp_action_header));

                size += sizeof(struct ofp_action_header);
            }
            break;
        case ACTION_SET_SRC_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + size);

                mac->type = htons(OFPAT_SET_DL_SRC);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_DST_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + size);

                mac->type = htons(OFPAT_SET_DL_DST);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_SRC_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_SRC);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = htonl(action[i].ip_addr);

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_DST_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_DST);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = htonl(action[i].ip_addr);

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_IP_TOS:
            {
                struct ofp_action_nw_tos *tos = (struct ofp_action_nw_tos *)(pkt + size);

                tos->type = htons(OFPAT_SET_NW_TOS);
                tos->len = htons(sizeof(struct ofp_action_nw_tos));
                tos->nw_tos = action[i].ip_tos;

                size += sizeof(struct ofp_action_nw_tos);
            }
            break;
        case ACTION_SET_SRC_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_SRC);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                port->tp_port = htons(action[i].port);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        case ACTION_SET_DST_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_DST);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                port->tp_port = htons(action[i].port);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        case ACTION_VENDOR:
            {
                struct ofp_action_vendor_header *vendor = (struct ofp_action_vendor_header *)(pkt + size);

                vendor->type = htons(OFPAT_VENDOR);
                vendor->len = htons(sizeof(struct ofp_action_vendor_header));
                vendor->vendor = htonl(action[i].vendor);

                size += sizeof(struct ofp_action_vendor_header);
            }
            break;
        default:
            break;
        }
    }

    return size;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process PACKET_OUT messages
 * \param pktout PKTOUT message
 */
static int ofp10_packet_out(const pktout_t *pktout)
{
    msg_t msg;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_packet_out *out = (struct ofp_packet_out *)pkt;

    int size = sizeof(struct ofp_packet_out);

    out->header.version = OFP_VERSION;
    out->header.type = OFPT_PACKET_OUT;

    if (pktout->xid)
        out->header.xid = htonl(pktout->xid);
    else
        out->header.xid = htonl(get_xid(pktout->dpid));

    if (pktout->buffer_id)
        out->buffer_id = htonl(pktout->buffer_id);
    else
        out->buffer_id = -1;

    if (pktout->port < __MAX_NUM_PORTS || pktout->port >= PORT_IN_PORT)
        out->in_port = htons(pktout->port);
    else {
        LOG_WARN(OFP_ID, "Received ofp_packet_out with wrong port (%u)", pktout->port);
        return -1;
    }

    int actions_len = ofp10_build_actions(pktout->num_actions, pktout->action, pkt + sizeof(struct ofp_packet_out));
    out->actions_len = htons(actions_len);

    size += actions_len;

    if (pktout->buffer_id == (uint32_t)-1 && pktout->total_len > 0) {
        memmove(pkt + size, pktout->data, MIN(pktout->total_len, __MAX_PKT_SIZE));
        size += MIN(pktout->total_len, __MAX_PKT_SIZE);
    }

    out->header.length = htons(size);

    msg.fd = get_fd(pktout->dpid);
    msg.length = size;
    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);
    
    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to set match fields
 * \param in_port Input port
 * \param info Packet structure
 * \param match OFP match structure
 */
static int ofp10_set_match_fields(uint32_t in_port, const pkt_info_t *info, struct ofp_match *match)
{
    match->wildcards = htonl(info->wildcards);
    match->in_port = htons(in_port);

    memmove(match->dl_src, info->src_mac, ETH_ALEN);
    memmove(match->dl_dst, info->dst_mac, ETH_ALEN);

    if (info->vlan_id > 0 && info->vlan_id < VLAN_NONE) {
        match->dl_vlan = htons(info->vlan_id);
        match->dl_vlan_pcp = info->vlan_pcp;
    } else {
        match->dl_vlan = htons(OFP_VLAN_NONE);
    }

    if (info->proto & PROTO_LLDP) { // LLDP
        match->dl_type = htons(0x88cc);
    } else if (info->proto & PROTO_ARP) { // ARP
        match->dl_type = htons(0x0806);
        match->nw_proto = htons(info->opcode);

        match->nw_src = htonl(info->src_ip);
        match->nw_dst = htonl(info->dst_ip);
    } else if (info->proto & (PROTO_IPV4 | PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // IPv4
        match->dl_type = htons(0x0800);
        match->nw_tos = info->ip_tos;

        match->nw_src = htonl(info->src_ip);
        match->nw_dst = htonl(info->dst_ip);

        if (info->proto & (PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_DHCP)) { // TCP,UDP,ICMP
            if (info->proto & PROTO_TCP)
                match->nw_proto = IPPROTO_TCP;
            else if (info->proto & PROTO_UDP || info->proto & PROTO_DHCP)
                match->nw_proto = IPPROTO_UDP;
            else if (info->proto & PROTO_ICMP)
                match->nw_proto = IPPROTO_ICMP;

            match->tp_src = htons(info->src_port);
            match->tp_dst = htons(info->dst_port);
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process FLOW_MOD messages
 * \param flow FLOW message
 * \param command Command
 */
static int ofp10_flow_mod(const flow_t *flow, int command)
{
    msg_t msg;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_flow_mod *mod = (struct ofp_flow_mod *)pkt;

    int size = sizeof(struct ofp_flow_mod);

    mod->header.version = OFP_VERSION;
    mod->header.type = OFPT_FLOW_MOD;

    if (flow->info.xid)
        mod->header.xid = htonl(flow->info.xid);
    else
        mod->header.xid = htonl(get_xid(flow->dpid));

    if (flow->info.buffer_id)
        mod->buffer_id = htonl(flow->info.buffer_id);
    else
        mod->buffer_id = -1;

    mod->cookie = htonl(flow->meta.cookie);

    if (command == FLOW_ADD)
        mod->command = htons(OFPFC_ADD);
    else if (command == FLOW_MODIFY)
        mod->command = htons(OFPFC_MODIFY);
    else if (command == FLOW_DELETE)
        mod->command = htons(OFPFC_DELETE);

    mod->idle_timeout = htons(flow->meta.idle_timeout);
    mod->hard_timeout = htons(flow->meta.hard_timeout);

    if (flow->meta.priority == 0)
        mod->priority = htons(0x8000); // default priority
    else
        mod->priority = htons(flow->meta.priority);

    mod->out_port = htons(OFPP_NONE);
    mod->flags = htons(flow->meta.flags | FLFG_SEND_REMOVED);

    ofp10_set_match_fields(flow->port, &flow->pkt_info, &mod->match);

    int actions_len = ofp10_build_actions(flow->num_actions, flow->action, pkt + sizeof(struct ofp_flow_mod));

    size += actions_len;

    mod->header.length = htons(size);

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
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = htons(sizeof(struct ofp_stats_request));
    request->header.xid = htonl(get_xid_w_fd(fd));

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
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    uint32_t fd = get_fd(flow->dpid);
    uint32_t xid = get_xid(flow->dpid);

    out.fd = fd;
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(xid);

    request->type = htons(OFPST_FLOW);

    struct ofp_flow_stats_request *stat = (struct ofp_flow_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    ofp10_set_match_fields(flow->port, &flow->pkt_info, &stat->match);

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
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    uint32_t fd = get_fd(flow->dpid);
    uint32_t xid = get_xid(flow->dpid);

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
 * \brief Function to update the configuration of a physical port
 * \param port PORT message
 */
static int ofp10_port_mod(const port_t *port)
{
    //TODO

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (port) messages
 * \param port PORT message
 */
static int ofp10_port_stats(const port_t *port)
{
    msg_t out;
    uint8_t pkt[__MAX_MSG_SIZE] = {0};

    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_port_stats_request);

    out.fd = get_fd(port->dpid);
    out.length = size;
    out.data = pkt;

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(get_xid(port->dpid));

    request->type = htons(OFPST_PORT);

    struct ofp_port_stats_request *stat = (struct ofp_port_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    if (port->port == PORT_NONE)
        stat->port_no = htons(OFPP_NONE);
    else if (port->port < __MAX_NUM_PORTS) {
        stat->port_no = htons(port->port);
    } else {
        LOG_WARN(OFP_ID, "Received ofp_port_stats_request with wrong port (%u)", port->port);
        return -1;
    }

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief OpenFlow 1.0 engine
 * \param msg OpenFlow message
 */
static int ofp10_engine(const msg_t *msg)
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
        // nothing to do
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
        // no way
        break;
    case OFPT_GET_CONFIG_REPLY:
        DEBUG("OFPT_GET_CONFIG_REPLY\n");
        // no implementation
        break;
    case OFPT_SET_CONFIG:
        DEBUG("OFPT_SET_CONFIG\n");
        // no implementation
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
        // nothing to do
        break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REQUEST\n");
        // no way
        break;
    case OFPT_QUEUE_GET_CONFIG_REPLY:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REPLY\n");
        // no implementation
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
            const msg_t *msg = ev->msg;
            ofp10_engine(msg);
        }
        break;
    case EV_DP_SEND_PACKET:
        PRINT_EV("EV_DP_SEND_PACKET\n");
        {
            const pktout_t *pktout = ev->pktout;
            ofp10_packet_out(pktout);
        }
        break;
    case EV_DP_INSERT_FLOW:
        PRINT_EV("EV_DP_INSERT_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            ofp10_flow_mod(flow, FLOW_ADD);
        }
        break;
    case EV_DP_MODIFY_FLOW:
        PRINT_EV("EV_DP_MODIFY_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            ofp10_flow_mod(flow, FLOW_MODIFY);
        }
        break;
    case EV_DP_DELETE_FLOW:
        PRINT_EV("EV_DP_DELETE_FLOW\n");
        {
            const flow_t *flow = ev->flow;
            ofp10_flow_mod(flow, FLOW_DELETE);
        }
        break;
    case EV_DP_REQUEST_FLOW_STATS:
        PRINT_EV("EV_DP_REQUEST_FLOW_STATS\n");
        {
            const flow_t *flow = ev->flow;
            ofp10_flow_stats(flow);
        }
        break;
    case EV_DP_REQUEST_AGGREGATE_STATS:
        PRINT_EV("EV_DP_REQUEST_AGGREGATE_STATS\n");
        {
            const flow_t *flow = ev->flow;
            ofp10_aggregate_stats(flow);
        }
        break;
    case EV_DP_MODIFY_PORT:
        PRINT_EV("EV_DP_MODIFY_PORT\n");
        {
            const port_t *port = ev->port;
            ofp10_port_mod(port);
        }
        break;
    case EV_DP_REQUEST_PORT_STATS:
        PRINT_EV("EV_DP_REQUEST_PORT_STATS\n");
        {
            const port_t *port = ev->port;
            ofp10_port_stats(port);
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

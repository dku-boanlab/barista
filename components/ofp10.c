/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup ofp
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

/**
 * \brief Function to generate HELLO messages
 * \param msg HELLO message
 */
static int ofp10_hello(const raw_msg_t *msg)
{
    raw_msg_t out = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header of_output;

    of_output.version = OFP_VERSION;
    of_output.type = OFPT_HELLO;
    of_output.length = htons(sizeof(struct ofp_header));
    of_output.xid = of_input->xid;

    out.data = (uint8_t *)&of_output;

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
    raw_msg_t out = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header of_output;

    of_output.version = OFP_VERSION;
    of_output.type = OFPT_ECHO_REPLY;
    of_output.length = htons(sizeof(struct ofp_header));
    of_output.xid = of_input->xid;

    out.data = (uint8_t *)&of_output;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to generate FEATURES_REQUEST messages
 * \param msg OF message
 */
static int ofp10_features_request(const raw_msg_t *msg)
{
    raw_msg_t out = {0};

    out.fd = msg->fd;
    out.length = sizeof(struct ofp_header);

    struct ofp_header *of_input = (struct ofp_header *)msg->data;
    struct ofp_header of_output;

    of_output.version = OFP_VERSION;
    of_output.type = OFPT_FEATURES_REQUEST;
    of_output.length = htons(sizeof(struct ofp_header));
    of_output.xid = ntohl(htonl(of_input->xid)+1);

    out.data = (uint8_t *)&of_output;

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

    sw.remote = FALSE;

    sw.n_tables = reply->n_tables;
    sw.n_buffers = ntohl(reply->n_buffers);

    uint32_t cap = ntohl(reply->capabilities);
    uint32_t new_cap = 0;

    if (cap & OFPC_FLOW_STATS) new_cap |= SC_FLOW_STATS;
    if (cap & OFPC_TABLE_STATS) new_cap |= SC_TABLE_STATS;
    if (cap & OFPC_PORT_STATS) new_cap |= SC_PORT_STATS;
    if (cap & OFPC_QUEUE_STATS) new_cap |= SC_QUEUE_STATS;

    sw.capabilities = new_cap;

    uint32_t actions = ntohl(reply->actions);
    uint32_t new_actions = 0;

    if (actions & 1) new_actions |= ACTION_OUTPUT;
    if (actions & 2) new_actions |= ACTION_SET_VLAN_VID;
    if (actions & 4) new_actions |= ACTION_SET_VLAN_PCP;
    if (actions & 8) new_actions |= ACTION_STRIP_VLAN;
    if (actions & 16) new_actions |= ACTION_SET_SRC_MAC;
    if (actions & 32) new_actions |= ACTION_SET_DST_MAC;
    if (actions & 64) new_actions |= ACTION_SET_SRC_IP;
    if (actions & 128) new_actions |= ACTION_SET_DST_IP;
    if (actions & 256) new_actions |= ACTION_SET_IP_TOS;
    if (actions & 512) new_actions |= ACTION_SET_SRC_PORT;
    if (actions & 1024) new_actions |= ACTION_SET_DST_PORT;

    sw.actions = actions;

    ev_sw_update_config(OFP_ID, &sw);

    return 0;
}

/**
 * \brief Function to generate GET_CONFIG_REQUEST messages
 * \param fd Socket
 */
static int ofp10_get_config_request(uint32_t fd)
{
    raw_msg_t out = {0};

    out.fd = fd;
    out.length = sizeof(struct ofp_header);

    struct ofp_header of_output;

    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);

    of_output.version = OFP_VERSION;
    of_output.type = OFPT_GET_CONFIG_REQUEST;
    of_output.length = htons(sizeof(struct ofp_header));
    of_output.xid = htonl(sw.xid);

    out.data = (uint8_t *)&of_output;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process GET_CONFIG_REPLY messages
 * \param msg GET_CONFIG_REPLY message
 */
static int ofp10_get_config_reply(const raw_msg_t *msg)
{
    struct ofp_switch_config *config = (struct ofp_switch_config *)msg->data;

    if (ntohs(config->flags) != 0 || ntohs(config->miss_send_len) != __MAX_PKT_SIZE) {
        DEBUG("fd: %d, config->flags: %d, config->miss_send_len: %d\n",
              msg->fd, ntohs(config->flags), ntohs(config->miss_send_len));
    }

    return 0;
}

/**
 * \brief Function to generate SET_CONFIG messages
 * \param fd Socket
 */
static int ofp10_set_config(uint32_t fd)
{
    raw_msg_t out = {0};

    out.fd = fd;
    out.length = sizeof(struct ofp_switch_config);

    struct ofp_switch_config of_output;

    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);

    of_output.header.version = OFP_VERSION;
    of_output.header.type = OFPT_SET_CONFIG;
    of_output.header.length = htons(sizeof(struct ofp_switch_config));
    of_output.header.xid = htonl(sw.xid);

    of_output.flags = htons(0x0);
    of_output.miss_send_len = htons(1500);

    out.data = (uint8_t *)&of_output;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to generate BARRIER_REQUEST messages
 * \param fd Socket
 */
static int ofp10_barrier_request(uint32_t fd)
{
    raw_msg_t out = {0};

    out.fd = fd;
    out.length = sizeof(struct ofp_header);

    struct ofp_header of_output;

    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);

    of_output.version = OFP_VERSION;
    of_output.type = OFPT_BARRIER_REQUEST;
    of_output.length = htons(sizeof(struct ofp_header));
    of_output.xid = htonl(sw.xid);

    out.data = (uint8_t *)&of_output;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

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

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

/////////////////////////////////////////////////////////////////////

static int ofp10_packet_out(const pktout_t *pktout);

/**
 * \brief Function to conduct a drop action into the data plane
 * \param pktin Pktin message
 */
static int discard_packet(const pktin_t *pktin)
{
    pktout_t out = {0};

    PKTOUT_INIT(out, pktin);

    out.num_actions = 1;
    out.action[0].type = ACTION_DISCARD;

    ofp10_packet_out(&out);

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
    uint32_t in_port = ntohs(in->in_port);

    pktin_t pktin = {0};

    switch_t sw = {0};
    sw.fd = msg->fd;
    ev_sw_get_dpid(OFP_ID, &sw);

    pktin.dpid = sw.dpid;
    pktin.port = in_port;

    pktin.xid = ntohl(in->header.xid);
    pktin.buffer_id = ntohl(in->buffer_id);

    pktin.reason = in->reason;

    if (in_port > __MAX_NUM_PORTS) {
        discard_packet(&pktin);
        return -1;
    }

    pktin.total_len = ntohs(in->total_len);
    uint8_t *data = (uint8_t *)msg->data + sizeof(struct ofp_packet_in) - 2;
    memmove(pktin.data, data, MIN(pktin.total_len, __MAX_PKT_SIZE));

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

    switch (ether_type) {
    case 0x0800: // IPv4
        {
            struct iphdr *ip_header = (struct iphdr *)(data + base);

            pktin.proto |= PROTO_IPV4;

            pktin.src_ip = ip_header->saddr;
            pktin.dst_ip = ip_header->daddr;

            if (ip_header->protocol == IPPROTO_ICMP) {
                struct icmphdr *icmp_header = (struct icmphdr *)((uint8_t *)ip_header + sizeof(struct iphdr));

                pktin.proto |= PROTO_ICMP;

                pktin.type = icmp_header->type;
                pktin.code = icmp_header->code;
            } else if (ip_header->protocol == IPPROTO_TCP) {
                struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + sizeof(struct iphdr));

                pktin.proto |= PROTO_TCP;

                pktin.src_port = ntohs(tcp_header->source);
                pktin.dst_port = ntohs(tcp_header->dest);
            } else if (ip_header->protocol == IPPROTO_UDP) {
                struct tcphdr *tcp_header = (struct tcphdr *)((uint8_t *)ip_header + sizeof(struct iphdr));

                pktin.src_port = ntohs(tcp_header->source);
                pktin.dst_port = ntohs(tcp_header->dest);

                if (pktin.dst_port == DHCP_SERVER_PORT || pktin.dst_port == DHCP_CLIENT_PORT) { // DHCP
                    pktin.proto &= !PROTO_IPV4;
                    pktin.proto |= PROTO_DHCP;
                } else { // UDP
                    pktin.proto |= PROTO_UDP;
                }
            } else { // IPv4
                // no pktin.src_port and pktin.dst_port
            }
        }
        break;
    case 0x0806: // ARP
        {
            struct arphdr *arp_header = (struct arphdr *)(data + base);

            pktin.proto |= PROTO_ARP;

            uint32_t *src_ip = (uint32_t *)arp_header->arp_spa;
            uint32_t *dst_ip = (uint32_t *)arp_header->arp_tpa;

            pktin.src_ip = *src_ip;
            pktin.dst_ip = *dst_ip;

            pktin.opcode = ntohs(arp_header->ar_op);

            // no pktin.dst_port
        }
        break;
    case 0x88cc: // LLDP
        {
            pktin.proto |= PROTO_LLDP;

            // no pktin.src_ip, pktin.dst_ip, pktin.src_port, pktin.dst_port
        }
        break;
    default:
        {
            pktin.proto |= PROTO_UNKNOWN;

            discard_packet(&pktin);

            return -1;
        }
        break;
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

    switch_t sw = {0};
    sw.fd = msg->fd;
    ev_sw_get_dpid(OFP_ID, &sw);

    flow.dpid = sw.dpid;
    flow.port = ntohs(removed->match.in_port);

    flow.xid = ntohl(removed->header.xid);
    flow.cookie = ntohll(removed->cookie);
    flow.priority = ntohs(removed->priority);

    flow.idle_timeout = ntohs(removed->idle_timeout);

    flow.remote = FALSE;

    // begin of match

    uint32_t wildcards = ntohl(removed->match.wildcards);
    uint32_t new_wildcards = 0;

    if (wildcards) {
        if (wildcards & OFPFW_IN_PORT) new_wildcards |= WCARD_PORT;
        if (wildcards & OFPFW_DL_VLAN) new_wildcards |= WCARD_VLAN;
        if (wildcards & OFPFW_DL_SRC) new_wildcards |= WCARD_SRC_MAC;
        if (wildcards & OFPFW_DL_DST) new_wildcards |= WCARD_DST_MAC;
        if (wildcards & OFPFW_DL_TYPE) new_wildcards |= WCARD_ETH_TYPE;
        if (wildcards & OFPFW_NW_PROTO) new_wildcards |= WCARD_PROTO;
        if (wildcards & OFPFW_TP_SRC) new_wildcards |= WCARD_SRC_PORT;
        if (wildcards & OFPFW_TP_DST) new_wildcards |= WCARD_DST_PORT;
        if (wildcards & OFPFW_NW_TOS) new_wildcards |= WCARD_IP_TOS;
        if (wildcards & OFPFW_NW_SRC_ALL) new_wildcards |= WCARD_SRC_IP;
        if (wildcards & OFPFW_NW_DST_ALL) new_wildcards |= WCARD_DST_IP;
    }

    flow.wildcards = new_wildcards;

    if (removed->match.dl_vlan) {
        flow.proto |= PROTO_VLAN;
        flow.vlan_id = ntohs(removed->match.dl_vlan);

        if (wildcards & OFPFW_DL_VLAN_PCP)
            flow.wildcards |= WCARD_VLAN_PCP;
        else if (removed->match.dl_vlan_pcp)
            flow.vlan_pcp = removed->match.dl_vlan_pcp;
    }

    switch (ntohs(removed->match.dl_type)) {
    case 0x0800: // IPv4
        flow.ip_tos = removed->match.nw_tos;

        flow.proto |= PROTO_IPV4;

        switch (removed->match.nw_proto) {
        case IPPROTO_TCP:
            flow.proto |= PROTO_TCP;
            break;
        case IPPROTO_UDP:
            flow.proto |= PROTO_UDP;
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

    flow.src_ip = removed->match.nw_src;
    flow.dst_ip = removed->match.nw_dst;

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

    if (ntohs(desc->port_no) == 0 || ntohs(desc->port_no) > __MAX_NUM_PORTS)
        return -1;

    port_t port = {0};

    switch_t sw = {0};
    sw.fd = msg->fd;
    ev_sw_get_dpid(OFP_ID, &sw);

    port.dpid = sw.dpid;
    port.port = ntohs(desc->port_no);

    port.remote = FALSE;

    memmove(port.hw_addr, desc->hw_addr, ETH_ALEN);

    uint32_t config = ntohl(desc->config);
    uint32_t new_config = 0;

    if (config & OFPPC_PORT_DOWN) new_config |= PC_PORT_DOWN;
    if (config & OFPPC_NO_FLOOD) new_config |= PC_NO_FLOOD;
    if (config & OFPPC_NO_FWD) new_config |= PC_NO_FWD;
    if (config & OFPPC_NO_PACKET_IN) new_config |= PC_NO_PACKET_IN;

    port.config = new_config;

    uint32_t state = ntohl(desc->state);
    uint32_t new_state = 0;

    if (state & OFPPS_LINK_DOWN) new_state |= PS_LINK_DOWN;

    port.state = new_state;

    uint32_t curr = ntohl(desc->curr);
    uint32_t new_curr = 0;

    if (curr & OFPPF_10MB_HD) new_curr |= PF_10MB_HD;
    if (curr & OFPPF_10MB_FD) new_curr |= PF_10MB_FD;
    if (curr & OFPPF_100MB_HD) new_curr |= PF_100MB_HD;
    if (curr & OFPPF_100MB_FD) new_curr |= PF_100MB_FD;
    if (curr & OFPPF_1GB_HD) new_curr |= PF_1GB_HD;
    if (curr & OFPPF_1GB_FD) new_curr |= PF_1GB_FD;

    port.curr = new_curr;

    uint32_t advertised = ntohl(desc->advertised);
    uint32_t new_ad = 0;

    if (advertised & OFPPF_10MB_HD) new_ad |= PF_10MB_HD;
    if (advertised & OFPPF_10MB_FD) new_ad |= PF_10MB_FD;
    if (advertised & OFPPF_100MB_HD) new_ad |= PF_100MB_HD;
    if (advertised & OFPPF_100MB_FD) new_ad |= PF_100MB_FD;
    if (advertised & OFPPF_1GB_HD) new_ad |= PF_1GB_HD;
    if (advertised & OFPPF_1GB_FD) new_ad |= PF_1GB_FD;

    port.advertised = new_ad;

    uint32_t supported = ntohl(desc->supported);
    uint32_t new_sp = 0;

    if (supported & OFPPF_10MB_HD) new_sp |= PF_10MB_HD;
    if (supported & OFPPF_10MB_FD) new_sp |= PF_10MB_FD;
    if (supported & OFPPF_100MB_HD) new_sp |= PF_100MB_HD;
    if (supported & OFPPF_100MB_FD) new_sp |= PF_100MB_FD;
    if (supported & OFPPF_1GB_HD) new_sp |= PF_1GB_HD;
    if (supported & OFPPF_1GB_FD) new_sp |= PF_1GB_FD;

    port.supported = new_sp;

    uint32_t peer = ntohl(desc->peer);
    uint32_t new_pr = 0;

    if (peer & OFPPF_10MB_HD) new_pr |= PF_10MB_HD;
    if (peer & OFPPF_10MB_FD) new_pr |= PF_10MB_FD;
    if (peer & OFPPF_100MB_HD) new_pr |= PF_100MB_HD;
    if (peer & OFPPF_100MB_FD) new_pr |= PF_100MB_FD;
    if (peer & OFPPF_1GB_HD) new_pr |= PF_1GB_HD;
    if (peer & OFPPF_1GB_FD) new_pr |= PF_1GB_FD;

    port.peer = new_pr;

    switch (status->reason) {
    case OFPPR_ADD:
        ev_dp_port_added(OFP_ID, &port);
        break;
    case OFPPR_MODIFY:
        if (port.config & OFPPC_PORT_DOWN) {
            ev_dp_port_deleted(OFP_ID, &port);
        } else if (port.state & OFPPS_LINK_DOWN) {
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

            switch_t sw = {0};
            sw.fd = msg->fd;
            ev_sw_get_dpid(OFP_ID, &sw);

            flow.dpid = sw.dpid;
            flow.port = ntohs(stats->match.in_port);

            flow.cookie = ntohll(stats->cookie);
            flow.priority = ntohs(stats->priority);

            flow.idle_timeout = ntohs(stats->idle_timeout);
            flow.hard_timeout = ntohs(stats->hard_timeout);

            flow.remote = FALSE;

            // begin of match

            uint32_t wildcards = ntohl(stats->match.wildcards);
            uint32_t new_wildcards = 0;

            if (wildcards) {
                if (wildcards & OFPFW_IN_PORT) new_wildcards |= WCARD_PORT;
                if (wildcards & OFPFW_DL_VLAN) new_wildcards |= WCARD_VLAN;
                if (wildcards & OFPFW_DL_SRC) new_wildcards |= WCARD_SRC_MAC;
                if (wildcards & OFPFW_DL_DST) new_wildcards |= WCARD_DST_MAC;
                if (wildcards & OFPFW_DL_TYPE) new_wildcards |= WCARD_ETH_TYPE;
                if (wildcards & OFPFW_NW_PROTO) new_wildcards |= WCARD_PROTO;
                if (wildcards & OFPFW_TP_SRC) new_wildcards |= WCARD_SRC_PORT;
                if (wildcards & OFPFW_TP_DST) new_wildcards |= WCARD_DST_PORT;
                if (wildcards & OFPFW_NW_TOS) new_wildcards |= WCARD_IP_TOS;
                if (wildcards & OFPFW_NW_SRC_ALL) new_wildcards |= WCARD_SRC_IP;
                if (wildcards & OFPFW_NW_DST_ALL) new_wildcards |= WCARD_DST_IP;
            }

            flow.wildcards = new_wildcards;

            if (stats->match.dl_vlan) {
                flow.proto |= PROTO_VLAN;
                flow.vlan_id = ntohs(stats->match.dl_vlan);

                if (wildcards & OFPFW_DL_VLAN_PCP)
                    flow.wildcards |= WCARD_VLAN_PCP;
                else if (stats->match.dl_vlan_pcp)
                    flow.vlan_pcp = stats->match.dl_vlan_pcp;
            }

            switch (ntohs(stats->match.dl_type)) {
            case 0x0800: // IPv4
                flow.ip_tos = stats->match.nw_tos;

                flow.proto |= PROTO_IPV4;

                switch (stats->match.nw_proto) {
                case IPPROTO_TCP:
                    flow.proto |= PROTO_TCP;
                    break;
                case IPPROTO_UDP:
                    flow.proto |= PROTO_UDP;
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

            flow.src_ip = stats->match.nw_src;
            flow.dst_ip = stats->match.nw_dst;

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

            switch_t sw = {0};
            sw.fd = msg->fd;
            ev_sw_get_dpid(OFP_ID, &sw);

            flow.dpid = sw.dpid;

            flow.remote = FALSE;

            flow.pkt_count = ntohll(stats->packet_count);
            flow.byte_count = ntohll(stats->byte_count);
            flow.flow_count = ntohl(stats->flow_count);

            ev_dp_aggregate_stats(OFP_ID, &flow);
        }
        break;
    case OFPST_TABLE:
        {
            //
        }
        break;
    case OFPST_PORT:
        {
            struct ofp_port_stats *stats = (struct ofp_port_stats *)reply->body;

            int size = ntohs(reply->header.length) - sizeof(struct ofp_stats_reply);
            int entries = size / sizeof(struct ofp_port_stats);

            switch_t sw = {0};
            sw.fd = msg->fd;
            ev_sw_get_dpid(OFP_ID, &sw);

            int i;
            for (i=0; i<entries; i++) {
                if (ntohs(stats[i].port_no) == 0 || ntohs(stats[i].port_no) > __MAX_NUM_PORTS)
                    continue;

                port_t port = {0};

                port.dpid = sw.dpid;
                port.port = ntohs(stats[i].port_no);

                port.remote = FALSE;

                port.rx_packets = ntohll(stats[i].rx_packets);
                port.rx_bytes = ntohll(stats[i].rx_bytes);
                port.tx_packets = ntohll(stats[i].tx_packets);
                port.tx_bytes = ntohll(stats[i].tx_bytes);

                ev_dp_port_stats(OFP_ID, &port);
            }
        }
        break;
    case OFPST_QUEUE:
        {
            //
        }
        break;
    case OFPST_VENDOR:
        {
            //
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * \brief Function to process PACKET_OUT messages
 * \param pktout PKTOUT message
 */
static int ofp10_packet_out(const pktout_t *pktout)
{
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_packet_out *out = (struct ofp_packet_out *)pkt;
    int size = sizeof(struct ofp_packet_out);

    out->header.version = OFP_VERSION;
    out->header.type = OFPT_PACKET_OUT;
    out->header.xid = htonl(pktout->xid);

    out->buffer_id = htonl(pktout->buffer_id);
    out->in_port = htons(pktout->port);

    int i;
    for (i=0; i<pktout->num_actions; i++) {
        switch (pktout->action[i].type) {
        case ACTION_DISCARD:
            {
                struct ofp_action_output *output = (struct ofp_action_output *)(pkt + size);

                output->len = htons(sizeof(struct ofp_action_output));

                size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_OUTPUT:
            {
                struct ofp_action_output *output = (struct ofp_action_output *)(pkt + size);

                output->type = htons(OFPAT_OUTPUT);
                output->len = htons(sizeof(struct ofp_action_output));

                if (pktout->action[i].port == PORT_FLOOD)
                    output->port = htons(OFPP_FLOOD);
                else if (pktout->action[i].port == PORT_CONTROLLER)
                    output->port = htons(OFPP_CONTROLLER);
                else if (pktout->action[i].port == PORT_ALL)
                    output->port = htons(OFPP_ALL);
                else if (pktout->action[i].port == PORT_NONE)
                    output->port = htons(OFPP_NONE);
                else {
                    output->port = htons(pktout->action[i].port);
                }

                output->max_len = 0;

                size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_SET_VLAN_VID:
            {
                struct ofp_action_vlan_vid *vid = (struct ofp_action_vlan_vid *)(pkt + size);

                vid->type = htons(OFPAT_SET_VLAN_VID);
                vid->len = htons(sizeof(struct ofp_action_vlan_vid));
                vid->vlan_vid = htons(pktout->action[i].vlan_id);

                size += sizeof(struct ofp_action_vlan_vid);
            }
            break;
        case ACTION_SET_VLAN_PCP:
            {
                struct ofp_action_vlan_pcp *pcp = (struct ofp_action_vlan_pcp *)(pkt + size);

                pcp->type = htons(OFPAT_SET_VLAN_PCP);
                pcp->len = htons(sizeof(struct ofp_action_vlan_pcp));
                pcp->vlan_pcp = pktout->action[i].vlan_pcp;

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
                memmove(mac->dl_addr, pktout->action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_DST_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + size);

                mac->type = htons(OFPAT_SET_DL_DST);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, pktout->action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_SRC_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_SRC);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = pktout->action[i].ip_addr;

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_DST_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_DST);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = pktout->action[i].ip_addr;

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_IP_TOS:
            {
                struct ofp_action_nw_tos *tos = (struct ofp_action_nw_tos *)(pkt + size);

                tos->type = htons(OFPAT_SET_NW_TOS);
                tos->len = htons(sizeof(struct ofp_action_nw_tos));
                tos->nw_tos = pktout->action[i].ip_tos;

                size += sizeof(struct ofp_action_nw_tos);
            }
            break;
        case ACTION_SET_SRC_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_SRC);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                uint16_t pt = pktout->action[i].port;
                port->tp_port = htons(pt);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        case ACTION_SET_DST_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_DST);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                uint16_t pt = pktout->action[i].port;
                port->tp_port = htons(pt);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        default:
            break;
        }
    }

    out->actions_len = htons(size - sizeof(struct ofp_packet_out));

    if (pktout->buffer_id == (uint32_t)-1 && pktout->total_len > 0) {
        memmove(pkt + size, pktout->data, MIN(pktout->total_len, __MAX_RAW_DATA_LEN - size));
        size += MIN(pktout->total_len, __MAX_RAW_DATA_LEN - size);
    }

    out->header.length = htons(size);

    raw_msg_t msg = {0};

    switch_t sw = {0};
    sw.dpid = pktout->dpid;
    ev_sw_get_fd(OFP_ID, &sw);

    msg.fd = sw.fd;
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
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_flow_mod *mod = (struct ofp_flow_mod *)pkt;
    int size = sizeof(struct ofp_flow_mod);

    mod->header.version = OFP_VERSION;
    mod->header.type = OFPT_FLOW_MOD;
    mod->header.xid = htonl(flow->xid);

    // begin of match

    if (flow->wildcards & WCARD_PORT)
        mod->match.wildcards |= OFPFW_IN_PORT;
    else if (flow->port > 0)
        mod->match.in_port = htons(flow->port);
    else
        mod->match.wildcards |= OFPFW_IN_PORT;

    //

    if (flow->wildcards & WCARD_SRC_MAC)
        mod->match.wildcards |= OFPFW_DL_SRC;
    else if (mac2int(flow->src_mac) > 0)
        memmove(mod->match.dl_src, flow->src_mac, ETH_ALEN);
    else
        mod->match.wildcards |= OFPFW_DL_SRC;

    if (flow->wildcards & WCARD_DST_MAC)
        mod->match.wildcards |= OFPFW_DL_DST;
    else if (mac2int(flow->dst_mac) > 0)
        memmove(mod->match.dl_dst, flow->dst_mac, ETH_ALEN);
    else
        mod->match.wildcards |= OFPFW_DL_DST;

    //

    if (flow->wildcards & WCARD_VLAN) {
        mod->match.wildcards |= OFPFW_DL_VLAN;

        if (flow->wildcards & WCARD_VLAN_PCP)
            mod->match.wildcards |= OFPFW_DL_VLAN_PCP;
        else if (flow->vlan_pcp > 0)
            mod->match.dl_vlan_pcp = flow->vlan_pcp;
        else
            mod->match.wildcards |= OFPFW_DL_VLAN_PCP;
    } else if (flow->proto & PROTO_VLAN) {
        mod->match.dl_vlan = htons(flow->vlan_id);

        if (flow->wildcards & WCARD_VLAN_PCP)
            mod->match.wildcards |= OFPFW_DL_VLAN_PCP;
        else if (flow->vlan_pcp > 0)
            mod->match.dl_vlan_pcp = flow->vlan_pcp;
        else
            mod->match.wildcards |= OFPFW_DL_VLAN_PCP;
    } else {
        mod->match.dl_vlan = htons(VLAN_NONE);
    }

    //

    if (flow->wildcards & WCARD_ETH_TYPE) {
        mod->match.wildcards |= OFPFW_DL_TYPE;
    } else if (flow->proto & PROTO_LLDP) { // LLDP
        mod->match.dl_type = htons(0x88cc);
    } else if (flow->proto & PROTO_ARP) { // ARP
        mod->match.dl_type = htons(0x0806);

        if (flow->wildcards & WCARD_SRC_IP)
            mod->match.wildcards |= OFPFW_NW_SRC_ALL;
        else if (flow->src_ip > 0)
            mod->match.nw_src = flow->src_ip;
        else
            mod->match.wildcards |= OFPFW_NW_SRC_ALL;

        if (flow->wildcards & WCARD_DST_IP)
            mod->match.wildcards |= OFPFW_NW_DST_ALL;
        else if (flow->dst_ip > 0)
            mod->match.nw_dst = flow->dst_ip;
        else
            mod->match.wildcards |= OFPFW_NW_DST_ALL;

        if (flow->wildcards & WCARD_OPCODE)
            mod->match.wildcards |= OFPFW_NW_PROTO;
        else if (flow->opcode > 0)
            mod->match.nw_proto = htons(flow->opcode);
        else
            mod->match.wildcards |= OFPFW_NW_PROTO;
    } else if (flow->proto & (PROTO_IPV4 | PROTO_TCP | PROTO_UDP | PROTO_ICMP)) { // IPv4
        mod->match.dl_type = htons(0x0800);

        if (flow->wildcards & WCARD_IP_TOS)
            mod->match.wildcards |= OFPFW_NW_TOS;
        else if (flow->ip_tos > 0)
            mod->match.nw_tos = flow->ip_tos;
        else
            mod->match.wildcards |= OFPFW_NW_TOS;

        if (flow->wildcards & WCARD_SRC_IP)
            mod->match.wildcards |= OFPFW_NW_SRC_ALL;
        else if (flow->src_ip > 0)
            mod->match.nw_src = flow->src_ip;
        else
            mod->match.wildcards |= OFPFW_NW_SRC_ALL;

        if (flow->wildcards & WCARD_DST_IP)
            mod->match.wildcards |= OFPFW_NW_DST_ALL;
        else if (flow->dst_ip > 0)
            mod->match.nw_dst = flow->dst_ip;
        else
            mod->match.wildcards |= OFPFW_NW_DST_ALL;

        if (flow->wildcards & WCARD_PROTO)
            mod->match.wildcards |= OFPFW_NW_PROTO;
        else if (flow->proto & (PROTO_TCP | PROTO_UDP | PROTO_ICMP)) { // TCP or UDP or ICMP
            if (flow->proto & PROTO_TCP)
                mod->match.nw_proto = IPPROTO_TCP;
            else if (flow->proto & PROTO_UDP)
                mod->match.nw_proto = IPPROTO_UDP;
            else if (flow->proto & PROTO_ICMP)
                mod->match.nw_proto = IPPROTO_ICMP;

            if (flow->wildcards & WCARD_SRC_PORT)
                mod->match.wildcards |= OFPFW_TP_SRC;
            else if (flow->src_port > 0)
                mod->match.tp_src = htons(flow->src_port);
            else
                mod->match.wildcards |= OFPFW_TP_SRC;

            if (flow->wildcards & WCARD_DST_PORT)
                mod->match.wildcards |= OFPFW_TP_DST;
            else if (flow->dst_port > 0)
                mod->match.tp_dst = htons(flow->dst_port);
            else
                mod->match.wildcards |= OFPFW_TP_DST;
        } else {
            mod->match.wildcards |= OFPFW_NW_PROTO;
        }
    } else {
        mod->match.wildcards |= OFPFW_DL_TYPE;
    }

    mod->match.wildcards = htonl(mod->match.wildcards);

    // end of match

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

    mod->buffer_id = htonl(flow->buffer_id);
    mod->out_port = htons(OFPP_NONE);

#if 0
    if (flow->flags & FLAG_SEND_REMOVED)
        mod->flags |= OFPFF_SEND_FLOW_REM;
#else
    mod->flags |= OFPFF_SEND_FLOW_REM;
#endif

    if (flow->flags & FLAG_CHECK_OVERLAP)
        mod->flags |= OFPFF_CHECK_OVERLAP;
    if (flow->flags & FLAG_EMERGENCY)
        mod->flags |= OFPFF_EMERG;

    mod->flags = htons(mod->flags);

    int i;
    for (i=0; i<flow->num_actions; i++) {
        switch (flow->action[i].type) {
        case ACTION_DISCARD:
            {
                size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_OUTPUT:
            {
                struct ofp_action_output *output = (struct ofp_action_output *)(pkt + size);

                output->type = htons(OFPAT_OUTPUT);
                output->len = htons(sizeof(struct ofp_action_output));

                if (flow->action[i].port == PORT_FLOOD)
                    output->port = htons(OFPP_FLOOD);
                else if (flow->action[i].port == PORT_CONTROLLER)
                    output->port = htons(OFPP_CONTROLLER);
                else if (flow->action[i].port == PORT_ALL)
                    output->port = htons(OFPP_ALL);
                else if (flow->action[i].port == PORT_NONE)
                    output->port = htons(OFPP_NONE);
                else {
                    output->port = htons(flow->action[i].port);
                }

                output->max_len = 0;

                size += sizeof(struct ofp_action_output);
            }
            break;
        case ACTION_SET_VLAN_VID:
            {
                struct ofp_action_vlan_vid *vid = (struct ofp_action_vlan_vid *)(pkt + size);

                vid->type = htons(OFPAT_SET_VLAN_VID);
                vid->len = htons(sizeof(struct ofp_action_vlan_vid));
                vid->vlan_vid = htons(flow->action[i].vlan_id);

                size += sizeof(struct ofp_action_vlan_vid);
            }
            break;
        case ACTION_SET_VLAN_PCP:
            {
                struct ofp_action_vlan_pcp *pcp = (struct ofp_action_vlan_pcp *)(pkt + size);

                pcp->type = htons(OFPAT_SET_VLAN_PCP);
                pcp->len = htons(sizeof(struct ofp_action_vlan_pcp));
                pcp->vlan_pcp = flow->action[i].vlan_pcp;

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
                memmove(mac->dl_addr, flow->action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_DST_MAC:
            {
                struct ofp_action_dl_addr *mac = (struct ofp_action_dl_addr *)(pkt + size);

                mac->type = htons(OFPAT_SET_DL_DST);
                mac->len = htons(sizeof(struct ofp_action_dl_addr));
                memmove(mac->dl_addr, flow->action[i].mac_addr, ETH_ALEN);

                size += sizeof(struct ofp_action_dl_addr);
            }
            break;
        case ACTION_SET_SRC_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_SRC);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = flow->action[i].ip_addr;

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_DST_IP:
            {
                struct ofp_action_nw_addr *ip = (struct ofp_action_nw_addr *)(pkt + size);

                ip->type = htons(OFPAT_SET_NW_DST);
                ip->len = htons(sizeof(struct ofp_action_nw_addr));
                ip->nw_addr = flow->action[i].ip_addr;

                size += sizeof(struct ofp_action_nw_addr);
            }
            break;
        case ACTION_SET_IP_TOS:
            {
                struct ofp_action_nw_tos *tos = (struct ofp_action_nw_tos *)(pkt + size);

                tos->type = htons(OFPAT_SET_NW_TOS);
                tos->len = htons(sizeof(struct ofp_action_nw_tos));
                tos->nw_tos = flow->action[i].ip_tos;

                size += sizeof(struct ofp_action_nw_tos);
            }
            break;
        case ACTION_SET_SRC_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_SRC);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                uint16_t pt = flow->action[i].port;
                port->tp_port = htons(pt);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        case ACTION_SET_DST_PORT:
            {
                struct ofp_action_tp_port *port = (struct ofp_action_tp_port *)(pkt + size);

                port->type = htons(OFPAT_SET_TP_DST);
                port->len = htons(sizeof(struct ofp_action_tp_port));
                uint16_t pt = flow->action[i].port;
                port->tp_port = htons(pt);

                size += sizeof(struct ofp_action_tp_port);
            }
            break;
        default:
            break;
        }
    }

    mod->header.length = htons(size);

    raw_msg_t msg;

    switch_t sw = {0};
    sw.dpid = flow->dpid;
    ev_sw_get_fd(OFP_ID, &sw);

    msg.fd = sw.fd;
    msg.length = size;

    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);

    ofp10_barrier_request(msg.fd);

    return 0;
}

/**
 * \brief Function to generate STATS_REQUEST (desc) messages
 * \param fd Socket
 */
static int ofp10_stats_desc_request(uint32_t fd)
{
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request);

    switch_t sw = {0};
    sw.fd = fd;
    ev_sw_get_xid(OFP_ID, &sw);

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = htons(sizeof(struct ofp_stats_request));
    request->header.xid = htonl(sw.xid);

    request->type = htons(OFPST_DESC);
    request->flags = htons(0);

    raw_msg_t out = {0};

    out.fd = fd;
    out.length = size;

    out.data = pkt;

    ev_ofp_msg_out(OFP_ID, &out);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (flow) messages
 * \param flow FLOW message
 */
static int ofp10_flow_stats(const flow_t *flow)
{
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    switch_t sw = {0};
    sw.dpid = flow->dpid;
    ev_sw_get_fd(OFP_ID, &sw);
    ev_sw_get_xid(OFP_ID, &sw);

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(sw.xid);

    request->type = htons(OFPST_FLOW);

    struct ofp_flow_stats_request *stat = (struct ofp_flow_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    // begin of match

    if (flow->wildcards & WCARD_PORT)
        stat->match.wildcards |= OFPFW_IN_PORT;
    else if (flow->port > 0)
        stat->match.in_port = htons(flow->port);
    else
        stat->match.wildcards |= OFPFW_IN_PORT;

    //

    if (flow->wildcards & WCARD_SRC_MAC)
        stat->match.wildcards |= OFPFW_DL_SRC;
    else if (mac2int(flow->src_mac) > 0)
        memmove(stat->match.dl_src, flow->src_mac, ETH_ALEN);
    else
        stat->match.wildcards |= OFPFW_DL_SRC;

    if (flow->wildcards & WCARD_DST_MAC)
        stat->match.wildcards |= OFPFW_DL_DST;
    else if (mac2int(flow->dst_mac) > 0)
        memmove(stat->match.dl_dst, flow->dst_mac, ETH_ALEN);
    else
        stat->match.wildcards |= OFPFW_DL_DST;

    //

    if (flow->wildcards & WCARD_VLAN) {
        stat->match.wildcards |= OFPFW_DL_VLAN;

        if (flow->wildcards & WCARD_VLAN_PCP)
            stat->match.wildcards |= OFPFW_DL_VLAN_PCP;
        else if (flow->vlan_pcp > 0)
            stat->match.dl_vlan_pcp = flow->vlan_pcp;
        else
            stat->match.wildcards |= OFPFW_DL_VLAN_PCP;
    } else if (flow->proto & PROTO_VLAN) {
        stat->match.dl_vlan = htons(flow->vlan_id);

        if (flow->wildcards & WCARD_VLAN_PCP)
            stat->match.wildcards |= OFPFW_DL_VLAN_PCP;
        else if (flow->vlan_pcp > 0)
            stat->match.dl_vlan_pcp = flow->vlan_pcp;
        else
            stat->match.wildcards |= OFPFW_DL_VLAN_PCP;
    } else {
        stat->match.dl_vlan = htons(VLAN_NONE);
    }

    //

    if (flow->wildcards & WCARD_ETH_TYPE) {
        stat->match.wildcards |= OFPFW_DL_TYPE;
    } else if (flow->proto & PROTO_LLDP) { // LLDP
        stat->match.dl_type = htons(0x88cc);
    } else if (flow->proto & PROTO_ARP) { // ARP
        stat->match.dl_type = htons(0x0806);

        if (flow->wildcards & WCARD_SRC_IP)
            stat->match.wildcards |= OFPFW_NW_SRC_ALL;
        else if (flow->src_ip > 0)
            stat->match.nw_src = flow->src_ip;
        else
            stat->match.wildcards |= OFPFW_NW_SRC_ALL;

        if (flow->wildcards & WCARD_DST_IP)
            stat->match.wildcards |= OFPFW_NW_DST_ALL;
        else if (flow->dst_ip > 0)
            stat->match.nw_dst = flow->dst_ip;
        else
            stat->match.wildcards |= OFPFW_NW_DST_ALL;

        if (flow->wildcards & WCARD_OPCODE)
            stat->match.wildcards |= OFPFW_NW_PROTO;
        else if (flow->opcode > 0)
            stat->match.nw_proto = htons(flow->opcode);
        else
            stat->match.wildcards |= OFPFW_NW_PROTO;
    } else if (flow->proto & (PROTO_IPV4 | PROTO_TCP | PROTO_UDP | PROTO_ICMP)) { // IPv4
        stat->match.dl_type = htons(0x0800);

        if (flow->wildcards & WCARD_IP_TOS)
            stat->match.wildcards |= OFPFW_NW_TOS;
        else if (flow->ip_tos > 0)
            stat->match.nw_tos = flow->ip_tos;
        else
            stat->match.wildcards |= OFPFW_NW_TOS;

        if (flow->wildcards & WCARD_SRC_IP)
            stat->match.wildcards |= OFPFW_NW_SRC_ALL;
        else if (flow->src_ip > 0)
            stat->match.nw_src = flow->src_ip;
        else
            stat->match.wildcards |= OFPFW_NW_SRC_ALL;

        if (flow->wildcards & WCARD_DST_IP)
            stat->match.wildcards |= OFPFW_NW_DST_ALL;
        else if (flow->dst_ip > 0)
            stat->match.nw_dst = flow->dst_ip;
        else
            stat->match.wildcards |= OFPFW_NW_DST_ALL;

        if (flow->wildcards & WCARD_PROTO)
            stat->match.wildcards |= OFPFW_NW_PROTO;
        else if (flow->proto & (PROTO_TCP | PROTO_UDP | PROTO_ICMP)) { // TCP or UDP or ICMP
            if (flow->proto & PROTO_TCP)
                stat->match.nw_proto = IPPROTO_TCP;
            else if (flow->proto & PROTO_UDP)
                stat->match.nw_proto = IPPROTO_UDP;
            else if (flow->proto & PROTO_ICMP)
                stat->match.nw_proto = IPPROTO_ICMP;

            if (flow->wildcards & WCARD_SRC_PORT)
                stat->match.wildcards |= OFPFW_TP_SRC;
            else if (flow->src_port > 0)
                stat->match.tp_src = htons(flow->src_port);
            else
                stat->match.wildcards |= OFPFW_TP_SRC;

            if (flow->wildcards & WCARD_DST_PORT)
                stat->match.wildcards |= OFPFW_TP_DST;
            else if (flow->dst_port > 0)
                stat->match.tp_dst = htons(flow->dst_port);
            else
                stat->match.wildcards |= OFPFW_TP_DST;
        } else {
            stat->match.wildcards |= OFPFW_NW_PROTO;
        }
    } else {
        stat->match.wildcards |= OFPFW_DL_TYPE;
    }

    stat->match.wildcards = htonl(stat->match.wildcards);

    // end of match

    stat->table_id = 0xff;
    stat->out_port = htons(OFPP_NONE);

    raw_msg_t msg;

    msg.fd = sw.fd;
    msg.length = size;

    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (aggregate) messages
 * \param flow FLOW message
 */
static int ofp10_aggregate_stats(const flow_t *flow)
{
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);

    switch_t sw = {0};
    sw.dpid = flow->dpid;
    ev_sw_get_fd(OFP_ID, &sw);
    ev_sw_get_xid(OFP_ID, &sw);

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(sw.xid);

    request->type = htons(OFPST_AGGREGATE);

    struct ofp_aggregate_stats_request *stat = (struct ofp_aggregate_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    // begin of match

    stat->match.wildcards = htonl(OFPFW_ALL);

    // end of match

    stat->table_id = 0xff;
    stat->out_port = htons(OFPP_NONE);

    raw_msg_t msg = {0};

    msg.fd = sw.fd;
    msg.length = size;

    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);

    return 0;
}

/**
 * \brief Function to process STATS_REQUEST (port) messages
 * \param port PORT message
 */
static int ofp10_port_stats(const port_t *port)
{
    uint8_t pkt[__MAX_RAW_DATA_LEN] = {0};
    struct ofp_stats_request *request = (struct ofp_stats_request *)pkt;
    int size = sizeof(struct ofp_stats_request) + sizeof(struct ofp_port_stats_request);

    switch_t sw = {0};
    sw.dpid = port->dpid;
    ev_sw_get_fd(OFP_ID, &sw);
    ev_sw_get_xid(OFP_ID, &sw);

    request->header.version = OFP_VERSION;
    request->header.type = OFPT_STATS_REQUEST;
    request->header.length = ntohs(size);
    request->header.xid = htonl(sw.xid);

    request->type = htons(OFPST_PORT);

    struct ofp_port_stats_request *stat = (struct ofp_port_stats_request *)(pkt + sizeof(struct ofp_stats_request));

    if (port->port == PORT_NONE)
        stat->port_no = htons(OFPP_NONE);
    else {
        stat->port_no = htons(port->port);
    }

    raw_msg_t msg = {0};

    msg.fd = sw.fd;
    msg.length = size;

    msg.data = pkt;

    ev_ofp_msg_out(OFP_ID, &msg);

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
        // nothing
        break;
    case OFPT_FEATURES_REQUEST:
        DEBUG("OFPT_FEATURES_REQUEST\n");
        // no way
        break;
    case OFPT_FEATURES_REPLY:
        DEBUG("OFPT_FEATURES_REPLY\n");
        ofp10_features_reply(msg);

        ofp10_set_config(msg->fd);
        ofp10_barrier_request(msg->fd);

        ofp10_get_config_request(msg->fd);
        ofp10_stats_desc_request(msg->fd);
        break;
    case OFPT_GET_CONFIG_REQUEST:
        DEBUG("OFPT_GET_CONFIG_REQUEST\n");
        // no way
        break;
    case OFPT_GET_CONFIG_REPLY:
        DEBUG("OFPT_GET_CONFIG_REPLY\n");
        ofp10_get_config_reply(msg);
        break;
    case OFPT_SET_CONFIG:
        DEBUG("OFPT_SET_CONFIG\n");
        // no way
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
        // nothing
        break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REQUEST\n");
        // no way
        break;
    case OFPT_QUEUE_GET_CONFIG_REPLY:
        DEBUG("OFPT_QUEUE_GET_CONFIG_REPLY\n");
        // nothing
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
 * \param cli The CLI pointer
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
            const raw_msg_t *msg = ev->msg;
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

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

/////////////////////////////////////////////////////////////////////

/** \brief The structure of the key context */
typedef struct _ctx_t ctx_t;

/** \brief The structure of the CLI context */
typedef struct cli_def cli_t;

/** \brief The structure of a component */
typedef struct _compnt_t compnt_t;

/** \brief The structure of component function pointers */
typedef struct _compnt_func_t compnt_func_t;

/** \brief The structure of a read-only event */
typedef struct _event_t event_t;

/** \brief The structure of a read-write event */
typedef struct _event_out_t event_out_t;

/** \brief The structure of an application */
typedef struct _app_t app_t;

/** \brief The structure of application function pointers */
typedef struct _app_func_t app_func_t;

/** \brief The structure of a read-only application event */
typedef struct _app_event_t app_event_t;

/** \brief The structure of a read-write application event */
typedef struct _app_event_out_t app_event_out_t;

/////////////////////////////////////////////////////////////////////

/** \brief The maximum length of the data field */
#define __MAX_RAW_DATA_LEN 1460

/** \brief The structure of a raw message */
typedef struct _raw_msg_t {
    uint32_t fd; /**< Network socket */
    uint16_t length; /**< The length of a message */

    uint8_t *data; /**< Pointer indicating the data */

    struct _raw_msg_t *prev, *next;
} raw_msg_t;

/** \brief The maximum length of data structures */
#define __MAX_STRUCT_SIZE 2048

/** \brief The structure of a message */
typedef struct _msg_t {
    uint32_t id; /**< Trigger ID */
    uint16_t type; /**< Event type */

    uint8_t data[__MAX_STRUCT_SIZE]; /**< Data */

    int ret; /**< Return value */

    struct _msg_t *prev, *next;
} msg_t;

/////////////////////////////////////////////////////////////////////

enum sw_capabilities {
    SC_FLOW_STATS  = 1 << 0,
    SC_TABLE_STATS = 1 << 1,
    SC_PORT_STATS  = 1 << 2,
    SC_QUEUE_STATS = 1 << 3,
};

/** \brief The structure of a switch */
typedef struct _switch_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Empty, only for ODP */

    uint32_t fd; /**< Socket */
    uint32_t xid; /**< Transaction ID */

    uint32_t remote; /**< Remote events, set by cluster */
    uint8_t version; /**< OpenFlow version */

    uint8_t n_tables; /**< The number of tables */
    uint32_t n_buffers; /**< The number of buffers */
    uint32_t capabilities; /**< The flags of capabilities */
    uint32_t actions; /**< The bitmap of actions */

    char mfr_desc[256]; /**< Manufacturer description */
    char hw_desc[256]; /**< Hardware description */
    char sw_desc[256]; /**< Software description */
    char serial_num[32]; /**< Serial number */
    char dp_desc[256]; /**< Datapath description */

    uint64_t pkt_count; /**< Packet count */
    uint64_t byte_count; /**< Byte count */
    uint32_t flow_count; /**< Flow count */

    struct _switch_t *prev; /**< The previous entry in the switch list */
    struct _switch_t *next; /**< The next entry in the switch list */
} switch_t;

/** \brief The maximum number of ports */
#define __MAX_NUM_PORTS 128

/** \brief The length of a MAC address */
#define ETH_ALEN 6

enum port_config {
    PC_PORT_DOWN    = 1 << 0,
    PC_NO_FLOOD     = 1 << 1,
    PC_NO_FWD       = 1 << 2,
    PC_NO_PACKET_IN = 1 << 3,
};

enum port_state {
    PS_LINK_DOWN = 1 << 0,
};

enum port_features {
    PF_10MB_HD  = 1 << 0,
    PF_10MB_FD  = 1 << 1,
    PF_100MB_HD = 1 << 2,
    PF_100MB_FD = 1 << 3,
    PF_1GB_HD   = 1 << 4,
    PF_1GB_FD   = 1 << 5,
    PF_10GB_FD  = 1 << 6,
};

/** \brief The structure of a port */
typedef struct _port_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    // link info
    uint64_t target_dpid; /**< The target datapath ID */
    uint32_t target_port; /**< The target port */

    uint32_t remote; /**< Remote events */
    uint8_t version; /**< OpenFlow version */

    // base info
    uint8_t hw_addr[ETH_ALEN]; /**< MAC address */

    uint32_t config; /**< Port configuration (port_config) */
    uint32_t state; /**< Port state (port_state) */

    uint32_t curr; /**< Current features (port_features) */
    uint32_t advertised; /**< Features being advertised by the port (port_features) */
    uint32_t supported; /**< Features supported by the port (port_features) */
    uint32_t peer; /**< Features advertised by the peer (port_features) */

    // stat info
    uint64_t rx_packets; /**< rx packet count */
    uint64_t rx_bytes; /**< rx byte count */
    uint64_t tx_packets; /**< tx packet count */
    uint64_t tx_bytes; /**< tx byte count */

    uint64_t old_rx_packets; /**< raw rx packet count */
    uint64_t old_rx_bytes; /**< raw rx byte count */
    uint64_t old_tx_packets; /**< raw tx packet count */
    uint64_t old_tx_bytes; /**< raw tx byte count */

    struct _port_t *prev; /**< The previous entry in the port list */
    struct _port_t *next; /**< The next entry in the port list */
} port_t;

/** \brief The structure of a host */
typedef struct _host_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t ip; /**< IP address */
    uint64_t mac; /**< MAC address */

    uint32_t remote; /**< Remote events, set by cluster */

    struct _host_t *prev; /**< The previous entry in the host table */
    struct _host_t *next; /**< The next entry in the host table */

    struct _host_t *r_next; /**< The next entry for removal */
} host_t;

/** \brief Protocol masks */
enum proto_mask {
    PROTO_VLAN    = 1 << 0,
    PROTO_ARP     = 1 << 1,
    PROTO_LLDP    = 1 << 2,
    PROTO_IPV4    = 1 << 3,
    PROTO_TCP     = 1 << 4,
    PROTO_UDP     = 1 << 5,
    PROTO_ICMP    = 1 << 6,
    PROTO_UNKNOWN = 1 << 7,
};

/** \brief The max size of the raw packet kept in the pktin structure */
#define __MAX_PKT_SIZE 1514

/** \brief The structure of an incoming packet */
typedef struct _pktin_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number(in_port) */

    uint32_t xid; /**< Transaction ID */
    uint32_t buffer_id; /**< Buffer ID */

    uint8_t version; /**< OpenFlow version */
    uint8_t reason; /**< Packet-In reason */

    uint16_t proto; /**< Protocol */

    uint16_t vlan_id; /**< VLAN ID */
    uint8_t vlan_pcp; /**< VLAN priority */

    uint8_t src_mac[ETH_ALEN]; /**< Source MAC address */
    uint8_t dst_mac[ETH_ALEN]; /**< Destination MAC address */

    uint32_t src_ip; /**< Source IP address */
    uint32_t dst_ip; /**< Destination IP address */

    union {
        uint16_t src_port; /**< Source port */
        uint16_t type; /**< ICMP type */
        uint16_t opcode; /**< ARP opcode */
    };
    union {
        uint16_t dst_port; /**< Destination port */
        uint16_t code; /**< ICMP code */
    };

    uint16_t total_len; /**< The length of data */
    uint8_t data[__MAX_PKT_SIZE]; /**< Ethernet frame */
} pktin_t;

/** \brief The action list supported by Barista */
enum {
    ACTION_DISCARD      = 0,
    ACTION_OUTPUT       = 1 << 0,
    ACTION_SET_VLAN_VID = 1 << 1,
    ACTION_SET_VLAN_PCP = 1 << 2,
    ACTION_STRIP_VLAN   = 1 << 3,
    ACTION_SET_SRC_MAC  = 1 << 4,
    ACTION_SET_DST_MAC  = 1 << 5,
    ACTION_SET_SRC_IP   = 1 << 6,
    ACTION_SET_DST_IP   = 1 << 7,
    ACTION_SET_IP_TOS   = 1 << 8,
    ACTION_SET_SRC_PORT = 1 << 9,
    ACTION_SET_DST_PORT = 1 << 10,
};

/** \brief The maximum number of actions */
#define __MAX_NUM_ACTIONS 8

/** \brief The no VLAN ID */
#define VLAN_NONE 0xffff

/** \brief The special port list supported by Barista */
enum {
    PORT_NONE       = 1 << 12,
    PORT_ALL        = 1 << 13,
    PORT_CONTROLLER = 1 << 14,
    PORT_FLOOD      = 1 << 15,
};

/** \brief The structure of an action */
typedef struct _action_t {
    uint16_t type; /**< action type */
    union {
        uint16_t port; /**< output, set_tp_src and dst */
        uint16_t vlan_id; /**< set_vlan_vid */
        uint8_t vlan_pcp; /**< set_vlan_pcp */
        uint8_t mac_addr[ETH_ALEN]; /**< set_dl_src and dst */
        uint32_t ip_addr; /**< set_nw_src and dst */
        uint8_t ip_tos; /**< set_nw_tos */
    }; 
} action_t;

/** \brief Function to initialize a pktout using pktin */
#define PKTOUT_INIT(x, y) { \
    x.dpid = y->dpid; \
    x.port = y->port; \
    x.xid = y->xid; \
    x.buffer_id = y->buffer_id; \
    x.version = y->version; \
}

/** \brief The structure of an outgoing packet */
typedef struct _pktout_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t xid; /**< Transaction ID */
    uint32_t buffer_id; /**< Buffer ID */

    uint8_t version; /**< OpenFlow version */

    uint16_t num_actions; /**< The number of actions */
    action_t action[__MAX_NUM_ACTIONS]; /**< Aactions */

    uint16_t total_len; /**< The length of raw data */
    uint8_t data[__MAX_PKT_SIZE]; /**< Ethernet frame (available when buffer_id = -1) */
} pktout_t;

/** \brief Default idle timeout */
#define DEFAULT_IDLE_TIMEOUT 10

/** \brief Default hard timeout */
#define DEFAULT_HARD_TIMEOUT 30

/** \brief Default priority */
#define DEFAULT_PRIORITY 0x8000

/** \brief Flow entry flags */
enum {
    FLAG_SEND_REMOVED  = 1 << 0,
    FLAG_CHECK_OVERLAP = 1 << 1,
    FLAG_EMERGENCY     = 1 << 2, // OpenFlow 1.0
};

/** \brief Flow wildcard masks */
enum flow_wildcards {
    WCARD_PORT     = 1 << 0,
    WCARD_VLAN     = 1 << 1,
    WCARD_VLAN_PCP = 1 << 2,
    WCARD_SRC_MAC  = 1 << 3,
    WCARD_DST_MAC  = 1 << 4,
    WCARD_ETH_TYPE = 1 << 5,
    WCARD_SRC_IP   = 1 << 6,
    WCARD_DST_IP   = 1 << 7,
    WCARD_IP_TOS   = 1 << 8,
    WCARD_PROTO    = 1 << 9,
    WCARD_SRC_PORT = 1 << 10,
    WCARD_DST_PORT = 1 << 11,
};

#define WCARD_OPCODE WCARD_SRC_PORT

/** \brief Flow commands */
enum flow_commands {
    FLOW_ADD,
    FLOW_MODIFY,
    FLOW_DELETE,
};

/** \brief Function to initialize a flow using pktin */
#define FLOW_INIT(x, y) { \
    x.dpid = y->dpid; \
    x.port = y->port; \
    x.xid = y->xid; \
    x.buffer_id = y->buffer_id; \
    x.idle_timeout = DEFAULT_IDLE_TIMEOUT; \
    x.hard_timeout = DEFAULT_HARD_TIMEOUT; \
    x.priority = DEFAULT_PRIORITY; \
    x.version = y->version; \
    x.vlan_id = y->vlan_id; \
    x.vlan_pcp = y->vlan_pcp; \
    x.proto = y->proto; \
    memmove(x.src_mac, y->src_mac, ETH_ALEN); \
    memmove(x.dst_mac, y->dst_mac, ETH_ALEN); \
    x.src_ip = y->src_ip; \
    x.dst_ip = y->dst_ip; \
    x.src_port = y->src_port; \
    x.dst_port = y->dst_port; \
}

/** \brief Function to compare two flows */
#define FLOW_COMPARE(x, y) ( \
    (x->dpid == y->dpid) && \
    (x->port == y->port) && \
    (x->wildcards == y->wildcards) && \
    (x->vlan_id == y->vlan_id) && \
    (x->vlan_pcp == y->vlan_pcp) && \
    (x->proto == y->proto) && \
    (memcmp(x->src_mac, y->src_mac, ETH_ALEN) == 0) && \
    (memcmp(x->dst_mac, y->dst_mac, ETH_ALEN) == 0) && \
    (x->src_ip == y->src_ip) && \
    (x->dst_ip == y->dst_ip) && \
    (x->src_port == y->src_port) && \
    (x->dst_port == y->dst_port) \
)

/** \brief The structure of a flow */
typedef struct _flow_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    // base info
    uint32_t xid; /**< Transaction ID */
    uint64_t cookie; /**< Cookie */
    uint32_t buffer_id; /**< Buffer ID */

    time_t insert_time; /**< Insert time (size: 8) */

    uint16_t idle_timeout; /**< Idle timeout */
    uint16_t hard_timeout; /**< Hard timeout */

    uint16_t flags; /**< Flow flags */
    uint16_t priority; /**< Flow priority */

    uint32_t remote; /**< Remote events */
    uint8_t version; /**< OpenFlow version */

    // match fields
    uint32_t wildcards; /**< Wildcard bitmasks */

    uint16_t vlan_id; /**< VLAN id */
    uint8_t vlan_pcp; /**< VLAN priority */

    uint16_t proto; /**< Protocol */
    uint8_t ip_tos; /**< IP type of service */

    uint8_t src_mac[ETH_ALEN]; /**< Source MAC address */
    uint8_t dst_mac[ETH_ALEN]; /**< Destination MAC address */

    uint32_t src_ip; /**< Source IP address */
    uint32_t dst_ip; /**< Destination IP address */

    union {
        uint16_t src_port; /**< Source port */
        uint16_t type; /**< ICMP type */
        uint16_t opcode; /**< ARP opcode */
    };
    union {
        uint16_t dst_port; /**< Destination port */
        uint16_t code; /**< ICMP code */
    };

    // stats
    uint32_t duration_sec; /**< Duration */
    uint32_t duration_nsec; /**< Duration */

    uint64_t pkt_count; /**< Packet count */
    uint64_t byte_count; /**< Byte count */
    uint32_t flow_count; /**< Flow count for aggregate stat */

    // actions
    uint16_t num_actions; /**< The number of actions */
    action_t action[__MAX_NUM_ACTIONS]; /**< Actions */

    struct _flow_t *prev; /**< The previous flow rule */
    struct _flow_t *next; /**< The next flow rule */

    struct _flow_t *r_next; /**< The next entry for removal */
} flow_t;

/** \brief The structure of traffic usage */
typedef struct _traffic_t {
    // traffic (inbound)
    uint64_t in_pkt_cnt; /**< The packet count of incoming OpenFlow messages */
    uint64_t in_byte_cnt; /**< The byte count of incoming OpenFlow messages */

    // traffic (outbound)
    uint64_t out_pkt_cnt; /**< The packet count of outgoing OpenFlow messages */
    uint64_t out_byte_cnt; /**< The byte count of outgoing OpenFlow messages */
} traffic_t;

/** \brief The structure of resource usage */
typedef struct _resource_t {
    double cpu; /**< CPU usage */
    double mem; /**< Memory usage */
} resource_t;

/////////////////////////////////////////////////////////////////////

#define __NUM_OF_ODP_FIELDS 8

enum odp_flags {
    ODP_DPID  = 1 << 0,
    ODP_PORT  = 1 << 1,
    ODP_PROTO = 1 << 2,
    ODP_VLAN  = 1 << 3,
    ODP_SRCIP = 1 << 4,
    ODP_DSTIP = 1 << 5,
    ODP_SPORT = 1 << 6,
    ODP_DPORT = 1 << 7,
};

/** \brief The structure of an operator-defined policy */
typedef struct _odp_t {
    uint16_t flag; /**< Enabled options */

    // policy for common
    uint64_t dpid[__MAX_POLICY_ENTRIES]; /**< Datapath ID */
    uint16_t port[__MAX_POLICY_ENTRIES]; /**< Physical port */

    // policy for PACKET_IN messages
    uint16_t proto; /**< Protocol */
    uint16_t vlan[__MAX_POLICY_ENTRIES]; /**< VLAN ID */
    uint32_t srcip[__MAX_POLICY_ENTRIES]; /**< Source IP address */
    uint32_t dstip[__MAX_POLICY_ENTRIES]; /**< Destination IP address */
    uint16_t sport[__MAX_POLICY_ENTRIES]; /**< Source port number */
    uint16_t dport[__MAX_POLICY_ENTRIES]; /**< Destination port number */
} odp_t;

/////////////////////////////////////////////////////////////////////

/** \brief The kinds of meta event conditions */
enum {
    META_GT,
    META_GTE,
    META_LT,
    META_LTE,
    META_EQ,
};

/** \brief The structure of a meta event */
typedef struct meta_event_t {
    int event;
    int condition;
    int threshold;
    char cmd[__CONF_STR_LEN];
} meta_event_t;

/////////////////////////////////////////////////////////////////////

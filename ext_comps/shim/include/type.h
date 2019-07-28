/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

/////////////////////////////////////////////////////////////////////

/** \brief The structure of the key context */
typedef struct _ctx_t ctx_t;

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

/** \brief The structure of the CLI context */
typedef struct cli_def cli_t;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a database */
typedef MYSQL database_t;

/** \brief The structure of a query result */
typedef MYSQL_RES query_result_t;

/** \brief The structure of a query row */
typedef MYSQL_ROW query_row_t;

/////////////////////////////////////////////////////////////////////

/** \brief The maximum length of data structures */
#define __MAX_MSG_SIZE 2048

/** \brief The structure of a message */
typedef struct _msg_t {
    union {
        uint32_t id; /**< Trigger ID */
        uint32_t fd; /**< Network socket */
    };
    union {
        uint16_t type; /**< Event type */
        uint16_t length; /**< The length of a message */
    };
    int16_t ret; /**< Return value */

    uint8_t *data; /**< Data */
} msg_t;

/////////////////////////////////////////////////////////////////////

/** \brief The maximum length of external messages */
#define __MAX_EXT_MSG_SIZE 2048

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a switch connection */
typedef struct _switch_conn_t {
    uint32_t fd; /**< Socket */
    uint32_t xid; /**< Transaction ID */
} switch_conn_t;

/** \brief The structure of a set of switch descriptions */
typedef struct _switch_desc_t {
    char mfr_desc[256]; /**< Manufacturer description */
    char hw_desc[256]; /**< Hardware description */
    char sw_desc[256]; /**< Software description */
    char serial_num[32]; /**< Serial number */
    char dp_desc[256]; /**< Datapath description */
} switch_desc_t;

/** \brief The structure of switch statistics */
typedef struct _switch_stat_t {
    uint64_t pkt_count; /**< Packet count */
    uint64_t byte_count; /**< Byte count */
    uint32_t flow_count; /**< Flow count */

    uint64_t old_pkt_count; /**< Previous packet count */
    uint64_t old_byte_count; /**< Previous byte count */
    uint32_t old_flow_count; /**< Previous flow count */
} switch_stat_t;

/** \brief The structure of a switch */
typedef struct _switch_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Empty, only for ODP */

    uint32_t remote; /**< Remote events, set by cluster */

    switch_conn_t conn; /**< Switch connection */
    switch_desc_t desc; /**< Switch descriptions */
    switch_stat_t stat; /**< Switch statistics */
} switch_t;

/////////////////////////////////////////////////////////////////////

/** \brief The length of a MAC address */
#define ETH_ALEN 6

enum port_config {
    PTCF_PORT_DOWN = 1 << 0,
};

enum port_state {
    PTST_LINK_DOWN = 1 << 0,
};

enum port_features {
    PTFT_10MB_HD    = 1 << 0,
    PTFT_10MB_FD    = 1 << 1,
    PTFT_100MB_HD   = 1 << 2,
    PTFT_100MB_FD   = 1 << 3,
    PTFT_1GB_HD     = 1 << 4,
    PTFT_1GB_FD     = 1 << 5,
    PTFT_10GB_FD    = 1 << 6,
    PTFT_COPPER     = 1 << 7,
    PTFT_FIBER      = 1 << 8,
    PTFT_AUTONEG    = 1 << 9,
    PTFT_PAUSE      = 1 << 10,
    PTFT_PAUSE_ASYM = 1 << 11,
};

/** \brief The structure of a link */
typedef struct _port_link_t {
    uint64_t dpid; /**< The neighbor datapath ID */
    uint32_t port; /**< The neighbor port */
} port_link_t;

/** \brief The maxmimum length of a port name */
#define MAX_PORT_NAME_LEN 16

/** \brief The structure of port information */
typedef struct _port_info_t {
    char name[MAX_PORT_NAME_LEN]; /**< Port name */
    uint8_t hw_addr[ETH_ALEN]; /**< MAC address */
    uint32_t config; /**< Port configuration (port_config) */
    uint32_t state; /**< Port state (port_state) */
    uint32_t curr; /**< Current features (port_features) */
    uint32_t advertized; /**< Features being advertized by the port */
    uint32_t supported; /**< Features supported by the port */
    uint32_t peer; /**< Features advertized by peer */
} port_info_t;

/** \brief The structure of port statistics */
typedef struct _port_stat_t {
    uint64_t rx_packets; /**< rx packet count */
    uint64_t rx_bytes; /**< rx byte count */
    uint64_t tx_packets; /**< tx packet count */
    uint64_t tx_bytes; /**< tx byte count */

    uint64_t old_rx_packets; /**< Previous rx packet count */
    uint64_t old_rx_bytes; /**< Previous rx byte count */
    uint64_t old_tx_packets; /**< Previous tx packet count */
    uint64_t old_tx_bytes; /**< Previous tx byte count */
} port_stat_t;

/** \brief The structure of a port */
typedef struct _port_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t remote; /**< Remote events */

    port_info_t info; /**< Port information */
    port_link_t link; /**< Link information */
    port_stat_t stat; /**< Port statstics */
} port_t;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a host */
typedef struct _host_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t remote; /**< Remote events, set by cluster */

    uint64_t mac; /**< MAC address */
    uint32_t ip; /**< IP address */
} host_t;

/////////////////////////////////////////////////////////////////////

/** \brief Protocol masks */
enum proto_mask {
    PROTO_VLAN    = 1 << 0,
    PROTO_ARP     = 1 << 1,
    PROTO_LLDP    = 1 << 2,
    PROTO_IPV4    = 1 << 3,
    PROTO_TCP     = 1 << 4,
    PROTO_UDP     = 1 << 5,
    PROTO_ICMP    = 1 << 6,
    PROTO_DHCP    = 1 << 7,
    PROTO_UNKNOWN = 1 << 8,
};

/** \brief Packet-In Reason */
enum pktin_reason {
    PKIN_NO_MATCH,
    PKIN_ACTION,
};

/** \brief The max size of the raw packet kept in the pktin structure */
#define __MAX_PKT_SIZE 1514

/** \brief Broadcast MAC address */
#define __BROADCAST_MAC 0xffffffffffff

/** \brief The structure of a packet */
typedef struct _pkt_info_t {
    uint16_t proto; /**< Protocol */

    uint16_t vlan_id; /**< VLAN ID */
    uint8_t vlan_pcp; /**< VLAN priority */

    uint8_t ip_tos; /**< IP type of service */

    uint8_t src_mac[ETH_ALEN]; /**< Source MAC address */
    uint8_t dst_mac[ETH_ALEN]; /**< Destination MAC address */

    uint32_t src_ip; /**< Source IP address */
    uint32_t dst_ip; /**< Destination IP address */

    union {
        uint16_t src_port; /**< Source port */
        uint16_t icmp_type; /**< ICMP type */
        uint16_t opcode; /**< ARP opcode */
        uint16_t tp_src; /**< Source port */
    };

    union {
        uint16_t dst_port; /**< Destination port */
        uint16_t icmp_code; /**< ICMP code */
        uint16_t tp_dst; /**< Destination port */
    };

    uint32_t wildcards; /**< Wildcard bitmasks */
} pkt_info_t;

/** \brief The structure of an incoming packet */
typedef struct _pktin_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number(in_port) */

    uint32_t xid; /**< Transaction ID */
    uint32_t buffer_id; /**< Buffer ID */

    uint8_t reason; /**< Packet-In reason */

    pkt_info_t pkt_info; /**< Packet information */

    uint16_t total_len; /**< The length of data */
    uint8_t data[__MAX_PKT_SIZE]; /**< Ethernet frame */
} pktin_t;

/////////////////////////////////////////////////////////////////////

/** \brief The maximum number of actions */
#define __MAX_NUM_ACTIONS 8

/** \brief The no VLAN ID */
#define VLAN_NONE 0xffff

/** \brief The action list supported by Barista */
enum action_type {
    ACTION_DROP = 0,
    ACTION_OUTPUT,
    ACTION_SET_VLAN_VID,
    ACTION_SET_VLAN_PCP,
    ACTION_STRIP_VLAN,
    ACTION_SET_SRC_MAC,
    ACTION_SET_DST_MAC,
    ACTION_SET_SRC_IP,
    ACTION_SET_DST_IP,
    ACTION_SET_IP_TOS,
    ACTION_SET_SRC_PORT,
    ACTION_SET_DST_PORT,
    ACTION_VENDOR,
};

/** \brief The special port list supported by Barista */
enum special_port {
    PORT_MAX        = 0xff00, /**< Maximum number of physical ports */
    PORT_IN_PORT    = 0xfff8, /**< Send pback to the input port */
    PORT_TABLE      = 0xfff9, /**< Perform actions in flow table (?) */
    PORT_NORMAL     = 0xfffa, /**< Process with normal L2/L3 switching */
    PORT_FLOOD      = 0xfffb, /**< All physical ports except input port and those disabled by STP */
    PORT_ALL        = 0xfffc, /**< All physical ports except input port */
    PORT_CONTROLLER = 0xfffd, /**< Send to controller */
    PORT_LOCAL      = 0xfffe, /**< Local openflow port (?) */
    PORT_NONE       = 0xffff, /**< Not associated with a physical port */
};

/** \brief The structure of an action */
typedef struct _action_t {
    uint16_t type; /**< action type */
    union {
        uint16_t port; /**< output, set tp_src and dst */
        uint16_t vlan_id; /**< set vlan_vid */
        uint8_t vlan_pcp; /**< set vlan_pcp */
        uint8_t mac_addr[ETH_ALEN]; /**< set dl_src and dst */
        uint32_t ip_addr; /**< set nw_src and dst */
        uint8_t ip_tos; /**< set nw_tos */
        uint32_t vendor; /**< set vender */
    }; 
} action_t;

/////////////////////////////////////////////////////////////////////

/** \brief Function to initialize a pktout using pktin */
#define PKTOUT_INIT(x, y) { \
    x.dpid = y->dpid; \
    x.port = y->port; \
    x.xid = y->xid; \
    x.buffer_id = y->buffer_id; \
}

/** \brief The structure of an outgoing packet */
typedef struct _pktout_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t xid; /**< Transaction ID */
    uint32_t buffer_id; /**< Buffer ID */

    uint16_t num_actions; /**< The number of actions */
    action_t action[__MAX_NUM_ACTIONS]; /**< Actions */

    uint16_t total_len; /**< The length of raw data */
    uint8_t data[__MAX_PKT_SIZE]; /**< Ethernet frame (available when buffer_id = -1) */
} pktout_t;

/////////////////////////////////////////////////////////////////////

/** \brief Default idle timeout */
#define DEFAULT_IDLE_TIMEOUT 10

/** \brief Default hard timeout */
#define DEFAULT_HARD_TIMEOUT 30

/** \brief Default priority */
#define DEFAULT_PRIORITY 0x8000

/** \brief Flow entry flags */
enum flow_flags {
    FLFG_SEND_REMOVED  = 1 << 0,
    FLFG_CHECK_OVERLAP = 1 << 1,
    FLFG_EMERG         = 1 << 2,
};

/** \brief Flow wildcard masks */
enum flow_wildcards {
    FLWD_IN_PORT      = 1 << 0,
    FLWD_VLAN         = 1 << 1,
    FLWD_SRC_MAC      = 1 << 2,
    FLWD_DST_MAC      = 1 << 3,
    FLWD_ETH_TYPE     = 1 << 4,
    FLWD_IP_PROTO     = 1 << 5,
    FLWD_SRC_PORT     = 1 << 6,
    FLWD_DST_PORT     = 1 << 7,
    FLWD_SRC_IP_SHIFT = 8,
    FLWD_SRC_IP_BITS  = 6,
    FLWD_SRC_IP_MASK  = ((1 << FLWD_SRC_IP_BITS) - 1) << FLWD_SRC_IP_SHIFT,
    FLWD_SRC_IP_ALL   = 32 << FLWD_SRC_IP_SHIFT,
    FLWD_DST_IP_SHIFT = 14,
    FLWD_DST_IP_BITS  = 6,
    FLWD_DST_IP_MASK  = ((1 << FLWD_DST_IP_BITS) - 1) << FLWD_DST_IP_SHIFT,
    FLWD_DST_IP_ALL   = 32 << FLWD_DST_IP_SHIFT,
    FLWD_VLAN_PCP     = 1 << 20,
    FLWD_IP_TOS       = 1 << 21,
    FLWD_ALL          = ((1 << 22) - 1)
};

#define FLWD_ICMP_TYPE FLWD_SRC_PORT
#define FLWD_ICMP_CODE FLWD_DST_PORT

#define FLWD_OPCODE FLWD_SRC_PORT

/** \brief Flow commands */
enum flow_commands {
    FLOW_ADD,
    FLOW_MODIFY,
    FLOW_DELETE,
};

/** \brief Reason that a flow is removed */
enum flow_removed_reason {
    FLRM_IDLE_TIMEOUT,
    FLRM_HARD_TIMEOUT,
    FLRM_DELETE,
};

/** \brief Function to initialize a flow using pktin */
#define FLOW_INIT(x, y) { \
    x.dpid = y->dpid; \
    x.port = y->port; \
    x.info.xid = y->xid; \
    x.info.buffer_id = y->buffer_id; \
    x.meta.idle_timeout = DEFAULT_IDLE_TIMEOUT; \
    x.meta.hard_timeout = DEFAULT_HARD_TIMEOUT; \
    x.meta.priority = DEFAULT_PRIORITY; \
    memmove(&x.pkt_info, &y->pkt_info, sizeof(pkt_info_t)); \
}

/** \brief Function to compare two flows */
#define FLOW_COMPARE(x, y) ( \
    (x->dpid == y->dpid) && \
    (x->port == y->port) && \
    (memcmp(&x->pkt_info, &y->pkt_info, sizeof(pkt_info_t)) == 0) \
)

/** \brief The structure of flow information */
typedef struct _flow_info_t {
    uint32_t xid; /**< Transaction ID */
    uint32_t buffer_id; /**< Buffer ID */
} flow_info_t;

/** \brief The structure of flow metadata */
typedef struct _flow_meta_t {
    uint64_t cookie; /**< Cookie */

    uint16_t idle_timeout; /**< Idle timeout */
    uint16_t hard_timeout; /**< Hard timeout */
    uint16_t priority; /**< Flow priority */
    uint16_t flags; /**< Flow flags */

    uint8_t reason; /**< Flow-Removed reason */
} flow_meta_t;

/** \brief The structure of flow statistics */
typedef struct _flow_stat_t {
    uint32_t duration_sec; /**< Duration */
    uint32_t duration_nsec; /**< Duration */

    uint64_t pkt_count; /**< Packet count */
    uint64_t byte_count; /**< Byte count */
    uint32_t flow_count; /**< Flow count for aggregate stats */
} flow_stat_t;

/** \brief The structure of a flow */
typedef struct _flow_t {
    uint64_t dpid; /**< Datapath ID */
    uint32_t port; /**< Port number */

    uint32_t remote; /**< Remote events */

    flow_info_t info; /**< Flow information */
    flow_meta_t meta; /**< Flow metadata */

    union {
        pkt_info_t match; /**< Match fields */
        pkt_info_t pkt_info; /**< Packet information */
    };

    flow_stat_t stat; /**< Flow statistics */

    uint16_t num_actions; /**< The number of actions */
    action_t action[__MAX_NUM_ACTIONS]; /**< Actions */

    ////

    struct _flow_t *prev; /**< The previous flow rule */
    struct _flow_t *next; /**< The next flow rule */

    union {
        time_t insert_time; /**< Injection time */
        struct _flow_t *r_next; /**< The next entry for removal */
    };
} flow_t;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of traffic usage */
typedef struct _traffic_t {
    uint64_t in_pkt_cnt; /**< The packet count of incoming OpenFlow messages */
    uint64_t in_byte_cnt; /**< The byte count of incoming OpenFlow messages */
    uint64_t out_pkt_cnt; /**< The packet count of outgoing OpenFlow messages */
    uint64_t out_byte_cnt; /**< The byte count of outgoing OpenFlow messages */
} traffic_t;

/////////////////////////////////////////////////////////////////////

/** \brief The structure of resource usage */
typedef struct _resource_t {
    double cpu; /**< CPU usage */
    double mem; /**< Memory usage */
} resource_t;

/////////////////////////////////////////////////////////////////////

/** \brief The number of ODP fields */
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

/** \brief The maximum length of a command */
#define __META_CMD_LENGTH 256

enum {
    META_GT,
    META_GTE,
    META_LT,
    META_LTE,
    META_EQ,
};

/** \brief The structure of a meta event */
typedef struct meta_event_t {
    int event; /**< Target event */
    int condition; /**< Trigger condition */
    int threshold; /**< Trigger threshold */
    char cmd[__META_CMD_LENGTH]; /**< Command to execute */
} meta_event_t;

/////////////////////////////////////////////////////////////////////

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup events Event Handler
 * \brief Functions to manage events for components
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "event.h"
#include "component.h"
#include "cli.h"

/////////////////////////////////////////////////////////////////////

/** \brief The pointer of the context structure */
static ctx_t *ev_ctx;

/** \brief The running flag of the event handler */
static int ev_on;

/////////////////////////////////////////////////////////////////////

/** \brief The MQ contexts to pull events and to handle request-response events */
void *ev_pull_ctx, *ev_rep_ctx;

/** \brief The MQ socket to pull events */
void *ev_pull_sock;

/** \brief The MQ sockets for components and workers */
void *ev_rep_comp, *ev_rep_work;

/////////////////////////////////////////////////////////////////////

/** \brief Switch related trigger function (non-const) */
static int sw_rw_raise(uint32_t id, uint16_t type, uint16_t len, switch_t *data);
/** \brief Port related trigger function (non-const) */
//static int port_rw_raise(uint32_t id, uint16_t type, uint16_t len, port_t *data);
/** \brief Host related trigger function (non-const) */
//static int host_rw_raise(uint32_t id, uint16_t type, uint16_t len, host_t *data);
/** \brief Flow related trigger function (non-const) */
//static int flow_rw_raise(uint32_t id, uint16_t type, uint16_t len, flow_t *data);

/////////////////////////////////////////////////////////////////////

/** \brief Raw message related trigger function (const) */
static int raw_ev_raise(uint32_t id, uint16_t type, uint16_t len, const raw_msg_t *data);
/** \brief Message related trigger function (const) */
static int msg_ev_raise(uint32_t id, uint16_t type, uint16_t len, const msg_t *data);
/** \brief Switch related trigger function (const) */
static int sw_ev_raise(uint32_t id, uint16_t type, uint16_t len, const switch_t *data);
/** \brief Port related trigger function (const) */
static int port_ev_raise(uint32_t id, uint16_t type, uint16_t len, const port_t *data);
/** \brief Host related trigger function (const) */
static int host_ev_raise(uint32_t id, uint16_t type, uint16_t len, const host_t *data);
/** \brief Packet-In related trigger function (const) */
static int pktin_ev_raise(uint32_t id, uint16_t type, uint16_t len, const pktin_t *data);
/** \brief Packet-Out related trigger function (const) */
static int pktout_ev_raise(uint32_t id, uint16_t type, uint16_t len, const pktout_t *data);
/** \brief Flow related trigger function (const) */
static int flow_ev_raise(uint32_t id, uint16_t type, uint16_t len, const flow_t *data);
/** \brief Resource related trigger function (const) */
static int rs_ev_raise(uint32_t id, uint16_t type, uint16_t len, const resource_t *data);
/** \brief Traffic related trigger function (const) */
static int tr_ev_raise(uint32_t id, uint16_t type, uint16_t len, const traffic_t *data);
/** \brief Log related trigger function (const) */
static int log_ev_raise(uint32_t id, uint16_t type, uint16_t len, const char *data);

/////////////////////////////////////////////////////////////////////

#include "event_zmq_pack.h"

// Upstream events //////////////////////////////////////////////////

/** \brief EV_OFP_MSG_IN */
void ev_ofp_msg_in(uint32_t id, const raw_msg_t *data) { raw_ev_raise(id, EV_OFP_MSG_IN, sizeof(raw_msg_t), (const raw_msg_t *)data); }
/** \brief EV_DP_RECEIVE_PACKET */
void ev_dp_receive_packet(uint32_t id, const pktin_t *data) { pktin_ev_raise(id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), data); }
/** \brief EV_DP_FLOW_EXPIRED */
void ev_dp_flow_expired(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_DELETED */
void ev_dp_flow_deleted(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_FLOW_DELETED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_STATS */
void ev_dp_flow_stats(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_AGGREGATE_STATS */
void ev_dp_aggregate_stats(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_PORT_ADDED */
void ev_dp_port_added(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_PORT_ADDED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_MODIFIED */
void ev_dp_port_modified(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_PORT_MODIFIED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_DELETED */
void ev_dp_port_deleted(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_PORT_DELETED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_STATS */
void ev_dp_port_stats(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_PORT_STATS, sizeof(port_t), data); }

// Downstream events ////////////////////////////////////////////////

/** \brief EV_OFP_MSG_OUT */
void ev_ofp_msg_out(uint32_t id, const msg_t *data) { msg_ev_raise(id, EV_OFP_MSG_OUT, sizeof(msg_t), (const msg_t *)data); }
/** \brief EV_DP_SEND_PACKET */
void ev_dp_send_packet(uint32_t id, const pktout_t *data) { pktout_ev_raise(id, EV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief EV_DP_INSERT_FLOW */
void ev_dp_insert_flow(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_MODIFY_FLOW */
void ev_dp_modify_flow(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_DELETE_FLOW */
void ev_dp_delete_flow(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_DELETE_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_FLOW_STATS */
void ev_dp_request_flow_stats(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_AGGREGATE_STATS */
void ev_dp_request_aggregate_stats(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_PORT_STATS */
void ev_dp_request_port_stats(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), data); }

// Internal events (request-response) ///////////////////////////////

/** \brief EV_SW_GET_DPID (fd) */
void ev_sw_get_dpid(uint32_t id, switch_t *data) { sw_rw_raise(id, EV_SW_GET_DPID, sizeof(switch_t), data); }
/** \brief EV_SW_GET_FD (dpid) */
void ev_sw_get_fd(uint32_t id, switch_t *data) { sw_rw_raise(id, EV_SW_GET_FD, sizeof(switch_t), data); }
/** \brief EV_SW_GET_XID (fd or dpid) */
void ev_sw_get_xid(uint32_t id, switch_t *data) { sw_rw_raise(id, EV_SW_GET_XID, sizeof(switch_t), data); }

// Internal events (notification) ///////////////////////////////////

/** \brief EV_SW_NEW_CONN */
void ev_sw_new_conn(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_EXPIRED_CONN */
void ev_sw_expired_conn(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_CONNECTED */
void ev_sw_connected(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_DISCONNECTED */
void ev_sw_disconnected(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_CONFIG */
void ev_sw_update_config(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_UPDATE_CONFIG, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_DESC */
void ev_sw_update_desc(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_HOST_ADDED */
void ev_host_added(uint32_t id, const host_t *data) { host_ev_raise(id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_HOST_DELETED */
void ev_host_deleted(uint32_t id, const host_t *data) { host_ev_raise(id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_LINK_ADDED */
void ev_link_added(uint32_t id, const port_t *data) { port_ev_raise(id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_LINK_DELETED */
void ev_link_deleted(uint32_t id, const port_t *data) { port_ev_raise(id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_FLOW_ADDED */
void ev_flow_added(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_FLOW_MODIFIED */
void ev_flow_modified(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_MODIFIED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_FLOW_DELETED */
void ev_flow_deleted(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_RS_UPDATE_USAGE */
void ev_rs_update_usage(uint32_t id, const resource_t *data) { rs_ev_raise(id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)data); }
/** \brief EV_TR_UPDATE_STATS */
void ev_tr_update_stats(uint32_t id, const traffic_t *data) { tr_ev_raise(id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)data); }
/** \brief EV_LOG_UPDATE_MSGS */
void ev_log_update_msgs(uint32_t id, const char *data) { log_ev_raise(id, EV_LOG_UPDATE_MSGS, strlen((const char *)data), (const char *)data); }

// Log events ///////////////////////////////////////////////////////

/** \brief EV_LOG_DEBUG */
void ev_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_ev_raise(id, EV_LOG_DEBUG, strlen(log), log);
}
/** \brief EV_LOG_INFO */
void ev_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_ev_raise(id, EV_LOG_INFO, strlen(log), log);
}
/** \brief EV_LOG_WARN */
void ev_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_ev_raise(id, EV_LOG_WARN, strlen(log), log);
}
/** \brief EV_LOG_ERROR */
void ev_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_ev_raise(id, EV_LOG_ERROR, strlen(log), log);
}
/** \brief EV_LOG_FATAL */
void ev_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_ev_raise(id, EV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process meta events
 * \return NULL
 */
static void *meta_events(void *null)
{
    int *num_events = ev_ctx->num_events;
    meta_event_t *meta = ev_ctx->meta_event;

    while (1) {
        int ev_id;
        for (ev_id=0; ev_id<__MAX_EVENTS; ev_id++) {
            int num_event = num_events[ev_id];

            int j;
            for (j=0; j<__MAX_META_EVENTS; j++) {
                if (meta[j].event == ev_id) {
                    switch (meta[j].condition) {
                    case META_GT: // >
                        if (num_event > meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_GTE: // >=
                        if (num_event >= meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_LT: // <
                        if (num_event < meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_LTE: // <=
                        if (num_event <= meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_EQ: // ==
                        if (num_event == meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }

            num_events[ev_id] = 0;
        }

        waitsec(1, 0);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize the event handler
 * \param ctx The context of the Barista NOS
 * \return 0: OK, -1: Error
 */
int event_init(ctx_t *ctx)
{
    ev_ctx = ctx;

    if (ev_on == FALSE) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &meta_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }

        ev_on = TRUE;

        // pull (outside)

        ev_pull_ctx = zmq_ctx_new();
        ev_pull_sock = zmq_socket(ev_pull_ctx, ZMQ_PULL);

        const int timeout = 1000;
        zmq_setsockopt(ev_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

        if (zmq_bind(ev_pull_sock, EXT_COMP_PULL_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        int i;
        for (i=0; i<__NUM_THREADS; i++) {
            if (pthread_create(&thread, NULL, &receive_events, NULL) < 0) {
                PERROR("pthread_create");
                return -1;
            }
        }

        // reply

        ev_rep_ctx = zmq_ctx_new();
        ev_rep_comp = zmq_socket(ev_rep_ctx, ZMQ_ROUTER);
        if (zmq_bind(ev_rep_comp, EXT_COMP_REPLY_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        ev_rep_work = zmq_socket(ev_rep_ctx, ZMQ_DEALER);
        if (zmq_bind(ev_rep_work, "inproc://ev_reply_workers")) {
            PERROR("zmq_bind");
            return -1;
        }

        for (i=0; i<__NUM_THREADS; i++) {
            if (pthread_create(&thread, NULL, &reply_events, NULL) < 0) {
                PERROR("pthread_create");
                return -1;
            }
        }

        if (pthread_create(&thread, NULL, &reply_proxy, ctx) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

#define ODP_FUNC compare_switch
#define ODP_TYPE switch_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_port
#define ODP_TYPE port_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_host
#define ODP_TYPE host_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_pktin
#define ODP_TYPE pktin_t
#include "odp_all.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_pktout
#define ODP_TYPE pktout_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_flow
#define ODP_TYPE flow_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

/////////////////////////////////////////////////////////////////////

/**
 * \brief The code of sw_rw_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME sw_rw_raise
#define FUNC_TYPE switch_t
#define FUNC_DATA sw_data
#include "event_rw_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of port_rw_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME port_rw_raise
//#define FUNC_TYPE port_t
//#define FUNC_DATA port_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of host_rw_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME host_rw_raise
//#define FUNC_TYPE host_t
//#define FUNC_DATA host_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of flow_rw_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME flow_rw_raise
//#define FUNC_TYPE flow_t
//#define FUNC_DATA flow_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of raw_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME raw_ev_raise
#define FUNC_TYPE raw_msg_t
#define FUNC_DATA raw_msg
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of msg_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME msg_ev_raise
#define FUNC_TYPE msg_t
#define FUNC_DATA msg
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of sw_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME sw_ev_raise
#define FUNC_TYPE switch_t
#define FUNC_DATA sw
#define ODP_FUNC compare_switch
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of port_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME port_ev_raise
#define FUNC_TYPE port_t
#define FUNC_DATA port
#define ODP_FUNC compare_port
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of host_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME host_ev_raise
#define FUNC_TYPE host_t
#define FUNC_DATA host
#define ODP_FUNC compare_host
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of pktin_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME pktin_ev_raise
#define FUNC_TYPE pktin_t
#define FUNC_DATA pktin
#define ODP_FUNC compare_pktin
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of pktout_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME pktout_ev_raise
#define FUNC_TYPE pktout_t
#define FUNC_DATA pktout
#define ODP_FUNC compare_pktout
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of flow_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME flow_ev_raise
#define FUNC_TYPE flow_t
#define FUNC_DATA flow
#define ODP_FUNC compare_flow
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/** 
 * \brief The code of rs_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME rs_ev_raise
#define FUNC_TYPE resource_t
#define FUNC_DATA resource
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of tr_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME tr_ev_raise
#define FUNC_TYPE traffic_t
#define FUNC_DATA traffic
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of log_ev_raise()
 * \param id Component ID
 * \param type Event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME log_ev_raise
#define FUNC_TYPE char
#define FUNC_DATA log
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * @}
 *
 * @}
 */

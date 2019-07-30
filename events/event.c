/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
static ctx_t *ev_ctx;

/////////////////////////////////////////////////////////////////////

/** \brief MQ context to pull events */
void *ev_pull_ctx;

/** \brief MQ socket to pull events */
void *ev_pull_sock;

/** \brief MQ context to reply events */
void *ev_rep_ctx;

/** \brief MQ socket to reply events */
void *ev_rep_sock;

/////////////////////////////////////////////////////////////////////

#include "event_msg_pack.h"

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

// Upstream events //////////////////////////////////////////////////

/** \brief EV_OFP_MSG_IN */
void ev_ofp_msg_in(uint32_t id, const msg_t *data) { msg_ev_raise(id, EV_OFP_MSG_IN, sizeof(msg_t), data); }
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
void ev_ofp_msg_out(uint32_t id, const msg_t *data) { msg_ev_raise(id, EV_OFP_MSG_OUT, sizeof(msg_t), data); }
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
/** \brief EV_DP_MODIFY_PORT */
void ev_dp_modify_port(uint32_t id, const port_t *data) { port_ev_raise(id, EV_DP_MODIFY_PORT, sizeof(port_t), data); }
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
void ev_sw_new_conn(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_NEW_CONN, sizeof(switch_t), data); }
/** \brief EV_SW_ESTABLISHED_CONN */
void ev_sw_established_conn(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_ESTABLISHED_CONN, sizeof(switch_t), data); }
/** \brief EV_SW_EXPIRED_CONN */
void ev_sw_expired_conn(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_EXPIRED_CONN, sizeof(switch_t), data); }
/** \brief EV_SW_CONNECTED */
void ev_sw_connected(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_CONNECTED, sizeof(switch_t), data); }
/** \brief EV_SW_DISCONNECTED */
void ev_sw_disconnected(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_DISCONNECTED, sizeof(switch_t), data); }
/** \brief EV_SW_UPDATE_DESC */
void ev_sw_update_desc(uint32_t id, const switch_t *data) { sw_ev_raise(id, EV_SW_UPDATE_DESC, sizeof(switch_t), data); }
/** \brief EV_HOST_ADDED */
void ev_host_added(uint32_t id, const host_t *data) { host_ev_raise(id, EV_HOST_ADDED, sizeof(host_t), data); }
/** \brief EV_HOST_DELETED */
void ev_host_deleted(uint32_t id, const host_t *data) { host_ev_raise(id, EV_HOST_DELETED, sizeof(host_t), data); }
/** \brief EV_LINK_ADDED */
void ev_link_added(uint32_t id, const port_t *data) { port_ev_raise(id, EV_LINK_ADDED, sizeof(port_t), data); }
/** \brief EV_LINK_DELETED */
void ev_link_deleted(uint32_t id, const port_t *data) { port_ev_raise(id, EV_LINK_DELETED, sizeof(port_t), data); }
/** \brief EV_FLOW_ADDED */
void ev_flow_added(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_ADDED, sizeof(flow_t), data); }
/** \brief EV_FLOW_MODIFIED */
void ev_flow_modified(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_MODIFIED, sizeof(flow_t), data); }
/** \brief EV_FLOW_DELETED */
void ev_flow_deleted(uint32_t id, const flow_t *data) { flow_ev_raise(id, EV_FLOW_DELETED, sizeof(flow_t), data); }
/** \brief EV_RS_UPDATE_USAGE */
void ev_rs_update_usage(uint32_t id, const resource_t *data) { rs_ev_raise(id, EV_RS_UPDATE_USAGE, sizeof(resource_t), data); }
/** \brief EV_TR_UPDATE_STATS */
void ev_tr_update_stats(uint32_t id, const traffic_t *data) { tr_ev_raise(id, EV_TR_UPDATE_STATS, sizeof(traffic_t), data); }
/** \brief EV_LOG_UPDATE_MSGS */
void ev_log_update_msgs(uint32_t id, const char *data) { log_ev_raise(id, EV_LOG_UPDATE_MSGS, strlen(data), data); }

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
 * \param null NULL
 */
static void *meta_events(void *null)
{
    int *num_events = ev_ctx->num_events;
    meta_event_t *meta = ev_ctx->meta_event;

    while (ev_ctx->ev_on) {
        int ev_id;
        for (ev_id=0; ev_id<__MAX_EVENTS; ev_id++) {
            int num_event = num_events[ev_id];

            int j;
            for (j=0; j<__MAX_META_EVENTS; j++) {
                if (meta[j].event == ev_id) {
                    switch (meta[j].condition) {
                    case META_GT: // >
                        if (num_event > meta[j].threshold) {
                            //meta[j].cmd
                        }
                        break;
                    case META_GTE: // >=
                        if (num_event >= meta[j].threshold) {
                            //meta[j].cmd
                        }
                        break;
                    case META_LT: // <
                        if (num_event < meta[j].threshold) {
                            //meta[j].cmd
                        }
                        break;
                    case META_LTE: // <=
                        if (num_event <= meta[j].threshold) {
                            //meta[j].cmd
                        }
                        break;
                    case META_EQ: // ==
                        if (num_event == meta[j].threshold) {
                            //meta[j].cmd
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
 */
int init_event(ctx_t *ctx)
{
    ev_ctx = ctx;

    if (ev_ctx->ev_on == FALSE) {
        pthread_t thread;

        ev_ctx->ev_on = TRUE;

        // meta event

        if (pthread_create(&thread, NULL, &meta_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }

        // pull

        ev_pull_ctx = zmq_ctx_new();
        ev_pull_sock = zmq_socket(ev_pull_ctx, ZMQ_PULL);

        if (zmq_bind(ev_pull_sock, __EXT_COMP_PULL_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        int timeout = 1000;
        zmq_setsockopt(ev_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

        if (pthread_create(&thread, NULL, &pull_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }

        // reply

        ev_rep_ctx = zmq_ctx_new();
        ev_rep_sock = zmq_socket(ev_rep_ctx, ZMQ_REP);

        if (zmq_bind(ev_rep_sock, __EXT_COMP_REPLY_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        zmq_setsockopt(ev_rep_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

        if (pthread_create(&thread, NULL, &reply_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    DEBUG("event_handler is initialized\n");

    return 0;
}

/**
 * \brief Function to destroy the event handler
 * \param ctx The context of the Barista NOS
 */
int destroy_event(ctx_t *ctx)
{
    ctx->ev_on = FALSE;

    waitsec(1, 0);

    zmq_close(ev_pull_sock);
    zmq_close(ev_rep_sock);

    zmq_ctx_destroy(ev_pull_ctx);
    zmq_ctx_destroy(ev_rep_ctx);

    DEBUG("event_handler is destroyed\n");

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

#define FUNC_NAME sw_rw_raise
#define FUNC_TYPE switch_t
#define FUNC_DATA sw_data
#include "event_rw_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

//#define FUNC_NAME port_rw_raise
//#define FUNC_TYPE port_t
//#define FUNC_DATA port_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

//#define FUNC_NAME host_rw_raise
//#define FUNC_TYPE host_t
//#define FUNC_DATA host_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

//#define FUNC_NAME flow_rw_raise
//#define FUNC_TYPE flow_t
//#define FUNC_DATA flow_data
//#include "event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

#define FUNC_NAME msg_ev_raise
#define FUNC_TYPE msg_t
#define FUNC_DATA msg
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME sw_ev_raise
#define FUNC_TYPE switch_t
#define FUNC_DATA sw
#define ODP_FUNC compare_switch
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME port_ev_raise
#define FUNC_TYPE port_t
#define FUNC_DATA port
#define ODP_FUNC compare_port
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME host_ev_raise
#define FUNC_TYPE host_t
#define FUNC_DATA host
#define ODP_FUNC compare_host
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME pktin_ev_raise
#define FUNC_TYPE pktin_t
#define FUNC_DATA pktin
#define ODP_FUNC compare_pktin
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME pktout_ev_raise
#define FUNC_TYPE pktout_t
#define FUNC_DATA pktout
#define ODP_FUNC compare_pktout
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME flow_ev_raise
#define FUNC_TYPE flow_t
#define FUNC_DATA flow
#define ODP_FUNC compare_flow
#include "event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME rs_ev_raise
#define FUNC_TYPE resource_t
#define FUNC_DATA resource
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

#define FUNC_NAME tr_ev_raise
#define FUNC_TYPE traffic_t
#define FUNC_DATA traffic
#include "event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

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

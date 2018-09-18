/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt_shim
 * @{
 * \defgroup compnt_event Component Event Handler
 * \brief Functions to manage events for external components
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "event.h"
#include "component.h"
#include "component_info.h"

#include <zmq.h>

/////////////////////////////////////////////////////////////////////

/** \brief The configuration of a target component */
extern compnt_t compnt;

/** \brief The running flag for event workers */
int ev_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief The contexts to push and request events */
void *ev_push_ctx, *ev_req_ctx;

/** \brief The sockets to push and request events */
void *ev_push_sock, *ev_req_sock;

/////////////////////////////////////////////////////////////////////

/** \brief The contexts to pull and reply events */
void *ev_pull_ctx, *ev_rep_ctx;

/** \brief The sockets to pull and reply events */
void *ev_pull_sock, *ev_rep_sock;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send events to the Barista NOS
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param data The pointer of the given data
 */
static int ev_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *data)
{
    msg_t msg = {0};
    msg.id = id;
    msg.type = type;
    memmove(msg.data, data, size);

    char *str = base64_encode((char *)&msg, sizeof(msg_t));
    zmq_send(ev_push_sock, str, strlen(str), 0);
    FREE(str);

    return 0;
}

// Upstream events //////////////////////////////////////////////////

/** \brief EV_OFP_MSG_IN */
void ev_ofp_msg_in(uint32_t id, const raw_msg_t *data) { EV_PUSH_MSG(id, EV_OFP_MSG_IN, sizeof(raw_msg_t), (const raw_msg_t *)data); }
/** \brief EV_DP_RECEIVE_PACKET */
void ev_dp_receive_packet(uint32_t id, const pktin_t *data) { EV_PUSH_MSG(id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), data); }
/** \brief EV_DP_FLOW_EXPIRED */
void ev_dp_flow_expired(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_DELETED */
void ev_dp_flow_deleted(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_FLOW_DELETED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_STATS */
void ev_dp_flow_stats(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_AGGREGATE_STATS */
void ev_dp_aggregate_stats(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_PORT_ADDED */
void ev_dp_port_added(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_DP_PORT_ADDED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_MODIFIED */
void ev_dp_port_modified(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_DP_PORT_MODIFIED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_DELETED */
void ev_dp_port_deleted(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_DP_PORT_DELETED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_STATS */
void ev_dp_port_stats(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_DP_PORT_STATS, sizeof(port_t), data); }

// Downstream events ////////////////////////////////////////////////

/** \brief EV_OFP_MSG_OUT */
void ev_ofp_msg_out(uint32_t id, const raw_msg_t *data) { EV_PUSH_MSG(id, EV_OFP_MSG_OUT, sizeof(raw_msg_t), (const raw_msg_t *)data); }
/** \brief EV_DP_SEND_PACKET */
void ev_dp_send_packet(uint32_t id, const pktout_t *data) { EV_PUSH_MSG(id, EV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief EV_DP_INSERT_FLOW */
void ev_dp_insert_flow(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_MODIFY_FLOW */
void ev_dp_modify_flow(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_DELETE_FLOW */
void ev_dp_delete_flow(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_DELETE_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_FLOW_STATS */
void ev_dp_request_flow_stats(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_AGGREGATE_STATS */
void ev_dp_request_aggregate_stats(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_PORT_STATS */
void ev_dp_request_port_stats(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), data); }

// Internal events (request-response) ///////////////////////////////

/** \brief EV_SW_GET_DPID (fd) */
void ev_sw_get_dpid(uint32_t id, switch_t *data) { EV_WRITE_MSG(id, EV_SW_GET_DPID, sizeof(switch_t), data); }
/** \brief EV_SW_GET_FD (dpid) */
void ev_sw_get_fd(uint32_t id, switch_t *data) { EV_WRITE_MSG(id, EV_SW_GET_FD, sizeof(switch_t), data); }
/** \brief EV_SW_GET_XID (fd or dpid) */
void ev_sw_get_xid(uint32_t id, switch_t *data) { EV_WRITE_MSG(id, EV_SW_GET_XID, sizeof(switch_t), data); }

// Internal events (notification) ///////////////////////////////////

/** \brief EV_SW_NEW_CONN */
void ev_sw_new_conn(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_EXPIRED_CONN */
void ev_sw_expired_conn(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_CONNECTED */
void ev_sw_connected(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_DISCONNECTED */
void ev_sw_disconnected(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_CONFIG */
void ev_sw_update_config(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_UPDATE_CONFIG, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_DESC */
void ev_sw_update_desc(uint32_t id, const switch_t *data) { EV_PUSH_MSG(id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_HOST_ADDED */
void ev_host_added(uint32_t id, const host_t *data) { EV_PUSH_MSG(id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_HOST_DELETED */
void ev_host_deleted(uint32_t id, const host_t *data) { EV_PUSH_MSG(id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_LINK_ADDED */
void ev_link_added(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_LINK_DELETED */
void ev_link_deleted(uint32_t id, const port_t *data) { EV_PUSH_MSG(id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_FLOW_ADDED */
void ev_flow_added(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_FLOW_DELETED */
void ev_flow_deleted(uint32_t id, const flow_t *data) { EV_PUSH_MSG(id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_RS_UPDATE_USAGE */
void ev_rs_update_usage(uint32_t id, const resource_t *data) { EV_PUSH_MSG(id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)data); }
/** \brief EV_TR_UPDATE_STATS */
void ev_tr_update_stats(uint32_t id, const traffic_t *data) { EV_PUSH_MSG(id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)data); }
/** \brief EV_LOG_UPDATE_MSGS */
void ev_log_update_msgs(uint32_t id, const char *data) { EV_PUSH_MSG(id, EV_LOG_UPDATE_MSGS, strlen((const char *)data), (const char *)data); }

// Log events ///////////////////////////////////////////////////////

/** \brief EV_LOG_DEBUG */
void ev_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    EV_PUSH_MSG(id, EV_LOG_DEBUG, strlen(log), log);
}
/** \brief EV_LOG_INFO */
void ev_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    EV_PUSH_MSG(id, EV_LOG_INFO, strlen(log), log);
}
/** \brief EV_LOG_WARN */
void ev_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    EV_PUSH_MSG(id, EV_LOG_WARN, strlen(log), log);
}
/** \brief EV_LOG_ERROR */
void ev_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    EV_PUSH_MSG(id, EV_LOG_ERROR, strlen(log), log);
}
/** \brief EV_LOG_FATAL */
void ev_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    EV_PUSH_MSG(id, EV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process events in an event queue and reply outputs
 * \return NULL
 */
static void *reply_events(void *null)
{
    waitsec(1, 0);

    while (ev_worker_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
	zmq_recv(ev_rep_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

	if (!ev_worker_on) break;

	char *decoded = base64_decode(buff);
	msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
	else if (msg->type > EV_NUM_EVENTS) continue;

        event_out_t out = {0};

        out.id = msg->id;
        out.type = msg->type;
        out.data = (char *)msg->data;

        const event_t *in = (const event_t *)&out;
        int ret = 0;

        switch (out.type) {

        // upstream events

        case EV_OFP_MSG_IN:
            out.length = sizeof(raw_msg_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_EXPIRED:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_STATS:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_PORT_ADDED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_PORT_MODIFIED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_PORT_STATS:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;

        // downstream events

        case EV_OFP_MSG_OUT:
            out.length = sizeof(raw_msg_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_SEND_PACKET:
            out.length = sizeof(pktout_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_INSERT_FLOW:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_MODIFY_FLOW:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_DELETE_FLOW:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_FLOW_STATS:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_PORT_STATS:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;

        // internal events (request-response)

        case EV_SW_GET_DPID:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_GET_FD:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_GET_XID:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;

        // internal events (notification)

        case EV_SW_NEW_CONN:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_EXPIRED_CONN:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_CONNECTED:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_UPDATE_CONFIG:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_SW_UPDATE_DESC:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_HOST_ADDED:
            out.length = sizeof(host_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_HOST_DELETED:
            out.length = sizeof(host_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_LINK_ADDED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_LINK_DELETED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_FLOW_ADDED:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_RS_UPDATE_USAGE:
            out.length = sizeof(resource_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_TR_UPDATE_STATS:
            out.length = sizeof(traffic_t);
            ret = compnt.handler(in, &out);
            break;

        default:
            break;
        }

        FREE(decoded);

        msg->ret = ret;

        char *str = base64_encode((char *)msg, sizeof(msg_t));
        zmq_send(ev_rep_sock, str, strlen(str), 0);
	FREE(str);

        if (!ev_worker_on) break;
    }

    return NULL;
}

/**
 * \brief Function to process events in an app event queue
 * \return NULL
 */
static void *deliver_events(void *null)
{
    waitsec(1, 0);

    while (ev_worker_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
	zmq_recv(ev_pull_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!ev_worker_on) break;

        char *decoded = base64_decode(buff);
	msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
	else if (msg->type > EV_NUM_EVENTS) continue;

        event_out_t out = {0};

        out.id = msg->id;
        out.type = msg->type;
        out.data = (char *)msg->data;

        const event_t *in = (const event_t *)&out;

        switch (out.type) {

        // upstream events

        case EV_OFP_MSG_IN:
            out.length = sizeof(raw_msg_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_EXPIRED:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_FLOW_STATS:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_PORT_ADDED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_PORT_MODIFIED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_PORT_STATS:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;

        // downstream events

        case EV_OFP_MSG_OUT:
            out.length = sizeof(raw_msg_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_SEND_PACKET:
            out.length = sizeof(pktout_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_INSERT_FLOW:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_MODIFY_FLOW:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_DELETE_FLOW:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_FLOW_STATS:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_REQUEST_PORT_STATS:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;

        // internal events (request-response)

        case EV_SW_GET_DPID:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_GET_FD:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_GET_XID:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;

        // internal events (notification)

        case EV_SW_NEW_CONN:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_EXPIRED_CONN:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_CONNECTED:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_UPDATE_CONFIG:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_SW_UPDATE_DESC:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_HOST_ADDED:
            out.length = sizeof(host_t);
            compnt.handler(in, &out);
            break;
        case EV_HOST_DELETED:
            out.length = sizeof(host_t);
            compnt.handler(in, &out);
            break;
        case EV_LINK_ADDED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_LINK_DELETED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_FLOW_ADDED:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_RS_UPDATE_USAGE:
            out.length = sizeof(resource_t);
            compnt.handler(in, &out);
            break;
        case EV_TR_UPDATE_STATS:
            out.length = sizeof(traffic_t);
            compnt.handler(in, &out);
            break;

        default:
            break;
        }

	FREE(decoded);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to destroy an event queue
 * \param ctx Context (= NULL)
 */
int destroy_ev_workers(ctx_t *ctx)
{
    ev_worker_on = FALSE;

    waitsec(1, 0);

    zmq_close(ev_push_sock);
    zmq_ctx_destroy(ev_push_ctx);

    zmq_close(ev_pull_sock);
    zmq_ctx_destroy(ev_pull_ctx);

    zmq_close(ev_req_sock);
    //zmq_ctx_destroy(ev_req_ctx);

    zmq_close(ev_rep_sock);
    zmq_ctx_destroy(ev_rep_ctx);

    return 0;
}

/**
 * \brief Function to initialize the event handler
 * \param ctx Context (= NULL)
 */
int event_init(ctx_t *ctx)
{
    ev_worker_on = TRUE;

    // Push (downstream)

    ev_push_ctx = zmq_ctx_new();
    ev_push_sock = zmq_socket(ev_push_ctx, ZMQ_PUSH);

    const int timeout = 1000;
    zmq_setsockopt(ev_push_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(ev_push_sock, EXT_COMP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    // Pull (upstream, intstream)

    ev_pull_ctx = zmq_ctx_new();
    ev_pull_sock = zmq_socket(ev_pull_ctx, ZMQ_PULL);

    zmq_setsockopt(ev_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_connect(ev_pull_sock, TARGET_COMP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    int i;
    for (i=0; i<__NUM_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &deliver_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    // Request (intsteam)

    ev_req_ctx = zmq_ctx_new();
    ev_req_sock = zmq_socket(ev_req_ctx, ZMQ_REQ);

    zmq_setsockopt(ev_req_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));
    zmq_setsockopt(ev_req_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(ev_req_sock, EXT_COMP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    // Reply (upstream, intstream)

    ev_rep_ctx = zmq_ctx_new();
    ev_rep_sock = zmq_socket(ev_req_ctx, ZMQ_REP);

    zmq_setsockopt(ev_rep_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));
    zmq_setsockopt(ev_rep_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(ev_rep_sock, TARGET_COMP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    for (i=0; i<__NUM_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &reply_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */

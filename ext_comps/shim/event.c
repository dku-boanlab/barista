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

/////////////////////////////////////////////////////////////////////

/** \brief The configuration of a target component */
extern compnt_t compnt;

/** \brief The running flag for event workers */
int ev_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ contexts to push, pull, request and reply events */
void *ev_push_ctx, *ev_pull_in_ctx, *ev_pull_out_ctx, *ev_req_ctx, *ev_rep_ctx;

/** \brief MQ sockets to pull and reply events */
void *ev_pull_in_sock, *ev_pull_out_sock, *ev_req_sock, *ev_rep_comp, *ev_rep_work;

/////////////////////////////////////////////////////////////////////

#include "event_json.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to have a handshake with the Barista NOS
 * \param id Component ID
 * \param name Component name
 */
static int handshake(uint32_t id, char *name)
{
    void *sock = zmq_socket(ev_req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, EXT_COMP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    } else {
        char out_str[__MAX_EXT_MSG_SIZE];
        sprintf(out_str, "#{\"id\": %u, \"name\": \"%s\"}", compnt.component_id, compnt.name);

        zmq_msg_t out_msg;
        zmq_msg_init_size(&out_msg, strlen(out_str));
        memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));

        if (zmq_send(sock, out_str, strlen(out_str)+1, 0) < 0) {
            PERROR("zmq_send");
            zmq_msg_close(&out_msg);
            zmq_close(sock);
            return -1;
        }

        zmq_msg_close(&out_msg);

        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, sock, 0);
        if (zmq_ret < 0) {
            PERROR("zmq_recv");
            zmq_msg_close(&in_msg);
            zmq_close(sock);
            return -1;
        } else {
            char *in_str = zmq_msg_data(&in_msg);
            in_str[zmq_ret] = '\0';

            if (strcmp(in_str, "#{\"return\": 0}") != 0) {
                zmq_msg_close(&in_msg);
                zmq_close(sock);
                return -1;
            }
        }

        zmq_msg_close(&in_msg);

        zmq_close(sock);
    }

    return 0;
}

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
    if (!ev_worker_on) return -1;

    char output[__MAX_EXT_MSG_SIZE];
    export_to_json(id, type, data, output);

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(output));
    memcpy(zmq_msg_data(&msg), output, strlen(output));

    void *sock = zmq_socket(ev_push_ctx, ZMQ_PUSH);
    if (zmq_connect(sock, EXT_COMP_PULL_ADDR)) {
        zmq_msg_close(&msg);
        return -1;
    } else {
        if (zmq_msg_send(&msg, sock, 0) < 0) {
            zmq_msg_close(&msg);
            zmq_close(sock);
            return -1;
        }

        zmq_msg_close(&msg);
        zmq_close(sock);
    }

    return 0;
}

/**
 * \brief Function to send events to the Barista NOS and receive responses from it
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the component
 */
static int ev_send_msg(uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!ev_worker_on) return -1;

    char out_str[__MAX_EXT_MSG_SIZE];
    export_to_json(id, type, input, out_str);

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(out_str));
    memcpy(zmq_msg_data(&msg), out_str, strlen(out_str));

    void *sock = zmq_socket(ev_req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, EXT_COMP_REPLY_ADDR)) {
        zmq_msg_close(&msg);
        return -1;
    } else {
        if (zmq_msg_send(&msg, sock, 0) < 0) {
            zmq_msg_close(&msg);
            zmq_close(sock);
            return -1;
        } else {
            zmq_msg_close(&msg);

            zmq_msg_t in_msg;
            zmq_msg_init(&in_msg);
            int zmq_ret = zmq_msg_recv(&in_msg, sock, 0);

            if (zmq_ret < 0) {
                zmq_msg_close(&in_msg);
                zmq_close(sock);
                return -1;
            } else {
                char *in_str = zmq_msg_data(&in_msg);
                in_str[zmq_ret] = '\0';

                int ret = import_from_json(&id, &type, in_str, output);

                zmq_msg_close(&in_msg);
                zmq_close(sock);

                return ret;
            }
        }
    }
}

// Upstream events //////////////////////////////////////////////////

/** \brief EV_OFP_MSG_IN */
void ev_ofp_msg_in(uint32_t id, const raw_msg_t *data) { ev_push_msg(id, EV_OFP_MSG_IN, sizeof(raw_msg_t), (const raw_msg_t *)data); }
/** \brief EV_DP_RECEIVE_PACKET */
void ev_dp_receive_packet(uint32_t id, const pktin_t *data) { ev_push_msg(id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), data); }
/** \brief EV_DP_FLOW_EXPIRED */
void ev_dp_flow_expired(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_DELETED */
void ev_dp_flow_deleted(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_FLOW_DELETED, sizeof(flow_t), data); }
/** \brief EV_DP_FLOW_STATS */
void ev_dp_flow_stats(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_AGGREGATE_STATS */
void ev_dp_aggregate_stats(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_PORT_ADDED */
void ev_dp_port_added(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_PORT_ADDED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_MODIFIED */
void ev_dp_port_modified(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_PORT_MODIFIED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_DELETED */
void ev_dp_port_deleted(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_PORT_DELETED, sizeof(port_t), data); }
/** \brief EV_DP_PORT_STATS */
void ev_dp_port_stats(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_PORT_STATS, sizeof(port_t), data); }

// Downstream events ////////////////////////////////////////////////

/** \brief EV_OFP_MSG_OUT */
void ev_ofp_msg_out(uint32_t id, const raw_msg_t *data) { ev_push_msg(id, EV_OFP_MSG_OUT, sizeof(raw_msg_t), (const raw_msg_t *)data); }
/** \brief EV_DP_SEND_PACKET */
void ev_dp_send_packet(uint32_t id, const pktout_t *data) { ev_push_msg(id, EV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief EV_DP_INSERT_FLOW */
void ev_dp_insert_flow(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_MODIFY_FLOW */
void ev_dp_modify_flow(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_DELETE_FLOW */
void ev_dp_delete_flow(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_DELETE_FLOW, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_FLOW_STATS */
void ev_dp_request_flow_stats(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_AGGREGATE_STATS */
void ev_dp_request_aggregate_stats(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), data); }
/** \brief EV_DP_REQUEST_PORT_STATS */
void ev_dp_request_port_stats(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), data); }

// Internal events (request-response) ///////////////////////////////

/** \brief EV_SW_GET_DPID (fd) */
void ev_sw_get_dpid(uint32_t id, switch_t *data) { ev_send_msg(id, EV_SW_GET_DPID, sizeof(switch_t), data, data); }
/** \brief EV_SW_GET_FD (dpid) */
void ev_sw_get_fd(uint32_t id, switch_t *data) { ev_send_msg(id, EV_SW_GET_FD, sizeof(switch_t), data, data); }
/** \brief EV_SW_GET_XID (fd or dpid) */
void ev_sw_get_xid(uint32_t id, switch_t *data) { ev_send_msg(id, EV_SW_GET_XID, sizeof(switch_t), data, data); }

// Internal events (notification) ///////////////////////////////////

/** \brief EV_SW_NEW_CONN */
void ev_sw_new_conn(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_EXPIRED_CONN */
void ev_sw_expired_conn(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_CONNECTED */
void ev_sw_connected(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_DISCONNECTED */
void ev_sw_disconnected(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_CONFIG */
void ev_sw_update_config(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_UPDATE_CONFIG, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_UPDATE_DESC */
void ev_sw_update_desc(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_HOST_ADDED */
void ev_host_added(uint32_t id, const host_t *data) { ev_push_msg(id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_HOST_DELETED */
void ev_host_deleted(uint32_t id, const host_t *data) { ev_push_msg(id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)data); }
/** \brief EV_LINK_ADDED */
void ev_link_added(uint32_t id, const port_t *data) { ev_push_msg(id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_LINK_DELETED */
void ev_link_deleted(uint32_t id, const port_t *data) { ev_push_msg(id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)data); }
/** \brief EV_FLOW_ADDED */
void ev_flow_added(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_FLOW_DELETED */
void ev_flow_deleted(uint32_t id, const flow_t *data) { ev_push_msg(id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)data); }
/** \brief EV_RS_UPDATE_USAGE */
void ev_rs_update_usage(uint32_t id, const resource_t *data) { ev_push_msg(id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)data); }
/** \brief EV_TR_UPDATE_STATS */
void ev_tr_update_stats(uint32_t id, const traffic_t *data) { ev_push_msg(id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)data); }
/** \brief EV_LOG_UPDATE_MSGS */
void ev_log_update_msgs(uint32_t id, const char *data) { ev_push_msg(id, EV_LOG_UPDATE_MSGS, strlen((const char *)data), (const char *)data); }

// Log events ///////////////////////////////////////////////////////

/** \brief EV_LOG_DEBUG */
void ev_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    ev_push_msg(id, EV_LOG_DEBUG, strlen(log), log);
}
/** \brief EV_LOG_INFO */
void ev_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    ev_push_msg(id, EV_LOG_INFO, strlen(log), log);
}
/** \brief EV_LOG_WARN */
void ev_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    ev_push_msg(id, EV_LOG_WARN, strlen(log), log);
}
/** \brief EV_LOG_ERROR */
void ev_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    ev_push_msg(id, EV_LOG_ERROR, strlen(log), log);
}
/** \brief EV_LOG_FATAL */
void ev_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    ev_push_msg(id, EV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process events in an event queue and reply outputs
 * \return NULL
 */
static void *reply_events(void *null)
{
    waitsec(1, 0);

    void *recv = zmq_socket(ev_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://ev_reply_workers");

    while (ev_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!ev_worker_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        char *in_str = zmq_msg_data(&in_msg);
        in_str[zmq_ret] = '\0';

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, in_str, msg.data);

        zmq_msg_close(&in_msg);

        if (msg.id == 0) continue;
	else if (msg.type > EV_NUM_EVENTS) continue;

        event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = msg.data;

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
        case EV_DP_FLOW_DELETED:
        case EV_DP_FLOW_STATS:
        case EV_DP_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_DP_PORT_ADDED:
        case EV_DP_PORT_MODIFIED:
        case EV_DP_PORT_DELETED:
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
        case EV_DP_MODIFY_FLOW:
        case EV_DP_DELETE_FLOW:
        case EV_DP_REQUEST_FLOW_STATS:
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
        case EV_SW_GET_FD:
        case EV_SW_GET_XID:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;

        // internal events (notification)

        case EV_SW_NEW_CONN:
        case EV_SW_EXPIRED_CONN:
        case EV_SW_CONNECTED:
        case EV_SW_DISCONNECTED:
        case EV_SW_UPDATE_CONFIG:
        case EV_SW_UPDATE_DESC:
            out.length = sizeof(switch_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_HOST_ADDED:
        case EV_HOST_DELETED:
            out.length = sizeof(host_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_LINK_ADDED:
        case EV_LINK_DELETED:
            out.length = sizeof(port_t);
            ret = compnt.handler(in, &out);
            break;
        case EV_FLOW_ADDED:
        case EV_FLOW_MODIFIED:
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

        msg.ret = ret;

        char output[__MAX_EXT_MSG_SIZE];
        export_to_json(msg.id, msg.type, msg.data, output);

        zmq_msg_t out_msg;
        zmq_msg_init(&out_msg);
        memcpy(zmq_msg_data(&out_msg), output, strlen(output));
        zmq_msg_send(&out_msg, recv, 0);
        zmq_msg_close(&out_msg);

        if (!ev_worker_on) break;
    }

    zmq_close(recv);

    return NULL;
}

/**
 * \brief Function to connect work threads to component threads via a queue proxy
 * \return NULL
 */
static void *reply_proxy(void *null)
{
    zmq_proxy(ev_rep_comp, ev_rep_work, NULL);

    return NULL;
}

/**
 * \brief Function to receive events from the Barista NOS
 * \return NULL
 */
static void *receive_events(void *null)
{
    waitsec(1, 0);

    while (ev_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, ev_pull_in_sock, 0);

        if (!ev_worker_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        zmq_msg_send(&in_msg, ev_pull_out_sock, 0);

        zmq_msg_close(&in_msg);
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

    void *recv = zmq_socket(ev_pull_out_ctx, ZMQ_PULL);
    zmq_connect(recv, "inproc://ev_pull_workers");

    while (ev_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!ev_worker_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        char *in_str = zmq_msg_data(&in_msg);
        in_str[zmq_ret] = '\0';

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, in_str, msg.data);

        zmq_msg_close(&in_msg);

        if (msg.id == 0) continue;
	else if (msg.type > EV_NUM_EVENTS) continue;

        event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = msg.data;

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
        case EV_DP_FLOW_DELETED:
        case EV_DP_FLOW_STATS:
        case EV_DP_AGGREGATE_STATS:
            out.length = sizeof(flow_t);
            compnt.handler(in, &out);
            break;
        case EV_DP_PORT_ADDED:
        case EV_DP_PORT_MODIFIED:
        case EV_DP_PORT_DELETED:
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
        case EV_DP_MODIFY_FLOW:
        case EV_DP_DELETE_FLOW:
        case EV_DP_REQUEST_FLOW_STATS:
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
        case EV_SW_GET_FD:
        case EV_SW_GET_XID:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;

        // internal events (notification)

        case EV_SW_NEW_CONN:
        case EV_SW_EXPIRED_CONN:
        case EV_SW_CONNECTED:
        case EV_SW_DISCONNECTED:
        case EV_SW_UPDATE_CONFIG:
        case EV_SW_UPDATE_DESC:
            out.length = sizeof(switch_t);
            compnt.handler(in, &out);
            break;
        case EV_HOST_ADDED:
        case EV_HOST_DELETED:
            out.length = sizeof(host_t);
            compnt.handler(in, &out);
            break;
        case EV_LINK_ADDED:
        case EV_LINK_DELETED:
            out.length = sizeof(port_t);
            compnt.handler(in, &out);
            break;
        case EV_FLOW_ADDED:
        case EV_FLOW_MODIFIED:
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
    }

    zmq_close(recv);

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

    zmq_close(ev_pull_in_sock);
    zmq_close(ev_pull_out_sock);
    zmq_close(ev_req_sock);
    zmq_close(ev_rep_comp);
    zmq_close(ev_rep_work);

    zmq_ctx_destroy(ev_push_ctx);
    zmq_ctx_destroy(ev_pull_in_ctx);
    zmq_ctx_destroy(ev_pull_out_ctx);
    zmq_ctx_destroy(ev_req_ctx);
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

    // Pull (upstream, intstream)

    ev_pull_in_ctx = zmq_ctx_new();
    ev_pull_in_sock = zmq_socket(ev_pull_in_ctx, ZMQ_PULL);

    if (zmq_bind(ev_pull_in_sock, TARGET_COMP_PULL_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    ev_pull_out_ctx = zmq_ctx_new();
    ev_pull_out_sock = zmq_socket(ev_pull_out_ctx, ZMQ_PUSH);

    if (zmq_bind(ev_pull_out_sock, "inproc://ev_pull_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, &receive_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &deliver_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    // Request (intsteam)

    ev_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    ev_rep_ctx = zmq_ctx_new();
    ev_rep_comp = zmq_socket(ev_rep_ctx, ZMQ_ROUTER);

    if (zmq_bind(ev_rep_comp, TARGET_COMP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    ev_rep_work = zmq_socket(ev_rep_ctx, ZMQ_DEALER);

    if (zmq_bind(ev_rep_work, "inproc://ev_reply_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    for (i=0; i<__NUM_REP_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &reply_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    if (pthread_create(&thread, NULL, &reply_proxy, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    if (handshake(compnt.component_id, compnt.name))
        return -1;

    return 0;
}

/**
 * @}
 *
 * @}
 */

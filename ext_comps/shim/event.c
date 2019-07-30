/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt_shim
 * @{
 * \defgroup compnt_event External Event Handler
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

/** \brief The configuration of an external component */
extern compnt_t compnt;

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for external event handler */
int ev_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ context to push events */
void *ev_push_ctx;

/** \brief MQ context to pull events */
void *ev_pull_ctx;

/** \brief MQ socket to pull events */
void *ev_pull_sock;

/** \brief MQ context to request events */
void *ev_req_ctx;

/** \brief MQ context to reply events */
void *ev_rep_ctx;

/** \brief MQ socket to reply events */
void *ev_rep_sock;

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
    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    sprintf(json_in, "#{\"id\": %u, \"name\": \"%s\"}", compnt.component_id, compnt.name);
    int len = strlen(json_in);

    void *req_sock = zmq_socket(ev_req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, __EXT_COMP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    zmq_send(req_sock, json_in, len, 0);

    char json_out[__MAX_EXT_MSG_SIZE] = {0};
    zmq_recv(req_sock, json_out, __MAX_EXT_MSG_SIZE, 0);

    if (strncmp(json_out, "#{\"return\": 0}", 14) != 0) {
        PRINTF("Failed to make a handshake\n");
        zmq_close(req_sock);
        return -1;
    } else {
        PRINTF("Connected to the Barista NOS\n");
    }

    zmq_close(req_sock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send events to the Barista NOS
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 */
static int ev_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *input)
{
    if (!ev_on) return -1;

    char json[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json, 0);

    void *push_sock = zmq_socket(ev_push_ctx, ZMQ_PUSH);

    if (zmq_connect(push_sock, __EXT_COMP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    //printf("%s: %s\n", __FUNCTION__, json);

    zmq_send(push_sock, json, len, 0);

    zmq_close(push_sock);

    return 0;
}

/**
 * \brief Function to send events to the Barista NOS and receive its responses
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the component
 */
static int ev_send_msg(uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!ev_on) return -1;

    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json_in, 0);

    void *req_sock = zmq_socket(ev_req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, __EXT_COMP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    //printf("%s: %s\n", __FUNCTION__, json_in);

    zmq_send(req_sock, json_in, len, 0);

    char json_out[__MAX_EXT_MSG_SIZE] = {0};
    zmq_recv(req_sock, json_out, __MAX_EXT_MSG_SIZE, 0);

    uint8_t data[__MAX_MSG_SIZE] = {0};

    msg_t msg = {0};
    msg.data = data;
    msg.ret = import_from_json(&msg.id, &msg.type, json_out, msg.data);

    if (compnt.in_perm[type] & COMPNT_WRITE && msg.id == id && msg.type == type)
        memcpy(output, msg.data, size);

    zmq_close(req_sock);

    return msg.ret;
}

// Upstream events //////////////////////////////////////////////////

/** \brief EV_OFP_MSG_IN */
void ev_ofp_msg_in(uint32_t id, const msg_t *data) { ev_push_msg(id, EV_OFP_MSG_IN, sizeof(msg_t), (const msg_t *)data); }
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
void ev_ofp_msg_out(uint32_t id, const msg_t *data) { ev_push_msg(id, EV_OFP_MSG_OUT, sizeof(msg_t), (const msg_t *)data); }
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
/** \brief EV_DP_MODIFY_PORT */
void ev_dp_modify_port(uint32_t id, const port_t *data) { ev_push_msg(id, EV_DP_MODIFY_PORT, sizeof(port_t), data); }
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
/** \brief EV_SW_ESTABLISHED_CONN */
void ev_sw_established_conn(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_ESTABLISHED_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_EXPIRED_CONN */
void ev_sw_expired_conn(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_CONNECTED */
void ev_sw_connected(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)data); }
/** \brief EV_SW_DISCONNECTED */
void ev_sw_disconnected(uint32_t id, const switch_t *data) { ev_push_msg(id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)data); }
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
 * \brief Function to process events
 * \param msg Application events
 */
static int process_events(msg_t *msg)
{
    int ret = 0;

    event_out_t out = {0};

    out.id = msg->id;
    out.type = msg->type;
    out.data = msg->data;

    const event_t *in = (const event_t *)&out;

    switch (out.type) {

    // upstream events

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
    case EV_DP_MODIFY_PORT:
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
    case EV_SW_ESTABLISHED_CONN:
    case EV_SW_EXPIRED_CONN:
    case EV_SW_CONNECTED:
    case EV_SW_DISCONNECTED:
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

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive events from the Barista NOS
 * \param null NULL
 */
static void *pull_events(void *null)
{
    while (ev_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!ev_on) break;
        else if (zmq_recv(ev_pull_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        process_events(&msg);

        if (!ev_on) break;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process requests from the Barista NOS and reply them
 * \param null NULL
 */
static void *reply_events(void *null)
{
    while (ev_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!ev_on) break;
        else if (zmq_recv(ev_rep_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        msg.ret = process_events(&msg);
        export_to_json(msg.id, msg.type, msg.data, json, msg.ret);
        zmq_send(ev_rep_sock, json, strlen(json), 0);

        if (!ev_on) break;
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
    pthread_t thread;

    ev_on = TRUE;

    // Push (downstream)

    ev_push_ctx = zmq_ctx_new();

    // Pull (upstream, intstream)

    ev_pull_ctx = zmq_ctx_new();
    ev_pull_sock = zmq_socket(ev_pull_ctx, ZMQ_PULL);

    if (zmq_bind(ev_pull_sock, TARGET_COMP_PULL_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    int timeout = 1000;
    zmq_setsockopt(ev_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (pthread_create(&thread, NULL, &pull_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    // Request (intsteam)

    ev_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    ev_rep_ctx = zmq_ctx_new();
    ev_rep_sock = zmq_socket(ev_rep_ctx, ZMQ_REP);

    if (zmq_bind(ev_rep_sock, TARGET_COMP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    zmq_setsockopt(ev_rep_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (pthread_create(&thread, NULL, &reply_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    if (handshake(compnt.component_id, compnt.name))
        return -1;

    return 0;
}

/**
 * \brief Function to destroy the event handler
 * \param ctx The context of the Barista NOS
 */
int destroy_event(ctx_t *ctx)
{
    ev_on = FALSE;

    waitsec(1, 0);

    zmq_close(ev_pull_sock);
    zmq_close(ev_rep_sock);

    zmq_ctx_destroy(ev_pull_ctx);
    zmq_ctx_destroy(ev_push_ctx);
    zmq_ctx_destroy(ev_rep_ctx);
    zmq_ctx_destroy(ev_req_ctx);

    return 0;
}

/**
 * @}
 *
 * @}
 */

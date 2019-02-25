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

/** \brief The configuration of an external component */
extern compnt_t compnt;

/** \brief The running flag for external event workers */
int ev_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ contexts to request events */
void *ev_req_ctx;

/** \brief MQ contexts to reply events */
void *ev_rep_ctx;

/** \brief MQ sockets to request events */
void *ev_req_sock;

/** \brief MQ sockets to reply events (component-side) */
void *ev_rep_comp;

/** \brief MQ sockets to reply events (worker-side)  */
void *ev_rep_work;

/////////////////////////////////////////////////////////////////////

#include "event_json.h"

/////////////////////////////////////////////////////////////////////

/** \brief Socket pointer to push events */
uint32_t ev_push_ptr;

/** \brief Socket to push events */
int ev_push_sock[__NUM_PULL_THREADS];

/** \brief Lock for av_push_sock */
pthread_spinlock_t ev_push_lock[__NUM_PULL_THREADS];

/////////////////////////////////////////////////////////////////////

#include "event_epoll_env.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to clean up a ZMQ message
 * \param data ZMQ message
 * \param hint Hint
 */
void mq_free(void *data, void *hint)
{
    free(data);
}

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
        char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
        sprintf(out_str, "#{\"id\": %u, \"name\": \"%s\"}", compnt.component_id, compnt.name);

        zmq_msg_t out_msg;
        zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
            PERROR("zmq_send");
            zmq_close(sock);
            return -1;
        }

        zmq_msg_t in_msg;
        zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
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

/** \brief The header of a JSON message */
typedef struct _json_header {
    uint16_t len;
    char json[__MAX_EXT_MSG_SIZE];
} __attribute__((packed)) json_header;

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

    json_header jsonh = {0};

    export_to_json(id, type, data, jsonh.json);
    jsonh.len = strlen(jsonh.json) + 2 + 1; // json_len + length variable + NULL

    int remain = jsonh.len;
    int bytes = 0;
    int done = 0;

    jsonh.len = htons(jsonh.len);

    uint32_t ptr = (ev_push_ptr++) % __NUM_PULL_THREADS;

    pthread_spin_lock(&ev_push_lock[ptr]);

    while (remain > 0) {
        bytes = write(ev_push_sock[ptr], (uint8_t *)&jsonh + done, remain);
        if (bytes < 0 && errno != EAGAIN) {
            break;
        } else if (bytes < 0) {
            continue;
        }

        remain -= bytes;
        done += bytes;
    }

    pthread_spin_unlock(&ev_push_lock[ptr]);

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

    char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
    export_to_json(id, type, input, out_str);

    zmq_msg_t out_msg;
    zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

    void *sock = zmq_socket(ev_req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, EXT_COMP_REPLY_ADDR)) {
        return -1;
    } else {
        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
            zmq_close(sock);
            return -1;
        } else {
            zmq_msg_t in_msg;
            zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
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
 * \param null NULL
 */
static void *reply_events(void *null)
{
    void *recv = zmq_socket(ev_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://ev_reply_workers");

    while (ev_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
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

        char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
        export_to_json(msg.id, msg.type, msg.data, out_str);

        zmq_msg_t out_msg;
        zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

        zmq_msg_send(&out_msg, recv, 0);

        zmq_msg_close(&in_msg);

        if (!ev_worker_on) break;
    }

    zmq_close(recv);

    return NULL;
}

/**
 * \brief Function to connect worker threads to component threads via a queue proxy
 * \param null NULL
 */
static void *reply_proxy(void *null)
{
    zmq_proxy(ev_rep_comp, ev_rep_work, NULL);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process events
 * \param msg Application events
 */
static int process_events(msg_t *msg)
{
    event_out_t out = {0};
    int ret = 0;

    out.id = msg->id;
    out.type = msg->type;
    out.data = msg->data;

    const event_t *in = (const event_t *)&out;

    switch (out.type) {

    // upstream events

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

    return ret;
}

/////////////////////////////////////////////////////////////////////

/** \brief The structure to keep the remaining part of a message */
typedef struct _buffer_t {
    int need; /**< The bytes that it needs to read */
    int done; /**< The bytes that it has */
    uint8_t temp[__MAX_EXT_MSG_SIZE]; /**< Temporary data */
} buffer_t;

/** \brief Buffers for all possible sockets */
buffer_t *buffer;

/**
 * \brief Function to initialize all buffers
 * \return None
 */
static void init_buffers(void)
{
    buffer = (buffer_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(buffer_t));
    if (buffer == NULL) {
        PERROR("calloc");
        return;
    }
}

/**
 * \brief Function to clean up a buffer
 * \param sock Network socket
 */
static void clean_buffer(int sock)
{
    if (buffer)
        memset(&buffer[sock], 0, sizeof(buffer_t));
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from the Barista NOS
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    buffer_t *b = &buffer[sock];

    uint8_t *temp = b->temp;
    int need = b->need;
    int done = b->done;

    int buf_ptr = 0;
    while (bytes > 0) {
        if (0 < done && done < 2) {
            if (done + bytes < 2) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                done += bytes;
                bytes = 0;

                break;
            } else {
                memmove(temp + done, rx_buf + buf_ptr, (2 - done));

                bytes -= (2 - done);
                buf_ptr += (2 - done);

                need = ntohs(((json_header *)temp)->len) - 2;
                done = 2;
            }
        }

        if (need == 0) {
            json_header *jsonh = (json_header *)(rx_buf + buf_ptr);

            if (bytes < 2) {
                memmove(temp, rx_buf + buf_ptr, bytes);

                need = 0;
                done = bytes;

                bytes = 0;
            } else {
                uint16_t len = ntohs(jsonh->len);

                if (bytes < len) {
                    memmove(temp, rx_buf + buf_ptr, bytes);

                    need = len - bytes;
                    done = bytes;

                    bytes = 0;
                } else if (len > 0) {
                    char *in_str = jsonh->json;

                    msg_t msg = {0};
                    import_from_json(&msg.id, &msg.type, in_str, msg.data);

                    if (msg.id != 0 && msg.type < EV_NUM_EVENTS)
                        process_events(&msg);

                    bytes -= len;
                    buf_ptr += len;

                    need = 0;
                    done = 0;
                } else {
                    return -1;
                }
            }
        } else {
            json_header *jsonh = (json_header *)temp;

            if (need > bytes) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                need -= bytes;
                done += bytes;

                bytes = 0;
            } else if (jsonh->len > 0) {
                memmove(temp + done, rx_buf + buf_ptr, need);

                char *in_str = jsonh->json;

                msg_t msg = {0};
                import_from_json(&msg.id, &msg.type, in_str, msg.data);

                if (msg.id != 0 && msg.type < EV_NUM_EVENTS)
                    process_events(&msg);

                bytes -= need;
                buf_ptr += need;

                need = 0;
                done = 0;
            } else {
                return -1;
            }
        }
    }

    b->need = need;
    b->done = done;

    return bytes;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to do something when the Barista NOS is connected
 * \return None
 */
static int new_connection(int sock)
{
    return 0;
}

/**
 * \brief Function to do something when the Barista NOS is disconnected
 * \return None
 */
static int closed_connection(int sock)
{
    clean_buffer(sock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to destroy an event queue
 * \param null NULL
 */
int destroy_ev_workers(ctx_t *null)
{
    ev_worker_on = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_spin_destroy(&ev_push_lock[i]);
        close(ev_push_sock[i]);
        ev_push_sock[i] = 0;
    }

    destroy_epoll_env();
    FREE(buffer);

    zmq_close(ev_req_sock);
    zmq_close(ev_rep_comp);
    zmq_close(ev_rep_work);

    waitsec(1, 0);

    zmq_ctx_destroy(ev_req_ctx);
    zmq_ctx_destroy(ev_rep_ctx);

    return 0;
}

/**
 * \brief Function to initialize the event handler
 * \param null NULL
 */
int event_init(ctx_t *null)
{
    ev_worker_on = TRUE;

    // Push (downstream)

    char push_type[__CONF_WORD_LEN];
    char push_addr[__CONF_WORD_LEN];
    int push_port;

    sscanf(EXT_COMP_PULL_ADDR, "%[^:]://%[^:]:%d", push_type, push_addr, &push_port);

    if (strcmp(push_type, "tcp") == 0) {
        struct sockaddr_in push;
        memset(&push, 0, sizeof(push));
        push.sin_family = AF_INET;
        push.sin_addr.s_addr = inet_addr(push_addr);
        push.sin_port = htons(push_port);

        int i;
        for (i=0; i<__NUM_PULL_THREADS; i++) {
            if ((ev_push_sock[i] = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                PERROR("socket");
                return -1;
            }

            if (connect(ev_push_sock[i], (struct sockaddr *)&push, sizeof(push)) < 0) {
                PERROR("connect");
                return -1;
            }

            pthread_spin_init(&ev_push_lock[i], PTHREAD_PROCESS_PRIVATE);
        }
    } else { // ipc
        struct sockaddr_un push;
        memset(&push, 0, sizeof(push));
        push.sun_family = AF_UNIX;
        strcpy(push.sun_path, push_addr);

        int i;
        for (i=0; i<__NUM_PULL_THREADS; i++) {
            if ((ev_push_sock[i] = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
                PERROR("socket");
                return -1;
            }

            if (connect(ev_push_sock[i], (struct sockaddr *)&push, sizeof(push)) < 0) {
                PERROR("connect");
                return -1;
            }

            pthread_spin_init(&ev_push_lock[i], PTHREAD_PROCESS_PRIVATE);
        }
    }

    // Pull (upstream, intstream)

    char pull_type[__CONF_WORD_LEN];
    char pull_addr[__CONF_WORD_LEN];
    int pull_port;

    sscanf(TARGET_COMP_PULL_ADDR, "%[^:]://%[^:]:%d", pull_type, pull_addr, &pull_port);

    init_buffers();
    create_epoll_env(pull_type, pull_addr, pull_port);

    pthread_t thread;
    if (pthread_create(&thread, NULL, &socket_listen, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    // Request (intsteam)

    ev_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    ev_rep_ctx = zmq_ctx_new();
    ev_rep_comp = zmq_socket(ev_rep_ctx, ZMQ_ROUTER);

    int timeout = 250;
    zmq_setsockopt(ev_rep_comp, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_bind(ev_rep_comp, TARGET_COMP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    ev_rep_work = zmq_socket(ev_rep_ctx, ZMQ_DEALER);

    zmq_setsockopt(ev_rep_work, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_bind(ev_rep_work, "inproc://ev_reply_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    int i;
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

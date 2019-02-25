/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup events
 * @{
 * \defgroup ev_msg External Event Handler
 * \brief Functions to handle events between the Barista NOS and external components
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

/////////////////////////////////////////////////////////////////////

#include "event_json.h"

/////////////////////////////////////////////////////////////////////

#include "event_epoll_env.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to clean up a ZMQ message
 * \param data ZMQ message
 * \param hint Hint
 */
static void mq_free(void *data, void *hint)
{
    free(data);
}

/**
 * \brief Function to activate an external component
 * \param in_str Handshake message
 */
static int activate_external_component(char *in_str)
{
    json_t *json = NULL;
    json_error_t error;

    json = json_loads(in_str, 0, &error);
    if (!json) {
        PERROR("json_loads");
        return -1;
    }

    uint32_t id = 0;
    json_t *j_id = json_object_get(json, "id");
    if (json_is_integer(j_id))
        id = json_integer_value(j_id);

    char name[__CONF_WORD_LEN] = {0};
    json_t *j_name = json_object_get(json, "name");
    if (json_is_string(j_name))
        strcpy(name, json_string_value(j_name));

    if (id == 0 || strlen(name) == 0) {
        json_decref(json);
        return -1;
    }

    int i;
    for (i=0; i<ev_ctx->num_compnts; i++) {
        if (ev_ctx->compnt_list[i]->component_id == id) {
            if (strcmp(ev_ctx->compnt_list[i]->name, name) == 0) {
                compnt_t *compnt = ev_ctx->compnt_list[i];

                if (compnt->activated) {
                    compnt->activated = FALSE;
                    waitsec(1, 0);
                }

                char push_type[__CONF_WORD_LEN];
                char push_addr[__CONF_WORD_LEN];
                int push_port;

                sscanf(compnt->pull_addr, "%[^:]://%[^:]:%d", push_type, push_addr, &push_port);

                if (strcmp(push_type, "tcp") == 0) {
                    struct sockaddr_in push;
                    memset(&push, 0, sizeof(push));
                    push.sin_family = AF_INET;
                    push.sin_addr.s_addr = inet_addr(push_addr);
                    push.sin_port = htons(push_port);

                    int j;
                    for (j=0; j<__NUM_PULL_THREADS; j++) {
                        if ((compnt->push_sock[j] = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                            compnt->activated = FALSE;
                            PERROR("socket");
                            return -1;
                        }

                        if (connect(compnt->push_sock[j], (struct sockaddr *)&push, sizeof(push)) < 0) {
                            compnt->activated = FALSE;
                            PERROR("connect");
                            return -1;
                        }
                    }
                } else { // ipc
                    struct sockaddr_un push;
                    memset(&push, 0, sizeof(push));
                    push.sun_family = AF_UNIX;
                    strcpy(push.sun_path, push_addr);

                    int j;
                    for (j=0; j<__NUM_PULL_THREADS; j++) {
                        if ((compnt->push_sock[j] = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
                            compnt->activated = FALSE;
                            PERROR("socket");
                            return -1;
                        }

                        if (connect(compnt->push_sock[j], (struct sockaddr *)&push, sizeof(push)) < 0) {
                            compnt->activated = FALSE;
                            PERROR("connect");
                            return -1;
                        }
                    }
                }

                compnt->activated = TRUE;

                json_decref(json);
                return 0;
            } else {
                json_decref(json);
                return -1;
            }
        }
    }

    json_decref(json);

    return -1;
}

/////////////////////////////////////////////////////////////////////

/** \brief The header of a JSON message */
typedef struct _json_header {
    uint16_t len;
    char json[__MAX_EXT_MSG_SIZE];
} __attribute__((packed)) json_header;

/**
 * \brief Function to send events to an external component
 * \param c Component context
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 */
static int ev_push_ext_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input)
{
    if (!c->activated) return -1;

    json_header jsonh = {0};

    export_to_json(id, type, input, jsonh.json);
    jsonh.len = strlen(jsonh.json) + 2 + 1; // json_len + length variable + NULL

    int remain = jsonh.len;
    int bytes = 0;
    int done = 0;

    jsonh.len = htons(jsonh.len);

    uint32_t ptr = (c->push_ptr++) % __NUM_PULL_THREADS;

    pthread_spin_lock(&c->push_lock[ptr]);

    while (remain > 0) {
        bytes = write(c->push_sock[ptr], (uint8_t *)&jsonh + done, remain);
        if (bytes < 0 && errno != EAGAIN) {
            c->activated = FALSE;
            break;
        } else if (bytes < 0) {
            continue;
        }

        remain -= bytes;
        done += bytes;
    }

    pthread_spin_unlock(&c->push_lock[ptr]);

    return 0;
}

/**
 * \brief Function to send events to an external component and receive responses from it
 * \param c Component context
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the component
 */
static int ev_send_ext_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!c->activated) return -1;

    char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
    export_to_json(id, type, input, out_str);

    zmq_msg_t out_msg;
    zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

    void *sock = zmq_socket(c->req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, c->reply_addr)) {
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

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process events
 * \param msg Events
 */
static int process_events(msg_t *msg)
{
    int ret = 0;

    switch (msg->type) {

    // upstream events

    case EV_DP_RECEIVE_PACKET:
        pktin_ev_raise(msg->id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), (const pktin_t *)msg->data);
        break;
    case EV_DP_FLOW_EXPIRED:
        flow_ev_raise(msg->id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_FLOW_DELETED:
        flow_ev_raise(msg->id, EV_DP_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_FLOW_STATS:
        flow_ev_raise(msg->id, EV_DP_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_AGGREGATE_STATS:
        flow_ev_raise(msg->id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_PORT_ADDED:
        port_ev_raise(msg->id, EV_DP_PORT_ADDED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_MODIFIED:
        port_ev_raise(msg->id, EV_DP_PORT_MODIFIED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_DELETED:
        port_ev_raise(msg->id, EV_DP_PORT_DELETED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_STATS:
        port_ev_raise(msg->id, EV_DP_PORT_STATS, sizeof(port_t), (const port_t *)msg->data);
        break;

    // downstream events

    case EV_DP_SEND_PACKET:
        pktout_ev_raise(msg->id, EV_DP_SEND_PACKET, sizeof(pktout_t), (const pktout_t *)msg->data);
        break;
    case EV_DP_INSERT_FLOW:
        flow_ev_raise(msg->id, EV_DP_INSERT_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_MODIFY_FLOW:
        flow_ev_raise(msg->id, EV_DP_MODIFY_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_DELETE_FLOW:
        flow_ev_raise(msg->id, EV_DP_DELETE_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_REQUEST_FLOW_STATS:
        flow_ev_raise(msg->id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_REQUEST_AGGREGATE_STATS:
        flow_ev_raise(msg->id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_REQUEST_PORT_STATS:
        port_ev_raise(msg->id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), (const port_t *)msg->data);
        break;

    // internal events (request-response)

    case EV_SW_GET_DPID:
        sw_rw_raise(msg->id, EV_SW_GET_DPID, sizeof(switch_t), (switch_t *)msg->data);
        break;
    case EV_SW_GET_FD:
        sw_rw_raise(msg->id, EV_SW_GET_FD, sizeof(switch_t), (switch_t *)msg->data);
        break;
    case EV_SW_GET_XID:
        sw_rw_raise(msg->id, EV_SW_GET_XID, sizeof(switch_t), (switch_t *)msg->data);
        break;

    // internal events (notification)

    case EV_SW_NEW_CONN:
        sw_ev_raise(msg->id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_EXPIRED_CONN:
        sw_ev_raise(msg->id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_CONNECTED:
        sw_ev_raise(msg->id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_DISCONNECTED:
        sw_ev_raise(msg->id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_UPDATE_CONFIG:
        sw_ev_raise(msg->id, EV_SW_UPDATE_CONFIG, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_UPDATE_DESC:
        sw_ev_raise(msg->id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_HOST_ADDED:
        host_ev_raise(msg->id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)msg->data);
        break;
    case EV_HOST_DELETED:
        host_ev_raise(msg->id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)msg->data);
        break;
    case EV_LINK_ADDED:
        port_ev_raise(msg->id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_LINK_DELETED:
        port_ev_raise(msg->id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_FLOW_ADDED:
        flow_ev_raise(msg->id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_FLOW_DELETED:
        flow_ev_raise(msg->id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_RS_UPDATE_USAGE:
        rs_ev_raise(msg->id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)msg->data);
        break;
    case EV_TR_UPDATE_STATS:
        tr_ev_raise(msg->id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)msg->data);
        break;
    case EV_LOG_UPDATE_MSGS:
        log_ev_raise(msg->id, EV_LOG_UPDATE_MSGS, strlen((const char *)msg->data), (const char *)msg->data);
        break;

    // log events

    case EV_LOG_DEBUG:
        log_ev_raise(msg->id, EV_LOG_DEBUG, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_INFO:
        log_ev_raise(msg->id, EV_LOG_INFO, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_WARN:
        log_ev_raise(msg->id, EV_LOG_WARN, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_ERROR:
        log_ev_raise(msg->id, EV_LOG_ERROR, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_FATAL:
        log_ev_raise(msg->id, EV_LOG_FATAL, strlen((const char *)msg->data), (const char *)msg->data);
        break;

    default:
        break;
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process requests from components
 * \param null NULL
 */
static void *reply_events(void *null)
{
    void *recv = zmq_socket(ev_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://ev_reply_workers");

    while (ev_ctx->ev_on) {
        zmq_msg_t in_msg;
        zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!ev_ctx->ev_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        char *in_str = zmq_msg_data(&in_msg);
        in_str[zmq_ret] = '\0';

        // handshake with external compnt
        if (in_str[0] == '#') {
            if (activate_external_component(in_str+1) == 0) {
                char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
                strcpy(out_str, "#{\"return\": 0}");

                zmq_msg_t out_msg;
                zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

                zmq_msg_send(&out_msg, recv, 0);
            } else {
                char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
                strcpy(out_str, "#{\"return\": -1}");

                zmq_msg_t out_msg;
                zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

                zmq_msg_send(&out_msg, recv, 0);
            }

            zmq_msg_close(&in_msg);

            continue;
        }

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, in_str, msg.data);

        zmq_msg_close(&in_msg);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        msg.ret = process_events(&msg);

        char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
        export_to_json(msg.id, msg.type, msg.data, out_str);

        zmq_msg_t out_msg;
        zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

        zmq_msg_send(&out_msg, recv, 0);

        zmq_msg_close(&in_msg);

        if (!ev_ctx->ev_on) break;
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
 * \brief Function to handle incoming messages from external components
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    // json_header = length (2) + json (__MAX_EXT_MSG_SIZE)

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
            }
        }
    }

    b->need = need;
    b->done = done;

    return bytes;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to do something when a new component is connected
 * \return None
 */
static int new_connection(int sock)
{
    return 0;
}

/**
 * \brief Function to do something when a component is disconnected
 * \return None
 */
static int closed_connection(int sock)
{
    clean_buffer(sock);

    return 0;
}

/**
 * @}
 *
 * @}
 */

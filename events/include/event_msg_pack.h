/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

/////////////////////////////////////////////////////////////////////

#include "event_json.h"

/////////////////////////////////////////////////////////////////////

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
                ev_ctx->compnt_list[i]->activated = TRUE;
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

/**
 * \brief Function to deactivate an external component
 * \param c Component context
 */
static int deactivate_external_component(compnt_t *c)
{
    c->activated = FALSE;

    return 0;
}

/////////////////////////////////////////////////////////////////////

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

    char out_str[__MAX_EXT_MSG_SIZE];
    export_to_json(id, type, input, out_str);

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(out_str));
    memcpy(zmq_msg_data(&msg), out_str, strlen(out_str));

    void *sock = zmq_socket(c->push_ctx, ZMQ_PUSH);
    if (zmq_connect(sock, c->pull_addr)) {
        zmq_msg_close(&msg);

        deactivate_external_component(c);

        return -1;
    } else {
        if (zmq_msg_send(&msg, sock, 0) < 0) {
            zmq_close(sock);
            zmq_msg_close(&msg);

            deactivate_external_component(c);

            return -1;
        }

        zmq_close(sock);
        zmq_msg_close(&msg);
    }

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

    char out_str[__MAX_EXT_MSG_SIZE];
    export_to_json(id, type, input, out_str);

    zmq_msg_t out_msg;
    zmq_msg_init_size(&out_msg, strlen(out_str));
    memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));

    void *sock = zmq_socket(c->req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, c->reply_addr)) {
        zmq_msg_close(&out_msg);

        deactivate_external_component(c);

        return -1;
    } else {
        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
            zmq_close(sock);
            zmq_msg_close(&out_msg);

            deactivate_external_component(c);

            return -1;
        } else {
            zmq_msg_close(&out_msg);

            zmq_msg_t in_msg;
            zmq_msg_init(&in_msg);
            int zmq_ret = zmq_msg_recv(&in_msg, sock, 0);

            if (zmq_ret < 0) {
                zmq_msg_close(&in_msg);
                zmq_close(sock);

                deactivate_external_component(c);

                return -1;
            } else {
                char *in_str = zmq_msg_data(&in_msg);
                in_str[zmq_ret] = '\0';

                int ret = import_from_json(&id, &type, in_str, output);

                zmq_close(sock);
                zmq_msg_close(&in_msg);

                return ret;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process requests from components
 * \return NULL
 */
static void *reply_events(void *null)
{
    void *recv = zmq_socket(ev_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://ev_reply_workers");

    while (ev_ctx->ev_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
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
                char out_str[] = "#{\"return\": 0}";

                zmq_msg_t out_msg;
                zmq_msg_init_size(&out_msg, strlen(out_str));
                memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));
                zmq_msg_send(&out_msg, recv, 0);
                zmq_msg_close(&out_msg);
            } else {
                char out_str[] = "#{\"return\": -1}";

                zmq_msg_t out_msg;
                zmq_msg_init_size(&out_msg, strlen(out_str));
                memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));
                zmq_msg_send(&out_msg, recv, 0);
                zmq_msg_close(&out_msg);
            }

            zmq_msg_close(&in_msg);

            continue;
        }

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, in_str, msg.data);

        zmq_msg_close(&in_msg);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        int ret = 0;

        switch (msg.type) {

        // request-response events

        case EV_SW_GET_DPID:
            sw_rw_raise(msg.id, EV_SW_GET_DPID, sizeof(switch_t), (switch_t *)msg.data);
            break;
        case EV_SW_GET_FD:
            sw_rw_raise(msg.id, EV_SW_GET_FD, sizeof(switch_t), (switch_t *)msg.data);
            break;
        case EV_SW_GET_XID:
            sw_rw_raise(msg.id, EV_SW_GET_XID, sizeof(switch_t), (switch_t *)msg.data);
            break;

        default:
            break;
        }

        msg.ret = ret;

        char out_str[__MAX_EXT_MSG_SIZE];
        export_to_json(msg.id, msg.type, msg.data, out_str);

        zmq_msg_t out_msg;
        zmq_msg_init_size(&out_msg, strlen(out_str));
        memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));
        zmq_msg_send(&out_msg, recv, 0);
        zmq_msg_close(&out_msg);

        if (!ev_ctx->ev_on) break;
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

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive events from external components
 * \return NULL
 */
static void *receive_events(void *null)
{
    while (ev_ctx->ev_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, ev_pull_in_sock, 0);

        if (!ev_ctx->ev_on) {
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
 * \brief Function to process events in an event queue
 * \return NULL
 */
static void *deliver_events(void *null)
{
    void *recv = zmq_socket(ev_pull_out_ctx, ZMQ_PULL);
    zmq_connect(recv, "inproc://ev_pull_workers");

    while (ev_ctx->ev_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
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

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, in_str, msg.data);

        zmq_msg_close(&in_msg);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        switch (msg.type) {

        // upstream events

        case EV_OFP_MSG_IN:
            raw_ev_raise(msg.id, EV_OFP_MSG_IN, sizeof(raw_msg_t), (const raw_msg_t *)msg.data);
            break;
        case EV_DP_RECEIVE_PACKET:
            pktin_ev_raise(msg.id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), (const pktin_t *)msg.data);
            break;
        case EV_DP_FLOW_EXPIRED:
            flow_ev_raise(msg.id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_FLOW_DELETED:
            flow_ev_raise(msg.id, EV_DP_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_FLOW_STATS:
            flow_ev_raise(msg.id, EV_DP_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_AGGREGATE_STATS:
            flow_ev_raise(msg.id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_PORT_ADDED:
            port_ev_raise(msg.id, EV_DP_PORT_ADDED, sizeof(port_t), (const port_t *)msg.data);
            break;
        case EV_DP_PORT_MODIFIED:
            port_ev_raise(msg.id, EV_DP_PORT_MODIFIED, sizeof(port_t), (const port_t *)msg.data);
            break;
        case EV_DP_PORT_DELETED:
            port_ev_raise(msg.id, EV_DP_PORT_DELETED, sizeof(port_t), (const port_t *)msg.data);
            break;
        case EV_DP_PORT_STATS:
            port_ev_raise(msg.id, EV_DP_PORT_STATS, sizeof(port_t), (const port_t *)msg.data);
            break;

        // downstream events

        case EV_OFP_MSG_OUT:
            raw_ev_raise(msg.id, EV_OFP_MSG_OUT, sizeof(raw_msg_t), (const raw_msg_t *)msg.data);
            break;
        case EV_DP_SEND_PACKET:
            pktout_ev_raise(msg.id, EV_DP_SEND_PACKET, sizeof(pktout_t), (const pktout_t *)msg.data);
            break;
        case EV_DP_INSERT_FLOW:
            flow_ev_raise(msg.id, EV_DP_INSERT_FLOW, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_MODIFY_FLOW:
            flow_ev_raise(msg.id, EV_DP_MODIFY_FLOW, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_DELETE_FLOW:
            flow_ev_raise(msg.id, EV_DP_DELETE_FLOW, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_REQUEST_FLOW_STATS:
            flow_ev_raise(msg.id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_REQUEST_AGGREGATE_STATS:
            flow_ev_raise(msg.id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_DP_REQUEST_PORT_STATS:
            port_ev_raise(msg.id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), (const port_t *)msg.data);
            break;

        // internal events

        case EV_SW_NEW_CONN:
            sw_ev_raise(msg.id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_SW_EXPIRED_CONN:
            sw_ev_raise(msg.id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_SW_CONNECTED:
            sw_ev_raise(msg.id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_SW_DISCONNECTED:
            sw_ev_raise(msg.id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_SW_UPDATE_CONFIG:
            sw_ev_raise(msg.id, EV_SW_UPDATE_CONFIG, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_SW_UPDATE_DESC:
            sw_ev_raise(msg.id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)msg.data);
            break;
        case EV_HOST_ADDED:
            host_ev_raise(msg.id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)msg.data);
            break;
        case EV_HOST_DELETED:
            host_ev_raise(msg.id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)msg.data);
            break;
        case EV_LINK_ADDED:
            port_ev_raise(msg.id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)msg.data);
            break;
        case EV_LINK_DELETED:
            port_ev_raise(msg.id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)msg.data);
            break;
        case EV_FLOW_ADDED:
            flow_ev_raise(msg.id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_FLOW_DELETED:
            flow_ev_raise(msg.id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg.data);
            break;
        case EV_RS_UPDATE_USAGE:
            rs_ev_raise(msg.id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)msg.data);
            break;
        case EV_TR_UPDATE_STATS:
            tr_ev_raise(msg.id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)msg.data);
            break;
        case EV_LOG_UPDATE_MSGS:
            log_ev_raise(msg.id, EV_LOG_UPDATE_MSGS, strlen((const char *)msg.data), (const char *)msg.data);
            break;

        // log events

        case EV_LOG_DEBUG:
            log_ev_raise(msg.id, EV_LOG_DEBUG, strlen((const char *)msg.data), (const char *)msg.data);
            break;
        case EV_LOG_INFO:
            log_ev_raise(msg.id, EV_LOG_INFO, strlen((const char *)msg.data), (const char *)msg.data);
            break;
        case EV_LOG_WARN:
            log_ev_raise(msg.id, EV_LOG_WARN, strlen((const char *)msg.data), (const char *)msg.data);
            break;
        case EV_LOG_ERROR:
            log_ev_raise(msg.id, EV_LOG_ERROR, strlen((const char *)msg.data), (const char *)msg.data);
            break;
        case EV_LOG_FATAL:
            log_ev_raise(msg.id, EV_LOG_FATAL, strlen((const char *)msg.data), (const char *)msg.data);
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
 * \param ctx The context of the Barista NOS
 */
int destroy_ev_workers(ctx_t *ctx)
{
    ctx->ev_on = FALSE;

    waitsec(1, 0);

    zmq_close(ev_pull_in_sock);
    zmq_close(ev_pull_out_sock);
    zmq_close(ev_rep_comp);
    zmq_close(ev_rep_work);

    zmq_ctx_destroy(ev_pull_in_ctx);
    zmq_ctx_destroy(ev_pull_out_ctx);
    zmq_ctx_destroy(ev_rep_ctx);

    return 0;
}

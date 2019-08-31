/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/**
 * \brief Function to activate an external component
 * \param msg Handshake message
 */
static int activate_external_component(char *msg)
{
    json_t *json = NULL;
    json_error_t error;

    json = json_loads(msg, 0, &error);
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

                if (compnt->site == COMPNT_INTERNAL) return -1;

                compnt->activated = TRUE;

                json_decref(json);

                return 0;
            } else {
                break;
            }
        }
    }

    json_decref(json);

    return -1;
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
static int ev_push_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input)
{
    if (!c->activated) return -1;

    char json[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json, 0);

    void *push_sock = zmq_socket(c->push_ctx, ZMQ_PUSH);

    if (zmq_connect(push_sock, c->push_addr)) {
        PERROR("zmq_connect");
        return -1;
    }

    //printf("%s: %s\n", __FUNCTION__, json);

    zmq_send(push_sock, json, len, 0);

    zmq_close(push_sock);

    return 0;
}

/**
 * \brief Function to send events to an external component and receive its responses
 * \param c Component context
 * \param id Component ID
 * \param type Event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the component
 */
static int ev_send_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!c->activated) return -1;

    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json_in, 0);

    void *req_sock = zmq_socket(c->req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, c->req_addr)) {
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

    if (c->in_perm[type] & COMPNT_WRITE && msg.id == id && msg.type == type)
        memcpy(output, msg.data, size);

    zmq_close(req_sock);

    return msg.ret;
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
        ret = pktin_ev_raise(msg->id, EV_DP_RECEIVE_PACKET, sizeof(pktin_t), (const pktin_t *)msg->data);
        break;
    case EV_DP_FLOW_EXPIRED:
        ret = flow_ev_raise(msg->id, EV_DP_FLOW_EXPIRED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_FLOW_DELETED:
        ret = flow_ev_raise(msg->id, EV_DP_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_FLOW_STATS:
        ret = flow_ev_raise(msg->id, EV_DP_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_AGGREGATE_STATS:
        ret = flow_ev_raise(msg->id, EV_DP_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_PORT_ADDED:
        ret = port_ev_raise(msg->id, EV_DP_PORT_ADDED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_MODIFIED:
        ret = port_ev_raise(msg->id, EV_DP_PORT_MODIFIED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_DELETED:
        ret = port_ev_raise(msg->id, EV_DP_PORT_DELETED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_PORT_STATS:
        ret = port_ev_raise(msg->id, EV_DP_PORT_STATS, sizeof(port_t), (const port_t *)msg->data);
        break;

    // downstream events

    case EV_DP_SEND_PACKET:
        ret = pktout_ev_raise(msg->id, EV_DP_SEND_PACKET, sizeof(pktout_t), (const pktout_t *)msg->data);
        break;
    case EV_DP_INSERT_FLOW:
        ret = flow_ev_raise(msg->id, EV_DP_INSERT_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_MODIFY_FLOW:
        ret = flow_ev_raise(msg->id, EV_DP_MODIFY_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_DELETE_FLOW:
        ret = flow_ev_raise(msg->id, EV_DP_DELETE_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_REQUEST_FLOW_STATS:
        ret = flow_ev_raise(msg->id, EV_DP_REQUEST_FLOW_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_REQUEST_AGGREGATE_STATS:
        ret = flow_ev_raise(msg->id, EV_DP_REQUEST_AGGREGATE_STATS, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_DP_MODIFY_PORT:
        ret = port_ev_raise(msg->id, EV_DP_MODIFY_PORT, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_DP_REQUEST_PORT_STATS:
        ret = port_ev_raise(msg->id, EV_DP_REQUEST_PORT_STATS, sizeof(port_t), (const port_t *)msg->data);
        break;

    // internal events (request-response)

    case EV_SW_GET_DPID:
        ret = sw_rw_raise(msg->id, EV_SW_GET_DPID, sizeof(switch_t), (switch_t *)msg->data);
        break;
    case EV_SW_GET_FD:
        ret = sw_rw_raise(msg->id, EV_SW_GET_FD, sizeof(switch_t), (switch_t *)msg->data);
        break;
    case EV_SW_GET_XID:
        ret = sw_rw_raise(msg->id, EV_SW_GET_XID, sizeof(switch_t), (switch_t *)msg->data);
        break;

    // internal events (notification)

    case EV_SW_NEW_CONN:
        ret = sw_ev_raise(msg->id, EV_SW_NEW_CONN, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_ESTABLISHED_CONN:
        ret = sw_ev_raise(msg->id, EV_SW_ESTABLISHED_CONN, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_EXPIRED_CONN:
        ret = sw_ev_raise(msg->id, EV_SW_EXPIRED_CONN, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_CONNECTED:
        ret = sw_ev_raise(msg->id, EV_SW_CONNECTED, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_DISCONNECTED:
        ret = sw_ev_raise(msg->id, EV_SW_DISCONNECTED, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_SW_UPDATE_DESC:
        ret = sw_ev_raise(msg->id, EV_SW_UPDATE_DESC, sizeof(switch_t), (const switch_t *)msg->data);
        break;
    case EV_HOST_ADDED:
        ret = host_ev_raise(msg->id, EV_HOST_ADDED, sizeof(host_t), (const host_t *)msg->data);
        break;
    case EV_HOST_DELETED:
        ret = host_ev_raise(msg->id, EV_HOST_DELETED, sizeof(host_t), (const host_t *)msg->data);
        break;
    case EV_LINK_ADDED:
        ret = port_ev_raise(msg->id, EV_LINK_ADDED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_LINK_DELETED:
        ret = port_ev_raise(msg->id, EV_LINK_DELETED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case EV_FLOW_ADDED:
        ret = flow_ev_raise(msg->id, EV_FLOW_ADDED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_FLOW_DELETED:
        ret = flow_ev_raise(msg->id, EV_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case EV_RS_UPDATE_USAGE:
        ret = rs_ev_raise(msg->id, EV_RS_UPDATE_USAGE, sizeof(resource_t), (const resource_t *)msg->data);
        break;
    case EV_TR_UPDATE_STATS:
        ret = tr_ev_raise(msg->id, EV_TR_UPDATE_STATS, sizeof(traffic_t), (const traffic_t *)msg->data);
        break;
    case EV_LOG_UPDATE_MSGS:
        ret = log_ev_raise(msg->id, EV_LOG_UPDATE_MSGS, strlen((const char *)msg->data), (const char *)msg->data);
        break;

    // log events

    case EV_LOG_DEBUG:
        ret = log_ev_raise(msg->id, EV_LOG_DEBUG, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_INFO:
        ret = log_ev_raise(msg->id, EV_LOG_INFO, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_WARN:
        ret = log_ev_raise(msg->id, EV_LOG_WARN, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_ERROR:
        ret = log_ev_raise(msg->id, EV_LOG_ERROR, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case EV_LOG_FATAL:
        ret = log_ev_raise(msg->id, EV_LOG_FATAL, strlen((const char *)msg->data), (const char *)msg->data);
        break;

    default:
        break;
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive events
 * \param null NULL
 */
static void *pull_events(void *null)
{
    while (ev_ctx->ev_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!ev_ctx->ev_on) break;
        else if (zmq_recv(ev_pull_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data =data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        process_events(&msg);

        if (!ev_ctx->ev_on) break;
    }

    DEBUG("pull_events() is terminated\n");

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle requests from components and reply them
 * \param null NULL
 */
static void *reply_events(void *null)
{
    while (ev_ctx->ev_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!ev_ctx->ev_on) break;
        else if (zmq_recv(ev_rep_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        // handshake with an external component
        if (json[0] == '#') {
            if (activate_external_component(json + 1) == 0)
                strcpy(json, "#{\"return\": 0}");
            else
                strcpy(json, "#{\"return\": -1}");

            zmq_send(ev_rep_sock, json, strlen(json), 0);

            continue;
        }

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data =data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > EV_NUM_EVENTS) continue;

        msg.ret = process_events(&msg);
        export_to_json(msg.id, msg.type, msg.data, json, msg.ret);
        zmq_send(ev_rep_sock, json, strlen(json), 0);

        if (!ev_ctx->ev_on) break;
    }

    DEBUG("reply_events() is terminated\n");

    return NULL;
}

/**
 * @}
 *
 * @}
 */

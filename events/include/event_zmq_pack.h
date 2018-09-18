/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include <zmq.h>
#include "base64.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send events to a component
 * \return NULL
 */
static int ev_push_ext_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *data)
{
    msg_t msg = {0};
    msg.id = id;
    msg.type = type;
    memmove(msg.data, data, size);

    char *str = base64_encode((char *)&msg, sizeof(msg_t));
    zmq_send(c->push_sock, str, strlen(str), 0);
    FREE(str);

    return 0;
}

static int ev_send_ext_msg(compnt_t *c, uint32_t id, uint16_t type, uint16_t size, const void *data, event_out_t *out)
{
    msg_t msg = {0};
    msg.id = id;
    msg.type = type;
    memmove(msg.data, data, size);

    char *str = base64_encode((char *)&msg, sizeof(msg_t));
    zmq_send(c->push_sock, str, strlen(str), 0);
    FREE(str);

    //

    return msg.ret;
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

    while (ev_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(recv, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!ev_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > EV_NUM_EVENTS) continue;

        switch (msg->type) {

        // request-response events

        case EV_SW_GET_DPID:
            sw_rw_raise(msg->id, EV_SW_GET_DPID, sizeof(switch_t), (switch_t *)msg->data);
            break;
        case EV_SW_GET_FD:
            sw_rw_raise(msg->id, EV_SW_GET_FD, sizeof(switch_t), (switch_t *)msg->data);
            break;
        case EV_SW_GET_XID:
            sw_rw_raise(msg->id, EV_SW_GET_XID, sizeof(switch_t), (switch_t *)msg->data);
            break;

        default:
            break;
        }

        FREE(decoded);

        char *str = base64_encode((char *)msg, sizeof(msg_t));
        zmq_send(recv, str, strlen(str), 0);
        FREE(str);

        if (!ev_on) break;
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
    while (ev_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(ev_pull_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!ev_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > EV_NUM_EVENTS) continue;

        switch (msg->type) {

        // upstream events

        case EV_OFP_MSG_IN:
            raw_ev_raise(msg->id, EV_OFP_MSG_IN, sizeof(raw_msg_t), (const raw_msg_t *)msg->data);
            break;
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

        case EV_OFP_MSG_OUT:
            raw_ev_raise(msg->id, EV_OFP_MSG_OUT, sizeof(raw_msg_t), (const raw_msg_t *)msg->data);
            break;
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

        // internal events

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

        FREE(decoded);
    }

    return NULL;
}

/**
 * \brief Function to destroy an event queue
 * \return None
 */
int destroy_ev_workers(ctx_t *ctx)
{
    ev_on = FALSE;

    waitsec(1, 0);

    zmq_close(ev_pull_sock);
    zmq_ctx_destroy(ev_pull_ctx);

    zmq_close(ev_rep_comp);
    zmq_close(ev_rep_work);
    zmq_ctx_destroy(ev_rep_ctx);

    return 0;
}

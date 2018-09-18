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
 * \brief Function to send events to an application
 * \return None
 */
static int av_push_ext_msg(app_t *c, uint32_t id, uint16_t type, uint16_t size, const void *data)
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

/**
 * \brief Function to send events to an application and receive responses from it
 * \return The return value received from an application
 */
static int av_send_ext_msg(app_t *c, uint32_t id, uint16_t type, uint16_t size, const void *data, app_event_out_t *out)
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
 * \brief Function to process requests from applications
 * \return NULL
 */
static void *reply_app_events(void *null)
{
    void *recv = zmq_socket(av_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://av_reply_workers");

    while (av_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(recv, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!av_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > AV_NUM_EVENTS) continue;

        switch (msg->type) {

        // request-response events

        default:
            break;
        }

        FREE(decoded);

        char *str = base64_encode((char *)msg, sizeof(msg_t));
        zmq_send(recv, str, strlen(str), 0);
        FREE(str);

        if (!av_on) break;
    }

    zmq_close(recv);

    return NULL;
}

/**
 * \brief Function to connect work threads to application threads via a queue proxy
 * \return NULL
 */
static void *reply_proxy(void *null)
{
    zmq_proxy(av_rep_app, av_rep_work, NULL);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive app events from external applications
 * \return NULL
 */
static void *receive_app_events(void *null)
{
    while (av_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(av_pull_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!av_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > AV_NUM_EVENTS) continue;

        switch (msg->type) {

        // upstream events

        case AV_DP_RECEIVE_PACKET:
            pktin_av_raise(msg->id, AV_DP_RECEIVE_PACKET, sizeof(pktin_t), (const pktin_t *)msg->data);
            break;
        case AV_DP_FLOW_EXPIRED:
            flow_av_raise(msg->id, AV_DP_FLOW_EXPIRED, sizeof(flow_t), (const flow_t *)msg->data);
            break;
        case AV_DP_FLOW_DELETED:
            flow_av_raise(msg->id, AV_DP_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
            break;
        case AV_DP_PORT_ADDED:
            port_av_raise(msg->id, AV_DP_PORT_ADDED, sizeof(port_t), (const port_t *)msg->data);
            break;
        case AV_DP_PORT_MODIFIED:
            port_av_raise(msg->id, AV_DP_PORT_MODIFIED, sizeof(port_t), (const port_t *)msg->data);
            break;
        case AV_DP_PORT_DELETED:
            port_av_raise(msg->id, AV_DP_PORT_DELETED, sizeof(port_t), (const port_t *)msg->data);
            break;

        // downstream events

        case AV_DP_SEND_PACKET:
            pktout_av_raise(msg->id, AV_DP_SEND_PACKET, sizeof(pktout_t), (const pktout_t *)msg->data);
            break;
        case AV_DP_INSERT_FLOW:
            flow_av_raise(msg->id, AV_DP_INSERT_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
            break;
        case AV_DP_MODIFY_FLOW:
            flow_av_raise(msg->id, AV_DP_MODIFY_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
            break;
        case AV_DP_DELETE_FLOW:
            flow_av_raise(msg->id, AV_DP_DELETE_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
            break;

        // log events

        case AV_LOG_DEBUG:
            log_av_raise(msg->id, AV_LOG_DEBUG, strlen((const char *)msg->data), (const char *)msg->data);
            break;
        case AV_LOG_INFO:
            log_av_raise(msg->id, AV_LOG_INFO, strlen((const char *)msg->data), (const char *)msg->data);
            break;
        case AV_LOG_WARN:
            log_av_raise(msg->id, AV_LOG_WARN, strlen((const char *)msg->data), (const char *)msg->data);
            break;
        case AV_LOG_ERROR:
            log_av_raise(msg->id, AV_LOG_ERROR, strlen((const char *)msg->data), (const char *)msg->data);
            break;
        case AV_LOG_FATAL:
            log_av_raise(msg->id, AV_LOG_FATAL, strlen((const char *)msg->data), (const char *)msg->data);
            break;

        default:
            break;
        }

        FREE(decoded);
    }

    return NULL;
}

/**
 * \brief Function to destroy an app event queue
 * \return None
 */
int destroy_av_workers(ctx_t *ctx)
{
    av_on = FALSE;

    waitsec(1, 0);

    zmq_close(av_pull_sock);
    zmq_ctx_destroy(av_pull_ctx);

    zmq_close(av_rep_app);
    zmq_close(av_rep_work);
    zmq_ctx_destroy(av_rep_ctx);

    return 0;
}

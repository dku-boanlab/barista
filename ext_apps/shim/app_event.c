/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_shim
 * @{
 * \defgroup app_event Application Event Handler
 * \brief Functions to manage app events for external applications
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "app_event.h"
#include "application.h"
#include "application_info.h"

#include <zmq.h>
#include "base64.h"

/////////////////////////////////////////////////////////////////////

/** \brief The configuration of a target application */
extern app_t app;

/** \brief The running flag for app event workers */
int av_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief The contexts to push and request events */
void *av_push_ctx, *av_req_ctx;

/** \brief The sockets to push and request events */
void *av_push_sock, *av_req_sock;

/////////////////////////////////////////////////////////////////////

/** \brief The contexts to pull and reply events */
void *av_pull_ctx, *av_rep_ctx;

/** \brief The sockets to pull and reply events */
void *av_pull_sock, *av_rep_sock;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send events to the Barista NOS
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param data The pointer of the given data
 */
static int av_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *data)
{
    msg_t msg = {0};
    msg.id = id;
    msg.type = type;
    memmove(msg.data, data, size);

    char *str = base64_encode((char *)&msg, sizeof(msg_t));
    zmq_send(av_push_sock, str, strlen(str), 0);
    FREE(str);

    return 0;
}

// Downstream events ////////////////////////////////////////////////

/** \brief AV_DP_SEND_PACKET */
void av_dp_send_packet(uint32_t id, const pktout_t *data) { av_push_msg(id, AV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief AV_DP_INSERT_FLOW */
void av_dp_insert_flow(uint32_t id, const flow_t *data) { av_push_msg(id, AV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_MODIFY_FLOW */
void av_dp_modify_flow(uint32_t id, const flow_t *data) { av_push_msg(id, AV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_DELETE_FLOW */
void av_dp_delete_flow(uint32_t id, const flow_t *data) { av_push_msg(id, AV_DP_DELETE_FLOW, sizeof(flow_t), data); }

// Internal events (request-response) ///////////////////////////////

// Log events ///////////////////////////////////////////////////////

/** \brief AV_LOG_DEBUG */
void av_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    av_push_msg(id, AV_LOG_DEBUG, strlen(log), log);
}
/** \brief AV_LOG_INFO */
void av_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    av_push_msg(id, AV_LOG_INFO, strlen(log), log);
}
/** \brief AV_LOG_WARN */
void av_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    av_push_msg(id, AV_LOG_WARN, strlen(log), log);
}
/** \brief AV_LOG_ERROR */
void av_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    av_push_msg(id, AV_LOG_ERROR, strlen(log), log);
}
/** \brief AV_LOG_FATAL */
void av_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    av_push_msg(id, AV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process app events in an app event queue and reply outputs
 * \return NULL
 */
static void *reply_app_events(void *null)
{
    waitsec(1, 0);

    while (av_worker_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(av_rep_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!av_worker_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > AV_NUM_EVENTS) continue;

        app_event_out_t out = {0};

        out.id = msg->id;
        out.type = msg->type;
        out.data = (char *)msg->data;

        const app_event_t *in = (const app_event_t *)&out;
        int ret = 0;

        switch (out.type) {

        // upstream events

        case AV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_FLOW_EXPIRED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_PORT_ADDED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_PORT_MODIFIED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;

        // internal events

        case AV_SW_CONNECTED:
            out.length = sizeof(switch_t);
            ret = app.handler(in, &out);
            break;
        case AV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            ret = app.handler(in, &out);
            break;
        case AV_HOST_ADDED:
            out.length = sizeof(host_t);
            ret = app.handler(in, &out);
            break;
        case AV_HOST_DELETED:
            out.length = sizeof(host_t);
            ret = app.handler(in, &out);
            break;
        case AV_LINK_ADDED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;
        case AV_LINK_DELETED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;
        case AV_FLOW_ADDED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;

        default:
            break;
        }

        FREE(decoded);

        msg->ret = ret;

        char *str = base64_encode((char *)msg, sizeof(msg_t));
        zmq_send(av_rep_sock, str, strlen(str), 0);
        FREE(str);

        if (!av_worker_on) break;
    }

    return NULL;
}

/**
 * \brief Function to process app events in an app event queue
 * \return NULL
 */
static void *deliver_app_events(void *null)
{
    waitsec(1, 0);

    while (av_worker_on) {
        char buff[__MAX_ZMQ_MSG_SIZE] = {0};
        zmq_recv(av_pull_sock, buff, __MAX_ZMQ_MSG_SIZE, 0);

        if (!av_worker_on) break;

        char *decoded = base64_decode(buff);
        msg_t *msg = (msg_t *)decoded;

        if (msg->id == 0) continue;
        else if (msg->type > AV_NUM_EVENTS) continue;

        app_event_out_t out = {0};

        out.id = msg->id;
        out.type = msg->type;
        out.data = (char *)msg->data;

        const app_event_t *in = (const app_event_t *)&out;

        switch (out.type) {

        // upstream events

        case AV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            app.handler(in, &out);
            break;
        case AV_DP_FLOW_EXPIRED:
            out.length = sizeof(flow_t);
            break;
        case AV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
            break;
        case AV_DP_PORT_ADDED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;
        case AV_DP_PORT_MODIFIED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;
        case AV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;

        // internal events

        case AV_SW_CONNECTED:
            out.length = sizeof(switch_t);
            app.handler(in, &out);
            break;
        case AV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            app.handler(in, &out);
            break;
        case AV_HOST_ADDED:
            out.length = sizeof(host_t);
            app.handler(in, &out);
            break;
        case AV_HOST_DELETED:
            out.length = sizeof(host_t);
            app.handler(in, &out);
            break;
        case AV_LINK_ADDED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;
        case AV_LINK_DELETED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;
        case AV_FLOW_ADDED:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
            break;
        case AV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
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
 * \brief Function to destroy an app event queue
 * \param ctx Context (= NULL)
 */
int destroy_av_workers(ctx_t *ctx)
{
    av_worker_on = FALSE;

    waitsec(1, 0);

    zmq_close(av_push_sock);
    zmq_ctx_destroy(av_push_ctx);

    zmq_close(av_pull_sock);
    zmq_ctx_destroy(av_pull_ctx);

    zmq_close(av_req_sock);
    //zmq_ctx_destroy(av_req_ctx);

    zmq_close(av_rep_sock);
    zmq_ctx_destroy(av_rep_ctx);

    return 0;
}

/**
 * \brief Function to initialize the app event handler
 * \param ctx Context (= NULL)
 */
int app_event_init(ctx_t *ctx)
{
    av_worker_on = TRUE;

    // Push (downstream)

    av_push_ctx = zmq_ctx_new();
    av_push_sock = zmq_socket(av_push_ctx, ZMQ_PUSH);

    const int timeout = 1000;
    zmq_setsockopt(av_push_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(av_push_sock, EXT_APP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    // Pull (upstream, intstream)

    av_pull_ctx = zmq_ctx_new();
    av_pull_sock = zmq_socket(av_pull_ctx, ZMQ_PULL);

    zmq_setsockopt(av_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_connect(av_pull_sock, TARGET_APP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    int i;
    for (i=0; i<__NUM_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &deliver_app_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    // Request (intsteam)

    av_req_ctx = zmq_ctx_new();
    av_req_sock = zmq_socket(av_req_ctx, ZMQ_REQ);

    zmq_setsockopt(av_req_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));
    zmq_setsockopt(av_req_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(av_req_sock, EXT_APP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    // Reply (upstream, intstream)

    av_rep_ctx = zmq_ctx_new();
    av_rep_sock = zmq_socket(av_req_ctx, ZMQ_REP);

    zmq_setsockopt(av_rep_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));
    zmq_setsockopt(av_rep_sock, ZMQ_SNDTIMEO, &timeout, sizeof(int));

    if (zmq_connect(av_rep_sock, TARGET_APP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    for (i=0; i<__NUM_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &reply_app_events, NULL) < 0) {
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

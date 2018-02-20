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

/////////////////////////////////////////////////////////////////////

/** \brief The configuration of a target application */
extern app_t app;

/** \brief The running flag for app event workers */
int av_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief The context to push events */
void *av_push_ctx;

/** \biref The socket to push events */
void *av_push_sock;

 /** \brief The context to request events */
void *av_req_ctx;

/** \brief The socket to request events */
void *av_req_sock;

/////////////////////////////////////////////////////////////////////

/** \brief The context to pull events */
void *av_pull_ctx;

/** \brief The socket to pull events */
void *av_pull_sock;

/** \brief The path to pull event */
char app_pull_path[__CONF_WORD_LEN];

/** \brief The context to reply events */
void *av_rep_ctx;

/** \brief The socket to reply events */
void *av_rep_sock;

/** \brief The path to reply event */
char app_rep_path[__CONF_WORD_LEN];

/////////////////////////////////////////////////////////////////////

#define AV_PUSH_MSG(i, t, s, d) { \
    msg_t msg = {0}; \
    msg.id = i; \
    msg.type = t; \
    memmove(msg.data, d, s); \
    zmq_send(av_push_sock, &msg, sizeof(msg_t), 0); \
}

#define AV_WRITE_MSG(i, t, s, d) { \
    msg_t msg = {0}; \
    msg.id = i; \
    msg.type = t; \
    memmove(msg.data, d, s); \
    zmq_send(av_req_sock, &msg, sizeof(msg_t), 0); \
    zmq_recv(av_req_sock, &msg, sizeof(msg_t), 0); \
    memmove(d, msg.data, s); \
}

// Downstream events ////////////////////////////////////////////////

/** \brief AV_DP_SEND_PACKET */
void av_dp_send_packet(uint32_t id, const pktout_t *data) { AV_PUSH_MSG(id, AV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief AV_DP_INSERT_FLOW */
void av_dp_insert_flow(uint32_t id, const flow_t *data) { AV_PUSH_MSG(id, AV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_MODIFY_FLOW */
void av_dp_modify_flow(uint32_t id, const flow_t *data) { AV_PUSH_MSG(id, AV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_DELETE_FLOW */
void av_dp_delete_flow(uint32_t id, const flow_t *data) { AV_PUSH_MSG(id, AV_DP_DELETE_FLOW, sizeof(flow_t), data); }

// Internal events (request-response) ///////////////////////////////

/** \brief AV_SW_GET_INFO (dpid) */
void av_sw_get_info(uint32_t id, switch_t *data) { AV_WRITE_MSG(id, AV_SW_GET_INFO, sizeof(switch_t), data); }
/** \brief AV_SW_GET_ALL_INFO (none) */
void av_sw_get_all_info(uint32_t id, switch_t *data) { AV_WRITE_MSG(id, AV_SW_GET_ALL_INFO, sizeof(switch_t), data); }
/** \brief AV_HOST_GET_INFO (MAC address or IP address) */
void av_host_get_info(uint32_t id, host_t *data) { AV_WRITE_MSG(id, AV_HOST_GET_INFO, sizeof(host_t), data); }
/** \brief AV_HOST_GET_ALL_INFO (none) */
void av_host_get_all_info(uint32_t id, host_t *data) { AV_WRITE_MSG(id, AV_HOST_GET_ALL_INFO, sizeof(host_t), data); }
/** \brief AV_LINK_GET_INFO (dpid) */
void av_link_get_info(uint32_t id, port_t *data) { AV_WRITE_MSG(id, AV_LINK_GET_INFO, sizeof(port_t), data); }
/** \brief AV_LINK_GET_ALL_INFO (none) */
void av_link_get_all_info(uint32_t id, port_t *data) { AV_WRITE_MSG(id, AV_LINK_GET_ALL_INFO, sizeof(port_t), data); }
/** \brief AV_FLOW_GET_INFO (dpid) */
void av_flow_get_info(uint32_t id, flow_t *data) { AV_WRITE_MSG(id, AV_FLOW_GET_INFO, sizeof(flow_t), data); }
/** \brief AV_FLOW_GET_ALL_INFO (none) */
void av_flow_get_all_info(uint32_t id, flow_t *data) { AV_WRITE_MSG(id, AV_FLOW_GET_ALL_INFO, sizeof(flow_t), data); }

// Log events ///////////////////////////////////////////////////////

/** \brief AV_LOG_DEBUG */
void av_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    AV_PUSH_MSG(id, AV_LOG_DEBUG, strlen(log), log);
}
/** \brief AV_LOG_INFO */
void av_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    AV_PUSH_MSG(id, AV_LOG_INFO, strlen(log), log);
}
/** \brief AV_LOG_WARN */
void av_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    AV_PUSH_MSG(id, AV_LOG_WARN, strlen(log), log);
}
/** \brief AV_LOG_ERROR */
void av_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    AV_PUSH_MSG(id, AV_LOG_ERROR, strlen(log), log);
}
/** \brief AV_LOG_FATAL */
void av_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    AV_PUSH_MSG(id, AV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process app events in an app event queue and reply outputs
 * \return NULL
 */
static void *reply_app_events(void *null)
{
    while (av_worker_on) {
        msg_t msg = {0};

        zmq_recv(av_rep_sock, &msg, sizeof(msg), 0);

        app_event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = (char *)msg.data;

        const app_event_t *in = (const app_event_t *)&out;

        int ret = 0;

        switch (msg.type) {

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

        // downstream events
        case AV_DP_SEND_PACKET:
            out.length = sizeof(pktout_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_INSERT_FLOW:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_MODIFY_FLOW:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_DELETE_FLOW:
            out.length = sizeof(flow_t);
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

        }

        msg.ret = ret;

        zmq_send(av_rep_sock, &msg, sizeof(msg), 0);
    }

    return NULL;
}

/**
 * \brief Function to process app events in an app event queue
 * \return NULL
 */
static void *deliver_app_events(void *null)
{
    while (av_worker_on) {
        msg_t msg = {0};

        zmq_recv(av_pull_sock, &msg, sizeof(msg), 0);

        app_event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = (char *)msg.data;

        const app_event_t *in = (const app_event_t *)&out;

        switch (msg.type) {

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

        // downstream events
        case AV_DP_SEND_PACKET:
            out.length = sizeof(pktout_t);
            app.handler(in, &out);
            break;
        case AV_DP_INSERT_FLOW:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
            break;
        case AV_DP_MODIFY_FLOW:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
            break;
        case AV_DP_DELETE_FLOW:
            out.length = sizeof(flow_t);
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
    }

    return NULL;
}

/**
 * \brief Function to destroy an app event queue
 * \param ctx Context (NULL)
 */
int destroy_av_workers(ctx_t *ctx)
{
    av_worker_on = FALSE;

    zmq_close(av_push_sock);
    zmq_ctx_destroy(av_pull_ctx);

    zmq_close(av_pull_sock);
    zmq_ctx_destroy(av_pull_ctx);

    zmq_close(av_req_sock);
    zmq_ctx_destroy(av_req_ctx);

    zmq_close(av_rep_sock);
    zmq_ctx_destroy(av_rep_ctx);

    return 0;
}

/**
 * \brief Function to initialize the app event handler
 * \param ctx Context (NULL)
 */
int app_event_init(ctx_t *ctx)
{
    av_worker_on = TRUE;

    // Push (downstream)

    av_push_ctx = zmq_ctx_new();
    av_push_sock = zmq_socket(av_push_ctx, ZMQ_PUSH);
    if (zmq_connect(av_push_sock, "ipc://../tmp/ext_app_event_handler")) {
        PERROR("zmq_connect");
        return -1;
    }

    // Pull (upstream, intstream)

    sprintf(app_pull_path, "ipc://../tmp/%s_push_pull", TARGET_APP);

    av_pull_ctx = zmq_ctx_new();
    av_pull_sock = zmq_socket(av_pull_ctx, ZMQ_PULL);
    if (zmq_connect(av_pull_sock, app_pull_path)) {
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
    if (zmq_connect(av_req_sock, "ipc://../tmp/app_req_handler")) {
        PERROR("zmq_connect");
        return -1;
    }

    // Reply (upstream, intstream)

    sprintf(app_rep_path, "ipc://../tmp/%s_req_rep", TARGET_APP);

    av_rep_ctx = zmq_ctx_new();
    av_rep_sock = zmq_socket(av_req_ctx, ZMQ_REP);
    if (zmq_connect(av_rep_sock, app_rep_path)) {
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


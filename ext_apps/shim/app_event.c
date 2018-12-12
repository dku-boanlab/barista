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

/////////////////////////////////////////////////////////////////////

/** \brief The configuration of a target application */
extern app_t app;

/** \brief The running flag for app event workers */
int av_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ contexts to push, pull, request and reply events */
void *av_push_ctx, *av_pull_in_ctx, *av_pull_out_ctx, *av_req_ctx, *av_rep_ctx;

/** \brief MQ sockets to pull and reply events */
void *av_pull_in_sock, *av_pull_out_sock, *av_req_sock, *av_rep_app, *av_rep_work;

/////////////////////////////////////////////////////////////////////

#include "app_event_json.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to have a handshake with the Barista NOS
 * \param id Application ID
 * \param name Application name
 */
static int handshake(uint32_t id, char *name)
{
    void *sock = zmq_socket(av_req_ctx, ZMQ_REQ);
    if (zmq_connect(sock, EXT_APP_REPLY_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    } else {
        char out_str[__MAX_EXT_MSG_SIZE];
        sprintf(out_str, "#{\"id\": %u, \"name\": \"%s\"}", app.app_id, app.name);

        zmq_msg_t out_msg;
        zmq_msg_init_size(&out_msg, strlen(out_str));
        memcpy(zmq_msg_data(&out_msg), out_str, strlen(out_str));

        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
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
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param data The pointer of the given data
 */
static int av_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *data)
{
    if (!av_worker_on) return -1;

    char output[__MAX_EXT_MSG_SIZE];
    export_to_json(id, type, data, output);

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(output));
    memcpy(zmq_msg_data(&msg), output, strlen(output));

    void *sock = zmq_socket(av_push_ctx, ZMQ_PUSH);
    if (zmq_connect(sock, EXT_APP_PULL_ADDR)) {
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

    void *recv = zmq_socket(av_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://av_reply_workers");

    while (av_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!av_worker_on) {
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
        else if (msg.type > AV_NUM_EVENTS) continue;

        app_event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = msg.data;

        const app_event_t *in = (const app_event_t *)&out;
        int ret = 0;

        switch (out.type) {

        // upstream events

        case AV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_FLOW_EXPIRED:
        case AV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
            break;
        case AV_DP_PORT_ADDED:
        case AV_DP_PORT_MODIFIED:
        case AV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;

        // internal events

        case AV_SW_CONNECTED:
        case AV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            ret = app.handler(in, &out);
            break;
        case AV_HOST_ADDED:
        case AV_HOST_DELETED:
            out.length = sizeof(host_t);
            ret = app.handler(in, &out);
            break;
        case AV_LINK_ADDED:
        case AV_LINK_DELETED:
            out.length = sizeof(port_t);
            ret = app.handler(in, &out);
            break;
        case AV_FLOW_ADDED:
        case AV_FLOW_MODIFIED:
        case AV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            ret = app.handler(in, &out);
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

        if (!av_worker_on) break;
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

/**
 * \brief Function to receive app events from the Barista NOS
 * \return NULL
 */
static void *receive_app_events(void *null)
{
    waitsec(1, 0);

    while (av_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, av_pull_in_sock, 0);

        if (!av_worker_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        zmq_msg_send(&in_msg, av_pull_out_sock, 0);

        zmq_msg_close(&in_msg);
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

    void *recv = zmq_socket(av_pull_out_ctx, ZMQ_PULL);
    zmq_connect(recv, "inproc://av_pull_workers");

    while (av_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!av_worker_on) {
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
        else if (msg.type > AV_NUM_EVENTS) continue;

        app_event_out_t out = {0};

        out.id = msg.id;
        out.type = msg.type;
        out.data = msg.data;

        const app_event_t *in = (const app_event_t *)&out;

        switch (out.type) {

        // upstream events

        case AV_DP_RECEIVE_PACKET:
            out.length = sizeof(pktin_t);
            app.handler(in, &out);
            break;
        case AV_DP_FLOW_EXPIRED:
        case AV_DP_FLOW_DELETED:
            out.length = sizeof(flow_t);
            break;
        case AV_DP_PORT_ADDED:
        case AV_DP_PORT_MODIFIED:
        case AV_DP_PORT_DELETED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;

        // internal events

        case AV_SW_CONNECTED:
        case AV_SW_DISCONNECTED:
            out.length = sizeof(switch_t);
            app.handler(in, &out);
            break;
        case AV_HOST_ADDED:
        case AV_HOST_DELETED:
            out.length = sizeof(host_t);
            app.handler(in, &out);
            break;
        case AV_LINK_ADDED:
        case AV_LINK_DELETED:
            out.length = sizeof(port_t);
            app.handler(in, &out);
            break;
        case AV_FLOW_ADDED:
        case AV_FLOW_MODIFIED:
        case AV_FLOW_DELETED:
            out.length = sizeof(flow_t);
            app.handler(in, &out);
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
 * \brief Function to destroy an app event queue
 * \param ctx Context (= NULL)
 */
int destroy_av_workers(ctx_t *ctx)
{
    av_worker_on = FALSE;

    waitsec(1, 0);

    zmq_close(av_pull_in_sock);
    zmq_close(av_pull_out_sock);
    zmq_close(av_req_sock);
    zmq_close(av_rep_app);
    zmq_close(av_rep_work);

    zmq_ctx_destroy(av_push_ctx);
    zmq_ctx_destroy(av_pull_in_ctx);
    zmq_ctx_destroy(av_pull_out_ctx);
    zmq_ctx_destroy(av_req_ctx);
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

    // Pull (upstream, intstream)

    av_pull_in_ctx = zmq_ctx_new();
    av_pull_in_sock = zmq_socket(av_pull_in_ctx, ZMQ_PULL);

    if (zmq_bind(av_pull_in_sock, TARGET_APP_PULL_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    av_pull_out_ctx = zmq_ctx_new();
    av_pull_out_sock = zmq_socket(av_pull_out_ctx, ZMQ_PUSH);

    if (zmq_bind(av_pull_out_sock, "inproc://av_pull_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, &receive_app_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &deliver_app_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    // Request (intsteam)

    av_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    av_rep_ctx = zmq_ctx_new();
    av_rep_app = zmq_socket(av_rep_ctx, ZMQ_ROUTER);

    if (zmq_bind(av_rep_app, TARGET_APP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    av_rep_work = zmq_socket(av_rep_ctx, ZMQ_DEALER);

    if (zmq_bind(av_rep_work, "inproc://av_reply_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    for (i=0; i<__NUM_REP_THREADS; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &reply_app_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    if (pthread_create(&thread, NULL, &reply_proxy, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    if (handshake(app.app_id, app.name))
        return -1;

    return 0;
}

/**
 * @}
 *
 * @}
 */

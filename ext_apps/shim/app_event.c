/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup app_shim
 * @{
 * \defgroup app_event External App Event Handler
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

/** \brief The context of an external application */
extern app_t app;

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for external app event handler */
int av_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ context to push app events */
void *av_push_ctx;

/** \brief MQ context to pull app events */
void *av_pull_ctx;

/** \brief MQ socket to pull app events */
void *av_pull_sock;

/** \brief MQ context to request app events */
void *av_req_ctx;

/** \brief MQ context to reply app events */
void *av_rep_ctx;

/** \brief MQ socket to reply app events */
void *av_rep_sock;

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
    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    sprintf(json_in, "#{\"id\": %u, \"name\": \"%s\"}", app.app_id, app.name);
    int len = strlen(json_in);

    void *req_sock = zmq_socket(av_req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, __EXT_APP_REPLY_ADDR)) {
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
 * \brief Function to send app events to the Barista NOS
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 */
static int av_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *input)
{
    if (!av_on) return -1;

    char json[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json, 0);

    void *push_sock = zmq_socket(av_push_ctx, ZMQ_PUSH);

    if (zmq_connect(push_sock, __EXT_APP_PULL_ADDR)) {
        PERROR("zmq_connect");
        return -1;
    }

    //printf("%s: %s\n", __FUNCTION__, json);

    zmq_send(push_sock, json, len, 0);

    zmq_close(push_sock);

    return 0;
}

#if 0
/**
 * \brief Function to send app events to the Barista NOS and receive its responses
 * \param a Application context
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the application
 */
static int av_send_msg(uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!av_on) return -1;

    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json_in, 0);

    void *req_sock = zmq_socket(av_req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, __EXT_APP_REPLY_ADDR)) {
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

    if (app.in_perm[type] & APP_WRITE && msg.id == id && msg.type == type)
        memcpy(output, msg.data, size);

    zmq_close(req_sock);

    return msg.ret;
}
#endif

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

//

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
 * \brief Function to process app events
 * \param msg Application events
 */
static int process_app_events(msg_t *msg)
{
    int ret = 0;

    app_event_out_t out = {0};

    out.id = msg->id;
    out.type = msg->type;
    out.data = msg->data;

    const app_event_t *in = (const app_event_t *)&out;

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

    // downstream events

    case AV_DP_SEND_PACKET:
        out.length = sizeof(pktout_t);
        ret = app.handler(in, &out);
        break;
    case AV_DP_INSERT_FLOW:
    case AV_DP_MODIFY_FLOW:
    case AV_DP_DELETE_FLOW:
        out.length = sizeof(flow_t);
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

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive app events from the Barista NOS
 * \param null NULL
 */
static void *pull_app_events(void *null)
{
    while (av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!av_on) break;
        else if (zmq_recv(av_pull_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        process_app_events(&msg);

        if (!av_on) break;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle requests from the Barista NOS and reply them
 * \param null NULL
 */
static void *reply_app_events(void *null)
{
    while (av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!av_on) break;
        else if (zmq_recv(av_rep_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        msg.ret = process_app_events(&msg);
        export_to_json(msg.id, msg.type, msg.data, json, msg.ret);
        zmq_send(av_rep_sock, json, strlen(json), 0);

        if (!av_on) break;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize the app event handler
 * \param ctx The context of the Barista NOS
 */
int init_app_event(ctx_t *ctx)
{
    pthread_t thread;

    av_on = TRUE;

    // Push (downstream)

    av_push_ctx = zmq_ctx_new();

    // Pull (upstream, intstream)

    av_pull_ctx = zmq_ctx_new();
    av_pull_sock = zmq_socket(av_pull_ctx, ZMQ_PULL);

    if (zmq_bind(av_pull_sock, TARGET_APP_PULL_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    int timeout = 1000;
    zmq_setsockopt(av_pull_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (pthread_create(&thread, NULL, &pull_app_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    // Request (intsteam)

    av_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    av_rep_ctx = zmq_ctx_new();
    av_rep_sock = zmq_socket(av_rep_ctx, ZMQ_REP);

    if (zmq_bind(av_rep_sock, TARGET_APP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    zmq_setsockopt(av_rep_sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (pthread_create(&thread, NULL, &reply_app_events, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    if (handshake(app.app_id, app.name))
        return -1;

    return 0;
}

/**
 * \brief Function to destroy the app event handler
 * \param ctx The context of the Barista NOS
 */
int destroy_app_event(ctx_t *ctx)
{
    av_on = FALSE;

    waitsec(1, 0);

    zmq_close(av_pull_sock);
    zmq_close(av_rep_sock);

    zmq_ctx_destroy(av_pull_ctx);
    zmq_ctx_destroy(av_push_ctx);
    zmq_ctx_destroy(av_rep_ctx);
    zmq_ctx_destroy(av_req_ctx);

    return 0;
}

/**
 * @}
 *
 * @}
 */

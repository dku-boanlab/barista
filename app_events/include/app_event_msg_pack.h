/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

/////////////////////////////////////////////////////////////////////

#include "app_event_json.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to activate an external application
 * \param in_str Handshake message
 */
static int activate_external_application(char *in_str)
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
    for (i=0; i<av_ctx->num_apps; i++) {
        if (av_ctx->app_list[i]->app_id == id) {
            if (strcmp(av_ctx->app_list[i]->name, name) == 0) {
                av_ctx->app_list[i]->activated = TRUE;
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
 * \brief Function to deactivate an external application
 * \param c Application context
 */
static int deactivate_external_application(app_t *c)
{
    c->activated = FALSE;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to send events to an external application
 * \param c Application context
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 */
static int av_push_ext_msg(app_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input)
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

        deactivate_external_application(c);

        return -1;
    } else {
        if (zmq_msg_send(&msg, sock, 0) < 0) {
            zmq_close(sock);
            zmq_msg_close(&msg);

            deactivate_external_application(c);

            return -1;
        }

        zmq_close(sock);
        zmq_msg_close(&msg);
    }

    return 0;
}

/**
 * \brief Function to send events to an external application and receive responses from it
 * \param c Application context
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the application
 */
static int av_send_ext_msg(app_t *c, uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
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

        deactivate_external_application(c);

        return -1;
    } else {
        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
            zmq_close(sock);
            zmq_msg_close(&out_msg);

            deactivate_external_application(c);

            return -1;
        } else {
            zmq_msg_close(&out_msg);

            zmq_msg_t in_msg;
            zmq_msg_init(&in_msg);
            int zmq_ret = zmq_msg_recv(&in_msg, sock, 0);

            if (zmq_ret < 0) {
                zmq_close(sock);
                zmq_msg_close(&in_msg);

                deactivate_external_application(c);

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
 * \brief Function to process requests from external applications and reply outputs
 * \param null NULL
 */
static void *reply_app_events(void *null)
{
    void *recv = zmq_socket(av_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://av_reply_workers");

    while (av_ctx->av_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!av_ctx->av_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        char *in_str = zmq_msg_data(&in_msg);
        in_str[zmq_ret] = '\0';

        // handshake with external app
        if (in_str[0] == '#') {
            if (activate_external_application(in_str+1) == 0) {
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
        else if (msg.type > AV_NUM_EVENTS) continue;

        int ret = 0;

        switch (msg.type) {

        // request-response events

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

        if (!av_ctx->av_on) break;
    }

    zmq_close(recv);

    return NULL;
}

/**
 * \brief Function to connect worker threads to application threads via a queue proxy
 * \param null NULL
 */
static void *reply_proxy(void *null)
{
    zmq_proxy(av_rep_app, av_rep_work, NULL);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process app events
 * \param msg Application events
 */
static int process_app_events(msg_t *msg)
{
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

    return 0;
}

/**
 * \brief Function to receive app events from external applications
 * \param null NULL
 */
static void *receive_app_events(void *null)
{
    while (av_ctx->av_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, av_pull_in_sock, 0);

        if (!av_ctx->av_on) {
            zmq_msg_close(&in_msg);
            break;
        } else if (zmq_ret == -1) {
            zmq_msg_close(&in_msg);
            continue;
        }

        char *in_str = zmq_msg_data(&in_msg);
        in_str[zmq_ret] = '\0';

        zmq_msg_send(&in_msg, av_pull_out_sock, 0);

        zmq_msg_close(&in_msg);
    }

    return NULL;
}

/**
 * \brief Function to get app events from an app event queue
 * \param null NULL
 */
static void *deliver_app_events(void *null)
{
    void *recv = zmq_socket(av_pull_out_ctx, ZMQ_PULL);
    zmq_connect(recv, "inproc://av_pull_workers");

    while (av_ctx->av_on) {
        zmq_msg_t in_msg;
        zmq_msg_init(&in_msg);
        int zmq_ret = zmq_msg_recv(&in_msg, recv, 0);

        if (!av_ctx->av_on) {
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

        process_app_events(&msg);
    }

    zmq_close(recv);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to destroy an app event queue
 * \param ctx The context of the Barista NOS
 */
int destroy_av_workers(ctx_t *ctx)
{
    ctx->av_on = FALSE;

    waitsec(1, 0);

    zmq_close(av_pull_in_sock);
    zmq_close(av_pull_out_sock);
    zmq_close(av_rep_app);
    zmq_close(av_rep_work);

    zmq_ctx_destroy(av_pull_in_ctx);
    zmq_ctx_destroy(av_pull_out_ctx);
    zmq_ctx_destroy(av_rep_ctx);

    return 0;
}

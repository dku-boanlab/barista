/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_events
 * @{
 * \defgroup av_msg External AppEvent Handler
 * \brief Functions to handle app events between the Barista NOS and external applications
 * @{
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
 * \param msg Handshake message
 */
static int activate_external_application(char *msg)
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
    for (i=0; i<av_ctx->num_apps; i++) {
        if (av_ctx->app_list[i]->app_id == id) {
            if (strcmp(av_ctx->app_list[i]->name, name) == 0) {
                app_t *app = av_ctx->app_list[i];

                if (app->activated) {
                    app->activated = FALSE;
                    waitsec(1, 0);
                }

                // connect to an external application

                app->activated = TRUE;

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
 * \brief Function to send app events to an external application
 * \param a Application context
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 */
static int av_push_msg(app_t *a, uint32_t id, uint16_t type, uint16_t size, const void *input)
{
    if (!a->activated) return -1;

    char json[__MAX_EXT_MSG_SIZE] = {0};
    export_to_json(id, type, input, json);

    // do something

    return 0;
}

/**
 * \brief Function to send app events to an external application and receive responses from it
 * \param a Application context
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param input The pointer of the given data
 * \param output The pointer to store the updated data
 * \return The return value received from the application
 */
static int av_send_msg(app_t *a, uint32_t id, uint16_t type, uint16_t size, const void *input, void *output)
{
    if (!a->activated) return -1;

    char json[__MAX_EXT_MSG_SIZE] = {0};
    export_to_json(id, type, input, json);

    // do something

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process app events
 * \param msg Application events
 */
static int process_app_events(msg_t *msg)
{
    int ret = 0;

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

    // internal events (request-reply)

    // internal events (notification)

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

    return ret;
}

/**
 * \brief Function to receive app events
 * \param null NULL
 */
static void *pull_app_events(void *null)
{
    while (av_ctx->av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};
        int zmq_ret = zmq_recv(av_pull_sock, json, __MAX_EXT_MSG_SIZE, 0);

        if (!av_ctx->av_on) break;
        else if (zmq_ret == -1) continue;

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        process_app_events(&msg);

        if (!av_ctx->av_on) break;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to process requests from external applications and reply outputs
 * \param null NULL
 */
static void *reply_app_events(void *null)
{
    while (av_ctx->av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};
        int zmq_ret = zmq_recv(av_rep_sock, json, __MAX_EXT_MSG_SIZE, 0);

        if (!av_ctx->av_on) break;
        else if (zmq_ret == -1) continue;

        // handshake with external app
        if (json[0] == '#') {
            if (activate_external_application(json + 1) == 0)
                strcpy(json, "#{\"return\": 0}");
            else
                strcpy(json, "#{\"return\": -1}");

            zmq_send(av_rep_sock, json, strlen(json), 0);

            continue;
        }

        msg_t msg = {0};
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        msg.ret = process_app_events(&msg);
        export_to_json(msg.id, msg.type, msg.data, json);
        zmq_send(av_rep_sock, json, strlen(json), 0);

        if (!av_ctx->av_on) break;
    }

    return NULL;
}

/**
 * @}
 *
 * @}
 */

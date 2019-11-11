/*
 * Copyright 2015-2019 NSSLab, KAIST
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

                if (app->site == APP_EXTERNAL)
                    app->activated = TRUE;

                json_decref(json);

                return 0;
            } else {
                ALOG_WARN(0, "Blocked the connection of an unauthorized application");
                ALOG_WARN(0, " - Registered Key: %u", av_ctx->app_list[i]->app_id);
                ALOG_WARN(0, " - Registered configuration: %s", av_ctx->app_list[i]->name);
                ALOG_WARN(0, " - Given Key: %u", id);
                ALOG_WARN(0, " - Given configuration: %s", name);
                ALOG_WARN(0, " - Reason: The configuration of the given application is not matched with the registered one.");

                json_decref(json);

                return -1;
            }
        }
    }

    ALOG_WARN(0, "Blocked the connection of an unauthorized application");
    ALOG_WARN(0, " - Registered Key: %u", av_ctx->app_list[i]->app_id);
    ALOG_WARN(0, " - Registered configuration: %s", av_ctx->app_list[i]->name);
    ALOG_WARN(0, " - Given Key: %u", id);
    ALOG_WARN(0, " - Given configuration: %s", name);
    ALOG_WARN(0, " - Reason: The key of the given application is not matched with the registered one.");

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
    int len = export_to_json(id, type, input, json, 0);

    void *push_sock = zmq_socket(a->push_ctx, ZMQ_PUSH);

    if (zmq_connect(push_sock, a->push_addr)) {
        PERROR("zmq_connect");
        return -1;
    }

    //printf("%s: %s\n", __FUNCTION__, json);

    zmq_send(push_sock, json, len, 0);

    zmq_close(push_sock);

    return 0;
}

/**
 * \brief Function to send app events to an external application and receive its responses
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

    char json_in[__MAX_EXT_MSG_SIZE] = {0};
    int len = export_to_json(id, type, input, json_in, 0);

#ifdef __ENABLE_SLOW_ZMQ
    waitsec(0, 100000);
#endif

    void *req_sock = zmq_socket(a->req_ctx, ZMQ_REQ);

    if (zmq_connect(req_sock, a->req_addr)) {
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

    if (a->in_perm[type] & APP_WRITE && msg.id == id && msg.type == type)
        memcpy(output, msg.data, size);

    zmq_close(req_sock);

    return msg.ret;
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
        ret = pktin_av_raise(msg->id, AV_DP_RECEIVE_PACKET, sizeof(pktin_t), (const pktin_t *)msg->data);
        break;
    case AV_DP_FLOW_EXPIRED:
        ret = flow_av_raise(msg->id, AV_DP_FLOW_EXPIRED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case AV_DP_FLOW_DELETED:
        ret = flow_av_raise(msg->id, AV_DP_FLOW_DELETED, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case AV_DP_PORT_ADDED:
        ret = port_av_raise(msg->id, AV_DP_PORT_ADDED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case AV_DP_PORT_MODIFIED:
        ret = port_av_raise(msg->id, AV_DP_PORT_MODIFIED, sizeof(port_t), (const port_t *)msg->data);
        break;
    case AV_DP_PORT_DELETED:
        ret = port_av_raise(msg->id, AV_DP_PORT_DELETED, sizeof(port_t), (const port_t *)msg->data);
        break;

    // downstream events

    case AV_DP_SEND_PACKET:
        ret = pktout_av_raise(msg->id, AV_DP_SEND_PACKET, sizeof(pktout_t), (const pktout_t *)msg->data);
        break;
    case AV_DP_INSERT_FLOW:
        ret = flow_av_raise(msg->id, AV_DP_INSERT_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case AV_DP_MODIFY_FLOW:
        ret = flow_av_raise(msg->id, AV_DP_MODIFY_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;
    case AV_DP_DELETE_FLOW:
        ret = flow_av_raise(msg->id, AV_DP_DELETE_FLOW, sizeof(flow_t), (const flow_t *)msg->data);
        break;

    // internal events (request-reply)

    // internal events (notification)

    // log events

    case AV_LOG_DEBUG:
        ret = log_av_raise(msg->id, AV_LOG_DEBUG, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case AV_LOG_INFO:
        ret = log_av_raise(msg->id, AV_LOG_INFO, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case AV_LOG_WARN:
        ret = log_av_raise(msg->id, AV_LOG_WARN, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case AV_LOG_ERROR:
        ret = log_av_raise(msg->id, AV_LOG_ERROR, strlen((const char *)msg->data), (const char *)msg->data);
        break;
    case AV_LOG_FATAL:
        ret = log_av_raise(msg->id, AV_LOG_FATAL, strlen((const char *)msg->data), (const char *)msg->data);
        break;

    default:
        break;
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to receive app events
 * \param null NULL
 */
static void *pull_app_events(void *null)
{
    while (av_ctx->av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!av_ctx->av_on) break;
        else if (zmq_recv(av_pull_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        process_app_events(&msg);

        if (!av_ctx->av_on) break;
    }

    DEBUG("pull_app_events() is terminated\n");

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle requests from external applications and reply them
 * \param null NULL
 */
static void *reply_app_events(void *null)
{
    while (av_ctx->av_on) {
        char json[__MAX_EXT_MSG_SIZE] = {0};

        if (!av_ctx->av_on) break;
        else if (zmq_recv(av_rep_sock, json, __MAX_EXT_MSG_SIZE, 0) < 0) continue;

        //printf("%s: %s\n", __FUNCTION__, json);

        // handshake with an external appplication
        if (json[0] == '#') {
            if (activate_external_application(json + 1) == 0)
                strcpy(json, "#{\"return\": 0}");
            else
                strcpy(json, "#{\"return\": -1}");

            zmq_send(av_rep_sock, json, strlen(json), 0);

            continue;
        }

        uint8_t data[__MAX_MSG_SIZE] = {0};

        msg_t msg = {0};
        msg.data = data;
        import_from_json(&msg.id, &msg.type, json, msg.data);

        if (msg.id == 0) continue;
        else if (msg.type > AV_NUM_EVENTS) continue;

        msg.ret = process_app_events(&msg);
        export_to_json(msg.id, msg.type, msg.data, json, msg.ret);
        zmq_send(av_rep_sock, json, strlen(json), 0);

        if (!av_ctx->av_on) break;
    }

    DEBUG("reply_app_events() is terminated\n");

    return NULL;
}

/**
 * @}
 *
 * @}
 */

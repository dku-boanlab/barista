/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/** \brief The context of an external application */
extern app_t app;

/** \brief The running flag for external app event workers */
int av_worker_on;

/////////////////////////////////////////////////////////////////////

/** \brief MQ context to request app events */
void *av_req_ctx;

/** \brief MQ context to reply app events */
void *av_rep_ctx;

/** \brief MQ socket to request app events */
void *av_req_sock;

/** \brief MQ socket to reply app events (application-side) */
void *av_rep_app;

/** \brief MQ socket to reply app events (worker-side) */
void *av_rep_work;

/////////////////////////////////////////////////////////////////////

#include "app_event_json.h"

/////////////////////////////////////////////////////////////////////

/** \brief Socket pointer to push app events */
uint32_t av_push_ptr;

/** \brief Socket to push app events */
int av_push_sock[__NUM_PULL_THREADS];

/** \brief Lock for av_push_sock */
pthread_spinlock_t av_push_lock[__NUM_PULL_THREADS];

/////////////////////////////////////////////////////////////////////

#include "app_event_epoll_env.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to clean up a ZMQ message
 * \param data ZMQ message
 * \param hint Hint
 */
void mq_free(void *data, void *hint)
{
    free(data);
}

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
        char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
        sprintf(out_str, "#{\"id\": %u, \"name\": \"%s\"}", app.app_id, app.name);

        zmq_msg_t out_msg;
        zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

        if (zmq_msg_send(&out_msg, sock, 0) < 0) {
            PERROR("zmq_send");
            zmq_close(sock);
            return -1;
        }

        zmq_msg_t in_msg;
        zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
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

/** \brief The header of a JSON message */
typedef struct _json_header {
    uint16_t len;
    char json[__MAX_EXT_MSG_SIZE];
} __attribute__((packed)) json_header;

/**
 * \brief Function to send app events to the Barista NOS
 * \param id Application ID
 * \param type Application event type
 * \param size The size of the given data
 * \param data The pointer of the given data
 */
static int av_push_msg(uint32_t id, uint16_t type, uint16_t size, const void *data)
{
    if (!av_worker_on) return -1;

    json_header jsonh = {0};

    export_to_json(id, type, data, jsonh.json);
    jsonh.len = strlen(jsonh.json) + 2 + 1; // json_len + length variable + NULL

    int remain = jsonh.len;
    int bytes = 0;
    int done = 0;

    jsonh.len = htons(jsonh.len);

    uint32_t ptr = (av_push_ptr++) % __NUM_PULL_THREADS;

    pthread_spin_lock(&av_push_lock[ptr]);

    while (remain > 0) {
        bytes = write(av_push_sock[ptr], (uint8_t *)&jsonh + done, remain);
        if (bytes < 0 && errno != EAGAIN) {
            break;
        } else if (bytes < 0) {
            continue;
        }

        remain -= bytes;
        done += bytes;
    }

    pthread_spin_unlock(&av_push_lock[ptr]);

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
 * \param null NULL
 */
static void *reply_app_events(void *null)
{
    void *recv = zmq_socket(av_rep_ctx, ZMQ_REP);
    zmq_connect(recv, "inproc://av_reply_workers");

    while (av_worker_on) {
        zmq_msg_t in_msg;
        zmq_msg_init_size(&in_msg, __MAX_EXT_MSG_SIZE);
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

        char *out_str = CALLOC(__MAX_EXT_MSG_SIZE, sizeof(char));
        export_to_json(msg.id, msg.type, msg.data, out_str);

        zmq_msg_t out_msg;
        zmq_msg_init_data(&out_msg, out_str, strlen(out_str), mq_free, NULL);

        zmq_msg_send(&out_msg, recv, 0);

        zmq_msg_close(&in_msg);

        if (!av_worker_on) break;
    }

    zmq_close(recv);

    return NULL;
}

/**
 * \brief Function to connect worker threads to app threads via a queue proxy
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
    app_event_out_t out = {0};
    int ret = 0;

    out.id = msg->id;
    out.type = msg->type;
    out.data = msg->data;

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

    return ret;
}

/////////////////////////////////////////////////////////////////////

/** \brief The structure to keep the remaining part of a message */
typedef struct _buffer_t {
    int need; /**< The bytes that it needs to read */
    int done; /**< The bytes that it has */
    uint8_t temp[__MAX_EXT_MSG_SIZE]; /**< Temporary data */
} buffer_t;

/** \brief Buffers for all possible sockets */
buffer_t *buffer;

/**
 * \brief Function to initialize all buffers
 * \return None
 */
static void init_buffers(void)
{
    buffer = (buffer_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(buffer_t));
    if (buffer == NULL) {
        PERROR("calloc");
        return;
    }
}

/**
 * \brief Function to clean up a buffer
 * \param sock Network socket
 */
static void clean_buffer(int sock)
{
    if (buffer)
        memset(&buffer[sock], 0, sizeof(buffer_t));
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from the Barista NOS
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    buffer_t *b = &buffer[sock];

    uint8_t *temp = b->temp;
    int need = b->need;
    int done = b->done;

    int buf_ptr = 0;
    while (bytes > 0) {
        if (0 < done && done < 2) {
            if (done + bytes < 2) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                done += bytes;
                bytes = 0;

                break;
            } else {
                memmove(temp + done, rx_buf + buf_ptr, (2 - done));

                bytes -= (2 - done);
                buf_ptr += (2 - done);

                need = ntohs(((json_header *)temp)->len) - 2;
                done = 2;
            }
        }

        if (need == 0) {
            json_header *jsonh = (json_header *)(rx_buf + buf_ptr);

            if (bytes < 2) {
                memmove(temp, rx_buf + buf_ptr, bytes);

                need = 0;
                done = bytes;

                bytes = 0;
            } else {
                uint16_t len = ntohs(jsonh->len);

                if (bytes < len) {
                    memmove(temp, rx_buf + buf_ptr, bytes);

                    need = len - bytes;
                    done = bytes;

                    bytes = 0;
                } else if (len > 0) {
                    char *in_str = jsonh->json;

                    msg_t msg = {0};
                    import_from_json(&msg.id, &msg.type, in_str, msg.data);

                    if (msg.id != 0 && msg.type < AV_NUM_EVENTS)
                        process_app_events(&msg);

                    bytes -= len;
                    buf_ptr += len;

                    need = 0;
                    done = 0;
                } else {
                    return -1;
                }
            }
        } else {
            json_header *jsonh = (json_header *)temp;

            if (need > bytes) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                need -= bytes;
                done += bytes;

                bytes = 0;
            } else if (jsonh->len > 0) {
                memmove(temp + done, rx_buf + buf_ptr, need);

                char *in_str = jsonh->json;

                msg_t msg = {0};
                import_from_json(&msg.id, &msg.type, in_str, msg.data);

                if (msg.id != 0 && msg.type < AV_NUM_EVENTS)
                    process_app_events(&msg);

                bytes -= need;
                buf_ptr += need;

                need = 0;
                done = 0;
            } else {
                return -1;
            }
        }
    }

    b->need = need;
    b->done = done;

    return bytes;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to do something when the Barista NOS is connected
 * \param sock Network socket
 */
static int new_connection(int sock)
{
    return 0;
}

/**
 * \brief Function to do something when the Barista NOS is disconnected
 * \param sock Network socket
 */
static int closed_connection(int sock)
{
    clean_buffer(sock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to destroy an app event queue
 * \param null NULL
 */
int destroy_av_workers(ctx_t *null)
{
    av_worker_on = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_spin_destroy(&av_push_lock[i]);
        close(av_push_sock[i]);
        av_push_sock[i] = 0;
    }

    destroy_epoll_env();
    FREE(buffer);

    zmq_close(av_req_sock);
    zmq_close(av_rep_app);
    zmq_close(av_rep_work);

    waitsec(1, 0);

    zmq_ctx_destroy(av_req_ctx);
    zmq_ctx_destroy(av_rep_ctx);

    return 0;
}

/**
 * \brief Function to initialize the app event handler
 * \param null NULL
 */
int app_event_init(ctx_t *null)
{
    av_worker_on = TRUE;

    // Push (downstream)

    char push_type[__CONF_WORD_LEN];
    char push_addr[__CONF_WORD_LEN];
    int push_port;

    sscanf(EXT_APP_PULL_ADDR, "%[^:]://%[^:]:%d", push_type, push_addr, &push_port);

    if (strcmp(push_type, "tcp") == 0) {
        struct sockaddr_in push;
        memset(&push, 0, sizeof(push));
        push.sin_family = AF_INET;
        push.sin_addr.s_addr = inet_addr(push_addr);
        push.sin_port = htons(push_port);

        int i;
        for (i=0; i<__NUM_PULL_THREADS; i++) {
            if ((av_push_sock[i] = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                PERROR("socket");
                return -1;
            }

            if (connect(av_push_sock[i], (struct sockaddr *)&push, sizeof(push)) < 0) {
                PERROR("connect");
                return -1;
            }

            pthread_spin_init(&av_push_lock[i], PTHREAD_PROCESS_PRIVATE);
        }
    } else { // ipc
        struct sockaddr_un push;
        memset(&push, 0, sizeof(push));
        push.sun_family = AF_UNIX;
        strcpy(push.sun_path, push_addr);

        int i;
        for (i=0; i<__NUM_PULL_THREADS; i++) {
            if ((av_push_sock[i] = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
                PERROR("socket");
                return -1;
            }

            if (connect(av_push_sock[i], (struct sockaddr *)&push, sizeof(push)) < 0) {
                PERROR("connect");
                return -1;
            }

            pthread_spin_init(&av_push_lock[i], PTHREAD_PROCESS_PRIVATE);
        }
    }

    // Pull (upstream, intstream)

    char pull_type[__CONF_WORD_LEN];
    char pull_addr[__CONF_WORD_LEN];
    int pull_port;

    sscanf(TARGET_APP_PULL_ADDR, "%[^:]://%[^:]:%d", pull_type, pull_addr, &pull_port);

    init_buffers();
    create_epoll_env(pull_type, pull_addr, pull_port);

    pthread_t thread;
    if (pthread_create(&thread, NULL, &socket_listen, NULL) < 0) {
        PERROR("pthread_create");
        return -1;
    }

    // Request (intsteam)

    av_req_ctx = zmq_ctx_new();

    // Reply (upstream, intstream)

    av_rep_ctx = zmq_ctx_new();
    av_rep_app = zmq_socket(av_rep_ctx, ZMQ_ROUTER);

    int timeout = 250;
    zmq_setsockopt(av_rep_app, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_bind(av_rep_app, TARGET_APP_REPLY_ADDR)) {
        PERROR("zmq_bind");
        return -1;
    }

    av_rep_work = zmq_socket(av_rep_ctx, ZMQ_DEALER);

    zmq_setsockopt(av_rep_work, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_bind(av_rep_work, "inproc://av_reply_workers")) {
        PERROR("zmq_bind");
        return -1;
    }

    int i;
    for (i=0; i<__NUM_REP_THREADS; i++) {
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

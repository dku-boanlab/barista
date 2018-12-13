/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup app_events Application Event Handler
 * \brief Functions to manage events for applications
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "app_event.h"
#include "application.h"
#include "cli.h"

/////////////////////////////////////////////////////////////////////

/** \brief Global context structure */
static ctx_t *av_ctx;

/////////////////////////////////////////////////////////////////////

/** \brief MQ contexts to pull and reply app events */
void *av_pull_in_ctx, *av_pull_out_ctx, *av_rep_ctx;

/** \brief MQ sockets to pull and reply app events */
void *av_pull_in_sock, *av_pull_out_sock, *av_rep_app, *av_rep_work;

/////////////////////////////////////////////////////////////////////

/** \brief Switch related trigger function (non-const) */
//static int sw_rw_raise(uint32_t id, uint16_t type, uint16_t len, switch_t *data);
/** \brief Port related trigger function (non-const) */
//static int port_rw_raise(uint32_t id, uint16_t type, uint16_t len, port_t *data);
/** \brief Host related trigger function (non-const) */
//static int host_rw_raise(uint32_t id, uint16_t type, uint16_t len, host_t *data);
/** \brief Flow related trigger function (non-const) */
//static int flow_rw_raise(uint32_t id, uint16_t type, uint16_t len, flow_t *data);

/////////////////////////////////////////////////////////////////////

/** \brief Switch related trigger function (const) */
static int sw_av_raise(uint32_t id, uint16_t type, uint16_t len, const switch_t *data);
/** \brief Port related trigger function (const) */
static int port_av_raise(uint32_t id, uint16_t type, uint16_t len, const port_t *data);
/** \brief Host related trigger function (const) */
static int host_av_raise(uint32_t id, uint16_t type, uint16_t len, const host_t *data);
/** \brief Packet-In related trigger function (const) */
static int pktin_av_raise(uint32_t id, uint16_t type, uint16_t len, const pktin_t *data);
/** \brief Packet-Out related trigger function (const) */
static int pktout_av_raise(uint32_t id, uint16_t type, uint16_t len, const pktout_t *data);
/** \brief Flow related trigger function (const) */
static int flow_av_raise(uint32_t id, uint16_t type, uint16_t len, const flow_t *data);
/** \brief Log related trigger function (const) */
static int log_av_raise(uint32_t id, uint16_t type, uint16_t len, const char *data);

/////////////////////////////////////////////////////////////////////

#include "app_event_msg_pack.h"

// Upstream events //////////////////////////////////////////////////

/** \brief AV_DP_RECEIVE_PACKET */
void av_dp_receive_packet(uint32_t id, const pktin_t *data) { pktin_av_raise(id, AV_DP_RECEIVE_PACKET, sizeof(pktin_t), data); }
/** \brief AV_DP_FLOW_EXPIRED */
void av_dp_flow_expired(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_DP_FLOW_EXPIRED, sizeof(flow_t), data); }
/** \brief AV_DP_FLOW_DELETED */
void av_dp_flow_deleted(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_DP_FLOW_DELETED, sizeof(flow_t), data); }
/** \brief AV_DP_PORT_ADDED */
void av_dp_port_added(uint32_t id, const port_t *data) { port_av_raise(id, AV_DP_PORT_ADDED, sizeof(port_t), data); }
/** \brief AV_DP_PORT_MODIFIED */
void av_dp_port_modified(uint32_t id, const port_t *data) { port_av_raise(id, AV_DP_PORT_MODIFIED, sizeof(port_t), data); }
/** \brief AV_DP_PORT_DELETED */
void av_dp_port_deleted(uint32_t id, const port_t *data) { port_av_raise(id, AV_DP_PORT_DELETED, sizeof(port_t), data); }

// Downstream events ////////////////////////////////////////////////

/** \brief AV_DP_SEND_PACKET */
void av_dp_send_packet(uint32_t id, const pktout_t *data) { pktout_av_raise(id, AV_DP_SEND_PACKET, sizeof(pktout_t), data); }
/** \brief AV_DP_INSERT_FLOW */
void av_dp_insert_flow(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_DP_INSERT_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_MODIFY_FLOW */
void av_dp_modify_flow(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_DP_MODIFY_FLOW, sizeof(flow_t), data); }
/** \brief AV_DP_DELETE_FLOW */
void av_dp_delete_flow(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_DP_DELETE_FLOW, sizeof(flow_t), data); }

// Internal events (request-response) ///////////////////////////////

// Internal events (notification) ///////////////////////////////////

/** \brief AV_SW_CONNECTED */
void av_sw_connected(uint32_t id, const switch_t *data) { sw_av_raise(id, AV_SW_CONNECTED, sizeof(switch_t), data); }
/** \brief AV_SW_DISCONNECTED */
void av_sw_disconnected(uint32_t id, const switch_t *data) { sw_av_raise(id, AV_SW_DISCONNECTED, sizeof(switch_t), data); }
/** \brief AV_HOST_ADDED */
void av_host_added(uint32_t id, const host_t *data) { host_av_raise(id, AV_HOST_ADDED, sizeof(host_t), data); }
/** \brief AV_HOST_DELETED */
void av_host_deleted(uint32_t id, const host_t *data) { host_av_raise(id, AV_HOST_DELETED, sizeof(host_t), data); }
/** \brief AV_LINK_ADDED */
void av_link_added(uint32_t id, const port_t *data) { port_av_raise(id, AV_LINK_ADDED, sizeof(port_t), data); }
/** \brief AV_LINK_DELETED */
void av_link_deleted(uint32_t id, const port_t *data) { port_av_raise(id, AV_LINK_DELETED, sizeof(port_t), data); }
/** \brief AV_FLOW_ADDED */
void av_flow_added(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_FLOW_ADDED, sizeof(flow_t), data); }
/** \brief AV_FLOW_MODIFIED */
void av_flow_modified(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_FLOW_ADDED, sizeof(flow_t), data); }
/** \brief AV_FLOW_DELETED */
void av_flow_deleted(uint32_t id, const flow_t *data) { flow_av_raise(id, AV_FLOW_DELETED, sizeof(flow_t), data); }

// Log events ///////////////////////////////////////////////////////

/** \brief AV_LOG_DEBUG */
void av_log_debug(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_av_raise(id, AV_LOG_DEBUG, strlen(log), log);
}
/** \brief AV_LOG_INFO */
void av_log_info(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_av_raise(id, AV_LOG_INFO, strlen(log), log);
}
/** \brief AV_LOG_WARN */
void av_log_warn(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_av_raise(id, AV_LOG_WARN, strlen(log), log);
}
/** \brief AV_LOG_ERROR */
void av_log_error(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_av_raise(id, AV_LOG_ERROR, strlen(log), log);
}
/** \brief AV_LOG_FATAL */
void av_log_fatal(uint32_t id, char *format, ...) {
    char log[__CONF_STR_LEN] = {0};
    va_list ap;

    va_start(ap, format);
    vsprintf(log, format, ap);
    va_end(ap);

    log_av_raise(id, AV_LOG_FATAL, strlen(log), log);
}

/////////////////////////////////////////////////////////////////////

#ifdef __ENABLE_META_EVENTS
/**
 * \brief Function to process meta app events
 * \return NULL
 */
static void *meta_app_events(void *null)
{
    int *num_events = av_ctx->num_app_events;
    meta_event_t *meta = av_ctx->meta_app_event;

    while (av_on) {
        int av_id;
        for (av_id=0; av_id<__MAX_APP_EVENTS; av_id++) {
            int num_event = num_events[av_id];

            int j;
            for (j=0; j<__MAX_META_EVENTS; j++) {
                if (meta[j].event == av_id) {
                    switch (meta[j].condition) {
                    case META_GT: // >
                        if (num_event > meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_GTE: // >=
                        if (num_event >= meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_LT: // <
                        if (num_event < meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_LTE: // <=
                        if (num_event <= meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    case META_EQ: // ==
                        if (num_event == meta[j].threshold) {
                            //char **args = split_line(meta[j].cmd);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }

            num_events[av_id] = 0;
        }

        waitsec(1, 0);
    }

    return NULL;
}
#endif /* __ENABLE_META_EVENTS */

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize the app event handler
 * \param ctx The context of the Barista NOS
 */
int app_event_init(ctx_t *ctx)
{
    av_ctx = ctx;

    if (av_ctx->av_on == FALSE) {
        pthread_t thread;

#ifdef __ENABLE_META_EVENTS
        if (pthread_create(&thread, NULL, &meta_app_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }
#endif /* __ENABLE_META_EVENTS */

        av_ctx->av_on = TRUE;

        // pull

        av_pull_in_ctx = zmq_ctx_new();
        av_pull_in_sock = zmq_socket(av_pull_in_ctx, ZMQ_PULL);

        if (zmq_bind(av_pull_in_sock, EXT_APP_PULL_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        av_pull_out_ctx = zmq_ctx_new();
        av_pull_out_sock = zmq_socket(av_pull_out_ctx, ZMQ_PUSH);

        if (zmq_bind(av_pull_out_sock, "inproc://av_pull_workers")) {
            PERROR("zmq_bind");
            return -1;
        }

        if (pthread_create(&thread, NULL, &receive_app_events, NULL) < 0) {
            PERROR("pthread_create");
            return -1;
        }

        int i;
        for (i=0; i<__NUM_PULL_THREADS; i++) {
            if (pthread_create(&thread, NULL, &deliver_app_events, NULL) < 0) {
                PERROR("pthread_create");
                return -1;
            }
        }

        // reply

        av_rep_ctx = zmq_ctx_new();
        av_rep_app = zmq_socket(av_rep_ctx, ZMQ_ROUTER);

        if (zmq_bind(av_rep_app, EXT_APP_REPLY_ADDR)) {
            PERROR("zmq_bind");
            return -1;
        }

        av_rep_work = zmq_socket(av_rep_ctx, ZMQ_DEALER);

        if (zmq_bind(av_rep_work, "inproc://av_reply_workers")) {
            PERROR("zmq_bind");
            return -1;
        }

        for (i=0; i<__NUM_REP_THREADS; i++) {
            if (pthread_create(&thread, NULL, &reply_app_events, NULL) < 0) {
                PERROR("pthread_create");
                return -1;
            }
        }

        if (pthread_create(&thread, NULL, &reply_proxy, ctx) < 0) {
            PERROR("pthread_create");
            return -1;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

#define ODP_FUNC compare_switch
#define ODP_TYPE switch_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_port
#define ODP_TYPE port_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_host
#define ODP_TYPE host_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_pktin
#define ODP_TYPE pktin_t
#include "odp_all.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_pktout
#define ODP_TYPE pktout_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

#define ODP_FUNC compare_flow
#define ODP_TYPE flow_t
#include "odp_partial.h"
#undef ODP_TYPE
#undef ODP_FUNC

/////////////////////////////////////////////////////////////////////

/**
 * \brief The code of sw_rw_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME sw_rw_raise
//#define FUNC_TYPE switch_t
//#define FUNC_DATA sw_data
//#include "app_event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of port_rw_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME port_rw_raise
//#define FUNC_TYPE port_t
//#define FUNC_DATA port_data
//#include "app_event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of host_rw_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME host_rw_raise
//#define FUNC_TYPE host_t
//#define FUNC_DATA host_data
//#include "app_event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of flow_rw_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
//#define FUNC_NAME flow_rw_raise
//#define FUNC_TYPE flow_t
//#define FUNC_DATA flow_data
//#include "app_event_rw_raise.h"
//#undef FUNC_NAME
//#undef FUNC_TYPE
//#undef FUNC_DATA

/**
 * \brief The code of sw_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME sw_av_raise
#define FUNC_TYPE switch_t
#define FUNC_DATA sw
#define ODP_FUNC compare_switch
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of port_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME port_av_raise
#define FUNC_TYPE port_t
#define FUNC_DATA port
#define ODP_FUNC compare_port
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of host_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME host_av_raise
#define FUNC_TYPE host_t
#define FUNC_DATA host
#define ODP_FUNC compare_host
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of pktin_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME pktin_av_raise
#define FUNC_TYPE pktin_t
#define FUNC_DATA pktin
#define ODP_FUNC compare_pktin
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of pktout_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME pktout_av_raise
#define FUNC_TYPE pktout_t
#define FUNC_DATA pktout
#define ODP_FUNC compare_pktout
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of flow_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME flow_av_raise
#define FUNC_TYPE flow_t
#define FUNC_DATA flow
#define ODP_FUNC compare_flow
#include "app_event_direct_raise.h"
#undef ODP_FUNC
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * \brief The code of log_av_raise()
 * \param id Application ID
 * \param type App event type
 * \param len The length of data
 * \param data Data
 */
#define FUNC_NAME log_av_raise
#define FUNC_TYPE char
#define FUNC_DATA log
#include "app_event_direct_raise.h"
#undef FUNC_NAME
#undef FUNC_TYPE
#undef FUNC_DATA

/**
 * @}
 *
 * @}
 */

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include "mac2int.h"
#include "ip2int.h"

#include <jansson.h>
#include "libcli.h"
#include <zmq.h>

#include "type.h"

/* Default */

#define __CONF_ARGC 16
#define __CONF_SHORT_LEN 16
#define __CONF_WORD_LEN 256
#define __CONF_STR_LEN 1024
#define __CONF_LSTR_LEN 2048
#define __DEFAULT_TABLE_SIZE 65536

/* Component / Application */

#define BATCH_LOGS 1024
#define LOG_UPDATE_TIME 1

/** \brief The default log file */
#define DEFAULT_LOG_FILE "../log/message.log"

#define FLOW_MGMT_REQUEST_TIME 5
#define TOPO_MGMT_REQUEST_TIME 5
#define STAT_MGMT_REQUEST_TIME 5

#define RESOURCE_MGMT_MONITOR_TIME 1
#define RESOURCE_MGMT_HISTORY 86400

/** \brief The default resource file */
#define DEFAULT_RESOURCE_FILE "../log/resource.log"

#define TRAFFIC_MGMT_MONITOR_TIME 1
#define TRAFFIC_MGMT_HISTORY 86400

/** \brief The default traffic file */
#define DEFAULT_TRAFFIC_FILE "../log/traffic.log"

#define CLUSTER_UPDATE_TIME_NS (1000*1000)
#define CLUSTER_DELIMITER 2000

/** \brief The default application role file */
#define DEFAULT_APP_ROLE_FILE "config/app_events.role"

/** \brief The number of pre-allocated mac entries used in l2_learning and l2_shortest */
#define MAC_PRE_ALLOC 4096

/** \brief The number of pre-allocated host entries used in host_mgmt */
#define HOST_PRE_ALLOC 4096

/** \biref The number of pre-allocated flow entries used in conflict */
#define ARR_PRE_ALLOC 65536

/** \brief The number of pre-allocated flow entries used in flow_mgmt */
#define FLOW_PRE_ALLOC 65536

/** \brief The number of pre-allocated switch entries used in switch_mgmt */
#define SW_PRE_ALLOC 1024

/* Barista NOS */

#define __SHOW_COMPONENT_ID

#define __HASHING_NAME_LENGTH 8

//#define EXT_COMP_PULL_ADDR "tcp://127.0.0.1:5001"
#define EXT_COMP_PULL_ADDR "ipc://tmp/ext_comp_pull"
//#define EXT_COMP_REPLY_ADDR "tcp://127.0.0.1:5002"
#define EXT_COMP_REPLY_ADDR "ipc://tmp/ext_comp_reply"

//#define EXT_APP_PULL_ADDR "tcp://127.0.0.1:6001"
#define EXT_APP_PULL_ADDR "ipc://tmp/ext_app_pull"
//#define EXT_APP_REPLY_ADDR "tcp://127.0.0.1:6002"
#define EXT_APP_REPLY_ADDR "ipc://tmp/ext_app_reply"

/** \brief The default component configuration file */
#define DEFAULT_COMPNT_CONFIG_FILE "config/components.conf"

/** \brief The default component configuration file */
#define BASE_COMPNT_CONFIG_FILE "config/components.base"

/** \brief The default application configuration file */
#define DEFAULT_APP_CONFIG_FILE "config/applications.conf"

/** \brief The default application configuration file */
#define BASE_APP_CONFIG_FILE "config/applications.base"

/** \brief The default operator-defined policy file */
#define DEFAULT_ODP_FILE "config/operator-defined.policy"

/** \brief Font color */
/* @{ */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
/* @} */

/** \brief TRUE, FALSE */
/** @{ */
#define TRUE 1
#define FALSE 0
/** @} */

/** \brief Min function */
#define MIN(a,b) (((a)<(b))?(a):(b))

/** \brief Max function */
#define MAX(a,b) (((a)>(b))?(a):(b))

/** \brief Timer */
#define waitsec(sec, nsec) \
{ \
    struct timespec req; \
    req.tv_sec = sec; \
    req.tv_nsec = nsec; \
    nanosleep(&req, NULL); \
}

/** \brief Activation code for the main function of each component */
#define activate() \
{ \
    *activated = TRUE; \
    waitsec(0, 1000); \
}

/** \brief Deactivation code for the cleanup function of each component */
#define deactivate() \
{ \
    *activated = FALSE; \
    waitsec(0, 1000); \
}

/** \brief Function to print a current event */
#define print_current_app_event(type) \
    fprintf(stderr, "MEASURE_APP_LATENCY %u\n", type);

/** \brief Function to measure the start time */
#define start_to_measure_app_time() \
    struct timespec start, end; \
    clock_gettime(CLOCK_REALTIME, &start);

/** \brief Function to measure the end time */
#define stop_measuring_app_time(app_name, av_type) \
    clock_gettime(CLOCK_REALTIME, &end); \
    fprintf(stderr, "MEASURE_APP_LATENCY %u %s %lu\n", av_type, app_name, \
           ((end.tv_sec * 1000000000 + end.tv_nsec) - (start.tv_sec * 1000000000 + start.tv_nsec)));

/** \brief Function to print a current event */
#define print_current_event(type) \
    fprintf(stderr, "MEASURE_COMP_LATENCY %u\n", type);

/** \brief Function to measure the start time */
#define start_to_measure_comp_time() \
    struct timespec start, end; \
    clock_gettime(CLOCK_REALTIME, &start);

/** \brief Function to measure the end time */
#define stop_measuring_comp_time(comp_name, ev_type) \
    clock_gettime(CLOCK_REALTIME, &end); \
    fprintf(stderr, "MEASURE_COMP_LATENCY %u %s %lu\n", ev_type, comp_name, \
           ((end.tv_sec * 1000000000 + end.tv_nsec) - (start.tv_sec * 1000000000 + start.tv_nsec)));

/** \brief Allocate a space */
#define MALLOC(x) malloc(x)
#define CALLOC(x, y) calloc(x, y)

/** \brief Free a space if the space is valid */
#define FREE(x) \
{ \
    if (x) { \
        free(x); \
        x = NULL; \
    } \
}

/** \brief Functions to change the byte orders of 64-bit fields */
/* @{ */
#ifdef WORDS_BIGENDIAN
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) ((((uint64_t)htonl(x)) <<32) + htonl(x>> 32))
#define ntohll(x) ((((uint64_t)ntohl(x)) <<32) + ntohl(x>> 32))
#endif
/* @} */

/** \brief Functions to print messages through the CLI */
/* @{ */
#define cli_bufcls(buf) memset(buf, 0, sizeof(buf))
#define cli_buffer(buf, format, args...) sprintf(&buf[strlen(buf)], format, ##args)
#define cli_bufprt(cli, buf) cli_print(cli, "%s", buf)
/* @} */

/** \brief Functions for debug and error messages */
/* @{ */
//#define __ENABLE_DEBUG
#ifdef __ENABLE_DEBUG
#define DEBUG(format, args...) ({ printf("%s: " format, __FUNCTION__, ##args); fflush(stdout); })
#define PRINTF(format, args...) ({ printf("%s: " format, __FUNCTION__, ##args); fflush(stdout); })
#define PRINT_EV(format, args...) ({ printf("%s: " format, __FUNCTION__, ##args); fflush(stdout); })
#else /* !__ENABLE_DEBUG */
#define DEBUG(format, args...) (void)0
#define PRINTF(format, args...) ({ printf(format, ##args); fflush(stdout); })
#define PRINT_EV(format, args...) (void)0
#endif /* !__ENABLE_DEBUG */
#define PERROR(msg) fprintf(stdout, "\x1b[31m[%s:%d] %s: %s\x1b[0m\n", __FUNCTION__, __LINE__, (msg), strerror(errno))
/* @} */

/** \brief Wrappers that trigger log events */
/* @{ */
#define LOG_FATAL(id, format, args...) ev_log_fatal(id, format, ##args)
#define LOG_ERROR(id, format, args...) ev_log_error(id, format, ##args)
#define LOG_WARN(id, format, args...)  ev_log_warn(id, format, ##args)
#define LOG_INFO(id, format, args...)  ev_log_info(id, format, ##args)
#define LOG_DEBUG(id, format, args...) ev_log_debug(id, format, ##args)
/* @} */

/** \brief Wrappers that trigger app log events */
/* @{ */
#define ALOG_FATAL(id, format, args...) av_log_fatal(id, format, ##args)
#define ALOG_ERROR(id, format, args...) av_log_error(id, format, ##args)
#define ALOG_WARN(id, format, args...)  av_log_warn(id, format, ##args)
#define ALOG_INFO(id, format, args...)  av_log_info(id, format, ##args)
#define ALOG_DEBUG(id, format, args...) av_log_debug(id, format, ##args)
/* @} */


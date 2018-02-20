/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

#pragma once

/** \brief The list of app events */
enum {
    AV_NONE,
    // upstream
    AV_DP_RECEIVE_PACKET,
    AV_DP_FLOW_EXPIRED,
    AV_DP_FLOW_DELETED,
    AV_DP_PORT_ADDED,
    AV_DP_PORT_MODIFIED,
    AV_DP_PORT_DELETED,
    AV_ALL_UPSTREAM,
    // downstream
    AV_DP_SEND_PACKET,
    AV_DP_INSERT_FLOW,
    AV_DP_MODIFY_FLOW,
    AV_DP_DELETE_FLOW,
    AV_ALL_DOWNSTREAM,
    // internal (request-response)
    AV_SW_GET_INFO,
    AV_SW_GET_ALL_INFO,
    AV_HOST_GET_INFO,
    AV_HOST_GET_ALL_INFO,
    AV_LINK_GET_INFO,
    AV_LINK_GET_ALL_INFO,
    AV_FLOW_GET_INFO,
    AV_FLOW_GET_ALL_INFO,
    AV_WRT_INTSTREAM,
    // internal (notification)
    AV_SW_CONNECTED,
    AV_SW_DISCONNECTED,
    AV_HOST_ADDED,
    AV_HOST_DELETED,
    AV_LINK_ADDED,
    AV_LINK_DELETED,
    AV_FLOW_ADDED,
    AV_FLOW_DELETED,
    AV_ALL_INTSTREAM,
    // log
    AV_LOG_DEBUG,
    AV_LOG_INFO,
    AV_LOG_WARN,
    AV_LOG_ERROR,
    AV_LOG_FATAL,
    AV_NUM_EVENTS,
};


/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "type.h"

// upstream /////////////////////////////////////////////////////////

void ev_ofp_msg_in(uint32_t id, const raw_msg_t *data);
void ev_dp_receive_packet(uint32_t id, const pktin_t *data);
void ev_dp_flow_expired(uint32_t id, const flow_t *data);
void ev_dp_flow_deleted(uint32_t id, const flow_t *data);
void ev_dp_flow_stats(uint32_t id, const flow_t *data);
void ev_dp_aggregate_stats(uint32_t id, const flow_t *data);
void ev_dp_port_added(uint32_t id, const port_t *data);
void ev_dp_port_modified(uint32_t id, const port_t *data);
void ev_dp_port_deleted(uint32_t id, const port_t *data);
void ev_dp_port_stats(uint32_t id, const port_t *data);

// downstream ///////////////////////////////////////////////////////

void ev_ofp_msg_out(uint32_t id, const raw_msg_t *data);
void ev_dp_send_packet(uint32_t id, const pktout_t *data);
void ev_dp_insert_flow(uint32_t id, const flow_t *data);
void ev_dp_modify_flow(uint32_t id, const flow_t *data);
void ev_dp_delete_flow(uint32_t id, const flow_t *data);
void ev_dp_request_flow_stats(uint32_t id, const flow_t *data);
void ev_dp_request_aggregate_stats(uint32_t id, const flow_t *data);
void ev_dp_request_port_stats(uint32_t id, const port_t *data);

// internal (request-response) //////////////////////////////////////

void ev_sw_get_dpid(uint32_t id, switch_t *data);
void ev_sw_get_fd(uint32_t id, switch_t *data);
void ev_sw_get_xid(uint32_t id, switch_t *data);

// internal (notification) //////////////////////////////////////////

void ev_sw_new_conn(uint32_t id, const switch_t *data);
void ev_sw_expired_conn(uint32_t id, const switch_t *data);
void ev_sw_connected(uint32_t id, const switch_t *data);
void ev_sw_disconnected(uint32_t id, const switch_t *data);
void ev_sw_update_config(uint32_t id, const switch_t *data);
void ev_sw_update_desc(uint32_t id, const switch_t *data);
void ev_host_added(uint32_t id, const host_t *data);
void ev_host_deleted(uint32_t id, const host_t *data);
void ev_link_added(uint32_t id, const port_t *data);
void ev_link_deleted(uint32_t id, const port_t *data);
void ev_flow_added(uint32_t id, const flow_t *data);
void ev_flow_deleted(uint32_t id, const flow_t *data);
void ev_rs_update_usage(uint32_t id, const resource_t *data);
void ev_tr_update_stats(uint32_t id, const traffic_t *data);
void ev_log_update_msgs(uint32_t id, const char *data);

// log //////////////////////////////////////////////////////////////

void ev_log_debug(uint32_t id, char *format, ...);
void ev_log_info(uint32_t id, char *format, ...);
void ev_log_warn(uint32_t id, char *format, ...);
void ev_log_error(uint32_t id, char *format, ...);
void ev_log_fatal(uint32_t id, char *format, ...);

/////////////////////////////////////////////////////////////////////

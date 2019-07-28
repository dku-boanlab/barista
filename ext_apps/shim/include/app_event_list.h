/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "type.h"

// upstream /////////////////////////////////////////////////////////

void av_dp_receive_packet(uint32_t id, const pktin_t *data);
void av_dp_flow_expired(uint32_t id, const flow_t *data);
void av_dp_flow_deleted(uint32_t id, const flow_t *data);
void av_dp_port_added(uint32_t id, const port_t *data);
void av_dp_port_modified(uint32_t id, const port_t *data);
void av_dp_port_deleted(uint32_t id, const port_t *data);

// downstream ///////////////////////////////////////////////////////

void av_dp_send_packet(uint32_t id, const pktout_t *data);
void av_dp_insert_flow(uint32_t id, const flow_t *data);
void av_dp_modify_flow(uint32_t id, const flow_t *data);
void av_dp_delete_flow(uint32_t id, const flow_t *data);

// internal (request-response) //////////////////////////////////////

// internal (notification) //////////////////////////////////////////

void av_sw_connected(uint32_t id, const switch_t *data);
void av_sw_disconnected(uint32_t id, const switch_t *data);
void av_host_added(uint32_t id, const host_t *data);
void av_host_deleted(uint32_t id, const host_t *data);
void av_link_added(uint32_t id, const port_t *data);
void av_link_deleted(uint32_t id, const port_t *data);
void av_flow_added(uint32_t id, const flow_t *data);
void av_flow_modified(uint32_t id, const flow_t *data);
void av_flow_deleted(uint32_t id, const flow_t *data);

// log //////////////////////////////////////////////////////////////

void av_log_debug(uint32_t id, char *format, ...);
void av_log_info(uint32_t id, char *format, ...);
void av_log_warn(uint32_t id, char *format, ...);
void av_log_error(uint32_t id, char *format, ...);
void av_log_fatal(uint32_t id, char *format, ...);

/////////////////////////////////////////////////////////////////////

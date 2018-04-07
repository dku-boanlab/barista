/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"
#include "mac2int.h"

#include "openflow10.h"

int ofp10_engine(const raw_msg_t *msg);

int ofp10_packet_out(const pktout_t *pktout);
int ofp10_flow_mod(const flow_t *flow, int flag);
int ofp10_flow_stats(const flow_t *flow);
int ofp10_aggregate_stats(const flow_t *flow);
int ofp10_port_stats(const port_t *port);

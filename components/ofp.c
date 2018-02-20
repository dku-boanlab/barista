/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup ofp OpenFlow Engine
 * \brief (Base) OpenFlow engine
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "ofp.h"

// The structure of the OpenFlow header
#include "openflow10.h"

/** \brief OpenFlow engine ID */
#define OFP_ID 3187676738

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int ofp_main(int *activated, int argc, char **argv)
{
    LOG_INFO(OFP_ID, "Init - OpenFlow engine");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int ofp_cleanup(int *activated)
{
    LOG_INFO(OFP_ID, "Clean up - OpenFlow engine");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int ofp_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int ofp_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_IN:
        PRINT_EV("EV_OFP_MSG_IN\n");
        {
            const raw_msg_t *msg = ev->msg;
            struct ofp_header *ofph = (struct ofp_header *)msg->data;

            if (ofph->version == 0x1) { // OpenFlow 1.0
                return ofp10_engine(msg);
            } else {
                return ofp10_engine(msg);
            }
        }
        break;
    case EV_DP_SEND_PACKET:
        PRINT_EV("EV_DP_SEND_PACKET\n");
        {
            const pktout_t *pktout = ev->pktout;

            int version = pktout->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = pktout->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_packet_out(pktout);
            } else {
                return ofp10_packet_out(pktout);
            }
        }
        break;
    case EV_DP_INSERT_FLOW:
        PRINT_EV("EV_DP_INSERT_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            int version = flow->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = flow->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_flow_mod(flow, FLOW_ADD);
            } else {
                return ofp10_flow_mod(flow, FLOW_ADD);
            }
        }
        break;
    case EV_DP_MODIFY_FLOW:
        PRINT_EV("EV_DP_MODIFY_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            int version = flow->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = flow->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_flow_mod(flow, FLOW_MODIFY);
            } else {
                return ofp10_flow_mod(flow, FLOW_MODIFY);
            }
        }
        break;
    case EV_DP_DELETE_FLOW:
        PRINT_EV("EV_DP_DELETE_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            int version = flow->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = flow->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_flow_mod(flow, FLOW_DELETE);
            } else {
                return ofp10_flow_mod(flow, FLOW_DELETE);
            }
        }
        break;
    case EV_DP_REQUEST_FLOW_STATS:
        PRINT_EV("EV_DP_REQUEST_FLOW_STATS\n");
        {
            const flow_t *flow = ev->flow;

            int version = flow->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = flow->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_flow_stats(flow);
            } else {
                return ofp10_flow_stats(flow);
            }
        }
        break;
    case EV_DP_REQUEST_AGGREGATE_STATS:
        PRINT_EV("EV_DP_REQUEST_AGGREGATE_STATS\n");
        {
            const flow_t *flow = ev->flow;

            int version = flow->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = flow->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_aggregate_stats(flow);
            } else {
                return ofp10_aggregate_stats(flow);
            }
        }
        break;
    case EV_DP_REQUEST_PORT_STATS:
        PRINT_EV("EV_DP_REQUEST_PORT_STATS\n");
        {
            const port_t *port = ev->port;

            int version = port->version;
            if (version != 0x1) {
                switch_t sw = {0};
                sw.dpid = port->dpid;
                ev_sw_get_version(OFP_ID, &sw);
                version = sw.version;
            }

            if (version == 0x1) { // OpenFlow 1.0
                return ofp10_port_stats(port);
            } else {
                return ofp10_port_stats(port);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */

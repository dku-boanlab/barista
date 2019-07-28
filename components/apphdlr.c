/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \defgroup compnt Components
 * @{
 * \defgroup apphdlr Application Handler
 * \brief (Base) application handler
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "apphdlr.h"

/** \brief Application handler ID */
#define APPHDLR_ID 2438843119

/////////////////////////////////////////////////////////////////////

// Application side (Application layer)

#include "appint.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int apphdlr_main(int *activated, int argc, char **argv)
{
    LOG_INFO(APPHDLR_ID, "Init - Application handler");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int apphdlr_cleanup(int *activated)
{
    LOG_INFO(APPHDLR_ID, "Clean up - Application handler");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int apphdlr_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int apphdlr_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {

    // upstream events

    case EV_DP_RECEIVE_PACKET:
        PRINT_EV("EV_DP_RECEIVE_PACKET\n");
        {
            av_dp_receive_packet(APPINT_ID, ev->pktin);
        }
        break;
    case EV_DP_FLOW_EXPIRED:
        PRINT_EV("EV_DP_FLOW_EXPIRED\n");
        {
            av_dp_flow_expired(APPINT_ID, ev->flow);
        }
        break;
    case EV_DP_FLOW_DELETED:
        PRINT_EV("EV_DP_FLOW_DELETED\n");
        {
            av_dp_flow_deleted(APPINT_ID, ev->flow);
        }
        break;
    case EV_DP_PORT_ADDED:
        PRINT_EV("EV_DP_PORT_ADDED\n");
        {
            av_dp_port_added(APPINT_ID, ev->port);
        }
        break;
    case EV_DP_PORT_MODIFIED:
        PRINT_EV("EV_DP_PORT_MODIFIED\n");
        {
            av_dp_port_modified(APPINT_ID, ev->port);
        }
        break;
    case EV_DP_PORT_DELETED:
        PRINT_EV("EV_DP_PORT_DELETED\n");
        {
            av_dp_port_deleted(APPINT_ID, ev->port);
        }
        break;

    // internal events (notification)

    case EV_SW_CONNECTED:
        PRINT_EV("EV_SW_CONNECTED\n");
        {
            av_sw_connected(APPINT_ID, ev->sw);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            av_sw_disconnected(APPINT_ID, ev->sw);
        }
        break;
    case EV_HOST_ADDED:
        PRINT_EV("EV_HOST_ADDED\n");
        {
            av_host_added(APPINT_ID, ev->host);
        }
        break;
    case EV_HOST_DELETED:
        PRINT_EV("EV_HOST_DELETED\n");
        {
            av_host_deleted(APPINT_ID, ev->host);
        }
        break;
    case EV_LINK_ADDED:
        PRINT_EV("EV_LINK_ADDED\n");
        {
            av_link_added(APPINT_ID, ev->port);
        }
        break;
    case EV_LINK_DELETED:
        PRINT_EV("EV_LINK_DELETED\n");
        {
            av_link_deleted(APPINT_ID, ev->port);
        }
        break;
    case EV_FLOW_ADDED:
        PRINT_EV("EV_FLOW_ADDED\n");
        {
            av_flow_added(APPINT_ID, ev->flow);
        }
        break;
    case EV_FLOW_MODIFIED:
        PRINT_EV("EV_FLOW_MODIFIED\n");
        {
            av_flow_modified(APPINT_ID, ev->flow);
        }
        break;
    case EV_FLOW_DELETED:
        PRINT_EV("EV_FLOW_DELETED\n");
        {
            av_flow_deleted(APPINT_ID, ev->flow);
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

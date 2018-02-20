/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \defgroup app Applications
 * @{
 * \defgroup appint Application Interface
 * \brief (Base) application interface
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

// inside of 'apphdlr.c'

// Application side (Application layer)

#include "app_event.h"

/** \brief Application interface ID */
#define APPINT_ID 3685861783

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int appint_main(int *activated, int argc, char **argv)
{
    LOG_INFO(APPHDLR_ID, "<%010u> Init - Application interface", APPINT_ID);

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int appint_cleanup(int *activated)
{
    LOG_INFO(APPHDLR_ID, "<%010u> Clean up - Application interface", APPINT_ID);

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int appint_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int appint_handler(const app_event_t *av, app_event_out_t *av_out)
{
    switch (av->type) {

    // downstream events

    case AV_DP_SEND_PACKET:
        PRINT_EV("AV_DP_SEND_PACKET\n");
        {
            ev_dp_send_packet(APPHDLR_ID, av->pktout);
        }
        break;
    case AV_DP_INSERT_FLOW:
        PRINT_EV("AV_DP_INSERT_FLOW\n");
        {
            ev_dp_insert_flow(APPHDLR_ID, av->flow);
        }
        break;
    case AV_DP_MODIFY_FLOW:
        PRINT_EV("AV_DP_MODIFY_FLOW\n");
        {
            ev_dp_modify_flow(APPHDLR_ID, av->flow);
        }
        break;
    case AV_DP_DELETE_FLOW:
        PRINT_EV("AV_DP_DELETE_FLOW\n");
        {
            ev_dp_delete_flow(APPHDLR_ID, av->flow);
        }
        break;

    // request-response events

    case AV_SW_GET_INFO:
        PRINT_EV("AV_SW_GET_INFO\n");
        {
            ev_sw_get_info(APPHDLR_ID, av_out->sw_data);
        }
        break;
    case AV_SW_GET_ALL_INFO:
        PRINT_EV("AV_SW_GET_ALL_INFO\n");
        {
            ev_sw_get_all_info(APPHDLR_ID, av_out->sw_data);
        }
        break;
    case AV_HOST_GET_INFO:
        PRINT_EV("AV_HOST_GET_INFO\n");
        {
            ev_host_get_info(APPHDLR_ID, av_out->host_data);
        }
        break;
    case AV_HOST_GET_ALL_INFO:
        PRINT_EV("AV_HOST_GET_ALL_INFO\n");
        {
            ev_host_get_all_info(APPHDLR_ID, av_out->host_data);
        }
        break;
    case AV_LINK_GET_INFO:
        PRINT_EV("AV_LINK_GET_INFO\n");
        {
            ev_link_get_info(APPHDLR_ID, av_out->port_data);
        }
        break;
    case AV_LINK_GET_ALL_INFO:
        PRINT_EV("AV_LINK_GET_ALL_INFO\n");
        {
            ev_link_get_all_info(APPHDLR_ID, av_out->port_data);
        }
        break;
    case AV_FLOW_GET_INFO:
        PRINT_EV("AV_FLOW_GET_INFO\n");
        {
            ev_flow_get_info(APPHDLR_ID, av_out->flow_data);
        }
        break;
    case AV_FLOW_GET_ALL_INFO:
        PRINT_EV("AV_FLOW_GET_ALL_INFO\n");
        {
            ev_flow_get_all_info(APPHDLR_ID, av_out->flow_data);
        }
        break;

    // log events

    case AV_LOG_DEBUG:
        PRINT_EV("AV_LOG_DEBUG\n");
        {
            LOG_DEBUG(APPHDLR_ID, "<%010u> %s", av->id, av->log);
        }
        break;
    case AV_LOG_INFO:
        PRINT_EV("AV_LOG_INFO\n");
        {
            LOG_INFO(APPHDLR_ID, "<%010u> %s", av->id, av->log);
        }
        break;
    case AV_LOG_WARN:
        PRINT_EV("AV_LOG_WARN\n");
        {
            LOG_WARN(APPHDLR_ID, "<%010u> %s", av->id, av->log);
        }
        break;
    case AV_LOG_ERROR:
        PRINT_EV("AV_LOG_ERROR\n");
        {
            LOG_ERROR(APPHDLR_ID, "<%010u> %s", av->id, av->log);
        }
        break;
    case AV_LOG_FATAL:
        PRINT_EV("AV_LOG_FATAL\n");
        {
            LOG_FATAL(APPHDLR_ID, "<%010u> %s", av->id, av->log);
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

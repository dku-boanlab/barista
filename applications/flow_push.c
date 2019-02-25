/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup push Flow Pusher
 * \brief (Admin) flow pusher
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "flow_push.h"

/** \brief Flow pusher ID */
#define PUSH_ID 2991799864

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int flow_push_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(PUSH_ID, "Init - Flow Pusher");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int flow_push_cleanup(int *activated)
{
    ALOG_INFO(PUSH_ID, "Clean up - Flow Pusher");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int flow_push_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int flow_push_handler(const app_event_t *av, app_event_out_t *av_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

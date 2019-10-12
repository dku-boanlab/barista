/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup event_monitor Core Event Monitor
 * \brief (Admin) core event monitor
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "event_monitor.h"

/** \brief Core Event Monitor ID */
#define EV_MONITOR_ID 2889962257

/////////////////////////////////////////////////////////////////////

/* \brief The flag to enable API monitoring */
int API_monitor_enabled;

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int event_monitor_main(int *activated, int argc, char **argv)
{
    LOG_INFO(EV_MONITOR_ID, "Init - Core event monitor");

    const char *API_monitor = getenv("API_monitor");
    if (API_monitor != NULL && strcmp(API_monitor, "API_monitor") == 0)
        API_monitor_enabled = TRUE;

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int event_monitor_cleanup(int *activated)
{
    LOG_INFO(EV_MONITOR_ID, "Clean up - Core event monitor");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int event_monitor_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int event_monitor_handler(const event_t *ev, event_out_t *ev_out)
{
    if (API_monitor_enabled) {

    }

    return 0;
}

/**
 * @}
 *
 * @}
 */

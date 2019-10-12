/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup app_event_monitor Application Event Monitor
 * \brief (Admin) application event monitor
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "app_event_monitor.h"

/** \brief Application event monitor ID */
#define AV_MONITOR_ID 2506540042

/////////////////////////////////////////////////////////////////////

/* \brief The flag to enable API monitoring */
int API_monitor_enabled;

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int app_event_monitor_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(AV_MONITOR_ID, "Init - Application event monitor");

    const char *API_monitor = getenv("API_monitor");
    if (API_monitor != NULL && strcmp(API_monitor, "API_monitor") == 0)
        API_monitor_enabled = TRUE;

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int app_event_monitor_cleanup(int *activated)
{
    ALOG_INFO(AV_MONITOR_ID, "Clean up - Application event monitor");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int app_event_monitor_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int app_event_monitor_handler(const app_event_t *av, app_event_out_t *av_out)
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

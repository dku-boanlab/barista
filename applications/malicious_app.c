/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup malicious_app Sample Malicious Application
 * \brief (Network) sample malicious application
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "malicious_app.h"

/** \brief Malicious application ID (1450876425) */
#define MALICIOUS_ID 1234567890

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int malicious_app_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(MALICIOUS_ID, "Init - Sample malicious application");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int malicious_app_cleanup(int *activated)
{
    ALOG_INFO(MALICIOUS_ID, "Clean up - Sample malicious application");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int malicious_app_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int malicious_app_handler(const app_event_t *av, app_event_out_t *av_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

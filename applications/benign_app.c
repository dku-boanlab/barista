/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup benign_app Sample Benign Application
 * \brief (Network) sample benign application
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "benign_app.h"

/** \brief Benign application ID */
#define BENIGN_ID 2178001229

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int benign_app_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(BENIGN_ID, "Init - Sample benign application");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int benign_app_cleanup(int *activated)
{
    ALOG_INFO(BENIGN_ID, "Clean up - Sample bengin application");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int benign_app_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int benign_app_handler(const app_event_t *av, app_event_out_t *av_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

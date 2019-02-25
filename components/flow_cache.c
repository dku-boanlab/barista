/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup flow_cache Flow Cache
 * \brief (Network) flow cache
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "flow_cache.h"

/** \brief Flow cache ID */
#define CACHE_ID 1697391255

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int flow_cache_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CACHE_ID, "Init - Flow cache");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int flow_cache_cleanup(int *activated)
{
    LOG_INFO(CACHE_ID, "Clean up - Flwo cache");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int flow_cache_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int flow_cache_handler(const event_t *ev, event_out_t *ev_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

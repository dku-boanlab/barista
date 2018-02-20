/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup conn_tls Packet I/O Engine with TLS
 * \brief (Base) packet I/O engine with TLS
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "conn_tls.h"

/** \brief TLS-based Packet I/O engine ID */
#define CONN_TLS_ID 1075988948

//TODO complete conn_tls

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int conn_tls_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CONN_TLS_ID, "Init - Packet I/O engine with TLS");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int conn_tls_cleanup(int *activated)
{
    LOG_INFO(CONN_TLS_ID, "Clean up - Packet I/O engine with TLS");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int conn_tls_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int conn_tls_handler(const event_t *ev, event_out_t *ev_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup dit Control Message Integrity
 * \brief (Security) control message integrity
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "dit.h"

/** \brief Control message integrity ID */
#define DIT_ID 3807334438

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int dit_main(int *activated, int argc, char **argv)
{
    LOG_INFO(DIT_ID, "Init - Internal message integrity");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int dit_cleanup(int *activated)
{
    LOG_INFO(DIT_ID, "Clean up - Internal message integrity");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int dit_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int dit_handler(const event_t *ev, event_out_t *ev_out)
{
    uint32_t old_checksum = ev_out->checksum;
    uint32_t checksum = hash_func((uint32_t *)ev->msg, ev->length / sizeof(uint32_t));

    if (old_checksum == 0) {
        // generate checksum
        ev_out->checksum = checksum;
    } else {
        // check checksum
        if (old_checksum != checksum) {
            LOG_WARN(DIT_ID, "Malformed internal message (old: %x, curr: %x)", old_checksum, checksum);
            return -1;
        }
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */

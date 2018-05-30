/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_shim
 * @{
 * \defgroup app_alt Application CLI Alternative
 * \brief Functions to convert CLI messages to logging messages
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "cli.h"

int cli_print(cli_t *cli, char *buf)
{
    printf("%s\n", buf);

    return 0;
}

/**
 * @}
 *
 * @}
 */

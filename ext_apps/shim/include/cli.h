/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_shim
 * @{
 * \ingroup app_alt Application CLI Alternative
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include <stdio.h>

/** \brief The context structure of the CLI */
typedef struct _cli_t {
} cli_t;

int cli_print(cli_t *cli, char *buf);

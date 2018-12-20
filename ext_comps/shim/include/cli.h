/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt_shim
 * @{
 * \ingroup compnt_alt Component CLI Alternative
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include <stdio.h>

/** \brief The structure of the CLI context */
typedef struct _cli_t {
} cli_t;

int cli_print(cli_t *cli, char *buf);

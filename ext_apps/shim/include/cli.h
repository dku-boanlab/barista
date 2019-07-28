/*
 * Copyright 2015-2019 NSSLab, KAIST
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


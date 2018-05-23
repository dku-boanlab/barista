/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "context.h"

#define USERID "admin"
#define USERPW "password"
#define ADMINPW "admin_password"

int start_cli(ctx_t *ctx);

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

char *ip_addr_str(const uint32_t ip);
uint32_t ip_addr_int(const char *ipaddr);

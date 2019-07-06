/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

uint64_t mac2int(const uint8_t *mac);
uint8_t *int2mac(const uint64_t mac, uint8_t *macaddr);
uint8_t *str2mac(const char *macaddr, uint8_t *mac);
char *mac2str(const uint8_t *mac, char *macaddr);

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

char *base64_encode(const char *bin, int len);
char *base64_encode_w_buffer(const char *bin, int len, char *buffer);
char *base64_decode(const char *encoded);
char *base64_decode_w_buffer(const char *encoded, char *buffer);

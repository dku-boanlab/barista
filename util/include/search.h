/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Hyeonseong Jo <hsjjo@kaist.ac.kr>
 */

#pragma once

#include "common.h"

int get_external_configurations(char (*temp)[__CONF_WORD_LEN], int *cnt, char *curr);

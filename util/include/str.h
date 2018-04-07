/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"

int str2args(char *str, int *argc, char **argv, int max_argc);
char *str_read(char *fname);
char *str_preproc(char *raw);

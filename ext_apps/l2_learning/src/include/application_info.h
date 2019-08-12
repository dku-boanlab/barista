/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#define TARGET_APP "l2_learning"

#ifdef __ENABLE_DOCKER
#define TARGET_APP_PULL_ADDR "tcp://0.0.0.0:6011"
#define TARGET_APP_REPLY_ADDR "tcp://0.0.0.0:6012"
#else
#define TARGET_APP_PULL_ADDR "tcp://127.0.0.1:6011"
#define TARGET_APP_REPLY_ADDR "tcp://127.0.0.1:6012"
#endif

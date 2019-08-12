/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#define TARGET_COMPNT "topo_mgmt"

#ifdef __ENABLE_DOCKER
#define TARGET_COMP_PULL_ADDR "tcp://0.0.0.0:5011"
#define TARGET_COMP_REPLY_ADDR "tcp://0.0.0.0:5012"
#else
#define TARGET_COMP_PULL_ADDR "tcp://127.0.0.1:5011"
#define TARGET_COMP_REPLY_ADDR "tcp://127.0.0.1:5012"
#endif

/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#define TARGET_APP "l2_shortest"

//#define TARGET_APP_PULL_ADDR "tcp://127.0.0.1:6021"
#define TARGET_APP_PULL_ADDR "ipc://tmp/l2_shortest_pull"
//#define TARGET_APP_REPLY_ADDR "tcp://127.0.0.1:6022"
#define TARGET_APP_REPLY_ADDR "ipc://tmp/l2_shortest_reply"


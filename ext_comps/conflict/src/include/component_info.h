/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#define TARGET_COMPNT "conflict"

//#define TARGET_COMP_PULL_ADDR "tcp://127.0.0.1:5011"
#define TARGET_COMP_PULL_ADDR "ipc://tmp/conflict_pull"
//#define TARGET_COMP_REPLY_ADDR "tcp://127.0.0.1:5012"
#define TARGET_COMP_REPLY_ADDR "ipc://tmp/conflict_reply"


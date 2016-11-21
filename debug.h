/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#ifndef DBG_H
#define DBG_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define describ_errno() (errno == 0 ? "None" : strerror(errno))

#define log_info(M, ...) fprintf(stdout, "[INFO] (%s:%d: errno: %s) " #M "\n", __FILE__, __LINE__, describ_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " #M "\n", __FILE__, __LINE__, describ_errno(), ##__VA_ARGS__)

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " #M "\n", __FILE__, __LINE__, describ_errno(), ##__VA_ARGS__)

#endif

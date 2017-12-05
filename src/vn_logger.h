/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_LOGGER_H
#define VINO_VN_LOGGER_H

#define VN_ERROR   "ERROR"
#define VN_WARN    "WARN"
#define VN_INFO    "INFO"

#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt".\n", \
    __FILE__, __LINE__, __func__, ##args)

void vn_log_error(const char *fmt, ...);

void vn_log_warn(const char *fmt, ...);

void vn_log_info(const char *fmt, ...);

#endif /* VINO_VN_LOGGER_H */

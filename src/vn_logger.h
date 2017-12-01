/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 *
 * version 2017/12/01
 */
#ifndef VINO_VN_LOGGER_H
#define VINO_VN_LOGGER_H

#define VN_DEBUG   "DEBUG"
#define VN_INFO    "INFO"
#define VN_WARN    "WARN"
#define VN_ERROR   "ERROR"
#define VN_FATAL   "FATAL"

/* 
 * debug mode
 */
void vn_log_debug(const char *fmt, ...);

/* 
 * information mode
 */
void vn_log_info(const char *fmt, ...);

/* 
 * warn mode
 */
void vn_log_warn(const char *fmt, ...);

/* 
 * error mode
 */
void vn_log_error(const char *fmt, ...);

/* 
 * fatal mode
 */
void vn_log_fatal(const char *fmt, ...);

#endif /* VINO_VN_LOGGER_H */

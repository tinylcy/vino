/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "vn_logger.h"
#include "vn_error.h"

static void vn_log_format(char *tag, const char *fmt, va_list ap) {
    time_t now;
    char *date;

    time(&now);
    if ((date = ctime(&now)) == NULL) {
        err_sys("[vn_log_format] ctime error");
    }
    date[strlen(date) - 1] = '\0';
    printf("%s [%s] ", date, tag);
    vprintf(fmt, ap);
    printf("\n"); 
}

void vn_log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vn_log_format(VN_ERROR, fmt, ap);
    va_end(ap);
}

void vn_log_warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vn_log_format(VN_WARN, fmt, ap);
    va_end(ap);
}

void vn_log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vn_log_format(VN_INFO, fmt, ap);
    va_end(ap);    
}
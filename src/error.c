/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 *
 * version 2017/12/01
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "error.h"

void err_doit(int use_errno, const char *fmt, va_list ap) {
    int errno_orig;
    char buf[MAXLINE + 1];

    errno_orig = errno;
    vsnprintf(buf, MAXLINE, fmt, ap);
    if (use_errno) {
        strncat(buf, ": ", MAXLINE + 1);
        strncat(buf, strerror(errno_orig), MAXLINE + 1);
    }
    strncat(buf, "\n", MAXLINE + 1);
    fputs(buf, stderr);

    return;
}

void err_ret(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}

void err_quit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}

void err_sys(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    exit(1);
}

void err_dump(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    abort();
    exit(1);
}

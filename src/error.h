/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <stdarg.h>

#define MAXLINE 1000

void err_doit(int useerrno, const char *fmt, va_list ap);

void err_ret(const char *fmt, ...);

void err_quit(const char *fmt, ...);

void err_sys(const char *fmt, ...);

void err_dump(const char *fmt, ...);
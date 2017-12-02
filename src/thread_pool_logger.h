#ifndef __THREAD_POOL_LOGGER_H__
#define __THREAD_POOL_LOGGER_H__

#include <stdio.h>
#include <pthread.h>

/* 
 * log level
 */
enum log_type {
    thread_pool_FATAL,
    thread_pool_ERROR,
    thread_pool_WARN,
    thread_pool_INFO,
    thread_pool_DEBUG
};

static const char* log_level[] = {
    "THREAD_POOL_FATAL",
    "THREAD_POOL_ERROR",
    "THREAD_POOL_WARN",
    "THREAD_POOL_INFO",
    "THREAD_POOL_DEBUG"
};

/*
 * flockfile(stdout);   等待stdout不再被其他线程锁定后，获取其文件句柄并增加锁定计数
 * io operation for stdout
 * funlockfile(stdout); 减少锁定计数
 */
#define debug(level, ...)   do { \
    if (level < thread_pool_DEBUG) {\
        flockfile(stdout); \
        printf("[ %s ] %p.%s: ", log_level[level], (void *)pthread_self(), __func__); \
        printf(__VA_ARGS__); \
        putchar('\n'); \
        fflush(stdout); \
        funlockfile(stdout);\
    }\
} while (0)

#endif  /* __THREAD_POOL_LOGGER_H__ */

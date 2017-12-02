#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "thread_pool_logger.h"
#include "vn_request.h"

#define WORK_QUEUE_POWER   16
#define WORK_QUEUE_SIZE    (1 << WORK_QUEUE_POWER)  // 65536
#define WORK_QUEUE_MASK    (WORK_QUEUE_SIZE - 1)
#define MAX_THREAD_NUM     512

/*
 * Just main thread can increase thread->in, we can make it safely.
 * However, thread->out may be increased in both main thread and
 * worker thread during load balance when new threads are added to 
 * our thread pool.
 */

/* type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
 * 如果 *ptr == oldval ，就将　newval　写入　*ptr
 *  在相等并写入的情况下返回操作之前的值
 */
#define thread_out_val(thread)           (__sync_val_compare_and_swap(&(thread)->out, 0, 0))
/* bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
 * 如果 *ptr == oldval ，就将　newval　写入　*ptr
 * 在相等并写入的情况下返回 true
 */
#define thread_out_bool(out_ptr, i, j)   (__sync_bool_compare_and_swap(out_ptr, i, j))

#define thread_queue_len(thread)     ((thread)->in - thread_out_val(thread))
#define thread_queue_full(thread)    (thread_queue_len(thread) == WORK_QUEUE_SIZE)
#define thread_queue_empty(thread)   (thread_queue_len(thread) == 0)
#define thread_queue_offset(val)     ((val) & WORK_QUEUE_MASK)  // 等价于 val %= WORK_QUEUE_SIZE

static pthread_t    main_tid;               // 主线程
static volatile int global_num_thread = 0;  // 全局线程数

/*
 * work
 */
typedef struct work {
    void          (*routine)(vn_http_event *);  // 任务执行函数
    vn_http_event *arg;                         // 函数参数
    struct work   *next;                        // 指向下一个任务
} work_t;

/*
 * work thread
 */
typedef struct thread {
    pthread_t id;        // 线程标识
    int       shutdown;  // 线程存活标识，１ - 死线程 0 - 活线程
#ifdef DEBUG
    int       num_works_done;
#endif
    size_t    in;   /* offset from start of work_queue where to put work next */
    size_t    out;  /* offset from start of work_queue where to get work next */
    work_t    work_queue[WORK_QUEUE_SIZE];  // 私有工作队列
} thread_t;

typedef struct thread_pool thread_pool_t;

/* thread schedule function */
typedef thread_t* (*schedule_thread_func)(thread_pool_t *thread_pool);

/*
 * lock-free thread pool
 */
struct thread_pool {
    int                  num_threads;              // 总的线程数
    thread_t             threads[MAX_THREAD_NUM];  // 线程数组
    schedule_thread_func schedule_thread;          // 线程调度算法
};

/*
 * initialize lock-free thread pool
 */
void* thread_pool_init(int num_worker_threads);

/*
 * add threads to lock-free thread pool
 */
int thread_pool_inc_threads(void *pool, int num_inc);

/*
 * remove threads from lock-free thread pool
 */
void thread_pool_dec_threads(void *pool, int num_dec);

/*
 * add work to work queue
 */
int thread_pool_add_work(void *pool, void(*routine)(vn_http_event *), vn_http_event *arg);

/*
 * destroy lock-free thread pool
 */
void thread_pool_destroy(void *pool, int finish);

/* 
 * thread schedule algorithm
 */
enum schedule_type {
    ROUND_ROBIN,  // 轮询分配
    LEAST_LOAD    // 最少者分配
};

/* 
 * set thread schedule algorithm, default is round-robin 
 */
void set_thread_schedule_algorithm(void *pool, enum schedule_type type);

#endif  /* __THREAD_POOL_H__ */

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#endif

#include <pthread.h>

typedef void* (*callback_func)(void *);

/*
 * each job corresponding to each HTTP request
 */
typedef struct job {
	callback_func p_callback_func;
	void *arg;
	struct job *next;
} job_t;

typedef struct threadpool {
	int thread_num; // thread_num is the number of threads in threadpool
	int job_max_num; // the max number of jobs in threadpool
	int job_cur_num; // the current number of thread in threadpool
	job_t *head;
	job_t *tail;
	pthread_t *pthreads;
	pthread_mutex_t mutex;
	pthread_cond_t queue_empty;
	pthread_cond_t queue_not_empty;
	pthread_cond_t queue_not_full;
} threadpool_t;

void *threadpool_function(void *arg);

/*
 * initialize the threadpool
 */
threadpool_t *threadpool_init(int thread_num, int job_max_num);

/*
 * add one job into threadpool
 */
int threadpool_add_job(threadpool_t *pool, callback_func p_callback_func, void *arg);

/*
 * destroy the threadpool
 */
int threadpool_destroy(threadpool_t *pool);

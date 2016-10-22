#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
#endif

#include <pthread.h>

typedef void *(*callback_func)(void*);

typedef struct job {
	callback_func p_callback_func;    // 线程回调函数
	void *arg;    // 回调函数参数
	struct job *next;
}job_t;

typedef struct threadpool {
	int thread_num;    // 线程池中开启线程的个数
	int job_max_num;    // 队列中最大job的个数
	job_t *head;    // 指向job的头指针
	job_t *tail;    // 指向job的尾指针
	pthread_t *pthreads;    // 线程池中所有线程的pthread_t
	pthread_mutex_t mutex;    // 互斥信号量
	pthread_cond_t queue_empty;    // 队列为空的条件变量
	pthread_cond_t queue_not_empty;    // 队列不为空的条件变量
	pthread_cond_t queue_not_full;    // 队列不为满的条件变量
	int job_cur_num;    // 队列当前的job个数
	int queue_close;    // 队列是否已经关闭
	int pool_close;    // 线程池是否已经关闭
} threadpool_t;

/*
 * 初始化线程池
 * @thread_num - 线程池开启的线程个数
 * @job_max_num - 队列的最大job个数
 * @return - 成功返回线程池地址，失败返回NULL
 */
threadpool_t *threadpool_init(int thread_num, int job_max_num);

/*
 * 向线程池中添加任务
 * @pool - 线程池地址
 * @p_callback_func - 回调函数
 * @arg - 回调函数参数
 * @return - 成功返回0，失败返回－1
 *
 */
int threadpool_add_job(threadpool_t *pool, callback_func p_callback_func, void *arg);

/*
 * 销毁线程池
 * @pool - 线程池地址
 * @return - 永远返回0
 *
 */
int threadpool_destroy(threadpool_t *pool);

/*
 * 线程池中线程函数
 * @pool - 线程池地址
 *
 */
void *threadpool_function(void *arg);












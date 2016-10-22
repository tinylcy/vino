#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

threadpool_t* threadpool_init(int thread_num, int job_max_num) {

	threadpool_t *pool = NULL;
	pool = (threadpool_t *)malloc(sizeof(threadpool_t));

	do {

		if(NULL == pool) {
			perror("failed to malloc threadpool");
			exit(1);
		}

		pool->thread_num = thread_num;
		pool->job_max_num = job_max_num;
		pool->job_cur_num = 0;
		pool->head = NULL;
		pool->tail = NULL;

		if(pthread_mutex_init(&(pool->mutex), NULL)) {
			perror("pthread_mutex_init");
			exit(1);
		}

		if(pthread_cond_init(&(pool->queue_empty), NULL)) {
			perror("pthread_cond_init");
			exit(1);
		}

		if(pthread_cond_init(&(pool->queue_not_empty), NULL)) {
			perror("pthread_cond_init");
			exit(1);
		}

		if(pthread_cond_init(&(pool->queue_not_full), NULL)) {
			perror("pthread_cond_init");
			exit(1);
		}

		pool->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);

		pool->queue_close = 0;
		pool->pool_close = 0;

		int i;
		for(i = 0; i < pool->thread_num; i++) {
			if(pthread_create(&(pool->pthreads[i]), NULL, threadpool_function, (void*)pool) < 0) {
				perror("pthread_create");
				exit(1);
			}
		}

		return pool;

	} while(0);

	return NULL;
}

int threadpool_add_job(threadpool_t *pool, callback_func p_callback_func, void *arg) {

	if(pool == NULL || p_callback_func == NULL) {
		return -1;
	}

	pthread_mutex_lock(&(pool->mutex));

	while((pool->job_cur_num == pool->job_max_num) && !(pool->pool_close || pool->queue_close)) {
		// 等待threadpool_function发送queue_not_full信号
		pthread_cond_wait(&(pool->queue_not_full), &(pool->mutex));    // 队列满的时候就等待
	}

	// 队列关闭或者线程关闭就退出
	if(pool->queue_close || pool->pool_close) {
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}

	job_t *pjob = (job_t*)malloc(sizeof(job_t));
	if(NULL == pjob) {
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}

	pjob->p_callback_func = p_callback_func;
	pjob->arg = arg;
	pjob->next = NULL;

	if(pool->head == NULL) {
		pool->head = pool->tail = pjob;
		// 队列空的时候，有任务来了，就通知线程池中的线程：队列非空。
		pthread_cond_broadcast(&(pool->queue_not_empty));
	} else {
		pool->tail->next = pjob;
		pool->tail = pjob;    // 把任务插入到队列的尾部
	}

	pool->job_cur_num++;
	pthread_mutex_unlock(&(pool->mutex));

	return 0;

}

void *threadpool_function(void *arg) {
	threadpool_t *pool = (threadpool_t*)arg;
	job_t *pjob = NULL;

	while(1) {
		pthread_mutex_lock(&(pool->mutex));
		while((pool->job_cur_num == 0) && !pool->pool_close) {    // 队列为空，就等待队列非空
			// 等待threadpool_add_job函数发送queue_not_empty信号
			pthread_cond_wait(&(pool->queue_not_empty), &(pool->mutex));
		}

		if(pool->pool_close) {    // 线程池关闭，线程就退出
			pthread_mutex_unlock(&(pool->mutex));
			pthread_exit(NULL);
		}

		pool->job_cur_num--;
		pjob = pool->head;
		if(pool->job_cur_num == 0) {
			pool->head = pool->tail = NULL;
		} else {
			pool->head = pjob->next;
		}

		if(pool->job_cur_num == 0) {
			// 通知destroy函数可以销毁线程池了
			pthread_cond_signal(&(pool->queue_empty));    
		} else if(pool->job_cur_num <= pool->job_max_num - 1) {
			// 向threadpool_add_job发送queue_not_full信号
			pthread_cond_broadcast(&(pool->queue_not_full));
		}

		pthread_mutex_unlock(&(pool->mutex));
		(*(pjob->p_callback_func))(pjob->arg);    // 线程真正要做的工作，调用回调函数
		free(pjob);
		pjob = NULL;
	}
}

int threadpool_destroy(threadpool_t *pool) {
	if(pool == NULL) {
		return 0;
	}

	pthread_mutex_unlock(&(pool->mutex));

	// 线程池已经退出了，就直接返回
	if(pool->queue_close && pool->pool_close) {
		pthread_mutex_unlock(&(pool->mutex));
		return 0;
	}

	pool->queue_close = 1;    // 关闭任务队列，不再接收新的任务了

	while(pool->job_cur_num != 0) {
		pthread_cond_wait(&(pool->queue_empty), &(pool->mutex));    // 等待队列为空
	}

	pool->pool_close = 1;    // 线程池关闭
	pthread_mutex_unlock(&(pool->mutex));   

	pthread_cond_broadcast(&(pool->queue_not_empty));
	pthread_cond_broadcast(&(pool->queue_not_full));

	int i;
	for(i = 0; i < pool->thread_num; i++) {
		pthread_join(pool->pthreads[i], NULL);    // 等待线程池中的所有线程执行完毕
	}

	pthread_mutex_destroy(&(pool->mutex));    // 清理资源
	pthread_cond_destroy(&(pool->queue_empty));
	pthread_cond_destroy(&(pool->queue_not_empty));
	pthread_cond_destroy(&(pool->queue_not_full));

	free(pool->pthreads);

	job_t *pjob = NULL;
	while(pool->head != NULL) {
		pjob = pool->head;
		pool->head = pjob->next;
		free(pjob);
	}

	free(pool);

	return 0;

}



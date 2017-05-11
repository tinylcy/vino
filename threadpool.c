/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */
#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

/*-----------------------------------------------------------*
	each thread runs this function: fetching the first job of
	queue in threadpool and executing the callback function.
  -----------------------------------------------------------*/
void *threadpool_function(void *arg) {

	threadpool_t *pool = (threadpool_t *)arg;

	while (1) {

		if (pthread_mutex_lock(&(pool->mutex)) != 0) {
			perror("fail to lock the mutex");
			pthread_exit(NULL);
		}

		while (pool->job_cur_num == 0) {
			if (pthread_cond_wait(&(pool->queue_not_empty), &(pool->mutex)) != 0) {
				perror("fail to wait queue_not_empty");
				pthread_exit(NULL);
			}
		}

		job_t *pjob = pool->head;

		pool->job_cur_num--;

		if (pool->job_cur_num == 0) {
			pool->head = pool->tail = NULL;
		} else {
			pool->head = pjob->next;
		}

		(*(pjob->p_callback_func))(pjob->arg);
		free(pjob);
		pjob = NULL;

		if (pthread_mutex_unlock(&(pool->mutex)) != 0) {
			perror("fail to unlock the mutex");
			pthread_exit(NULL);
		}
	}

}

/*-----------------------------------------------------------*
	initialize threadpool.
 -----------------------------------------------------------*/
threadpool_t *threadpool_init(int thread_num, int job_max_num) {

	threadpool_t *pool = NULL;
	pool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if (pool == NULL) {
		perror("fail to malloc threadpool");
		return NULL;
	}

	pool->thread_num = thread_num;
	pool->job_max_num = job_max_num;
	pool->job_cur_num = 0;
	pool->head = NULL;
	pool->tail = NULL;

	if (pthread_mutex_init(&(pool->mutex), NULL) != 0) {
		perror("fail to initialize the mutex");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_init(&(pool->queue_not_empty), NULL) != 0) {
		perror("fail to initialize queue_not_empty");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
		perror("fail to initialize queue_not_full");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_init(&(pool->queue_empty), NULL) != 0) {
		perror("fail to initialize queue_empty");
		exit(EXIT_FAILURE);
	}

	pool->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * pool->thread_num);
	if (pool->pthreads == NULL) {
		perror("fail to malloc pthreads");
		exit(EXIT_FAILURE);
	}

	int i;
	for (i = 0; i < pool->thread_num; i++) {
		if (pthread_create(&(pool->pthreads[i]), NULL, threadpool_function, (void*)pool) != 0) {
			perror("fail to create the pthread");
			exit(EXIT_FAILURE);
		}
	}

	return pool;

}

/*-----------------------------------------------------------*
add one job into threadpool
 -----------------------------------------------------------*/
int threadpool_add_job(threadpool_t *pool, callback_func p_callback_func, void *arg) {
	if (pool == NULL || p_callback_func == NULL) {
		return -1;
	}

	if (pthread_mutex_lock(&(pool->mutex)) != 0) {
		perror("fail to lock the mutex");
		return -1;
	}

	while (pool->job_cur_num == pool->job_max_num - 1) {
		pthread_cond_wait(&(pool->queue_not_full), &(pool->mutex));
	}

	job_t *pjob = (job_t *)malloc(sizeof(job_t));
	if (pjob == NULL) {
		perror("fail to malloc job");
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}

	pjob->p_callback_func = p_callback_func;
	pjob->arg = arg;
	pjob->next = NULL;

	if (pool->job_cur_num == 0) {
		pool->head = pool->tail = pjob;
	} else {
		pool->tail->next = pjob;
		pool->tail = pjob;
	}

	pool->job_cur_num++;

	if (pthread_mutex_unlock(&(pool->mutex)) != 0) {
		perror("fail to unlock the mutex");
		return -1;
	}

	if (pthread_cond_signal(&(pool->queue_not_empty)) != 0) {
		perror("fail to signal signal queue_not_empty");
		return -1;
	}

	return 0;
}

/*-----------------------------------------------------------*
    destroy threadpool.
 -----------------------------------------------------------*/
int threadpool_destroy(threadpool_t *pool) {

	// TODO
	return 0;
}

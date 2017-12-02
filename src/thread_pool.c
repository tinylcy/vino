/***************************************************************************
** Name         : lock_free_thread_pool.c
** Author       : xhjcehust
** Version      : v1.0
** Date         : 2015-05
** Description  : lock free thread pool.
**
** CSDN Blog    : http://blog.csdn.net/xhjcehust
** E-mail       : hjxiaohust@gmail.com
**
** This file may be redistributed under the terms of the GNU Public License.
***************************************************************************/

#include "thread_pool.h"

/*
 * round robin thread schedule algorithm
 */
static thread_t* round_robin_schedule(thread_pool_t *thread_pool) {
    static int current_thread_index = -1;

    assert(thread_pool && thread_pool->num_threads > 0);

    current_thread_index = (current_thread_index + 1) % thread_pool->num_threads;

    return &thread_pool->threads[current_thread_index];
}

/*
 * least load thread schedule algorithm
 */
static thread_t* least_load_schedule(thread_pool_t *thread_pool) {
    int i;
    int min_works_thread_index = 0;

    assert(thread_pool && thread_pool->num_threads > 0);

    /* to avoid race condition, we adapt the simplest min value algorithm instead of min-heap */
    for (i = 1; i < thread_pool->num_threads; i++) {
        if (thread_queue_len(&thread_pool->threads[i]) <
                thread_queue_len(&thread_pool->threads[min_works_thread_index])) {
            min_works_thread_index = i;
        }
    }

    return &thread_pool->threads[min_works_thread_index];
}

/*
 * collection for thread schedule alogrithm
 */
static const schedule_thread_func schedule_alogrithms[] = {
    [ROUND_ROBIN] = round_robin_schedule,
    [LEAST_LOAD]  = least_load_schedule
};

/*
 * check if every work queue is empty, 1 - empty, 0 - not empty 
 */
static int all_queue_empty(thread_pool_t *thread_pool) {
    int i;

    for (i = 0; i < thread_pool->num_threads; i++) {
        if (!thread_queue_empty(&thread_pool->threads[i])) {
            return 0;
        }
    }

    return 1;
}

/*
 * do nothing when signal send to here
 */
static void sig_do_nothing(int signo) {
    return;
}

/*
 * get work concurrently
 */
static work_t* get_work_concurrently(thread_t *thread) {
    work_t *work = NULL;
    size_t tmp;

    do {
        work = NULL;
        if (thread_queue_len(thread) <= 0) break;
        tmp = thread->out;
        // prefetch work
        work = &thread->work_queue[thread_queue_offset(tmp)];
    } while (!thread_out_bool(&thread->out, tmp, tmp + 1));  // 因为主线程和工作线程都会从工作队列中取任务

    return work;
}

/*
 * start address for thread function
 */
static void* thread_pool_thread(void *arg) {
    thread_t *thread = arg;
    work_t *work = NULL;
    sigset_t signal_mask, oldmask;
    int rc, sig_caught;

    /* SIGUSR1 handler has been set in thread_pool_init */
    __sync_fetch_and_add(&global_num_thread, 1);  // 原子add操作
    pthread_kill(main_tid, SIGUSR1);  // WTF?

    sigemptyset(&oldmask);
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGUSR1);

    while (1) {
        rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);  // WTF?
        if (rc != 0) {
            debug(thread_pool_ERROR, "SIG_BLOCK failed");
            pthread_exit(NULL);
        }

        while (thread_queue_empty(thread) && !thread->shutdown) {
            debug(thread_pool_DEBUG, "I'm sleeping");
            rc = sigwait(&signal_mask, &sig_caught);
            if (rc != 0) {
                debug(thread_pool_ERROR, "sigwait failed");
                pthread_exit(NULL);
            }
        }

        rc = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);  // WTF?
        if (rc != 0) {
            debug(thread_pool_ERROR, "SIG_SETMASK failed");
            pthread_exit(NULL);
        }

        debug(thread_pool_DEBUG, "I'm awake");

        if (thread->shutdown) {
            debug(thread_pool_DEBUG, "exit");
#ifdef DEBUG
            debug(thread_pool_INFO, "%ld: %d\n", thread->id, thread->num_works_done);
#endif
            pthread_exit(NULL);
        }

        work = get_work_concurrently(thread);
        if (work) {
            (*(work->routine))(work->arg);  // 线程执行任务
#ifdef DEBUG
            thread->num_works_done++;
#endif
        }

        if (thread_queue_empty(thread)) {
            pthread_kill(main_tid, SIGUSR1);  // WTF?
        }
    }
}

/*
 * spawn new thread for thread pool
 */
static void spawn_new_thread(thread_pool_t *thread_pool, int index) {
    memset(&thread_pool->threads[index], 0, sizeof(thread_t));

    if (pthread_create(&thread_pool->threads[index].id,
            NULL, 
            thread_pool_thread,
            (void *)(&thread_pool->threads[index])) != 0) {
        debug(thread_pool_ERROR, "pthread_create failed");
        exit(0);
    }
}

/*
 * wait for thread registration done
 */
static int wait_for_thread_registration(int num_expected) {
    sigset_t signal_mask, oldmask;
    int rc, sig_caught;

    sigemptyset(&oldmask);
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGUSR1);

    rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);  // WTF?
    if (rc != 0) {
        debug(thread_pool_ERROR, "SIG_BLOCK failed");
        return -1;
    }

    while (global_num_thread < num_expected) {
        rc = sigwait(&signal_mask, &sig_caught);
        if (rc != 0) {
            debug(thread_pool_ERROR, "sigwait failed");
            return -1;
        }
    }

    rc = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);  // WTF?
    if (rc != 0) {
        debug(thread_pool_ERROR, "SIG_SETMASK failed");
        return -1;
    }

    return 0;
}

/*
 * dispatch work to designated thread
 */
static int dispatch_work2thread(thread_pool_t *thread_pool,
                                thread_t *thread, 
                                void(*routine)(vn_http_event *), 
                                vn_http_event *arg) {
    work_t *work = NULL;

    if (thread_queue_full(thread)) {
        debug(thread_pool_WARN, "queue of thread selected is full!!!");
        return -1;
    }

    work = &thread->work_queue[thread_queue_offset(thread->in)];
    work->routine = routine;
    work->arg = arg;
    work->next = NULL;
    thread->in++;

    // 环形队列为空判断条件，详见算法与数据结构书籍
    if (thread_queue_len(thread) == 1) {
        debug(thread_pool_DEBUG, "signal has task");
        pthread_kill(thread->id, SIGUSR1);
    }

    return 0;
}

/*
 * get first thread whose work queue is not average level
 */
static int get_first_id(int arr[], int len, int (*fun)(int)) {
    int i;

    for (i = 0; i < len; i++) {
        if (fun(arr[i])) {
            return i;
        }
    }

    return -1;
}

/*
 * check if val is negtive
 */
static int isnegtive(int val) {
    return val < 0;
}

/*
 * check if val is positive
 */
static int ispositive(int val) {
    return val > 0;
}

/*
 * the load balance algorithm
 */
static void balance_thread_load(thread_pool_t *thread_pool) {
    int count[MAX_THREAD_NUM];
    int i, out, sum = 0, avg;
    int first_neg_id, first_pos_id, tmp, migrate_num;
    thread_t *from, *to;
    work_t *work;

    for (i = 0; i < thread_pool->num_threads; i++) {
        count[i] = thread_queue_len(&thread_pool->threads[i]);
        sum += count[i];
    }

    avg = sum / thread_pool->num_threads;
    if (avg == 0) {
        return;
    }

    for (i = 0; i < thread_pool->num_threads; i++) {
        count[i] -= avg;
    }

    while (1) {
        first_neg_id = get_first_id(count, thread_pool->num_threads, isnegtive);
        first_pos_id = get_first_id(count, thread_pool->num_threads, ispositive);

        if (first_neg_id < 0) {
            break;
        }

        tmp = count[first_neg_id] + count[first_pos_id];
        if (tmp > 0) {
            migrate_num = -count[first_neg_id];
            count[first_neg_id] = 0;
            count[first_pos_id] = tmp;
        } else {
            migrate_num = count[first_pos_id];
            count[first_pos_id] = 0;
            count[first_neg_id] = tmp;
        }

        from = &thread_pool->threads[first_pos_id];
        to = &thread_pool->threads[first_neg_id];

        for (i = 0; i < migrate_num; i++) {
            work = get_work_concurrently(from);
            if (work) {
                dispatch_work2thread(thread_pool, to, work->routine, work->arg);
            }
        }
    }

    from = &thread_pool->threads[first_pos_id];
    /* just migrate count[first_pos_id] - 1 works to other threads*/
    for (i = 1; i < count[first_pos_id]; i++) {
        to = &thread_pool->threads[i - 1];
        if (to == from) {
            continue;
        }
        work = get_work_concurrently(from);
        if (work) {
            dispatch_work2thread(thread_pool, to, work->routine, work->arg);
        }
    }
}

/*
 * migrate work to other threads when current thread is died 
 */
static int migrate_work(thread_pool_t *thread_pool, thread_t *from) {
    size_t i;
    work_t *work;
    thread_t *to;

    for (i = from->out; i < from->in; i++) {
        work = &from->work_queue[thread_queue_offset(i)];
        to = thread_pool->schedule_thread(thread_pool);
        if (dispatch_work2thread(thread_pool, to, work->routine, work->arg) < 0) {
            return -1;
        }
    }
#ifdef DEBUG
    printf("%ld migrate_work: %u\n", from->id, thread_queue_len(from));
#endif
    return 0;
}

void* thread_pool_init(int num_threads) {
    int i;
    thread_pool_t *thread_pool;

    if (num_threads <= 0) {
        debug(thread_pool_ERROR, "num_threads parameter shoud be positive!!!");
        return NULL;
    } 
    else if (num_threads > MAX_THREAD_NUM) {
        debug(thread_pool_ERROR, "num_threads parameter shoud be less than MAX_THREAD_NUM!!!");
        return NULL;
    }

    thread_pool = malloc(sizeof(*thread_pool));
    if (thread_pool == NULL) {
        debug(thread_pool_ERROR, "malloc failed");
        return NULL;
    }

    memset(thread_pool, 0, sizeof(*thread_pool));

    thread_pool->num_threads     = num_threads;
    thread_pool->schedule_thread = round_robin_schedule;

    /* all threads are set SIGUSR1 with sig_do_nothing */
    if (signal(SIGUSR1, sig_do_nothing) == SIG_ERR) {  // WTF?
        debug(thread_pool_ERROR, "signal failed");
        return NULL;
    }

    main_tid = pthread_self();  // 获取主线程ID

    for (i = 0; i < thread_pool->num_threads; i++) {
        spawn_new_thread(thread_pool, i);
    }
    
    if (wait_for_thread_registration(thread_pool->num_threads) < 0) {
        pthread_exit(NULL);
    }

    return (void *)thread_pool;
}

int thread_pool_inc_threads(void *pool, int num_inc) {
    thread_pool_t *thread_pool = pool;
    int i, num_threads;

    assert(thread_pool && num_inc > 0);

    num_threads = thread_pool->num_threads + num_inc;
    if (num_threads > MAX_THREAD_NUM) {
        debug(thread_pool_ERROR, "add too many threads!!!");
        return -1;
    }

    for (i = thread_pool->num_threads; i < num_threads; i++) {
        spawn_new_thread(thread_pool, i);
    }

    if (wait_for_thread_registration(num_threads) < 0) {
        pthread_exit(NULL);
    }
    
    thread_pool->num_threads = num_threads;
    balance_thread_load(thread_pool);

    return 0;
}

void thread_pool_dec_threads(void *pool, int num_dec) {
    thread_pool_t *thread_pool = pool;
    int i, num_threads;

    assert(thread_pool && num_dec > 0);

    if (num_dec > thread_pool->num_threads) {
        num_dec = thread_pool->num_threads;
    }

    num_threads = thread_pool->num_threads;
    thread_pool->num_threads -= num_dec;

    for (i = thread_pool->num_threads; i < num_threads; i++) {
        thread_pool->threads[i].shutdown = 1;
        pthread_kill(thread_pool->threads[i].id, SIGUSR1);  // WTF?
    }

    for (i = thread_pool->num_threads; i < num_threads; i++) {
        pthread_join(thread_pool->threads[i].id, NULL);  // WTF?
        /* migrate remaining work to other threads */
        if (migrate_work(thread_pool, &thread_pool->threads[i]) < 0) {
            debug(thread_pool_WARN, "work lost during migration!!!");
        }
    }

    if (thread_pool->num_threads == 0 && !all_queue_empty(thread_pool)) {
         debug(thread_pool_WARN, "No thread in pool with work unfinished!!!");
    }
}

int thread_pool_add_work(void *pool, void(*routine)(vn_http_event *), vn_http_event *arg) {
    thread_pool_t *thread_pool = pool;
    thread_t *thread;

    assert(thread_pool);

    thread = thread_pool->schedule_thread(thread_pool);

    return dispatch_work2thread(thread_pool, thread, routine, arg);
}

/*
 * @finish: 1, complete remaining works before return
 *          0, drop remaining works and return directly
 */
void thread_pool_destroy(void *pool, int finish) {
    thread_pool_t *thread_pool = pool;
    int i;

    assert(thread_pool);

    if (finish == 1) {
        sigset_t signal_mask, oldmask;
        int rc, sig_caught;

        debug(thread_pool_DEBUG, "wait all work done");

        sigemptyset(&oldmask);
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGUSR1);

        rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
        if (rc != 0) {
            debug(thread_pool_ERROR, "SIG_BLOCK failed");
            pthread_exit(NULL);
        }

        while (!all_queue_empty(thread_pool)) {
            rc = sigwait(&signal_mask, &sig_caught);
            if (rc != 0) {
                debug(thread_pool_ERROR, "sigwait failed");
                pthread_exit(NULL);
            }
        }

        rc = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
        if (rc != 0) {
            debug(thread_pool_ERROR, "SIG_SETMASK failed");
            pthread_exit(NULL);
        }
    }

    for (i = 0; i < thread_pool->num_threads; i++) {
        thread_pool->threads[i].shutdown = 1;
        /* wake up thread */
        pthread_kill(thread_pool->threads[i].id, SIGUSR1);
    }
    debug(thread_pool_DEBUG, "wait worker thread exit");

    for (i = 0; i < thread_pool->num_threads; i++) {
        pthread_join(thread_pool->threads[i].id, NULL);
    }

    free(thread_pool);
}

void set_thread_schedule_algorithm(void *pool, enum schedule_type type) {
    thread_pool_t *thread_pool = pool;

    assert(thread_pool);

    thread_pool->schedule_thread = schedule_alogrithms[type];
}

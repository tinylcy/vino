/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#include <stdlib.h>
#include <sys/time.h>

#include "vn_event_timer.h"
#include "vn_priority_queue.h"
#include "error.h"

extern vn_priority_queue pq;

int vn_event_timer_init(void) {
    vn_pq_init(&pq);
    vn_event_time_update();
    return 0;
}

void vn_event_time_update(void) {
    struct timeval tv;
    vn_sec_t sec;
    vn_msec_t msec;

    if (gettimeofday(&tv, NULL) < 0) {
        err_sys("[vn_time_update] gettimeofday error");
    }
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    vn_current_msec = sec * 1000 + msec;
}

vn_msec_t vn_event_find_timer(void) {
    long timer;
    vn_priority_queue_node *node;

    if (vn_pq_isempty(&pq)) {
        return 0;
    }

    node = vn_pq_min(&pq);
    timer = node->key - vn_current_msec;
    return timer > 0 ? timer : 0;
}

void vn_event_expire_timers(void) {
    vn_priority_queue_node *node;
    vn_http_event *event;

    while (!vn_pq_isempty(&pq)) {
        node = vn_pq_min(&pq);

        if (node->deleted == VN_PQ_DELETED) {
            vn_pq_delete_min(&pq);
            continue;
        }

        /* 
         * If node.key < vn_current_msec, timeout.
         * Be careful, the gap should be cast to Long type.
         */
        if ((long) (node->key - vn_current_msec) > 0) {
            return;
        }

        event = (vn_http_event *) node->data;
    
        if (event->handler) {
            (*event->handler)(event);
        }
        vn_pq_delete_min(&pq);
    }
}

void vn_event_add_timer(vn_http_event *event, vn_msec_t timer) {
    vn_priority_queue_node *node;
    vn_msec_t key;

    if ((node = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node))) == NULL) {
        err_sys("[vn_event_add_timer] malloc error");
    }

    vn_event_time_update();
    key = vn_current_msec + timer;
    node->key = key;
    node->data = (void *) event;
    node->deleted = VN_PQ_NOT_DELETED;
    vn_pq_insert(&pq, node);

    event->pq_node = node;
}

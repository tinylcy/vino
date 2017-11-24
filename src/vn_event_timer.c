/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#include <time.h>
#include "vn_event_timer.h"
#include "vn_priority_queue.h"

extern vn_priority_queue pq;

int vn_event_timer_init(void) {
    vn_pq_init(&pq);
    return 0;
}

void vn_time_update(void) {
    struct timeval tv;
    time_t sec;
    time_t msec;

    vn_gettimeofday(&tv);
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    vn_current_msec = sec * 1000 + msec;
}

time_t vn_event_find_timer(void) {
    time_t timer;
    vn_priority_queue_node node;

    if (vn_pq_isempty(&pq)) {
        return 0;
    }

    node = vn_pq_min(&pq);
    timer = node.key - vn_current_msec;
    return timer;
}

void vn_event_expire_timers(void) {
    vn_priority_queue_node node;
    vn_http_event *event;

    while (!vn_pq_isempty(&pq)) {
        node = vn_pq_min(&pq);
        if (node.key - vn_current_msec > 0) {
            return;
        }

        node = vn_pq_delete_min(&pq);
        event = node.data;
        (*event->handler)(event);
    }
}

void vn_event_add_timer(vn_http_event *event, time_t timer) {
    vn_priority_queue_node node;
    time_t key;

    key = vn_current_msec + timer;
    node.key = key;
    node.data = event;
    vn_pq_insert(&pq, node);
}

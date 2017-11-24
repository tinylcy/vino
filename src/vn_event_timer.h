/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_EVENT_TIMER_H
#define VINO_VN_EVENT_TIMER_H

#include <sys/time.h>
#include "vn_priority_queue.h"

/* Default timeout (ms) */
#define VN_DEFAULT_TIMEOUT 500

#define vn_gettimeofday(tp) (void) gettimeofday(tp, NULL);

volatile time_t vn_current_msec;

vn_priority_queue pq;

int vn_event_timer_init(void);

void vn_time_update(void);

time_t vn_event_find_timer(void);

void vn_event_expire_timers(void);

void vn_event_add_timer(vn_http_event *event, time_t timer);

#endif /* VINO_VN_EVENT_TIMER_H */
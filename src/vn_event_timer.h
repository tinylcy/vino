/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_EVENT_TIMER_H
#define VINO_VN_EVENT_TIMER_H

#include <sys/time.h>
#include "vn_request.h"
#include "vn_priority_queue.h"

#define VN_DEFAULT_TIMEOUT  500    /* ms */

typedef time_t        vn_sec_t;    /* Seconds */
typedef unsigned long vn_msec_t;   /* Milliseconds */

volatile vn_msec_t vn_current_msec;

vn_priority_queue pq;

int vn_event_timer_init(void);

void vn_event_time_update(void);

vn_msec_t vn_event_find_timer(void);

void vn_event_expire_timers(void);

void vn_event_add_timer(vn_http_connection *conn, vn_msec_t timer);

#endif /* VINO_VN_EVENT_TIMER_H */
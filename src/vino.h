/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include "vn_request.h"

/* HTTP request event */
typedef struct vn_http_event_s {
    int fd;
    int epfd;
    vn_http_request hr;
} vn_http_event;

void vn_init_http_event(vn_http_event *event, int fd, int epfd);

void vn_serve_http_event(vn_http_event *http_event);

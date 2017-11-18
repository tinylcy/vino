/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */

/* HTTP request event */
typedef struct vn_http_event_s {
    int fd;
    int epfd;
    vn_http_request hr;
} vn_http_event;

/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#ifndef VINO_VN_VINO_H
#define VINO_VN_VINO_H

#include "vn_request.h"

#define VN_BUFSIZE      8192
#define VN_HEADERS_SIZE 4096

/* HTTP request event */
typedef struct vn_http_event_s {
    int             fd;
    int             epfd;
    char            buf[VN_BUFSIZE];
    char            *bufptr;
    size_t          remain_size;
    vn_http_request hr;
} vn_http_event;

void vn_init_http_event(vn_http_event *event, int fd, int epfd);

/*
 * Check whether the HTTP request message is fully buffered.
 * If true, call the corresponding method to deal with.
 */ 
void vn_handle_http_event(vn_http_event *event);

/*
 * Handle HTTP GET request
 */
void vn_handle_get_event(vn_http_event *event);

void vn_send_resp_headers(vn_http_event *event, int code, unsigned long content_length);

void vn_send_resp_error(vn_http_event *event, int code, const char *reason);

void vn_send_resp(vn_http_event *event, int code, const char *body);

const char *vn_status_message(int code);

#endif /* VINO_VN_VINO_H */
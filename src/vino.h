/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_VINO_H
#define VINO_VN_VINO_H

#include "vn_request.h"

#define VN_BUFSIZE                   8192
#define VN_HEADERS_SIZE              4096
#define VN_BODY_SIZE                 8192
#define VN_FILETYPE_SIZE             50
#define VN_PARENT_DIR                "../"
#define VN_DEFAULT_STATIC_RES_DIR    "html"

typedef void (*timeout_handler)(void *);

/* HTTP request event */
typedef struct vn_http_event_s {
    int               fd;
    int               epfd;
    char              buf[VN_BUFSIZE];
    char              *bufptr;
    size_t            remain_size;
    vn_http_request   hr;
    timeout_handler   handler;
} vn_http_event;

/*
 * Initialize HTTP request event.
 */ 
void vn_init_http_event(vn_http_event *event, int fd, int epfd);

/*
 * Check whether the HTTP request message is fully buffered.
 * If true, call the corresponding method to deal with.
 */ 
void vn_handle_http_event(vn_http_event *event);

/*
 * Handle HTTP GET request.
 */
void vn_handle_get_event(vn_http_event *event);

void vn_close_http_event(void *event);

/* 
 * Create HTTP response header and store it into headers.
 */ 
void vn_build_resp_headers(char *headers, int code, const char *reason, const char *content_type, 
                            unsigned int content_length);

/* 
 * Create HTTP error response body and store it into body.
 */ 
void vn_build_resp_error_body(char *body);

void vn_send_resp(vn_http_event *event, const char *headers, const char *body);

const char *vn_status_message(int code);

#endif /* VINO_VN_VINO_H */
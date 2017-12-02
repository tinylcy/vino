/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_REQUEST_H
#define VINO_VN_REQUEST_H

#include <stdio.h>
#include "vn_priority_queue.h"
#include "vn_linked_list.h"
#include "util.h"

#define VN_BUFSIZE                8192
#define VN_MAX_HTTP_HEADERS       20
#define VN_MAX_HTTP_HEADER_NAME   50
#define VN_MAX_HTTP_HEADER_VALUE  200

/* HTTP request message */
typedef struct vn_http_request_s {

    unsigned int state;
    const char *pos;
    const char *last;

    /* HTTP request line */
    vn_str method;  /* "GET" */
    vn_str uri;     /* "/index.html" */
    vn_str proto;   /* "HTTP/1.1" */
 
    /*
     * Query string(part of URI), for example:
     *     GET /vino/index.html?param1=value1&param2=value2
     *         |      uri      |       query-string       |
     */
    vn_str         query_string;

    /* HTTP request headers */
    unsigned short header_cnt;
    const char     *header_name_start, *header_value_start;
    int            header_name_len, header_value_len;
    vn_linked_list header_name_list;
    vn_linked_list header_value_list;

    /* HTTP body */
    vn_str         body;
    size_t         body_len;     /* HTTP body length (is exists) */
} vn_http_request;

typedef void (*timeout_handler)(void *);

/* HTTP request event */
typedef struct vn_http_event_s {
    int                     fd;
    int                     epfd;
    char                    buf[VN_BUFSIZE];
    char                    *bufptr;
    size_t                  remain_size;
    vn_http_request         hr;
    timeout_handler         handler;
    vn_priority_queue_node  *pq_node;
} vn_http_event;

/*
 * Initialize HTTP request message.
 */
void vn_init_http_request(vn_http_request *hr);       

/*
 * Initialize HTTP request event.
 */ 
void vn_init_http_event(vn_http_event *event, int fd, int epfd);

/*
 * Reset HTTP request event. 
 */ 
void vn_reset_http_event(vn_http_event *event);

/*
 * Search and return the header `name` in parsed HTTP request
 * message `hr`.
 * 
 * If header `name` is not found, NULL is returned.
 */
vn_str *vn_get_http_header(vn_http_request *hr, const char *name);

/*
 * Close the connection, don't forget to cast the
 * type of `event` to (vn_http_event *).
 */
void vn_close_http_event(void *event);

#endif /* VINO_VN_REQUEST_H */
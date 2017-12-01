/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_REQUEST_H
#define VINO_VN_REQUEST_H

#include <stdio.h>
#include "vn_priority_queue.h"
#include "util.h"

#define VN_BUFSIZE                8192
#define VN_MAX_HTTP_HEADERS       20
#define VN_MAX_HTTP_HEADER_NAME   50
#define VN_MAX_HTTP_HEADER_VALUE  200

/* HTTP request message */
typedef struct vn_http_request_s {

    unsigned int state;
    const char *pos;

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
    vn_str         header_names[VN_MAX_HTTP_HEADERS];
    vn_str         header_values[VN_MAX_HTTP_HEADERS];

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
 * Fetch substring from `s` to `end` into `vs`.
 * 
 * Skip initial delimiters characters. Record the first non-delimiter
 * character at the beginning of `vs`. Then scan the rest of the string 
 * until a delimiter character or end-of-string is found.
 */
const char *vn_skip(const char *s, const char *end,
                        const char *delims, vn_str *vs);

/*
 * Initialize HTTP request message.
 */
void vn_init_http_request(vn_http_request *hr);       

/*
 * Search and return the header `name` in parsed HTTP request
 * message `hr`.
 * 
 * If header `name` is not found, NULL is returned.
 */
vn_str *vn_get_http_header(vn_http_request *hr, const char *name);

/*
 * Print HTTP request message.
 */
void vn_print_http_request(vn_http_request *hr);

#endif /* VINO_VN_REQUEST_H */
/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_REQUEST_H
#define VINO_VN_REQUEST_H

#include <stdio.h>
#include "vn_priority_queue.h"
#include "vn_linked_list.h"
#include "vn_palloc.h"
#include "util.h"

#define VN_BUFSIZE                8192
#define VN_MAX_HTTP_HEADERS       20
#define VN_MAX_HTTP_HEADER_NAME   50
#define VN_MAX_HTTP_HEADER_VALUE  200

/* HTTP request message */
typedef struct vn_http_request_s {

    /* HTTP parser state */
    unsigned int    state;
    const char     *pos;
    const char     *last;

    /* HTTP request line */
    vn_str method;  /* "GET" */
    vn_str uri;     /* "/index.html" */
    vn_str proto;   /* "HTTP/1.1" */
 
    /*
     * Query string(part of URI), for example:
     *     GET /vino/index.html?param1=value1&param2=value2
     *         |      uri      |       query-string       |
     */
    vn_str          query_string;

    /* HTTP request headers */
    unsigned short  header_cnt;
    const char     *header_name_start, *header_value_start;
    int             header_name_len, header_value_len;
    vn_linked_list  header_name_list;
    vn_linked_list  header_value_list;

    /* HTTP body */
    vn_str         body;
    size_t         body_len;     /* HTTP body length (is exists) */
} vn_http_request;

typedef void (*timeout_handler)(void *);

/* HTTP request connection */
typedef struct vn_http_connection_s {
    int                      fd;
    int                      epfd;
    vn_pool_t               *pool;

    /* HTTP request buffer */
    char                     req_buf[VN_BUFSIZE];
    char                    *req_buf_ptr;
    size_t                   remain_size;

    /* HTTP response buffer */
    char                    *resp_headers;            /* HTTP response headers (include response line) buffer (heap memory) */
    char                    *resp_headers_ptr;        /* Point to the next byte to be sent */
    size_t                   resp_headers_left;       /* Number of bytes left to be sent */
    char                    *resp_body;               /* HTTP response body buffer (mmap memory) */
    char                    *resp_body_ptr;           /* Point to the next byte to be sent */     
    size_t                   resp_body_left;          /* Number of bytes left to be sent */ 
    size_t                   resp_file_size;          /* The size of the mapped file */  
    int                      resp_mem_type;           /* The type of memory allocation, malloc or mmap */

    int                      keep_alive;

    vn_http_request          request;
    timeout_handler          handler;
    vn_priority_queue_node  *pq_node;
} vn_http_connection;

/*
 * Initialize HTTP request message.
 */
void vn_init_http_request(vn_http_request *req);       

/*
 * Initialize HTTP request connection.
 */ 
void vn_init_http_connection(vn_http_connection *conn, int fd, int epfd);

/*
 * Reset HTTP request connectin. 
 */ 
void vn_reset_http_connection(vn_http_connection *conn);

/*
 * Search and return the header `name` in parsed HTTP request
 * message `req`.
 * 
 * If header `name` is not found, NULL is returned.
 */
vn_str *vn_get_http_header(vn_http_request *req, const char *name);

/*
 * Close the connection, don't forget to cast the
 * type of `connection` to (vn_http_connection *).
 */
void vn_close_http_connection(void *connection);

#endif /* VINO_VN_REQUEST_H */
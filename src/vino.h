/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 * 
 * version 2017/12/01
 */
#ifndef VINO_VINO_H
#define VINO_VINO_H

#include "vn_request.h"

#define VINO_VERSION   "2.0"
#define VN_PORT        "8080"
#define VN_RUNNING     1
#define VN_ACCEPT      1

#define VN_HEADERS_SIZE             4096
#define VN_BODY_SIZE                8192
#define VN_FILETYPE_SIZE            50
#define VN_PARENT_DIR               "../"
#define VN_DEFAULT_STATIC_RES_DIR   "html"
#define VN_DEFAULT_PAGE             "/index.html"

#define VN_CONN_KEEP_ALIVE          1
#define VN_CONN_CLOSE               0 

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

/*
 * Handle HTTP HEAD request.
 */
void vn_handle_head_event(vn_http_event *event);

/*
 * Handle HTTP POST request.
 */
void vn_handle_post_event(vn_http_event *event);

/*
 * Handle HTTP PUT request.
 */
void vn_handle_put_event(vn_http_event *event);

/*
 * Handle HTTP DELETE request.
 */
void vn_handle_delete_event(vn_http_event *event);

/*
 * Handle HTTP CONNECT request.
 */
void vn_handle_connect_event(vn_http_event *event);

/*
 * Handle HTTP OPTIONS request.
 */
void vn_handle_options_event(vn_http_event *event);

/*
 * Handle HTTP TRACE request.
 */
void vn_handle_trace_event(vn_http_event *event);

/*
 * Close the connection, don't forget to cast the
 * type of `event` to (vn_http_event *).
 */
void vn_close_http_event(void *event);

/* 
 * Create HTTP response header and store it into `headers`.
 */ 
void vn_build_resp_headers(char *headers, 
                           int code, 
                           const char *reason, 
                           const char *content_type, 
                           size_t content_length,
                           short keep_alive);

/* 
 * Create HTTP Not Found response body and store it into `body`.
 */ 
void vn_build_resp_404_body(char *body, const char *uri);

const char *vn_status_message(int code);

#endif /* VINO_VINO_H */

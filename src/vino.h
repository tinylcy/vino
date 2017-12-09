/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_VINO_H
#define VINO_VN_VINO_H

#include <sys/epoll.h>
#include "vn_request.h"

#define VINO_VERSION      "2.0"
#define VN_PORT           "8080"
#define VN_RUNNING        1
#define VN_ACCEPT         1
#define VN_KEEP_READING   1
#define VN_KEEP_WRITING   1
#define VN_KEEP_PARSING   1

#define VN_HEADERS_SIZE              4096
#define VN_BODY_SIZE                 8192
#define VN_FILETYPE_SIZE             50
#define VN_PARENT_DIR                "../"
#define VN_DEFAULT_STATIC_RES_DIR    "html"
#define VN_DEFAULT_PAGE              "/index.html"

#define VN_CONN_KEEP_ALIVE           1
#define VN_CONN_CLOSE                0 

/*
 * Check the connection is currently readable or writeable.
 */ 
void vn_handle_http_connection(uint32_t events, vn_http_connection_t *conn);

/*
 * If the connection is currently readbale, read the HTTP request message
 * into connection buffer, and parse it. If the buffer is parsed successfully,
 * send the response to remote (call vn_handle_write_event).
 */
void vn_handle_read_event(vn_http_connection_t *conn);

/*
 * If the connection is currently writeable, send the buffered data
 * to the remote. 
 */
void vn_handle_write_event(vn_http_connection_t *conn);

/*
 * Handle HTTP GET request.
 */
void vn_handle_get_connection(vn_http_connection_t *conn);

/* 
 * Create HTTP response header and store it into `headers`.
 */ 
void vn_build_resp_headers(char *headers, 
                           int code, 
                           const char *reason, 
                           const char *content_type, 
                           unsigned int content_length, 
                           short keep_alive);

/* 
 * Create HTTP Not Found response body and store it into `body`.
 */ 
void vn_build_resp_404_body(char *body, const char *uri);

void vn_build_resp_403_body(char *body, const char *uri);

const char *vn_status_message(int code);

#endif /* VINO_VN_VINO_H */
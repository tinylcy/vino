/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include "vn_request.h"
#include "vn_linked_list.h"
#include "vn_priority_queue.h"
#include "vn_logger.h"
#include "util.h"
#include "vn_error.h"

void vn_init_http_request(vn_http_request *req) {
    /* Initialize HTTP parser (DFA) state */
    req->state = 0;
    req->pos = req->last = NULL;

    /* Initialize HTTP request line */
    req->method.p = req->uri.p = req->proto.p = NULL;
    req->method.len = req->uri.len = req->proto.len = 0;
    req->query_string.p = NULL;
    req->query_string.len = 0;

    /* Initialize HTTP request headers */
    req->header_cnt = 0;
    req->header_name_len = req->header_value_len = 0;
    req->header_name_start = req->header_value_start = NULL;
    vn_linked_list_init(&req->header_name_list);
    vn_linked_list_init(&req->header_value_list);
}

void vn_init_http_connection(vn_http_connection *conn, int fd, int epfd) {
    conn->fd = fd;
    conn->epfd = epfd;

    /* Initialize buffer */
    memset(conn->req_buf, '\0', VN_BUFSIZE);
    conn->req_buf_ptr = conn->req_buf;
    conn->remain_size = VN_BUFSIZE;

    vn_init_http_request(&conn->request);
    /* Initialize HTTP request parser state */
    conn->request.pos = conn->request.last = conn->req_buf;

    /* Initialize HTTP response buffer */
    conn->resp_headers = conn->resp_headers_ptr = NULL;
    conn->resp_headers_left = 0;
    conn->resp_body = conn->resp_body_ptr = NULL;
    conn->resp_body_left = conn->resp_file_size = 0;

    conn->handler = vn_close_http_connection;
    conn->pq_node = NULL;
}

void vn_reset_http_connection(vn_http_connection *conn) {
    /* Reset HTTP request buffer */
    memset(conn->req_buf, '\0', VN_BUFSIZE);
    conn->req_buf_ptr = conn->req_buf;
    conn->remain_size = VN_BUFSIZE;

    vn_init_http_request(&conn->request);
    /* Reset HTTP request parser state */
    conn->request.pos = conn->request.last = conn->req_buf;

    /* Reset HTTP response buffer */
    free(conn->resp_headers);
    conn->resp_headers = conn->resp_headers_ptr = NULL;
    conn->resp_headers_left = 0;
    conn->resp_body = conn->resp_body_ptr = NULL;
    conn->resp_body_left = conn->resp_file_size = 0;

    conn->handler = vn_close_http_connection; 

    /* Be careful: the `pq_node` shouldn't be reset to NULL */ 
}

vn_str *vn_get_http_header(vn_http_request *req, const char *name) {
    int i;
    vn_str *name_str, *value_str;
    vn_linked_list *name_list, *value_list;
    vn_linked_list_node *name_node, *value_node;

    name_list = &req->header_name_list;
    value_list = &req->header_value_list;

    if (vn_linked_list_isempty(name_list) && vn_linked_list_isempty(value_list)) {
        return NULL;
    }

    name_node = name_list->head;
    value_node = value_list->head;
    while (name_node && value_node) {
        name_str = (vn_str *) name_node->data;
        value_str = (vn_str *) value_node->data;
        if (!vn_str_cmp(name_str, name)) {
            return value_str;
        }
        name_node = name_node->next;
        value_node = value_node->next;
    }
    return NULL;
}

void vn_close_http_connection(void *connection) {
    vn_http_connection *conn;
    vn_http_request req;
    vn_priority_queue_node *pq_node;

    conn = (vn_http_connection *) connection;
    req = conn->request;
    pq_node = conn->pq_node;

    if (close(conn->fd) < 0) {
        err_sys("[vn_close_http_connection] close error");
    }

#ifdef DEBUG
    DEBUG_PRINT("The connection has been closed, [fd = %d]", conn->fd);
#endif

    vn_linked_list_free(&req.header_name_list);
    vn_linked_list_free(&req.header_value_list);
    if (!pq_node) { 
        vn_log_warn("The connection has no corresponding node in priority queue.");
    } else {
        pq_node->data = NULL;
    }

    free(conn->resp_headers);
    if (conn->resp_body && conn->resp_file_size > 0) {
        if (munmap(conn->resp_body, conn->resp_file_size) < 0) {
            err_sys("[vn_close_http_connection] munmap error");
        }
    }
    free(conn);

}
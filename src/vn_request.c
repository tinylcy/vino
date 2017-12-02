/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "vn_request.h"
#include "vn_linked_list.h"
#include "util.h"
#include "vn_error.h"

void vn_init_http_request(vn_http_request *hr) {
    /* Initialize HTTP parser (DFA) state */
    hr->state = 0;
    hr->pos = hr->last = NULL;

    /* Initialize HTTP request line */
    hr->method.p = hr->uri.p = hr->proto.p = NULL;
    hr->method.len = hr->uri.len = hr->proto.len = 0;
    hr->query_string.p = NULL;
    hr->query_string.len = 0;

    /* Initialize HTTP request headers */
    hr->header_cnt = 0;
    hr->header_name_len = hr->header_value_len = 0;
    hr->header_name_start = hr->header_value_start = NULL;
    vn_linked_list_init(&hr->header_name_list);
    vn_linked_list_init(&hr->header_value_list);
}

void vn_init_http_event(vn_http_event *event, int fd, int epfd) {
    event->fd = fd;
    event->epfd = epfd;

    /* Initialize buffer */
    memset(event->buf, '\0', VN_BUFSIZE);
    event->bufptr = event->buf;
    event->remain_size = VN_BUFSIZE;

    vn_init_http_request(&event->hr);
    /* Initialize HTTP request parser state */
    event->hr.pos = event->hr.last = event->buf;

    event->handler = vn_close_http_event;
    event->pq_node = NULL;
}

void vn_reset_http_event(vn_http_event *event) {
    /* Reset buffer */
    memset(event->buf, '\0', VN_BUFSIZE);
    event->bufptr = event->buf;
    event->remain_size = VN_BUFSIZE;

    vn_init_http_request(&event->hr);
    /* Reset HTTP request parser state */
    event->hr.pos = event->hr.last = event->buf;

    event->handler = vn_close_http_event;  
}

vn_str *vn_get_http_header(vn_http_request *hr, const char *name) {
    int i;
    vn_str *name_str, *value_str;
    vn_linked_list *name_list, *value_list;
    vn_linked_list_node *name_node, *value_node;

    name_list = &hr->header_name_list;
    value_list = &hr->header_value_list;

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

void vn_close_http_event(void *event) {
    vn_http_event *ev = (vn_http_event *) event;
    vn_http_request hr = ev->hr;

    if (close(ev->fd) < 0) {
        err_sys("[vn_close_http_event] close error");
    }
    vn_linked_list_free(&hr.header_name_list);
    vn_linked_list_free(&hr.header_value_list);
    free(ev);
}
/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vn_request.h"
#include "util.h"
#include "error.h"

const char *vn_skip(const char *s, const char *end, const char *delims,
                         vn_str *vs) {
    vs->p = s;
    while (s < end && strchr(delims, *s) == NULL) { s++; }
    vs->len = s - vs->p;
    while (s < end && strchr(delims, *s) != NULL) { s++; } 
    return s;                    
}

void vn_init_http_request(vn_http_request *hr) {
    hr->method.p = hr->uri.p = hr->proto.p = NULL;
    hr->method.len = hr->uri.len = hr->proto.len = 0;
    hr->query_string.p = NULL;
    hr->query_string.len = 0;
    hr->header_cnt = 0;
}

int vn_http_line_headers_buffer(vn_http_event *event, size_t buf_len) {
    vn_http_request *hr;
    char *buf;
    int i;

    hr = &event->hr;
    buf = event->buf;
    if (buf == NULL || strlen(buf) == 0 || buf_len == 0) {
        return VN_AGAIN;
    }

    for (i = 0; i < buf_len && buf[i] != '\0'; i++) {
        if (!isprint(buf[i]) && buf[i] != CR && buf[i] != LF) {
            return VN_MALFORMED;
        } else if (buf[i] == LF && i + 2 < buf_len && buf[i + 1] == CR && buf[i + 2] == LF) {
            hr->body.p = buf + i + 3;
            return VN_COMPLETED;
        }
    }
    return VN_AGAIN;
}

int vn_http_body_buffer(vn_http_event *event, size_t buf_len) {
    const char *buf;
    vn_str *body_len_str;
    int i, body_len_int;
    vn_http_request *hr;

    hr = &event->hr;

    body_len_str = vn_get_http_header(hr, "Content-Length");
    if (!vn_str_cmp(&hr->method, "get") || NULL == body_len_str) {
        return VN_COMPLETED;
    }

    body_len_int = vn_str_atoi(body_len_str);

    buf = hr->body.p;
    for (i = 0; i < body_len_int && i < buf_len && buf[i] != '\0'; i++) {
        if (!isprint(buf[i]) && buf[i] != CR && buf[i] != LF) {
            return VN_MALFORMED;
        }
    }
    if (i < body_len_int) {
        return VN_AGAIN;
    }
    hr->body.len = hr->body_len = body_len_int;
    return VN_COMPLETED;

}

int vn_parse_http_line_headers(const char *buf, int buf_len, vn_http_request *hr) {
    int len, rv;
    const char *s = buf, *end = buf + buf_len;
    const char *qs;  /* Point to the start of query string  */

    s = vn_skip(s, end, " ", &hr->method);
    s = vn_skip(s, end, " ", &hr->uri);
    s = vn_skip(s, end, "\r\n", &hr->proto);

    /* Check if uri contains qurey parameters */
    if ((qs = memchr(hr->uri.p, '?', hr->uri.len)) != NULL) {
        hr->query_string.p = qs + 1;
        hr->query_string.len = hr->uri.len - (qs - hr->uri.p) - 1;
        hr->uri.len = qs - hr->uri.p;
    }

    rv = vn_parse_http_headers(s, hr->body.p - s, hr);
    if (rv == -1) {
        err_sys("[vn_parse_http_line_headers] vn_parse_http_headers error");
    }
    
    return 0;
}

int vn_parse_http_headers(const char *buf, int buf_len, vn_http_request *hr) {
    int i = 0;
    const char *s = buf, *end = buf + buf_len;
    while (s < end) {
        vn_str *name = &hr->header_names[i];
        vn_str *value = &hr->header_values[i];
        s = vn_skip(s, end, " :", name);
        s = vn_skip(s, end, "\r\n", value);
        
        if (name->len > 0 && value->len == 0) {
            continue;
        }

        if (name->len > 0 && value -> len == 0) {
            name->p = value->p = NULL;
            name->len = value->len = 0;
        }

        i++;
    }

    hr->header_cnt = i;

    if (s < end) {
        return -1;
    }

    return 0;
}

int vn_parse_http_body(const char *buf, int buf_len, vn_http_request *hr) {
    // TODO
    return 0;
}

vn_str *vn_get_http_header(vn_http_request *hr, const char *name) {
    int i;
    for (i = 0; i < hr->header_cnt; i++) {
        if (!vn_str_cmp(&hr->header_names[i], name)) {
            return &hr->header_values[i];
        }
    }
    return NULL;
}

void vn_print_http_request(vn_http_request *hr) {
    int i;
    char name[VN_MAX_HTTP_HEADER_NAME], value[VN_MAX_HTTP_HEADER_VALUE];

    if (vn_get_string(&hr->method, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [method] error");
    }
    printf("\nMethod: %s\n", value);

    if (vn_get_string(&hr->uri, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [uri] error");
    }
    printf("URI: %s\n", value);

    if (vn_get_string(&hr->proto, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [proto] error");
    }
    printf("Proto: %s\n", value);

    if (vn_get_string(&hr->query_string, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [query_string] error");
    }
    printf("Query-String: %s\n", value);

    for (i = 0; i < hr->header_cnt; i++) {
        if (vn_get_string(&hr->header_names[i], name, VN_MAX_HTTP_HEADER_NAME) < 0 ||
            vn_get_string(&hr->header_values[i], value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
            err_sys("[print_http_request] vn_get_string [header] error");
        }
        printf("| %s | %s |\n", name, value);
    }

    if (vn_get_string(&hr->body, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [body] error");
    }
    printf("\nBody:\n%s", value);

}
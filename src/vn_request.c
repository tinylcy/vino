/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vn_request.h"
#include "error.h"

#define CR '\r'
#define LF '\n'

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

int vn_http_get_request_len(const char *buf, size_t buf_len) {
    int i;
    if (buf == NULL || strlen(buf) == 0 || buf_len == 0) {
        return 0;
    }
    for (i = 0; i < buf_len; i++) {
        if (!isprint(buf[i]) && buf[i] != CR && buf[i] != LF) {
            return -1;
        } else if (buf[i] == LF && i + 2 < buf_len && buf[i + 1] == CR && buf[i + 2] == LF) {
            return i + 3;
        }
    }

    return 0;
}

int vn_parse_http_request(const char *buf, int buf_len, vn_http_request *hr) {
    int len, rv;
    const char *s = buf, *end = buf + buf_len;
    const char *qs;  /* Point to the start of query string  */
    
    len = vn_http_get_request_len(buf, buf_len);
    if (len == 0) {               /* HTTP request message is buffered incompletely */
        return 0;
    } else if (len < 0) {         /* HTTP request message is wrong format */
        return -1;
    }

    s = vn_skip(s, end, " ", &hr->method);
    s = vn_skip(s, end, " ", &hr->uri);
    s = vn_skip(s, end, "\r\n", &hr->proto);

    /* Check if uri contains qurey parameters */
    if ((qs = memchr(hr->uri.p, '?', hr->uri.len)) != NULL) {
        hr->query_string.p = qs + 1;
        hr->query_string.len = hr->uri.len - (qs - hr->uri.p) - 1;
        hr->uri.len = qs - hr->uri.p;
    }

    rv = vn_parse_http_headers(s, end - s, hr);
    if (rv < 0) {
        return -1;
    } else {
        return buf_len;
    }
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
    } else if (s == end) {
        return buf_len;
    }
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

}
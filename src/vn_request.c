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
    hr->state = 0;
    hr->method.p = hr->uri.p = hr->proto.p = NULL;
    hr->method.len = hr->uri.len = hr->proto.len = 0;
    hr->query_string.p = NULL;
    hr->query_string.len = 0;
    hr->header_cnt = 0;
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

    // if (hr->body.len > 0 && vn_get_string(&hr->body, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
    //     err_sys("[print_http_request] vn_get_string [body] error");
    // }
    // printf("\nBody:\n%s", value);

}
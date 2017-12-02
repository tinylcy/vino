/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdlib.h>
#include "vn_http_parse.h"
#include "vn_request.h"
#include "vn_linked_list.h"
#include "util.h"
#include "vn_error.h"

#define vn_str3_cmp(m, c0, c1, c2, c3)                                \
    *((unsigned int *) m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define vn_str4_cmp(m, c0, c1, c2, c3)                                \
    *((unsigned int *) m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

int vn_http_parse_request_line(vn_http_request *hr, const char *buf) {
    const char *p, *m;
    char ch;

    enum {
        sw_start = 0,
        sw_method,
        sw_space_before_uri,
        sw_after_slash_in_uri,
        sw_uri,
        sw_question_mark_before_query_string,
        sw_query_string,
        sw_space_before_version,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_slash_before_version,
        sw_version_major_digit,
        sw_version_dot,
        sw_version_minor_digit,
        sw_almost_done,
        sw_done
    } state;

    state = hr->state;

    for (p = hr->pos; p < hr->last; p++, hr->pos++) {
        ch = *p;

        switch (state) {

        /* HTTP method: GET, POST */
        case sw_start:
            if (ch == CR || ch == LF) {
                break;
            }
            if (ch < 'A' || ch > 'Z') {
                return VN_HTTP_PARSE_INVALID_METHOD;
            }
            hr->method.p = p;
            state = sw_method;
            break;

        case sw_method:
            if (ch == ' ') {
                hr->method.len = p - hr->method.p;
                m = hr->method.p;

                switch (p - m) {
                case 3:
                    if (!vn_str3_cmp(m, 'G', 'E', 'T', ' ')) {
                        return VN_HTTP_PARSE_INVALID_METHOD;
                    }
                case 4:
                    if (!vn_str4_cmp(m, 'P', 'O', 'S', 'T')) {
                        return VN_HTTP_PARSE_INVALID_METHOD;
                    }
                }
                
                state = sw_space_before_uri;
                break;
            }

            if (ch < 'A' || ch > 'Z') {
                return VN_HTTP_PARSE_INVALID_METHOD;
            }

            state = sw_method;
            break;

        case sw_space_before_uri:
            if (ch == '/') {
                hr->uri.p = p;
                state = sw_after_slash_in_uri;
                break;
            }
            return VN_HTTP_PARSE_INVALID_REQUEST;

        case sw_after_slash_in_uri:
            switch (ch) {
            case ' ':
                hr->uri.len = p - hr->uri.p;
                state = sw_http_H;
                break;
            case '?':
                hr->uri.len = p - hr->uri.p;
                state = sw_question_mark_before_query_string;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_URI;
            default:
                state = sw_uri;
                break;
            }
            break;

        case sw_uri:
            switch (ch) {
            case ' ':
                hr->uri.len = p - hr->uri.p;
                state = sw_space_before_version;
                break;
            case '?':
                hr->uri.len = p - hr->uri.p;
                state = sw_question_mark_before_query_string;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_URI;
            default:
                state = sw_uri;
                break;
            }
            break;
        
        case sw_question_mark_before_query_string:
            switch (ch) {
            case ' ':
                hr->query_string.p = NULL;
                hr->query_string.len = 0;
                state = sw_space_before_version;
                break;
            default:
                hr->query_string.p = p;
                state = sw_query_string;
                break;
            }
            break;
        
        case sw_query_string:
            switch (ch) {
            case ' ':
                hr->query_string.len = p - hr->query_string.p;
                state = sw_space_before_version;
                break;
            default:
                state = sw_query_string;
                break;
            }
            break;

        case sw_space_before_version:
            if (ch != 'H') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            hr->proto.p = p;
            state = sw_http_H;
            break;
        
        case sw_http_H:
            if (ch != 'T') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_http_HT;
            break;
        
        case sw_http_HT:
            if (ch != 'T') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_http_HTT;
            break;

        case sw_http_HTT:
            if (ch != 'P') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_http_HTTP;
            break;

        case sw_http_HTTP:
            if (ch != '/') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_slash_before_version;
            break;

        case sw_slash_before_version:
            if (ch != '1') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_version_major_digit;
            break;

        case sw_version_major_digit:
            if (ch != '.') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_version_dot;
            break;

        case sw_version_dot:
            if (!(ch == '0' || ch == '1')) {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_version_minor_digit;
            break;
        
        case sw_version_minor_digit:
            if (ch != '\r') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            state = sw_almost_done;
            hr->proto.len = p - hr->proto.p;
            break;
        
        case sw_almost_done:
            if (ch != '\n') {
                return VN_HTTP_PARSE_INVALID_VERSION;
            }
            hr->pos++;
            hr->state = sw_start;
            return VN_OK;
        }
            
    }

    return VN_AGAIN;
}

int vn_http_parse_header_line(vn_http_request *hr, const char *buf) {
    const char *p;
    char ch;
    vn_str *name_str, *value_str;

    enum {
        sw_start = 0,
        sw_name,
        sw_colon_after_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_almost_done,
        sw_headers_almost_done
    } state;

    state = sw_start;

    for (p = hr->pos; p < hr->last; p++, hr->pos++) {
        ch = *p;

        switch (state) {

        case sw_start:
            switch (ch) {
            case CR:
                state = sw_headers_almost_done;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_HEADER;
            default:
                hr->header_name_start = p;
                state = sw_name;
                break;
            }
            break;

        case sw_name:
            switch (ch) {
            case ':':
                hr->header_name_len = p - hr->header_name_start;
                state = sw_colon_after_name;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_HEADER;
            case CR:
                state = sw_almost_done;
                break;
            default:
                state = sw_name;
                break;
            }
            break;
        
        case sw_colon_after_name:
            if (ch != ' ') {
                return VN_HTTP_PARSE_INVALID_HEADER;
            }
            state = sw_space_before_value;
            break;
        
        case sw_space_before_value:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_HEADER;
            default:
                hr->header_value_start = p;
                state = sw_value;
                break;
            }
            break;

        case sw_value:
            switch (ch) {
            case CR:
                hr->header_value_len = p - hr->header_value_start;
                state = sw_almost_done;
                break;
            case ' ':
                hr->header_value_len = p - hr->header_value_start;
                state = sw_space_after_value;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_HEADER;
            default:
                state = sw_value;
                break;
            }
            break;

        case sw_space_after_value:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case '\0':
                return VN_HTTP_PARSE_INVALID_HEADER;
            default:
                state = sw_value;
                break;
            }
            break;
        
        case sw_almost_done:
            switch (ch) {
            case LF:
                hr->header_cnt += 1;

                if ((name_str = (vn_str *) malloc(sizeof(vn_str))) == NULL ||
                    (value_str = (vn_str *) malloc(sizeof(vn_str))) == NULL) {
                    err_sys("[vn_http_parse_header_line] malloc error");
                }
                name_str->p = hr->header_name_start;
                name_str->len = hr->header_name_len;
                value_str->p = hr->header_value_start;
                value_str->len = hr->header_value_len;
                
                vn_linked_list_append(&hr->header_name_list, name_str);
                vn_linked_list_append(&hr->header_value_list, value_str);

                hr->header_name_start = hr->header_value_start = NULL;
                hr->header_name_len = hr->header_value_len = 0;

                hr->state = sw_start;
                hr->pos += 1;
                
                return VN_HTTP_PARSE_HEADER_DONE;
            default:
                return VN_HTTP_PARSE_INVALID_HEADER;
            }
            break;

        case sw_headers_almost_done:
            if (ch != LF) {
                return VN_HTTP_PARSE_INVALID_HEADER;
            }
            hr->state = sw_start;
            hr->pos += 1;
            return VN_OK;
        }
    }

    return VN_AGAIN;
}
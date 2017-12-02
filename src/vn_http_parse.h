/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_HTTP_PARSE_H
#define VINO_VN_HTTP_PARSE_H

#include "vn_request.h"

#define CR '\r'
#define LF '\n'

#define VN_OK                            0
#define VN_AGAIN                         1

#define VN_HTTP_PARSE_INVALID_METHOD    -1
#define VN_HTTP_PARSE_INVALID_REQUEST   -2
#define VN_HTTP_PARSE_INVALID_URI       -3
#define VN_HTTP_PARSE_INVALID_VERSION   -4

#define VN_HTTP_PARSE_HEADER_DONE        2
#define VN_HTTP_PARSE_INVALID_HEADER    -5

int vn_http_parse_request_line(vn_http_request *hr, const char *buf);

int vn_http_parse_header_line(vn_http_request *hr, const char *buf);

#endif /* VINO_VN_HTTP_PARSE_H */
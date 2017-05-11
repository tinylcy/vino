/*
 * Copyright(C) Chenyang Li
 * Copyright(C) tinyhttpd
 */

#ifndef REQ_MAX_BUF
#define REQ_MAX_BUF 8192
#define HTTP_MAX_BUF 8192
#define HTTP_PARSE_OK 0
#define HTTP_PARSE_AGAIN 1
#define CR '\r'
#define LR '\n'
#define CRLF '\r\n'
#define CRLFCRLF '\r\n\r\n'

#include <stdio.h>
#include <stdlib.h>

typedef struct http_request_s {
	int fd;
	int epfd;
	char buf[REQ_MAX_BUF];
	size_t pos;
	size_t last;

	int status;
	char *method_start;
	char *method_end;
	char *uri_start;
	char *uri_end;
	char *query_start;
	char *query_end;
	char *version_start;
	char *version_end;

} http_request_t;

void http_request_init(http_request_t *request, int fd, int epfd);

int http_request_parse(http_request_t *request);

char *http_request_get_method(http_request_t *request);

char *http_request_get_uri(http_request_t *request);

char *http_request_get_query(http_request_t *request);

char *http_request_get_version(http_request_t *request);

#endif

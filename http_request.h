/*
 * Copyright(C) Chenyang Li
 * Copyright(C) tinyhttpd
 */

#ifndef REQ_MAX_BUF
#define REQ_MAX_BUF 8192

#include <stdio.h>
#include <stdlib.h>

typedef struct http_request_s {
	int fd;
	int epfd;
	char buf[REQ_MAX_BUF];
	size_t pos;
	size_t last;
} http_request_t;

void http_request_init(http_request_t *request, int fd, int epfd);

#endif

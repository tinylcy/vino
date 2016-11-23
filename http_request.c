/*
 * Copyright(C) Chenyang Li
 * Copyright(C) tinyhttpd
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_request.h"

void http_request_init(http_request_t *request, int fd, int epfd) {
	request->fd = fd;
	request->epfd = epfd;
	request->pos = 0;
	request->last = 0;
}

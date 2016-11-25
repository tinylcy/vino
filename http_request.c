 /*
  * Copyright(C) Chenyang Li
  * Copyright(C) tinyhttpd
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_request.h"
#include "util.h"

void http_request_init(http_request_t *request, int fd, int epfd) {
	memset(request, 0, sizeof(http_request_t));
	request->fd = fd;
    request->epfd = epfd;
    request->pos = 0;
	request->last = 0;
	request->status = 0;
}

int http_request_parse(http_request_t *request) {
	
	enum status {
		st_method = 0,
		st_uri,
		st_version,
		st_done
	};

	char *bufp;
	char *before_version;

	enum status s = request->status;
	
	for(bufp = &request->buf[request->pos]; bufp < &request->buf[request->last]; bufp++) {
		switch(s) {
			
			case st_method:
				if(bufp == &request->buf[request->pos] && (*bufp == 'G' || *bufp == 'P')) {
					request->method_start = bufp;
				}
				if(*bufp == ' ') {
					request->method_end = bufp - 1;
					s = st_uri;
				}
				break;

			case st_uri:
				if(*bufp == ' ' && bufp == request->method_end + 1) {
					continue;
				}
				if(*bufp == ' ') {
					request->uri_end = bufp - 1;
					char *pivot = NULL;
					pivot = contains(request->uri_start, request->uri_end, '?');
					if(pivot != NULL) {
						request->query_start = pivot + 1;
						request->query_end = request->uri_end;
						request->uri_end = pivot - 1;
					} else {
						request->query_start = NULL;
						request->query_end = NULL;
					}
					s = st_version;
				}
				if(*bufp == '/' && bufp == request->method_end + 2) {
					request->uri_start = bufp;
				}
				break;

			case st_version:
				before_version = (request->query_end == NULL ? request->uri_end : request->query_end);
				if(*bufp == ' ' && bufp == before_version + 1) {
					continue;
				}
				if(*bufp == 'H' && bufp == before_version + 2) {
					request->version_start = bufp;
				}
				if(*bufp == '\r' || *bufp == '\n') {
					request->version_end = bufp - 1;
					s = st_done;
				}
				break;

			case st_done:
				break;
				

		}
	}

	if(s != st_done) {
		return HTTP_PARSE_AGAIN; 
	}

	return HTTP_PARSE_OK;

}

char *http_request_get_method(http_request_t *request) {
	int size = request->method_end - request->method_start + 1;
	char *method = (char *)malloc(sizeof(char) * size);
	strncpy(method, request->method_start, size);
	return method;
}

char *http_request_get_uri(http_request_t *request) {
	int size = request->uri_end - request->uri_start + 1;
	char *uri = (char *)malloc(sizeof(char) * size);
	strncpy(uri, request->uri_start, size);
	return uri;
}

char *http_request_get_query(http_request_t *request) {
	if(request->query_start == NULL || request->query_end == NULL) {
		return NULL;
	}
	int size = request->query_end - request->query_start + 1;
	char *query = (char *)malloc(sizeof(char) * size);
	strncpy(query, request->query_start, size);
	return query;
}

char *http_request_get_version(http_request_t *request) {
	int size = request->version_end - request->version_start + 1;
	char *version = (char *)malloc(sizeof(char) * size);
	strncpy(version, request->version_start, size);
	return version;
}

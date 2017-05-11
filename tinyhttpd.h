/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#define DYNAMIC_PATH "cgi-bin"
#define PORT "PORT"
#define THREAD_NUM "THREAD_NUM"
#define JOB_MAX_NUM "JOB_MAX_NUM"

#include "http_headers_parser.h"
#include "http_request.h"

/* store the configuration params */
struct httpd_conf {
	int port;
	int thread_num;
	int job_max_num;
};

int make_server_socket_q(int, int);
int make_server_socket(int);
int connect_to_server(char*, int);

int make_socket_non_blocking(int);

void init_conf(struct httpd_conf*);
void process_request(void *req_ptr);
void http_response(http_request_t *request);
void header(FILE*, char*);

char *file_type(char*);

void not_implement(int);
void do_404(char*, int);
void do_ls(char*, int);

void serve_static(char *, int);

void serve_dynamic(http_request_headers_t*, int);
void serve_get_dynamic(http_request_headers_t*, int);
void serve_post_dynamic(http_request_headers_t*, int);

void sanitize(char*);
int is_dynamic(const char*);
